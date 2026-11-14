// game_stubs.cc: two functions of game units that the player data units the
// converter compiles (map.cc, dbfuncs.cc) refer to, and that it leaves out:
// Log (writer.cc), which writes the game's log files, and CleanHouseField
// (houses.cc). The converter writes no game log, and nothing it does cleans
// a house.

#include "houses.hh"
#include "utils.hh"
#include "writer.hh"

void Log(char *, char *, ...) {
}

void CleanHouseField(int, int, int) {
	error("CleanHouseField: Not available in the converter.\n");
}
