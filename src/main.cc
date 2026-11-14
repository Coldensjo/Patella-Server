// main.cc: the game server's start and its main loop. main reads the command
// line ("daemon", "nofork", "--config FILE"), makes the server a daemon if
// asked and runs InitAll, LaunchGame and ExitAll; when the game ends for a
// reboot it starts BINPATH/reboot-daily.
// LaunchGame waits for SIGUSR1 (data from the connections) and SIGALRM (the
// beat) and advances the game with AdvanceGame. Here too: the signal
// handlers, the beat's timer, the PID file (LockGame, UnlockGame) and the
// world config from the query manager (LoadWorldConfig).

#include <csignal>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <exception>
#include <fstream>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>

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
#include "mudinc.hh"
#include "shm.hh"
#include "strings.hh"
#include "writer.hh"
#include "houses.hh"
#include "moveuse.hh"
#include "magic.hh"
#include "crmain.hh"
#include "crnonpl.hh"
#include "crplayer.hh"
#include "sending.hh"
#include "receiving.hh"
#include "main.hh"

// The length of a round (RoundNr) in milliseconds. AdvanceGame also runs the
// skills, the cron system and the creatures once a round, 250, 500 and 750
// milliseconds after the round begins.
const int ROUND_DURATION = 1000;

bool BeADaemon;
int SigUsr1Counter;
int SigAlarmCounter;
bool SaveMapOn;
bool Reboot;

void ProcessCommand() {
	int Command = GetCommand();
	if (Command != 0) {
		char *CommandBuffer = GetCommandBuffer();
		if (Command == COMMAND_BROADCAST) {
			if (CommandBuffer != nullptr) {
				BroadcastMessage(TALK_ADMIN_MESSAGE, "%s", CommandBuffer);
			} else {
				error("ProcessCommand: Text f\xfcr Broadcast ist NULL.\n");
			}
		} else {
			error("ProcessCommand: Unbekanntes Kommando %d.\n", Command);
		}
		SetCommand(0, nullptr);
	}
}

void AdvanceGame(int Delay) {
	static int CreaturesTimeCounter = 0;
	static int SkillTimeCounter = 0;
	static int CronTimeCounter = 0;
	static int OtherTimeCounter = 0;
	static uint32_t NextMinute = 60;
	static int OldAmbiente = -1;
	static bool Lag = false;

	CreaturesTimeCounter += Delay;
	SkillTimeCounter += Delay;
	CronTimeCounter += Delay;
	OtherTimeCounter += Delay;

	if (CreaturesTimeCounter >= ROUND_DURATION + 750) {
		CreaturesTimeCounter -= ROUND_DURATION;
		ProcessCreatures();
	}

	if (CronTimeCounter >= ROUND_DURATION + 500) {
		CronTimeCounter -= ROUND_DURATION;
		ProcessCronSystem();
	}

	if (SkillTimeCounter >= ROUND_DURATION + 250) {
		SkillTimeCounter -= ROUND_DURATION;
		ProcessSkills();
	}

	if (OtherTimeCounter >= ROUND_DURATION) {
		OtherTimeCounter -= ROUND_DURATION;
		RoundNr++;
		SetRoundNr(RoundNr);
		ProcessConnections();
		ProcessMonsterhomes();
		ProcessMonsterRaids();
		ProcessCommunicationControl();
		ProcessReaderThreadReplies(RefreshSector, SendMails);
		ProcessWriterThreadReplies();
		ProcessCommand();

		int Brightness, Color;
		GetAmbiente(Brightness, Color);
		if (Brightness != OldAmbiente) {
			OldAmbiente = Brightness;
			TConnection *Connection = GetFirstConnection();
			while (Connection != nullptr) {
				if (Connection->IsOnline()) {
					SendAmbiente(Connection);
				}
				Connection = GetNextConnection();
			}
		}

		if (RoundNr % 10 == 0) {
			NetLoadCheck();
		}

		if (RoundNr >= NextMinute) {
			int Hour, Minute;
			GetRealTime(Hour, Minute);
			RefreshCylinders();

			if (Minute % 5 == 0) {
				CreatePlayerList(true);
			}

			if (Minute % 15 == 0) {
				SavePlayerDataOrder();
			}

			if (Minute == 0) {
				NetLoadSummary();
			}

			if (Minute == 55) {
				WriteKillStatistics();
			}

			int Time = Hour * 60 + Minute;
			if ((Time + 5) % 1440 == RebootTime) {
				if (Reboot) {
					BroadcastMessage(TALK_ADMIN_MESSAGE, "Server is saving game in 5 minutes.\nPlease come back in 10 minutes.");
				} else {
					BroadcastMessage(TALK_ADMIN_MESSAGE, "Server is going down in 5 minutes.\nPlease log out.");
				}
				CloseGame();
			}

			if ((Time + 3) % 1440 == RebootTime) {
				if (Reboot) {
					BroadcastMessage(TALK_ADMIN_MESSAGE, "Server is saving game in 3 minutes.\nPlease come back in 10 minutes.");
				} else {
					BroadcastMessage(TALK_ADMIN_MESSAGE, "Server is going down in 3 minutes.\nPlease log out.");
				}
			}

			if ((Time + 1) % 1440 == RebootTime) {
				if (Reboot) {
					BroadcastMessage(TALK_ADMIN_MESSAGE, "Server is saving game in one minute.\nPlease log out.");
				} else {
					BroadcastMessage(TALK_ADMIN_MESSAGE, "Server is going down in one minute.\nPlease log out.");
				}
			}

			if (Time == RebootTime) {
				CloseGame();
				LogoutAllPlayers();
				SendAll();
				if (Reboot) {
					RefreshMap();
				}
				SaveMap();
				SaveMapOn = false;
				EndGame();
			}

			NextMinute = GetRoundForNextMinute();
		}

		CleanupDynamicStrings();
	}

	if (Delay > Beat) {
		Log("lag", "Verz\xf6gerung %d msec.\n", Delay);
	}

	if (Delay >= ROUND_DURATION) {
		if (!Lag && RoundNr > 10) {
			error("AdvanceGame: Keine Kreaturbewegung wegen Lag (Verz\xf6gerung: %d msec).\n", Delay);
		}
		Lag = true;
	} else {
		MoveCreatures(Delay);
		Lag = false;
	}

	SendAll();
}

