// test_monsters.cc: the conversion of the monster races, the raids and the
// monster homes (patella-convert monsters). The data the converter's reader
// gets from the .mon and .evt files and monster.db (mon_files.cc, which reads
// them as the game's LoadRace, LoadMonsterRaid and LoadMonsterhomes did) and
// the data the game's loaders (LoadMonsterRaces, LoadRaids and
// LoadMonsterHomes of monsterdata.cc) get from the monsters.toml, raids.toml
// and monster_homes.toml patella-convert makes of them must be the same,
// field by field and in order; the raid files' comments must stand where
// they stood; and the reader must refuse what the game refused, and what the
// conversion refuses besides, with the file and line.
//
//   convert_test_monsters
//     With made-up files in a temporary folder.
//   convert_test_monsters --original TAR SCRATCH DATA_FOLDER
//     With the original mon/ folder and dat/monster.db of TAR (a tar of the
//     pristine installation), extracted into SCRATCH (a folder on the file
//     system the game ran on, whose readdir order is the raids' order; not
//     /mnt/c): every .mon and .evt and monster.db must convert, the round
//     trips must hold, and the written monsters.toml, raids.toml and
//     monster_homes.toml must be exactly the committed ones of DATA_FOLDER.
//     Exit code 77 (skipped) when TAR is not there.

#include <climits>
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>

#include "monsterdata.hh"
#include "mon_files.hh"
#include "monsters_writer.hh"

#include "test_common.hh"

// A value of a spell as text: numbers, quoted texts, [a, b], {k = v, ...}.
static std::string ValueText(const TScriptValue &Value) {
	switch (Value.Kind) {
		case TScriptValue::NUMBER:
			return std::to_string(Value.Number);
		case TScriptValue::TEXT:
			return "\"" + Value.Text + "\"";
		case TScriptValue::ARRAY: {
			std::string Text = "[";
			for (size_t i = 0; i < Value.Elements.size(); i++) {
				Text += (i > 0 ? ", " : "") + ValueText(Value.Elements[i]);
			}
			return Text + "]";
		}
		case TScriptValue::TABLE: {
			std::string Text = "{";
			for (size_t i = 0; i < Value.Fields.size(); i++) {
				Text += (i > 0 ? ", " : "") + Value.Fields[i].first + " = " + ValueText(Value.Fields[i].second);
			}
			return Text + "}";
		}
	}
	return "?";
}

static std::string ItemText(const TMonsterItemData &Item) {
	return std::to_string(Item.Type) + "," + std::to_string(Item.Maximum) + "," + std::to_string(Item.Chance);
}

// Every field of the races but the positions (Where), as "field\tvalue"
// lines.
static std::string DumpRaces(const std::vector<TMonsterData> &Races) {
	std::string Out = "races\t" + std::to_string(Races.size()) + "\n";
	for (size_t r = 0; r < Races.size(); r++) {
		const TMonsterData &R = Races[r];
		std::string P = "race" + std::to_string(r) + ".";
		auto Add = [&](const std::string &Field, const std::string &Value) {
			Out += P + Field + "\t" + Value + "\n";
		};
		Add("key", R.Key);
		Add("number", std::to_string(R.Number));
		Add("name", R.Name);
		Add("article", R.Article);
		Add("outfit.id", std::to_string(R.Outfit.OutfitID));
		if (R.Outfit.OutfitID == 0) {
			Add("outfit.object_type", std::to_string(R.Outfit.ObjectType));
		} else {
			for (int i = 0; i < 4; i++) {
				Add("outfit.color" + std::to_string(i), std::to_string(R.Outfit.Colors[i]));
			}
		}
		Add("corpse", std::to_string(R.Corpse));
		Add("female_corpse", std::to_string(R.FemaleCorpse));
		Add("blood", R.Blood);
		Add("experience", std::to_string(R.Experience));
		Add("summon_cost", std::to_string(R.SummonCost));
		Add("flee_threshold", std::to_string(R.FleeThreshold));
		Add("attack", std::to_string(R.Attack));
		Add("defend", std::to_string(R.Defend));
		Add("armor", std::to_string(R.Armor));
		Add("poison", std::to_string(R.Poison));
		Add("lose_target", std::to_string(R.LoseTarget));
		for (int i = 0; i < 4; i++) {
			Add("strategy" + std::to_string(i), std::to_string(R.Strategy[i]));
		}
		Add("flags", std::to_string(R.Flags.size()));
		for (size_t i = 0; i < R.Flags.size(); i++) {
			Add("flag" + std::to_string(i), R.Flags[i]);
		}
		Add("skills", std::to_string(R.Skills.size()));
		for (size_t i = 0; i < R.Skills.size(); i++) {
			const TMonsterSkillData &S = R.Skills[i];
			Add("skill" + std::to_string(i), S.Skill + "," + std::to_string(S.Actual) + "," + std::to_string(S.Minimum)
					+ "," + std::to_string(S.Maximum) + "," + std::to_string(S.NextLevel) + ","
					+ std::to_string(S.FactorPercent) + "," + std::to_string(S.AddLevel));
		}
		Add("talks", std::to_string(R.Talk.size()));
		for (size_t i = 0; i < R.Talk.size(); i++) {
			Add("talk" + std::to_string(i), R.Talk[i]);
		}
		Add("items", std::to_string(R.Inventory.size()));
		for (size_t i = 0; i < R.Inventory.size(); i++) {
			Add("item" + std::to_string(i), ItemText(R.Inventory[i]));
		}
		Add("spells", std::to_string(R.Spells.size()));
		for (size_t i = 0; i < R.Spells.size(); i++) {
			Add("spell" + std::to_string(i), ValueText(R.Spells[i]));
		}
		Add("spell_positions", std::to_string(R.SpellWhere.size()));
		Add("script", R.Script);
	}
	return Out;
}

