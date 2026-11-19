// magicdata.cc: the data of the spells (magicdata.hh): the syllables of their
// words; the spell table, loaded from spells.toml (data/spells.toml in the
// repository, written by tools/convert from the table the game had in its
// code) and, for the commands, from commands.toml (data/commands.toml, written
// by tools/convert from that table and the rights the code checked); the
// rights of the commands, from commands.toml; the scripts the entries of
// spells.toml and commands.toml name (their key script), which magic.cc loads;
// and the circles, loaded from circles.toml (data/circles.toml, converted from
// the original circles.dat by tools/convert). The game reads the three files
// into a staged set of these tables, which then changes places with the live
// one (ReadStagedMagicData, SwapStagedMagicData), at start and on a reload;
// the converter reads them into the live tables (LoadCircles, LoadSpells,
// LoadCommands).

#include "tomlfile.hh"

#include <climits>
#include <cstdio>
#include <cstring>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include "magicdata.hh"
#include "mudinc.hh"
#include "strings.hh"

char SpellSyllable[MAX_SYLLABLES][6] = {
	"",
	"al",
	"ad",
	"ex",
	"ut",
	"om",
	"para",
	"ana",
	"evo",
	"ori",
	"mort",
	"lux",
	"liber",
	"vita",
	"flam",
	"pox",
	"hur",
	"moe",
	"ani",
	"ina",
	"eta",
	"amo",
	"hora",
	"gran",
	"cogni",
	"res",
	"mas",
	"vis",
	"som",
	"aqua",
	"frigo",
	"tera",
	"ura",
	"sio",
	"grav",
	"ito",
	"pan",
	"vid",
	"isa",
	"iva",
	"con",
	"",
	"",
	"",
	"",
	"",
	"",
	"",
	"",
	"",
	"",
};
TSpellList SpellList[MAX_SPELLS];
int CommandRight[MAX_SPELLS][MAX_COMMAND_RIGHTS];
TCircle Circle[MAX_CIRCLES];

// The script of each spell and command (the key script) and where the key is,
// "<file>:<line>:<column>"; both empty for none.
static std::string SpellScript[MAX_SPELLS];
static std::string SpellScriptPosition[MAX_SPELLS];

// The staged set of the reload (ReadStagedMagicData): the same tables.
static TSpellList StagedSpellList[MAX_SPELLS];
static int StagedCommandRight[MAX_SPELLS][MAX_COMMAND_RIGHTS];
static TCircle StagedCircle[MAX_CIRCLES];
static std::string StagedSpellScript[MAX_SPELLS];
static std::string StagedSpellScriptPosition[MAX_SPELLS];

// The talkactions, live and staged (ReadStagedTalkactions).
static std::vector<TTalkaction> Talkactions;
static std::vector<TTalkaction> StagedTalkactions;

// The tables a read fills: the live set or the staged set.
struct TMagicTables {
	TSpellList *Spells;
	int (*Rights)[MAX_COMMAND_RIGHTS];
	TCircle *Circles;
	std::string *Scripts;
	std::string *ScriptPositions;
	// A spell's name may take one of SpellNames[0] to
	// SpellNames[ReusableNames - 1] again (ReadStagedMagicData); 0 for the
	// loaders of the live set.
	size_t ReusableNames;
};

static TMagicTables LiveTables() {
	return {SpellList, CommandRight, Circle, SpellScript, SpellScriptPosition, 0};
}

// Every name of a spell or command the reads have made a static string of
// (AddStaticString), in the order made.
static std::vector<const char *> SpellNames;

// The text of the error ReadStagedMagicData throws itself.
static char MagicDataError[512];

// The longest script name the key script may give.
const int MAX_SCRIPT_NAME_LENGTH = 255;

