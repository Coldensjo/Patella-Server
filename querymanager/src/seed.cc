// seed.cc: seeding a database from a game server installation (seed.hh).

#include "seed.hh"

#include "protocol.hh"
#include "script.hh"

#include <algorithm>
#include <cstdio>
#include <filesystem>
#include <stdexcept>

#include <toml++/toml.hpp>

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

std::string where(const toml::source_region &region) {
	return (region.path ? *region.path : std::string("?")) + ":" + std::to_string(region.begin.line) + ":"
		+ std::to_string(region.begin.column);
}

// The Latin-1 bytes of a TOML string (UTF-8).
std::string latin1(const toml::node &node, const char *key) {
	if (!node.is_string()) {
		throw std::runtime_error(where(node.source()) + ": " + key + " must be a string");
	}
	const std::string &text = node.as_string()->get();
	std::string out;
	for (size_t i = 0; i < text.size(); i++) {
		unsigned char c = static_cast<unsigned char>(text[i]);
		if (c < 0x80) {
			out += static_cast<char>(c);
		} else if ((c == 0xC2 || c == 0xC3) && i + 1 < text.size()) {
			out += static_cast<char>(((c & 0x1F) << 6) | (text[i + 1] & 0x3F));
			i++;
		} else {
			throw std::runtime_error(where(node.source()) + ": " + key + " has a character outside Latin-1");
		}
	}
	return out;
}

// A folder of [paths]: a relative one joined to the config's folder.
std::string configPath(const toml::table &paths, const char *key, const std::filesystem::path &folder) {
	const toml::node *node = paths.get(key);
	if (node == nullptr) {
		return "";
	}
	std::string value = latin1(*node, key);
	if (value.empty() || value[0] == '/') {
		return value;
	}
	return (folder / value).string();
}

} // namespace

TibiaConfig readConfigToml(const std::string &path) {
	toml::table file;
	try {
		file = toml::parse_file(path);
	} catch (const toml::parse_error &e) {
		throw std::runtime_error(where(e.source()) + ": " + std::string(e.description()));
	}
	TibiaConfig config;
	if (const toml::node *world = file.get("world")) {
		if (!world->is_table()) {
			throw std::runtime_error(where(world->source()) + ": world must be a table");
		}
		if (const toml::node *name = world->as_table()->get("name")) {
			config.world = latin1(*name, "name");
		}
		if (const toml::node *state = world->as_table()->get("state")) {
			config.privateWorld = latin1(*state, "state") == "private";
		}
	}
	if (const toml::node *paths = file.get("paths")) {
		if (!paths->is_table()) {
			throw std::runtime_error(where(paths->source()) + ": paths must be a table");
		}
		std::filesystem::path folder = std::filesystem::absolute(path).parent_path();
		config.userPath = configPath(*paths->as_table(), "user", folder);
		config.dataPath = configPath(*paths->as_table(), "data", folder);
	}
	if (const toml::node *entries = file.get("query_manager")) {
		if (!entries->is_array_of_tables()) {
			throw std::runtime_error(where(entries->source()) + ": query_manager must be an array of tables");
		}
		for (const toml::node &node : *entries->as_array()) {
			const toml::table &table = *node.as_table();
			QueryManagerEntry entry;
			for (const char *key : {"host", "port", "password"}) {
				if (!table.contains(key)) {
					throw std::runtime_error(where(node.source()) + ": an entry of query_manager needs " + key);
				}
			}
			entry.host = latin1(*table.get("host"), "host");
			const toml::node *port = table.get("port");
			if (!port->is_integer() || port->as_integer()->get() < 0 || port->as_integer()->get() > 65535) {
				throw std::runtime_error(where(port->source()) + ": port must be an integer from 0 to 65535");
			}
			entry.port = static_cast<uint32_t>(port->as_integer()->get());
			entry.password = latin1(*table.get("password"), "password");
			config.queryManagers.push_back(entry);
		}
	}
	return config;
}

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
//   type 0 (NORMAL)       SetConfigDefaults presets WorldType = NORMAL
//   reboot hour 9         SetConfigDefaults presets RebootTime = 540 minutes
//   address 127.0.0.1     the game binds its listen socket to this address;
//                         loopback keeps a test server private
//   port 7171             Tibia's customary game port; the query manager
//                         entries of the config use 7173..7176 next to it
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
		// PlayerDataPath (src/dbfuncs.cc): "%s/%02ld/%ld.usr" with
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
	// config.toml, or the original .tibia of an installation not yet
	// converted (tools/convert).
	std::string configFile = options.gameRoot + "/config.toml";
	TibiaConfig config;
	if (std::filesystem::is_regular_file(configFile)) {
		config = readConfigToml(configFile);
	} else {
		configFile = options.gameRoot + "/.tibia";
		config = readTibiaConfig(configFile);
	}
	if (config.world.empty()) {
		throw std::runtime_error(configFile + " names no world");
	}
	if (config.queryManagers.empty()) {
		throw std::runtime_error(configFile + " names no query manager");
	}
	std::string userPath = config.userPath.empty() ? options.gameRoot + "/usr" : config.userPath;
	std::string dataPath = config.dataPath.empty() ? options.gameRoot + "/dat" : config.dataPath;
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
			importPlayers(storage, userPath, report.worldId, report);
		}
		if (options.importHouseOwners) {
			importHouseOwners(storage, dataPath + "/owners.dat", report.worldId, report);
		}
		storage.commit();
	} catch (...) {
		storage.rollback();
		throw;
	}
	return report;
}

} // namespace qm
