// writer.cc: the two threads that take slow work off the game thread. The
// protocol thread writes the log files (LOGPATH/<name>.log) whose lines Log
// queues for it. The writer thread carries out the orders the game queues
// (InsertOrder): the query manager requests of logouts, player lists, kill
// statistics, punishments, deaths, buddies and logins that did not happen
// (decrementIsOnline), and the saving of the player files. Its replies
// (broadcasts, direct messages, logouts) go back to the game thread, which
// handles them in ProcessWriterThreadReplies.

#include <algorithm>
#include <cerrno>
#include <cstdarg>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <time.h>

#include "script.hh"
#include "config.hh"
#include "threads.hh"
#include "dbfuncs.hh"
#include "query.hh"
#include "objects.hh"
#include "map.hh"
#include "info.hh"
#include "cr.hh"
#include "crplayer.hh"
#include "containr.hh"
#include "operate.hh"
#include "connections.hh"
#include "sending.hh"
#include "utils.hh"
#include "time.hh"
#include "TXTEASymmetricKey.hh"
#include "writer.hh"

const int PROTOCOL_TEXTLENGTH = 256;

struct TProtocolThreadOrder {
	char ProtocolName[20];
	char Text[PROTOCOL_TEXTLENGTH];
};

enum TWriterThreadOrderType {
	ORDER_TERMINATE = 0,
	ORDER_LOGOUT = 1,
	ORDER_PLAYERLIST = 2,
	ORDER_KILLSTATISTICS = 3,
	ORDER_PUNISHMENT = 4,
	ORDER_CHARACTERDEATH = 5,
	ORDER_ADDBUDDY = 6,
	ORDER_REMOVEBUDDY = 7,
	ORDER_DECREMENTISONLINE = 8,
	ORDER_SAVEPLAYERDATA = 9,
};

struct TWriterThreadOrder {
	TWriterThreadOrderType OrderType;
	void *Data;
};

struct TLogoutOrderData {
	uint32_t CharacterID;
	int Level;
	int Profession;
	time_t LastLoginTime;
	int TutorActivities;
	char Residence[30];
};

struct TPlayerlistOrderData {
	int NumberOfPlayers;
	char *PlayerNames;
	int *PlayerLevels;
	int *PlayerProfessions;
};

struct TKillStatisticsOrderData {
	int NumberOfRaces;
	char *RaceNames;
	int *KilledPlayers;
	int *KilledCreatures;
};

struct TPunishmentOrderData {
	uint32_t GamemasterID;
	char GamemasterName[30];
	char CriminalName[30];
	char CriminalIPAddress[16];
	int Reason;
	int Action;
	char Comment[200];
	int NumberOfStatements;
	vector<TReportedStatement> *ReportedStatements;
	uint32_t StatementID;
	bool IPBanishment;
};

struct TCharacterDeathOrderData {
	uint32_t CharacterID;
	int Level;
	uint32_t Offender;
	char Remark[30];
	bool Unjustified;
	time_t Time;
};

struct TBuddyOrderData {
	uint32_t AccountID;
	uint32_t Buddy;
};

enum TWriterThreadReplyType {
	REPLY_BROADCAST = 0,
	REPLY_DIRECT = 1,
	REPLY_LOGOUT = 2,
};

struct TWriterThreadReply {
	TWriterThreadReplyType ReplyType;
	void *Data;
};

struct TDirectReplyData {
	uint32_t CharacterID;
	char Message[100];
};

const int PROTOCOL_BUFFER_SIZE = 1000;
const int ORDER_BUFFER_SIZE = 2000;
const int REPLY_BUFFER_SIZE = 100;

// external functions that writer.hh cannot declare: they take an enum declared in this file
void InsertOrder(TWriterThreadOrderType OrderType, void *Data);
void GetOrder(TWriterThreadOrderType &OrderType, void *&Data);
void InsertReply(TWriterThreadReplyType ReplyType, void *Data);
bool GetReply(TWriterThreadReplyType &ReplyType, void *&Data);

static Thread ProtocolThread = 0;
static Thread WriterThread = 0;
static TQueryManagerConnection *QueryManagerConnection = nullptr;
static TProtocolThreadOrder ProtocolBuffer[PROTOCOL_BUFFER_SIZE];
static TWriterThreadOrder OrderBuffer[ORDER_BUFFER_SIZE];
static TWriterThreadReply ReplyBuffer[REPLY_BUFFER_SIZE];
static int ProtocolPointerWrite;
static int ProtocolPointerRead;
static int OrderPointerWrite;
static int OrderPointerRead;
static int ReplyPointerWrite;
static int ReplyPointerRead;
static Semaphore ProtocolMutex(1);
static Semaphore ProtocolBufferEmpty(PROTOCOL_BUFFER_SIZE);
static Semaphore ProtocolBufferFull(0);
static Semaphore OrderBufferEmpty(ORDER_BUFFER_SIZE);
static Semaphore OrderBufferFull(0);

void InitProtocol() {
	ProtocolPointerWrite = 0;
	ProtocolPointerRead = 0;
}

