#include <algorithm>
#include <climits>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#include "script.hh"
#include "config.hh"
#include "threads.hh"
#include "dbfuncs.hh"
#include "objects.hh"
#include "map.hh"
#include "info.hh"
#include "cr.hh"
#include "crmain.hh"
#include "crplayer.hh"
#include "containr.hh"
#include "strings.hh"
#include "utils.hh"
#include "time.hh"

const char *GetName(Object Obj) {
	char ObjectName[50];
	const char *Help;

	ObjectName[0] = 0;
	if (Obj.isCreature()) {
		TCreature *cr = GetCreature(Obj);
		if (cr != nullptr) {
			snprintf(ObjectName, sizeof(ObjectName), "%s", cr->Name);
		} else {
			error("GetName: Kreatur %d existiert nicht.\n", Obj.getCreatureID());
		}
	} else {
		Help = Obj.getObjectType().getName(1);
		if (Help != nullptr) {
			strcpy(ObjectName, Help);
		}

		if (Obj.getFlag(LIQUIDCONTAINER) && Obj.getAttribute(CONTAINERLIQUIDTYPE) != LIQUID_NONE) {
			strcat(ObjectName, " of ");
			strcat(ObjectName, GetLiquidName(Obj.getAttribute(CONTAINERLIQUIDTYPE)));
		}

		if (Obj.getFlag(LIQUIDPOOL) && Obj.getAttribute(POOLLIQUIDTYPE) != LIQUID_NONE) {
			strcat(ObjectName, " of ");
			strcat(ObjectName, GetLiquidName(Obj.getAttribute(POOLLIQUIDTYPE)));
		}
	}

	return Plural(ObjectName, Obj.getFlag(CUMULATIVE) ? Obj.getAttribute(AMOUNT) : 1);
}

const char *GetInfo(Object Obj) {
	if (!Obj.exists()) {
		error("GetInfo: \xdc" "bergebenes Objekt existiert nicht.\n");
		return nullptr;
	}

	return Obj.getObjectType().getDescription();
}

int GetWeight(Object Obj, int Count) {
	if (!Obj.exists()) {
		error("GetWeight: \xdc" "bergebenes Objekt existiert nicht.\n");
		return 0;
	}

	if (!Obj.getFlag(TAKE)) {
		if (Obj.getObjectType().getTypeID() == 2904) {
			return 19500;
		}
		if (Obj.getObjectType().getTypeID() == 3458) {
			return 50000;
		}
		if (Obj.getObjectType().getTypeID() == 3510) {
			return 22800;
		}
		if (Obj.getObjectType().getTypeID() == 4311) {
			return 80000;
		}

		error("GetWeight: Objekttyp %d ist nicht nehmbar.\n", Obj.getObjectType().getTypeID());
		return 0;
	}

	int Weight = Obj.getAttribute(WEIGHT);
	if (Obj.getFlag(CUMULATIVE)) {
		if (Count == -1) {
			Count = Obj.getAttribute(AMOUNT);
		}
		Weight *= Count;
	}

	return Weight;
}

int GetRowWeight(Object Obj) {
	if (Obj == NONE) {
		return 0;
	}

	int Weight = 0;
	while (Obj != NONE) {
		Weight += GetCompleteWeight(Obj);
		Obj++;
	}

	return Weight;
}

int GetCompleteWeight(Object Obj) {
	int Weight = GetWeight(Obj, -1);
	if (Obj.getFlag(CONTAINER) || Obj.getFlag(CHEST)) {
		Weight += GetRowWeight(GetFirstContainerObject(Obj));
	}

	return Weight;
}

uint32_t GetObjectCreatureID(Object Obj) {
	if (!Obj.exists()) {
		error("GetObjectCreatureID: \xdc" "bergebenes Objekt existiert nicht.\n");
		return 0;
	}

	while (!Obj.isCreature()) {
		if (Obj.isMapContainer()) {
			return 0;
		}
		Obj = Obj.getContainer();
	}

	return Obj.getCreatureID();
}

int GetObjectBodyPosition(Object Obj) {
	if (!Obj.exists()) {
		error("GetObjectBodyPosition: \xdc" "bergebenes Objekt existiert nicht.\n");
		return 0;
	}

	while (!Obj.isMapContainer() && !Obj.isBodyContainer()) {
		Obj = Obj.getContainer();
	}

	return Obj.getObjectType().getTypeID();
}

int GetObjectRNum(Object Obj) {
	if (!Obj.exists()) {
		error("GetObjectRNum: \xdc" "bergebenes Objekt existiert nicht.\n");
		return 0;
	}

	if (Obj.isMapContainer()) {
		error("GetObjectRNum: Objekt ist MapContainer.\n");
		return 0;
	}

	Object Search = GetFirstContainerObject(Obj.getContainer());
	int Count = 0;
	while (Search != Obj) {
		if (Search == NONE) {
			error("GetObjectRNum: Objekt liegt nicht in Container\n");
			return 0;
		}
		Count++;
		Search++;
	}

	return Count;
}

bool ObjectInRange(uint32_t CreatureID, Object Obj, int Range) {
	int x, y, z;
	TCreature *cr = GetCreature(CreatureID);
	if (cr == nullptr) {
		error("ObjectInRange: Ung\xfcltige Kreatur CreatureID=%d \xfc" "bergeben.\n", CreatureID);
		return false;
	}

	if (!Obj.exists()) {
		error("ObjectInRange: \xdc" "bergebenes Objekt existiert nicht.\n");
		return false;
	}

	GetObjectCoordinates(Obj, x, y, z);
	return abs(cr->posx - x) <= Range && abs(cr->posy - y) <= Range && cr->posz == z;
}

