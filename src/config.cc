// config.cc: the server's settings. ReadConfig reads them from .tibia in the
// home folder: the paths, the shared memory key, the debug level, the world's
// name and state, the beat and the query managers. The world config (type,
// reboot time, address, port, player limits) is filled in by main.cc's
// LoadWorldConfig from the query manager. DisguisePassword disguises a query
// manager password before it is sent.

#include <cstdio>
#include <cstdlib>
#include <cstring>

#include "script.hh"
#include "threads.hh"
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
int GamePort;
char GameAddress[16];
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

// A database entry of the config (admindatabase, volatiledatabase,
// webdatabase, forumdatabase, managerdatabase): (product, database, login,
// password, host, port). The game connects to none of them, so the entry is
// read and dropped; the password still goes through DisguisePassword, which
// throws for a password longer than its key.
static void SkipDatabaseSettings(TReadScriptFile &Script) {
	Script.readSymbol('(');
	Script.readIdentifier();
	Script.readSymbol(',');
	Script.readString();
	Script.readSymbol(',');
	Script.readString();
	Script.readSymbol(',');
	char Password[30];
	strcpy(Password, Script.readString());
	DisguisePassword(Password, PasswordKey);
	Script.readSymbol(',');
	Script.readString();
	Script.readSymbol(',');
	Script.readString();
	Script.readSymbol(')');
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
	GamePort = 0;
	strcpy(GameAddress, "0.0.0.0");
	DebugLevel = 1;
	PrivateWorld = false;
	WorldType = NORMAL;
	MaxPlayers = 0;
	MaxNewbies = 0;
	PremiumPlayerBuffer = 0;
	PremiumNewbieBuffer = 0;
	Beat = 200;
	RebootTime = 540;
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

		// These entries are read and dropped: the game uses none of them. The
		// query managers it connects to are those of "querymanager".
		if (strcmp(Ident, "adminport") == 0 || strcmp(Ident, "querymanagerport") == 0) {
			Script.readNumber();
		}

		if (strcmp(Ident, "adminaddress") == 0 || strcmp(Ident, "querymanageraddress") == 0
				|| strcmp(Ident, "querymanageradminpw") == 0 || strcmp(Ident, "querymanagergamepw") == 0
				|| strcmp(Ident, "querymanagerwebpw") == 0) {
			Script.readString();
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

		if (strcmp(Ident, "admindatabase") == 0 || strcmp(Ident, "volatiledatabase") == 0
				|| strcmp(Ident, "webdatabase") == 0 || strcmp(Ident, "forumdatabase") == 0
				|| strcmp(Ident, "managerdatabase") == 0) {
			SkipDatabaseSettings(Script);
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
