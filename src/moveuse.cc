// moveuse.cc: the move and use rules. LoadDataBase reads moveuse.dat into one
// rule database per event type (use, multi-use, movement, collision and
// separation); HandleEvent runs the actions of the first rule whose conditions
// hold. Also here: the object functions those actions use, depot boxes, mail
// (a stamped letter or parcel goes into the addressee's open depot at once,
// otherwise into DelayedMail until SendMails adds it to the depot in the
// player's data), and the built-in uses of objects (the Use* functions).

#include <cstdarg>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <unistd.h>

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
#include "crmain.hh"
#include "crnonpl.hh"
#include "crplayer.hh"
#include "crskill.hh"
#include "houses.hh"
#include "magic.hh"
#include "sending.hh"
#include "strings.hh"
#include "writer.hh"
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

// The number of event types, one rule database each (MoveUseDatabases).
const int NUMBER_OF_EVENT_TYPES = 5;

// The most parameters a condition or an action has.
const int MAX_PARAMETERS = 5;

// The objects a rule names (a PARAMETER_OBJECT, GetEventObject's Number):
// "null", "obj1", "obj2", "user" and "temp".
const int EVENTOBJECT_NULL = 0;
const int EVENTOBJECT_OBJ1 = 1;
const int EVENTOBJECT_OBJ2 = 2;
const int EVENTOBJECT_USER = 3;
const int EVENTOBJECT_TEMP = 4;

struct TMoveUseCondition {
	ModifierType Modifier;
	ConditionType Condition;
	int Parameters[MAX_PARAMETERS];
};

struct TMoveUseAction {
	ActionType Action;
	int Parameters[MAX_PARAMETERS];
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
	TMoveUseDatabase() : Rules(1, 100, 100) { NumberOfRules = 0; }
};

struct TDelayedMail {
	uint32_t CharacterID;
	int DepotNumber;
	unsigned char *Packet;
	int PacketSize;
};

// Functions with external linkage that moveuse.hh cannot declare: they take an
// enum declared in this file.
bool CheckCondition(EventType, TMoveUseCondition &Condition, Object User, Object Obj1, Object Obj2, Object &Temp);
void ExecuteAction(EventType Event, TMoveUseAction &Action, Object User, Object Obj1, Object Obj2, Object &Temp);
bool HandleEvent(EventType Event, Object User, Object Obj1, Object Obj2);

vector<TMoveUseCondition> MoveUseConditions(1, 1000, 1000);
int NumberOfMoveUseConditions;
vector<TMoveUseAction> MoveUseActions(1, 1000, 1000);
int NumberOfMoveUseActions;
TMoveUseDatabase MoveUseDatabases[NUMBER_OF_EVENT_TYPES];
vector<TDelayedMail> DelayedMail(0, 10, 10);
int DelayedMails;

// ChangeObject, LoadDepotBox, SaveDepotBox, SendMail, UseContainer, UseChest,
// UseLiquidContainer, UseFood, UseTextObject, UseAnnouncer, UseKeyDoor,
// UseNameDoor, UseLevelDoor, UseQuestDoor, UseWeapon, UseChangeObject,
// UseObject, UseObjects, MovementEvent, SeparationEvent and CollisionEvent
// throw RESULT; LoadParameters, LoadCondition, LoadAction, LoadDataBase and
// InitMoveUse throw const char *. None has an exception specification (C++17
// has no dynamic ones), so an exception of another type passes to the caller.

Object GetEventObject(int Number, Object User, Object Obj1, Object Obj2, Object Temp) {
	switch (Number) {
		case EVENTOBJECT_NULL:
			return NONE;
		case EVENTOBJECT_OBJ1:
			return Obj1;
		case EVENTOBJECT_OBJ2:
			return Obj2;
		case EVENTOBJECT_USER:
			return User;
		case EVENTOBJECT_TEMP:
			return Temp;
		default:
			error("GetEventObject: Unbekannte Nummer %d.\n", Number);
			return NONE;
	}
}

bool Compare(int Value1, int Operator, int Value2) {
	switch (Operator) {
		case '<':
			return Value1 < Value2;
		case '>':
			return Value1 > Value2;
		case '=':
			return Value1 == Value2;
		case 'N':
			return Value1 != Value2;
		case 'L':
			return Value1 <= Value2;
		case 'G':
			return Value1 >= Value2;
		default:
			error("Compare: Ung\xfcltiger Operator %d.\n", Operator);
			return false;
	}
}

bool CheckCondition(EventType, TMoveUseCondition &Condition, Object User, Object Obj1, Object Obj2, Object &Temp) {
	Object Obj;
	int x1, y1, z1;
	int x2, y2, z2;
	TCreature *cr;
	bool fulfilled = false;

	switch (Condition.Condition) {
		case CONDITION_ISPOSITION:
			Obj = GetEventObject(Condition.Parameters[0], User, Obj1, Obj2, Temp);
			UnpackAbsoluteCoordinate(Condition.Parameters[1], x1, y1, z1);
			GetObjectCoordinates(Obj, x2, y2, z2);
			fulfilled = x1 == x2 && y1 == y2 && z1 == z2;
			break;
		case CONDITION_ISTYPE:
			Obj = GetEventObject(Condition.Parameters[0], User, Obj1, Obj2, Temp);
			fulfilled = Obj.getObjectType().getTypeID() == Condition.Parameters[1];
			break;
		case CONDITION_ISCREATURE:
			Obj = GetEventObject(Condition.Parameters[0], User, Obj1, Obj2, Temp);
			fulfilled = Obj.isCreature();
			break;
		case CONDITION_ISPLAYER:
			Obj = GetEventObject(Condition.Parameters[0], User, Obj1, Obj2, Temp);
			if (Obj.isCreature()) {
				cr = GetCreature(Obj);
				fulfilled = cr != nullptr && cr->Type == PLAYER;
			}
			break;
		case CONDITION_HASFLAG:
			Obj = GetEventObject(Condition.Parameters[0], User, Obj1, Obj2, Temp);
			fulfilled = Obj.getFlag((FLAG)Condition.Parameters[1]);
			break;
		case CONDITION_HASTYPEATTRIBUTE:
			Obj = GetEventObject(Condition.Parameters[0], User, Obj1, Obj2, Temp);
			fulfilled = Compare(Obj.getAttribute((TYPEATTRIBUTE)Condition.Parameters[1]), Condition.Parameters[2], Condition.Parameters[3]);
			break;
		case CONDITION_HASINSTANCEATTRIBUTE:
			Obj = GetEventObject(Condition.Parameters[0], User, Obj1, Obj2, Temp);
			fulfilled = Compare(Obj.getAttribute((INSTANCEATTRIBUTE)Condition.Parameters[1]), Condition.Parameters[2], Condition.Parameters[3]);
			break;
		case CONDITION_HASTEXT: {
			Obj = GetEventObject(Condition.Parameters[0], User, Obj1, Obj2, Temp);
			// Separate statements fix the call order: the parameter's string
			// first. Both calls can log.
			char *Text = GetDynamicString(Condition.Parameters[1]);
			fulfilled = strcmp(GetDynamicString(Obj.getAttribute(TEXTSTRING)), Text) == 0;
			break;
		}
		case CONDITION_ISPEACEFUL:
			Obj = GetEventObject(Condition.Parameters[0], User, Obj1, Obj2, Temp);
			if (Obj.isCreature()) {
				cr = GetCreature(Obj);
				fulfilled = cr != nullptr && cr->IsPeaceful();
			}
			break;
		case CONDITION_MAYLOGOUT:
			Obj = GetEventObject(Condition.Parameters[0], User, Obj1, Obj2, Temp);
			if (Obj.isCreature()) {
				cr = GetCreature(Obj);
				fulfilled = cr != nullptr && cr->Type == PLAYER && cr->LogoutPossible() == LOGOUT_POSSIBLE;
			}
			break;
		case CONDITION_HASPROFESSION:
			Obj = GetEventObject(Condition.Parameters[0], User, Obj1, Obj2, Temp);
			if (Obj.isCreature()) {
				cr = GetCreature(Obj);
				fulfilled = cr != nullptr && cr->Type == PLAYER && ((TPlayer *)cr)->GetEffectiveProfession() == Condition.Parameters[1];
			}
			break;
		case CONDITION_HASLEVEL:
			Obj = GetEventObject(Condition.Parameters[0], User, Obj1, Obj2, Temp);
			if (Obj.isCreature()) {
				cr = GetCreature(Obj);
				fulfilled = cr != nullptr && Compare(cr->Skills[SKILL_LEVEL]->Get(), Condition.Parameters[1], Condition.Parameters[2]);
			}
			break;
		case CONDITION_HASRIGHT:
			Obj = GetEventObject(Condition.Parameters[0], User, Obj1, Obj2, Temp);
			if (Obj.isCreature()) {
				cr = GetCreature(Obj);
				fulfilled = cr != nullptr && cr->Type == PLAYER && CheckRight(cr->ID, (RIGHT)Condition.Parameters[1]);
			}
			break;
		case CONDITION_HASQUESTVALUE:
			Obj = GetEventObject(Condition.Parameters[0], User, Obj1, Obj2, Temp);
			if (Obj.isCreature()) {
				cr = GetCreature(Obj);
				fulfilled = cr != nullptr && cr->Type == PLAYER
						&& Compare(((TPlayer *)cr)->GetQuestValue(Condition.Parameters[1]), Condition.Parameters[2], Condition.Parameters[3]);
			}
			break;
		case CONDITION_TESTSKILL:
			Obj = GetEventObject(Condition.Parameters[0], User, Obj1, Obj2, Temp);
			if (Obj.isCreature()) {
				cr = GetCreature(Obj);
				if (in(Condition.Parameters[1], 0, MAX_SKILLS - 1)) {
					fulfilled = cr != nullptr && cr->Skills[Condition.Parameters[1]]->Probe(Condition.Parameters[2], Condition.Parameters[3], true);
				} else {
					error("CheckCondition (TestSkill): Ung\xfcltige Skillnummer %d.\n", Condition.Parameters[1]);
				}
			}
			break;
		case CONDITION_COUNTOBJECTS:
			Obj = GetEventObject(Condition.Parameters[0], User, Obj1, Obj2, Temp);
			fulfilled = Compare(CountObjectsInContainer(GetMapContainer(Obj)), Condition.Parameters[1], Condition.Parameters[2]);
			break;
		case CONDITION_COUNTOBJECTSONMAP:
			UnpackAbsoluteCoordinate(Condition.Parameters[0], x1, y1, z1);
			fulfilled = Compare(CountObjectsInContainer(GetMapContainer(x1, y1, z1)), Condition.Parameters[1], Condition.Parameters[2]);
			break;
		case CONDITION_ISOBJECTTHERE:
			UnpackAbsoluteCoordinate(Condition.Parameters[0], x1, y1, z1);
			Temp = GetFirstSpecObject(x1, y1, z1, ObjectType(Condition.Parameters[1]));
			fulfilled = Temp != NONE;
			break;
		case CONDITION_ISCREATURETHERE:
			UnpackAbsoluteCoordinate(Condition.Parameters[0], x1, y1, z1);
			Temp = GetFirstSpecObject(x1, y1, z1, ObjectType(TYPEID_CREATURE));
			fulfilled = Temp != NONE;
			break;
		case CONDITION_ISPLAYERTHERE:
			UnpackAbsoluteCoordinate(Condition.Parameters[0], x1, y1, z1);
			Temp = GetFirstSpecObject(x1, y1, z1, ObjectType(TYPEID_CREATURE));
			if (Temp != NONE) {
				cr = GetCreature(Temp);
				fulfilled = cr != nullptr && cr->Type == PLAYER;
			}
			break;
		case CONDITION_ISOBJECTININVENTORY:
			Obj = GetEventObject(Condition.Parameters[0], User, Obj1, Obj2, Temp);
			if (Obj.isCreature()) {
				// The type is built before getCreatureID() is called.
				ObjectType Type(Condition.Parameters[1]);
				Temp = GetInventoryObject(Obj.getCreatureID(), Type, Condition.Parameters[2]);
				fulfilled = Temp != NONE;
			}
			break;
		case CONDITION_ISPROTECTIONZONE:
			Obj = GetEventObject(Condition.Parameters[0], User, Obj1, Obj2, Temp);
			GetObjectCoordinates(Obj, x1, y1, z1);
			fulfilled = IsProtectionZone(x1, y1, z1);
			break;
		case CONDITION_ISHOUSE:
			Obj = GetEventObject(Condition.Parameters[0], User, Obj1, Obj2, Temp);
			GetObjectCoordinates(Obj, x1, y1, z1);
			fulfilled = IsHouse(x1, y1, z1);
			break;
		case CONDITION_ISHOUSEOWNER:
			Obj = GetEventObject(Condition.Parameters[0], User, Obj1, Obj2, Temp);
			GetObjectCoordinates(Obj, x1, y1, z1);
			Obj = GetEventObject(Condition.Parameters[1], User, Obj1, Obj2, Temp);
			if (Obj.isCreature()) {
				cr = GetCreature(Obj);
				fulfilled = cr != nullptr && cr->Type == PLAYER && IsOwner(GetHouseID(x1, y1, z1), (TPlayer *)cr);
			}
			break;
		case CONDITION_ISDRESSED:
			Obj = GetEventObject(Condition.Parameters[0], User, Obj1, Obj2, Temp);
			if (Obj.getFlag(CLOTHES)) {
				fulfilled = (uint32_t)GetObjectBodyPosition(Obj) == Obj.getAttribute(BODYPOSITION);
			} else {
				error("CheckCondition (IsDressed): Objekt ist kein Kleidungsst\xfc" "ck.\n");
			}
			break;
		case CONDITION_RANDOM:
			fulfilled = random(1, 100) <= Condition.Parameters[0];
			break;
		default:
			error("CheckCondition: Unbekannte Bedingung %d.\n", Condition.Condition);
			return false;
	}

	if (Condition.Modifier == MODIFIER_NORMAL) {
		return fulfilled;
	} else if (Condition.Modifier == MODIFIER_INVERT) {
		return !fulfilled;
	} else if (Condition.Modifier == MODIFIER_TRUE) {
		return true;
	} else {
		return false;
	}
}

