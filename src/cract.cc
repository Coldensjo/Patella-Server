#include <algorithm>
#include <cstdint>
#include <cstdlib>

#include "script.hh"
#include "config.hh"
#include "threads.hh"
#include "dbfuncs.hh"
#include "objects.hh"
#include "map.hh"
#include "info.hh"
#include "cr.hh"
#include "containr.hh"
#include "crinc.hh"
#include "crmain.hh"
#include "crnonpl.hh"
#include "crplayer.hh"
#include "houses.hh"
#include "operate.hh"
#include "connections.hh"
#include "sending.hh"
#include "strings.hh"
#include "utils.hh"
#include "time.hh"
#include "TXTEASymmetricKey.hh"
#include "cract.hh"

// Go, Move, Trade, Use, Turn, Attack, ToDoGo, ToDoRotate, both ToDoMove,
// ToDoTrade, both ToDoUse, ToDoTurn and ToDoAttack are declared throw(RESULT)
// in the original. C++17 has no dynamic exception specifications, so an
// exception of another type now propagates instead of ending in
// std::unexpected().

struct TShortwayPoint {
	TShortwayPoint() {
		Waylength = -1;
		Heuristic = -1;
		Predecessor = nullptr;
		NextToExpand = nullptr;
	}
	int x;
	int y;
	int Waypoints;
	int Waylength;
	int Heuristic;
	TShortwayPoint *Predecessor;
	TShortwayPoint *NextToExpand;
};

struct TShortway {
	TShortway(TCreature *cr, int VisibleX, int VisibleY);
	~TShortway();
	bool Calculate(int DestX, int DestY, bool MustReach, int MaxSteps);
private:
	void FillMap();
	void ClearMap();
	void Expand(TShortwayPoint *Node);
	matrix<TShortwayPoint> *Map;
	TShortwayPoint *FirstToExpand;
	TCreature *cr;
	int VisibleX;
	int VisibleY;
	int StartX;
	int StartY;
	int StartZ;
	int MinWaypoints;
};

TShortway::TShortway(TCreature *cr, int VisibleX, int VisibleY) {
	Map = nullptr;
	if (cr == nullptr) {
		error("TShortway::TShortway: \xdc" "bergebene Kreatur ist NULL.\n");
		return;
	}

	if (VisibleX < 1 || VisibleY < 1 || VisibleX > 100 || VisibleY > 100) {
		error("TShortway::TShortway: Ung\xfcltiger Sichtbarkeitsbereich %d*%d.\n", VisibleX, VisibleY);
		return;
	}

	this->cr = cr;
	this->VisibleX = VisibleX;
	this->VisibleY = VisibleY;
	StartX = cr->posx;
	StartY = cr->posy;
	StartZ = cr->posz;
	Map = new matrix<TShortwayPoint>(-VisibleX - 1, VisibleX + 1, -VisibleY - 1, VisibleY + 1);
	FillMap();
}

TShortway::~TShortway() {
	delete Map;
}

void TShortway::FillMap() {
	MinWaypoints = 1000;
	for (int x = -VisibleX; x <= VisibleX; x++) {
		for (int y = -VisibleY; y <= VisibleY; y++) {
			int Waypoints = -1;
			Object Obj = GetFirstObject(StartX + x, StartY + y, StartZ);
			if (Obj.exists() && Obj.getFlag(BANK) && !Obj.getFlag(UNPASS)) {
				Waypoints = Obj.getAttribute(WAYPOINTS);
				if (Waypoints == 0) {
					error("TShortway::FillMap: Ung\xfcltiger Wegpunkte-Wert %d f\xfcr Bank %d.\n",
							Waypoints, Obj.getObjectType().getTypeID());
					Waypoints = -1;
				}

				if (!cr->MovePossible(StartX + x, StartY + y, StartZ, false, false)) {
					Waypoints = -1;
				}

				if (Waypoints > 0 && Waypoints < MinWaypoints) {
					MinWaypoints = Waypoints;
				}
			}

			(*Map)(x, y).Waypoints = Waypoints;
			(*Map)(x, y).x = x;
			(*Map)(x, y).y = y;
		}
	}

	for (int x = -VisibleX - 1; x <= VisibleX + 1; x++) {
		(*Map)(x, -VisibleY - 1).Waypoints = -1;
		(*Map)(x, VisibleY + 1).Waypoints = -1;
	}

	for (int y = -VisibleY; y <= VisibleY; y++) {
		(*Map)(-VisibleX - 1, y).Waypoints = -1;
		(*Map)(VisibleX + 1, y).Waypoints = -1;
	}
}

void TShortway::ClearMap() {
	for (int x = -VisibleX; x <= VisibleX; x++) {
		for (int y = -VisibleY; y <= VisibleY; y++) {
			(*Map)(x, y).Predecessor = nullptr;
			(*Map)(x, y).NextToExpand = nullptr;
			(*Map)(x, y).Waylength = 0x7fffffff;
			(*Map)(x, y).Heuristic = 0x7fffffff;
		}
	}

	FirstToExpand = nullptr;
}

bool TShortway::Calculate(int DestX, int DestY, bool MustReach, int MaxSteps) {
	if (Map == nullptr) {
		error("TShortway::Calculate: Karte existiert nicht.\n");
		return false;
	}

	int dx = DestX - StartX;
	int dy = DestY - StartY;
	if (abs(dx) > VisibleX || abs(dy) > VisibleY) {
		return false;
	}

	if (dx == 0 && dy == 0) {
		return true;
	}

	ClearMap();
	(*Map)(dx, dy).Waylength = 0;
	FirstToExpand = &(*Map)(dx, dy);
	while (FirstToExpand != nullptr) {
		Expand(FirstToExpand);
	}

	if ((*Map)(0, 0).Waylength == 0x7fffffff) {
		return false;
	}

	TShortwayPoint *Search = &(*Map)(0, 0);
	while (Search->Predecessor != nullptr && MaxSteps > 0
			&& (MustReach || abs(Search->x - dx) > 1 || abs(Search->y - dy) > 1)) {
		TToDoEntry TD;
		TD.Code = TDGo;
		TD.Go.x = Search->Predecessor->x + StartX;
		TD.Go.y = Search->Predecessor->y + StartY;
		TD.Go.z = StartZ;
		cr->ToDoAdd(TD);
		Search = Search->Predecessor;
		MaxSteps--;
	}

	return true;
}

