// Storage interface of the query manager.
//
// The request handlers only talk to this interface; the SQLite
// implementation is in sqlite_storage.cc and is the only place with SQL.
//
// Conventions:
//  - Strings are in the game's 8-bit character set (ISO-8859-1), exactly as
//    they travel on the wire. An implementation converts as it needs to.
//  - Times are Unix seconds in a uint32_t, as the game sends them.
//  - An id of 0 means "none" (no account, no owner, no bidder).
//  - Methods throw StorageError when the backend fails. A `bool` result
//    reports whether the operation applied (row existed, row was new, ...).
//  - The interface is not thread-safe; the dispatcher serialises requests.

#ifndef QM_STORAGE_HH
#define QM_STORAGE_HH

#include "playerdata.hh"

#include <cstdint>
#include <optional>
#include <stdexcept>
#include <string>
#include <vector>

namespace qm {

class StorageError : public std::runtime_error {
public:
	explicit StorageError(const std::string &what) : std::runtime_error(what) {}
};

// The values of the loadWorldConfig response.
struct WorldConfig {
	uint8_t type = 0;                    // TWorldType: 0 NORMAL, 1 NON_PVP, 2 PVP_ENFORCED
	uint8_t rebootHour = 0;              // hour of day; the game multiplies by 60
	uint8_t address[4] = {0, 0, 0, 0};   // address the game binds its listen socket to
	uint16_t port = 0;
	uint16_t maxPlayers = 0;
	uint16_t premiumPlayerBuffer = 0;
	uint16_t maxNewbies = 0;
	uint16_t premiumNewbieBuffer = 0;
};

struct WorldRecord {
	uint32_t id = 0;
	std::string name;
	WorldConfig config;
	bool privateWorld = false;       // state = "private" in config.toml
	bool online = false;             // false after a playerlist with count 0xFFFF
	uint32_t onlineRecord = 0;       // highest player count reported so far
	uint32_t onlineRecordTime = 0;
};

struct AccountRecord {
	uint32_t id = 0;
	std::string password;            // as the client sends it; compared byte for byte
	uint32_t premiumUntil = 0;       // premium while now < premiumUntil
	uint32_t banishedUntil = 0;      // banished while now < banishedUntil
	bool deleted = false;
	bool gamemaster = false;         // may log in with a gamemaster client
};

struct CharacterRecord {
	uint32_t id = 0;
	uint32_t worldId = 0;
	uint32_t accountId = 0;
	std::string name;
	uint8_t sex = 1;                 // 1 male, 2 female (TCreature::Sex)
	std::string guild;
	std::string rank;
	std::string title;
	bool guildLeader = false;
	uint16_t level = 0;              // as of the last logout
	std::string profession;          // as of the last logout
	std::string residence;           // as of the last logout
	uint32_t lastLogin = 0;
	uint32_t tutorActivities = 0;
	uint32_t isOnline = 0;           // counter, see decrementIsOnline
	bool deleted = false;
	bool namelocked = false;
};

struct PlayerEntry {
	uint32_t id = 0;
	std::string name;
};

struct OnlinePlayer {
	std::string name;
	uint16_t level = 0;
	std::string profession;
};

struct KilledCreatures {
	std::string race;
	uint32_t killedPlayers = 0;
	uint32_t killedCreatures = 0;
};

struct CharacterDeath {
	uint32_t characterId = 0;
	uint16_t level = 0;
	uint32_t offenderId = 0;
	std::string remark;
	bool unjustified = false;
	uint32_t time = 0;
};

struct HouseRecord {
	uint16_t id = 0;
	std::string name;
	uint32_t rent = 0;
	std::string description;
	uint16_t size = 0;
	uint16_t x = 0;
	uint16_t y = 0;
	uint8_t z = 0;
	std::string town;
	bool guildhouse = false;
};

struct HouseOwner {
	uint16_t houseId = 0;
	uint32_t ownerId = 0;
	std::string ownerName;           // empty if the character is not known
	uint32_t paidUntil = 0;
};

struct AuctionResult {
	uint16_t houseId = 0;
	uint32_t characterId = 0;
	std::string characterName;
	uint32_t bid = 0;
};

struct HouseTransfer {
	uint16_t houseId = 0;
	uint32_t newOwnerId = 0;         // 0: the owner moves out, nobody moves in
	std::string newOwnerName;
	uint32_t price = 0;
};

// One banishment of an account (schema version 3): who banished it, why,
// when, until when (PERMANENT_BANISHMENT: for good) and whether it was a
// final warning. characterId is the character it was given for (0: none).
struct BanishmentRecord {
	uint32_t id = 0;
	uint32_t accountId = 0;
	uint32_t characterId = 0;
	uint32_t gamemasterId = 0;
	std::string reason;
	std::string comment;
	uint32_t time = 0;
	uint32_t until = 0;
	bool finalWarning = false;
};

// The end of a banishment that never ends (banished while now < until).
const uint32_t PERMANENT_BANISHMENT = 0xFFFFFFFF;

// An account's banishments so far.
struct BanishmentHistory {
	uint32_t count = 0;
	bool finalWarning = false;       // one of them was a final warning
};

// A notation on a character (schema version 3): a note in its record, no
// punishment.
struct NotationRecord {
	uint32_t id = 0;
	uint32_t characterId = 0;
	uint32_t gamemasterId = 0;
	std::string reason;
	std::string comment;
	uint32_t time = 0;
};

struct HouseEviction {
	uint16_t houseId = 0;
	uint32_t ownerId = 0;
};

class Storage {
public:
	virtual ~Storage() = default;

