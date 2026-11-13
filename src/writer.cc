#include <cstdint>
#include <cstdlib>
#include <time.h>

#include "script.hh"
#include "config.hh"
#include "threads.hh"
#include "dbfuncs.hh"
#include "query.hh"
#include "objects.hh"
#include "map.hh"
#include "info.hh"
#include "cr.hh"
#include "containr.hh"
#include "operate.hh"
#include "connections.hh"
#include "utils.hh"
#include "time.hh"
#include "TXTEASymmetricKey.hh"
#include "writer.hh"

const int PROTOCOL_TEXTLENGTH = 256;

struct TProtocolThreadOrder {
	char ProtocolName[20];
	char Text[256];
};

enum TWriterThreadOrderType {
	ORDER_TERMINATE = 0,
	ORDER_LOGOUT = 1,
	ORDER_PLAYERLIST = 2,
	ORDER_KILLSTATISTICS = 3,
	ORDER_PUNISHMENT = 4,
	ORDER_CHARACTERDEATH = 5,
	ORDER_ADDBUDDY = 6,
	ORDER_REMOVEBUDDY = 7,
	ORDER_DECREMENTISONLINE = 8,
	ORDER_SAVEPLAYERDATA = 9,
};

struct TWriterThreadOrder {
	TWriterThreadOrderType OrderType;
	void *Data;
};

struct TLogoutOrderData {
	uint32_t CharacterID;
	int Level;
	int Profession;
	time_t LastLoginTime;
	int TutorActivities;
	char Residence[30];
};

struct TPlayerlistOrderData {
	int NumberOfPlayers;
	char *PlayerNames;
	int *PlayerLevels;
	int *PlayerProfessions;
};

struct TKillStatisticsOrderData {
	int NumberOfRaces;
	char *RaceNames;
	int *KilledPlayers;
	int *KilledCreatures;
};

struct TNotationOrderData {
	uint32_t GamemasterID;
	char GamemasterName[30];
	char CharacterName[30];
	char Comment[200];
};

struct TNameLockOrderData {
	uint32_t GamemasterID;
	char GamemasterName[30];
	char CharacterName[30];
	bool OldNameVisible;
};

struct TPunishmentOrderData {
	uint32_t GamemasterID;
	char GamemasterName[30];
	char CriminalName[30];
	char CriminalIPAddress[16];
	int Reason;
	int Action;
	char Comment[200];
	int NumberOfStatements;
	vector<TReportedStatement> *ReportedStatements;
	uint32_t StatementID;
	bool IPBanishment;
};

struct TCharacterDeathOrderData {
	uint32_t CharacterID;
	int Level;
	uint32_t Offender;
	char Remark[30];
	bool Unjustified;
	time_t Time;
};

struct TBuddyOrderData {
	uint32_t AccountID;
	uint32_t Buddy;
};

enum TWriterThreadReplyType {
	REPLY_BROADCAST = 0,
	REPLY_DIRECT = 1,
	REPLY_LOGOUT = 2,
};

struct TWriterThreadReply {
	TWriterThreadReplyType ReplyType;
	void *Data;
};

struct TDirectReplyData {
	uint32_t CharacterID;
	char Message[100];
};

const int PROTOCOL_BUFFER_SIZE = 1000;
const int ORDER_BUFFER_SIZE = 2000;
const int REPLY_BUFFER_SIZE = 100;

// external functions that writer.hh cannot declare: they take an enum declared in this file
void InsertOrder(TWriterThreadOrderType OrderType, void *Data);
void GetOrder(TWriterThreadOrderType &OrderType, void *&Data);
void InsertReply(TWriterThreadReplyType ReplyType, void *Data);
bool GetReply(TWriterThreadReplyType &ReplyType, void *&Data);

[[maybe_unused]] static Thread ProtocolThread = 0;	// writer.c:137
[[maybe_unused]] static Thread WriterThread = 0;	// writer.c:138
[[maybe_unused]] static TQueryManagerConnection *QueryManagerConnection = nullptr;	// writer.c:139
[[maybe_unused]] static TProtocolThreadOrder ProtocolBuffer[1000];	// writer.c:144
[[maybe_unused]] static TWriterThreadOrder OrderBuffer[2000];	// writer.c:145
[[maybe_unused]] static TWriterThreadReply ReplyBuffer[100];	// writer.c:146
[[maybe_unused]] static int ProtocolPointerWrite;	// writer.c:147
[[maybe_unused]] static int ProtocolPointerRead;	// writer.c:148
[[maybe_unused]] static int OrderPointerWrite;	// writer.c:149
[[maybe_unused]] static int OrderPointerRead;	// writer.c:150
[[maybe_unused]] static int ReplyPointerWrite;	// writer.c:151
[[maybe_unused]] static int ReplyPointerRead;	// writer.c:152
[[maybe_unused]] static Semaphore ProtocolMutex(1);	// writer.c:154
[[maybe_unused]] static Semaphore ProtocolBufferEmpty(1000);	// writer.c:155
[[maybe_unused]] static Semaphore ProtocolBufferFull(0);	// writer.c:156
[[maybe_unused]] static Semaphore OrderBufferEmpty(2000);	// writer.c:157
[[maybe_unused]] static Semaphore OrderBufferFull(0);	// writer.c:158

// 0x08068b80  writer.c:170
void InitProtocol() { abort(); /* STUB */ }

// 0x08068ba0  writer.c:180
void InsertProtocolOrder(char * /*ProtocolName*/, char * /*Text*/) { abort(); /* STUB */ }

// 0x08068cd0  writer.c:206
void GetProtocolOrder(char * /*ProtocolName*/, char * /*Text*/) { abort(); /* STUB */ }

// 0x08068d80  writer.c:219
void WriteProtocol(char * /*ProtocolName*/, char * /*Text*/) { abort(); /* STUB */ }

