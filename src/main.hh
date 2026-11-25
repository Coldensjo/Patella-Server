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
// ConfigFile: the file of --config, or nullptr for ReadConfig's default.
void InitAll(const char *ConfigFile);
void ExitAll();
void LaunchGame();
int DaemonInit(bool NoFork);

// The shutdown the admin port schedules (patella-admin shutdown): in Minutes
// minutes (0: at the next round), with the warnings of the daily reboot at
// 5, 3 and 1 minutes left (and at every 10 before), logins closed from 5
// minutes left on; at the end what the daily reboot does: the players
// logged out, the map saved, the game ended. Restart: Reboot stays set, so
// main starts BINPATH/reboot-daily as after the daily reboot (and the map is
// refreshed first, as then); else the game stops. A new one replaces one
// scheduled. CancelShutdown: false when none is scheduled; the logins open
// again. ShutdownSecondsLeft: -1 when none is scheduled.
void ScheduleShutdown(int Minutes, bool Restart);
bool CancelShutdown();
int ShutdownSecondsLeft();
bool ShutdownRestarts();

extern bool BeADaemon;
extern int SigUsr1Counter;
extern int SigAlarmCounter;
extern bool SaveMapOn;
extern bool Reboot;

#endif
