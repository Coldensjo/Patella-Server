#include <cstdio>
#include <cstdlib>
#include <cstring>

#include "script.hh"
#include "threads.hh"
#include "database.hh"
#include "query.hh"
#include "config.hh"
#include "utils.hh"

static const char PasswordKey[9] = "Pm-,o%yD";

char BINPATH[4096];
char DATAPATH[4096];
char LOGPATH[4096];
char MAPPATH[4096];
char MONSTERPATH[4096];
char NPCPATH[4096];
char ORIGMAPPATH[4096];
char SAVEPATH[4096];
char USERPATH[4096];
int SHMKey;
int AdminPort;
int GamePort;
int QueryManagerPort;
char AdminAddress[16];
char GameAddress[16];
char QueryManagerAddress[16];
char QueryManagerAdminPW[9];
char QueryManagerGamePW[9];
char QueryManagerWebPW[9];
int DebugLevel;
bool PrivateWorld;
TWorldType WorldType;
char WorldName[30];
int MaxPlayers;
int MaxNewbies;
int PremiumPlayerBuffer;
int PremiumNewbieBuffer;
int Beat;
int RebootTime;
TDatabaseSettings ADMIN_DATABASE;
TDatabaseSettings VOLATILE_DATABASE;
TDatabaseSettings WEB_DATABASE;
TDatabaseSettings FORUM_DATABASE;
TDatabaseSettings MANAGER_DATABASE;
int NumberOfQueryManagers;
TQueryManagerSettings QUERY_MANAGER[MAX_QUERY_MANAGERS];

void DisguisePassword(char *Password, const char *Key) {
	if (Password == nullptr) {
		throw "password is null";
	}

	if (Key == nullptr) {
		throw "key for disguising password is null";
	}

	if (strlen(Key) < strlen(Password)) {
		throw "key for disguising password is too short";
	}

	for (int i = 0; i < static_cast<int>(strlen(Password)); i++) {
		int a = Password[i];
		int b = Key[i];
		Password[i] = static_cast<char>((b - a + 94) % 94 + 33);
	}
}

