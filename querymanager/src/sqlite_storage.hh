// SQLite implementation of the storage interface.

#ifndef QM_SQLITE_STORAGE_HH
#define QM_SQLITE_STORAGE_HH

#include "storage.hh"

struct sqlite3;

namespace qm {

class SqliteStorage : public Storage {
public:
	// Opens the database file, creating it and its schema if needed.
	// ":memory:" gives a private in-memory database.
	explicit SqliteStorage(const std::string &path);
	~SqliteStorage() override;

	SqliteStorage(const SqliteStorage &) = delete;
	SqliteStorage &operator=(const SqliteStorage &) = delete;

	void begin() override;
	void commit() override;
	void rollback() override;

	void setApplicationPassword(uint8_t applicationType, const std::string &password) override;
	bool checkApplicationPassword(uint8_t applicationType, const std::string &password) override;
	uint32_t putWorld(const std::string &name, const WorldConfig &config, bool privateWorld) override;
	std::optional<WorldRecord> findWorld(const std::string &name) override;
	std::optional<WorldRecord> getWorld(uint32_t worldId) override;
	void setWorldOnline(uint32_t worldId, bool online) override;
	void setOnlineRecord(uint32_t worldId, uint32_t players, uint32_t time) override;

	void putAccount(const AccountRecord &account) override;
	std::optional<AccountRecord> findAccount(uint32_t accountId) override;
	void addAccountRight(uint32_t accountId, const std::string &right) override;
	std::vector<std::string> listAccountRights(uint32_t accountId) override;
	uint32_t countOnlineCharacters(uint32_t accountId, uint32_t exceptCharacterId) override;
	bool addBuddy(uint32_t accountId, uint32_t buddyId) override;
	bool removeBuddy(uint32_t accountId, uint32_t buddyId) override;
	std::vector<PlayerEntry> listBuddies(uint32_t accountId) override;

	void banishIpAddress(const std::string &address, uint32_t until) override;
	bool isIpBanished(const std::string &address, uint32_t now) override;

	void putCharacter(const CharacterRecord &character) override;
	std::optional<CharacterRecord> findCharacter(uint32_t characterId) override;
	std::optional<CharacterRecord> findCharacterByName(const std::string &name) override;
	uint32_t nextCharacterId() override;
	void addCharacterRight(uint32_t characterId, const std::string &right) override;
	std::vector<std::string> listCharacterRights(uint32_t characterId) override;
	std::vector<PlayerEntry> listPlayers(uint32_t worldId, uint32_t minimumId, uint32_t limit) override;
	std::vector<CharacterRecord> listAccountCharacters(uint32_t accountId) override;
	bool incrementIsOnline(uint32_t characterId) override;
	bool decrementIsOnline(uint32_t characterId) override;
	uint32_t clearIsOnline(uint32_t worldId) override;
	bool recordLogout(uint32_t characterId, uint16_t level, const std::string &profession,
			const std::string &residence, uint32_t lastLogin, uint16_t tutorActivities) override;

	void replaceOnlinePlayers(uint32_t worldId, const std::vector<OnlinePlayer> &players) override;
	void addKilledCreatures(uint32_t worldId, const std::vector<KilledCreatures> &entries,
			uint32_t time) override;
	void addCharacterDeath(const CharacterDeath &death) override;

	void replaceHouses(uint32_t worldId, const std::vector<HouseRecord> &houses) override;
	std::vector<HouseOwner> listHouseOwners(uint32_t worldId) override;
	bool insertHouseOwner(uint32_t worldId, uint16_t houseId, uint32_t ownerId, uint32_t paidUntil) override;
	bool updateHouseOwner(uint32_t worldId, uint16_t houseId, uint32_t ownerId, uint32_t paidUntil) override;
	bool deleteHouseOwner(uint32_t worldId, uint16_t houseId) override;
	bool setHouseTransfer(uint32_t worldId, uint16_t houseId, uint32_t termination,
			uint32_t newOwnerId, bool accepted, uint32_t price) override;
	std::vector<HouseTransfer> listHouseTransfers(uint32_t worldId, uint32_t now) override;
	bool cancelHouseTransfer(uint32_t worldId, uint16_t houseId) override;
	std::vector<HouseEviction> listHousesOfFreeAccounts(uint32_t worldId, uint32_t now) override;
	std::vector<uint16_t> listHousesOfDeletedCharacters(uint32_t worldId) override;

	std::vector<uint16_t> listAuctions(uint32_t worldId) override;
	bool startAuction(uint32_t worldId, uint16_t houseId) override;
	bool setAuctionBid(uint32_t worldId, uint16_t houseId, uint32_t bidderId, uint32_t bid,
			uint32_t finishTime) override;
	std::vector<AuctionResult> takeFinishedAuctions(uint32_t worldId, uint32_t now) override;
	void excludeFromAuctions(uint32_t worldId, uint32_t characterId, bool banish, uint32_t now) override;

	// Number of rows of a table; for the seeding report and the tests.
	uint64_t countRows(const std::string &table);

private:
	void exec(const char *sql);
	void createSchema();

	sqlite3 *Db;
	bool InTransaction;
};

} // namespace qm

#endif
