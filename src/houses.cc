// houses.cc: the houses. Their areas, owners, subowners, guests and name doors and who may enter; the auctions,
// transfers, evictions and rents settled with the query manager at start-up; the clean-up that moves the items in a
// house to its owner's depot; and the owners file (owners.dat). The houses and their areas come from houses.toml
// (housesdata.cc).

#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <time.h>

#include "script.hh"
#include "strings.hh"
#include "threads.hh"
#include "writer.hh"
#include "dbfuncs.hh"
#include "query.hh"
#include "config.hh"
#include "objects.hh"
#include "map.hh"
#include "info.hh"
#include "triggers.hh"
#include "cr.hh"
#include "crplayer.hh"
#include "containr.hh"
#include "operate.hh"
#include "utils.hh"
#include "time.hh"
#include "housesdata.hh"
#include "houses.hh"

// The entries ChangeSubowners and ChangeGuests take from the edited list.
const int MAX_SUBOWNERS = 10;
const int MAX_GUESTS = 100;

// The time a rent pays for, in seconds: thirty days, as the letter to a new
// owner says.
const int RENT_PERIOD = 2592000;

struct THelpDepot {
	uint32_t CharacterID;
	Object Box;
	int DepotNr;
};

static TQueryManagerConnection *QueryManagerConnection;
int PaymentExtension;
vector<THelpDepot> HelpDepot(1, 10, 10);
int HelpDepots;

// ShowSubownerList, ShowGuestList, ChangeSubowners, ChangeGuests, both
// KickGuest, KickGuests, ShowNameDoor, ChangeNameDoor, FinishAuctions and
// CollectRents report a failure by throwing a RESULT; LoadHouses,
// LoadOwners, SaveOwners and InitHouses throw a const char *.
// They have no exception specification: an exception of another type passes
// on to the caller.

int CheckAccessRight(char *Rule, TPlayer *pl) {
	if (Rule == nullptr) {
		error("CheckAccessRight: Rule is NULL.\n");
		return 0;
	}

	if (pl == nullptr) {
		error("CheckAccessRight: pl is NULL.\n");
		return 0;
	}

	bool PositiveRule = true;
	if (Rule[0] == '!') {
		PositiveRule = false;
		Rule++;
	}

	if (strchr(Rule, '@') != nullptr) {
		if (pl->Guild[0] == 0) {
			return 0;
		}

		char Pattern[200];
		if (pl->Rank[0] == 0) {
			snprintf(Pattern, 200, "@%s", pl->Guild);
		} else {
			snprintf(Pattern, 200, "%s@%s", pl->Rank, pl->Guild);
		}

		if (!MatchString(Rule, Pattern)) {
			return 0;
		}
	} else {
		if (!MatchString(Rule, pl->Name)) {
			return 0;
		}
	}

	return PositiveRule ? 1 : -1;
}

THouse *GetHouse(unsigned short ID) {
	int low = 0;
	int high = Houses - 1;
	while (low <= high) {
		int test = (low + high) / 2;
		if (House(test).ID == ID) {
			return &House(test);
		}

		if (House(test).ID < ID) {
			low = test + 1;
		}

		if (House(test).ID > ID) {
			high = test - 1;
		}
	}

	error("GetHouse: House with ID %d not found.\n", ID);
	return nullptr;
}

bool IsOwner(unsigned short HouseID, TPlayer *pl) {
	THouse *house = GetHouse(HouseID);
	if (house == nullptr) {
		error("IsOwner: House with ID %d does not exist.\n", HouseID);
		return false;
	}

	if (pl == nullptr) {
		error("IsOwner: pl is NULL.\n");
		return false;
	}

	return house->OwnerID == pl->ID;
}

bool IsSubowner(unsigned short HouseID, TPlayer *pl, int TimeStamp) {
	THouse *house = GetHouse(HouseID);
	if (house == nullptr) {
		error("IsSubowner: House with ID %d does not exist.\n", HouseID);
		return false;
	}

	if (pl == nullptr) {
		error("IsSubowner: pl is NULL.\n");
		return false;
	}

	if (TimeStamp < house->LastTransition) {
		return false;
	}

	for (int i = 0; i < house->Subowners; i++) {
		switch (CheckAccessRight(house->Subowner(i).Name, pl)) {
			case -1: return false;
			case 1: return true;
		}
	}

	return false;
}

bool IsGuest(unsigned short HouseID, TPlayer *pl, int TimeStamp) {
	THouse *house = GetHouse(HouseID);
	if (house == nullptr) {
		error("IsGuest: House with ID %d does not exist.\n", HouseID);
		return false;
	}

	if (pl == nullptr) {
		error("IsGuest: pl is NULL.\n");
		return false;
	}

	if (TimeStamp < house->LastTransition) {
		return false;
	}

	for (int i = 0; i < house->Guests; i++) {
		switch (CheckAccessRight(house->Guest(i).Name, pl)) {
			case -1: return false;
			case 1: return true;
		}
	}

	return false;
}

bool IsInvited(unsigned short HouseID, TPlayer *pl, int TimeStamp) {
	if (pl == nullptr) {
		error("IsInvited: pl is NULL.\n");
		return false;
	}

	return IsOwner(HouseID, pl) || IsSubowner(HouseID, pl, TimeStamp) || IsGuest(HouseID, pl, TimeStamp);
}

char *GetHouseName(unsigned short HouseID) {
	THouse *house = GetHouse(HouseID);
	if (house == nullptr) {
		error("GetHouseName: House with ID %d does not exist.\n", HouseID);
		return nullptr;
	}

	return house->Name;
}

char *GetHouseOwner(unsigned short HouseID) {
	THouse *house = GetHouse(HouseID);
	if (house == nullptr) {
		error("GetHouseOwner: House with ID %d does not exist.\n", HouseID);
		return nullptr;
	}

	return house->OwnerName;
}

void ShowSubownerList(unsigned short HouseID, TPlayer *pl, char *Buffer) {
	THouse *house = GetHouse(HouseID);
	if (house == nullptr) {
		error("ShowSubownerList: House with ID %d does not exist.\n", HouseID);
		throw ERROR;
	}

	if (pl == nullptr) {
		error("ShowSubownerList: pl is NULL.\n");
		throw ERROR;
	}

	if (Buffer == nullptr) {
		error("ShowSubownerList: Buffer is NULL.\n");
		throw ERROR;
	}

	if (!IsOwner(HouseID, pl)) {
		throw NOTACCESSIBLE;
	}

	print(3, "Editing subowner list of house %d.\n", HouseID);
	sprintf(Buffer, "# Subowners of %s\n", house->Name);
	for (int i = 0; i < house->Subowners; i++) {
		strcat(Buffer, house->Subowner(i));
		strcat(Buffer, "\n");
	}
}

