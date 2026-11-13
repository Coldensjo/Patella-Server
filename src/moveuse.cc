#include <cstdint>
#include <cstdlib>

#include "script.hh"
#include "config.hh"
#include "objects.hh"
#include "map.hh"
#include "info.hh"
#include "dbfuncs.hh"
#include "cr.hh"
#include "containr.hh"
#include "operate.hh"
#include "connections.hh"
#include "utils.hh"
#include "time.hh"
#include "reader.hh"
#include "TXTEASymmetricKey.hh"
#include "moveuse.hh"

enum EventType {
	EVENT_USE = 0,
	EVENT_MULTIUSE = 1,
	EVENT_MOVEMENT = 2,
	EVENT_COLLISION = 3,
	EVENT_SEPARATION = 4,
};

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

struct TMoveUseCondition {
	ModifierType Modifier;
	ConditionType Condition;
	int Parameters[5];
};

struct TMoveUseAction {
	ActionType Action;
	int Parameters[5];
};

struct TMoveUseRule {
	int FirstCondition;
	int LastCondition;
	int FirstAction;
	int LastAction;
};

struct TMoveUseDatabase {
	vector<TMoveUseRule> Rules;
	int NumberOfRules;
	// inline  moveuse.c:88
	TMoveUseDatabase() : Rules(1, 100, 100) { abort(); /* STUB */ }
};

struct TDelayedMail {
	uint32_t CharacterID;
	int DepotNumber;
	unsigned char *Packet;
	int PacketSize;
};

// external functions that moveuse.hh cannot declare: they take an enum declared in this file
bool CheckCondition(EventType, TMoveUseCondition &Condition, Object User, Object Obj1, Object Obj2, Object &Temp);
void ExecuteAction(EventType Event, TMoveUseAction &Action, Object User, Object Obj1, Object Obj2, Object &Temp);
bool HandleEvent(EventType Event, Object User, Object Obj1, Object Obj2);

vector<TMoveUseCondition> MoveUseConditions(1, 1000, 1000);	// moveuse.c:99
int NumberOfMoveUseConditions;	// moveuse.c:100
vector<TMoveUseAction> MoveUseActions(1, 1000, 1000);	// moveuse.c:101
int NumberOfMoveUseActions;	// moveuse.c:102
TMoveUseDatabase MoveUseDatabases[5];	// moveuse.c:103
vector<TDelayedMail> DelayedMail(0, 10, 10);	// moveuse.c:104
int DelayedMails;	// moveuse.c:105

// 0x08072450  moveuse.c:116
Object GetEventObject(int /*Number*/, Object /*User*/, Object /*Obj1*/, Object /*Obj2*/, Object /*Temp*/) { abort(); /* STUB */ }

// 0x080724c0  moveuse.c:132
bool Compare(int /*Value1*/, int /*Operator*/, int /*Value2*/) { abort(); /* STUB */ }

// 0x08072530  moveuse.c:154
bool CheckCondition(EventType, TMoveUseCondition & /*Condition*/, Object /*User*/, Object /*Obj1*/, Object /*Obj2*/, Object & /*Temp*/) { abort(); /* STUB */ }

// 0x08073880  moveuse.c:357
Object CreateObject(Object /*Con*/, ObjectType /*Type*/, uint32_t /*Value*/) { abort(); /* STUB */ }

// 0x08073a00  moveuse.c:378
void ChangeObject(Object /*Obj*/, ObjectType /*NewType*/, uint32_t /*Value*/) { abort(); /* STUB */ }

// 0x08073cf0  moveuse.c:412
void MoveOneObject(Object /*Obj*/, Object /*Con*/) { abort(); /* STUB */ }

// 0x08073ef0  moveuse.c:441
void MoveAllObjects(Object /*Obj*/, Object /*Dest*/, Object /*Exclude*/, bool /*MoveUnmovable*/) { abort(); /* STUB */ }

// 0x08074120  moveuse.c:465
void DeleteAllObjects(Object /*Obj*/, Object /*Exclude*/, bool /*DeleteUnmovable*/) { abort(); /* STUB */ }

// 0x080742e0  moveuse.c:487
void ClearField(Object /*Obj*/, Object /*Exclude*/) { abort(); /* STUB */ }

// 0x080746f0  moveuse.c:509
void LoadDepotBox(uint32_t /*CreatureID*/, int /*Nr*/, Object /*Con*/) { abort(); /* STUB */ }

// 0x080748b0  moveuse.c:544
void SaveDepotBox(uint32_t /*CreatureID*/, int /*Nr*/, Object /*Con*/) { abort(); /* STUB */ }

// 0x08074a60  moveuse.c:581
void SendMail(Object /*Obj*/) { abort(); /* STUB */ }

