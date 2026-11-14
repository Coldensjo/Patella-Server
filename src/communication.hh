// communication.hh: the acceptor and communication threads, their stacks, the network load and the waiting list
// (communication.cc).

#ifndef PATELLA_COMMUNICATION_HH
#define PATELLA_COMMUNICATION_HH

#include <cstdint>
#include <sys/types.h>

#include "threads.hh"
#include "containr.hh"

struct TConnection;
struct TPlayerData;
struct TQueryManagerConnectionPool;
struct TRSAPrivateKey;
struct TWaitinglistEntry;

const int TERMINALTYPES = 3;

void InitCommunicationThreadStacks();
void GetCommunicationThreadStack(int &StackNumber, void *&Stack);
void AttachCommunicationThreadStack(int StackNumber);
void ReleaseCommunicationThreadStack(int StackNumber);
void ExitCommunicationThreadStacks();
void InitLoadHistory();
bool LagDetected();
void NetLoad(int Amount, bool Send);
void NetLoadSummary();
void NetLoadCheck();
bool WriteToSocket(TConnection *Connection, unsigned char *Buffer, int Size);
bool SendLoginMessage(TConnection *Connection, int Type, char *Message, int WaitingTime);
bool SendData(TConnection *Connection);
bool GetWaitinglistEntry(char *Name, uint32_t &NextTry, bool &FreeAccount, bool &Newbie);
void InsertWaitinglistEntry(char *Name, uint32_t NextTry, bool FreeAccount, bool Newbie);
void DeleteWaitinglistEntry(char *Name);
int GetWaitinglistPosition(char *Name, bool FreeAccount, bool Newbie);
int CheckWaitingTime(char *Name, TConnection *Connection, bool FreeAccount, bool Newbie);
int ReadFromSocket(TConnection *Connection, unsigned char *Buffer, int Size);
bool CallGameThread(TConnection *Connection);
bool CheckConnection(TConnection *Connection);
TPlayerData *PerformRegistration(TConnection *Connection, char *PlayerName, uint32_t AccountID, char *PlayerPassword, bool GamemasterClient);
bool HandleLogin(TConnection *Connection);
bool ReceiveCommand(TConnection *Connection);
void IncrementActiveConnections();
void DecrementActiveConnections();
void CommunicationThread(int Socket);
int HandleConnection(void *Data);
bool OpenSocket();
int AcceptorThreadLoop(void *);
void ChooseThreadStacks();
// The communication threads that can run at once, one for each connection:
// the number of own stacks, and the limit of ActiveConnections without them.
const int MAX_COMMUNICATION_THREADS = 1100;

// The stack of one communication thread, an own one or the thread library's.
const int COMMUNICATION_THREAD_STACK_SIZE = 65536;

// The entries of LoadHistory, the received bytes per player of the last
// NetLoadCheck calls.
const int LOAD_HISTORY_SIZE = 360;

void InitCommunication();
void ExitCommunication();

extern int TCPSocket;
extern Thread AcceptorThread;
extern pid_t AcceptorThreadPID;
extern int ActiveConnections;
extern Semaphore CommunicationThreadMutex;
extern bool UseOwnStacks;
extern unsigned char CommunicationThreadStacks[MAX_COMMUNICATION_THREADS * COMMUNICATION_THREAD_STACK_SIZE];
extern pid_t LastUsingCommunicationThread[MAX_COMMUNICATION_THREADS];
extern int FreeCommunicationThreadStacks[MAX_COMMUNICATION_THREADS];
extern int NumberOfFreeCommunicationThreadStacks;
extern TRSAPrivateKey PrivateKey;
extern Semaphore RSAMutex;
extern TQueryManagerConnectionPool QueryManagerConnectionPool;
extern int LoadHistory[LOAD_HISTORY_SIZE];
extern int LoadHistoryPointer;
extern int TotalLoad;
extern int TotalSend;
extern int TotalRecv;
extern uint32_t LagEnd;
extern uint32_t EarliestFreeAccountAdmissionRound;
extern store<TWaitinglistEntry, 100> Waitinglist;
extern TWaitinglistEntry *WaitinglistHead;

#endif
