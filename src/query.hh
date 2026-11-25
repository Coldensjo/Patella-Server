// query.hh: the game server's client of the query manager (query.cc), with the query types, status bytes and login
// error codes of its protocol.

#ifndef PATELLA_QUERY_HH
#define PATELLA_QUERY_HH

#include <cstdint>
#include <cstdlib>
#include <time.h>

#include "utils.hh"
#include "threads.hh"

// The ApplicationType that the game server sends in its login request
// (main.cc's SetQueryManagerLoginData), followed by its world name. The same
// name as in the query manager's protocol.hh.
const int APPLICATION_GAME = 1;

// The query types the game sends: the first byte of a request (prepareQuery),
// named after the TQueryManagerConnection method that sends it (QUERY_LOGIN:
// connect's login request). The same names as in the query manager's
// protocol.hh, which lists the other tools' too. 54 and 55 are this project's
// own: the player data, which the 2006 game kept in its player files; so are
// 56 to 59, the admin port's punishments and renames (scripting_admin.cc).
const int QUERY_LOGIN = 0;
const int QUERY_LOGIN_GAME = 20;
const int QUERY_LOGOUT_GAME = 21;
const int QUERY_SET_NAMELOCK = 23;
const int QUERY_BANISH_ACCOUNT = 25;
const int QUERY_SET_NOTATION = 26;
const int QUERY_REPORT_STATEMENT = 27;
const int QUERY_BANISH_IP_ADDRESS = 28;
const int QUERY_LOG_CHARACTER_DEATH = 29;
const int QUERY_ADD_BUDDY = 30;
const int QUERY_REMOVE_BUDDY = 31;
const int QUERY_DECREMENT_IS_ONLINE = 32;
const int QUERY_FINISH_AUCTIONS = 33;
const int QUERY_TRANSFER_HOUSES = 35;
const int QUERY_EVICT_FREE_ACCOUNTS = 36;
const int QUERY_EVICT_DELETED_CHARACTERS = 37;
const int QUERY_EVICT_EX_GUILDLEADERS = 38;
const int QUERY_INSERT_HOUSE_OWNER = 39;
const int QUERY_UPDATE_HOUSE_OWNER = 40;
const int QUERY_DELETE_HOUSE_OWNER = 41;
const int QUERY_GET_HOUSE_OWNERS = 42;
const int QUERY_GET_AUCTIONS = 43;
const int QUERY_START_AUCTION = 44;
const int QUERY_INSERT_HOUSES = 45;
const int QUERY_CLEAR_IS_ONLINE = 46;
const int QUERY_CREATE_PLAYERLIST = 47;
const int QUERY_LOG_KILLED_CREATURES = 48;
const int QUERY_LOAD_PLAYERS = 50;
const int QUERY_EXCLUDE_FROM_AUCTIONS = 51;
const int QUERY_CANCEL_HOUSE_TRANSFER = 52;
const int QUERY_LOAD_WORLD_CONFIG = 53;
const int QUERY_LOAD_PLAYER_DATA = 54;
const int QUERY_SAVE_PLAYER_DATA = 55;
const int QUERY_BANISH_ACCOUNT_FOR = 56;
const int QUERY_LIFT_BANISHMENT = 57;
const int QUERY_BANISH_IP_ADDRESS_FOR = 58;
const int QUERY_RENAME_CHARACTER = 59;

// The status byte of a response, which executeQuery returns: STATUS_FAILED
// also when the request could not be sent or answered. STATUS_ERROR is
// followed by an error code in the queries that have them.
const int STATUS_OK = 0;
const int STATUS_ERROR = 1;
const int STATUS_FAILED = 3;

// loginGame's error codes (after STATUS_ERROR), which PerformRegistration
// turns into the messages for the client. 5 has no meaning.
const int LOGIN_CHARACTER_UNKNOWN = 1;
const int LOGIN_CHARACTER_DELETED = 2;
const int LOGIN_WRONG_WORLD = 3;
const int LOGIN_NOT_INVITED = 4;
const int LOGIN_WRONG_PASSWORD = 6;
const int LOGIN_ACCOUNT_BLOCKED = 7;
const int LOGIN_ACCOUNT_DELETED = 8;
const int LOGIN_IP_BLOCKED = 9;
const int LOGIN_ACCOUNT_BANISHED = 10;
const int LOGIN_NAMELOCKED = 11;
const int LOGIN_IP_BANISHED = 12;
const int LOGIN_ALREADY_ONLINE = 13;
const int LOGIN_NOT_GAMEMASTER = 14;
const int LOGIN_WRONG_ACCOUNT = 15;

