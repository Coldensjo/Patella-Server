// querymanager-seed: creates or extends a query manager database.
//
// Mode 1 (--game-root): import a game server installation, see seed.hh: the
// world, the characters and their player data, the house owners.
// Mode 2 (--add-account): create an account and assign characters to it.
// The installation has no accounts, so a client login needs this step; the
// account number and password are the operator's choice. It can also create
// new characters: they get the next free character id and have no player
// data, so the game starts them as new characters (LoadPlayerData,
// src/dbfuncs.cc).
// --login-password, alone or with either mode, stores the password the
// login server (loginserver/) presents. It sends it as it is, without the
// game's DisguisePassword.

#include "protocol.hh"
#include "seed.hh"
#include "sqlite_storage.hh"
#include "text.hh"

#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <string>
#include <vector>

namespace {

void usage(const char *program) {
	std::fprintf(stderr,
		"usage: %s --db PATH --game-root DIR [world options] [--no-players] [--no-player-data]\n"
		"          [--no-house-owners]\n"
		"       %s --db PATH --add-account ID --password PASSWORD [account options]\n"
		"       %s --db PATH --login-password PASSWORD [with either form above]\n"
		"world options (defaults in parentheses):\n"
		"  --world-type N             0 normal, 1 non-PvP, 2 PvP-enforced (0)\n"
		"  --reboot-hour N            hour of the daily reboot (9)\n"
		"  --address A.B.C.D          address the game server listens on (127.0.0.1)\n"
		"  --port N                   port the game server listens on (7171)\n"
		"  --max-players N            (1000)\n"
		"  --premium-player-buffer N  (100)\n"
		"  --max-newbies N            (200)\n"
		"  --premium-newbie-buffer N  (50)\n"
		"account options:\n"
		"  --premium-days N           premium time from now (0: free account)\n"
		"  --gamemaster               the account may use a gamemaster client\n"
		"  --right NAME               give the account a right; may be repeated\n"
		"  --character NAME           assign this character to the account; may be repeated\n"
		"  --new-character NAME       create a character of this account; may be repeated;\n"
		"                             needs --world\n"
		"  --world NAME               world of the new characters\n"
		"  --sex N                    sex of the new characters: 1 male, 2 female (1)\n",
		program, program, program);
}

bool parseNumber(const char *text, long minimum, long maximum, long &value) {
	char *end = nullptr;
	value = std::strtol(text, &end, 10);
	return end != text && *end == '\0' && value >= minimum && value <= maximum;
}

bool parseAddress(const char *text, uint8_t address[4]) {
	unsigned a = 0, b = 0, c = 0, d = 0;
	char extra = 0;
	if (std::sscanf(text, "%u.%u.%u.%u%c", &a, &b, &c, &d, &extra) != 4
			|| a > 255 || b > 255 || c > 255 || d > 255) {
		return false;
	}
	address[0] = static_cast<uint8_t>(a);
	address[1] = static_cast<uint8_t>(b);
	address[2] = static_cast<uint8_t>(c);
	address[3] = static_cast<uint8_t>(d);
	return true;
}

void setLoginPassword(qm::SqliteStorage &storage, const std::string &password) {
	storage.begin();
	try {
		storage.setApplicationPassword(qm::APPLICATION_LOGIN, password);
		storage.commit();
	} catch (...) {
		storage.rollback();
		throw;
	}
	std::printf("login server password stored\n");
}

int addAccount(qm::SqliteStorage &storage, uint32_t accountId, const std::string &password,
		long premiumDays, bool gamemaster, const std::vector<std::string> &rights,
		const std::vector<std::string> &characters, const std::vector<std::string> &newCharacters,
		const std::string &worldName, uint8_t sex) {
	for (const std::string &right : rights) {
		if (!qm::isKnownRight(right)) {
			std::fprintf(stderr, "unknown right: %s\n", right.c_str());
			return 2;
		}
	}
	for (const std::string &name : newCharacters) {
		if (name.empty() || name.size() > qm::MAX_NAME_LENGTH) {
			std::fprintf(stderr, "invalid character name: %s\n", name.c_str());
			return 2;
		}
	}
	std::vector<uint32_t> newIds;
	storage.begin();
	try {
		qm::AccountRecord account;
		account.id = accountId;
		account.password = password;
		account.gamemaster = gamemaster;
		if (premiumDays > 0) {
			// Times are 32-bit on the wire and in the database; far-off ends
			// stop at the largest value instead of wrapping into the past.
			int64_t until = static_cast<int64_t>(std::time(nullptr)) + static_cast<int64_t>(premiumDays) * 86400;
			account.premiumUntil = until > 0xFFFFFFFFLL ? 0xFFFFFFFFu : static_cast<uint32_t>(until);
		}
		storage.putAccount(account);
		for (const std::string &right : rights) {
			storage.addAccountRight(accountId, right);
		}
		for (const std::string &name : characters) {
			std::optional<qm::CharacterRecord> character = storage.findCharacterByName(name);
			if (!character) {
				storage.rollback();
				std::fprintf(stderr, "unknown character: %s\n", name.c_str());
				return 1;
			}
			character->accountId = accountId;
			storage.putCharacter(*character);
		}
		if (!newCharacters.empty()) {
			std::optional<qm::WorldRecord> world = storage.findWorld(worldName);
			if (!world) {
				storage.rollback();
				std::fprintf(stderr, "unknown world: %s\n", worldName.c_str());
				return 1;
			}
			for (const std::string &name : newCharacters) {
				if (storage.findCharacterByName(name)) {
					storage.rollback();
					std::fprintf(stderr, "character exists already: %s\n", name.c_str());
					return 1;
				}
				qm::CharacterRecord character;
				character.id = storage.nextCharacterId();
				character.worldId = world->id;
				character.accountId = accountId;
				character.name = name;
				character.sex = sex;
				storage.putCharacter(character);
				newIds.push_back(character.id);
			}
		}
		storage.commit();
	} catch (...) {
		storage.rollback();
		throw;
	}
	std::printf("account %u stored, %zu character(s) assigned, %zu right(s)\n", accountId,
		characters.size(), rights.size());
	for (size_t i = 0; i < newIds.size(); i++) {
		std::printf("new character %u: %s\n", newIds[i], newCharacters[i].c_str());
	}
	return 0;
}

} // namespace