void InsertProtocolOrder(char *ProtocolName, char *Text) {
	if (ProtocolName == nullptr) {
		error("InsertProtocolOrder: Protokoll-Name nicht angegeben.\n");
		return;
	}

	if (Text == nullptr) {
		error("InsertProtocolOrder: Text nicht angegeben.\n");
		return;
	}

	if (ProtocolPointerWrite - ProtocolPointerRead >= PROTOCOL_BUFFER_SIZE && strcmp(ProtocolName, "error") != 0) {
		error("InsertProtocolOrder: Protokoll-Puffer ist voll => Vergr\xf6\xdf" "ern.\n");
	}

	ProtocolMutex.down();
	ProtocolBufferEmpty.down();
	strcpy(ProtocolBuffer[ProtocolPointerWrite % PROTOCOL_BUFFER_SIZE].ProtocolName, ProtocolName);
	strcpy(ProtocolBuffer[ProtocolPointerWrite % PROTOCOL_BUFFER_SIZE].Text, Text);
	ProtocolPointerWrite++;
	ProtocolBufferFull.up();
	ProtocolMutex.up();
}

void GetProtocolOrder(char *ProtocolName, char *Text) {
	ProtocolBufferFull.down();
	strcpy(ProtocolName, ProtocolBuffer[ProtocolPointerRead % PROTOCOL_BUFFER_SIZE].ProtocolName);
	strcpy(Text, ProtocolBuffer[ProtocolPointerRead % PROTOCOL_BUFFER_SIZE].Text);
	ProtocolPointerRead++;
	ProtocolBufferEmpty.up();
}

void WriteProtocol(char *ProtocolName, char *Text) {
	char FileName[4096];

	sprintf(FileName, "%s/%s.log", LOGPATH, ProtocolName);
	FILE *f = fopen(FileName, "at");
	if (f != nullptr) {
		fprintf(f, "%s", Text);
		if (fclose(f) != 0) {
			error("WriteProtocol: Fehler %d beim Schlie\xdf" "en der Datei.\n", errno);
		}
	}
}

int ProtocolThreadLoop(void *) {
	char ProtocolName[20];
	char Text[PROTOCOL_TEXTLENGTH];

	while (true) {
		GetProtocolOrder(ProtocolName, Text);
		if (ProtocolName[0] == 0) {
			break;
		}

		WriteProtocol(ProtocolName, Text);
	}

	return 0;
}

void InitLog(char *ProtocolName) {
	time_t Time;
	char FileName[4096];

	if (ProtocolName == nullptr) {
		error("InitLog: Protokoll-Name nicht angegeben.\n");
		return;
	}

	sprintf(FileName, "%s/%s.log", LOGPATH, ProtocolName);
	FILE *f = fopen(FileName, "at");
	if (f == nullptr) {
		error("InitLog: Kann Protokoll %s nicht anlegen.\n", ProtocolName);
		return;
	}

	time(&Time);
	fprintf(f, "-------------------------------------------------------------------------------\n");
	fprintf(f, "Tibia - Graphical Multi-User-Dungeon\n");
	fprintf(f, "%s.log - gestartet %s", ProtocolName, ctime(&Time));
	fclose(f);
}

void Log(char *ProtocolName, char *Text, ...) {
	if (ProtocolName == nullptr || ProtocolName[0] == 0) {
		error("Log: Protokoll-Name nicht angegeben.\n");
		return;
	}

	bool WriteDate = true;
	if (strcmp(ProtocolName, "bugreport") == 0 || strcmp(ProtocolName, "client-error") == 0 || strcmp(ProtocolName, "load") == 0) {
		WriteDate = false;
	}

	va_list args;
	char Output[PROTOCOL_TEXTLENGTH], Line[PROTOCOL_TEXTLENGTH];
	time_t Time;

	va_start(args, Text);
	vsnprintf(Output, PROTOCOL_TEXTLENGTH, Text, args);
	va_end(args);
	Output[PROTOCOL_TEXTLENGTH - 1] = 0;

	if (WriteDate) {
		print(2, "%s.log: %s", ProtocolName, Output);
		time(&Time);
		tm *t = localtime(&Time);
		snprintf(Line, PROTOCOL_TEXTLENGTH, "%02d.%02d.%04d %02d:%02d:%02d (%ld): %s", t->tm_mday, t->tm_mon + 1, t->tm_year + 1900, t->tm_hour, t->tm_min, t->tm_sec, (long)(int32_t)RoundNr, Output);
	} else {
		strcpy(Line, Output);
	}

	Line[PROTOCOL_TEXTLENGTH - 2] = 0;
	if (Line[0] != 0 && Line[strlen(Line) - 1] != '\n') {
		strcat(Line, "\n");
	}

	if (ProtocolThread != 0) {
		InsertProtocolOrder(ProtocolName, Line);
	} else {
		WriteProtocol(ProtocolName, Line);
	}
}

void InitWriterBuffers() {
	OrderPointerWrite = 0;
	OrderPointerRead = 0;
	ReplyPointerWrite = 0;
	ReplyPointerRead = 0;
}

int GetOrderBufferSpace() {
	if (WriterThread == 0) {
		return 0x7fffffff;
	}

	return ORDER_BUFFER_SIZE - (OrderPointerWrite - OrderPointerRead);
}

