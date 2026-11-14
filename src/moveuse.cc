// moveuse.cc: what the move and use rules do. LoadDataBase (moveusedata.cc)
// reads moveuse.toml into one rule database per event type (use, multi-use,
// movement, collision and separation); HandleEvent runs the actions of the
// first rule whose conditions hold. Also here: the object functions those
// actions use, depot boxes, mail (a stamped letter or parcel goes into the
// addressee's open depot at once, otherwise into DelayedMail until SendMails
// adds it to the depot in the player's data), and the built-in uses of objects
// (the Use* functions).

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
#include "moveusedata.hh"

struct TDelayedMail {
	uint32_t CharacterID;
	int DepotNumber;
	unsigned char *Packet;
	int PacketSize;
};

// Functions with external linkage that moveuse.hh does not declare: they take
// an enum of moveusedata.hh, which moveuse.hh does not include.
bool CheckCondition(EventType, TMoveUseCondition &Condition, Object User, Object Obj1, Object Obj2, Object &Temp);
void ExecuteAction(EventType Event, TMoveUseAction &Action, Object User, Object Obj1, Object Obj2, Object &Temp);
bool HandleEvent(EventType Event, Object User, Object Obj1, Object Obj2);

vector<TDelayedMail> DelayedMail(0, 10, 10);
int DelayedMails;

// ChangeObject, LoadDepotBox, SaveDepotBox, SendMail, UseContainer, UseChest,
// UseLiquidContainer, UseFood, UseTextObject, UseAnnouncer, UseKeyDoor,
// UseNameDoor, UseLevelDoor, UseQuestDoor, UseWeapon, UseChangeObject,
// UseObject, UseObjects, MovementEvent, SeparationEvent and CollisionEvent
// throw RESULT; InitMoveUse throws const char * (LoadDataBase's). None has an
// exception specification (C++17 has no dynamic ones), so an exception of
// another type passes to the caller.

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
			error("GetEventObject: Unknown number %d.\n", Number);
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
			error("Compare: Invalid operator %d.\n", Operator);
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
					error("CheckCondition (TestSkill): Invalid skill number %d.\n", Condition.Parameters[1]);
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
				error("CheckCondition (IsDressed): Object is not a piece of clothing.\n");
			}
			break;
		case CONDITION_RANDOM:
			fulfilled = random(1, 100) <= Condition.Parameters[0];
			break;
		default:
			error("CheckCondition: Unknown condition %d.\n", Condition.Condition);
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
		error("moveuse::CreateObject: Exception %d, object %d, position [%d,%d,%d].\n", r, Type.getTypeID(), x, y, z);
		return NONE;
	}
}

