/* SQL Class by Sushi */

#include <engine/shared/protocol.h>

#include <mysql_connection.h>
	
#include <cppconn/driver.h>
#include <cppconn/exception.h>
#include <cppconn/resultset.h>
#include <cppconn/statement.h>

class CSQL
{
public:
	CSQL(class CGameContext *pGameServer);

	sql::Driver *driver;
	sql::Connection *connection;
	sql::Statement *statement;
	sql::ResultSet *results;
	
	// copy of config vars
	const char* database;
	const char* prefix;
	const char* user;
	const char* pass;
	const char* ip;
	int port;
	
	bool connect();
	void disconnect();
	
	void create_tables();
	void create_account(const char* name, const char* pass, int client_id);
	void delete_account(const char* name);
	void delete_account(int client_id);
	void change_password(int client_id, const char* new_pass);

	void login(const char* name, const char* pass, int client_id);
	void update(int client_id);
	void update_all();

/*	static void change_password_thread(void *user);
	static void login_thread(void *user);
	static void update_thread(void *user);
	static void create_account_thread(void *user);*/
};

struct CSqlData
{
	CSQL *m_SqlData;
	char name[32];
	char pass[32];
	int m_ClientID;
	int Account_id[MAX_CLIENTS];
	float Account_exp[MAX_CLIENTS];
	int Account_money[MAX_CLIENTS];
	int Account_level[MAX_CLIENTS];
};

struct CAccountData
{
	int UserID[MAX_CLIENTS];
	
	bool logged_in[MAX_CLIENTS];

	float exp[MAX_CLIENTS];
	int money[MAX_CLIENTS];
};
