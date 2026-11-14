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

// The query types: the first byte of a request (prepareQuery), named after the
// TQueryManagerConnection method that sends it. The same names as in the
// query manager's protocol.hh.
const int QUERY_LOGIN = 0;
const int QUERY_CHECK_ACCOUNT_PASSWORD = 10;
const int QUERY_LOGIN_ADMIN = 12;
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
const int QUERY_GET_KEPT_CHARACTERS = 200;
const int QUERY_GET_DELETED_CHARACTERS = 201;
const int QUERY_DELETE_OLD_CHARACTER = 202;
const int QUERY_GET_HIDDEN_CHARACTERS = 203;
const int QUERY_CREATE_HIGHSCORES = 204;
const int QUERY_CREATE_CENSUS = 205;
const int QUERY_CREATE_KILL_STATISTICS = 206;
const int QUERY_GET_PLAYERS_ONLINE = 207;
const int QUERY_GET_WORLDS = 208;
const int QUERY_GET_SERVER_LOAD = 209;
const int QUERY_INSERT_PAYMENT_DATA_OLD = 210;
const int QUERY_ADD_PAYMENT_OLD = 211;
const int QUERY_CANCEL_PAYMENT_OLD = 212;
const int QUERY_INSERT_PAYMENT_DATA_NEW = 213;
const int QUERY_ADD_PAYMENT_NEW = 214;
const int QUERY_CANCEL_PAYMENT_NEW = 215;

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

struct TQueryManagerSettings {
	char Host[50];
	int Port;
	char Password[30];
};

const int MAX_NUMBER_OF_QUERYMANAGERS = 10;

extern int NumberOfQueryManagers;
extern TQueryManagerSettings QUERY_MANAGER[10];

struct TQueryManagerConnection {
	// The default is what TQueryManagerConnectionPool::init passes to every
	// element of its new[] (0x4000); default arguments are not in the DWARF.
	TQueryManagerConnection(int QueryBufferSize = 16384);
	~TQueryManagerConnection();
	bool isConnected() { return Socket >= 0; }
	int checkAccountPassword(uint32_t AccountID, char *Password, char *IPAddress);
	int loginAdmin(uint32_t AccountID, bool PrivateWorld, int &NumberOfCharacters, char (*Characters)[30], char (*Worlds)[30], unsigned char (*IPAddresses)[4], unsigned short *Ports, unsigned short &PremiumDaysLeft);
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
	int getKeptCharacters(uint32_t MinimumCharacterID, int &NumberOfPlayers, uint32_t *CharacterIDs);
	int getDeletedCharacters(uint32_t MinimumCharacterID, int &NumberOfPlayers, uint32_t *CharacterIDs);
	int deleteOldCharacter(uint32_t CharacterID);
	int getHiddenCharacters(uint32_t MinimumCharacterID, int &NumberOfPlayers, uint32_t *CharacterIDs);
	int createHighscores(int NumberOfPlayers, uint32_t *CharacterIDs, int *ExpPoints, int *ExpLevel, int *Fist, int *Club, int *Axe, int *Sword, int *Distance, int *Shielding, int *Magic, int *Fishing);
	int createCensus();
	int createKillStatistics();
	int getPlayersOnline(int &NumberOfWorlds, char (*Names)[30], unsigned short *Players);
	int getWorlds(int &NumberOfWorlds, char (*Names)[30]);
	int getServerLoad(char *World, int Period, int *Data);
	int insertPaymentDataOld(uint32_t PurchaseNr, uint32_t ReferenceNr, char *FirstName, char *LastName, char *Company, char *Street, char *Zip, char *City, char *Country, char *State, char *Phone, char *Fax, char *EMail, char *PaymentMethod, uint32_t ProductID, char *Registrant, uint32_t AccountID, uint32_t &PaymentID);
	int addPaymentOld(uint32_t AccountID, char *Description, uint32_t PaymentID, int Days, int &ActionTaken);
	int cancelPaymentOld(uint32_t PurchaseNr, uint32_t ReferenceNr, uint32_t AccountID, bool &IllegalUse, char *EMailAddress);
	int insertPaymentDataNew(uint32_t PurchaseNr, uint32_t ReferenceNr, char *FirstName, char *LastName, char *Company, char *Street, char *Zip, char *City, char *Country, char *State, char *Phone, char *Fax, char *EMail, char *PaymentMethod, uint32_t ProductID, char *Registrant, char *PaymentKey, uint32_t &PaymentID);
	int addPaymentNew(char *PaymentKey, uint32_t PaymentID, int &ActionTaken, char *EMailReceiver);
	int cancelPaymentNew(uint32_t PurchaseNr, uint32_t ReferenceNr, char *PaymentKey, bool &IllegalUse, bool &Present, char *EMailAddress);
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
	void getBytes(unsigned char *Buffer, int Count);
	int executeQuery(int Timeout, bool AutoReconnect);
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

struct TQueryManagerConnectionPool {
	TQueryManagerConnectionPool(int Connections);
	void init();
	void exit();
	TQueryManagerConnection *getConnection();
	void releaseConnection(TQueryManagerConnection *Connection);
private:
	int NumberOfConnections;
	TQueryManagerConnection *QueryManagerConnection;
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
