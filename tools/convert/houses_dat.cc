// houses_dat.cc: the readers of the original houseareas.dat and houses.dat
// (houses_dat.hh). These are the game's LoadHouseAreas and LoadHouses as they
// were before houses.toml, up to where LoadHouses handed the houses to the
// query manager, moved here unchanged but for these things: the file names
// are given instead of made from DATAPATH; the log lines are left out (the
// TOML may go to standard output); the FileExists checks are left out, so a
// file that is not there is the script reader's error instead of no areas or
// houses; HouseAreas starts from 0 as the game's did; a name longer than 49
// characters or a description longer than 499 is refused where the old
// reader wrote past THouse's Name or Description; and what the game did not
// keep is noted for the conversion: the area names, and each house's rent
// offset, area number and fields, which the old reader marked on the map
// with SetHouseID.
//
// Quirks of the format the readers keep: the names before "=" in houses.dat
// are not looked at, only the order of the values counts; GuildHouse is true
// only for the identifier true (in any case), anything else is false; the
// commas between the fields may be left out or doubled.

#include <cstdio>
#include <cstdlib>
#include <cstring>

#include "script.hh"
#include "utils.hh"
#include "housesdata.hh"

#include "houses_dat.hh"

void ReadHouseAreasDat(const char *FileName, THousesDatExtras &Extras) {
	HouseAreas = 0;
	Extras.AreaNames.clear();

	TReadScriptFile Script;
	Script.open(const_cast<char *>(FileName));
	while (true) {
		Script.nextToken();
		if (Script.Token == ENDOFFILE) {
			break;
		}

		if (Script.Token != IDENTIFIER) {
			Script.error("Identifier expected");
		}

		Script.readSymbol('=');
		Script.readSymbol('(');
		HouseArea(HouseAreas).ID = Script.readNumber();
		Script.readSymbol(',');
		Extras.AreaNames.push_back(Script.readString());
		Script.readSymbol(',');
		HouseArea(HouseAreas).SQMPrice = Script.readNumber();
		Script.readSymbol(',');
		HouseArea(HouseAreas).DepotNr = Script.readNumber();
		Script.readSymbol(')');
		HouseAreas++;
	}

	Script.close();
}

void ReadHousesDat(const char *FileName, THousesDatExtras &Extras) {
	Houses = 0;
	MaxHouseX = 0;
	MaxHouseY = 0;
	Extras.Houses.clear();

	int x, y, z;
	TReadScriptFile Script;
	Script.open(const_cast<char *>(FileName));
	while (true) {
		Script.nextToken();
		if (Script.Token == ENDOFFILE) {
			break;
		}

		if (Script.Token != IDENTIFIER) {
			Script.error("Identifier expected");
		}

		Extras.Houses.emplace_back();
		THouseDatHouse &Noted = Extras.Houses.back();

		Script.readSymbol('=');
		House(Houses).ID = Script.readNumber();
		if (House(Houses).ID == 0) {
			error("LoadHouses: Invalid ID %d.\n", House(Houses).ID);
			Script.close();
			throw "Cannot load houses";
		}

		if (Houses > 0 && House(Houses).ID <= House(Houses - 1).ID) {
			error("LoadHouses: IDs not sorted in ascending order (ID=%d).\n", House(Houses).ID);
			Script.close();
			throw "Cannot load houses";
		}

		Script.readIdentifier();
		Script.readSymbol('=');
		if (strlen(Script.readString()) >= sizeof(House(Houses).Name)) {
			Script.close();
			throw "a house name longer than 49 characters, where the old reader wrote past THouse's Name";
		}
		strcpy(House(Houses).Name, Script.getString());

		Script.readIdentifier();
		Script.readSymbol('=');
		if (strlen(Script.readString()) >= sizeof(House(Houses).Description)) {
			Script.close();
			throw "a house description longer than 499 characters, where the old reader wrote past THouse's "
					"Description";
		}
		strcpy(House(Houses).Description, Script.getString());

		Script.readIdentifier();
		Script.readSymbol('=');
		int RentOffset = Script.readNumber();
		Noted.RentOffset = RentOffset;

		Script.readIdentifier();
		Script.readSymbol('=');
		THouseArea *Area = GetHouseArea(Script.readNumber());
		if (Area == nullptr) {
			error("LoadHouses: Area for house %d does not exist.\n", House(Houses).ID);
			Script.close();
			throw "Cannot load houses";
		}

		Noted.Area = Area->ID;
		House(Houses).DepotNr = Area->DepotNr;

		Script.readIdentifier();
		Script.readSymbol('=');
		House(Houses).GuildHouse = (strcmp(Script.readIdentifier(), "true") == 0);
		House(Houses).NoAuction = false;

		Script.readIdentifier();
		Script.readSymbol('=');
		Script.readCoordinate(House(Houses).ExitX, House(Houses).ExitY, House(Houses).ExitZ);
		if (House(Houses).ExitX == 0) {
			error("LoadHouses: Ausgang f\xfcr Haus %d nicht gesetzt.\n", House(Houses).ID);
		}

		Script.readIdentifier();
		Script.readSymbol('=');
		Script.readCoordinate(House(Houses).CenterX, House(Houses).CenterY, House(Houses).CenterZ);
		if (House(Houses).CenterX == 0) {
			error("LoadHouses: Ausgang f\xfcr Haus %d nicht gesetzt.\n", House(Houses).ID);
		}

		Script.readIdentifier();
		Script.readSymbol('=');
		Script.readSymbol('{');
		House(Houses).Size = 0;
		while (true) {
			Script.nextToken();
			if (Script.Token == SPECIAL && Script.getSpecial() == '}') {
				break;
			}

			if (Script.Token == SPECIAL && Script.getSpecial() == ',') {
				continue;
			}

			Script.getCoordinate(x, y, z);
			// The game called SetHouseID(x, y, z, House(Houses).ID) here.
			Noted.Fields.push_back({x, y, z});
			House(Houses).Size++;

			if (abs(x - House(Houses).CenterX) > MaxHouseX) {
				MaxHouseX = abs(x - House(Houses).CenterX);
			}

			if (abs(y - House(Houses).CenterY) > MaxHouseY) {
				MaxHouseY = abs(y - House(Houses).CenterY);
			}
		}

		House(Houses).Rent = Area->SQMPrice * House(Houses).Size + RentOffset;
		House(Houses).OwnerID = 0;
		House(Houses).OwnerName[0] = 0;
		House(Houses).LastTransition = 0;
		House(Houses).PaidUntil = 0;
		House(Houses).Subowners = 0;
		House(Houses).Guests = 0;
		Houses++;
	}

	Script.close();
}