void ReadConfig() {
	BINPATH[0] = 0;
	DATAPATH[0] = 0;
	LOGPATH[0] = 0;
	MAPPATH[0] = 0;
	MONSTERPATH[0] = 0;
	NPCPATH[0] = 0;
	ORIGMAPPATH[0] = 0;
	SAVEPATH[0] = 0;
	USERPATH[0] = 0;
	SHMKey = 0;
	AdminPort = 0;
	GamePort = 0;
	QueryManagerPort = 0;
	strcpy(AdminAddress, "0.0.0.0");
	strcpy(GameAddress, "0.0.0.0");
	strcpy(QueryManagerAddress, "0.0.0.0");
	QueryManagerAdminPW[0] = 0;
	QueryManagerGamePW[0] = 0;
	QueryManagerWebPW[0] = 0;
	DebugLevel = 1;
	PrivateWorld = false;
	WorldType = NORMAL;
	MaxPlayers = 0;
	MaxNewbies = 0;
	PremiumPlayerBuffer = 0;
	PremiumNewbieBuffer = 0;
	Beat = 200;
	RebootTime = 540;
	ADMIN_DATABASE.Database[0] = 0;
	VOLATILE_DATABASE.Database[0] = 0;
	WEB_DATABASE.Database[0] = 0;
	FORUM_DATABASE.Database[0] = 0;
	MANAGER_DATABASE.Database[0] = 0;
	NumberOfQueryManagers = 0;

	TReadScriptFile Script;
	char FileName[4096];

	sprintf(FileName, ".tibia");
	if (getenv("HOME") != nullptr) {
		sprintf(FileName, "%s/.tibia", getenv("HOME"));
	}
	if (getenv("home") != nullptr) {
		sprintf(FileName, "%s/.tibia", getenv("home"));
	}

	if (!FileExists(FileName)) {
		throw "cannot find config-file";
	}

	Script.open(FileName);
	while (true) {
		Script.nextToken();
		if (Script.Token == ENDOFFILE) {
			break;
		}

		if (Script.Token != IDENTIFIER) {
			Script.error("Identifier expected");
		}

		char Ident[30];
		strcpy(Ident, Script.getIdentifier());
		Script.readSymbol('=');

		if (strcmp(Ident, "binpath") == 0) {
			strcpy(BINPATH, Script.readString());
		}

		if (strcmp(Ident, "mappath") == 0) {
			strcpy(MAPPATH, Script.readString());
		}

		if (strcmp(Ident, "origmappath") == 0) {
			strcpy(ORIGMAPPATH, Script.readString());
		}

		if (strcmp(Ident, "datapath") == 0) {
			strcpy(DATAPATH, Script.readString());
		}

		if (strcmp(Ident, "monsterpath") == 0) {
			strcpy(MONSTERPATH, Script.readString());
		}

		if (strcmp(Ident, "npcpath") == 0) {
			strcpy(NPCPATH, Script.readString());
		}

		if (strcmp(Ident, "userpath") == 0) {
			strcpy(USERPATH, Script.readString());
		}

		if (strcmp(Ident, "logpath") == 0) {
			strcpy(LOGPATH, Script.readString());
		}

		if (strcmp(Ident, "savepath") == 0) {
			strcpy(SAVEPATH, Script.readString());
		}

		if (strcmp(Ident, "shm") == 0) {
			SHMKey = Script.readNumber();
		}

		if (strcmp(Ident, "adminport") == 0) {
			AdminPort = Script.readNumber();
		}

		if (strcmp(Ident, "adminaddress") == 0) {
			strcpy(AdminAddress, Script.readString());
		}

		if (strcmp(Ident, "querymanagerport") == 0) {
			QueryManagerPort = Script.readNumber();
		}

		if (strcmp(Ident, "querymanageraddress") == 0) {
			strcpy(QueryManagerAddress, Script.readString());
		}

		if (strcmp(Ident, "querymanageradminpw") == 0) {
			strcpy(QueryManagerAdminPW, Script.readString());
		}

		if (strcmp(Ident, "querymanagergamepw") == 0) {
			strcpy(QueryManagerGamePW, Script.readString());
		}

		if (strcmp(Ident, "querymanagerwebpw") == 0) {
			strcpy(QueryManagerWebPW, Script.readString());
		}

		if (strcmp(Ident, "debuglevel") == 0) {
			DebugLevel = Script.readNumber();
		}

		if (strcmp(Ident, "state") == 0) {
			PrivateWorld = (strcmp(Script.readIdentifier(), "private") == 0);
		}

		if (strcmp(Ident, "world") == 0) {
			strcpy(WorldName, Script.readString());
		}

		if (strcmp(Ident, "beat") == 0) {
			Beat = Script.readNumber();
		}

		if (strcmp(Ident, "admindatabase") == 0) {
			Script.readSymbol('(');
			strcpy(ADMIN_DATABASE.Product, Script.readIdentifier());
			Script.readSymbol(',');
			strcpy(ADMIN_DATABASE.Database, Script.readString());
			Script.readSymbol(',');
			strcpy(ADMIN_DATABASE.Login, Script.readString());
			Script.readSymbol(',');
			strcpy(ADMIN_DATABASE.Password, Script.readString());
			DisguisePassword(ADMIN_DATABASE.Password, PasswordKey);
			Script.readSymbol(',');
			strcpy(ADMIN_DATABASE.Host, Script.readString());
			Script.readSymbol(',');
			strcpy(ADMIN_DATABASE.Port, Script.readString());
			Script.readSymbol(')');
		}

		if (strcmp(Ident, "volatiledatabase") == 0) {
			Script.readSymbol('(');
			strcpy(VOLATILE_DATABASE.Product, Script.readIdentifier());
			Script.readSymbol(',');
			strcpy(VOLATILE_DATABASE.Database, Script.readString());
			Script.readSymbol(',');
			strcpy(VOLATILE_DATABASE.Login, Script.readString());
			Script.readSymbol(',');
			strcpy(VOLATILE_DATABASE.Password, Script.readString());
			DisguisePassword(VOLATILE_DATABASE.Password, PasswordKey);
			Script.readSymbol(',');
			strcpy(VOLATILE_DATABASE.Host, Script.readString());
			Script.readSymbol(',');
			strcpy(VOLATILE_DATABASE.Port, Script.readString());
			Script.readSymbol(')');
		}

		if (strcmp(Ident, "webdatabase") == 0) {
			Script.readSymbol('(');
			strcpy(WEB_DATABASE.Product, Script.readIdentifier());
			Script.readSymbol(',');
			strcpy(WEB_DATABASE.Database, Script.readString());
			Script.readSymbol(',');
			strcpy(WEB_DATABASE.Login, Script.readString());
			Script.readSymbol(',');
			strcpy(WEB_DATABASE.Password, Script.readString());
			DisguisePassword(WEB_DATABASE.Password, PasswordKey);
			Script.readSymbol(',');
			strcpy(WEB_DATABASE.Host, Script.readString());
			Script.readSymbol(',');
			strcpy(WEB_DATABASE.Port, Script.readString());
			Script.readSymbol(')');
		}

		if (strcmp(Ident, "forumdatabase") == 0) {
			Script.readSymbol('(');
			strcpy(FORUM_DATABASE.Product, Script.readIdentifier());
			Script.readSymbol(',');
			strcpy(FORUM_DATABASE.Database, Script.readString());
			Script.readSymbol(',');
			strcpy(FORUM_DATABASE.Login, Script.readString());
			Script.readSymbol(',');
			strcpy(FORUM_DATABASE.Password, Script.readString());
			DisguisePassword(FORUM_DATABASE.Password, PasswordKey);
			Script.readSymbol(',');
			strcpy(FORUM_DATABASE.Host, Script.readString());
			Script.readSymbol(',');
			strcpy(FORUM_DATABASE.Port, Script.readString());
			Script.readSymbol(')');
		}

		if (strcmp(Ident, "managerdatabase") == 0) {
			Script.readSymbol('(');
			strcpy(MANAGER_DATABASE.Product, Script.readIdentifier());
			Script.readSymbol(',');
			strcpy(MANAGER_DATABASE.Database, Script.readString());
			Script.readSymbol(',');
			strcpy(MANAGER_DATABASE.Login, Script.readString());
			Script.readSymbol(',');
			strcpy(MANAGER_DATABASE.Password, Script.readString());
			DisguisePassword(MANAGER_DATABASE.Password, PasswordKey);
			Script.readSymbol(',');
			strcpy(MANAGER_DATABASE.Host, Script.readString());
			Script.readSymbol(',');
			strcpy(MANAGER_DATABASE.Port, Script.readString());
			Script.readSymbol(')');
		}

		if (strcmp(Ident, "querymanager") == 0) {
			Script.readSymbol('{');
			do {
				if (NumberOfQueryManagers >= MAX_QUERY_MANAGERS) {
					Script.error("Cannot handle more query managers");
				}

				Script.readSymbol('(');
				strcpy(QUERY_MANAGER[NumberOfQueryManagers].Host, Script.readString());
				Script.readSymbol(',');
				QUERY_MANAGER[NumberOfQueryManagers].Port = Script.readNumber();
				Script.readSymbol(',');
				strcpy(QUERY_MANAGER[NumberOfQueryManagers].Password, Script.readString());
				DisguisePassword(QUERY_MANAGER[NumberOfQueryManagers].Password, PasswordKey);
				Script.readSymbol(')');
				NumberOfQueryManagers++;
			} while (Script.readSpecial() != '}');
		}
	}

	Script.close();
}
