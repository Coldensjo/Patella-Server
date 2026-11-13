#include <cstdint>
#include <cstdlib>
#include <sys/types.h>

#include "config.hh"
#include "script.hh"
#include "threads.hh"
#include "dbfuncs.hh"
#include "objects.hh"
#include "map.hh"
#include "info.hh"
#include "cr.hh"
#include "containr.hh"
#include "operate.hh"
#include "connections.hh"
#include "utils.hh"
#include "time.hh"
#include "communication.hh"
#include "TXTEASymmetricKey.hh"

const int PINGTIME = 30;
const int IDLETIME = 900;

TConnection Connections[1100];	// connections.c:32
TConnection *FirstSendingConnection;	// connections.c:33
Semaphore ConnectionMutex(1);	// connections.c:34
int ConnectionIterator;	// connections.c:515

// 0x080cad10  connections.c:45
TConnection::TConnection() { abort(); /* STUB */ }

// 0x080cad40  connections.c:53
void TConnection::Process() { abort(); /* STUB */ }

// 0x080cb0c0  connections.c:75
void TConnection::ResetTimer(int /*Command*/) { abort(); /* STUB */ }

// 0x080cb120  connections.c:85
void TConnection::EmergencyPing() { abort(); /* STUB */ }

// 0x080cb170  connections.c:95
pid_t TConnection::GetPID() { abort(); /* STUB */ }

// 0x080cb1a0  connections.c:107
int TConnection::GetSocket() { abort(); /* STUB */ }

// 0x080cb1e0  connections.c:119
char *TConnection::GetIPAddress() { abort(); /* STUB */ }

// 0x080cb220  connections.c:131
void TConnection::Free() { abort(); /* STUB */ }

// 0x080cb230  connections.c:139
void TConnection::Assign() { abort(); /* STUB */ }

// 0x080cb280  connections.c:150
void TConnection::Connect(int /*_Socket*/) { abort(); /* STUB */ }

// 0x080cb330  connections.c:172
void TConnection::Login() { abort(); /* STUB */ }

// 0x080cb370  connections.c:183
bool TConnection::JoinGame(TReadBuffer & /*Buffer*/) { abort(); /* STUB */ }

// 0x080cb6c0  connections.c:270
void TConnection::EnterGame() { abort(); /* STUB */ }

// 0x080cb700  connections.c:280
void TConnection::Die() { abort(); /* STUB */ }

// 0x080cb720  connections.c:290
void TConnection::Logout(int /*Delay*/, bool /*StopFight*/) { abort(); /* STUB */ }

// 0x080cb7e0  connections.c:309
void TConnection::Close(bool /*Delay*/) { abort(); /* STUB */ }

// 0x080cb850  connections.c:321
void TConnection::Disconnect() { abort(); /* STUB */ }

// 0x080cb900  connections.c:334
TPlayer *TConnection::GetPlayer() { abort(); /* STUB */ }

// 0x080cb950  connections.c:347
const char *TConnection::GetName() { abort(); /* STUB */ }

// 0x080cb990  connections.c:359
void TConnection::GetPosition(int & /*x*/, int & /*y*/, int & /*z*/) { abort(); /* STUB */ }

// 0x080cba30  connections.c:376
bool TConnection::IsVisible(int /*x*/, int /*y*/, int /*z*/) { abort(); /* STUB */ }

// 0x080cbae0  connections.c:393
KNOWNCREATURESTATE TConnection::KnownCreature(uint32_t /*ID*/, bool /*UpdateFollows*/) { abort(); /* STUB */ }

// 0x080cbb30  connections.c:409
uint32_t TConnection::NewKnownCreature(uint32_t /*NewID*/) { abort(); /* STUB */ }

// 0x080cbd40  connections.c:465
void TConnection::ClearKnownCreatureTable(bool /*Unchain*/) { abort(); /* STUB */ }

// 0x080cbdb0  connections.c:479
void TConnection::UnchainKnownCreature(uint32_t /*ID*/) { abort(); /* STUB */ }

// 0x080cbe50  connections.c:522
TConnection *AssignFreeConnection() { abort(); /* STUB */ }

// 0x080cbec0  connections.c:537
TConnection *GetFirstConnection() { abort(); /* STUB */ }

// 0x080cbf10  connections.c:546
TConnection *GetNextConnection() { abort(); /* STUB */ }

// 0x080cbf60  connections.c:558
void ProcessConnections() { abort(); /* STUB */ }

// 0x080cbfd0  connections.c:572
void InitConnections() { abort(); /* STUB */ }

// 0x080cc000  connections.c:581
void ExitConnections() { abort(); /* STUB */ }
