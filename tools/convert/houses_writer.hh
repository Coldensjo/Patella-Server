// houses_writer.hh: writes the game's house areas and houses as houses.toml
// (houses_writer.cc).

#ifndef PATELLA_CONVERT_HOUSES_WRITER_HH
#define PATELLA_CONVERT_HOUSES_WRITER_HH

#include <string>

#include "houses_dat.hh"

// The text of houses.toml for the areas and houses as the game's HouseArea
// and House hold them, with what Extras noted: the area names, and each
// house's rent offset, area and fields. Throws a const char * for what
// houses.toml cannot hold: an area number twice, or a coordinate outside x
// and y 0 to 65535 and z 0 to 15; and for tables that do not go together
// with Extras (a house whose area is not there, or whose rent is not its
// area's price times its fields plus its rent offset).
std::string HousesToToml(const THousesDatExtras &Extras);

#endif
