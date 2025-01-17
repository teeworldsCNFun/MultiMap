/* SQL class by Sushi */
#include "../gamecontext.h"

#include <engine/shared/config.h>

static LOCK sql_lock = 0;
class CGameContext *m_pGameServer;
CGameContext *GameServer() { return m_pGameServer; }

CSQL::CSQL(class CGameContext *pGameServer)
{
	if(sql_lock == 0)
		sql_lock = lock_create();

	m_pGameServer = pGameServer;
		
	// set database info
	database = g_Config.m_SvSqlDatabase;
	prefix = g_Config.m_SvSqlPrefix;
	user = g_Config.m_SvSqlUser;
	pass = g_Config.m_SvSqlPw;
	ip = g_Config.m_SvSqlIp;
	port = g_Config.m_SvSqlPort;
}

bool CSQL::connect()
{
	try 
	{
		// Create connection
		driver = get_driver_instance();
		char buf[256];
		str_format(buf, sizeof(buf), "tcp://%s:%d", ip, port);
		connection = driver->connect(buf, user, pass);
		
		// Create Statement
		statement = connection->createStatement();
		
		// Create database if not exists
		str_format(buf, sizeof(buf), "CREATE DATABASE IF NOT EXISTS %s", database);
		statement->execute(buf);
		
		// Connect to specific database
		connection->setSchema(database);
		dbg_msg("SQL", "SQL connection established");
		return true;
	} 
	catch (sql::SQLException &e)
	{
		dbg_msg("SQL", "ERROR: SQL connection failed");
		return false;
	}
}

void CSQL::disconnect()
{
	try
	{
		delete connection;
		dbg_msg("SQL", "SQL connection disconnected");
	}
	catch (sql::SQLException &e)
	{
		dbg_msg("SQL", "ERROR: No SQL connection");
	}
}

// create tables... should be done only once
void CSQL::create_tables()
{
	// create connection
	if(connect())
	{
		try
		{
			// create tables
			char buf[2048];
			str_format(buf, sizeof(buf), 
			"CREATE TABLE IF NOT EXISTS %s_Account "
			"(UserID INT AUTO_INCREMENT PRIMARY KEY, "
			"Username VARCHAR(31) NOT NULL, "
			"Password VARCHAR(32) NOT NULL, "
			"Exp BIGINT DEFAULT 0, "
			"Level BIGINT DEFAULT 0, "
			"Money BIGINT DEFAULT 0);", prefix);
			statement->execute(buf);
			dbg_msg("SQL", "Tables were created successfully");

			// delete statement
			delete statement;
		}
		catch (sql::SQLException &e)
		{
			dbg_msg("SQL", "ERROR: Tables were NOT created");
		}
		
		// disconnect from database
		disconnect();
	}	
}

// create Account
static void create_account_thread(void *user)
{
	lock_wait(sql_lock);
	
	CSqlData *Data = (CSqlData *)user;
	
	if(GameServer()->m_apPlayers[Data->m_ClientID])
	{
		// Connect to database
		if(Data->m_SqlData->connect())
		{
			try
			{
				// check if allready exists
				char buf[512];
				str_format(buf, sizeof(buf), "SELECT * FROM %s_Account WHERE Username='%s';", Data->m_SqlData->prefix, Data->name);
				Data->m_SqlData->results = Data->m_SqlData->statement->executeQuery(buf);
				if(Data->m_SqlData->results->next())
				{
					// Account found
					dbg_msg("SQL", "Account '%s' allready exists", Data->name);
					
					GameServer()->SendChatTarget(Data->m_ClientID, "This acoount allready exists!");
				}
				else
				{
					// create Account \o/
					str_format(buf, sizeof(buf), "INSERT INTO %s_Account(Username, Password) VALUES ('%s', '%s');", 
					Data->m_SqlData->prefix, 
					Data->name, Data->pass);
					
					Data->m_SqlData->statement->execute(buf);
					dbg_msg("SQL", "Account '%s' was successfully created", Data->name);
					
					GameServer()->SendChatTarget(Data->m_ClientID, "Acoount was created successfully.");
					GameServer()->SendChatTarget(Data->m_ClientID, "You may login now. (/login <user> <pass>)");
				}
				
				// delete statement
				delete Data->m_SqlData->statement;
				delete Data->m_SqlData->results;
			}
			catch (sql::SQLException &e)
			{
				dbg_msg("SQL", "ERROR: Could not create Account");
			}
			
			// disconnect from database
			Data->m_SqlData->disconnect();
		}
	}
	
	delete Data;
	
	lock_release(sql_lock);
}