void TShortway::Expand(TShortwayPoint *Node) {
	if (Node == nullptr) {
		error("TShortway::Expand: \xdc" "bergebener Knoten ist NULL.\n");
		return;
	}

	FirstToExpand = Node->NextToExpand;

	int BasicWaylength = Node->Waylength + Node->Waypoints;
	if (BasicWaylength >= (*Map)(0, 0).Waylength) {
		return;
	}

	for (int x = -1; x <= 1; x++) {
		for (int y = -1; y <= 1; y++) {
			if (x == 0 && y == 0) {
				continue;
			}

			TShortwayPoint *Dest = &(*Map)(Node->x + x, Node->y + y);
			int Waylength = BasicWaylength;
			if (abs(x) + abs(y) != 1) {
				Waylength += 2 * Node->Waypoints;
			}

			if (Waylength >= Dest->Waylength) {
				continue;
			}

			Dest->Waylength = Waylength;
			Dest->Predecessor = Node;
			if ((Dest->x == 0 && Dest->y == 0) || Dest->Waypoints == -1) {
				continue;
			}

			if (Dest->Heuristic != 0x7fffffff) {
				if (FirstToExpand == Dest) {
					FirstToExpand = Dest->NextToExpand;
				} else {
					// FirstToExpand is not checked for NULL, as in the original.
					TShortwayPoint *Search = FirstToExpand;
					while (Search->NextToExpand != nullptr && Search->NextToExpand != Dest) {
						Search = Search->NextToExpand;
					}

					if (Search->NextToExpand != nullptr) {
						Search->NextToExpand = Dest->NextToExpand;
					} else {
						error("TShortway::Expand: Knoten steht nicht in der ExpandList.\n");
					}
				}
			}

			Dest->Heuristic = Dest->Waylength + Dest->Waypoints
					+ (abs(Dest->x) + abs(Dest->y) - 1) * MinWaypoints;
			if (FirstToExpand == nullptr || Dest->Heuristic <= FirstToExpand->Heuristic) {
				Dest->NextToExpand = FirstToExpand;
				FirstToExpand = Dest;
			} else {
				TShortwayPoint *Search = FirstToExpand;
				while (Search->NextToExpand != nullptr
						&& Search->NextToExpand->Heuristic < Dest->Heuristic) {
					Search = Search->NextToExpand;
				}

				Dest->NextToExpand = Search->NextToExpand;
				Search->NextToExpand = Dest;
			}
		}
	}
}

bool TCreature::SetOnMap() {
	bool NoRoom = false;
	if (!SearchLoginField(posx, posy, posz, 1, Type == PLAYER)) {
		NoRoom = true;
		if (IsHouse(posx, posy, posz)) {
			unsigned short HouseID = GetHouseID(posx, posy, posz);
			GetExitPosition(HouseID, posx, posy, posz);
			NoRoom = !SearchLoginField(posx, posy, posz, 1, Type == PLAYER);
		}
	}

	if (NoRoom) {
		posx = startx;
		posy = starty;
		posz = startz;
	}

	Object Obj = GetMapContainer(posx, posy, posz);
	if (Obj == NONE) {
		error("TCreature::SetOnMap: Kartencontainer f\xfcr Punkt [%d,%d,%d] existiert nicht.\n",
				posx, posy, posz);
		return false;
	}

	try {
		Create(Obj, ObjectType(TYPEID_CREATURE), ID);
	} catch (RESULT r) {
		error("TCreature::SetOnMap: Kann Kreatur nicht setzen ([%d,%d,%d] - Exception %d).\n",
				posx, posy, posz, r);
		CrObject = NONE;
		if (Type == PLAYER) {
			SendResult(GetConnection(), r);
		}
		return false;
	}

	return true;
}

bool TCreature::DelOnMap() {
	if (CrObject != NONE) {
		Object Help = CrObject;
		CrObject = NONE;
		try {
			Delete(Help, -1);
		} catch (RESULT r) {
			error("TCreature::DelOnMap: Error Deleting CreatureObject: %d\n", r);
			return false;
		}
	}

	return true;
}

void TCreature::Go(int x, int y, int z) {
	if (z != posz || abs(x - posx) > 1 || abs(y - posy) > 1) {
		throw NOTACCESSIBLE;
	}

	// The divisor is not guarded against a timer value of 7, as in the
	// original.
	if (Skills[SKILL_DRUNKEN] != nullptr && Skills[SKILL_DRUNKEN]->TimerValue() > 0 && Skills[SKILL_DRUNKEN]->Get() == 0
			&& rand() % (7 - Skills[SKILL_DRUNKEN]->TimerValue()) == 0) {
		int dx = 0;
		int dy = 0;
		switch (rand() % 4) {
			case 0: dx = 1; break;
			case 1: dx = -1; break;
			case 2: dy = 1; break;
			case 3: dy = -1; break;
		}

		x = posx + dx;
		y = posy + dy;
		if (ToDoClear() && Type == PLAYER) {
			SendSnapback(GetConnection());
		}

		ToDoTalk(Type == MONSTER ? TALK_ANIMAL_LOW : TALK_SAY, nullptr, "Hicks!", false);
		ToDoStart();
	}

	if (!MovePossible(x, y, z, true, false)) {
		if (Type == PLAYER) {
			if (z > 0 && GetHeight(posx, posy, posz) >= 24
					&& (posx == x || posy == y)
					&& !CoordinateFlag(posx, posy, z - 1, BANK)
					&& !CoordinateFlag(posx, posy, z - 1, UNPASS)
					&& MovePossible(x, y, z - 1, true, true)) {
				z--;
			}

			if (z < 15 && GetHeight(x, y, z + 1) >= 24
					&& (posx == x || posy == y)
					&& !CoordinateFlag(x, y, z, BANK)
					&& !CoordinateFlag(x, y, z, UNPASS)
					&& MovePossible(x, y, z + 1, true, true)) {
				z++;
			}
		}

		if (z == posz || !MovePossible(x, y, z, true, true)) {
			throw MOVENOTPOSSIBLE;
		}
	}

	::Move(ID, CrObject, GetMapContainer(x, y, z), -1, false, NONE);
}

