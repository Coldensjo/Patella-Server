#include <cstdlib>

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
#include "reader.hh"
#include "communication.hh"
#include "TXTEASymmetricKey.hh"
#include "main.hh"

bool BeADaemon;	// main.c:39
int SigUsr1Counter;	// main.c:40
int SigAlarmCounter;	// main.c:41
bool SaveMapOn;	// main.c:42
bool Reboot;	// main.c:43

// 0x0804b2d0  main.c:54
void ProcessCommand() { abort(); /* STUB */ }

// 0x0804b350  main.c:73
// local static: int CreaturesTimeCounter = 0  (main.c:74, 0x081256ec .data)
// local static: int SkillTimeCounter = 0  (main.c:75, 0x081256f0 .data)
// local static: int CronTimeCounter = 0  (main.c:76, 0x081256f4 .data)
// local static: int OtherTimeCounter = 0  (main.c:77, 0x081256f8 .data)
// local static: uint32_t NextMinute = 60  (main.c:78, 0x081256fc .data)
// local static: int OldAmbiente = -1  (main.c:79, 0x08125700 .data)
// local static: bool Lag = false  (main.c:80, 0x08125704 .data)
void AdvanceGame(int /*Delay*/) { abort(); /* STUB */ }

// 0x0804b780  main.c:172
void HandlerSigAlarm(int) { abort(); /* STUB */ }

// 0x0804b7d0  main.c:186
void TimeReset() { abort(); /* STUB */ }

// 0x0804b7e0  main.c:194
void InitTime() { abort(); /* STUB */ }

// 0x0804b850  main.c:204
void ExitTime() { abort(); /* STUB */ }

// 0x0804b8b0  main.c:223
void SigHupHandler(int) { abort(); /* STUB */ }

// 0x0804b8c0  main.c:232
void SigAbortHandler(int) { abort(); /* STUB */ }

// 0x0804b8f0  main.c:241
void HandlerSigUsr1(int) { abort(); /* STUB */ }

// 0x0804b900  main.c:252
void DefaultHandler(int /*signo*/) { abort(); /* STUB */ }

// 0x0804ba60  main.c:289
void ErrorHandler(int /*signo*/) { abort(); /* STUB */ }

// 0x0804bad0  main.c:315
void InitSignalHandler() { abort(); /* STUB */ }

// 0x0804bd40  main.c:355
void ExitSignalHandler() { abort(); /* STUB */ }

// 0x0804bd50  main.c:367
void UnlockGame() { abort(); /* STUB */ }

// 0x0804bfd0  main.c:385
void LockGame() { abort(); /* STUB */ }

// 0x0804c570  main.c:411
void LoadWorldConfig() { abort(); /* STUB */ }

// 0x0804c6d0  main.c:432
void InitAll() { abort(); /* STUB */ }

// 0x0804c820  main.c:469
void ExitAll() { abort(); /* STUB */ }

// 0x0804c8d0  main.c:493
void LaunchGame() { abort(); /* STUB */ }

// 0x0804c9c0  main.c:528
int DaemonInit(bool /*NoFork*/) { abort(); /* STUB */ }

// 0x0804ca50  main.c:550
int main(int /*argc*/, char ** /*argv*/) { abort(); /* STUB */ }