void CSQL::create_account(const char* name, const char* pass, int m_ClientID)
{
	CSqlData *tmp = new CSqlData();
	str_copy(tmp->name, name, sizeof(tmp->name));
	str_copy(tmp->pass, pass, sizeof(tmp->pass));
	tmp->m_ClientID = m_ClientID;
	tmp->m_SqlData = this;
	
	void *register_thread = thread_init(create_account_thread, tmp);
#if defined(CONF_FAMILY_UNIX)
	pthread_detach((pthread_t)register_thread);
#endif
}

// change password
static void change_password_thread(void *user)
{
	lock_wait(sql_lock);
	
	CSqlData *Data = (CSqlData *)user;
	
	// Connect to database
	if(Data->m_SqlData->connect())
	{
		try
		{
			// Connect to database
			Data->m_SqlData->connect();
			
			// check if Account exists
			char buf[512];
			str_format(buf, sizeof(buf), "SELECT * FROM %s_Account WHERE UserID='%d';", Data->m_SqlData->prefix, Data->Account_id[Data->m_ClientID]);
			Data->m_SqlData->results = Data->m_SqlData->statement->executeQuery(buf);
			if(Data->m_SqlData->results->next())
			{
				// update Account data
				str_format(buf, sizeof(buf), "UPDATE %s_Account SET Password='%s' WHERE UserID='%d'", Data->m_SqlData->prefix, Data->pass, Data->Account_id[Data->m_ClientID]);
				Data->m_SqlData->statement->execute(buf);
				
				// get Account name from database
				str_format(buf, sizeof(buf), "SELECT name FROM %s_Account WHERE UserID='%d';", Data->m_SqlData->prefix, Data->Account_id[Data->m_ClientID]);
				
				// create results
				Data->m_SqlData->results = Data->m_SqlData->statement->executeQuery(buf);

				// jump to result
				Data->m_SqlData->results->next();
				
				// finally the name is there \o/
				char acc_name[32];
				str_copy(acc_name, Data->m_SqlData->results->getString("Username").c_str(), sizeof(acc_name));	
				dbg_msg("SQL", "Account '%s' changed password.", acc_name);
				
				// Success
				str_format(buf, sizeof(buf), "Successfully changed your password to '%s'.", Data->pass);
				GameServer()->SendBroadcast(buf, Data->m_ClientID);
				GameServer()->SendChatTarget(Data->m_ClientID, buf);
			}
			else
				dbg_msg("SQL", "Account seems to be deleted");
			
			// delete statement and results
			delete Data->m_SqlData->statement;
			delete Data->m_SqlData->results;
		}
		catch (sql::SQLException &e)
		{
			dbg_msg("SQL", "ERROR: Could not update Account");
		}
		
		// disconnect from database
		Data->m_SqlData->disconnect();
	}
	
	delete Data;
	
	lock_release(sql_lock);
}

void CSQL::change_password(int m_ClientID, const char* new_pass)
{
	CSqlData *tmp = new CSqlData();
	tmp->m_ClientID = m_ClientID;
	tmp->Account_id[m_ClientID] = GameServer()->m_apPlayers[m_ClientID]->m_AccData.m_UserID;
	str_copy(tmp->pass, new_pass, sizeof(tmp->pass));
	tmp->m_SqlData = this;
	
	void *change_pw_thread = thread_init(change_password_thread, tmp);
#if defined(CONF_FAMILY_UNIX)
	pthread_detach((pthread_t)change_pw_thread);
#endif
}

