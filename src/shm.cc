#include <cerrno>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/types.h>
#include <unistd.h>

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
#include "writer.hh"
#include "shm.hh"

enum GAMESTATE {
	GAME_STARTING = 0,
	GAME_RUNNING = 1,
	GAME_CLOSING = 2,
	GAME_ENDING = 3,
};

const int PRINTBUFFERENTRIES = 200;

// The errors ErrorHandler writes to error.log; it counts the later ones
// without writing them.
const int MAX_LOGGED_ERRORS = 0x8000;

struct TSharedMemory {
	int Command;
	char CommandBuffer[256];
	uint32_t RoundNr;
	uint32_t ObjectCounter;
	uint32_t Errors;
	int PlayersOnline;
	int NewbiesOnline;
	int PrintBufferPosition;
	char PrintBuffer[PRINTBUFFERENTRIES][128];
	GAMESTATE GameState;
	int32_t GameThreadPID;
};
static_assert(sizeof(TSharedMemory) == 25892, "TSharedMemory is mapped by the operator tools; keep the 32-bit layout");
static_assert(offsetof(TSharedMemory, RoundNr) == 260, "TSharedMemory is mapped by the operator tools; keep the 32-bit layout");

bool IsGameServer = false;
bool VerboseOutput = false;
TSharedMemory *SHM = nullptr;
Semaphore LogfileMutex(1);

void StartGame() {
	if (SHM == nullptr) {
		error("StartGame: SharedMemory existiert nicht.\n");
		return;
	}

	if (SHM->GameState == GAME_STARTING) {
		SHM->GameState = GAME_RUNNING;
	}
}

void CloseGame() {
	if (SHM == nullptr) {
		error("CloseGame: SharedMemory existiert nicht.\n");
		return;
	}

	if (SHM->GameState == GAME_RUNNING) {
		SHM->GameState = GAME_CLOSING;
	}
}

void EndGame() {
	if (SHM == nullptr) {
		error("EndGame: SharedMemory existiert nicht.\n");
		return;
	}

	SHM->GameState = GAME_ENDING;
}

bool GameRunning() {
	if (SHM == nullptr) {
		error("GameRunning: SharedMemory existiert nicht.\n");
		return false;
	}

	return SHM->GameState == GAME_STARTING
		|| SHM->GameState == GAME_RUNNING
		|| SHM->GameState == GAME_CLOSING;
}

bool GameStarting() {
	if (SHM == nullptr) {
		error("GameStarting: SharedMemory existiert nicht.\n");
		return false;
	}

	return SHM->GameState == GAME_STARTING;
}

bool GameEnding() {
	if (SHM == nullptr) {
		error("GameEnding: SharedMemory existiert nicht.\n");
		return false;
	}

	return SHM->GameState == GAME_CLOSING
		|| SHM->GameState == GAME_ENDING;
}

pid_t GetGameThreadPID() {
	if (SHM == nullptr) {
		return 0;
	}

	return SHM->GameThreadPID;
}

void ErrorHandler(char *Text) {
	if (VerboseOutput) {
		printf("%s", Text);
	}

	if (SHM != nullptr) {
		// Text is handed to Log as its format string, as in the original.
		if (SHM->Errors < MAX_LOGGED_ERRORS) {
			Log("error", Text);
		}

		SHM->Errors++;
		if (SHM->Errors == MAX_LOGGED_ERRORS) {
			Log("error", "Zu viele Fehler. Keine weitere Protokollierung.\n");
		}
	}
}

void PrintHandler(int Level, char *Text) {
	if (Level > DebugLevel) {
		return;
	}

	if (VerboseOutput) {
		printf("%s", Text);
	}

	if (SHM != nullptr) {
		LogfileMutex.down();
		strncpy(SHM->PrintBuffer[SHM->PrintBufferPosition], Text, 127);
		SHM->PrintBuffer[SHM->PrintBufferPosition][126] = 0;
		if (SHM->PrintBuffer[SHM->PrintBufferPosition][0] != 0
				&& SHM->PrintBuffer[SHM->PrintBufferPosition][strlen(SHM->PrintBuffer[SHM->PrintBufferPosition]) - 1] != '\n') {
			strcat(SHM->PrintBuffer[SHM->PrintBufferPosition], "\n");
		}

		if (SHM->PrintBufferPosition + 1 < PRINTBUFFERENTRIES) {
			SHM->PrintBufferPosition = SHM->PrintBufferPosition + 1;
		} else {
			SHM->PrintBufferPosition = 0;
		}
		LogfileMutex.up();
	}
}

void IncrementObjectCounter() {
	if (SHM != nullptr) {
		SHM->ObjectCounter++;
	}
}

void DecrementObjectCounter() {
	if (SHM != nullptr) {
		SHM->ObjectCounter--;
	}
}