void ShowGuestList(unsigned short HouseID, TPlayer *pl, char *Buffer) {
	THouse *house = GetHouse(HouseID);
	if (house == nullptr) {
		error("ShowGuestList: House with ID %d does not exist.\n", HouseID);
		throw ERROR;
	}

	if (pl == nullptr) {
		error("ShowGuestList: pl is NULL.\n");
		throw ERROR;
	}

	if (Buffer == nullptr) {
		error("ShowGuestList: Buffer is NULL.\n");
		throw ERROR;
	}

	if ((!IsOwner(HouseID, pl) && !IsSubowner(HouseID, pl, INT32_MAX)) || !CheckRight(pl->ID, PREMIUM_ACCOUNT)) {
		throw NOTACCESSIBLE;
	}

	print(3, "Editing guest list of house %d.\n", HouseID);
	sprintf(Buffer, "# Guests of %s\n", house->Name);
	for (int i = 0; i < house->Guests; i++) {
		strcat(Buffer, house->Guest(i));
		strcat(Buffer, "\n");
	}
}

void ChangeSubowners(unsigned short HouseID, TPlayer *pl, char *Buffer) {
	THouse *house = GetHouse(HouseID);
	if (house == nullptr) {
		error("ChangeSubowners: House with ID %d does not exist.\n", HouseID);
		throw ERROR;
	}

	if (pl == nullptr) {
		error("ChangeSubowners: pl is NULL.\n");
		throw ERROR;
	}

	if (Buffer == nullptr) {
		error("ChangeSubowners: Buffer is NULL.\n");
		throw ERROR;
	}

	if (!IsOwner(HouseID, pl)) {
		throw NOTACCESSIBLE;
	}

	Log("houses", "%s changes the list of subowners of house %d.\n", pl->Name, HouseID);

	// Quirk: appending the newline writes one byte past the terminator of the
	// caller's text.
	int l = (int)strlen(Buffer);
	Buffer[l] = '\n';
	Buffer[l + 1] = 0;

	house->Subowners = 0;
	char *p = Buffer;
	while (*p != 0) {
		char *q = strchr(p, '\n');
		if (q == nullptr) {
			error("ChangeSubowners: No newline at end of text.\n");
			break;
		}

		if (*p != '#' && p != q) {
			l = (int)(q - p);
			if (l >= 60) {
				l = 59;
			}
			p[l] = 0;
			strcpy(house->Subowner(house->Subowners), p);
			house->Subowners++;
			if (house->Subowners >= MAX_SUBOWNERS) {
				break;
			}
		}

		p = q + 1;
	}

	KickGuests(HouseID);
}

void ChangeGuests(unsigned short HouseID, TPlayer *pl, char *Buffer) {
	THouse *house = GetHouse(HouseID);
	if (house == nullptr) {
		error("ChangeGuests: House with ID %d does not exist.\n", HouseID);
		throw ERROR;
	}

	if (pl == nullptr) {
		error("ChangeGuests: pl is NULL.\n");
		throw ERROR;
	}

	if (Buffer == nullptr) {
		error("ChangeGuests: Buffer is NULL.\n");
		throw ERROR;
	}

	if ((!IsOwner(HouseID, pl) && !IsSubowner(HouseID, pl, INT32_MAX)) || !CheckRight(pl->ID, PREMIUM_ACCOUNT)) {
		throw NOTACCESSIBLE;
	}

	Log("houses", "%s changes the list of guests of house %d.\n", pl->Name, HouseID);

	// Quirk: appending the newline writes one byte past the terminator of the
	// caller's text.
	int l = (int)strlen(Buffer);
	Buffer[l] = '\n';
	Buffer[l + 1] = 0;

	house->Guests = 0;
	char *p = Buffer;
	while (*p != 0) {
		char *q = strchr(p, '\n');
		if (q == nullptr) {
			error("ChangeGuests: No newline at end of text.\n");
			break;
		}

		if (*p != '#' && p != q) {
			l = (int)(q - p);
			if (l >= 60) {
				l = 59;
			}
			p[l] = 0;
			strcpy(house->Guest(house->Guests), p);
			house->Guests++;
			if (house->Guests >= MAX_GUESTS) {
				break;
			}
		}

		p = q + 1;
	}

	KickGuests(HouseID);
}

void GetExitPosition(unsigned short HouseID, int &x, int &y, int &z) {
	THouse *house = GetHouse(HouseID);
	if (house == nullptr) {
		error("GetExitPosition: House with ID %d does not exist.\n", HouseID);
		return;
	}

	x = house->ExitX;
	y = house->ExitY;
	z = house->ExitZ;
}

void KickGuest(unsigned short HouseID, TPlayer *Guest) {
	THouse *house = GetHouse(HouseID);
	if (house == nullptr) {
		error("KickGuest(houses 1): House with ID %d does not exist.\n", HouseID);
		throw ERROR;
	}

	if (Guest == nullptr) {
		error("KickGuest(houses 1): Guest is NULL.\n");
		throw ERROR;
	}

	Object Obj = Guest->GetCreatureObject();
	GraphicalEffect(Obj, EFFECT_POFF);
	Object Bed = GetFirstObject(Guest->posx, Guest->posy, Guest->posz);
	while (Bed != NONE && !Bed.getFlag(BED)) {
		Bed++;
	}

	if (Bed != NONE && Bed.getAttribute(TEXTSTRING) != 0) {
		print(3, "Player stands on a bed while being kicked out of the house.\n");
		try {
			UseObjects(0, Bed, Bed);
		} catch (RESULT r) {
			error("KickGuest: Exception %d while clearing the bed.\n", r);
		}
	}

	Move(0, Obj, GetMapContainer(house->ExitX, house->ExitY, house->ExitZ), -1, false, NONE);
	GraphicalEffect(Obj, EFFECT_TELEPORT);
}

void KickGuest(unsigned short HouseID, TPlayer *Host, TPlayer *Guest) {
	THouse *house = GetHouse(HouseID);
	if (house == nullptr) {
		error("KickGuest(houses 2): House with ID %d does not exist.\n", HouseID);
		throw ERROR;
	}

	if (Host == nullptr) {
		error("KickGuest(houses 2): Host is NULL.\n");
		throw ERROR;
	}

	if (Guest == nullptr) {
		error("KickGuest(houses 2): Guest is NULL.\n");
		throw ERROR;
	}

	bool SelfKick = (Host == Guest);
	if (GetHouseID(Guest->posx, Guest->posy, Guest->posz) != HouseID) {
		throw NOTACCESSIBLE;
	}

	if (!IsOwner(HouseID, Host) && !IsSubowner(HouseID, Host, INT32_MAX) && !SelfKick) {
		throw NOTACCESSIBLE;
	}

	if (CheckRight(Guest->ID, ENTER_HOUSES) && !SelfKick) {
		throw NOTACCESSIBLE;
	}

	if (IsOwner(HouseID, Guest) && !SelfKick) {
		throw NOTACCESSIBLE;
	}

	KickGuest(HouseID, Guest);
}