void TCreature::Rotate(unsigned char Dir) {
	Direction = Dir;
	AnnounceChangedObject(CrObject, OBJECT_CHANGED);
}

void TCreature::Rotate(TCreature *Target) {
	if (Target == nullptr) {
		error("TCreature::Rotate: Target ist NULL.\n");
		return;
	}

	int Dir;
	int dx = Target->posx - posx;
	int dy = Target->posy - posy;
	if (abs(dx) < abs(dy)) {
		Dir = (dy < 0) ? DIRECTION_NORTH : DIRECTION_SOUTH;
	} else {
		Dir = (dx < 0) ? DIRECTION_WEST : DIRECTION_EAST;
	}

	Rotate((unsigned char)Dir);
}

void TCreature::Move(Object Obj, int zx, int zy, int zz, unsigned char Count) {
	if (!Obj.exists()) {
		throw NOTACCESSIBLE;
	}

	if (Obj == GetCreatureObject()) {
		Go(zx, zy, zz);
		return;
	}

	if (Obj.isCreature()) {
		Combat.DelayAttack(2000);
	}

	if (Obj.getFlag(CUMULATIVE) && Count > Obj.getAttribute(AMOUNT)) {
		Count = Obj.getAttribute(AMOUNT);
	}

	if (zx != POSITION_NOT_ON_MAP) {
		Object Dest = GetMapContainer(zx, zy, zz);
		if (!Dest.exists()) {
			throw ERROR;
		}

		if (Obj.getFlag(HANG) && CoordinateHookable(zx, zy, zz) && !ObjectInRange(ID, Dest, 1)) {
			if (ToDoClear() && Type == PLAYER) {
				SendSnapback(GetConnection());
			}

			if (GetObjectCreatureID(Obj) != ID) {
				ToDoMove(Obj, POSITION_NOT_ON_MAP, 0, 0, 1);
			}

			ToDoGo(zx, zy, zz, false, 0x7fffffff);
			ToDoMove(Obj, zx, zy, zz, 1);
			ToDoStart();
		} else {
			::Move(ID, Obj, Dest, Count, false, NONE);
		}
		return;
	}

	if (zy == 0) {
		for (int b = FIRST_BODY_POSITION; b <= LAST_BODY_POSITION; b++) {
			try {
				CheckInventoryDestination(Obj, GetBodyContainer(ID, b), false);
				zy = b;
				if (b != BODY_RIGHTHAND && b != BODY_LEFTHAND && b != BODY_AMMO) {
					break;
				}
			} catch (RESULT) {
			}
		}

		if (zy == 0) {
			for (int b = FIRST_BODY_POSITION; b <= LAST_BODY_POSITION; b++) {
				Object Help = GetBodyObject(ID, b);
				if (Help != NONE && Help.getFlag(CONTAINER)) {
					try {
						CheckContainerDestination(Obj, Help);
						zy = b;
					} catch (RESULT) {
					}
				}
			}

			if (zy == 0) {
				throw NOROOM;
			}
		}
	}

	if (zy < FIRST_BODY_POSITION || (zy > LAST_BODY_POSITION && zy < POSITION_CONTAINER_BASE)
			|| zy > POSITION_CONTAINER_BASE + MAX_OPEN_CONTAINERS - 1) {
		error("TCreature::Move: Ung\xfcltiger Containercode %d.\n", zy);
		throw ERROR;
	}

	Object DestCon = GetBodyContainer(ID, zy);
	Object DestObj = NONE;
	if (zy < POSITION_CONTAINER_BASE) {
		DestObj = GetBodyObject(ID, zy);
	} else {
		if (zz < 254) {
			DestObj = GetObject(ID, zx, zy, zz, zz, ObjectType(0));
		}

		if (zz == 254) {
			if (DestCon == NONE) {
				throw NOTACCESSIBLE;
			}
			DestCon = DestCon.getContainer();
		}
	}

	if (DestObj != NONE && DestObj.getFlag(CONTAINER)) {
		DestCon = DestObj;
		DestObj = NONE;
	}

	if (DestObj != NONE && Obj.getFlag(CUMULATIVE)
			&& Obj.getObjectType() == DestObj.getObjectType()) {
		int help = Count;
		if (DestObj.getAttribute(AMOUNT) + help > MAX_AMOUNT) {
			help = MAX_AMOUNT - DestObj.getAttribute(AMOUNT);
		}

		if (help > 0) {
			try {
				Merge(ID, Obj, DestObj, help, NONE);
				Count -= help;
				if (Count == 0) {
					return;
				}
			} catch (RESULT r) {
				if (r == TOOHEAVY) {
					throw;
				}
			}
		}

		DestObj = NONE;
	}

	if (DestCon == NONE) {
		throw OUTOFRANGE;
	}

	try {
		::Move(ID, Obj, DestCon, Count, false, DestObj);
	} catch (RESULT r) {
		if (zy >= POSITION_CONTAINER_BASE || DestObj == NONE
				|| (r != NOROOM && r != HANDSNOTFREE && r != HANDBLOCKED && r != ONEWEAPONONLY)) {
			throw r;
		}

		::Move(ID, DestObj, Obj.getContainer(), -1, false, NONE);
		::Move(ID, Obj, DestCon, Count, false, DestObj);
	}
}