// login stuff
static void login_thread(void *user)
{
	lock_wait(sql_lock);
	
	CSqlData *Data = (CSqlData *)user;
	
	if(GameServer()->m_apPlayers[Data->m_ClientID] && !GameServer()->AccountData()->logged_in[Data->m_ClientID])
	{
		// Connect to database
		if(Data->m_SqlData->connect())
		{
			try
			{		
				// check if Account exists
				char buf[1024];
				str_format(buf, sizeof(buf), "SELECT * FROM %s_Account WHERE Username='%s';", Data->m_SqlData->prefix, Data->name);
				Data->m_SqlData->results = Data->m_SqlData->statement->executeQuery(buf);
				if(Data->m_SqlData->results->next())
				{
					// check for right pw and get data
					str_format(buf, sizeof(buf), "SELECT UserID, "
					"Level, Exp, Money "
					"FROM %s_Account WHERE Username='%s' AND Password='%s';", Data->m_SqlData->prefix, Data->name, Data->pass);
					
					// create results
					Data->m_SqlData->results = Data->m_SqlData->statement->executeQuery(buf);
					
					// if match jump to it
					if(Data->m_SqlData->results->next())
					{
						// never use player directly!
						// finally save the result to AccountData() \o/

						// check if Account allready is logged in
						for(int i = 0; i < MAX_CLIENTS; i++)
						{
							if(GameServer()->AccountData()->UserID[i] == Data->m_SqlData->results->getInt("UserID"))
							{
								dbg_msg("SQL", "Account '%s' already is logged in", Data->name);
								
								GameServer()->SendChatTarget(Data->m_ClientID, "This Account is already logged in.");
								
								// delete statement and results
								delete Data->m_SqlData->statement;
								delete Data->m_SqlData->results;
								
								// disconnect from database
								Data->m_SqlData->disconnect();
								
								// delete Data
								delete Data;
	
								// release lock
								lock_release(sql_lock);
								
								return;
							}
						}

						GameServer()->AccountData()->UserID[Data->m_ClientID] = Data->m_SqlData->results->getInt("UserID");
						GameServer()->AccountData()->exp[Data->m_ClientID] = (float)Data->m_SqlData->results->getDouble("Exp");
						GameServer()->AccountData()->money[Data->m_ClientID] = Data->m_SqlData->results->getInt("Money");

						// login should be the last thing
						GameServer()->AccountData()->logged_in[Data->m_ClientID] = true;
						dbg_msg("SQL", "Account '%s' logged in sucessfully", Data->name);
						
						GameServer()->SendChatTarget(Data->m_ClientID, "You are now logged in.");
						char buf[512];
						str_format(buf, sizeof(buf), "Welcome %s!", Data->name);
						GameServer()->SendBroadcast(buf, Data->m_ClientID);
					}
					else
					{
						// wrong password
						dbg_msg("SQL", "Account '%s' is not logged in due to wrong password", Data->name);
						
						GameServer()->SendChatTarget(Data->m_ClientID, "The password you entered is wrong.");
					}
				}
				else
				{
					// no Account
					dbg_msg("SQL", "Account '%s' does not exists", Data->name);
					
					GameServer()->SendChatTarget(Data->m_ClientID, "This Account does not exists.");
					GameServer()->SendChatTarget(Data->m_ClientID, "Please register first. (/register <user> <pass>)");
				}
				
				// delete statement and results
				delete Data->m_SqlData->statement;
				delete Data->m_SqlData->results;
			}
			catch (sql::SQLException &e)
			{
				dbg_msg("SQL", "ERROR: Could not login Account");
			}
			
			// disconnect from database
			Data->m_SqlData->disconnect();
		}
	}
	
	delete Data;
	
	lock_release(sql_lock);
}

void CSQL::login(const char* name, const char* pass, int m_ClientID)
{
	CSqlData *tmp = new CSqlData();
	str_copy(tmp->name, name, sizeof(tmp->name));
	str_copy(tmp->pass, pass, sizeof(tmp->pass));
	tmp->m_ClientID = m_ClientID;
	tmp->m_SqlData = this;
	
	void *login_account_thread = thread_init(login_thread, tmp);
#if defined(CONF_FAMILY_UNIX)
	pthread_detach((pthread_t)login_account_thread);
#endif
}