void KickGuests(unsigned short HouseID) {
	THouse *house = GetHouse(HouseID);
	if (house == nullptr) {
		error("KickGuests: House with ID %d does not exist.\n", HouseID);
		throw ERROR;
	}

	TFindCreatures Search(MaxHouseX, MaxHouseY, house->CenterX, house->CenterY, FIND_PLAYERS);
	while (true) {
		uint32_t ID = Search.getNext();
		if (ID == 0) {
			break;
		}

		TPlayer *Guest = GetPlayer(ID);
		if (GetHouseID(Guest->posx, Guest->posy, Guest->posz) == HouseID
				&& !IsInvited(HouseID, Guest, INT32_MAX)
				&& !CheckRight(ID, ENTER_HOUSES)) {
			KickGuest(HouseID, Guest);
		}
	}
}

bool MayOpenDoor(Object Door, TPlayer *pl) {
	if (!Door.exists()) {
		error("MayOpenDoor: Door does not exist.\n");
		return false;
	}

	if (pl == nullptr) {
		error("MayOpenDoor: pl is NULL.\n");
		return false;
	}

	if (!Door.getFlag(NAMEDOOR) || !Door.getFlag(TEXT)) {
		error("MayOpenDoor: Door is not a NameDoor.\n");
		return false;
	}

	char *Text = GetDynamicString(Door.getAttribute(TEXTSTRING));
	if (Text == nullptr) {
		return false;
	}

	int l = (int)strlen(Text);
	char Buffer[l + 2];
	strcpy(Buffer, Text);
	Buffer[l] = '\n';
	Buffer[l + 1] = 0;

	char *p = Buffer;
	while (*p != 0) {
		char *q = strchr(p, '\n');
		if (q == nullptr) {
			error("MayOpenDoor: No newline at end of text.\n");
			return false;
		}

		if (*p != '#' && p != q) {
			*q = 0;
			switch (CheckAccessRight(p, pl)) {
				case -1: return false;
				case 1: return true;
			}
		}

		p = q + 1;
	}

	return false;
}

void ShowNameDoor(Object Door, TPlayer *pl, char *Buffer) {
	if (!Door.exists()) {
		error("ShowNameDoor: Door does not exist.\n");
		throw ERROR;
	}

	if (pl == nullptr) {
		error("ShowNameDoor: pl is NULL.\n");
		throw ERROR;
	}

	if (Buffer == nullptr) {
		error("ShowNameDoor: Buffer is NULL.\n");
		throw ERROR;
	}

	int x, y, z;
	GetObjectCoordinates(Door, x, y, z);

	unsigned short HouseID = GetHouseID(x, y, z);
	if (HouseID == 0) {
		error("ShowNameDoor: Door at coordinate [%d,%d,%d] belongs to no house.\n", x, y, z);
		throw ERROR;
	}

	if (!IsOwner(HouseID, pl)) {
		print(3, "Player %s is not the tenant of house %d.\n", pl->Name, HouseID);
		throw NOTACCESSIBLE;
	}

	print(3, "Editing NameDoor.\n");
	if (!Door.getFlag(TEXT)) {
		error("ShowNameDoor: Door at coordinate [%d,%d,%d] has no text.\n", x, y, z);
		throw ERROR;
	}

	char *Text = GetDynamicString(Door.getAttribute(TEXTSTRING));
	if (Text != nullptr) {
		strcpy(Buffer, Text);
	} else {
		strcpy(Buffer, "# Players allowed to open this door\n");
	}
}

void ChangeNameDoor(Object Door, TPlayer *pl, char *Buffer) {
	if (!Door.exists()) {
		error("ChangeNameDoor: Door does not exist.\n");
		throw ERROR;
	}

	if (pl == nullptr) {
		error("ChangeNameDoor: pl is NULL.\n");
		throw ERROR;
	}

	if (Buffer == nullptr) {
		error("ChangeNameDoor: Buffer is NULL.\n");
		throw ERROR;
	}

	int x, y, z;
	GetObjectCoordinates(Door, x, y, z);

	unsigned short HouseID = GetHouseID(x, y, z);
	if (HouseID == 0) {
		error("ChangeNameDoor: Door at coordinate [%d,%d,%d] belongs to no house.\n", x, y, z);
		throw ERROR;
	}

	if (!IsOwner(HouseID, pl)) {
		throw NOTACCESSIBLE;
	}

	if (!Door.getFlag(TEXT)) {
		error("ChangeNameDoor: Door at coordinate [%d,%d,%d] has no text.\n", x, y, z);
		throw ERROR;
	}

	Log("houses", "%s changes NameDoor of house %d at coordinate [%d,%d,%d].\n", pl->Name, HouseID, x, y, z);
	DeleteDynamicString(Door.getAttribute(TEXTSTRING));
	Change(Door, TEXTSTRING, AddDynamicString(Buffer));
}

int DeleteCoins(Object Con, ObjectType Type, int Amount) {
again:
	if (Amount == 0) {
		return 0;
	}

	Object Obj = GetFirstContainerObject(Con);
	while (Obj != NONE) {
		if (Obj.getObjectType() == Type) {
			if ((int)Obj.getAttribute(AMOUNT) > Amount) {
				Change(Obj, AMOUNT, Obj.getAttribute(AMOUNT) - Amount);
				return 0;
			}

			Amount -= Obj.getAttribute(AMOUNT);
			DeleteObject(Obj);
			goto again;
		}

		if (Obj.getFlag(CONTAINER)) {
			Amount = DeleteCoins(Obj, Type, Amount);
			if (Amount == 0) {
				return 0;
			}
		}

		Obj++;
	}

	return Amount;
}

void DeleteMoney(Object Con, int Amount) {
	int Crystal = CountObjects(GetFirstContainerObject(Con), GetSpecialObject(MONEY_TENTHOUSAND), 0);
	int Platinum = CountObjects(GetFirstContainerObject(Con), GetSpecialObject(MONEY_HUNDRED), 0);
	int Gold = CountObjects(GetFirstContainerObject(Con), GetSpecialObject(MONEY_ONE), 0);
	CalculateChange(Amount, Gold, Platinum, Crystal);

	if (Gold > 0) {
		DeleteCoins(Con, GetSpecialObject(MONEY_ONE), Gold);
	}

	if (Gold < 0) {
		ChangeObject(SetObject(Con, GetSpecialObject(MONEY_ONE), 0), AMOUNT, -Gold);
	}

	if (Platinum > 0) {
		DeleteCoins(Con, GetSpecialObject(MONEY_HUNDRED), Platinum);
	}

	if (Platinum < 0) {
		ChangeObject(SetObject(Con, GetSpecialObject(MONEY_HUNDRED), 0), AMOUNT, -Platinum);
	}

	if (Crystal > 0) {
		DeleteCoins(Con, GetSpecialObject(MONEY_TENTHOUSAND), Crystal);
	}
}