// The names of the rights, by their number (RIGHT in info.hh).
const char *const RightNames[NUMBER_OF_RIGHTS] = {
	"PREMIUM_ACCOUNT",
	"NOTATION",
	"NAMELOCK",
	"STATEMENT_REPORT",
	"BANISHMENT",
	"FINAL_WARNING",
	"IP_BANISHMENT",
	"KICK",
	"HOME_TELEPORT",
	"GAMEMASTER_BROADCAST",
	"ANONYMOUS_BROADCAST",
	"NO_BANISHMENT",
	"ALLOW_MULTICLIENT",
	"LOG_COMMUNICATION",
	"READ_GAMEMASTER_CHANNEL",
	"READ_TUTOR_CHANNEL",
	"HIGHLIGHT_HELP_CHANNEL",
	"SEND_BUGREPORTS",
	"NAME_INSULTING",
	"NAME_SENTENCE",
	"NAME_NONSENSICAL_LETTERS",
	"NAME_BADLY_FORMATTED",
	"NAME_NO_PERSON",
	"NAME_CELEBRITY",
	"NAME_COUNTRY",
	"NAME_FAKE_IDENTITY",
	"NAME_FAKE_POSITION",
	"STATEMENT_INSULTING",
	"STATEMENT_SPAMMING",
	"STATEMENT_ADVERT_OFFTOPIC",
	"STATEMENT_ADVERT_MONEY",
	"STATEMENT_NON_ENGLISH",
	"STATEMENT_CHANNEL_OFFTOPIC",
	"STATEMENT_VIOLATION_INCITING",
	"CHEATING_BUG_ABUSE",
	"CHEATING_GAME_WEAKNESS",
	"CHEATING_MACRO_USE",
	"CHEATING_MODIFIED_CLIENT",
	"CHEATING_HACKING",
	"CHEATING_MULTI_CLIENT",
	"CHEATING_ACCOUNT_TRADING",
	"CHEATING_ACCOUNT_SHARING",
	"GAMEMASTER_THREATENING",
	"GAMEMASTER_PRETENDING",
	"GAMEMASTER_INFLUENCE",
	"GAMEMASTER_FALSE_REPORTS",
	"KILLING_EXCESSIVE_UNJUSTIFIED",
	"DESTRUCTIVE_BEHAVIOUR",
	"SPOILING_AUCTION",
	"INVALID_PAYMENT",
	"TELEPORT_TO_CHARACTER",
	"TELEPORT_TO_MARK",
	"TELEPORT_VERTICAL",
	"TELEPORT_TO_COORDINATE",
	"LEVITATE",
	"SPECIAL_MOVEUSE",
	"MODIFY_GOSTRENGTH",
	"SHOW_COORDINATE",
	"RETRIEVE",
	"ENTER_HOUSES",
	"OPEN_NAMEDOORS",
	"INVULNERABLE",
	"UNLIMITED_MANA",
	"KEEP_INVENTORY",
	"ALL_SPELLS",
	"UNLIMITED_CAPACITY",
	"ZERO_CAPACITY",
	"ATTACK_EVERYWHERE",
	"NO_ATTACK",
	"NO_RUNES",
	"NO_LOGOUT_BLOCK",
	"GAMEMASTER_OUTFIT",
	"ILLUMINATE",
	"CHANGE_PROFESSION",
	"IGNORED_BY_MONSTERS",
	"SHOW_KEYHOLE_NUMBERS",
	"CREATE_OBJECTS",
	"CREATE_MONEY",
	"CREATE_MONSTERS",
	"CHANGE_SKILLS",
	"CLEANUP_FIELDS",
	"NO_STATISTICS",
	"RELOAD_SCRIPTS",
	"ONLINE_LIST",
};

// The flags of a spell as spells.toml names them, by their bit in
// TSpellList's Flags.
static const struct {
	const char *Key;
	int Flag;
} SpellFlagKeys[] = {
	{"aggressive", SPELLFLAG_AGGRESSIVE},
	{"premium", SPELLFLAG_PREMIUM},
	{"factor_max_100", SPELLFLAG_FACTOR_MAX_100},
	{"factor_min_100", SPELLFLAG_FACTOR_MIN_100},
};

// The number of a syllable's text, or 0 for none.
static int GetSyllableByText(const std::string &Text) {
	for (int i = 1; i < MAX_SYLLABLES; i++) {
		if (SpellSyllable[i][0] != 0 && Text == SpellSyllable[i]) {
			return i;
		}
	}
	return 0;
}

// Fills Syllable from the words of a spell as the game writes them
// (GetSpellString): the class syllable and the second syllable as one word,
// each further syllable as a word of its own, separated by single spaces. The
// words of a command (Command) start with al or om, those of a spell with ad,
// ex or ut.
static void ReadWords(const toml::table &Entry, unsigned char *Syllable, bool Command) {
	const toml::node *Node = Entry.get("words");
	if (!Node->is_string()) {
		TomlError(Node->source(), "words must be a string");
	}

	const std::string &Words = Node->as_string()->get();
	std::vector<std::string> Parts;
	size_t Start = 0;
	while (true) {
		size_t End = Words.find(' ', Start);
		Parts.push_back(Words.substr(Start, End == std::string::npos ? std::string::npos : End - Start));
		if (End == std::string::npos) {
			break;
		}
		Start = End + 1;
	}
	for (const std::string &Part : Parts) {
		if (Part.empty()) {
			TomlError(Node->source(), "words must be syllables separated by single spaces");
		}
	}

	// The class syllable (al, ad, ex, ut or om) is the first two letters.
	std::string Class = Parts[0].substr(0, 2);
	int ClassSyllable = GetSyllableByText(Class);
	bool CommandClass = ClassSyllable == SYLLABLE_AL || ClassSyllable == SYLLABLE_OM;
	bool SpellClass = ClassSyllable == SYLLABLE_AD || ClassSyllable == SYLLABLE_EX || ClassSyllable == SYLLABLE_UT;
	if (Command && !CommandClass) {
		TomlError(Node->source(), "words must start with al or om (ad, ex and ut start spells, in spells.toml)");
	}
	if (!Command && !SpellClass) {
		TomlError(Node->source(), "words must start with ad, ex or ut (al and om start commands, in commands.toml)");
	}
	Parts[0] = Parts[0].substr(2);
	if (Parts[0].empty()) {
		if (Parts.size() > 1) {
			TomlError(Node->source(), "words must be syllables separated by single spaces");
		}
		Parts.clear();
	}
	if (1 + Parts.size() > static_cast<size_t>(MAX_SPELL_SYLLABLES)) {
		TomlError(Node->source(), "words have more than %d syllables", MAX_SPELL_SYLLABLES);
	}

	Syllable[0] = static_cast<unsigned char>(ClassSyllable);
	for (size_t i = 0; i < Parts.size(); i++) {
		int Number = GetSyllableByText(Parts[i]);
		if (Number == 0) {
			TomlError(Node->source(), "unknown syllable \"%s\" in words", Parts[i].c_str());
		}
		Syllable[i + 1] = static_cast<unsigned char>(Number);
	}
}

