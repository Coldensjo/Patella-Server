// operate.cc: the operations of the game world and what they announce: the
// checks and moves of objects (Create, Move, Merge, Change, Delete, ...),
// Look, Talk, Use and the other actions of the creatures, the cron system,
// the refresh of map sectors and the map patches, the chat channels, the
// communication log and the parties.

#include <cstdarg>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#include "config.hh"
#include "script.hh"
#include "threads.hh"
#include "dbfuncs.hh"
#include "objects.hh"
#include "map.hh"
#include "info.hh"
#include "cr.hh"
#include "containr.hh"
#include "connections.hh"
#include "operate.hh"
#include "query.hh"
#include "utils.hh"
#include "time.hh"
#include "reader.hh"
#include "TXTEASymmetricKey.hh"
#include "strings.hh"
#include "crmain.hh"
#include "crplayer.hh"
#include "sending.hh"
#include "triggers.hh"
#include "magic.hh"
#include "houses.hh"
#include "scripting.hh"
#include <algorithm>
#include <ctime>
#include <dirent.h>
#include <unistd.h>
#include "mudinc.hh"

struct TChannel {
	uint32_t Moderator;
	char ModeratorName[30];
	vector<uint32_t> Subscriber;
	int Subscribers;
	vector<uint32_t> InvitedPlayer;
	int InvitedPlayers;
	// Quirk: ModeratorName stays uninitialised.
	TChannel() : Subscriber(0, 10, 10), InvitedPlayer(0, 10, 10) {
		Moderator = 0;
		Subscribers = 0;
		InvitedPlayers = 0;
	}
};

struct TStatement {
	uint32_t StatementID;
	uint32_t TimeStamp;
	uint32_t CharacterID;
	int Mode;
	int Channel;
	uint32_t Text;
	bool Reported;
};

struct TListener {
	uint32_t StatementID;
	uint32_t CharacterID;
};

struct TParty {
	uint32_t Leader;
	vector<uint32_t> Member;
	int Members;
	vector<uint32_t> InvitedPlayer;
	int InvitedPlayers;
	TParty() : Member(0, 10, 10), InvitedPlayer(0, 10, 10) {
		Leader = 0;
		Members = 0;
		InvitedPlayers = 0;
	}
};

int Channels = PUBLIC_CHANNELS;
int StatementID = 0;
int Parties = 0;
vector<TChannel> Channel(0, 18, 10);
int CurrentChannelID;
int CurrentSubscriberNumber;
fifo<TStatement> Statements(1024);
fifo<TListener> Listeners(1024);
vector<TParty> Party(0, 100, 50);

// CheckTopMoveObject through both CheckWeight overloads, Create, Copy, Move,
// Merge, both Change overloads, Delete, Empty and Look report a failure by
// throwing a RESULT. An exception of another type passes to the caller.

void AnnounceMovingCreature(uint32_t CreatureID, Object Con) {
	TCreature *cr = GetCreature(CreatureID);
	if (cr == nullptr) {
		error("AnnounceMovingCreature: Creature %d does not exist.\n", CreatureID);
		return;
	}

	int x, y, z;
	GetObjectCoordinates(Con, x, y, z);
	TFindCreatures Search(std::abs(x - cr->posx) / 2 + 17, std::abs(y - cr->posy) / 2 + 15, (x + cr->posx) / 2, (y + cr->posy) / 2, FIND_PLAYERS);
	while (true) {
		uint32_t ID = Search.getNext();
		if (ID == 0) {
			break;
		}

		TPlayer *pl = GetPlayer(ID);
		if (pl != nullptr && pl->GetConnection() != nullptr) {
			SendMoveCreature(pl->GetConnection(), CreatureID, x, y, z);
		}
	}
}

void AnnounceChangedCreature(uint32_t CreatureID, int Type) {
	TCreature *cr = GetCreature(CreatureID);
	if (cr == nullptr) {
		error("AnnounceChangedCreature: Creature %d does not exist (Type=%d).\n", CreatureID, Type);
		return;
	}

	for (TKnownCreature *KnownCreature = cr->FirstKnowingConnection; KnownCreature != nullptr; KnownCreature = KnownCreature->Next) {
		TConnection *Connection = KnownCreature->Connection;
		if (Connection->GetState() != CONNECTION_GAME || KnownCreature->State == KNOWNCREATURE_OUTDATED) {
			continue;
		}

		if (Connection->IsVisible(cr->posx, cr->posy, cr->posz)) {
			switch (Type) {
				case CREATURE_HEALTH:
					SendCreatureHealth(Connection, CreatureID);
					break;
				case CREATURE_LIGHT:
					SendCreatureLight(Connection, CreatureID);
					break;
				case CREATURE_OUTFIT:
					SendCreatureOutfit(Connection, CreatureID);
					break;
				case CREATURE_SPEED:
					SendCreatureSpeed(Connection, CreatureID);
					break;
				case CREATURE_SKULL:
					SendCreatureSkull(Connection, CreatureID);
					break;
				case CREATURE_PARTY:
					SendCreatureParty(Connection, CreatureID);
					break;
			}
		} else {
			KnownCreature->State = KNOWNCREATURE_OUTDATED;
		}
	}
}

void AnnounceChangedField(Object Obj, int Type) {
	if (!Obj.exists()) {
		error("AnnounceChangedField: Given object does not exist.\n");
		return;
	}

	int x, y, z;
	GetObjectCoordinates(Obj, x, y, z);
	TFindCreatures Search(16, 14, x, y, FIND_PLAYERS);
	while (true) {
		uint32_t ID = Search.getNext();
		if (ID == 0) {
			break;
		}

		TPlayer *pl = GetPlayer(ID);
		if (pl == nullptr || pl->GetConnection() == nullptr) {
			continue;
		}

		if (!pl->GetConnection()->IsVisible(x, y, z)) {
			continue;
		}

		switch (Type) {
			case OBJECT_DELETED:
				SendDeleteField(pl->GetConnection(), x, y, z, Obj);
				break;
			case OBJECT_CREATED:
				SendAddField(pl->GetConnection(), x, y, z, Obj);
				break;
			case OBJECT_CHANGED:
				SendChangeField(pl->GetConnection(), x, y, z, Obj);
				break;
			default:
				error("AnnounceChangedField: Invalid type %d.\n", Type);
				return;
		}
	}
}

void AnnounceChangedContainer(Object Obj, int Type) {
	if (!Obj.exists()) {
		error("AnnounceChangedContainer: Given object does not exist.\n");
		return;
	}

	TFindCreatures Search(1, 1, Obj, FIND_PLAYERS);
	while (true) {
		uint32_t ID = Search.getNext();
		if (ID == 0) {
			break;
		}

		TPlayer *pl = GetPlayer(ID);
		if (pl == nullptr || pl->GetConnection() == nullptr) {
			continue;
		}

		for (int con = 0; con < MAX_OPEN_CONTAINERS; con++) {
			if (pl->GetOpenContainer(con) != Obj.getContainer()) {
				continue;
			}

			switch (Type) {
				case OBJECT_DELETED:
					SendDeleteInContainer(pl->GetConnection(), con, Obj);
					break;
				case OBJECT_CREATED:
					SendCreateInContainer(pl->GetConnection(), con, Obj);
					break;
				case OBJECT_CHANGED:
					SendChangeInContainer(pl->GetConnection(), con, Obj);
					break;
				default:
					error("AnnounceChangedContainer: Invalid type %d.\n", Type);
					return;
			}
		}
	}
}

void AnnounceChangedInventory(Object Obj, int Type) {
	if (!Obj.exists()) {
		error("AnnounceChangedInventory: Given object does not exist.\n");
		return;
	}

	TCreature *cr = GetCreature(GetObjectCreatureID(Obj));
	if (cr == nullptr) {
		error("AnnounceChangedInventory: Object is not in any inventory.\n");
		return;
	}

	if (cr->Type != PLAYER) {
		return;
	}

	switch (Type) {
		case OBJECT_DELETED:
			SendDeleteInventory(cr->GetConnection(), GetObjectBodyPosition(Obj));
			break;
		case OBJECT_CREATED:
		case OBJECT_CHANGED:
			SendSetInventory(cr->GetConnection(), GetObjectBodyPosition(Obj), Obj);
			break;
		default:
			error("AnnounceChangedInventory: Invalid type %d.\n", Type);
			break;
	}
}

void AnnounceChangedObject(Object Obj, int Type) {
	if (!Obj.exists()) {
		error("AnnounceChangedObject: Given object does not exist.\n");
		return;
	}

	if (Obj.getContainer().isMapContainer()) {
		AnnounceChangedField(Obj, Type);
	} else if (Obj.getContainer().isBodyContainer()) {
		AnnounceChangedInventory(Obj, Type);
	} else {
		AnnounceChangedContainer(Obj, Type);
	}
}

void AnnounceGraphicalEffect(int x, int y, int z, int Type) {
	if (!IsOnMap(x, y, z)) {
		return;
	}

	TFindCreatures Search(16, 14, x, y, FIND_PLAYERS);
	while (true) {
		uint32_t ID = Search.getNext();
		if (ID == 0) {
			break;
		}

		TPlayer *pl = GetPlayer(ID);
		if (pl != nullptr && pl->GetConnection() != nullptr && pl->GetConnection()->IsVisible(x, y, z)) {
			SendGraphicalEffect(pl->GetConnection(), x, y, z, Type);
		}
	}
}

void AnnounceTextualEffect(int x, int y, int z, int Color, char *Text) {
	if (!IsOnMap(x, y, z)) {
		return;
	}

	TFindCreatures Search(16, 14, x, y, FIND_PLAYERS);
	while (true) {
		uint32_t ID = Search.getNext();
		if (ID == 0) {
			break;
		}

		TPlayer *pl = GetPlayer(ID);
		if (pl != nullptr && pl->GetConnection() != nullptr && pl->GetConnection()->IsVisible(x, y, z)) {
			SendTextualEffect(pl->GetConnection(), x, y, z, Color, Text);
		}
	}
}

void AnnounceMissile(int sx, int sy, int sz, int zx, int zy, int zz, int Type) {
	if (!IsOnMap(sx, sy, sz) || !IsOnMap(zx, zy, zz)) {
		return;
	}

	TFindCreatures Search(std::abs(sx - zx) / 2 + 17, std::abs(sy - zy) / 2 + 15, (sx + zx) / 2, (sy + zy) / 2, FIND_PLAYERS);
	while (true) {
		uint32_t ID = Search.getNext();
		if (ID == 0) {
			break;
		}

		TPlayer *pl = GetPlayer(ID);
		if (pl == nullptr || pl->GetConnection() == nullptr) {
			continue;
		}

		if (pl->GetConnection()->IsVisible(sx, sy, sz) || pl->GetConnection()->IsVisible(zx, zy, zz)) {
			SendMissileEffect(pl->GetConnection(), sx, sy, sz, zx, zy, zz, Type);
		}
	}
}

void CheckTopMoveObject(uint32_t CreatureID, Object Obj, Object Ignore) {
	if (!Obj.exists()) {
		error("CheckTopMoveObject: Object does not exist.\n");
		throw ERROR;
	}

	if (CreatureID == 0 || !IsCreaturePlayer(CreatureID)) {
		return;
	}

	Object Con = Obj.getContainer();
	if (!Con.isMapContainer()) {
		return;
	}

	if (Obj.isCreature() && Obj.getCreatureID() == CreatureID) {
		return;
	}

	Object Best = NONE;
	for (Object Help = GetFirstContainerObject(Con); Help != NONE; Help++) {
		if (Help == Ignore) {
			continue;
		}

		if (Best == NONE || (!Help.getFlag(UNMOVE) && (!Best.isCreature() || !Help.isCreature()))) {
			Best = Help;
		}

		if (!Help.getFlag(BANK) && !Help.getFlag(CLIP) && !Help.getFlag(BOTTOM) && !Help.getFlag(TOP) && !Help.isCreature()) {
			break;
		}
	}

	if (Obj != Best) {
		throw NOTACCESSIBLE;
	}
}

void CheckTopUseObject(uint32_t CreatureID, Object Obj) {
	if (!Obj.exists()) {
		error("CheckTopUseObject: Object does not exist.\n");
		throw ERROR;
	}

	if (CreatureID == 0 || !IsCreaturePlayer(CreatureID)) {
		return;
	}

	Object Con = Obj.getContainer();
	if (!Con.isMapContainer()) {
		return;
	}

	Object Best = NONE;
	for (Object Help = GetFirstContainerObject(Con); Help != NONE; Help++) {
		if (Best == NONE || (!Help.isCreature() && !Help.getFlag(LIQUIDPOOL))) {
			Best = Help;
		}

		if (Help.getFlag(FORCEUSE)) {
			break;
		}

		if (!Help.getFlag(BANK) && !Help.getFlag(CLIP) && !Help.getFlag(BOTTOM) && !Help.getFlag(TOP) && !Help.isCreature()) {
			break;
		}
	}

	if (Obj != Best) {
		throw NOTACCESSIBLE;
	}
}

void CheckTopMultiuseObject(uint32_t CreatureID, Object Obj) {
	if (!Obj.exists()) {
		error("CheckTopMultiuseObject: Object does not exist.\n");
		throw ERROR;
	}

	if (CreatureID == 0 || !IsCreaturePlayer(CreatureID)) {
		return;
	}

	Object Con = Obj.getContainer();
	if (!Con.isMapContainer()) {
		return;
	}

	Object Best = NONE;
	for (Object Help = GetFirstContainerObject(Con); Help != NONE; Help++) {
		if (Best == NONE || !Help.getFlag(LIQUIDPOOL)) {
			Best = Help;
		}

		if (Help.getFlag(FORCEUSE)) {
			break;
		}

		if (!Help.getFlag(BANK) && !Help.getFlag(CLIP) && !Help.getFlag(BOTTOM) && !Help.getFlag(TOP)) {
			break;
		}
	}

	if (Obj != Best) {
		throw NOTACCESSIBLE;
	}
}

void CheckMoveObject(uint32_t CreatureID, Object Obj, bool Take) {
	if (CreatureID == 0) {
		return;
	}

	if (!ObjectAccessible(CreatureID, Obj, 1)) {
		throw NOTACCESSIBLE;
	}

	if (Obj.getFlag(UNMOVE)) {
		throw NOTMOVABLE;
	}

	if (Obj.isCreature() && Obj.getCreatureID() != CreatureID) {
		TCreature *cr = GetCreature(Obj);
		if (cr == nullptr) {
			error("CheckMoveObject: Creature does not exist.\n");
			throw ERROR;
		}

		if (GetRaceUnpushable(cr->Race) && (WorldType != NON_PVP || !cr->IsPeaceful())) {
			throw NOTMOVABLE;
		}
	}

	if (Take && !Obj.getFlag(TAKE)) {
		throw NOTTAKABLE;
	}
}