// update stuff
static void update_thread(void *user)
{
	lock_wait(sql_lock);
	
	CSqlData *Data = (CSqlData *)user;
	
	// Connect to database
	if(Data->m_SqlData->connect())
	{
		try
		{
			// check if Account exists
			char buf[1024];
			str_format(buf, sizeof(buf), "SELECT * FROM %s_Account WHERE UserID='%d';", Data->m_SqlData->prefix, Data->Account_id[Data->m_ClientID]);
			Data->m_SqlData->results = Data->m_SqlData->statement->executeQuery(buf);
			if(Data->m_SqlData->results->next())
			{
				// update Account data
				str_format(buf, sizeof(buf), "UPDATE %s_Account SET "
				"Exp='%f', Money='%d', "
				"WHERE UserID='%d';", 
					Data->m_SqlData->prefix, Data->Account_exp[Data->m_ClientID], Data->Account_money[Data->m_ClientID], 
					Data->Account_id[Data->m_ClientID]);
				Data->m_SqlData->statement->execute(buf);
				
				// get Account name from database
				str_format(buf, sizeof(buf), "SELECT name FROM %s_Account WHERE UserID='%d';", Data->m_SqlData->prefix, Data->Account_id[Data->m_ClientID]);
				
				// create results
				Data->m_SqlData->results = Data->m_SqlData->statement->executeQuery(buf);

				// jump to result
				Data->m_SqlData->results->next();
				
				// finally the nae is there \o/
				char acc_name[32];
				str_copy(acc_name, Data->m_SqlData->results->getString("name").c_str(), sizeof(acc_name));	
				dbg_msg("SQL", "Account '%s' was saved successfully", acc_name);
			}
			else
				dbg_msg("SQL", "Account seems to be deleted");
			
			// delete statement and results
			delete Data->m_SqlData->statement;
			delete Data->m_SqlData->results;
		}
		catch (sql::SQLException &e)
		{
			dbg_msg("SQL", "ERROR: Could not update Account");
		}
		
		// disconnect from database
		Data->m_SqlData->disconnect();
	}
	
	delete Data;
	
	lock_release(sql_lock);
}

void CSQL::update(int m_ClientID)
{
	CSqlData *tmp = new CSqlData();
	tmp->m_ClientID = m_ClientID;
	tmp->Account_id[m_ClientID] = GameServer()->m_apPlayers[m_ClientID]->m_AccData.m_UserID;
	tmp->Account_exp[m_ClientID] = GameServer()->m_apPlayers[m_ClientID]->m_AccData.m_Exp;
	tmp->Account_money[m_ClientID] = GameServer()->m_apPlayers[m_ClientID]->m_AccData.m_Money;
	tmp->Account_level[m_ClientID] = GameServer()->m_apPlayers[m_ClientID]->m_AccData.m_Level;
	tmp->m_SqlData = this;
	
	void *update_account_thread = thread_init(update_thread, tmp);
#if defined(CONF_FAMILY_UNIX)
	pthread_detach((pthread_t)update_account_thread);
#endif
}

// update all
void CSQL::update_all()
{
	lock_wait(sql_lock);
	
	// Connect to database
	if(connect())
	{
		try
		{
			char buf[512];
			char acc_name[32];
			for(int i = 0; i < MAX_CLIENTS; i++)
			{
				if(!GameServer()->m_apPlayers[i])
					continue;
				
				if(!GameServer()->m_apPlayers[i]->LoggedIn)
					continue;
				
				// check if Account exists
				str_format(buf, sizeof(buf), "SELECT * FROM %s_Account WHERE UserID='%d';", prefix, GameServer()->m_apPlayers[i]->m_AccData.m_UserID);
				results = statement->executeQuery(buf);
				if(results->next())
				{
					
					// get Account name from database
					str_format(buf, sizeof(buf), "SELECT name FROM %s_Account WHERE UserID='%d';", prefix, GameServer()->m_apPlayers[i]->m_AccData.m_UserID);
					
					// create results
					results = statement->executeQuery(buf);

					// jump to result
					results->next();
					
					// finally the name is there \o/	
					str_copy(acc_name, results->getString("name").c_str(), sizeof(acc_name));	
					dbg_msg("SQL", "Account '%s' was saved successfully", acc_name);
				}
				else
					dbg_msg("SQL", "Account seems to be deleted");
				
				// delete results
				delete results;
			}
			
			// delete statement
			delete statement;
		}
		catch (sql::SQLException &e)
		{
			dbg_msg("SQL", "ERROR: Could not update Account");
		}
		
		// disconnect from database
		disconnect();
	}

	lock_release(sql_lock);
}
	
	
