// housesdata.hh: the houses as the world defines them (housesdata.cc): the
// house areas and the houses, their tables and their loader, which reads
// houses.toml. houses.cc holds the owners, guests, doors, rents and auctions.

#ifndef PATELLA_HOUSESDATA_HH
#define PATELLA_HOUSESDATA_HH

#include <cstdint>

#include "containr.hh"
#include "tomlfile.hh"

struct THouseGuest {
	char Name[60];
	operator char *() { return Name; }
};

struct THouseArea {
	unsigned short ID;
	int SQMPrice;
	int DepotNr;
};

struct THouse {
	unsigned short ID;
	char Name[50];
	char Description[500];
	int Size;
	uint32_t Rent;
	int DepotNr;
	bool NoAuction;
	bool GuildHouse;
	int ExitX;
	int ExitY;
	int ExitZ;
	int CenterX;
	int CenterY;
	int CenterZ;
	uint32_t OwnerID;
	char OwnerName[30];
	int LastTransition;
	int PaidUntil;
	vector<THouseGuest> Subowner;
	int Subowners;
	vector<THouseGuest> Guest;
	int Guests;
	int Help;
	THouse() : Subowner(0, 4, 5), Guest(0, 9, 10) {}
};

// The fields houses.toml may give (exit, center and the house's fields): x
// and y 0 to 65535, z 0 to 15.
const int MAX_HOUSE_COORDINATE_XY = 65535;
const int MAX_HOUSE_COORDINATE_Z = 15;

// What the loader does with each field of a house, in the order of
// houses.toml: SetHouseID (map.cc) in the game.
typedef void THouseFieldFunction(int x, int y, int z, unsigned short HouseID);

// Reads the [[area]] and [[house]] entries of a parsed houses.toml into
// HouseArea and House, in their order, as the game's old LoadHouseAreas and
// LoadHouses did from houseareas.dat and houses.dat: HouseAreas, Houses,
// MaxHouseX and MaxHouseY start from 0; each house gets its area's depot,
// Rent = the area's SQMPrice * Size + rent_offset, NoAuction false and no
// owner, subowners or guests, and SetField is called for each of its fields.
// Logs "LoadHouses: Exit for house ... not set." for an exit or a center with
// x 0, as the old reader did. Throws a const char *
// "<file>:<line>:<column>: <what>" for what is wrong with it.
void ReadHouseData(const toml::table &File, THouseFieldFunction *SetField);

// ReadHouseData of the houses.toml file FileName.
void LoadHouseData(const char *FileName, THouseFieldFunction *SetField);

THouseArea *GetHouseArea(unsigned short ID);

extern vector<THouseArea> HouseArea;
extern int HouseAreas;
extern vector<THouse> House;
extern int Houses;
extern int MaxHouseX;
extern int MaxHouseY;

#endif