void InsertOrder(TWriterThreadOrderType OrderType, void *Data) {
	if (WriterThread == 0) {
		return;
	}

	if (OrderPointerWrite - OrderPointerRead >= ORDER_BUFFER_SIZE) {
		error("InsertOrder (Writer): Order-Puffer ist voll => Vergr\xf6\xdf" "ern.\n");
	}

	OrderBufferEmpty.down();
	OrderBuffer[OrderPointerWrite % ORDER_BUFFER_SIZE].OrderType = OrderType;
	OrderBuffer[OrderPointerWrite % ORDER_BUFFER_SIZE].Data = Data;
	OrderPointerWrite++;
	OrderBufferFull.up();
}

void GetOrder(TWriterThreadOrderType &OrderType, void *&Data) {
	OrderBufferFull.down();
	OrderType = OrderBuffer[OrderPointerRead % ORDER_BUFFER_SIZE].OrderType;
	Data = OrderBuffer[OrderPointerRead % ORDER_BUFFER_SIZE].Data;
	OrderPointerRead++;
	OrderBufferEmpty.up();
}

void InsertReply(TWriterThreadReplyType ReplyType, void *Data) {
	if (ReplyPointerWrite - ReplyPointerRead >= REPLY_BUFFER_SIZE) {
		error("InsertReply (Writer): Puffer ist voll; R\xfc" "ckmeldung wird verworfen.\n");
		return;
	}

	ReplyBuffer[ReplyPointerWrite % REPLY_BUFFER_SIZE].ReplyType = ReplyType;
	ReplyBuffer[ReplyPointerWrite % REPLY_BUFFER_SIZE].Data = Data;
	ReplyPointerWrite++;
}

void DirectReply(uint32_t CharacterID, char *Text, ...) {
	if (CharacterID == 0) {
		return;
	}

	if (Text == nullptr) {
		error("SendDirectReply: Kein Text angegeben.\n");
		return;
	}

	TDirectReplyData *Data = new TDirectReplyData;
	Data->CharacterID = CharacterID;

	va_list args;
	va_start(args, Text);
	vsnprintf(Data->Message, 100, Text, args);
	va_end(args);

	InsertReply(REPLY_DIRECT, Data);
}

void LogoutReply(char *PlayerName) {
	int Length = static_cast<int>(strlen(PlayerName));
	char *Buffer = new char[Length + 1];
	strcpy(Buffer, PlayerName);
	InsertReply(REPLY_LOGOUT, Buffer);
}

bool GetReply(TWriterThreadReplyType &ReplyType, void *&Data) {
	if (ReplyPointerRead == ReplyPointerWrite) {
		return false;
	}

	ReplyType = ReplyBuffer[ReplyPointerRead % REPLY_BUFFER_SIZE].ReplyType;
	Data = ReplyBuffer[ReplyPointerRead % REPLY_BUFFER_SIZE].Data;
	ReplyPointerRead++;
	return true;
}

void TerminateWriterOrder() {
	InsertOrder(ORDER_TERMINATE, nullptr);
}

void LogoutOrder(TPlayer *Player) {
	if (Player == nullptr) {
		error("LogoutOrder: \xdc" "bergebener Spieler existiert nicht.\n");
		return;
	}

	TLogoutOrderData *Data = new TLogoutOrderData;
	Data->CharacterID = Player->ID;
	Data->Level = Player->Skills[SKILL_LEVEL]->Get();
	Data->Profession = Player->GetActiveProfession();
	if (Player->PlayerData != nullptr) {
		Data->LastLoginTime = Player->PlayerData->LastLoginTime;
	} else {
		error("LogoutOrder: PlayerData ist NULL.\n");
		Data->LastLoginTime = 0;
	}
	Data->TutorActivities = Player->TutorActivities;

	switch (Player->startx) {
		case 32097:
			strcpy(Data->Residence, "Rookgaard");
			break;
		case 32360:
			strcpy(Data->Residence, "Carlin");
			break;
		case 32369:
			strcpy(Data->Residence, "Thais");
			break;
		case 32595:
			strcpy(Data->Residence, "Port Hope");
			break;
		case 32649:
			strcpy(Data->Residence, "Kazordoon");
			break;
		case 32732:
			strcpy(Data->Residence, "Ab'Dendriel");
			break;
		case 32957:
			strcpy(Data->Residence, "Venore");
			break;
		case 33194:
			strcpy(Data->Residence, "Ankrahmun");
			break;
		case 33213:
			strcpy(Data->Residence, "Darashia");
			break;
		case 33217:
			strcpy(Data->Residence, "Edron");
			break;
		default:
			error("LogoutOrder: Unbekannte Startkoordinate [%d,%d,%d] bei Spieler %s.\n", Player->startx, Player->starty, Player->startz, Player->Name);
			strcpy(Data->Residence, "Unknown");
			break;
	}

	InsertOrder(ORDER_LOGOUT, Data);
}