Object CreateObject(Object Con, ObjectType Type, uint32_t Value) {
again:
	try {
		if (Con.isBodyContainer()) {
			return CreateAtCreature(GetObjectCreatureID(Con), Type, Value);
		} else {
			return Create(Con, Type, Value);
		}
	} catch (RESULT r) {
		if (r == CONTAINERFULL || r == TOOHEAVY || r == NOTTAKABLE) {
			Con = GetMapContainer(Con);
			goto again;
		}
		int x, y, z;
		GetObjectCoordinates(Con, x, y, z);
		error("moveuse::CreateObject: Exception %d, Objekt %d, Position [%d,%d,%d].\n", r, Type.getTypeID(), x, y, z);
		return NONE;
	}
}

void ChangeObject(Object Obj, ObjectType NewType, uint32_t Value) {
	if (!Obj.exists()) {
		error("ChangeObject: \xdc" "bergebenes Objekt existiert nicht (1, NewType=%d).\n", NewType.getTypeID());
		return;
	}

	Object Con = Obj.getContainer();
again:
	if (!Obj.exists()) {
		error("ChangeObject: \xdc" "bergebenes Objekt existiert nicht (2, NewType=%d).\n", NewType.getTypeID());
		return;
	}

	try {
		if (Obj.getFlag(CUMULATIVE) && Obj.getAttribute(AMOUNT) > 1) {
			Move(0, Obj, Con, Obj.getAttribute(AMOUNT) - 1, true, NONE);
		}
		Change(Obj, NewType, Value);
	} catch (RESULT r) {
		if (!Obj.exists()) {
			error("ChangeObject: \xdc" "bergebenes Objekt existiert nicht (3, NewType=%d).\n", NewType.getTypeID());
			return;
		}

		if (r == CONTAINERFULL) {
			Con = GetMapContainer(Obj);
		} else if (r == TOOHEAVY) {
			Move(0, Obj, GetMapContainer(Obj), -1, true, NONE);
		} else if (r == NOROOM && Obj.getFlag(CUMULATIVE) && Obj.getAttribute(AMOUNT) > 1) {
			Move(0, Obj, GetMapContainer(Con), Obj.getAttribute(AMOUNT) - 1, true, NONE);
		} else {
			throw;
		}
		goto again;
	}
}

void MoveOneObject(Object Obj, Object Con) {
	if (!Obj.exists()) {
		error("MoveOneObject: \xdc" "bergebenes Objekt existiert nicht.\n");
		return;
	}

	if (!Con.exists() || !Con.getFlag(CONTAINER)) {
		error("MoveOneObject: \"Con\" ist kein Container.\n");
		return;
	}

	if (Obj.isCreature() && !Con.isMapContainer()) {
		error("MoveOneObject: \"Con\" ist kein MapContainer.\n");
		return;
	}

	try {
		if (Obj.getFlag(LIQUIDPOOL) || Obj.getFlag(MAGICFIELD)) {
			Delete(Obj, -1);
		} else {
			Move(0, Obj, Con, -1, true, NONE);
		}
	} catch (RESULT r) {
		int x, y, z;
		GetObjectCoordinates(Con, x, y, z);
		if (r != DESTROYED) {
			error("MoveOneObject: Exception %d; Koordinate [%d,%d,%d].\n", r, x, y, z);
		}
		if (r == CONTAINERFULL) {
			Delete(Obj, -1);
		}
	}
}

void MoveAllObjects(Object Obj, Object Dest, Object Exclude, bool MoveUnmovable) {
	if (Obj == NONE) {
		return;
	}

	if (!Dest.isMapContainer()) {
		error("MoveAllObjects: \"Dest\" ist kein Mapcontainer.\n");
		return;
	}

	MoveAllObjects(Obj.getNextObject(), Dest, Exclude, MoveUnmovable);
	if (Obj == Exclude) {
		return;
	}

	ObjectType Type = Obj.getObjectType();
	try {
		if (Obj.getFlag(LIQUIDPOOL) || Obj.getFlag(MAGICFIELD)) {
			Delete(Obj, -1);
		} else if (MoveUnmovable || !Obj.getFlag(UNMOVE)) {
			Move(0, Obj, Dest, -1, true, NONE);
		}
	} catch (RESULT r) {
		int x, y, z;
		GetObjectCoordinates(Dest, x, y, z);
		if (r != DESTROYED) {
			error("MoveAllObjects: Exception %d; Objekt %d, Zielkoordinate [%d,%d,%d].\n", r, Type.getTypeID(), x, y, z);
		}
	}
}

void DeleteAllObjects(Object Obj, Object Exclude, bool DeleteUnmovable) {
	if (Obj == NONE) {
		return;
	}

	DeleteAllObjects(Obj.getNextObject(), Exclude, DeleteUnmovable);
	if (Obj == Exclude) {
		return;
	}

	if (Obj.isCreature()) {
		int x, y, z;
		GetObjectCoordinates(Obj, x, y, z);
		error("DeleteAllObjects: An Position [%d,%d,%d] soll eine Kreatur gel\xf6scht werden.\n", x, y, z);
		return;
	}

	try {
		if (DeleteUnmovable || !Obj.getFlag(UNMOVE) || Obj.getFlag(LIQUIDPOOL) || Obj.getFlag(MAGICFIELD)) {
			Delete(Obj, -1);
		}
	} catch (RESULT r) {
		error("DeleteAllObjects: Exception %d.\n", r);
	}
}

void ClearField(Object Obj, Object Exclude) {
	if (!Obj.exists()) {
		error("ClearField: Objekt existiert nicht.\n");
		return;
	}

	int x, y, z;
	GetObjectCoordinates(Obj, x, y, z);
	// The container is fetched before getNextObject() is called.
	Object Con;
	if (MovePossible(x + 1, y, z)) {
		Con = GetMapContainer(x + 1, y, z);
	} else if (MovePossible(x, y + 1, z)) {
		Con = GetMapContainer(x, y + 1, z);
	} else if (MovePossible(x - 1, y, z)) {
		Con = GetMapContainer(x - 1, y, z);
	} else if (MovePossible(x, y - 1, z)) {
		Con = GetMapContainer(x, y - 1, z);
	} else if (JumpPossible(x + 1, y, z, false)) {
		Con = GetMapContainer(x + 1, y, z);
	} else if (JumpPossible(x, y + 1, z, false)) {
		Con = GetMapContainer(x, y + 1, z);
	} else if (JumpPossible(x - 1, y, z, false)) {
		Con = GetMapContainer(x - 1, y, z);
	} else if (JumpPossible(x, y - 1, z, false)) {
		Con = GetMapContainer(x, y - 1, z);
	} else {
		return;
	}
	MoveAllObjects(Obj.getNextObject(), Con, Exclude, true);
}

void LoadDepotBox(uint32_t CreatureID, int Nr, Object Con) {
	TCreature *cr = GetCreature(CreatureID);
	if (cr == nullptr) {
		error("moveuse::LoadDepotBox: Kreatur nicht gefunden.\n");
		return;
	}

	if (!Con.exists()) {
		error("moveuse::LoadDepotBox: \xdc" "bergebener Container existiert nicht.\n");
		return;
	}

	if (!Con.getFlag(CONTAINER)) {
		error("moveuse::LoadDepotBox: \xdc" "bergebenes Objekt ist kein Container.\n");
		return;
	}

	if (cr->Type != PLAYER) {
		error("moveuse::LoadDepotBox: Kreatur ist kein Spieler.\n");
		return;
	}

	TPlayer *pl = (TPlayer *)cr;
	LoadDepot(pl->PlayerData, Nr, Con);
	pl->Depot = Con;
	pl->DepotNr = Nr;
	int Count = CountObjects(Con) - 1;
	pl->DepotSpace = GetDepotSize(Nr, CheckRight(pl->ID, PREMIUM_ACCOUNT)) - Count;
	print(3, "Depot von %s hat %d freie Pl\xe4tze.\n", pl->Name, pl->DepotSpace);
	SendMessage(pl->GetConnection(), TALK_STATUS_MESSAGE, "Your depot contains %d item%s.", Count, (Count == 1 ? "" : "s"));
	if (pl->DepotSpace < 0) {
		SendMessage(pl->GetConnection(), TALK_INFO_MESSAGE, "Your depot is full. Remove surplus items before storing new ones.");
	}
}

void SaveDepotBox(uint32_t CreatureID, int Nr, Object Con) {
	TCreature *cr = GetCreature(CreatureID);
	if (cr == nullptr) {
		error("moveuse::SaveDepotBox: Kreatur nicht gefunden.\n");
		return;
	}

	if (!Con.exists()) {
		error("moveuse::SaveDepotBox: \xdc" "bergebener Container existiert nicht.\n");
		return;
	}

	if (!Con.getFlag(CONTAINER)) {
		error("moveuse::SaveDepotBox: \xdc" "bergebenes Objekt ist kein Container.\n");
		return;
	}

	if (cr->Type != PLAYER) {
		error("moveuse::SaveDepotBox: Kreatur ist kein Spieler.\n");
		return;
	}

	TPlayer *pl = (TPlayer *)cr;
	int Count = CountObjects(Con) - 1;
	Log("game", "Speichere Depot %d von %s ... Depotgr\xf6\xdf" "e: %d.\n", Nr, pl->Name, Count);
	print(3, "Depot von %s: berechnete freie Pl\xe4tze %d, tats\xe4" "chliche Objekte %d.\n", pl->Name, pl->DepotSpace, Count);
	SaveDepot(pl->PlayerData, Nr, Con);
	pl->Depot = NONE;
	while (true) {
		Object Obj = GetFirstContainerObject(Con);
		if (Obj == NONE) {
			break;
		}
		Delete(Obj, -1);
	}
}

void SendMail(Object Obj) {
	if (!Obj.exists()) {
		error("SendMail: \xdc" "bergebenes Objekt existiert nicht.\n");
		return;
	}

	if (Obj.getObjectType() != GetSpecialObject(PARCEL_NEW) && Obj.getObjectType() != GetSpecialObject(LETTER_NEW)) {
		return;
	}

	print(3, "Versende Post...\n");
	char *Text;
	if (Obj.getObjectType() == GetSpecialObject(LETTER_NEW)) {
		Text = GetDynamicString(Obj.getAttribute(TEXTSTRING));
	} else {
		Object Label = GetFirstContainerObject(Obj);
		while (Label != NONE && Label.getObjectType() != GetSpecialObject(PARCEL_LABEL)) {
			Label++;
		}
		if (Label == NONE) {
			return;
		}
		Text = GetDynamicString(Label.getAttribute(TEXTSTRING));
	}

	if (Text == nullptr) {
		return;
	}

	// Quirks: both lines are copied up to 256 characters, so the terminator can
	// land one byte behind Addressee or Town, and a text without a line break
	// makes the second loop start one byte behind the terminator.
	int pos = 0;
	char Addressee[256];
	while (Text[pos] != '\n' && Text[pos] != 0 && pos < 256) {
		Addressee[pos] = Text[pos];
		pos++;
	}
	Addressee[pos] = 0;

	int pos2 = 0;
	char Town[256];
	while (Text[pos + 1 + pos2] != '\n' && Text[pos + 1 + pos2] != 0 && pos2 < 256) {
		Town[pos2] = Text[pos + 1 + pos2];
		pos2++;
	}
	Town[pos2] = 0;

	Trim(Addressee);
	Trim(Town);
	if (Addressee[0] == 0 || strlen(Addressee) >= 30 || Town[0] == 0) {
		return;
	}

	Log("game", "Post an %s in %s.\n", Addressee, Town);
	int DepotNumber = GetDepotNumber(Town);
	if (DepotNumber == -1) {
		return;
	}

	TPlayer *Player;
	uint32_t CharacterID;
	bool PlayerOnline = IdentifyPlayer(Addressee, true, true, Player) == 0;
	if (PlayerOnline) {
		CharacterID = Player->ID;
	} else {
		CharacterID = GetCharacterID(Addressee);
		if (CharacterID == 0) {
			return;
		}
	}

	if (PlayerOnline && Player->DepotNr == DepotNumber && Player->Depot != NONE) {
		print(3, "Adressat ist eingeloggt und hat Depot offen.\n");
		try {
			Move(0, Obj, Player->Depot, -1, true, NONE);
			Change(Obj, GetSpecialObject(Obj.getObjectType() == GetSpecialObject(LETTER_NEW) ? LETTER_STAMPED : PARCEL_STAMPED), 0);
			Log("game", "Post an %s in %s versandt.\n", Addressee, Town);
			Player->DepotSpace -= CountObjects(Obj);
			print(3, "Depot von %s hat jetzt %d freie Pl\xe4tze.\n", Player->Name, Player->DepotSpace);
			SendMessage(Player->GetConnection(), TALK_INFO_MESSAGE, "New mail has arrived.");
			print(3, "Post erfolgreich versandt.\n");
		} catch (RESULT r) {
			if (r != CONTAINERFULL) {
				error("SendMail (Depot offen): Exception %d.\n", r);
			}
			print(3, "Exception %d beim Postversand.\n", r);
		}
		return;
	}

	print(3, "Adressat ist nicht eingeloggt oder hat Depot geschlossen.\n");
	Change(Obj, GetSpecialObject(Obj.getObjectType() == GetSpecialObject(LETTER_NEW) ? LETTER_STAMPED : PARCEL_STAMPED), 0);
	try {
		TDynamicWriteBuffer Buffer(1024);
		SaveObjects(Obj, Buffer, true);
		Delete(Obj, -1);
		TDelayedMail &D = DelayedMail(DelayedMails);
		D.CharacterID = CharacterID;
		D.DepotNumber = DepotNumber;
		D.PacketSize = Buffer.getDataSize();
		D.Packet = new unsigned char[D.PacketSize];
		memcpy(D.Packet, Buffer.getData(), D.PacketSize);
	} catch (const char *s) {
		error("SendMail: Kann Paket nicht schreiben (%s).\n", s);
		return;
	} catch (RESULT r) {
		error("SendMail: Kann Paket nicht l\xf6schen (%d).\n", r);
		return;
	}
	DelayedMails++;

	Log("game", "Post an %s in %s zur\xfc" "ckgestellt.\n", Addressee, Town);
	print(3, "Post zur\xfc" "ckgestellt.\n");
	if (PlayerOnline) {
		SendMails(Player->PlayerData);
	} else {
		LoadCharacterOrder(CharacterID);
	}
}

