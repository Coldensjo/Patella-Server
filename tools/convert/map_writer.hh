// map_writer.hh: writes the game's map settings as map.toml (map_writer.cc).

#ifndef PATELLA_CONVERT_MAP_WRITER_HH
#define PATELLA_CONVERT_MAP_WRITER_HH

#include <string>

#include "map_dat.hh"

// The text of map.toml for the map settings as the game's variables
// (mapdata.hh) hold them, with the depots Extras names. Throws a const char *
// for what map.toml cannot hold: a start position or a mark outside x and y 0
// to 65535 and z 0 to 15, or a start position with x 0; and for variables the
// old reader cannot have left (a sector range, objects or a cache size it
// refuses, a depot number outside 0 to 8).
std::string MapToToml(const TMapDatExtras &Extras);

#endif
