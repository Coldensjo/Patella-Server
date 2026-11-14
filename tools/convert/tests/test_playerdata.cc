// The player data through the query manager, with made-up player files (one
// of them with the edge cases): what the game's old reader (LoadPlayerFile)
// read from a file must come back the same, field by field, from
// LoadPlayerData after SavePlayerData, and after querymanager-seed's import
// of the file (ReadPlayerFileRecord); the item text in the database must be
// the file's; a character without data starts as a new one; and a save while
// the query manager is gone must keep the data in a file under the save path
// that the next load or start sends, while a load in that time must refuse.
// The query manager runs in this process, on a port the system chooses, with
// a database in a temporary folder.

#include <atomic>
#include <cerrno>
#include <climits>
#include <csignal>
#include <cstring>
#include <ctime>
#include <memory>
#include <optional>
#include <thread>
#include <vector>

#include "config.hh"
#include "dbfuncs.hh"
#include "objects.hh"
#include "player_records.hh"
#include "query.hh"
#include "usr_commands.hh"
#include "usr_files.hh"
#include "utils.hh"

#include "handlers.hh"
#include "log.hh"
#include "server.hh"
#include "sqlite_storage.hh"

#include "test_common.hh"

namespace {

const uint32_t KNIGHT = 1234;
const uint32_t SORCERER = 5678;
const uint32_t NEWCOMER = 999;

// The player files, as the game's SavePlayerData wrote them. The knight has
// everything: nested containers, a text with quotes, a line break and
// ISO-8859-1 letters, negative values, two depots, an outfit of an object;
// his murders are made at run time, one older than 30 days.
std::string KnightFile(const std::string &Murders) {
	return "ID              = 1234\n"
		"Name            = \"Made Up Knight\"\n"
		"Race            = 1\n"
		"Profession      = 1\n"
		"OriginalOutfit  = (131,78-69-58-76)\n"
		"CurrentOutfit   = (0,3058)\n"
		"LastLogin       = 1146000000\n"
		"LastLogout      = 1146003600\n"
		"StartPosition   = [32097,32219,7]\n"
		"CurrentPosition = [32100,32220,6]\n"
		"PlayerkillerEnd = 1146100000\n"
		"\n"
		"Skill = (0,20,20,1,0,0,0,0,0,0,2,98000,100,105000,-3)\n"
		"Skill = (1,0,0,0,0,0,0,0,0,0,0,0,100,0,0)\n"
		"Skill = (7,35,35,10,0,0,0,0,40,200,0,0,110,4000,10)\n"
		"\n"
		"Spells      = {1,22,255}\n"
		"QuestValues = {(17,1),(499,-7)}\n"
		"Murders     = {" + Murders + "}\n"
		"\n"
		"Inventory   = {1 Content={3355},\n"
		"               3 Content={2853 Content={3031 Amount=12, 2820 String=\"Gr\xFC\xDF \\\"dich\\\"\\nzweite Zeile\"}}}\n"
		"\n"
		"Depots      = {1 Content={3502 Content={3031 Amount=100, 2853 Content={3031 Amount=1}}}, \n"
		"               4 Content={3502}}\n";
}

// The sorcerer has nothing: no inventory, no depots, no murders.
const char *const SorcererFile =
	"ID              = 5678\n"
	"Name            = \"Made Up Sorceress\"\n"
	"Race            = 1\n"
	"Profession      = 3\n"
	"OriginalOutfit  = (136,0-0-0-0)\n"
	"CurrentOutfit   = (136,0-0-0-0)\n"
	"LastLogin       = 0\n"
	"LastLogout      = 0\n"
	"StartPosition   = [32097,32219,7]\n"
	"CurrentPosition = [32097,32219,7]\n"
	"PlayerkillerEnd = 0\n"
	"\n"
	"Skill = (0,1,1,1,0,0,0,0,0,0,0,0,100,100,0)\n"
	"\n"
	"Spells      = {}\n"
	"QuestValues = {}\n"
	"Murders     = {}\n"
	"\n"
	"Inventory   = {}\n"
	"\n"
	"Depots      = {}\n";

// The edge cases, all in one made-up character: a name of 29 characters with
// an ISO-8859-1 letter, the largest and the most negative values the game's
// numbers take (INT_MIN itself marks a skill that was never set), an outfit
// of object type 65535, every skill, every spell (0 to 255), every quest
// value, 20 murders (as many as the game keeps, all recent), every body
// position and all nine depots, the last one with item text longer than 65534
// bytes (the long form of a string on the wire) of texts with quotes,
// backslashes, line breaks and ISO-8859-1 letters.
std::string MaximalFile(const std::vector<int> &Murders) {
	const std::string Max = "2147483647";
	const std::string Min = "-2147483647";
	std::string File = "ID              = 4242\n"
		"Name            = \"Made Up Maximal Char\xE4" "cter Xyz\"\n"
		"Race            = " + Max + "\n"
		"Profession      = " + Min + "\n"
		"OriginalOutfit  = (" + Max + ",255-0-255-128)\n"
		"CurrentOutfit   = (0,65535)\n"
		"LastLogin       = " + Max + "\n"
		"LastLogout      = " + Min + "\n"
		"StartPosition   = [65535,65535,15]\n"
		"CurrentPosition = [0,0,0]\n"
		"PlayerkillerEnd = " + Min + "\n"
		"\n";
	for (int SkillNr = 0; SkillNr < MAX_SKILLS; SkillNr++) {
		File += "Skill = (" + std::to_string(SkillNr);
		for (int Value = 0; Value < 14; Value++) {
			File += "," + ((SkillNr + Value) % 2 == 0 ? Max : Min);
		}
		File += ")\n";
	}
	File += "\nSpells      = {";
	for (int SpellNr = 0; SpellNr < MAX_SPELLS; SpellNr++) {
		File += (SpellNr == 0 ? "" : ",") + std::to_string(SpellNr);
	}
	File += "}\nQuestValues = {";
	for (int Number = 0; Number < MAX_QUESTS; Number++) {
		File += (Number == 0 ? "(" : ",(") + std::to_string(Number) + "," + (Number % 2 == 0 ? Max : Min) + ")";
	}
	File += "}\nMurders     = {";
	for (size_t i = 0; i < Murders.size(); i++) {
		File += (i == 0 ? "" : ",") + std::to_string(Murders[i]);
	}
	File += "}\n\nInventory   = {";
	for (int Position = 1; Position <= 10; Position++) {
		if (Position != 1) {
			File += ",\n               ";
		}
		File += std::to_string(Position) + " Content=";
		File += Position == 3 ? "{2853 Content={2820 String=\"\\\\ \\\"\\n\xFF\"}}" : "{3355}";
	}
	File += "}\n\nDepots      = {";
	// A text of 3000 bytes, 3300 in the file: 100 times 27 letters, a quote,
	// a backslash and a line break.
	std::string Text;
	for (int i = 0; i < 100; i++) {
		Text += std::string(27, (char)(0xC0 + i % 32)) + "\\\"\\\\\\n";
	}
	for (int DepotNr = 0; DepotNr < MAX_DEPOTS; DepotNr++) {
		if (DepotNr != 0) {
			File += ", \n               ";
		}
		File += std::to_string(DepotNr) + " Content={3502 Content={3031 Amount=100";
		for (int i = 0; DepotNr == MAX_DEPOTS - 1 && i < 25; i++) {
			File += ", 2820 String=\"" + Text + "\"";
		}
		File += "}}";
	}
	File += "}\n";
	return File;
}

// The query manager of the test, on the port the first start got.
struct TQueryManager {
	std::unique_ptr<qm::SqliteStorage> Storage;
	std::unique_ptr<qm::Dispatcher> Dispatcher;
	std::unique_ptr<qm::Server> Server;
	std::thread Thread;
	uint16_t Port = 0;

