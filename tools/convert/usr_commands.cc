// usr_commands.cc: patella-convert usr-export and usr-check (usr_commands.hh).
// Both talk to the query manager as the game does, with the game's own
// requests (loadPlayers, loadPlayerData, savePlayerData of src/query.cc) and
// the game's own record (WritePlayerData and ReadPlayerData of
// src/dbfuncs.cc), on one connection; they log in with the world name and the
// query manager entries of the installation's config.toml.

#include <algorithm>
#include <cerrno>
#include <climits>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <dirent.h>
#include <map>
#include <memory>
#include <stdexcept>
#include <string>
#include <sys/stat.h>
#include <unistd.h>
#include <utility>
#include <vector>

#include "config.hh"
#include "dbfuncs.hh"
#include "player_records.hh"
#include "query.hh"
#include "usr_commands.hh"
#include "usr_files.hh"
#include "utils.hh"

// The connection's buffer: as large as the game's player data connections
// (src/dbfuncs.cc), and enough for loadPlayers' 10000 names.
static const int CONNECTION_BUFFER_SIZE = 4 * 1024 * 1024;

// loadPlayers returns at most this many characters at once (InitPlayerIndex).
static const int PLAYERS_PER_LOAD = 10000;

// What SavePlayerData keeps of the murders: those of the last 30 days.
static const int MURDER_MEMORY = 2592000;

namespace {

struct TSlotDeleter {
	void operator()(TPlayerData *Slot) const {
		delete[] Slot->Inventory;
		for (int DepotNr = 0; DepotNr < MAX_DEPOTS; DepotNr++) {
			delete[] Slot->Depot[DepotNr];
		}
		delete Slot;
	}
};

using TSlot = std::unique_ptr<TPlayerData, TSlotDeleter>;

// A zeroed slot, as AssignPlayerPoolSlot hands it to LoadPlayerData.
TSlot NewSlot(uint32_t CharacterID) {
	TSlot Slot(new TPlayerData);
	memset(Slot.get(), 0, sizeof(TPlayerData));
	Slot->CharacterID = CharacterID;
	return Slot;
}

// Reads the config, the object types and the login data, and connects to the
// query manager.
std::unique_ptr<TQueryManagerConnection> Connect(const char *Config) {
	SetConfigDefaults();
	try {
		LoadConfig(Config);
	} catch (const char *Text) {
		throw std::runtime_error(Text);
	}
	LoadPlayerFileObjectTypes(DATAPATH);
	SetQueryManagerLoginData(APPLICATION_GAME, WorldName);
	std::unique_ptr<TQueryManagerConnection> Connection(new TQueryManagerConnection(CONNECTION_BUFFER_SIZE));
	if (!Connection->isConnected()) {
		throw std::runtime_error("cannot connect to the query manager");
	}
	return Connection;
}

// The player files of a folder (NN/<id>.usr), ordered by path, with their
// character IDs.
std::vector<std::pair<std::string, uint32_t>> PlayerFiles(const std::string &UserPath) {
	std::vector<std::pair<std::string, uint32_t>> Files;
	DIR *Top = opendir(UserPath.c_str());
	if (Top == nullptr) {
		throw std::runtime_error("cannot open the folder " + UserPath);
	}
	while (dirent *Sub = readdir(Top)) {
		if (Sub->d_name[0] == '.') {
			continue;
		}
		std::string Folder = UserPath + "/" + Sub->d_name;
		DIR *Dir = opendir(Folder.c_str());
		if (Dir == nullptr) {
			continue;
		}
		while (dirent *Entry = readdir(Dir)) {
			char *End;
			long ID = strtol(Entry->d_name, &End, 10);
			if (End == Entry->d_name || strcmp(End, ".usr") != 0 || ID == 0) {
				continue;
			}
			Files.emplace_back(Folder + "/" + Entry->d_name, (uint32_t)ID);
		}
		closedir(Dir);
	}
	closedir(Top);
	std::sort(Files.begin(), Files.end());
	return Files;
}

} // namespace

