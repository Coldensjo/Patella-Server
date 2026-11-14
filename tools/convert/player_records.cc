// player_records.cc: a player file turned into the record of savePlayerData
// (player_records.hh).

#include <cstring>
#include <memory>
#include <stdexcept>
#include <unistd.h>

#include "dbfuncs.hh"
#include "objects.hh"
#include "objects_srv.hh"
#include "player_records.hh"
#include "usr_files.hh"
#include "utils.hh"

void LoadPlayerFileObjectTypes(const std::string &DataPath) {
	std::string Items = DataPath + "/items.toml";
	std::string Objects = DataPath + "/objects.srv";
	// Both loaders refuse a special meaning that a type has already, so the
	// meanings of an earlier load (of another installation) are cleared; the
	// types themselves are overwritten.
	for (int Meaning = 0; Meaning < SPECIALMEANINGS; Meaning++) {
		SpecialObjects[Meaning] = ObjectType(0);
	}
	try {
		if (access(Items.c_str(), R_OK) == 0) {
			LoadItems(Items.c_str());
		} else if (access(Objects.c_str(), R_OK) == 0) {
			TObjectsSrvExtras Extras;
			ReadObjectsSrv(Objects.c_str(), Extras);
		} else {
			throw std::runtime_error("no items.toml or objects.srv in " + DataPath);
		}
	} catch (const char *Text) {
		throw std::runtime_error(std::string("cannot load the object types: ") + Text);
	}
}

// Frees the object streams of a slot.
static void FreeSlot(TPlayerData *Slot) {
	delete[] Slot->Inventory;
	Slot->Inventory = nullptr;
	for (int DepotNr = 0; DepotNr < MAX_DEPOTS; DepotNr++) {
		delete[] Slot->Depot[DepotNr];
		Slot->Depot[DepotNr] = nullptr;
	}
}

// The error texts of the game's code while ReadPlayerFileRecord runs.
static std::vector<std::string> *ErrorLines = nullptr;

static void CollectError(char *Text) {
	std::string Line = Text;
	while (!Line.empty() && Line.back() == '\n') {
		Line.pop_back();
	}
	ErrorLines->push_back(Line);
}

PlayerFileRecord ReadPlayerFileRecord(const std::string &Path, uint32_t CharacterID, std::vector<uint8_t> &Record,
		std::vector<std::string> &Errors) {
	// TPlayerData is large; AssignPlayerPoolSlot zeroes the slot it loads into.
	std::unique_ptr<TPlayerData> Slot(new TPlayerData);
	memset(Slot.get(), 0, sizeof(TPlayerData));
	Slot->CharacterID = CharacterID;
	TErrorFunction *Before = ErrorFunction;
	ErrorLines = &Errors;
	SetErrorFunction(CollectError);
	TPlayerFileResult Result = LoadPlayerFile(Slot.get(), Path.c_str());
	SetErrorFunction(Before);
	ErrorLines = nullptr;

	TDynamicWriteBuffer Buffer(16384);
	try {
		// All murders the file has: they were of the last 30 days when the
		// game wrote the file, and the game reads them all.
		WritePlayerData(Buffer, Slot.get(), 0);
	} catch (const char *Text) {
		FreeSlot(Slot.get());
		throw std::runtime_error(std::string("cannot write the record: ") + Text);
	}
	FreeSlot(Slot.get());

	Record.assign(Buffer.getData(), Buffer.getData() + Buffer.getDataSize());
	switch (Result) {
	case PLAYERFILE_LOADED:
		return PlayerFileRecord::Loaded;
	case PLAYERFILE_MISSING:
		return PlayerFileRecord::Missing;
	default:
		return PlayerFileRecord::Damaged;
	}
}