void ChangeObject(Object Obj, ObjectType NewType, uint32_t Value) {
	if (!Obj.exists()) {
		error("ChangeObject: Given object does not exist (1, NewType=%d).\n", NewType.getTypeID());
		return;
	}

	Object Con = Obj.getContainer();
again:
	if (!Obj.exists()) {
		error("ChangeObject: Given object does not exist (2, NewType=%d).\n", NewType.getTypeID());
		return;
	}

	try {
		if (Obj.getFlag(CUMULATIVE) && Obj.getAttribute(AMOUNT) > 1) {
			Move(0, Obj, Con, Obj.getAttribute(AMOUNT) - 1, true, NONE);
		}
		Change(Obj, NewType, Value);
	} catch (RESULT r) {
		if (!Obj.exists()) {
			error("ChangeObject: Given object does not exist (3, NewType=%d).\n", NewType.getTypeID());
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
		error("MoveOneObject: Given object does not exist.\n");
		return;
	}

	if (!Con.exists() || !Con.getFlag(CONTAINER)) {
		error("MoveOneObject: \"Con\" is not a container.\n");
		return;
	}

	if (Obj.isCreature() && !Con.isMapContainer()) {
		error("MoveOneObject: \"Con\" is not a MapContainer.\n");
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
			error("MoveOneObject: Exception %d; coordinate [%d,%d,%d].\n", r, x, y, z);
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
		error("MoveAllObjects: \"Dest\" is not a Mapcontainer.\n");
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
			error("MoveAllObjects: Exception %d; object %d, target coordinate [%d,%d,%d].\n", r, Type.getTypeID(), x, y, z);
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
		error("DeleteAllObjects: A creature is to be deleted at position [%d,%d,%d].\n", x, y, z);
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
		error("ClearField: Object does not exist.\n");
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
		error("moveuse::LoadDepotBox: Creature not found.\n");
		return;
	}

	if (!Con.exists()) {
		error("moveuse::LoadDepotBox: Given container does not exist.\n");
		return;
	}

	if (!Con.getFlag(CONTAINER)) {
		error("moveuse::LoadDepotBox: Given object is not a container.\n");
		return;
	}

	if (cr->Type != PLAYER) {
		error("moveuse::LoadDepotBox: Creature is not a player.\n");
		return;
	}

	TPlayer *pl = (TPlayer *)cr;
	LoadDepot(pl->PlayerData, Nr, Con);
	pl->Depot = Con;
	pl->DepotNr = Nr;
	int Count = CountObjects(Con) - 1;
	pl->DepotSpace = GetDepotSize(Nr, CheckRight(pl->ID, PREMIUM_ACCOUNT)) - Count;
	print(3, "Depot of %s has %d free slots.\n", pl->Name, pl->DepotSpace);
	SendMessage(pl->GetConnection(), TALK_STATUS_MESSAGE, "Your depot contains %d item%s.", Count, (Count == 1 ? "" : "s"));
	if (pl->DepotSpace < 0) {
		SendMessage(pl->GetConnection(), TALK_INFO_MESSAGE, "Your depot is full. Remove surplus items before storing new ones.");
	}
}

void SaveDepotBox(uint32_t CreatureID, int Nr, Object Con) {
	TCreature *cr = GetCreature(CreatureID);
	if (cr == nullptr) {
		error("moveuse::SaveDepotBox: Creature not found.\n");
		return;
	}

	if (!Con.exists()) {
		error("moveuse::SaveDepotBox: Given container does not exist.\n");
		return;
	}

	if (!Con.getFlag(CONTAINER)) {
		error("moveuse::SaveDepotBox: Given object is not a container.\n");
		return;
	}

	if (cr->Type != PLAYER) {
		error("moveuse::SaveDepotBox: Creature is not a player.\n");
		return;
	}

	TPlayer *pl = (TPlayer *)cr;
	int Count = CountObjects(Con) - 1;
	Log("game", "Saving depot %d of %s ... depot size: %d.\n", Nr, pl->Name, Count);
	print(3, "Depot of %s: computed free slots %d, actual objects %d.\n", pl->Name, pl->DepotSpace, Count);
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
		error("SendMail: Given object does not exist.\n");
		return;
	}

	if (Obj.getObjectType() != GetSpecialObject(PARCEL_NEW) && Obj.getObjectType() != GetSpecialObject(LETTER_NEW)) {
		return;
	}

	print(3, "Sending mail...\n");
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

	Log("game", "Mail to %s in %s.\n", Addressee, Town);
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
		print(3, "Addressee is logged in and has the depot open.\n");
		try {
			Move(0, Obj, Player->Depot, -1, true, NONE);
			Change(Obj, GetSpecialObject(Obj.getObjectType() == GetSpecialObject(LETTER_NEW) ? LETTER_STAMPED : PARCEL_STAMPED), 0);
			Log("game", "Mail to %s in %s sent.\n", Addressee, Town);
			Player->DepotSpace -= CountObjects(Obj);
			print(3, "Depot of %s now has %d free slots.\n", Player->Name, Player->DepotSpace);
			SendMessage(Player->GetConnection(), TALK_INFO_MESSAGE, "New mail has arrived.");
			print(3, "Mail sent successfully.\n");
		} catch (RESULT r) {
			if (r != CONTAINERFULL) {
				error("SendMail (depot open): Exception %d.\n", r);
			}
			print(3, "Exception %d while sending mail.\n", r);
		}
		return;
	}

	print(3, "Addressee is not logged in or has the depot closed.\n");
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
		error("SendMail: Cannot write parcel (%s).\n", s);
		return;
	} catch (RESULT r) {
		error("SendMail: Cannot delete parcel (%d).\n", r);
		return;
	}
	DelayedMails++;

	Log("game", "Mail to %s in %s held back.\n", Addressee, Town);
	print(3, "Mail held back.\n");
	if (PlayerOnline) {
		SendMails(Player->PlayerData);
	} else {
		LoadCharacterOrder(CharacterID);
	}
}