void HandlerSigAlarm(int) {
	struct itimerval v, ov;

	SigAlarmCounter++;
	v.it_interval.tv_sec = 0;
	v.it_interval.tv_usec = 0;
	v.it_value.tv_sec = 0;
	v.it_value.tv_usec = Beat * 1000;
	setitimer(ITIMER_REAL, &v, &ov);
}

void TimeReset() {
	SigAlarmCounter = 0;
}

void InitTime() {
	TimeReset();
	handler(SIGALRM, HandlerSigAlarm);
	HandlerSigAlarm(SIGALRM);
}

void ExitTime() {
	struct itimerval v, ov;

	v.it_interval.tv_sec = 0;
	v.it_interval.tv_usec = 0;
	v.it_value.tv_sec = 0;
	v.it_value.tv_usec = 0;
	setitimer(ITIMER_REAL, &v, &ov);
	handler(SIGALRM, SIG_IGN);
}

void SigHupHandler(int) {
}

void SigAbortHandler(int) {
	print(1, "SigAbortHandler: schalte Writer-Thread ab.\n");
	AbortWriter();
}

void HandlerSigUsr1(int) {
	SigUsr1Counter++;
}

void DefaultHandler(int signo) {
	char s[128];

	// sigdescr_np gives the signal's description, the text sys_siglist[signo]
	// held before glibc stopped exporting it.
	snprintf(s, sizeof(s), "SigNr. %d: %s", signo, sigdescr_np(signo));
	print(1, "DefaultHandler: Beende Game-Server (%s).\n", s);

	handler(SIGINT, SIG_IGN);
	handler(SIGQUIT, SIG_IGN);
	handler(SIGTERM, SIG_IGN);
	handler(SIGXCPU, SIG_IGN);
	handler(SIGXFSZ, SIG_IGN);
	handler(SIGPWR, SIG_IGN);

	SaveMapOn = (signo == SIGQUIT || signo == SIGTERM || signo == SIGPWR);
	if (signo == SIGTERM) {
		int Hour, Minute;
		GetRealTime(Hour, Minute);
		RebootTime = (Hour * 60 + Minute + 6) % 1440;
		CloseGame();
	} else {
		EndGame();
	}

	Reboot = false;
}