void TCreature::Trade(Object Obj, uint32_t PartnerID) {
	if (Type != PLAYER) {
		error("TCreature::Trade: Nur Spieler k\xf6nnen handeln.\n");
		throw ERROR;
	}

	TPlayer *self = (TPlayer *)this;
	if (!Obj.exists() || !ObjectAccessible(ID, Obj, 1)) {
		throw NOTACCESSIBLE;
	}

	if (ID == PartnerID) {
		throw CROSSREFERENCE;
	}

	TPlayer *Partner = GetPlayer(PartnerID);
	if (Partner == nullptr || Partner->Type != PLAYER) {
		throw PLAYERNOTONLINE;
	}

	if (self->TradeObject != NONE) {
		throw ALREADYTRADING;
	}

	if (CountObjects(Obj) > 100) {
		throw TOOMANYOBJECTS;
	}

	if (ObjectDistance(GetCreatureObject(), Partner->GetCreatureObject()) > 2) {
		throw OUTOFRANGE;
	}

	if (!ThrowPossible(posx, posy, posz, Partner->posx, Partner->posy, Partner->posz, 0)) {
		throw CANNOTTHROW;
	}

	if (Partner->TradeObject != NONE && Partner->TradePartner != ID) {
		throw PARTNERTRADING;
	}

	if (Partner->TradeObject != NONE) {
		for (Object Help = Partner->TradeObject; !Help.isMapContainer(); Help = Help.getContainer()) {
			if (Help == Obj) {
				throw NOTACCESSIBLE;
			}
		}

		for (Object Help = Obj; !Help.isMapContainer(); Help = Help.getContainer()) {
			if (Help == Partner->TradeObject) {
				throw NOTACCESSIBLE;
			}
		}
	}

	self->TradeObject = Obj;
	self->TradePartner = PartnerID;
	self->TradeAccepted = false;

	if (Partner->TradeObject != NONE) {
		SendTradeOffer(Partner->GetConnection(), Name, false, self->TradeObject);
		SendTradeOffer(GetConnection(), Name, true, self->TradeObject);
		SendTradeOffer(GetConnection(), Partner->Name, false, Partner->TradeObject);
	} else {
		SendMessage(Partner->GetConnection(), TALK_INFO_MESSAGE, "%s wants to trade with you.", Name);
		SendTradeOffer(GetConnection(), Name, true, self->TradeObject);
	}
}

void TCreature::Use(Object Obj1, Object Obj2, unsigned char Dummy) {
	if (!Obj1.exists() || (Obj2 != NONE && !Obj2.exists())) {
		throw DESTROYED;
	}

	if (Obj2 != NONE && !Obj1.getFlag(DISTUSE) && !ObjectInRange(ID, Obj2, 1)) {
		int x, y, z;
		GetObjectCoordinates(Obj2, x, y, z);
		if (ToDoClear() && Type == PLAYER) {
			SendSnapback(GetConnection());
		}

		if (GetObjectCreatureID(Obj1) != ID) {
			ToDoMove(Obj1, POSITION_NOT_ON_MAP, 0, 0, 1);
		}

		ToDoGo(x, y, z, false, 0x7fffffff);
		ToDoUse(2, Obj1, Obj2);
		ToDoStart();
		return;
	}

	if (Obj2 != NONE && Obj1.getFlag(DISTUSE) && !ObjectInRange(ID, Obj2, 7)) {
		throw OUTOFRANGE;
	}

	if (Obj2 != NONE) {
		EarliestMultiuseTime = ServerMilliseconds + 1000;
	}

	::Use(ID, Obj1, Obj2, Dummy);
}

void TCreature::Turn(Object Obj) {
	if (!Obj.exists()) {
		throw DESTROYED;
	}

	::Turn(ID, Obj);
}

void TCreature::Attack() {
	Combat.Attack();
}

