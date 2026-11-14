// connections.hh: TConnection, one client's connection (its state from
// assignment through login, game and logout to disconnection, its buffers,
// its timers and the creatures its client knows), and the connection table.

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

// The size of the connection table (Connections), of a connection's
// KnownCreatureTable and of its OutData, the ring buffer SendData
// (communication.cc) sends from.
const int MAX_CONNECTIONS = 1100;
const int MAX_KNOWN_CREATURES = 150;
const int OUT_DATA_SIZE = 16384;

struct TConnection {
	TConnection();
	void Process();
	void ResetTimer(int Command);
	void EmergencyPing();
	void SetSymmetricKey(unsigned char *_SymmetricKey) { SymmetricKey.init(_SymmetricKey); }
	TXTEASymmetricKey &GetSymmetricKey() { return SymmetricKey; }
	CONNECTIONSTATE GetState() { return State; }
	bool IsOnline() { return State >= CONNECTION_LOGIN && State <= CONNECTION_LOGOUT; }
	pid_t GetPID();
	int GetSocket();
	char *GetIPAddress();
	bool IsOk() { return ConnectionIsOk; }
	bool ClosingDelayed() { return ClosingIsDelayed; }
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
	int GetTerminalType() { return TerminalType; }
	int GetTerminalVersion() { return TerminalVersion; }
	int GetTerminalOffsetX() { return TerminalOffsetX; }
	int GetTerminalOffsetY() { return TerminalOffsetY; }
	int GetTerminalWidth() { return TerminalWidth; }
	int GetTerminalHeight() { return TerminalHeight; }
	unsigned char InData[2048];
	int InDataSize;
	bool SigIOPending;
	bool WaitingForACK;
	unsigned char OutData[OUT_DATA_SIZE];
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
	TKnownCreature KnownCreatureTable[MAX_KNOWN_CREATURES];
};

extern TConnection *FirstSendingConnection;

TConnection *AssignFreeConnection();
TConnection *GetFirstConnection();
TConnection *GetNextConnection();
void ProcessConnections();
void InitConnections();

extern TConnection Connections[MAX_CONNECTIONS];
extern Semaphore ConnectionMutex;
extern int ConnectionIterator;

#endif
