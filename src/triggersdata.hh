// triggersdata.hh: the trigger rules (triggersdata.cc): their events,
// conditions and actions, the tables the game keeps them in, and their loader,
// which reads triggers.toml. This is the schema of the file: the names of the
// conditions and actions with the kinds of their arguments, how each kind is
// checked and held. What a condition or action does is the function of that
// name in the trigger library (data/scripts/lib/triggers.lua and
// lib/triggers_actions.lua); triggers.cc hands the rules to it and runs them.

#ifndef PATELLA_TRIGGERSDATA_HH
#define PATELLA_TRIGGERSDATA_HH

#include <string>
#include <vector>

#include "containr.hh"
#include "tomlfile.hh"

enum EventType {
	EVENT_USE = 0,
	EVENT_MULTIUSE = 1,
	EVENT_MOVEMENT = 2,
	EVENT_COLLISION = 3,
	EVENT_SEPARATION = 4,
};

// How a condition counts: as it is, inverted ("!" in moveuse.dat), or as
// fulfilled whatever it gives ("~").
enum ModifierType {
	MODIFIER_NORMAL = 0,
	MODIFIER_INVERT = 1,
	MODIFIER_TRUE = 2,
};

// The condition and action types: the indices of ConditionSignatures and
// ActionSignatures (the names triggers.toml gives), not code; the old reader
// of moveuse.dat (tools/convert) sets them by name.
enum ConditionType {
	CONDITION_ISPOSITION = 0,
	CONDITION_ISTYPE = 1,
	CONDITION_ISCREATURE = 2,
	CONDITION_ISPLAYER = 3,
	CONDITION_HASFLAG = 4,
	CONDITION_HASTYPEATTRIBUTE = 5,
	CONDITION_HASINSTANCEATTRIBUTE = 6,
	CONDITION_HASTEXT = 7,
	CONDITION_ISPEACEFUL = 8,
	CONDITION_MAYLOGOUT = 9,
	CONDITION_HASPROFESSION = 10,
	CONDITION_HASLEVEL = 11,
	CONDITION_HASRIGHT = 12,
	CONDITION_HASQUESTVALUE = 13,
	CONDITION_TESTSKILL = 14,
	CONDITION_COUNTOBJECTS = 15,
	CONDITION_COUNTOBJECTSONMAP = 16,
	CONDITION_ISOBJECTTHERE = 17,
	CONDITION_ISCREATURETHERE = 18,
	CONDITION_ISPLAYERTHERE = 19,
	CONDITION_ISOBJECTININVENTORY = 20,
	CONDITION_ISPROTECTIONZONE = 21,
	CONDITION_ISHOUSE = 22,
	CONDITION_ISHOUSEOWNER = 23,
	CONDITION_ISDRESSED = 24,
	CONDITION_RANDOM = 25,
};

enum ActionType {
	ACTION_CREATEONMAP = 0,
	ACTION_CREATE = 1,
	ACTION_MONSTERONMAP = 2,
	ACTION_MONSTER = 3,
	ACTION_EFFECTONMAP = 4,
	ACTION_EFFECT = 5,
	ACTION_TEXTONMAP = 6,
	ACTION_TEXT = 7,
	ACTION_CHANGEONMAP = 8,
	ACTION_CHANGE = 9,
	ACTION_CHANGEREL = 10,
	ACTION_SETATTRIBUTE = 11,
	ACTION_CHANGEATTRIBUTE = 12,
	ACTION_SETQUESTVALUE = 13,
	ACTION_DAMAGE = 14,
	ACTION_SETSTART = 15,
	ACTION_WRITENAME = 16,
	ACTION_WRITETEXT = 17,
	ACTION_LOGOUT = 18,
	ACTION_MOVEALLONMAP = 19,
	ACTION_MOVEALL = 20,
	ACTION_MOVEALLREL = 21,
	ACTION_MOVETOPONMAP = 22,
	ACTION_MOVETOP = 23,
	ACTION_MOVETOPREL = 24,
	ACTION_MOVE = 25,
	ACTION_MOVEREL = 26,
	ACTION_RETRIEVE = 27,
	ACTION_DELETEALLONMAP = 28,
	ACTION_DELETETOPONMAP = 29,
	ACTION_DELETEONMAP = 30,
	ACTION_DELETE = 31,
	ACTION_DELETEININVENTORY = 32,
	ACTION_DESCRIPTION = 33,
	ACTION_LOADDEPOT = 34,
	ACTION_SAVEDEPOT = 35,
	ACTION_SENDMAIL = 36,
	ACTION_NOP = 37,
};

// What a parameter of a condition or an action is, and how the tables hold
// it: an event object (EVENTOBJECT_), an object type number, a flag, a type
// attribute or an instance attribute by number, a coordinate
// (PackAbsoluteCoordinate), a vector (PackRelativeCoordinate), a right
// (RIGHT), a skill number, a number, a text (a dynamic string's number,
// AddDynamicString) or a comparison ('<', 'L' for <=, '=', 'G' for >=, '>',
// 'N' for <>, as the old Compare of triggers.cc took it).
enum ParameterType {
	PARAMETER_OBJECT = 0,
	PARAMETER_TYPE = 1,
	PARAMETER_FLAG = 2,
	PARAMETER_TYPEATTRIBUTE = 3,
	PARAMETER_INSTANCEATTRIBUTE = 4,
	PARAMETER_COORDINATE = 5,
	PARAMETER_VECTOR = 6,
	PARAMETER_RIGHT = 7,
	PARAMETER_SKILL = 8,
	PARAMETER_NUMBER = 9,
	PARAMETER_TEXT = 10,
	PARAMETER_COMPARISON = 11,
};