void TCreature::NotifyGo() {
	int x, y, z;
	GetObjectCoordinates(CrObject, x, y, z);
	int dx = x - posx;
	int dy = y - posy;
	int dz = z - posz;
	int oldx = posx;
	int oldy = posy;
	int oldz = posz;
	bool DiagonalMove = (dz == 0 && dx != 0 && dy != 0);

	MoveChainCreature(this, x, y);

	if (Type == PLAYER && GetConnection() != nullptr) {
		if (abs(dz) <= 1 && abs(dx) <= 1 && abs(dy) <= 1) {
			if (dz == 1) {
				posz++;
				posx--;
				posy--;
				dx++;
				dy++;
				SendFloors(GetConnection(), false);
			}

			if (dz == -1) {
				posz--;
				posx++;
				posy++;
				dx--;
				dy--;
				SendFloors(GetConnection(), true);
			}

			while (dx > 0) {
				posx++;
				dx--;
				SendRow(GetConnection(), DIRECTION_EAST);
			}

			while (dx < 0) {
				posx--;
				dx++;
				SendRow(GetConnection(), DIRECTION_WEST);
			}

			while (dy > 0) {
				posy++;
				dy--;
				SendRow(GetConnection(), DIRECTION_SOUTH);
			}

			while (dy < 0) {
				posy--;
				dy++;
				SendRow(GetConnection(), DIRECTION_NORTH);
			}
		} else {
			posx += dx;
			posy += dy;
			posz += dz;
			SendFullScreen(GetConnection());
		}
	} else {
		posx += dx;
		posy += dy;
		posz += dz;
	}

	if (Type == PLAYER) {
		TPlayer *self = (TPlayer *)this;
		for (int con = 0; con < MAX_OPEN_CONTAINERS; con++) {
			Object Con = self->GetOpenContainer(con);
			if (Con == NONE) {
				continue;
			}

			if (!Con.exists()) {
				error("TCreature::NotifyGo: OpenContainer existiert nicht. (%s, [%d,%d,%d]->[%d,%d,%d])\n",
						Name, oldx, oldy, oldz, posx, posy, posz);
				self->SetOpenContainer(con, NONE);
				SendCloseContainer(GetConnection(), con);
			} else if (!ObjectAccessible(ID, Con, 1)) {
				self->SetOpenContainer(con, NONE);
				SendCloseContainer(GetConnection(), con);
			}
		}

		if (self->TradeObject != NONE) {
			TPlayer *Partner = GetPlayer(self->TradePartner);
			if (!self->TradeObject.exists()) {
				error("TCreature::NotifyGo: Handelsobjekt existiert nicht mehr.\n");
				error("# H\xe4ndler %s an [%d,%d,%d]\n", Name, posx, posy, posz);
				if (Partner != nullptr) {
					error("# Partner %s an [%d,%d,%d]\n",
							Partner->Name, Partner->posx, Partner->posy, Partner->posz);
				}
			}

			if (Partner == nullptr
					|| !ObjectAccessible(ID, self->TradeObject, 1)
					|| ObjectDistance(GetCreatureObject(), Partner->GetCreatureObject()) > 2
					|| !ThrowPossible(posx, posy, posz, Partner->posx, Partner->posy, Partner->posz, 0)) {
				SendCloseTrade(GetConnection());
				SendMessage(GetConnection(), TALK_FAILURE_MESSAGE, "Trade cancelled.");
				self->RejectTrade();
			}
		}
	}

	Object Obj = GetFirstObject(posx, posy, posz);
	while (!Obj.getFlag(BANK) && Obj != NONE) {
		Obj++;
	}

	if (Obj == NONE) {
		error("TCreature::NotifyGo: Auf Feld [%d,%d,%d] befindet sich kein Bank.\n", posx, posy, posz);
		return;
	}

	int Waypoints = Obj.getAttribute(WAYPOINTS);
	if (DiagonalMove) {
		Waypoints *= 3;
	}

	EarliestWalkTime = ServerMilliseconds + (Waypoints * 1000 / GetSpeed() + Beat - 1) / Beat * Beat;
}

void TCreature::NotifyTurn(Object DestCon) {
	int x, y, z;
	GetObjectCoordinates(DestCon, x, y, z);
	int dx = x - posx;
	int dy = y - posy;
	if (dx > 0) {
		Direction = DIRECTION_EAST;
	} else if (dx < 0) {
		Direction = DIRECTION_WEST;
	} else if (dy < 0) {
		Direction = DIRECTION_NORTH;
	} else if (dy > 0) {
		Direction = DIRECTION_SOUTH;
	}
}

void TCreature::NotifyDelete() {
	if (Type == PLAYER && GetConnection() != nullptr) {
		GetConnection()->Logout(30, true);
		LoggingOut = true;
	}

	if (Type == PLAYER) {
		((TPlayer *)this)->RejectTrade();
	}

	DeleteChainCreature(this);
}

void TCreature::NotifyCreate() {
	InsertChainCreature(this, 0, 0);
}

void TCreature::NotifyChangeInventory() {
	if (CrObject == NONE) {
		return;
	}

	if (!CrObject.exists()) {
		error("TCreature::NotifyChangeInventory: Kreatur-Objekt existiert nicht.\n");
		error("# Kreatur: %s, Position: %d/%d/%d.\n", Name, posx, posy, posz);
		return;
	}

	Combat.CheckCombatValues();
	if (Type != PLAYER) {
		return;
	}

	int NewDelta[MAX_SKILLS];
	for (int i = 0; i < MAX_SKILLS; i++) {
		NewDelta[i] = 0;
	}

	for (int b = FIRST_BODY_POSITION; b <= LAST_BODY_POSITION; b++) {
		Object Obj = GetBodyObject(ID, b);
		if (Obj == NONE || !Obj.getFlag(SKILLBOOST)) {
			continue;
		}

		if (!Obj.getFlag(CLOTHES)) {
			error("TCreature::NotifyChangeInventory: Objekt %d hat SKILLBOOST, aber nicht CLOTHES.\n",
					Obj.getObjectType().getTypeID());
			continue;
		}

		if ((int)Obj.getAttribute(BODYPOSITION) != b) {
			continue;
		}

		// A negative skill number is not rejected, as in the original.
		int SkillNr = Obj.getAttribute(SKILLNUMBER);
		if (SkillNr > MAX_SKILLS - 1) {
			error("TCreature::NotifyChangeInventory: Objekt %d hat ung\xfcltige SKILLNUMBER %d.\n",
					Obj.getObjectType().getTypeID(), SkillNr);
			continue;
		}

		int Delta = Obj.getAttribute(SKILLMODIFICATION);
		if (Delta >= 0x8000) {
			Delta -= 0x10000;
		}
		NewDelta[SkillNr] += Delta;
	}

	bool SkillsChanged = false;
	for (int i = 0; i < MAX_SKILLS; i++) {
		if (Skills[i]->GetDAct() == NewDelta[i]) {
			continue;
		}

		Skills[i]->SetDAct(NewDelta[i]);
		if (i == SKILL_GOSTRENGTH) {
			AnnounceChangedCreature(ID, CREATURE_SPEED);
		}

		if (i == SKILL_ILLUSION) {
			if (NewDelta[SKILL_ILLUSION] > 0) {
				if (!IsInvisible() && Skills[SKILL_ILLUSION]->TimerValue() != 0) {
					SetTimer(SKILL_ILLUSION, 0, 0, 0, -1);
				}
				Outfit.OutfitID = 0;
				Outfit.ObjectType = 0;
			} else if (Skills[SKILL_ILLUSION]->TimerValue() == 0) {
				Outfit = OrgOutfit;
			}

			AnnounceChangedCreature(ID, CREATURE_OUTFIT);
		}

		SkillsChanged = true;
	}

	if (SkillsChanged) {
		SendPlayerSkills(GetConnection());
		((TPlayer *)this)->CheckState();
	}

	SendPlayerData(GetConnection());
}