// 0x08075220  moveuse.c:679
void SendMails(TPlayerData * /*PlayerData*/) { abort(); /* STUB */ }

// 0x080754b0  moveuse.c:733
void TextEffect(char * /*Text*/, int /*x*/, int /*y*/, int /*z*/, int /*Radius*/) { abort(); /* STUB */ }

// 0x080755c0  moveuse.c:758
void ExecuteAction(EventType /*Event*/, TMoveUseAction & /*Action*/, Object /*User*/, Object /*Obj1*/, Object /*Obj2*/, Object & /*Temp*/) { abort(); /* STUB */ }

// 0x08077e80  moveuse.c:1106
// local static: int RecursionDepth = 0  (moveuse.c:1107, 0x08127954 .data)
bool HandleEvent(EventType /*Event*/, Object /*User*/, Object /*Obj1*/, Object /*Obj2*/) { abort(); /* STUB */ }

// 0x08078010  moveuse.c:1145
void UseContainer(uint32_t /*CreatureID*/, Object /*Con*/, unsigned char /*Window*/) { abort(); /* STUB */ }

// 0x080781d0  moveuse.c:1180
void UseChest(uint32_t /*CreatureID*/, Object /*Chest*/) { abort(); /* STUB */ }

// 0x08078960  moveuse.c:1259
void UseLiquidContainer(uint32_t /*CreatureID*/, Object /*Obj*/, Object /*Dest*/) { abort(); /* STUB */ }

// 0x08078f60  moveuse.c:1341
void UseFood(uint32_t /*CreatureID*/, Object /*Obj*/) { abort(); /* STUB */ }

// 0x08079110  moveuse.c:1367
void UseTextObject(uint32_t /*CreatureID*/, Object /*Obj*/) { abort(); /* STUB */ }

// 0x08079200  moveuse.c:1390
void UseAnnouncer(uint32_t /*CreatureID*/, Object /*Obj*/) { abort(); /* STUB */ }

// 0x08079620  moveuse.c:1437
void UseKeyDoor(uint32_t, Object /*Key*/, Object /*Door*/) { abort(); /* STUB */ }

// 0x08079870  moveuse.c:1470
void UseNameDoor(uint32_t /*CreatureID*/, Object /*Door*/) { abort(); /* STUB */ }

// 0x08079b10  moveuse.c:1513
void UseLevelDoor(uint32_t /*CreatureID*/, Object /*Door*/) { abort(); /* STUB */ }

// 0x08079db0  moveuse.c:1543
void UseQuestDoor(uint32_t /*CreatureID*/, Object /*Door*/) { abort(); /* STUB */ }

// 0x0807a060  moveuse.c:1573
void UseWeapon(uint32_t /*CreatureID*/, Object /*Weapon*/, Object /*Target*/) { abort(); /* STUB */ }

// 0x0807a2f0  moveuse.c:1603
void UseChangeObject(uint32_t /*CreatureID*/, Object /*Obj*/) { abort(); /* STUB */ }

// 0x0807a870  moveuse.c:1634
void UseObject(uint32_t /*CreatureID*/, Object /*Obj*/) { abort(); /* STUB */ }

// 0x0807a9a0  moveuse.c:1654
void UseObjects(uint32_t /*CreatureID*/, Object /*Obj1*/, Object /*Obj2*/) { abort(); /* STUB */ }

// 0x0807aaf0  moveuse.c:1684
void MovementEvent(Object /*Obj*/, Object /*Start*/, Object /*Dest*/) { abort(); /* STUB */ }

// 0x0807ace0  moveuse.c:1708
void SeparationEvent(Object /*Obj*/, Object /*Start*/) { abort(); /* STUB */ }

// 0x0807b180  moveuse.c:1757
void CollisionEvent(Object /*Obj*/, Object /*Dest*/) { abort(); /* STUB */ }

// 0x0807b8b0  moveuse.c:1833
void LoadParameters(TReadScriptFile & /*Script*/, int * /*Parameters*/, int /*NumberOfParameters*/, ...) { abort(); /* STUB */ }

// 0x0807bcf0  moveuse.c:1914
void LoadCondition(TReadScriptFile & /*Script*/, TMoveUseCondition & /*Condition*/) { abort(); /* STUB */ }

// 0x0807c500  moveuse.c:1953
void LoadAction(TReadScriptFile & /*Script*/, TMoveUseAction & /*Action*/) { abort(); /* STUB */ }

// 0x0807d100  moveuse.c:2002
void LoadDataBase() { abort(); /* STUB */ }

// 0x0807d520  moveuse.c:2084
void InitMoveUse() { abort(); /* STUB */ }

// 0x0807d560  moveuse.c:2095
void ExitMoveUse() { abort(); /* STUB */ }