void CheckMapDestination(uint32_t CreatureID, Object Obj, Object MapCon) {
	if (CreatureID == 0) {
		return;
	}

	int x, y, z;
	GetObjectCoordinates(MapCon, x, y, z);
	if (!Obj.isCreature()) {
		if (MovePossible(x, y, z)) {
			goto fieldOk;
		}

		if (Obj.getFlag(UNPASS)) {
			throw NOROOM;
		}

		if (CoordinateLayable(x, y, z)) {
			goto fieldOk;
		}

		if (CoordinateHookable(x, y, z) && Obj.getFlag(HANG) && !CoordinateFlag(x, y, z, HANG)) {
			goto fieldOk;
		}

		throw NOROOM;
	}

fieldOk:
	int sx, sy, sz;
	GetObjectCoordinates(Obj, sx, sy, sz);
	if (Obj.isCreature()) {
		// Quirk: the creature is not checked for nullptr.
		TCreature *cr = GetCreature(Obj);
		if (std::abs(x - cr->posx) > 1 || std::abs(y - cr->posy) > 1 || std::abs(z - cr->posz) > 1) {
			throw OUTOFRANGE;
		}

		if (z + 1 == cr->posz && GetHeight(cr->posx, cr->posy, cr->posz) < 24) {
			throw NOROOM;
		}

		if (z == cr->posz + 1 && GetHeight(x, y, z) < 24) {
			throw NOWAY;
		}

		if (cr->Type == MONSTER && z != cr->posz) {
			throw OUTOFRANGE;
		}

		if (!cr->MovePossible(x, y, z, true, z != cr->posz)) {
			throw (CreatureID == cr->ID ? MOVENOTPOSSIBLE : NOROOM);
		}

		if (CreatureID != cr->ID && CoordinateAvoid(x, y, z)) {
			throw NOROOM;
		}

		if (CreatureID != cr->ID && IsProtectionZone(sx, sy, sz) && !IsProtectionZone(x, y, z)) {
			throw PROTECTIONZONE;
		}
	} else if (!Obj.getFlag(TAKE) && !ObjectInRange(CreatureID, MapCon, 2)) {
		throw OUTOFRANGE;
	}

	if (Obj.getFlag(HANG) && CoordinateHookable(x, y, z)) {
		TCreature *cr = GetCreature(CreatureID);
		if (cr == nullptr) {
			error("CheckMapDestination: Executing creature does not exist.\n");
			throw ERROR;
		}

		if (cr->posz < z) {
			throw DOWNSTAIRS;
		}

		if (cr->posz > z) {
			throw UPSTAIRS;
		}

		if (CoordinateFlag(x, y, z, HOOKSOUTH)) {
			if (!in(cr->posy, y, y + 1) || !in(cr->posx, x - 1, x + 1)) {
				throw OUTOFRANGE;
			}
		}

		if (CoordinateFlag(x, y, z, HOOKEAST)) {
			if (!in(cr->posx, x, x + 1) || !in(cr->posy, y - 1, y + 1)) {
				throw OUTOFRANGE;
			}
		}

		return;
	}

	if (!ThrowPossible(sx, sy, sz, x, y, z, 1)) {
		throw CANNOTTHROW;
	}
}

void CheckMapPlace(uint32_t CreatureID, ObjectType Type, Object MapCon) {
	int x, y, z;
	GetObjectCoordinates(MapCon, x, y, z);
	if (!Type.getFlag(UNMOVE) && !CoordinateFlag(x, y, z, BANK)) {
		if (!Type.getFlag(HANG) || !CoordinateHookable(x, y, z)) {
			throw NOROOM;
		}
	}

	if (CreatureID != 0 && !MovePossible(x, y, z)) {
		if (Type.getFlag(UNPASS)) {
			throw NOROOM;
		}

		if (!CoordinateLayable(x, y, z)) {
			if (!CoordinateHookable(x, y, z) || !Type.getFlag(HANG)) {
				throw NOROOM;
			}
		}
	}
}

void CheckContainerDestination(Object Obj, Object Con) {
	Object Help = Con;
	while (!Help.isMapContainer()) {
		if (Help == Obj) {
			throw CROSSREFERENCE;
		}
		Help = Help.getContainer();
	}

	if (!Con.getFlag(CHEST)) {
		if (CountObjectsInContainer(Con) >= (int)Con.getAttribute(CAPACITY)) {
			throw CONTAINERFULL;
		}
	}
}

void CheckContainerPlace(ObjectType Type, Object Con, Object oldObj) {
	if (Type == GetSpecialObject(DEPOT_CHEST)) {
		return;
	}

	if (Con.getFlag(CHEST)) {
		return;
	}

	if (Type.getFlag(UNMOVE)) {
		throw NOTMOVABLE;
	}

	if (!Type.getFlag(TAKE)) {
		throw NOTTAKABLE;
	}

	if (oldObj != NONE) {
		return;
	}

	if (CountObjectsInContainer(Con) >= (int)Con.getAttribute(CAPACITY)) {
		throw CONTAINERFULL;
	}
}

void CheckDepotSpace(uint32_t CreatureID, Object Source, Object Destination, int Count) {
	if (CreatureID == 0) {
		return;
	}

	TCreature *cr = GetCreature(CreatureID);
	if (cr == nullptr) {
		error("CheckDepotSpace: Creature %lu does not exist.\n", (unsigned long)CreatureID);
		throw ERROR;
	}

	if (Destination == NONE) {
		error("CheckDepotSpace: Target does not exist.\n");
		throw ERROR;
	}

	if (cr->Type != PLAYER) {
		return;
	}

	TPlayer *pl = (TPlayer *)cr;
	if (pl->Depot == NONE || Count <= pl->DepotSpace) {
		return;
	}

	while (Source != NONE && !Source.isMapContainer()) {
		if (Source == pl->Depot) {
			return;
		}
		Source = Source.getContainer();
	}

	while (!Destination.isMapContainer()) {
		if (Destination == pl->Depot) {
			throw CONTAINERFULL;
		}
		Destination = Destination.getContainer();
	}
}

void CheckInventoryDestination(Object Obj, Object Con, bool Split) {
	int Position = GetObjectBodyPosition(Con);
	if (Position != BODY_RIGHTHAND && Position != BODY_LEFTHAND && Position != BODY_AMMO) {
		if (!Obj.getFlag(CLOTHES)) {
			throw WRONGPOSITION;
		}

		int ClothesType = Obj.getAttribute(BODYPOSITION);
		if (ClothesType == BODY_BOTHHANDS) {
			throw WRONGPOSITION2;
		}

		if (Position != ClothesType) {
			throw WRONGCLOTHES;
		}
	}

	uint32_t CreatureID = GetObjectCreatureID(Con);
	if (Obj.getFlag(CLOTHES) && Obj.getAttribute(BODYPOSITION) == BODY_BOTHHANDS && (Position == BODY_RIGHTHAND || Position == BODY_LEFTHAND)) {
		Object Help = GetBodyObject(CreatureID, BODY_RIGHTHAND);
		if (Help != NONE && Help != Obj) {
			throw HANDSNOTFREE;
		}

		Help = GetBodyObject(CreatureID, BODY_LEFTHAND);
		if (Help != NONE && Help != Obj) {
			throw HANDSNOTFREE;
		}

		return;
	}

	if (GetBodyObject(CreatureID, Position) != NONE) {
		throw NOROOM;
	}

	if (Position == BODY_RIGHTHAND || Position == BODY_LEFTHAND) {
		for (int Hand = BODY_RIGHTHAND; Hand <= BODY_LEFTHAND; Hand++) {
			Object Help = GetBodyObject(CreatureID, Hand);
			if (Help == NONE) {
				continue;
			}

			if (Help.getFlag(CLOTHES) && Help.getAttribute(BODYPOSITION) == BODY_BOTHHANDS) {
				throw HANDBLOCKED;
			}

			if ((Obj.getFlag(WEAPON) || Obj.getFlag(THROW) || Obj.getFlag(BOW) || Obj.getFlag(WAND))
					&& (Help.getFlag(WEAPON) || Help.getFlag(THROW) || Help.getFlag(BOW) || Help.getFlag(WAND))) {
				if (Split || Help != Obj) {
					throw ONEWEAPONONLY;
				}
			}
		}
	}
}

void CheckInventoryPlace(ObjectType Type, Object Con, Object OldObj) {
	int Position = GetObjectBodyPosition(Con);
	if (Position != BODY_RIGHTHAND && Position != BODY_LEFTHAND && Position != BODY_AMMO) {
		if (!Type.getFlag(CLOTHES)) {
			throw WRONGPOSITION;
		}

		int ClothesType = Type.getAttribute(BODYPOSITION);
		if (ClothesType == BODY_BOTHHANDS) {
			throw WRONGPOSITION2;
		}

		if (Position != ClothesType) {
			throw WRONGCLOTHES;
		}
	}

	uint32_t CreatureID = GetObjectCreatureID(Con);
	if (Type.getFlag(CLOTHES) && Type.getAttribute(BODYPOSITION) == BODY_BOTHHANDS && (Position == BODY_RIGHTHAND || Position == BODY_LEFTHAND)) {
		if ((GetBodyObject(CreatureID, BODY_RIGHTHAND) != NONE && GetBodyObject(CreatureID, BODY_RIGHTHAND) != OldObj)
				|| (GetBodyObject(CreatureID, BODY_LEFTHAND) != NONE && GetBodyObject(CreatureID, BODY_LEFTHAND) != OldObj)) {
			throw HANDSNOTFREE;
		}
		return;
	}

	if (GetBodyObject(CreatureID, Position) != NONE && GetBodyObject(CreatureID, Position) != OldObj) {
		throw NOROOM;
	}

	if (Position == BODY_RIGHTHAND || Position == BODY_LEFTHAND) {
		for (int Hand = BODY_RIGHTHAND; Hand <= BODY_LEFTHAND; Hand++) {
			Object Help = GetBodyObject(CreatureID, Hand);
			if (Help == NONE || Help == OldObj) {
				continue;
			}

			if (Help.getFlag(CLOTHES) && Help.getAttribute(BODYPOSITION) == BODY_BOTHHANDS) {
				throw HANDBLOCKED;
			}

			if ((Type.getFlag(WEAPON) || Type.getFlag(THROW) || Type.getFlag(BOW) || Type.getFlag(WAND))
					&& (Help.getFlag(WEAPON) || Help.getFlag(THROW) || Help.getFlag(BOW) || Help.getFlag(WAND))) {
				throw ONEWEAPONONLY;
			}
		}
	}
}

void CheckWeight(uint32_t CreatureID, Object Obj, int Count) {
	if (GetObjectCreatureID(Obj) == CreatureID) {
		return;
	}

	TCreature *cr = GetCreature(CreatureID);
	if (cr == nullptr) {
		error("CheckWeight: Creature %d does not exist.\n", CreatureID);
		throw ERROR;
	}

	if (!Obj.getFlag(TAKE)) {
		throw TOOHEAVY;
	}

	if (cr->Type == PLAYER && CheckRight(CreatureID, UNLIMITED_CAPACITY)) {
		return;
	}

	if (cr->Type == PLAYER && CheckRight(CreatureID, ZERO_CAPACITY)) {
		throw TOOHEAVY;
	}

	TSkill *sk = cr->Skills[SKILL_CARRYSTRENGTH];
	if (sk == nullptr) {
		error("CheckWeight: Skill CARRYSTRENGTH does not exist.\n");
		throw ERROR;
	}

	int Weight;
	if (Obj.getFlag(CUMULATIVE)) {
		Weight = GetWeight(Obj, Count);
	} else {
		Weight = GetCompleteWeight(Obj);
	}

	if (Weight + GetInventoryWeight(CreatureID) > sk->Get() * 100) {
		throw TOOHEAVY;
	}
}

void CheckWeight(uint32_t CreatureID, ObjectType Type, uint32_t Data, int oldWeight) {
	TCreature *cr = GetCreature(CreatureID);
	if (cr == nullptr) {
		error("CheckWeight: Creature %d does not exist.\n", CreatureID);
		throw ERROR;
	}

	if (!Type.getFlag(TAKE)) {
		throw TOOHEAVY;
	}

	if (cr->Type == PLAYER && CheckRight(CreatureID, UNLIMITED_CAPACITY)) {
		return;
	}

	if (cr->Type == PLAYER && CheckRight(CreatureID, ZERO_CAPACITY)) {
		throw TOOHEAVY;
	}

	TSkill *sk = cr->Skills[SKILL_CARRYSTRENGTH];
	if (sk == nullptr) {
		error("CheckWeight: Skill CARRYSTRENGTH does not exist.\n");
		throw ERROR;
	}

	int Weight = Type.getAttribute(WEIGHT);
	if (Type.getFlag(CUMULATIVE) && Data > 1) {
		Weight = Weight * Data;
	}

	if (Weight <= oldWeight) {
		return;
	}

	if (Weight + GetInventoryWeight(CreatureID) > oldWeight + sk->Get() * 100) {
		throw TOOHEAVY;
	}
}

void NotifyCreature(uint32_t CreatureID, Object Obj, bool Inventory) {
	if (CreatureID == 0) {
		return;
	}

	if (Obj.isCreature()) {
		return;
	}

	TCreature *cr = GetCreature(CreatureID);
	if (cr == nullptr) {
		error("NotifyCreature: Creature does not exist.\n");
		return;
	}

	if (Obj.getFlag(LIGHT) && Inventory) {
		AnnounceChangedCreature(CreatureID, CREATURE_LIGHT);
	}

	cr->NotifyChangeInventory();
}

void NotifyCreature(uint32_t CreatureID, ObjectType Type, bool Inventory) {
	if (CreatureID == 0) {
		return;
	}

	TCreature *cr = GetCreature(CreatureID);
	if (cr == nullptr) {
		error("NotifyCreature: Creature does not exist.\n");
		return;
	}

	if (Type.getFlag(LIGHT) && Inventory) {
		AnnounceChangedCreature(CreatureID, CREATURE_LIGHT);
	}

	cr->NotifyChangeInventory();
}

void NotifyAllCreatures(Object Obj, int Type, Object OldCon) {
	if (!Obj.exists()) {
		error("NotifyAllCreatures: Given object does not exist.\n");
		return;
	}

	if (!Obj.isCreature()) {
		return;
	}

	if (Type == OBJECT_MOVED) {
		if (!OldCon.exists()) {
			error("NotifyAllCreatures: Given old container does not exist.\n");
			return;
		}

		int x, y, z, oldx, oldy, oldz;
		GetObjectCoordinates(Obj, x, y, z);
		GetObjectCoordinates(OldCon, oldx, oldy, oldz);
		if (std::abs(x - oldx) > 20 || std::abs(y - oldy) > 20) {
			TFindCreatures Search1(10, 10, oldx, oldy, FIND_ALL);
			while (true) {
				uint32_t ID = Search1.getNext();
				if (ID == 0) {
					break;
				}
				GetCreature(ID)->CreatureMoveStimulus(Obj.getCreatureID(), OBJECT_CHANGED);
			}

			TFindCreatures Search2(10, 10, x, y, FIND_ALL);
			while (true) {
				uint32_t ID = Search2.getNext();
				if (ID == 0) {
					break;
				}
				GetCreature(ID)->CreatureMoveStimulus(Obj.getCreatureID(), OBJECT_CHANGED);
			}
		} else {
			TFindCreatures Search((std::abs(x - oldx) + 1) / 2 + 10, (std::abs(y - oldy) + 1) / 2 + 10, (x + oldx) / 2, (y + oldy) / 2, FIND_ALL);
			while (true) {
				uint32_t ID = Search.getNext();
				if (ID == 0) {
					break;
				}
				GetCreature(ID)->CreatureMoveStimulus(Obj.getCreatureID(), OBJECT_CHANGED);
			}
		}
	} else {
		TFindCreatures Search(10, 10, Obj, FIND_ALL);
		while (true) {
			uint32_t ID = Search.getNext();
			if (ID == 0) {
				break;
			}
			GetCreature(ID)->CreatureMoveStimulus(Obj.getCreatureID(), Type);
		}
	}
}

void NotifyTrades(Object Obj) {
	if (!Obj.exists()) {
		error("NotifyTrades: Given object does not exist.\n");
		return;
	}

	if (Obj.isCreature()) {
		return;
	}

	TFindCreatures Search(12, 10, Obj, FIND_PLAYERS);
	while (true) {
		uint32_t ID = Search.getNext();
		if (ID == 0) {
			break;
		}

		TPlayer *pl = GetPlayer(ID);
		if (pl == nullptr) {
			error("NotifyTrades: Player does not exist.\n");
			continue;
		}

		Object TradeObject = pl->InspectTrade(true, 0);
		if (TradeObject == NONE) {
			continue;
		}

		bool Affected = (TradeObject == Obj);
		if (!Affected) {
			Object Help = Obj;
			while (!Help.isMapContainer()) {
				if (TradeObject == Help) {
					Affected = true;
				}
				Help = Help.getContainer();
			}
		}

		if (!Affected) {
			Object Help = TradeObject;
			while (!Help.isMapContainer()) {
				if (Help == Obj) {
					Affected = true;
				}
				Help = Help.getContainer();
			}
		}

		if (Affected) {
			SendCloseTrade(pl->GetConnection());
			SendMessage(pl->GetConnection(), TALK_FAILURE_MESSAGE, "Trade cancelled.");
			pl->RejectTrade();
		}
	}
}

