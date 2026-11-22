// test_config.cc: the round trip of the server config. The settings the game
// ends up with from a .tibia, through the old reader (ReadTibiaConfig), and
// from the config.toml patella-convert makes of it, through the game's
// LoadConfig, must be the same; LoadConfig must refuse what is wrong with the
// file and line.
//
//   convert_test_config
//     With made-up files in a temporary folder.
//   convert_test_config --original TIBIA CONFIG BASE
//     With the original .tibia and the committed config.toml: CONFIG must be
//     exactly what patella-convert --base BASE makes of TIBIA, and give the
//     same settings. Exit code 77 (skipped) when TIBIA is not there.
//
// The settings compare equal with one mapping: a path of the .tibia inside
// BASE (or relative, which was relative to the game's working folder) stands
// for the same path under the folder of config.toml.

#include <algorithm>
#include <climits>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

#include <unistd.h>

#include "config.hh"
#include "config_writer.hh"
#include "query.hh"
#include "tibia_config.hh"
#include "tomlfile.hh"
#include "tomlwrite.hh"

namespace fs = std::filesystem;

static int Failures = 0;

#define CHECK(Condition) \
	do { \
		if (!(Condition)) { \
			fprintf(stderr, "%s:%d: CHECK failed: %s\n", __FILE__, __LINE__, #Condition); \
			Failures++; \
		} \
	} while (0)

// Everything config.hh and query.hh hold, as the game sees it.
struct TSettings {
	std::vector<std::pair<std::string, std::string>> Values;
};

static const struct {
	const char *Name;
	char *Path;
} PathSettings[] = {
	{"BINPATH", BINPATH}, {"DATAPATH", DATAPATH}, {"LOGPATH", LOGPATH},
	{"MAPPATH", MAPPATH}, {"ORIGMAPPATH", ORIGMAPPATH}, {"SAVEPATH", SAVEPATH}, {"USERPATH", USERPATH},
};

// The path of the .tibia as config.toml stands for it, under Folder.
static std::string MapPath(const std::string &Path, std::string Base, const std::string &Folder) {
	while (Base.size() > 1 && Base.back() == '/') {
		Base.pop_back();
	}
	if (Path.empty()) {
		return Path;
	}
	if (Path[0] != '/') {
		return Folder + "/" + Path;
	}
	if (Base.empty()) {
		return Path;
	}
	if (Path == Base) {
		return Folder;
	}
	if (Path.compare(0, Base.size() + 1, Base + "/") == 0 && Path.size() > Base.size() + 1) {
		return Folder + Path.substr(Base.size());
	}
	return Path;
}

static TSettings Take(const std::string &Base = "", const std::string &Folder = "") {
	TSettings Settings;
	auto Add = [&](const std::string &Name, const std::string &Value) {
		Settings.Values.emplace_back(Name, Value);
	};
	for (const auto &Entry : PathSettings) {
		Add(Entry.Name, MapPath(Entry.Path, Base, Folder));
	}
	Add("SHMKey", std::to_string(SHMKey));
	Add("GamePort", std::to_string(GamePort));
	Add("GameAddress", GameAddress);
	Add("DebugLevel", std::to_string(DebugLevel));
	Add("PrivateWorld", std::to_string(PrivateWorld));
	Add("WorldType", std::to_string(WorldType));
	Add("WorldName", WorldName);
	Add("MaxPlayers", std::to_string(MaxPlayers));
	Add("MaxNewbies", std::to_string(MaxNewbies));
	Add("PremiumPlayerBuffer", std::to_string(PremiumPlayerBuffer));
	Add("PremiumNewbieBuffer", std::to_string(PremiumNewbieBuffer));
	Add("Beat", std::to_string(Beat));
	Add("RebootTime", std::to_string(RebootTime));
	Add("MonsterCorpseDecay", std::to_string(MonsterCorpseDecay));
	Add("PlayerCorpseDecay", std::to_string(PlayerCorpseDecay));
	Add("BloodDecay", std::to_string(BloodDecay));
	Add("ExperienceRate", std::to_string(ExperienceRate));
	Add("MagicLevelRate", std::to_string(MagicLevelRate));
	Add("SkillRate", std::to_string(SkillRate));
	Add("SpawnSpeed", std::to_string(SpawnSpeed));
	Add("SpawnAmount", std::to_string(SpawnAmount));
	for (const TMonsterSpawnRate &Rate : MonsterSpawnRates) {
		Add("MonsterSpawnRates[" + Rate.Race + "]", std::to_string(Rate.Speed) + "," + std::to_string(Rate.Amount));
	}
	Add("LootChance", std::to_string(LootChance));
	for (const TMonsterLootRate &Rate : MonsterLootRates) {
		Add("MonsterLootRates[" + Rate.Race + "]", std::to_string(Rate.Chance));
	}
	Add("DeathPenalty", std::to_string(DeathPenalty));
	Add("DeathItemDrop", std::to_string(DeathItemDrop));
	Add("DeathContainerDrop", std::to_string(DeathContainerDrop));
	Add("HitpointRegeneration", std::to_string(HitpointRegeneration));
	Add("ManaRegeneration", std::to_string(ManaRegeneration));
	Add("SoulInterval", std::to_string(SoulInterval));
	Add("SoulPromotedInterval", std::to_string(SoulPromotedInterval));
	Add("SoulDuration", std::to_string(SoulDuration));
	Add("SoulOfflineInterval", std::to_string(SoulOfflineInterval));
	Add("FightLock", std::to_string(FightLock));
	Add("KillLock", std::to_string(KillLock));
	Add("RedSkullDays", std::to_string(RedSkullDays));
	for (int i = 0; i < 3; i++) {
		Add("RedSkullKills[" + std::to_string(i) + "]", std::to_string(RedSkullKills[i]));
		Add("BanishmentKills[" + std::to_string(i) + "]", std::to_string(BanishmentKills[i]));
	}
	Add("IdleMinutes", std::to_string(IdleMinutes));
	Add("NumberOfQueryManagers", std::to_string(NumberOfQueryManagers));
	for (int i = 0; i < NumberOfQueryManagers && i < MAX_QUERY_MANAGERS; i++) {
		std::string Name = "QUERY_MANAGER[" + std::to_string(i) + "].";
		Add(Name + "Host", QUERY_MANAGER[i].Host);
		Add(Name + "Port", std::to_string(QUERY_MANAGER[i].Port));
		Add(Name + "Password", QUERY_MANAGER[i].Password);
	}
	return Settings;
}

// Fills every setting with garbage, so that a reader that leaves one alone is
// caught.
static void Poison() {
	for (const auto &Entry : PathSettings) {
		memset(Entry.Path, 'X', 100);
		Entry.Path[100] = 0;
	}
	SHMKey = GamePort = DebugLevel = MaxPlayers = MaxNewbies = 0x5A5A5A5A;
	PremiumPlayerBuffer = PremiumNewbieBuffer = Beat = RebootTime = NumberOfQueryManagers = 0x5A5A5A5A;
	MonsterCorpseDecay = PlayerCorpseDecay = BloodDecay = 90.5;
	ExperienceRate = MagicLevelRate = SkillRate = 0x5A5A5A5A;
	SpawnSpeed = SpawnAmount = 90.5;
	MonsterSpawnRates.assign(1, TMonsterSpawnRate{"poison", 90.5, 90.5, "poison"});
	LootChance = 90.5;
	MonsterLootRates.assign(1, TMonsterLootRate{"poison", 90.5, "poison"});
	DeathPenalty = 90.5;
	DeathItemDrop = 0x5A5A5A5A;
	DeathContainerDrop = false;
	HitpointRegeneration = ManaRegeneration = 0x5A5A5A5A;
	SoulInterval = SoulPromotedInterval = SoulDuration = SoulOfflineInterval = 0x5A5A5A5A;
	FightLock = KillLock = RedSkullDays = IdleMinutes = 0x5A5A5A5A;
	for (int i = 0; i < 3; i++) {
		RedSkullKills[i] = BanishmentKills[i] = 0x5A5A5A5A;
	}
	strcpy(GameAddress, "9.9.9.9");
	PrivateWorld = true;
	WorldType = PVP_ENFORCED;
	strcpy(WorldName, "Poison");
	for (TQueryManagerSettings &Settings : QUERY_MANAGER) {
		strcpy(Settings.Host, "poison");
		Settings.Port = -1;
		strcpy(Settings.Password, "poison");
	}
}

static std::string Printable(const std::string &Text) {
	std::string Out;
	for (unsigned char c : Text) {
		if (c >= 0x20 && c < 0x7F && c != '\\') {
			Out += static_cast<char>(c);
		} else {
			char Code[8];
			snprintf(Code, sizeof(Code), "\\x%02X", c);
			Out += Code;
		}
	}
	return Out;
}

// Prints the differences; returns whether there were none.
static bool Same(const TSettings &Expected, const TSettings &Actual, const char *What) {
	bool Equal = true;
	size_t Count = std::max(Expected.Values.size(), Actual.Values.size());
	for (size_t i = 0; i < Count; i++) {
		std::string Name = i < Expected.Values.size() ? Expected.Values[i].first : Actual.Values[i].first;
		std::string A = i < Expected.Values.size() ? Expected.Values[i].second : "(none)";
		std::string B = i < Actual.Values.size() ? Actual.Values[i].second : "(none)";
		if (A != B) {
			fprintf(stderr, "%s: %s: .tibia gives \"%s\", config.toml \"%s\"\n", What, Name.c_str(),
					Printable(A).c_str(), Printable(B).c_str());
			Equal = false;
		}
	}
	if (!Equal) {
		Failures++;
	}
	return Equal;
}

static void WriteFile(const fs::path &Path, const std::string &Text) {
	fs::create_directories(Path.parent_path());
	std::ofstream Out(Path, std::ios::binary);
	Out << Text;
}

static std::string ReadFile(const fs::path &Path) {
	std::ifstream In(Path, std::ios::binary);
	std::stringstream Text;
	Text << In.rdbuf();
	return Text.str();
}

// The error LoadConfig throws for Text in the file Path, or "" for none.
static std::string LoadError(const fs::path &Path, const std::string &Text) {
	WriteFile(Path, Text);
	try {
		LoadConfig(Path.c_str());
	} catch (const char *Error) {
		return Error;
	}
	return "";
}

// Converts a .tibia text and loads the result. Returns the config.toml text.
static std::string RoundTrip(const fs::path &Folder, const char *Name, const std::string &Tibia, const char *Base,
		const std::vector<std::string> &Dropped) {
	fs::path TibiaFile = Folder / (std::string(Name) + ".tibia");
	fs::path ConfigFile = Folder / Name / "config.toml";
	WriteFile(TibiaFile, Tibia);

	Poison();
	TTibiaConfigExtras Extras;
	try {
		ReadTibiaConfig(TibiaFile.c_str(), Extras);
	} catch (const char *Error) {
		fprintf(stderr, "%s: ReadTibiaConfig: %s\n", Name, Error);
		Failures++;
		return "";
	}
	CHECK(Extras.Dropped == Dropped);
	TSettings Expected = Take(Base != nullptr ? Base : "", (Folder / Name).string());
	std::string Text = ConfigToToml(Extras, Base);
	WriteFile(ConfigFile, Text);

	Poison();
	try {
		LoadConfig(ConfigFile.c_str());
	} catch (const char *Error) {
		fprintf(stderr, "%s: LoadConfig: %s\n%s", Name, Error, Text.c_str());
		Failures++;
		return Text;
	}
	Same(Expected, Take(), Name);
	return Text;
}

static void TestRoundTrips(const fs::path &Folder) {
	// Every entry the old reader knows, the dropped ones included; paths inside
	// and outside the base, with a trailing '/', a quote, the base itself and a
	// relative one; Latin-1 text and escapes.
	std::string Full =
		"# Tibia - Graphical Multi-User-Dungeon\n"
		"# made-up .tibia\n"
		"BINPATH     = \"/inst/bin\"\n"
		"MAPPATH     = \"/inst/map/\"\n"
		"ORIGMAPPATH = \"/inst\"\n"
		"DATAPATH    = \"/elsewhere/dat\"\n"
		"USERPATH    = \"/inst/u s\\\"r\"\n"
		"LOGPATH     = \"/institute/log\"\n"
		"SAVEPATH    = \"/inst/save\"\n"
		"MONSTERPATH = \"relative/mon\"\n"
		"NPCPATH     = \"/inst/n\xe4pc\"\n"
		"SHM = 4711\n"
		"DebugLevel = -3\n"
		"Beat = 100\n"
		"AdminPort = 7172\n"
		"QueryManagerPort = 1\n"
		"AdminAddress = \"10.0.0.1\"\n"
		"QueryManagerAddress = \"10.0.0.2\"\n"
		"QueryManagerAdminPW = \"a\"\n"
		"QueryManagerGamePW = \"b\"\n"
		"QueryManagerWebPW = \"c\"\n"
		"AdminDatabase = (mysql,\"db\",\"login\",\"pw\",\"host\",\"3306\")\n"
		"VolatileDatabase = (mysql,\"db\",\"login\",\"pw\",\"host\",\"3306\")\n"
		"WebDatabase = (mysql,\"db\",\"login\",\"pw\",\"host\",\"3306\")\n"
		"ForumDatabase = (mysql,\"db\",\"login\",\"pw\",\"host\",\"3306\")\n"
		"ManagerDatabase = (mysql,\"db\",\"login\",\"pw\",\"host\",\"3306\")\n"
		"QueryManager = {(\"127.0.0.1\",7173,\"a\\\"b\\\\c\"),(\"qm.example\",-5,\"\"),(\"h\",7175,\"12345678\")}\n"
		"World = \"Z\xe4nera\"\n"
		"State = private\n";
	std::string Text = RoundTrip(Folder, "full", Full, "/inst/", {"monsterpath", "npcpath", "adminport",
		"querymanagerport", "adminaddress",
		"querymanageraddress", "querymanageradminpw", "querymanagergamepw", "querymanagerwebpw", "admindatabase",
		"volatiledatabase", "webdatabase", "forumdatabase", "managerdatabase"});
	CHECK(Text.find("bin = \"bin\"") != std::string::npos);
	CHECK(Text.find("map = \"map/\"") != std::string::npos);
	CHECK(Text.find("origmap = \".\"") != std::string::npos);
	CHECK(Text.find("data = \"/elsewhere/dat\"") != std::string::npos);
	CHECK(Text.find("log = \"/institute/log\"") != std::string::npos);
	CHECK(Text.find("user = \"u s\\\"r\"") != std::string::npos);
	CHECK(Text.find("npc =") == std::string::npos);
	CHECK(Text.find("monster =") == std::string::npos);
	CHECK(Text.find("name = \"Z\xc3\xa4nera\"") != std::string::npos);
	CHECK(Text.find("state = \"private\"") != std::string::npos);
	CHECK(Text.find("password = \"a\\\"b\\\\c\"") != std::string::npos);
	CHECK(Text.find("adminport") == std::string::npos);

	// Without a base every path stays as written.
	RoundTrip(Folder, "nobase", "BINPATH = \"/inst/bin\"\nWorld = \"W\"\nState = public\n", nullptr, {});

	// What the .tibia leaves out keeps its default, also in config.toml.
	RoundTrip(Folder, "minimal", "World = \"Minimal\"\n", "/game", {});
	RoundTrip(Folder, "empty", "# nothing\n", "/game", {});
	// Any state but "private" is public.
	RoundTrip(Folder, "state", "State = closed\n", "/game", {});

	// Ten query managers are the most there can be.
	std::string Ten = "QueryManager = {";
	for (int i = 0; i < MAX_QUERY_MANAGERS; i++) {
		Ten += std::string(i == 0 ? "" : ",") + "(\"h" + std::to_string(i) + "\"," + std::to_string(7000 + i) + ",\"p\")";
	}
	RoundTrip(Folder, "ten", Ten + "}\n", "/game", {});

	// An empty config.toml gives the defaults, as an empty .tibia does.
	Poison();
	CHECK(LoadError(Folder / "defaults.toml", "") == "");
	TSettings Defaults = Take();
	Poison();
	SetConfigDefaults();
	Same(Take(), Defaults, "defaults");
	CHECK(Beat == 200 && DebugLevel == 1 && RebootTime == 540 && strcmp(GameAddress, "0.0.0.0") == 0);
	CHECK(MonsterCorpseDecay == 1.0 && PlayerCorpseDecay == 1.0 && BloodDecay == 1.0);
	CHECK(ExperienceRate == 1 && MagicLevelRate == 1 && SkillRate == 1);
}

static void TestRates(const fs::path &Folder) {
	// Each key of [rates] sets its own multiplier; one left out keeps 1.
	Poison();
	CHECK(LoadError(Folder / "rates.toml", "[rates]\nexperience = 5\nmagic_level = 3\nskill = 2\n") == "");
	CHECK(ExperienceRate == 5 && MagicLevelRate == 3 && SkillRate == 2);
	Poison();
	CHECK(LoadError(Folder / "rates.toml", "[rates]\nskill = 1000\n") == "");
	CHECK(ExperienceRate == 1 && MagicLevelRate == 1 && SkillRate == 1000);

	// MultiplyRate: 1 keeps the amount, more multiplies it, up to INT_MAX.
	CHECK(MultiplyRate(5, 1) == 5);
	CHECK(MultiplyRate(5, 5) == 25);
	CHECK(MultiplyRate(0, 5) == 0);
	CHECK(MultiplyRate(1, 1000) == 1000);
	CHECK(MultiplyRate(INT_MAX, 1) == INT_MAX);
	CHECK(MultiplyRate(INT_MAX / 2 + 1, 2) == INT_MAX);
	CHECK(MultiplyRate(3000000, 1000) == INT_MAX);
}

static void TestSpawn(const fs::path &Folder) {
	// The global multipliers, and the races' own ones; a race's field left out
	// is the global one, a race not named gets the global ones.
	Poison();
	CHECK(LoadError(Folder / "spawn.toml", "[spawn]\nspeed = 2.0\namount = 3\n"
			"[spawn.monsters]\nrat = { speed = 4.0 }\ndragon = { speed = 0.5, amount = 2.0 }\n"
			"[spawn.monsters.orc]\namount = 0.5\n") == "");
	CHECK(SpawnSpeed == 2.0 && SpawnAmount == 3.0);
	CHECK(MonsterSpawnRates.size() == 3);
	CHECK(GetSpawnSpeed("rat") == 4.0 && GetSpawnAmount("rat") == 3.0);
	CHECK(GetSpawnSpeed("dragon") == 0.5 && GetSpawnAmount("dragon") == 2.0);
	CHECK(GetSpawnSpeed("orc") == 2.0 && GetSpawnAmount("orc") == 0.5);
	CHECK(GetSpawnSpeed("troll") == 2.0 && GetSpawnAmount("troll") == 3.0);
	for (const TMonsterSpawnRate &Rate : MonsterSpawnRates) {
		if (Rate.Race == "rat") {
			CHECK(Rate.Where.find("spawn.toml:5:1") != std::string::npos);
		}
	}
	Poison();
	CHECK(LoadError(Folder / "spawn.toml", "[spawn.monsters]\nrat = { amount = 2.0 }\n") == "");
	CHECK(SpawnSpeed == 1.0 && SpawnAmount == 1.0);
	CHECK(GetSpawnSpeed("rat") == 1.0 && GetSpawnAmount("rat") == 2.0);

	// ApplySpawnSpeed divides the time, ApplySpawnAmount multiplies the
	// amount; rounded, at least 1, at most INT_MAX; 1 keeps the value.
	CHECK(ApplySpawnSpeed(600, 1.0) == 600);
	CHECK(ApplySpawnSpeed(600, 4.0) == 150);
	CHECK(ApplySpawnSpeed(600, 0.5) == 1200);
	CHECK(ApplySpawnSpeed(10, 4.0) == 3);
	CHECK(ApplySpawnSpeed(1, 4.0) == 1);
	CHECK(ApplySpawnSpeed(INT_MAX, 0.01) == INT_MAX);
	CHECK(ApplySpawnAmount(3, 1.0) == 3);
	CHECK(ApplySpawnAmount(3, 2.0) == 6);
	CHECK(ApplySpawnAmount(3, 0.5) == 2);
	CHECK(ApplySpawnAmount(1, 0.01) == 1);
	CHECK(ApplySpawnAmount(INT_MAX, 1000.0) == INT_MAX);
}

static void TestLoot(const fs::path &Folder) {
	// The global chance and the races' own ones.
	Poison();
	CHECK(LoadError(Folder / "loot.toml", "[loot]\nchance = 2\n[loot.monsters]\nrat = { chance = 4.0 }\n") == "");
	CHECK(LootChance == 2.0 && MonsterLootRates.size() == 1);
	CHECK(GetLootChance("rat") == 4.0 && GetLootChance("troll") == 2.0);
	CHECK(MonsterLootRates[0].Where.find("loot.toml:4:1") != std::string::npos);
	Poison();
	CHECK(LoadError(Folder / "loot.toml", "[loot.monsters]\nrat = {}\n") == "");
	CHECK(LootChance == 1.0 && GetLootChance("rat") == 1.0);

	// ApplyLootChance multiplies the chance (Probability + 1 in 1000), from 0
	// to 999; 1 and a certain item keep Probability.
	CHECK(ApplyLootChance(99, 1.0) == 99);
	CHECK(ApplyLootChance(99, 2.0) == 199);
	CHECK(ApplyLootChance(99, 0.5) == 49);
	CHECK(ApplyLootChance(0, 2.0) == 1);
	CHECK(ApplyLootChance(0, 0.01) == 0);
	CHECK(ApplyLootChance(499, 3.0) == 999);
	CHECK(ApplyLootChance(999, 0.5) == 999);
	CHECK(ApplyLootChance(1000, 2.0) == 1000);
}

static void TestDeath(const fs::path &Folder) {
	Poison();
	CHECK(LoadError(Folder / "death.toml", "[death]\npenalty = 0.5\nitem_drop = 0\ncontainer_drop = false\n") == "");
	CHECK(DeathPenalty == 0.5 && DeathItemDrop == 0 && !DeathContainerDrop);
	Poison();
	CHECK(LoadError(Folder / "death.toml", "[death]\npenalty = 0\n") == "");
	CHECK(DeathPenalty == 0.0 && DeathItemDrop == 10 && DeathContainerDrop);

	// DeathLoss: at 1 the original integer formula, otherwise rounded, at most
	// all.
	DeathPenalty = 1.0;
	CHECK(DeathLoss(1000, 10) == 100);
	CHECK(DeathLoss(999, 7) == 69);
	CHECK(DeathLoss(INT_MAX, 10) == INT_MAX / 10);
	DeathPenalty = 0.5;
	CHECK(DeathLoss(1000, 10) == 50);
	CHECK(DeathLoss(999, 7) == 35);
	DeathPenalty = 0.0;
	CHECK(DeathLoss(1000, 10) == 0);
	DeathPenalty = 10.0;
	CHECK(DeathLoss(1000, 10) == 1000);
	CHECK(DeathLoss(1000, 7) == 700);
	DeathPenalty = 1.0;
}

static void TestRegeneration(const fs::path &Folder) {
	Poison();
	CHECK(LoadError(Folder / "regeneration.toml", "[regeneration]\nhp = 2\nmana = 3\n") == "");
	CHECK(HitpointRegeneration == 2 && ManaRegeneration == 3);
	Poison();
	CHECK(LoadError(Folder / "regeneration.toml", "[regeneration]\nmana = 1000\n") == "");
	CHECK(HitpointRegeneration == 1 && ManaRegeneration == 1000);
}

static void TestSoul(const fs::path &Folder) {
	Poison();
	CHECK(LoadError(Folder / "soul.toml",
			"[soul]\ninterval = 30\npromoted_interval = 5\nduration = 600\noffline_interval = 300\n") == "");
	CHECK(SoulInterval == 30 && SoulPromotedInterval == 5 && SoulDuration == 600 && SoulOfflineInterval == 300);
	Poison();
	CHECK(LoadError(Folder / "soul.toml", "[soul]\noffline_interval = 60\n") == "");
	CHECK(SoulInterval == 120 && SoulPromotedInterval == 15 && SoulDuration == 240 && SoulOfflineInterval == 60);
	Poison();
	CHECK(LoadError(Folder / "soul.toml", "[soul]\ninterval = 240\npromoted_interval = 240\n") == "");
}

static void TestPvp(const fs::path &Folder) {
	Poison();
	CHECK(LoadError(Folder / "pvp.toml", "[pvp]\nfight_lock = 30\nkill_lock = 600\nred_skull_days = 7\n"
			"red_skull = { day = 2, week = 4, month = 8 }\nbanishment = { day = 4, week = 8, month = 16 }\n") == "");
	CHECK(FightLock == 30 && KillLock == 600 && RedSkullDays == 7);
	CHECK(RedSkullKills[0] == 2 && RedSkullKills[1] == 4 && RedSkullKills[2] == 8);
	CHECK(BanishmentKills[0] == 4 && BanishmentKills[1] == 8 && BanishmentKills[2] == 16);
	// A limit left out keeps its default.
	Poison();
	CHECK(LoadError(Folder / "pvp.toml", "[pvp]\nred_skull = { week = 7 }\n") == "");
	CHECK(FightLock == 60 && KillLock == 900 && RedSkullDays == 30);
	CHECK(RedSkullKills[0] == 3 && RedSkullKills[1] == 7 && RedSkullKills[2] == 10);
	CHECK(BanishmentKills[0] == 6 && BanishmentKills[1] == 10 && BanishmentKills[2] == 20);

	Poison();
	CHECK(LoadError(Folder / "idle.toml", "[idle]\nminutes = 30\n") == "");
	CHECK(IdleMinutes == 30);
}

static void TestDecay(const fs::path &Folder) {
	// Each key of [decay] sets its own multiplier, written with or without a
	// fraction; one left out keeps 1.
	Poison();
	CHECK(LoadError(Folder / "decay.toml", "[decay]\nmonster_corpse = 2.0\nplayer_corpse = 0.5\nblood = 3\n") == "");
	CHECK(MonsterCorpseDecay == 2.0 && PlayerCorpseDecay == 0.5 && BloodDecay == 3.0);
	Poison();
	CHECK(LoadError(Folder / "decay.toml", "[decay]\nblood = 1.5\n") == "");
	CHECK(MonsterCorpseDecay == 1.0 && PlayerCorpseDecay == 1.0 && BloodDecay == 1.5);
	Poison();
	CHECK(LoadError(Folder / "decay.toml", "[decay]\nmonster_corpse = 0.01\nplayer_corpse = 1000\n") == "");
	CHECK(MonsterCorpseDecay == 0.01 && PlayerCorpseDecay == 1000.0 && BloodDecay == 1.0);

	// A multiplier of 1 changes nothing, whatever the time.
	CHECK(MultiplyDecayTime(1800, 1.0) == 1800);
	CHECK(MultiplyDecayTime(0, 1.0) == 0);
	CHECK(MultiplyDecayTime(-1, 1.0) == -1);
	// The times of items.toml's corpses and pools.
	CHECK(MultiplyDecayTime(1800, 2.0) == 3600);
	CHECK(MultiplyDecayTime(1200, 0.5) == 600);
	CHECK(MultiplyDecayTime(120, 1.5) == 180);
	CHECK(MultiplyDecayTime(10, 2.0) == 20);
	// Rounded to whole seconds, at least 1 and at most INT_MAX.
	CHECK(MultiplyDecayTime(10, 0.25) == 3);
	CHECK(MultiplyDecayTime(10, 0.24) == 2);
	CHECK(MultiplyDecayTime(10, 0.01) == 1);
	CHECK(MultiplyDecayTime(1, 0.01) == 1);
	CHECK(MultiplyDecayTime(INT_MAX, 1000.0) == INT_MAX);
	CHECK(MultiplyDecayTime(INT_MAX / 2 + 1, 2.0) == INT_MAX);
	// A time of 0 or less is not a time to multiply.
	CHECK(MultiplyDecayTime(0, 2.0) == 0);
	CHECK(MultiplyDecayTime(-5, 2.0) == -5);
}

static void TestErrors(const fs::path &Folder) {
	fs::path File = Folder / "errors" / "config.toml";
	std::string Name = File.string();
	auto Expect = [&](const std::string &Text, int Line, const char *What) {
		std::string Error = LoadError(File, Text);
		std::string Where = Name + ":" + std::to_string(Line) + ":";
		bool Ok = Error.compare(0, Where.size(), Where) == 0 && Error.find(What) != std::string::npos;
		if (!Ok) {
			fprintf(stderr, "for:\n%s\nexpected \"%s...%s\", got \"%s\"\n", Text.c_str(), Where.c_str(), What,
					Error.c_str());
			Failures++;
		}
	};
	Expect("beat = 50\nbeet = 50\n", 2, "unknown key \"beet\" in the top level");
	Expect("[paths]\nbin = \"bin\"\nbinn = \"a\"\n", 3, "unknown key \"binn\" in [paths]");
	Expect("[world]\nworld = \"x\"\n", 2, "unknown key \"world\" in [world]");
	Expect("beat = \"50\"\n", 1, "beat must be an integer");
	Expect("debug_level = 1.5\n", 1, "debug_level must be an integer");
	Expect("\nshm_key = 3000000000\n", 2, "shm_key must lie between");
	Expect("world = 5\n", 1, "world must be a table");
	Expect("paths = \"/game\"\n", 1, "paths must be a table");
	Expect("query_manager = 1\n", 1, "query_manager must be an array");
	Expect("query_manager = [1]\n", 1, "must be a table");
	Expect("[world]\nstate = \"secret\"\n", 2, "state must be \"public\" or \"private\"");
	Expect("[world]\nstate = true\n", 2, "state must be a string");
	Expect("[world]\nname = \"abcdefghijklmnopqrstuvwxyz0123\"\n", 2, "name is longer than 29 characters");
	Expect("[world]\nname = \"\xc4\x80\"\n", 2, "name has a character outside Latin-1");
	Expect("[world]\nname = \"a\\u0000b\"\n", 2, "name has a zero character");
	Expect("[[query_manager]]\nhost = \"h\"\nport = 1\npassword = \"123456789\"\n", 4,
			"key for disguising password is too short");
	Expect("[[query_manager]]\nhost = \"h\"\npassword = \"p\"\n", 1, "an entry of query_manager needs port");
	Expect("[[query_manager]]\nhost = \"h\"\nport = 1\npassword = \"p\"\nextra = 1\n", 5,
			"unknown key \"extra\" in [[query_manager]]");
	std::string Eleven;
	for (int i = 0; i <= MAX_QUERY_MANAGERS; i++) {
		Eleven += "[[query_manager]]\nhost = \"h\"\nport = 1\npassword = \"p\"\n";
	}
	Expect(Eleven, 4 * MAX_QUERY_MANAGERS + 1, "more than 10 query managers");
	Expect("decay = 2.0\n", 1, "decay must be a table");
	Expect("[decay]\ncorpse = 2.0\n", 2, "unknown key \"corpse\" in [decay]");
	Expect("[decay]\nblood = \"2\"\n", 2, "blood must be a number");
	Expect("[decay]\nmonster_corpse = true\n", 2, "monster_corpse must be a number");
	Expect("[decay]\nmonster_corpse = 0\n", 2, "monster_corpse must lie between 0.01 and 1000");
	Expect("[decay]\nplayer_corpse = -2.0\n", 2, "player_corpse must lie between 0.01 and 1000");
	Expect("[decay]\n\nblood = 1000.5\n", 3, "blood must lie between 0.01 and 1000");
	Expect("[decay]\nblood = inf\n", 2, "blood must lie between 0.01 and 1000");
	Expect("[decay]\nblood = nan\n", 2, "blood must lie between 0.01 and 1000");
	Expect("rates = 2\n", 1, "rates must be a table");
	Expect("[rates]\nexp = 2\n", 2, "unknown key \"exp\" in [rates]");
	Expect("[rates]\nexperience = 1.5\n", 2, "experience must be an integer");
	Expect("[rates]\nmagic_level = 0\n", 2, "magic_level must lie between 1 and 1000");
	Expect("[rates]\nskill = 1001\n", 2, "skill must lie between 1 and 1000");
	Expect("spawn = 2\n", 1, "spawn must be a table");
	Expect("[spawn]\nrate = 2.0\n", 2, "unknown key \"rate\" in [spawn]");
	Expect("[spawn]\nspeed = 0\n", 2, "speed must lie between 0.01 and 1000");
	Expect("[spawn]\namount = 1001\n", 2, "amount must lie between 0.01 and 1000");
	Expect("[spawn]\nmonsters = 4\n", 2, "monsters must be a table");
	Expect("[spawn.monsters]\nrat = 4.0\n", 2, "rat must be a table");
	Expect("[spawn.monsters]\nrat = { rate = 4.0 }\n", 2, "unknown key \"rate\" in [spawn.monsters] rat");
	Expect("[spawn.monsters]\nrat = { speed = 0 }\n", 2, "speed must lie between 0.01 and 1000");
	Expect("[spawn.monsters]\nrat = { amount = \"2\" }\n", 2, "amount must be a number");
	Expect("loot = 2\n", 1, "loot must be a table");
	Expect("[loot]\nrate = 2.0\n", 2, "unknown key \"rate\" in [loot]");
	Expect("[loot]\nchance = 0\n", 2, "chance must lie between 0.01 and 1000");
	Expect("[loot.monsters]\nrat = 4.0\n", 2, "rat must be a table");
	Expect("[loot.monsters]\nrat = { speed = 4.0 }\n", 2, "unknown key \"speed\" in [loot.monsters] rat");
	Expect("[loot.monsters]\nrat = { chance = 1001 }\n", 2, "chance must lie between 0.01 and 1000");
	Expect("[death]\nloss = 1\n", 2, "unknown key \"loss\" in [death]");
	Expect("[death]\npenalty = -0.5\n", 2, "penalty must lie between 0 and 10");
	Expect("[death]\nitem_drop = 101\n", 2, "item_drop must lie between 0 and 100");
	Expect("[death]\nitem_drop = 5.5\n", 2, "item_drop must be an integer");
	Expect("[death]\ncontainer_drop = 1\n", 2, "container_drop must be true or false");
	Expect("[regeneration]\nspeed = 2\n", 2, "unknown key \"speed\" in [regeneration]");
	Expect("[regeneration]\nhp = 0\n", 2, "hp must lie between 1 and 1000");
	Expect("[regeneration]\nmana = 1.5\n", 2, "mana must be an integer");
	Expect("[soul]\nrate = 2\n", 2, "unknown key \"rate\" in [soul]");
	Expect("[soul]\ninterval = 0\n", 2, "interval must lie between 1 and 86400");
	Expect("[soul]\nduration = 86401\n", 2, "duration must lie between 1 and 86400");
	Expect("[soul]\ninterval = 300\n", 2, "interval (300) must not be longer than duration (240)");
	Expect("[soul]\n\nduration = 100\n", 3, "interval (120) must not be longer than duration (100)");
	Expect("[soul]\nduration = 30\npromoted_interval = 60\ninterval = 30\n", 3,
			"promoted_interval (60) must not be longer than duration (30)");
	Expect("[pvp]\nwhite_skull = 1\n", 2, "unknown key \"white_skull\" in [pvp]");
	Expect("[pvp]\nfight_lock = 0\n", 2, "fight_lock must lie between 1 and 86400");
	Expect("[pvp]\nred_skull_days = 366\n", 2, "red_skull_days must lie between 1 and 365");
	Expect("[pvp]\nred_skull = 3\n", 2, "red_skull must be a table");
	Expect("[pvp]\nbanishment = { year = 1 }\n", 2, "unknown key \"year\" in [pvp] banishment");
	Expect("[pvp]\nbanishment = { month = 21 }\n", 2, "month must lie between 1 and 20");
	Expect("[idle]\nseconds = 60\n", 2, "unknown key \"seconds\" in [idle]");
	Expect("[idle]\nminutes = 0\n", 2, "minutes must lie between 1 and 1440");
	Expect("rsa_key = \"\"\n", 1, "rsa_key must name a file");
	Expect("\nrsa_key = 5\n", 2, "rsa_key must be a string");
	Expect("beat = 50\nbeat = 60\n", 2, "");
	Expect("beat =\n", 1, "");

	std::string Missing;
	try {
		ReadConfig((Folder / "missing.toml").c_str());
	} catch (const char *Error) {
		Missing = Error;
	}
	CHECK(Missing == "cannot find config-file " + (Folder / "missing.toml").string());
}

static void TestFileNames(const fs::path &Folder) {
	// Without --config: config.toml in $HOME, or in $home if that is set too.
	WriteFile(Folder / "home" / "config.toml", "beat = 77\n");
	WriteFile(Folder / "home2" / "config.toml", "beat = 78\n");
	setenv("HOME", (Folder / "home").c_str(), 1);
	unsetenv("home");
	ReadConfig(nullptr);
	CHECK(Beat == 77);
	setenv("home", (Folder / "home2").c_str(), 1);
	ReadConfig(nullptr);
	CHECK(Beat == 78);
	unsetenv("home");

	// A relative --config: its paths are relative to its folder, as a full path.
	WriteFile(Folder / "rel" / "sub" / "config.toml",
		"[paths]\ndata = \"dat\"\nmap = \".\"\nsave = \"../save/\"\nlog = \"/abs/log\"\nuser = \"\"\n");
	char Here[4096];
	CHECK(getcwd(Here, sizeof(Here)) != nullptr);
	CHECK(chdir((Folder / "rel").c_str()) == 0);
	Poison();
	ReadConfig("./sub/../sub/config.toml");
	std::string Sub = fs::weakly_canonical(Folder / "rel" / "sub").string();
	CHECK(DATAPATH == Sub + "/dat");
	CHECK(MAPPATH == Sub);
	CHECK(SAVEPATH == Sub + "/../save/");
	CHECK(strcmp(LOGPATH, "/abs/log") == 0);
	CHECK(USERPATH[0] == 0);
	CHECK(BINPATH[0] == 0);
	// Without rsa_key: key.pem in the config file's folder.
	CHECK(RSAKEYPATH == Sub + "/key.pem");

	// rsa_key: relative to the config file's folder, or absolute.
	WriteFile(Folder / "rel" / "sub" / "relkey.toml", "rsa_key = \"keys/server.pem\"\n");
	WriteFile(Folder / "rel" / "sub" / "abskey.toml", "rsa_key = \"/etc/patella/key.pem\"\n");
	ReadConfig("sub/relkey.toml");
	CHECK(RSAKEYPATH == Sub + "/keys/server.pem");
	ReadConfig("sub/abskey.toml");
	CHECK(strcmp(RSAKEYPATH, "/etc/patella/key.pem") == 0);
	SetConfigDefaults();
	CHECK(RSAKEYPATH[0] == 0);
	CHECK(chdir(Here) == 0);
}

static void TestStrings(const fs::path &Folder) {
	// Every Latin-1 character but zero goes through TomlQuote and back.
	std::string All;
	for (int c = 1; c < 256; c++) {
		All += static_cast<char>(c);
	}
	WriteFile(Folder / "strings.toml", "text = " + TomlQuote(All) + "\n");
	toml::table Table = TomlParseFile((Folder / "strings.toml").c_str());
	std::string Back;
	CHECK(Utf8ToLatin1(Table["text"].value_or(std::string()), Back));
	CHECK(Back == All);
	CHECK(Latin1ToUtf8("\xe4") == "\xc3\xa4");
	CHECK(!Utf8ToLatin1("\xe2\x82\xac", Back));
}

static int Original(const char *Tibia, const char *Config, const char *Base) {
	if (!fs::is_regular_file(Tibia)) {
		printf("skipped: no %s\n", Tibia);
		return 77;
	}

	Poison();
	TTibiaConfigExtras Extras;
	try {
		ReadTibiaConfig(Tibia, Extras);
	} catch (const char *Error) {
		fprintf(stderr, "ReadTibiaConfig: %s\n", Error);
		return 1;
	}
	std::string Folder = fs::absolute(Config).parent_path().lexically_normal().string();
	TSettings Expected = Take(Base, Folder);

	std::string Text = ConfigToToml(Extras, Base);
	if (ReadFile(Config) != Text) {
		fprintf(stderr, "%s is not what patella-convert config --base %s makes of %s; it makes:\n%s", Config, Base,
				Tibia, Text.c_str());
		Failures++;
	}

	Poison();
	try {
		LoadConfig(Config);
	} catch (const char *Error) {
		fprintf(stderr, "LoadConfig: %s\n", Error);
		return 1;
	}
	TSettings Actual = Take();
	if (Same(Expected, Actual, "original")) {
		printf("%s and %s give the same %d settings (paths under %s stand for those under %s):\n", Tibia, Config,
				static_cast<int>(Actual.Values.size()), Base, Folder.c_str());
		for (const auto &Value : Actual.Values) {
			printf("  %s = \"%s\"\n", Value.first.c_str(), Printable(Value.second).c_str());
		}
	}
	return Failures == 0 ? 0 : 1;
}

int main(int argc, char **argv) {
	if (argc == 5 && strcmp(argv[1], "--original") == 0) {
		return Original(argv[2], argv[3], argv[4]);
	}
	if (argc != 1) {
		fprintf(stderr, "usage: convert_test_config [--original TIBIA CONFIG BASE]\n");
		return 2;
	}

	char Pattern[] = "/tmp/convert-test-config-XXXXXX";
	if (mkdtemp(Pattern) == nullptr) {
		perror("mkdtemp");
		return 1;
	}
	fs::path Folder = fs::canonical(Pattern);
	try {
		TestRoundTrips(Folder);
		TestDecay(Folder);
		TestRates(Folder);
		TestSpawn(Folder);
		TestLoot(Folder);
		TestDeath(Folder);
		TestRegeneration(Folder);
		TestSoul(Folder);
		TestPvp(Folder);
		TestErrors(Folder);
		TestFileNames(Folder);
		TestStrings(Folder);
	} catch (const char *Error) {
		fprintf(stderr, "unexpected error: %s\n", Error);
		Failures++;
	}
	fs::remove_all(Folder);
	if (Failures != 0) {
		fprintf(stderr, "%d failures\n", Failures);
		return 1;
	}
	printf("all passed\n");
	return 0;
}