// 0x08068e10  writer.c:233
int ProtocolThreadLoop(void *) { abort(); /* STUB */ }

// 0x08068f80  writer.c:248
void InitLog(char * /*ProtocolName*/) { abort(); /* STUB */ }

// 0x08069060  writer.c:274
void Log(char * /*ProtocolName*/, char * /*Text*/, ...) { abort(); /* STUB */ }

// 0x080692b0  writer.c:315
void InitWriterBuffers() { abort(); /* STUB */ }

// 0x080692e0  writer.c:327
int GetOrderBufferSpace() { abort(); /* STUB */ }

// 0x08069310  writer.c:337
void InsertOrder(TWriterThreadOrderType /*OrderType*/, void * /*Data*/) { abort(); /* STUB */ }

// 0x080693c0  writer.c:353
void GetOrder(TWriterThreadOrderType & /*OrderType*/, void *& /*Data*/) { abort(); /* STUB */ }

// 0x08069440  writer.c:366
void InsertReply(TWriterThreadReplyType /*ReplyType*/, void * /*Data*/) { abort(); /* STUB */ }

// 0x080694d0  writer.c:381
void DirectReply(uint32_t /*CharacterID*/, char * /*Text*/, ...) { abort(); /* STUB */ }

// 0x080695a0  writer.c:402
void LogoutReply(char * /*PlayerName*/) { abort(); /* STUB */ }

// 0x08069640  writer.c:416
bool GetReply(TWriterThreadReplyType & /*ReplyType*/, void *& /*Data*/) { abort(); /* STUB */ }

// 0x080696b0  writer.c:435
void TerminateWriterOrder() { abort(); /* STUB */ }

// 0x08069740  writer.c:444
void LogoutOrder(TPlayer * /*Player*/) { abort(); /* STUB */ }

// 0x08069a00  writer.c:483
void PlayerlistOrder(int /*NumberOfPlayers*/, char * /*PlayerNames*/, int * /*PlayerLevels*/, int * /*PlayerProfessions*/) { abort(); /* STUB */ }

// 0x08069b20  writer.c:510
void KillStatisticsOrder(int /*NumberOfRaces*/, char * /*RaceNames*/, int * /*KilledPlayers*/, int * /*KilledCreatures*/) { abort(); /* STUB */ }

// 0x08069c40  writer.c:538
void PunishmentOrder(TCreature * /*cr*/, char * /*Name*/, char * /*IPAddress*/, int /*Reason*/, int /*Action*/, char * /*Comment*/, int /*NumberOfStatements*/, vector<TReportedStatement> * /*ReportedStatements*/, uint32_t /*StatementID*/, bool /*IPBanishment*/) { abort(); /* STUB */ }

// 0x08069df0  writer.c:569
void CharacterDeathOrder(TCreature * /*cr*/, int /*OldLevel*/, uint32_t /*Offender*/, char * /*Remark*/, bool /*Unjustified*/) { abort(); /* STUB */ }

// 0x08069f20  writer.c:594
void AddBuddyOrder(TCreature * /*cr*/, uint32_t /*Buddy*/) { abort(); /* STUB */ }

// 0x0806a030  writer.c:615
void RemoveBuddyOrder(TCreature * /*cr*/, uint32_t /*Buddy*/) { abort(); /* STUB */ }

// 0x0806a140  writer.c:636
void DecrementIsOnlineOrder(uint32_t /*CharacterID*/) { abort(); /* STUB */ }

// 0x0806a1e0  writer.c:645
void SavePlayerDataOrder() { abort(); /* STUB */ }

// 0x0806a270  writer.c:659
void ProcessLogoutOrder(void * /*RawData*/) { abort(); /* STUB */ }

// 0x0806a340  writer.c:680
void ProcessPlayerlistOrder(void * /*RawData*/) { abort(); /* STUB */ }

// 0x0806a560  writer.c:730
void ProcessKillStatisticsOrder(void * /*RawData*/) { abort(); /* STUB */ }

// 0x0806a670  writer.c:766
void ProcessPunishmentOrder(void * /*RawData*/) { abort(); /* STUB */ }

// 0x0806b150  writer.c:911
void ProcessCharacterDeathOrder(void * /*RawData*/) { abort(); /* STUB */ }

// 0x0806b1e0  writer.c:928
void ProcessAddBuddyOrder(void * /*RawData*/) { abort(); /* STUB */ }

// 0x0806b250  writer.c:945
void ProcessRemoveBuddyOrder(void * /*RawData*/) { abort(); /* STUB */ }

// 0x0806b2c0  writer.c:962
void ProcessDecrementIsOnlineOrder(void * /*RawData*/) { abort(); /* STUB */ }

// 0x0806b300  writer.c:973
int WriterThreadLoop(void *) { abort(); /* STUB */ }

// 0x0806b570  writer.c:1011
void ProcessBroadcastReply(void * /*RawData*/) { abort(); /* STUB */ }

// 0x0806b5c0  writer.c:1027
void ProcessDirectReply(void * /*RawData*/) { abort(); /* STUB */ }

// 0x0806b630  writer.c:1044
void ProcessLogoutReply(void * /*RawData*/) { abort(); /* STUB */ }

// 0x0806b6b0  writer.c:1064
void ProcessWriterThreadReplies() { abort(); /* STUB */ }

// 0x0806b7e0  writer.c:1088
void ClearPlayers() { abort(); /* STUB */ }

// 0x0806b830  writer.c:1103
void InitWriter() { abort(); /* STUB */ }

// 0x0806ba00  writer.c:1121
void AbortWriter() { abort(); /* STUB */ }

// 0x0806ba40  writer.c:1133
void ExitWriter() { abort(); /* STUB */ }