void TCreature::CreatureMoveStimulus(uint32_t CreatureID, int Type) {
	if (CreatureID == ID || CreatureID == 0 || CreatureID != Combat.GetAttackDest()) {
		return;
	}

	if (Type != OBJECT_CHANGED || !LockToDo || IsDead || ActToDo >= NrToDo) {
		return;
	}

	if (ToDoList(ActToDo).Code != TDAttack || Combat.GetChaseMode() != CHASE_MODE_FOLLOW) {
		return;
	}

	if (Combat.AttackReady() || Combat.GetAttackDelay() <= 200) {
		return;
	}

	TCreature *dest = GetCreature(Combat.GetAttackDest());
	if (dest == nullptr) {
		return;
	}

	if (ObjectDistance(GetCreatureObject(), dest->GetCreatureObject()) <= 1) {
		return;
	}

	if (ToDoClear() && this->Type == PLAYER) {
		SendSnapback(GetConnection());
	}

	try {
		ToDoWait(200);
		ToDoAttack();
		ToDoStart();
	} catch (RESULT r) {
		ToDoClear();
		if (this->Type == PLAYER) {
			SendResult(GetConnection(), r);
		}

		ToDoWait(Combat.GetAttackDelay());
		ToDoStart();
	}
}

int TCreature::CalculateDelay() {
	TToDoEntry TD = ToDoList(ActToDo);
	if (TD.Code == TDGo && EarliestWalkTime > ServerMilliseconds) {
		return EarliestWalkTime - ServerMilliseconds;
	}

	if (TD.Code == TDUse && TD.Use.Obj2 != 0 && EarliestMultiuseTime > ServerMilliseconds) {
		return EarliestMultiuseTime - ServerMilliseconds;
	}

	if (TD.Code == TDAttack && (!Combat.AttackReady() || EarliestSpellTime > ServerMilliseconds)) {
		return std::max<int>(Combat.GetAttackDelay(), EarliestSpellTime - ServerMilliseconds);
	}

	if (TD.Code == TDWait && (EarliestWalkTime > ServerMilliseconds || TD.Wait.Time > ServerMilliseconds)) {
		return std::max<uint32_t>(EarliestWalkTime, TD.Wait.Time) - ServerMilliseconds;
	}

	return 0;
}

void TCreature::Execute() {
again:
	if (!LockToDo || IsDead || NextWakeup > ServerMilliseconds) {
		return;
	}

	if (ActToDo >= NrToDo) {
		ToDoClear();
		IdleStimulus();
		return;
	}

	int Delay = CalculateDelay();
	if (Delay > 0) {
		if (Stop) {
			ToDoClear();
			if (Type == PLAYER) {
				SendSnapback(GetConnection());
			}
		} else {
			ToDoQueue.insert(ServerMilliseconds + Delay, ID);
			NextWakeup = ServerMilliseconds + Delay;
		}
		return;
	}

	TToDoEntry TD = ToDoList(ActToDo);
	ActToDo++;

	try {
		switch (TD.Code) {
			case TDWait:
				break;

			case TDGo:
				Go(TD.Go.x, TD.Go.y, TD.Go.z);
				break;

			case TDRotate:
				Rotate((unsigned char)TD.Rotate.Direction);
				break;

			case TDMove:
				Move(Object(TD.Move.Obj), TD.Move.x, TD.Move.y, TD.Move.z, TD.Move.Count);
				break;

			case TDTrade:
				Trade(Object(TD.Trade.Obj), TD.Trade.Partner);
				break;

			case TDUse:
				Use(Object(TD.Use.Obj1), Object(TD.Use.Obj2), TD.Use.Dummy);
				break;

			case TDTurn:
				Turn(Object(TD.Turn.Obj));
				break;

			case TDAttack:
				Attack();
				break;

			case TDTalk:
				if (TD.Talk.Text == 0) {
					error("TCreature::Execute: TD.Text ist Null bei %s.\n", Name);
				} else if (GetDynamicString(TD.Talk.Text) == nullptr) {
					error("TCreature::Execute: Text ist NULL bei %s.\n", Name);
				}

				Talk(ID, TD.Talk.Mode, GetDynamicString(TD.Talk.Addressee),
						GetDynamicString(TD.Talk.Text), TD.Talk.CheckSpamming);
				break;

			case TDChangeState:
				if (Type == NPC) {
					ChangeNPCState(this, TD.ChangeState.NewState, true);
				}
				break;
		}

		if (Stop) {
			ToDoClear();
			if (Type == PLAYER) {
				SendSnapback(GetConnection());
			}
			return;
		}
	} catch (RESULT r) {
		// ToDoClear resets Stop before it is read here, as in the original.
		bool SnapbackNecessary = false;
		if (ToDoClear() || Stop) {
			SnapbackNecessary = true;
		}

		if (r == EXHAUSTED) {
			ToDoWait(1000);
			ToDoStart();
		} else {
			ToDoYield();
		}

		if (Type == PLAYER) {
			SendResult(GetConnection(), r);
			if (SnapbackNecessary && r != MOVENOTPOSSIBLE && r != NOTINVITED
					&& r != ENTERPROTECTIONZONE) {
				SendSnapback(GetConnection());
			}
		}
		return;
	}

	goto again;
}

bool TCreature::ToDoClear() {
	bool SnapbackNecessary = false;
	for (int i = 0; i < NrToDo; i++) {
		TToDoEntry &TD = ToDoList(i);
		switch (TD.Code) {
			case TDGo:
				if (i >= ActToDo) {
					SnapbackNecessary = true;
				}
				break;

			case TDTalk:
				DeleteDynamicString(TD.Talk.Text);
				DeleteDynamicString(TD.Talk.Addressee);
				break;

			case TDChangeState:
				if (i >= ActToDo && Type == NPC) {
					ChangeNPCState(this, TD.ChangeState.NewState, false);
				}
				break;

			default:
				break;
		}
	}

	LockToDo = false;
	ActToDo = 0;
	NrToDo = 0;
	Stop = false;
	return SnapbackNecessary;
}

