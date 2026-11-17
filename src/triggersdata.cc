// triggersdata.cc: the trigger rules (triggersdata.hh): the schema of
// triggers.toml, the tables the game keeps the rules in and their loader,
// which reads triggers.toml (data/triggers.toml in the repository, converted
// from the original moveuse.dat by tools/convert). The game reads the file
// into a staged set of the tables, which then changes places with the live
// one (ReadStagedTriggerRules, SwapStagedTriggerRules), at start and on a
// reload; the converter reads it into the live tables (LoadTriggerRules).

#include "tomlfile.hh"

#include <climits>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <string>
#include <utility>

#include "script.hh"
#include "objects.hh"
#include "map.hh"
#include "info.hh"
#include "crskill.hh"
#include "strings.hh"
#include "utils.hh"
#include "triggersdata.hh"

vector<TTriggerCondition> TriggerConditions(1, 1000, 1000);
int NumberOfTriggerConditions;
vector<TTriggerAction> TriggerActions(1, 1000, 1000);
int NumberOfTriggerActions;
TTriggerDatabase TriggerDatabases[NUMBER_OF_EVENT_TYPES];
std::vector<TTriggerRuleEntry> TriggerRules;
std::vector<TTriggerScript> TriggerScripts;
std::vector<std::string> TriggerConditionPlaces;
std::vector<std::string> TriggerActionPlaces;
std::vector<uint32_t> TriggerTexts;

// The staged set of the reload (ReadStagedTriggerRules): the same tables.
static vector<TTriggerCondition> StagedConditions(1, 1000, 1000);
static int StagedNumberOfConditions;
static vector<TTriggerAction> StagedActions(1, 1000, 1000);
static int StagedNumberOfActions;
static TTriggerDatabase StagedDatabases[NUMBER_OF_EVENT_TYPES];
static std::vector<TTriggerRuleEntry> StagedRules;
static std::vector<TTriggerScript> StagedScripts;
static std::vector<std::string> StagedConditionPlaces;
static std::vector<std::string> StagedActionPlaces;
static std::vector<uint32_t> StagedTexts;

// The tables a read fills: the live set or the staged set.
struct TTriggerTables {
	vector<TTriggerCondition> &Conditions;
	int &NumberOfConditions;
	vector<TTriggerAction> &Actions;
	int &NumberOfActions;
	TTriggerDatabase *Databases;
	std::vector<TTriggerRuleEntry> &Rules;
	std::vector<TTriggerScript> &Scripts;
	std::vector<std::string> &ConditionPlaces;
	std::vector<std::string> &ActionPlaces;
	std::vector<uint32_t> &Texts;
};

static TTriggerTables LiveTables() {
	return {TriggerConditions, NumberOfTriggerConditions, TriggerActions, NumberOfTriggerActions, TriggerDatabases,
			TriggerRules, TriggerScripts, TriggerConditionPlaces, TriggerActionPlaces, TriggerTexts};
}

static TTriggerTables StagedTables() {
	return {StagedConditions, StagedNumberOfConditions, StagedActions, StagedNumberOfActions, StagedDatabases,
			StagedRules, StagedScripts, StagedConditionPlaces, StagedActionPlaces, StagedTexts};
}