void PlayerlistOrder(int NumberOfPlayers, char *PlayerNames, int *PlayerLevels, int *PlayerProfessions) {
	if (PlayerNames == nullptr) {
		error("PlayerlistOrder: PlayerNames ist NULL.\n");
		return;
	}

	if (PlayerLevels == nullptr) {
		error("PlayerlistOrder: PlayerLevels ist NULL.\n");
		return;
	}

	if (PlayerProfessions == nullptr) {
		error("PlayerlistOrder: PlayerProfessions ist NULL.\n");
		return;
	}

	TPlayerlistOrderData *Data = new TPlayerlistOrderData;
	Data->NumberOfPlayers = NumberOfPlayers;
	Data->PlayerNames = PlayerNames;
	Data->PlayerLevels = PlayerLevels;
	Data->PlayerProfessions = PlayerProfessions;
	InsertOrder(ORDER_PLAYERLIST, Data);
}

void KillStatisticsOrder(int NumberOfRaces, char *RaceNames, int *KilledPlayers, int *KilledCreatures) {
	if (RaceNames == nullptr) {
		error("KillStatisticsOrder: RaceNames ist NULL.\n");
		return;
	}

	if (KilledPlayers == nullptr) {
		error("KillStatisticsOrder: KilledPlayers ist NULL.\n");
		return;
	}

	if (KilledCreatures == nullptr) {
		error("KillStatisticsOrder: KilledCreatures ist NULL.\n");
		return;
	}

	TKillStatisticsOrderData *Data = new TKillStatisticsOrderData;
	Data->NumberOfRaces = NumberOfRaces;
	Data->RaceNames = RaceNames;
	Data->KilledPlayers = KilledPlayers;
	Data->KilledCreatures = KilledCreatures;
	InsertOrder(ORDER_KILLSTATISTICS, Data);
}

void PunishmentOrder(TCreature *cr, char *Name, char *IPAddress, int Reason, int Action, char *Comment, int NumberOfStatements, vector<TReportedStatement> *ReportedStatements, uint32_t StatementID, bool IPBanishment) {
	if (Name == nullptr) {
		error("PunishmentOrder: Name ist NULL.\n");
		return;
	}

	if (Comment == nullptr) {
		error("PunishmentOrder: Comment ist NULL.\n");
		return;
	}

	TPunishmentOrderData *Data = new TPunishmentOrderData;
	Data->GamemasterID = (cr != nullptr ? cr->ID : 0);
	strcpy(Data->GamemasterName, (cr != nullptr ? cr->Name : "automatic"));
	strcpy(Data->CriminalName, Name);
	if (IPAddress != nullptr) {
		strcpy(Data->CriminalIPAddress, IPAddress);
	} else {
		Data->CriminalIPAddress[0] = 0;
	}
	Data->Reason = Reason;
	Data->Action = Action;
	strcpy(Data->Comment, Comment);
	Data->NumberOfStatements = NumberOfStatements;
	Data->ReportedStatements = ReportedStatements;
	Data->StatementID = StatementID;
	Data->IPBanishment = IPBanishment;
	InsertOrder(ORDER_PUNISHMENT, Data);
}

void CharacterDeathOrder(TCreature *cr, int OldLevel, uint32_t Offender, char *Remark, bool Unjustified) {
	if (cr == nullptr) {
		error("CharacterDeathOrder: cr ist NULL.\n");
		return;
	}

	if (Remark == nullptr) {
		error("CharacterDeathOrder: Remark ist NULL.\n");
		return;
	}

	TCharacterDeathOrderData *Data = new TCharacterDeathOrderData;
	Data->CharacterID = cr->ID;
	Data->Level = OldLevel;
	Data->Offender = Offender;
	strcpy(Data->Remark, Remark);
	Data->Unjustified = Unjustified;
	Data->Time = time(nullptr);
	InsertOrder(ORDER_CHARACTERDEATH, Data);
}

void AddBuddyOrder(TCreature *cr, uint32_t Buddy) {
	if (cr == nullptr) {
		error("AddBuddyOrder: cr ist NULL.\n");
		return;
	}

	if (cr->Type != PLAYER) {
		error("AddBuddyOrder: Kreatur ist kein Spieler.\n");
		return;
	}

	TBuddyOrderData *Data = new TBuddyOrderData;
	Data->AccountID = static_cast<TPlayer *>(cr)->AccountID;
	Data->Buddy = Buddy;
	InsertOrder(ORDER_ADDBUDDY, Data);
}

void RemoveBuddyOrder(TCreature *cr, uint32_t Buddy) {
	if (cr == nullptr) {
		error("RemoveBuddyOrder: cr ist NULL.\n");
		return;
	}

	if (cr->Type != PLAYER) {
		error("RemoveBuddyOrder: Kreatur ist kein Spieler.\n");
		return;
	}

	TBuddyOrderData *Data = new TBuddyOrderData;
	Data->AccountID = static_cast<TPlayer *>(cr)->AccountID;
	Data->Buddy = Buddy;
	InsertOrder(ORDER_REMOVEBUDDY, Data);
}

void DecrementIsOnlineOrder(uint32_t CharacterID) {
	// the character ID itself travels in the order's data pointer
	InsertOrder(ORDER_DECREMENTISONLINE, reinterpret_cast<void *>(static_cast<uintptr_t>(CharacterID)));
}

void SavePlayerDataOrder() {
	InsertOrder(ORDER_SAVEPLAYERDATA, nullptr);
}