// The number of event types, one rule database each (TriggerDatabases).
const int NUMBER_OF_EVENT_TYPES = 5;

// The number of condition types and action types.
const int NUMBER_OF_CONDITION_TYPES = 26;
const int NUMBER_OF_ACTION_TYPES = 38;

// The most parameters a condition or an action has.
const int MAX_PARAMETERS = 5;

// The objects a rule names (a PARAMETER_OBJECT; the trigger library gets the
// name): "null", "obj1", "obj2", "user" and "temp".
const int EVENTOBJECT_NULL = 0;
const int EVENTOBJECT_OBJ1 = 1;
const int EVENTOBJECT_OBJ2 = 2;
const int EVENTOBJECT_USER = 3;
const int EVENTOBJECT_TEMP = 4;
const int NUMBER_OF_EVENTOBJECTS = 5;

struct TTriggerCondition {
	ModifierType Modifier;
	ConditionType Condition;
	int Parameters[MAX_PARAMETERS];
};

struct TTriggerAction {
	ActionType Action;
	int Parameters[MAX_PARAMETERS];
};

// A rule: its conditions and actions, as index ranges into TriggerConditions
// and TriggerActions (both empty for a rule with a script), its number in the
// file (1 for the first [[rule]]; the trigger library knows the rule by it)
// and its script, an index into TriggerScripts, or -1 for none.
struct TTriggerRule {
	int FirstCondition;
	int LastCondition;
	int FirstAction;
	int LastAction;
	int Number;
	int Script;
};

struct TTriggerDatabase {
	vector<TTriggerRule> Rules;
	int NumberOfRules;
	TTriggerDatabase() : Rules(1, 100, 100) { NumberOfRules = 0; }
};

// A condition type or an action type as triggers.toml names it, with its
// parameters.
struct TTriggerSignature {
	const char *Key;
	int NumberOfParameters;
	ParameterType Parameters[MAX_PARAMETERS];
};

// The names triggers.toml gives: the condition and action types, by their
// number; the event types; the event objects (EVENTOBJECT_); the instance
// attributes (INSTANCEATTRIBUTE); the rights a rule can ask for; the
// comparisons, with the character the tables hold. The skills are objects.hh's
// SkillKeys.
extern const TTriggerSignature ConditionSignatures[NUMBER_OF_CONDITION_TYPES];
extern const TTriggerSignature ActionSignatures[NUMBER_OF_ACTION_TYPES];
extern const char *const EventKeys[NUMBER_OF_EVENT_TYPES];
extern const char *const EventObjectKeys[NUMBER_OF_EVENTOBJECTS];
extern const char *const InstanceAttributeKeys[];
struct TTriggerKey {
	const char *Key;
	int Value;
};
extern const TTriggerKey RightKeys[2];
extern const TTriggerKey ComparisonKeys[6];

// The script of a rule (script = "triggers/<name>.lua"): its name, the place
// of the key ("<file>:<line>:<column>", for LoadScript's errors) and the
// number LoadScript gave it (-1 until InitTriggers loads it).
struct TTriggerScript {
	std::string Name;
	std::string Where;
	int Number;
};

// A rule in the order of the file: its event and its number in that event's
// database.
struct TTriggerRuleEntry {
	int Event;
	int Rule;
};

// Reads the [[rule]] entries of a parsed triggers.toml into the tables, in
// their order: each rule becomes the next rule of its event's database, its
// conditions and actions are appended to TriggerConditions and
// TriggerActions after the ones there, and its texts are added as dynamic
// strings in their order. Records each rule in TriggerRules, each script in
// TriggerScripts and the place of each condition and action in
// TriggerConditionPlaces and TriggerActionPlaces. Empties the databases,
// TriggerRules and TriggerScripts first, as the game's old LoadDataBase
// emptied the databases; InitTriggers sets the condition and action counts
// to 0. Throws a const char * "<file>:<line>:<column>: <what>" for what is
// wrong with it. The object types must be loaded: a type a rule names must
// exist.
void ReadTriggerRules(const toml::table &File);

// ReadTriggerRules of the triggers.toml file FileName.
void LoadTriggerRules(const char *FileName);

// LoadTriggerRules of triggers.toml in the data folder (DATAPATH), with the
// log lines of the game's old LoadDataBase.
void LoadTriggers();

extern vector<TTriggerCondition> TriggerConditions;
extern int NumberOfTriggerConditions;
extern vector<TTriggerAction> TriggerActions;
extern int NumberOfTriggerActions;
extern TTriggerDatabase TriggerDatabases[NUMBER_OF_EVENT_TYPES];

// The rules in the order of the file: TriggerRules[k] is rule k + 1.
extern std::vector<TTriggerRuleEntry> TriggerRules;
// The scripts the rules name (TTriggerRule::Script).
extern std::vector<TTriggerScript> TriggerScripts;
// The places of the conditions and actions in the file, "<file>:<line>:
// <column>" of their entries, by their numbers in TriggerConditions and
// TriggerActions (entry 0 unused).
extern std::vector<std::string> TriggerConditionPlaces;
extern std::vector<std::string> TriggerActionPlaces;

#endif