void CleanField(int x, int y, int z, Object Box) {
	if (Box == NONE) {
		error("CleanField: Depot box does not exist.\n");
		return;
	}

	Object Obj = GetFirstObject(x, y, z);
	while (Obj != NONE) {
		if (Obj.getFlag(NAMEDOOR)) {
			DeleteDynamicString(Obj.getAttribute(TEXTSTRING));
			Change(Obj, TEXTSTRING, 0);
			DeleteDynamicString(Obj.getAttribute(EDITOR));
			Change(Obj, EDITOR, 0);
		}

		if (!Obj.getFlag(UNMOVE) && Obj.getFlag(TAKE)) {
			Object Help = Obj;
			Obj++;
			MoveObject(Help, Box);
		} else {
			if (Obj.getFlag(CONTAINER)) {
				Object Content = GetFirstContainerObject(Obj);
				while (Content != NONE) {
					Object Help = Content;
					Content++;
					MoveObject(Help, Box);
				}
			}

			if (!Obj.getFlag(UNMOVE)) {
				Object Help = Obj;
				Obj++;
				DeleteObject(Help);
			} else {
				Obj++;
			}
		}
	}
}

void CleanHouse(THouse *house, TPlayerData *PlayerData) {
	if (house == nullptr) {
		error("CleanHouse: house is NULL.\n");
		return;
	}

	if (house->OwnerID == 0) {
		error("CleanHouse: House %d has no owner.\n", house->ID);
		return;
	}

	bool PlayerDataAssigned = false;
	if (PlayerData == nullptr) {
		PlayerData = AssignPlayerPoolSlot(house->OwnerID, false);
		if (PlayerData == nullptr) {
			error("CleanHouse: Cannot assign a slot for player data.\n");
			return;
		}
		PlayerDataAssigned = true;
	}

	int x, y, z;
	GetStartPosition(x, y, z, true);

	Object Box = SetObject(GetMapContainer(x, y, z), GetSpecialObject(DEPOT_LOCKER), 0);
	LoadDepot(PlayerData, house->DepotNr, Box);

	for (int z = 0; z <= 15; z++) {
		for (int x = house->CenterX - MaxHouseX - 1; x <= house->CenterX + MaxHouseX; x++) {
			for (int y = house->CenterY - MaxHouseY - 1; y <= house->CenterY + MaxHouseY; y++) {
				if (GetHouseID(x, y, z) == house->ID
						|| (CoordinateFlag(x, y, z, HOOKSOUTH)
							&& (GetHouseID(x - 1, y + 1, z) == house->ID
								|| GetHouseID(x, y + 1, z) == house->ID
								|| GetHouseID(x + 1, y + 1, z) == house->ID))
						|| (CoordinateFlag(x, y, z, HOOKEAST)
							&& (GetHouseID(x + 1, y - 1, z) == house->ID
								|| GetHouseID(x + 1, y, z) == house->ID
								|| GetHouseID(x + 1, y + 1, z) == house->ID))) {
					CleanField(x, y, z, Box);
				}
			}
		}
	}

	SaveDepot(PlayerData, house->DepotNr, Box);
	DeleteObject(Box);
	if (PlayerDataAssigned) {
		ReleasePlayerPoolSlot(PlayerData);
	}
}

void ClearHouse(THouse *house) {
	house->OwnerID = 0;
	house->OwnerName[0] = 0;
	house->Subowners = 0;
	house->Guests = 0;
}

void FinishAuctions() {
	int x, y, z;
	GetStartPosition(x, y, z, true);

	Object Con = SetObject(GetMapContainer(x, y, z), GetSpecialObject(DEPOT_LOCKER), 0);

	Log("houses", "Processing finished auctions...\n");

	int NumberOfAuctions = Houses;
	unsigned short HouseIDs[NumberOfAuctions];
	uint32_t CharacterIDs[NumberOfAuctions];
	char CharacterNames[NumberOfAuctions][30];
	int Bids[NumberOfAuctions];
	if (QueryManagerConnection->finishAuctions(NumberOfAuctions, HouseIDs, CharacterIDs, CharacterNames, Bids) != 0) {
		error("FinishAuctions: Cannot get auctions.\n");
		DeleteObject(Con);
		return;
	}

	char HelpWorld[60];
	AddSlashes(HelpWorld, WorldName);

	time_t TimeStamp = static_cast<int32_t>(time(nullptr));
	for (int i = 0; i < NumberOfAuctions; i++) {
		Log("houses", "Auction for house %d finished: won by %s for %d gold.\n", HouseIDs[i], CharacterNames[i], Bids[i]);
		Log("houses", "on reset: INSERT INTO HouseAssignments VALUES ('%s',%d,%d,%d);\n", HelpWorld, HouseIDs[i], CharacterIDs[i], Bids[i]);

		THouse *house = GetHouse(HouseIDs[i]);
		if (house == nullptr) {
			error("FinishAuctions: House with number %d does not exist.\n", HouseIDs[i]);
			continue;
		}

		if (house->OwnerID != 0) {
			error("FinishAuctions: House with number %d already belongs to character %ld.\n", HouseIDs[i], (long)(int32_t)house->OwnerID);
			continue;
		}

		TPlayerData *PlayerData = AssignPlayerPoolSlot(CharacterIDs[i], false);
		if (PlayerData == nullptr) {
			error("FinishAuctions: Cannot assign a slot for player data.\n");
			continue;
		}

		LoadDepot(PlayerData, house->DepotNr, Con);
		if (CountMoney(GetFirstContainerObject(Con)) < (int)(house->Rent + Bids[i])) {
			Log("houses", "Bidder does not have enough money.\n");
			house->OwnerID = CharacterIDs[i];
			strcpy(house->OwnerName, CharacterNames[i]);
			house->PaidUntil = 0;
		} else {
			Log("houses", "Bidder has enough money.\n");
			DeleteMoney(Con, house->Rent + Bids[i]);

			char Welcome[500];
			snprintf(Welcome, 500,
					"Welcome!\n"
					"\n"
					"Congratulations on your choice\n"
					"for house \"%s\".\n"
					"The rent for the first month\n"
					"has already been debited to your\n"
					"depot. The next rent will be\n"
					"payable in thirty days.\n"
					"Have a good time in your new home!",
					house->Name);
			ChangeObject(SetObject(Con, GetSpecialObject(LETTER_STAMPED), 0), TEXTSTRING, AddDynamicString(Welcome));
			SaveDepot(PlayerData, house->DepotNr, Con);

			house->OwnerID = CharacterIDs[i];
			strcpy(house->OwnerName, CharacterNames[i]);
			house->LastTransition = (int)TimeStamp;
			house->PaidUntil = (int)TimeStamp + RENT_PERIOD;
		}

		while (GetFirstContainerObject(Con) != NONE) {
			DeleteObject(GetFirstContainerObject(Con));
		}

		ReleasePlayerPoolSlot(PlayerData);
	}

	for (int i = 0; i < Houses; i++) {
		if (House(i).PaidUntil == 0 && House(i).OwnerID != 0) {
			if (QueryManagerConnection->excludeFromAuctions(House(i).OwnerID, true) != 0) {
				error("FinishAuctions: Cannot banish bidder.\n");
			}

			ClearHouse(&House(i));
		}
	}

	DeleteObject(Con);
}

