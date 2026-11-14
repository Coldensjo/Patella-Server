// houses_dat.hh: the readers of the original houseareas.dat and houses.dat
// (houses_dat.cc), the game's LoadHouseAreas and LoadHouses as they were
// before the game read houses.toml.

#ifndef PATELLA_CONVERT_HOUSES_DAT_HH
#define PATELLA_CONVERT_HOUSES_DAT_HH

#include <string>
#include <vector>

// A field of a house, as houses.dat gives it.
struct THouseDatField {
	int x;
	int y;
	int z;
};

// What the game takes from a house of houses.dat but does not keep in THouse:
// its rent offset (THouse's Rent is the sum), the number of its area (THouse
// keeps only the area's depot) and its fields, in the order of the file (the
// game marked them on the map with SetHouseID).
struct THouseDatHouse {
	int RentOffset = 0;
	unsigned short Area = 0;
	std::vector<THouseDatField> Fields;
};

// What the conversion needs besides the game's tables: the name of each area
// (which the game skipped), by its index in HouseArea, and the above for each
// house, by its index in House.
struct THousesDatExtras {
	std::vector<std::string> AreaNames;
	std::vector<THouseDatHouse> Houses;
};

// Reads the houseareas.dat file FileName into the game's HouseArea and
// HouseAreas (housesdata.hh), as the game's old LoadHouseAreas did. Throws a
// const char * as it did: the script reader's errors, also for a file that
// is not there.
void ReadHouseAreasDat(const char *FileName, THousesDatExtras &Extras);

// Reads the houses.dat file FileName into the game's House, Houses, MaxHouseX
// and MaxHouseY, as the game's old LoadHouses did before it handed the houses
// to the query manager. The areas must be read. Throws a const char * as it
// did ("Cannot load houses" and the script reader's errors, also for a file
// that is not there), and for a name longer than 49 characters or a
// description longer than 499, where the old reader wrote past THouse's Name
// or Description.
void ReadHousesDat(const char *FileName, THousesDatExtras &Extras);

#endif
