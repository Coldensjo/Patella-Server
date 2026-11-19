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
// The file of the server's RSA private key (rsa_key, by default key.pem in
// the config file's folder), as a full path unless written as an absolute one.
extern char RSAKEYPATH[4096];
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

// [decay]: how long objects lie before they decay, as multipliers of the
// times items.toml gives (total_expire_time), each MIN_DECAY_MULTIPLIER to
// MAX_DECAY_MULTIPLIER and 1 by default: of the corpses monsters leave
// (monster_corpse), of those players leave (player_corpse), and of the liquid
// pools (blood): the splash a hit leaves, the pool a death leaves, and with
// that pool's type also a spilled liquid. map.cc's CronExpire applies them.
extern double MonsterCorpseDecay;
extern double PlayerCorpseDecay;
extern double BloodDecay;

const double MIN_DECAY_MULTIPLIER = 0.01;
const double MAX_DECAY_MULTIPLIER = 1000.0;

// Time (seconds, a type's TOTALEXPIRETIME) times a [decay] multiplier, rounded
// to whole seconds: at least 1, so that what decays still lies for a moment,
// and at most INT_MAX. A multiplier of 1 and a time of 0 or less give Time as
// it is.
int MultiplyDecayTime(int Time, double Multiplier);

// The key DisguisePassword is given for the query manager passwords.
extern const char PasswordKey[9];

void DisguisePassword(char *Password, const char *Key);

// Sets every setting to its value for a config that does not name it: empty
// paths (LoadConfig then sets RSAKEYPATH to key.pem in the file's folder),
// SHM key 0, debug level 1, a public world without a name, beat 200, the
// [decay] multipliers 1, no query managers; and the world config to type
// NORMAL, reboot time 540, address 0.0.0.0, port 0 and no players.
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