void CollectRents() {
	int x, y, z;
	GetStartPosition(x, y, z, true);

	Object Con = SetObject(GetMapContainer(x, y, z), GetSpecialObject(DEPOT_LOCKER), 0);

	for (int i = 0; i < Houses; i++) {
		House(i).Help = 0;
	}

	Log("houses", "Processing voluntary terminations...\n");

	int NumberOfTransfers = Houses;
	unsigned short HouseIDs[NumberOfTransfers];
	uint32_t NewOwnerIDs[NumberOfTransfers];
	char NewOwnerNames[NumberOfTransfers][30];
	int Prices[NumberOfTransfers];
	if (QueryManagerConnection->transferHouses(NumberOfTransfers, HouseIDs, NewOwnerIDs, NewOwnerNames, Prices) != 0) {
		error("CollectRents: Cannot get terminations.\n");
		DeleteObject(Con);
		return;
	}

	char HelpWorld[60];
	AddSlashes(HelpWorld, WorldName);

	time_t TimeStamp = static_cast<int32_t>(time(nullptr));
	for (int i = 0; i < NumberOfTransfers; i++) {
		THouse *house = GetHouse(HouseIDs[i]);
		if (house == nullptr) {
			error("CollectRents: House with number %d does not exist.\n", HouseIDs[i]);
			continue;
		}

		if (NewOwnerIDs[i] != 0 && Prices[i] > 0) {
			Log("houses", "Selling house %d to %ld for %d gold.\n", HouseIDs[i], (long)(int32_t)NewOwnerIDs[i], Prices[i]);

			TPlayerData *PlayerData = AssignPlayerPoolSlot(NewOwnerIDs[i], false);
			if (PlayerData == nullptr) {
				error("CollectRents: Cannot assign a slot for player data (1a).\n");
				continue;
			}

			LoadDepot(PlayerData, house->DepotNr, Con);
			if (CountMoney(GetFirstContainerObject(Con)) < Prices[i]) {
				Log("houses", "Buyer does not have enough money.\n");

				char Warning[500];
				snprintf(Warning, 500,
						"Warning!\n"
						"\n"
						"You have not enough gold to pay\n"
						"the price for house \"%s\".\n"
						"Therefore the transfer is cancelled.",
						house->Name);
				ChangeObject(SetObject(Con, GetSpecialObject(LETTER_STAMPED), 0), TEXTSTRING, AddDynamicString(Warning));
				SaveDepot(PlayerData, house->DepotNr, Con);

				while (GetFirstContainerObject(Con) != NONE) {
					DeleteObject(GetFirstContainerObject(Con));
				}

				ReleasePlayerPoolSlot(PlayerData);

				Log("houses", "on reset: UPDATE HouseOwners SET Termination=null,NewOwner=null,Accepted=0,Price=null WHERE World='%s' AND HouseID=%d;\n", HelpWorld, HouseIDs[i]);
				QueryManagerConnection->cancelHouseTransfer(HouseIDs[i]);
				continue;
			}

			DeleteMoney(Con, Prices[i]);
			SaveDepot(PlayerData, house->DepotNr, Con);

			while (GetFirstContainerObject(Con) != NONE) {
				DeleteObject(GetFirstContainerObject(Con));
			}

			ReleasePlayerPoolSlot(PlayerData);

			PlayerData = AssignPlayerPoolSlot(house->OwnerID, false);
			if (PlayerData == nullptr) {
				error("CollectRents: Cannot assign a slot for player data (1b).\n");
				continue;
			}

			LoadDepot(PlayerData, house->DepotNr, Con);

			int Price = Prices[i];
			while (Price >= 10000) {
				int Crystal = std::min(Price / 10000, 100);
				ChangeObject(SetObject(Con, GetSpecialObject(MONEY_TENTHOUSAND), 0), AMOUNT, Crystal);
				Price -= Crystal * 10000;
			}

			if (Price >= 100) {
				ChangeObject(SetObject(Con, GetSpecialObject(MONEY_HUNDRED), 0), AMOUNT, Price / 100);
			}

			Price = Price % 100;
			if (Price > 0) {
				ChangeObject(SetObject(Con, GetSpecialObject(MONEY_ONE), 0), AMOUNT, Price);
			}

			SaveDepot(PlayerData, house->DepotNr, Con);

			while (GetFirstContainerObject(Con) != NONE) {
				DeleteObject(GetFirstContainerObject(Con));
			}

			ReleasePlayerPoolSlot(PlayerData);
		}

		Log("houses", "Vacating house %d of %ld.\n", HouseIDs[i], (long)(int32_t)house->OwnerID);
		if (NewOwnerIDs[i] == 0) {
			CleanHouse(house, nullptr);
		}

		ClearHouse(house);
		house->Help = 1;

		if (NewOwnerIDs[i] != 0) {
			Log("houses", "Transferring house %d to %ld.\n", HouseIDs[i], (long)(int32_t)NewOwnerIDs[i]);
			house->OwnerID = NewOwnerIDs[i];
			strcpy(house->OwnerName, NewOwnerNames[i]);
			house->LastTransition = (int)TimeStamp;
		}

		if (NewOwnerIDs[i] != 0) {
			Log("houses", "on reset: UPDATE HouseOwners SET Termination=%d,NewOwner=%ld,Accepted=1,Price=%d WHERE World='%s' AND HouseID=%d;\n", (int)TimeStamp, (long)(int32_t)NewOwnerIDs[i], Prices[i], HelpWorld, HouseIDs[i]);
		} else {
			Log("houses", "on reset: UPDATE HouseOwners SET Termination=%d,NewOwner=null,Accepted=0,Price=0 WHERE World='%s' AND HouseID=%d;\n", (int)TimeStamp, HelpWorld, HouseIDs[i]);
		}
	}

	int NumberOfEvictions = Houses;
	uint32_t OwnerIDs[NumberOfEvictions];
	if (QueryManagerConnection->evictFreeAccounts(NumberOfEvictions, HouseIDs, OwnerIDs) != 0) {
		error("CollectRents: Cannot get payment data.\n");
		DeleteObject(Con);
		return;
	}

	for (int i = 0; i < NumberOfEvictions; i++) {
		THouse *house = GetHouse(HouseIDs[i]);
		if (house == nullptr) {
			error("CollectRents: House with number %d does not exist.\n", HouseIDs[i]);
			continue;
		}

		Log("houses", "Account of house %d is no longer paid.\n", HouseIDs[i]);
		if (OwnerIDs[i] != house->OwnerID) {
			Log("houses", "... but house was just transferred.\n");
			continue;
		}

		CleanHouse(house, nullptr);
		ClearHouse(house);
		house->Help = 1;
	}

	NumberOfEvictions = Houses;
	if (QueryManagerConnection->evictDeletedCharacters(NumberOfEvictions, HouseIDs) != 0) {
		error("CollectRents: Cannot get deleted characters.\n");
		DeleteObject(Con);
		return;
	}

	for (int i = 0; i < NumberOfEvictions; i++) {
		THouse *house = GetHouse(HouseIDs[i]);
		if (house == nullptr) {
			error("CollectRents: House with number %d does not exist.\n", HouseIDs[i]);
			continue;
		}

		Log("houses", "Owner of house %d is deleted.\n", HouseIDs[i]);
		CleanHouse(house, nullptr);
		ClearHouse(house);
		house->Help = 1;
	}

	int NumberOfGuildhouses = 0;
	for (int i = 0; i < Houses; i++) {
		if (House(i).GuildHouse && House(i).OwnerID != 0) {
			HouseIDs[NumberOfGuildhouses] = House(i).ID;
			OwnerIDs[NumberOfGuildhouses] = House(i).OwnerID;
			NumberOfGuildhouses++;
		}
	}

	if (NumberOfGuildhouses > 0) {
		if (QueryManagerConnection->evictExGuildleaders(NumberOfGuildhouses, NumberOfEvictions, HouseIDs, OwnerIDs) != 0) {
			error("CollectRents: Cannot get guild ranks.\n");
			NumberOfEvictions = 0;
		}

		for (int i = 0; i < NumberOfEvictions; i++) {
			THouse *house = GetHouse(HouseIDs[i]);
			if (house == nullptr) {
				error("CollectRents: House with number %d does not exist.\n", HouseIDs[i]);
				continue;
			}

			Log("houses", "Tenant of guildhouse %d is no longer a guild leader.\n", HouseIDs[i]);
			CleanHouse(house, nullptr);
			ClearHouse(house);
			house->Help = 1;
		}
	}

	for (int i = 0; i < Houses; i++) {
		if (House(i).Help == 1) {
			if (QueryManagerConnection->deleteHouseOwner(House(i).ID) != 0) {
				error("CollectRents: Cannot remove house %d from HouseOwners.\n", House(i).ID);
			}
		}
	}

	Log("houses", "Collecting rents...\n");
	for (int i = 0; i < Houses; i++) {
		if (House(i).OwnerID == 0 || House(i).PaidUntil > TimeStamp) {
			continue;
		}

		Log("houses", "Collecting rent for house %d from %ld.\n", House(i).ID, (long)(int32_t)House(i).OwnerID);

		time_t Deadline = std::max(House(i).PaidUntil + 604800, PaymentExtension);
		TPlayerData *PlayerData = AssignPlayerPoolSlot(House(i).OwnerID, false);
		if (PlayerData == nullptr) {
			error("CollectRents: Cannot assign a slot for player data (2).\n");
			continue;
		}

		LoadDepot(PlayerData, House(i).DepotNr, Con);
		if (CountMoney(GetFirstContainerObject(Con)) < (int)House(i).Rent) {
			if (TimeStamp < Deadline) {
				Log("houses", "Tenant gets a reminder.\n");

				char Warning[500];
				int Days = (int)(Deadline - TimeStamp - 3600) / 86400 + 1;
				snprintf(Warning, 500,
						"Warning!\n"
						"\n"
						"The monthly rent of %ld gold\n"
						"for your house \"%s\"\n"
						"is payable. Have it available\n"
						"within %d day%s, or you will\n"
						"lose this house.",
						(long)(int32_t)House(i).Rent, House(i).Name, Days, (Days != 1) ? "s" : "");
				ChangeObject(SetObject(Con, GetSpecialObject(LETTER_STAMPED), 0), TEXTSTRING, AddDynamicString(Warning));
				SaveDepot(PlayerData, House(i).DepotNr, Con);
			} else {
				Log("houses", "Tenant is evicted.\n");
				if (QueryManagerConnection->excludeFromAuctions(House(i).OwnerID, false) != 0) {
					error("CollectRents: Cannot banish tenant.\n");
				}

				CleanHouse(&House(i), PlayerData);
				ClearHouse(&House(i));
			}
		} else {
			Log("houses", "Tenant has enough money.\n");
			DeleteMoney(Con, House(i).Rent);
			SaveDepot(PlayerData, House(i).DepotNr, Con);
			House(i).PaidUntil += RENT_PERIOD;
		}

		while (GetFirstContainerObject(Con) != NONE) {
			DeleteObject(GetFirstContainerObject(Con));
		}

		ReleasePlayerPoolSlot(PlayerData);
	}

	DeleteObject(Con);
}