void SendMails(TPlayerData *PlayerData) {
	if (PlayerData == nullptr) {
		error("SendMails: PlayerData ist NULL.\n");
		return;
	}

	if (PlayerData->CharacterID == 0) {
		error("SendMails: Slot ist nicht belegt.\n");
		return;
	}

	// Locked is the ID of the thread that holds the slot: gettid(), which is
	// what getpid() returned per thread under LinuxThreads.
	if (PlayerData->Locked != gettid()) {
		error("SendMails: Slot ist nicht vom GameThread gesperrt.\n");
		return;
	}

	for (int i = 0; i < DelayedMails; i++) {
		TDelayedMail &D = DelayedMail(i);
		if (D.CharacterID != PlayerData->CharacterID) {
			continue;
		}

		if (!in(D.DepotNumber, 0, MAX_DEPOTS - 1)) {
			error("SendMails: Ung\xfcltige Depotnummer %d.\n", D.DepotNumber);
			D.DepotNumber = 0;
		}

		bool DepotEmpty = PlayerData->Depot[D.DepotNumber] == nullptr;
		int NewDepotSize;
		if (DepotEmpty) {
			NewDepotSize = D.PacketSize + 5;
		} else {
			NewDepotSize = D.PacketSize + PlayerData->DepotSize[D.DepotNumber];
		}

		unsigned char *NewDepot = new unsigned char[NewDepotSize];
		memcpy(NewDepot, D.Packet, D.PacketSize);
		delete[] D.Packet;
		if (DepotEmpty) {
			TWriteBuffer Buffer(&NewDepot[D.PacketSize], 5);
			Buffer.writeWord((unsigned short)GetSpecialObject(DEPOT_CHEST).getTypeID());
			Buffer.writeByte(END_OF_ATTRIBUTES);
			Buffer.writeWord(END_OF_OBJECTS);
		} else {
			memcpy(&NewDepot[D.PacketSize], PlayerData->Depot[D.DepotNumber], PlayerData->DepotSize[D.DepotNumber]);
			delete[] PlayerData->Depot[D.DepotNumber];
		}

		PlayerData->Depot[D.DepotNumber] = NewDepot;
		PlayerData->Dirty = true;
		PlayerData->DepotSize[D.DepotNumber] = NewDepotSize;
		Log("game", "Post an %s zugestellt.\n", PlayerData->Name);
		if (i != DelayedMails - 1) {
			// Member-call syntax evaluates DelayedMail(i) first; with "="
			// C++17 evaluates the right side first.
			DelayedMail(i).operator=(DelayedMail(DelayedMails - 1));
		}
		DelayedMails--;
		i--;
	}
}

void TextEffect(char *Text, int x, int y, int z, int Radius) {
	if (Text == nullptr) {
		error("TextEffect: Text existiert nicht.\n");
		return;
	}

	TFindCreatures Search(Radius, Radius, x, y, FIND_PLAYERS);
	while (true) {
		uint32_t ID = Search.getNext();
		if (ID == 0) {
			break;
		}

		TPlayer *pl = GetPlayer(ID);
		if (pl == nullptr) {
			continue;
		}

		TConnection *Connection = pl->GetConnection();
		if (Connection == nullptr) {
			continue;
		}

		int destX, destY, destZ;
		Connection->GetPosition(destX, destY, destZ);
		if (Radius <= 7 && z != destZ) {
			continue;
		}

		if (Radius <= 30 && z != destZ && (z > 7 || destZ > 7)) {
			continue;
		}

		SendTalk(Connection, 0, "", TALK_ANIMAL_LOW, x, y, z, Text);
	}
}