// Whether Name is a script name as the scripting engine takes it (LoadScript):
// only a-z, 0-9, '_', '-', '/' and '.'; parts between single '/' that are
// neither empty nor "." nor ".."; a file name that is more than the ending
// ".lua"; at most MAX_SCRIPT_NAME_LENGTH characters.
static bool ValidScriptName(const std::string &Name) {
	size_t Length = Name.size();
	if (Length > static_cast<size_t>(MAX_SCRIPT_NAME_LENGTH) || Length < 4
			|| Name.compare(Length - 4, 4, ".lua") != 0) {
		return false;
	}

	size_t PartStart = 0;
	for (size_t i = 0; i <= Length; i++) {
		char c = i < Length ? Name[i] : 0;
		if (c == '/' || i == Length) {
			size_t PartLength = i - PartStart;
			if (PartLength == 0 || Name.compare(PartStart, PartLength, ".") == 0
					|| Name.compare(PartStart, PartLength, "..") == 0) {
				return false;
			}
			if (i == Length && PartLength <= 4) {
				// The file name is only ".lua".
				return false;
			}
			PartStart = i + 1;
		} else if (!((c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || c == '_' || c == '-' || c == '.')) {
			return false;
		}
	}
	return true;
}

// Reads the key script of the entry of SpellNr, a script name below Folder
// ("spells/", "runes/" or "commands/"); What names the entry's kind in the
// error text. No key: the entry has no script.
static void ReadScript(const toml::table &Entry, int SpellNr, const char *Folder, const char *What,
		const TMagicTables &Tables) {
	Tables.Scripts[SpellNr].clear();
	Tables.ScriptPositions[SpellNr].clear();
	auto Found = Entry.find("script");
	if (Found == Entry.end()) {
		return;
	}
	// The errors and Where name the key's place.
	const toml::source_region &Where = Found->first.source();
	const toml::node &Node = Found->second;
	if (!Node.is_string()) {
		TomlError(Where, "script must be a string");
	}

	const std::string &Name = Node.as_string()->get();
	if (!ValidScriptName(Name)) {
		TomlError(Where, "script \"%s\" is not a script name (a-z, 0-9, _, -, / and ., no part . or .., "
				"ending in .lua, at most %d characters)", Name.c_str(), MAX_SCRIPT_NAME_LENGTH);
	}
	size_t FolderLength = strlen(Folder);
	if (Name.compare(0, FolderLength, Folder) != 0) {
		TomlError(Where, "the script of %s must be in %s", What, Folder);
	}

	char Position[4096 + 64];
	snprintf(Position, sizeof(Position), "%s:%u:%u", Where.path != nullptr ? Where.path->c_str() : "(unknown file)",
			Where.begin.line, Where.begin.column);
	Tables.Scripts[SpellNr] = Name;
	Tables.ScriptPositions[SpellNr] = Position;
}

// The static string of a spell's or command's name: one an earlier read made
// with the same text (Tables.ReusableNames), or a new one.
static const char *SpellName(const char *Name, const TMagicTables &Tables) {
	for (size_t i = 0; i < Tables.ReusableNames; i++) {
		if (strcmp(SpellNames[i], Name) == 0) {
			return SpellNames[i];
		}
	}
	const char *Text = AddStaticString(Name);
	SpellNames.push_back(Text);
	return Text;
}

// Reads an integer of Min..Max under Key into Value, which stays 0 when there
// is none.
template<typename T>
static void ReadNumber(const toml::table &Table, const char *Key, T &Value, int Min, int Max) {
	int Number = 0;
	TomlGetNumber(Table, Key, Number, Min, Max);
	Value = static_cast<T>(Number);
}

// One [[spell]] of spells.toml. Defined notes the spell numbers filled so
// far.
static void ReadSpell(const toml::node &Node, bool *Defined, const TMagicTables &Tables) {
	if (!Node.is_table()) {
		TomlError(Node.source(), "an entry of spell must be a table");
	}

	const toml::table &Entry = *Node.as_table();
	TomlCheckKeys(Entry, "[[spell]]",
			{"number", "name", "words", "level", "mana", "soul_points", "flags", "rune", "script"});
	for (const char *Key : {"number", "name", "words"}) {
		if (!Entry.contains(Key)) {
			TomlError(Node.source(), "an entry of spell needs %s", Key);
		}
	}

	int SpellNr = 0;
	TomlGetNumber(Entry, "number", SpellNr, 1, MAX_SPELLS - 1);
	if (Defined[SpellNr]) {
		TomlError(Entry.get("number")->source(), "spell %d is defined twice", SpellNr);
	}
	Defined[SpellNr] = true;

	TSpellList &Spell = Tables.Spells[SpellNr];
	memset(&Spell, 0, sizeof(TSpellList));

	char Name[MAX_SPELL_NAME_LENGTH + 1];
	TomlGetString(Entry, "name", Name, sizeof(Name));
	Spell.Comment = SpellName(Name, Tables);

	ReadWords(Entry, Spell.Syllable, false);
	for (int Other = 1; Other < MAX_SPELLS; Other++) {
		if (Other != SpellNr && Defined[Other]
				&& memcmp(Tables.Spells[Other].Syllable, Spell.Syllable, sizeof(Spell.Syllable)) == 0) {
			TomlError(Entry.get("words")->source(), "these words are already the ones of spell %d", Other);
		}
	}

	if (Spell.Syllable[0] == SYLLABLE_AD) {
		ReadScript(Entry, SpellNr, "runes/", "a rune spell (class ad)", Tables);
	} else {
		ReadScript(Entry, SpellNr, "spells/", "a spell of the class ex or ut", Tables);
	}

	ReadNumber(Entry, "level", Spell.Level, 0, USHRT_MAX);
	ReadNumber(Entry, "mana", Spell.Mana, 0, INT_MAX);
	ReadNumber(Entry, "soul_points", Spell.SoulPoints, 0, INT_MAX);

	if (const toml::array *Flags = TomlGetArray(Entry, "flags")) {
		for (const toml::node &FlagNode : *Flags) {
			if (!FlagNode.is_string()) {
				TomlError(FlagNode.source(), "a flag must be a string");
			}
			const std::string &Key = FlagNode.as_string()->get();
			int Flag = 0;
			for (const auto &Known : SpellFlagKeys) {
				if (Key == Known.Key) {
					Flag = Known.Flag;
				}
			}
			if (Flag == 0) {
				TomlError(FlagNode.source(), "unknown flag \"%s\"", Key.c_str());
			}
			Spell.Flags = static_cast<unsigned short>(Spell.Flags | Flag);
		}
	}

	if (const toml::table *Rune = TomlGetTable(Entry, "rune")) {
		TomlCheckKeys(*Rune, "rune", {"group", "number", "magic_level", "charges"});
		for (const char *Key : {"group", "number"}) {
			if (!Rune->contains(Key)) {
				TomlError(Rune->source(), "rune needs %s", Key);
			}
		}
		ReadNumber(*Rune, "group", Spell.RuneGr, 1, UCHAR_MAX);
		ReadNumber(*Rune, "number", Spell.RuneNr, 0, UCHAR_MAX);
		ReadNumber(*Rune, "magic_level", Spell.RuneLevel, 0, USHRT_MAX);
		ReadNumber(*Rune, "charges", Spell.Amount, 0, INT_MAX);
	}
}

// LoadSpells into Tables.
static void ReadSpells(const char *FileName, const TMagicTables &Tables) {
	for (int i = 1; i < MAX_SPELLS; i++) {
		memset(&Tables.Spells[i], 0, sizeof(TSpellList));
	}
	for (int i = 0; i < MAX_SPELLS; i++) {
		Tables.Scripts[i].clear();
		Tables.ScriptPositions[i].clear();
	}

	toml::table Spells = TomlParseFile(FileName);
	TomlCheckKeys(Spells, "the top level", {"spell"});

	bool Defined[MAX_SPELLS] = {};
	if (const toml::array *Entries = TomlGetArray(Spells, "spell")) {
		for (const toml::node &Node : *Entries) {
			ReadSpell(Node, Defined, Tables);
		}
	}
}

void LoadSpells(const char *FileName) {
	ReadSpells(FileName, LiveTables());
}

TCommandRightChecks GetCommandRightChecks(int SpellNr) {
	switch (SpellNr) {
		case SPELL_MOVE:
			return {4, {"vertical", "speed", "coordinate", "mark"}};
		case SPELL_GET_ITEM_COUNT:
		case SPELL_GET_ITEM:
		case SPELL_GET_EXPERIENCE:
		case SPELL_CHANGE_DATA:
		case SPELL_GET_SKILL_EXPERIENCE:
		case SPELL_TELEPORT_TO_FRIEND:
		case SPELL_RETRIEVE_FRIEND:
		case SPELL_SUMMON_WILD_CREATURE:
		case SPELL_GET_POSITION:
		case SPELL_TEMPLE_TELEPORT:
		case SPELL_CREATE_GOLD:
		case SPELL_CHANGE_PROFESSION_OR_SEX:
		case SPELL_KICK_PLAYER:
		case SPELL_GET_QUEST_VALUE:
		case SPELL_SET_QUEST_VALUE:
		case SPELL_DESINTEGRATE_SPELL:
		case SPELL_LEVITATE_GAMEMASTER:
		case SPELL_CLEAR_QUEST_VALUES:
		case SPELL_KILL_ALL_CREATURES:
		case SPELL_START_MONSTERRAID:
		case SPELL_RELOAD_SCRIPTS:
		case SPELL_ONLINE_LIST:
			return {1, {""}};
		default:
			return {0, {}};
	}
}

const char *GetSpellScript(int SpellNr) {
	if (SpellNr < 0 || SpellNr >= MAX_SPELLS || SpellScript[SpellNr].empty()) {
		return nullptr;
	}
	return SpellScript[SpellNr].c_str();
}

const char *GetSpellScriptPosition(int SpellNr) {
	if (SpellNr < 0 || SpellNr >= MAX_SPELLS || SpellScript[SpellNr].empty()) {
		return nullptr;
	}
	return SpellScriptPosition[SpellNr].c_str();
}

int GetRightByName(const char *Name) {
	for (int Right = 0; Right < NUMBER_OF_RIGHTS; Right++) {
		if (strcmp(Name, RightNames[Right]) == 0) {
			return Right;
		}
	}
	return NO_RIGHT;
}

// The right a string of commands.toml names. What names the value in the
// error text.
static int ReadRight(const toml::node &Node, const char *What) {
	if (!Node.is_string()) {
		TomlError(Node.source(), "%s must be a string", What);
	}
	const std::string &Name = Node.as_string()->get();
	int Right = GetRightByName(Name.c_str());
	if (Right == NO_RIGHT) {
		TomlError(Node.source(), "unknown right \"%s\"", Name.c_str());
	}
	return Right;
}

// The rights of a command under right: a string for a command that checks
// one right, a table by the names of its checks for one that checks more,
// and nothing for one that checks none.
static void ReadCommandRights(const toml::table &Entry, int SpellNr, const TMagicTables &Tables) {
	TCommandRightChecks Checks = GetCommandRightChecks(SpellNr);
	const toml::node *Node = Entry.get("right");
	if (Checks.Count == 0) {
		if (Node != nullptr) {
			TomlError(Node->source(), "command %d checks no right", SpellNr);
		}
		return;
	}
	if (Node == nullptr) {
		TomlError(Entry.source(), "command %d needs right", SpellNr);
	}

	if (Checks.Names[0][0] == 0) {
		Tables.Rights[SpellNr][COMMAND_RIGHT] = ReadRight(*Node, "right");
		return;
	}

	if (!Node->is_table()) {
		TomlError(Node->source(), "right of command %d must be a table of its %d right checks", SpellNr,
				Checks.Count);
	}
	const toml::table &Rights = *Node->as_table();
	for (const auto &[Key, Value] : Rights) {
		bool Known = false;
		for (int i = 0; i < Checks.Count; i++) {
			Known = Known || Key.str() == Checks.Names[i];
		}
		if (!Known) {
			TomlError(Value.source(), "unknown key \"%s\" in right of command %d", std::string(Key.str()).c_str(),
					SpellNr);
		}
	}
	for (int i = 0; i < Checks.Count; i++) {
		const toml::node *Check = Rights.get(Checks.Names[i]);
		if (Check == nullptr) {
			TomlError(Node->source(), "right of command %d needs %s", SpellNr, Checks.Names[i]);
		}
		Tables.Rights[SpellNr][i] = ReadRight(*Check, Checks.Names[i]);
	}
}

// One [[command]] of commands.toml. Defined notes the command numbers filled
// so far.
static void ReadCommand(const toml::node &Node, bool *Defined, const TMagicTables &Tables) {
	if (!Node.is_table()) {
		TomlError(Node.source(), "an entry of command must be a table");
	}

	const toml::table &Entry = *Node.as_table();
	TomlCheckKeys(Entry, "[[command]]", {"number", "name", "words", "right", "script"});
	for (const char *Key : {"number", "name", "words"}) {
		if (!Entry.contains(Key)) {
			TomlError(Node.source(), "an entry of command needs %s", Key);
		}
	}

	int SpellNr = 0;
	TomlGetNumber(Entry, "number", SpellNr, 1, MAX_SPELLS - 1);
	if (Defined[SpellNr]) {
		TomlError(Entry.get("number")->source(), "command %d is defined twice", SpellNr);
	}
	if (Tables.Spells[SpellNr].Syllable[0] != 0) {
		TomlError(Entry.get("number")->source(), "%d is the number of a spell in spells.toml", SpellNr);
	}
	Defined[SpellNr] = true;

	TSpellList &Spell = Tables.Spells[SpellNr];
	memset(&Spell, 0, sizeof(TSpellList));

	char Name[MAX_SPELL_NAME_LENGTH + 1];
	TomlGetString(Entry, "name", Name, sizeof(Name));
	Spell.Comment = SpellName(Name, Tables);

	ReadWords(Entry, Spell.Syllable, true);
	for (int Other = 1; Other < MAX_SPELLS; Other++) {
		if (Other != SpellNr && Tables.Spells[Other].Syllable[0] != 0
				&& memcmp(Tables.Spells[Other].Syllable, Spell.Syllable, sizeof(Spell.Syllable)) == 0) {
			TomlError(Entry.get("words")->source(), "these words are already the ones of %s %d",
					Defined[Other] ? "command" : "spell", Other);
		}
	}

	ReadCommandRights(Entry, SpellNr, Tables);
	ReadScript(Entry, SpellNr, "commands/", "a command", Tables);
}

// LoadCommands into Tables.
static void ReadCommands(const char *FileName, const TMagicTables &Tables) {
	for (int i = 0; i < MAX_SPELLS; i++) {
		for (int j = 0; j < MAX_COMMAND_RIGHTS; j++) {
			Tables.Rights[i][j] = NO_RIGHT;
		}
	}

	toml::table Commands = TomlParseFile(FileName);
	TomlCheckKeys(Commands, "the top level", {"command"});

	bool Defined[MAX_SPELLS] = {};
	if (const toml::array *Entries = TomlGetArray(Commands, "command")) {
		for (const toml::node &Node : *Entries) {
			ReadCommand(Node, Defined, Tables);
		}
	}
}

void LoadCommands(const char *FileName) {
	ReadCommands(FileName, LiveTables());
}

// The most rows circles.toml may have (and fields in a row): the size
// circles.dat allowed.
const int MAX_CIRCLE_ROWS = 21;

// LoadCircles into Tables.
static void ReadCircles(const char *FileName, const TMagicTables &Tables) {
	TCircle *Circle = Tables.Circles;
	for (int i = 0; i < MAX_CIRCLES; i++) {
		memset(&Circle[i], 0, sizeof(TCircle));
	}

	toml::table File = TomlParseFile(FileName);
	TomlCheckKeys(File, "the top level", {"rows"});
	const toml::array *Rows = TomlGetArray(File, "rows");
	if (Rows == nullptr) {
		TomlError(File.source(), "rows is missing");
	}

	// A square of fields with the center in the middle: as many rows as
	// fields in a row, an odd number.
	int Size = static_cast<int>(Rows->size());
	if (Size % 2 == 0 || Size > MAX_CIRCLE_ROWS) {
		TomlError(Rows->source(), "rows must hold an odd number of rows, at most %d", MAX_CIRCLE_ROWS);
	}

	int Center = Size / 2;
	for (int y = 0; y < Size; y++) {
		const toml::node &RowNode = *Rows->get(static_cast<size_t>(y));
		if (!RowNode.is_string()) {
			TomlError(RowNode.source(), "a row must be a string");
		}

		// The fields, one character each, separated by single spaces.
		const std::string &Row = RowNode.as_string()->get();
		bool Square = static_cast<int>(Row.size()) == 2 * Size - 1;
		for (int i = 1; Square && i < 2 * Size - 1; i += 2) {
			Square = Row[static_cast<size_t>(i)] == ' ';
		}
		if (!Square) {
			TomlError(RowNode.source(), "row %d must be %d fields separated by single spaces, one for each row", y + 1,
					Size);
		}

		for (int x = 0; x < Size; x++) {
			char Field = Row[static_cast<size_t>(2 * x)];
			if (Field == '.') {
				continue;
			}
			if (Field < '0' || Field > '9') {
				TomlError(RowNode.source(), "row %d, field %d is neither a radius 0 to 9 nor \".\"", y + 1, x + 1);
			}

			int Radius = Field - '0';
			if (Circle[Radius].Count == MAX_CIRCLE_FIELDS) {
				TomlError(RowNode.source(), "row %d, field %d: circle %d has more than %d fields", y + 1, x + 1,
						Radius, MAX_CIRCLE_FIELDS);
			}
			Circle[Radius].x[Circle[Radius].Count] = x - Center;
			Circle[Radius].y[Circle[Radius].Count] = y - Center;
			Circle[Radius].Count++;
		}
	}
}

void LoadCircles(const char *FileName) {
	ReadCircles(FileName, LiveTables());
}

void ReadStagedMagicData(const char *CirclesFile, const char *SpellsFile, const char *CommandsFile) {
	ClearStagedMagicData();
	TMagicTables Staged = {StagedSpellList, StagedCommandRight, StagedCircle, StagedSpellScript,
			StagedSpellScriptPosition, SpellNames.size()};
	ReadCircles(CirclesFile, Staged);
	ReadSpells(SpellsFile, Staged);
	ReadCommands(CommandsFile, Staged);

	for (int i = 1; i < MAX_SPELLS; i++) {
		int Class = SpellList[i].Syllable[0];
		if (Class == 0 || StagedSpellList[i].Syllable[0] != 0) {
			continue;
		}
		bool Command = Class == SYLLABLE_AL || Class == SYLLABLE_OM;
		snprintf(MagicDataError, sizeof(MagicDataError),
				"%s: %s %d (%s) is gone; a reload cannot remove or renumber a spell or command",
				Command ? CommandsFile : SpellsFile, Command ? "command" : "spell", i,
				SpellList[i].Comment != nullptr ? SpellList[i].Comment : "");
		throw MagicDataError;
	}
}

void SwapStagedMagicData() {
	for (int i = 0; i < MAX_SPELLS; i++) {
		std::swap(SpellList[i], StagedSpellList[i]);
		for (int j = 0; j < MAX_COMMAND_RIGHTS; j++) {
			std::swap(CommandRight[i][j], StagedCommandRight[i][j]);
		}
		SpellScript[i].swap(StagedSpellScript[i]);
		SpellScriptPosition[i].swap(StagedSpellScriptPosition[i]);
	}
	for (int i = 0; i < MAX_CIRCLES; i++) {
		std::swap(Circle[i], StagedCircle[i]);
	}
	Talkactions.swap(StagedTalkactions);
}

void ClearStagedMagicData() {
	for (int i = 0; i < MAX_SPELLS; i++) {
		memset(&StagedSpellList[i], 0, sizeof(TSpellList));
		for (int j = 0; j < MAX_COMMAND_RIGHTS; j++) {
			StagedCommandRight[i][j] = NO_RIGHT;
		}
		StagedSpellScript[i].clear();
		StagedSpellScriptPosition[i].clear();
	}
	for (int i = 0; i < MAX_CIRCLES; i++) {
		memset(&StagedCircle[i], 0, sizeof(TCircle));
	}
	StagedTalkactions.clear();
}

// Whether the entry SpellNr of the staged set differs from the live one.
static bool StagedSpellDiffers(int SpellNr) {
	const TSpellList &Old = SpellList[SpellNr];
	const TSpellList &New = StagedSpellList[SpellNr];
	if (memcmp(Old.Syllable, New.Syllable, sizeof(Old.Syllable)) != 0 || Old.RuneGr != New.RuneGr
			|| Old.RuneNr != New.RuneNr || Old.Level != New.Level || Old.RuneLevel != New.RuneLevel
			|| Old.Flags != New.Flags || Old.Mana != New.Mana || Old.SoulPoints != New.SoulPoints
			|| Old.Amount != New.Amount) {
		return true;
	}
	if (strcmp(Old.Comment != nullptr ? Old.Comment : "", New.Comment != nullptr ? New.Comment : "") != 0) {
		return true;
	}
	for (int j = 0; j < MAX_COMMAND_RIGHTS; j++) {
		if (CommandRight[SpellNr][j] != StagedCommandRight[SpellNr][j]) {
			return true;
		}
	}
	return SpellScript[SpellNr] != StagedSpellScript[SpellNr];
}

TMagicChanges CompareStagedMagicData() {
	TMagicChanges Changes = {0, 0, 0, 0, false};
	for (int i = 1; i < MAX_SPELLS; i++) {
		int Class = StagedSpellList[i].Syllable[0];
		if (Class == 0) {
			continue;
		}
		bool Command = Class == SYLLABLE_AL || Class == SYLLABLE_OM;
		if (SpellList[i].Syllable[0] == 0) {
			(Command ? Changes.CommandsAdded : Changes.SpellsAdded)++;
		} else if (StagedSpellDiffers(i)) {
			(Command ? Changes.CommandsChanged : Changes.SpellsChanged)++;
		}
	}
	Changes.CirclesChanged = memcmp(Circle, StagedCircle, sizeof(Circle)) != 0;
	return Changes;
}

int CountParameterWords(const TSpellList &Spell) {
	int Count = 0;
	for (int j = 0; j < MAX_SPELL_SYLLABLES && Spell.Syllable[j] != 0; j++) {
		if (Spell.Syllable[j] == SYLLABLE_PARA) {
			Count++;
		}
	}
	return Count;
}

// The command number a value of command in talkactions.toml gives: a staged
// command (al or om).
static int ReadTalkactionCommand(const toml::node &Node) {
	std::optional<int64_t> Number = Node.value<int64_t>();
	if (!Node.is_integer() || !Number || *Number < 1 || *Number > MAX_SPELLS - 1) {
		TomlError(Node.source(), "command must be a command number, 1 to %d", MAX_SPELLS - 1);
	}
	int Class = StagedSpellList[*Number].Syllable[0];
	if (Class != SYLLABLE_AL && Class != SYLLABLE_OM) {
		TomlError(Node.source(), "%d is no command of commands.toml", static_cast<int>(*Number));
	}
	return static_cast<int>(*Number);
}

// One [[talkaction]] of talkactions.toml, into StagedTalkactions.
static void ReadTalkaction(const toml::node &Node) {
	if (!Node.is_table()) {
		TomlError(Node.source(), "an entry of talkaction must be a table");
	}

	const toml::table &Entry = *Node.as_table();
	TomlCheckKeys(Entry, "[[talkaction]]", {"words", "command", "param", "self"});
	for (const char *Key : {"words", "command"}) {
		if (!Entry.contains(Key)) {
			TomlError(Node.source(), "an entry of talkaction needs %s", Key);
		}
	}

	TTalkaction Talk = {};
	char Words[MAX_TALKACTION_WORDS + 1];
	TomlGetString(Entry, "words", Words, sizeof(Words));
	const toml::source_region &WordsAt = Entry.get("words")->source();
	if (Words[0] != '/' && Words[0] != '!') {
		TomlError(WordsAt, "words must start with / or !");
	}
	if (Words[1] == 0) {
		TomlError(WordsAt, "words need a character after the / or !");
	}
	for (const char *c = Words; *c != 0; c++) {
		if (isSpace(static_cast<unsigned char>(*c)) || *c == ',') {
			TomlError(WordsAt, "words may hold no white space and no comma");
		}
	}
	for (const TTalkaction &Other : StagedTalkactions) {
		if (stricmp(const_cast<char *>(Other.Words.c_str()), Words, -1) == 0) {
			TomlError(WordsAt, "the talkaction %s is defined twice", Words);
		}
	}
	Talk.Words = Words;

	const toml::node &Command = *Entry.get("command");
	if (const toml::array *List = Command.as_array()) {
		if (List->empty() || List->size() > MAX_TALKACTION_COMMANDS) {
			TomlError(Command.source(), "command must list 1 to %d commands", MAX_TALKACTION_COMMANDS);
		}
		for (const toml::node &Item : *List) {
			int SpellNr = ReadTalkactionCommand(Item);
			int Count = CountParameterWords(StagedSpellList[SpellNr]);
			for (int i = 0; i < Talk.CommandCount; i++) {
				if (CountParameterWords(StagedSpellList[Talk.Commands[i]]) == Count) {
					TomlError(Item.source(), "command %d has as many parameter words as command %d", SpellNr,
							Talk.Commands[i]);
				}
			}
			Talk.Commands[Talk.CommandCount++] = SpellNr;
		}
	} else {
		Talk.Commands[Talk.CommandCount++] = ReadTalkactionCommand(Command);
	}

	char Param[510];
	if (TomlGetString(Entry, "param", Param, sizeof(Param))) {
		if (Param[0] == 0) {
			TomlError(Entry.get("param")->source(), "param must not be empty");
		}
		Talk.HasParam = true;
		Talk.Param = Param;
	}
	if (const toml::node *Self = Entry.get("self")) {
		if (!Self->is_boolean()) {
			TomlError(Self->source(), "self must be true or false");
		}
		Talk.Self = Self->value_or(false);
		if (Talk.Self && Talk.HasParam) {
			TomlError(Self->source(), "a talkaction with param takes no self");
		}
	}
	StagedTalkactions.push_back(std::move(Talk));
}

void ReadStagedTalkactions(const char *FileName) {
	StagedTalkactions.clear();
	toml::table File = TomlParseFile(FileName);
	TomlCheckKeys(File, "the top level", {"talkaction"});
	if (const toml::array *Entries = TomlGetArray(File, "talkaction")) {
		for (const toml::node &Node : *Entries) {
			ReadTalkaction(Node);
		}
	}
}

const TTalkaction *FindTalkaction(const char *Words) {
	for (const TTalkaction &Talk : Talkactions) {
		if (stricmp(const_cast<char *>(Talk.Words.c_str()), const_cast<char *>(Words), -1) == 0) {
			return &Talk;
		}
	}
	return nullptr;
}

int StagedTalkactionCount() {
	return static_cast<int>(StagedTalkactions.size());
}

bool StagedTalkactionsDiffer() {
	if (Talkactions.size() != StagedTalkactions.size()) {
		return true;
	}
	for (size_t i = 0; i < Talkactions.size(); i++) {
		const TTalkaction &Old = Talkactions[i];
		const TTalkaction &New = StagedTalkactions[i];
		if (Old.Words != New.Words || Old.CommandCount != New.CommandCount || Old.HasParam != New.HasParam
				|| Old.Param != New.Param || Old.Self != New.Self
				|| memcmp(Old.Commands, New.Commands, sizeof(Old.Commands)) != 0) {
			return true;
		}
	}
	return false;
}