bool ObjectAccessible(uint32_t CreatureID, Object Obj, int Range) {
	if (!Obj.exists()) {
		error("ObjectAccessible: \xdc" "bergebenes Objekt existiert nicht.\n");
		return false;
	}

	uint32_t ID = 0;
	if (!Obj.isCreature()) {
		ID = GetObjectCreatureID(Obj);
	}

	if (ID == CreatureID) {
		return true;
	}

	if (ID != 0) {
		return false;
	}

	if (Obj.getFlag(HANG)) {
		int x, y, z;
		GetObjectCoordinates(Obj, x, y, z);
		if (CoordinateHookable(x, y, z)) {
			TCreature *cr = GetCreature(CreatureID);
			if (cr == nullptr) {
				error("ObjectAccessible: Kreatur existiert nicht.\n");
				return false;
			}

			if (CoordinateFlag(x, y, z, HOOKSOUTH)) {
				if (!in(cr->posy, y, y + Range) || !in(cr->posx, x - Range, x + Range)) {
					return false;
				}
			}

			if (CoordinateFlag(x, y, z, HOOKEAST)) {
				if (!in(cr->posx, x, x + Range) || !in(cr->posy, y - Range, y + Range)) {
					return false;
				}
			}
		}
	}

	return ObjectInRange(CreatureID, Obj, Range);
}

const char *GetLiquidName(uint32_t nr) {
	switch (nr) {
		case LIQUID_NONE: return "nothing";
		case LIQUID_WATER: return "water";
		case LIQUID_WINE: return "wine";
		case LIQUID_BEER: return "beer";
		case LIQUID_MUD: return "mud";
		case LIQUID_BLOOD: return "blood";
		case LIQUID_SLIME: return "slime";
		case LIQUID_OIL: return "oil";
		case LIQUID_URINE: return "urine";
		case LIQUID_MILK: return "milk";
		case LIQUID_MANAFLUID: return "manafluid";
		case LIQUID_LIFEFLUID: return "lifefluid";
		case LIQUID_LEMONADE: return "lemonade";
		default: {
			error("GetLiquidName: Ung\xfcltiger Fl\xfcssigkeitstyp %d\n", nr);
			return "unknown";
		}
	}
}

unsigned char GetLiquidColor(uint32_t nr) {
	switch (nr) {
		case LIQUID_NONE: return LIQUID_COLOR_NONE;
		case LIQUID_WATER: return LIQUID_COLOR_BLUE;
		case LIQUID_WINE: return LIQUID_COLOR_PURPLE;
		case LIQUID_BEER: return LIQUID_COLOR_BROWN;
		case LIQUID_MUD: return LIQUID_COLOR_BROWN;
		case LIQUID_BLOOD: return LIQUID_COLOR_RED;
		case LIQUID_SLIME: return LIQUID_COLOR_GREEN;
		case LIQUID_OIL: return LIQUID_COLOR_BROWN;
		case LIQUID_URINE: return LIQUID_COLOR_YELLOW;
		case LIQUID_MILK: return LIQUID_COLOR_WHITE;
		case LIQUID_MANAFLUID: return LIQUID_COLOR_PURPLE;
		case LIQUID_LIFEFLUID: return LIQUID_COLOR_RED;
		case LIQUID_LEMONADE: return LIQUID_COLOR_YELLOW;
		default: {
			error("GetLiquidColor: Ung\xfcltiger Fl\xfcssigkeitstyp %d\n", nr);
			return LIQUID_COLOR_NONE;
		}
	}
}

int ObjectDistance(Object Obj1, Object Obj2) {
	if (!Obj1.exists() || !Obj2.exists()) {
		error("ObjectDistance: \xdc" "bergebene Objekte existieren nicht.\n");
		return INT_MAX;
	}

	int x1, y1, z1, x2, y2, z2;
	GetObjectCoordinates(Obj1, x1, y1, z1);
	GetObjectCoordinates(Obj2, x2, y2, z2);
	if (z1 != z2) {
		return INT_MAX;
	}

	return std::max(abs(x1 - x2), abs(y1 - y2));
}

Object GetBodyContainer(uint32_t CreatureID, int Position) {
	if ((Position < FIRST_BODY_POSITION || Position > LAST_BODY_POSITION)
			&& (Position < POSITION_CONTAINER_BASE || Position > POSITION_CONTAINER_BASE + MAX_OPEN_CONTAINERS - 1)) {
		error("GetBodyContainer: ung\xfcltige Position: %d\n", Position);
		return NONE;
	}

	TCreature *cr = GetCreature(CreatureID);
	if (cr == nullptr) {
		error("GetBodyContainer: Kreatur %d existiert nicht.\n", CreatureID);
		return NONE;
	}

	if (cr->GetCreatureObject() == NONE) {
		return NONE;
	}

	if (Position >= POSITION_CONTAINER_BASE) {
		if (cr->Type != PLAYER) {
			error("GetBodyContainer: Nur Spieler haben offene Container.\n");
			return NONE;
		}

		return static_cast<TPlayer *>(cr)->GetOpenContainer(Position - POSITION_CONTAINER_BASE);
	}

	// As in the original, the error does not end the function.
	if (!cr->GetCreatureObject().exists()) {
		error("GetBodyContainer: Kreatur-Objekt von %s existiert nicht (Pos %d).\n", cr->Name, Position);
	}

	return GetContainerObject(cr->GetCreatureObject(), Position - FIRST_BODY_POSITION);
}