void SendMails(TPlayerData *PlayerData) {
	if (PlayerData == nullptr) {
		error("SendMails: PlayerData is NULL.\n");
		return;
	}

	if (PlayerData->CharacterID == 0) {
		error("SendMails: Slot is not in use.\n");
		return;
	}

	// Locked is the ID of the thread that holds the slot: gettid(), which is
	// what getpid() returned per thread under LinuxThreads.
	if (PlayerData->Locked != gettid()) {
		error("SendMails: Slot is not locked by the GameThread.\n");
		return;
	}

	for (int i = 0; i < DelayedMails; i++) {
		TDelayedMail &D = DelayedMail(i);
		if (D.CharacterID != PlayerData->CharacterID) {
			continue;
		}

		if (!in(D.DepotNumber, 0, MAX_DEPOTS - 1)) {
			error("SendMails: Invalid depot number %d.\n", D.DepotNumber);
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
		Log("game", "Mail to %s delivered.\n", PlayerData->Name);
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
		error("TextEffect: Text does not exist.\n");
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
					error("ExecuteAction (CHANGEONMAP): No object %d at [%d,%d,%d].\n", Action.Parameters[1], x, y, z);
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
					error("ExecuteAction (CHANGEREL): No object %d at [%d,%d,%d].\n", Action.Parameters[2], x, y, z);
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
					error("ExecuteAction (SETQUESTVALUE): Object is not a creature but type %ld.\n", (long)Obj.getObjectType().getTypeID());
					return;
				}
				cr = GetCreature(Obj);
				if (cr == nullptr || cr->Type != PLAYER) {
					error("ExecuteAction (SETQUESTVALUE): Creature does not exist or is not a player.\n");
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
					error("ExecuteAction (DAMAGE): Object is not a creature but type %d.\n", Obj.getObjectType().getTypeID());
					return;
				}
				cr = GetCreature(Obj);
				if (cr == nullptr) {
					error("ExecuteAction (DAMAGE): Creature does not exist.\n");
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
				Log("game", "Setting new start position for %s.\n", cr->Name);
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
					error("ExecuteAction (MOVETOPONMAP): No object %d at [%d,%d,%d].\n", Action.Parameters[1], x, y, z);
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
					error("ExecuteAction (DELETEONMAP): No object %d at [%d,%d,%d].\n", Action.Parameters[1], x, y, z);
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
				error("ExecuteAction: Unknown action %d.\n", Action.Action);
				break;
		}
	} catch (RESULT r) {
		error("ExecuteAction: Exception %d (action %d).\n", r, Action.Action);
	}
}

bool HandleEvent(EventType Event, Object User, Object Obj1, Object Obj2) {
	static int RecursionDepth = 0;
	if (RecursionDepth > 10) {
		error("HandleEvent: Endless loop suspected in the move/use system.\n");
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
		error("UseContainer: Given object does not exist.\n");
		throw ERROR;
	}

	if (!Con.getFlag(CONTAINER)) {
		error("UseContainer: Given object is not a container.\n");
		throw ERROR;
	}

	if (Window >= MAX_OPEN_CONTAINERS) {
		error("UseContainer: Invalid window number %d.\n", Window);
		throw ERROR;
	}

	if (Con.isCreature()) {
		throw NOTUSABLE;
	}

	TPlayer *pl = GetPlayer(CreatureID);
	if (pl == nullptr) {
		error("UseContainer: Player %d does not exist.\n", CreatureID);
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
		error("UseChest: Given object does not exist.\n");
		throw ERROR;
	}

	if (!Chest.getFlag(CHEST)) {
		error("UseChest: Given object is not a treasure chest.\n");
		throw ERROR;
	}

	int x, y, z;
	GetObjectCoordinates(Chest, x, y, z);
	int Number = Chest.getAttribute(CHESTQUESTNUMBER);
	if (!in(Number, 1, MAX_QUESTS - 1)) {
		error("UseChest: Invalid number %d on treasure chest at position [%d,%d,%d].\n", Number, x, y, z);
		throw ERROR;
	}

	if (CountObjectsInContainer(Chest) != 1) {
		error("UseChest: Treasure chest at position [%d,%d,%d] does not hold exactly one object.\n", x, y, z);
		throw ERROR;
	}

	Object Treasure = GetFirstContainerObject(Chest);
	if (Treasure.getFlag(UNMOVE) || !Treasure.getFlag(TAKE)) {
		// Quirk: no coordinates are passed for the three %d.
		error("UseChest: Treasure at position [%d,%d,%d] cannot be taken.\n");
		throw ERROR;
	}

	TPlayer *pl = GetPlayer(CreatureID);
	if (pl == nullptr) {
		error("UseChest: Player %d does not exist.\n", CreatureID);
		throw ERROR;
	}

	if (pl->GetQuestValue(Number) != 0) {
		print(3, "Treasure chest is already known.\n");
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
			error("UseChest: Treasure at position [%d,%d,%d] is too heavy after all.\n", x, y, z);
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
		error("UseLiquidContainer: Given object Obj does not exist.\n");
		throw ERROR;
	}

	if (!Dest.exists()) {
		error("UseLiquidContainer: Given object Dest does not exist (Obj %d).\n", Obj.getObjectType().getTypeID());
		throw ERROR;
	}

	if (!Obj.getFlag(LIQUIDCONTAINER)) {
		error("UseLiquidContainer: Given object is not a liquid container.\n");
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
			error("UseLiquidContainer: Player %d does not exist.\n", CreatureID);
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
		error("UseFood: Given object does not exist.\n");
		throw ERROR;
	}

	if (!Obj.getFlag(FOOD)) {
		error("UseFood: Given object is not food.\n");
		throw ERROR;
	}

	TPlayer *pl = GetPlayer(CreatureID);
	if (pl == nullptr) {
		error("UseFood: Creature %d does not exist.\n", CreatureID);
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
		error("UseTextObject: Given object does not exist.\n");
		throw ERROR;
	}

	if (!Obj.getFlag(TEXT)) {
		error("UseTextObject: Given object is not a document.\n");
		throw ERROR;
	}

	TPlayer *pl = GetPlayer(CreatureID);
	if (pl == nullptr) {
		error("UseTextObject: Creature %d does not exist.\n", CreatureID);
		throw ERROR;
	}

	SendEditText(pl->GetConnection(), Obj);
}

