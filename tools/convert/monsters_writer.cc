// monsters_writer.cc: the writers of the converted monster data
// (monsters_writer.hh).

#include "monsters_writer.hh"

#include <algorithm>
#include <climits>
#include <cstdio>
#include <filesystem>

#include "tomlfile.hh"
#include "tomlwrite.hh"

namespace {

const char *const MONSTERS_HEADER =
	"# monsters.toml: the monster races of the game server, converted from the\n"
	"# original mon/*.mon files by patella-convert. The game reads monsters.toml in\n"
	"# its data folder ([paths] data of config.toml) at start. A race is named by\n"
	"# its key, the name of its original file without \".mon\" (\"demon\",\n"
	"# \"illusion\", \"slime2\"; the names players see are not unique): 1 to 30\n"
	"# characters of a-z, 0-9 and '_'. Keys and race numbers are unique. An\n"
	"# unknown key, a missing one, a value of the wrong type or out of range is an\n"
	"# error.\n"
	"#\n"
	"# Each [[monster]] is one race, in the order of the race numbers:\n"
	"#   key             The race's key.\n"
	"#   number          Its race number, 1 to 511.\n"
	"#   name            The name players see, 1 to 29 characters.\n"
	"#   article         \"a\", \"an\" or \"\" (at most 2 characters).\n"
	"#   outfit          { id = <outfit>, colors = [<head>, <body>, <legs>,\n"
	"#                   <feet>] } (id 1 to 65535, colors 0 to 255) or { id = 0,\n"
	"#                   object_type = <type> } (0 to 65535).\n"
	"#   corpse          The object type of its corpse; female_corpse (optional)\n"
	"#                   the female one, the same as corpse when missing.\n"
	"#   blood           \"blood\", \"slime\", \"bones\", \"fire\" or \"energy\".\n"
	"#   experience, summon_cost, flee_threshold, attack, defend, armor, poison,\n"
	"#   lose_target     Numbers, as the original's keys.\n"
	"#   strategy        [<nearest>, <weakest>, <most damage>, <random>], the\n"
	"#                   chance of each in percent.\n"
	"#   flags           (optional) Names of the race's flags: \"kick_boxes\",\n"
	"#                   \"kick_creatures\", \"see_invisible\", \"unpushable\",\n"
	"#                   \"distance_fighting\", \"no_summon\", \"no_convince\",\n"
	"#                   \"no_illusion\", \"no_burning\", \"no_poison\", \"no_energy\",\n"
	"#                   \"no_hit\", \"no_life_drain\", \"no_paralyze\".\n"
	"#   skills          (optional) { skill = <name>, actual, minimum, maximum,\n"
	"#                   next_level, factor_percent, add_level }, the skill named\n"
	"#                   as the original's files name it (\"hitpoints\",\n"
	"#                   \"gostrength\", ...).\n"
	"#   talk            (optional) The texts it says; one starting with '#' is\n"
	"#                   said loud, from its fourth character on.\n"
	"#   inventory       (optional) { type, maximum, chance }: an object type, the\n"
	"#                   most it carries of it, and the chance in 1/1000.\n"
	"#   script          (optional) \"monsters/<key>.lua\", a script in the scripts\n"
	"#                   folder with its hooks (onThink, onHealthChange, onTarget,\n"
	"#                   onDeath).\n"
	"#   [[monster.spell]] (optional, any number) One spell, in order, tried each\n"
	"#                   time the monster thinks when Game.rand() % one_in == 0\n"
	"#                   (one_in at least 1): shape, the shape's keys, impact, the\n"
	"#                   impact's keys and one_in. The spell library\n"
	"#                   (scripts/lib/monsters.lua) checks them:\n"
	"#     shape \"actor\"        effect\n"
	"#     shape \"victim\"       range, animation, effect\n"
	"#     shape \"origin\"       radius, effect\n"
	"#     shape \"destination\"  range, animation, radius, effect\n"
	"#     shape \"angle\"        angle, range, effect\n"
	"#     impact \"damage\"      damage_type, damage, variation\n"
	"#     impact \"field\"       field_type\n"
	"#     impact \"healing\"     amount, variation\n"
	"#     impact \"speed\"       percent, variation, duration\n"
	"#     impact \"drunken\"     strength, variation, duration\n"
	"#     impact \"strength\"    skills, percent, variation, duration\n"
	"#     impact \"outfit\"      outfit (as the race's), duration\n"
	"#     impact \"summon\"      race (a race's key), maximum\n";

const char *const RAIDS_HEADER =
	"# raids.toml: the raids of the game server, converted from the original\n"
	"# mon/*.evt files by patella-convert, with their comments. The game reads\n"
	"# raids.toml in its data folder ([paths] data of config.toml) at start. A raid\n"
	"# is named by its key, the name of its original file without \".evt\"; the\n"
	"# start_monsterraid command names it. The raids are in the order in which the\n"
	"# original game read its files on the reference installation (readdir of\n"
	"# mon/), which is the order in which their start rounds are drawn. An unknown\n"
	"# key, a missing one, a value of the wrong type or out of range is an error,\n"
	"# as is an unknown race key.\n"
	"#\n"
	"# Each [[raid]] is one raid:\n"
	"#   key             The raid's key.\n"
	"#   description     (optional) A text the game does not use.\n"
	"#   type            \"small\" or \"big\".\n"
	"#   interval        The mean time between two of its raids in seconds; or\n"
	"#   date            the time of its one raid (seconds since 1970). Exactly\n"
	"#                   one of the two.\n"
	"#   script          (optional) \"raids/<key>.lua\", a script in the scripts\n"
	"#                   folder with its hooks (onRaid, onWave).\n"
	"#   [[raid.wave]]   One wave or more, in order:\n"
	"#     delay         Seconds after the raid's start.\n"
	"#     position      [x, y, z], where its monsters appear.\n"
	"#     spread        How far from position, in fields along x and y.\n"
	"#     race          The race's key (monsters.toml).\n"
	"#     count         [<min>, <max>]: how many, 0 <= min <= max, max >= 1.\n"
	"#     radius        (optional) How far the monsters walk; none: no limit.\n"
	"#     lifetime      (optional) Seconds until they vanish; 0 or none: never.\n"
	"#     message       (optional) A text broadcast when the wave comes.\n"
	"#     location      (optional) A text the game does not use.\n"
	"#     inventory     (optional) { type, maximum, chance } as for a race: the\n"
	"#                   objects one of the wave's monsters gets.\n";

const char *const HOMES_HEADER =
	"# monster_homes.toml: the monster homes of the game server, converted from the\n"
	"# original dat/monster.db by patella-convert. The game reads monster_homes.toml\n"
	"# in its data folder ([paths] data of config.toml) at start and fills the homes\n"
	"# in the order of the file. An unknown key, a missing one, a value of the wrong\n"
	"# type or out of range is an error, as is an unknown race key.\n"
	"#\n"
	"# homes is an array of one inline table per home:\n"
	"#   race            The race's key (monsters.toml).\n"
	"#   position        [x, y, z], the middle of the home.\n"
	"#   radius          How far from position its monsters appear and walk, in\n"
	"#                   fields along x and y.\n"
	"#   amount          The most monsters it has.\n"
	"#   regeneration    The time in seconds after which a monster comes back.\n";

std::string Int(int Value) {
	return std::to_string(Value);
}

std::string Item(const TMonsterItemData &Item) {
	return "{ type = " + Int(Item.Type) + ", maximum = " + Int(Item.Maximum) + ", chance = " + Int(Item.Chance) + " }";
}

// An array of one element per line.
std::string Lines(const char *Key, const std::vector<std::string> &Elements) {
	std::string Text = std::string(Key) + " = [\n";
	for (const std::string &Element : Elements) {
		Text += "\t" + Element + ",\n";
	}
	return Text + "]\n";
}

std::string MonsterEntry(const TMonsterData &R) {
	std::string Text = "[[monster]]\n";
	Text += "key = " + TomlQuote(R.Key) + "\n";
	Text += "number = " + Int(R.Number) + "\n";
	Text += "name = " + TomlQuote(R.Name) + "\n";
	Text += "article = " + TomlQuote(R.Article) + "\n";
	TScriptValue Outfit;
	Outfit.Kind = TScriptValue::TABLE;
	TScriptValue Id;
	Id.Number = R.Outfit.OutfitID;
	Outfit.Fields.emplace_back("id", Id);
	if (R.Outfit.OutfitID == 0) {
		TScriptValue Type;
		Type.Number = R.Outfit.ObjectType;
		Outfit.Fields.emplace_back("object_type", Type);
	} else {
		TScriptValue Colors;
		Colors.Kind = TScriptValue::ARRAY;
		for (unsigned char Color : R.Outfit.Colors) {
			TScriptValue Value;
			Value.Number = Color;
			Colors.Elements.push_back(Value);
		}
		Outfit.Fields.emplace_back("colors", Colors);
	}
	Text += "outfit = " + TomlValue(Outfit) + "\n";
	Text += "corpse = " + Int(R.Corpse) + "\n";
	if (R.FemaleCorpse != R.Corpse) {
		Text += "female_corpse = " + Int(R.FemaleCorpse) + "\n";
	}
	Text += "blood = " + TomlQuote(R.Blood) + "\n";
	Text += "experience = " + Int(R.Experience) + "\n";
	Text += "summon_cost = " + Int(R.SummonCost) + "\n";
	Text += "flee_threshold = " + Int(R.FleeThreshold) + "\n";
	Text += "attack = " + Int(R.Attack) + "\n";
	Text += "defend = " + Int(R.Defend) + "\n";
	Text += "armor = " + Int(R.Armor) + "\n";
	Text += "poison = " + Int(R.Poison) + "\n";
	Text += "lose_target = " + Int(R.LoseTarget) + "\n";
	Text += "strategy = [" + Int(R.Strategy[0]) + ", " + Int(R.Strategy[1]) + ", " + Int(R.Strategy[2]) + ", "
			+ Int(R.Strategy[3]) + "]\n";
	if (!R.Flags.empty()) {
		std::vector<std::string> Flags;
		for (const std::string &Flag : R.Flags) {
			Flags.push_back(TomlQuote(Flag));
		}
		Text += Lines("flags", Flags);
	}
	if (!R.Skills.empty()) {
		std::vector<std::string> Skills;
		for (const TMonsterSkillData &S : R.Skills) {
			Skills.push_back("{ skill = " + TomlQuote(S.Skill) + ", actual = " + Int(S.Actual) + ", minimum = "
					+ Int(S.Minimum) + ", maximum = " + Int(S.Maximum) + ", next_level = " + Int(S.NextLevel)
					+ ", factor_percent = " + Int(S.FactorPercent) + ", add_level = " + Int(S.AddLevel) + " }");
		}
		Text += Lines("skills", Skills);
	}
	if (!R.Talk.empty()) {
		std::vector<std::string> Talk;
		for (const std::string &Line : R.Talk) {
			Talk.push_back(TomlQuote(Line));
		}
		Text += Lines("talk", Talk);
	}
	if (!R.Inventory.empty()) {
		std::vector<std::string> Inventory;
		for (const TMonsterItemData &I : R.Inventory) {
			Inventory.push_back(Item(I));
		}
		Text += Lines("inventory", Inventory);
	}
	if (!R.Script.empty()) {
		Text += "script = " + TomlQuote(R.Script) + "\n";
	}
	for (size_t i = 0; i < R.Spells.size(); i++) {
		Text += i == 0 ? "\n[[monster.spell]]\n" : "[[monster.spell]]\n";
		for (const auto &Field : R.Spells[i].Fields) {
			Text += Field.first + " = " + TomlValue(Field.second) + "\n";
		}
	}
	return Text;
}

// One line of a raid as written: the line of the .evt it stands for, its
// text, whether it closes a multi-line array (no comment goes after it) and
// whether it lies inside one (a comment there is indented).
struct TOutLine {
	int Source = 0;
	std::string Text;
	bool Closing = false;
	bool Inside = false;
};

std::string Comment(const std::string &Latin1) {
	std::string Text = Latin1;
	while (!Text.empty() && (Text.back() == ' ' || Text.back() == '\t')) {
		Text.pop_back();
	}
	return "#" + Latin1ToUtf8(Text);
}

void Add(std::vector<TOutLine> &Lines, int Source, const std::string &Text, bool Closing = false,
		bool Inside = false) {
	TOutLine Line;
	Line.Source = Source;
	Line.Text = Text;
	Line.Closing = Closing;
	Line.Inside = Inside;
	Lines.push_back(Line);
}

void AddWaveKey(std::vector<TOutLine> &Lines, const TRaidWaveData &Wave, const TRaidKey &Key) {
	const std::string &Name = Key.Name;
	std::string Value;
	if (Name == "delay") {
		Add(Lines, Key.Line, "[[raid.wave]]");
		Value = Int(Wave.Delay);
	} else if (Name == "position") {
		Value = "[" + Int(Wave.x) + ", " + Int(Wave.y) + ", " + Int(Wave.z) + "]";
	} else if (Name == "spread") {
		Value = Int(Wave.Spread);
	} else if (Name == "race") {
		Value = TomlQuote(Wave.Race);
	} else if (Name == "count") {
		Value = "[" + Int(Wave.MinCount) + ", " + Int(Wave.MaxCount) + "]";
	} else if (Name == "radius") {
		Value = Int(Wave.Radius);
	} else if (Name == "lifetime") {
		Value = Int(Wave.Lifetime);
	} else if (Name == "message") {
		Value = TomlQuote(Wave.Message);
	} else if (Name == "location") {
		Value = TomlQuote(Wave.Location);
	} else if (Name == "inventory") {
		std::vector<int> ItemLines = Key.ItemLines;
		ItemLines.erase(std::unique(ItemLines.begin(), ItemLines.end()), ItemLines.end());
		if (ItemLines.size() <= 1) {
			std::string Items;
			for (size_t i = 0; i < Wave.Inventory.size(); i++) {
				Items += (i > 0 ? ", " : "") + Item(Wave.Inventory[i]);
			}
			Value = "[" + Items + "]";
		} else {
			Add(Lines, Key.Line, "inventory = [");
			for (int Source : ItemLines) {
				std::string Items;
				for (size_t i = 0; i < Wave.Inventory.size(); i++) {
					if (Key.ItemLines[i] == Source) {
						Items += (Items.empty() ? "" : ", ") + Item(Wave.Inventory[i]);
					}
				}
				Add(Lines, Source, "\t" + Items + ",", false, true);
			}
			Add(Lines, Key.EndLine, "]", true);
			return;
		}
	} else {
		throw "a wave key patella-convert does not know (a fault of patella-convert)";
	}
	Add(Lines, Key.Line, Name + " = " + Value);
}

std::string RaidEntry(const TRaidFile &File) {
	const TRaidData &Raid = File.Raid;
	const TRaidLayout &Layout = File.Layout;
	std::vector<TOutLine> Lines;
	int First = Layout.RaidKeys.empty() ? 1 : Layout.RaidKeys[0].Line;
	Add(Lines, First, "[[raid]]");
	Add(Lines, First, "key = " + TomlQuote(Raid.Key));
	for (const TRaidKey &Key : Layout.RaidKeys) {
		std::string Value;
		if (Key.Name == "description") {
			Value = TomlQuote(Raid.Description);
		} else if (Key.Name == "type") {
			Value = Raid.Big ? "\"big\"" : "\"small\"";
		} else if (Key.Name == "interval") {
			Value = Int(Raid.Interval);
		} else if (Key.Name == "date") {
			Value = Int(Raid.Date);
		} else {
			throw "a raid key patella-convert does not know (a fault of patella-convert)";
		}
		Add(Lines, Key.Line, Key.Name + " = " + Value);
	}
	if (!Raid.Script.empty()) {
		Add(Lines, First, "script = " + TomlQuote(Raid.Script));
	}
	if (Layout.WaveKeys.size() != Raid.Waves.size()) {
		throw "a raid's layout does not fit its waves (a fault of patella-convert)";
	}
	for (size_t i = 0; i < Raid.Waves.size(); i++) {
		for (const TRaidKey &Key : Layout.WaveKeys[i]) {
			AddWaveKey(Lines, Raid.Waves[i], Key);
		}
	}

	// The comments of their own lines, before the first line that stands
	// for a later line of the .evt; then those after a token, at the end of
	// the last line that stands for their line (else on a line of their own).
	std::vector<TRaidComment> Comments = Layout.Comments;
	std::vector<TRaidComment> Trailing;
	for (const TRaidComment &Kept : Comments) {
		if (Kept.Trailing) {
			Trailing.push_back(Kept);
		}
	}
	for (const TRaidComment &Kept : Trailing) {
		bool Placed = false;
		for (size_t i = Lines.size(); i-- > 0;) {
			if (Lines[i].Source == Kept.Line && !Lines[i].Closing) {
				Lines[i].Text += " " + Comment(Kept.Text);
				Placed = true;
				break;
			}
		}
		if (!Placed) {
			for (TRaidComment &Other : Comments) {
				if (Other.Line == Kept.Line) {
					Other.Trailing = false;
				}
			}
		}
	}
	for (const TRaidComment &Kept : Comments) {
		if (Kept.Trailing) {
			continue;
		}
		size_t At = 0;
		while (At < Lines.size() && Lines[At].Source <= Kept.Line) {
			At++;
		}
		bool Inside = At < Lines.size() && (Lines[At].Inside || Lines[At].Closing);
		TOutLine Line;
		Line.Source = Kept.Line;
		Line.Text = (Inside ? "\t" : "") + Comment(Kept.Text);
		Line.Inside = Inside;
		Lines.insert(Lines.begin() + static_cast<long>(At), Line);
	}

	std::string Text;
	for (size_t i = 0; i < Lines.size(); i++) {
		if (i > 0) {
			int From = Lines[i - 1].Source, To = Lines[i].Source;
			auto Blank = std::upper_bound(Layout.BlankLines.begin(), Layout.BlankLines.end(), From);
			if (Blank != Layout.BlankLines.end() && *Blank < To) {
				Text += "\n";
			}
		}
		Text += Lines[i].Text + "\n";
	}
	return Text;
}

} // namespace