void NotifyDepot(uint32_t CreatureID, Object Obj, int Count) {
	if (CreatureID == 0) {
		return;
	}

	TCreature *cr = GetCreature(CreatureID);
	if (cr == nullptr) {
		error("NotifyDepot: Creature %lu does not exist.\n", (unsigned long)CreatureID);
		return;
	}

	if (!Obj.exists()) {
		error("NotifyDepot: Object does not exist.\n");
		throw ERROR;
	}

	if (cr->Type != PLAYER) {
		return;
	}

	TPlayer *pl = (TPlayer *)cr;
	if (pl->Depot == NONE) {
		return;
	}

	while (!Obj.isMapContainer()) {
		if (Obj == pl->Depot) {
			pl->DepotSpace += Count;
			print(3, "New depot free space for %s: %d\n", pl->Name, pl->DepotSpace);
			return;
		}
		Obj = Obj.getContainer();
	}
}

void CloseContainer(Object Con, bool Force) {
	if (!Con.exists()) {
		error("CloseContainer: Given container does not exist.\n");
		return;
	}

	if (Con.isCreature()) {
		return;
	}

	TFindCreatures Search(12, 10, Con, FIND_PLAYERS);
	while (true) {
		uint32_t ID = Search.getNext();
		if (ID == 0) {
			break;
		}

		TPlayer *pl = GetPlayer(ID);
		if (pl == nullptr) {
			error("CloseContainer: Player does not exist.\n");
			continue;
		}

		for (int con = 0; con < MAX_OPEN_CONTAINERS; con++) {
			bool Affected = false;
			Object Obj = pl->GetOpenContainer(con);
			while (Obj != NONE && !Obj.isMapContainer()) {
				if (Obj == Con) {
					Affected = true;
				}
				Obj = Obj.getContainer();
			}

			if (Affected) {
				if (!Force && ObjectAccessible(pl->ID, Con, 1)) {
					SendContainer(pl->GetConnection(), con);
				} else {
					pl->SetOpenContainer(con, NONE);
					SendCloseContainer(pl->GetConnection(), con);
				}
			}
		}
	}
}

Object Create(Object Con, ObjectType Type, uint32_t Value) {
	if (!Con.exists()) {
		error("Create: Target object does not exist.\n");
		throw ERROR;
	}

	if (!Con.getFlag(CONTAINER) && !Con.getFlag(CHEST)) {
		error("Create: Target object is not a container.\n");
		throw ERROR;
	}

	if (Type == ObjectType(0)) {
		error("Create: Object type does not exist.\n");
		throw ERROR;
	}

	if (Con.isMapContainer()) {
		CheckMapPlace(0, Type, Con);
	} else {
		if (Con.isBodyContainer()) {
			CheckInventoryPlace(Type, Con, NONE);
		} else {
			CheckContainerPlace(Type, Con, NONE);
		}

		uint32_t newCreature = GetObjectCreatureID(Con);
		if (newCreature != 0) {
			CheckWeight(newCreature, Type, Value, 0);
		}
	}

	if (Con.isMapContainer() && Type.getFlag(CUMULATIVE)) {
		if (Value == 0) {
			Value = 1;
		}

		int x, y, z;
		GetObjectCoordinates(Con, x, y, z);
		Object Top = GetTopObject(x, y, z, true);
		if (Top != NONE && Top.getObjectType() == Type && Top.getAttribute(AMOUNT) + Value <= MAX_AMOUNT) {
			Change(Top, AMOUNT, Top.getAttribute(AMOUNT) + Value);
			return Top;
		}
	}

	Object Obj = SetObject(Con, Type, Type.isCreature() ? Value : 0);
	if (Obj.getFlag(CUMULATIVE)) {
		ChangeObject(Obj, AMOUNT, Value);
	}

	if (Obj.getFlag(MAGICFIELD)) {
		ChangeObject(Obj, RESPONSIBLE, Value);
	}

	if (Obj.getFlag(LIQUIDPOOL)) {
		ChangeObject(Obj, POOLLIQUIDTYPE, Value);
	}

	if (Obj.getFlag(LIQUIDCONTAINER)) {
		ChangeObject(Obj, CONTAINERLIQUIDTYPE, Value);
	}

	if (Obj.getFlag(KEY)) {
		ChangeObject(Obj, KEYNUMBER, Value);
	}

	if (Obj.getFlag(RUNE)) {
		ChangeObject(Obj, CHARGES, Value);
	}

	if (Type.isCreature()) {
		TCreature *cr = GetCreature(Value);
		if (cr == nullptr) {
			error("Create: Invalid creature ID %d\n", Value);
			Delete(Obj, -1);
			throw ERROR;
		}

		for (int i = LAST_BODY_POSITION; i > 0; i--) {
			SetObject(Obj, ObjectType(i), 0);
		}

		cr->SetCreatureObject(Obj);
		cr->NotifyCreate();
	}

	AnnounceChangedObject(Obj, OBJECT_CREATED);
	NotifyTrades(Obj);
	NotifyCreature(GetObjectCreatureID(Obj), Obj, Obj.getContainer().isBodyContainer());
	MovementEvent(Obj, Obj.getContainer(), Obj.getContainer());
	CollisionEvent(Obj, Obj.getContainer());
	NotifyAllCreatures(Obj, OBJECT_CREATED, NONE);
	return Obj;
}

Object Copy(Object Con, Object Source) {
	if (!Con.exists()) {
		error("Copy: Target does not exist.\n");
		throw ERROR;
	}

	if (!Con.getFlag(CONTAINER) && !Con.getFlag(CHEST)) {
		error("Copy: Target is not a container.\n");
		throw ERROR;
	}

	if (!Source.exists()) {
		error("Copy: Source does not exist.\n");
		throw ERROR;
	}

	if (Source.isCreature()) {
		error("Copy: Source is a creature.\n");
		throw ERROR;
	}

	if (Con.isMapContainer()) {
		CheckMapDestination(0, Source, Con);
	} else {
		uint32_t newCreature = GetObjectCreatureID(Con);
		if (newCreature != 0) {
			CheckWeight(newCreature, Source, -1);
		}

		if (Con.isBodyContainer()) {
			CheckInventoryDestination(Source, Con, true);
		} else {
			CheckContainerDestination(Source, Con);
		}
	}

	Object Obj = CopyObject(Con, Source);
	if (Source.getFlag(CONTAINER)) {
		for (Object Help = GetFirstContainerObject(Source); Help != NONE; Help++) {
			Copy(Obj, Help);
		}
	}

	AnnounceChangedObject(Obj, OBJECT_CREATED);
	NotifyTrades(Obj);
	NotifyCreature(GetObjectCreatureID(Obj), Obj, Obj.getContainer().isBodyContainer());
	MovementEvent(Obj, Obj.getContainer(), Obj.getContainer());
	CollisionEvent(Obj, Obj.getContainer());
	NotifyAllCreatures(Obj, OBJECT_CREATED, NONE);
	return Obj;
}

void Move(uint32_t CreatureID, Object Obj, Object Con, int Count, bool NoMerge, Object Ignore) {
	if (!Obj.exists()) {
		error("Move: Given object does not exist.\n");
		throw ERROR;
	}

	if (!Con.exists()) {
		error("Move: Target object does not exist.\n");
		int x, y, z;
		GetObjectCoordinates(Obj, x, y, z);
		error("# Object %d at [%d,%d,%d]\n", Obj.getObjectType().getTypeID(), x, y, z);
		throw ERROR;
	}

	if (!Con.getFlag(CONTAINER)) {
		error("Move: Target object is not a container.\n");
		throw ERROR;
	}

	if (Obj.getFlag(CUMULATIVE) && (Count == 0 || Count > (int)Obj.getAttribute(AMOUNT))) {
		error("Move: Invalid count %d of %d pieces for cumulative object.\n", Count, Obj.getAttribute(AMOUNT));
		throw ERROR;
	}

	if (!NoMerge && Con.isMapContainer() && Obj.getFlag(CUMULATIVE) && Obj.getContainer() != Con) {
		int x, y, z;
		GetObjectCoordinates(Con, x, y, z);
		Object Top = GetTopObject(x, y, z, true);
		if (Top != NONE) {
			try {
				Merge(CreatureID, Obj, Top, Count, Ignore);
				return;
			} catch (RESULT r) {
				if (r == DESTROYED) {
					throw;
				}
			}
		}
	}

	// An object that no longer exists is only logged; Move carries on.
	if (!Obj.exists()) {
		error("Move: Given object no longer exists.\n");
	}

	if (Obj.getFlag(CUMULATIVE) && Count == -1) {
		Count = Obj.getAttribute(AMOUNT);
	}

	CheckTopMoveObject(CreatureID, Obj, Ignore);
	CheckMoveObject(CreatureID, Obj, !Con.isMapContainer());
	if (Con.isMapContainer()) {
		CheckMapDestination(CreatureID, Obj, Con);
	} else {
		bool Split = Obj.getFlag(CUMULATIVE) && Count < (int)Obj.getAttribute(AMOUNT);
		if (Con.isBodyContainer()) {
			CheckInventoryDestination(Obj, Con, Split);
		} else {
			CheckContainerDestination(Obj, Con);
		}

		if (Split) {
			CheckDepotSpace(CreatureID, NONE, Con, 1);
		} else {
			CheckDepotSpace(CreatureID, Obj, Con, CountObjects(Obj));
		}

		uint32_t newCreature = GetObjectCreatureID(Con);
		if (newCreature != 0) {
			CheckWeight(newCreature, Obj, Count);
		}
	}

	Object Rest = NONE;
	if (Obj.getFlag(CUMULATIVE) && Count < (int)Obj.getAttribute(AMOUNT)) {
		Rest = Obj;
		Obj = SplitObject(Obj, Count);
	}

	Object oldCon = Obj.getContainer();
	uint32_t oldCreature = GetObjectCreatureID(Obj);
	if (Con != oldCon) {
		SeparationEvent(Obj, oldCon);
	}

	if (!Obj.isCreature() && Rest == NONE) {
		AnnounceChangedObject(Obj, OBJECT_DELETED);
		NotifyTrades(Obj);
		NotifyDepot(CreatureID, Obj, CountObjects(Obj));
	}

	if (Obj.getFlag(CONTAINER) && CreatureID == 0) {
		CloseContainer(Obj, true);
	}

	if (Obj.isCreature()) {
		TCreature *cr = GetCreature(Obj);
		if (cr == nullptr) {
			error("Move: Creature with invalid ID %d.\n", Obj.getCreatureID());
		} else {
			cr->NotifyTurn(Con);
		}
		AnnounceMovingCreature(Obj.getCreatureID(), Con);
	}

	MoveObject(Obj, Con);
	if (!Obj.isCreature()) {
		AnnounceChangedObject(Obj, OBJECT_CREATED);
		NotifyTrades(Obj);
		NotifyDepot(CreatureID, Obj, -CountObjects(Obj));
	}

	if (Rest != NONE) {
		AnnounceChangedObject(Rest, OBJECT_CHANGED);
		NotifyTrades(Rest);
	}

	if (Obj.isCreature()) {
		TCreature *cr = GetCreature(Obj);
		if (cr == nullptr) {
			error("Move: Creature with invalid ID %d.\n", Obj.getCreatureID());
		} else {
			cr->NotifyGo();
		}
	}

	NotifyCreature(oldCreature, Obj, oldCon.isBodyContainer());
	NotifyCreature(GetObjectCreatureID(Obj), Obj, Obj.getContainer().isBodyContainer());
	if (Obj.getFlag(CONTAINER)) {
		CloseContainer(Obj, false);
	}

	MovementEvent(Obj, oldCon, Con);
	CollisionEvent(Obj, Obj.getContainer());
	NotifyAllCreatures(Obj, OBJECT_MOVED, oldCon);
}

void Merge(uint32_t CreatureID, Object Obj, Object Dest, int Count, Object Ignore) {
	if (!Obj.exists()) {
		error("Merge: Given object does not exist.\n");
		throw ERROR;
	}

	if (!Dest.exists()) {
		error("Merge: Target object does not exist.\n");
		throw ERROR;
	}

	if (Obj.getFlag(CUMULATIVE) && (Count == 0 || Count > (int)Obj.getAttribute(AMOUNT))) {
		error("Merge: Invalid count of %d pieces for cumulative object.\n", Count);
		throw ERROR;
	}

	if (Obj == Dest) {
		return;
	}

	if (Obj.getObjectType() != Dest.getObjectType()) {
		throw NOMATCH;
	}

	if (!Obj.getFlag(CUMULATIVE)) {
		throw NOTCUMULABLE;
	}

	if (Count == -1) {
		Count = Obj.getAttribute(AMOUNT);
	}

	if (Dest.getAttribute(AMOUNT) + Count > MAX_AMOUNT) {
		throw TOOMANYPARTS;
	}

	Object Con = Dest.getContainer();
	CheckTopMoveObject(CreatureID, Obj, Ignore);
	CheckMoveObject(CreatureID, Obj, !Con.isMapContainer());
	if (Con.isMapContainer()) {
		CheckMapDestination(CreatureID, Obj, Con);
	} else {
		CheckDepotSpace(CreatureID, Obj, Con, 0);
		uint32_t newCreature = GetObjectCreatureID(Con);
		if (newCreature != 0) {
			CheckWeight(newCreature, Obj, Count);
		}
	}

	Object oldCon = Obj.getContainer();
	uint32_t oldCreature = GetObjectCreatureID(Obj);
	if (Obj.getFlag(MOVEMENTEVENT)) {
		error("Merge: Movement event for cumulative object %d is not triggered.\n", Obj.getObjectType().getTypeID());
	}

	if (Dest.getContainer() != oldCon) {
		SeparationEvent(Obj, oldCon);
	}

	if (Count < (int)Obj.getAttribute(AMOUNT)) {
		ChangeObject(Obj, AMOUNT, Obj.getAttribute(AMOUNT) - Count);
		AnnounceChangedObject(Obj, OBJECT_CHANGED);
		NotifyTrades(Obj);
		ChangeObject(Dest, AMOUNT, Dest.getAttribute(AMOUNT) + Count);
	} else {
		AnnounceChangedObject(Obj, OBJECT_DELETED);
		NotifyTrades(Obj);
		NotifyDepot(CreatureID, Obj, 1);
		MergeObjects(Obj, Dest);
	}

	AnnounceChangedObject(Dest, OBJECT_CHANGED);
	NotifyTrades(Dest);
	NotifyCreature(oldCreature, Dest, oldCon.isBodyContainer());
	NotifyCreature(GetObjectCreatureID(Dest), Dest, Dest.getContainer().isBodyContainer());
	CollisionEvent(Dest, Dest.getContainer());
	NotifyAllCreatures(Dest, OBJECT_CHANGED, NONE);
}

