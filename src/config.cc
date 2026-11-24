// config.cc: the server's settings. ReadConfig reads them from config.toml
// (data/config.toml in the repository is an example, converted from the
// original .tibia by tools/convert): the paths, the shared memory key, the
// debug level, the world's name and state, the beat, the gameplay settings
// that are not from the .tibia ([decay], [rates], [spawn], [loot], [death],
// [regeneration], [soul], [pvp], [idle], [lure], [logout], [vocations];
// config.hh), the query managers and the file of the RSA key (communication.cc's ReadPrivateKey
// reads it).
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
double SpawnSpeed;
double SpawnAmount;
std::vector<TMonsterSpawnRate> MonsterSpawnRates;
double LootChance;
std::vector<TMonsterLootRate> MonsterLootRates;
double DeathPenalty;
int DeathItemDrop;
bool DeathContainerDrop;
int HitpointRegeneration;
int ManaRegeneration;
int SoulInterval;
int SoulPromotedInterval;
int SoulDuration;
int SoulOfflineInterval;
int FightLock;
int KillLock;
int RedSkullDays;
int RedSkullKills[3];
int BanishmentKills[3];
int IdleMinutes;
bool LureLimit;
double LureRadius;
int LureFloors;
bool LogoutToTemple;
bool LogoutFightBlock;
int VocationGains[VOCATIONS][LEVEL_GAINS];
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

static const TMonsterSpawnRate *FindMonsterSpawnRate(const char *RaceKey) {
	for (const TMonsterSpawnRate &Rate : MonsterSpawnRates) {
		if (Rate.Race == RaceKey) {
			return &Rate;
		}
	}
	return nullptr;
}

double GetSpawnSpeed(const char *RaceKey) {
	const TMonsterSpawnRate *Rate = FindMonsterSpawnRate(RaceKey);
	return Rate != nullptr && Rate->Speed != 0.0 ? Rate->Speed : SpawnSpeed;
}

double GetSpawnAmount(const char *RaceKey) {
	const TMonsterSpawnRate *Rate = FindMonsterSpawnRate(RaceKey);
	return Rate != nullptr && Rate->Amount != 0.0 ? Rate->Amount : SpawnAmount;
}

// Value rounded, at least 1 and at most INT_MAX.
static int RoundSpawnValue(double Value) {
	Value = std::round(Value);
	if (Value < 1.0) {
		return 1;
	}
	if (Value >= static_cast<double>(INT_MAX)) {
		return INT_MAX;
	}
	return static_cast<int>(Value);
}

int ApplySpawnSpeed(int Regeneration, double Speed) {
	if (Speed == 1.0) {
		return Regeneration;
	}
	return RoundSpawnValue(Regeneration / Speed);
}

int ApplySpawnAmount(int Amount, double Multiplier) {
	if (Multiplier == 1.0) {
		return Amount;
	}
	return RoundSpawnValue(Amount * Multiplier);
}

double GetLootChance(const char *RaceKey) {
	for (const TMonsterLootRate &Rate : MonsterLootRates) {
		if (Rate.Race == RaceKey) {
			return Rate.Chance != 0.0 ? Rate.Chance : LootChance;
		}
	}
	return LootChance;
}

int ApplyLootChance(int Probability, double Multiplier) {
	if (Multiplier == 1.0 || Probability >= 999) {
		return Probability;
	}

	// The item drops for random(0, 999) <= Probability, in Probability + 1
	// of 1000 draws.
	double Draws = std::round((Probability + 1) * Multiplier);
	if (Draws < 1.0) {
		return 0;
	}
	if (Draws >= 1000.0) {
		return 999;
	}
	return static_cast<int>(Draws) - 1;
}

int DeathLoss(int Exp, int Percent) {
	if (DeathPenalty == 1.0) {
		return (int)((int64_t)Exp * Percent / 100);
	}

	double Loss = std::round(static_cast<double>(Exp) * Percent / 100.0 * DeathPenalty);
	if (Loss <= 0.0) {
		return 0;
	}
	if (Loss >= static_cast<double>(Exp)) {
		return Exp;
	}
	return static_cast<int>(Loss);
}

// Reads [pvp]'s table Key ({ day, week, month }, each 1 to MAX_MURDER_KILLS)
// into Kills, over what it holds.
static void ReadKillLimits(const toml::table &Pvp, const char *Key, int Kills[3]) {
	const toml::table *Limits = TomlGetTable(Pvp, Key);
	if (Limits == nullptr) {
		return;
	}
	std::string What = std::string("[pvp] ") + Key;
	TomlCheckKeys(*Limits, What.c_str(), {"day", "week", "month"});
	TomlGetNumber(*Limits, "day", Kills[0], 1, MAX_MURDER_KILLS);
	TomlGetNumber(*Limits, "week", Kills[1], 1, MAX_MURDER_KILLS);
	TomlGetNumber(*Limits, "month", Kills[2], 1, MAX_MURDER_KILLS);
}

