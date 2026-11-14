// usr_files.hh: the player files usr/NN/<id>.usr, in which the game kept its
// player data (TPlayerData, src/dbfuncs.hh) until the data moved into the
// query manager's database, and the game's old reader and writer of them
// (usr_files.cc). The objects in them are item text, which LoadPlayerFile
// turns into the slot's object streams with the object types the game has
// loaded (LoadItems or ReadObjectsSrv).

#ifndef PATELLA_CONVERT_USR_FILES_HH
#define PATELLA_CONVERT_USR_FILES_HH

#include <cstddef>
#include <cstdint>

struct TPlayerData;

// The file of a character in the folder UserPath: "UserPath/NN/<id>.usr",
// NN being the ID modulo 100 with two digits; the ID is printed as a signed
// 32-bit number, as the game did.
void PlayerFilePath(const char *UserPath, uint32_t CharacterID, char *Path, size_t Size);

enum TPlayerFileResult {
	// The file was read.
	PLAYERFILE_LOADED,
	// There is no file; the slot has what a new character starts with.
	PLAYERFILE_MISSING,
	// The file could not be read to its end. The slot has what was read
	// before the error and no inventory and no depots: the old
	// LoadPlayerData let the character in like that.
	PLAYERFILE_DAMAGED,
};

// The old LoadPlayerData: fills the slot of Slot->CharacterID from FileName.
// The slot must be zeroed, as AssignPlayerPoolSlot left it.
TPlayerFileResult LoadPlayerFile(TPlayerData *Slot, const char *FileName);

// The old SavePlayerData: writes the slot to FileName, whose folder must
// exist. Like it, it leaves out the murders older than 30 days. False if the
// file could not be written (it is removed then).
bool SavePlayerFile(TPlayerData *Slot, const char *FileName);

#endif
