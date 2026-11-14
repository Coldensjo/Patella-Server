// map_dat.hh: the reader of the original map.dat (map_dat.cc), the game's
// ReadMapConfig as it was before the game read map.toml.

#ifndef PATELLA_CONVERT_MAP_DAT_HH
#define PATELLA_CONVERT_MAP_DAT_HH

#include <vector>

// What the conversion needs besides the game's variables: the numbers of the
// depots map.dat gave (DepotInfo holds entries for others too, which no one
// set), ascending, each once.
struct TMapDatExtras {
	std::vector<int> Depots;
};

// Reads the map.dat file FileName, and the files it includes (mem.dat, found
// next to it), into the game's variables (mapdata.hh), as the game's old
// ReadMapConfig did. Throws a const char * as it did: the script reader's
// errors, also for a file that is not there, and the texts of its checks
// ("illegal value for SectorXMin", ...); and, as script errors with the file
// and line, for an identifier longer than 29 characters or a mark name longer
// than 19, where the old reader wrote past its buffers, and for a depot
// number outside 0 to 8, which map.toml cannot hold (LoadDepot refuses such a
// depot).
void ReadMapDat(const char *FileName, TMapDatExtras &Extras);

#endif
