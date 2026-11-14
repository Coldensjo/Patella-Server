// usr_commands.hh: patella-convert's commands for the player files
// (usr_commands.cc): usr-export writes the player data of the query manager's
// database as player files, usr-check proves that the player data survives
// the way through the query manager. The import of player files is
// querymanager-seed's (player_records.hh).

#ifndef PATELLA_CONVERT_USR_COMMANDS_HH
#define PATELLA_CONVERT_USR_COMMANDS_HH

#include <string>
#include <vector>

struct TPlayerData;

// argv: the command's arguments, without "patella-convert usr-...".
int UsrExport(int argc, char **argv);
int UsrCheck(int argc, char **argv);

// The names of the fields of TPlayerData in which A and B differ; the object
// streams are compared byte by byte.
std::vector<std::string> DifferentPlayerDataFields(const TPlayerData &A, const TPlayerData &B);

// What a save that keeps the murders later than Limitation (WritePlayerData)
// and the load after it make of the slot's murders: the kept ones, oldest
// first, at the end of the list.
void KeepRecentMurders(TPlayerData *Slot, int Limitation);

#endif