Object GetBodyObject(uint32_t CreatureID, int Position) {
	if (Position < FIRST_BODY_POSITION || Position > LAST_BODY_POSITION) {
		error("GetBodyObject: ung\xfcltige Position %d\n", Position);
		return NONE;
	}

	Object BodyCon = GetBodyContainer(CreatureID, Position);
	if (BodyCon == NONE) {
		return NONE;
	}

	return GetFirstContainerObject(BodyCon);
}

Object GetTopObject(int x, int y, int z, bool Move) {
	Object Obj = GetFirstObject(x, y, z);
	while (Obj != NONE) {
		if (Obj.getNextObject() == NONE) {
			break;
		}

		if (!Obj.getFlag(BANK) && !Obj.getFlag(CLIP) && !Obj.getFlag(BOTTOM) && !Obj.getFlag(TOP)
				&& (!Move || !Obj.isCreature())) {
			break;
		}

		Obj++;
	}

	return Obj;
}

Object GetObject(uint32_t CreatureID, int x, int y, int z, int RNum, ObjectType Type) {
	Object Obj;
	if (x == POSITION_NOT_ON_MAP) {
		if (y == 0) {
			return NONE;
		}

		if ((y > LAST_BODY_POSITION && y < POSITION_CONTAINER_BASE) || y > POSITION_CONTAINER_BASE + MAX_OPEN_CONTAINERS - 1) {
			error("GetObject: Ung\xfcltiger ContainerCode x=%d,y=%d,z=%d,RNum=%d,Type=%d.\n",
					x, y, z, RNum, Type.getTypeID());
			return NONE;
		}

		if (y >= POSITION_CONTAINER_BASE) {
			TCreature *cr = GetCreature(CreatureID);
			if (cr == nullptr) {
				error("GetObject: Kreatur %d existiert nicht.\n", CreatureID);
				return NONE;
			}

			if (cr->Type != PLAYER) {
				error("GetObject: Nur Spieler haben offene Container.\n");
				return NONE;
			}

			Object Con = static_cast<TPlayer *>(cr)->GetOpenContainer(y - POSITION_CONTAINER_BASE);
			if (Con == NONE) {
				return NONE;
			}

			Obj = GetContainerObject(Con, RNum);
		} else {
			Obj = GetBodyObject(CreatureID, y);
		}
	} else if (RNum != -1) {
		Obj = GetFirstObject(x, y, z);
		while (Obj != NONE) {
			if (Obj.getFlag(DISGUISE)) {
				ObjectType Disguise(Obj.getAttribute(DISGUISETARGET));
				if (Disguise == Type) {
					break;
				}
			} else if (Obj.getObjectType() == Type) {
				break;
			}

			Obj++;
		}
	} else {
		Obj = GetTopObject(x, y, z, false);
	}

	if (Obj == NONE) {
		return NONE;
	}

	if (Type == ObjectType(0)) {
		return Obj;
	}

	if (Obj.getFlag(DISGUISE)) {
		ObjectType Disguise(Obj.getAttribute(DISGUISETARGET));
		if (Disguise == Type) {
			return Obj;
		}
	} else if (Obj.getObjectType() == Type) {
		return Obj;
	}

	return NONE;
}

Object GetRowObject(Object Obj, ObjectType Type, uint32_t Value, bool Recurse) {
	while (Obj != NONE) {
		if (Recurse && Obj.getFlag(CONTAINER)) {
			Object Help = GetRowObject(GetFirstContainerObject(Obj), Type, Value, Recurse);
			if (Help != NONE) {
				return Help;
			}
		}

		if (!(Obj.getObjectType() != Type)
				&& (!Type.getFlag(LIQUIDCONTAINER) || Obj.getAttribute(CONTAINERLIQUIDTYPE) == Value)
				&& (!Type.getFlag(KEY) || Obj.getAttribute(KEYNUMBER) == Value)) {
			return Obj;
		}

		Obj++;
	}

	return NONE;
}

Object GetInventoryObject(uint32_t CreatureID, ObjectType Type, uint32_t Value) {
	TCreature *cr = GetCreature(CreatureID);
	if (cr == nullptr) {
		error("GetInventoryObject: Kreatur %d existiert nicht.\n", CreatureID);
		return NONE;
	}

	for (Object BodyCon = GetFirstContainerObject(cr->GetCreatureObject()); BodyCon != NONE; BodyCon++) {
		Object BodyObj = GetFirstContainerObject(BodyCon);
		if (BodyObj != NONE && BodyObj.getFlag(CONTAINER)) {
			Object Obj = GetRowObject(GetFirstContainerObject(BodyObj), Type, Value, true);
			if (Obj != NONE) {
				return Obj;
			}
		}
	}

	for (Object BodyCon = GetFirstContainerObject(cr->GetCreatureObject()); BodyCon != NONE; BodyCon++) {
		Object Obj = GetRowObject(GetFirstContainerObject(BodyCon), Type, Value, false);
		if (Obj != NONE) {
			return Obj;
		}
	}

	return NONE;
}

int CountObjectsInContainer(Object Con) {
	if (!Con.exists()) {
		error("CountObjectsInContainer: Container existiert nicht.\n");
		return 0;
	}

	int Count = 0;
	Object Search = GetFirstContainerObject(Con);
	while (Search != NONE) {
		Count++;
		Search++;
	}

	return Count;
}

int CountObjects(Object Obj) {
	if (!Obj.exists()) {
		return 0;
	}

	int count = 1;
	if (Obj.getFlag(CONTAINER)) {
		Obj = GetFirstContainerObject(Obj);
		while (Obj != NONE) {
			count += CountObjects(Obj);
			Obj++;
		}
	}

	return count;
}