void ExecuteAction(EventType Event, TMoveUseAction &Action, Object User, Object Obj1, Object Obj2, Object &Temp) {
	Object Obj, Con;
	int x, y, z, dx, dy, dz;
	TCreature *cr, *cr2;
	char Text[256], *Text2;

	try {
		switch (Action.Action) {
			case ACTION_CREATEONMAP:
				UnpackAbsoluteCoordinate(Action.Parameters[0], x, y, z);
				Temp = CreateObject(GetMapContainer(x, y, z), ObjectType(Action.Parameters[1]), Action.Parameters[2]);
				break;
			case ACTION_CREATE:
				Obj = GetEventObject(Action.Parameters[0], User, Obj1, Obj2, Temp);
				Temp = CreateObject(Obj.getContainer(), ObjectType(Action.Parameters[1]), Action.Parameters[2]);
				break;
			case ACTION_MONSTERONMAP:
				UnpackAbsoluteCoordinate(Action.Parameters[0], x, y, z);
				CreateMonster(Action.Parameters[1], x, y, z, 0, 0, true);
				break;
			case ACTION_MONSTER:
				Obj = GetEventObject(Action.Parameters[0], User, Obj1, Obj2, Temp);
				GetObjectCoordinates(Obj, x, y, z);
				CreateMonster(Action.Parameters[1], x, y, z, 0, 0, true);
				break;
			case ACTION_EFFECTONMAP:
				UnpackAbsoluteCoordinate(Action.Parameters[0], x, y, z);
				GraphicalEffect(GetMapContainer(x, y, z), Action.Parameters[1]);
				break;
			case ACTION_EFFECT:
				Obj = GetEventObject(Action.Parameters[0], User, Obj1, Obj2, Temp);
				GraphicalEffect(GetMapContainer(Obj), Action.Parameters[1]);
				break;
			case ACTION_TEXTONMAP:
				UnpackAbsoluteCoordinate(Action.Parameters[0], x, y, z);
				TextEffect(GetDynamicString(Action.Parameters[1]), x, y, z, Action.Parameters[2]);
				break;
			case ACTION_TEXT:
				Obj = GetEventObject(Action.Parameters[0], User, Obj1, Obj2, Temp);
				GetObjectCoordinates(Obj, x, y, z);
				TextEffect(GetDynamicString(Action.Parameters[1]), x, y, z, Action.Parameters[2]);
				break;
			case ACTION_CHANGEONMAP:
				UnpackAbsoluteCoordinate(Action.Parameters[0], x, y, z);
				Temp = GetFirstSpecObject(x, y, z, ObjectType(Action.Parameters[1]));
				if (Temp == NONE) {
					error("ExecuteAction (CHANGEONMAP): Kein Objekt %d auf [%d,%d,%d].\n", Action.Parameters[1], x, y, z);
					return;
				}
				ChangeObject(Temp, ObjectType(Action.Parameters[2]), Action.Parameters[3]);
				break;
			case ACTION_CHANGE:
				Obj = GetEventObject(Action.Parameters[0], User, Obj1, Obj2, Temp);
				ChangeObject(Obj, ObjectType(Action.Parameters[1]), Action.Parameters[2]);
				break;
			case ACTION_CHANGEREL:
				Obj = GetEventObject(Action.Parameters[0], User, Obj1, Obj2, Temp);
				GetObjectCoordinates(Obj, x, y, z);
				UnpackRelativeCoordinate(Action.Parameters[1], dx, dy, dz);
				x += dx;
				y += dy;
				z += dz;
				Temp = GetFirstSpecObject(x, y, z, ObjectType(Action.Parameters[2]));
				if (Temp == NONE) {
					error("ExecuteAction (CHANGEREL): Kein Objekt %d auf [%d,%d,%d].\n", Action.Parameters[2], x, y, z);
					return;
				}
				ChangeObject(Temp, ObjectType(Action.Parameters[3]), Action.Parameters[4]);
				break;
			case ACTION_SETATTRIBUTE:
				Obj = GetEventObject(Action.Parameters[0], User, Obj1, Obj2, Temp);
				Change(Obj, (INSTANCEATTRIBUTE)Action.Parameters[1], Action.Parameters[2]);
				break;
			case ACTION_CHANGEATTRIBUTE:
				Obj = GetEventObject(Action.Parameters[0], User, Obj1, Obj2, Temp);
				x = Obj.getAttribute((INSTANCEATTRIBUTE)Action.Parameters[1]) + Action.Parameters[2];
				if (x <= 0) {
					x = Obj.getAttribute((INSTANCEATTRIBUTE)Action.Parameters[1]) != 0;
				}
				Change(Obj, (INSTANCEATTRIBUTE)Action.Parameters[1], x);
				break;
			case ACTION_SETQUESTVALUE:
				Obj = GetEventObject(Action.Parameters[0], User, Obj1, Obj2, Temp);
				if (!Obj.isCreature()) {
					error("ExecuteAction (SETQUESTVALUE): Objekt ist keine Kreatur, sondern Typ %ld.\n", (long)Obj.getObjectType().getTypeID());
					return;
				}
				cr = GetCreature(Obj);
				if (cr == nullptr || cr->Type != PLAYER) {
					error("ExecuteAction (SETQUESTVALUE): Kreatur existiert nicht oder ist kein Spieler.\n");
					return;
				}
				((TPlayer *)cr)->SetQuestValue(Action.Parameters[1], Action.Parameters[2]);
				break;
			case ACTION_DAMAGE:
				Obj = GetEventObject(Action.Parameters[0], User, Obj1, Obj2, Temp);
				cr2 = nullptr;
				if (Obj != NONE) {
					if (Obj.isCreature()) {
						cr2 = GetCreature(Obj);
					}
					if (Obj.getFlag(MAGICFIELD) && Obj.getAttribute(RESPONSIBLE) != 0) {
						// CronInfo() runs before the type attribute is read.
						uint32_t Remaining = CronInfo(Obj, false);
						if (Remaining + 5 > Obj.getAttribute(TOTALEXPIRETIME)) {
							cr2 = GetCreature(Obj.getAttribute(RESPONSIBLE));
						}
					}
				}
				Obj = GetEventObject(Action.Parameters[1], User, Obj1, Obj2, Temp);
				if (!Obj.isCreature()) {
					error("ExecuteAction (DAMAGE): Objekt ist keine Kreatur, sondern Typ %d.\n", Obj.getObjectType().getTypeID());
					return;
				}
				cr = GetCreature(Obj);
				if (cr == nullptr) {
					error("ExecuteAction (DAMAGE): Kreatur existiert nicht.\n");
					return;
				}
				cr->Damage(cr2, Action.Parameters[3], Action.Parameters[2]);
				break;
			case ACTION_SETSTART:
				Obj = GetEventObject(Action.Parameters[0], User, Obj1, Obj2, Temp);
				if (!Obj.isCreature()) {
					throw ERROR;
				}
				cr = GetCreature(Obj);
				if (cr == nullptr || cr->Type != PLAYER) {
					throw ERROR;
				}
				Log("game", "Setze neue Startposition f\xfcr %s.\n", cr->Name);
				UnpackAbsoluteCoordinate(Action.Parameters[1], cr->startx, cr->starty, cr->startz);
				((TPlayer *)cr)->SaveData();
				break;
			case ACTION_WRITENAME:
				Obj = GetEventObject(Action.Parameters[0], User, Obj1, Obj2, Temp);
				if (!Obj.isCreature()) {
					throw ERROR;
				}
				cr = GetCreature(Obj);
				if (cr == nullptr) {
					throw ERROR;
				}
				memset(Text, 0, sizeof(Text));
				Text2 = GetDynamicString(Action.Parameters[1]);
				for (int pos = 0; pos < (int)strlen(Text2); pos++) {
					if (Text2[pos] == '%' && Text2[pos + 1] == 'N') {
						strcat(Text, cr->Name);
						pos++;
					} else {
						Text[strlen(Text)] = Text2[pos];
					}
					if (Text[200] != 0) {
						throw ERROR;
					}
				}
				Obj = GetEventObject(Action.Parameters[2], User, Obj1, Obj2, Temp);
				if (Obj == NONE || !Obj.getFlag(TEXT)) {
					throw ERROR;
				}
				DeleteDynamicString(Obj.getAttribute(TEXTSTRING));
				Change(Obj, TEXTSTRING, AddDynamicString(Text));
				Change(Obj, EDITOR, 0);
				break;
			case ACTION_WRITETEXT:
				Obj = GetEventObject(Action.Parameters[1], User, Obj1, Obj2, Temp);
				if (Obj == NONE || !Obj.getFlag(TEXT)) {
					throw ERROR;
				}
				DeleteDynamicString(Obj.getAttribute(TEXTSTRING));
				Change(Obj, TEXTSTRING, AddDynamicString(GetDynamicString(Action.Parameters[0])));
				Change(Obj, EDITOR, 0);
				break;
			case ACTION_LOGOUT:
				Obj = GetEventObject(Action.Parameters[0], User, Obj1, Obj2, Temp);
				if (!Obj.isCreature()) {
					throw ERROR;
				}
				cr = GetCreature(Obj);
				if (cr == nullptr) {
					throw ERROR;
				}
				cr->StartLogout(true, true);
				break;
			case ACTION_MOVEALLONMAP:
				UnpackAbsoluteCoordinate(Action.Parameters[0], x, y, z);
				Obj = GetFirstObject(x, y, z);
				UnpackAbsoluteCoordinate(Action.Parameters[1], x, y, z);
				Con = GetMapContainer(x, y, z);
				MoveAllObjects(Obj, Con, NONE, false);
				break;
			case ACTION_MOVEALL:
				Obj = GetEventObject(Action.Parameters[0], User, Obj1, Obj2, Temp);
				Obj = GetFirstContainerObject(Obj.getContainer());
				UnpackAbsoluteCoordinate(Action.Parameters[1], x, y, z);
				Con = GetMapContainer(x, y, z);
				MoveAllObjects(Obj, Con, Obj, false);
				break;
			case ACTION_MOVEALLREL:
				Obj = GetEventObject(Action.Parameters[0], User, Obj1, Obj2, Temp);
				Obj = GetFirstContainerObject(Obj.getContainer());
				GetObjectCoordinates(Obj, x, y, z);
				UnpackRelativeCoordinate(Action.Parameters[1], dx, dy, dz);
				Con = GetMapContainer(x + dx, y + dy, z + dz);
				MoveAllObjects(Obj, Con, Obj, false);
				break;
			case ACTION_MOVETOPONMAP:
				UnpackAbsoluteCoordinate(Action.Parameters[0], x, y, z);
				Obj = GetFirstSpecObject(x, y, z, ObjectType(Action.Parameters[1]));
				if (Obj == NONE) {
					error("ExecuteAction (MOVETOPONMAP): Kein Objekt %d auf [%d,%d,%d].\n", Action.Parameters[1], x, y, z);
					return;
				}
				UnpackAbsoluteCoordinate(Action.Parameters[2], x, y, z);
				Con = GetMapContainer(x, y, z);
				MoveAllObjects(Obj.getNextObject(), Con, NONE, true);
				break;
			case ACTION_MOVETOP:
				Obj = GetEventObject(Action.Parameters[0], User, Obj1, Obj2, Temp);
				UnpackAbsoluteCoordinate(Action.Parameters[1], x, y, z);
				Con = GetMapContainer(x, y, z);
				MoveAllObjects(Obj.getNextObject(), Con, NONE, true);
				break;
			case ACTION_MOVETOPREL:
				Obj = GetEventObject(Action.Parameters[0], User, Obj1, Obj2, Temp);
				GetObjectCoordinates(Obj, x, y, z);
				UnpackRelativeCoordinate(Action.Parameters[1], dx, dy, dz);
				Con = GetMapContainer(x + dx, y + dy, z + dz);
				MoveAllObjects(Obj.getNextObject(), Con, NONE, true);
				break;
			case ACTION_MOVE:
				Obj = GetEventObject(Action.Parameters[0], User, Obj1, Obj2, Temp);
				UnpackAbsoluteCoordinate(Action.Parameters[1], x, y, z);
				Con = GetMapContainer(x, y, z);
				MoveOneObject(Obj, Con);
				break;
			case ACTION_MOVEREL:
				Obj = GetEventObject(Action.Parameters[0], User, Obj1, Obj2, Temp);
				GetObjectCoordinates(GetEventObject(Action.Parameters[1], User, Obj1, Obj2, Temp), x, y, z);
				UnpackRelativeCoordinate(Action.Parameters[2], dx, dy, dz);
				Con = GetMapContainer(x + dx, y + dy, z + dz);
				MoveOneObject(Obj, Con);
				break;
			case ACTION_RETRIEVE:
				GetObjectCoordinates(GetEventObject(Action.Parameters[0], User, Obj1, Obj2, Temp), x, y, z);
				UnpackRelativeCoordinate(Action.Parameters[1], dx, dy, dz);
				Obj = GetTopObject(x + dx, y + dy, z + dz, false);
				if (Obj == NONE || Obj.getFlag(UNMOVE)) {
					return;
				}
				UnpackRelativeCoordinate(Action.Parameters[2], dx, dy, dz);
				Con = GetMapContainer(x + dx, y + dy, z + dz);
				MoveOneObject(Obj, Con);
				break;
			case ACTION_DELETEALLONMAP:
				UnpackAbsoluteCoordinate(Action.Parameters[0], x, y, z);
				Obj = GetFirstObject(x, y, z);
				DeleteAllObjects(Obj, Obj, false);
				break;
			case ACTION_DELETETOPONMAP:
				UnpackAbsoluteCoordinate(Action.Parameters[0], x, y, z);
				Obj = GetFirstSpecObject(x, y, z, ObjectType(Action.Parameters[1]));
				if (Obj == NONE) {
					throw ERROR;
				}
				DeleteAllObjects(Obj.getNextObject(), Obj, true);
				break;
			case ACTION_DELETEONMAP:
				UnpackAbsoluteCoordinate(Action.Parameters[0], x, y, z);
				Obj = GetFirstSpecObject(x, y, z, ObjectType(Action.Parameters[1]));
				if (Obj == NONE) {
					error("ExecuteAction (DELETEONMAP): Kein Objekt %d auf [%d,%d,%d].\n", Action.Parameters[1], x, y, z);
					return;
				}
				Delete(Obj, (Event == EVENT_USE || Event == EVENT_MULTIUSE) ? 1 : -1);
				break;
			case ACTION_DELETE:
				Obj = GetEventObject(Action.Parameters[0], User, Obj1, Obj2, Temp);
				if (Obj == NONE) {
					throw ERROR;
				}
				Delete(Obj, (Event == EVENT_USE || Event == EVENT_MULTIUSE) ? 1 : -1);
				break;
			case ACTION_DELETEININVENTORY:
				Obj = GetEventObject(Action.Parameters[0], User, Obj1, Obj2, Temp);
				if (!Obj.isCreature()) {
					throw ERROR;
				}
				DeleteAtCreature(Obj.getCreatureID(), ObjectType(Action.Parameters[1]), 1, Action.Parameters[2]);
				break;
			case ACTION_DESCRIPTION:
				Obj = GetEventObject(Action.Parameters[1], User, Obj1, Obj2, Temp);
				if (Obj.isCreature()) {
					cr = GetCreature(Obj);
					if (cr != nullptr && cr->Type == PLAYER) {
						Obj = GetEventObject(Action.Parameters[0], User, Obj1, Obj2, Temp);
						SendMessage(cr->GetConnection(), TALK_INFO_MESSAGE, "%s.", GetInfo(Obj));
					}
				}
				break;
			case ACTION_LOADDEPOT:
				Obj = GetEventObject(Action.Parameters[2], User, Obj1, Obj2, Temp);
				if (!Obj.isCreature()) {
					throw ERROR;
				}
				UnpackAbsoluteCoordinate(Action.Parameters[0], x, y, z);
				Con = GetFirstSpecObject(x, y, z, ObjectType(Action.Parameters[1]));
				if (Con == NONE) {
					throw ERROR;
				}
				LoadDepotBox(Obj.getCreatureID(), Action.Parameters[3], Con);
				break;
			case ACTION_SAVEDEPOT:
				Obj = GetEventObject(Action.Parameters[2], User, Obj1, Obj2, Temp);
				if (!Obj.isCreature()) {
					throw ERROR;
				}
				UnpackAbsoluteCoordinate(Action.Parameters[0], x, y, z);
				Con = GetFirstSpecObject(x, y, z, ObjectType(Action.Parameters[1]));
				if (Con == NONE) {
					throw ERROR;
				}
				SaveDepotBox(Obj.getCreatureID(), Action.Parameters[3], Con);
				break;
			case ACTION_SENDMAIL:
				Obj = GetEventObject(Action.Parameters[0], User, Obj1, Obj2, Temp);
				SendMail(Obj);
				break;
			case ACTION_NOP:
				break;
			default:
				error("ExecuteAction: Unbekannte Aktion %d.\n", Action.Action);
				break;
		}
	} catch (RESULT r) {
		error("ExecuteAction: Exception %d (Aktion %d).\n", r, Action.Action);
	}
}

bool HandleEvent(EventType Event, Object User, Object Obj1, Object Obj2) {
	static int RecursionDepth = 0;
	if (RecursionDepth > 10) {
		error("HandleEvent: Endlosschleife im Move/Use-System vermutet.\n");
		return false;
	}

	RecursionDepth++;
	TMoveUseDatabase &DB = MoveUseDatabases[Event];
	for (int rule = 1; rule <= DB.NumberOfRules; rule++) {
		TMoveUseRule &Rule = DB.Rules(rule);
		bool fulfilled = true;
		Object Temp = NONE;
		for (int condition = Rule.FirstCondition; condition <= Rule.LastCondition; condition++) {
			TMoveUseCondition &Condition = MoveUseConditions(condition);
			if (!CheckCondition(Event, Condition, User, Obj1, Obj2, Temp)) {
				fulfilled = false;
				break;
			}
		}

		if (fulfilled) {
			for (int action = Rule.FirstAction; action <= Rule.LastAction; action++) {
				TMoveUseAction &Action = MoveUseActions(action);
				ExecuteAction(Event, Action, User, Obj1, Obj2, Temp);
			}
			RecursionDepth--;
			return true;
		}
	}

	RecursionDepth--;
	return false;
}

void UseContainer(uint32_t CreatureID, Object Con, unsigned char Window) {
	if (!Con.exists()) {
		error("UseContainer: \xdc" "bergebenes Objekt existiert nicht.\n");
		throw ERROR;
	}

	if (!Con.getFlag(CONTAINER)) {
		error("UseContainer: \xdc" "bergebenes Objekt ist kein Container.\n");
		throw ERROR;
	}

	if (Window >= MAX_OPEN_CONTAINERS) {
		error("UseContainer: Ung\xfcltige Fensternummer %d.\n", Window);
		throw ERROR;
	}

	if (Con.isCreature()) {
		throw NOTUSABLE;
	}

	TPlayer *pl = GetPlayer(CreatureID);
	if (pl == nullptr) {
		error("UseContainer: Spieler %d existiert nicht.\n", CreatureID);
		throw ERROR;
	}

	for (int con = 0; con < MAX_OPEN_CONTAINERS; con++) {
		if (pl->GetOpenContainer(con) == Con) {
			pl->SetOpenContainer(con, NONE);
			SendCloseContainer(pl->GetConnection(), con);
			return;
		}
	}

	pl->SetOpenContainer(Window, Con);
	SendContainer(pl->GetConnection(), Window);
}