void Change(Object Obj, ObjectType NewType, uint32_t Value) {
	if (!Obj.exists()) {
		error("Change: Given object does not exist.\n");
		throw ERROR;
	}

	if (Obj.isCreature() || NewType.isCreature()) {
		error("Change: Cannot change creatures.\n");
		throw ERROR;
	}

	if (NewType == ObjectType(0)) {
		Delete(Obj, -1);
		return;
	}

	if (Obj.getFlag(CONTAINER)) {
		if (!NewType.getFlag(CONTAINER) && GetFirstContainerObject(Obj) != NONE) {
			error("Change: Target object %d for %d is no longer a container.\n", NewType.getTypeID(), Obj.getObjectType().getTypeID());
			throw EMPTYCONTAINER;
		}

		if (NewType.getFlag(CONTAINER) && (int)NewType.getAttribute(CAPACITY) < CountObjectsInContainer(Obj)) {
			error("Change: Target object %d for %d is a smaller container.\n", NewType.getTypeID(), Obj.getObjectType().getTypeID());
			throw EMPTYCONTAINER;
		}
	}

	if (Obj.getFlag(CHEST) && !NewType.getFlag(CHEST)) {
		error("Change: Treasure chest must stay a treasure chest.\n");
		throw ERROR;
	}

	if (Obj.getFlag(CUMULATIVE) && Obj.getObjectType() != NewType && Obj.getAttribute(AMOUNT) > 1) {
		throw SPLITOBJECT;
	}

	uint32_t Creature = GetObjectCreatureID(Obj);
	if (Creature != 0 && Obj.getObjectType() != NewType) {
		if (Obj.getContainer().isBodyContainer()) {
			CheckInventoryPlace(NewType, Obj.getContainer(), Obj);
		} else {
			CheckContainerPlace(NewType, Obj.getContainer(), Obj);
		}

		CheckWeight(Creature, NewType, (Obj.getFlag(CUMULATIVE) ? Obj.getAttribute(AMOUNT) : 1), GetWeight(Obj, -1));
	}

	ObjectType OldType = Obj.getObjectType();
	if (Obj.getFlag(CONTAINER) && !NewType.getFlag(CONTAINER)) {
		CloseContainer(Obj, true);
	}

	ChangeObject(Obj, NewType);
	if (Obj.getFlag(CUMULATIVE) && !OldType.getFlag(CUMULATIVE)) {
		ChangeObject(Obj, AMOUNT, Value);
	}

	if (Obj.getFlag(MAGICFIELD) && !OldType.getFlag(MAGICFIELD)) {
		ChangeObject(Obj, RESPONSIBLE, Value);
	}

	if (Obj.getFlag(LIQUIDPOOL) && !OldType.getFlag(LIQUIDPOOL)) {
		ChangeObject(Obj, POOLLIQUIDTYPE, Value);
	}

	if (Obj.getFlag(LIQUIDCONTAINER) && !OldType.getFlag(LIQUIDCONTAINER)) {
		ChangeObject(Obj, CONTAINERLIQUIDTYPE, Value);
	}

	if (Obj.getFlag(KEY) && !OldType.getFlag(KEY)) {
		ChangeObject(Obj, KEYNUMBER, Value);
	}

	if (Obj.getFlag(RUNE) && !OldType.getFlag(RUNE)) {
		ChangeObject(Obj, CHARGES, Value);
	}

	AnnounceChangedObject(Obj, OBJECT_CHANGED);
	NotifyTrades(Obj);
	NotifyCreature(GetObjectCreatureID(Obj), Obj, Obj.getContainer().isBodyContainer());
	NotifyAllCreatures(Obj, OBJECT_CHANGED, NONE);
}

void Change(Object Obj, INSTANCEATTRIBUTE Attribute, uint32_t Value) {
	if (!Obj.exists()) {
		error("Change: Given object does not exist.\n");
		throw ERROR;
	}

	if (Attribute == CONTENT || Attribute == CHESTQUESTNUMBER || Attribute == DOORLEVEL || Attribute == DOORQUESTNUMBER
			|| Attribute == DOORQUESTVALUE || Attribute == ABSTELEPORTDESTINATION || Attribute == REMAININGEXPIRETIME) {
		error("Change: Cannot change attribute %d.\n", Attribute);
		throw ERROR;
	}

	uint32_t Creature = GetObjectCreatureID(Obj);
	if (Creature != 0 && Attribute == AMOUNT && Value > Obj.getAttribute(AMOUNT)) {
		CheckWeight(Creature, Obj.getObjectType(), Value, GetWeight(Obj, -1));
	}

	ChangeObject(Obj, Attribute, Value);
	if (Attribute == AMOUNT || Attribute == CONTAINERLIQUIDTYPE || Attribute == POOLLIQUIDTYPE) {
		AnnounceChangedObject(Obj, OBJECT_CHANGED);
	}

	NotifyTrades(Obj);
	NotifyCreature(GetObjectCreatureID(Obj), Obj, Obj.getContainer().isBodyContainer());
}

void Delete(Object Obj, int Count) {
	if (!Obj.exists()) {
		error("Delete: Given object does not exist.\n");
		throw ERROR;
	}

	if (Obj.getFlag(CUMULATIVE) && (Count == 0 || Count > (int)Obj.getAttribute(AMOUNT))) {
		error("Delete: Invalid count of %d pieces for cumulative object.\n", Count);
		throw ERROR;
	}

	Object Rest = NONE;
	if (Obj.getFlag(CUMULATIVE) && Count != -1 && Count < (int)Obj.getAttribute(AMOUNT)) {
		Rest = Obj;
		Obj = SplitObject(Obj, Count);
	} else {
		NotifyTrades(Obj);
	}

	if (Obj.getFlag(CONTAINER) || Obj.getFlag(CHEST)) {
		if (Obj.getFlag(CONTAINER)) {
			CloseContainer(Obj, true);
		}

		while (true) {
			Object Help = GetFirstContainerObject(Obj);
			if (Help == NONE) {
				break;
			}
			DeleteObject(Help);
		}
	}

	Object oldCon = Obj.getContainer();
	uint32_t oldCreature = GetObjectCreatureID(Obj);
	ObjectType oldType = Obj.getObjectType();
	SeparationEvent(Obj, oldCon);
	if (!Obj.exists()) {
		return;
	}

	if (Rest == NONE) {
		AnnounceChangedObject(Obj, OBJECT_DELETED);
	}

	NotifyAllCreatures(Obj, OBJECT_DELETED, NONE);
	if (oldType.isCreature()) {
		TCreature *cr = GetCreature(oldCreature);
		if (cr == nullptr) {
			error("Delete: Creature with invalid ID %d.\n", oldCreature);
		} else {
			cr->NotifyDelete();
		}
	}

	DeleteObject(Obj);
	if (Rest != NONE) {
		AnnounceChangedObject(Rest, OBJECT_CHANGED);
		NotifyTrades(Rest);
	}

	NotifyCreature(oldCreature, oldType, oldCon.isBodyContainer());
}

void Empty(Object Con, int Rest) {
	if (!Con.exists()) {
		error("Empty: Given container does not exist.\n");
		throw ERROR;
	}

	if (Rest < 0) {
		error("Empty: Invalid remainder %d.\n", Rest);
		throw ERROR;
	}

	if (!Con.getFlag(CONTAINER)) {
		error("Empty: Given object is not a container.\n");
		throw ERROR;
	}

	int Count = CountObjectsInContainer(Con);
	CloseContainer(Con, true);
	bool IsCorpse = Con.getFlag(CORPSE);
	while (Count > Rest) {
		Object Obj = GetFirstContainerObject(Con);
		if (IsCorpse) {
			Delete(Obj, -1);
		} else {
			Move(0, Obj, Con.getContainer(), -1, false, NONE);
		}
		Count--;
	}
}

void GraphicalEffect(int x, int y, int z, int Type) {
	AnnounceGraphicalEffect(x, y, z, Type);
}

void GraphicalEffect(Object Obj, int Type) {
	if (!Obj.exists()) {
		return;
	}

	int x, y, z;
	GetObjectCoordinates(Obj, x, y, z);
	AnnounceGraphicalEffect(x, y, z, Type);
}

void TextualEffect(Object Obj, int Color, char *Text, ...) {
	if (!Obj.exists()) {
		error("TextualEffect: Given Mapcontainer does not exist.\n");
		return;
	}

	if (!Obj.isMapContainer()) {
		Obj = GetMapContainer(Obj);
	}

	char Buffer[10];
	va_list args;
	va_start(args, Text);
	vsnprintf(Buffer, sizeof(Buffer), Text, args);
	va_end(args);
	int x, y, z;
	GetObjectCoordinates(Obj, x, y, z);
	AnnounceTextualEffect(x, y, z, Color, Buffer);
}

void Missile(Object Start, Object Dest, int Type) {
	if (!Start.exists()) {
		error("Missile: Given start point does not exist.\n");
		return;
	}

	if (!Dest.exists()) {
		error("Missile: Given target point does not exist.\n");
		return;
	}

	int sx, sy, sz, zx, zy, zz;
	GetObjectCoordinates(Start, sx, sy, sz);
	GetObjectCoordinates(Dest, zx, zy, zz);
	AnnounceMissile(sx, sy, sz, zx, zy, zz, Type);
}

void Look(uint32_t CreatureID, Object Obj) {
	TPlayer *pl = GetPlayer(CreatureID);
	if (pl == nullptr) {
		error("Look: Creature does not exist.\n");
		throw ERROR;
	}

	if (!Obj.exists()) {
		error("Look: Object does not exist.\n");
		throw ERROR;
	}

	if (Obj.isCreature()) {
		TCreature *cr = GetCreature(Obj);
		if (cr == nullptr) {
			error("Look: Object %d has no creature!\n", Obj.getObjectType().getTypeID());
			return;
		}

		if (cr->Type != PLAYER) {
			SendMessage(pl->GetConnection(), TALK_INFO_MESSAGE, "You see %s.", cr->Name);
			return;
		}

		TPlayer *Opp = (TPlayer *)cr;
		char Player[50], Vocation[50], Membership[200], Pronoun[4];
		bool Yourself = (CreatureID == Obj.getCreatureID());
		if (Yourself) {
			strcpy(Player, "yourself");
		} else {
			TSkill *sk = Opp->Skills[SKILL_LEVEL];
			snprintf(Player, sizeof(Player), "%s (Level %d)", Opp->Name, (sk != nullptr ? sk->Get() : 1));
		}

		if (Opp->Sex == SEX_FEMALE) {
			strcpy(Pronoun, "She");
		} else {
			strcpy(Pronoun, "He");
		}

		int Profession = Opp->GetActiveProfession();
		if (Yourself) {
			if (Profession != PROFESSION_NONE) {
				char HelpProfession[30];
				GetProfessionName(HelpProfession, Profession, true, false);
				snprintf(Vocation, sizeof(Vocation), "You are %s", HelpProfession);
			} else {
				strcpy(Vocation, "You have no vocation");
			}
		} else {
			if (Profession != PROFESSION_NONE) {
				char HelpProfession[30];
				GetProfessionName(HelpProfession, Profession, true, false);
				snprintf(Vocation, sizeof(Vocation), "%s is %s", Pronoun, HelpProfession);
			} else {
				snprintf(Vocation, sizeof(Vocation), "%s has no vocation", Pronoun);
			}
		}

		if (Opp->Guild[0] != 0) {
			if (Yourself) {
				strcpy(Membership, "You are ");
			} else {
				snprintf(Membership, sizeof(Membership), "%s is ", Pronoun);
			}

			if (Opp->Rank[0] != 0) {
				strcat(Membership, Opp->Rank);
			} else {
				strcat(Membership, "a member");
			}

			strcat(Membership, " of the ");
			strcat(Membership, Opp->Guild);
			if (Opp->Title[0] != 0) {
				strcat(Membership, " (");
				strcat(Membership, Opp->Title);
				strcat(Membership, ")");
			}
		} else {
			Membership[0] = 0;
		}

		if (Membership[0] != 0) {
			SendMessage(pl->GetConnection(), TALK_INFO_MESSAGE, "You see %s. %s. %s.", Player, Vocation, Membership);
		} else {
			SendMessage(pl->GetConnection(), TALK_INFO_MESSAGE, "You see %s. %s.", Player, Vocation);
		}
		return;
	}

	char s[500], s2[500];
	strcpy(s, "You see ");
	strcat(s, GetName(Obj));
	if (Obj.getFlag(LEVELDOOR)) {
		sprintf(s2, " for level %lu", (unsigned long)Obj.getAttribute(DOORLEVEL));
		strcat(s, s2);
	}

	if (Obj.getFlag(CONTAINER)) {
		sprintf(s2, " (Vol:%lu)", (unsigned long)Obj.getAttribute(CAPACITY));
		strcat(s, s2);
	}

	if (Obj.getFlag(WEAPON)) {
		sprintf(s2, " (Atk:%lu Def:%lu)", (unsigned long)Obj.getAttribute(WEAPONATTACKVALUE), (unsigned long)Obj.getAttribute(WEAPONDEFENDVALUE));
		strcat(s, s2);
	}

	if (Obj.getFlag(THROW)) {
		sprintf(s2, " (Atk:%lu Def:%lu)", (unsigned long)Obj.getAttribute(THROWATTACKVALUE), (unsigned long)Obj.getAttribute(THROWDEFENDVALUE));
		strcat(s, s2);
	}

	if (Obj.getFlag(SHIELD)) {
		sprintf(s2, " (Def:%lu)", (unsigned long)Obj.getAttribute(SHIELDDEFENDVALUE));
		strcat(s, s2);
	}

	if (Obj.getFlag(ARMOR)) {
		sprintf(s2, " (Arm:%lu)", (unsigned long)Obj.getAttribute(ARMORVALUE));
		strcat(s, s2);
	}

	if (Obj.getFlag(KEY)) {
		sprintf(s2, " (Key:%04ld)", (long)(int32_t)Obj.getAttribute(KEYNUMBER));
		strcat(s, s2);
	}

	if (Obj.getFlag(KEYDOOR) && CheckRight(pl->ID, SHOW_KEYHOLE_NUMBERS)) {
		sprintf(s2, " (Door:%04ld)", (long)(int32_t)Obj.getAttribute(KEYHOLENUMBER));
		strcat(s, s2);
	}

	if (Obj.getFlag(SHOWDETAIL)) {
		if (Obj.getFlag(WEAROUT) && Obj.getAttribute(TOTALUSES) > 1) {
			sprintf(s2, " that has %ld charge%s left", (long)(int32_t)Obj.getAttribute(REMAININGUSES), (Obj.getAttribute(REMAININGUSES) != 1 ? "s" : ""));
			strcat(s, s2);
		} else if (Obj.getFlag(EXPIRE) || Obj.getFlag(EXPIRESTOP)) {
			int TimeLeft = ((Obj.getFlag(EXPIRE) ? CronInfo(Obj, false) : Obj.getAttribute(SAVEDEXPIRETIME)) + 59) / 60;
			if (TimeLeft > 0) {
				sprintf(s2, " that has energy for %d minute%s left", TimeLeft, (TimeLeft != 1 ? "s" : ""));
				strcat(s, s2);
			} else {
				strcat(s, " that is brand-new");
			}
		} else {
			error("Look: Object %d has flag SHOWDETAIL, but neither WEAROUT nor EXPIRE.\n", Obj.getObjectType().getTypeID());
		}
	}

	if (Obj.getFlag(RESTRICTLEVEL) || Obj.getFlag(RESTRICTPROFESSION)) {
		char s3[100];
		strcat(s, ".\nIt can only be wielded by ");
		s2[0] = 0;
		if (Obj.getFlag(RESTRICTPROFESSION)) {
			int c = 0;
			for (int i = 5; i > 0; i--) {
				if (i == 5 ? (Obj.getAttribute(PROFESSIONS) & 1) == 0 : (Obj.getAttribute(PROFESSIONS) & (1 << i)) == 0) {
					continue;
				}

				if (c == 0) {
					s3[0] = 0;
				}

				if (c == 1) {
					sprintf(s3, " and %s", s2);
				}

				if (c > 1) {
					sprintf(s3, ", %s", s2);
				}

				switch (i) {
					case PROFESSION_KNIGHT:
						strcpy(s2, "knights");
						break;
					case PROFESSION_PALADIN:
						strcpy(s2, "paladins");
						break;
					case PROFESSION_SORCERER:
						strcpy(s2, "sorcerers");
						break;
					case PROFESSION_DRUID:
						strcpy(s2, "druids");
						break;
					case 5:
						strcpy(s2, "players without vocation");
						break;
				}

				strcat(s2, s3);
				c++;
			}
		} else {
			strcpy(s2, "players");
		}

		if (Obj.getFlag(RESTRICTLEVEL)) {
			sprintf(s3, " of level %ld or higher", (long)(int32_t)Obj.getAttribute(MINIMUMLEVEL));
			strcat(s2, s3);
		}

		strcat(s, s2);
	}

	if (Obj.getFlag(RUNE)) {
		int MagicLevel;
		GetMagicItemDescription(Obj, s2, MagicLevel);
		if (MagicLevel != 0) {
			char s3[20];
			sprintf(s3, " for magic level %d", MagicLevel);
			strcat(s, s3);
		}

		strcat(s, ". It's an \"");
		strcat(s, s2);
		strcat(s, "\"-spell");
		sprintf(s2, " (%ldx)", (long)(int32_t)Obj.getAttribute(CHARGES));
		strcat(s, s2);
	}

	if (Obj.getFlag(BED) && Obj.getAttribute(TEXTSTRING) != 0) {
		sprintf(s2, ". %s is sleeping there", GetDynamicString(Obj.getAttribute(TEXTSTRING)));
		strcat(s, s2);
	}

	if (Obj.getFlag(NAMEDOOR)) {
		int x, y, z;
		GetObjectCoordinates(Obj, x, y, z);
		unsigned short HouseID = GetHouseID(x, y, z);
		if (HouseID != 0) {
			sprintf(s2, ". It belongs to house '%s'", GetHouseName(HouseID));
			strcat(s, s2);
			char *Owner = GetHouseOwner(HouseID);
			if (Owner != nullptr && Owner[0] != 0) {
				sprintf(s2, ". %s owns this house", Owner);
			} else {
				strcpy(s2, ". Nobody owns this house");
			}
			strcat(s, s2);
		} else {
			error("Look: NameDoor at [%d,%d,%d] belongs to no house.\n", x, y, z);
		}
	}

	strcat(s, ".");
	if ((Obj.getFlag(LIQUIDCONTAINER) && Obj.getAttribute(CONTAINERLIQUIDTYPE) == LIQUID_NONE)
			|| (Obj.getFlag(LIQUIDPOOL) && Obj.getAttribute(POOLLIQUIDTYPE) == LIQUID_NONE)) {
		strcat(s, " It is empty.");
	}

	if (CreatureID == 0 || ObjectInRange(CreatureID, Obj, 1)) {
		if (Obj.getFlag(TAKE) && GetWeight(Obj, -1) > 0) {
			int w = GetCompleteWeight(Obj);
			if (Obj.getFlag(CUMULATIVE) && Obj.getAttribute(AMOUNT) > 1 && IsCountable(Obj.getObjectType().getName(1))) {
				sprintf(s2, "\nThey weigh %d.%02d oz.", w / 100, w % 100);
			} else {
				sprintf(s2, "\nIt weighs %d.%02d oz.", w / 100, w % 100);
			}
			strcat(s, s2);
		}

		if (GetInfo(Obj) != nullptr) {
			snprintf(s2, sizeof(s2), "\n%s.", GetInfo(Obj));
			strcat(s, s2);
		}
	}

	if (Obj.getFlag(TEXT)) {
		unsigned short Size = (unsigned short)Obj.getAttribute(FONTSIZE);
		if (Size == 0) {
			if (Obj.getAttribute(TEXTSTRING) != 0) {
				snprintf(s2, sizeof(s2), "\n%s.", GetDynamicString(Obj.getAttribute(TEXTSTRING)));
			} else {
				s2[0] = 0;
			}
		} else if (Size == 1) {
			s2[0] = 0;
		} else if (Obj.getAttribute(TEXTSTRING) == 0) {
			strcpy(s2, "\nNothing is written on it.");
		} else if (CreatureID != 0 && !ObjectInRange(CreatureID, Obj, Size)) {
			strcpy(s2, "\nYou are too far away to read it.");
		} else if (Obj.getAttribute(EDITOR) != 0) {
			// Separate statements set the call order (the text first); both
			// calls can log.
			char *Text = GetDynamicString(Obj.getAttribute(TEXTSTRING));
			snprintf(s2, sizeof(s2), "\n%s has written: %s", GetDynamicString(Obj.getAttribute(EDITOR)), Text);
		} else {
			snprintf(s2, sizeof(s2), "\nYou read: %s", GetDynamicString(Obj.getAttribute(TEXTSTRING)));
		}
		strcat(s, s2);
	}

	SendMessage(pl->GetConnection(), TALK_INFO_MESSAGE, "%s", s);
}