const TTriggerSignature ConditionSignatures[NUMBER_OF_CONDITION_TYPES] = {
	{"is_position", 2, {PARAMETER_OBJECT, PARAMETER_COORDINATE}},
	{"is_type", 2, {PARAMETER_OBJECT, PARAMETER_TYPE}},
	{"is_creature", 1, {PARAMETER_OBJECT}},
	{"is_player", 1, {PARAMETER_OBJECT}},
	{"has_flag", 2, {PARAMETER_OBJECT, PARAMETER_FLAG}},
	{"has_type_attribute", 4, {PARAMETER_OBJECT, PARAMETER_TYPEATTRIBUTE, PARAMETER_COMPARISON, PARAMETER_NUMBER}},
	{"has_instance_attribute", 4,
			{PARAMETER_OBJECT, PARAMETER_INSTANCEATTRIBUTE, PARAMETER_COMPARISON, PARAMETER_NUMBER}},
	{"has_text", 2, {PARAMETER_OBJECT, PARAMETER_TEXT}},
	{"is_peaceful", 1, {PARAMETER_OBJECT}},
	{"may_logout", 1, {PARAMETER_OBJECT}},
	{"has_profession", 2, {PARAMETER_OBJECT, PARAMETER_NUMBER}},
	{"has_level", 3, {PARAMETER_OBJECT, PARAMETER_COMPARISON, PARAMETER_NUMBER}},
	{"has_right", 2, {PARAMETER_OBJECT, PARAMETER_RIGHT}},
	{"has_quest_value", 4, {PARAMETER_OBJECT, PARAMETER_NUMBER, PARAMETER_COMPARISON, PARAMETER_NUMBER}},
	{"test_skill", 4, {PARAMETER_OBJECT, PARAMETER_SKILL, PARAMETER_NUMBER, PARAMETER_NUMBER}},
	{"count_objects", 3, {PARAMETER_OBJECT, PARAMETER_COMPARISON, PARAMETER_NUMBER}},
	{"count_objects_on_map", 3, {PARAMETER_COORDINATE, PARAMETER_COMPARISON, PARAMETER_NUMBER}},
	{"is_object_there", 2, {PARAMETER_COORDINATE, PARAMETER_TYPE}},
	{"is_creature_there", 1, {PARAMETER_COORDINATE}},
	{"is_player_there", 1, {PARAMETER_COORDINATE}},
	{"is_object_in_inventory", 3, {PARAMETER_OBJECT, PARAMETER_TYPE, PARAMETER_NUMBER}},
	{"is_protection_zone", 1, {PARAMETER_OBJECT}},
	{"is_house", 1, {PARAMETER_OBJECT}},
	{"is_house_owner", 2, {PARAMETER_OBJECT, PARAMETER_OBJECT}},
	{"is_dressed", 1, {PARAMETER_OBJECT}},
	{"random", 1, {PARAMETER_NUMBER}},
};

const TTriggerSignature ActionSignatures[NUMBER_OF_ACTION_TYPES] = {
	{"create_on_map", 3, {PARAMETER_COORDINATE, PARAMETER_TYPE, PARAMETER_NUMBER}},
	{"create", 3, {PARAMETER_OBJECT, PARAMETER_TYPE, PARAMETER_NUMBER}},
	{"monster_on_map", 2, {PARAMETER_COORDINATE, PARAMETER_NUMBER}},
	{"monster", 2, {PARAMETER_OBJECT, PARAMETER_NUMBER}},
	{"effect_on_map", 2, {PARAMETER_COORDINATE, PARAMETER_NUMBER}},
	{"effect", 2, {PARAMETER_OBJECT, PARAMETER_NUMBER}},
	{"text_on_map", 3, {PARAMETER_COORDINATE, PARAMETER_TEXT, PARAMETER_NUMBER}},
	{"text", 3, {PARAMETER_OBJECT, PARAMETER_TEXT, PARAMETER_NUMBER}},
	{"change_on_map", 4, {PARAMETER_COORDINATE, PARAMETER_TYPE, PARAMETER_TYPE, PARAMETER_NUMBER}},
	{"change", 3, {PARAMETER_OBJECT, PARAMETER_TYPE, PARAMETER_NUMBER}},
	{"change_rel", 5, {PARAMETER_OBJECT, PARAMETER_VECTOR, PARAMETER_TYPE, PARAMETER_TYPE, PARAMETER_NUMBER}},
	{"set_attribute", 3, {PARAMETER_OBJECT, PARAMETER_INSTANCEATTRIBUTE, PARAMETER_NUMBER}},
	{"change_attribute", 3, {PARAMETER_OBJECT, PARAMETER_INSTANCEATTRIBUTE, PARAMETER_NUMBER}},
	{"set_quest_value", 3, {PARAMETER_OBJECT, PARAMETER_NUMBER, PARAMETER_NUMBER}},
	{"damage", 4, {PARAMETER_OBJECT, PARAMETER_OBJECT, PARAMETER_NUMBER, PARAMETER_NUMBER}},
	{"set_start", 2, {PARAMETER_OBJECT, PARAMETER_COORDINATE}},
	{"write_name", 3, {PARAMETER_OBJECT, PARAMETER_TEXT, PARAMETER_OBJECT}},
	{"write_text", 2, {PARAMETER_TEXT, PARAMETER_OBJECT}},
	{"logout", 1, {PARAMETER_OBJECT}},
	{"move_all_on_map", 2, {PARAMETER_COORDINATE, PARAMETER_COORDINATE}},
	{"move_all", 2, {PARAMETER_OBJECT, PARAMETER_COORDINATE}},
	{"move_all_rel", 2, {PARAMETER_OBJECT, PARAMETER_VECTOR}},
	{"move_top_on_map", 3, {PARAMETER_COORDINATE, PARAMETER_TYPE, PARAMETER_COORDINATE}},
	{"move_top", 2, {PARAMETER_OBJECT, PARAMETER_COORDINATE}},
	{"move_top_rel", 2, {PARAMETER_OBJECT, PARAMETER_VECTOR}},
	{"move", 2, {PARAMETER_OBJECT, PARAMETER_COORDINATE}},
	{"move_rel", 3, {PARAMETER_OBJECT, PARAMETER_OBJECT, PARAMETER_VECTOR}},
	{"retrieve", 3, {PARAMETER_OBJECT, PARAMETER_VECTOR, PARAMETER_VECTOR}},
	{"delete_all_on_map", 1, {PARAMETER_COORDINATE}},
	{"delete_top_on_map", 2, {PARAMETER_COORDINATE, PARAMETER_TYPE}},
	{"delete_on_map", 2, {PARAMETER_COORDINATE, PARAMETER_TYPE}},
	{"delete", 1, {PARAMETER_OBJECT}},
	{"delete_in_inventory", 3, {PARAMETER_OBJECT, PARAMETER_TYPE, PARAMETER_NUMBER}},
	{"description", 2, {PARAMETER_OBJECT, PARAMETER_OBJECT}},
	{"load_depot", 4, {PARAMETER_COORDINATE, PARAMETER_TYPE, PARAMETER_OBJECT, PARAMETER_NUMBER}},
	{"save_depot", 4, {PARAMETER_COORDINATE, PARAMETER_TYPE, PARAMETER_OBJECT, PARAMETER_NUMBER}},
	{"send_mail", 1, {PARAMETER_OBJECT}},
	{"nop", 0, {}},
};