static std::string DumpRaids(const std::vector<TRaidData> &Raids) {
	std::string Out = "raids\t" + std::to_string(Raids.size()) + "\n";
	for (size_t r = 0; r < Raids.size(); r++) {
		const TRaidData &R = Raids[r];
		std::string P = "raid" + std::to_string(r) + ".";
		auto Add = [&](const std::string &Field, const std::string &Value) {
			Out += P + Field + "\t" + Value + "\n";
		};
		Add("key", R.Key);
		Add("description", R.Description);
		Add("big", std::to_string(R.Big));
		Add("interval", std::to_string(R.Interval));
		Add("date", std::to_string(R.Date));
		Add("script", R.Script);
		Add("waves", std::to_string(R.Waves.size()));
		for (size_t w = 0; w < R.Waves.size(); w++) {
			const TRaidWaveData &W = R.Waves[w];
			std::string Q = "wave" + std::to_string(w) + ".";
			Add(Q + "delay", std::to_string(W.Delay));
			Add(Q + "position", std::to_string(W.x) + "," + std::to_string(W.y) + "," + std::to_string(W.z));
			Add(Q + "spread", std::to_string(W.Spread));
			Add(Q + "race", W.Race);
			Add(Q + "count", std::to_string(W.MinCount) + "," + std::to_string(W.MaxCount));
			Add(Q + "radius", std::to_string(W.Radius));
			Add(Q + "lifetime", std::to_string(W.Lifetime));
			Add(Q + "message", W.Message);
			Add(Q + "location", W.Location);
			Add(Q + "items", std::to_string(W.Inventory.size()));
			for (size_t i = 0; i < W.Inventory.size(); i++) {
				Add(Q + "item" + std::to_string(i), ItemText(W.Inventory[i]));
			}
		}
	}
	return Out;
}

static std::string DumpHomes(const std::vector<TMonsterHomeData> &Homes) {
	std::string Out = "homes\t" + std::to_string(Homes.size()) + "\n";
	for (size_t h = 0; h < Homes.size(); h++) {
		const TMonsterHomeData &H = Homes[h];
		Out += "home" + std::to_string(h) + "\t" + H.Race + "," + std::to_string(H.x) + "," + std::to_string(H.y)
				+ "," + std::to_string(H.z) + "," + std::to_string(H.Radius) + "," + std::to_string(H.Amount) + ","
				+ std::to_string(H.Regeneration) + "\n";
	}
	return Out;
}

static std::vector<TRaidData> RaidsOf(const TMonsterFolder &Folder) {
	std::vector<TRaidData> Raids;
	for (const TRaidFile &File : Folder.Raids) {
		Raids.push_back(File.Raid);
	}
	return Raids;
}

// Writes the three files into Out, loads them with the game's loaders and
// compares with what the reader read. Returns whether all three are the same.
static bool RoundTrip(const TMonsterFolder &Folder, const std::vector<TMonsterHomeData> &Homes, const fs::path &Out) {
	try {
		WriteMonsterFiles(Folder, Homes, Out.c_str());
	} catch (const char *Error) {
		fprintf(stderr, "WriteMonsterFiles: %s\n", Error);
		Failures++;
		return false;
	}
	std::vector<TMonsterData> Races;
	std::vector<TRaidData> Raids;
	std::vector<TMonsterHomeData> LoadedHomes;
	try {
		LoadMonsterRaces((Out / "monsters.toml").c_str(), Races);
		LoadRaids((Out / "raids.toml").c_str(), Races, Raids);
		LoadMonsterHomes((Out / "monster_homes.toml").c_str(), Races, LoadedHomes);
	} catch (const char *Error) {
		fprintf(stderr, "loading the written files: %s\n", Error);
		Failures++;
		return false;
	}
	bool Equal = Same(DumpRaces(Folder.Races), DumpRaces(Races), "races", "the reader", "LoadMonsterRaces");
	Equal = Same(DumpRaids(RaidsOf(Folder)), DumpRaids(Raids), "raids", "the reader", "LoadRaids") && Equal;
	Equal = Same(DumpHomes(Homes), DumpHomes(LoadedHomes), "homes", "the reader", "LoadMonsterHomes") && Equal;
	return Equal;
}

