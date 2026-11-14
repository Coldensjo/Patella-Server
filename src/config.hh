// config.hh: the server's settings (config.cc): the paths, the world config and
// the other values read from config.toml or from the query manager.

#ifndef PATELLA_CONFIG_HH
#define PATELLA_CONFIG_HH

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

// The key DisguisePassword is given for the query manager passwords.
extern const char PasswordKey[9];

void DisguisePassword(char *Password, const char *Key);

// Sets every setting to its value for a config that does not name it: empty
// paths, SHM key 0, debug level 1, a public world without a name, beat 200,
// no query managers; and the world config to type NORMAL, reboot time 540,
// address 0.0.0.0, port 0 and no players.
void SetConfigDefaults();

// Reads the settings from the TOML file FileName, over the defaults. Throws a
// const char * "<file>:<line>:<column>: <what>" for a syntax error, an
// unknown key or a value of the wrong type or out of range.
void LoadConfig(const char *FileName);

// Reads the settings from FileName, or with nullptr from config.toml in the
// home folder ($home before $HOME, the current folder without either).
// Throws a const char * when the file does not exist or LoadConfig fails.
void ReadConfig(const char *FileName);

#endif