int CountObjects(Object Obj, ObjectType Type, uint32_t Value) {
	int count = 0;
	if (!Obj.exists()) {
		return 0;
	}

	while (Obj != NONE) {
		// As in the original, objects inside containers are counted with Value 0.
		if (Obj.getFlag(CONTAINER)) {
			count += CountObjects(GetFirstContainerObject(Obj), Type, 0);
		}

		if (!(Obj.getObjectType() != Type)
				&& (!Type.getFlag(LIQUIDCONTAINER) || Obj.getAttribute(CONTAINERLIQUIDTYPE) == Value)
				&& (!Type.getFlag(KEY) || Obj.getAttribute(KEYNUMBER) == Value)) {
			if (Obj.getFlag(CUMULATIVE)) {
				count += Obj.getAttribute(AMOUNT);
			} else {
				count++;
			}
		}

		Obj++;
	}

	return count;
}

int CountInventoryObjects(uint32_t CreatureID, ObjectType Type, uint32_t Value) {
	TCreature *cr = GetCreature(CreatureID);
	if (cr == nullptr) {
		error("CountInventoryObjects: Kreatur %d existiert nicht; Objekttyp %d.\n", CreatureID, Type.getTypeID());
		return 0;
	}

	if (cr->GetCreatureObject() == NONE) {
		error("CountInventoryObjects: Kreatur %s hat kein Kreatur-Objekt.\n", cr->Name);
		return 0;
	}

	return CountObjects(GetFirstContainerObject(cr->GetCreatureObject()), Type, Value);
}

int CountMoney(Object Obj) {
	int count = 0;
	if (!Obj.exists()) {
		return 0;
	}

	while (Obj != NONE) {
		// Written as member calls so that the object type is read before
		// GetSpecialObject is called, as in the original.
		if (Obj.getObjectType().operator==(GetSpecialObject(MONEY_ONE))) {
			count += Obj.getAttribute(AMOUNT);
		}

		if (Obj.getObjectType().operator==(GetSpecialObject(MONEY_HUNDRED))) {
			count += Obj.getAttribute(AMOUNT) * 100;
		}

		if (Obj.getObjectType().operator==(GetSpecialObject(MONEY_TENTHOUSAND))) {
			count += Obj.getAttribute(AMOUNT) * 10000;
		}

		if (Obj.getFlag(CONTAINER)) {
			count += CountMoney(GetFirstContainerObject(Obj));
		}

		Obj++;
	}

	return count;
}

int CountInventoryMoney(uint32_t CreatureID) {
	TCreature *cr = GetCreature(CreatureID);
	if (cr == nullptr) {
		error("CountInventoryMoney: Kreatur %d existiert nicht.\n", CreatureID);
		return 0;
	}

	if (cr->GetCreatureObject() == NONE) {
		error("CountInventoryMoney: Kreatur %s hat kein Kreatur-Objekt.\n", cr->Name);
		return 0;
	}

	return CountMoney(GetFirstContainerObject(cr->GetCreatureObject()));
}

void CalculateChange(int Amount, int &Gold, int &Platinum, int &Crystal) {
	print(3, "Zahle %d mit %d/%d/%d M\xfcnzen...\n", Amount, Gold, Platinum, Crystal);
	if (Crystal * 10000 + Platinum * 100 + Gold < Amount) {
		error("CalculateChange: %d/%d/%d M\xfcnzen reichen nicht zur Bezahlung von %d.\n",
				Gold, Platinum, Crystal, Amount);
		return;
	}

	if (Platinum * 100 + Gold < Amount % 10000) {
		int Change = Amount % 10000 - 10000;
		Crystal = Amount / 10000 + 1;
		Platinum = Change / 100;
		Gold = Change % 100;
	} else {
		int Rest;
		if (Crystal < Amount / 10000) {
			Rest = Amount - Crystal * 10000;
		} else {
			Crystal = Amount / 10000;
			Rest = Amount % 10000;
		}

		if (Gold < Rest % 100) {
			Platinum = Rest / 100 + 1;
			Gold = Rest % 100 - 100;
		} else if (Platinum < Rest / 100) {
			Gold = Rest - Platinum * 100;
		} else {
			Platinum = Rest / 100;
			Gold = Rest % 100;
		}
	}

	print(3, "Verwende %d/%d/%d M\xfcnzen.\n", Gold, Platinum, Crystal);
	if (Crystal * 10000 + Platinum * 100 + Gold != Amount) {
		error("CalculateChange: Fehlerhafte Berechnung: %d/%d/%d M\xfcnzen f\xfcr %d.\n",
				Gold, Platinum, Crystal, Amount);
	}
}

int GetHeight(int x, int y, int z) {
	int Height = 0;
	Object Obj = GetFirstObject(x, y, z);
	while (Obj != NONE) {
		if (Obj.getFlag(HEIGHT)) {
			Height += Obj.getAttribute(ELEVATION);
		}
		Obj++;
	}

	return Height;
}

bool JumpPossible(int x, int y, int z, bool AvoidPlayers) {
	Object Obj = GetFirstObject(x, y, z);
	if (Obj == NONE) {
		return false;
	}

	bool BankThere = false;
	while (Obj != NONE) {
		if (Obj.getFlag(BANK)) {
			BankThere = true;
		}

		if (Obj.getFlag(UNPASS) && Obj.getFlag(UNMOVE)) {
			return false;
		}

		if (AvoidPlayers && Obj.isCreature()) {
			TCreature *cr = GetCreature(Obj);
			if (cr != nullptr && cr->Type == PLAYER) {
				return false;
			}
		}

		Obj++;
	}

	return BankThere;
}