void IncrementPlayersOnline() {
	if (SHM != nullptr) {
		SHM->PlayersOnline++;
	}
}

void DecrementPlayersOnline() {
	if (SHM != nullptr) {
		SHM->PlayersOnline--;
	}
}

int GetPlayersOnline() {
	if (SHM == nullptr) {
		return 0;
	}

	return SHM->PlayersOnline;
}

void IncrementNewbiesOnline() {
	if (SHM != nullptr) {
		SHM->NewbiesOnline++;
	}
}

void DecrementNewbiesOnline() {
	if (SHM != nullptr) {
		SHM->NewbiesOnline--;
	}
}

int GetNewbiesOnline() {
	if (SHM == nullptr) {
		return 0;
	}

	return SHM->NewbiesOnline;
}

void SetRoundNr(uint32_t RoundNr) {
	if (SHM != nullptr) {
		SHM->RoundNr = RoundNr;
	}
}

void SetCommand(int Command, char *Text) {
	if (SHM != nullptr) {
		SHM->Command = Command;
		if (Text != nullptr) {
			strncpy(SHM->CommandBuffer, Text, sizeof(SHM->CommandBuffer));
			SHM->CommandBuffer[sizeof(SHM->CommandBuffer) - 1] = 0;
		} else {
			SHM->CommandBuffer[0] = 0;
		}
	}
}

int GetCommand() {
	if (SHM == nullptr) {
		return 0;
	}

	return SHM->Command;
}

char *GetCommandBuffer() {
	if (SHM == nullptr || SHM->Command == 0) {
		return nullptr;
	}

	return SHM->CommandBuffer;
}

bool DeleteSHM() {
	int SHMID = shmget(SHMKey, 0, 0);
	if (SHMID == -1) {
		if (VerboseOutput) {
			puts("DeleteSHM: SharedMemory existiert nicht.");
		}
		return true;
	}

	int retval = shmctl(SHMID, IPC_RMID, nullptr);
	if (retval == -1) {
		if (VerboseOutput) {
			puts("DeleteSHM: Kann SharedMemory nicht l\xf6schen.");
		}
		return false;
	}

	return true;
}

void CreateSHM() {
	int iter = 0;

again:
	int SHMID = shmget(SHMKey, sizeof(TSharedMemory), IPC_CREAT | IPC_EXCL | 0777);
	if (SHMID == -1) {
		if (errno == EEXIST && iter == 0) {
			if (!DeleteSHM()) {
				throw "Cannot delete SharedMemory";
			}
			iter++;
			goto again;
		}

		if (VerboseOutput) {
			printf("CreateSHM: Kann SharedMemory nicht anlegen (Fehler %d).\n", errno);
		}
		throw "Cannot create SharedMemory";
	}
}

void AttachSHM() {
	int SHMID = shmget(SHMKey, sizeof(TSharedMemory), 0);
	if (SHMID == -1) {
		if (VerboseOutput) {
			puts("AttachSHM: Kann SharedMemory nicht fassen.");
		}
		SHM = nullptr;
		throw "Cannot get SharedMemory";
	}

	SHM = static_cast<TSharedMemory *>(shmat(SHMID, nullptr, 0));
	if (SHM == reinterpret_cast<TSharedMemory *>(-1)) {
		if (VerboseOutput) {
			puts("AttachSHM: Kann SharedMemory nicht anbinden.");
		}
		SHM = nullptr;
		throw "Cannot attach SharedMemory";
	}
}

void DetachSHM() {
	if (SHM != nullptr) {
		int result = shmdt(SHM);
		if (result == -1 && VerboseOutput) {
			puts("DetachSHM: Kann SharedMemory nicht l\xf6schen.");
		}
		SHM = nullptr;
	}
}

void InitSHM(bool Verbose) {
	IsGameServer = true;
	VerboseOutput = Verbose;
	CreateSHM();
	AttachSHM();
	SetErrorFunction(ErrorHandler);
	SetPrintFunction(PrintHandler);

	memset(SHM, 0, sizeof(TSharedMemory));
	strcpy(SHM->PrintBuffer[0], "SHM initialized. System printing is working!\n");
	SHM->Errors = 0;
	SHM->RoundNr = 0;
	SHM->PrintBufferPosition = 1;
	SHM->ObjectCounter = 0;
	SHM->GameState = GAME_STARTING;
	// The process ID, not gettid(): the operator tools read this value from
	// the segment and signal it from their own process. InitSHM runs on the
	// main thread, which is the game thread, so it is that thread's ID too.
	SHM->GameThreadPID = getpid();
}

void ExitSHM() {
	SetErrorFunction(nullptr);
	SetPrintFunction(nullptr);
	DetachSHM();
	DeleteSHM();
}