// file:line:column of Where, for errors found after the config is read.
static std::string SourceLocation(const toml::source_region &Where, const char *FileName) {
	char Location[4096 + 64];
	snprintf(Location, sizeof(Location), "%s:%u:%u",
			Where.path != nullptr ? Where.path->c_str() : FileName, Where.begin.line, Where.begin.column);
	return Location;
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
	SpawnSpeed = 1.0;
	SpawnAmount = 1.0;
	MonsterSpawnRates.clear();
	LootChance = 1.0;
	MonsterLootRates.clear();
	DeathPenalty = 1.0;
	DeathItemDrop = 10;
	DeathContainerDrop = true;
	HitpointRegeneration = 1;
	ManaRegeneration = 1;
	SoulInterval = 120;
	SoulPromotedInterval = 15;
	SoulDuration = 240;
	SoulOfflineInterval = 900;
	FightLock = 60;
	KillLock = 900;
	RedSkullDays = 30;
	RedSkullKills[0] = 3;
	RedSkullKills[1] = 5;
	RedSkullKills[2] = 10;
	BanishmentKills[0] = 6;
	BanishmentKills[1] = 10;
	BanishmentKills[2] = 20;
	IdleMinutes = 15;
	LureLimit = true;
	LureRadius = 1.0;
	LureFloors = 2;
	LogoutToTemple = false;
	LogoutFightBlock = true;
	for (auto &Gains : VocationGains) {
		for (int &Gain : Gains) {
			Gain = -1;
		}
	}
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
			{"debug_level", "beat", "shm_key", "rsa_key", "world", "decay", "rates", "spawn", "loot", "death", "regeneration", "soul", "pvp", "idle", "lure", "logout", "vocations", "paths", "query_manager"});
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

	if (const toml::table *Spawn = TomlGetTable(Config, "spawn")) {
		TomlCheckKeys(*Spawn, "[spawn]", {"speed", "amount", "monsters"});
		TomlGetFloat(*Spawn, "speed", SpawnSpeed, MIN_SPAWN_MULTIPLIER, MAX_SPAWN_MULTIPLIER);
		TomlGetFloat(*Spawn, "amount", SpawnAmount, MIN_SPAWN_MULTIPLIER, MAX_SPAWN_MULTIPLIER);
		if (const toml::table *Monsters = TomlGetTable(*Spawn, "monsters")) {
			for (const auto &Entry : *Monsters) {
				std::string Race(Entry.first.str());
				if (!Entry.second.is_table()) {
					TomlError(Entry.second.source(), "%s must be a table", Race.c_str());
				}
				const toml::table &Monster = *Entry.second.as_table();
				std::string What = "[spawn.monsters] " + Race;
				TomlCheckKeys(Monster, What.c_str(), {"speed", "amount"});
				TMonsterSpawnRate Rate;
				Rate.Race = Race;
				Rate.Speed = 0.0;
				Rate.Amount = 0.0;
				TomlGetFloat(Monster, "speed", Rate.Speed, MIN_SPAWN_MULTIPLIER, MAX_SPAWN_MULTIPLIER);
				TomlGetFloat(Monster, "amount", Rate.Amount, MIN_SPAWN_MULTIPLIER, MAX_SPAWN_MULTIPLIER);
				Rate.Where = SourceLocation(Entry.first.source(), FileName);
				MonsterSpawnRates.push_back(Rate);
			}
		}
	}

	if (const toml::table *Loot = TomlGetTable(Config, "loot")) {
		TomlCheckKeys(*Loot, "[loot]", {"chance", "monsters"});
		TomlGetFloat(*Loot, "chance", LootChance, MIN_LOOT_MULTIPLIER, MAX_LOOT_MULTIPLIER);
		if (const toml::table *Monsters = TomlGetTable(*Loot, "monsters")) {
			for (const auto &Entry : *Monsters) {
				std::string Race(Entry.first.str());
				if (!Entry.second.is_table()) {
					TomlError(Entry.second.source(), "%s must be a table", Race.c_str());
				}
				const toml::table &Monster = *Entry.second.as_table();
				std::string What = "[loot.monsters] " + Race;
				TomlCheckKeys(Monster, What.c_str(), {"chance"});
				TMonsterLootRate Rate;
				Rate.Race = Race;
				Rate.Chance = 0.0;
				TomlGetFloat(Monster, "chance", Rate.Chance, MIN_LOOT_MULTIPLIER, MAX_LOOT_MULTIPLIER);
				Rate.Where = SourceLocation(Entry.first.source(), FileName);
				MonsterLootRates.push_back(Rate);
			}
		}
	}

	if (const toml::table *Death = TomlGetTable(Config, "death")) {
		TomlCheckKeys(*Death, "[death]", {"penalty", "item_drop", "container_drop"});
		TomlGetFloat(*Death, "penalty", DeathPenalty, MIN_DEATH_PENALTY, MAX_DEATH_PENALTY);
		TomlGetNumber(*Death, "item_drop", DeathItemDrop, 0, 100);
		TomlGetBool(*Death, "container_drop", DeathContainerDrop);
	}

	if (const toml::table *Regeneration = TomlGetTable(Config, "regeneration")) {
		TomlCheckKeys(*Regeneration, "[regeneration]", {"hp", "mana"});
		TomlGetNumber(*Regeneration, "hp", HitpointRegeneration, MIN_RATE_MULTIPLIER, MAX_RATE_MULTIPLIER);
		TomlGetNumber(*Regeneration, "mana", ManaRegeneration, MIN_RATE_MULTIPLIER, MAX_RATE_MULTIPLIER);
	}

	if (const toml::table *Soul = TomlGetTable(Config, "soul")) {
		TomlCheckKeys(*Soul, "[soul]", {"interval", "promoted_interval", "duration", "offline_interval"});
		TomlGetNumber(*Soul, "interval", SoulInterval, 1, MAX_SOUL_TIME);
		TomlGetNumber(*Soul, "promoted_interval", SoulPromotedInterval, 1, MAX_SOUL_TIME);
		TomlGetNumber(*Soul, "duration", SoulDuration, 1, MAX_SOUL_TIME);
		TomlGetNumber(*Soul, "offline_interval", SoulOfflineInterval, 1, MAX_SOUL_TIME);
		// A kill gives duration / interval soul points: at least one.
		for (const char *Key : {"interval", "promoted_interval"}) {
			int Interval = strcmp(Key, "interval") == 0 ? SoulInterval : SoulPromotedInterval;
			if (Interval > SoulDuration) {
				const toml::node *Node = Soul->get(Key) != nullptr ? Soul->get(Key) : Soul->get("duration");
				TomlError(Node->source(), "%s (%d) must not be longer than duration (%d)", Key, Interval,
						SoulDuration);
			}
		}
	}

	if (const toml::table *Pvp = TomlGetTable(Config, "pvp")) {
		TomlCheckKeys(*Pvp, "[pvp]", {"fight_lock", "kill_lock", "red_skull_days", "red_skull", "banishment"});
		TomlGetNumber(*Pvp, "fight_lock", FightLock, 1, MAX_LOCK_TIME);
		TomlGetNumber(*Pvp, "kill_lock", KillLock, 1, MAX_LOCK_TIME);
		TomlGetNumber(*Pvp, "red_skull_days", RedSkullDays, 1, 365);
		ReadKillLimits(*Pvp, "red_skull", RedSkullKills);
		ReadKillLimits(*Pvp, "banishment", BanishmentKills);
	}

	if (const toml::table *Idle = TomlGetTable(Config, "idle")) {
		TomlCheckKeys(*Idle, "[idle]", {"minutes"});
		TomlGetNumber(*Idle, "minutes", IdleMinutes, 1, MAX_IDLE_MINUTES);
	}

	if (const toml::table *Lure = TomlGetTable(Config, "lure")) {
		TomlCheckKeys(*Lure, "[lure]", {"limit", "radius", "floors"});
		TomlGetBool(*Lure, "limit", LureLimit);
		TomlGetFloat(*Lure, "radius", LureRadius, 1.0, MAX_LURE_RADIUS);
		TomlGetNumber(*Lure, "floors", LureFloors, 0, MAX_LURE_FLOORS);
	}

	if (const toml::table *Logout = TomlGetTable(Config, "logout")) {
		TomlCheckKeys(*Logout, "[logout]", {"to_temple", "fight_block"});
		TomlGetBool(*Logout, "to_temple", LogoutToTemple);
		TomlGetBool(*Logout, "fight_block", LogoutFightBlock);
	}

	if (const toml::table *Vocations = TomlGetTable(Config, "vocations")) {
		static const char *const Names[VOCATIONS] = {"none", "knight", "paladin", "sorcerer", "druid"};
		TomlCheckKeys(*Vocations, "[vocations]", {Names[0], Names[1], Names[2], Names[3], Names[4]});
		for (int Vocation = 0; Vocation < VOCATIONS; Vocation++) {
			const toml::table *Gains = TomlGetTable(*Vocations, Names[Vocation]);
			if (Gains == nullptr) {
				continue;
			}
			std::string What = std::string("[vocations] ") + Names[Vocation];
			TomlCheckKeys(*Gains, What.c_str(), {"hp", "mana", "capacity"});
			TomlGetNumber(*Gains, "hp", VocationGains[Vocation][0], 0, MAX_LEVEL_GAIN);
			TomlGetNumber(*Gains, "mana", VocationGains[Vocation][1], 0, MAX_LEVEL_GAIN);
			TomlGetNumber(*Gains, "capacity", VocationGains[Vocation][2], 0, MAX_LEVEL_GAIN);
		}
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

bool LureIsOriginal() {
	return LureLimit && LureRadius == 1.0 && LureFloors == 2;
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