static const char *const DEMON =
	"# Tibia - graphical Multi-User-Dungeon\r\n"
	"# MonsterRace File\r\n"
	"\r\n"
	"RaceNumber    = 35\r\n"
	"Name          = \"demon\"\r\n"
	"Article       = \"a\"\r\n"
	"Outfit        = (35, 1-2-3-4)\r\n"
	"Corpse        = 4097\r\n"
	"Blood         = Fire\r\n"
	"Experience    = 6000\r\n"
	"SummonCost    = 0\r\n"
	"FleeThreshold = 0\r\n"
	"Attack        = 80\r\n"
	"Defend        = 65\r\n"
	"Armor         = 40\r\n"
	"Poison        = 0\r\n"
	"LoseTarget    = 10\r\n"
	"Strategy      = (70, 10, 10, 10)\r\n"
	"Flags         = {NoParalyze, KickBoxes,\r\n"
	"                 Unpushable}\r\n"
	"Skills        = {(HitPoints, 8200, 0, 8200, 0, 0, 0),\r\n"
	"                 (GoStrength, 80, 0, 80, 0, 0, 0)}\r\n"
	"Spells        = {Actor (13) -> Healing (120, 30) : 7,\r\n"
	"                 Victim (7, 0, 0) -> Damage (512, 70, 30) : 8,\r\n"
	"                 Origin (0, 13) -> Summon (49, 1) : 12,\r\n"
	"                 Destination (7, 4, 6, 7) -> Field (1) : 3,\r\n"
	"                 Angle (0, 8, 12) -> Speed (-70, 20, 6) : 10,\r\n"
	"                 Actor (14) -> Drunken (3, 2, 40) : 9,\r\n"
	"                 Victim (7, 0, 0) -> Strength (2, -70, 20, 6) : 11,\r\n"
	"                 Actor (13) -> Outfit ((0, 2978), 10) : 30,\r\n"
	"                 Actor (13) -> Outfit ((111, 5-6-7-8), 3) : 1}\r\n"
	"Inventory     = {(3420, 1, 7),\r\n"
	"                 (3031, 100, 700)}\r\n"
	"Talk          = {\"#Y MUHAHAHAHA!\", \"Your resistance is futile! \\\"\xe4\"}\r\n";

// A race with nothing but its number and name: every other key the game's
// default. Its file name has a digit and '_'.
static const char *const PLAIN = "RaceNumber = 49\nName = \"fire elemental\"\n";

// Corpses, an outfit of an object, an unknown key and an unknown flag.
static const char *const HARE =
	"RaceNumber    = 232\n"
	"Name          = \"The Halloween Hare\"\n"
	"Article       = \"\"\n"
	"Outfit        = (0, 4173)\n"
	"Corpses       = 4173, 0\n"
	"Unknown       = Flags = {SeeInvisible, Glowing}\n";

static const char *const RAID =
	"######\r\n"
	"# Ablauf: eine Welle \xfc\xe4\r\n"
	"\r\n"
	"Description = \"a made-up raid\"\r\n"
	"Type      = BigRaid\r\n"
	"Interval  = 1209600\r\n"
	"#           (alle zwei Wochen)\r\n"
	"\r\n"
	"\r\n"
	"# Ort: hier\r\n"
	"Delay     = 1\r\n"
	"Position  = [32681,31598,7]\r\n"
	"Spread    = 6\r\n"
	"Race      = 35     Count     = (9,18)\r\n"
	"# Message   = \"commented out\"\r\n"
	"\r\n"
	"# Ort: dort\r\n"
	"Delay     = 60\r\n"
	"Location  = \"somewhere\"\r\n"
	"Position  = [32635,31724,7]\r\n"
	"Spread    = 12\r\n"
	"Race      = 49\r\n"
	"Count     = (0,1)\r\n"
	"Radius    = 5\r\n"
	"Lifetime  = 600\r\n"
	"Message   = \"Many \\\"beasts\\\" are roaming!\"\r\n"
	"Inventory = {( 3393,   1,   8), # amazon helmet\r\n"
	"             # a line between\r\n"
	"             ( 3394,   1,   6), ( 3395, 2, 3), # two\r\n"
	"             ( 3437,   1,   4)} # amazon shield\r\n"
	"\r\n"
	"Delay = 70 Position = [1,2,3] Race = 232 Count = (1,1) Inventory = {(3244,1,1000)}  # questbackpack\r\n"
	"Spread = 0\r\n";

static const char *const DATED =
	"Type = SmallRaid\n"
	"Date = 1130763600\n"
	"Delay = 0\n"
	"Position = [32624,32017,07]\n"
	"Spread = 1\n"
	"Race = 232\n"
	"Count = (1,1)";

