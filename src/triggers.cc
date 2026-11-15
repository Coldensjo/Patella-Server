// triggers.cc: the events that run the trigger rules. LoadTriggers
// (triggersdata.cc) reads triggers.toml into one rule database per event type
// (use, multi-use, movement, collision and separation); InitTriggers hands
// each rule's conditions and actions to the trigger library
// (data/scripts/lib/triggers.lua, scripting.hh "Trigger rules") and loads the
// rules' own scripts; HandleEvent tries the rules of an event in order and
// stops at the first that holds, whose actions the library has run. Also
// here: the object functions those actions use, depot boxes, mail (a stamped
// letter or parcel goes into the addressee's open depot at once, otherwise
// into DelayedMail until SendMails adds it to the depot in the player's
// data), and the built-in uses of objects (the Use* functions).

#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <unistd.h>
#include <vector>

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
#include "scripting.hh"
#include "triggers.hh"
#include "triggersdata.hh"

struct TDelayedMail {
	uint32_t CharacterID;
	int DepotNumber;
	unsigned char *Packet;
	int PacketSize;
};

// A function with external linkage that triggers.hh does not declare: it
// takes an enum of triggersdata.hh, which triggers.hh does not include.
bool HandleEvent(EventType Event, Object User, Object Obj1, Object Obj2);

// The trigger library (LoadScript's number of "lib/triggers.lua"), -1 until
// InitTriggers loads it.
static int TriggerLibrary = -1;

// The rules' filters, by rule number (TTriggerRule::Number; entry 0 unused):
// the library's getRuleFilter, the type obj1 must have for the rule to hold,
// 0 for a rule without one (and for a rule with a script). Set at start.
static std::vector<int> RuleFilters;

vector<TDelayedMail> DelayedMail(0, 10, 10);
int DelayedMails;

