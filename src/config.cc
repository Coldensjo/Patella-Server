// config.cc: the server's settings. ReadConfig reads them from config.toml
// (data/config.toml in the repository is an example, converted from the
// original .tibia by tools/convert): the paths, the shared memory key, the
// debug level, the world's name and state, the beat, the [decay] and [rates]
// multipliers, the query managers and the file of the RSA key (communication.cc's
// ReadPrivateKey reads it).
// The world config (type, reboot time, address, port, player limits) is filled
// in by main.cc's LoadWorldConfig from the query manager. DisguisePassword
// disguises a query manager password before it is sent.

#include "tomlfile.hh"

#include <climits>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <string>

#include "query.hh"
#include "config.hh"
#include "utils.hh"

const char PasswordKey[9] = "Pm-,o%yD";

char BINPATH[4096];
char DATAPATH[4096];
char LOGPATH[4096];
char MAPPATH[4096];
char ORIGMAPPATH[4096];
char SAVEPATH[4096];
char USERPATH[4096];
char RSAKEYPATH[4096];
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
double MonsterCorpseDecay;
double PlayerCorpseDecay;
double BloodDecay;
int ExperienceRate;
int MagicLevelRate;
int SkillRate;
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

int MultiplyDecayTime(int Time, double Multiplier) {
	if (Multiplier == 1.0 || Time <= 0) {
		return Time;
	}

	double Multiplied = std::round(Time * Multiplier);
	if (Multiplied < 1.0) {
		return 1;
	}
	if (Multiplied >= static_cast<double>(INT_MAX)) {
		return INT_MAX;
	}
	return static_cast<int>(Multiplied);
}

int MultiplyRate(int Amount, int Multiplier) {
	if (Multiplier == 1) {
		return Amount;
	}

	int64_t Multiplied = static_cast<int64_t>(Amount) * Multiplier;
	if (Multiplied > INT_MAX) {
		return INT_MAX;
	}
	return static_cast<int>(Multiplied);
}

void SetConfigDefaults() {
	BINPATH[0] = 0;
	DATAPATH[0] = 0;
	LOGPATH[0] = 0;
	MAPPATH[0] = 0;
	ORIGMAPPATH[0] = 0;
	SAVEPATH[0] = 0;
	USERPATH[0] = 0;
	RSAKEYPATH[0] = 0;
	SHMKey = 0;
	GamePort = 0;
	strcpy(GameAddress, "0.0.0.0");
	DebugLevel = 1;
	PrivateWorld = false;
	WorldType = NORMAL;
	WorldName[0] = 0;
	MaxPlayers = 0;
	MaxNewbies = 0;
	PremiumPlayerBuffer = 0;
	PremiumNewbieBuffer = 0;
	Beat = 200;
	RebootTime = 540;
	MonsterCorpseDecay = 1.0;
	PlayerCorpseDecay = 1.0;
	BloodDecay = 1.0;
	ExperienceRate = 1;
	MagicLevelRate = 1;
	SkillRate = 1;
	NumberOfQueryManagers = 0;
}

// A path of Table (a folder of [paths], or rsa_key). A relative one is taken
// relative to Folder, the folder of the config file as a full path without a
// trailing '/': it is written out as Folder/<path>, and "." as Folder. An
// absolute or empty one is kept as it is written.
static void ReadPath(const toml::table &Paths, const char *Key, const std::string &Folder, char *Path) {
	char Value[4096];
	if (!TomlGetString(Paths, Key, Value, sizeof(Value))) {
		return;
	}

	if (Value[0] == 0 || Value[0] == '/') {
		strcpy(Path, Value);
		return;
	}

	std::string Resolved = strcmp(Value, ".") == 0 ? Folder : (Folder == "/" ? "" : Folder) + "/" + Value;
	if (Resolved.size() >= sizeof(Value)) {
		TomlError(Paths.get(Key)->source(), "%s is longer than %d characters as a full path",
				Key, static_cast<int>(sizeof(Value)) - 1);
	}
	strcpy(Path, Resolved.c_str());
}

// The folder of a file as a full path, normalized, without a trailing '/'
// (but "/" for the root).
static std::string FolderOf(const char *FileName) {
	std::string Folder = std::filesystem::absolute(FileName).parent_path().lexically_normal().string();
	while (Folder.size() > 1 && Folder.back() == '/') {
		Folder.pop_back();
	}
	return Folder;
}

// An entry of [[query_manager]]: host, port and password, all three needed.
// The password is the one written in the file; it is kept disguised.
static void ReadQueryManager(const toml::node &Node) {
	if (!Node.is_table()) {
		TomlError(Node.source(), "an entry of query_manager must be a table");
	}

	const toml::table &Entry = *Node.as_table();
	TomlCheckKeys(Entry, "[[query_manager]]", {"host", "port", "password"});
	for (const char *Key : {"host", "port", "password"}) {
		if (!Entry.contains(Key)) {
			TomlError(Node.source(), "an entry of query_manager needs %s", Key);
		}
	}

	if (NumberOfQueryManagers >= MAX_QUERY_MANAGERS) {
		TomlError(Node.source(), "more than %d query managers", MAX_QUERY_MANAGERS);
	}

	TQueryManagerSettings &Settings = QUERY_MANAGER[NumberOfQueryManagers];
	TomlGetString(Entry, "host", Settings.Host, sizeof(Settings.Host));
	TomlGetNumber(Entry, "port", Settings.Port, INT_MIN, INT_MAX);
	TomlGetString(Entry, "password", Settings.Password, sizeof(Settings.Password));
	try {
		DisguisePassword(Settings.Password, PasswordKey);
	} catch (const char *Text) {
		TomlError(Entry.get("password")->source(), "%s", Text);
	}
	NumberOfQueryManagers++;
}

