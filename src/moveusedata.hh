// moveusedata.hh: the move and use rules (moveusedata.cc): their events,
// conditions and actions, the tables the game keeps them in, and their loader,
// which reads moveuse.toml. moveuse.cc holds what the rules do.

#ifndef PATELLA_MOVEUSEDATA_HH
#define PATELLA_MOVEUSEDATA_HH

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
// 'N' for <>, as Compare takes it).
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

// The number of event types, one rule database each (MoveUseDatabases).
const int NUMBER_OF_EVENT_TYPES = 5;

// The number of condition types and action types.
const int NUMBER_OF_CONDITION_TYPES = 26;
const int NUMBER_OF_ACTION_TYPES = 38;

// The most parameters a condition or an action has.
const int MAX_PARAMETERS = 5;

// The objects a rule names (a PARAMETER_OBJECT, GetEventObject's Number):
// "null", "obj1", "obj2", "user" and "temp".
const int EVENTOBJECT_NULL = 0;
const int EVENTOBJECT_OBJ1 = 1;
const int EVENTOBJECT_OBJ2 = 2;
const int EVENTOBJECT_USER = 3;
const int EVENTOBJECT_TEMP = 4;
const int NUMBER_OF_EVENTOBJECTS = 5;

struct TMoveUseCondition {
	ModifierType Modifier;
	ConditionType Condition;
	int Parameters[MAX_PARAMETERS];
};

struct TMoveUseAction {
	ActionType Action;
	int Parameters[MAX_PARAMETERS];
};

// A rule: its conditions and actions, as index ranges into MoveUseConditions
// and MoveUseActions.
struct TMoveUseRule {
	int FirstCondition;
	int LastCondition;
	int FirstAction;
	int LastAction;
};

struct TMoveUseDatabase {
	vector<TMoveUseRule> Rules;
	int NumberOfRules;
	TMoveUseDatabase() : Rules(1, 100, 100) { NumberOfRules = 0; }
};

// A condition type or an action type as moveuse.toml names it, with its
// parameters.
struct TMoveUseSignature {
	const char *Key;
	int NumberOfParameters;
	ParameterType Parameters[MAX_PARAMETERS];
};

// The names moveuse.toml gives: the condition and action types, by their
// number; the event types; the event objects (EVENTOBJECT_); the instance
// attributes (INSTANCEATTRIBUTE); the skills by skill number (only 0 to 22
// exist, MAX_SKILLS is the size of the skill tables); the rights a rule can
// ask for; the comparisons, with the character the tables hold.
extern const TMoveUseSignature ConditionSignatures[NUMBER_OF_CONDITION_TYPES];
extern const TMoveUseSignature ActionSignatures[NUMBER_OF_ACTION_TYPES];
extern const char *const EventKeys[NUMBER_OF_EVENT_TYPES];
extern const char *const EventObjectKeys[NUMBER_OF_EVENTOBJECTS];
extern const char *const InstanceAttributeKeys[];
extern const char *const SkillKeys[];
extern const int NUMBER_OF_SKILL_KEYS;
struct TMoveUseKey {
	const char *Key;
	int Value;
};
extern const TMoveUseKey RightKeys[2];
extern const TMoveUseKey ComparisonKeys[6];

// Reads the [[rule]] entries of a parsed moveuse.toml into the tables, in
// their order: each rule becomes the next rule of its event's database, its
// conditions and actions are appended to MoveUseConditions and
// MoveUseActions after the ones there, and its texts are added as dynamic
// strings in their order. Empties the databases first, as the game's old
// LoadDataBase did; InitMoveUse sets the condition and action counts to 0.
// Throws a const char * "<file>:<line>:<column>: <what>" for what is wrong
// with it. The object types must be loaded: a type a rule names must exist.
void ReadMoveUseRules(const toml::table &File);

// ReadMoveUseRules of the moveuse.toml file FileName.
void LoadMoveUseRules(const char *FileName);

// LoadMoveUseRules of moveuse.toml in the data folder (DATAPATH), with the
// log lines of the game's old LoadDataBase.
void LoadDataBase();

extern vector<TMoveUseCondition> MoveUseConditions;
extern int NumberOfMoveUseConditions;
extern vector<TMoveUseAction> MoveUseActions;
extern int NumberOfMoveUseActions;
extern TMoveUseDatabase MoveUseDatabases[NUMBER_OF_EVENT_TYPES];

#endif