const char *const EventKeys[NUMBER_OF_EVENT_TYPES] = {
	"use",
	"multi_use",
	"movement",
	"collision",
	"separation",
};

const char *const EventObjectKeys[NUMBER_OF_EVENTOBJECTS] = {
	"null",
	"obj1",
	"obj2",
	"user",
	"temp",
};

// The instance attributes' names of the map files (InstanceAttributeNames)
// in snake_case.
const char *const InstanceAttributeKeys[INSTANCEATTRIBUTES] = {
	"content",
	"chest_quest_number",
	"amount",
	"key_number",
	"keyhole_number",
	"level",
	"door_quest_number",
	"door_quest_value",
	"charges",
	"string",
	"editor",
	"container_liquid_type",
	"pool_liquid_type",
	"abs_teleport_destination",
	"responsible",
	"remaining_expire_time",
	"saved_expire_time",
	"remaining_uses",
};

const TTriggerKey RightKeys[2] = {
	{"premium_account", PREMIUM_ACCOUNT},
	{"special_moveuse", SPECIAL_MOVEUSE},
};

const TTriggerKey ComparisonKeys[6] = {
	{"<", '<'},
	{"<=", 'L'},
	{"=", '='},
	{">=", 'G'},
	{">", '>'},
	{"<>", 'N'},
};

// The longest text a rule may give: the longest string the script reader of
// moveuse.dat took.
const int MAX_TRIGGER_TEXT_LENGTH = TReadScriptFile::MAXIMUMSTRINGLENGTH - 2;

// The coordinates PackAbsoluteCoordinate and PackRelativeCoordinate hold
// without loss: x and y in 14 bits each, z in 4.
const int MIN_COORDINATE_XY = 24576;
const int MAX_COORDINATE_XY = 24576 + 16383;
const int MAX_COORDINATE_Z = 15;
const int MIN_VECTOR_XY = -8192;
const int MAX_VECTOR_XY = 8191;
const int MIN_VECTOR_Z = -8;
const int MAX_VECTOR_Z = 7;

// "<file>:<line>:<column>" of a place in the file, as TomlError writes it.
static std::string Place(const toml::source_region &Where) {
	return std::string(Where.path != nullptr ? Where.path->c_str() : "(unknown file)") + ":"
			+ std::to_string(Where.begin.line) + ":" + std::to_string(Where.begin.column);
}