// ChangeObject, LoadDepotBox, SaveDepotBox, SendMail, UseContainer, UseChest,
// UseLiquidContainer, UseFood, UseTextObject, UseAnnouncer, UseKeyDoor,
// UseNameDoor, UseLevelDoor, UseQuestDoor, UseWeapon, UseChangeObject,
// UseObject, UseObjects, MovementEvent, SeparationEvent and CollisionEvent
// throw RESULT; InitTriggers throws const char * (LoadTriggers', LoadScript's,
// RequireHandler's and its own for the trigger library's errors). None has an
// exception specification (C++17 has no dynamic ones), so an exception of
// another type passes to the caller.

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
		error("triggers::CreateObject: Exception %d, object %d, position [%d,%d,%d].\n", r, Type.getTypeID(), x, y, z);
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
		error("triggers::LoadDepotBox: Creature not found.\n");
		return;
	}

	if (!Con.exists()) {
		error("triggers::LoadDepotBox: Given container does not exist.\n");
		return;
	}

	if (!Con.getFlag(CONTAINER)) {
		error("triggers::LoadDepotBox: Given object is not a container.\n");
		return;
	}

	if (cr->Type != PLAYER) {
		error("triggers::LoadDepotBox: Creature is not a player.\n");
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
		error("triggers::SaveDepotBox: Creature not found.\n");
		return;
	}

	if (!Con.exists()) {
		error("triggers::SaveDepotBox: Given container does not exist.\n");
		return;
	}

	if (!Con.getFlag(CONTAINER)) {
		error("triggers::SaveDepotBox: Given object is not a container.\n");
		return;
	}

	if (cr->Type != PLAYER) {
		error("triggers::SaveDepotBox: Creature is not a player.\n");
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

// HandleEvent: the rules of the event in the order of triggers.toml; the
// first that holds ends the search and gives true. A rule of conditions and
// actions is checked by the library's evaluateRule(rule, event, user, obj1,
// obj2), which also runs the actions when the conditions hold (as the loops
// over CheckCondition and ExecuteAction did); a rule with a script by its
// onCheck(event, user, obj1, obj2), and onRun with the same arguments when
// that gives true. The objects are Items, nil for NONE, made once per event
// A rule with a filter T (getRuleFilter) begins with the condition
// is_type(obj1, T), and evaluateRule's first step is then Item.getId(obj1),
// that is Obj1.getObjectType().getTypeID(), compared with T; when they differ
// it gives false and does nothing else. HandleEvent makes that same call
// itself (one AccessObject, as getId made it, at the same point of the loop)
// and passes over the rule without calling Lua when the type is not T; for
// an object that no longer exists both give the type of the hash table's
// entry 0, without a log. For NONE, where AccessObject logs, it calls
// evaluateRule as before. The call is made per rule, not once per event:
// getId made it per rule, and AccessObject swaps obj1's sector in when it
// finds it swapped out (as when a condition of an earlier rule swapped in
// another sector, which swapped obj1's out).
//
// (TScriptEvent).
//
// Exceptions: CheckCondition caught nothing, so an exception of a condition
// left HandleEvent at once, without RecursionDepth--; ExecuteAction caught
// RESULT (logged "ExecuteAction: Exception <code> (action ...)" and went on
// with the next action) and nothing else, so any other exception left
// HandleEvent the same way. The library catches an action's result code as
// ExecuteAction did; whatever escapes evaluateRule, onCheck or onRun (a
// condition's result code, or ERROR for any other Lua error, logged by
// TScriptEvent) leaves HandleEvent in the same way as before, also without
// RecursionDepth--.
bool HandleEvent(EventType Event, Object User, Object Obj1, Object Obj2) {
	static int RecursionDepth = 0;
	if (RecursionDepth > 10) {
		error("HandleEvent: Endless loop suspected in the trigger rules.\n");
		return false;
	}

	RecursionDepth++;
	TTriggerDatabase &DB = TriggerDatabases[Event];
	TScriptEvent Call(EventKeys[Event], User.getObjectID(), Obj1.getObjectID(), Obj2.getObjectID());
	for (int rule = 1; rule <= DB.NumberOfRules; rule++) {
		TTriggerRule &Rule = DB.Rules(rule);
		bool fulfilled;
		if (Rule.Script >= 0) {
			int Script = TriggerScripts[static_cast<size_t>(Rule.Script)].Number;
			fulfilled = Call.check(Script, "onCheck", 0);
			if (fulfilled) {
				Call.run(Script, "onRun");
			size_t Number = static_cast<size_t>(Rule.Number);
			int Filter = Number < RuleFilters.size() ? RuleFilters[Number] : 0;
			if (Filter != 0 && Obj1 != NONE && Obj1.getObjectType().getTypeID() != Filter) {
				continue;
			}
			}
		} else {
			fulfilled = Call.check(TriggerLibrary, "evaluateRule", Rule.Number);
		}

		if (fulfilled) {
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
	RequireHandler(TriggerLibrary, "getRuleFilter", nullptr);
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


	RuleFilters.assign(TriggerRules.size() + 1, 0);
	for (size_t k = 0; k < TriggerRules.size(); k++) {
		const TTriggerRuleEntry &Entry = TriggerRules[k];
		TTriggerRule &Rule = TriggerDatabases[Entry.Event].Rules(Entry.Rule);
		if (Rule.Script >= 0) {
			continue;
		}

		std::string Where = "lib/triggers.lua: getRuleFilter of rule " + std::to_string(Rule.Number);
		TScriptCall Call(TriggerLibrary, "getRuleFilter");
		Call.addNumber(Rule.Number);
		int Filter;
		try {
			Filter = Call.runNumber();
		} catch (RESULT r) {
			const char *Message = Call.failure();
			if (Message != nullptr) {
				ThrowTriggerError(Where, Message);
			}
			ThrowTriggerError(Where, "the trigger library raised result " + std::to_string(static_cast<int>(r)));
		}
		if (Filter < 0) {
			ThrowTriggerError(Where, "gave " + std::to_string(Filter) + ", not a type or 0");
		}
		RuleFilters[static_cast<size_t>(Rule.Number)] = Filter;
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
		error("UseObject: Given object does not exist.\n");
		throw ERROR;
	}

	if (!Obj.getFlag(USEEVENT)) {
		error("UseObject: Given object is not usable.\n");
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

// The text of the start errors PrepareTriggerRules throws, which lives on
// after the throw.
static std::string TriggerError;

// Throws "<Where>: <What>" as a const char * (a start error, as the TOML
// loaders' errors).
[[noreturn]] static void ThrowTriggerError(const std::string &Where, const std::string &What) {
	TriggerError = Where + ": " + What;
	throw TriggerError.c_str();
}

// Adds the parameters of a condition or action of Signature to Call,
// converted by their kind (scripting.hh, "Trigger rules"): an object as its
// name, a coordinate or vector as a Position, a text as its dynamic string's
// number, a comparison as its text, anything else as its number.
static void AddTriggerParameters(TScriptCall &Call, const TTriggerSignature &Signature, const int *Parameters) {
	for (int i = 0; i < Signature.NumberOfParameters; i++) {
		int Value = Parameters[i];
		int x, y, z;
		switch (Signature.Parameters[i]) {
			case PARAMETER_OBJECT:
				if (Value < 0 || Value >= NUMBER_OF_EVENTOBJECTS) {
					throw "an object the trigger rules do not know";
				}
				Call.addString(EventObjectKeys[Value]);
				break;
			case PARAMETER_COORDINATE:
				UnpackAbsoluteCoordinate(static_cast<uint32_t>(Value), x, y, z);
				Call.addPosition(x, y, z);
				break;
			case PARAMETER_VECTOR:
				UnpackRelativeCoordinate(static_cast<uint32_t>(Value), x, y, z);
				Call.addPosition(x, y, z);
				break;
			case PARAMETER_TEXT:
				Call.addNumber(static_cast<uint32_t>(Value));
				break;
			case PARAMETER_COMPARISON: {
				const char *Name = nullptr;
				for (const TTriggerKey &Key : ComparisonKeys) {
					if (Key.Value == Value) {
						Name = Key.Key;
					}
				}
				if (Name == nullptr) {
					throw "a comparison the trigger rules do not know";
				}
				Call.addString(Name);
				break;
			}
			case PARAMETER_TYPE:
			case PARAMETER_FLAG:
			case PARAMETER_TYPEATTRIBUTE:
			case PARAMETER_INSTANCEATTRIBUTE:
			case PARAMETER_RIGHT:
			case PARAMETER_SKILL:
			case PARAMETER_NUMBER:
				Call.addNumber(Value);
				break;
		}
	}
}

// Runs a prepareCondition or prepareAction call; a failure is the start error
// "<Where>: <what>": Lua's message for a Lua error (logged by TScriptCall
// before, with the traceback), the result code for one the library raised.
static void RunPreparation(TScriptCall &Call, const std::string &Where) {
	try {
		Call.run();
	} catch (RESULT r) {
		const char *Message = Call.failure();
		if (Message != nullptr) {
			ThrowTriggerError(Where, Message);
		}
		ThrowTriggerError(Where, "the trigger library raised result " + std::to_string(static_cast<int>(r)));
	}
}

// Loads the trigger library and hands it every rule's conditions and actions
// in the order of triggers.toml (prepareCondition, prepareAction); loads the
// scripts the rules name and checks their handlers; then asks the library for
// every rule's filter (getRuleFilter, RuleFilters). Throws a const char * for
// what fails, with the place of the entry in triggers.toml where there is
// one.
static void PrepareTriggerRules() {
	TriggerLibrary = LoadScript("lib/triggers.lua", nullptr);
	RequireHandler(TriggerLibrary, "prepareCondition", nullptr);
	RequireHandler(TriggerLibrary, "prepareAction", nullptr);
	RequireHandler(TriggerLibrary, "evaluateRule", nullptr);

	for (size_t k = 0; k < TriggerRules.size(); k++) {
		const TTriggerRuleEntry &Entry = TriggerRules[k];
		TTriggerRule &Rule = TriggerDatabases[Entry.Event].Rules(Entry.Rule);
		if (Rule.Script >= 0) {
			TTriggerScript &Script = TriggerScripts[static_cast<size_t>(Rule.Script)];
			Script.Number = LoadScript(Script.Name.c_str(), Script.Where.c_str());
			RequireHandler(Script.Number, "onCheck", Script.Where.c_str());
			RequireHandler(Script.Number, "onRun", Script.Where.c_str());
			continue;
		}

		for (int i = Rule.FirstCondition; i <= Rule.LastCondition; i++) {
			TTriggerCondition &Condition = TriggerConditions(i);
			const TTriggerSignature &Signature = ConditionSignatures[Condition.Condition];
			TScriptCall Call(TriggerLibrary, "prepareCondition");
			Call.addNumber(Rule.Number);
			Call.addString(Signature.Key);
			Call.addBoolean(Condition.Modifier == MODIFIER_INVERT);
			Call.addBoolean(Condition.Modifier == MODIFIER_TRUE);
			AddTriggerParameters(Call, Signature, Condition.Parameters);
			RunPreparation(Call, TriggerConditionPlaces[static_cast<size_t>(i)]);
		}
		for (int i = Rule.FirstAction; i <= Rule.LastAction; i++) {
			TTriggerAction &Action = TriggerActions(i);
			const TTriggerSignature &Signature = ActionSignatures[Action.Action];
			TScriptCall Call(TriggerLibrary, "prepareAction");
			Call.addNumber(Rule.Number);
			Call.addString(Signature.Key);
			AddTriggerParameters(Call, Signature, Action.Parameters);
			RunPreparation(Call, TriggerActionPlaces[static_cast<size_t>(i)]);
		}
	}
}

void InitTriggers() {
	NumberOfTriggerConditions = 0;
	NumberOfTriggerActions = 0;
	LoadTriggers();
	PrepareTriggerRules();
	DelayedMails = 0;
}

void ExitTriggers() {
	for (int i = 0; i < DelayedMails; i++) {
		error("ExitTriggers: Parcel to %ld was not delivered.\n", (long)(int32_t)DelayedMail(i).CharacterID);
	}
}