	// --- transactions; a request is handled inside one
	virtual void begin() = 0;
	virtual void commit() = 0;
	virtual void rollback() = 0;

	// --- applications and worlds
	// Password a program of this application type must present at login.
	virtual void setApplicationPassword(uint8_t applicationType, const std::string &password) = 0;
	virtual bool checkApplicationPassword(uint8_t applicationType, const std::string &password) = 0;
	// Creates the world or updates its configuration; returns the world id.
	virtual uint32_t putWorld(const std::string &name, const WorldConfig &config, bool privateWorld) = 0;
	virtual std::optional<WorldRecord> findWorld(const std::string &name) = 0;
	virtual std::optional<WorldRecord> getWorld(uint32_t worldId) = 0;
	virtual void setWorldOnline(uint32_t worldId, bool online) = 0;
	virtual void setOnlineRecord(uint32_t worldId, uint32_t players, uint32_t time) = 0;

	// --- accounts
	virtual void putAccount(const AccountRecord &account) = 0;
	virtual std::optional<AccountRecord> findAccount(uint32_t accountId) = 0;
	virtual void addAccountRight(uint32_t accountId, const std::string &right) = 0;
	virtual std::vector<std::string> listAccountRights(uint32_t accountId) = 0;
	// Number of characters of the account with a non-zero online counter,
	// not counting `exceptCharacterId`.
	virtual uint32_t countOnlineCharacters(uint32_t accountId, uint32_t exceptCharacterId) = 0;
	virtual bool addBuddy(uint32_t accountId, uint32_t buddyId) = 0;
	virtual bool removeBuddy(uint32_t accountId, uint32_t buddyId) = 0;
	// Buddies whose character still exists, ordered by character id.
	virtual std::vector<PlayerEntry> listBuddies(uint32_t accountId) = 0;

	// --- IP banishments
	virtual void banishIpAddress(const std::string &address, uint32_t until) = 0;
	virtual bool isIpBanished(const std::string &address, uint32_t now) = 0;

	// --- characters
	virtual void putCharacter(const CharacterRecord &character) = 0;
	virtual std::optional<CharacterRecord> findCharacter(uint32_t characterId) = 0;
	// Case-insensitive.
	virtual std::optional<CharacterRecord> findCharacterByName(const std::string &name) = 0;
	// One above the highest character id of all worlds (1 if there is none);
	// an id no character has, for creating a new one.
	virtual uint32_t nextCharacterId() = 0;
	virtual void addCharacterRight(uint32_t characterId, const std::string &right) = 0;
	virtual std::vector<std::string> listCharacterRights(uint32_t characterId) = 0;
	// Characters of the world that are not deleted, with id >= minimumId,
	// ordered by id, at most `limit` of them.
	virtual std::vector<PlayerEntry> listPlayers(uint32_t worldId, uint32_t minimumId, uint32_t limit) = 0;
	// Characters of the account that are not deleted, in all worlds, ordered
	// by name without regard to case.
	virtual std::vector<CharacterRecord> listAccountCharacters(uint32_t accountId) = 0;
	virtual bool incrementIsOnline(uint32_t characterId) = 0;
	// Does not go below zero. False if the character does not exist.
	virtual bool decrementIsOnline(uint32_t characterId) = 0;
	// Resets the counter of every character of the world; returns how many
	// characters had a non-zero counter.
	virtual uint32_t clearIsOnline(uint32_t worldId) = 0;
	// Stores what the game reports at logout and adds to the tutor
	// activities. False if the character does not exist.
	virtual bool recordLogout(uint32_t characterId, uint16_t level, const std::string &profession,
			const std::string &residence, uint32_t lastLogin, uint16_t tutorActivities) = 0;