// Sets entry Number of Places to Place, making room for it.
static void SetPlace(std::vector<std::string> &Places, int Number, const std::string &Where) {
	if (Places.size() <= static_cast<size_t>(Number)) {
		Places.resize(static_cast<size_t>(Number) + 1);
	}
	Places[static_cast<size_t>(Number)] = Where;
}

// Whether Name is a script name of a rule: "triggers/<name>.lua" and a script
// name as scripting.hh defines it (only a-z, 0-9, '_', '-', '/' and '.'; no
// part empty, "." or ".."; at most 256 characters).
static bool ValidRuleScript(const std::string &Name) {
	const std::string Prefix = "triggers/";
	const std::string Ending = ".lua";
	if (Name.size() > 256 || Name.size() <= Prefix.size() + Ending.size()
			|| Name.compare(0, Prefix.size(), Prefix) != 0
			|| Name.compare(Name.size() - Ending.size(), Ending.size(), Ending) != 0) {
		return false;
	}
	size_t PartStart = 0;
	for (size_t i = 0; i <= Name.size(); i++) {
		char c = i < Name.size() ? Name[i] : 0;
		if (c == '/' || c == 0) {
			std::string Part = Name.substr(PartStart, i - PartStart);
			if (Part.empty() || Part == "." || Part == ".." || (c == 0 && Part == Ending)) {
				return false;
			}
			PartStart = i + 1;
		} else if (!((c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || c == '_' || c == '-' || c == '.')) {
			return false;
		}
	}
	return true;
}

// The index of Key in Keys (Count of them), or -1.
static int FindKey(const char *const *Keys, int Count, const std::string &Key) {
	for (int i = 0; i < Count; i++) {
		if (Key == Keys[i]) {
			return i;
		}
	}
	return -1;
}

// An integer of Min..Max. What names it in the error text.
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

// A string that is one of Count keys; returns its index.
static int ReadKey(const toml::node &Node, const char *What, const char *const *Keys, int Count) {
	if (!Node.is_string()) {
		TomlError(Node.source(), "%s must be a string", What);
	}
	int Index = FindKey(Keys, Count, Node.as_string()->get());
	if (Index == -1) {
		TomlError(Node.source(), "%s: \"%s\" is unknown", What, Node.as_string()->get().c_str());
	}
	return Index;
}

// A string that is one of the keys of Count TTriggerKeys; returns its value.
static int ReadKeyValue(const toml::node &Node, const char *What, const TTriggerKey *Keys, int Count) {
	if (!Node.is_string()) {
		TomlError(Node.source(), "%s must be a string", What);
	}
	for (int i = 0; i < Count; i++) {
		if (Node.as_string()->get() == Keys[i].Key) {
			return Keys[i].Value;
		}
	}
	TomlError(Node.source(), "%s: \"%s\" is unknown", What, Node.as_string()->get().c_str());
}

// [x, y, z] with each in its range.
static void ReadTriple(const toml::node &Node, const char *What, int *Values, int MinXY, int MaxXY, int MinZ,
		int MaxZ) {
	const toml::array *Array = Node.as_array();
	if (Array == nullptr || Array->size() != 3) {
		TomlError(Node.source(), "%s must be an array [x, y, z]", What);
	}
	for (size_t i = 0; i < 3; i++) {
		static const char *const Axes[3] = {"x", "y", "z"};
		std::string Name = std::string(Axes[i]) + " of " + What;
		Values[i] = ReadInteger(*Array->get(i), Name.c_str(), i < 2 ? MinXY : MinZ, i < 2 ? MaxXY : MaxZ);
	}
}

// One parameter of the type Type from Node into Parameter. What names it in
// the error text ("argument 2 of is_type"). A text's dynamic string is noted
// in Texts.
static void ReadParameter(const toml::node &Node, ParameterType Type, const char *What, int &Parameter,
		std::vector<uint32_t> &Texts) {
	int Values[3];
	switch (Type) {
		case PARAMETER_OBJECT:
			Parameter = ReadKey(Node, What, EventObjectKeys, NUMBER_OF_EVENTOBJECTS);
			break;
		case PARAMETER_TYPE:
			Parameter = ReadInteger(Node, What, INT_MIN, INT_MAX);
			if (!ObjectTypeExists(Parameter)) {
				TomlError(Node.source(), "%s: object type %d does not exist", What, Parameter);
			}
			break;
		case PARAMETER_FLAG:
			Parameter = ReadKey(Node, What, FlagKeys, FLAGS);
			break;
		case PARAMETER_TYPEATTRIBUTE:
			Parameter = ReadKey(Node, What, TypeAttributeKeys, TYPEATTRIBUTES);
			break;
		case PARAMETER_INSTANCEATTRIBUTE:
			Parameter = ReadKey(Node, What, InstanceAttributeKeys, INSTANCEATTRIBUTES);
			break;
		case PARAMETER_COORDINATE:
			ReadTriple(Node, What, Values, MIN_COORDINATE_XY, MAX_COORDINATE_XY, 0, MAX_COORDINATE_Z);
			Parameter = static_cast<int>(PackAbsoluteCoordinate(Values[0], Values[1], Values[2]));
			break;
		case PARAMETER_VECTOR:
			ReadTriple(Node, What, Values, MIN_VECTOR_XY, MAX_VECTOR_XY, MIN_VECTOR_Z, MAX_VECTOR_Z);
			Parameter = static_cast<int>(PackRelativeCoordinate(Values[0], Values[1], Values[2]));
			break;
		case PARAMETER_RIGHT:
			Parameter = ReadKeyValue(Node, What, RightKeys, 2);
			break;
		case PARAMETER_SKILL:
			Parameter = ReadKey(Node, What, SkillKeys, NUMBER_OF_SKILL_KEYS);
			break;
		case PARAMETER_NUMBER:
			Parameter = ReadInteger(Node, What, INT_MIN, INT_MAX);
			break;
		case PARAMETER_TEXT: {
			if (!Node.is_string()) {
				TomlError(Node.source(), "%s must be a string", What);
			}
			std::string Text;
			if (!Utf8ToLatin1(Node.as_string()->get(), Text)) {
				TomlError(Node.source(), "%s has a character outside Latin-1", What);
			}
			if (Text.find('\0') != std::string::npos) {
				TomlError(Node.source(), "%s has a zero character", What);
			}
			if (static_cast<int>(Text.size()) > MAX_TRIGGER_TEXT_LENGTH) {
				TomlError(Node.source(), "%s is longer than %d characters", What, MAX_TRIGGER_TEXT_LENGTH);
			}
			uint32_t Number = AddDynamicString(Text.data());
			if (Number != 0) {
				Texts.push_back(Number);
			}
			Parameter = static_cast<int>(Number);
			break;
		}
		case PARAMETER_COMPARISON:
			Parameter = ReadKeyValue(Node, What, ComparisonKeys, 6);
			break;
	}
}

// The entry Node of conditions or actions (List names it, Kind names one of
// its types): checks its keys against Keys, finds its type by name among
// Count signatures and reads its args into Parameters (its texts noted in
// Texts). Returns the type.
static int ReadEntry(const toml::node &Node, const char *List, const char *Kind,
		std::initializer_list<const char *> Keys, const TTriggerSignature *Signatures, int Count, int *Parameters,
		std::vector<uint32_t> &Texts) {
	const toml::table *Entry = Node.as_table();
	if (Entry == nullptr) {
		TomlError(Node.source(), "an entry of %s must be a table", List);
	}
	std::string What = std::string("an entry of ") + List;
	TomlCheckKeys(*Entry, What.c_str(), Keys);
	const toml::node *Name = Entry->get("name");
	if (Name == nullptr) {
		TomlError(Node.source(), "an entry of %s needs name", List);
	}
	if (!Name->is_string()) {
		TomlError(Name->source(), "name must be a string");
	}
	int Type = -1;
	for (int i = 0; i < Count; i++) {
		if (Name->as_string()->get() == Signatures[i].Key) {
			Type = i;
		}
	}
	if (Type == -1) {
		TomlError(Name->source(), "unknown %s \"%s\"", Kind, Name->as_string()->get().c_str());
	}

	const TTriggerSignature &Signature = Signatures[Type];
	const toml::array *Args = TomlGetArray(*Entry, "args");
	size_t Given = Args != nullptr ? Args->size() : 0;
	if (Given != static_cast<size_t>(Signature.NumberOfParameters)) {
		TomlError(Args != nullptr ? Args->source() : Node.source(), "%s takes %d arguments, not %d", Signature.Key,
				Signature.NumberOfParameters, static_cast<int>(Given));
	}
	for (int i = 0; i < Signature.NumberOfParameters; i++) {
		std::string Argument = "argument " + std::to_string(i + 1) + " of " + Signature.Key;
		ReadParameter(*Args->get(static_cast<size_t>(i)), Signature.Parameters[i], Argument.c_str(), Parameters[i],
				Texts);
	}
	return Type;
}

// A true or false under Key of Table, false when there is none.
static bool ReadBoolean(const toml::table &Table, const char *Key) {
	const toml::node *Node = Table.get(Key);
	if (Node == nullptr) {
		return false;
	}
	if (!Node->is_boolean()) {
		TomlError(Node->source(), "%s must be true or false", Key);
	}
	return Node->as_boolean()->get();
}

// The non-empty array under Key of a rule.
static const toml::array &ReadList(const toml::table &Rule, const char *Key) {
	const toml::array *List = TomlGetArray(Rule, Key);
	if (List == nullptr) {
		TomlError(Rule.source(), "a rule needs %s", Key);
	}
	if (List->empty()) {
		TomlError(List->source(), "%s must not be empty", Key);
	}
	return *List;
}

// One [[rule]] of triggers.toml, into Tables.
static void ReadRule(const toml::node &Node, const TTriggerTables &Tables) {
	const toml::table *Rule = Node.as_table();
	if (Rule == nullptr) {
		TomlError(Node.source(), "an entry of rule must be a table");
	}
	TomlCheckKeys(*Rule, "[[rule]]", {"section", "event", "script", "conditions", "actions", "note"});

	// The section and the note are for the reader; the game leaves them.
	if (const toml::array *Section = TomlGetArray(*Rule, "section")) {
		for (const toml::node &Name : *Section) {
			std::string Latin1;
			if (!Name.is_string() || !Utf8ToLatin1(Name.as_string()->get(), Latin1)) {
				TomlError(Name.source(), "a section name must be a Latin-1 string");
			}
		}
	}
	if (const toml::node *Note = Rule->get("note")) {
		std::string Latin1;
		if (!Note->is_string() || !Utf8ToLatin1(Note->as_string()->get(), Latin1)) {
			TomlError(Note->source(), "note must be a Latin-1 string");
		}
	}

	const toml::node *Event = Rule->get("event");
	if (Event == nullptr) {
		TomlError(Node.source(), "a rule needs event");
	}
	int EventType = ReadKey(*Event, "event", EventKeys, NUMBER_OF_EVENT_TYPES);

	// A rule has a script, or conditions and actions.
	if (const toml::node *Script = Rule->get("script")) {
		std::string Name = Script->is_string() ? Script->as_string()->get() : std::string();
		if (!Script->is_string() || !ValidRuleScript(Name)) {
			TomlError(Script->source(), "script must be a script name \"triggers/<name>.lua\" (a-z, 0-9, _, -, /, .)");
		}
		for (const char *Key : {"conditions", "actions"}) {
			if (const toml::node *List = Rule->get(Key)) {
				TomlError(List->source(), "a rule with script has no %s", Key);
			}
		}
		TTriggerDatabase &Database = Tables.Databases[EventType];
		Database.NumberOfRules++;
		TTriggerRule &Entry = Database.Rules(Database.NumberOfRules);
		Entry.FirstCondition = Tables.NumberOfConditions + 1;
		Entry.LastCondition = Tables.NumberOfConditions;
		Entry.FirstAction = Tables.NumberOfActions + 1;
		Entry.LastAction = Tables.NumberOfActions;
		Entry.Script = static_cast<int>(Tables.Scripts.size());
		Tables.Scripts.push_back(TTriggerScript{Name, Place(Script->source()), -1});
		Tables.Rules.push_back(TTriggerRuleEntry{EventType, Database.NumberOfRules});
		Entry.Number = static_cast<int>(Tables.Rules.size());
		return;
	}

	const toml::array &Conditions = ReadList(*Rule, "conditions");
	const toml::array &Actions = ReadList(*Rule, "actions");

	TTriggerDatabase &Database = Tables.Databases[EventType];
	Database.NumberOfRules++;
	TTriggerRule &Entry = Database.Rules(Database.NumberOfRules);
	Tables.Rules.push_back(TTriggerRuleEntry{EventType, Database.NumberOfRules});
	Entry.Number = static_cast<int>(Tables.Rules.size());
	Entry.Script = -1;
	Entry.FirstCondition = Tables.NumberOfConditions + 1;
	for (const toml::node &ConditionNode : Conditions) {
		Tables.NumberOfConditions++;
		SetPlace(Tables.ConditionPlaces, Tables.NumberOfConditions, Place(ConditionNode.source()));
		TTriggerCondition &Condition = Tables.Conditions(Tables.NumberOfConditions);
		int Type = ReadEntry(ConditionNode, "conditions", "condition", {"name", "args", "negate", "ignore_result"},
				ConditionSignatures, NUMBER_OF_CONDITION_TYPES, Condition.Parameters, Tables.Texts);
		Condition.Condition = static_cast<ConditionType>(Type);
		const toml::table &Table = *ConditionNode.as_table();
		bool Negate = ReadBoolean(Table, "negate");
		bool IgnoreResult = ReadBoolean(Table, "ignore_result");
		if (Negate && IgnoreResult) {
			TomlError(ConditionNode.source(), "a condition cannot have both negate and ignore_result");
		}
		Condition.Modifier = Negate ? MODIFIER_INVERT : IgnoreResult ? MODIFIER_TRUE : MODIFIER_NORMAL;
	}
	Entry.LastCondition = Tables.NumberOfConditions;

	Entry.FirstAction = Tables.NumberOfActions + 1;
	for (const toml::node &ActionNode : Actions) {
		Tables.NumberOfActions++;
		SetPlace(Tables.ActionPlaces, Tables.NumberOfActions, Place(ActionNode.source()));
		TTriggerAction &Action = Tables.Actions(Tables.NumberOfActions);
		int Type = ReadEntry(ActionNode, "actions", "action", {"name", "args"}, ActionSignatures,
				NUMBER_OF_ACTION_TYPES, Action.Parameters, Tables.Texts);
		Action.Action = static_cast<ActionType>(Type);
	}
	Entry.LastAction = Tables.NumberOfActions;
}

// ReadTriggerRules into Tables.
static void ReadRules(const toml::table &File, const TTriggerTables &Tables) {
	for (int i = 0; i < NUMBER_OF_EVENT_TYPES; i++) {
		Tables.Databases[i].NumberOfRules = 0;
	}
	Tables.Rules.clear();
	Tables.Scripts.clear();
	Tables.Texts.clear();

	TomlCheckKeys(File, "the top level", {"rule"});
	if (const toml::array *Rules = TomlGetArray(File, "rule")) {
		for (const toml::node &Node : *Rules) {
			ReadRule(Node, Tables);
		}
	}
}

void ReadTriggerRules(const toml::table &File) {
	ReadRules(File, LiveTables());
}

void LoadTriggerRules(const char *FileName) {
	toml::table File = TomlParseFile(FileName);
	ReadTriggerRules(File);
}

void ReadStagedTriggerRules(const char *FileName) {
	ClearStagedTriggerRules();
	try {
		toml::table File = TomlParseFile(FileName);
		ReadRules(File, StagedTables());
	} catch (...) {
		ClearStagedTriggerRules();
		throw;
	}
}

// Swaps two of the project's containers (containr.hh), which have neither a
// copy nor a swap, by their raw bytes, as their growth moves their elements:
// each takes the other's entries and memory.
template<class T>
static void SwapContainers(T &A, T &B) {
	unsigned char Help[sizeof(T)];
	memcpy(Help, static_cast<void *>(&A), sizeof(T));
	memcpy(static_cast<void *>(&A), static_cast<void *>(&B), sizeof(T));
	memcpy(static_cast<void *>(&B), Help, sizeof(T));
}

void SwapStagedTriggerRules() {
	SwapContainers(TriggerConditions, StagedConditions);
	std::swap(NumberOfTriggerConditions, StagedNumberOfConditions);
	SwapContainers(TriggerActions, StagedActions);
	std::swap(NumberOfTriggerActions, StagedNumberOfActions);
	for (int i = 0; i < NUMBER_OF_EVENT_TYPES; i++) {
		SwapContainers(TriggerDatabases[i].Rules, StagedDatabases[i].Rules);
		std::swap(TriggerDatabases[i].NumberOfRules, StagedDatabases[i].NumberOfRules);
	}
	TriggerRules.swap(StagedRules);
	TriggerScripts.swap(StagedScripts);
	TriggerConditionPlaces.swap(StagedConditionPlaces);
	TriggerActionPlaces.swap(StagedActionPlaces);
	TriggerTexts.swap(StagedTexts);
}

void ClearStagedTriggerRules() {
	for (uint32_t Number : StagedTexts) {
		DeleteDynamicString(Number);
	}
	StagedTexts.clear();
	StagedNumberOfConditions = 0;
	StagedNumberOfActions = 0;
	for (int i = 0; i < NUMBER_OF_EVENT_TYPES; i++) {
		StagedDatabases[i].NumberOfRules = 0;
	}
	StagedRules.clear();
	StagedScripts.clear();
	StagedConditionPlaces.clear();
	StagedActionPlaces.clear();
}

// Whether the parameters of a condition or action of Signature differ in the
// live set (Old) and the staged set (New); a text by its content.
static bool ParametersDiffer(const TTriggerSignature &Signature, const int *Old, const int *New) {
	for (int i = 0; i < Signature.NumberOfParameters; i++) {
		if (Signature.Parameters[i] == PARAMETER_TEXT) {
			const char *OldText = GetDynamicString(static_cast<uint32_t>(Old[i]));
			const char *NewText = GetDynamicString(static_cast<uint32_t>(New[i]));
			if (strcmp(OldText != nullptr ? OldText : "", NewText != nullptr ? NewText : "") != 0) {
				return true;
			}
		} else if (Old[i] != New[i]) {
			return true;
		}
	}
	return false;
}

// Whether rule k + 1 of the file differs in the live set and the staged set.
static bool StagedRuleDiffers(size_t k) {
	const TTriggerRuleEntry &OldEntry = TriggerRules[k];
	const TTriggerRuleEntry &NewEntry = StagedRules[k];
	if (OldEntry.Event != NewEntry.Event) {
		return true;
	}
	TTriggerRule &Old = TriggerDatabases[OldEntry.Event].Rules(OldEntry.Rule);
	TTriggerRule &New = StagedDatabases[NewEntry.Event].Rules(NewEntry.Rule);
	if ((Old.Script >= 0) != (New.Script >= 0)) {
		return true;
	}
	if (Old.Script >= 0) {
		return TriggerScripts[static_cast<size_t>(Old.Script)].Name
				!= StagedScripts[static_cast<size_t>(New.Script)].Name;
	}
	if (Old.LastCondition - Old.FirstCondition != New.LastCondition - New.FirstCondition
			|| Old.LastAction - Old.FirstAction != New.LastAction - New.FirstAction) {
		return true;
	}
	for (int i = 0; i <= Old.LastCondition - Old.FirstCondition; i++) {
		const TTriggerCondition &OldCondition = TriggerConditions(Old.FirstCondition + i);
		const TTriggerCondition &NewCondition = StagedConditions(New.FirstCondition + i);
		if (OldCondition.Modifier != NewCondition.Modifier || OldCondition.Condition != NewCondition.Condition
				|| ParametersDiffer(ConditionSignatures[OldCondition.Condition], OldCondition.Parameters,
						NewCondition.Parameters)) {
			return true;
		}
	}
	for (int i = 0; i <= Old.LastAction - Old.FirstAction; i++) {
		const TTriggerAction &OldAction = TriggerActions(Old.FirstAction + i);
		const TTriggerAction &NewAction = StagedActions(New.FirstAction + i);
		if (OldAction.Action != NewAction.Action
				|| ParametersDiffer(ActionSignatures[OldAction.Action], OldAction.Parameters, NewAction.Parameters)) {
			return true;
		}
	}
	return false;
}

TTriggerChanges CompareStagedTriggerRules() {
	TTriggerChanges Changes = {0, 0, 0};
	size_t Old = TriggerRules.size();
	size_t New = StagedRules.size();
	for (size_t k = 0; k < Old && k < New; k++) {
		if (StagedRuleDiffers(k)) {
			Changes.Changed++;
		}
	}
	Changes.Added = New > Old ? static_cast<int>(New - Old) : 0;
	Changes.Removed = Old > New ? static_cast<int>(Old - New) : 0;
	return Changes;
}