void StartAuctions() {
	Log("houses", "Starting new auctions...\n");

	for (int i = 0; i < Houses; i++) {
		House(i).Help = 0;
	}

	int NumberOfAuctions = Houses;
	unsigned short HouseIDs[NumberOfAuctions];
	if (QueryManagerConnection->getAuctions(NumberOfAuctions, HouseIDs) != 0) {
		error("StartAuctions: Cannot get running auctions.\n");
		return;
	}

	for (int i = 0; i < NumberOfAuctions; i++) {
		THouse *house = GetHouse(HouseIDs[i]);
		if (house == nullptr) {
			error("StartAuctions: House with number %d does not exist (1).\n", HouseIDs[i]);
			continue;
		}

		house->Help = 1;
	}

	char HelpWorld[60];
	AddSlashes(HelpWorld, WorldName);

	for (int i = 0; i < Houses; i++) {
		if (House(i).OwnerID == 0 && House(i).Help == 0 && !House(i).NoAuction) {
			Log("houses", "Registering house %d for auction.\n", House(i).ID);
			Log("houses", "on reset: DELETE FROM Auctions WHERE World='%s' AND HouseID=%d;\n", HelpWorld, House(i).ID);
			if (QueryManagerConnection->startAuction(House(i).ID) != 0) {
				error("StartAuctions: Cannot register house %d for auction.\n", House(i).ID);
			}
		}
	}

	for (int i = 0; i < Houses; i++) {
		House(i).Help = 0;
	}

	int NumberOfHouses = Houses;
	uint32_t OwnerIDs[NumberOfHouses];
	char OwnerNames[NumberOfHouses][30];
	int PaidUntils[NumberOfHouses];
	if (QueryManagerConnection->getHouseOwners(NumberOfHouses, HouseIDs, OwnerIDs, OwnerNames, PaidUntils) != 0) {
		error("StartAuctions: Cannot get registered rented houses.\n");
		return;
	}

	for (int i = 0; i < NumberOfHouses; i++) {
		THouse *house = GetHouse(HouseIDs[i]);
		if (house == nullptr) {
			error("StartAuctions: House with number %d does not exist (2).\n", HouseIDs[i]);
			continue;
		}

		house->Help = 1;
		if (house->OwnerID != OwnerIDs[i]) {
			house->Help = 2;
		}

		if (house->PaidUntil != PaidUntils[i]) {
			house->Help = 2;
		}
	}

	Log("houses", "Updating list of tenants...\n");
	for (int i = 0; i < Houses; i++) {
		if (House(i).OwnerID == 0 && House(i).Help != 0) {
			Log("houses", "Removing house %d, which is no longer rented.\n", House(i).ID);
			if (QueryManagerConnection->deleteHouseOwner(House(i).ID) != 0) {
				error("StartAuctions: Cannot remove house %d, which is no longer rented.\n", House(i).ID);
			}
		}

		if (House(i).OwnerID != 0 && House(i).Help == 0) {
			Log("houses", "Registering rented house %d.\n", House(i).ID);
			if (QueryManagerConnection->insertHouseOwner(House(i).ID, House(i).OwnerID, House(i).PaidUntil) != 0) {
				error("StartAuctions: Cannot register rented house %d.\n", House(i).ID);
			}
		}

		if (House(i).Help == 2) {
			Log("houses", "Updating rented house %d.\n", House(i).ID);
			if (QueryManagerConnection->updateHouseOwner(House(i).ID, House(i).OwnerID, House(i).PaidUntil) != 0) {
				error("StartAuctions: Cannot update data of rented house %d.\n", House(i).ID);
			}
		}
	}
}

