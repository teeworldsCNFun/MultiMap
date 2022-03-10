/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#ifndef GAME_SERVER_GAMECONTEXT_H
#define GAME_SERVER_GAMECONTEXT_H

#include <engine/server.h>
#include <engine/console.h>
#include <engine/shared/memheap.h>

#include <game/layers.h>
#include <game/voting.h>

#include <vector>

#include "eventhandler.h"
#include "gamecontroller.h"
#include "gameworld.h"
#include "player.h"
#include "GoaWay/sql.h"

#include <localization/components/localization.h>

/*
	Tick
		Game Context (CGameContext::tick)
			Game World (GAMEWORLD::tick)
				Reset world if requested (GAMEWORLD::reset)
				All entities in the world (ENTITY::tick)
				All entities in the world (ENTITY::tick_defered)
				Remove entities marked for deletion (GAMEWORLD::remove_entities)
			Game Controller (GAMECONTROLLER::tick)
			All players (CPlayer::tick)


	Snap
		Game Context (CGameContext::snap)
			Game World (GAMEWORLD::snap)
				All entities in the world (ENTITY::snap)
			Game Controller (GAMECONTROLLER::snap)
			Events handler (EVENT_HANDLER::snap)
			All players (CPlayer::snap)

*/
class CGameContext : public IGameServer
{
	IServer *m_pServer;
	class IConsole *m_pConsole;
	std::vector<CLayers> m_vLayers;
	std::vector<CCollision> m_vCollision;
	CNetObjHandler m_NetObjHandler;
	CTuningParams m_Tuning;

	/* SQL */
	CSQL *m_Sql;
	CAccountData *m_AccountData;

	static bool ConTuneParam(IConsole::IResult *pResult, void *pUserData);
	static bool ConTuneReset(IConsole::IResult *pResult, void *pUserData);
	static bool ConTuneDump(IConsole::IResult *pResult, void *pUserData);
	static bool ConPause(IConsole::IResult *pResult, void *pUserData);
	static bool ConChangeMap(IConsole::IResult *pResult, void *pUserData);
	static bool ConRestart(IConsole::IResult *pResult, void *pUserData);
	static bool ConBroadcast(IConsole::IResult *pResult, void *pUserData);
	static bool ConSay(IConsole::IResult *pResult, void *pUserData);
	static bool ConSetTeam(IConsole::IResult *pResult, void *pUserData);
	static bool ConSetTeamAll(IConsole::IResult *pResult, void *pUserData);
	static bool ConSwapTeams(IConsole::IResult *pResult, void *pUserData);
	static bool ConShuffleTeams(IConsole::IResult *pResult, void *pUserData);
	static bool ConLockTeams(IConsole::IResult *pResult, void *pUserData);
	static bool ConAddVote(IConsole::IResult *pResult, void *pUserData);
	static bool ConRemoveVote(IConsole::IResult *pResult, void *pUserData);
	static bool ConForceVote(IConsole::IResult *pResult, void *pUserData);
	static bool ConClearVotes(IConsole::IResult *pResult, void *pUserData);
	static bool ConVote(IConsole::IResult *pResult, void *pUserData);
	static bool ConchainSpecialMotdupdate(IConsole::IResult *pResult, void *pUserData, IConsole::FCommandCallback pfnCallback, void *pCallbackUserData);
	
	// Register Chat Command
	void RCC(const char *pName, const char *pParams, IConsole::FCommandCallback pfnFunc);

	// Chat cmds
	static bool ConRegister(IConsole::IResult *pResult, void *pUserData);
	static bool ConLogin(IConsole::IResult *pResult, void *pUserData);
	static bool ConLanguage(IConsole::IResult *pResult, void *pUserData);

	CGameContext(int Resetting);
	void Construct(int Resetting);

	bool m_Resetting;
public:
	IServer *Server() const { return m_pServer; }
	class IConsole *Console() { return m_pConsole; }
	CCollision *Collision(int MapID) { return &(m_vCollision[MapID]); }
	CTuningParams *Tuning() { return &m_Tuning; }

	/* SQL */
	CSQL *Sql() const { return m_Sql; };
	CAccountData *AccountData() {return m_AccountData; };



	CGameContext();
	~CGameContext();

	void Clear();

	CEventHandler m_Events;
	CPlayer *m_apPlayers[MAX_CLIENTS];

	IGameController *m_pController;
	CGameWorld m_World;

	// helper functions
	class CCharacter *GetPlayerChar(int ClientID);

	int m_LockTeams;

