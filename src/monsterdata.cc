// monsterdata.cc: the loaders of monsters.toml, raids.toml and
// monster_homes.toml (monsterdata.hh), the monster races, the raids and the
// monster homes of the game (data/ in the repository, converted from the
// original mon/*.mon, mon/*.evt and dat/monster.db by tools/convert; each
// file's header describes its keys). They need no game state, so
// tools/convert links this file too.

#include "tomlfile.hh"

#include <algorithm>
#include <climits>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <iterator>
#include <set>
#include <string>
#include <utility>
#include <vector>

#include "crmain.hh"
#include "monsterdata.hh"

// The longest name and article of a race (TRaceData::Name and Article hold
// them with their terminating zero).
const int MAX_RACE_NAME_LENGTH = 29;
const int MAX_RACE_ARTICLE_LENGTH = 2;

// The ranges of an outfit: its number, a color, the object type of an outfit
// of id 0. An object type (a corpse, an object carried) has the same range.
const int MAX_OUTFIT = 65535;
const int MAX_OUTFIT_COLOR = 255;
const int MAX_OBJECT_TYPE = 65535;

// The range of a field's coordinates: x and y as the map's sectors give them,
// z its floors.
const int MAX_POSITION_XY = 65535;
const int MAX_POSITION_Z = 15;

// The chance of a strategy in percent, of an object carried in 1/1000.
const int MAX_STRATEGY_CHANCE = 100;
const int MAX_ITEM_CHANCE = 1000;

// The longest text (talk, message, description, location): the longest
// string the original files' reader took (TReadScriptFile).
const int MAX_TEXT_LENGTH = 3999;

// The longest script name: the longest LoadScript takes.
const int MAX_SCRIPT_LENGTH = 256;

// "<file>:<line>:<column>" of a place in a file.
static std::string Position(const toml::source_region &Where) {
	char Text[4096 + 64];
	snprintf(Text, sizeof(Text), "%s:%u:%u", Where.path != nullptr ? Where.path->c_str() : "(unknown file)",
			Where.begin.line, Where.begin.column);
	return Text;
}

// An integer of Min..Max; What names it in the error text.
static int ReadInteger(const toml::node &Node, const char *What, int Min, int Max) {
	if (!Node.is_integer()) {
		TomlError(Node.source(), "%s must be an integer", What);
	}
	int64_t Number = Node.as_integer()->get();
	if (Number < Min || Number > Max) {
		TomlError(Node.source(), "%s must lie between %d and %d", What, Min, Max);
	}
	return static_cast<int>(Number);
}

// The value under Key, which the entry Entry (a table What, at Where) must
// have.
static const toml::node &Required(const toml::table &Entry, const toml::source_region &Where, const char *What,
		const char *Key) {
	const toml::node *Node = Entry.get(Key);
	if (Node == nullptr) {
		TomlError(Where, "%s needs %s", What, Key);
	}
	return *Node;
}

static const toml::table &ReadTable(const toml::node &Node, const char *What) {
	if (!Node.is_table()) {
		TomlError(Node.source(), "%s must be a table", What);
	}
	return *Node.as_table();
}

static const toml::array &ReadArray(const toml::node &Node, const char *What) {
	if (!Node.is_array()) {
		TomlError(Node.source(), "%s must be an array", What);
	}
	return *Node.as_array();
}

// A text as Latin-1, without a zero character, of at most MaxLength
// characters.
static std::string ReadText(const toml::node &Node, const char *What, int MaxLength) {
	if (!Node.is_string()) {
		TomlError(Node.source(), "%s must be a string", What);
	}
	std::string Latin1;
	if (!Utf8ToLatin1(Node.as_string()->get(), Latin1)) {
		TomlError(Node.source(), "%s has a character outside Latin-1", What);
	}
	if (Latin1.find('\0') != std::string::npos) {
		TomlError(Node.source(), "%s has a zero character", What);
	}
	if (static_cast<int>(Latin1.size()) > MaxLength) {
		TomlError(Node.source(), "%s is longer than %d characters", What, MaxLength);
	}
	return Latin1;
}