void UseChest(uint32_t CreatureID, Object Chest) {
	if (!Chest.exists()) {
		error("UseChest: \xdc" "bergebenes Objekt existiert nicht.\n");
		throw ERROR;
	}

	if (!Chest.getFlag(CHEST)) {
		error("UseChest: \xdc" "bergebenes Objekt ist keine Schatztruhe.\n");
		throw ERROR;
	}

	int x, y, z;
	GetObjectCoordinates(Chest, x, y, z);
	int Number = Chest.getAttribute(CHESTQUESTNUMBER);
	if (!in(Number, 1, MAX_QUESTS - 1)) {
		error("UseChest: Ung\xfcltige Nummer %d auf Schatztruhe an Position [%d,%d,%d].\n", Number, x, y, z);
		throw ERROR;
	}

	if (CountObjectsInContainer(Chest) != 1) {
		error("UseChest: Schatztruhe auf Position [%d,%d,%d] enth\xe4lt nicht genau ein Objekt.\n", x, y, z);
		throw ERROR;
	}

	Object Treasure = GetFirstContainerObject(Chest);
	if (Treasure.getFlag(UNMOVE) || !Treasure.getFlag(TAKE)) {
		// Quirk: no coordinates are passed for the three %d.
		error("UseChest: Schatz auf Position [%d,%d,%d] ist nicht nehmbar.\n");
		throw ERROR;
	}

	TPlayer *pl = GetPlayer(CreatureID);
	if (pl == nullptr) {
		error("UseChest: Spieler %d existiert nicht.\n", CreatureID);
		throw ERROR;
	}

	if (pl->GetQuestValue(Number) != 0) {
		print(3, "Schatztruhe ist schon bekannt.\n");
		SendMessage(pl->GetConnection(), TALK_INFO_MESSAGE, "The %s is empty.", Chest.getObjectType().getName(-1));
		return;
	}

	if (!CheckRight(CreatureID, UNLIMITED_CAPACITY)) {
		int Weight = GetCompleteWeight(Treasure);
		if (Weight + GetInventoryWeight(CreatureID) > pl->Skills[SKILL_CARRYSTRENGTH]->Get() * 100 || CheckRight(CreatureID, ZERO_CAPACITY)) {
			SendMessage(pl->GetConnection(), TALK_INFO_MESSAGE, "You have found %s. Weighing %d.%02d oz %s too heavy.",
					GetName(Treasure), Weight / 100, Weight % 100,
					(Treasure.getFlag(CUMULATIVE) && Treasure.getAttribute(AMOUNT) > 1) ? "they are" : "it is");
			return;
		}
	}

	Treasure = Copy(Treasure.getContainer(), Treasure);
	bool first = true;
	int pos = 0;
	bool invert = Treasure.getFlag(MOVEMENTEVENT);
again:
	pos++;
	try {
		Object Obj = GetBodyObject(CreatureID, pos);
		if (first != invert && Obj == NONE) {
			Move(CreatureID, Treasure, GetBodyContainer(CreatureID, pos), -1, true, NONE);
		} else if (first == invert && Obj != NONE && Obj.getFlag(CONTAINER)) {
			Move(CreatureID, Treasure, Obj, -1, true, NONE);
		} else {
			throw NOROOM;
		}
	} catch (RESULT r) {
		if (r == TOOHEAVY) {
			error("UseChest: Schatz auf Position [%d,%d,%d] ist doch zu schwer.\n", x, y, z);
		}

		if (pos < LAST_BODY_POSITION) {
			goto again;
		}

		if (first) {
			first = false;
			pos = 0;
			goto again;
		}

		SendMessage(pl->GetConnection(), TALK_INFO_MESSAGE, "You have found %s, but you have no room to take %s.\n",
				GetName(Treasure), (Treasure.getFlag(CUMULATIVE) && Treasure.getAttribute(AMOUNT) > 1) ? "them" : "it");
		Delete(Treasure, -1);
		return;
	}

	SendMessage(pl->GetConnection(), TALK_INFO_MESSAGE, "You have found %s.\n", GetName(Treasure));
	pl->SetQuestValue(Number, 1);
}

void UseLiquidContainer(uint32_t CreatureID, Object Obj, Object Dest) {
	if (!Obj.exists()) {
		error("UseLiquidContainer: \xdc" "bergebenes Objekt Obj existiert nicht.\n");
		throw ERROR;
	}

	if (!Dest.exists()) {
		error("UseLiquidContainer: \xdc" "bergebenes Objekt Dest existiert nicht (Obj %d).\n", Obj.getObjectType().getTypeID());
		throw ERROR;
	}

	if (!Obj.getFlag(LIQUIDCONTAINER)) {
		error("UseLiquidContainer: \xdc" "bergebenes Objekt ist kein Fl\xfcssigkeitscontainer.\n");
		throw ERROR;
	}

	if (Dest.getFlag(LIQUIDSOURCE) && Obj.getAttribute(CONTAINERLIQUIDTYPE) == LIQUID_NONE) {
		Change(Obj, CONTAINERLIQUIDTYPE, Dest.getAttribute(SOURCELIQUIDTYPE));
		return;
	}

	if (Dest.getFlag(LIQUIDCONTAINER) && Obj.getAttribute(CONTAINERLIQUIDTYPE) != LIQUID_NONE && Dest.getAttribute(CONTAINERLIQUIDTYPE) == LIQUID_NONE) {
		Change(Dest, CONTAINERLIQUIDTYPE, Obj.getAttribute(CONTAINERLIQUIDTYPE));
		Change(Obj, CONTAINERLIQUIDTYPE, LIQUID_NONE);
		return;
	}

	if (!Dest.isCreature()) {
		Object Help = GetFirstContainerObject(Dest.getContainer());
		while (Help != NONE) {
			if (Help.isCreature() && Help.getCreatureID() == CreatureID) {
				Dest = Help;
				break;
			}
			Help++;
		}
	}

	if (Dest.isCreature() && Dest.getCreatureID() == CreatureID) {
		TPlayer *pl = GetPlayer(CreatureID);
		if (pl == nullptr) {
			error("UseLiquidContainer: Spieler %d existiert nicht.\n", CreatureID);
			throw ERROR;
		}

		char *Sound;
		switch (Obj.getAttribute(CONTAINERLIQUIDTYPE)) {
			case LIQUID_NONE:
				return;
			case LIQUID_WINE:
			case LIQUID_BEER:
				Change(Obj, CONTAINERLIQUIDTYPE, LIQUID_NONE);
				Sound = "Aah...";
				if (pl->Skills[SKILL_DRUNKEN]->TimerValue() < 5) {
					pl->SetTimer(SKILL_DRUNKEN, pl->Skills[SKILL_DRUNKEN]->TimerValue() + 1, 120, 120, -1);
				}
				break;
			case LIQUID_SLIME:
				Change(Obj, CONTAINERLIQUIDTYPE, LIQUID_NONE);
				Sound = "Urgh!";
				pl->Damage(nullptr, 200, DAMAGE_POISON_CONDITION);
				break;
			case LIQUID_URINE:
				Change(Obj, CONTAINERLIQUIDTYPE, LIQUID_NONE);
				Sound = "Urgh!";
				break;
			case LIQUID_MANAFLUID:
			case LIQUID_LIFEFLUID:
				DrinkPotion(CreatureID, Obj);
				Sound = "Aaaah...";
				break;
			case LIQUID_LEMONADE:
				Change(Obj, CONTAINERLIQUIDTYPE, LIQUID_NONE);
				Sound = "Mmmh.";
				break;
			default:
				Change(Obj, CONTAINERLIQUIDTYPE, LIQUID_NONE);
				Sound = "Gulp.";
				break;
		}
		Talk(CreatureID, TALK_SAY, nullptr, Sound, false);
		return;
	}

	if (Obj.getAttribute(CONTAINERLIQUIDTYPE) == LIQUID_NONE) {
		throw NOTUSABLE;
	}

	int x, y, z;
	GetObjectCoordinates(Dest, x, y, z);
	if (!CoordinateLayable(x, y, z)) {
		throw NOROOM;
	}

	int LiquidType = Obj.getAttribute(CONTAINERLIQUIDTYPE);
	Change(Obj, CONTAINERLIQUIDTYPE, LIQUID_NONE);
	CreatePool(GetMapContainer(Dest), GetSpecialObject(BLOOD_POOL), LiquidType);
}

void UseFood(uint32_t CreatureID, Object Obj) {
	if (!Obj.exists()) {
		error("UseFood: \xdc" "bergebenes Objekt existiert nicht.\n");
		throw ERROR;
	}

	if (!Obj.getFlag(FOOD)) {
		error("UseFood: \xdc" "bergebenes Objekt ist kein Nahrungsmittel.\n");
		throw ERROR;
	}

	TPlayer *pl = GetPlayer(CreatureID);
	if (pl == nullptr) {
		error("UseFood: Kreatur %d existiert nicht.\n", CreatureID);
		throw ERROR;
	}

	TSkill *sk = pl->Skills[SKILL_EATING];
	if (sk->TimerValue() > sk->GetMax() - (int)Obj.getAttribute(NUTRITION) * 12) {
		throw FEDUP;
	}

	pl->SetTimer(SKILL_EATING, sk->TimerValue() + Obj.getAttribute(NUTRITION) * 12, 0, 0, -1);
	Delete(Obj, 1);
}

void UseTextObject(uint32_t CreatureID, Object Obj) {
	if (!Obj.exists()) {
		error("UseTextObject: \xdc" "bergebenes Objekt existiert nicht.\n");
		throw ERROR;
	}

	if (!Obj.getFlag(TEXT)) {
		error("UseTextObject: \xdc" "bergebenes Objekt ist kein Schriftst\xfc" "ck.\n");
		throw ERROR;
	}

	TPlayer *pl = GetPlayer(CreatureID);
	if (pl == nullptr) {
		error("UseTextObject: Kreatur %d existiert nicht.\n", CreatureID);
		throw ERROR;
	}

	SendEditText(pl->GetConnection(), Obj);
}

void UseAnnouncer(uint32_t CreatureID, Object Obj) {
	if (!Obj.exists()) {
		error("UseAnnouncer: \xdc" "bergebenes Objekt existiert nicht.\n");
		throw ERROR;
	}

	if (!Obj.getFlag(INFORMATION)) {
		error("UseAnnouncer: \xdc" "bergebenes Objekt liefert keine Informationen.\n");
		throw ERROR;
	}

	TPlayer *pl = GetPlayer(CreatureID);
	if (pl == nullptr) {
		error("UseAnnouncer: Kreatur %d existiert nicht.\n", CreatureID);
		throw ERROR;
	}

	int Year, Cycle, Day, Hour, Minute;
	char Help[200];
	int Blessings = 0;
	switch (Obj.getAttribute(INFORMATIONTYPE)) {
		case INFORMATION_DATE:
			GetDate(Year, Cycle, Day);
			GetTime(Hour, Minute);
			SendMessage(pl->GetConnection(), TALK_INFO_MESSAGE, "It is the %dth day of the %dth cycle in the year %d. The time is %d:%.2d.",
					Day, Cycle, Year, Hour, Minute);
			break;
		case INFORMATION_TIME:
			GetTime(Hour, Minute);
			SendMessage(pl->GetConnection(), TALK_INFO_MESSAGE, "The time is %d:%.2d.", Hour, Minute);
			break;
		case INFORMATION_BLESSINGS:
			strcpy(Help, "Received blessings:");
			if (pl->GetQuestValue(FIRST_BLESSING_QUEST) != 0) {
				strcat(Help, "\nWisdom of Solitude");
				Blessings++;
			}
			if (pl->GetQuestValue(FIRST_BLESSING_QUEST + 1) != 0) {
				strcat(Help, "\nSpark of the Phoenix");
				Blessings++;
			}
			if (pl->GetQuestValue(FIRST_BLESSING_QUEST + 2) != 0) {
				strcat(Help, "\nFire of the Suns");
				Blessings++;
			}
			if (pl->GetQuestValue(FIRST_BLESSING_QUEST + 3) != 0) {
				strcat(Help, "\nSpiritual Shielding");
				Blessings++;
			}
			if (pl->GetQuestValue(LAST_BLESSING_QUEST) != 0) {
				strcat(Help, "\nEmbrace of Tibia");
				Blessings++;
			}
			if (Blessings == 0) {
				strcpy(Help, "No blessings received.");
			}
			SendMessage(pl->GetConnection(), TALK_INFO_MESSAGE, "%s", Help);
			break;
		case INFORMATION_SPELLBOOK:
			SendEditText(pl->GetConnection(), Obj);
			break;
		default:
			error("UseAnnouncer: Ung\xfcltiger Informationstyp %d.\n", Obj.getAttribute(INFORMATIONTYPE));
			break;
	}
}

void UseKeyDoor(uint32_t, Object Key, Object Door) {
	if (!Key.exists()) {
		error("UseKeyDoor: \xdc" "bergebenes Objekt Key existiert nicht.\n");
		throw ERROR;
	}

	if (!Door.exists()) {
		error("UseKeyDoor: \xdc" "bergebenes Objekt Door existiert nicht.\n");
		throw ERROR;
	}

	if (!Key.getFlag(KEY)) {
		error("UseKeyDoor: \xdc" "bergebenes Objekt ist kein Schl\xfcssel.\n");
		throw ERROR;
	}

	if (!Door.getFlag(KEYDOOR)) {
		throw NOTUSABLE;
	}

	if (Door.getAttribute(KEYHOLENUMBER) == 0 || Key.getAttribute(KEYNUMBER) == 0
			|| Door.getAttribute(KEYHOLENUMBER) != Key.getAttribute(KEYNUMBER)) {
		throw NOKEYMATCH;
	}

	ObjectType Target = Door.getAttribute(KEYDOORTARGET);
	if (Target == 0) {
		error("UseKeyDoor: Zielt\xfcr f\xfcr T\xfcr %d nicht spezifiziert.\n", Door.getObjectType().getTypeID());
		throw ERROR;
	}

	if (Target.getFlag(UNPASS)) {
		ClearField(Door, NONE);
	}

	Change(Door, Target, 0);
}