void UseAnnouncer(uint32_t CreatureID, Object Obj) {
	if (!Obj.exists()) {
		error("UseAnnouncer: Given object does not exist.\n");
		throw ERROR;
	}

	if (!Obj.getFlag(INFORMATION)) {
		error("UseAnnouncer: Given object gives no information.\n");
		throw ERROR;
	}

	TPlayer *pl = GetPlayer(CreatureID);
	if (pl == nullptr) {
		error("UseAnnouncer: Creature %d does not exist.\n", CreatureID);
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
			error("UseAnnouncer: Invalid information type %d.\n", Obj.getAttribute(INFORMATIONTYPE));
			break;
	}
}

void UseKeyDoor(uint32_t, Object Key, Object Door) {
	if (!Key.exists()) {
		error("UseKeyDoor: Given object Key does not exist.\n");
		throw ERROR;
	}

	if (!Door.exists()) {
		error("UseKeyDoor: Given object Door does not exist.\n");
		throw ERROR;
	}

	if (!Key.getFlag(KEY)) {
		error("UseKeyDoor: Given object is not a key.\n");
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
		error("UseKeyDoor: Target door for door %d not specified.\n", Door.getObjectType().getTypeID());
		throw ERROR;
	}

	if (Target.getFlag(UNPASS)) {
		ClearField(Door, NONE);
	}

	Change(Door, Target, 0);
}

void UseNameDoor(uint32_t CreatureID, Object Door) {
	if (!Door.exists()) {
		error("UseNameDoor: Given object Door does not exist.\n");
		throw ERROR;
	}

	if (!Door.getFlag(NAMEDOOR)) {
		error("UseNameDoor: Given object Door is not a labeled door.\n");
		throw ERROR;
	}

	if (!Door.getFlag(TEXT)) {
		error("UseNameDoor: Given object Door has no text.\n");
		throw ERROR;
	}

	TPlayer *pl = GetPlayer(CreatureID);
	if (pl == nullptr) {
		error("UseNameDoor: Player does not exist.\n");
		throw ERROR;
	}

	int x, y, z;
	GetObjectCoordinates(Door, x, y, z);
	unsigned short HouseID = GetHouseID(x, y, z);
	if (HouseID == 0) {
		error("UseNameDoor: Coordinate [%d,%d,%d] belongs to no house.\n", x, y, z);
		throw ERROR;
	}

	if (!IsOwner(HouseID, pl) && !CheckRight(pl->ID, OPEN_NAMEDOORS) && !MayOpenDoor(Door, pl)) {
		throw NOTACCESSIBLE;
	}

	ObjectType Target = Door.getAttribute(NAMEDOORTARGET);
	if (Target == 0) {
		error("UseNameDoor: Target door for door %d not specified.\n", Door.getObjectType().getTypeID());
		throw ERROR;
	}

	if (Target.getFlag(UNPASS)) {
		ClearField(Door, NONE);
	}

	Change(Door, Target, 0);
}

