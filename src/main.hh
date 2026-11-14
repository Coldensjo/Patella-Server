// main.hh: the start, the main loop and the signal handlers of main.cc, and the
// flags they share.

#ifndef PATELLA_MAIN_HH
#define PATELLA_MAIN_HH

void ProcessCommand();
void AdvanceGame(int Delay);
void HandlerSigAlarm(int);
void TimeReset();
void InitTime();
void ExitTime();
void SigHupHandler(int);
void SigAbortHandler(int);
void HandlerSigUsr1(int);
void DefaultHandler(int signo);
void InitSignalHandler();
void UnlockGame();
void LockGame();
void LoadWorldConfig();
void InitAll();
void ExitAll();
void LaunchGame();
int DaemonInit(bool NoFork);

extern bool BeADaemon;
extern int SigUsr1Counter;
extern int SigAlarmCounter;
extern bool SaveMapOn;
extern bool Reboot;

#endif