void ProcessLogoutOrder(void *RawData) {
	TLogoutOrderData *Data = static_cast<TLogoutOrderData *>(RawData);
	if (Data == nullptr) {
		error("ProcessLogoutOrder: Keine Daten \xfc" "bergeben.\n");
		return;
	}

	char HelpProfession[30];
	GetProfessionName(HelpProfession, Data->Profession, false, true);
	if (Data->TutorActivities > 0) {
		print(3, "%d Tutor-Punkte f\xfc" "r Spieler %lu.\n", Data->TutorActivities, (unsigned long)Data->CharacterID);
	}

	if (QueryManagerConnection->logoutGame(Data->CharacterID, Data->Level, HelpProfession, Data->Residence, Data->LastLoginTime, Data->TutorActivities) != 0) {
		error("ProcessLogoutOrder: Logout f\xfc" "r Spieler %ld fehlgeschlagen.\n", (long)(int32_t)Data->CharacterID);
	}

	delete Data;
}

void ProcessPlayerlistOrder(void *RawData) {
	TPlayerlistOrderData *Data = static_cast<TPlayerlistOrderData *>(RawData);
	if (Data == nullptr) {
		error("ProcessPlayerlistOrder: Keine Daten \xfc" "bergeben.\n");
		return;
	}

	bool NewRecord;

	// as in the original, a failed request returns without releasing the order
	if (Data->NumberOfPlayers > 0) {
		char *Names[Data->NumberOfPlayers];
		int Levels[Data->NumberOfPlayers];
		char Professions[Data->NumberOfPlayers][30];
		for (int i = 0; i < Data->NumberOfPlayers; i++) {
			Names[i] = Data->PlayerNames + i * 30;
			Levels[i] = Data->PlayerLevels[i];
			GetProfessionName(Professions[i], Data->PlayerProfessions[i], false, true);
		}

		if (QueryManagerConnection->createPlayerlist(Data->NumberOfPlayers, Names, Levels, Professions, NewRecord) != 0) {
			error("ProcessPlayerlistOrder: Anfrage fehlgeschlagen (2).\n");
			return;
		}
	} else {
		if (QueryManagerConnection->createPlayerlist(Data->NumberOfPlayers, nullptr, nullptr, nullptr, NewRecord) != 0) {
			error("ProcessPlayerlistOrder: Anfrage fehlgeschlagen (1).\n");
			return;
		}
	}

	if (NewRecord) {
		char *Message = new char[100];
		snprintf(Message, 100, "New record: %d players are logged in.", Data->NumberOfPlayers);
		InsertReply(REPLY_BROADCAST, Message);
	}

	delete[] Data->PlayerNames;
	delete[] Data->PlayerLevels;
	delete[] Data->PlayerProfessions;
	delete Data;
}

void ProcessKillStatisticsOrder(void *RawData) {
	TKillStatisticsOrderData *Data = static_cast<TKillStatisticsOrderData *>(RawData);
	if (Data == nullptr) {
		error("ProcessKillStatisticsOrder: Keine Daten \xfc" "bergeben.\n");
		return;
	}

	char *RaceNames[Data->NumberOfRaces];
	int KilledPlayers[Data->NumberOfRaces];
	int KilledCreatures[Data->NumberOfRaces];
	for (int i = 0; i < Data->NumberOfRaces; i++) {
		RaceNames[i] = Data->RaceNames + i * 30;
		KilledPlayers[i] = Data->KilledPlayers[i];
		KilledCreatures[i] = Data->KilledCreatures[i];
	}

	// as in the original, a failed request returns without releasing the order
	if (QueryManagerConnection->logKilledCreatures(Data->NumberOfRaces, RaceNames, KilledPlayers, KilledCreatures) != 0) {
		error("ProcessKillStatisticsOrder: Anfrage fehlgeschlagen.\n");
		return;
	}

	delete[] Data->RaceNames;
	delete[] Data->KilledPlayers;
	delete[] Data->KilledCreatures;
	delete Data;
}

