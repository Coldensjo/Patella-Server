#ifndef PATELLA_SHM_HH
#define PATELLA_SHM_HH

#include <cstdint>
#include <sys/types.h>

struct Semaphore;
struct TSharedMemory;

// The command an operator tool leaves in the shared memory with SetCommand,
// which ProcessCommand (main.cc) carries out in the next round: broadcast the
// text of the command buffer. 0 is no command.
const int COMMAND_BROADCAST = 1;

void StartGame();
void CloseGame();
void EndGame();
bool GameRunning();
bool GameStarting();
bool GameEnding();
pid_t GetGameThreadPID();
void ErrorHandler(char *Text);
void PrintHandler(int Level, char *Text);
void IncrementObjectCounter();
void DecrementObjectCounter();
void IncrementPlayersOnline();
void DecrementPlayersOnline();
int GetPlayersOnline();
void IncrementNewbiesOnline();
void DecrementNewbiesOnline();
int GetNewbiesOnline();
void SetRoundNr(uint32_t RoundNr);
void SetCommand(int Command, char *Text);
int GetCommand();
char *GetCommandBuffer();
bool DeleteSHM();
void CreateSHM();
void AttachSHM();
void DetachSHM();
void InitSHM(bool Verbose);
void ExitSHM();

extern bool IsGameServer;
extern bool VerboseOutput;
extern TSharedMemory *SHM;
extern Semaphore LogfileMutex;

#endif