void UseLevelDoor(uint32_t CreatureID, Object Door) {
	if (!Door.exists() || !Door.getFlag(LEVELDOOR) || !Door.getContainer().isMapContainer()) {
		error("UseLevelDoor: Given object Door is not a level door.\n");
		throw ERROR;
	}

	TPlayer *pl = GetPlayer(CreatureID);
	if (pl == nullptr) {
		error("UseLevelDoor: Player does not exist.\n");
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
		error("UseLevelDoor: Target door for door %d not specified or not passable.\n", Door.getObjectType().getTypeID());
		throw ERROR;
	}

	Change(Door, Target, 0);
	Move(0, pl->GetCreatureObject(), Door.getContainer(), -1, false, NONE);
}

void UseQuestDoor(uint32_t CreatureID, Object Door) {
	if (!Door.exists() || !Door.getFlag(QUESTDOOR) || !Door.getContainer().isMapContainer()) {
		error("UseQuestDoor: Given object Door is not a quest door.\n");
		throw ERROR;
	}

	TPlayer *pl = GetPlayer(CreatureID);
	if (pl == nullptr) {
		error("UseQuestDoor: Player does not exist.\n");
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
		error("UseQuestDoor: Target door for door %d not specified or not passable.\n", Door.getObjectType().getTypeID());
		throw ERROR;
	}

	Change(Door, Target, 0);
	Move(0, pl->GetCreatureObject(), Door.getContainer(), -1, false, NONE);
}

void UseWeapon(uint32_t CreatureID, Object Weapon, Object Target) {
	if (!Weapon.exists() || !Weapon.getFlag(WEAPON) || !in(Weapon.getAttribute(WEAPONTYPE), WEAPON_SWORD, WEAPON_AXE)) {
		error("UseWeapon: Given weapon does not exist or is not a weapon.\n");
		throw ERROR;
	}

	if (!Target.exists() || !Target.getFlag(DESTROY)) {
		error("UseWeapon: Given target does not exist or is not destructible.\n");
		throw ERROR;
	}

	TPlayer *pl = GetPlayer(CreatureID);
	if (pl == nullptr) {
		error("UseWeapon: Player %d does not exist.\n", CreatureID);
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
		error("UseChangeObject: Object does not exist or is not a CHANGEUSE object.\n");
		throw ERROR;
	}

	TPlayer *pl = GetPlayer(CreatureID);
	if (pl == nullptr) {
		error("UseChangeObject: Player %d does not exist.\n", CreatureID);
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
		error("UseObjects: Given object does not exist.\n");
		throw ERROR;
	}

	if (!Obj.getFlag(USEEVENT)) {
		error("UseObjects: Given object is not usable.\n");
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
		error("UseObjects: Given object Obj1 does not exist.\n");
		throw ERROR;
	}

	if (!Obj2.exists()) {
		error("UseObjects: Given object Obj2 does not exist.\n");
		throw ERROR;
	}

	if (!Obj1.getFlag(USEEVENT)) {
		error("UseObjects: Given object Obj1 is not usable.\n");
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
		error("MovementEvent: Given object does not exist.\n");
		throw ERROR;
	}

	if (!Start.exists() || (!Start.getFlag(CONTAINER) && !Start.getFlag(CHEST))) {
		error("MovementEvent: \"Start\" is not a container.\n");
		throw ERROR;
	}

	if (!Dest.exists() || (!Dest.getFlag(CONTAINER) && !Dest.getFlag(CHEST))) {
		error("MovementEvent: \"Dest\" is not a container.\n");
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
		error("SeparationEvent: Given object does not exist.\n");
		throw ERROR;
	}

	if (!Start.exists()) {
		error("SeparationEvent: Given container does not exist.\n");
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
				error("SeparationEvent: Target door for door %d not specified or passable.\n", Help.getObjectType().getTypeID());
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
		error("CollisionEvent: Given object does not exist.\n");
		throw ERROR;
	}

	if (!Dest.exists()) {
		error("CollisionEvent: Given container does not exist.\n");
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

void InitMoveUse() {
	NumberOfMoveUseConditions = 0;
	NumberOfMoveUseActions = 0;
	LoadDataBase();
	DelayedMails = 0;
}

void ExitMoveUse() {
	for (int i = 0; i < DelayedMails; i++) {
		error("ExitMoveUse: Parcel to %ld was not delivered.\n", (long)(int32_t)DelayedMail(i).CharacterID);
	}
}