static const char *const HOMES =
	"# Race     X     Y  Z Radius Amount Regen.\n"
	"\n"
	"# ====== 0997,0988,09 ====================\n"
	"    35 31927 31622  9     50      3    700\n"
	"    49 31931 31657  9      0      1    600\n"
	"   232 0 0 0 0 0 0\n"
	"0 # zero for end of file\n";

static void TestConversion(const fs::path &Folder) {
	fs::path Mon = Folder / "mon";
	WriteFile(Mon / "demon.mon", DEMON);
	WriteFile(Mon / "fire_elemental2.mon", PLAIN);
	WriteFile(Mon / "halloweenhare.mon", HARE);
	WriteFile(Mon / "made_up.evt", RAID);
	WriteFile(Mon / "dated.evt", DATED);
	WriteFile(Mon / "notes.txt", "not read\n");
	WriteFile(Folder / "monster.db", HOMES);

	TMonsterFolder Data;
	std::vector<TMonsterHomeData> Homes;
	try {
		ReadMonsterFolder(Mon.c_str(), Data);
		ReadMonsterHomes((Folder / "monster.db").c_str(), Data.Races, Homes, Data.Ignored);
	} catch (const char *Error) {
		fprintf(stderr, "made-up files: %s\n", Error);
		Failures++;
		return;
	}

	// What the reader made of them.
	CHECK(Data.Races.size() == 3);
	CHECK(Data.Raids.size() == 2);
	CHECK(Homes.size() == 3);
	if (Data.Races.size() != 3 || Data.Raids.size() != 2 || Homes.size() != 3) {
		return;
	}
	const TMonsterData &Demon = Data.Races[0];
	const TMonsterData &Plain = Data.Races[1];
	const TMonsterData &Hare = Data.Races[2];
	CHECK(Demon.Key == "demon" && Demon.Number == 35 && Demon.Blood == "fire");
	CHECK(Demon.Outfit.OutfitID == 35 && Demon.Outfit.Colors[0] == 1 && Demon.Outfit.Colors[3] == 4);
	CHECK(Demon.Corpse == 4097 && Demon.FemaleCorpse == 4097);
	CHECK((Demon.Flags == std::vector<std::string>{"kick_boxes", "unpushable", "no_paralyze"}));
	CHECK(Demon.Skills.size() == 2 && Demon.Skills[0].Skill == "hitpoints" && Demon.Skills[1].Skill == "gostrength");
	CHECK(Demon.Talk.size() == 2 && Demon.Talk[1] == "Your resistance is futile! \"\xe4");
	CHECK(Demon.Spells.size() == 9);
	if (Demon.Spells.size() == 9) {
		CHECK(ValueText(Demon.Spells[0]) == "{shape = \"actor\", effect = 13, impact = \"healing\", amount = 120, "
				"variation = 30, one_in = 7}");
		CHECK(ValueText(Demon.Spells[2]) == "{shape = \"origin\", radius = 0, effect = 13, impact = \"summon\", "
				"race = \"fire_elemental2\", maximum = 1, one_in = 12}");
		CHECK(ValueText(Demon.Spells[3]) == "{shape = \"destination\", range = 7, animation = 4, radius = 6, "
				"effect = 7, impact = \"field\", field_type = 1, one_in = 3}");
		CHECK(ValueText(Demon.Spells[4]) == "{shape = \"angle\", angle = 0, range = 8, effect = 12, impact = "
				"\"speed\", percent = -70, variation = 20, duration = 6, one_in = 10}");
		CHECK(ValueText(Demon.Spells[6]) == "{shape = \"victim\", range = 7, animation = 0, effect = 0, impact = "
				"\"strength\", skills = 2, percent = -70, variation = 20, duration = 6, one_in = 11}");
		CHECK(ValueText(Demon.Spells[7]) == "{shape = \"actor\", effect = 13, impact = \"outfit\", outfit = {id = 0, "
				"object_type = 2978}, duration = 10, one_in = 30}");
		CHECK(ValueText(Demon.Spells[8]) == "{shape = \"actor\", effect = 13, impact = \"outfit\", outfit = {id = "
				"111, colors = [5, 6, 7, 8]}, duration = 3, one_in = 1}");
	}
	// TRaceData's defaults, the outfit the race number.
	CHECK(Plain.Key == "fire_elemental2" && Plain.Number == 49 && Plain.Article.empty() && Plain.Blood == "blood");
	CHECK(Plain.Outfit.OutfitID == 49 && Plain.Outfit.Colors[0] == 0 && Plain.Corpse == 0);
	CHECK(Plain.Strategy[0] == 100 && Plain.Strategy[1] == 0 && Plain.Experience == 0 && Plain.Flags.empty());
	CHECK(Hare.Outfit.OutfitID == 0 && Hare.Outfit.ObjectType == 4173);
	CHECK(Hare.Corpse == 4173 && Hare.FemaleCorpse == 0);
	CHECK((Hare.Flags == std::vector<std::string>{"see_invisible"}));

	// readdir's order of the raids is not checked here (it is the file
	// system's); find them by key.
	const TRaidFile *Made = nullptr;
	const TRaidFile *Dated = nullptr;
	for (const TRaidFile &File : Data.Raids) {
		(File.Raid.Key == "made_up" ? Made : Dated) = &File;
	}
	CHECK(Made != nullptr && Dated != nullptr);
	if (Made == nullptr || Dated == nullptr) {
		return;
	}
	CHECK(Made->Raid.Big && Made->Raid.Interval == 1209600 && Made->Raid.Date == 0);
	CHECK(Made->Raid.Description == "a made-up raid");
	CHECK(Made->Raid.Waves.size() == 3);
	if (Made->Raid.Waves.size() == 3) {
		const TRaidWaveData &W0 = Made->Raid.Waves[0];
		const TRaidWaveData &W1 = Made->Raid.Waves[1];
		const TRaidWaveData &W2 = Made->Raid.Waves[2];
		CHECK(W0.Race == "demon" && W0.MinCount == 9 && W0.MaxCount == 18 && W0.Radius == INT_MAX);
		CHECK(W0.Lifetime == 0 && W0.Message.empty() && W0.Inventory.empty());
		CHECK(W1.Race == "fire_elemental2" && W1.Radius == 5 && W1.Lifetime == 600 && W1.Location == "somewhere");
		CHECK(W1.Message == "Many \"beasts\" are roaming!" && W1.Inventory.size() == 4);
		CHECK(W2.Race == "halloweenhare" && W2.x == 1 && W2.Spread == 0 && W2.Inventory.size() == 1);
	}
	CHECK(!Dated->Raid.Big && Dated->Raid.Date == 1130763600 && Dated->Raid.Interval == 0);
	CHECK(Homes[0].Race == "demon" && Homes[1].Regeneration == 600 && Homes[2].Race == "halloweenhare");

	// The ignored: the unknown key and flag, the description and the location.
	std::string Ignored;
	for (const std::string &Line : Data.Ignored) {
		Ignored += Line + "\n";
	}
	CHECK(Ignored.find("halloweenhare.mon, line 6: unknown key \"unknown\"") != std::string::npos);
	CHECK(Ignored.find("halloweenhare.mon, line 6: unknown flag \"glowing\"") != std::string::npos);
	CHECK(Ignored.find("made_up.evt, line 4: Description") != std::string::npos);
	CHECK(Ignored.find("made_up.evt, line 19: Location") != std::string::npos);
	CHECK(Data.Ignored.size() == 4);

	// The round trip; the homes' (0 0 0) refused by LoadMonsterHomes? No:
	// their position may be 0; only a wave's x must be at least 1.
	fs::path Out = Folder / "data";
	if (!RoundTrip(Data, Homes, Out)) {
		fprintf(stderr, "%s", ReadFile(Out / "raids.toml").c_str());
	}

	// The comments where they stood.
	std::string Raids = ReadFile(Out / "raids.toml");
	CHECK(Raids.find("\n######\n# Ablauf: eine Welle \xc3\xbc\xc3\xa4\n\n[[raid]]\nkey = \"made_up\"\n"
			"description = \"a made-up raid\"\ntype = \"big\"\ninterval = 1209600\n#           (alle zwei Wochen)\n"
			"\n# Ort: hier\n[[raid.wave]]\ndelay = 1\nposition = [32681, 31598, 7]\nspread = 6\n"
			"race = \"demon\"\ncount = [9, 18]\n# Message   = \"commented out\"\n\n# Ort: dort\n[[raid.wave]]\n")
			!= std::string::npos);
	CHECK(Raids.find("inventory = [\n"
			"\t{ type = 3393, maximum = 1, chance = 8 }, # amazon helmet\n"
			"\t# a line between\n"
			"\t{ type = 3394, maximum = 1, chance = 6 }, { type = 3395, maximum = 2, chance = 3 }, # two\n"
			"\t{ type = 3437, maximum = 1, chance = 4 }, # amazon shield\n"
			"]\n\n[[raid.wave]]\ndelay = 70\nposition = [1, 2, 3]\nrace = \"halloweenhare\"\ncount = [1, 1]\n"
			"inventory = [{ type = 3244, maximum = 1, chance = 1000 }] # questbackpack\nspread = 0\n")
			!= std::string::npos);
	std::string Monsters = ReadFile(Out / "monsters.toml");
	CHECK(Monsters.find("key = \"halloweenhare\"\nnumber = 232\nname = \"The Halloween Hare\"\narticle = \"\"\n"
			"outfit = { id = 0, object_type = 4173 }\ncorpse = 4173\nfemale_corpse = 0\n") != std::string::npos);
	CHECK(Monsters.find("\"Your resistance is futile! \\\"\xc3\xa4\",\n") != std::string::npos);
	CHECK(Monsters.find("key = \"demon\"") < Monsters.find("key = \"fire_elemental2\""));
}