// Talk, Use, Turn, CreatePool, EditText, CreateAtCreature, DeleteAtCreature,
// OpenChannel, InviteToChannel, ExcludeFromChannel and JoinChannel report a
// failure by throwing a RESULT, ApplyPatch and ApplyPatches by throwing a
// const char *. An exception of another type passes to the caller.

void Talk(uint32_t CreatureID, int Mode, char *Addressee, char *Text, bool CheckSpamming) {
	int x, y, z;
	char *Name;
	int Channel = 0;
	int SpellType = 0;
	int Muting = 0;

	TCreature *cr = GetCreature(CreatureID);
	if (cr == nullptr) {
		error("Talk: Given creature %d does not exist.\n", CreatureID);
		throw ERROR;
	}

	GetObjectCoordinates(cr->GetCreatureObject(), x, y, z);
	Name = cr->Name;
	if (Text == nullptr) {
		error("Talk: Text is NULL (speaker: %s, mode: %d).\n", Name, Mode);
		throw ERROR;
	}

	if (strlen(Text) > 255) {
		error("Talk: Text is too long (speaker: %s, length: %d).\n", Name, (int)strlen(Text));
		throw ERROR;
	}

	if (Mode == TALK_CHANNEL_CALL || Mode == TALK_GAMEMASTER_CHANNELCALL || Mode == TALK_HIGHLIGHT_CHANNELCALL || Mode == TALK_ANONYMOUS_CHANNELCALL) {
		// The channel number saturates at the range of a 32-bit int, as atoi()
		// does where long has 32 bits.
		Channel = (int)std::clamp<long>(strtol(Addressee, nullptr, 10), INT32_MIN, INT32_MAX);
		if (Channel < 0 || Channel >= Channels) {
			error("Talk: Invalid channel %d.\n", Channel);
			return;
		}
	}

	print(2, "%s: \"%s\"\n", Name, Text);

	if (cr->Type == PLAYER && CheckSpamming) {
		Muting = ((TPlayer *)cr)->CheckForMuting();
		if (Muting > 0
				&& (Mode == TALK_SAY || Mode == TALK_WHISPER || Mode == TALK_YELL || Mode == TALK_PRIVATE_MESSAGE
					|| ((Mode == TALK_CHANNEL_CALL || Mode == TALK_HIGHLIGHT_CHANNELCALL) && in(Channel, GAMEMASTER_CHANNEL, HELP_CHANNEL)))) {
			SendMessage(cr->GetConnection(), TALK_FAILURE_MESSAGE, "You are still muted for %d second%s.", Muting, (Muting != 1 ? "s" : ""));
			return;
		}
	}

	if (cr->Type == PLAYER && Muting == 0) {
		if (EventSay(CreatureID, Text, Mode)) {
			return;
		}
		SpellType = CheckForSpell(CreatureID, Text);
		if (SpellType == 1 || SpellType == 5 || SpellType < 0) {
			return;
		}
		if (SpellType != 0) {
			Mode = TALK_SAY;
		}
	}

	if (cr->Type == PLAYER && Mode == TALK_YELL) {
		if (cr->EarliestYellRound > RoundNr) {
			throw EXHAUSTED;
		}
		cr->EarliestYellRound = RoundNr + 30;
		strUpper(Text);
	}

	if (cr->Type == PLAYER && Mode == TALK_CHANNEL_CALL && Channel == TRADE_CHANNEL) {
		if (cr->EarliestTradeChannelRound > RoundNr) {
			SendMessage(cr->GetConnection(), TALK_FAILURE_MESSAGE, "You may only place one offer in two minutes.");
			return;
		}
		cr->EarliestTradeChannelRound = RoundNr + 120;
	}

	if (cr->Type == PLAYER && CheckSpamming && SpellType == 0
			&& (Mode == TALK_SAY || Mode == TALK_WHISPER || Mode == TALK_YELL || Mode == TALK_PRIVATE_MESSAGE
				|| ((Mode == TALK_CHANNEL_CALL || Mode == TALK_HIGHLIGHT_CHANNELCALL) && in(Channel, GAMEMASTER_CHANNEL, HELP_CHANNEL)))) {
		if (!CheckRight(cr->ID, NO_BANISHMENT)) {
			Muting = ((TPlayer *)cr)->RecordTalk();
		}
		if (Muting > 0) {
			SendMessage(cr->GetConnection(), TALK_FAILURE_MESSAGE, "You are muted for %d second%s.", Muting, (Muting != 1 ? "s" : ""));
			return;
		}
	}

	if (Mode == TALK_PRIVATE_MESSAGE || Mode == TALK_GAMEMASTER_ANSWER || Mode == TALK_PLAYER_ANSWER || Mode == TALK_GAMEMASTER_MESSAGE || Mode == TALK_ANONYMOUS_MESSAGE) {
		if (Addressee == nullptr) {
			error("Talk: Addressee for message not given.\n");
			throw ERROR;
		}

		TPlayer *pl;
		int Result = IdentifyPlayer(Addressee, false, !CheckRight(cr->ID, READ_GAMEMASTER_CHANNEL), pl);
		switch (Result) {
			case 0:
				break;
			case -1:
				throw PLAYERNOTONLINE;
			case -2:
				throw NAMEAMBIGUOUS;
			default:
				error("Talk: Invalid return value from IdentifyPlayer.\n");
				throw ERROR;
		}

		if (Mode == TALK_PRIVATE_MESSAGE) {
			Muting = ((TPlayer *)cr)->RecordMessage(pl->ID);
			if (Muting > 0) {
				SendMessage(cr->GetConnection(), TALK_FAILURE_MESSAGE, "You have addressed too many players. You are muted for %d second%s.", Muting, (Muting != 1 ? "s" : ""));
				return;
			}
		}

		if (Mode == TALK_ANONYMOUS_MESSAGE) {
			SendMessage(pl->GetConnection(), TALK_ADMIN_MESSAGE, "%s", Text);
		} else {
			uint32_t StatementID = LogCommunication(cr->ID, Mode, 0, Text);
			LogListener(StatementID, (TPlayer *)cr);
			uint32_t HelpID = LogListener(StatementID, pl);
			if (Mode == TALK_GAMEMASTER_ANSWER) {
				SendTalk(pl->GetConnection(), HelpID, "Gamemaster", TALK_GAMEMASTER_ANSWER, Text, 0);
			} else {
				SendTalk(pl->GetConnection(), HelpID, Name, Mode, Text, 0);
			}
		}

		if (cr != nullptr && cr->Type == PLAYER) {
			SendMessage(cr->GetConnection(), TALK_FAILURE_MESSAGE, "Message sent to %s.", (Mode == TALK_PLAYER_ANSWER ? "Gamemaster" : pl->Name));
		}
		return;
	} else if (Mode == TALK_SAY || Mode == TALK_WHISPER || Mode == TALK_YELL || Mode == TALK_ANIMAL_LOW || Mode == TALK_ANIMAL_LOUD) {
		uint32_t StatementID;
		if (cr->Type == PLAYER) {
			StatementID = LogCommunication(cr->ID, Mode, 0, Text);
		} else {
			StatementID = 0;
		}

		int Radius = (Mode == TALK_YELL || Mode == TALK_ANIMAL_LOUD) ? 30 : 7;
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
			int dx = abs(x - destX);
			int dy = abs(y - destY);
			int dz = abs(z - destZ);
			switch (Mode) {
				case TALK_SAY:
				case TALK_ANIMAL_LOW: {
					if (dz == 0 && dx <= 7 && dy <= 5) {
						uint32_t HelpID = LogListener(StatementID, pl);
						SendTalk(Connection, HelpID, Name, Mode, x, y, z, Text);
					}
					break;
				}
				case TALK_WHISPER: {
					if (dz == 0 && dx <= 7 && dy <= 5) {
						if (dx <= 1 && dy <= 1) {
							uint32_t HelpID = LogListener(StatementID, pl);
							SendTalk(Connection, HelpID, Name, Mode, x, y, z, Text);
						} else {
							SendTalk(Connection, 0, Name, Mode, x, y, z, "pspsps");
						}
					}
					break;
				}
				case TALK_YELL:
				case TALK_ANIMAL_LOUD: {
					if (dx <= 30 && dy <= 30 && (dz == 0 || (z <= 7 && destZ <= 7))) {
						uint32_t HelpID = LogListener(StatementID, pl);
						SendTalk(Connection, HelpID, Name, Mode, x, y, z, Text);
					}
					break;
				}
			}
		}
	} else if (Mode == TALK_GAMEMASTER_BROADCAST) {
		uint32_t StatementID = LogCommunication(cr->ID, Mode, 0, Text);
		TConnection *Connection = GetFirstConnection();
		while (Connection != nullptr) {
			if (Connection->IsOnline()) {
				uint32_t HelpID = LogListener(StatementID, Connection->GetPlayer());
				SendTalk(Connection, HelpID, Name, Mode, Text, 0);
			}
			Connection = GetNextConnection();
		}
	} else if (Mode == TALK_ANONYMOUS_BROADCAST) {
		TConnection *Connection = GetFirstConnection();
		while (Connection != nullptr) {
			if (Connection->IsOnline()) {
				// Quirk: the text goes out as the format string itself.
				SendMessage(Connection, TALK_ADMIN_MESSAGE, Text);
			}
			Connection = GetNextConnection();
		}
	} else if (Mode == TALK_CHANNEL_CALL || Mode == TALK_GAMEMASTER_CHANNELCALL || Mode == TALK_HIGHLIGHT_CHANNELCALL || Mode == TALK_ANONYMOUS_CHANNELCALL) {
		uint32_t StatementID = 0;
		if (Mode != TALK_ANONYMOUS_CHANNELCALL) {
			StatementID = LogCommunication(cr->ID, Mode, Channel, Text);
		}

		for (uint32_t CreatureID = GetFirstSubscriber(Channel); CreatureID != 0; CreatureID = GetNextSubscriber()) {
			TPlayer *recv = GetPlayer(CreatureID);
			if (recv == nullptr) {
				continue;
			}
			if (Channel == GUILD_CHANNEL && (cr->Type != PLAYER || strcmp(((TPlayer *)cr)->Guild, recv->Guild) != 0)) {
				continue;
			}

			LogListener(StatementID, recv);
			uint32_t HelpStatementID = 0;
			if (in(Channel, GAME_CHANNEL, HELP_CHANNEL) || Channel == TUTOR_CHANNEL || CheckRight(recv->ID, LOG_COMMUNICATION)) {
				HelpStatementID = StatementID;
			}
			SendTalk(recv->GetConnection(), HelpStatementID, Name, Mode, Channel, Text);
		}
	} else {
		error("Talk: Invalid talk mode %c.\n", Mode);
	}

	if (Mode == TALK_SAY && cr->Type == PLAYER) {
		TFindCreatures Search(3, 3, CreatureID, FIND_NPCS);
		while (true) {
			uint32_t ID = Search.getNext();
			if (ID == 0) {
				break;
			}

			TCreature *cr = GetCreature(ID);
			if (cr == nullptr) {
				error("Talk: Creature does not exist.\n");
				continue;
			}

			if (cr->posz == z) {
				cr->TalkStimulus(CreatureID, Text);
			}
		}
	}
}

void Use(uint32_t CreatureID, Object Obj1, Object Obj2, unsigned char Info) {
	if (!Obj1.exists()) {
		error("Use: Given object Obj 1 does not exist.\n");
		throw ERROR;
	}

	if (Obj2 != NONE && !Obj2.exists()) {
		error("Use: Given object Obj 2 does not exist.\n");
		throw ERROR;
	}

	CheckTopUseObject(CreatureID, Obj1);
	if (!ObjectAccessible(CreatureID, Obj1, 1)) {
		throw NOTACCESSIBLE;
	}

	if (Obj2 != NONE && !Obj1.getFlag(DISTUSE)) {
		CheckTopMultiuseObject(CreatureID, Obj2);
	}

	if (Obj2 != NONE && !ObjectAccessible(CreatureID, Obj2, 1)) {
		if (!Obj1.getFlag(DISTUSE)) {
			throw NOTACCESSIBLE;
		}

		TCreature *cr = GetCreature(CreatureID);
		if (cr == nullptr) {
			error("Use: Causing creature does not exist.\n");
			throw ERROR;
		}

		int x, y, z;
		GetObjectCoordinates(Obj2, x, y, z);
		if (z < cr->posz) {
			throw UPSTAIRS;
		}
		if (z > cr->posz) {
			throw DOWNSTAIRS;
		}
		if (!ThrowPossible(cr->posx, cr->posy, cr->posz, x, y, z, 0)) {
			throw CANNOTTHROW;
		}
	}

	// The built-in uses (scripting.hh): the entries of [[use]] of items.toml
	// in order; the first whose flag Obj1 has and whose script's onUse gives
	// true has handled the use. A result code it raises goes up from here.
	for (const TUseEntry &Entry : UseEntries) {
		if (!Obj1.getFlag(Entry.Flag)) {
			continue;
		}
		TScriptCall Call(Entry.ScriptNumber, "onUse");
		Call.addPlayer(CreatureID);
		Call.addItem(Obj1.getObjectID());
		Call.addItem(Obj2.getObjectID());
		Call.addNumber(static_cast<int>(Info));
		if (Call.runBoolean()) {
			return;
		}
	}

	throw NOTUSABLE;
}

