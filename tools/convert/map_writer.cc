// map_writer.cc: writes the game's map settings as map.toml (map_writer.hh).

#include "map_writer.hh"

#include <cstdint>

#include "dbfuncs.hh"
#include "mapdata.hh"
#include "tomlwrite.hh"

static std::string Header() {
	std::string Text;
	Text += "# map.toml: the map settings of the game server, converted from the original\n";
	Text += "# map.dat and the mem.dat it includes by patella-convert. The game reads\n";
	Text += "# map.toml in its data folder ([paths] data of config.toml). An unknown or\n";
	Text += "# missing key, a value of the wrong type or out of range, or a depot given\n";
	Text += "# twice, is an error.\n";
	Text += "#\n";
	Text += "#   refreshed_cylinders  How many cylinders the game refreshes each minute.\n";
	Text += "#                        A cylinder is the sectors of one x and y on all\n";
	Text += "#                        floors; the game goes through them row by row and\n";
	Text += "#                        loads the fields marked Refresh in them anew from\n";
	Text += "#                        the original map, but in sectors near a player.\n";
	Text += "#   newbie_start         The field [x, y, z] new characters start on, their\n";
	Text += "#                        home; a player without a vocation whose home lies\n";
	Text += "#                        in a premium area gets it as home again while his\n";
	Text += "#                        account is not premium. x 1 to 65535.\n";
	Text += "#   veteran_start        The same for a player with a vocation. x 1 to 65535.\n";
	Text += "#\n";
	Text += "# [sectors] is the extent of the map in sectors of 32 by 32 fields (the files\n";
	Text += "# XXXX-YYYY-ZZ.sec): x_min to x_max and y_min to y_max (1 to 2046), and the\n";
	Text += "# floors z_min to z_max (0 to 15, 7 is the ground).\n";
	Text += "#\n";
	Text += "# [memory] is the room for the objects:\n";
	Text += "#   objects     How many object numbers the game makes room for at first, a\n";
	Text += "#               power of 2; it doubles them when few are left.\n";
	Text += "#   cache_size  How many objects the game keeps in memory, a multiple of\n";
	Text += "#               32768; when they are all in use, it swaps out the sector used\n";
	Text += "#               longest ago.\n";
	Text += "#\n";
	Text += "# Each [[mark]] is a named field a gamemaster can teleport to:\n";
	Text += "#   name      Its name, at most 19 characters. A name is looked up in any\n";
	Text += "#             case, and the first mark of that name counts.\n";
	Text += "#   position  Its field [x, y, z].\n";
	Text += "#\n";
	Text += "# Each [[depot]] is a town's depot:\n";
	Text += "#   id    Its number, 0 to 8 (a player has one depot of each number), each\n";
	Text += "#         at most once. The move and use rules (load_depot, save_depot)\n";
	Text += "#         and the areas of houses.toml name it.\n";
	Text += "#   town  The town's name, at most 19 characters: letters and parcels sent\n";
	Text += "#         to the town go into this depot, and the query manager lists the\n";
	Text += "#         town's houses under it.\n";
	Text += "#   size  How many objects the depot holds, twice as many for a premium\n";
	Text += "#         account; below 1 counts as 1 (an error is logged).\n";
	Text += "#\n";
	Text += "# Coordinates: x and y 0 to 65535, z 0 to 15.\n";
	return Text;
}

static std::string Coordinate(int x, int y, int z, int MinX) {
	if (x < MinX || x > MAX_MAP_COORDINATE_XY || y < 0 || y > MAX_MAP_COORDINATE_XY || z < 0
			|| z > MAX_MAP_COORDINATE_Z) {
		throw "a field outside x and y 0 to 65535 and z 0 to 15, or a start position with x 0, which map.toml "
				"cannot hold";
	}
	return "[" + std::to_string(x) + ", " + std::to_string(y) + ", " + std::to_string(z) + "]";
}

std::string MapToToml(const TMapDatExtras &Extras) {
	if (SectorXMin < MIN_SECTOR_XY || SectorXMax > MAX_SECTOR_XY || SectorXMin > SectorXMax
			|| SectorYMin < MIN_SECTOR_XY || SectorYMax > MAX_SECTOR_XY || SectorYMin > SectorYMax || SectorZMin < 0
			|| SectorZMax > MAX_SECTOR_Z || SectorZMin > SectorZMax) {
		throw "a sector range the old reader refuses";
	}
	if (HashTableSize <= 0 || (HashTableSize & (HashTableSize - 1)) != 0 || OBCount <= 0
			|| OBCount > INT32_MAX / OBJECTS_PER_BLOCK) {
		throw "objects or a cache size the old reader refuses";
	}

	std::string Text = Header();
	Text += "\n";
	Text += "refreshed_cylinders = " + std::to_string(RefreshedCylinders) + "\n";
	Text += "newbie_start = "
			+ Coordinate(NewbieStartPositionX, NewbieStartPositionY, NewbieStartPositionZ, 1) + "\n";
	Text += "veteran_start = "
			+ Coordinate(VeteranStartPositionX, VeteranStartPositionY, VeteranStartPositionZ, 1) + "\n";

	Text += "\n[sectors]\n";
	Text += "x_min = " + std::to_string(SectorXMin) + "\n";
	Text += "x_max = " + std::to_string(SectorXMax) + "\n";
	Text += "y_min = " + std::to_string(SectorYMin) + "\n";
	Text += "y_max = " + std::to_string(SectorYMax) + "\n";
	Text += "z_min = " + std::to_string(SectorZMin) + "\n";
	Text += "z_max = " + std::to_string(SectorZMax) + "\n";

	Text += "\n[memory]\n";
	Text += "objects = " + std::to_string(HashTableSize) + "\n";
	Text += "cache_size = " + std::to_string(OBCount * OBJECTS_PER_BLOCK) + "\n";

	for (int i = 0; i < Marks; i++) {
		const TMark &Entry = Mark(i);
		Text += "\n[[mark]]\n";
		Text += "name = " + TomlQuote(Entry.Name) + "\n";
		Text += "position = " + Coordinate(Entry.x, Entry.y, Entry.z, 0) + "\n";
	}

	for (size_t i = 0; i < Extras.Depots.size(); i++) {
		int ID = Extras.Depots[i];
		if (ID < 0 || ID > MAX_DEPOTS - 1 || (i > 0 && ID <= Extras.Depots[i - 1])) {
			throw "depot numbers outside 0 to 8, or not ascending";
		}
		const TDepotInfo &Entry = DepotInfo(ID);
		Text += "\n[[depot]]\n";
		Text += "id = " + std::to_string(ID) + "\n";
		Text += "town = " + TomlQuote(Entry.Town) + "\n";
		Text += "size = " + std::to_string(Entry.Size) + "\n";
	}
	return Text;
}