// Keys given twice, read as the game read them: the last value, the lists of
// both, the flags of both; noted, and the round trip holds.
static void TestTwice(const fs::path &Folder) {
	fs::path Mon = Folder / "twice" / "mon";
	WriteFile(Mon / "rat.mon", "RaceNumber = 7\nName = \"x\"\nName = \"rat\"\nCorpse = 1\nCorpses = 3, 4\n"
			"Flags = {KickBoxes}\nFlags = {NoHit}\nTalk = {\"a\"}\nTalk = {\"b\"}\n");
	WriteFile(Mon / "twice.evt", "Type = SmallRaid\nInterval = 5\nDelay = 1\nPosition = [1,2,3]\nSpread = 1\n"
			"Race = 7\nCount = (1,2)   Count = (2,3)\nInventory = {(1,1,1)}\nSpread = 4\nInventory = {(2,1,1)}\n");
	TMonsterFolder Data;
	try {
		ReadMonsterFolder(Mon.c_str(), Data);
	} catch (const char *Error) {
		fprintf(stderr, "keys given twice: %s\n", Error);
		Failures++;
		return;
	}
	CHECK(Data.Races.size() == 1 && Data.Raids.size() == 1);
	if (Data.Races.size() != 1 || Data.Raids.size() != 1 || Data.Raids[0].Raid.Waves.size() != 1) {
		return;
	}
	const TMonsterData &Rat = Data.Races[0];
	CHECK(Rat.Name == "rat" && Rat.Corpse == 3 && Rat.FemaleCorpse == 4);
	CHECK((Rat.Flags == std::vector<std::string>{"kick_boxes", "no_hit"}));
	CHECK((Rat.Talk == std::vector<std::string>{"a", "b"}));
	const TRaidWaveData &Wave = Data.Raids[0].Raid.Waves[0];
	CHECK(Wave.MinCount == 2 && Wave.MaxCount == 3 && Wave.Spread == 4 && Wave.Inventory.size() == 2);
	CHECK(Data.Ignored.size() == 7);
	CHECK(Data.Ignored.size() == 7 && Data.Ignored[0] == "rat.mon, line 3: name given twice, of which the game took "
			"the last" && Data.Ignored[6] == "twice.evt, line 10: inventory given twice in a wave, of which the game "
			"took the objects of both");
	RoundTrip(Data, {}, Folder / "twice" / "data");
}