int main(int argc, char **argv) {
	std::string database;
	qm::SeedOptions options;
	options.world = qm::defaultWorldConfig();
	bool haveAccount = false;
	long accountId = 0;
	std::string password;
	bool havePassword = false;
	long premiumDays = 0;
	bool gamemaster = false;
	std::vector<std::string> rights;
	std::vector<std::string> characters;
	std::vector<std::string> newCharacters;
	std::string worldName;
	long sex = 1;
	std::string loginPassword;
	bool haveLoginPassword = false;

	for (int i = 1; i < argc; i++) {
		std::string option = argv[i];
		if (option == "--help" || option == "-h") {
			usage(argv[0]);
			return 0;
		}
		if (option == "--no-players") {
			options.importPlayers = false;
			continue;
		}
		if (option == "--no-player-data") {
			options.importPlayerData = false;
			continue;
		}
		if (option == "--no-house-owners") {
			options.importHouseOwners = false;
			continue;
		}
		if (option == "--gamemaster") {
			gamemaster = true;
			continue;
		}
		if (i + 1 >= argc) {
			usage(argv[0]);
			return 2;
		}
		const char *value = argv[++i];
		long number = 0;
		bool ok = true;
		if (option == "--db") {
			database = value;
		} else if (option == "--game-root") {
			options.gameRoot = value;
		} else if (option == "--world-type") {
			ok = parseNumber(value, 0, 2, number);
			options.world.type = static_cast<uint8_t>(number);
		} else if (option == "--reboot-hour") {
			ok = parseNumber(value, 0, 23, number);
			options.world.rebootHour = static_cast<uint8_t>(number);
		} else if (option == "--address") {
			ok = parseAddress(value, options.world.address);
		} else if (option == "--port") {
			ok = parseNumber(value, 1, 65535, number);
			options.world.port = static_cast<uint16_t>(number);
		} else if (option == "--max-players") {
			ok = parseNumber(value, 0, 65535, number);
			options.world.maxPlayers = static_cast<uint16_t>(number);
		} else if (option == "--premium-player-buffer") {
			ok = parseNumber(value, 0, 65535, number);
			options.world.premiumPlayerBuffer = static_cast<uint16_t>(number);
		} else if (option == "--max-newbies") {
			ok = parseNumber(value, 0, 65535, number);
			options.world.maxNewbies = static_cast<uint16_t>(number);
		} else if (option == "--premium-newbie-buffer") {
			ok = parseNumber(value, 0, 65535, number);
			options.world.premiumNewbieBuffer = static_cast<uint16_t>(number);
		} else if (option == "--add-account") {
			ok = parseNumber(value, 1, 0x7FFFFFFF, accountId);
			haveAccount = true;
		} else if (option == "--password") {
			// The terminal gives UTF-8; the client sends ISO-8859-1, and
			// HandleLogin reads at most 29 bytes of it.
			password = qm::utf8ToLatin1(value);
			ok = password.size() <= qm::MAX_NAME_LENGTH;
			havePassword = true;
		} else if (option == "--login-password") {
			loginPassword = value;
			ok = !loginPassword.empty();
			haveLoginPassword = true;
		} else if (option == "--premium-days") {
			ok = parseNumber(value, 0, 36500, premiumDays);
		} else if (option == "--right") {
			rights.push_back(value);
		} else if (option == "--character") {
			// The terminal gives UTF-8; names are stored in ISO-8859-1.
			characters.push_back(qm::utf8ToLatin1(value));
		} else if (option == "--new-character") {
			newCharacters.push_back(qm::utf8ToLatin1(value));
		} else if (option == "--world") {
			worldName = qm::utf8ToLatin1(value);
		} else if (option == "--sex") {
			ok = parseNumber(value, 1, 2, sex);
		} else {
			usage(argv[0]);
			return 2;
		}
		if (!ok) {
			std::fprintf(stderr, "invalid value for %s: %s\n", option.c_str(), value);
			return 2;
		}
	}

	bool haveRoot = !options.gameRoot.empty();
	if (database.empty() || (haveRoot && haveAccount) || (!haveRoot && !haveAccount && !haveLoginPassword)) {
		usage(argv[0]);
		return 2;
	}
	if (haveAccount && !havePassword) {
		std::fprintf(stderr, "--add-account needs --password\n");
		return 2;
	}
	if (!newCharacters.empty() && (!haveAccount || worldName.empty())) {
		std::fprintf(stderr, "--new-character needs --add-account and --world\n");
		return 2;
	}

	try {
		// The login password is stored last, so that a run that fails
		// changes nothing.
		qm::SqliteStorage storage(database);
		if (!haveRoot && !haveAccount) {
			setLoginPassword(storage, loginPassword);
			return 0;
		}
		if (haveAccount) {
			int result = addAccount(storage, static_cast<uint32_t>(accountId), password, premiumDays,
				gamemaster, rights, characters, newCharacters, worldName, static_cast<uint8_t>(sex));
			if (result == 0 && haveLoginPassword) {
				setLoginPassword(storage, loginPassword);
			}
			return result;
		}

		qm::SeedReport report = qm::seedFromInstallation(storage, options);
		for (const std::string &warning : report.warnings) {
			std::fprintf(stderr, "warning: %s\n", warning.c_str());
		}
		const qm::WorldConfig &world = options.world;
		std::printf("world:                  %s (id %u, %s)\n", report.worldName.c_str(), report.worldId,
			report.privateWorld ? "private" : "public");
		std::printf("world config:           type %u, reboot %u:00, %u.%u.%u.%u:%u, players %u (premium"
			" buffer %u), newbies %u (premium buffer %u)\n",
			static_cast<unsigned>(world.type), static_cast<unsigned>(world.rebootHour),
			static_cast<unsigned>(world.address[0]), static_cast<unsigned>(world.address[1]),
			static_cast<unsigned>(world.address[2]), static_cast<unsigned>(world.address[3]),
			static_cast<unsigned>(world.port), static_cast<unsigned>(world.maxPlayers),
			static_cast<unsigned>(world.premiumPlayerBuffer), static_cast<unsigned>(world.maxNewbies),
			static_cast<unsigned>(world.premiumNewbieBuffer));
		std::printf("query manager entries:  %zu\n", report.queryManagerEntries);
		std::printf("player files:           %zu\n", report.playerFiles);
		std::printf("players imported:       %zu (male %zu, female %zu, other outfit %zu)\n",
			report.playersImported, report.playersMale, report.playersFemale, report.playersOtherOutfit);
		std::printf("players skipped:        %zu\n", report.playersSkipped);
		if (options.importPlayers && options.importPlayerData) {
			std::printf("player data imported:   %zu (%zu damaged files read as far as the old game read them)\n",
				report.playerDataImported, report.playerDataDamaged);
			std::printf("player data kept:       %zu (the database had it already)\n", report.playerDataKept);
			std::printf("player data failed:     %zu\n", report.playerDataFailed);
		}
		std::printf("house owners imported:  %zu (%zu without a character)\n", report.houseOwners,
			report.houseOwnersWithoutCharacter);
		std::printf("characters in database: %llu\n",
			static_cast<unsigned long long>(storage.countRows("characters")));
		std::printf("accounts in database:   %llu\n",
			static_cast<unsigned long long>(storage.countRows("accounts")));
		if (haveLoginPassword) {
			setLoginPassword(storage, loginPassword);
		}
	} catch (const std::exception &e) {
		std::fprintf(stderr, "error: %s\n", e.what());
		return 1;
	}
	return 0;
}