void Turn(uint32_t CreatureID, Object Obj) {
	if (!Obj.exists()) {
		error("Turn: Given object does not exist.\n");
		throw ERROR;
	}

	if (!ObjectAccessible(CreatureID, Obj, 1)) {
		throw NOTACCESSIBLE;
	}

	if (!Obj.getFlag(ROTATE)) {
		throw NOTTURNABLE;
	}

	ObjectType NewType = (int)Obj.getAttribute(ROTATETARGET);
	if (NewType == 0) {
		error("Turn: Object %d is destroyed by turning.\n", Obj.getObjectType().getTypeID());
	}

	Change(Obj, NewType, 0);
}

void CreatePool(Object MapCon, ObjectType Type, uint32_t LiquidType) {
	if (!MapCon.exists()) {
		error("CreatePool: Given MapContainer does not exist.\n");
		throw ERROR;
	}

	if (!MapCon.isMapContainer()) {
		error("CreatePool: Given object is not a MapContainer.\n");
		throw ERROR;
	}

	Object Obj = GetFirstContainerObject(MapCon);
	while (Obj != NONE) {
		Object Help = Obj;
		Obj++;
		if (Help.getFlag(BOTTOM)) {
			if (!Help.getFlag(LIQUIDPOOL)) {
				throw NOROOM;
			}

			try {
				Delete(Help, -1);
			} catch (RESULT r) {
				error("CreatePool: Exception %d while deleting the old pool.\n", r);
			}
		}
	}

	Create(MapCon, Type, LiquidType);
}

void EditText(uint32_t CreatureID, Object Obj, char *Text) {
	if (!Obj.exists()) {
		error("EditText: Given object does not exist.\n");
		throw ERROR;
	}

	if (!Obj.getFlag(WRITE) && (!Obj.getFlag(WRITEONCE) || Obj.getAttribute(TEXTSTRING) != 0)) {
		error("EditText: Object is not writable.\n");
		throw ERROR;
	}

	TCreature *cr = GetCreature(CreatureID);
	if (cr == nullptr) {
		error("EditText: Creature does not exist.\n");
		throw ERROR;
	}

	if (strlen(Text) >= Obj.getAttribute(Obj.getFlag(WRITE) ? MAXLENGTH : MAXLENGTHONCE)) {
		throw TOOLONG;
	}

	if (CreatureID != 0 && !ObjectAccessible(CreatureID, Obj, 1)) {
		throw NOTACCESSIBLE;
	}

	if ((Obj.getAttribute(TEXTSTRING) == 0 && Text[0] != 0)
			|| (Obj.getAttribute(TEXTSTRING) != 0 && strcmp(Text, GetDynamicString(Obj.getAttribute(TEXTSTRING))) != 0)) {
		DeleteDynamicString(Obj.getAttribute(TEXTSTRING));
		Change(Obj, TEXTSTRING, AddDynamicString(Text));
		DeleteDynamicString(Obj.getAttribute(EDITOR));
		Change(Obj, EDITOR, AddDynamicString(cr->Name));
	} else {
		print(3, "Text has not changed.\n");
	}
}

Object CreateAtCreature(uint32_t CreatureID, ObjectType Type, uint32_t Value) {
	TCreature *cr = GetCreature(CreatureID);
	if (cr == nullptr) {
		error("CreateAtCreature: Cannot find creature.\n");
		throw ERROR;
	}

	try {
		CheckWeight(CreatureID, Type, Value, 0);
	} catch (RESULT) {
		return Create(GetMapContainer(cr->GetCreatureObject()), Type, Value);
	}

	int pos = 0;
	bool first = true;
	bool invert = Type.getFlag(MOVEMENTEVENT);
again:
	pos++;
	try {
		Object Obj = GetBodyObject(CreatureID, pos);
		if (first != invert && Obj == NONE) {
			return Create(GetBodyContainer(CreatureID, pos), Type, Value);
		}
		if (first == invert && Obj != NONE && Obj.getFlag(CONTAINER)) {
			return Create(Obj, Type, Value);
		}
	} catch (RESULT r) {
		if (r == TOOHEAVY) {
			first = false;
			pos = LAST_BODY_POSITION;
		}
	}

	if (pos <= LAST_BODY_POSITION - 1) {
		goto again;
	}

	if (first) {
		first = false;
		pos = 0;
		goto again;
	}

	return Create(GetMapContainer(cr->GetCreatureObject()), Type, Value);
}

void DeleteAtCreature(uint32_t CreatureID, ObjectType Type, int Amount, uint32_t Data) {
	while (Amount > 0) {
		Object Obj = GetInventoryObject(CreatureID, Type, Data);
		if (Obj == NONE) {
			error("DeleteAtCreature: No object found.\n");
			throw ERROR;
		}

		if (Obj.getFlag(CUMULATIVE) && (int)Obj.getAttribute(AMOUNT) <= Amount) {
			Amount -= Obj.getAttribute(AMOUNT);
			Delete(Obj, -1);
		} else if (Obj.getFlag(CUMULATIVE)) {
			Change(Obj, AMOUNT, Obj.getAttribute(AMOUNT) - Amount);
			Amount = 0;
		} else {
			Delete(Obj, -1);
			Amount--;
		}
	}
}

void ProcessCronSystem() {
	while (true) {
		Object Obj = CronCheck();
		if (Obj == NONE) {
			break;
		}

		try {
			ObjectType Dest = (int)Obj.getAttribute(EXPIRETARGET);
			if (Obj.getFlag(CONTAINER)) {
				Empty(Obj, (Dest == 0 || !Dest.getFlag(CONTAINER)) ? 0 : (int)Dest.getAttribute(CAPACITY));
			}
			Change(Obj, Dest, 0);
		} catch (RESULT r) {
			error("ProcessCronSystem: Exception %d while decaying %d.\n", r, Obj.getObjectType().getTypeID());
			try {
				Delete(Obj, -1);
			} catch (RESULT r) {
				error("ProcessCronSystem: Cannot delete object - exception %d.\n", r);
			}
		}
	}
}

bool SectorRefreshable(int SectorX, int SectorY, int SectorZ) {
	TFindCreatures SearchPlayers(31, 31, SectorX * SECTOR_SIZE + SECTOR_SIZE / 2, SectorY * SECTOR_SIZE + SECTOR_SIZE / 2, FIND_PLAYERS);
	while (true) {
		uint32_t ID = SearchPlayers.getNext();
		if (ID == 0) {
			break;
		}

		TCreature *cr = GetCreature(ID);
		if (cr == nullptr) {
			error("SectorRefreshable: Creature does not exist.\n");
			continue;
		}

		if (cr->posz <= 7 && SectorZ <= 7) {
			return false;
		}

		if (cr->posz > 7 && SectorZ - cr->posz >= -2 && SectorZ - cr->posz <= 2) {
			return false;
		}
	}

	return true;
}

void RefreshSector(int SectorX, int SectorY, int SectorZ, unsigned char *Data, int Size) {
	if (!SectorRefreshable(SectorX, SectorY, SectorZ)) {
		return;
	}

	TReadBuffer Buffer(Data, Size);
	RefreshSector(SectorX, SectorY, SectorZ, Buffer);

	TFindCreatures SearchMonsters(16, 16, SectorX * SECTOR_SIZE + SECTOR_SIZE / 2, SectorY * SECTOR_SIZE + SECTOR_SIZE / 2, FIND_ALL);
	while (true) {
		uint32_t CreatureID = SearchMonsters.getNext();
		if (CreatureID == 0) {
			break;
		}

		TCreature *cr = GetCreature(CreatureID);
		if (cr == nullptr) {
			error("RefreshSector: Creature does not exist.\n");
			continue;
		}

		if (cr->posz != SectorZ) {
			continue;
		}

		bool PositionOk = true;
		int x = cr->posx;
		int y = cr->posy;
		int z = cr->posz;
		Object Obj = GetFirstObject(x, y, z);
		while (Obj != NONE) {
			if (!Obj.isCreature() && Obj.getFlag(UNPASS)) {
				PositionOk = false;
			}
			Obj++;
		}

		if (!PositionOk) {
			try {
				if (SearchFreeField(x, y, z, 1, 0, false)) {
					Move(0, cr->GetCreatureObject(), GetMapContainer(x, y, z), -1, false, NONE);
				} else if (cr->Type == NPC) {
					print(2, "NPC at [%d,%d,%d] must be reset to its start.\n", x, y, z);
					Move(0, cr->GetCreatureObject(), GetMapContainer(cr->startx, cr->starty, cr->startz), -1, false, NONE);
				} else if (cr->Type == MONSTER) {
					print(2, "Monster at [%d,%d,%d] must be deleted.\n", x, y, z);
					delete cr;
				} else {
					error("Player at [%d,%d,%d] is affected by refresh.\n", x, y, z);
				}
			} catch (RESULT r) {
				error("RefreshSector: Exception %d while relocating the monster.\n", r);
			}
		}
	}
}

void RefreshMap() {
	TDynamicWriteBuffer HelpBuffer(65536);
	for (int x = SectorXMin; x <= SectorXMax; x++) {
		for (int y = SectorYMin; y <= SectorYMax; y++) {
			for (int z = SectorZMin; z <= SectorZMax; z++) {
				if (!SectorRefreshable(x, y, z)) {
					continue;
				}

				char FileName[4096];
				sprintf(FileName, "%s/%04u-%04u-%02u.sec", ORIGMAPPATH, x, y, z);
				if (!FileExists(FileName)) {
					continue;
				}

				try {
					TReadScriptFile Script;
					Script.open(FileName);
					bool Refreshable = false;
					int xx = -1;
					int yy = -1;
					HelpBuffer.rewind();
					while (true) {
						Script.nextToken();
						if (Script.Token == ENDOFFILE) {
							break;
						}

						if (Script.Token == SPECIAL && Script.getSpecial() == ',') {
							continue;
						}

						if (Script.Token == BYTES) {
							unsigned char *Help = Script.getBytesequence();
							xx = Help[0];
							yy = Help[1];
							Script.readSymbol(':');
							Refreshable = false;
						} else if (Script.Token == IDENTIFIER) {
							if (xx == -1) {
								Script.error("coordinate expected");
							}
							if (strcmp(Script.getIdentifier(), "refresh") == 0) {
								Refreshable = true;
							}
							if (strcmp(Script.getIdentifier(), "content") == 0) {
								Script.readSymbol('=');
								if (Refreshable) {
									HelpBuffer.writeByte((unsigned char)xx);
									HelpBuffer.writeByte((unsigned char)yy);
								}
								LoadObjects(Script, HelpBuffer, !Refreshable);
							}
						}
					}
					Script.close();

					int DataSize = HelpBuffer.getDataSize();
					if (DataSize != 0) {
						TReadBuffer ReadBuffer(HelpBuffer.getData(), DataSize);
						RefreshSector(x, y, z, ReadBuffer);
					}
				} catch (const char *s) {
					error("RefreshMap: Error while processing the file (%s).\n", s);
				}
			}
		}
	}
}

void RefreshCylinders() {
	static int RefreshX = 2147483647;
	static int RefreshY = 2147483647;
	for (int i = 0; i < RefreshedCylinders; i++) {
		if (RefreshX < SectorXMax) {
			RefreshX++;
		} else {
			RefreshX = SectorXMin;
			if (RefreshY < SectorYMax) {
				RefreshY++;
			} else {
				RefreshY = SectorYMin;
			}
		}

		for (int z = SectorZMin; z <= SectorZMax; z++) {
			if (SectorRefreshable(RefreshX, RefreshY, z)) {
				LoadSectorOrder(RefreshX, RefreshY, z);
			}
		}
	}
}

void ApplyPatch(int SectorX, int SectorY, int SectorZ, bool FullSector, TReadScriptFile &Script, bool SaveHouses) {
	if (!in(SectorX, SectorXMin, SectorXMax) || !in(SectorY, SectorYMin, SectorYMax) || !in(SectorZ, SectorZMin, SectorZMax)) {
		return;
	}

	PrepareHouseCleanup();
	PatchSector(SectorX, SectorY, SectorZ, FullSector, Script, SaveHouses);
	FinishHouseCleanup();

	TFindCreatures SearchMonsters(16, 16, SectorX * SECTOR_SIZE + SECTOR_SIZE / 2, SectorY * SECTOR_SIZE + SECTOR_SIZE / 2, FIND_ALL);
	while (true) {
		uint32_t CreatureID = SearchMonsters.getNext();
		if (CreatureID == 0) {
			break;
		}

		TCreature *cr = GetCreature(CreatureID);
		if (cr == nullptr) {
			error("ApplyPatch: Creature does not exist.\n");
			continue;
		}

		if (cr->posz != SectorZ) {
			continue;
		}

		bool PositionOk = true;
		int x = cr->posx;
		int y = cr->posy;
		int z = cr->posz;
		Object Obj = GetFirstObject(x, y, z);
		while (Obj != NONE) {
			if (!Obj.isCreature() && Obj.getFlag(UNPASS)) {
				PositionOk = false;
			}
			Obj++;
		}

		if (PositionOk) {
			MoveObject(cr->GetCreatureObject(), GetMapContainer(x, y, z));
		} else {
			try {
				if (SearchFreeField(x, y, z, 1, 0, false)) {
					Move(0, cr->GetCreatureObject(), GetMapContainer(x, y, z), -1, false, NONE);
				} else if (cr->Type == NPC) {
					print(2, "NPC at [%d,%d,%d] must be reset to its start.\n", x, y, z);
					Move(0, cr->GetCreatureObject(), GetMapContainer(cr->startx, cr->starty, cr->startz), -1, false, NONE);
				} else if (cr->Type == MONSTER) {
					print(2, "Monster at [%d,%d,%d] must be deleted.\n", x, y, z);
					Delete(cr->GetCreatureObject(), -1);
					cr->StartLogout(false, false);
				} else {
					error("Player at [%d,%d,%d] is affected by patch.\n", x, y, z);
				}
			} catch (RESULT r) {
				error("ApplyPatch: Exception %d while relocating the monster.\n", r);
			}
		}
	}
}

void ApplyPatches() {
	char FileName[4096];
	sprintf(FileName, "%s/save-houses", SAVEPATH);
	bool SaveHouses = FileExists(FileName);
	if (SaveHouses) {
		print(2, "Houses are not patched.\n");
		unlink(FileName);
	}

	DIR *WorkDir = opendir(SAVEPATH);
	if (WorkDir == nullptr) {
		error("ApplyPatches: Subdirectory %s not found.\n", SAVEPATH);
		return;
	}

	int nr, x, y, z;
	int MaxPatch = -1;
	while (true) {
		dirent *DirEntry = readdir(WorkDir);
		if (DirEntry == nullptr) {
			break;
		}

		if (strlen(DirEntry->d_name) <= 4) {
			continue;
		}

		if (strcmp(DirEntry->d_name + strlen(DirEntry->d_name) - 4, ".pat") == 0
				&& sscanf(DirEntry->d_name, "%d.pat", &nr) == 1) {
			if (nr > MaxPatch) {
				MaxPatch = nr;
			}
		} else if (strcmp(DirEntry->d_name + strlen(DirEntry->d_name) - 4, ".sec") == 0
				&& sscanf(DirEntry->d_name, "%d-%d-%d.sec", &x, &y, &z) == 3) {
			print(2, "Patching complete sector %d/%d/%d ...\n", x, y, z);
			sprintf(FileName, "%s/%s", SAVEPATH, DirEntry->d_name);
			TReadScriptFile Script;
			Script.open(FileName);
			ApplyPatch(x, y, z, true, Script, SaveHouses);
			Script.close();
			unlink(FileName);
		}
	}
	closedir(WorkDir);

	for (int nr = 0; nr <= MaxPatch; nr++) {
		sprintf(FileName, "%s/%03d.pat", SAVEPATH, nr);
		if (!FileExists(FileName)) {
			continue;
		}

		TReadScriptFile Script;
		Script.open(FileName);
		if (stricmp(Script.readIdentifier(), "sector", -1) != 0) {
			Script.error("Sector expected");
		}
		x = Script.readNumber();
		Script.readSymbol(',');
		y = Script.readNumber();
		Script.readSymbol(',');
		z = Script.readNumber();
		print(2, "Patching parts of sector %d/%d/%d (patch %d) ...\n", x, y, z, nr);
		ApplyPatch(x, y, z, false, Script, false);
		Script.close();
		unlink(FileName);
	}
}

