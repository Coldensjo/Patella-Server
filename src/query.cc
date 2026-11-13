#include <algorithm>
#include <cstdint>
#include <cstdlib>
#include <time.h>

#include "script.hh"
#include "config.hh"
#include "threads.hh"
#include "objects.hh"
#include "map.hh"
#include "info.hh"
#include "dbfuncs.hh"
#include "query.hh"
#include "utils.hh"

int ApplicationType;	// query.c:85
char LoginData[30];	// query.c:86

// 0x080e0210  query.c:97
void SetQueryManagerLoginData(int /*Type*/, char * /*Data*/) { abort(); /* STUB */ }

// 0x080e0310  query.c:109
TQueryManagerConnection::TQueryManagerConnection(int QueryBufferSize) : Buffer(new unsigned char[std::max(QueryBufferSize, 16384)]), ReadBuffer(Buffer, std::max(QueryBufferSize, 16384)), WriteBuffer(Buffer, std::max(QueryBufferSize, 16384)) { abort(); /* STUB */ }

// 0x080e0440  query.c:123
TQueryManagerConnection::~TQueryManagerConnection() { abort(); /* STUB */ }

// 0x080e04b0  query.c:132
void TQueryManagerConnection::connect() { abort(); /* STUB */ }

// 0x080e0820  query.c:184
void TQueryManagerConnection::disconnect() { abort(); /* STUB */ }

// 0x080e0870  query.c:195
int TQueryManagerConnection::write(unsigned char * /*Buffer*/, int /*Size*/) { abort(); /* STUB */ }

// 0x080e0900  query.c:217
int TQueryManagerConnection::read(unsigned char * /*Buffer*/, int /*Size*/, int /*Timeout*/) { abort(); /* STUB */ }

// 0x080e09e0  query.c:254
void TQueryManagerConnection::prepareQuery(int /*QueryType*/) { abort(); /* STUB */ }

// 0x080e0a60  query.c:270
void TQueryManagerConnection::sendFlag(bool /*f*/) { abort(); /* STUB */ }

// 0x080e0a80  query.c:278
void TQueryManagerConnection::sendByte(unsigned char /*b*/) { abort(); /* STUB */ }

// 0x080e0af0  query.c:291
void TQueryManagerConnection::sendWord(unsigned short /*w*/) { abort(); /* STUB */ }

// 0x080e0b60  query.c:304
void TQueryManagerConnection::sendQuad(uint32_t /*q*/) { abort(); /* STUB */ }

// 0x080e0bd0  query.c:316
void TQueryManagerConnection::sendString(char * /*s*/) { abort(); /* STUB */ }

// 0x080e0c40  query.c:329
void TQueryManagerConnection::sendBytes(unsigned char * /*Buffer*/, int /*Count*/) { abort(); /* STUB */ }

// 0x080e0cb0  query.c:342
bool TQueryManagerConnection::getFlag() { abort(); /* STUB */ }

// 0x080e0d10  query.c:350
unsigned char TQueryManagerConnection::getByte() { abort(); /* STUB */ }

// 0x080e0d70  query.c:363
unsigned short TQueryManagerConnection::getWord() { abort(); /* STUB */ }

// 0x080e0dd0  query.c:376
uint32_t TQueryManagerConnection::getQuad() { abort(); /* STUB */ }

// 0x080e0e30  query.c:389
void TQueryManagerConnection::getString(char * /*Buffer*/, int /*MaxLength*/) { abort(); /* STUB */ }

// 0x080e0ea0  query.c:402
void TQueryManagerConnection::getBytes(unsigned char * /*Buffer*/, int /*Count*/) { abort(); /* STUB */ }

// 0x080e0f10  query.c:416
int TQueryManagerConnection::executeQuery(int /*Timeout*/, bool /*AutoReconnect*/) { abort(); /* STUB */ }

// 0x080e1440  query.c:505
int TQueryManagerConnection::checkAccountPassword(uint32_t /*AccountID*/, char * /*Password*/, char * /*IPAddress*/) { abort(); /* STUB */ }

// 0x080e1610  query.c:531
int TQueryManagerConnection::loginAdmin(uint32_t /*AccountID*/, bool /*PrivateWorld*/, int & /*NumberOfCharacters*/, char (* /*Characters*/)[30], char (* /*Worlds*/)[30], unsigned char (* /*IPAddresses*/)[4], unsigned short * /*Ports*/, unsigned short & /*PremiumDaysLeft*/) { abort(); /* STUB */ }

// 0x080e1950  query.c:561
int TQueryManagerConnection::loadWorldConfig(int & /*WorldType*/, int & /*RebootTime*/, int * /*IPAddress*/, int & /*Port*/, int & /*MaxPlayers*/, int & /*PremiumPlayerBuffer*/, int & /*MaxNewbies*/, int & /*PremiumNewbieBuffer*/) { abort(); /* STUB */ }

