#include <cstdint>
#include <cstdlib>
#include <sys/types.h>

#include "threads.hh"
#include "script.hh"
#include "dbfuncs.hh"
#include "query.hh"
#include "config.hh"
#include "objects.hh"
#include "map.hh"
#include "info.hh"
#include "cr.hh"
#include "containr.hh"
#include "operate.hh"
#include "connections.hh"
#include "vlong.hh"
#include "TRSAPublicKey.hh"
#include "utils.hh"
#include "time.hh"
#include "TXTEASymmetricKey.hh"
#include "RSA_PrivateKey.hh"
#include "communication.hh"
#include "TRSAPrivateKey.hh"

struct TWaitinglistEntry {
	TWaitinglistEntry *Next;
	char Name[30];
	uint32_t NextTry;
	bool FreeAccount;
	bool Newbie;
	bool Sleeping;
};

[[maybe_unused]] static int TERMINALVERSION[3] = {
	770, 770, 770,
};	// communication.c:39
int TCPSocket;	// communication.c:52
Thread AcceptorThread;	// communication.c:53
pid_t AcceptorThreadPID;	// communication.c:54
int ActiveConnections;	// communication.c:55
Semaphore CommunicationThreadMutex(1);	// communication.c:56
bool UseOwnStacks;	// communication.c:57
unsigned char CommunicationThreadStacks[72089600];	// communication.c:58
pid_t LastUsingCommunicationThread[1100];	// communication.c:59
int FreeCommunicationThreadStacks[1100];	// communication.c:60
int NumberOfFreeCommunicationThreadStacks;	// communication.c:61
TRSAPrivateKey PrivateKey;	// communication.c:62
Semaphore RSAMutex(1);	// communication.c:63
TQueryManagerConnectionPool QueryManagerConnectionPool(10);	// communication.c:66
int LoadHistory[360];	// communication.c:69
int LoadHistoryPointer;	// communication.c:70
int TotalLoad;	// communication.c:71
int TotalSend;	// communication.c:71
int TotalRecv;	// communication.c:71
uint32_t LagEnd;	// communication.c:72
uint32_t EarliestFreeAccountAdmissionRound;	// communication.c:73
store<TWaitinglistEntry, 100> Waitinglist;	// communication.c:76
TWaitinglistEntry *WaitinglistHead;	// communication.c:77

// 0x080dbbd0  communication.c:88
void InitCommunicationThreadStacks() { abort(); /* STUB */ }

// 0x080dbc30  communication.c:104
void GetCommunicationThreadStack(int & /*StackNumber*/, void *& /*Stack*/) { abort(); /* STUB */ }

// 0x080dbd10  communication.c:131
void AttachCommunicationThreadStack(int /*StackNumber*/) { abort(); /* STUB */ }

// 0x080dbd30  communication.c:139
void ReleaseCommunicationThreadStack(int /*StackNumber*/) { abort(); /* STUB */ }

// 0x080dbd70  communication.c:150
void ExitCommunicationThreadStacks() { abort(); /* STUB */ }

// 0x080dbe10  communication.c:175
void InitLoadHistory() { abort(); /* STUB */ }

// 0x080dbe70  communication.c:190
bool LagDetected() { abort(); /* STUB */ }

// 0x080dbe90  communication.c:198
void NetLoad(int /*Amount*/, bool /*Send*/) { abort(); /* STUB */ }

// 0x080dbee0  communication.c:209
void NetLoadSummary() { abort(); /* STUB */ }

// 0x080dbf50  communication.c:223
// local static: int LastRecv = 0  (communication.c:224, 0x081283cc .data)
void NetLoadCheck() { abort(); /* STUB */ }

// 0x080dc0c0  communication.c:270
bool WriteToSocket(TConnection * /*Connection*/, unsigned char * /*Buffer*/, int /*Size*/) { abort(); /* STUB */ }

// 0x080dc280  communication.c:311
bool SendLoginMessage(TConnection * /*Connection*/, int /*Type*/, char * /*Message*/, int /*WaitingTime*/) { abort(); /* STUB */ }

// 0x080dc430  communication.c:353
bool SendData(TConnection * /*Connection*/) { abort(); /* STUB */ }

// 0x080dc580  communication.c:385
bool SendData(TConnection * /*Connection*/, char * /*Data*/, int /*Size*/) { abort(); /* STUB */ }

// 0x080dc610  communication.c:410
bool GetWaitinglistEntry(char * /*Name*/, uint32_t & /*NextTry*/, bool & /*FreeAccount*/, bool & /*Newbie*/) { abort(); /* STUB */ }

// 0x080dc6b0  communication.c:448
void InsertWaitinglistEntry(char * /*Name*/, uint32_t /*NextTry*/, bool /*FreeAccount*/, bool /*Newbie*/) { abort(); /* STUB */ }

// 0x080dc7b0  communication.c:476
void DeleteWaitinglistEntry(char * /*Name*/) { abort(); /* STUB */ }

// 0x080dc890  communication.c:498
int GetWaitinglistPosition(char * /*Name*/, bool /*FreeAccount*/, bool /*Newbie*/) { abort(); /* STUB */ }

// 0x080dca60  communication.c:533
int CheckWaitingTime(char * /*Name*/, TConnection * /*Connection*/, bool /*FreeAccount*/, bool /*Newbie*/) { abort(); /* STUB */ }

// 0x080dccf0  communication.c:588
int ReadFromSocket(TConnection * /*Connection*/, unsigned char * /*Buffer*/, int /*Size*/) { abort(); /* STUB */ }

// 0x080dcd90  communication.c:615
bool CallGameThread(TConnection * /*Connection*/) { abort(); /* STUB */ }

// 0x080dce10  communication.c:631
bool CheckConnection(TConnection * /*Connection*/) { abort(); /* STUB */ }

// 0x080dce70  communication.c:643
TPlayerData *PerformRegistration(TConnection * /*Connection*/, char * /*PlayerName*/, uint32_t /*AccountID*/, char * /*PlayerPassword*/, bool /*GamemasterClient*/) { abort(); /* STUB */ }

// 0x080dd5b0  communication.c:800
bool HandleLogin(TConnection * /*Connection*/) { abort(); /* STUB */ }

// 0x080dde60  communication.c:985
bool ReceiveCommand(TConnection * /*Connection*/) { abort(); /* STUB */ }

// 0x080de220  communication.c:1068
void IncrementActiveConnections() { abort(); /* STUB */ }

// 0x080de250  communication.c:1078
void DecrementActiveConnections() { abort(); /* STUB */ }

// 0x080de280  communication.c:1089
void CommunicationThread(int /*Socket*/) { abort(); /* STUB */ }

// 0x080de550  communication.c:1165
int HandleConnection(void * /*Data*/) { abort(); /* STUB */ }

// 0x080de690  communication.c:1197
bool OpenSocket() { abort(); /* STUB */ }

// 0x080de8b0  communication.c:1245
int AcceptorThreadLoop(void *) { abort(); /* STUB */ }

// 0x080dec20  communication.c:1354
void CheckThreadlibVersion() { abort(); /* STUB */ }

// 0x080dec60  communication.c:1364
void InitCommunication() { abort(); /* STUB */ }

// 0x080dee30  communication.c:1384
void ExitCommunication() { abort(); /* STUB */ }