bool FieldPossible(int x, int y, int z, int Type) {
	Object Obj = GetFirstObject(x, y, z);
	if (Obj == NONE || !Obj.getFlag(BANK)) {
		return false;
	}

	while (Obj != NONE) {
		if (!Obj.isCreature() && Obj.getFlag(UNPASS)) {
			return false;
		}

		if (!Obj.isCreature() && Obj.getFlag(UNLAY)) {
			return false;
		}

		if ((Type == FIELD_MAGICWALL || Type == FIELD_RUSHWOOD) && Obj.getFlag(UNPASS)) {
			return false;
		}

		Obj++;
	}

	return true;
}

bool SearchFreeField(int &x, int &y, int &z, int Distance, unsigned short HouseID, bool Jump) {
	int Direction = 4;
	int Dist = 0;
	int dx = 0, dy = 0;
	do {
		if ((Jump && JumpPossible(x + dx, y + dy, z, true))
				|| (!Jump && MovePossible(x + dx, y + dy, z)
					&& (!CoordinateAvoid(x + dx, y + dy, z) || CoordinateFlag(x + dx, y + dy, z, BED)))) {
			if (HouseID == 0xFFFF || !IsHouse(x + dx, y + dy, z)
					|| (HouseID != 0 && GetHouseID(x + dx, y + dy, z) == HouseID)) {
				x += dx;
				y += dy;
				return true;
			}
		}

		switch (Direction) {
			case 1: {
				dy--;
				if (dy + Dist == 0) {
					Direction = 2;
				}
				break;
			}
			case 2: {
				dx--;
				if (dx + Dist == 0) {
					Direction = 3;
				}
				break;
			}
			case 3: {
				dy++;
				if (dy == Dist) {
					Direction = 4;
				}
				break;
			}
			case 4: {
				dx++;
				if (dx == Dist + 1) {
					Direction = 1;
					Dist = dx;
				}
				break;
			}
		}
	} while (Dist <= Distance);

	return false;
}

bool SearchLoginField(int &x, int &y, int &z, int Distance, bool Player) {
	unsigned short HouseID = GetHouseID(x, y, z);
	if (SearchFreeField(x, y, z, Distance, HouseID, false)
			&& (!Player || !IsNoLogoutField(x, y, z))) {
		return true;
	}

	int Direction = 4;
	int Dist = 0;
	int dx = 0;
	int dy = 0;
	do {
		Object Obj = GetFirstObject(x + dx, y + dy, z);
		bool BankThere = false;
		if (Obj == NONE) {
			goto next;
		}

		if (Player && IsNoLogoutField(x + dx, y + dy, z)) {
			goto next;
		}

		if (IsHouse(x + dx, y + dy, z)
				&& (HouseID == 0 || GetHouseID(x + dx, y + dy, z) != HouseID)) {
			goto next;
		}

		while (Obj != NONE) {
			if (Obj.getFlag(BANK)) {
				BankThere = true;
			}

			if (Obj.isCreature()) {
				goto next;
			}

			if (Obj.getFlag(UNPASS) && Obj.getFlag(UNMOVE)) {
				goto next;
			}

			if (!Player && Obj.getFlag(AVOID) && Obj.getFlag(UNMOVE)) {
				goto next;
			}

			Obj++;
		}

		if (BankThere) {
			x += dx;
			y += dy;
			return true;
		}

next:
		switch (Direction) {
			case 1: {
				dy--;
				if (dy + Dist == 0) {
					Direction = 2;
				}
				break;
			}
			case 2: {
				dx--;
				if (dx + Dist == 0) {
					Direction = 3;
				}
				break;
			}
			case 3: {
				dy++;
				if (dy == Dist) {
					Direction = 4;
				}
				break;
			}
			case 4: {
				dx++;
				if (dx == Dist + 1) {
					Direction = 1;
					Dist = dx;
				}
				break;
			}
		}
	} while (Dist <= Distance);

	return false;
}

bool SearchSpawnField(int &x, int &y, int &z, int Distance, bool Player) {
	bool Minimize = Distance >= 0;
	Distance = abs(Distance);
	unsigned short HouseID = GetHouseID(x, y, z);
	matrix<int> Map(-Distance, Distance, -Distance, Distance);
	for (int dx = -Distance; dx <= Distance; dx++) {
		for (int dy = -Distance; dy <= Distance; dy++) {
			Map(dx, dy) = INT_MAX;
		}
	}
	Map(0, 0) = 0;

	int BestX = 0, BestY = 0, TieBreaker = -1;
	for (int Pass = 0; true; Pass++) {
		bool Expanded = false;
		bool Found = false;
		for (int dx = -Distance; dx <= Distance; dx++) for (int dy = -Distance; dy <= Distance; dy++) {
			if (Map(dx, dy) != Pass) {
				continue;
			}

			if (IsHouse(x + dx, y + dy, z)
					&& (HouseID == 0 || GetHouseID(x + dx, y + dy, z) != HouseID)) {
				continue;
			}

			if (!Player && IsProtectionZone(x + dx, y + dy, z)) {
				continue;
			}

			Object Obj = GetFirstObject(x + dx, y + dy, z);
			if (Obj == NONE) {
				continue;
			}

			bool ExpansionPossible = true, LoginPossible = true, LoginBad = false;
			while (Obj != NONE) {
				if (Obj.isCreature()) {
					LoginPossible = false;
				}

				if (Obj.getFlag(UNPASS)) {
					if (Obj.getFlag(UNMOVE)) {
						ExpansionPossible = false;
						LoginPossible = false;
					} else {
						LoginBad = true;
					}
				}

				if (Obj.getFlag(AVOID)) {
					if (Obj.getFlag(UNMOVE)) {
						ExpansionPossible = false;
						LoginPossible = LoginPossible && !Player;
					}
					LoginBad = true;
				}

				Obj++;
			}

			if (ExpansionPossible || Pass == 0) {
				for (int ddx = -1; ddx <= 1; ddx++) for (int ddy = -1; ddy <= 1; ddy++) {
					if (in(dx + ddx, -Distance, Distance) && in(dy + ddy, -Distance, Distance)) {
						if (Map(dx + ddx, dy + ddy) > Pass) {
							Map(dx + ddx, dy + ddy) = Pass + abs(ddx) + abs(ddy);
						}
					}
				}
				Expanded = true;
			}

			if (!LoginPossible) {
				continue;
			}

			if (IsNoLogoutField(x + dx, y + dy, z) && Player) {
				continue;
			}

			int NewTieBreaker = random(0, 99);
			if (!LoginBad) {
				NewTieBreaker += 100;
			}

			if (NewTieBreaker > TieBreaker) {
				BestX = dx;
				BestY = dy;
				TieBreaker = NewTieBreaker;
				Found = true;
			}
		}

		if ((Found && Minimize) || !Expanded) {
			break;
		}
	}

	if (TieBreaker < 0) {
		return false;
	}

	x += BestX;
	y += BestY;
	return true;
}

