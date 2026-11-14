// mapdata.hh: the map's settings as the world defines them (mapdata.cc): the
// range of the sectors, the cylinders refreshed per minute, the memory
// settings, the start positions, the marks and the depots, with their loader,
// which reads map.toml. map.cc holds the objects, the sectors and what is done
// with them.

#ifndef PATELLA_MAPDATA_HH
#define PATELLA_MAPDATA_HH

#include "containr.hh"

// The objects are kept in blocks of OBJECTS_PER_BLOCK (map.toml's cache_size
// is a multiple of it).
const int OBJECTS_PER_BLOCK = 32768;

// The sectors map.toml may give: x and y 1 to 2046, z 0 to 15.
const int MIN_SECTOR_XY = 1;
const int MAX_SECTOR_XY = 2046;
const int MAX_SECTOR_Z = 15;

// The fields map.toml may give (the start positions and the marks): x and y 0
// to 65535, z 0 to 15. A start position's x 0 means none.
const int MAX_MAP_COORDINATE_XY = 65535;
const int MAX_MAP_COORDINATE_Z = 15;

struct TDepotInfo {
	char Town[20];
	int Size;
};

struct TMark {
	char Name[20];
	int x;
	int y;
	int z;
};

// Reads the map.toml file FileName into the variables below, as the game's
// old ReadMapConfig did from map.dat and the mem.dat it included: the depots
// into DepotInfo by their numbers, the marks into Mark in their order, and
// OBCount as the number of blocks of OBJECTS_PER_BLOCK objects cache_size
// gives. Throws a const char * "<file>:<line>:<column>: <what>" for what is
// wrong with it, and as TomlParseFile does for a file that is not there or
// not TOML.
void LoadMapData(const char *FileName);

// LoadMapData of DATAPATH/map.toml.
void ReadMapConfig();

extern int SectorXMin;
extern int SectorXMax;
extern int SectorYMin;
extern int SectorYMax;
extern int SectorZMin;
extern int SectorZMax;
extern int RefreshedCylinders;
extern int HashTableSize;
extern int OBCount;
extern vector<TDepotInfo> DepotInfo;
extern vector<TMark> Mark;
extern int Marks;
extern int NewbieStartPositionX;
extern int NewbieStartPositionY;
extern int NewbieStartPositionZ;
extern int VeteranStartPositionX;
extern int VeteranStartPositionY;
extern int VeteranStartPositionZ;

#endif