// The error the reader throws for the file Name with the text Text in the
// mon/ folder (with the races of RACES), or "" for none.
static const char *const RACES[][2] = {
	{"demon.mon", "RaceNumber = 35\nName = \"demon\"\n"},
	{"orc.mon", "RaceNumber = 5\nName = \"orc\"\n"},
};

static std::string FolderError(const fs::path &Folder, const std::string &Name, const std::string &Text,
		const std::string &Other = "") {
	fs::path Mon = Folder / "refusals" / "mon";
	fs::remove_all(Mon);
	for (const auto &Race : RACES) {
		WriteFile(Mon / Race[0], Race[1]);
	}
	WriteFile(Mon / Name, Text);
	if (!Other.empty()) {
		WriteFile(Mon / "other.txt", Other);
	}
	TMonsterFolder Data;
	try {
		ReadMonsterFolder(Mon.c_str(), Data);
	} catch (const char *Error) {
		return Error;
	}
	return "";
}

static std::string HomesError(const fs::path &Folder, const std::string &Text) {
	fs::path File = Folder / "refusals" / "monster.db";
	WriteFile(File, Text);
	std::vector<TMonsterData> Races(1);
	Races[0].Key = "orc";
	Races[0].Number = 5;
	std::vector<TMonsterHomeData> Homes;
	std::vector<std::string> Ignored;
	try {
		ReadMonsterHomes(File.c_str(), Races, Homes, Ignored);
	} catch (const char *Error) {
		return Error;
	}
	return "";
}

static void Expect(const std::string &Error, const std::string &Name, int Line, const char *What,
		const std::string &Text) {
	std::string Where = Name + "\", line " + std::to_string(Line) + ": ";
	bool Ok = Error.find("error in script-file \"" + Where) != std::string::npos
			&& Error.find(What) != std::string::npos;
	if (!Ok) {
		fprintf(stderr, "for:\n%s\nexpected \"%s%s\", got \"%s\"\n", Text.c_str(), Where.c_str(), What,
				Error.c_str());
		Failures++;
	}
}