void LoadConfig(const char *FileName) {
	SetConfigDefaults();

	toml::table Config = TomlParseFile(FileName);
	TomlCheckKeys(Config, "the top level",
			{"debug_level", "beat", "shm_key", "rsa_key", "world", "decay", "rates", "paths", "query_manager"});
	std::string Folder = FolderOf(FileName);

	TomlGetNumber(Config, "debug_level", DebugLevel, INT_MIN, INT_MAX);
	TomlGetNumber(Config, "beat", Beat, INT_MIN, INT_MAX);
	TomlGetNumber(Config, "shm_key", SHMKey, INT_MIN, INT_MAX);

	// The RSA key file: key.pem next to the config file unless rsa_key names
	// another.
	std::string DefaultKey = (Folder == "/" ? "" : Folder) + "/key.pem";
	if (DefaultKey.size() >= sizeof(RSAKEYPATH)) {
		throw "the folder of the config file is too long for the default rsa_key";
	}
	strcpy(RSAKEYPATH, DefaultKey.c_str());
	ReadPath(Config, "rsa_key", Folder, RSAKEYPATH);
	if (RSAKEYPATH[0] == 0) {
		TomlError(Config.get("rsa_key")->source(), "rsa_key must name a file");
	}

	if (const toml::table *World = TomlGetTable(Config, "world")) {
		TomlCheckKeys(*World, "[world]", {"name", "state"});
		TomlGetString(*World, "name", WorldName, sizeof(WorldName));
		char State[30];
		if (TomlGetString(*World, "state", State, sizeof(State))) {
			if (strcmp(State, "private") == 0) {
				PrivateWorld = true;
			} else if (strcmp(State, "public") == 0) {
				PrivateWorld = false;
			} else {
				TomlError(World->get("state")->source(), "state must be \"public\" or \"private\"");
			}
		}
	}

	if (const toml::table *Decay = TomlGetTable(Config, "decay")) {
		TomlCheckKeys(*Decay, "[decay]", {"monster_corpse", "player_corpse", "blood"});
		TomlGetFloat(*Decay, "monster_corpse", MonsterCorpseDecay, MIN_DECAY_MULTIPLIER, MAX_DECAY_MULTIPLIER);
		TomlGetFloat(*Decay, "player_corpse", PlayerCorpseDecay, MIN_DECAY_MULTIPLIER, MAX_DECAY_MULTIPLIER);
		TomlGetFloat(*Decay, "blood", BloodDecay, MIN_DECAY_MULTIPLIER, MAX_DECAY_MULTIPLIER);
	}

	if (const toml::table *Rates = TomlGetTable(Config, "rates")) {
		TomlCheckKeys(*Rates, "[rates]", {"experience", "magic_level", "skill"});
		TomlGetNumber(*Rates, "experience", ExperienceRate, MIN_RATE_MULTIPLIER, MAX_RATE_MULTIPLIER);
		TomlGetNumber(*Rates, "magic_level", MagicLevelRate, MIN_RATE_MULTIPLIER, MAX_RATE_MULTIPLIER);
		TomlGetNumber(*Rates, "skill", SkillRate, MIN_RATE_MULTIPLIER, MAX_RATE_MULTIPLIER);
	}

	if (const toml::table *Paths = TomlGetTable(Config, "paths")) {
		TomlCheckKeys(*Paths, "[paths]",
				{"bin", "data", "map", "origmap", "user", "log", "save"});
		ReadPath(*Paths, "bin", Folder, BINPATH);
		ReadPath(*Paths, "data", Folder, DATAPATH);
		ReadPath(*Paths, "map", Folder, MAPPATH);
		ReadPath(*Paths, "origmap", Folder, ORIGMAPPATH);
		ReadPath(*Paths, "user", Folder, USERPATH);
		ReadPath(*Paths, "log", Folder, LOGPATH);
		ReadPath(*Paths, "save", Folder, SAVEPATH);
	}

	if (const toml::array *QueryManagers = TomlGetArray(Config, "query_manager")) {
		for (const toml::node &Node : *QueryManagers) {
			ReadQueryManager(Node);
		}
	}
}

void ReadConfig(const char *FileName) {
	char DefaultFileName[4096];
	if (FileName == nullptr) {
		strcpy(DefaultFileName, "config.toml");
		if (getenv("HOME") != nullptr) {
			snprintf(DefaultFileName, sizeof(DefaultFileName), "%s/config.toml", getenv("HOME"));
		}
		if (getenv("home") != nullptr) {
			snprintf(DefaultFileName, sizeof(DefaultFileName), "%s/config.toml", getenv("home"));
		}
		FileName = DefaultFileName;
	}

	if (!FileExists(const_cast<char *>(FileName))) {
		static char ErrorText[4096 + 32];
		snprintf(ErrorText, sizeof(ErrorText), "cannot find config-file %s", FileName);
		throw static_cast<const char *>(ErrorText);
	}

	LoadConfig(FileName);
}
