#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <sys/types.h>

#include "script.hh"
#include "config.hh"
#include "threads.hh"
#include "dbfuncs.hh"
#include "objects.hh"
#include "map.hh"
#include "info.hh"
#include "cr.hh"
#include "containr.hh"
#include "operate.hh"
#include "utils.hh"
#include "time.hh"
#include "shm.hh"

enum GAMESTATE {
	GAME_STARTING = 0,
	GAME_RUNNING = 1,
	GAME_CLOSING = 2,
	GAME_ENDING = 3,
};

const int PRINTBUFFERENTRIES = 200;

struct TSharedMemory {
	int Command;
	char CommandBuffer[256];
	uint32_t RoundNr;
	uint32_t ObjectCounter;
	uint32_t Errors;
	int PlayersOnline;
	int NewbiesOnline;
	int PrintBufferPosition;
	char PrintBuffer[200][128];
	GAMESTATE GameState;
	int32_t GameThreadPID;
};
static_assert(sizeof(TSharedMemory) == 25892, "TSharedMemory is mapped by the operator tools; keep the 32-bit layout");
static_assert(offsetof(TSharedMemory, RoundNr) == 260, "TSharedMemory is mapped by the operator tools; keep the 32-bit layout");

bool IsGameServer = false;	// shm.c:46
bool VerboseOutput = false;	// shm.c:47
TSharedMemory *SHM = nullptr;	// shm.c:48
Semaphore LogfileMutex(1);	// shm.c:49

// 0x08056670  shm.c:61
void StartGame() { abort(); /* STUB */ }

// 0x080566b0  shm.c:74
void CloseGame() { abort(); /* STUB */ }

// 0x080566f0  shm.c:87
void EndGame() { abort(); /* STUB */ }

// 0x08056720  shm.c:99
bool LoginAllowed() { abort(); /* STUB */ }

// 0x08056750  shm.c:111
bool GameRunning() { abort(); /* STUB */ }

// 0x08056780  shm.c:123
bool GameStarting() { abort(); /* STUB */ }

// 0x080567c0  shm.c:135
bool GameEnding() { abort(); /* STUB */ }

// 0x08056800  shm.c:147
pid_t GetGameThreadPID() { abort(); /* STUB */ }

// 0x08056820  shm.c:162
void ErrorHandler(char * /*Text*/) { abort(); /* STUB */ }

// 0x080568b0  shm.c:179
void PrintHandler(int /*Level*/, char * /*Text*/) { abort(); /* STUB */ }

// 0x080569e0  shm.c:202
int GetPrintlogPosition() { abort(); /* STUB */ }

// 0x08056a00  shm.c:211
char *GetPrintlogLine(int /*Line*/) { abort(); /* STUB */ }

// 0x08056a30  shm.c:220
void IncrementObjectCounter() { abort(); /* STUB */ }

// 0x08056a50  shm.c:228
void DecrementObjectCounter() { abort(); /* STUB */ }

// 0x08056a70  shm.c:236
uint32_t GetObjectCounter() { abort(); /* STUB */ }

// 0x08056a90  shm.c:245
void IncrementPlayersOnline() { abort(); /* STUB */ }

// 0x08056ab0  shm.c:253
void DecrementPlayersOnline() { abort(); /* STUB */ }

// 0x08056ad0  shm.c:261
int GetPlayersOnline() { abort(); /* STUB */ }

// 0x08056af0  shm.c:270
void IncrementNewbiesOnline() { abort(); /* STUB */ }

// 0x08056b10  shm.c:278
void DecrementNewbiesOnline() { abort(); /* STUB */ }

// 0x08056b30  shm.c:286
int GetNewbiesOnline() { abort(); /* STUB */ }

// 0x08056b50  shm.c:295
void SetRoundNr(uint32_t /*RoundNr*/) { abort(); /* STUB */ }

// 0x08056b70  shm.c:303
uint32_t GetRoundNr() { abort(); /* STUB */ }

// 0x08056b90  shm.c:312
void SetCommand(int /*Command*/, char * /*Text*/) { abort(); /* STUB */ }

// 0x08056be0  shm.c:325
int GetCommand() { abort(); /* STUB */ }

// 0x08056c00  shm.c:334
char *GetCommandBuffer() { abort(); /* STUB */ }

// 0x08056c30  shm.c:349
bool DeleteSHM() { abort(); /* STUB */ }

// 0x08056cc0  shm.c:367
void CreateSHM() { abort(); /* STUB */ }

// 0x08056d90  shm.c:389
void AttachSHM() { abort(); /* STUB */ }

// 0x08056e70  shm.c:408
void DetachSHM() { abort(); /* STUB */ }

// 0x08056eb0  shm.c:427
void InitSHM(bool /*Verbose*/) { abort(); /* STUB */ }

// 0x08057000  shm.c:448
void ExitSHM() { abort(); /* STUB */ }

// 0x08057060  shm.c:460
void InitSHMExtern(bool /*Verbose*/) { abort(); /* STUB */ }

// 0x08057090  shm.c:469
void ExitSHMExtern() { abort(); /* STUB */ }