void UseNameDoor(uint32_t CreatureID, Object Door) {
	if (!Door.exists()) {
		error("UseNameDoor: \xdc" "bergebenes Objekt Door existiert nicht.\n");
		throw ERROR;
	}

	if (!Door.getFlag(NAMEDOOR)) {
		error("UseNameDoor: \xdc" "bergebenes Objekt Door ist keine beschriftete T\xfcr.\n");
		throw ERROR;
	}

	if (!Door.getFlag(TEXT)) {
		error("UseNameDoor: \xdc" "bergebenes Objekt Door tr\xe4gt keinen Text.\n");
		throw ERROR;
	}

	TPlayer *pl = GetPlayer(CreatureID);
	if (pl == nullptr) {
		error("UseNameDoor: Spieler existiert nicht.\n");
		throw ERROR;
	}

	int x, y, z;
	GetObjectCoordinates(Door, x, y, z);
	unsigned short HouseID = GetHouseID(x, y, z);
	if (HouseID == 0) {
		error("UseNameDoor: Koordinate [%d,%d,%d] geh\xf6rt zu keinem Haus.\n", x, y, z);
		throw ERROR;
	}

	if (!IsOwner(HouseID, pl) && !CheckRight(pl->ID, OPEN_NAMEDOORS) && !MayOpenDoor(Door, pl)) {
		throw NOTACCESSIBLE;
	}

	ObjectType Target = Door.getAttribute(NAMEDOORTARGET);
	if (Target == 0) {
		error("UseNameDoor: Zielt\xfcr f\xfcr T\xfcr %d nicht spezifiziert.\n", Door.getObjectType().getTypeID());
		throw ERROR;
	}

	if (Target.getFlag(UNPASS)) {
		ClearField(Door, NONE);
	}

	Change(Door, Target, 0);
}

void UseLevelDoor(uint32_t CreatureID, Object Door) {
	if (!Door.exists() || !Door.getFlag(LEVELDOOR) || !Door.getContainer().isMapContainer()) {
		error("UseLevelDoor: \xdc" "bergebenes Objekt Door ist keine Level-T\xfcr.\n");
		throw ERROR;
	}

	TPlayer *pl = GetPlayer(CreatureID);
	if (pl == nullptr) {
		error("UseLevelDoor: Spieler existiert nicht.\n");
		throw ERROR;
	}

	if (!Door.getFlag(UNPASS)) {
		throw NOTUSABLE;
	}

	if (pl->Skills[SKILL_LEVEL]->Get() < (int)Door.getAttribute(DOORLEVEL)) {
		SendMessage(pl->GetConnection(), TALK_INFO_MESSAGE, "%s.", GetInfo(Door));
		return;
	}

	ObjectType Target = Door.getAttribute(LEVELDOORTARGET);
	if (Target == 0 || Target.getFlag(UNPASS)) {
		error("UseLevelDoor: Zielt\xfcr f\xfcr T\xfcr %d nicht spezifiziert oder nicht passierbar.\n", Door.getObjectType().getTypeID());
		throw ERROR;
	}

	Change(Door, Target, 0);
	Move(0, pl->GetCreatureObject(), Door.getContainer(), -1, false, NONE);
}

void UseQuestDoor(uint32_t CreatureID, Object Door) {
	if (!Door.exists() || !Door.getFlag(QUESTDOOR) || !Door.getContainer().isMapContainer()) {
		error("UseQuestDoor: \xdc" "bergebenes Objekt Door ist keine Quest-T\xfcr.\n");
		throw ERROR;
	}

	TPlayer *pl = GetPlayer(CreatureID);
	if (pl == nullptr) {
		error("UseQuestDoor: Spieler existiert nicht.\n");
		throw ERROR;
	}

	if (!Door.getFlag(UNPASS)) {
		throw NOTUSABLE;
	}

	if (pl->GetQuestValue(Door.getAttribute(DOORQUESTNUMBER)) != (int)Door.getAttribute(DOORQUESTVALUE)) {
		SendMessage(pl->GetConnection(), TALK_INFO_MESSAGE, "%s.", GetInfo(Door));
		return;
	}

	ObjectType Target = Door.getAttribute(QUESTDOORTARGET);
	if (Target == 0 || Target.getFlag(UNPASS)) {
		error("UseQuestDoor: Zielt\xfcr f\xfcr T\xfcr %d nicht spezifiziert oder nicht passierbar.\n", Door.getObjectType().getTypeID());
		throw ERROR;
	}

	Change(Door, Target, 0);
	Move(0, pl->GetCreatureObject(), Door.getContainer(), -1, false, NONE);
}

void UseWeapon(uint32_t CreatureID, Object Weapon, Object Target) {
	if (!Weapon.exists() || !Weapon.getFlag(WEAPON) || !in(Weapon.getAttribute(WEAPONTYPE), WEAPON_SWORD, WEAPON_AXE)) {
		error("UseWeapon: \xdc" "bergebene Waffe existiert nicht oder ist keine Waffe.\n");
		throw ERROR;
	}

	if (!Target.exists() || !Target.getFlag(DESTROY)) {
		error("UseWeapon: \xdc" "bergebenes Ziel existiert nicht oder ist nicht zerst\xf6rbar.\n");
		throw ERROR;
	}

	TPlayer *pl = GetPlayer(CreatureID);
	if (pl == nullptr) {
		error("UseWeapon: Spieler %d existiert nicht.\n", CreatureID);
		throw ERROR;
	}

	if (!Target.getContainer().isMapContainer()) {
		throw NOTACCESSIBLE;
	}

	GraphicalEffect(Target, EFFECT_POFF);
	if (random(1, 3) != 1) {
		return;
	}

	ObjectType Dest = Target.getAttribute(DESTROYTARGET);
	if (Target.getFlag(CONTAINER)) {
		Empty(Target, (Dest == 0 || !Dest.getFlag(CONTAINER)) ? 0 : (int)Dest.getAttribute(CAPACITY));
	}

	Change(Target, Dest, 0);
}

void UseChangeObject(uint32_t CreatureID, Object Obj) {
	if (!Obj.exists() || !Obj.getFlag(CHANGEUSE)) {
		error("UseChangeObject: Objekt existiert nicht oder ist kein CHANGEUSE-Objekt.\n");
		throw ERROR;
	}

	TPlayer *pl = GetPlayer(CreatureID);
	if (pl == nullptr) {
		error("UseChangeObject: Spieler %d existiert nicht.\n", CreatureID);
		throw ERROR;
	}

	ObjectType NewType = Obj.getAttribute(CHANGETARGET);
	if (!Obj.getFlag(UNPASS) && NewType != 0 && NewType.getFlag(UNPASS)) {
		ClearField(Obj, Obj);
	} else if (!Obj.getFlag(UNLAY) && NewType != 0 && NewType.getFlag(UNLAY)) {
		int x, y, z;
		GetObjectCoordinates(Obj, x, y, z);
		if (CoordinateLayable(x + 1, y, z)) {
			MoveAllObjects(Obj.getNextObject(), GetMapContainer(x + 1, y, z), NONE, true);
		} else if (CoordinateLayable(x, y + 1, z)) {
			MoveAllObjects(Obj.getNextObject(), GetMapContainer(x, y + 1, z), NONE, true);
		} else if (CoordinateLayable(x - 1, y, z)) {
			MoveAllObjects(Obj.getNextObject(), GetMapContainer(x - 1, y, z), NONE, true);
		} else if (CoordinateLayable(x, y - 1, z)) {
			MoveAllObjects(Obj.getNextObject(), GetMapContainer(x, y - 1, z), NONE, true);
		}
	}

	Change(Obj, NewType, 0);
}

void UseObject(uint32_t CreatureID, Object Obj) {
	if (!Obj.exists()) {
		error("UseObjects: \xdc" "bergebenes Objekt existiert nicht.\n");
		throw ERROR;
	}

	if (!Obj.getFlag(USEEVENT)) {
		error("UseObjects: \xdc" "bergebenes Objekt ist nicht benutzbar.\n");
		throw ERROR;
	}

	TCreature *cr = nullptr;
	if (CreatureID != 0) {
		cr = GetCreature(CreatureID);
	}

	Object User = NONE;
	if (cr != nullptr) {
		User = cr->GetCreatureObject();
	}

	if (!HandleEvent(EVENT_USE, User, Obj, NONE)) {
		throw NOTUSABLE;
	}
}

void UseObjects(uint32_t CreatureID, Object Obj1, Object Obj2) {
	if (!Obj1.exists()) {
		error("UseObjects: \xdc" "bergebenes Objekt Obj1 existiert nicht.\n");
		throw ERROR;
	}

	if (!Obj2.exists()) {
		error("UseObjects: \xdc" "bergebenes Objekt Obj2 existiert nicht.\n");
		throw ERROR;
	}

	if (!Obj1.getFlag(USEEVENT)) {
		error("UseObjects: \xdc" "bergebenes Objekt Obj1 ist nicht benutzbar.\n");
		throw ERROR;
	}

	TCreature *cr = nullptr;
	if (CreatureID != 0) {
		cr = GetCreature(CreatureID);
	}

	Object User = NONE;
	if (cr != nullptr) {
		User = cr->GetCreatureObject();
	}

	if (!HandleEvent(EVENT_MULTIUSE, User, Obj1, Obj2)) {
		throw NOTUSABLE;
	}
}

void MovementEvent(Object Obj, Object Start, Object Dest) {
	if (!Obj.exists()) {
		error("MovementEvent: \xdc" "bergebenes Objekt existiert nicht.\n");
		throw ERROR;
	}

	if (!Start.exists() || (!Start.getFlag(CONTAINER) && !Start.getFlag(CHEST))) {
		error("MovementEvent: \"Start\" ist kein Container.\n");
		throw ERROR;
	}

	if (!Dest.exists() || (!Dest.getFlag(CONTAINER) && !Dest.getFlag(CHEST))) {
		error("MovementEvent: \"Dest\" ist kein Container.\n");
		throw ERROR;
	}

	if (!Obj.getFlag(MOVEMENTEVENT)) {
		return;
	}

	HandleEvent(EVENT_MOVEMENT, NONE, Obj, NONE);
	if (!Obj.exists()) {
		throw DESTROYED;
	}
}

void SeparationEvent(Object Obj, Object Start) {
	if (!Obj.exists()) {
		error("SeparationEvent: \xdc" "bergebenes Objekt existiert nicht.\n");
		throw ERROR;
	}

	if (!Start.exists()) {
		error("SeparationEvent: \xdc" "bergebener Container existiert nicht.\n");
		throw ERROR;
	}

	if (!Start.isMapContainer()) {
		return;
	}

	if (Obj.getFlag(SEPARATIONEVENT)) {
		Object Obj2 = GetFirstContainerObject(Start);
		while (Obj2 != NONE && Obj2.exists()) {
			Object Help = Obj2;
			Obj2++;
			if (Help == Obj) {
				continue;
			}

			if (Obj2 == Obj) {
				Obj2++;
			}

			HandleEvent(EVENT_SEPARATION, NONE, Obj, Help);
			if (!Obj.exists()) {
				throw DESTROYED;
			}
		}
	}

	Object Obj2 = GetFirstContainerObject(Start);
	while (Obj2 != NONE && Obj2.exists()) {
		Object Help = Obj2;
		Obj2++;
		if (Help == Obj) {
			continue;
		}

		if (Obj2 == Obj) {
			Obj2++;
		}

		if (Obj.isCreature() && (Help.getFlag(LEVELDOOR) || Help.getFlag(QUESTDOOR))) {
			ObjectType Target = Help.getAttribute(Help.getFlag(LEVELDOOR) ? LEVELDOORTARGET : QUESTDOORTARGET);
			if (Target == 0 || !Target.getFlag(UNPASS)) {
				error("SeparationEvent: Zielt\xfcr f\xfcr T\xfcr %d nicht spezifiziert oder passierbar.\n", Help.getObjectType().getTypeID());
				throw ERROR;
			}

			ClearField(Help, Obj);
			Change(Help, Target, 0);
			return;
		}

		if (Help.getFlag(SEPARATIONEVENT)) {
			HandleEvent(EVENT_SEPARATION, NONE, Help, Obj);
		}

		if (!Obj.exists()) {
			throw DESTROYED;
		}
	}
}