// A key of a race or a raid: 1 to MAX_MONSTER_KEY_LENGTH characters of a-z,
// 0-9 and '_'.
static std::string ReadKey(const toml::node &Node, const char *What) {
	if (!Node.is_string()) {
		TomlError(Node.source(), "%s must be a string", What);
	}
	const std::string &Key = Node.as_string()->get();
	bool Valid = !Key.empty() && Key.size() <= static_cast<size_t>(MAX_MONSTER_KEY_LENGTH);
	for (char c : Key) {
		if (!((c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || c == '_')) {
			Valid = false;
		}
	}
	if (!Valid) {
		TomlError(Node.source(), "%s \"%s\" must be 1 to %d characters of a-z, 0-9 and _", What, Key.c_str(),
				MAX_MONSTER_KEY_LENGTH);
	}
	return Key;
}

// A race key that names one of Keys (the races loaded before).
static std::string ReadRaceKey(const toml::node &Node, const char *What, const std::set<std::string> &Keys) {
	std::string Key = ReadKey(Node, What);
	if (Keys.count(Key) == 0) {
		TomlError(Node.source(), "%s \"%s\" is no race of monsters.toml", What, Key.c_str());
	}
	return Key;
}

// The outfit, an inline table: { id = <1 to 65535>, colors = [<head>, <body>,
// <legs>, <feet>] } with each color 0 to 255, or { id = 0, object_type = <0 to
// 65535> }. The bytes of the union the outfit does not use are 0.
static TOutfit ReadOutfit(const toml::node &Node) {
	if (!Node.is_table()) {
		TomlError(Node.source(),
				"outfit must be a table { id = ..., colors = [...] } or { id = 0, object_type = ... }");
	}
	const toml::table &Table = *Node.as_table();
	TomlCheckKeys(Table, "outfit", {"id", "colors", "object_type"});

	TOutfit Outfit;
	memset(&Outfit, 0, sizeof(Outfit));
	Outfit.OutfitID = ReadInteger(Required(Table, Node.source(), "outfit", "id"), "id of outfit", 0, MAX_OUTFIT);

	const toml::node *Colors = Table.get("colors");
	const toml::node *ObjectType = Table.get("object_type");
	if (Outfit.OutfitID == 0) {
		if (Colors != nullptr) {
			TomlError(Colors->source(), "an outfit of id 0 has no colors (it gives object_type)");
		}
		if (ObjectType == nullptr) {
			TomlError(Node.source(), "an outfit of id 0 needs object_type");
		}
		Outfit.ObjectType = static_cast<unsigned short>(
				ReadInteger(*ObjectType, "object_type of outfit", 0, MAX_OBJECT_TYPE));
	} else {
		if (ObjectType != nullptr) {
			TomlError(ObjectType->source(), "only an outfit of id 0 has object_type");
		}
		if (Colors == nullptr) {
			TomlError(Node.source(), "an outfit of id 1 or more needs colors");
		}
		const toml::array *Array = Colors->as_array();
		if (Array == nullptr || Array->size() != 4) {
			TomlError(Colors->source(), "colors of outfit must be an array [<head>, <body>, <legs>, <feet>]");
		}
		for (int i = 0; i < 4; i++) {
			Outfit.Colors[i] = static_cast<unsigned char>(
					ReadInteger(*Array->get(static_cast<size_t>(i)), "a color of outfit", 0, MAX_OUTFIT_COLOR));
		}
	}
	return Outfit;
}

// A field [x, y, z]: x and y MinXY to 65535, z 0 to 15.
static void ReadPosition(const toml::node &Node, const char *What, int MinXY, int &x, int &y, int &z) {
	const toml::array *Array = Node.as_array();
	if (Array == nullptr || Array->size() != 3) {
		TomlError(Node.source(), "%s must be an array [x, y, z]", What);
	}
	std::string Name = std::string("x of ") + What;
	x = ReadInteger(*Array->get(0), Name.c_str(), MinXY, MAX_POSITION_XY);
	Name = std::string("y of ") + What;
	y = ReadInteger(*Array->get(1), Name.c_str(), MinXY, MAX_POSITION_XY);
	Name = std::string("z of ") + What;
	z = ReadInteger(*Array->get(2), Name.c_str(), 0, MAX_POSITION_Z);
}

// An inventory: an array of { type, maximum, chance }, the object type (0 to
// 65535), the most of it (1 or more) and the chance in 1/1000 (0 to 1000).
static std::vector<TMonsterItemData> ReadInventory(const toml::node &Node) {
	std::vector<TMonsterItemData> Inventory;
	for (const toml::node &Element : ReadArray(Node, "inventory")) {
		const toml::table &Table = ReadTable(Element, "an entry of inventory");
		TomlCheckKeys(Table, "an entry of inventory", {"type", "maximum", "chance"});
		TMonsterItemData Item;
		Item.Type = ReadInteger(Required(Table, Element.source(), "an entry of inventory", "type"), "type", 0,
				MAX_OBJECT_TYPE);
		Item.Maximum = ReadInteger(Required(Table, Element.source(), "an entry of inventory", "maximum"),
				"maximum", 1, INT_MAX);
		Item.Chance = ReadInteger(Required(Table, Element.source(), "an entry of inventory", "chance"), "chance",
				0, MAX_ITEM_CHANCE);
		Inventory.push_back(Item);
	}
	return Inventory;
}

// The script key of Entry, if there is one: "<Folder>/<name>.lua", a script
// name as scripting.hh defines it (only a-z, 0-9, '_', '-', '/' and '.'; no
// part empty, "." or ".."; a file name that is more than ".lua"; at most
// MAX_SCRIPT_LENGTH characters). Empty when there is none.
static std::string ReadScript(const toml::table &Entry, const char *Folder) {
	const toml::node *Node = Entry.get("script");
	if (Node == nullptr) {
		return std::string();
	}
	if (!Node->is_string()) {
		TomlError(Node->source(), "script must be a string");
	}
	const std::string &Name = Node->as_string()->get();
	const std::string Prefix = std::string(Folder) + "/";
	const std::string Ending = ".lua";
	bool Valid = Name.size() <= static_cast<size_t>(MAX_SCRIPT_LENGTH)
			&& Name.size() > Prefix.size() + Ending.size()
			&& Name.compare(0, Prefix.size(), Prefix) == 0
			&& Name.compare(Name.size() - Ending.size(), Ending.size(), Ending) == 0;
	size_t PartStart = 0;
	for (size_t i = 0; Valid && i <= Name.size(); i++) {
		char c = i < Name.size() ? Name[i] : 0;
		if (c == 0 && i < Name.size()) {
			Valid = false;
		} else if (c == '/' || c == 0) {
			std::string Part = Name.substr(PartStart, i - PartStart);
			if (Part.empty() || Part == "." || Part == ".." || (c == 0 && Part == Ending)) {
				Valid = false;
			}
			PartStart = i + 1;
		} else if (!((c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || c == '_' || c == '-' || c == '.')) {
			Valid = false;
		}
	}
	if (!Valid) {
		TomlError(Node->source(), "script \"%s\" must be a script name \"%s/<name>.lua\" (a-z, 0-9, _, -, / and ., "
				"no part . or .., at most %d characters)", Name.c_str(), Folder, MAX_SCRIPT_LENGTH);
	}
	return Name;
}

// A value of a spell as it is: an integer (a C++ int), a string (Latin-1,
// without a zero character), an array or a table (its keys in the file's
// order; toml++ keeps them sorted, so they are put back in the order of their
// places).
static TScriptValue ReadValue(const toml::node &Node) {
	TScriptValue Value;
	if (Node.is_integer()) {
		Value.Kind = TScriptValue::NUMBER;
		Value.Number = ReadInteger(Node, "a number of a spell", INT_MIN, INT_MAX);
	} else if (Node.is_string()) {
		Value.Kind = TScriptValue::TEXT;
		Value.Text = ReadText(Node, "a text of a spell", MAX_TEXT_LENGTH);
	} else if (Node.is_array()) {
		Value.Kind = TScriptValue::ARRAY;
		for (const toml::node &Element : *Node.as_array()) {
			Value.Elements.push_back(ReadValue(Element));
		}
	} else if (Node.is_table()) {
		Value.Kind = TScriptValue::TABLE;
		std::vector<std::pair<toml::source_position, std::pair<std::string, TScriptValue>>> Fields;
		for (const auto &Field : *Node.as_table()) {
			Fields.emplace_back(Field.first.source().begin,
					std::make_pair(std::string(Field.first.str()), ReadValue(Field.second)));
		}
		std::stable_sort(Fields.begin(), Fields.end(), [](const auto &a, const auto &b) {
			return a.first.line != b.first.line ? a.first.line < b.first.line : a.first.column < b.first.column;
		});
		for (auto &Field : Fields) {
			Value.Fields.push_back(std::move(Field.second));
		}
	} else {
		TomlError(Node.source(), "a value of a spell must be an integer, a string, an array or a table");
	}
	return Value;
}

// The names of a race's flags (monsterdata.hh).
static const char *const RaceFlags[] = {
	"kick_boxes", "kick_creatures", "see_invisible", "unpushable", "distance_fighting", "no_summon",
	"no_convince", "no_illusion", "no_burning", "no_poison", "no_energy", "no_hit", "no_life_drain",
	"no_paralyze",
};

// One [[monster]]: every key of the header that is not optional needed, no
// others.
static TMonsterData ReadRace(const toml::node &Node) {
	const toml::table &Entry = ReadTable(Node, "an entry of monster");
	const toml::source_region &Where = Node.source();
	const char *What = "[[monster]]";
	TomlCheckKeys(Entry, What, {"key", "number", "name", "article", "outfit", "corpse", "female_corpse", "blood",
			"experience", "summon_cost", "flee_threshold", "attack", "defend", "armor", "poison", "lose_target",
			"strategy", "flags", "skills", "talk", "inventory", "script", "spell"});

	TMonsterData Race;
	Race.Where = Position(Where);
	Race.Key = ReadKey(Required(Entry, Where, What, "key"), "key");
	Race.Number = ReadInteger(Required(Entry, Where, What, "number"), "number", 1, MAX_RACES - 1);

	const toml::node &Name = Required(Entry, Where, What, "name");
	Race.Name = ReadText(Name, "name", MAX_RACE_NAME_LENGTH);
	if (Race.Name.empty()) {
		TomlError(Name.source(), "name must not be empty");
	}

	const toml::node &Article = Required(Entry, Where, What, "article");
	Race.Article = ReadText(Article, "article", MAX_RACE_ARTICLE_LENGTH);
	if (Race.Article != "a" && Race.Article != "an" && !Race.Article.empty()) {
		TomlError(Article.source(), "article must be \"a\", \"an\" or \"\"");
	}

	Race.Outfit = ReadOutfit(Required(Entry, Where, What, "outfit"));
	Race.Corpse = ReadInteger(Required(Entry, Where, What, "corpse"), "corpse", 0, MAX_OBJECT_TYPE);
	Race.FemaleCorpse = Race.Corpse;
	if (const toml::node *FemaleCorpse = Entry.get("female_corpse")) {
		Race.FemaleCorpse = ReadInteger(*FemaleCorpse, "female_corpse", 0, MAX_OBJECT_TYPE);
	}

	const toml::node &Blood = Required(Entry, Where, What, "blood");
	Race.Blood = ReadText(Blood, "blood", MAX_TEXT_LENGTH);
	if (Race.Blood != "blood" && Race.Blood != "slime" && Race.Blood != "bones" && Race.Blood != "fire"
			&& Race.Blood != "energy") {
		TomlError(Blood.source(), "blood must be \"blood\", \"slime\", \"bones\", \"fire\" or \"energy\"");
	}

	Race.Experience = ReadInteger(Required(Entry, Where, What, "experience"), "experience", INT_MIN, INT_MAX);
	Race.SummonCost = ReadInteger(Required(Entry, Where, What, "summon_cost"), "summon_cost", INT_MIN, INT_MAX);
	Race.FleeThreshold = ReadInteger(Required(Entry, Where, What, "flee_threshold"), "flee_threshold", INT_MIN,
			INT_MAX);
	Race.Attack = ReadInteger(Required(Entry, Where, What, "attack"), "attack", INT_MIN, INT_MAX);
	Race.Defend = ReadInteger(Required(Entry, Where, What, "defend"), "defend", INT_MIN, INT_MAX);
	Race.Armor = ReadInteger(Required(Entry, Where, What, "armor"), "armor", INT_MIN, INT_MAX);
	Race.Poison = ReadInteger(Required(Entry, Where, What, "poison"), "poison", INT_MIN, INT_MAX);
	Race.LoseTarget = ReadInteger(Required(Entry, Where, What, "lose_target"), "lose_target", INT_MIN, INT_MAX);

	const toml::node &Strategy = Required(Entry, Where, What, "strategy");
	const toml::array *StrategyArray = Strategy.as_array();
	if (StrategyArray == nullptr || StrategyArray->size() != 4) {
		TomlError(Strategy.source(), "strategy must be an array [<nearest>, <weakest>, <most damage>, <random>]");
	}
	for (int i = 0; i < 4; i++) {
		Race.Strategy[i] = ReadInteger(*StrategyArray->get(static_cast<size_t>(i)), "a chance of strategy", 0,
				MAX_STRATEGY_CHANCE);
	}

	if (const toml::node *Flags = Entry.get("flags")) {
		for (const toml::node &Flag : ReadArray(*Flags, "flags")) {
			std::string FlagName = ReadText(Flag, "a flag", MAX_TEXT_LENGTH);
			if (std::find_if(std::begin(RaceFlags), std::end(RaceFlags),
					[&FlagName](const char *Known) { return FlagName == Known; }) == std::end(RaceFlags)) {
				TomlError(Flag.source(), "unknown flag \"%s\"", FlagName.c_str());
			}
			Race.Flags.push_back(FlagName);
		}
	}

	if (const toml::node *Skills = Entry.get("skills")) {
		for (const toml::node &Element : ReadArray(*Skills, "skills")) {
			const char *SkillWhat = "an entry of skills";
			const toml::table &Table = ReadTable(Element, SkillWhat);
			const toml::source_region &SkillWhere = Element.source();
			TomlCheckKeys(Table, SkillWhat,
					{"skill", "actual", "minimum", "maximum", "next_level", "factor_percent", "add_level"});
			TMonsterSkillData Skill;
			const toml::node &SkillName = Required(Table, SkillWhere, SkillWhat, "skill");
			Skill.Skill = ReadText(SkillName, "skill", MAX_TEXT_LENGTH);
			Skill.Where = Position(SkillName.source());
			Skill.Actual = ReadInteger(Required(Table, SkillWhere, SkillWhat, "actual"), "actual", INT_MIN,
					INT_MAX);
			Skill.Minimum = ReadInteger(Required(Table, SkillWhere, SkillWhat, "minimum"), "minimum", INT_MIN,
					INT_MAX);
			Skill.Maximum = ReadInteger(Required(Table, SkillWhere, SkillWhat, "maximum"), "maximum", INT_MIN,
					INT_MAX);
			Skill.NextLevel = ReadInteger(Required(Table, SkillWhere, SkillWhat, "next_level"), "next_level",
					INT_MIN, INT_MAX);
			Skill.FactorPercent = ReadInteger(Required(Table, SkillWhere, SkillWhat, "factor_percent"),
					"factor_percent", INT_MIN, INT_MAX);
			Skill.AddLevel = ReadInteger(Required(Table, SkillWhere, SkillWhat, "add_level"), "add_level",
					INT_MIN, INT_MAX);
			Race.Skills.push_back(Skill);
		}
	}

	if (const toml::node *Talk = Entry.get("talk")) {
		for (const toml::node &Text : ReadArray(*Talk, "talk")) {
			Race.Talk.push_back(ReadText(Text, "a text of talk", MAX_TEXT_LENGTH));
		}
	}

	if (const toml::node *Inventory = Entry.get("inventory")) {
		Race.Inventory = ReadInventory(*Inventory);
	}

	Race.Script = ReadScript(Entry, "monsters");

	if (const toml::node *Spells = Entry.get("spell")) {
		for (const toml::node &Spell : ReadArray(*Spells, "spell")) {
			ReadTable(Spell, "an entry of spell");
			Race.Spells.push_back(ReadValue(Spell));
			Race.SpellWhere.push_back(Position(Spell.source()));
		}
	}
	return Race;
}

void LoadMonsterRaces(const char *FileName, std::vector<TMonsterData> &Races) {
	Races.clear();
	toml::table File = TomlParseFile(FileName);
	TomlCheckKeys(File, "the top level", {"monster"});
	std::set<std::string> Keys;
	std::set<int> Numbers;
	if (const toml::array *Entries = TomlGetArray(File, "monster")) {
		for (const toml::node &Node : *Entries) {
			TMonsterData Race = ReadRace(Node);
			if (!Keys.insert(Race.Key).second) {
				TomlError(Node.as_table()->get("key")->source(), "race key \"%s\" given twice", Race.Key.c_str());
			}
			if (!Numbers.insert(Race.Number).second) {
				TomlError(Node.as_table()->get("number")->source(), "race number %d given twice", Race.Number);
			}
			Races.push_back(std::move(Race));
		}
	}
}

// The keys of the races, for the checks of the race keys of raids and homes.
static std::set<std::string> RaceKeys(const std::vector<TMonsterData> &Races) {
	std::set<std::string> Keys;
	for (const TMonsterData &Race : Races) {
		Keys.insert(Race.Key);
	}
	return Keys;
}

// One [[raid.wave]].
static TRaidWaveData ReadWave(const toml::node &Node, const std::set<std::string> &Races) {
	const toml::table &Entry = ReadTable(Node, "an entry of wave");
	const toml::source_region &Where = Node.source();
	const char *What = "[[raid.wave]]";
	TomlCheckKeys(Entry, What, {"delay", "position", "spread", "race", "count", "radius", "lifetime", "message",
			"location", "inventory"});

	TRaidWaveData Wave;
	Wave.Where = Position(Where);
	Wave.Delay = ReadInteger(Required(Entry, Where, What, "delay"), "delay", 0, INT_MAX);
	// x 0 counted as no position in the original files.
	ReadPosition(Required(Entry, Where, What, "position"), "position", 1, Wave.x, Wave.y, Wave.z);
	Wave.Spread = ReadInteger(Required(Entry, Where, What, "spread"), "spread", 0, MAX_POSITION_XY);
	Wave.Race = ReadRaceKey(Required(Entry, Where, What, "race"), "race", Races);

	const toml::node &Count = Required(Entry, Where, What, "count");
	const toml::array *CountArray = Count.as_array();
	if (CountArray == nullptr || CountArray->size() != 2) {
		TomlError(Count.source(), "count must be an array [<min>, <max>]");
	}
	Wave.MinCount = ReadInteger(*CountArray->get(0), "min of count", 0, INT_MAX);
	Wave.MaxCount = ReadInteger(*CountArray->get(1), "max of count", 1, INT_MAX);
	if (Wave.MinCount > Wave.MaxCount) {
		TomlError(Count.source(), "min of count is greater than max");
	}

	Wave.Radius = INT_MAX;
	if (const toml::node *Radius = Entry.get("radius")) {
		Wave.Radius = ReadInteger(*Radius, "radius", 0, INT_MAX);
	}
	Wave.Lifetime = 0;
	if (const toml::node *Lifetime = Entry.get("lifetime")) {
		Wave.Lifetime = ReadInteger(*Lifetime, "lifetime", 0, INT_MAX);
	}
	if (const toml::node *Message = Entry.get("message")) {
		Wave.Message = ReadText(*Message, "message", MAX_TEXT_LENGTH);
	}
	if (const toml::node *Location = Entry.get("location")) {
		Wave.Location = ReadText(*Location, "location", MAX_TEXT_LENGTH);
	}
	if (const toml::node *Inventory = Entry.get("inventory")) {
		Wave.Inventory = ReadInventory(*Inventory);
	}
	return Wave;
}

// One [[raid]].
static TRaidData ReadRaid(const toml::node &Node, const std::set<std::string> &Races) {
	const toml::table &Entry = ReadTable(Node, "an entry of raid");
	const toml::source_region &Where = Node.source();
	const char *What = "[[raid]]";
	TomlCheckKeys(Entry, What, {"key", "description", "type", "interval", "date", "script", "wave"});

	TRaidData Raid;
	Raid.Where = Position(Where);
	Raid.Key = ReadKey(Required(Entry, Where, What, "key"), "key");
	if (const toml::node *Description = Entry.get("description")) {
		Raid.Description = ReadText(*Description, "description", MAX_TEXT_LENGTH);
	}

	const toml::node &Type = Required(Entry, Where, What, "type");
	std::string TypeName = ReadText(Type, "type", MAX_TEXT_LENGTH);
	if (TypeName != "small" && TypeName != "big") {
		TomlError(Type.source(), "type must be \"small\" or \"big\"");
	}
	Raid.Big = TypeName == "big";

	const toml::node *Interval = Entry.get("interval");
	const toml::node *Date = Entry.get("date");
	if (Interval == nullptr && Date == nullptr) {
		TomlError(Where, "[[raid]] needs interval or date");
	}
	if (Interval != nullptr && Date != nullptr) {
		TomlError(Date->source(), "a raid has interval or date, not both");
	}
	Raid.Interval = Interval != nullptr ? ReadInteger(*Interval, "interval", 1, INT_MAX) : 0;
	Raid.Date = Date != nullptr ? ReadInteger(*Date, "date", 1, INT_MAX) : 0;

	Raid.Script = ReadScript(Entry, "raids");

	const toml::array &Waves = ReadArray(Required(Entry, Where, What, "wave"), "wave");
	if (Waves.empty()) {
		TomlError(Where, "[[raid]] needs a wave");
	}
	for (const toml::node &Wave : Waves) {
		Raid.Waves.push_back(ReadWave(Wave, Races));
	}
	return Raid;
}

void LoadRaids(const char *FileName, const std::vector<TMonsterData> &Races, std::vector<TRaidData> &Raids) {
	Raids.clear();
	std::set<std::string> Keys = RaceKeys(Races);
	toml::table File = TomlParseFile(FileName);
	TomlCheckKeys(File, "the top level", {"raid"});
	std::set<std::string> RaidKeys;
	if (const toml::array *Entries = TomlGetArray(File, "raid")) {
		for (const toml::node &Node : *Entries) {
			TRaidData Raid = ReadRaid(Node, Keys);
			if (!RaidKeys.insert(Raid.Key).second) {
				TomlError(Node.as_table()->get("key")->source(), "raid key \"%s\" given twice", Raid.Key.c_str());
			}
			Raids.push_back(std::move(Raid));
		}
	}
}

// One home, { race, position, radius, amount, regeneration }.
static TMonsterHomeData ReadHome(const toml::node &Node, const std::set<std::string> &Races) {
	const char *What = "an entry of homes";
	const toml::table &Entry = ReadTable(Node, What);
	const toml::source_region &Where = Node.source();
	TomlCheckKeys(Entry, What, {"race", "position", "radius", "amount", "regeneration"});

	TMonsterHomeData Home;
	Home.Where = Position(Where);
	Home.Race = ReadRaceKey(Required(Entry, Where, What, "race"), "race", Races);
	ReadPosition(Required(Entry, Where, What, "position"), "position", 0, Home.x, Home.y, Home.z);
	Home.Radius = ReadInteger(Required(Entry, Where, What, "radius"), "radius", 0, INT_MAX);
	Home.Amount = ReadInteger(Required(Entry, Where, What, "amount"), "amount", 0, INT_MAX);
	Home.Regeneration = ReadInteger(Required(Entry, Where, What, "regeneration"), "regeneration", 0, INT_MAX);
	return Home;
}

void LoadMonsterHomes(const char *FileName, const std::vector<TMonsterData> &Races,
		std::vector<TMonsterHomeData> &Homes) {
	Homes.clear();
	std::set<std::string> Keys = RaceKeys(Races);
	toml::table File = TomlParseFile(FileName);
	TomlCheckKeys(File, "the top level", {"homes"});
	if (const toml::array *Entries = TomlGetArray(File, "homes")) {
		for (const toml::node &Node : *Entries) {
			Homes.push_back(ReadHome(Node, Keys));
		}
	}
}
