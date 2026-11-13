#include <cstdlib>

#include "script.hh"
#include "threads.hh"
#include "database.hh"
#include "query.hh"
#include "config.hh"
#include "utils.hh"

[[maybe_unused]] static const char PasswordKey[9] = "Pm-,o%yD";	// config.c:59
char BINPATH[4096];	// config.c:18
char DATAPATH[4096];	// config.c:19
char LOGPATH[4096];	// config.c:20
char MAPPATH[4096];	// config.c:21
char MONSTERPATH[4096];	// config.c:22
char NPCPATH[4096];	// config.c:23
char ORIGMAPPATH[4096];	// config.c:24
char SAVEPATH[4096];	// config.c:25
char USERPATH[4096];	// config.c:26
int SHMKey;	// config.c:28
int AdminPort;	// config.c:29
int GamePort;	// config.c:30
int QueryManagerPort;	// config.c:31
char AdminAddress[16];	// config.c:32
char GameAddress[16];	// config.c:33
char QueryManagerAddress[16];	// config.c:34
char QueryManagerAdminPW[9];	// config.c:35
char QueryManagerGamePW[9];	// config.c:36
char QueryManagerWebPW[9];	// config.c:37
int DebugLevel;	// config.c:39
bool PrivateWorld;	// config.c:40
TWorldType WorldType;	// config.c:41
char WorldName[30];	// config.c:42
int MaxPlayers;	// config.c:43
int MaxNewbies;	// config.c:44
int PremiumPlayerBuffer;	// config.c:45
int PremiumNewbieBuffer;	// config.c:46
int Beat;	// config.c:47
int RebootTime;	// config.c:48
TDatabaseSettings ADMIN_DATABASE;	// config.c:50
TDatabaseSettings VOLATILE_DATABASE;	// config.c:51
TDatabaseSettings WEB_DATABASE;	// config.c:52
TDatabaseSettings FORUM_DATABASE;	// config.c:53
TDatabaseSettings MANAGER_DATABASE;	// config.c:54
int NumberOfQueryManagers;	// config.c:56
TQueryManagerSettings QUERY_MANAGER[10];	// config.c:57

// 0x08052c10  config.c:71
void DisguisePassword(char * /*Password*/, const char * /*Key*/) { abort(); /* STUB */ }

// 0x08052d00  config.c:88
void ReadConfig() { abort(); /* STUB */ }
