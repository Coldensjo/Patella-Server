// tibia_config.cc: the reader of the original .tibia config (tibia_config.hh).
// This is the game's ReadConfig as it was before config.toml, moved here
// unchanged but for three things: the file name is given instead of taken from
// $HOME, the defaults come from the game's SetConfigDefaults, and the dropped
// entries and the passwords as written are noted for the conversion.

#include <cstdio>
#include <cstdlib>
#include <cstring>

#include "script.hh"
#include "query.hh"
#include "config.hh"
#include "utils.hh"

#include "tibia_config.hh"

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

void ReadTibiaConfig(const char *FileName, TTibiaConfigExtras &Extras) {
	SetConfigDefaults();
	Extras = TTibiaConfigExtras();

	TReadScriptFile Script;

	if (!FileExists(const_cast<char *>(FileName))) {
		throw "cannot find config-file";
	}

	Script.open(const_cast<char *>(FileName));
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
			Extras.Dropped.push_back(Ident);
		}

		if (strcmp(Ident, "adminaddress") == 0 || strcmp(Ident, "querymanageraddress") == 0
				|| strcmp(Ident, "querymanageradminpw") == 0 || strcmp(Ident, "querymanagergamepw") == 0
				|| strcmp(Ident, "querymanagerwebpw") == 0) {
			Script.readString();
			Extras.Dropped.push_back(Ident);
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
			Extras.Dropped.push_back(Ident);
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
				Extras.Password[NumberOfQueryManagers] = QUERY_MANAGER[NumberOfQueryManagers].Password;
				DisguisePassword(QUERY_MANAGER[NumberOfQueryManagers].Password, PasswordKey);
				Script.readSymbol(')');
				NumberOfQueryManagers++;
			} while (Script.readSpecial() != '}');
		}
	}

	Script.close();
}