	// voting
	void StartVote(const char *pDesc, const char *pCommand, const char *pReason);
	void EndVote();
	void SendVoteSet(int ClientID);
	void SendVoteStatus(int ClientID, int Total, int Yes, int No);
	void AbortVoteKickOnDisconnect(int ClientID);

	int m_ChatResponseTargetID;

	int m_VoteCreator;
	int64 m_VoteCloseTime;
	bool m_VoteUpdate;
	int m_VotePos;
	char m_aVoteDescription[VOTE_DESC_LENGTH];
	char m_aVoteCommand[VOTE_CMD_LENGTH];
	char m_aVoteReason[VOTE_REASON_LENGTH];
	int m_NumVoteOptions;
	int m_VoteEnforce;
	enum
	{
		VOTE_ENFORCE_UNKNOWN=0,
		VOTE_ENFORCE_NO,
		VOTE_ENFORCE_YES,
	};
	CHeap *m_pVoteOptionHeap;
	CVoteOptionServer *m_pVoteOptionFirst;
	CVoteOptionServer *m_pVoteOptionLast;

	// helper functions
	void CreateDamageInd(vec2 Pos, float AngleMod, int Amount, int MapID);
	void CreateExplosion(vec2 Pos, int Owner, int Weapon, bool NoDamage, int MapID);
	void CreateHammerHit(vec2 Pos, int MapID);
	void CreatePlayerSpawn(vec2 Pos, int MapID);
	void CreateDeath(vec2 Pos, int Who, int MapID);
	void CreateSound(vec2 Pos, int Sound, int Mask, int MapID);
	void CreateSoundGlobal(int Sound, int Target=-1);
	void CreateLaserDotEvent(vec2 Pos0, vec2 Pos1, int LifeSpan, int MapID);
	void CreateHammerDotEvent(vec2 Pos, int LifeSpan, int MapID);
	void CreateLoveEvent(vec2 Pos, int MapID);

		struct LaserDotState
		{
			vec2 m_Pos0;
			vec2 m_Pos1;
			int m_LifeSpan;
			int m_SnapID;
		};
		array<LaserDotState> m_LaserDots;

		struct HammerDotState
		{
			vec2 m_Pos;
			int m_LifeSpan;
			int m_SnapID;
		};
		array<HammerDotState> m_HammerDots;

		struct LoveDotState
		{
			vec2 m_Pos;
			int m_LifeSpan;
			int m_SnapID;
		};
		array<LoveDotState> m_LoveDots;


	enum
	{
		CHAT_ALL=-2,
		CHAT_SPEC=-1,
		CHAT_RED=0,
		CHAT_BLUE=1
	};

	// network
	void SendChatTarget(int To, const char *pText, ...);
	void SendChat(int ClientID, int Team, const char *pText);
	void SendEmoticon(int ClientID, int Emoticon);
	void SendWeaponPickup(int ClientID, int Weapon);
	void SendBroadcast(const char *pText, int ClientID);

	//
	void CheckPureTuning();
	void SendTuningParams(int ClientID);

	//
	void SwapTeams();

	// engine events
	virtual void OnInit();
	virtual void OnInitMap(int MapID);
	virtual void OnConsoleInit();
	virtual void OnShutdown();

	virtual void OnTick();
	virtual void OnPreSnap();
	virtual void OnSnap(int ClientID);
	virtual void OnPostSnap();

	virtual void OnMessage(int MsgID, CUnpacker *pUnpacker, int ClientID);

	virtual void OnClientConnected(int ClientID, int MapChange);
	virtual void OnClientEnter(int ClientID);
	virtual void KillCharacter(int ClientID);
	virtual void OnClientDrop(int ClientID, const char *pReason);
	virtual void OnClientDirectInput(int ClientID, void *pInput);
	virtual void OnClientPredictedInput(int ClientID, void *pInput);

	void PrepareClientChangeMap(int ClientID) override;

	virtual bool IsClientReady(int ClientID);
	virtual bool IsClientPlayer(int ClientID);

	void InitGeolocation();

	virtual const char *GameType();
	virtual const char *Version();
	virtual const char *NetVersion();
};

inline int CmaskAll() { return -1; }
inline int CmaskOne(int ClientID) { return 1<<ClientID; }
inline int CmaskAllExceptOne(int ClientID) { return 0x7fffffff^CmaskOne(ClientID); }
inline bool CmaskIsSet(int Mask, int ClientID) { return (Mask&CmaskOne(ClientID)) != 0; }
#endif