void PrepareHouseCleanup() {
	HelpDepots = 0;
}

void FinishHouseCleanup() {
	for (int i = 1; i <= HelpDepots; i++) {
		TPlayerData *PlayerData = AssignPlayerPoolSlot(HelpDepot(i).CharacterID, false);
		if (PlayerData == nullptr) {
			error("FinishHouseCleanup: Cannot assign a slot for player data.\n");
			return;
		}

		SaveDepot(PlayerData, HelpDepot(i).DepotNr, HelpDepot(i).Box);
		DeleteObject(HelpDepot(i).Box);
		ReleasePlayerPoolSlot(PlayerData);
	}

	HelpDepots = 0;
}

void CleanHouseField(int x, int y, int z) {
	unsigned short HouseID = GetHouseID(x, y, z);
	if (HouseID == 0 && CoordinateFlag(x, y, z, HOOKSOUTH)) {
		HouseID = GetHouseID(x, y + 1, z);
		if (HouseID == 0) {
			HouseID = GetHouseID(x - 1, y + 1, z);
		}

		if (HouseID == 0) {
			HouseID = GetHouseID(x + 1, y + 1, z);
		}
	}

	if (HouseID == 0 && CoordinateFlag(x, y, z, HOOKEAST)) {
		HouseID = GetHouseID(x + 1, y, z);
		if (HouseID == 0) {
			HouseID = GetHouseID(x + 1, y - 1, z);
		}

		if (HouseID == 0) {
			HouseID = GetHouseID(x + 1, y + 1, z);
		}
	}

	if (HouseID == 0) {
		error("CleanHouseField: No house found for field [%d,%d,%d].\n", x, y, z);
		return;
	}

	THouse *house = GetHouse(HouseID);
	if (house == nullptr || house->OwnerID == 0) {
		return;
	}

	int DepotLoaded = -1;
	for (int i = 1; i <= HelpDepots; i++) {
		if (HelpDepot(i).CharacterID == house->OwnerID && HelpDepot(i).DepotNr == house->DepotNr) {
			DepotLoaded = i;
		}
	}

	if (DepotLoaded == -1) {
		TPlayerData *PlayerData = AssignPlayerPoolSlot(house->OwnerID, false);
		if (PlayerData == nullptr) {
			error("CleanHouseField: Cannot assign a slot for player data.\n");
			return;
		}

		HelpDepots++;
		HelpDepot(HelpDepots).CharacterID = house->OwnerID;
		HelpDepot(HelpDepots).DepotNr = house->DepotNr;

		int sx, sy, sz;
		GetStartPosition(sx, sy, sz, true);
		HelpDepot(HelpDepots).Box = SetObject(GetMapContainer(sx, sy, sz), GetSpecialObject(DEPOT_LOCKER), 0);
		LoadDepot(PlayerData, house->DepotNr, HelpDepot(HelpDepots).Box);
		DepotLoaded = HelpDepots;
		ReleasePlayerPoolSlot(PlayerData);
	}

	CleanField(x, y, z, HelpDepot(DepotLoaded).Box);
}

void LoadHouses() {
	// houseareas.dat and houses.dat, which the two lines named, are one file
	// now; the areas come first in it.
	Log("houses", "Loading house area data...\n");
	Log("houses", "Loading house data...\n");

	char FileName[4096];
	sprintf(FileName, "%s/houses.toml", DATAPATH);
	LoadHouseData(FileName, SetHouseID);

	QueryManagerConnection = new TQueryManagerConnection(std::max(0x4000, Houses * 600));

	unsigned short HouseIDs[Houses];
	int PositionsX[Houses], PositionsY[Houses], PositionsZ[Houses];
	char *Names[Houses], *Descriptions[Houses], Towns[Houses][30];
	int Rents[Houses], Sizes[Houses];
	bool Guildhouses[Houses];
	for (int i = 0; i < Houses; i++) {
		HouseIDs[i] = House(i).ID;
		Names[i] = House(i).Name;
		Rents[i] = House(i).Rent;
		Descriptions[i] = House(i).Description;
		Sizes[i] = House(i).Size;
		PositionsX[i] = House(i).CenterX;
		PositionsY[i] = House(i).CenterY;
		PositionsZ[i] = House(i).CenterZ;
		strcpy(Towns[i], GetDepotName(House(i).DepotNr));
		Guildhouses[i] = House(i).GuildHouse;
	}

	if (QueryManagerConnection->insertHouses(Houses, HouseIDs, Names, Rents, Descriptions, Sizes, PositionsX, PositionsY, PositionsZ, Towns, Guildhouses) != 0) {
		error("LoadHouses: Cannot register the master data of the houses.\n");
	}
}