void CollisionEvent(Object Obj, Object Dest) {
	if (!Obj.exists()) {
		error("CollisionEvent: \xdc" "bergebenes Objekt existiert nicht.\n");
		throw ERROR;
	}

	if (!Dest.exists()) {
		error("CollisionEvent: \xdc" "bergebener Container existiert nicht.\n");
		throw ERROR;
	}

	if (!Dest.isMapContainer()) {
		return;
	}

	if (Obj.getFlag(TELEPORTABSOLUTE) || Obj.getFlag(TELEPORTRELATIVE)) {
		int x, y, z;
		if (Obj.getFlag(TELEPORTABSOLUTE)) {
			UnpackAbsoluteCoordinate(Obj.getAttribute(ABSTELEPORTDESTINATION), x, y, z);
		} else {
			GetObjectCoordinates(Obj, x, y, z);
			int dx, dy, dz;
			UnpackRelativeCoordinate(Obj.getAttribute(RELTELEPORTDISPLACEMENT), dx, dy, dz);
			x += dx;
			y += dy;
			z += dz;
		}

		MoveAllObjects(Obj.getNextObject(), GetMapContainer(x, y, z), NONE, true);
		GraphicalEffect(x, y, z, Obj.getAttribute(Obj.getFlag(TELEPORTABSOLUTE) ? ABSTELEPORTEFFECT : RELTELEPORTEFFECT));
		if (!Obj.exists()) {
			throw DESTROYED;
		}
		return;
	}

	if (Obj.getFlag(COLLISIONEVENT)) {
		Object Obj2 = GetFirstContainerObject(Dest);
		while (Obj2 != NONE && Obj2.exists()) {
			Object Help = Obj2;
			Obj2++;
			if (Help == Obj) {
				continue;
			}

			if (Obj2 == Obj) {
				Obj2++;
			}

			HandleEvent(EVENT_COLLISION, NONE, Obj, Help);
			if (!Obj.exists()) {
				throw DESTROYED;
			}
		}
	}

	Object Obj2 = GetFirstContainerObject(Dest);
	while (Obj2 != NONE && Obj2.exists()) {
		Object Help = Obj2;
		Obj2++;
		if (Help == Obj) {
			continue;
		}

		if (Obj2 == Obj) {
			Obj2++;
		}

		if (Help.getFlag(TELEPORTABSOLUTE) || Help.getFlag(TELEPORTRELATIVE)) {
			int x, y, z;
			if (Help.getFlag(TELEPORTABSOLUTE)) {
				UnpackAbsoluteCoordinate(Help.getAttribute(ABSTELEPORTDESTINATION), x, y, z);
			} else {
				GetObjectCoordinates(Help, x, y, z);
				int dx, dy, dz;
				UnpackRelativeCoordinate(Help.getAttribute(RELTELEPORTDISPLACEMENT), dx, dy, dz);
				x += dx;
				y += dy;
				z += dz;
			}

			MoveOneObject(Obj, GetMapContainer(x, y, z));
			GraphicalEffect(x, y, z, Help.getAttribute(Help.getFlag(TELEPORTABSOLUTE) ? ABSTELEPORTEFFECT : RELTELEPORTEFFECT));
		} else if (Help.getFlag(COLLISIONEVENT)) {
			HandleEvent(EVENT_COLLISION, NONE, Help, Obj);
		}

		if (!Obj.exists()) {
			throw DESTROYED;
		}
	}
}

void LoadParameters(TReadScriptFile &Script, int *Parameters, int NumberOfParameters, ...) {
	if (NumberOfParameters > MAX_PARAMETERS) {
		error("LoadParameters: Zu viele Parameter (%d).\n", NumberOfParameters);
		Script.error("too many parameters");
	}

	if (NumberOfParameters == 0) {
		return;
	}

	if (Script.Token != SPECIAL || Script.getSpecial() != '(') {
		Script.error("'(' expected");
	}

	va_list va;
	char *Ident;
	int x, y, z;
	va_start(va, NumberOfParameters);
	for (int i = 0; i < NumberOfParameters; i++) {
		if (i > 0) {
			Script.readSymbol(',');
		}

		switch (va_arg(va, int)) {
			case PARAMETER_OBJECT:
				Ident = Script.readIdentifier();
				if (strcmp(Ident, "null") == 0) {
					Parameters[i] = EVENTOBJECT_NULL;
				} else if (strcmp(Ident, "obj1") == 0) {
					Parameters[i] = EVENTOBJECT_OBJ1;
				} else if (strcmp(Ident, "obj2") == 0) {
					Parameters[i] = EVENTOBJECT_OBJ2;
				} else if (strcmp(Ident, "user") == 0) {
					Parameters[i] = EVENTOBJECT_USER;
				} else if (strcmp(Ident, "temp") == 0) {
					Parameters[i] = EVENTOBJECT_TEMP;
				} else {
					Script.error("Object expected");
				}
				break;
			case PARAMETER_TYPE:
				Parameters[i] = Script.readNumber();
				if (!ObjectTypeExists(Parameters[i])) {
					Script.error("Unknown object type");
				}
				break;
			case PARAMETER_FLAG:
				Parameters[i] = GetFlagByName(Script.readIdentifier());
				if (Parameters[i] == -1) {
					Script.error("Unknown flag");
				}
				break;
			case PARAMETER_TYPEATTRIBUTE:
				Parameters[i] = GetTypeAttributeByName(Script.readIdentifier());
				if (Parameters[i] == -1) {
					Script.error("Unknown type attribute");
				}
				break;
			case PARAMETER_INSTANCEATTRIBUTE:
				Parameters[i] = GetInstanceAttributeByName(Script.readIdentifier());
				if (Parameters[i] == -1) {
					Script.error("Unknown instance attribute");
				}
				break;
			case PARAMETER_COORDINATE:
				Script.readCoordinate(x, y, z);
				Parameters[i] = PackAbsoluteCoordinate(x, y, z);
				break;
			case PARAMETER_VECTOR:
				Script.readCoordinate(x, y, z);
				Parameters[i] = PackRelativeCoordinate(x, y, z);
				break;
			case PARAMETER_RIGHT:
				Ident = Script.readIdentifier();
				if (strcmp(Ident, "premium_account") == 0) {
					Parameters[i] = PREMIUM_ACCOUNT;
				} else if (strcmp(Ident, "special_moveuse") == 0) {
					Parameters[i] = SPECIAL_MOVEUSE;
				} else {
					Script.error("Unknown right");
				}
				break;
			case PARAMETER_SKILL:
				Parameters[i] = GetSkillByName(Script.readIdentifier());
				if (Parameters[i] == -1) {
					Script.error("Unknown skill");
				}
				break;
			case PARAMETER_NUMBER:
				Parameters[i] = Script.readNumber();
				break;
			case PARAMETER_TEXT:
				Parameters[i] = AddDynamicString(Script.readString());
				break;
			case PARAMETER_COMPARISON:
				Parameters[i] = Script.readSpecial();
				if (strchr("<L=G>N", Parameters[i]) == nullptr) {
					Script.error("Unknown comparison operator");
				}
				break;
		}
	}
	va_end(va);

	Script.readSymbol(')');
	Script.nextToken();
}

void LoadCondition(TReadScriptFile &Script, TMoveUseCondition &Condition) {
	char Ident[30];
	strcpy(Ident, Script.getIdentifier());
	Script.nextToken();
	if (strcmp(Ident, "isposition") == 0) {
		Condition.Condition = CONDITION_ISPOSITION;
		LoadParameters(Script, Condition.Parameters, 2, PARAMETER_OBJECT, PARAMETER_COORDINATE);
	}
	if (strcmp(Ident, "istype") == 0) {
		Condition.Condition = CONDITION_ISTYPE;
		LoadParameters(Script, Condition.Parameters, 2, PARAMETER_OBJECT, PARAMETER_TYPE);
	}
	if (strcmp(Ident, "iscreature") == 0) {
		Condition.Condition = CONDITION_ISCREATURE;
		LoadParameters(Script, Condition.Parameters, 1, PARAMETER_OBJECT);
	}
	if (strcmp(Ident, "isplayer") == 0) {
		Condition.Condition = CONDITION_ISPLAYER;
		LoadParameters(Script, Condition.Parameters, 1, PARAMETER_OBJECT);
	}
	if (strcmp(Ident, "hasflag") == 0) {
		Condition.Condition = CONDITION_HASFLAG;
		LoadParameters(Script, Condition.Parameters, 2, PARAMETER_OBJECT, PARAMETER_FLAG);
	}
	if (strcmp(Ident, "hastypeattribute") == 0) {
		Condition.Condition = CONDITION_HASTYPEATTRIBUTE;
		LoadParameters(Script, Condition.Parameters, 4, PARAMETER_OBJECT, PARAMETER_TYPEATTRIBUTE, PARAMETER_COMPARISON, PARAMETER_NUMBER);
	}
	if (strcmp(Ident, "hasinstanceattribute") == 0) {
		Condition.Condition = CONDITION_HASINSTANCEATTRIBUTE;
		LoadParameters(Script, Condition.Parameters, 4, PARAMETER_OBJECT, PARAMETER_INSTANCEATTRIBUTE, PARAMETER_COMPARISON, PARAMETER_NUMBER);
	}
	if (strcmp(Ident, "hastext") == 0) {
		Condition.Condition = CONDITION_HASTEXT;
		LoadParameters(Script, Condition.Parameters, 2, PARAMETER_OBJECT, PARAMETER_TEXT);
	}
	if (strcmp(Ident, "ispeaceful") == 0) {
		Condition.Condition = CONDITION_ISPEACEFUL;
		LoadParameters(Script, Condition.Parameters, 1, PARAMETER_OBJECT);
	}
	if (strcmp(Ident, "maylogout") == 0) {
		Condition.Condition = CONDITION_MAYLOGOUT;
		LoadParameters(Script, Condition.Parameters, 1, PARAMETER_OBJECT);
	}
	if (strcmp(Ident, "hasprofession") == 0) {
		Condition.Condition = CONDITION_HASPROFESSION;
		LoadParameters(Script, Condition.Parameters, 2, PARAMETER_OBJECT, PARAMETER_NUMBER);
	}
	if (strcmp(Ident, "haslevel") == 0) {
		Condition.Condition = CONDITION_HASLEVEL;
		LoadParameters(Script, Condition.Parameters, 3, PARAMETER_OBJECT, PARAMETER_COMPARISON, PARAMETER_NUMBER);
	}
	if (strcmp(Ident, "hasright") == 0) {
		Condition.Condition = CONDITION_HASRIGHT;
		LoadParameters(Script, Condition.Parameters, 2, PARAMETER_OBJECT, PARAMETER_RIGHT);
	}
	if (strcmp(Ident, "hasquestvalue") == 0) {
		Condition.Condition = CONDITION_HASQUESTVALUE;
		LoadParameters(Script, Condition.Parameters, 4, PARAMETER_OBJECT, PARAMETER_NUMBER, PARAMETER_COMPARISON, PARAMETER_NUMBER);
	}
	if (strcmp(Ident, "testskill") == 0) {
		Condition.Condition = CONDITION_TESTSKILL;
		LoadParameters(Script, Condition.Parameters, 4, PARAMETER_OBJECT, PARAMETER_SKILL, PARAMETER_NUMBER, PARAMETER_NUMBER);
	}
	if (strcmp(Ident, "countobjects") == 0) {
		Condition.Condition = CONDITION_COUNTOBJECTS;
		LoadParameters(Script, Condition.Parameters, 3, PARAMETER_OBJECT, PARAMETER_COMPARISON, PARAMETER_NUMBER);
	}
	if (strcmp(Ident, "countobjectsonmap") == 0) {
		Condition.Condition = CONDITION_COUNTOBJECTSONMAP;
		LoadParameters(Script, Condition.Parameters, 3, PARAMETER_COORDINATE, PARAMETER_COMPARISON, PARAMETER_NUMBER);
	}
	if (strcmp(Ident, "isobjectthere") == 0) {
		Condition.Condition = CONDITION_ISOBJECTTHERE;
		LoadParameters(Script, Condition.Parameters, 2, PARAMETER_COORDINATE, PARAMETER_TYPE);
	}
	if (strcmp(Ident, "iscreaturethere") == 0) {
		Condition.Condition = CONDITION_ISCREATURETHERE;
		LoadParameters(Script, Condition.Parameters, 1, PARAMETER_COORDINATE);
	}
	if (strcmp(Ident, "isplayerthere") == 0) {
		Condition.Condition = CONDITION_ISPLAYERTHERE;
		LoadParameters(Script, Condition.Parameters, 1, PARAMETER_COORDINATE);
	}
	if (strcmp(Ident, "isobjectininventory") == 0) {
		Condition.Condition = CONDITION_ISOBJECTININVENTORY;
		LoadParameters(Script, Condition.Parameters, 3, PARAMETER_OBJECT, PARAMETER_TYPE, PARAMETER_NUMBER);
	}
	if (strcmp(Ident, "isprotectionzone") == 0) {
		Condition.Condition = CONDITION_ISPROTECTIONZONE;
		LoadParameters(Script, Condition.Parameters, 1, PARAMETER_OBJECT);
	}
	if (strcmp(Ident, "ishouse") == 0) {
		Condition.Condition = CONDITION_ISHOUSE;
		LoadParameters(Script, Condition.Parameters, 1, PARAMETER_OBJECT);
	}
	if (strcmp(Ident, "ishouseowner") == 0) {
		Condition.Condition = CONDITION_ISHOUSEOWNER;
		LoadParameters(Script, Condition.Parameters, 2, PARAMETER_OBJECT, PARAMETER_OBJECT);
	}
	if (strcmp(Ident, "isdressed") == 0) {
		Condition.Condition = CONDITION_ISDRESSED;
		LoadParameters(Script, Condition.Parameters, 1, PARAMETER_OBJECT);
	}
	if (strcmp(Ident, "random") == 0) {
		Condition.Condition = CONDITION_RANDOM;
		LoadParameters(Script, Condition.Parameters, 1, PARAMETER_NUMBER);
	}
}