static void TestRefusals(const fs::path &Folder) {
	auto Race = [&](const std::string &Text, int Line, const char *What) {
		Expect(FolderError(Folder, "test.mon", Text), "test.mon", Line, What, Text);
	};
	auto Raid = [&](const std::string &Text, int Line, const char *What) {
		Expect(FolderError(Folder, "test.evt", Text), "test.evt", Line, What, Text);
	};
	auto Homes = [&](const std::string &Text, int Line, const char *What) {
		Expect(HomesError(Folder, Text), "monster.db", Line, What, Text);
	};
	const std::string Head = "RaceNumber = 7\nName = \"rat\"\n";

	// The game's refusals of a .mon.
	Race("Name = \"rat\"\n", 1, "race number expected");
	Race("RaceNumber = 512\nName = \"rat\"\n", 1, "illegal race number");
	Race("RaceNumber = 0\n", 1, "illegal race number");
	// Which of two files with one race number is read second is readdir's
	// choice.
	std::string Twice = FolderError(Folder, "test.mon", "RaceNumber = 35\nName = \"demon two\"\n");
	CHECK(Twice.find("\", line 1: race already defined") != std::string::npos);
	Race(Head + "Blood = Ink\n", 3, "unknown blood type");
	Race(Head + "Skills = {(Swimming, 1, 1, 1, 1, 1, 1)}\n", 3, "unknown skill name");
	Race(Head + "Spells = {Ring (1) -> Damage (1, 2, 3) : 4}\n", 3, "unknown spell shape");
	Race(Head + "Spells = {Actor (1) -> Teleport (1, 2) : 4}\n", 3, "unknown spell impact");
	Race(Head + "Spells = {Actor (1) -> Field (1) : 0}\n", 3, "zero spell delay");
	Race(Head + "Spells = {Actor (1, 2) -> Field (1) : 1}\n", 3, "special-char expected");
	Race(Head + "Outfit = (5, 1)\n", 3, "byte-sequence expected");
	Race(Head + "Strategy = (1, 2, 3)\n", 3, "special-char expected");
	Race(Head + "5 = 3\n", 3, "identifier expected");
	Race(Head + "Attack = \"x\"\n", 3, "number expected");
	Race(Head + "Name = rat\n", 3, "string expected");
	// The conversion's refusals besides.
	Race("RaceNumber = 7\n", 2, "a race without a name");
	Race("RaceNumber = 7\nName = \"abcdefghijklmnopqrstuvwxyz0123\"\n", 2, "a name longer than 29 bytes");
	Race(Head + "Article = \"the\"\n", 3, "an article longer than 2 bytes");
	Race(Head + "Talk = {\"hi\", \"\"}\n", 3, "an empty talk");
	Race(Head + "Talk = {\"#Y\"}\n", 3, "a loud talk shorter than 3 bytes");
	Race(Head + "Outfit = (70000, 0-0-0-0)\n", 3, "an outfit id outside 0 to 65535");
	Race(Head + "Outfit = (0, 70000)\n", 3, "an object type outside 0 to 65535");
	Race(Head + "Outfit = (5, 0-0-0)\n", 3, "colors that are not four numbers");
	Race(Head + "Outfit = (5, 0-0-0-256)\n", 3, "colors that are not four numbers");
	Race(Head + "Spells = {Actor (1) -> Field (1) : -2}\n", 3, "a negative spell delay");
	Race(Head + "Spells = {Actor (1) -> Summon (99, 1) : 2}\n", 3, "a summon of race 99, of which there is no .mon");
	Race(Head + std::string("Talk = {\"a\0b\"}\n", 15), 3, "a string with a zero byte");
	std::string Include = Head + "@\"other.txt\"\n";
	Expect(FolderError(Folder, "test.mon", Include, "Attack = 5\n"), "test.mon", 3, "an include", Include);
	Expect(FolderError(Folder, "Rat.mon", Head), "Rat.mon", 1, "cannot be a race's key", Head);
	Expect(FolderError(Folder, "rat-2.mon", Head), "rat-2.mon", 1, "cannot be a race's key", Head);

	// The game's refusals of an .evt.
	const std::string Wave = "Delay = 1\nPosition = [1,2,3]\nSpread = 1\nRace = 5\nCount = (1,2)\n";
	Raid("Interval = 5\n", 1, "type expected");
	Raid("Description = \"x\"\nInterval = 5\n", 2, "type expected");
	Raid("Type = SmallRaid\nWhen = 5\n", 2, "date or interval expected");
	Raid("Type = SmallRaid\nInterval = 5\nPosition = [1,2,3]\n", 3, "delay expected");
	Raid("Type = SmallRaid\nInterval = 5\nDelay = 1\nPosition = [1,2,3]\nRace = 5\nCount = (3,2)\n", 6,
			"mincount greater than maxcount");
	Raid("Type = SmallRaid\nInterval = 5\nDelay = 1\nPosition = [1,2,3]\nRace = 5\nCount = (0,0)\n", 6,
			"illegal number of monsters");
	Raid("Type = SmallRaid\nInterval = 5\nDelay = 1\nRace = 5\nCount = (1,2)\nDelay = 2\n", 6, "position expected");
	// The checks after a wave's keys: at the next delay or the end of the
	// file (the line after the last line break).
	Raid("Type = SmallRaid\nInterval = 5\nDelay = 1\nPosition = [0,2,3]\nRace = 5\nCount = (1,2)\n", 7,
			"position expected");
	Raid("Type = SmallRaid\nInterval = 5\nDelay = 1\nPosition = [1,2,3]\nCount = (1,2)\n", 6, "race expected");
	Raid("Type = SmallRaid\nInterval = 5\nDelay = 1\nPosition = [1,2,3]\nRace = 5\n", 6, "count expected");
	Raid("Type = SmallRaid\nInterval = 5\nDelay = 1\nPosition = [1,2,3]\nRace = 512\n", 5, "illegal race number");
	Raid("Type = SmallRaid\nInterval = 5\n" + Wave + "Spread = \"x\"\n", 8, "number expected");
	// The conversion's refusals besides.
	Raid("Type = SmallRaid\nInterval = 5\nDelay = 1\nPosition = [1,2,3]\nSpread = 1\nRace = 6\nCount = (1,2)\n", 3,
			"a wave of race 6, of which there is no .mon");
	Raid("Type = SmallRaid\nInterval = 5\n#a\x01" "b\n" + Wave, 3, "a comment with a control character");
	Expect(FolderError(Folder, "Big.evt", "Type = SmallRaid\n"), "Big.evt", 1, "cannot be a raid's key", "");

	// The game's refusals of monster.db, and the conversion's.
	Homes("5 1 2 3 4 5\n", 2, "number expected");
	Homes("5 1 2 3 4 5 6\n", 2, "number expected");
	Homes("5 1 2 x 4 5 6\n0\n", 1, "number expected");
	Homes("6 1 2 3 4 5 6\n0\n", 1, "a home of race 6, of which there is no .mon");
	CHECK(HomesError(Folder, "5 1 2 3 4 5 6\n-0\n") == "");
}