void LoadOwners() {
	Log("houses", "Loading tenant data...\n");

	PaymentExtension = 0;

	bool ClearGuests = false;
	bool ClearBeds = false;
	char FileName[4096];
	sprintf(FileName, "%s/owners.dat", DATAPATH);
	if (!FileExists(FileName)) {
		Log("houses", "No tenant data found.\n");
		return;
	}

	TReadScriptFile Script;
	Script.open(FileName);
	while (true) {
		Script.nextToken();
		if (Script.Token == ENDOFFILE) {
			break;
		}

		if (Script.Token != IDENTIFIER) {
			Script.error("Identifier expected");
		}

		if (strcmp(Script.getIdentifier(), "extension") == 0) {
			Script.readSymbol('=');
			PaymentExtension = Script.readNumber();
			continue;
		}

		if (strcmp(Script.getIdentifier(), "clearguests") == 0) {
			ClearGuests = true;
			continue;
		}

		if (strcmp(Script.getIdentifier(), "clearbeds") == 0) {
			ClearBeds = true;
			continue;
		}

		if (strcmp(Script.getIdentifier(), "id") != 0) {
			Script.error("Unknown identifier");
		}

		Script.readSymbol('=');
		unsigned short ID = Script.readNumber();
		THouse *house = GetHouse(ID);
		if (house == nullptr) {
			error("LoadOwners: House with ID %d does not exist.", ID);
			throw "Cannot load owners";
		}

		Script.readIdentifier();
		Script.readSymbol('=');
		house->OwnerID = Script.readNumber();

		Script.readIdentifier();
		Script.readSymbol('=');
		house->LastTransition = Script.readNumber();

		Script.readIdentifier();
		Script.readSymbol('=');
		house->PaidUntil = Script.readNumber();

		Script.readIdentifier();
		Script.readSymbol('=');
		Script.readSymbol('{');
		while (true) {
			Script.nextToken();
			if (Script.Token == SPECIAL && Script.getSpecial() == '}') {
				break;
			}

			if (Script.Token == SPECIAL && Script.getSpecial() == ',') {
				continue;
			}

			strcpy(house->Guest(house->Guests), Script.getString());
			house->Guests++;
		}

		Script.readIdentifier();
		Script.readSymbol('=');
		Script.readSymbol('{');
		while (true) {
			Script.nextToken();
			if (Script.Token == SPECIAL && Script.getSpecial() == '}') {
				break;
			}

			if (Script.Token == SPECIAL && Script.getSpecial() == ',') {
				continue;
			}

			strcpy(house->Subowner(house->Subowners), Script.getString());
			house->Subowners++;
		}

		if (ClearGuests) {
			house->Guests = 0;
		}
	}

	Script.close();

	int NumberOfHouses = Houses;
	unsigned short HouseIDs[NumberOfHouses];
	uint32_t OwnerIDs[NumberOfHouses];
	char OwnerNames[NumberOfHouses][30];
	int PaidUntils[NumberOfHouses];
	if (QueryManagerConnection->getHouseOwners(NumberOfHouses, HouseIDs, OwnerIDs, OwnerNames, PaidUntils) != 0) {
		error("LoadOwners: Cannot get names of tenants.\n");
		throw "Cannot load owners";
	}

	for (int i = 0; i < NumberOfHouses; i++) {
		THouse *house = GetHouse(HouseIDs[i]);
		if (house == nullptr) {
			error("LoadOwners: House %d does not exist.\n", HouseIDs[i]);
			continue;
		}

		if (house->OwnerID != 0) {
			strcpy(house->OwnerName, OwnerNames[i]);
		}
	}

	if (ClearBeds) {
		print(1, "Clearing all beds...\n");
		for (int i = 0; i < Houses; i++) {
			THouse &house = House(i);
			for (int x = house.CenterX - MaxHouseX; x <= house.CenterX + MaxHouseX; x++) {
				for (int y = house.CenterY - MaxHouseY; y <= house.CenterY + MaxHouseY; y++) {
					for (int z = SectorZMin; z <= SectorZMax; z++) {
						Object Bed = GetFirstObject(x, y, z);
						while (Bed != NONE && !Bed.getFlag(BED)) {
							Bed++;
						}

						if (Bed == NONE || Bed.getAttribute(TEXTSTRING) == 0) {
							continue;
						}

						try {
							UseObjects(0, Bed, Bed);
						} catch (RESULT r) {
							// Quirk: no argument is passed for the %d.
							error("LoadOwners: Exception %d while clearing a bed.\n");
						}
					}
				}
			}
		}
	}
}

void SaveOwners() {
	Log("houses", "Saving tenant data...\n");

	char FileName[4096];
	sprintf(FileName, "%s/owners.dat", DATAPATH);

	TWriteScriptFile Script;
	Script.open(FileName);
	if (PaymentExtension > (int)time(nullptr)) {
		Script.writeText("Extension = ");
		Script.writeNumber(PaymentExtension);
		Script.writeLn();
		Script.writeLn();
	}

	for (int i = 0; i < Houses; i++) {
		if (House(i).OwnerID == 0) {
			continue;
		}

		Script.writeText("ID = ");
		Script.writeNumber(House(i).ID);
		Script.writeLn();
		Script.writeText("Owner = ");
		Script.writeNumber(House(i).OwnerID);
		Script.writeLn();
		Script.writeText("LastTransition = ");
		Script.writeNumber(House(i).LastTransition);
		Script.writeLn();
		Script.writeText("PaidUntil = ");
		Script.writeNumber(House(i).PaidUntil);
		Script.writeLn();

		Script.writeText("Guests = {");
		for (int j = 0; j < House(i).Guests; j++) {
			if (j > 0) {
				Script.writeText(",");
			}
			Script.writeString(House(i).Guest(j));
		}
		Script.writeText("}");
		Script.writeLn();

		Script.writeText("Subowners = {");
		for (int j = 0; j < House(i).Subowners; j++) {
			if (j > 0) {
				Script.writeText(",");
			}
			Script.writeString(House(i).Subowner(j));
		}
		Script.writeText("}");
		Script.writeLn();
		Script.writeLn();
	}

	Script.close();
}

void InitHouses() {
	try {
		InitLog("houses");
		LoadHouses();
		LoadOwners();
		FinishAuctions();
		CollectRents();
		StartAuctions();
		delete QueryManagerConnection;
	} catch (RESULT r) {
		error("InitHouses: Exception %d.\n", r);
		throw "Cannot initialize houses";
	}
}

void ExitHouses() {
	try {
		SaveOwners();
	} catch (const char *s) {
		error("ExitHouses: Error while saving the owners (%s).\n", s);
	}
}