void LoadAction(TReadScriptFile &Script, TMoveUseAction &Action) {
	char Ident[30];
	strcpy(Ident, Script.getIdentifier());
	Script.nextToken();
	if (strcmp(Ident, "createonmap") == 0) {
		Action.Action = ACTION_CREATEONMAP;
		LoadParameters(Script, Action.Parameters, 3, PARAMETER_COORDINATE, PARAMETER_TYPE, PARAMETER_NUMBER);
	}
	if (strcmp(Ident, "create") == 0) {
		Action.Action = ACTION_CREATE;
		LoadParameters(Script, Action.Parameters, 3, PARAMETER_OBJECT, PARAMETER_TYPE, PARAMETER_NUMBER);
	}
	if (strcmp(Ident, "monsteronmap") == 0) {
		Action.Action = ACTION_MONSTERONMAP;
		LoadParameters(Script, Action.Parameters, 2, PARAMETER_COORDINATE, PARAMETER_NUMBER);
	}
	if (strcmp(Ident, "monster") == 0) {
		Action.Action = ACTION_MONSTER;
		LoadParameters(Script, Action.Parameters, 2, PARAMETER_OBJECT, PARAMETER_NUMBER);
	}
	if (strcmp(Ident, "effectonmap") == 0) {
		Action.Action = ACTION_EFFECTONMAP;
		LoadParameters(Script, Action.Parameters, 2, PARAMETER_COORDINATE, PARAMETER_NUMBER);
	}
	if (strcmp(Ident, "effect") == 0) {
		Action.Action = ACTION_EFFECT;
		LoadParameters(Script, Action.Parameters, 2, PARAMETER_OBJECT, PARAMETER_NUMBER);
	}
	if (strcmp(Ident, "textonmap") == 0) {
		Action.Action = ACTION_TEXTONMAP;
		LoadParameters(Script, Action.Parameters, 3, PARAMETER_COORDINATE, PARAMETER_TEXT, PARAMETER_NUMBER);
	}
	if (strcmp(Ident, "text") == 0) {
		Action.Action = ACTION_TEXT;
		LoadParameters(Script, Action.Parameters, 3, PARAMETER_OBJECT, PARAMETER_TEXT, PARAMETER_NUMBER);
	}
	if (strcmp(Ident, "changeonmap") == 0) {
		Action.Action = ACTION_CHANGEONMAP;
		LoadParameters(Script, Action.Parameters, 4, PARAMETER_COORDINATE, PARAMETER_TYPE, PARAMETER_TYPE, PARAMETER_NUMBER);
	}
	if (strcmp(Ident, "change") == 0) {
		Action.Action = ACTION_CHANGE;
		LoadParameters(Script, Action.Parameters, 3, PARAMETER_OBJECT, PARAMETER_TYPE, PARAMETER_NUMBER);
	}
	if (strcmp(Ident, "changerel") == 0) {
		Action.Action = ACTION_CHANGEREL;
		LoadParameters(Script, Action.Parameters, 5, PARAMETER_OBJECT, PARAMETER_VECTOR, PARAMETER_TYPE, PARAMETER_TYPE, PARAMETER_NUMBER);
	}
	if (strcmp(Ident, "setattribute") == 0) {
		Action.Action = ACTION_SETATTRIBUTE;
		LoadParameters(Script, Action.Parameters, 3, PARAMETER_OBJECT, PARAMETER_INSTANCEATTRIBUTE, PARAMETER_NUMBER);
	}
	if (strcmp(Ident, "changeattribute") == 0) {
		Action.Action = ACTION_CHANGEATTRIBUTE;
		LoadParameters(Script, Action.Parameters, 3, PARAMETER_OBJECT, PARAMETER_INSTANCEATTRIBUTE, PARAMETER_NUMBER);
	}
	if (strcmp(Ident, "setquestvalue") == 0) {
		Action.Action = ACTION_SETQUESTVALUE;
		LoadParameters(Script, Action.Parameters, 3, PARAMETER_OBJECT, PARAMETER_NUMBER, PARAMETER_NUMBER);
	}
	if (strcmp(Ident, "damage") == 0) {
		Action.Action = ACTION_DAMAGE;
		LoadParameters(Script, Action.Parameters, 4, PARAMETER_OBJECT, PARAMETER_OBJECT, PARAMETER_NUMBER, PARAMETER_NUMBER);
	}
	if (strcmp(Ident, "setstart") == 0) {
		Action.Action = ACTION_SETSTART;
		LoadParameters(Script, Action.Parameters, 2, PARAMETER_OBJECT, PARAMETER_COORDINATE);
	}
	if (strcmp(Ident, "writename") == 0) {
		Action.Action = ACTION_WRITENAME;
		LoadParameters(Script, Action.Parameters, 3, PARAMETER_OBJECT, PARAMETER_TEXT, PARAMETER_OBJECT);
	}
	if (strcmp(Ident, "writetext") == 0) {
		Action.Action = ACTION_WRITETEXT;
		LoadParameters(Script, Action.Parameters, 2, PARAMETER_TEXT, PARAMETER_OBJECT);
	}
	if (strcmp(Ident, "logout") == 0) {
		Action.Action = ACTION_LOGOUT;
		LoadParameters(Script, Action.Parameters, 1, PARAMETER_OBJECT);
	}
	if (strcmp(Ident, "moveallonmap") == 0) {
		Action.Action = ACTION_MOVEALLONMAP;
		LoadParameters(Script, Action.Parameters, 2, PARAMETER_COORDINATE, PARAMETER_COORDINATE);
	}
	if (strcmp(Ident, "moveall") == 0) {
		Action.Action = ACTION_MOVEALL;
		LoadParameters(Script, Action.Parameters, 2, PARAMETER_OBJECT, PARAMETER_COORDINATE);
	}
	if (strcmp(Ident, "moveallrel") == 0) {
		Action.Action = ACTION_MOVEALLREL;
		LoadParameters(Script, Action.Parameters, 2, PARAMETER_OBJECT, PARAMETER_VECTOR);
	}
	if (strcmp(Ident, "movetoponmap") == 0) {
		Action.Action = ACTION_MOVETOPONMAP;
		LoadParameters(Script, Action.Parameters, 3, PARAMETER_COORDINATE, PARAMETER_TYPE, PARAMETER_COORDINATE);
	}
	if (strcmp(Ident, "movetop") == 0) {
		Action.Action = ACTION_MOVETOP;
		LoadParameters(Script, Action.Parameters, 2, PARAMETER_OBJECT, PARAMETER_COORDINATE);
	}
	if (strcmp(Ident, "movetoprel") == 0) {
		Action.Action = ACTION_MOVETOPREL;
		LoadParameters(Script, Action.Parameters, 2, PARAMETER_OBJECT, PARAMETER_VECTOR);
	}
	if (strcmp(Ident, "move") == 0) {
		Action.Action = ACTION_MOVE;
		LoadParameters(Script, Action.Parameters, 2, PARAMETER_OBJECT, PARAMETER_COORDINATE);
	}
	if (strcmp(Ident, "moverel") == 0) {
		Action.Action = ACTION_MOVEREL;
		LoadParameters(Script, Action.Parameters, 3, PARAMETER_OBJECT, PARAMETER_OBJECT, PARAMETER_VECTOR);
	}
	if (strcmp(Ident, "retrieve") == 0) {
		Action.Action = ACTION_RETRIEVE;
		LoadParameters(Script, Action.Parameters, 3, PARAMETER_OBJECT, PARAMETER_VECTOR, PARAMETER_VECTOR);
	}
	if (strcmp(Ident, "deleteallonmap") == 0) {
		Action.Action = ACTION_DELETEALLONMAP;
		LoadParameters(Script, Action.Parameters, 1, PARAMETER_COORDINATE);
	}
	if (strcmp(Ident, "deletetoponmap") == 0) {
		Action.Action = ACTION_DELETETOPONMAP;
		LoadParameters(Script, Action.Parameters, 2, PARAMETER_COORDINATE, PARAMETER_TYPE);
	}
	if (strcmp(Ident, "deleteonmap") == 0) {
		Action.Action = ACTION_DELETEONMAP;
		LoadParameters(Script, Action.Parameters, 2, PARAMETER_COORDINATE, PARAMETER_TYPE);
	}
	if (strcmp(Ident, "delete") == 0) {
		Action.Action = ACTION_DELETE;
		LoadParameters(Script, Action.Parameters, 1, PARAMETER_OBJECT);
	}
	if (strcmp(Ident, "deleteininventory") == 0) {
		Action.Action = ACTION_DELETEININVENTORY;
		LoadParameters(Script, Action.Parameters, 3, PARAMETER_OBJECT, PARAMETER_TYPE, PARAMETER_NUMBER);
	}
	if (strcmp(Ident, "description") == 0) {
		Action.Action = ACTION_DESCRIPTION;
		LoadParameters(Script, Action.Parameters, 2, PARAMETER_OBJECT, PARAMETER_OBJECT);
	}
	if (strcmp(Ident, "loaddepot") == 0) {
		Action.Action = ACTION_LOADDEPOT;
		LoadParameters(Script, Action.Parameters, 4, PARAMETER_COORDINATE, PARAMETER_TYPE, PARAMETER_OBJECT, PARAMETER_NUMBER);
	}
	if (strcmp(Ident, "savedepot") == 0) {
		Action.Action = ACTION_SAVEDEPOT;
		LoadParameters(Script, Action.Parameters, 4, PARAMETER_COORDINATE, PARAMETER_TYPE, PARAMETER_OBJECT, PARAMETER_NUMBER);
	}
	if (strcmp(Ident, "sendmail") == 0) {
		Action.Action = ACTION_SENDMAIL;
		LoadParameters(Script, Action.Parameters, 1, PARAMETER_OBJECT);
	}
	if (strcmp(Ident, "nop") == 0) {
		Action.Action = ACTION_NOP;
		LoadParameters(Script, Action.Parameters, 0);
	}
}

void LoadDataBase() {
	char FileName[4096];
	print(1, "Lade Move/Use-Datenbank ...\n");
	sprintf(FileName, "%s/moveuse.dat", DATAPATH);

	TReadScriptFile Script;
	Script.open(FileName);
	for (int i = 0; i < NUMBER_OF_EVENT_TYPES; i++) {
		MoveUseDatabases[i].NumberOfRules = 0;
	}

	Script.nextToken();
	while (Script.Token != ENDOFFILE) {
		char Ident[30];
		strcpy(Ident, Script.getIdentifier());
		if (strcmp(Ident, "begin") == 0) {
			Script.readString();
			Script.nextToken();
			continue;
		}

		if (strcmp(Ident, "end") == 0) {
			Script.nextToken();
			continue;
		}

		int EventType = -1;
		if (strcmp(Ident, "use") == 0) {
			EventType = EVENT_USE;
		}
		if (strcmp(Ident, "multiuse") == 0) {
			EventType = EVENT_MULTIUSE;
		}
		if (strcmp(Ident, "movement") == 0) {
			EventType = EVENT_MOVEMENT;
		}
		if (strcmp(Ident, "collision") == 0) {
			EventType = EVENT_COLLISION;
		}
		if (strcmp(Ident, "separation") == 0) {
			EventType = EVENT_SEPARATION;
		}
		if (EventType == -1) {
			Script.error("Unknown event type");
		}

		MoveUseDatabases[EventType].NumberOfRules++;
		TMoveUseRule &Rule = MoveUseDatabases[EventType].Rules(MoveUseDatabases[EventType].NumberOfRules);
		Rule.FirstCondition = NumberOfMoveUseConditions + 1;
		Script.readSymbol(',');
		while (true) {
			Script.nextToken();
			NumberOfMoveUseConditions++;
			TMoveUseCondition &Condition = MoveUseConditions(NumberOfMoveUseConditions);
			Condition.Modifier = MODIFIER_NORMAL;
			if (Script.Token == SPECIAL && Script.getSpecial() == '!') {
				Condition.Modifier = MODIFIER_INVERT;
				Script.nextToken();
			} else if (Script.Token == SPECIAL && Script.getSpecial() == '~') {
				Condition.Modifier = MODIFIER_TRUE;
				Script.nextToken();
			}

			LoadCondition(Script, Condition);
			if (Script.Token != SPECIAL || Script.getSpecial() != ',') {
				break;
			}
		}

		if (Script.Token != SPECIAL || Script.getSpecial() != 'I') {
			Script.error("'->' expected");
		}

		Rule.LastCondition = NumberOfMoveUseConditions;
		Script.nextToken();
		Rule.FirstAction = NumberOfMoveUseActions + 1;
		while (true) {
			NumberOfMoveUseActions++;
			TMoveUseAction &Action = MoveUseActions(NumberOfMoveUseActions);
			LoadAction(Script, Action);
			if (Script.Token != SPECIAL || Script.getSpecial() != ',') {
				break;
			}
			Script.nextToken();
		}

		if (Script.Token == STRING) {
			Script.nextToken();
		}
		Rule.LastAction = NumberOfMoveUseActions;
	}

	Script.close();
	print(1, "Move/Use-Datenbank mit %d/%d/%d/%d/%d Regeln gelesen.\n",
			MoveUseDatabases[EVENT_USE].NumberOfRules, MoveUseDatabases[EVENT_MULTIUSE].NumberOfRules,
			MoveUseDatabases[EVENT_MOVEMENT].NumberOfRules, MoveUseDatabases[EVENT_COLLISION].NumberOfRules,
			MoveUseDatabases[EVENT_SEPARATION].NumberOfRules);
}

void InitMoveUse() {
	NumberOfMoveUseConditions = 0;
	NumberOfMoveUseActions = 0;
	LoadDataBase();
	DelayedMails = 0;
}

void ExitMoveUse() {
	for (int i = 0; i < DelayedMails; i++) {
		error("ExitMoveUse: Paket an %ld wurde nicht zugestellt.\n", (long)(int32_t)DelayedMail(i).CharacterID);
	}
}