bool SearchFlightField(uint32_t FugitiveID, uint32_t PursuerID, int &x, int &y, int &z) {
	TCreature *Fugitive = GetCreature(FugitiveID);
	if (Fugitive == nullptr) {
		error("SearchFlightField: Fl\xfc" "chtling existiert nicht.\n");
		return false;
	}

	TCreature *Pursuer = GetCreature(PursuerID);
	if (Pursuer == nullptr) {
		error("SearchFlightField: Verfolger existiert nicht.\n");
		return false;
	}

	if (Fugitive->posz != Pursuer->posz) {
		error("SearchFlightField: Fl\xfc" "chtling und Verfolger sind auf verschiedenen Ebenen.\n");
		return false;
	}

	int Try[9];
	for (int i = 0; i < 9; i++) {
		Try[i] = 0;
	}

	int dx = Fugitive->posx - Pursuer->posx;
	int dy = Fugitive->posy - Pursuer->posy;

	if (dx > abs(dy)) {
		Try[0] = 1;
	}
	if (-dy > abs(dx)) {
		Try[0] = 3;
	}
	if (-dx > abs(dy)) {
		Try[0] = 5;
	}
	if (dy > abs(dx)) {
		Try[0] = 7;
	}

	if (dx >= 0) {
		Try[1] = 1;
	}
	if (dy <= 0) {
		Try[2] = 3;
	}
	if (dx <= 0) {
		Try[3] = 5;
	}
	if (dy >= 0) {
		Try[4] = 7;
	}
	RandomShuffle(&Try[1], 4);

	if (dx >= dy) {
		Try[5] = 2;
	}
	if (dx <= -dy) {
		Try[6] = 4;
	}
	if (dx <= dy) {
		Try[7] = 6;
	}
	if (dx >= -dy) {
		Try[8] = 8;
	}
	RandomShuffle(&Try[5], 4);

	z = Fugitive->posz;
	for (int i = 0; i < 9; i++) {
		if (Try[i] == 0) {
			continue;
		}

		switch (Try[i]) {
			case 1: x = Fugitive->posx + 1; y = Fugitive->posy;     break;
			case 2: x = Fugitive->posx + 1; y = Fugitive->posy - 1; break;
			case 3: x = Fugitive->posx;     y = Fugitive->posy - 1; break;
			case 4: x = Fugitive->posx - 1; y = Fugitive->posy - 1; break;
			case 5: x = Fugitive->posx - 1; y = Fugitive->posy;     break;
			case 6: x = Fugitive->posx - 1; y = Fugitive->posy + 1; break;
			case 7: x = Fugitive->posx;     y = Fugitive->posy + 1; break;
			case 8: x = Fugitive->posx + 1; y = Fugitive->posy + 1; break;
			default: {
				error("SearchFlightField: Ung\xfcltige Richtung %d.\n", Try[i]);
				return false;
			}
		}

		if (Fugitive->MovePossible(x, y, z, false, false)) {
			return true;
		}
	}

	return false;
}

bool SearchSummonField(int &x, int &y, int &z, int Distance) {
	int TieBreaker = -1;
	int BestX = 0, BestY = 0;
	for (int dx = -Distance; dx <= Distance; dx++) {
		for (int dy = -Distance; dy <= Distance; dy++) {
			int NewTieBreaker = random(0, 99);
			if (NewTieBreaker > TieBreaker
					&& MovePossible(x + dx, y + dy, z)
					&& !CoordinateAvoid(x + dx, y + dy, z)
					&& !IsProtectionZone(x + dx, y + dy, z)
					&& !IsHouse(x + dx, y + dy, z)
					&& ThrowPossible(x, y, z, x + dx, y + dy, z, 0)) {
				TieBreaker = NewTieBreaker;
				BestX = x + dx;
				BestY = y + dy;
			}
		}
	}

	if (TieBreaker == -1) {
		return false;
	}

	x = BestX;
	y = BestY;
	return true;
}