void ProcessPunishmentOrder(void *RawData) {
	TPunishmentOrderData *Data = static_cast<TPunishmentOrderData *>(RawData);
	if (Data == nullptr) {
		error("ProcessPunishmentOrder: Keine Daten \xfc" "bergeben.\n");
		return;
	}

	bool ok = true;
	uint32_t BanishmentID = 0;

	if (Data->Action == BANISHMENT_ACTION_NOTATION) {
		switch (QueryManagerConnection->setNotation(Data->GamemasterID, Data->CriminalName, Data->CriminalIPAddress, const_cast<char *>(GetBanishmentReason(Data->Reason)), Data->Comment, BanishmentID)) {
			case 0:
				DirectReply(Data->GamemasterID, "Notation for player %s inserted.", Data->CriminalName);
				Log("banish", "%s notiert zu %s: %s.\n", Data->GamemasterName, Data->CriminalName, Data->Comment);
				break;
			case 1:
				DirectReply(Data->GamemasterID, "A player with this name does not exist. Perhaps he/she has been renamed?");
				ok = false;
				break;
			case 2:
				DirectReply(Data->GamemasterID, "You may not report a god or gamemaster.");
				ok = false;
				break;
		}
	}

	if (ok && (Data->Action == BANISHMENT_ACTION_NAMELOCK || Data->Action == BANISHMENT_ACTION_NAMELOCK_BANISHMENT
			|| Data->Action == BANISHMENT_ACTION_NAMELOCK_BANISHMENT_FINAL_WARNING)) {
		switch (QueryManagerConnection->setNamelock(Data->GamemasterID, Data->CriminalName, Data->CriminalIPAddress, const_cast<char *>(GetBanishmentReason(Data->Reason)), Data->Comment)) {
			case 0:
				DirectReply(Data->GamemasterID, "Player %s reported for renaming.", Data->CriminalName);
				Log("banish", "%s meldet %s zur Namens\xe4nderung.\n", Data->GamemasterName, Data->CriminalName);
				break;
			case 1:
				DirectReply(Data->GamemasterID, "A player with this name does not exist. Perhaps he/she has already been renamed?");
				ok = false;
				break;
			case 2:
			case 4:
				DirectReply(Data->GamemasterID, "This name has already been approved.");
				ok = false;
				break;
			case 3:
				DirectReply(Data->GamemasterID, "This player has already been reported.");
				break;
		}
	}

	if (ok && (Data->Action == BANISHMENT_ACTION_BANISHMENT || Data->Action == BANISHMENT_ACTION_NAMELOCK_BANISHMENT
			|| Data->Action == BANISHMENT_ACTION_BANISHMENT_FINAL_WARNING || Data->Action == BANISHMENT_ACTION_NAMELOCK_BANISHMENT_FINAL_WARNING)) {
		bool FinalWarning = (Data->Action == BANISHMENT_ACTION_BANISHMENT_FINAL_WARNING
				|| Data->Action == BANISHMENT_ACTION_NAMELOCK_BANISHMENT_FINAL_WARNING);
		int Days;
		switch (QueryManagerConnection->banishAccount(Data->GamemasterID, Data->CriminalName, Data->CriminalIPAddress, const_cast<char *>(GetBanishmentReason(Data->Reason)), Data->Comment, FinalWarning, Days, BanishmentID)) {
			case 0:
				if (Days == -1) {
					DirectReply(Data->GamemasterID, "Account of player %s banished infinitely.", Data->CriminalName);
				} else if (FinalWarning) {
					DirectReply(Data->GamemasterID, "Account of player %s banished for %d days with final warning.", Data->CriminalName, Days);
				} else {
					DirectReply(Data->GamemasterID, "Account of player %s banished for %d days.", Data->CriminalName, Days);
				}
				LogoutReply(Data->CriminalName);
				Log("banish", "%s verbannt Account von Spieler %s.\n", Data->GamemasterName, Data->CriminalName);
				break;
			case 1:
				DirectReply(Data->GamemasterID, "A player with this name does not exist. Perhaps he/she has been renamed?");
				ok = false;
				break;
			case 2:
				DirectReply(Data->GamemasterID, "You may not report a god or gamemaster.");
				ok = false;
				break;
			case 3:
				DirectReply(Data->GamemasterID, "Player %s has already been banished.", Data->CriminalName);
				LogoutReply(Data->CriminalName);
				break;
		}
	}

	if (ok && Data->StatementID != 0) {
		if (Data->NumberOfStatements == 0 || Data->ReportedStatements == nullptr) {
			error("ProcessPunishmentOrder: Statements existieren nicht.\n");
			delete Data;
			return;
		}

		int NumberOfStatements = Data->NumberOfStatements;
		uint32_t StatementIDs[NumberOfStatements];
		uint32_t TimeStamps[NumberOfStatements];
		uint32_t CharacterIDs[NumberOfStatements];
		char Channels[NumberOfStatements][30];
		char Texts[NumberOfStatements][256];
		for (int i = 0; i < NumberOfStatements; i++) {
			StatementIDs[i] = (*Data->ReportedStatements)(i).StatementID;
			TimeStamps[i] = (*Data->ReportedStatements)(i).TimeStamp;
			CharacterIDs[i] = (*Data->ReportedStatements)(i).CharacterID;
			switch ((*Data->ReportedStatements)(i).Mode) {
				case TALK_SAY:
					strcpy(Channels[i], "Say");
					break;
				case TALK_WHISPER:
					strcpy(Channels[i], "Whisper");
					break;
				case TALK_YELL:
					strcpy(Channels[i], "Yell");
					break;
				case TALK_PRIVATE_MESSAGE:
				case TALK_GAMEMASTER_MESSAGE:
					strcpy(Channels[i], "Private Message");
					break;
				case TALK_CHANNEL_CALL:
				case TALK_GAMEMASTER_CHANNELCALL:
				case TALK_HIGHLIGHT_CHANNELCALL:
					switch ((*Data->ReportedStatements)(i).Channel) {
						case GUILD_CHANNEL:
							strcpy(Channels[i], "Guild Channel");
							break;
						case GAMEMASTER_CHANNEL:
							strcpy(Channels[i], "Gamemaster Channel");
							break;
						case TUTOR_CHANNEL:
							strcpy(Channels[i], "Tutor Channel");
							break;
						case GAME_CHANNEL:
							strcpy(Channels[i], "Game Chat");
							break;
						case TRADE_CHANNEL:
							strcpy(Channels[i], "Trade Channel");
							break;
						case REALLIFE_CHANNEL:
							strcpy(Channels[i], "Reallife Chat");
							break;
						case HELP_CHANNEL:
							strcpy(Channels[i], "Help Channel");
							break;
						default:
							strcpy(Channels[i], "Private Chat Channel");
							break;
					}
					break;
				case TALK_GAMEMASTER_REQUEST:
					strcpy(Channels[i], "Gamemaster Request");
					break;
				case TALK_GAMEMASTER_ANSWER:
					strcpy(Channels[i], "Gamemaster Answer");
					break;
				case TALK_PLAYER_ANSWER:
					strcpy(Channels[i], "Player Answer");
					break;
				case TALK_GAMEMASTER_BROADCAST:
					strcpy(Channels[i], "Broadcast");
					break;
				default:
					error("ProcessPunishmentOrder: Ung\xfcltiger Modus %d.\n", (*Data->ReportedStatements)(i).Mode);
					strcpy(Channels[i], "Unknown");
					break;
			}
			strcpy(Texts[i], (*Data->ReportedStatements)(i).Text);
		}

		switch (QueryManagerConnection->reportStatement(Data->GamemasterID, Data->CriminalName, const_cast<char *>(GetBanishmentReason(Data->Reason)), Data->Comment, BanishmentID, Data->StatementID, NumberOfStatements, StatementIDs, TimeStamps, CharacterIDs, Channels, Texts)) {
			case 0:
				if (Data->Action == BANISHMENT_ACTION_STATEMENT_REPORT) {
					DirectReply(Data->GamemasterID, "Statement of %s reported.", Data->CriminalName);
				}
				Log("banish", "%s meldet \xc4u\xdf" "erung von Spieler %s.\n", Data->GamemasterName, Data->CriminalName);
				break;
			case 1:
				if (Data->Action == BANISHMENT_ACTION_STATEMENT_REPORT) {
					DirectReply(Data->GamemasterID, "A player with this name does not exist. Perhaps he/she has been renamed?");
				}
				ok = false;
				break;
			case 2:
				if (Data->Action == BANISHMENT_ACTION_STATEMENT_REPORT) {
					DirectReply(Data->GamemasterID, "Statement has already been reported.");
				}
				ok = false;
				break;
		}
	}

	if (ok && Data->IPBanishment) {
		switch (QueryManagerConnection->banishIPAddress(Data->GamemasterID, Data->CriminalName, Data->CriminalIPAddress, const_cast<char *>(GetBanishmentReason(Data->Reason)), Data->Comment)) {
			case 0:
				DirectReply(Data->GamemasterID, "IP address of %s banished.", Data->CriminalName);
				LogoutReply(Data->CriminalName);
				Log("banish", "%s sperrt die IP-Adresse von %s.\n", Data->GamemasterName, Data->CriminalName);
				break;
			case 1:
				DirectReply(Data->GamemasterID, "A player with this name does not exist. Perhaps he/she has been renamed?");
				break;
			case 2:
				DirectReply(Data->GamemasterID, "You may not report a god or gamemaster.");
				break;
		}
	}

	delete Data;
}