std::vector<std::string> DifferentPlayerDataFields(const TPlayerData &A, const TPlayerData &B) {
	std::vector<std::string> Fields;
#define SAME_VALUE(Field) if (A.Field != B.Field) Fields.push_back(#Field)
#define SAME_BYTES(Field) if (memcmp(&A.Field, &B.Field, sizeof(A.Field)) != 0) Fields.push_back(#Field)
	SAME_VALUE(CharacterID);
	SAME_VALUE(Locked);
	SAME_VALUE(Sticky);
	SAME_VALUE(Dirty);
	SAME_VALUE(Race);
	SAME_BYTES(OriginalOutfit);
	SAME_BYTES(CurrentOutfit);
	SAME_VALUE(LastLoginTime);
	SAME_VALUE(LastLogoutTime);
	SAME_VALUE(startx);
	SAME_VALUE(starty);
	SAME_VALUE(startz);
	SAME_VALUE(posx);
	SAME_VALUE(posy);
	SAME_VALUE(posz);
	SAME_VALUE(Profession);
	SAME_VALUE(PlayerkillerEnd);
	SAME_BYTES(Actual);
	SAME_BYTES(Maximum);
	SAME_BYTES(Minimum);
	SAME_BYTES(DeltaAct);
	SAME_BYTES(MagicDeltaAct);
	SAME_BYTES(Cycle);
	SAME_BYTES(MaxCycle);
	SAME_BYTES(Count);
	SAME_BYTES(MaxCount);
	SAME_BYTES(AddLevel);
	SAME_BYTES(Experience);
	SAME_BYTES(FactorPercent);
	SAME_BYTES(NextLevel);
	SAME_BYTES(Delta);
	SAME_BYTES(SpellList);
	SAME_BYTES(QuestValues);
	SAME_BYTES(MurderTimestamps);
	if (A.InventorySize != B.InventorySize || (A.Inventory == nullptr) != (B.Inventory == nullptr)
			|| (A.Inventory != nullptr && memcmp(A.Inventory, B.Inventory, A.InventorySize) != 0)) {
		Fields.push_back("Inventory");
	}
	for (int DepotNr = 0; DepotNr < MAX_DEPOTS; DepotNr++) {
		if (A.DepotSize[DepotNr] != B.DepotSize[DepotNr] || (A.Depot[DepotNr] == nullptr) != (B.Depot[DepotNr] == nullptr)
				|| (A.Depot[DepotNr] != nullptr && memcmp(A.Depot[DepotNr], B.Depot[DepotNr], A.DepotSize[DepotNr]) != 0)) {
			Fields.push_back("Depot");
			break;
		}
	}
	SAME_VALUE(AccountID);
	SAME_VALUE(Sex);
	if (strcmp(A.Name, B.Name) != 0) {
		Fields.push_back("Name");
	}
	SAME_BYTES(Rights);
	SAME_BYTES(Guild);
	SAME_BYTES(Rank);
	SAME_BYTES(Title);
	SAME_VALUE(Buddies);
	SAME_BYTES(Buddy);
	SAME_BYTES(BuddyName);
	SAME_VALUE(EarliestYellRound);
	SAME_VALUE(EarliestTradeChannelRound);
	SAME_VALUE(EarliestSpellTime);
	SAME_VALUE(EarliestMultiuseTime);
	SAME_VALUE(TalkBufferFullTime);
	SAME_VALUE(MutingEndRound);
	SAME_VALUE(NumberOfMutings);
#undef SAME_VALUE
#undef SAME_BYTES
	return Fields;
}

void KeepRecentMurders(TPlayerData *Slot, int Limitation) {
	int Kept[20];
	int Count = 0;
	for (int i = 0; i < 20; i++) {
		if (Slot->MurderTimestamps[i] > Limitation) {
			Kept[Count++] = Slot->MurderTimestamps[i];
		}
	}
	memset(Slot->MurderTimestamps, 0, sizeof(Slot->MurderTimestamps));
	for (int i = 0; i < Count; i++) {
		Slot->MurderTimestamps[20 - Count + i] = Kept[i];
	}
}

namespace {

TSlot CopySlot(const TPlayerData &Source) {
	TSlot Slot(new TPlayerData);
	memcpy(Slot.get(), &Source, sizeof(TPlayerData));
	if (Source.Inventory != nullptr) {
		Slot->Inventory = new unsigned char[Source.InventorySize];
		memcpy(Slot->Inventory, Source.Inventory, Source.InventorySize);
	}
	for (int DepotNr = 0; DepotNr < MAX_DEPOTS; DepotNr++) {
		if (Source.Depot[DepotNr] != nullptr) {
			Slot->Depot[DepotNr] = new unsigned char[Source.DepotSize[DepotNr]];
			memcpy(Slot->Depot[DepotNr], Source.Depot[DepotNr], Source.DepotSize[DepotNr]);
		}
	}
	return Slot;
}

bool ReadWholeFile(const std::string &Path, std::string &Data) {
	FILE *File = fopen(Path.c_str(), "rb");
	if (File == nullptr) {
		return false;
	}
	Data.clear();
	char Buffer[65536];
	size_t Read;
	while ((Read = fread(Buffer, 1, sizeof(Buffer), File)) > 0) {
		Data.append(Buffer, Read);
	}
	bool Ok = ferror(File) == 0;
	fclose(File);
	return Ok;
}

// usr-check counts the game's messages instead of printing them: they name
// player files.
int GameMessages = 0;

void CountMessage(char *) {
	GameMessages++;
}

void CountPrint(int, char *) {
	GameMessages++;
}

int ArgumentsUsage(const char *Usage) {
	fprintf(stderr, "usage: %s\n", Usage);
	return 2;
}

} // namespace