void TCreature::ToDoStop() {
	if (LockToDo) {
		Stop = true;
	} else if (Type == PLAYER) {
		SendSnapback(GetConnection());
	}
}

void TCreature::ToDoAdd(TToDoEntry &TD) {
	if (LockToDo) {
		if (ToDoClear() && Type == PLAYER) {
			SendSnapback(GetConnection());
		}
	}

	ToDoList(NrToDo) = TD;
	NrToDo++;
}

void TCreature::ToDoStart() {
	if (NrToDo == 0) {
		return;
	}

	LockToDo = true;
	ActToDo = 0;
	int Delay = std::max<int>(CalculateDelay(), 1);
	ToDoQueue.insert(ServerMilliseconds + Delay, ID);
	NextWakeup = ServerMilliseconds + Delay;
}

void TCreature::ToDoYield() {
	if (!LockToDo) {
		ToDoWait(0);
		ToDoStart();
	}
}

void TCreature::ToDoWait(int Delay) {
	TToDoEntry TD;
	TD.Code = TDWait;
	TD.Wait.Time = ServerMilliseconds + Delay;
	ToDoAdd(TD);
}

void TCreature::ToDoTalk(int Mode, char *Addressee, char *Text, bool CheckSpamming) {
	TToDoEntry TD;
	TD.Code = TDTalk;
	if (Addressee != nullptr) {
		TD.Talk.Addressee = AddDynamicString(Addressee);
	} else {
		TD.Talk.Addressee = 0;
	}

	TD.Talk.CheckSpamming = CheckSpamming;
	TD.Talk.Mode = Mode;
	if (Text == nullptr) {
		error("TCreature::ToDoTalk: Text ist NULL bei %s.\n", Name);
	}

	TD.Talk.Text = AddDynamicString(Text);
	if (TD.Talk.Text == 0) {
		error("TCreature::ToDoTalk: TD.Text ist Null bei %s.\n", Name);
	}

	ToDoAdd(TD);
}

void TCreature::ToDoChangeState(int NewState) {
	TToDoEntry TD;
	TD.Code = TDChangeState;
	TD.ChangeState.NewState = NewState;
	ToDoAdd(TD);
}

void TCreature::ToDoGo(int DestX, int DestY, int DestZ, bool Dest, int MaxSteps) {
	if (DestZ > posz) {
		throw DOWNSTAIRS;
	}

	if (DestZ < posz) {
		throw UPSTAIRS;
	}

	if (LockToDo && ToDoList(NrToDo - 1).Code == TDGo
			&& ToDoList(NrToDo - 1).Go.x == DestX
			&& ToDoList(NrToDo - 1).Go.y == DestY
			&& ToDoList(NrToDo - 1).Go.z == DestZ) {
		throw NOERROR;
	}

	int dx = DestX - posx;
	int dy = DestY - posy;
	if (dx == 0 && dy == 0) {
		return;
	}

	if (!Dest && abs(dx) <= 1 && abs(dy) <= 1) {
		return;
	}

	if (abs(dx) + abs(dy) == 1) {
		TToDoEntry TD;
		TD.Code = TDGo;
		TD.Go.x = DestX;
		TD.Go.y = DestY;
		TD.Go.z = DestZ;
		ToDoAdd(TD);
	} else {
		TShortway Shortway(this, Type == PLAYER ? 7 : 10, Type == PLAYER ? 7 : 10);
		bool Result = Shortway.Calculate(DestX, DestY, Dest, MaxSteps);
		if (!Result) {
			ToDoClear();
			if (Type == PLAYER) {
				SendSnapback(GetConnection());
			}
			throw NOWAY;
		}
	}
}

void TCreature::ToDoRotate(int Direction) {
	// Negative directions are not rejected, as in the original.
	if (Direction > DIRECTION_WEST) {
		throw ERROR;
	}

	TToDoEntry TD;
	TD.Code = TDRotate;
	TD.Rotate.Direction = Direction;
	ToDoAdd(TD);
}

void TCreature::ToDoMove(int sx, int sy, int sz, ObjectType Type, unsigned char RNum, int zx, int zy, int zz, unsigned char Count) {
	Object Obj = GetObject(ID, sx, sy, sz, RNum, Type);
	if (!Obj.exists()) {
		throw NOTACCESSIBLE;
	}

	if (sx != POSITION_NOT_ON_MAP) {
		if (sz > posz) {
			throw DOWNSTAIRS;
		}

		if (sz < posz) {
			throw UPSTAIRS;
		}

		if (!ObjectInRange(ID, Obj, 1)) {
			ToDoGo(sx, sy, sz, false, 0x7fffffff);
		}
	}

	if (Obj.isCreature()) {
		Object Dest = GetFirstObject(zx, zy, zz);
		if (!Dest.exists() || !Dest.getFlag(BANK)) {
			throw NOTACCESSIBLE;
		}

		// The original reads the waypoints of the bank into a local it never
		// uses.
		Dest.getAttribute(WAYPOINTS);
		TCreature *cr = GetCreature(Obj);
		if (cr == nullptr) {
			error("TCreature::ToDoMove: Kreatur existiert nicht.\n");
			throw ERROR;
		}

		if (cr->EarliestWalkTime > ServerMilliseconds) {
			ToDoWait(cr->EarliestWalkTime - ServerMilliseconds + 1000);
		} else {
			ToDoWait(1000);
		}
	} else {
		ToDoWait(100);
	}

	TToDoEntry TD;
	TD.Code = TDMove;
	TD.Move.Obj = Obj.getObjectID();
	TD.Move.x = zx;
	TD.Move.y = zy;
	TD.Move.z = zz;
	TD.Move.Count = Count;
	ToDoAdd(TD);
}

