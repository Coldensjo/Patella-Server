// Seeding from a made-up installation in a temporary directory.

#include "handlers.hh"
#include "log.hh"
#include "script.hh"
#include "seed.hh"
#include "sqlite_storage.hh"
#include "test.hh"

#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <stdexcept>
#include <string>

#include <unistd.h>

using namespace qm;

namespace {

namespace fs = std::filesystem;

void writeFile(const fs::path &path, const std::string &content) {
	fs::create_directories(path.parent_path());
	std::ofstream out(path, std::ios::binary);
	out << content;
}

std::string playerFile(uint32_t id, const std::string &name, uint32_t outfit, uint32_t lastLogin) {
	return "ID              = " + std::to_string(id) + "\n"
		"Name            = \"" + name + "\"\n"
		"Race            = 1\n"
		"Profession      = 2\n"
		"OriginalOutfit  = (" + std::to_string(outfit) + ",3-86-3-86)\n"
		"CurrentOutfit   = (" + std::to_string(outfit) + ",3-86-3-86)\n"
		"LastLogin       = " + std::to_string(lastLogin) + "\n"
		"LastLogout      = " + std::to_string(lastLogin + 60) + "\n"
		"StartPosition   = [32097,32219,7]\n"
		"CurrentPosition = [32097,32219,7]\n"
		"PlayerkillerEnd = 0\n"
		"\n"
		"Skill = (0,9,0,1,0,0,0,0,0,0,1,100,0,100,100)\n"
		"Skill = (1,3,0,0,0,0,0,0,0,0,0,100,100,100,100)\n"
		"\n"
		"Spells      = {8,42}\n"
		"QuestValues = {(17,1)}\n"
		"Murders     = {}\n"
		"\n"
		"Inventory   = {8 Content={2120}}\n"
		"\n"
		"Depots      = {1 Content={2594 Content={3031 Amount=27}}}\n";
}

// The .tibia of the made-up installation. The password "abcx/" is sent by
// the game as "n,I3a" (worked out by hand from DisguisePassword).
const char *const TibiaFile =
	"# Tibia - Graphical Multi-User-Dungeon\n"
	"# .tibia: made-up test configuration\n"
	"\n"
	"USERPATH    = \"/nowhere/usr\"\n"
	"SHM = 12345\n"
	"Beat = 50\n"
	"\n"
	"# QueryManager\n"
	"QueryManager = {(\"127.0.0.1\",7173,\"abcx/\"),(\"127.0.0.1\",7174,\"abcx/\")}\n"
	"\n"
	"World = \"Testia\"\n"
	"State = private\n";

const char *const OwnersFile =
	"ID = 101\n"
	"Owner = 1234\n"
	"LastTransition = 1100000000\n"
	"PaidUntil = 1100500000\n"
	"Guests = {\"id\",\"Owner\",\"some \\\"quoted\\\" guest\"}\n"
	"Subowners = {}\n"
	"\n"
	"ID = 102\n"
	"Owner = 99999\n"
	"LastTransition = 1100000001\n"
	"PaidUntil = 1100500001\n"
	"Guests = {}\n"
	"Subowners = {\"PaidUntil\"}\n"
	"\n"
	"ID = 103\n"
	"Owner = 0\n"
	"LastTransition = 0\n"
	"PaidUntil = 0\n"
	"Guests = {}\n"
	"Subowners = {}\n";

void testDisguise() {
	CHECK(disguisePassword("", PasswordKey) == "");
	CHECK(disguisePassword("abc", PasswordKey) == "n,I");
	CHECK(disguisePassword("abcx/", PasswordKey) == "n,I3a");
	// Every result character is printable ASCII above the space.
	std::string all = disguisePassword("!~!~!~!~", PasswordKey);
	for (char c : all) {
		CHECK(c >= 33 && c <= 126);
	}
	CHECK_THROWS(disguisePassword("123456789", PasswordKey), std::runtime_error);
}

void testScriptReader(const fs::path &root) {
	writeFile(root / "script.txt",
		"# comment = \"not a string\"\n"
		"Key_1 = \"a \\\"b\\\"\\nc\\\\\" # trailing comment\n"
		"Number = 4294967295\n"
		"Outfit = (130,3-86)\n"
		"Position = [1,2,3]\n");
	ScriptReader script((root / "script.txt").string());
	CHECK(script.next() == Token::Identifier && script.text() == "key_1");
	CHECK(script.line() == 2);
	CHECK(script.next() == Token::Special && script.special() == '=');
	CHECK(script.next() == Token::String && script.text() == "a \"b\"\nc\\");
	CHECK(script.next() == Token::Identifier && script.text() == "number");
	CHECK(script.next() == Token::Special);
	CHECK(script.next() == Token::Number && script.number() == 4294967295u);
	CHECK(script.next() == Token::Identifier && script.text() == "outfit");
	CHECK(script.next() == Token::Special);
	CHECK(script.next() == Token::Special && script.isSpecial('('));
	CHECK(script.next() == Token::Number && script.number() == 130);
	CHECK(script.next() == Token::Special && script.isSpecial(','));
	CHECK(script.next() == Token::Number && script.number() == 3);
	CHECK(script.next() == Token::Special && script.isSpecial('-'));
	CHECK(script.next() == Token::Number && script.number() == 86);
	CHECK(script.next() == Token::Special && script.isSpecial(')'));
	CHECK(script.next() == Token::Identifier && script.text() == "position");
	for (int i = 0; i < 8; i++) {
		script.next();
	}
	CHECK(script.isSpecial(']'));
	CHECK(script.next() == Token::EndOfFile);
	CHECK(script.next() == Token::EndOfFile);

	CHECK_THROWS(ScriptReader((root / "missing.txt").string()), std::runtime_error);
	writeFile(root / "open.txt", "Name = \"never closed\n");
	ScriptReader open((root / "open.txt").string());
	open.next();
	open.next();
	CHECK_THROWS(open.next(), std::runtime_error);
}

void testParsers(const fs::path &root) {
	TibiaConfig config = readTibiaConfig((root / ".tibia").string());
	CHECK(config.world == "Testia");
	CHECK(config.privateWorld);
	CHECK(config.queryManagers.size() == 2);
	CHECK(config.queryManagers[0].host == "127.0.0.1");
	CHECK(config.queryManagers[0].port == 7173);
	CHECK(config.queryManagers[1].port == 7174);
	CHECK(config.queryManagers[1].password == "abcx/");

	PlayerFile player = readPlayerFile((root / "usr/34/1234.usr").string());
	CHECK(player.hasId && player.id == 1234);
	CHECK(player.hasName && player.name == "Made Up Knight");
	CHECK(player.originalOutfit == 131);
	CHECK(player.lastLogin == 1146000000);

	CHECK(sexFromOutfit(127) == 0);
	CHECK(sexFromOutfit(128) == 1);
	CHECK(sexFromOutfit(134) == 1);
	CHECK(sexFromOutfit(135) == 0);
	CHECK(sexFromOutfit(136) == 2);
	CHECK(sexFromOutfit(142) == 2);
	CHECK(sexFromOutfit(143) == 0);
	CHECK(sexFromOutfit(75) == 0);

	// Guest names that look like keys are strings, not keys.
	std::vector<OwnerEntry> owners = readOwners((root / "dat/owners.dat").string());
	CHECK(owners.size() == 3);
	CHECK(owners[0].houseId == 101 && owners[0].ownerId == 1234 && owners[0].paidUntil == 1100500000);
	CHECK(owners[1].houseId == 102 && owners[1].ownerId == 99999 && owners[1].paidUntil == 1100500001);
	CHECK(owners[2].houseId == 103 && owners[2].ownerId == 0);
}

void testSeed(const fs::path &root) {
	SqliteStorage storage(":memory:");
	SeedOptions options;
	options.gameRoot = root.string();
	options.world = defaultWorldConfig();
	options.world.port = 7999;
	SeedReport report = seedFromInstallation(storage, options);

	CHECK(report.worldName == "Testia");
	CHECK(report.privateWorld);
	CHECK(report.queryManagerEntries == 2);
	CHECK(report.playerFiles == 6);
	CHECK(report.playersImported == 4);
	CHECK(report.playersSkipped == 2);        // wrong directory, no name
	CHECK(report.playersMale == 2);
	CHECK(report.playersFemale == 1);
	CHECK(report.playersOtherOutfit == 1);
	CHECK(report.houseOwners == 2);
	CHECK(report.houseOwnersWithoutCharacter == 1);
	CHECK(report.warnings.size() == 2);
	CHECK(storage.countRows("characters") == 4);
	CHECK(storage.countRows("accounts") == 0);
	CHECK(storage.countRows("house_owners") == 2);

	WorldRecord world = *storage.findWorld("Testia");
	CHECK(world.privateWorld);
	CHECK(world.config.port == 7999);
	CHECK(world.config.rebootHour == 9);
	CHECK(world.config.address[0] == 127 && world.config.address[3] == 1);

	CharacterRecord knight = *storage.findCharacterByName("made up knight");
	CHECK(knight.id == 1234 && knight.sex == 1 && knight.accountId == 0 && knight.worldId == world.id);
	CHECK(knight.lastLogin == 1146000000);
	CHECK(storage.findCharacter(5678)->sex == 2);
	CHECK(storage.findCharacter(77)->sex == 1);                 // gamemaster outfit: stored as male
	CHECK(storage.findCharacter(900)->name == "M\xE4" "de \xDCp Dru\xEF" "d");
	CHECK(!storage.findCharacter(4321));                        // was in the wrong directory
	CHECK(storage.nextCharacterId() == 5679);

	// A new character, as querymanager-seed --new-character creates it: the
	// next free id, no player file, and it logs in like any other.
	AccountRecord newAccount;
	newAccount.id = 222222;
	newAccount.password = "pw2";
	storage.putAccount(newAccount);
	storage.addAccountRight(222222, "INVITED");                 // Testia is private
	CharacterRecord fresh;
	fresh.id = storage.nextCharacterId();
	fresh.worldId = world.id;
	fresh.accountId = 222222;
	fresh.name = "Made Up Newcomer";
	fresh.sex = 2;
	storage.putCharacter(fresh);
	CHECK(storage.nextCharacterId() == 5680);
	CHECK(storage.findCharacterByName("made up newcomer")->id == 5679);

	// The game logs in with the disguised password and its world name.
	CHECK(storage.checkApplicationPassword(APPLICATION_GAME, "n,I3a"));
	CHECK(!storage.checkApplicationPassword(APPLICATION_GAME, "abcx/"));
	Dispatcher dispatcher(storage);
	Session session;
	WriteBuffer login;
	login.writeByte(QUERY_LOGIN);
	login.writeByte(APPLICATION_GAME);
	login.writeString("n,I3a");
	login.writeString("Testia");
	std::vector<uint8_t> response = dispatcher.handle(session, login.data());
	CHECK(response.size() == 1 && response[0] == STATUS_OK);
	WriteBuffer loginGame;
	loginGame.writeByte(QUERY_LOGIN_GAME);
	loginGame.writeQuad(222222);
	loginGame.writeString("Made Up Newcomer");
	loginGame.writeString("pw2");
	loginGame.writeString("127.0.0.1");
	loginGame.writeFlag(true);
	loginGame.writeFlag(false);
	loginGame.writeFlag(false);
	response = dispatcher.handle(session, loginGame.data());
	CHECK(!response.empty() && response[0] == STATUS_OK);

	// The owner entries show up once the game has sent its house list.
	CHECK(storage.listHouseOwners(world.id).empty());
	HouseRecord house;
	house.id = 101;
	house.name = "Test Lane 1";
	storage.replaceHouses(world.id, {house});
	std::vector<HouseOwner> owners = storage.listHouseOwners(world.id);
	CHECK(owners.size() == 1 && owners[0].ownerId == 1234 && owners[0].ownerName == "Made Up Knight"
		&& owners[0].paidUntil == 1100500000);

	// Seeding again keeps what the database learned in between.
	AccountRecord account;
	account.id = 111111;
	account.password = "pw";
	storage.putAccount(account);
	knight.accountId = 111111;
	knight.guild = "Test Guild";
	storage.putCharacter(knight);
	SeedReport again = seedFromInstallation(storage, options);
	CHECK(again.playersImported == 4);
	CHECK(again.houseOwners == 0);
	CHECK(storage.countRows("characters") == 5);
	CHECK(storage.findCharacter(1234)->accountId == 111111);
	CHECK(storage.findCharacter(1234)->guild == "Test Guild");
	CHECK(storage.countRows("worlds") == 1);

	// Options: nothing but the world.
	SqliteStorage bare(":memory:");
	options.importPlayers = false;
	options.importHouseOwners = false;
	SeedReport bareReport = seedFromInstallation(bare, options);
	CHECK(bareReport.playerFiles == 0);
	CHECK(bare.countRows("characters") == 0);
	CHECK(bare.nextCharacterId() == 1);
	CHECK(bare.countRows("house_owners") == 0);
	CHECK(bare.countRows("worlds") == 1);

	// An installation without .tibia is refused and leaves no trace.
	SqliteStorage untouched(":memory:");
	options.gameRoot = (root / "nothing").string();
	CHECK_THROWS(seedFromInstallation(untouched, options), std::runtime_error);
	CHECK(untouched.countRows("worlds") == 0);
}

void all() {
	setLogFile("/dev/null");

	char pattern[] = "/tmp/qm-test-seed-XXXXXX";
	const char *directory = mkdtemp(pattern);
	if (directory == nullptr) {
		throw std::runtime_error("mkdtemp failed");
	}
	fs::path root(directory);

	writeFile(root / ".tibia", TibiaFile);
	writeFile(root / "dat/owners.dat", OwnersFile);
	writeFile(root / "usr/34/1234.usr", playerFile(1234, "Made Up Knight", 131, 1146000000));
	writeFile(root / "usr/78/5678.usr", playerFile(5678, "Made Up Sorceress", 138, 1146000001));
	writeFile(root / "usr/77/77.usr", playerFile(77, "Made Up Gamemaster", 75, 1146000002));
	writeFile(root / "usr/00/900.usr", playerFile(900, "M\xE4" "de \xDCp Dru\xEF" "d", 129, 1146000003));
	// In the wrong directory for its id, and one without a name.
	writeFile(root / "usr/12/4321.usr", playerFile(4321, "Made Up Stray", 128, 1));
	writeFile(root / "usr/55/555.usr", "ID = 555\nRace = 1\n");
	// Not player files.
	writeFile(root / "usr/34/1234.bak", "ID = 1\n");
	writeFile(root / "usr/readme", "nothing\n");

	try {
		testDisguise();
		testScriptReader(root);
		testParsers(root);
		testSeed(root);
	} catch (...) {
		fs::remove_all(root);
		throw;
	}
	fs::remove_all(root);
}

} // namespace

int main() {
	return test::run(all);
}