void InitSignalHandler() {
	int count = 0;

	if (handler(SIGHUP, SigHupHandler) != SIG_ERR) {
		count++;
	}
	if (handler(SIGINT, DefaultHandler) != SIG_ERR) {
		count++;
	}
	if (handler(SIGQUIT, DefaultHandler) != SIG_ERR) {
		count++;
	}
	if (handler(SIGABRT, SigAbortHandler) != SIG_ERR) {
		count++;
	}
	if (handler(SIGUSR1, SIG_IGN) != SIG_ERR) {
		count++;
	}
	if (handler(SIGUSR2, SIG_IGN) != SIG_ERR) {
		count++;
	}
	if (handler(SIGPIPE, SIG_IGN) != SIG_ERR) {
		count++;
	}
	if (handler(SIGALRM, SIG_IGN) != SIG_ERR) {
		count++;
	}
	if (handler(SIGTERM, DefaultHandler) != SIG_ERR) {
		count++;
	}
	if (handler(SIGSTKFLT, SIG_IGN) != SIG_ERR) {
		count++;
	}
	if (handler(SIGCHLD, SIG_IGN) != SIG_ERR) {
		count++;
	}
	if (handler(SIGTSTP, SIG_IGN) != SIG_ERR) {
		count++;
	}
	if (handler(SIGTTIN, SIG_IGN) != SIG_ERR) {
		count++;
	}
	if (handler(SIGTTOU, SIG_IGN) != SIG_ERR) {
		count++;
	}
	if (handler(SIGURG, SIG_IGN) != SIG_ERR) {
		count++;
	}
	if (handler(SIGXCPU, DefaultHandler) != SIG_ERR) {
		count++;
	}
	if (handler(SIGXFSZ, DefaultHandler) != SIG_ERR) {
		count++;
	}
	if (handler(SIGVTALRM, SIG_IGN) != SIG_ERR) {
		count++;
	}
	if (handler(SIGWINCH, SIG_IGN) != SIG_ERR) {
		count++;
	}
	if (handler(SIGPOLL, SIG_IGN) != SIG_ERR) {
		count++;
	}
	if (handler(SIGPWR, DefaultHandler) != SIG_ERR) {
		count++;
	}

	// Quirk: 21 handlers are set up, but the expected number printed is 28.
	print(1, "InitSignalHandler: %d Signalhandler eingerichtet (Soll=%d)\n", count, 28);
}

void UnlockGame() {
	char FileName[4096];
	// When the read stores nothing (an empty file), Pid stays 0 and the file
	// is kept.
	int Pid = 0;
	std::ifstream IF;

	strcpy(FileName, SAVEPATH);
	strcat(FileName, "/game.pid");
	IF.open(FileName, std::ios::in);
	if (!IF.fail()) {
		IF >> Pid;
		IF.close();
		if (Pid == getpid()) {
			unlink(FileName);
		}
	}
}

// Throws a const char * when game.pid names another process.
void LockGame() {
	std::ifstream IF;
	std::ofstream OF;
	char FileName[4096];

	strcpy(FileName, SAVEPATH);
	strcat(FileName, "/game.pid");
	IF.open(FileName, std::ios::in);
	if (IF.fail()) {
		IF.close();
		OF.open(FileName, std::ios::out | std::ios::trunc);
		OF << getpid();
		atexit(UnlockGame);
	} else {
		// Pid stays 0 when the read fails, so an empty or non-numeric
		// game.pid does not stop the start.
		int Pid = 0;
		IF >> Pid;
		IF.close();
		if (Pid != 0) {
			throw "Game-Server is already running, PID file exists.";
		}
	}
}

// Throws a const char * when the query manager cannot be reached or does not
// give the world config.
void LoadWorldConfig() {
	TQueryManagerConnection Connection;
	if (!Connection.isConnected()) {
		error("LoadWorldConfig: Kann nicht zum Query-Manager verbinden.\n");
		throw "cannot connect to querymanager";
	}

	int HelpWorldType;
	int HelpGameAddress[4];
	if (Connection.loadWorldConfig(HelpWorldType, RebootTime, HelpGameAddress, GamePort,
			MaxPlayers, PremiumPlayerBuffer, MaxNewbies, PremiumNewbieBuffer) != 0) {
		error("LoadWorldConfig: Kann Konfigurationsdaten nicht holen.\n");
		throw "cannot load world config";
	}

	WorldType = static_cast<TWorldType>(HelpWorldType);
	snprintf(GameAddress, sizeof(GameAddress), "%d.%d.%d.%d",
			HelpGameAddress[0], HelpGameAddress[1], HelpGameAddress[2], HelpGameAddress[3]);
}