// 0x080e1c10  query.c:588
int TQueryManagerConnection::loginGame(uint32_t /*AccountID*/, char * /*PlayerName*/, char * /*Password*/, char * /*IPAddress*/, bool /*PrivateWorld*/, bool /*PremiumAccountRequired*/, bool /*GamemasterRequired*/, uint32_t & /*CharacterID*/, int & /*Sex*/, char * /*Guild*/, char * /*Rank*/, char * /*Title*/, int & /*NumberOfBuddies*/, uint32_t * /*BuddyIDs*/, char (* /*BuddyNames*/)[30], unsigned char * /*Rights*/, bool & /*PremiumAccountActivated*/) { abort(); /* STUB */ }

// 0x080e3010  query.c:760
int TQueryManagerConnection::logoutGame(uint32_t /*CharacterID*/, int /*Level*/, char * /*Profession*/, char * /*Residence*/, time_t /*LastLoginTime*/, int /*TutorActivities*/) { abort(); /* STUB */ }

// 0x080e3240  query.c:777
int TQueryManagerConnection::setNotation(uint32_t /*GamemasterID*/, char * /*PlayerName*/, char * /*IPAddress*/, char * /*Reason*/, char * /*Comment*/, uint32_t & /*BanishmentID*/) { abort(); /* STUB */ }

// 0x080e34f0  query.c:806
int TQueryManagerConnection::setNamelock(uint32_t /*GamemasterID*/, char * /*PlayerName*/, char * /*IPAddress*/, char * /*Reason*/, char * /*Comment*/) { abort(); /* STUB */ }

// 0x080e3760  query.c:834
int TQueryManagerConnection::banishAccount(uint32_t /*GamemasterID*/, char * /*PlayerName*/, char * /*IPAddress*/, char * /*Reason*/, char * /*Comment*/, bool & /*FinalWarning*/, int & /*Days*/, uint32_t & /*BanishmentID*/) { abort(); /* STUB */ }

// 0x080e3b00  query.c:867
int TQueryManagerConnection::reportStatement(uint32_t /*ReporterID*/, char * /*PlayerName*/, char * /*Reason*/, char * /*Comment*/, uint32_t /*BanishmentID*/, uint32_t /*StatementID*/, int /*NumberOfStatements*/, uint32_t * /*StatementIDs*/, uint32_t * /*TimeStamps*/, uint32_t * /*CharacterIDs*/, char (* /*Channels*/)[30], char (* /*Texts*/)[256]) { abort(); /* STUB */ }

// 0x080e3fc0  query.c:908
int TQueryManagerConnection::banishIPAddress(uint32_t /*GamemasterID*/, char * /*PlayerName*/, char * /*IPAddress*/, char * /*Reason*/, char * /*Comment*/) { abort(); /* STUB */ }

// 0x080e4230  query.c:936
int TQueryManagerConnection::logCharacterDeath(uint32_t /*CharacterID*/, int /*Level*/, uint32_t /*Offender*/, char * /*Remark*/, bool /*Unjustified*/, time_t /*Time*/) { abort(); /* STUB */ }

// 0x080e4480  query.c:956
int TQueryManagerConnection::addBuddy(uint32_t /*AccountID*/, uint32_t /*Buddy*/) { abort(); /* STUB */ }

// 0x080e45b0  query.c:972
int TQueryManagerConnection::removeBuddy(uint32_t /*AccountID*/, uint32_t /*Buddy*/) { abort(); /* STUB */ }

// 0x080e46e0  query.c:988
int TQueryManagerConnection::decrementIsOnline(uint32_t /*CharacterID*/) { abort(); /* STUB */ }

// 0x080e47e0  query.c:1004
int TQueryManagerConnection::finishAuctions(int & /*NumberOfAuctions*/, unsigned short * /*HouseIDs*/, uint32_t * /*CharacterIDs*/, char (* /*CharacterNames*/)[30], int * /*Bids*/) { abort(); /* STUB */ }

// 0x080e4a30  query.c:1030
int TQueryManagerConnection::excludeFromAuctions(uint32_t /*CharacterID*/, bool /*Banish*/) { abort(); /* STUB */ }

// 0x080e4b40  query.c:1047
int TQueryManagerConnection::transferHouses(int & /*NumberOfTransfers*/, unsigned short * /*HouseIDs*/, uint32_t * /*NewOwnerIDs*/, char (* /*NewOwnerNames*/)[30], int * /*Prices*/) { abort(); /* STUB */ }

// 0x080e4d90  query.c:1073
int TQueryManagerConnection::cancelHouseTransfer(unsigned short /*HouseID*/) { abort(); /* STUB */ }