std::string TomlValue(const TScriptValue &Value) {
	switch (Value.Kind) {
		case TScriptValue::NUMBER:
			return Int(Value.Number);
		case TScriptValue::TEXT:
			return TomlQuote(Value.Text);
		case TScriptValue::ARRAY: {
			std::string Text = "[";
			for (size_t i = 0; i < Value.Elements.size(); i++) {
				Text += (i > 0 ? ", " : "") + TomlValue(Value.Elements[i]);
			}
			return Text + "]";
		}
		case TScriptValue::TABLE: {
			std::string Text = "{ ";
			for (size_t i = 0; i < Value.Fields.size(); i++) {
				Text += (i > 0 ? ", " : "") + Value.Fields[i].first + " = " + TomlValue(Value.Fields[i].second);
			}
			return Text + " }";
		}
	}
	return "";
}

std::string MonstersToToml(const std::vector<TMonsterData> &Races) {
	std::string Text = MONSTERS_HEADER;
	for (const TMonsterData &Race : Races) {
		Text += "\n" + MonsterEntry(Race);
	}
	return Text;
}

std::string RaidsToToml(const std::vector<TRaidFile> &Raids) {
	std::string Text = RAIDS_HEADER;
	for (const TRaidFile &Raid : Raids) {
		Text += "\n" + RaidEntry(Raid);
	}
	return Text;
}

std::string MonsterHomesToToml(const std::vector<TMonsterHomeData> &Homes) {
	std::string Text = HOMES_HEADER;
	Text += "\nhomes = [\n";
	for (const TMonsterHomeData &Home : Homes) {
		Text += "\t{ race = " + TomlQuote(Home.Race) + ", position = [" + Int(Home.x) + ", " + Int(Home.y) + ", "
				+ Int(Home.z) + "], radius = " + Int(Home.Radius) + ", amount = " + Int(Home.Amount)
				+ ", regeneration = " + Int(Home.Regeneration) + " },\n";
	}
	return Text + "]\n";
}

void WriteMonsterFiles(const TMonsterFolder &Folder, const std::vector<TMonsterHomeData> &Homes,
		const char *DataFolder) {
	namespace fs = std::filesystem;
	fs::path Data(DataFolder);
	std::error_code Error;
	fs::create_directories(Data, Error);
	WriteTextFile((Data / "monsters.toml").c_str(), MonstersToToml(Folder.Races));
	WriteTextFile((Data / "raids.toml").c_str(), RaidsToToml(Folder.Raids));
	WriteTextFile((Data / "monster_homes.toml").c_str(), MonsterHomesToToml(Homes));
}