void InitAll(const char *ConfigFile) {
	try {
		ReadConfig(ConfigFile);
		SetQueryManagerLoginData(APPLICATION_GAME, WorldName);
		LoadWorldConfig();
		InitSHM(!BeADaemon);
		LockGame();
		InitLog("game");
		srand(static_cast<unsigned int>(time(nullptr)));
		InitSignalHandler();
		InitConnections();
		InitCommunication();
		InitWriter();
		InitReader();
		InitObjects();
		InitMap();
		InitMoveUse();
		InitMagic();
		InitCr();
		InitHouses();
		InitTime();
		ApplyPatches();
	} catch (const char *s) {
		error("Initialisierungsfehler: %s\n", s);
		exit(1);
	}
}

void ExitAll() {
	EndGame();
	ExitTime();
	ExitCr();
	ExitMoveUse();
	ExitHouses();
	ExitMap(SaveMapOn);
	ExitReader();
	ExitWriter();
	ExitCommunication();
	ExitSHM();
}

void LaunchGame() {
	SigUsr1Counter = 0;
	handler(SIGUSR1, HandlerSigUsr1);
	StartGame();
	print(1, "LaunchGame: Game-Server ist bereit (Pid=%d).\n", getpid());
	SaveMapOn = true;
	TimeReset();

	// SIGUSR1 stays blocked except inside sigsuspend, which waits with no
	// signal blocked: a SIGUSR1 that comes after the counters are tested ends
	// the wait instead of being missed.
	sigset_t BlockSet, PauseSet;
	sigemptyset(&BlockSet);
	sigaddset(&BlockSet, SIGUSR1);
	sigemptyset(&PauseSet);

	while (GameRunning()) {
		sigprocmask(SIG_BLOCK, &BlockSet, nullptr);
		while (SigUsr1Counter == 0 && SigAlarmCounter == 0) {
			sigsuspend(&PauseSet);
		}

		if (SigUsr1Counter > 0) {
			SigUsr1Counter = 0;
			ReceiveData();
		}

		if (SigAlarmCounter > 0) {
			int Time = SigAlarmCounter * Beat;
			SigAlarmCounter = 0;
			AdvanceGame(Time);
		}
	}

	LogoutAllPlayers();
}

int DaemonInit(bool NoFork) {
	if (!NoFork) {
		pid_t Pid = fork();
		if (Pid < 0) {
			return 1;
		}
		if (Pid != 0) {
			exit(0);
		}
		setsid();
	}

	umask(0);
	umask(0177);
	chdir(SAVEPATH);

	int openmax = static_cast<int>(sysconf(_SC_OPEN_MAX));
	if (openmax < 0) {
		openmax = 1024;
	}

	for (int i = 0; i < openmax; i++) {
		close(i);
	}

	return 0;
}

int main(int argc, char **argv) {
	bool NoFork = false;
	const char *ConfigFile = nullptr;

	BeADaemon = false;
	Reboot = true;
	for (int i = 1; i < argc; i++) {
		if (strcmp(argv[i], "daemon") == 0) {
			BeADaemon = true;
		}
		if (strcmp(argv[i], "nofork") == 0) {
			NoFork = true;
		}
		if (strcmp(argv[i], "--config") == 0) {
			if (i + 1 >= argc) {
				fprintf(stderr, "game: --config needs a file name\n");
				return 2;
			}
			i++;
			ConfigFile = argv[i];
		}
	}

	if (!BeADaemon) {
		puts("Tibia Game-Server\n(c) by CIP Productions, 2003.\n");
	}

	if (BeADaemon) {
		if (DaemonInit(NoFork) != 0) {
			return 2;
		}
	}

	InitAll(ConfigFile);
	atexit(ExitAll);

	try {
		LaunchGame();
	} catch (RESULT r) {
		error("main: Nicht abgefangene Exception %d.\n", r);
		Reboot = false;
	} catch (const char *s) {
		error("main: Nicht abgefangene Exception \"%s\".\n", s);
		Reboot = false;
	} catch (std::exception &e) {
		error("main: Nicht abgefangene Exception %s.\n", e.what());
		Reboot = false;
	} catch (...) {
		error("main: Nicht abgefangene Exception unbekannten Typs.\n");
		Reboot = false;
	}

	if (Reboot) {
		char FileName[4096];

		UnlockGame();
		sprintf(FileName, "%s/reboot-daily", BINPATH);
		if (FileExists(FileName)) {
			ExitAll();
			print(1, "Starte Game-Server neu...\n");
			execv(FileName, argv);
		}
		print(1, "Reboot-Skript existiert nicht.\n");
	} else {
		print(1, "Beende Game-Server...\n");
	}

	return 0;
}
