#include "seed.hh"

#include "protocol.hh"
#include "script.hh"

#include <algorithm>
#include <cstdio>
#include <filesystem>
#include <stdexcept>

namespace qm {

const char PasswordKey[] = "Pm-,o%yD";

std::string disguisePassword(const std::string &password, const std::string &key) {
	if (key.size() < password.size()) {
		throw std::runtime_error("key for disguising password is too short");
	}
	std::string out = password;
	for (size_t i = 0; i < out.size(); i++) {
		// The game works on signed chars and uses C's truncating division.
		int a = static_cast<signed char>(key[i]) - static_cast<signed char>(out[i]) + 94;
		out[i] = static_cast<char>(a % 94 + 33);
	}
	return out;
}

namespace {

void expectSpecial(ScriptReader &script, char c) {
	if (script.next() != Token::Special || script.special() != c) {
		throw std::runtime_error(script.path() + ":" + std::to_string(script.line())
			+ ": '" + std::string(1, c) + "' expected");
	}
}

uint32_t expectNumber(ScriptReader &script) {
	if (script.next() != Token::Number) {
		throw std::runtime_error(script.path() + ":" + std::to_string(script.line()) + ": number expected");
	}
	return script.number();
}

std::string expectString(ScriptReader &script) {
	if (script.next() != Token::String) {
		throw std::runtime_error(script.path() + ":" + std::to_string(script.line()) + ": string expected");
	}
	return script.text();
}

} // namespace

TibiaConfig readTibiaConfig(const std::string &path) {
	TibiaConfig config;
	ScriptReader script(path);
	while (script.next() != Token::EndOfFile) {
		if (script.token() != Token::Identifier) {
			continue;
		}
		std::string key = script.text();
		if (key == "world") {
			expectSpecial(script, '=');
			config.world = expectString(script);
		} else if (key == "state") {
			expectSpecial(script, '=');
			if (script.next() != Token::Identifier) {
				throw std::runtime_error(path + ": identifier expected after State");
			}
			config.privateWorld = script.text() == "private";
		} else if (key == "querymanager") {
			// QueryManager = {("host",port,"password"),...}
			expectSpecial(script, '=');
			expectSpecial(script, '{');
			while (true) {
				expectSpecial(script, '(');
				QueryManagerEntry entry;
				entry.host = expectString(script);
				expectSpecial(script, ',');
				entry.port = expectNumber(script);
				expectSpecial(script, ',');
				entry.password = expectString(script);
				expectSpecial(script, ')');
				config.queryManagers.push_back(entry);
				if (script.next() != Token::Special) {
					throw std::runtime_error(path + ": ',' or '}' expected in QueryManager");
				}
				if (script.special() == '}') {
					break;
				}
				if (script.special() != ',') {
					throw std::runtime_error(path + ": ',' or '}' expected in QueryManager");
				}
			}
		}
	}
	return config;
}

PlayerFile readPlayerFile(const std::string &path) {
	PlayerFile player;
	ScriptReader script(path);
	while (script.next() != Token::EndOfFile) {
		if (script.token() != Token::Identifier) {
			continue;
		}
		std::string key = script.text();
		if (key == "id") {
			expectSpecial(script, '=');
			player.id = expectNumber(script);
			player.hasId = true;
		} else if (key == "name") {
			expectSpecial(script, '=');
			player.name = expectString(script);
			player.hasName = true;
		} else if (key == "originaloutfit") {
			// OriginalOutfit = (id,colors)
			expectSpecial(script, '=');
			expectSpecial(script, '(');
			player.originalOutfit = expectNumber(script);
		} else if (key == "lastlogin") {
			expectSpecial(script, '=');
			player.lastLogin = expectNumber(script);
		} else if (key == "skill") {
			// The keys of interest all come before the skills.
			break;
		}
	}
	return player;
}

uint8_t sexFromOutfit(uint32_t outfit) {
	if (outfit >= 128 && outfit <= 134) {
		return 1;
	}
	if (outfit >= 136 && outfit <= 142) {
		return 2;
	}
	return 0;
}

std::vector<OwnerEntry> readOwners(const std::string &path) {
	std::vector<OwnerEntry> owners;
	ScriptReader script(path);
	bool open = false;
	OwnerEntry entry;
	while (script.next() != Token::EndOfFile) {
		if (script.token() != Token::Identifier) {
			continue;
		}
		std::string key = script.text();
		if (key == "id") {
			if (open) {
				owners.push_back(entry);
			}
			entry = OwnerEntry();
			expectSpecial(script, '=');
			uint32_t id = expectNumber(script);
			if (id > 0xFFFF) {
				throw std::runtime_error(path + ": house id out of range");
			}
			entry.houseId = static_cast<uint16_t>(id);
			open = true;
		} else if (key == "owner") {
			expectSpecial(script, '=');
			entry.ownerId = expectNumber(script);
		} else if (key == "paiduntil") {
			expectSpecial(script, '=');
			entry.paidUntil = expectNumber(script);
		}
		// "LastTransition", "Guests", "Subowners", "Extension" and the
		// "ClearGuests"/"ClearBeds" switches are the game's own business.
		// Guest names are strings, so they are never taken for a key.
	}
	if (open) {
		owners.push_back(entry);
	}
	return owners;
}

// World configuration that is in no file of the installation.
//   type 0 (NORMAL)       ReadConfig presets WorldType = NORMAL
//   reboot hour 9         ReadConfig presets RebootTime = 540 minutes
//   address 127.0.0.1     the game binds its listen socket to this address;
//                         loopback keeps a test server private
//   port 7171             Tibia's customary game port; the QueryManager
//                         entries in .tibia use 7173..7176 next to it
//   1000 players, 100 of them reserved for premium accounts
//                         below the 1100 connection thread stacks the game
//                         allocates (InitCommunication)
//   200 newbies, 50 of them reserved for premium accounts
// The player numbers are plain choices; nothing in the installation says
// what Zanera ran with.
WorldConfig defaultWorldConfig() {
	WorldConfig config;
	config.type = 0;
	config.rebootHour = 9;
	config.address[0] = 127;
	config.address[1] = 0;
	config.address[2] = 0;
	config.address[3] = 1;
	config.port = 7171;
	config.maxPlayers = 1000;
	config.premiumPlayerBuffer = 100;
	config.maxNewbies = 200;
	config.premiumNewbieBuffer = 50;
	return config;
}

namespace {

void importPlayers(Storage &storage, const std::string &userPath, uint32_t worldId, SeedReport &report) {
	namespace fs = std::filesystem;
	if (!fs::is_directory(userPath)) {
		throw std::runtime_error("no player directory " + userPath);
	}
	std::vector<fs::path> files;
	for (const fs::directory_entry &directory : fs::directory_iterator(userPath)) {
		if (!directory.is_directory()) {
			continue;
		}
		for (const fs::directory_entry &file : fs::directory_iterator(directory.path())) {
			if (file.is_regular_file() && file.path().extension() == ".usr") {
				files.push_back(file.path());
			}
		}
	}
	std::sort(files.begin(), files.end());
	report.playerFiles = files.size();

	for (const fs::path &file : files) {
		// PlayerDataPath (dbfuncs.c): "%s/%02ld/%ld.usr" with
		// CharacterID % 100 as the directory.
		std::string where = file.parent_path().filename().string() + "/" + file.filename().string();
		PlayerFile player;
		try {
			player = readPlayerFile(file.string());
		} catch (const std::exception &e) {
			report.playersSkipped++;
			report.warnings.push_back(where + ": " + e.what());
			continue;
		}
		if (!player.hasId || !player.hasName || player.id == 0 || player.name.empty()) {
			report.playersSkipped++;
			report.warnings.push_back(where + ": no ID or Name");
			continue;
		}
		char expected[32];
		std::snprintf(expected, sizeof(expected), "%02u/%u.usr", player.id % 100, player.id);
		if (where != expected) {
			report.playersSkipped++;
			report.warnings.push_back(where + ": ID in the file belongs to " + expected);
			continue;
		}
		if (player.name.size() > MAX_NAME_LENGTH) {
			report.playersSkipped++;
			report.warnings.push_back(where + ": name is longer than 29 characters");
			continue;
		}

		// Keep what an earlier seeding or the running system stored.
		CharacterRecord character;
		std::optional<CharacterRecord> existing = storage.findCharacter(player.id);
		if (existing) {
			character = *existing;
		}
		character.id = player.id;
		character.worldId = worldId;
		character.name = player.name;
		character.lastLogin = player.lastLogin;
		uint8_t sex = sexFromOutfit(player.originalOutfit);
		if (sex == 1) {
			report.playersMale++;
		} else if (sex == 2) {
			report.playersFemale++;
		} else {
			report.playersOtherOutfit++;
			sex = 1;
		}
		character.sex = sex;
		try {
			storage.putCharacter(character);
		} catch (const StorageError &e) {
			report.playersSkipped++;
			report.warnings.push_back(where + ": " + e.what());
			continue;
		}
		report.playersImported++;
	}
}

void importHouseOwners(Storage &storage, const std::string &path, uint32_t worldId, SeedReport &report) {
	if (!std::filesystem::is_regular_file(path)) {
		report.warnings.push_back("no " + path + ", house owners not imported");
		return;
	}
	for (const OwnerEntry &owner : readOwners(path)) {
		if (owner.ownerId == 0) {
			continue;
		}
		if (!storage.findCharacter(owner.ownerId)) {
			report.houseOwnersWithoutCharacter++;
		}
		if (storage.insertHouseOwner(worldId, owner.houseId, owner.ownerId, owner.paidUntil)) {
			report.houseOwners++;
		}
	}
}

} // namespace

SeedReport seedFromInstallation(Storage &storage, const SeedOptions &options) {
	SeedReport report;
	TibiaConfig config = readTibiaConfig(options.gameRoot + "/.tibia");
	if (config.world.empty()) {
		throw std::runtime_error(".tibia has no World entry");
	}
	if (config.queryManagers.empty()) {
		throw std::runtime_error(".tibia has no QueryManager entry");
	}
	report.worldName = config.world;
	report.privateWorld = config.privateWorld;
	report.queryManagerEntries = config.queryManagers.size();

	// The game tries the entries in order and sends the password of the
	// entry it connected to. One password is stored, so they must agree.
	std::string password = disguisePassword(config.queryManagers[0].password, PasswordKey);
	for (const QueryManagerEntry &entry : config.queryManagers) {
		if (disguisePassword(entry.password, PasswordKey) != password) {
			report.warnings.push_back("QueryManager entries have different passwords; using the first");
			break;
		}
	}

	storage.begin();
	try {
		storage.setApplicationPassword(APPLICATION_GAME, password);
		report.worldId = storage.putWorld(config.world, options.world, config.privateWorld);
		if (options.importPlayers) {
			importPlayers(storage, options.gameRoot + "/usr", report.worldId, report);
		}
		if (options.importHouseOwners) {
			importHouseOwners(storage, options.gameRoot + "/dat/owners.dat", report.worldId, report);
		}
		storage.commit();
	} catch (...) {
		storage.rollback();
		throw;
	}
	return report;
}

} // namespace qm
