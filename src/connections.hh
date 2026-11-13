#ifndef PATELLA_CONNECTIONS_HH
#define PATELLA_CONNECTIONS_HH

#include <cstdint>
#include <cstdlib>
#include <sys/types.h>

#include "TXTEASymmetricKey.hh"

struct Semaphore;
struct TConnection;
struct TPlayer;
struct TReadBuffer;

enum KNOWNCREATURESTATE {
	KNOWNCREATURE_FREE = 0,
	KNOWNCREATURE_UPTODATE = 1,
	KNOWNCREATURE_OUTDATED = 2,
};

struct TKnownCreature {
	KNOWNCREATURESTATE State;
	uint32_t CreatureID;
	TKnownCreature *Next;
	TConnection *Connection;
};

enum CONNECTIONSTATE {
	CONNECTION_FREE = 0,
	CONNECTION_ASSIGNED = 1,
	CONNECTION_CONNECTED = 2,
	CONNECTION_LOGIN = 3,
	CONNECTION_GAME = 4,
	CONNECTION_DEAD = 5,
	CONNECTION_LOGOUT = 6,
	CONNECTION_DISCONNECTED = 7,
};

struct TConnection {
	TConnection();
	void Process();
	void ResetTimer(int Command);
	void EmergencyPing();
	// inline  connections.h:56
	void SetSymmetricKey(unsigned char * /*_SymmetricKey*/) { abort(); /* STUB */ }
	TXTEASymmetricKey &GetSymmetricKey();
	// inline  connections.h:60
	CONNECTIONSTATE GetState() { abort(); /* STUB */ }
	// inline  connections.h:61
	bool IsOnline() { abort(); /* STUB */ }
	pid_t GetPID();
	int GetSocket();
	char *GetIPAddress();
	bool ConnectionOk();
	bool ClosingDelayed();
	void Free();
	void Assign();
	void Connect(int _Socket);
	void Login();
	bool JoinGame(TReadBuffer &Buffer);
	void EnterGame();
	void Die();
	void Logout(int Delay, bool StopFight);
	void Close(bool Delay);
	void Disconnect();
	// inline  connections.h:81
	int GetTerminalType() { abort(); /* STUB */ }
	// inline  connections.h:82
	int GetTerminalVersion() { abort(); /* STUB */ }
	int GetTerminalOffsetX();
	int GetTerminalOffsetY();
	// inline  connections.h:85
	int GetTerminalWidth() { abort(); /* STUB */ }
	// inline  connections.h:86
	int GetTerminalHeight() { abort(); /* STUB */ }
	unsigned char InData[2048];
	int InDataSize;
	bool SigIOPending;
	bool WaitingForACK;
	unsigned char OutData[16384];
	int NextToSend;
	int NextToCommit;
	int NextToWrite;
	bool Overflow;
	bool WillingToSend;
	TConnection *NextSendingConnection;
	unsigned int RandomSeed;
	TPlayer *GetPlayer();
	const char *GetName();
	void GetPosition(int &x, int &y, int &z);
	bool IsVisible(int x, int y, int z);
	KNOWNCREATURESTATE KnownCreature(uint32_t ID, bool UpdateFollows);
	uint32_t NewKnownCreature(uint32_t NewID);
private:
	void ClearKnownCreatureTable(bool Unchain);
	void UnchainKnownCreature(uint32_t ID);
	CONNECTIONSTATE State;
	pid_t PID;
	int Socket;
	char IPAddress[16];
	TXTEASymmetricKey SymmetricKey;
	bool ConnectionIsOk;
	bool ClosingIsDelayed;
	uint32_t TimeStamp;
	uint32_t TimeStampAction;
	int TerminalType;
	int TerminalVersion;
	int TerminalOffsetX;
	int TerminalOffsetY;
	int TerminalWidth;
	int TerminalHeight;
	uint32_t CharacterID;
	char Name[31];
	TKnownCreature KnownCreatureTable[150];
};

extern TConnection *FirstSendingConnection;

TConnection *AssignFreeConnection();
TConnection *GetFirstConnection();
TConnection *GetNextConnection();
void ProcessConnections();
void InitConnections();
void ExitConnections();

extern TConnection Connections[1100];
extern Semaphore ConnectionMutex;
extern int ConnectionIterator;

#endif