bool ThrowPossible(int sx, int sy, int sz, int zx, int zy, int zz, int Power) {
	int zmin = sz - Power;
	if (zmin < 0) {
		zmin = 0;
	}

	for (int z = sz - 1; z >= zmin; z--) {
		Object Obj = GetFirstObject(sx, sy, z);
		if (Obj != NONE && Obj.getFlag(BANK)) {
			zmin = z + 1;
			break;
		}
	}

	if (zmin > zz) {
		return false;
	}

	for (int z = zmin; z <= std::min(sz, zz); z++) {
		int newx, newy, newz;
		if (sx == zx && sy == zy) {
			newx = zx;
			newy = zy;
		} else {
			int d = std::max(abs(zx - sx), abs(zy - sy));
			int i = 1;
			if (CoordinateFlag(sx, sy, sz, HOOKEAST) && zx < sx) {
				i = 0;
			}
			if (CoordinateFlag(sx, sy, sz, HOOKSOUTH) && zy < sy) {
				i = 0;
			}

			while (i <= d) {
				if (CoordinateFlag((sx * (d - i) + zx * i) / d, (sy * (d - i) + zy * i) / d, z, UNTHROW)) {
					break;
				}
				i++;
			}

			i--;
			newx = (sx * (d - i) + zx * i) / d;
			newy = (sy * (d - i) + zy * i) / d;
		}

		newz = z;
		if (z <= 14) {
			int i;
			for (i = z; i < zz; i++) {
				Object Obj = GetFirstObject(newx, newy, i);
				if (Obj != NONE && Obj.getFlag(BANK)) {
					break;
				}
			}
			newz = i;
		}

		if (newx == zx && newy == zy && newz == zz) {
			return true;
		}
	}

	return false;
}

void GetCreatureLight(uint32_t CreatureID, int &Brightness, int &Color) {
	TCreature *cr = GetCreature(CreatureID);
	if (cr == nullptr) {
		error("GetCreatureLight: Kreatur existiert nicht.\n");
		Brightness = 0;
		Color = 0;
		return;
	}

	Brightness = 0;
	if (cr->Skills[SKILL_SHINING] != nullptr) {
		Brightness = cr->Skills[SKILL_SHINING]->TimerValue();
	}

	if (cr->Type == PLAYER && CheckRight(CreatureID, ILLUMINATE)) {
		Brightness = std::max(Brightness, 7);
	}

	int Red = Brightness * 5;
	int Green = Brightness * 5;
	int Blue = Brightness * 5;
	for (int i = FIRST_BODY_POSITION; i <= LAST_BODY_POSITION; i++) {
		Object Obj = GetBodyObject(CreatureID, i);
		if (Obj != NONE && Obj.getFlag(LIGHT)) {
			int B = Obj.getAttribute(BRIGHTNESS);
			int C = Obj.getAttribute(LIGHTCOLOR);
			int r = (C / 36) * B;
			int g = (C / 6 % 6) * B;
			int b = (C % 6) * B;
			Brightness = std::max(Brightness, B);
			Red = std::max(Red, r);
			Green = std::max(Green, g);
			Blue = std::max(Blue, b);
		}
	}

	if (Brightness == 0) {
		Color = 0;
		return;
	}

	Color = (Red / Brightness) * 36 + (Green / Brightness) * 6 + Blue / Brightness;
}

int GetInventoryWeight(uint32_t CreatureID) {
	TCreature *cr = GetCreature(CreatureID);
	if (cr == nullptr) {
		error("GetInventoryWeight: Kreatur %d existiert nicht.\n", CreatureID);
		return 0;
	}

	return GetRowWeight(GetFirstContainerObject(cr->GetCreatureObject()));
}

bool CheckRight(uint32_t CharacterID, RIGHT Right) {
	TPlayer *pl = GetPlayer(CharacterID);
	if (pl == nullptr) {
		error("CheckRight: Spieler existiert nicht; Right=%d.\n", Right);
		return false;
	}

	// The original compares the right as a signed number.
	if (static_cast<int>(Right) < 0 || static_cast<int>(Right) / 8 >= static_cast<int>(sizeof(pl->Rights))) {
		error("CheckRight: Ung\xfcltige Rechtnummer %d.\n", Right);
		return false;
	}

	return (pl->Rights[static_cast<int>(Right) / 8] & (1 << (static_cast<int>(Right) % 8))) != 0;
}