void ProcessCharacterDeathOrder(void *RawData) {
	TCharacterDeathOrderData *Data = static_cast<TCharacterDeathOrderData *>(RawData);
	if (Data == nullptr) {
		error("ProcessCharacterDeathOrder: Keine Daten \xfc" "bergeben.\n");
		return;
	}

	if (QueryManagerConnection->logCharacterDeath(Data->CharacterID, Data->Level, Data->Offender, Data->Remark, Data->Unjustified, Data->Time) != 0) {
		error("ProcessCharacterDeathOrder: Protokollierung fehlgeschlagen.\n");
	}

	delete Data;
}

void ProcessAddBuddyOrder(void *RawData) {
	TBuddyOrderData *Data = static_cast<TBuddyOrderData *>(RawData);
	if (Data == nullptr) {
		error("ProcessAddBuddyOrder: Keine Daten \xfc" "bergeben.\n");
		return;
	}

	if (QueryManagerConnection->addBuddy(Data->AccountID, Data->Buddy) != 0) {
		error("ProcessAddBuddyOrder: Aufnahme fehlgeschlagen.\n");
	}

	delete Data;
}

void ProcessRemoveBuddyOrder(void *RawData) {
	TBuddyOrderData *Data = static_cast<TBuddyOrderData *>(RawData);
	if (Data == nullptr) {
		error("ProcessRemoveBuddyOrder: Keine Daten \xfc" "bergeben.\n");
		return;
	}

	if (QueryManagerConnection->removeBuddy(Data->AccountID, Data->Buddy) != 0) {
		error("ProcessRemoveBuddyOrder: Entfernen fehlgeschlagen.\n");
	}

	delete Data;
}

void ProcessDecrementIsOnlineOrder(void *RawData) {
	uint32_t CharacterID = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(RawData));
	if (QueryManagerConnection->decrementIsOnline(CharacterID) != 0) {
		error("ProcessDecrementIsOnlineOrder: Verringerung fehlgeschlagen.\n");
	}
}