// Extracts mon/ and dat/monster.db of Tar into Folder.
static bool Extract(const char *Tar, const fs::path &Folder) {
	pid_t Child = fork();
	if (Child == 0) {
		execlp("tar", "tar", "-xf", Tar, "-C", Folder.c_str(), "mon", "dat/monster.db",
				static_cast<char *>(nullptr));
		_exit(127);
	}
	int Status = 0;
	waitpid(Child, &Status, 0);
	return WIFEXITED(Status) && WEXITSTATUS(Status) == 0;
}

static int Original(const char *Tar, const char *Scratch, const char *DataFolder) {
	if (!fs::is_regular_file(Tar)) {
		printf("skipped: no %s\n", Tar);
		return 77;
	}
	fs::path Folder(Scratch);
	fs::remove_all(Folder);
	fs::create_directories(Folder);
	if (!Extract(Tar, Folder)) {
		fprintf(stderr, "cannot extract mon/ and dat/monster.db of %s\n", Tar);
		return 1;
	}

	TMonsterFolder Data;
	std::vector<TMonsterHomeData> Homes;
	try {
		ReadMonsterFolder((Folder / "mon").c_str(), Data);
		ReadMonsterHomes((Folder / "dat" / "monster.db").c_str(), Data.Races, Homes, Data.Ignored);
	} catch (const char *Error) {
		fprintf(stderr, "%s\n", Error);
		return 1;
	}
	size_t Spells = 0, Waves = 0, Comments = 0;
	for (const TMonsterData &Race : Data.Races) {
		Spells += Race.Spells.size();
	}
	for (const TRaidFile &File : Data.Raids) {
		Waves += File.Raid.Waves.size();
		Comments += File.Layout.Comments.size();
	}
	printf("%zu races with %zu spells; %zu raids with %zu waves and %zu comments; %zu homes; %zu things the game "
			"ignored\n", Data.Races.size(), Spells, Data.Raids.size(), Waves, Comments, Homes.size(),
			Data.Ignored.size());
	for (const std::string &Line : Data.Ignored) {
		printf("ignored: %s\n", Line.c_str());
	}

	fs::path Out = Folder / "data";
	int Before = Failures;
	if (RoundTrip(Data, Homes, Out)) {
		printf("the loaders give the reader's data for all %zu races, %zu raids and %zu homes\n", Data.Races.size(),
				Data.Raids.size(), Homes.size());
	}
	int Same = 0;
	for (const char *Name : {"monsters.toml", "raids.toml", "monster_homes.toml"}) {
		fs::path Committed = fs::path(DataFolder) / Name;
		if (ReadFile(Out / Name) != ReadFile(Committed)) {
			fprintf(stderr, "%s is not what patella-convert monsters writes\n", Committed.c_str());
			Failures++;
		} else {
			Same++;
		}
	}
	printf("%d of 3 written files equal the committed ones\n", Same);
	return Failures == Before ? 0 : 1;
}

int main(int argc, char **argv) {
	if (argc == 5 && strcmp(argv[1], "--original") == 0) {
		int Result = Original(argv[2], argv[3], argv[4]);
		if (Failures != 0) {
			fprintf(stderr, "%d failures\n", Failures);
		}
		return Result;
	}
	if (argc != 1) {
		fprintf(stderr, "usage: convert_test_monsters [--original TAR SCRATCH DATA_FOLDER]\n");
		return 2;
	}

	fs::path Folder = TemporaryFolder("convert-test-monsters");
	try {
		TestConversion(Folder);
		TestTwice(Folder);
		TestRefusals(Folder);
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