// Writes the player file of a character into Output; false if that failed.
// Empty is counted if the query manager has no data of it.
static bool ExportCharacter(TQueryManagerConnection *Connection, const std::string &Output, uint32_t CharacterID,
		int &Empty) {
	TSlot Slot = NewSlot(CharacterID);
	int Result = Connection->loadPlayerData(Slot.get());
	if (Result == 1) {
		Empty++;
		return true;
	}
	char Path[4096];
	PlayerFilePath(Output.c_str(), CharacterID, Path, sizeof(Path));
	std::string Folder(Path, strrchr(Path, '/') - Path);
	return Result == 0 && (mkdir(Folder.c_str(), 0700) == 0 || errno == EEXIST) && SavePlayerFile(Slot.get(), Path);
}

// patella-convert usr-export CONFIG OUTPUT_FOLDER [CHARACTER_ID...]
int UsrExport(int argc, char **argv) {
	const char *Usage = "patella-convert usr-export CONFIG_TOML OUTPUT_FOLDER [CHARACTER_ID...]";
	if (argc < 2 || argv[0][0] == '-' || argv[1][0] == '-') {
		return ArgumentsUsage(Usage);
	}
	std::vector<uint32_t> Chosen;
	for (int i = 2; i < argc; i++) {
		char *End;
		unsigned long ID = strtoul(argv[i], &End, 10);
		if (End == argv[i] || *End != 0 || ID == 0 || ID > 0xFFFFFFFFul) {
			return ArgumentsUsage(Usage);
		}
		Chosen.push_back((uint32_t)ID);
	}

	std::unique_ptr<TQueryManagerConnection> Connection = Connect(argv[0]);
	std::string Output = argv[1];
	if (mkdir(Output.c_str(), 0700) != 0 && errno != EEXIST) {
		throw std::runtime_error("cannot create the folder " + Output);
	}

	// The chosen characters, or all of the world (loadPlayers).
	if (Chosen.empty()) {
		static char Names[PLAYERS_PER_LOAD][30];
		static uint32_t CharacterIDs[PLAYERS_PER_LOAD];
		uint32_t MinimumCharacterID = 0;
		while (true) {
			int NumberOfPlayers = 0;
			if (Connection->loadPlayers(MinimumCharacterID, NumberOfPlayers, Names, CharacterIDs) != 0) {
				throw std::runtime_error("loadPlayers failed");
			}
			Chosen.insert(Chosen.end(), CharacterIDs, CharacterIDs + NumberOfPlayers);
			if (NumberOfPlayers < PLAYERS_PER_LOAD) {
				break;
			}
			MinimumCharacterID = CharacterIDs[NumberOfPlayers - 1] + 1;
		}
	}

	int Characters = 0, Written = 0, Empty = 0, Failed = 0;
	for (uint32_t CharacterID : Chosen) {
		Characters++;
		int EmptyBefore = Empty;
		if (!ExportCharacter(Connection.get(), Output, CharacterID, Empty)) {
			Failed++;
		} else if (Empty == EmptyBefore) {
			Written++;
		}
	}

	printf("characters: %d, player files written: %d, without player data: %d, failed: %d\n", Characters, Written,
			Empty, Failed);
	return Failed == 0 ? 0 : 1;
}