// The query managers the config may list (QUERY_MANAGER, config.cc).
const int MAX_QUERY_MANAGERS = 10;

struct TPlayerData;

struct TQueryManagerSettings {
	char Host[50];
	int Port;
	char Password[30];
};

extern int NumberOfQueryManagers;
extern TQueryManagerSettings QUERY_MANAGER[MAX_QUERY_MANAGERS];

struct TQueryManagerConnection {
	// TQueryManagerConnectionPool::init makes each of its connections with the
	// default, 16384 bytes, the smallest buffer a connection gets.
	TQueryManagerConnection(int QueryBufferSize = 16384);
	~TQueryManagerConnection();
	bool isConnected() { return Socket >= 0; }
	int loadWorldConfig(int &WorldType, int &RebootTime, int *IPAddress, int &Port, int &MaxPlayers, int &PremiumPlayerBuffer, int &MaxNewbies, int &PremiumNewbieBuffer);
	int loginGame(uint32_t AccountID, char *PlayerName, char *Password, char *IPAddress, bool PrivateWorld, bool PremiumAccountRequired, bool GamemasterRequired, uint32_t &CharacterID, int &Sex, char *Guild, char *Rank, char *Title, int &NumberOfBuddies, uint32_t *BuddyIDs, char (*BuddyNames)[30], unsigned char *Rights, bool &PremiumAccountActivated);
	int logoutGame(uint32_t CharacterID, int Level, char *Profession, char *Residence, time_t LastLoginTime, int TutorActivities);
	int setNotation(uint32_t GamemasterID, char *PlayerName, char *IPAddress, char *Reason, char *Comment, uint32_t &BanishmentID);
	int setNamelock(uint32_t GamemasterID, char *PlayerName, char *IPAddress, char *Reason, char *Comment);
	int banishAccount(uint32_t GamemasterID, char *PlayerName, char *IPAddress, char *Reason, char *Comment, bool &FinalWarning, int &Days, uint32_t &BanishmentID);
	int reportStatement(uint32_t ReporterID, char *PlayerName, char *Reason, char *Comment, uint32_t BanishmentID, uint32_t StatementID, int NumberOfStatements, uint32_t *StatementIDs, uint32_t *TimeStamps, uint32_t *CharacterIDs, char (*Channels)[30], char (*Texts)[256]);
	int banishIPAddress(uint32_t GamemasterID, char *PlayerName, char *IPAddress, char *Reason, char *Comment);
	int logCharacterDeath(uint32_t CharacterID, int Level, uint32_t Offender, char *Remark, bool Unjustified, time_t Time);
	int addBuddy(uint32_t AccountID, uint32_t Buddy);
	int removeBuddy(uint32_t AccountID, uint32_t Buddy);
	int decrementIsOnline(uint32_t CharacterID);
	int finishAuctions(int &NumberOfAuctions, unsigned short *HouseIDs, uint32_t *CharacterIDs, char (*CharacterNames)[30], int *Bids);
	int excludeFromAuctions(uint32_t CharacterID, bool Banish);
	// The admin port's requests (this project's own; the query manager's
	// handlers.cc has the layouts). Each gives 0 on success, the error code
	// of an ERROR response (1, 2 or 3), or -1 when the request failed.
	// banishAccountFor: Days 0 is for good; Until is the banishment's end
	// (0xFFFFFFFF: for good). banishIPAddressFor: the same for an address.
	int banishAccountFor(uint32_t GamemasterID, const char *PlayerName, const char *Reason, const char *Comment,
			uint32_t Days, uint32_t &BanishmentID, uint32_t &Until);
	int liftBanishment(const char *PlayerName);
	int banishIPAddressFor(uint32_t GamemasterID, const char *IPAddress, const char *Reason, const char *Comment,
			uint32_t Days, uint32_t &Until);
	int renameCharacter(uint32_t CharacterID, const char *NewName);
	int transferHouses(int &NumberOfTransfers, unsigned short *HouseIDs, uint32_t *NewOwnerIDs, char (*NewOwnerNames)[30], int *Prices);
	int cancelHouseTransfer(unsigned short HouseID);
	int evictFreeAccounts(int &NumberOfEvictions, unsigned short *HouseIDs, uint32_t *OwnerIDs);
	int evictDeletedCharacters(int &NumberOfEvictions, unsigned short *HouseIDs);
	int evictExGuildleaders(int NumberOfGuildhouses, int &NumberOfEvictions, unsigned short *HouseIDs, uint32_t *Guildleaders);
	int insertHouseOwner(unsigned short HouseID, uint32_t OwnerID, int PaidUntil);
	int updateHouseOwner(unsigned short HouseID, uint32_t OwnerID, int PaidUntil);
	int deleteHouseOwner(unsigned short HouseID);
	int getHouseOwners(int &NumberOfHouses, unsigned short *HouseIDs, uint32_t *OwnerIDs, char (*OwnerNames)[30], int *PaidUntils);
	int getAuctions(int &NumberOfAuctions, unsigned short *HouseIDs);
	int startAuction(unsigned short HouseID);
	int insertHouses(int NumberOfHouses, unsigned short *HouseIDs, char **Names, int *Rents, char **Descriptions, int *Sizes, int *PositionsX, int *PositionsY, int *PositionsZ, char (*Towns)[30], bool *Guildhouses);
	int clearIsOnline(int &NumberOfAffectedPlayers);
	int createPlayerlist(int NumberOfPlayers, char **Names, int *Levels, char (*Professions)[30], bool &NewRecord);
	int logKilledCreatures(int NumberOfRaces, char **Names, int *KilledPlayers, int *KilledCreatures);
	int loadPlayers(uint32_t MinimumCharacterID, int &NumberOfPlayers, char (*Names)[30], uint32_t *CharacterIDs);
	// 0 with the slot of Slot->CharacterID filled, 1 if the query manager has
	// no data of the character, -1 if the request failed or its data could
	// not be read.
	int loadPlayerData(TPlayerData *Slot);
	// Record: the record WritePlayerData (dbfuncs.cc) wrote. 0, or -1 if the
	// request failed.
	int savePlayerData(uint32_t CharacterID, unsigned char *Record, int Size);
private:
	void connect();
	void disconnect();
	int write(unsigned char *Buffer, int Size);
	int read(unsigned char *Buffer, int Size, int Timeout);
protected:
	void prepareQuery(int QueryType);
	void sendFlag(bool f);
	void sendByte(unsigned char b);
	void sendWord(unsigned short w);
	void sendQuad(uint32_t q);
	void sendString(char *s);
	void sendBytes(unsigned char *Buffer, int Count);
	bool getFlag();
	unsigned char getByte();
	unsigned short getWord();
	uint32_t getQuad();
	void getString(char *Buffer, int MaxLength);
	int executeQuery(int Timeout, bool AutoReconnect);
	// 0, the error code of an ERROR response (1 to MaxCode) or -1 (logged).
	int adminResult(int Result, int MaxCode, const char *Name);
private:
	int BufferSize;
	unsigned char *Buffer;
protected:
	TReadBuffer ReadBuffer;
private:
	TWriteBuffer WriteBuffer;
	int Socket;
	bool QueryOk;
};

// Connections that threads share, one request at a time each. Each connection
// gets a buffer of QueryBufferSize bytes (TQueryManagerConnection's default
// unless given).
struct TQueryManagerConnectionPool {
	TQueryManagerConnectionPool(int Connections, int QueryBufferSize = 16384);
	void init();
	void exit();
	TQueryManagerConnection *getConnection();
	void releaseConnection(TQueryManagerConnection *Connection);
private:
	int NumberOfConnections;
	int BufferSize;
	TQueryManagerConnection **QueryManagerConnection;
	bool *QueryManagerConnectionFree;
	Semaphore FreeQueryManagerConnections;
	Semaphore QueryManagerConnectionMutex;
};

struct TQueryManagerPoolConnection {
	TQueryManagerPoolConnection(TQueryManagerConnectionPool *Pool);
	~TQueryManagerPoolConnection();
	TQueryManagerConnection *operator->() { return QueryManagerConnection; }
private:
	TQueryManagerConnectionPool *QueryManagerConnectionPool;
	TQueryManagerConnection *QueryManagerConnection;
};

void SetQueryManagerLoginData(int Type, char *Data);

extern int ApplicationType;
extern char LoginData[30];

#endif
