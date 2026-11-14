// player_records.hh: the import of the player files into the query manager's
// database, as querymanager-seed does it (player_records.cc): a player file
// read with the game's old reader (usr_files.hh) and turned into the record
// that the game's savePlayerData would send for it (WritePlayerData,
// src/dbfuncs.cc). So the database gets what the game itself would have saved
// after loading the file. Only standard types here, so that the seed need not
// see the game's headers.

#ifndef PATELLA_CONVERT_PLAYER_RECORDS_HH
#define PATELLA_CONVERT_PLAYER_RECORDS_HH

#include <cstdint>
#include <string>
#include <vector>

// Loads the object types the item texts name, from the data folder: its
// items.toml, or the original objects.srv when there is no items.toml. Throws
// std::runtime_error if neither is there or the file cannot be read.
void LoadPlayerFileObjectTypes(const std::string &DataPath);

enum class PlayerFileRecord {
	Loaded,     // the file was read
	Damaged,    // the file could not be read to its end (usr_files.hh)
	Missing,    // there is no such file
};

// Reads the player file Path of the character CharacterID and gives the
// record of savePlayerData, without the character ID, in Record, and the
// error texts of the game's reader, one per line, in Errors. Throws
// std::runtime_error if the record cannot be written.
PlayerFileRecord ReadPlayerFileRecord(const std::string &Path, uint32_t CharacterID, std::vector<uint8_t> &Record,
		std::vector<std::string> &Errors);

#endif