// patella-convert usr-check [--read-only] CONFIG [USER_FOLDER]
int UsrCheck(int argc, char **argv) {
	const char *Usage = "patella-convert usr-check [--read-only] CONFIG_TOML [USER_FOLDER]";
	bool ReadOnly = false;
	std::vector<const char *> Arguments;
	for (int i = 0; i < argc; i++) {
		if (strcmp(argv[i], "--read-only") == 0) {
			ReadOnly = true;
		} else if (argv[i][0] == '-') {
			return ArgumentsUsage(Usage);
		} else {
			Arguments.push_back(argv[i]);
		}
	}
	if (Arguments.empty() || Arguments.size() > 2) {
		return ArgumentsUsage(Usage);
	}

	std::unique_ptr<TQueryManagerConnection> Connection = Connect(Arguments[0]);
	std::string UserPath = Arguments.size() > 1 ? Arguments[1] : USERPATH;
	SetErrorFunction(CountMessage);
	SetPrintFunction(CountPrint);
	std::vector<std::pair<std::string, uint32_t>> Files = PlayerFiles(UserPath);

	// The old writer's file of each character holds a player's data: it goes
	// into TMPDIR (tools/playerdata_check.py sets its run folder, which it
	// removes on every exit path), else /tmp.
	const char *TempFolder = getenv("TMPDIR");
	std::string TempTemplate = std::string(TempFolder != nullptr && TempFolder[0] != 0 ? TempFolder : "/tmp")
			+ "/patella-usr-check-XXXXXX";
	std::vector<char> TempName(TempTemplate.begin(), TempTemplate.end());
	TempName.push_back(0);
	char *TempPath = TempName.data();
	int TempFile = mkstemp(TempPath);
	if (TempFile < 0) {
		throw std::runtime_error("cannot create a temporary file");
	}
	close(TempFile);

	int Damaged = 0;
	int ImportSame = 0, ImportDifferent = 0, NotStored = 0, LoadFailed = 0;
	int SaveSame = 0, SaveDifferent = 0, SaveFailed = 0, MurdersDropped = 0;
	int ExportSame = 0, ExportDifferent = 0;
	std::map<std::string, int> ImportFields, SaveFields;
	for (const auto &File : Files) {
		uint32_t CharacterID = File.second;
		TSlot Original = NewSlot(CharacterID);
		if (LoadPlayerFile(Original.get(), File.first.c_str()) == PLAYERFILE_DAMAGED) {
			Damaged++;
		}

		// The import: what querymanager-seed stored for the file.
		TSlot Imported = NewSlot(CharacterID);
		int Result = Connection->loadPlayerData(Imported.get());
		if (Result == 1) {
			NotStored++;
		} else if (Result != 0) {
			LoadFailed++;
		} else {
			std::vector<std::string> Fields = DifferentPlayerDataFields(*Original, *Imported);
			if (Fields.empty()) {
				ImportSame++;
			} else {
				ImportDifferent++;
				for (const std::string &Field : Fields) {
					ImportFields[Field]++;
				}
			}

			// What the old writer makes of it, against the file.
			std::string Before, After;
			if (SavePlayerFile(Imported.get(), TempPath) && ReadWholeFile(File.first, Before)
					&& ReadWholeFile(TempPath, After) && Before == After) {
				ExportSame++;
			} else {
				ExportDifferent++;
			}
		}

		if (ReadOnly) {
			continue;
		}

		// The save: the file's data saved and loaded again as the game does.
		TDynamicWriteBuffer Record(16384);
		int Limitation = (int)time(nullptr) - MURDER_MEMORY;
		try {
			WritePlayerData(Record, Original.get(), Limitation);
		} catch (const char *) {
			SaveFailed++;
			continue;
		}
		TSlot Saved = NewSlot(CharacterID);
		if (Connection->savePlayerData(CharacterID, Record.getData(), Record.getDataSize()) != 0
				|| Connection->loadPlayerData(Saved.get()) != 0) {
			SaveFailed++;
			continue;
		}
		TSlot Expected = CopySlot(*Original);
		KeepRecentMurders(Expected.get(), Limitation);
		if (memcmp(Expected->MurderTimestamps, Original->MurderTimestamps, sizeof(Original->MurderTimestamps)) != 0) {
			MurdersDropped++;
		}
		std::vector<std::string> Fields = DifferentPlayerDataFields(*Expected, *Saved);
		if (Fields.empty()) {
			SaveSame++;
		} else {
			SaveDifferent++;
			for (const std::string &Field : Fields) {
				SaveFields[Field]++;
			}
		}
	}
	unlink(TempPath);

	SetErrorFunction(nullptr);
	SetPrintFunction(nullptr);
	printf("player files: %zu, damaged (read like the old game did): %d; game messages (not shown): %d\n",
			Files.size(), Damaged, GameMessages);
	printf("import: %d the same as the old reader's, %d different, %d not in the database, %d failed to load\n",
			ImportSame, ImportDifferent, NotStored, LoadFailed);
	for (const auto &Field : ImportFields) {
		printf("  import differs in %s: %d\n", Field.first.c_str(), Field.second);
	}
	printf("old writer: %d imported files written back byte for byte, %d not\n", ExportSame, ExportDifferent);
	if (!ReadOnly) {
		printf("save: %d the same after savePlayerData and loadPlayerData, %d different, %d failed"
				" (%d with murders older than 30 days, which a save drops)\n", SaveSame, SaveDifferent, SaveFailed,
				MurdersDropped);
		for (const auto &Field : SaveFields) {
			printf("  save differs in %s: %d\n", Field.first.c_str(), Field.second);
		}
	}

	// A player file the import left out did not come through either.
	bool Ok = ImportDifferent == 0 && NotStored == 0 && LoadFailed == 0 && SaveDifferent == 0 && SaveFailed == 0;
	return Ok ? 0 : 1;
}
