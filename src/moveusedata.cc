// moveusedata.cc: the move and use rules (moveusedata.hh): the tables the game
// keeps them in and their loader, which reads moveuse.toml (data/moveuse.toml
// in the repository, converted from the original moveuse.dat by
// tools/convert).

#include "tomlfile.hh"

#include <climits>
#include <cstdio>
#include <cstring>
#include <string>

#include "script.hh"
#include "config.hh"
#include "objects.hh"
#include "map.hh"
#include "info.hh"
#include "crskill.hh"
#include "strings.hh"
#include "utils.hh"
#include "moveusedata.hh"

vector<TMoveUseCondition> MoveUseConditions(1, 1000, 1000);
int NumberOfMoveUseConditions;
vector<TMoveUseAction> MoveUseActions(1, 1000, 1000);
int NumberOfMoveUseActions;
TMoveUseDatabase MoveUseDatabases[NUMBER_OF_EVENT_TYPES];

const TMoveUseSignature ConditionSignatures[NUMBER_OF_CONDITION_TYPES] = {
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

const TMoveUseSignature ActionSignatures[NUMBER_OF_ACTION_TYPES] = {
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

// The skill names GetSkillByName knows in snake_case, by skill number.
const char *const SkillKeys[] = {
	"level",
	"magic_level",
	"hit_points",
	"mana",
	"go_strength",
	"carry_strength",
	"shielding",
	"distance_fighting",
	"sword_fighting",
	"club_fighting",
	"axe_fighting",
	"fist_fighting",
	"magic_defense",
	"fishing",
	"eating",
	"shining",
	"illusion",
	"poison",
	"burning",
	"energy",
	"drunken",
	"mana_shield",
	"soul_points",
};
const int NUMBER_OF_SKILL_KEYS = sizeof(SkillKeys) / sizeof(SkillKeys[0]);

const TMoveUseKey RightKeys[2] = {
	{"premium_account", PREMIUM_ACCOUNT},
	{"special_moveuse", SPECIAL_MOVEUSE},
};

const TMoveUseKey ComparisonKeys[6] = {
	{"<", '<'},
	{"<=", 'L'},
	{"=", '='},
	{">=", 'G'},
	{">", '>'},
	{"<>", 'N'},
};

// The longest text a rule may give: the longest string the script reader of
// moveuse.dat took.
const int MAX_MOVEUSE_TEXT_LENGTH = TReadScriptFile::MAXIMUMSTRINGLENGTH - 2;

// The coordinates PackAbsoluteCoordinate and PackRelativeCoordinate hold
// without loss: x and y in 14 bits each, z in 4.
const int MIN_COORDINATE_XY = 24576;
const int MAX_COORDINATE_XY = 24576 + 16383;
const int MAX_COORDINATE_Z = 15;
const int MIN_VECTOR_XY = -8192;
const int MAX_VECTOR_XY = 8191;
const int MIN_VECTOR_Z = -8;
const int MAX_VECTOR_Z = 7;

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

// A string that is one of the keys of Count TMoveUseKeys; returns its value.
static int ReadKeyValue(const toml::node &Node, const char *What, const TMoveUseKey *Keys, int Count) {
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
// the error text ("argument 2 of is_type").
static void ReadParameter(const toml::node &Node, ParameterType Type, const char *What, int &Parameter) {
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
			if (static_cast<int>(Text.size()) > MAX_MOVEUSE_TEXT_LENGTH) {
				TomlError(Node.source(), "%s is longer than %d characters", What, MAX_MOVEUSE_TEXT_LENGTH);
			}
			Parameter = static_cast<int>(AddDynamicString(Text.data()));
			break;
		}
		case PARAMETER_COMPARISON:
			Parameter = ReadKeyValue(Node, What, ComparisonKeys, 6);
			break;
	}
}

// The entry Node of conditions or actions (List names it, Kind names one of
// its types): checks its keys against Keys, finds its type by name among
// Count signatures and reads its args into Parameters. Returns the type.
static int ReadEntry(const toml::node &Node, const char *List, const char *Kind,
		std::initializer_list<const char *> Keys, const TMoveUseSignature *Signatures, int Count, int *Parameters) {
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

	const TMoveUseSignature &Signature = Signatures[Type];
	const toml::array *Args = TomlGetArray(*Entry, "args");
	size_t Given = Args != nullptr ? Args->size() : 0;
	if (Given != static_cast<size_t>(Signature.NumberOfParameters)) {
		TomlError(Args != nullptr ? Args->source() : Node.source(), "%s takes %d arguments, not %d", Signature.Key,
				Signature.NumberOfParameters, static_cast<int>(Given));
	}
	for (int i = 0; i < Signature.NumberOfParameters; i++) {
		std::string Argument = "argument " + std::to_string(i + 1) + " of " + Signature.Key;
		ReadParameter(*Args->get(static_cast<size_t>(i)), Signature.Parameters[i], Argument.c_str(), Parameters[i]);
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

// One [[rule]] of moveuse.toml.
static void ReadRule(const toml::node &Node) {
	const toml::table *Rule = Node.as_table();
	if (Rule == nullptr) {
		TomlError(Node.source(), "an entry of rule must be a table");
	}
	TomlCheckKeys(*Rule, "[[rule]]", {"section", "event", "conditions", "actions", "note"});

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
	const toml::array &Conditions = ReadList(*Rule, "conditions");
	const toml::array &Actions = ReadList(*Rule, "actions");

	TMoveUseDatabase &Database = MoveUseDatabases[EventType];
	Database.NumberOfRules++;
	TMoveUseRule &Entry = Database.Rules(Database.NumberOfRules);
	Entry.FirstCondition = NumberOfMoveUseConditions + 1;
	for (const toml::node &ConditionNode : Conditions) {
		NumberOfMoveUseConditions++;
		TMoveUseCondition &Condition = MoveUseConditions(NumberOfMoveUseConditions);
		int Type = ReadEntry(ConditionNode, "conditions", "condition", {"name", "args", "negate", "ignore_result"},
				ConditionSignatures, NUMBER_OF_CONDITION_TYPES, Condition.Parameters);
		Condition.Condition = static_cast<ConditionType>(Type);
		const toml::table &Table = *ConditionNode.as_table();
		bool Negate = ReadBoolean(Table, "negate");
		bool IgnoreResult = ReadBoolean(Table, "ignore_result");
		if (Negate && IgnoreResult) {
			TomlError(ConditionNode.source(), "a condition cannot have both negate and ignore_result");
		}
		Condition.Modifier = Negate ? MODIFIER_INVERT : IgnoreResult ? MODIFIER_TRUE : MODIFIER_NORMAL;
	}
	Entry.LastCondition = NumberOfMoveUseConditions;

	Entry.FirstAction = NumberOfMoveUseActions + 1;
	for (const toml::node &ActionNode : Actions) {
		NumberOfMoveUseActions++;
		TMoveUseAction &Action = MoveUseActions(NumberOfMoveUseActions);
		int Type = ReadEntry(ActionNode, "actions", "action", {"name", "args"}, ActionSignatures,
				NUMBER_OF_ACTION_TYPES, Action.Parameters);
		Action.Action = static_cast<ActionType>(Type);
	}
	Entry.LastAction = NumberOfMoveUseActions;
}

void ReadMoveUseRules(const toml::table &File) {
	for (int i = 0; i < NUMBER_OF_EVENT_TYPES; i++) {
		MoveUseDatabases[i].NumberOfRules = 0;
	}

	TomlCheckKeys(File, "the top level", {"rule"});
	if (const toml::array *Rules = TomlGetArray(File, "rule")) {
		for (const toml::node &Node : *Rules) {
			ReadRule(Node);
		}
	}
}

void LoadMoveUseRules(const char *FileName) {
	toml::table File = TomlParseFile(FileName);
	ReadMoveUseRules(File);
}

void LoadDataBase() {
	char FileName[4096];
	print(1, "Lade Move/Use-Datenbank ...\n");
	sprintf(FileName, "%s/moveuse.toml", DATAPATH);
	LoadMoveUseRules(FileName);
	print(1, "Move/Use-Datenbank mit %d/%d/%d/%d/%d Regeln gelesen.\n",
			MoveUseDatabases[EVENT_USE].NumberOfRules, MoveUseDatabases[EVENT_MULTIUSE].NumberOfRules,
			MoveUseDatabases[EVENT_MOVEMENT].NumberOfRules, MoveUseDatabases[EVENT_COLLISION].NumberOfRules,
			MoveUseDatabases[EVENT_SEPARATION].NumberOfRules);
}
