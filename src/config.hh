#ifndef PATELLA_CONFIG_HH
#define PATELLA_CONFIG_HH

struct TDatabaseSettings;

enum TWorldType {
	NORMAL = 0,
	NON_PVP = 1,
	PVP_ENFORCED = 2,
};

extern char DATAPATH[4096];
extern char MAPPATH[4096];
extern char ORIGMAPPATH[4096];
extern char USERPATH[4096];
extern char LOGPATH[4096];
extern char SAVEPATH[4096];
extern char BINPATH[4096];
extern char MONSTERPATH[4096];
extern char NPCPATH[4096];
extern int SHMKey;
extern int GamePort;
extern char GameAddress[16];
extern int DebugLevel;
extern bool PrivateWorld;
extern TWorldType WorldType;
extern char WorldName[30];
extern int MaxPlayers;
extern int MaxNewbies;
extern int PremiumPlayerBuffer;
extern int PremiumNewbieBuffer;
extern int Beat;
extern int RebootTime;

void DisguisePassword(char *Password, const char *Key);
void ReadConfig();

extern int AdminPort;
extern int QueryManagerPort;
extern char AdminAddress[16];
extern char QueryManagerAddress[16];
extern char QueryManagerAdminPW[9];
extern char QueryManagerGamePW[9];
extern char QueryManagerWebPW[9];
extern TDatabaseSettings ADMIN_DATABASE;
extern TDatabaseSettings VOLATILE_DATABASE;
extern TDatabaseSettings WEB_DATABASE;
extern TDatabaseSettings FORUM_DATABASE;
extern TDatabaseSettings MANAGER_DATABASE;

#endif