	// --- statistics
	virtual void replaceOnlinePlayers(uint32_t worldId, const std::vector<OnlinePlayer> &players) = 0;
	virtual void addKilledCreatures(uint32_t worldId, const std::vector<KilledCreatures> &entries,
			uint32_t time) = 0;
	virtual void addCharacterDeath(const CharacterDeath &death) = 0;

	// --- houses
	// Replaces the house list of the world.
	virtual void replaceHouses(uint32_t worldId, const std::vector<HouseRecord> &houses) = 0;
	// Owners of houses that are in the house list, ordered by house id.
	virtual std::vector<HouseOwner> listHouseOwners(uint32_t worldId) = 0;
	// False if the house already has an owner entry. The house need not be
	// in the house list (seeding runs before the game sends the list); the
	// listings only show entries of listed houses.
	virtual bool insertHouseOwner(uint32_t worldId, uint16_t houseId, uint32_t ownerId, uint32_t paidUntil) = 0;
	// False (and nothing changes) if the house has no owner entry. A pending
	// transfer stays.
	virtual bool updateHouseOwner(uint32_t worldId, uint16_t houseId, uint32_t ownerId, uint32_t paidUntil) = 0;
	// False if there was no owner entry.
	virtual bool deleteHouseOwner(uint32_t worldId, uint16_t houseId) = 0;
	// Registers that the owner gives the house up at `termination`, to
	// `newOwnerId` (0: nobody) for `price`. This is the web site's side of
	// transferHouses. False if the house has no owner entry.
	virtual bool setHouseTransfer(uint32_t worldId, uint16_t houseId, uint32_t termination,
			uint32_t newOwnerId, bool accepted, uint32_t price) = 0;
	// Transfers whose termination time has come and which either have no new
	// owner or were accepted by the new owner. Read-only.
	virtual std::vector<HouseTransfer> listHouseTransfers(uint32_t worldId, uint32_t now) = 0;
	virtual bool cancelHouseTransfer(uint32_t worldId, uint16_t houseId) = 0;
	// Houses whose owner belongs to an account that is known and not premium.
	virtual std::vector<HouseEviction> listHousesOfFreeAccounts(uint32_t worldId, uint32_t now) = 0;
	// Houses whose owner is a character that is marked deleted.
	virtual std::vector<uint16_t> listHousesOfDeletedCharacters(uint32_t worldId) = 0;

	// --- auctions
	// Houses of the house list that are up for auction, ordered by house id.
	virtual std::vector<uint16_t> listAuctions(uint32_t worldId) = 0;
	// False if the house is unknown or already up for auction.
	virtual bool startAuction(uint32_t worldId, uint16_t houseId) = 0;
	// Records the highest bid and the end of an auction. This is the web
	// site's side of finishAuctions. False if the house is not up for auction.
	virtual bool setAuctionBid(uint32_t worldId, uint16_t houseId, uint32_t bidderId, uint32_t bid,
			uint32_t finishTime) = 0;
	// Auctions that have a bidder and whose finish time has come. They are
	// removed, so each is returned once.
	virtual std::vector<AuctionResult> takeFinishedAuctions(uint32_t worldId, uint32_t now) = 0;
	// Withdraws the bids of the character in the world's running auctions
	// and records the exclusion.
	virtual void excludeFromAuctions(uint32_t worldId, uint32_t characterId, bool banish, uint32_t now) = 0;

	// --- player data (playerdata.hh)
	// The character's player data, keyed by character id alone (the game's
	// player files were, too); nothing if none is stored.
	virtual std::optional<PlayerData> loadPlayerData(uint32_t characterId) = 0;
	// Replaces the character's player data (data.characterId) as a whole.
	virtual void savePlayerData(const PlayerData &data) = 0;
	virtual bool hasPlayerData(uint32_t characterId) = 0;

	// --- banishments, notations, renames (schema version 3)
	// Records the banishment and sets the account's banished_until to its
	// end (the later of that and a banishment already running); gives its id.
	virtual uint32_t addBanishment(const BanishmentRecord &banishment) = 0;
	// The account's banishments recorded so far.
	virtual BanishmentHistory banishmentHistory(uint32_t accountId) = 0;
	// Ends the account's banishment (banished_until 0); false when it was
	// not banished at now.
	virtual bool liftBanishment(uint32_t accountId, uint32_t now) = 0;
	// Records the notation; gives its id.
	virtual uint32_t addNotation(const NotationRecord &notation) = 0;
	virtual std::vector<NotationRecord> listNotations(uint32_t characterId) = 0;
	// Gives the character the name (characters and player_data) and lifts
	// its namelock; false when another character has the name (in any case).
	virtual bool renameCharacter(uint32_t characterId, const std::string &name) = 0;
};

} // namespace qm

#endif