	void start(const std::string &Database) {
		Storage.reset(new qm::SqliteStorage(Database));
		Dispatcher.reset(new qm::Dispatcher(*Storage));
		Server.reset(new qm::Server(*Dispatcher, "127.0.0.1", Port));
		Server->start();
		Port = Server->port();
		Thread = std::thread([this]() { Server->run(); });
	}

	void stop() {
		Server->stop();
		Thread.join();
		Server.reset();
		Dispatcher.reset();
		Storage.reset();
	}
};

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

TSlot NewSlot(uint32_t CharacterID) {
	TSlot Slot(new TPlayerData);
	memset(Slot.get(), 0, sizeof(TPlayerData));
	Slot->CharacterID = CharacterID;
	return Slot;
}

TSlot LoadFile(const fs::path &Path, uint32_t CharacterID) {
	TSlot Slot = NewSlot(CharacterID);
	CHECK(LoadPlayerFile(Slot.get(), Path.c_str()) == PLAYERFILE_LOADED);
	return Slot;
}

TSlot Copy(const TPlayerData &Source) {
	TSlot Slot = NewSlot(Source.CharacterID);
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

bool Same(const TPlayerData &A, const TPlayerData &B) {
	std::vector<std::string> Fields = DifferentPlayerDataFields(A, B);
	for (const std::string &Field : Fields) {
		fprintf(stderr, "  differs in %s\n", Field.c_str());
	}
	return Fields.empty();
}

// The file's text after "Key = " up to the end of its value.
std::string FileValue(const std::string &File, const std::string &Key, const std::string &Next) {
	size_t Start = File.find(Key);
	Start = File.find("= ", Start) + 2;
	size_t End = File.rfind("\n\n", File.find(Next));
	return File.substr(Start, End - Start);
}

void Quiet(int, char *) {
}

void QuietError(char *) {
}

void Run(const fs::path &Root) {
	// The game ignores SIGPIPE; a write to a stopped query manager raises it.
	signal(SIGPIPE, SIG_IGN);
	SetPrintFunction(Quiet);
	qm::setLogFile("/dev/null");
	LoadItems(PATELLA_ITEMS_TOML);

	int Now = (int)time(nullptr);
	int RecentMurder = Now - 86400;
	int OldMurder = Now - 40 * 86400;
	fs::path Users = Root / "usr";
	fs::path KnightPath = Users / "34" / "1234.usr";
	fs::path SorcererPath = Users / "78" / "5678.usr";
	std::string KnightText = KnightFile(std::to_string(OldMurder) + "," + std::to_string(RecentMurder));
	WriteFile(KnightPath, KnightText);
	WriteFile(SorcererPath, SorcererFile);
	fs::create_directories(Root / "save");

	// The query manager, and the game's settings for it.
	TQueryManager Manager;
	std::string Database = (Root / "querymanager.sqlite").string();
	Manager.start(Database);
	Manager.Storage->setApplicationPassword(qm::APPLICATION_GAME, "secret");
	qm::WorldConfig World;
	uint32_t WorldID = Manager.Storage->putWorld("Testia", World, false);
	for (uint32_t ID : {KNIGHT, SORCERER, NEWCOMER}) {
		qm::CharacterRecord Character;
		Character.id = ID;
		Character.worldId = WorldID;
		Character.name = "Made Up " + std::to_string(ID);
		Manager.Storage->putCharacter(Character);
	}
	NumberOfQueryManagers = 1;
	strcpy(QUERY_MANAGER[0].Host, "127.0.0.1");
	QUERY_MANAGER[0].Port = Manager.Port;
	strcpy(QUERY_MANAGER[0].Password, "secret");
	SetQueryManagerLoginData(APPLICATION_GAME, "Testia");
	snprintf(SAVEPATH, sizeof(SAVEPATH), "%s", (Root / "save").c_str());
	InitPlayerData();

	TSlot Knight = LoadFile(KnightPath, KNIGHT);
	TSlot Sorcerer = LoadFile(SorcererPath, SORCERER);
	CHECK(Knight->MurderTimestamps[18] == OldMurder && Knight->MurderTimestamps[19] == RecentMurder);
	CHECK(Knight->Depot[1] != nullptr && Knight->Depot[4] != nullptr && Knight->Depot[0] == nullptr);

	// Saved and loaded again: the same, but for the murder older than 30
	// days, which the save drops (as the player file's writer did).
	for (TPlayerData *Original : {Knight.get(), Sorcerer.get()}) {
		TSlot Saved = Copy(*Original);
		CHECK(SavePlayerData(Saved.get()));
		TSlot Loaded = NewSlot(Original->CharacterID);
		CHECK(LoadPlayerData(Loaded.get()));
		TSlot Expected = Copy(*Original);
		KeepRecentMurders(Expected.get(), Now - 2592000);
		CHECK(Same(*Expected, *Loaded));
	}
	CHECK(fs::is_empty(Root / "save"));

	// The database has the file's item text, byte for byte.
	std::optional<qm::PlayerData> Stored = Manager.Storage->loadPlayerData(KNIGHT);
	CHECK(Stored.has_value());
	if (Stored) {
		CHECK(Stored->inventory == FileValue(KnightText, "Inventory", "Depots"));
		CHECK(Stored->depots.size() == 2);
		if (Stored->depots.size() == 2) {
			CHECK(Stored->depots[0].number == 1
				&& Stored->depots[0].content == "{3502 Content={3031 Amount=100, 2853 Content={3031 Amount=1}}}");
			CHECK(Stored->depots[1].number == 4 && Stored->depots[1].content == "{3502}");
		}
		CHECK(Stored->murders == std::vector<int32_t>({RecentMurder}));
		CHECK(Stored->skills.size() == 3 && Stored->skills[0].delta == -3);
		CHECK(Stored->questValues.size() == 2 && Stored->questValues[1].value == -7);
		CHECK(Stored->currentOutfit.id == 0 && Stored->currentOutfit.objectType == 3058);
	}

	// The import of querymanager-seed: the file's record stored as it is,
	// with all its murders; loaded, it is what the old reader read.
	{
		std::vector<uint8_t> Record;
		std::vector<std::string> Errors;
		CHECK(ReadPlayerFileRecord(KnightPath.string(), KNIGHT, Record, Errors) == PlayerFileRecord::Loaded);
		CHECK(Errors.empty());
		qm::PlayerData Data;
		qm::ReadBuffer In(Record);
		qm::readPlayerData(In, Data);
		CHECK(In.eof());
		Data.characterId = KNIGHT;
		Manager.Storage->savePlayerData(Data);
		TSlot Loaded = NewSlot(KNIGHT);
		CHECK(LoadPlayerData(Loaded.get()));
		CHECK(Same(*Knight, *Loaded));

		// The old writer gives the file back byte for byte, but for the
		// murder older than 30 days, which it drops.
		fs::path Written = Root / "written.usr";
		CHECK(SavePlayerFile(Loaded.get(), Written.c_str()));
		CHECK(ReadFile(Written) == KnightFile(std::to_string(RecentMurder)));
	}

	// The edge cases: they come back the same from the query manager, the
	// import stores the record the game's save stores, and the old writer
	// gives the file back byte for byte.
	{
		const uint32_t MAXIMAL = 4242;
		qm::CharacterRecord Character;
		Character.id = MAXIMAL;
		Character.worldId = WorldID;
		Character.name = "Made Up Maximal";
		Manager.Storage->putCharacter(Character);
		std::vector<int> Murders;
		for (int i = 0; i < 20; i++) {
			Murders.push_back(Now - 20 * 86400 + i * 3600);
		}
		std::string MaximalText = MaximalFile(Murders);
		fs::path MaximalPath = Users / "42" / "4242.usr";
		WriteFile(MaximalPath, MaximalText);
		TSlot Maximal = LoadFile(MaximalPath, MAXIMAL);
		CHECK(strcmp(Maximal->Name, "Made Up Maximal Char\xE4" "cter Xyz") == 0);
		CHECK(Maximal->Race == INT_MAX && Maximal->Profession == -INT_MAX
			&& Maximal->CurrentOutfit.ObjectType == 65535);
		CHECK(Maximal->Minimum[MAX_SKILLS - 1] != INT_MIN && Maximal->SpellList[0] != 0
			&& Maximal->SpellList[MAX_SPELLS - 1] != 0);
		CHECK(Maximal->QuestValues[MAX_QUESTS - 1] == -INT_MAX && Maximal->MurderTimestamps[0] == Murders[0]);
		for (int DepotNr = 0; DepotNr < MAX_DEPOTS; DepotNr++) {
			CHECK(Maximal->Depot[DepotNr] != nullptr);
		}

		TSlot Saved = Copy(*Maximal);
		CHECK(SavePlayerData(Saved.get()));
		TSlot Loaded = NewSlot(MAXIMAL);
		CHECK(LoadPlayerData(Loaded.get()));
		CHECK(Same(*Maximal, *Loaded));
		std::optional<qm::PlayerData> Data = Manager.Storage->loadPlayerData(MAXIMAL);
		CHECK(Data.has_value());
		if (Data) {
			CHECK(Data->name == "Made Up Maximal Char\xE4" "cter Xyz");
			CHECK(Data->skills.size() == MAX_SKILLS && Data->spells.size() == MAX_SPELLS
				&& Data->questValues.size() == MAX_QUESTS && Data->murders.size() == 20);
			CHECK(Data->depots.size() == MAX_DEPOTS && Data->depots.back().content.size() > 0xFFFF);

			std::vector<uint8_t> Record;
			std::vector<std::string> Errors;
			CHECK(ReadPlayerFileRecord(MaximalPath.string(), MAXIMAL, Record, Errors) == PlayerFileRecord::Loaded);
			qm::PlayerData Imported;
			qm::ReadBuffer In(Record);
			qm::readPlayerData(In, Imported);
			CHECK(In.eof());
			Imported.characterId = MAXIMAL;
			CHECK(Imported == *Data);
		}

		fs::path Written = Root / "maximal.usr";
		CHECK(SavePlayerFile(Loaded.get(), Written.c_str()));
		CHECK(ReadFile(Written) == MaximalText);
	}

	// No data: a new character, as for a missing player file.
	{
		TSlot Loaded = NewSlot(NEWCOMER);
		CHECK(LoadPlayerData(Loaded.get()));
		TSlot Expected = NewSlot(NEWCOMER);
		SetNewPlayerData(Expected.get());
		CHECK(Same(*Expected, *Loaded));
		CHECK(Loaded->Race == 1 && Loaded->Minimum[0] == INT_MIN && Loaded->Inventory == nullptr);
	}

	// The query manager stops: the save keeps the data in a file and says
	// it is safe; a load must refuse, also the load of another character
	// (the query manager is gone for it too).
	fs::path Kept = Root / "save" / "playerdata-1234.rec";
	Manager.stop();
	SetErrorFunction(QuietError);
	TSlot Moved = Copy(*Knight);
	Moved->posx = 32111;
	CHECK(SavePlayerData(Moved.get()));
	CHECK(fs::is_regular_file(Kept));
	{
		TSlot Loaded = NewSlot(KNIGHT);
		CHECK(!LoadPlayerData(Loaded.get()));
		CHECK(Loaded->Inventory == nullptr);
		TSlot Other = NewSlot(SORCERER);
		CHECK(!LoadPlayerData(Other.get()));
	}
	CHECK(fs::is_regular_file(Kept));

	// Back: the next load sends the kept record first.
	Manager.start(Database);
	{
		TSlot Loaded = NewSlot(KNIGHT);
		CHECK(LoadPlayerData(Loaded.get()));
		CHECK(Loaded->posx == 32111);
		CHECK(!fs::exists(Kept));
	}

	// Gone again while the game shuts down: the next start sends what was kept.
	Manager.stop();
	Moved->posx = 32122;
	CHECK(SavePlayerData(Moved.get()));
	CHECK(fs::is_regular_file(Kept));
	ExitPlayerData();
	Manager.start(Database);
	InitPlayerData();
	CHECK(!fs::exists(Kept));
	CHECK(Manager.Storage->loadPlayerData(KNIGHT)->posX == 32122);

	// A successful save makes a kept record obsolete and removes it.
	Manager.stop();
	CHECK(SavePlayerData(Moved.get()));
	CHECK(fs::is_regular_file(Kept));
	Manager.start(Database);
	Moved->posx = 32133;
	CHECK(SavePlayerData(Moved.get()));
	CHECK(!fs::exists(Kept));
	CHECK(Manager.Storage->loadPlayerData(KNIGHT)->posX == 32133);

	// Neither the query manager nor the save folder: not safe, so the pool
	// keeps the slot dirty.
	Manager.stop();
	snprintf(SAVEPATH, sizeof(SAVEPATH), "%s", (Root / "nowhere").c_str());
	CHECK(!SavePlayerData(Moved.get()));
	snprintf(SAVEPATH, sizeof(SAVEPATH), "%s", (Root / "save").c_str());
	Manager.start(Database);

	// Stored item text the game cannot read (an object type it does not
	// know): the load refuses rather than start the character without it.
	{
		qm::PlayerData Damaged = *Manager.Storage->loadPlayerData(SORCERER);
		Damaged.inventory = "{1 Content={65000}}";
		Manager.Storage->savePlayerData(Damaged);
		TSlot Loaded = NewSlot(SORCERER);
		CHECK(!LoadPlayerData(Loaded.get()));
		CHECK(Loaded->Inventory == nullptr);
	}
	SetErrorFunction(nullptr);

	ExitPlayerData();
	Manager.stop();
}

} // namespace

int main() {
	char Pattern[] = "/tmp/convert-test-playerdata-XXXXXX";
	if (mkdtemp(Pattern) == nullptr) {
		perror("mkdtemp");
		return 1;
	}
	fs::path Root(Pattern);
	try {
		Run(Root);
	} catch (const char *Text) {
		fprintf(stderr, "error: %s\n", Text);
		Failures++;
	} catch (const std::exception &e) {
		fprintf(stderr, "error: %s\n", e.what());
		Failures++;
	}
	fs::remove_all(Root);
	if (Failures != 0) {
		fprintf(stderr, "%d checks failed\n", Failures);
		return 1;
	}
	return 0;
}