void TCreature::ToDoMove(Object Obj, int zx, int zy, int zz, unsigned char Count) {
	if (!Obj.exists()) {
		throw NOTACCESSIBLE;
	}

	if (!ObjectInRange(ID, Obj, 1)) {
		int x, y, z;
		GetObjectCoordinates(Obj, x, y, z);
		ToDoGo(x, y, z, false, 0x7fffffff);
	}

	ToDoWait(Obj.isCreature() ? 1000 : 100);

	TToDoEntry TD;
	TD.Code = TDMove;
	TD.Move.Obj = Obj.getObjectID();
	TD.Move.x = zx;
	TD.Move.y = zy;
	TD.Move.z = zz;
	TD.Move.Count = Count;
	ToDoAdd(TD);
}

void TCreature::ToDoTrade(int x, int y, int z, ObjectType Type, unsigned char RNum, uint32_t TradePartner) {
	Object Obj = GetObject(ID, x, y, z, RNum, Type);
	if (!Obj.exists()) {
		throw NOTACCESSIBLE;
	}

	if (Obj.getFlag(UNMOVE)) {
		throw NOTMOVABLE;
	}

	if (!Obj.getFlag(TAKE)) {
		throw NOTTAKABLE;
	}

	if (Obj.isCreature()) {
		error("TCreature::ToDoTrade: Objekt ist eine Kreatur.\n");
		throw ERROR;
	}

	if (TradePartner == 0) {
		error("TCreature::ToDoTrade: Handelspartner ist Null.\n");
		throw ERROR;
	}

	TCreature *Partner = GetCreature(TradePartner);
	if (Partner == nullptr) {
		throw PLAYERNOTONLINE;
	}

	if (Partner->Type != PLAYER) {
		error("TCreature::ToDoTrade: Handelspartner von %s ist kein Spieler.\n", Name);
		throw ERROR;
	}

	if (x != POSITION_NOT_ON_MAP) {
		if (z > posz) {
			throw DOWNSTAIRS;
		}

		if (z < posz) {
			throw UPSTAIRS;
		}

		if (!ObjectInRange(ID, Obj, 1)) {
			ToDoGo(x, y, z, false, 0x7fffffff);
		}
	}

	TToDoEntry TD;
	TD.Code = TDTrade;
	TD.Trade.Obj = Obj.getObjectID();
	TD.Trade.Partner = TradePartner;
	ToDoAdd(TD);
}

void TCreature::ToDoUse(unsigned char Count, int x1, int y1, int z1, ObjectType Type1, unsigned char RNum1, unsigned char Dummy, int x2, int y2, int z2, ObjectType Type2, unsigned char RNum2) {
	Object Obj1, Obj2;
	Obj1 = GetObject(ID, x1, y1, z1, RNum1, Type1);
	if (!Obj1.exists()) {
		throw NOTACCESSIBLE;
	}

	if (Count >= 2) {
		Obj2 = GetObject(ID, x2, y2, z2, RNum2, Type2);
		if (!Obj2.exists()) {
			throw NOTACCESSIBLE;
		}
	} else {
		Obj2 = NONE;
	}

	if (x1 != POSITION_NOT_ON_MAP) {
		if (z1 > posz) {
			throw DOWNSTAIRS;
		}

		if (z1 < posz) {
			throw UPSTAIRS;
		}

		if (!ObjectInRange(ID, Obj1, 1)) {
			ToDoGo(x1, y1, z1, false, 0x7fffffff);
		}
	}

	ToDoWait(100);

	TToDoEntry TD;
	TD.Code = TDUse;
	TD.Use.Obj1 = Obj1.getObjectID();
	TD.Use.Obj2 = Obj2.getObjectID();
	TD.Use.Dummy = Dummy;
	ToDoAdd(TD);
}

void TCreature::ToDoUse(unsigned char Count, Object Obj1, Object Obj2) {
	if (!Obj1.exists()) {
		throw NOTACCESSIBLE;
	}

	if (Count >= 2) {
		if (!Obj2.exists()) {
			throw NOTACCESSIBLE;
		}
	} else {
		Obj2 = NONE;
	}

	if (!ObjectInRange(ID, Obj1, 1)) {
		int x, y, z;
		GetObjectCoordinates(Obj1, x, y, z);
		ToDoGo(x, y, z, false, 0x7fffffff);
	}

	ToDoWait(100);

	// The original stores Count in the Dummy field here.
	TToDoEntry TD;
	TD.Code = TDUse;
	TD.Use.Obj1 = Obj1.getObjectID();
	TD.Use.Obj2 = Obj2.getObjectID();
	TD.Use.Dummy = Count;
	ToDoAdd(TD);
}

void TCreature::ToDoTurn(int x, int y, int z, ObjectType Type, unsigned char RNum) {
	Object Obj = GetObject(ID, x, y, z, RNum, Type);
	if (!Obj.exists()) {
		throw NOTACCESSIBLE;
	}

	if (x != POSITION_NOT_ON_MAP) {
		if (z > posz) {
			throw DOWNSTAIRS;
		}

		if (z < posz) {
			throw UPSTAIRS;
		}

		if (!ObjectInRange(ID, Obj, 1)) {
			ToDoGo(x, y, z, false, 0x7fffffff);
		}
	}

	ToDoWait(100);

	TToDoEntry TD;
	TD.Code = TDTurn;
	TD.Turn.Obj = Obj.getObjectID();
	ToDoAdd(TD);
}

void TCreature::ToDoAttack() {
	Combat.CanToDoAttack();
	if (Combat.GetDistance() != 1) {
		ToDoWait(100);
	}

	TToDoEntry TD;
	TD.Code = TDAttack;
	ToDoAdd(TD);
}