// 0x080e4e90  query.c:1089
int TQueryManagerConnection::evictFreeAccounts(int & /*NumberOfEvictions*/, unsigned short * /*HouseIDs*/, uint32_t * /*OwnerIDs*/) { abort(); /* STUB */ }

// 0x080e5030  query.c:1114
int TQueryManagerConnection::evictDeletedCharacters(int & /*NumberOfEvictions*/, unsigned short * /*HouseIDs*/) { abort(); /* STUB */ }

// 0x080e51a0  query.c:1135
int TQueryManagerConnection::evictExGuildleaders(int /*NumberOfGuildhouses*/, int & /*NumberOfEvictions*/, unsigned short * /*HouseIDs*/, uint32_t * /*Guildleaders*/) { abort(); /* STUB */ }

// 0x080e53e0  query.c:1157
int TQueryManagerConnection::insertHouseOwner(unsigned short /*HouseID*/, uint32_t /*OwnerID*/, int /*PaidUntil*/) { abort(); /* STUB */ }

// 0x080e5550  query.c:1174
int TQueryManagerConnection::updateHouseOwner(unsigned short /*HouseID*/, uint32_t /*OwnerID*/, int /*PaidUntil*/) { abort(); /* STUB */ }

// 0x080e56c0  query.c:1191
int TQueryManagerConnection::deleteHouseOwner(unsigned short /*HouseID*/) { abort(); /* STUB */ }

// 0x080e57c0  query.c:1206
int TQueryManagerConnection::getHouseOwners(int & /*NumberOfHouses*/, unsigned short * /*HouseIDs*/, uint32_t * /*OwnerIDs*/, char (* /*OwnerNames*/)[30], int * /*PaidUntils*/) { abort(); /* STUB */ }

// 0x080e5a10  query.c:1232
int TQueryManagerConnection::getAuctions(int & /*NumberOfAuctions*/, unsigned short * /*HouseIDs*/) { abort(); /* STUB */ }

// 0x080e5b80  query.c:1254
int TQueryManagerConnection::startAuction(unsigned short /*HouseID*/) { abort(); /* STUB */ }

// 0x080e5c80  query.c:1269
int TQueryManagerConnection::insertHouses(int /*NumberOfHouses*/, unsigned short * /*HouseIDs*/, char ** /*Names*/, int * /*Rents*/, char ** /*Descriptions*/, int * /*Sizes*/, int * /*PositionsX*/, int * /*PositionsY*/, int * /*PositionsZ*/, char (* /*Towns*/)[30], bool * /*Guildhouses*/) { abort(); /* STUB */ }

// 0x080e6080  query.c:1297
int TQueryManagerConnection::clearIsOnline(int & /*NumberOfAffectedPlayers*/) { abort(); /* STUB */ }

// 0x080e6170  query.c:1313
int TQueryManagerConnection::createPlayerlist(int /*NumberOfPlayers*/, char ** /*Names*/, int * /*Levels*/, char (* /*Professions*/)[30], bool & /*NewRecord*/) { abort(); /* STUB */ }

// 0x080e63b0  query.c:1334
int TQueryManagerConnection::logKilledCreatures(int /*NumberOfRaces*/, char ** /*Names*/, int * /*KilledPlayers*/, int * /*KilledCreatures*/) { abort(); /* STUB */ }

// 0x080e6590  query.c:1355
int TQueryManagerConnection::loadPlayers(uint32_t /*MinimumCharacterID*/, int & /*NumberOfPlayers*/, char (* /*Names*/)[30], uint32_t * /*CharacterIDs*/) { abort(); /* STUB */ }

// 0x080e6770  query.c:1376
int TQueryManagerConnection::getKeptCharacters(uint32_t /*MinimumCharacterID*/, int & /*NumberOfPlayers*/, uint32_t * /*CharacterIDs*/) { abort(); /* STUB */ }

// 0x080e6900  query.c:1394
int TQueryManagerConnection::getDeletedCharacters(uint32_t /*MinimumCharacterID*/, int & /*NumberOfPlayers*/, uint32_t * /*CharacterIDs*/) { abort(); /* STUB */ }

// 0x080e6a90  query.c:1411
int TQueryManagerConnection::deleteOldCharacter(uint32_t /*CharacterID*/) { abort(); /* STUB */ }

// 0x080e6b90  query.c:1427
int TQueryManagerConnection::getHiddenCharacters(uint32_t /*MinimumCharacterID*/, int & /*NumberOfPlayers*/, uint32_t * /*CharacterIDs*/) { abort(); /* STUB */ }

// 0x080e6d20  query.c:1444
int TQueryManagerConnection::createHighscores(int /*NumberOfPlayers*/, uint32_t * /*CharacterIDs*/, int * /*ExpPoints*/, int * /*ExpLevel*/, int * /*Fist*/, int * /*Club*/, int * /*Axe*/, int * /*Sword*/, int * /*Distance*/, int * /*Shielding*/, int * /*Magic*/, int * /*Fishing*/) { abort(); /* STUB */ }