int WriterThreadLoop(void *) {
	while (true) {
		TWriterThreadOrderType OrderType;
		void *Data;
		GetOrder(OrderType, Data);
		switch (OrderType) {
			case ORDER_TERMINATE:
				return 0;
			case ORDER_LOGOUT:
				ProcessLogoutOrder(Data);
				break;
			case ORDER_PLAYERLIST:
				ProcessPlayerlistOrder(Data);
				break;
			case ORDER_KILLSTATISTICS:
				ProcessKillStatisticsOrder(Data);
				break;
			case ORDER_PUNISHMENT:
				ProcessPunishmentOrder(Data);
				break;
			case ORDER_CHARACTERDEATH:
				ProcessCharacterDeathOrder(Data);
				break;
			case ORDER_ADDBUDDY:
				ProcessAddBuddyOrder(Data);
				break;
			case ORDER_REMOVEBUDDY:
				ProcessRemoveBuddyOrder(Data);
				break;
			case ORDER_DECREMENTISONLINE:
				ProcessDecrementIsOnlineOrder(Data);
				break;
			case ORDER_SAVEPLAYERDATA:
				SavePlayerPoolSlots();
				break;
			default:
				error("WriterThreadLoop: Unbekanntes Kommando %d.\n", OrderType);
				break;
		}
	}
}

void ProcessBroadcastReply(void *RawData) {
	char *Data = static_cast<char *>(RawData);
	if (Data == nullptr) {
		error("ProcessBroadcastReply: Keine Daten \xfc" "bergeben.\n");
		return;
	}

	// the message itself is the format string, as in the original
	BroadcastMessage(TALK_STATUS_MESSAGE, Data);
	delete[] Data;
}

void ProcessDirectReply(void *RawData) {
	TDirectReplyData *Data = static_cast<TDirectReplyData *>(RawData);
	if (Data == nullptr) {
		error("ProcessDirectReply: Keine Daten \xfc" "bergeben.\n");
		return;
	}

	TPlayer *pl = GetPlayer(Data->CharacterID);
	if (pl != nullptr) {
		SendMessage(pl->GetConnection(), TALK_INFO_MESSAGE, "%s", Data->Message);
	}

	delete Data;
}

void ProcessLogoutReply(void *RawData) {
	char *CharacterName = static_cast<char *>(RawData);
	if (CharacterName == nullptr) {
		error("ProcessLogoutReply: Keine Daten \xfc" "bergeben.\n");
		return;
	}

	TPlayer *pl = GetPlayer(CharacterName);
	if (pl != nullptr) {
		GraphicalEffect(pl->GetCreatureObject(), EFFECT_MAGIC_GREEN);
		pl->StartLogout(true, true);
	}

	delete[] CharacterName;
}

void ProcessWriterThreadReplies() {
	while (true) {
		TWriterThreadReplyType ReplyType;
		void *Data;
		if (!GetReply(ReplyType, Data)) {
			break;
		}

		switch (ReplyType) {
			case REPLY_BROADCAST:
				ProcessBroadcastReply(Data);
				break;
			case REPLY_DIRECT:
				ProcessDirectReply(Data);
				break;
			case REPLY_LOGOUT:
				ProcessLogoutReply(Data);
				break;
			default:
				error("ProcessWriterThreadReplies: Unbekannte R\xfc" "ckmeldung %d.\n", ReplyType);
				break;
		}
	}
}

void ClearPlayers() {
	int NumberOfAffectedPlayers;
	if (QueryManagerConnection->clearIsOnline(NumberOfAffectedPlayers) != 0) {
		error("ClearPlayers: Kann IsOnline-Flags nicht l\xf6schen.\n");
		return;
	}

	if (NumberOfAffectedPlayers != 0) {
		error("ClearPlayers: %d Spieler waren als eingeloggt markiert.\n", NumberOfAffectedPlayers);
	}
}

// The original is declared throw(const char *). C++17 has no dynamic exception
// specifications, so an exception of another type now propagates instead of
// ending in std::unexpected().
void InitWriter() {
	QueryManagerConnection = new TQueryManagerConnection(std::max(0x4000, MaxPlayers * 66 + 2));
	if (!QueryManagerConnection->isConnected()) {
		throw "cannot connect to query manager";
	}

	ClearPlayers();

	InitProtocol();
	ProtocolThread = StartThread(ProtocolThreadLoop, nullptr, false);
	if (ProtocolThread == 0) {
		throw "cannot start protocol thread";
	}

	InitWriterBuffers();
	WriterThread = StartThread(WriterThreadLoop, nullptr, false);
	if (WriterThread == 0) {
		throw "cannot start writer thread";
	}
}

void AbortWriter() {
	if (WriterThread != 0) {
		WriterThread = 0;
		// The original calls pthread_cancel(WriterThread) here, after it has
		// cleared WriterThread, so it asks for thread 0 to be cancelled.
		// LinuxThreads answered ESRCH and cancelled nothing; NPTL would
		// dereference a null thread descriptor. The call is left out, which
		// keeps the original effect.
		OrderBufferEmpty.up();
	}
}

void ExitWriter() {
	if (ProtocolThread != 0) {
		InsertProtocolOrder("", "");
		JoinThread(ProtocolThread);
		ProtocolThread = 0;
	}

	if (WriterThread != 0) {
		TerminateWriterOrder();
		JoinThread(WriterThread);
		WriterThread = 0;
	}

	delete QueryManagerConnection;
}