bool CheckBanishmentRight(uint32_t CharacterID, int Reason, int Action) {
	TPlayer *pl = GetPlayer(CharacterID);
	if (pl == nullptr) {
		error("CheckBanishmentRight: Spieler existiert nicht.\n");
		return false;
	}

	// %ld printed the 32-bit ID as a signed number.
	if (!in(Reason, 0, LAST_BANISHMENT_RIGHT - FIRST_BANISHMENT_RIGHT)) {
		error("CheckBanishmentRight: Ung\xfcltiger Banngrund %d von Spieler %ld.\n",
				Reason, static_cast<long>(static_cast<int32_t>(CharacterID)));
		return false;
	}

	if (!in(Action, BANISHMENT_ACTION_NOTATION, BANISHMENT_ACTION_STATEMENT_REPORT)) {
		error("CheckBanishmentRight: Ung\xfcltige Aktion %d von Spieler %ld.\n",
				Action, static_cast<long>(static_cast<int32_t>(CharacterID)));
		return false;
	}

	if (!CheckRight(CharacterID, static_cast<RIGHT>(Reason + FIRST_BANISHMENT_RIGHT))) {
		return false;
	}

	switch (Action) {
		case BANISHMENT_ACTION_NOTATION: {
			return CheckRight(CharacterID, NOTATION);
		}
		case BANISHMENT_ACTION_NAMELOCK: {
			return in(Reason + FIRST_BANISHMENT_RIGHT, FIRST_NAMEBANISHMENT_RIGHT, LAST_NAMEBANISHMENT_RIGHT)
					&& CheckRight(CharacterID, NAMELOCK);
		}
		case BANISHMENT_ACTION_BANISHMENT: {
			return !in(Reason + FIRST_BANISHMENT_RIGHT, FIRST_NAMEBANISHMENT_RIGHT, LAST_NAMEBANISHMENT_RIGHT)
					&& CheckRight(CharacterID, BANISHMENT);
		}
		case BANISHMENT_ACTION_NAMELOCK_BANISHMENT: {
			return in(Reason + FIRST_BANISHMENT_RIGHT, FIRST_NAMEBANISHMENT_RIGHT, LAST_NAMEBANISHMENT_RIGHT)
					&& CheckRight(CharacterID, NAMELOCK)
					&& CheckRight(CharacterID, BANISHMENT);
		}
		case BANISHMENT_ACTION_BANISHMENT_FINAL_WARNING: {
			return !in(Reason + FIRST_BANISHMENT_RIGHT, FIRST_NAMEBANISHMENT_RIGHT, LAST_NAMEBANISHMENT_RIGHT)
					&& CheckRight(CharacterID, BANISHMENT)
					&& CheckRight(CharacterID, FINAL_WARNING);
		}
		case BANISHMENT_ACTION_NAMELOCK_BANISHMENT_FINAL_WARNING: {
			return in(Reason + FIRST_BANISHMENT_RIGHT, FIRST_NAMEBANISHMENT_RIGHT, LAST_NAMEBANISHMENT_RIGHT)
					&& CheckRight(CharacterID, NAMELOCK)
					&& CheckRight(CharacterID, BANISHMENT)
					&& CheckRight(CharacterID, FINAL_WARNING);
		}
		case BANISHMENT_ACTION_STATEMENT_REPORT: {
			return (in(Reason + FIRST_BANISHMENT_RIGHT, FIRST_REPORT_RIGHT, LAST_REPORT_RIGHT)
						|| Reason + FIRST_BANISHMENT_RIGHT == DESTRUCTIVE_BEHAVIOUR)
					&& CheckRight(CharacterID, STATEMENT_REPORT);
		}
	}

	return false;
}

const char *GetBanishmentReason(int Reason) {
	switch (Reason + FIRST_BANISHMENT_RIGHT) {
		case NAME_INSULTING: return "NAME_INSULTING";
		case NAME_SENTENCE: return "NAME_SENTENCE";
		case NAME_NONSENSICAL_LETTERS: return "NAME_NONSENSICAL_LETTERS";
		case NAME_BADLY_FORMATTED: return "NAME_BADLY_FORMATTED";
		case NAME_NO_PERSON: return "NAME_NO_PERSON";
		case NAME_CELEBRITY: return "NAME_CELEBRITY";
		case NAME_COUNTRY: return "NAME_COUNTRY";
		case NAME_FAKE_IDENTITY: return "NAME_FAKE_IDENTITY";
		case NAME_FAKE_POSITION: return "NAME_FAKE_POSITION";
		case STATEMENT_INSULTING: return "STATEMENT_INSULTING";
		case STATEMENT_SPAMMING: return "STATEMENT_SPAMMING";
		case STATEMENT_ADVERT_OFFTOPIC: return "STATEMENT_ADVERT_OFFTOPIC";
		case STATEMENT_ADVERT_MONEY: return "STATEMENT_ADVERT_MONEY";
		case STATEMENT_NON_ENGLISH: return "STATEMENT_NON_ENGLISH";
		case STATEMENT_CHANNEL_OFFTOPIC: return "STATEMENT_CHANNEL_OFFTOPIC";
		case STATEMENT_VIOLATION_INCITING: return "STATEMENT_VIOLATION_INCITING";
		case CHEATING_BUG_ABUSE: return "CHEATING_BUG_ABUSE";
		case CHEATING_GAME_WEAKNESS: return "CHEATING_GAME_WEAKNESS";
		case CHEATING_MACRO_USE: return "CHEATING_MACRO_USE";
		case CHEATING_MODIFIED_CLIENT: return "CHEATING_MODIFIED_CLIENT";
		case CHEATING_HACKING: return "CHEATING_HACKING";
		case CHEATING_MULTI_CLIENT: return "CHEATING_MULTI_CLIENT";
		case CHEATING_ACCOUNT_TRADING: return "CHEATING_ACCOUNT_TRADING";
		case CHEATING_ACCOUNT_SHARING: return "CHEATING_ACCOUNT_SHARING";
		case GAMEMASTER_THREATENING: return "GAMEMASTER_THREATENING";
		case GAMEMASTER_PRETENDING: return "GAMEMASTER_PRETENDING";
		case GAMEMASTER_INFLUENCE: return "GAMEMASTER_INFLUENCE";
		case GAMEMASTER_FALSE_REPORTS: return "GAMEMASTER_FALSE_REPORTS";
		case KILLING_EXCESSIVE_UNJUSTIFIED: return "KILLING_EXCESSIVE_UNJUSTIFIED";
		case DESTRUCTIVE_BEHAVIOUR: return "DESTRUCTIVE_BEHAVIOUR";
		case SPOILING_AUCTION: return "SPOILING_AUCTION";
		case INVALID_PAYMENT: return "INVALID_PAYMENT";
		default: {
			error("GetBanishmentReason: Ung\xfcltiger Verbannungsgrund %d.\n", Reason);
			return "";
		}
	}
}