int GetNumberOfChannels() {
	return Channels;
}

bool ChannelActive(int ChannelID) {
	if (ChannelID < 0 || ChannelID >= Channels) {
		error("ChannelActive: Invalid channel number %d.\n", ChannelID);
		return false;
	}

	return ChannelID < PUBLIC_CHANNELS || Channel(ChannelID).Moderator != 0;
}

bool ChannelAvailable(int ChannelID, uint32_t CharacterID) {
	if (!ChannelActive(ChannelID)) {
		return false;
	}

	TPlayer *pl = GetPlayer(CharacterID);
	if (pl == nullptr) {
		error("ChannelAvailable: Player does not exist.\n");
		return false;
	}

	switch (ChannelID) {
		case GUILD_CHANNEL:
			return pl->Guild[0] != 0;
		case GAMEMASTER_CHANNEL:
			return CheckRight(CharacterID, READ_GAMEMASTER_CHANNEL);
		case TUTOR_CHANNEL:
			return CheckRight(CharacterID, READ_TUTOR_CHANNEL);
		case REQUEST_QUEUE:
			return CheckRight(CharacterID, READ_GAMEMASTER_CHANNEL);
		case GAME_CHANNEL:
		case TRADE_CHANNEL:
		case REALLIFE_CHANNEL:
		case HELP_CHANNEL:
			return true;
	}

	if (ChannelID < PUBLIC_CHANNELS) {
		return false;
	}

	TChannel &C = Channel(ChannelID);
	if (CharacterID == C.Moderator) {
		return true;
	}

	for (int i = 0; i < C.InvitedPlayers; i++) {
		if (CharacterID == C.InvitedPlayer(i)) {
			return true;
		}
	}

	return false;
}

char *GetChannelName(int ChannelID, uint32_t CharacterID) {
	static char ChannelName[40];

	if (!ChannelActive(ChannelID)) {
		return "Unknown";
	}

	TPlayer *pl = GetPlayer(CharacterID);
	if (pl == nullptr) {
		error("GetChannelName: Player does not exist.\n");
		return "Unknown";
	}

	switch (ChannelID) {
		case GUILD_CHANNEL:
			return pl->Guild;
		case GAMEMASTER_CHANNEL:
			return "Gamemaster";
		case TUTOR_CHANNEL:
			return "Tutor";
		case REQUEST_QUEUE:
			return "Rule Violations";
		case GAME_CHANNEL:
			return "Game-Chat";
		case TRADE_CHANNEL:
			return "Trade";
		case REALLIFE_CHANNEL:
			return "RL-Chat";
		case HELP_CHANNEL:
			return "Help";
	}

	if (ChannelID >= PUBLIC_CHANNELS) {
		// Quirk: a 30-character moderator name writes one byte past
		// ChannelName.
		sprintf(ChannelName, "%s's Channel", Channel(ChannelID).ModeratorName);
		return ChannelName;
	}

	error("GetChannelName: Unused channel %d.\n", ChannelID);
	return "Unknown";
}

bool ChannelSubscribed(int ChannelID, uint32_t CharacterID) {
	if (!ChannelActive(ChannelID)) {
		return false;
	}

	TChannel &C = Channel(ChannelID);
	for (int i = 0; i < C.Subscribers; i++) {
		if (CharacterID == C.Subscriber(i)) {
			return true;
		}
	}

	return false;
}

uint32_t GetFirstSubscriber(int ChannelID) {
	if (!ChannelActive(ChannelID)) {
		return 0;
	}

	CurrentChannelID = ChannelID;
	CurrentSubscriberNumber = 0;
	return GetNextSubscriber();
}

uint32_t GetNextSubscriber() {
	if (!ChannelActive(CurrentChannelID)) {
		return 0;
	}

	TChannel &C = Channel(CurrentChannelID);
	if (CurrentSubscriberNumber >= C.Subscribers) {
		return 0;
	}

	return C.Subscriber(CurrentSubscriberNumber++);
}

bool MayOpenChannel(uint32_t CharacterID) {
	if (Channels >= 65535) {
		return false;
	}

	if (!CheckRight(CharacterID, PREMIUM_ACCOUNT)) {
		return false;
	}

	for (int i = PUBLIC_CHANNELS; i < Channels; i++) {
		if (Channel(i).Moderator == CharacterID) {
			return false;
		}
	}

	return true;
}

void OpenChannel(uint32_t CharacterID) {
	TPlayer *pl = GetPlayer(CharacterID);
	if (pl == nullptr) {
		error("OpenChannel: Creature %lu does not exist.\n", (unsigned long)CharacterID);
		throw ERROR;
	}

	if (Channels >= 65535) {
		error("OpenChannel: Too many channels.\n");
		throw ERROR;
	}

	if (!CheckRight(CharacterID, PREMIUM_ACCOUNT)) {
		throw NOPREMIUMACCOUNT;
	}

	for (int i = PUBLIC_CHANNELS; i < Channels; i++) {
		if (Channel(i).Moderator == CharacterID) {
			SendOpenOwnChannel(pl->GetConnection(), i);
			return;
		}
	}

	int ChannelID = Channels;
	for (int i = PUBLIC_CHANNELS; i < Channels; i++) {
		if (Channel(i).Moderator == 0) {
			ChannelID = i;
		}
	}

	if (ChannelID == Channels) {
		Channels++;
	}

	TChannel &C = Channel(ChannelID);
	C.Moderator = CharacterID;
	strcpy(C.ModeratorName, pl->Name);
	C.Subscriber(0) = CharacterID;
	C.Subscribers = 1;
	C.InvitedPlayers = 0;
	SendOpenOwnChannel(pl->GetConnection(), ChannelID);
}

void CloseChannel(int ChannelID) {
	if (ChannelID < PUBLIC_CHANNELS || ChannelID >= Channels) {
		error("CloseChannel: Invalid ChannelID %d.\n", ChannelID);
		return;
	}

	TChannel &C = Channel(ChannelID);
	if (C.Moderator == 0) {
		error("CloseChannel: Channel is already closed.\n");
		return;
	}

	C.Moderator = 0;
}

void InviteToChannel(uint32_t CharacterID, char *Name) {
	TPlayer *pl = GetPlayer(CharacterID);
	if (pl == nullptr) {
		error("InviteToChannel: Creature %lu does not exist.\n", (unsigned long)CharacterID);
		return;
	}

	int channel = -1;
	for (int i = PUBLIC_CHANNELS; i < Channels; i++) {
		if (Channel(i).Moderator == CharacterID) {
			channel = i;
		}
	}

	if (channel == -1) {
		return;
	}

	TPlayer *Opp = nullptr;
	uint32_t OppID = 0;
	switch (IdentifyPlayer(Name, false, !CheckRight(pl->ID, READ_GAMEMASTER_CHANNEL), Opp)) {
		case 0:
			OppID = Opp->ID;
			Name = Opp->Name;
			break;
		case -1:
			OppID = GetCharacterID(Name);
			if (OppID == 0) {
				throw PLAYERNOTEXISTING;
			}
			Name = GetCharacterName(Name);
			break;
		case -2:
			OppID = GetCharacterID(Name);
			if (OppID == 0) {
				throw NAMEAMBIGUOUS;
			}
			Name = GetCharacterName(Name);
			break;
	}

	print(3, "Inviting player %s (%lu) to private channel.\n", Name, (unsigned long)OppID);
	TChannel &C = Channel(channel);
	if (CharacterID == OppID) {
		return;
	}

	for (int i = 0; i < C.InvitedPlayers; i++) {
		if (C.InvitedPlayer(i) == OppID) {
			SendMessage(pl->GetConnection(), TALK_INFO_MESSAGE, "%s has already been invited.", Name);
			return;
		}
	}

	C.InvitedPlayer(C.InvitedPlayers) = OppID;
	C.InvitedPlayers++;
	SendMessage(pl->GetConnection(), TALK_INFO_MESSAGE, "%s has been invited.", Name);
	if (Opp != nullptr) {
		SendMessage(Opp->GetConnection(), TALK_INFO_MESSAGE, "%s invites you to %s private chat channel.", pl->Name, (pl->Sex == SEX_MALE ? "his" : "her"));
	}
}

void ExcludeFromChannel(uint32_t CharacterID, char *Name) {
	TPlayer *pl = GetPlayer(CharacterID);
	if (pl == nullptr) {
		error("ExcludeFromChannel: Creature %lu does not exist.\n", (unsigned long)CharacterID);
		return;
	}

	int channel = -1;
	for (int i = PUBLIC_CHANNELS; i < Channels; i++) {
		if (Channel(i).Moderator == CharacterID) {
			channel = i;
		}
	}

	if (channel == -1) {
		return;
	}

	TPlayer *Opp = nullptr;
	uint32_t OppID = 0;
	switch (IdentifyPlayer(Name, false, !CheckRight(pl->ID, READ_GAMEMASTER_CHANNEL), Opp)) {
		case 0:
			OppID = Opp->ID;
			Name = Opp->Name;
			break;
		case -1:
			OppID = GetCharacterID(Name);
			if (OppID == 0) {
				throw PLAYERNOTEXISTING;
			}
			break;
		case -2:
			OppID = GetCharacterID(Name);
			if (OppID == 0) {
				throw NAMEAMBIGUOUS;
			}
			break;
	}

	print(3, "Excluding player %s (%lu) from private channel.\n", Name, (unsigned long)OppID);
	TChannel &C = Channel(channel);
	if (CharacterID == OppID) {
		return;
	}

	int Position = -1;
	for (int i = 0; i < C.InvitedPlayers; i++) {
		if (C.InvitedPlayer(i) == OppID) {
			Position = i;
		}
	}

	if (Position == -1) {
		SendMessage(pl->GetConnection(), TALK_INFO_MESSAGE, "%s has not been invited.", Name);
		return;
	}

	C.InvitedPlayers--;
	C.InvitedPlayer(Position) = C.InvitedPlayer(C.InvitedPlayers);
	SendMessage(pl->GetConnection(), TALK_INFO_MESSAGE, "%s has been excluded.", Name);
	if (ChannelSubscribed(channel, OppID)) {
		LeaveChannel(channel, OppID, false);
		// Quirk: Opp is NULL when the excluded player was not found online,
		// and it is dereferenced all the same.
		SendCloseChannel(Opp->GetConnection(), channel);
	}
}

bool JoinChannel(int ChannelID, uint32_t CharacterID) {
	if (!ChannelActive(ChannelID)) {
		throw ERROR;
	}

	if (!ChannelAvailable(ChannelID, CharacterID)) {
		throw NOTACCESSIBLE;
	}

	TChannel &C = Channel(ChannelID);
	if (!ChannelSubscribed(ChannelID, CharacterID)) {
		C.Subscriber(C.Subscribers) = CharacterID;
		C.Subscribers++;
	}

	return CharacterID == C.Moderator;
}

void LeaveChannel(int ChannelID, uint32_t CharacterID, bool Close) {
	if (!ChannelActive(ChannelID)) {
		return;
	}

	TChannel &C = Channel(ChannelID);
	for (int i = 0; i < C.Subscribers; i++) {
		if (CharacterID == C.Subscriber(i)) {
			C.Subscribers--;
			C.Subscriber(i) = C.Subscriber(C.Subscribers);
			break;
		}
	}

	if (Close && CharacterID == C.Moderator) {
		for (int i = 0; i < C.Subscribers; i++) {
			TPlayer *pl = GetPlayer(C.Subscriber(i));
			if (pl != nullptr) {
				SendCloseChannel(pl->GetConnection(), ChannelID);
			}
		}
		C.Subscribers = 0;
	}

	if (ChannelID >= PUBLIC_CHANNELS && C.Subscribers == 0) {
		CloseChannel(ChannelID);
	}
}

void LeaveAllChannels(uint32_t CharacterID) {
	for (int ChannelID = 0; ChannelID < Channels; ChannelID++) {
		if (ChannelActive(ChannelID)) {
			LeaveChannel(ChannelID, CharacterID, false);
		}
	}
}

uint32_t LogCommunication(uint32_t CharacterID, int Mode, int Channel, char *Text) {
	if (Text == nullptr) {
		error("LogCommunication: Text is NULL.\n");
		return 0;
	}

	if (CharacterID == 0) {
		error("LogCommunication: CharacterID is Null.\n");
		return 0;
	}

	StatementID++;
	TStatement *Statement = Statements.append();
	Statement->StatementID = StatementID;
	Statement->TimeStamp = (uint32_t)time(nullptr);
	Statement->CharacterID = CharacterID;
	Statement->Mode = Mode;
	Statement->Channel = Channel;
	Statement->Text = AddDynamicString(Text);
	Statement->Reported = false;
	return StatementID;
}

uint32_t LogListener(uint32_t StatementID, TPlayer *pl) {
	if (StatementID == 0 || pl == nullptr || !CheckRight(pl->ID, LOG_COMMUNICATION)) {
		return 0;
	}

	TListener *Listener = Listeners.append();
	Listener->StatementID = StatementID;
	Listener->CharacterID = pl->ID;
	return StatementID;
}

void ProcessCommunicationControl() {
	uint32_t Limit = (uint32_t)time(nullptr) - 1800;
	while (true) {
		TStatement *OldestStatement = Statements.tail()();
		if (OldestStatement == nullptr || OldestStatement->TimeStamp >= Limit) {
			break;
		}
		DeleteDynamicString(OldestStatement->Text);
		Statements.remove();
	}

	uint32_t LowestStatementID = 0;
	if (Statements.tail()() != nullptr) {
		LowestStatementID = Statements.tail()()->StatementID;
	}

	while (true) {
		TListener *OldestListener = Listeners.tail()();
		if (OldestListener == nullptr || OldestListener->StatementID >= LowestStatementID) {
			break;
		}
		Listeners.remove();
	}
}