// 0x080e7150  query.c:1472
int TQueryManagerConnection::createCensus() { abort(); /* STUB */ }

// 0x080e7200  query.c:1486
int TQueryManagerConnection::createKillStatistics() { abort(); /* STUB */ }

// 0x080e72b0  query.c:1500
int TQueryManagerConnection::getPlayersOnline(int & /*NumberOfWorlds*/, char (* /*Names*/)[30], unsigned short * /*Players*/) { abort(); /* STUB */ }

// 0x080e7450  query.c:1519
int TQueryManagerConnection::getWorlds(int & /*NumberOfWorlds*/, char (* /*Names*/)[30]) { abort(); /* STUB */ }

// 0x080e75b0  query.c:1535
int TQueryManagerConnection::getServerLoad(char * /*World*/, int /*Period*/, int * /*Data*/) { abort(); /* STUB */ }

// 0x080e7740  query.c:1556
int TQueryManagerConnection::insertPaymentDataOld(uint32_t /*PurchaseNr*/, uint32_t /*ReferenceNr*/, char * /*FirstName*/, char * /*LastName*/, char * /*Company*/, char * /*Street*/, char * /*Zip*/, char * /*City*/, char * /*Country*/, char * /*State*/, char * /*Phone*/, char * /*Fax*/, char * /*EMail*/, char * /*PaymentMethod*/, uint32_t /*ProductID*/, char * /*Registrant*/, uint32_t /*AccountID*/, uint32_t & /*PaymentID*/) { abort(); /* STUB */ }

// 0x080e7d00  query.c:1599
int TQueryManagerConnection::addPaymentOld(uint32_t /*AccountID*/, char * /*Description*/, uint32_t /*PaymentID*/, int /*Days*/, int & /*ActionTaken*/) { abort(); /* STUB */ }

// 0x080e7f70  query.c:1629
int TQueryManagerConnection::cancelPaymentOld(uint32_t /*PurchaseNr*/, uint32_t /*ReferenceNr*/, uint32_t /*AccountID*/, bool & /*IllegalUse*/, char * /*EMailAddress*/) { abort(); /* STUB */ }

// 0x080e81e0  query.c:1658
int TQueryManagerConnection::insertPaymentDataNew(uint32_t /*PurchaseNr*/, uint32_t /*ReferenceNr*/, char * /*FirstName*/, char * /*LastName*/, char * /*Company*/, char * /*Street*/, char * /*Zip*/, char * /*City*/, char * /*Country*/, char * /*State*/, char * /*Phone*/, char * /*Fax*/, char * /*EMail*/, char * /*PaymentMethod*/, uint32_t /*ProductID*/, char * /*Registrant*/, char * /*PaymentKey*/, uint32_t & /*PaymentID*/) { abort(); /* STUB */ }

// 0x080e87a0  query.c:1702
int TQueryManagerConnection::addPaymentNew(char * /*PaymentKey*/, uint32_t /*PaymentID*/, int & /*ActionTaken*/, char * /*EMailReceiver*/) { abort(); /* STUB */ }

// 0x080e89e0  query.c:1731
int TQueryManagerConnection::cancelPaymentNew(uint32_t /*PurchaseNr*/, uint32_t /*ReferenceNr*/, char * /*PaymentKey*/, bool & /*IllegalUse*/, bool & /*Present*/, char * /*EMailAddress*/) { abort(); /* STUB */ }

// 0x080e8d20  query.c:1761
TQueryManagerConnectionPool::TQueryManagerConnectionPool(int Connections) : FreeQueryManagerConnections(Connections), QueryManagerConnectionMutex(1) { abort(); /* STUB */ }

// 0x080e8dc0  query.c:1778
void TQueryManagerConnectionPool::init() { abort(); /* STUB */ }

// 0x080e8f10  query.c:1796
void TQueryManagerConnectionPool::exit() { abort(); /* STUB */ }

// 0x080e8f90  query.c:1808
TQueryManagerConnection *TQueryManagerConnectionPool::getConnection() { abort(); /* STUB */ }

// 0x080e9000  query.c:1825
void TQueryManagerConnectionPool::releaseConnection(TQueryManagerConnection * /*Connection*/) { abort(); /* STUB */ }

// 0x080e90f0  query.c:1843
TQueryManagerPoolConnection::TQueryManagerPoolConnection(TQueryManagerConnectionPool * /*Pool*/) { abort(); /* STUB */ }

// 0x080e9220  query.c:1857
TQueryManagerPoolConnection::~TQueryManagerPoolConnection() { abort(); /* STUB */ }
