// Seeding: fills a query manager database from a game server installation.
//
// What the installation provides:
//   .tibia            world name ("World"), public/private ("State") and the
//                     query manager password ("QueryManager" entries)
//   usr/NN/<id>.usr   one file per character: id, name, outfit, last login
//   dat/owners.dat    house owners and the time their rent is paid until
// What it does not provide, and seeding therefore takes from options:
//   the world configuration the game asks for with loadWorldConfig.
// Accounts, passwords, premium time, guilds, rights and buddy lists are in
// none of these files; characters are imported without an account.

#ifndef QM_SEED_HH
#define QM_SEED_HH

#include "storage.hh"

#include <cstddef>
#include <string>
#include <vector>

namespace qm {

// Key ReadConfig (config.c, 0x8052d00) passes to DisguisePassword for the
// QueryManager entries; the static PasswordKey at 0x80f0a71 in bin/game.
extern const char PasswordKey[];

// DisguisePassword (config.c, 0x8052c10). The game applies it to the
// password it read from .tibia and sends the result in the login request.
// Throws std::runtime_error if the key is shorter than the password.
std::string disguisePassword(const std::string &password, const std::string &key);

struct QueryManagerEntry {
	std::string host;
	uint32_t port = 0;
	std::string password;        // as written in .tibia
};

struct TibiaConfig {
	std::string world;
	bool privateWorld = false;
	std::vector<QueryManagerEntry> queryManagers;
};

// Reads the settings seeding needs from a .tibia file.
TibiaConfig readTibiaConfig(const std::string &path);

// What a .usr file says about a character.
struct PlayerFile {
	uint32_t id = 0;
	std::string name;
	uint32_t originalOutfit = 0;
	uint32_t lastLogin = 0;
	bool hasId = false;
	bool hasName = false;
};

// Reads the head of a .usr file (LoadPlayerData, dbfuncs.c, 0x806c240,
// reads the same keys with the same reader).
PlayerFile readPlayerFile(const std::string &path);

// Sex for an outfit, from the ranges TPlayer::CheckOutfit (crplayer.c,
// 0x80addc0) accepts: 128..134 for Sex 1 (male), 136..142 for Sex 2
// (female). Returns 0 for any other outfit.
uint8_t sexFromOutfit(uint32_t outfit);

struct OwnerEntry {
	uint16_t houseId = 0;
	uint32_t ownerId = 0;
	uint32_t paidUntil = 0;
};

// Reads the house owners from dat/owners.dat (LoadOwners, houses.c,
// 0x80663d0, reads the same file).
std::vector<OwnerEntry> readOwners(const std::string &path);

struct SeedOptions {
	std::string gameRoot;                // directory with .tibia, usr/ and dat/
	WorldConfig world;                   // values for loadWorldConfig
	bool importPlayers = true;
	bool importHouseOwners = true;
};

struct SeedReport {
	std::string worldName;
	uint32_t worldId = 0;
	bool privateWorld = false;
	size_t queryManagerEntries = 0;
	size_t playerFiles = 0;              // .usr files found
	size_t playersImported = 0;
	size_t playersSkipped = 0;           // unreadable or inconsistent files
	size_t playersMale = 0;
	size_t playersFemale = 0;
	size_t playersOtherOutfit = 0;       // sex not derivable, stored as male
	size_t houseOwners = 0;
	size_t houseOwnersWithoutCharacter = 0;
	std::vector<std::string> warnings;
};

// The defaults for the world configuration (see seed.cc for the reasons).
WorldConfig defaultWorldConfig();

// Seeds the database in one transaction. Throws on a missing or unusable
// installation; the database is unchanged in that case.
SeedReport seedFromInstallation(Storage &storage, const SeedOptions &options);

} // namespace qm

#endif