int GetCommunicationContext(uint32_t CharacterID, uint32_t StatementID, int &NumberOfStatements, vector<TReportedStatement> *&ReportedStatements) {
	fifoIterator<TStatement> iter = Statements.head();
	while (iter() != nullptr && iter()->StatementID != StatementID) {
		iter--;
	}

	TStatement *Statement = iter();
	if (Statement == nullptr) {
		return 1;
	}

	if (CharacterID == 0) {
		if ((Statement->Mode != TALK_CHANNEL_CALL && Statement->Mode != TALK_GAMEMASTER_CHANNELCALL && Statement->Mode != TALK_HIGHLIGHT_CHANNELCALL)
				|| (!in(Statement->Channel, GAME_CHANNEL, HELP_CHANNEL) && Statement->Channel != TUTOR_CHANNEL)) {
			error("GetCommunicationContext: Statement may not be reported.\n");
			return 1;
		}
	}

	if (Statement->Reported) {
		return 2;
	}

	bool StatementContained = false;
	NumberOfStatements = 0;
	int FreeSpace = std::max(MaxPlayers * 66 + 2, 16384) - 287;
	ReportedStatements = new vector<TReportedStatement>(0, 100, 100);
	fifoIterator<TListener> iter2 = Listeners.tail();
	for (iter = Statements.tail(); iter() != nullptr; iter++) {
		if (!in(iter()->TimeStamp, Statement->TimeStamp - 180, Statement->TimeStamp + 60)) {
			continue;
		}

		if (FreeSpace <= 0 && iter()->TimeStamp > Statement->TimeStamp) {
			error("GetCommunicationContext: Context becomes too large. Cutting off end.\n");
			break;
		}

		if (CharacterID != 0) {
			while (iter2() != nullptr && iter2()->StatementID < iter()->StatementID) {
				iter2++;
			}
			while (iter2() != nullptr && iter2()->StatementID == iter()->StatementID && iter2()->CharacterID != CharacterID) {
				iter2++;
			}
			if (iter2() == nullptr || iter2()->StatementID > iter()->StatementID) {
				continue;
			}
		} else {
			if (iter()->Mode != TALK_CHANNEL_CALL && iter()->Mode != TALK_GAMEMASTER_CHANNELCALL && iter()->Mode != TALK_HIGHLIGHT_CHANNELCALL) {
				continue;
			}
			if (Statement->Channel != iter()->Channel) {
				continue;
			}
		}

		(*ReportedStatements)(NumberOfStatements).StatementID = iter()->StatementID;
		(*ReportedStatements)(NumberOfStatements).TimeStamp = iter()->TimeStamp;
		(*ReportedStatements)(NumberOfStatements).CharacterID = iter()->CharacterID;
		(*ReportedStatements)(NumberOfStatements).Mode = iter()->Mode;
		(*ReportedStatements)(NumberOfStatements).Channel = iter()->Channel;
		if (iter()->Text == 0) {
			(*ReportedStatements)(NumberOfStatements).Text[0] = 0;
		} else {
			strcpy((*ReportedStatements)(NumberOfStatements).Text, GetDynamicString(iter()->Text));
		}
		FreeSpace -= (int)strlen((*ReportedStatements)(NumberOfStatements).Text) + 46;
		NumberOfStatements++;
		if (iter()->StatementID == StatementID) {
			StatementContained = true;
		}
	}

	if (FreeSpace < 0) {
		error("GetCommunicationContext: Context becomes %d bytes too large. Deleting start.\n", -FreeSpace);
		for (int i = 0; i < NumberOfStatements && FreeSpace < 0; i++) {
			if ((*ReportedStatements)(i).StatementID != StatementID) {
				(*ReportedStatements)(i).StatementID = 0;
				FreeSpace += (int)strlen((*ReportedStatements)(i).Text) + 46;
			}
		}
		if (FreeSpace < 0) {
			error("GetCommunicationContext: FreeSpace is still negative (%d).\n", FreeSpace);
		}
	}

	if (!StatementContained) {
		error("GetCommunicationContext: Statement is not contained in context.\n");
		delete ReportedStatements;
		return 1;
	}

	Statement->Reported = true;
	return 0;
}

TParty *GetParty(uint32_t Leader) {
	for (int i = 0; i < Parties; i++) {
		if (Party(i).Leader == Leader) {
			return &Party(i);
		}
	}

	return nullptr;
}

bool IsInvitedToParty(uint32_t Guest, uint32_t Host) {
	TParty *P = GetParty(Host);
	if (P != nullptr) {
		for (int i = 0; i < P->InvitedPlayers; i++) {
			if (Guest == P->InvitedPlayer(i)) {
				return true;
			}
		}
	}

	return false;
}

void DisbandParty(uint32_t Leader) {
	TPlayer *leader = GetPlayer(Leader);
	TParty *P = GetParty(Leader);
	if (P == nullptr) {
		error("DisbandParty: Party of leader %lu not found.\n", (unsigned long)Leader);
		return;
	}

	for (int i = 0; i < P->Members; i++) {
		TPlayer *pl = GetPlayer(P->Member(i));
		if (pl != nullptr) {
			pl->LeaveParty();
		}
	}

	P->Leader = 0;

	for (int i = 0; i < P->Members; i++) {
		TPlayer *pl = GetPlayer(P->Member(i));
		if (pl != nullptr) {
			SendMessage(pl->GetConnection(), TALK_INFO_MESSAGE, "Your party has been disbanded.");
			for (int j = 0; j < P->Members; j++) {
				SendCreatureParty(pl->GetConnection(), P->Member(j));
				SendCreatureSkull(pl->GetConnection(), P->Member(j));
			}
		}
	}

	if (leader != nullptr) {
		for (int i = 0; i < P->InvitedPlayers; i++) {
			TPlayer *pl = GetPlayer(P->InvitedPlayer(i));
			if (pl != nullptr) {
				SendCreatureParty(pl->GetConnection(), Leader);
				SendCreatureParty(leader->GetConnection(), pl->ID);
			}
		}
	}
}

void InviteToParty(uint32_t Host, uint32_t Guest) {
	if (Host == Guest) {
		return;
	}

	TPlayer *host = GetPlayer(Host);
	if (host == nullptr) {
		error("InviteToParty: Inviting player %lu does not exist.\n", (unsigned long)Host);
		return;
	}

	TPlayer *guest = GetPlayer(Guest);
	if (guest == nullptr) {
		SendResult(host->GetConnection(), PLAYERNOTONLINE);
		return;
	}

	if (host->GetPartyLeader(true) == 0) {
		if (guest->GetPartyLeader(true) != 0) {
			SendMessage(host->GetConnection(), TALK_INFO_MESSAGE, "%s is already member of a party.", guest->Name);
			return;
		}

		TParty *P = GetParty(0);
		if (P == nullptr) {
			P = &Party(Parties);
			Parties++;
		}

		P->Leader = Host;
		P->Member(0) = Host;
		P->Members = 1;
		P->InvitedPlayer(0) = Guest;
		P->InvitedPlayers = 1;
		host->JoinParty(Host);
		SendMessage(host->GetConnection(), TALK_INFO_MESSAGE, "%s has been invited.", guest->Name);
		SendMessage(guest->GetConnection(), TALK_INFO_MESSAGE, "%s invites you to %s party.", host->Name, (host->Sex == SEX_MALE ? "his" : "her"));
		SendCreatureParty(host->GetConnection(), Host);
		SendCreatureSkull(host->GetConnection(), Host);
		SendCreatureParty(host->GetConnection(), Guest);
		SendCreatureParty(guest->GetConnection(), Host);
	} else if (host->GetPartyLeader(false) == Host) {
		if (guest->GetPartyLeader(true) != 0) {
			SendMessage(host->GetConnection(), TALK_INFO_MESSAGE, "%s is already member of %s party.", guest->Name, (guest->GetPartyLeader(true) == Host ? "your" : "a"));
			return;
		}

		TParty *P = GetParty(Host);
		if (P == nullptr) {
			error("InviteToParty: Party of leader %lu not found.\n", (unsigned long)Host);
			return;
		}

		for (int i = 0; i < P->InvitedPlayers; i++) {
			if (P->InvitedPlayer(i) == Guest) {
				SendMessage(host->GetConnection(), TALK_INFO_MESSAGE, "%s has already been invited.", guest->Name);
				return;
			}
		}

		P->InvitedPlayer(P->InvitedPlayers) = Guest;
		P->InvitedPlayers++;
		SendMessage(host->GetConnection(), TALK_INFO_MESSAGE, "%s has been invited.", guest->Name);
		SendMessage(guest->GetConnection(), TALK_INFO_MESSAGE, "%s invites you to %s party.", host->Name, (host->Sex == SEX_MALE ? "his" : "her"));
		SendCreatureParty(host->GetConnection(), Guest);
		SendCreatureParty(guest->GetConnection(), Host);
	} else {
		SendMessage(host->GetConnection(), TALK_INFO_MESSAGE, "You may not invite players.");
	}
}

void RevokeInvitation(uint32_t Host, uint32_t Guest) {
	if (Host == Guest) {
		return;
	}

	TPlayer *host = GetPlayer(Host);
	if (host == nullptr) {
		error("RevokeInvitation: Inviting player %lu does not exist.\n", (unsigned long)Host);
		return;
	}

	TPlayer *guest = GetPlayer(Guest);
	if (host->GetPartyLeader(false) != Host) {
		SendMessage(host->GetConnection(), TALK_INFO_MESSAGE, "You may not invite players.");
		return;
	}

	TParty *P = GetParty(Host);
	if (P == nullptr) {
		error("RevokeInvitation: Party of leader %lu not found.\n", (unsigned long)Host);
		return;
	}

	int Position = -1;
	for (int i = 0; i < P->InvitedPlayers; i++) {
		if (P->InvitedPlayer(i) == Guest) {
			Position = i;
		}
	}

	if (Position == -1) {
		if (guest != nullptr) {
			SendMessage(host->GetConnection(), TALK_INFO_MESSAGE, "%s has not been invited.", guest->Name);
		} else {
			SendMessage(host->GetConnection(), TALK_INFO_MESSAGE, "This player has not been invited.");
		}
		return;
	}

	for (int i = Position; i < P->InvitedPlayers - 1; i++) {
		P->InvitedPlayer(i) = P->InvitedPlayer(i + 1);
	}
	P->InvitedPlayers--;

	if (guest != nullptr) {
		SendMessage(host->GetConnection(), TALK_INFO_MESSAGE, "Invitation for %s has been revoked.", guest->Name);
		SendMessage(guest->GetConnection(), TALK_INFO_MESSAGE, "%s has revoked %s invitation.", host->Name, (host->Sex == SEX_MALE ? "his" : "her"));
		SendCreatureParty(host->GetConnection(), Guest);
		SendCreatureParty(guest->GetConnection(), Host);
	} else {
		SendMessage(host->GetConnection(), TALK_INFO_MESSAGE, "Invitation has been revoked.");
	}

	if (P->Members == 1 && P->InvitedPlayers == 0) {
		DisbandParty(Host);
	}
}

void JoinParty(uint32_t Guest, uint32_t Host) {
	if (Guest == Host) {
		return;
	}

	TPlayer *guest = GetPlayer(Guest);
	if (guest == nullptr) {
		error("JoinParty: Invited player %lu does not exist.\n", (unsigned long)Guest);
		return;
	}

	TPlayer *host = GetPlayer(Host);
	if (host == nullptr) {
		SendResult(guest->GetConnection(), PLAYERNOTONLINE);
		return;
	}

	if (guest->GetPartyLeader(true) != 0) {
		SendMessage(guest->GetConnection(), TALK_INFO_MESSAGE, "You are already member of %s party.", (guest->GetPartyLeader(true) == host->GetPartyLeader(true) ? "this" : "a"));
		return;
	}

	TParty *P = GetParty(Host);
	int Position = -1;
	if (P != nullptr) {
		for (int i = 0; i < P->InvitedPlayers; i++) {
			if (P->InvitedPlayer(i) == Guest) {
				Position = i;
			}
		}
	}

	if (Position == -1) {
		SendMessage(guest->GetConnection(), TALK_INFO_MESSAGE, "%s has not invited you.", host->Name);
		return;
	}

	P->Member(P->Members) = Guest;
	P->Members++;
	P->InvitedPlayers--;
	P->InvitedPlayer(Position) = P->InvitedPlayer(P->InvitedPlayers);
	guest->JoinParty(Host);
	SendMessage(guest->GetConnection(), TALK_INFO_MESSAGE, "You have joined %s's party.", host->Name);
	SendCreatureParty(guest->GetConnection(), Guest);
	SendCreatureSkull(guest->GetConnection(), Guest);
	for (int i = 0; i < P->Members; i++) {
		if (P->Member(i) != Guest) {
			SendCreatureParty(guest->GetConnection(), P->Member(i));
			SendCreatureSkull(guest->GetConnection(), P->Member(i));
			TPlayer *pl = GetPlayer(P->Member(i));
			if (pl != nullptr) {
				SendMessage(pl->GetConnection(), TALK_INFO_MESSAGE, "%s has joined the party.", guest->Name);
				SendCreatureParty(pl->GetConnection(), Guest);
				SendCreatureSkull(pl->GetConnection(), Guest);
			}
		}
	}
}

void PassLeadership(uint32_t OldLeader, uint32_t NewLeader) {
	if (OldLeader == NewLeader) {
		return;
	}

	TPlayer *oldLeader = GetPlayer(OldLeader);
	if (oldLeader == nullptr) {
		error("PassLeadership: Old leader %lu does not exist.\n", (unsigned long)OldLeader);
		return;
	}

	TPlayer *newLeader = GetPlayer(NewLeader);
	if (newLeader == nullptr) {
		SendResult(oldLeader->GetConnection(), PLAYERNOTONLINE);
		return;
	}

	if (oldLeader->GetPartyLeader(false) != OldLeader) {
		SendMessage(oldLeader->GetConnection(), TALK_INFO_MESSAGE, "You are not leader of a party.");
		return;
	}

	if (newLeader->GetPartyLeader(false) != OldLeader) {
		SendMessage(oldLeader->GetConnection(), TALK_INFO_MESSAGE, "%s is not member of your party.", newLeader->Name);
		return;
	}

	TParty *P = GetParty(OldLeader);
	if (P == nullptr) {
		error("PassLeadership: Party of leader %lu not found.\n", (unsigned long)OldLeader);
		return;
	}

	P->Leader = NewLeader;
	for (int i = 0; i < P->Members; i++) {
		TPlayer *pl = GetPlayer(P->Member(i));
		if (pl != nullptr) {
			pl->JoinParty(NewLeader);
		}
	}

	for (int i = 0; i < P->Members; i++) {
		TPlayer *pl = GetPlayer(P->Member(i));
		if (pl != nullptr) {
			if (pl == newLeader) {
				SendMessage(pl->GetConnection(), TALK_INFO_MESSAGE, "You are now leader of your party.");
			} else {
				SendMessage(pl->GetConnection(), TALK_INFO_MESSAGE, "%s is now leader of your party.", newLeader->Name);
			}
			SendCreatureParty(pl->GetConnection(), OldLeader);
			SendCreatureParty(pl->GetConnection(), NewLeader);
		}
	}

	int InvitedPlayers = P->InvitedPlayers;
	P->InvitedPlayers = 0;
	for (int i = 0; i < InvitedPlayers; i++) {
		TPlayer *pl = GetPlayer(P->InvitedPlayer(i));
		if (pl != nullptr) {
			SendCreatureParty(pl->GetConnection(), OldLeader);
			SendCreatureParty(oldLeader->GetConnection(), pl->ID);
		}
	}
}

void LeaveParty(uint32_t Member, bool Forced) {
	TPlayer *member = GetPlayer(Member);
	if (member == nullptr) {
		error("LeaveParty: Member does not exist.\n");
		return;
	}

	if (member->GetPartyLeader(false) == 0) {
		error("LeaveParty: Player is not a member of a party.\n");
		return;
	}

	if (!Forced && member->EarliestLogoutRound > RoundNr) {
		SendMessage(member->GetConnection(), TALK_INFO_MESSAGE, "You may not leave your party during or immediately after a fight!");
		return;
	}

	TParty *P = GetParty(member->GetPartyLeader(false));
	if (P == nullptr) {
		error("LeaveParty: Party of leader %lu not found.\n", (unsigned long)member->GetPartyLeader(false));
		return;
	}

	if (P->Members == 1 || (P->Members == 2 && P->InvitedPlayers == 0)) {
		DisbandParty(P->Leader);
		return;
	}

	if (P->Leader == Member) {
		uint32_t NewLeader = P->Member(0);
		if (NewLeader == Member) {
			NewLeader = P->Member(1);
		}
		PassLeadership(Member, NewLeader);
	}

	int Position = -1;
	for (int i = 0; i < P->Members; i++) {
		if (P->Member(i) == Member) {
			Position = i;
		}
	}

	if (Position == -1) {
		error("LeaveParty: Member not found.\n");
		return;
	}

	for (int i = Position; i < P->Members - 1; i++) {
		P->Member(i) = P->Member(i + 1);
	}
	P->Members--;

	member->LeaveParty();
	if (!Forced) {
		SendMessage(member->GetConnection(), TALK_INFO_MESSAGE, "You have left the party.");
	}
	SendCreatureParty(member->GetConnection(), Member);
	SendCreatureSkull(member->GetConnection(), Member);
	for (int i = 0; i < P->Members; i++) {
		SendCreatureParty(member->GetConnection(), P->Member(i));
		SendCreatureSkull(member->GetConnection(), P->Member(i));
		TPlayer *pl = GetPlayer(P->Member(i));
		if (pl != nullptr) {
			SendMessage(pl->GetConnection(), TALK_INFO_MESSAGE, "%s has left the party.", member->Name);
			SendCreatureParty(pl->GetConnection(), Member);
			SendCreatureSkull(pl->GetConnection(), Member);
		}
	}
}
