#include <algorithm>
#include <cerrno>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <exception>
#include <fcntl.h>
#include <poll.h>
#include <signal.h>
#include <time.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "threads.hh"
#include "script.hh"
#include "dbfuncs.hh"
#include "query.hh"
#include "config.hh"
#include "objects.hh"
#include "map.hh"
#include "info.hh"
#include "cr.hh"
#include "containr.hh"
#include "operate.hh"
#include "connections.hh"
#include "vlong.hh"
#include "TRSAPublicKey.hh"
#include "utils.hh"
#include "time.hh"
#include "TXTEASymmetricKey.hh"
#include "RSA_PrivateKey.hh"
#include "communication.hh"
#include "TRSAPrivateKey.hh"
#include "crplayer.hh"
#include "mudinc.hh"
#include "shm.hh"
#include "writer.hh"

struct TWaitinglistEntry {
	TWaitinglistEntry *Next;
	char Name[30];
	uint32_t NextTry;
	bool FreeAccount;
	bool Newbie;
	bool Sleeping;
};

static int TERMINALVERSION[3] = {
	770, 770, 770,
};	// communication.c:39
int TCPSocket;	// communication.c:52
Thread AcceptorThread;	// communication.c:53
pid_t AcceptorThreadPID;	// communication.c:54
int ActiveConnections;	// communication.c:55
Semaphore CommunicationThreadMutex(1);	// communication.c:56
bool UseOwnStacks;	// communication.c:57
unsigned char CommunicationThreadStacks[72089600];	// communication.c:58
pid_t LastUsingCommunicationThread[1100];	// communication.c:59
int FreeCommunicationThreadStacks[1100];	// communication.c:60
int NumberOfFreeCommunicationThreadStacks;	// communication.c:61
TRSAPrivateKey PrivateKey;	// communication.c:62
Semaphore RSAMutex(1);	// communication.c:63
TQueryManagerConnectionPool QueryManagerConnectionPool(10);	// communication.c:66
int LoadHistory[360];	// communication.c:69
int LoadHistoryPointer;	// communication.c:70
int TotalLoad;	// communication.c:71
int TotalSend;	// communication.c:71
int TotalRecv;	// communication.c:71
uint32_t LagEnd;	// communication.c:72
uint32_t EarliestFreeAccountAdmissionRound;	// communication.c:73
store<TWaitinglistEntry, 100> Waitinglist;	// communication.c:76
TWaitinglistEntry *WaitinglistHead;	// communication.c:77

// Not in the original: the login time-out timer of each communication
// thread, which stands in for the per-thread alarm() of LinuxThreads, and
// whether timer_create gave this thread one (alarm() could not fail).
static thread_local timer_t LoginTimer;
static thread_local bool LoginTimerCreated;

// 0x080dbbd0  communication.c:88
void InitCommunicationThreadStacks() {
	if (!UseOwnStacks) {
		return;
	}

	memset(CommunicationThreadStacks, 0xAA, sizeof(CommunicationThreadStacks));
	for (int i = 0; i < 1100; i++) {
		LastUsingCommunicationThread[i] = 0;
		FreeCommunicationThreadStacks[i] = i;
	}
	NumberOfFreeCommunicationThreadStacks = 1100;
}

// 0x080dbc30  communication.c:104
void GetCommunicationThreadStack(int &StackNumber, void *&Stack) {
	if (!UseOwnStacks) {
		error("GetCommunicationThreadStack: Bibliothek unterst\xfctzt keine eigenen Stacks.\n");
		StackNumber = -1;
		Stack = nullptr;
		return;
	}

	CommunicationThreadMutex.down();
	StackNumber = -1;
	Stack = nullptr;
	for (int i = 0; i < NumberOfFreeCommunicationThreadStacks; i++) {
		int Number = FreeCommunicationThreadStacks[i];
		// The original tests kill(pid, 0) == -1: under LinuxThreads the stored
		// value is the PID of the thread that last ran on this stack.
		if (LastUsingCommunicationThread[Number] == 0
				|| tgkill(getpid(), LastUsingCommunicationThread[Number], 0) == -1) {
			StackNumber = Number;
			Stack = &CommunicationThreadStacks[Number * 65536];
			NumberOfFreeCommunicationThreadStacks--;
			FreeCommunicationThreadStacks[i] = FreeCommunicationThreadStacks[NumberOfFreeCommunicationThreadStacks];
			break;
		}
	}
	CommunicationThreadMutex.up();
}

// 0x080dbd10  communication.c:131
void AttachCommunicationThreadStack(int StackNumber) {
	// The original stores getpid(), under LinuxThreads the ID of the calling thread.
	LastUsingCommunicationThread[StackNumber] = gettid();
}

// 0x080dbd30  communication.c:139
void ReleaseCommunicationThreadStack(int StackNumber) {
	CommunicationThreadMutex.down();
	FreeCommunicationThreadStacks[NumberOfFreeCommunicationThreadStacks] = StackNumber;
	NumberOfFreeCommunicationThreadStacks++;
	CommunicationThreadMutex.up();
}

// 0x080dbd70  communication.c:150
void ExitCommunicationThreadStacks() {
	if (!UseOwnStacks) {
		return;
	}

	if (NumberOfFreeCommunicationThreadStacks != 1100) {
		error("FreeCommunicationThreadStacks: Nicht alle Stacks freigegeben.\n");
	}

	int HighestStackAddress = -1;
	int LowestStackAddress = 65536;
	for (int Stack = 0; Stack < 1100; Stack++) {
		for (int i = 0; i < 65536; i++) {
			if (CommunicationThreadStacks[Stack * 65536 + i] != 0xAA) {
				if (i > HighestStackAddress) {
					HighestStackAddress = i;
				}
				if (i < LowestStackAddress) {
					LowestStackAddress = i;
				}
			}
		}
	}

	if (65536 - (HighestStackAddress - LowestStackAddress) < 32768) {
		error("Maximale Stack-Ausdehnung: %d..%d\n", LowestStackAddress, HighestStackAddress);
	}
}

// 0x080dbe10  communication.c:175
void InitLoadHistory() {
	for (int i = 0; i < 360; i++) {
		LoadHistory[i] = 0;
	}
	LoadHistoryPointer = 0;
	TotalLoad = 0;
	TotalSend = 0;
	TotalRecv = 0;
	LagEnd = 0;
	EarliestFreeAccountAdmissionRound = 0;
	InitLog("netload");
}

// 0x080dbe70  communication.c:190
bool LagDetected() {
	return RoundNr <= LagEnd;
}

// 0x080dbe90  communication.c:198
void NetLoad(int Amount, bool Send) {
	CommunicationThreadMutex.down();
	if (Send) {
		TotalSend += Amount;
	} else {
		TotalRecv += Amount;
	}
	CommunicationThreadMutex.up();
}

// 0x080dbee0  communication.c:209
void NetLoadSummary() {
	CommunicationThreadMutex.down();
	Log("netload", "gesendet:  %d Bytes.\n", TotalSend);
	Log("netload", "empfangen: %d Bytes.\n", TotalRecv);
	TotalSend = 0;
	TotalRecv = 0;
	CommunicationThreadMutex.up();
}

// 0x080dbf50  communication.c:223
void NetLoadCheck() {
	static int LastRecv = 0;

	int DeltaRecv = TotalRecv - LastRecv;
	LastRecv = TotalRecv;
	if (DeltaRecv < 0) {
		return;
	}

	int CurrentLoad;
	if (GetPlayersOnline() != 0) {
		CurrentLoad = DeltaRecv / GetPlayersOnline();
	} else {
		CurrentLoad = 0;
	}

	TotalLoad -= LoadHistory[LoadHistoryPointer];
	LoadHistory[LoadHistoryPointer] = CurrentLoad;
	TotalLoad += CurrentLoad;
	LoadHistoryPointer++;
	if (LoadHistoryPointer == 360) {
		LoadHistoryPointer = 0;
	}

	if (RoundNr < 3600 || GetPlayersOnline() < 50) {
		return;
	}

	if (CurrentLoad < TotalLoad / 720) {
		Log("game", "Lag erkannt!\n");
		LagEnd = RoundNr + 30;

		int Delay;
		if (PremiumPlayerBuffer != 0) {
			Delay = (GetPlayersOnline() - (MaxPlayers - 2 * PremiumPlayerBuffer)) * 30 / PremiumPlayerBuffer;
		} else {
			Delay = 60;
		}
		EarliestFreeAccountAdmissionRound = std::max<uint32_t>(EarliestFreeAccountAdmissionRound, RoundNr + Delay);

		TConnection *Connection = GetFirstConnection();
		while (Connection != nullptr) {
			if (Connection->IsOnline()) {
				Connection->EmergencyPing();
			}
			Connection = GetNextConnection();
		}
	}
}

// 0x080dc0c0  communication.c:270
bool WriteToSocket(TConnection *Connection, unsigned char *Buffer, int Size) {
	while (Size % 8 != 0) {
		Buffer[Size + 2] = static_cast<unsigned char>(rand_r(&Connection->RandomSeed));
		Size++;
	}

	for (int Offset = 2; Offset < Size + 2; Offset += 8) {
		Connection->GetSymmetricKey().encrypt(Buffer + Offset);
	}

	TWriteBuffer WriteBuffer(Buffer, Size + 2);
	WriteBuffer.writeWord(static_cast<unsigned short>(Size));

	int Chances = 50;
	int nleft = Size + 2;
	while (nleft > 0) {
		int nwritten = static_cast<int>(write(Connection->GetSocket(), Buffer, nleft));
		if (nwritten == -1 && errno == EINTR) {
			continue;
		}

		if (nwritten == -1 && errno == EAGAIN && Chances > 0) {
			DelayThread(0, 100000);
			Chances--;
			continue;
		}

		if (nwritten == -1 && (errno == ECONNRESET || errno == EPIPE || errno == EAGAIN)) {
			Log("game", "Verbindung an Socket %d zusammengebrochen.\n", Connection->GetSocket());
			return false;
		}

		if (nwritten <= 0) {
			error("WriteToSocket: Fehler %d beim Senden an Socket %d.\n", errno, Connection->GetSocket());
			return false;
		}

		nleft -= nwritten;
		Buffer += nwritten;
	}

	NetLoad(Size + 2 + 48, true);
	return true;
}

// 0x080dc280  communication.c:311
bool SendLoginMessage(TConnection *Connection, int Type, char *Message, int WaitingTime) {
	if (Type < 20 || Type > 22) {
		error("SendLoginMessage: Ung\xfcltiger Meldungstyp %d.\n", Type);
		return true;
	}

	if (Message == nullptr) {
		error("SendLoginMessage: Message ist NULL.\n");
		return true;
	}

	if (Type == 22 && !in(WaitingTime, 1, 255)) {
		error("SendLoginMessage: Ung\xfcltige Wartezeit %d.\n", WaitingTime);
		return true;
	}

	try {
		unsigned char Data[302];
		TWriteBuffer Buffer(Data + 2, 300);
		Buffer.writeWord(0);
		Buffer.writeByte(static_cast<unsigned char>(Type));
		if (strlen(Message) > 290) {
			error("SendLoginMessage: Botschaft zu lang (%s).\n", Message);
			return true;
		}

		Buffer.writeString(Message);
		if (Type == 22) {
			Buffer.writeByte(static_cast<unsigned char>(WaitingTime));
		}

		int Size = Buffer.getDataSize();
		Buffer.rewind();
		Buffer.writeWord(static_cast<unsigned short>(Size - 2));
		return WriteToSocket(Connection, Data, Size);
	} catch (const char *s) {
		error("SendLoginMessage: Fehler beim F\xfcllen des Puffers (%s)\n", s);
	}

	return true;
}

// 0x080dc430  communication.c:353
bool SendData(TConnection *Connection) {
	if (Connection == nullptr) {
		error("SendData: Verbindung ist NULL.\n");
		return false;
	}

	int DataSize = Connection->NextToCommit - Connection->NextToSend;
	int PacketSize = (DataSize + 7) / 8 * 8 + 2;
	// The original declares PacketSize + 2 bytes, and WriteToSocket then pads
	// PacketSize (never a multiple of 8) with 6 more bytes behind the end of
	// the array. The 6 bytes are part of the array here; the write buffer
	// keeps the original limit.
	unsigned char Buffer[PacketSize + 2 + 6];
	TWriteBuffer WriteBuffer(Buffer, PacketSize + 2);
	WriteBuffer.writeWord(0);
	WriteBuffer.writeWord(static_cast<unsigned short>(DataSize));

	int Offset = Connection->NextToSend % 16384;
	if (Offset + DataSize > 16384) {
		WriteBuffer.writeBytes(&Connection->OutData[Offset], 16384 - Offset);
		WriteBuffer.writeBytes(Connection->OutData, Offset + DataSize - 16384);
	} else {
		WriteBuffer.writeBytes(&Connection->OutData[Offset], DataSize);
	}

	if (!WriteToSocket(Connection, Buffer, PacketSize)) {
		return false;
	}

	Connection->NextToSend += DataSize;
	return true;
}

// 0x080dc580  communication.c:385
bool SendData(TConnection *Connection, char *Data, int Size) {
	if (Connection == nullptr) {
		error("SendData: Connection ist NULL.\n");
		return false;
	}

	if (Data == nullptr) {
		error("SendData: Data ist NULL.\n");
		return true;
	}

	memcpy(&Connection->OutData[2], Data, Size);
	Connection->NextToSend = 0;
	Connection->NextToCommit = Size + 2;
	return SendData(Connection);
}

// 0x080dc610  communication.c:410
bool GetWaitinglistEntry(char *Name, uint32_t &NextTry, bool &FreeAccount, bool &Newbie) {
	CommunicationThreadMutex.down();
	TWaitinglistEntry *Search = WaitinglistHead;
	while (Search != nullptr) {
		if (stricmp(Name, Search->Name, -1) == 0) {
			NextTry = Search->NextTry;
			FreeAccount = Search->FreeAccount;
			Newbie = Search->Newbie;
			CommunicationThreadMutex.up();
			return true;
		}
		Search = Search->Next;
	}

	CommunicationThreadMutex.up();
	return false;
}

// 0x080dc6b0  communication.c:448
void InsertWaitinglistEntry(char *Name, uint32_t NextTry, bool FreeAccount, bool Newbie) {
	CommunicationThreadMutex.down();
	TWaitinglistEntry *Last = nullptr;
	TWaitinglistEntry *Search = WaitinglistHead;
	while (Search != nullptr) {
		if (stricmp(Name, Search->Name, -1) == 0) {
			Search->NextTry = NextTry;
			Search->FreeAccount = FreeAccount;
			Search->Newbie = Newbie;
			CommunicationThreadMutex.up();
			return;
		}
		Last = Search;
		Search = Search->Next;
	}

	TWaitinglistEntry *Entry = Waitinglist.getFreeItem();
	strcpy(Entry->Name, Name);
	Entry->NextTry = NextTry;
	Entry->FreeAccount = FreeAccount;
	Entry->Newbie = Newbie;
	Entry->Next = nullptr;
	Entry->Sleeping = false;
	if (Last != nullptr) {
		Last->Next = Entry;
	} else {
		WaitinglistHead = Entry;
	}
	CommunicationThreadMutex.up();

	Log("queue", "F\xfcge %s in die Warteschlange ein.\n", Name);
}

// 0x080dc7b0  communication.c:476
void DeleteWaitinglistEntry(char *Name) {
	CommunicationThreadMutex.down();
	TWaitinglistEntry *Entry = nullptr;
	if (WaitinglistHead != nullptr) {
		if (stricmp(Name, WaitinglistHead->Name, -1) == 0) {
			Entry = WaitinglistHead;
			WaitinglistHead = Entry->Next;
		} else {
			TWaitinglistEntry *Search = WaitinglistHead;
			while (Search != nullptr && Search->Next != nullptr) {
				if (stricmp(Name, Search->Next->Name, -1) == 0) {
					Entry = Search->Next;
					Search->Next = Entry->Next;
					break;
				}
				Search = Search->Next;
			}
		}
	}

	if (Entry != nullptr) {
		Waitinglist.deleteItem(Entry);
	}
	CommunicationThreadMutex.up();
}

// 0x080dc890  communication.c:498
int GetWaitinglistPosition(char *Name, bool FreeAccount, bool Newbie) {
	int FreeNewbies = 0;
	int FreeVeterans = 0;
	int PremiumNewbies = 0;
	int PremiumVeterans = 0;

	CommunicationThreadMutex.down();
	while (WaitinglistHead != nullptr && RoundNr > WaitinglistHead->NextTry + 60) {
		TWaitinglistEntry *Entry = WaitinglistHead;
		Log("queue", "L\xf6sche %s aus der Warteliste wegen Inaktivit\xe4t.\n", Entry->Name);
		WaitinglistHead = WaitinglistHead->Next;
		Waitinglist.deleteItem(Entry);
	}

	TWaitinglistEntry *Search = WaitinglistHead;
	while (Search != nullptr) {
		if (stricmp(Name, Search->Name, -1) == 0) {
			break;
		}

		if (!Search->Sleeping) {
			if (RoundNr > Search->NextTry + 5) {
				Search->Sleeping = true;
			} else if (Search->FreeAccount) {
				if (Search->Newbie) {
					FreeNewbies++;
				} else {
					FreeVeterans++;
				}
			} else {
				if (Search->Newbie) {
					PremiumNewbies++;
				} else {
					PremiumVeterans++;
				}
			}
		}
		Search = Search->Next;
	}
	CommunicationThreadMutex.up();

	if (FreeAccount) {
		if (Newbie) {
			return FreeNewbies + FreeVeterans + PremiumNewbies + PremiumVeterans + 1;
		}

		if (GetNewbiesOnline() < MaxNewbies - PremiumNewbieBuffer) {
			return FreeNewbies + FreeVeterans + PremiumVeterans + 1;
		}

		return FreeVeterans + PremiumVeterans + 1;
	}

	if (Newbie) {
		return PremiumNewbies + PremiumVeterans + 1;
	}

	if (GetNewbiesOnline() < MaxNewbies) {
		return PremiumNewbies + PremiumVeterans + 1;
	}

	return PremiumVeterans + 1;
}

// 0x080dca60  communication.c:533
int CheckWaitingTime(char *Name, TConnection *Connection, bool FreeAccount, bool Newbie) {
	const char *Reason = nullptr;
	int Position = GetWaitinglistPosition(Name, FreeAccount, Newbie);
	int WaitingTime = 0;

	int PlayersOnline = GetPlayersOnline();
	int NewbiesOnline = GetNewbiesOnline();

	if (PlayersOnline > GetOrderBufferSpace() - Position) {
		print(3, "Order-Puffer ist fast voll.\n");
		Reason = "The server is overloaded.";
		WaitingTime = Position / 2 + 10;
	} else if (FreeAccount && RoundNr < EarliestFreeAccountAdmissionRound) {
		print(3, "Keine FreeAccounts zugelassen nach MassKick.\n");
		Reason = "The server is overloaded.\nOnly players with premium accounts\nare admitted at the moment.";
		WaitingTime = Position / 2 + static_cast<int>(EarliestFreeAccountAdmissionRound - RoundNr);
	} else if (!FreeAccount && PlayersOnline > MaxPlayers - Position) {
		print(3, "Zu viele Spieler online.\n");
		Reason = "There are too many players online.";
		WaitingTime = Position / 2 + 3;
	} else if (FreeAccount && PlayersOnline > MaxPlayers - PremiumPlayerBuffer - Position) {
		print(3, "Kein Platz mehr f\xfcr FreeAccounts.\n");
		Reason = "Too many players online.\nOnly players with premium accounts\nare admitted at the moment.";
		WaitingTime = Position / 2 + 5;
	} else if (Newbie && !FreeAccount && NewbiesOnline > MaxNewbies - Position) {
		print(3, "Kein Platz mehr f\xfcr Newbies.\n");
		Reason = "There are too many players online\non the beginners' island, Rookgaard.";
		WaitingTime = Position / 2 + 3;
	} else if (Newbie && FreeAccount && NewbiesOnline > MaxNewbies - PremiumNewbieBuffer - Position) {
		print(3, "Kein Platz mehr f\xfcr Newbies mit FreeAccount.\n");
		Reason = "There are too many players online\non the beginners' island, Rookgaard.\nOnly players with premium accounts\nare admitted at the moment.";
		WaitingTime = Position / 2 + 5;
	}

	if (WaitingTime > 240) {
		WaitingTime = 240;
	}

	if (WaitingTime > 0) {
		char Message[250];
		snprintf(Message, 250, "%s\n\nYou are at place %d on the waiting list.", Reason, Position);
		SendLoginMessage(Connection, 22, Message, WaitingTime);
	}

	return WaitingTime;
}

// 0x080dccf0  communication.c:588
int ReadFromSocket(TConnection *Connection, unsigned char *Buffer, int Size) {
	int Chances = 50;
	int nleft = Size;
	while (nleft > 0) {
		int nread = static_cast<int>(read(Connection->GetSocket(), Buffer, nleft));
		if (nread == 0) {
			break;
		}

		if (nread < 0) {
			if (errno == EINTR) {
				continue;
			}

			if (errno != EAGAIN || nleft == Size || Chances == 0) {
				return nread;
			}

			Chances--;
			usleep(100000);
			continue;
		}

		nleft -= nread;
		Buffer += nread;
	}

	return Size - nleft;
}

// 0x080dcd90  communication.c:615
bool CallGameThread(TConnection *Connection) {
	if (GameRunning()) {
		Connection->WaitingForACK = true;
		// The original calls kill(GetGameThreadPID(), SIGUSR1). The game thread
		// is the main thread, so its thread ID is the process ID.
		if (tgkill(getpid(), GetGameThreadPID(), SIGUSR1) != 0) {
			error("CallGameThread: Can't send SIGUSR1 to pid %d\n", GetGameThreadPID());
			SendLoginMessage(Connection, 20, "The server is not online.\nPlease try again later.", -1);
			return false;
		}
	}

	return true;
}

// 0x080dce10  communication.c:631
bool CheckConnection(TConnection *Connection) {
	pollfd Status;
	Status.fd = Connection->GetSocket();
	Status.events = POLLIN;
	if (poll(&Status, 1, 0) < 0) {
		return false;
	}

	return (Status.revents & POLLIN) == 0;
}

// 0x080dce70  communication.c:643
TPlayerData *PerformRegistration(TConnection *Connection, char *PlayerName, uint32_t AccountID, char *PlayerPassword, bool GamemasterClient) {
	TQueryManagerPoolConnection QueryManagerConnection(&QueryManagerConnectionPool);
	if (QueryManagerConnection.operator->() == nullptr) {
		error("PerformRegistration: Kann Verbindung zum Query-Manager nicht herstellen.\n");
		SendLoginMessage(Connection, 20, "Internal error, closing connection.", -1);
		return nullptr;
	}

	if (!CheckConnection(Connection)) {
		return nullptr;
	}

	print(3, "Spieler \"%s\" loggt ein.\n", PlayerName);

	uint32_t CharacterID;
	int Sex, NumberOfBuddies;
	bool PremiumAccountActivated;
	unsigned char Rights[12];
	uint32_t BuddyIDs[100];
	char Guild[31], Rank[31], Title[31], BuddyNames[100][30];
	memset(Rights, 0, sizeof(Rights));

	switch (QueryManagerConnection->loginGame(AccountID, PlayerName, PlayerPassword,
			Connection->GetIPAddress(), PrivateWorld, false, GamemasterClient,
			CharacterID, Sex, Guild, Rank, Title, NumberOfBuddies, BuddyIDs,
			BuddyNames, Rights, PremiumAccountActivated)) {
	case 0:
		break;

	case 1:
		print(3, "Spieler existiert nicht.\n");
		SendLoginMessage(Connection, 20, "Character doesn't exist.\nCreate a new character on the Tibia website\nat \"www.tibia.com\".", -1);
		return nullptr;

	case 2:
		print(3, "Spieler wurde gel\xf6scht.\n");
		SendLoginMessage(Connection, 20, "Character doesn't exist.\nCreate a new character on the Tibia website.", -1);
		return nullptr;

	case 3:
		print(3, "Spieler lebt nicht auf dieser Welt.\n");
		SendLoginMessage(Connection, 20, "Character doesn't live on this world.\nPlease login on the right world.", -1);
		return nullptr;

	case 4:
		print(3, "Spieler ist nicht eingeladen.\n");
		SendLoginMessage(Connection, 20, "This world is private and you have not been invited to play on it.", -1);
		return nullptr;

	case 6:
		Log("game", "Falsches Pa\xdfwort f\xfcr Spieler %s; Login von %s.\n", PlayerName, Connection->GetIPAddress());
		SendLoginMessage(Connection, 20, "Accountnumber or password is not correct.", -1);
		return nullptr;

	case 7:
		Log("game", "Spieler %s blockiert; Login von %s.\n", PlayerName, Connection->GetIPAddress());
		SendLoginMessage(Connection, 20, "Account disabled for five minutes. Please wait.", -1);
		return nullptr;

	case 8:
		Log("game", "Account von Spieler %s wurde gel\xf6scht.\n", PlayerName);
		SendLoginMessage(Connection, 20, "Accountnumber or password is not correct.", -1);
		return nullptr;

	case 9:
		Log("game", "IP-Adresse %s f\xfcr Spieler %s blockiert.\n", Connection->GetIPAddress(), PlayerName);
		SendLoginMessage(Connection, 20, "IP address blocked for 30 minutes. Please wait.", -1);
		return nullptr;

	case 10:
		print(3, "Account ist verbannt.\n");
		SendLoginMessage(Connection, 20, "Your account is banished.", -1);
		return nullptr;

	case 11:
		print(3, "Character muss umbenannt werden.\n");
		SendLoginMessage(Connection, 20, "Your character is banished because of his/her name.", -1);
		return nullptr;

	case 12:
		print(3, "IP-Adresse ist gesperrt.\n");
		SendLoginMessage(Connection, 20, "Your IP address is banished.", -1);
		return nullptr;

	case 13:
		print(3, "Schon andere Charaktere desselben Accounts eingeloggt.\n");
		SendLoginMessage(Connection, 20, "You may only login with one character\nof your account at the same time.", -1);
		return nullptr;

	case 14:
		print(3, "Login mit Gamemaster-Client auf Nicht-Gamemaster-Account.\n");
		SendLoginMessage(Connection, 20, "You may only login with a Gamemaster account.", -1);
		return nullptr;

	case 15:
		Log("game", "Falsche Accountnummer %lu f\xfcr %s.\n", static_cast<unsigned long>(AccountID), PlayerName);
		SendLoginMessage(Connection, 20, "Login failed due to corrupt data.", -1);
		return nullptr;

	case -1:
		SendLoginMessage(Connection, 20, "Internal error, closing connection.", -1);
		return nullptr;

	default:
		error("PerformRegistration: Unbekannter R\xfc" "ckgabewert vom QueryManager.\n");
		SendLoginMessage(Connection, 20, "Internal error, closing connection.", -1);
		return nullptr;
	}

	if (!CheckConnection(Connection)) {
		QueryManagerConnection->decrementIsOnline(CharacterID);
		return nullptr;
	}

	if (AccountID == 0) {
		error("PerformRegistration: Spieler %s wurde noch keinem Account zugewiesen.\n", PlayerName);
		SendLoginMessage(Connection, 20, "Character is not assigned to an account.\nPerform this on the Tibia website\nat \"www.tibia.com\".", -1);
		return nullptr;
	}

	PlayerName[29] = 0;
	if (PremiumAccountActivated) {
		SendLoginMessage(Connection, 21, "Your Premium Account is now activated.\nHave a lot of fun in Tibia.", -1);
	}

	Log("game", "Spieler %s loggt ein an Socket %d von %s.\n", PlayerName, Connection->GetSocket(), Connection->GetIPAddress());

	TPlayerData *PlayerData = AssignPlayerPoolSlot(CharacterID, true);
	if (PlayerData == nullptr) {
		error("PerformRegistration: Kann keinen Slot f\xfcr Spielerdaten zuweisen.\n");
		QueryManagerConnection->decrementIsOnline(CharacterID);
		SendLoginMessage(Connection, 20, "There are too many players online.\nPlease try again later.", -1);
		return nullptr;
	}

	// The original compares with getpid(), which under LinuxThreads is the calling thread.
	bool ReadOnly = PlayerData->Locked != gettid();
	if (!ReadOnly || PlayerData->AccountID == 0) {
		PlayerData->AccountID = AccountID;
		PlayerData->Sex = Sex;
		strcpy(PlayerData->Name, PlayerName);
		memcpy(PlayerData->Rights, Rights, sizeof(PlayerData->Rights));
		strcpy(PlayerData->Guild, Guild);
		strcpy(PlayerData->Rank, Rank);
		strcpy(PlayerData->Title, Title);
	}

	if (!ReadOnly && PlayerData->Buddies == 0) {
		PlayerData->Buddies = NumberOfBuddies;
		for (int i = 0; i < NumberOfBuddies; i++) {
			PlayerData->Buddy[i] = BuddyIDs[i];
			strcpy(PlayerData->BuddyName[i], BuddyNames[i]);
		}
	}

	return PlayerData;
}

// 0x080dd5b0  communication.c:800
bool HandleLogin(TConnection *Connection) {
	TReadBuffer Buffer(Connection->InData + 2, 2046);
	unsigned char Command;
	try {
		Command = Buffer.readByte();
	} catch (const char *s) {
		error("HandleLogin: Fehler beim Auslesen des Kommandos (%s).\n", s);
		return false;
	}

	if (Command != 10) {
		print(3, "Ung\xfcltiges Init-Kommando %d.\n", Command);
		return false;
	}

	unsigned short TerminalType, TerminalVersion;
	bool GamemasterClient;
	uint32_t AccountID;
	char PlayerName[31];
	char PlayerPassword[31];
	// Not in bin/game: a 7.72 client sends terminal type and version in
	// clear before the RSA block (command, 2 words, 128 bytes: 133 bytes),
	// and leaves them out of the block. bin/game reads only the 7.70 layout
	// (command, 128 bytes: 129 bytes), which is kept unchanged for every
	// other packet size.
	bool ClientLayout772 = (Connection->InDataSize == 1 + 4 + 128);
	try {
		if (ClientLayout772) {
			TerminalType = Buffer.readWord();
			TerminalVersion = Buffer.readWord();
		}
		unsigned char AssymmetricData[128];
		Buffer.readBytes(AssymmetricData, 128);

		RSAMutex.down();
		try {
			PrivateKey.decrypt(AssymmetricData);
		} catch (const char *s) {
			error("HandleLogin: Fehler beim Entschl\xfcsseln (%s).\n", s);
			RSAMutex.up();
			SendLoginMessage(Connection, 20, "Login failed due to corrupt data.", -1);
			return false;
		}
		RSAMutex.up();

		TReadBuffer Help(AssymmetricData, 128);
		Help.readByte();
		Connection->SetSymmetricKey(AssymmetricData + Help.getPosition());
		Help.skip(16);
		if (!ClientLayout772) {
			TerminalType = Help.readWord();
			TerminalVersion = Help.readWord();
		}
		GamemasterClient = Help.readByte() != 0;
		AccountID = Help.readQuad();
		Help.readString(PlayerName, 30);
		Help.readString(PlayerPassword, 30);
	} catch (const char *s) {
		print(3, "Fehler beim Auslesen der Login-Daten (%s).\n", s);
		SendLoginMessage(Connection, 20, "Login failed due to corrupt data.", -1);
		return false;
	}

	if (PlayerName[0] == 0) {
		SendLoginMessage(Connection, 20, "You must enter a character name.", -1);
		return false;
	}

	if (TerminalType >= TERMINALTYPES || TerminalVersion < TERMINALVERSION[TerminalType]) {
		SendLoginMessage(Connection, 20, "Your terminal version is too old.\nPlease get a new version at\nhttp://www.tibia.com.", -1);
		return false;
	}

	if (!GameRunning()) {
		SendLoginMessage(Connection, 20, "The server is not online.\nPlease try again later.", -1);
		return false;
	}

	if (GameStarting()) {
		SendLoginMessage(Connection, 20, "The game is just starting.\nPlease try again later.", -1);
		return false;
	}

	if (GameEnding()) {
		SendLoginMessage(Connection, 20, "The game is just going down.\nPlease try again later.", -1);
		return false;
	}

	TPlayerData *PlayerData;
	bool ReadOnly;
	uint32_t NextTry;
	bool FreeAccount, Newbie;

again:
	if (GetWaitinglistEntry(PlayerName, NextTry, FreeAccount, Newbie)) {
		print(3, "Spieler auf Warteliste.\n");
		int RemainingWaitingTime = static_cast<int>(NextTry - RoundNr);
		if (RemainingWaitingTime > 0) {
			Log("queue", "%s meldet sich %d Sekunden zu fr\xfch an.\n", PlayerName, RemainingWaitingTime);
			SendLoginMessage(Connection, 22, "It's not your turn yet.", RemainingWaitingTime);
			return false;
		}

		if (RemainingWaitingTime < -60) {
			Log("queue", "%s meldet sich %d Sekunden zu sp\xe4t an.\n", PlayerName, -RemainingWaitingTime);
			DeleteWaitinglistEntry(PlayerName);
			goto again;
		}

		int WaitingTime = CheckWaitingTime(PlayerName, Connection, FreeAccount, Newbie);
		if (WaitingTime > 0) {
			InsertWaitinglistEntry(PlayerName, RoundNr + WaitingTime, FreeAccount, Newbie);
			return false;
		}

		DeleteWaitinglistEntry(PlayerName);
		PlayerData = PerformRegistration(Connection, PlayerName, AccountID, PlayerPassword, GamemasterClient);
		if (PlayerData == nullptr) {
			return false;
		}

		// The original compares with getpid(), which under LinuxThreads is the calling thread.
		ReadOnly = PlayerData->Locked != gettid();
	} else {
		print(3, "Spieler nicht auf Warteliste.\n");
		PlayerData = PerformRegistration(Connection, PlayerName, AccountID, PlayerPassword, GamemasterClient);
		if (PlayerData == nullptr) {
			return false;
		}

		ReadOnly = PlayerData->Locked != gettid();
		FreeAccount = (PlayerData->Rights[PREMIUM_ACCOUNT / 8] & (1 << (PREMIUM_ACCOUNT % 8))) == 0;
		Newbie = PlayerData->Profession == 0
				&& (PlayerData->Rights[NO_LOGOUT_BLOCK / 8] & (1 << (NO_LOGOUT_BLOCK % 8))) == 0;

		if (FreeAccount && (MaxPlayers == PremiumPlayerBuffer
				|| (Newbie && MaxNewbies == PremiumNewbieBuffer))) {
			SendLoginMessage(Connection, 20, "Only players with premium accounts\nare allowed to enter this world.", -1);

			TQueryManagerPoolConnection QueryManagerConnection(&QueryManagerConnectionPool);
			if (QueryManagerConnection.operator->() == nullptr) {
				error("HandleLogin: Kann Verbindung zum Query-Manager nicht herstellen.\n");
			} else {
				QueryManagerConnection->decrementIsOnline(PlayerData->CharacterID);
			}

			if (ReadOnly) {
				DecreasePlayerPoolSlotSticky(PlayerData);
			} else {
				ReleasePlayerPoolSlot(PlayerData);
			}
			return false;
		}

		if (!IsPlayerOnline(PlayerName)) {
			int WaitingTime = CheckWaitingTime(PlayerName, Connection, FreeAccount, Newbie);
			if (WaitingTime > 0) {
				InsertWaitinglistEntry(PlayerName, RoundNr + WaitingTime, FreeAccount, Newbie);

				TQueryManagerPoolConnection QueryManagerConnection(&QueryManagerConnectionPool);
				if (QueryManagerConnection.operator->() == nullptr) {
					error("HandleLogin: Kann Verbindung zum Query-Manager nicht herstellen.\n");
				} else {
					QueryManagerConnection->decrementIsOnline(PlayerData->CharacterID);
				}

				if (ReadOnly) {
					DecreasePlayerPoolSlotSticky(PlayerData);
				} else {
					ReleasePlayerPoolSlot(PlayerData);
				}
				return false;
			}
		}
	}

	if (!ReadOnly) {
		IncreasePlayerPoolSlotSticky(PlayerData);
		ReleasePlayerPoolSlot(PlayerData);
	}

	TWriteBuffer WriteBuffer(Connection->InData + 2, 2046);
	try {
		WriteBuffer.writeByte(11);
		WriteBuffer.writeWord(TerminalType);
		WriteBuffer.writeWord(TerminalVersion);
		WriteBuffer.writeQuad(PlayerData->CharacterID);
	} catch (const char *s) {
		error("HandleLogin: Fehler beim Zusammenbauen des Login-Pakets (%s).\n", s);
		SendLoginMessage(Connection, 20, "Internal error, closing connection.", -1);
		return false;
	}

	Connection->NextToSend = 0;
	Connection->NextToCommit = 0;
	Connection->InDataSize = WriteBuffer.getDataSize();
	Connection->NextToWrite = 0;
	Connection->Login();
	return CallGameThread(Connection);
}

// 0x080dde60  communication.c:985
bool ReceiveCommand(TConnection *Connection) {
	if (Connection == nullptr) {
		error("ReceiveCommand: Connection ist NULL.\n");
		return false;
	}

again:
	unsigned char Help[2];
	int n = ReadFromSocket(Connection, Help, 2);
	if (n == 0) {
		return false;
	}

	if (n < 0) {
		return true;
	}

	if (n < 2) {
		NetLoad(n + 48, false);
		print(3, "Zu wenig Daten an Socket %d.\n", Connection->GetSocket());
		return true;
	}

	Connection->InDataSize = Help[0] + Help[1] * 256;
	if (Connection->InDataSize > 2045 || Connection->InDataSize == 0) {
		int away = Connection->InDataSize;
		print(3, "Paket an Socket %d zu gro\xdf oder leer, wird verworfen (%d Bytes)\n", Connection->GetSocket(), away);
		while (away > 0) {
			away -= 2048;
			n = static_cast<int>(read(Connection->GetSocket(), Connection->InData, 2048));
			NetLoad(n + 48, false);
		}
		return true;
	}

	if (Connection->GetState() == CONNECTION_CONNECTED) {
		int Count = ReadFromSocket(Connection, Connection->InData + 2, Connection->InDataSize);
		if (Count < 0) {
			SendLoginMessage(Connection, 20, "Internal error, closing connection.", -1);
			return false;
		}

		NetLoad(Count + 48, false);
		if (Count != Connection->InDataSize) {
			goto again;
		}
	} else {
		int PacketSize = Connection->InDataSize;
		int Count = ReadFromSocket(Connection, Connection->InData, PacketSize);
		if (Count < 0) {
			SendLoginMessage(Connection, 20, "Internal error, closing connection.", -1);
			return false;
		}

		NetLoad(Count + 48, false);
		if (Count != PacketSize) {
			goto again;
		}

		if (PacketSize % 8 != 0) {
			print(3, "Ung\xfcltige Paketl\xe4nge %d f\xfcr verschl\xfcsseltes Paket von %s.\n", PacketSize, Connection->GetName());
			goto again;
		}

		for (int Offset = 0; Offset < PacketSize; Offset += 8) {
			Connection->GetSymmetricKey().decrypt(Connection->InData + Offset);
		}

		Connection->InDataSize = Connection->InData[0] + Connection->InData[1] * 256;
		if (Connection->InDataSize > 2045 || Connection->InDataSize == 0) {
			// The original logs the size of the packet here, not the size of
			// the user data it has just read.
			print(3, "Nutzdaten (%d Bytes) von Paket an Socket %d zu gro\xdf oder leer.\n", PacketSize, Connection->GetSocket());
			goto again;
		}
	}

	bool Result;
	if (Connection->GetState() == CONNECTION_CONNECTED) {
		// alarm(0) in the original; see CommunicationThread.
		if (LoginTimerCreated) {
			struct itimerspec LoginTimeout;
			memset(&LoginTimeout, 0, sizeof(LoginTimeout));
			timer_settime(LoginTimer, 0, &LoginTimeout, nullptr);
		}
		Result = HandleLogin(Connection);
	} else {
		Result = CallGameThread(Connection);
	}

	if (!Result) {
		return false;
	}

	if (!Connection->WaitingForACK) {
		goto again;
	}

	Connection->SigIOPending = true;
	return true;
}

// 0x080de220  communication.c:1068
void IncrementActiveConnections() {
	CommunicationThreadMutex.down();
	ActiveConnections++;
	CommunicationThreadMutex.up();
}

// 0x080de250  communication.c:1078
void DecrementActiveConnections() {
	CommunicationThreadMutex.down();
	ActiveConnections--;
	CommunicationThreadMutex.up();
}

// 0x080de280  communication.c:1089
void CommunicationThread(int Socket) {
	TConnection *Connection = AssignFreeConnection();
	if (Connection == nullptr) {
		print(2, "Keine Verbindung mehr frei.\n");
		if (close(Socket) < 0) {
			error("CommunicationThread: Fehler %d beim Schlie\xdf" "en der Socket (1).\n", errno);
		}
		return;
	}

	Connection->Connect(Socket);
	Connection->WaitingForACK = false;

	// The original calls fcntl(Socket, F_SETOWN, getpid()), which under
	// LinuxThreads makes this thread the receiver of the socket's SIGIO. With
	// NPTL only F_SETOWN_EX with F_OWNER_TID sends it to one thread.
	struct f_owner_ex Owner;
	Owner.type = F_OWNER_TID;
	Owner.pid = gettid();
	if (fcntl(Socket, F_SETOWN_EX, &Owner) < 0) {
		error("CommunicationThread: F_SETOWN fehlgeschlagen f\xfcr Socket %d.\n", Socket);
		if (close(Socket) < 0) {
			error("CommunicationThread: Fehler %d beim Schlie\xdf" "en der Socket (2).\n", errno);
		}
		Connection->Free();
		return;
	}

	if (fcntl(Socket, F_SETFL, O_NONBLOCK | O_ASYNC) < 0) {
		error("ConnectionThread: F_SETFL fehlgeschlagen f\xfcr Socket %d.\n", Socket);
		if (close(Socket) < 0) {
			error("CommunicationThread: Fehler %d beim Schlie\xdf" "en der Socket (3).\n", errno);
		}
		Connection->Free();
		return;
	}

	sigset_t SignalSet;
	sigfillset(&SignalSet);
	sigprocmask(SIG_SETMASK, &SignalSet, nullptr);

	// The original calls alarm(5) here and alarm(0) in ReceiveCommand. Under
	// LinuxThreads that timer and its SIGALRM belonged to this thread. With
	// NPTL alarm() is the one timer of the process, which the game thread
	// uses for its beat, so the login time-out is a timer that signals this
	// thread.
	struct sigevent LoginTimerEvent;
	memset(&LoginTimerEvent, 0, sizeof(LoginTimerEvent));
	LoginTimerEvent.sigev_notify = SIGEV_THREAD_ID;
	LoginTimerEvent.sigev_signo = SIGALRM;
	LoginTimerEvent._sigev_un._tid = gettid();
	LoginTimerCreated = (timer_create(CLOCK_REALTIME, &LoginTimerEvent, &LoginTimer) == 0);
	if (LoginTimerCreated) {
		struct itimerspec LoginTimeout;
		memset(&LoginTimeout, 0, sizeof(LoginTimeout));
		LoginTimeout.it_value.tv_sec = 5;
		timer_settime(LoginTimer, 0, &LoginTimeout, nullptr);
	}

	if (!ReceiveCommand(Connection)) {
		Connection->Close(true);
	}
	Connection->SigIOPending = false;

	// The original waits on SignalSet as filled above, i.e. on every signal.
	// Under LinuxThreads only signals sent to this thread's own PID reached
	// it. With NPTL a thread in sigwait also takes process-directed signals:
	// the beat SIGALRM of the game thread's setitimer and anything kill()ed
	// to the server's PID. So the thread waits only for the signals it acts
	// on, and hands each of those that neither its login timer raised nor a
	// thread of this process sent (si_code does not tell tgkill from kill on
	// every kernel, si_pid does) to the main thread, whose thread ID is the
	// process ID. SIGIO (from F_SETOWN_EX) and SIGPIPE (from send) are always
	// directed at this thread.
	sigemptyset(&SignalSet);
	sigaddset(&SignalSet, SIGHUP);
	sigaddset(&SignalSet, SIGUSR1);
	sigaddset(&SignalSet, SIGUSR2);
	sigaddset(&SignalSet, SIGPIPE);
	sigaddset(&SignalSet, SIGALRM);
	sigaddset(&SignalSet, SIGIO);

	while (GameRunning() && Connection->ConnectionOk()) {
		siginfo_t Info;
		int Signal = sigwaitinfo(&SignalSet, &Info);
		if ((Signal == SIGHUP || Signal == SIGUSR1 || Signal == SIGUSR2 || Signal == SIGALRM)
				&& Info.si_code != SI_TIMER && Info.si_pid != getpid()) {
			tgkill(getpid(), getpid(), Signal);
			continue;
		}

		switch (Signal) {
		case SIGUSR1:
			if (!Connection->SigIOPending) {
				break;
			}
			[[fallthrough]];

		case SIGIO:
			if (Connection->WaitingForACK) {
				Connection->SigIOPending = true;
			} else {
				Connection->SigIOPending = false;
				if (!ReceiveCommand(Connection)) {
					Connection->Close(true);
				}
			}
			break;

		case SIGUSR2:
			if (!SendData(Connection)) {
				Connection->Close(false);
			}
			break;

		case SIGHUP:
		case SIGPIPE:
			Connection->Close(false);
			break;

		case SIGALRM:
			if (Connection->GetState() == CONNECTION_CONNECTED) {
				print(2, "Login-TimeOut f\xfcr Socket %d.\n", Socket);
				Connection->Close(false);
			}
			break;
		}
	}

	if (LoginTimerCreated) {
		timer_delete(LoginTimer);
		LoginTimerCreated = false;
	}

	while (Connection->IsOnline()) {
		DelayThread(1, 0);
	}

	if (Connection->ClosingDelayed()) {
		DelayThread(2, 0);
	}

	if (close(Socket) < 0) {
		error("CommunicationThread: Fehler %d beim Schlie\xdf" "en der Socket (4).\n", errno);
	}
	Connection->Free();
}

// 0x080de550  communication.c:1165
int HandleConnection(void *Data) {
	// Data is no pointer: the acceptor packs the stack number and the socket
	// into it. The original casts it to int directly.
	int Socket = static_cast<int>(reinterpret_cast<intptr_t>(Data)) % 65536;
	int StackNumber = static_cast<int>(reinterpret_cast<intptr_t>(Data)) / 65536;

	if (UseOwnStacks) {
		AttachCommunicationThreadStack(StackNumber);
	}

	try {
		CommunicationThread(Socket);
	} catch (RESULT r) {
		error("HandleConnection: Nicht abgefangene Exception %d.\n", r);
	} catch (const char *s) {
		error("HandleConnection: Nicht abgefangene Exception \"%s\".\n", s);
	} catch (std::exception &e) {
		error("HandleConnection: Nicht abgefangene Exception %s.\n", e.what());
	} catch (...) {
		error("HandleConnection: Nicht abgefangene Exception unbekannten Typs.\n");
	}

	// Not in the original, where an escaping exception left nothing behind:
	// a POSIX timer outlives its thread.
	if (LoginTimerCreated) {
		timer_delete(LoginTimer);
		LoginTimerCreated = false;
	}

	DecrementActiveConnections();
	if (UseOwnStacks) {
		ReleaseCommunicationThreadStack(StackNumber);
	}

	return 0;
}

// 0x080de690  communication.c:1197
bool OpenSocket() {
	print(1, "Starte Game-Server...\n");
	print(1, "Pid %d - horche an Port %d\n", getpid(), GamePort);

	TCPSocket = socket(AF_INET, SOCK_STREAM, 0);
	if (TCPSocket < 0) {
		error("LaunchServer: Kann Socket nicht \xf6" "ffnen.\n");
		return false;
	}

	struct linger l;
	l.l_onoff = 0;
	l.l_linger = 0;
	if (setsockopt(TCPSocket, SOL_SOCKET, SO_LINGER, &l, sizeof(l)) < 0) {
		error("LaunchServer: Socket wurde nicht auf LINGER=0 gesetzt.\n");
		return false;
	}

	struct sockaddr_in ServerAddress;
	memset(&ServerAddress, 0, sizeof(ServerAddress));
	ServerAddress.sin_family = AF_INET;
	ServerAddress.sin_addr.s_addr = inet_addr(GameAddress);
	ServerAddress.sin_port = htons(static_cast<unsigned short>(GamePort));

	// The original passes the address structure as the value of SO_REUSEADDR.
	// Its first four bytes are not zero, so the option is switched on.
	if (setsockopt(TCPSocket, SOL_SOCKET, SO_REUSEADDR, &ServerAddress, sizeof(ServerAddress)) != 0) {
		error("LaunchServer: Fehler %d bei setsockopt.\n", errno);
		return false;
	}

	if (bind(TCPSocket, reinterpret_cast<struct sockaddr *>(&ServerAddress), sizeof(ServerAddress)) < 0) {
		error("LaunchServer: Fehler %d bei bind.\n", errno);
		print(1, "Bind Error Again -> Begin FloodBind :(\n");
		while (bind(TCPSocket, reinterpret_cast<struct sockaddr *>(&ServerAddress), sizeof(ServerAddress)) < 0) {
			sleep(1);
		}
	}

	errno = 0;
	if (listen(TCPSocket, 512) < 0) {
		error("LaunchServer: Fehler %d bei listen.\n", errno);
		return false;
	}

	return true;
}

// 0x080de8b0  communication.c:1245
int AcceptorThreadLoop(void *) {
	// Not in the original: under LinuxThreads this thread had its own PID, so
	// neither the game thread's beat SIGALRM (whose SA_INTERRUPT would break
	// off accept) nor a signal kill()ed to the server's PID reached it. With
	// NPTL it blocks every signal but SIGHUP, which ExitCommunication sends
	// it. The communication threads it starts inherit this mask.
	sigset_t SignalSet;
	sigfillset(&SignalSet);
	sigdelset(&SignalSet, SIGHUP);
	sigprocmask(SIG_SETMASK, &SignalSet, nullptr);

	// The original stores getpid(), under LinuxThreads the ID of this thread.
	AcceptorThreadPID = gettid();
	print(1, "Warte auf Clients...\n");

	socklen_t clilen;
	struct sockaddr_in cli_addr;
	while (GameRunning()) {
		try {
			clilen = sizeof(cli_addr);
			errno = 0;
			int NewSocket = accept(TCPSocket, reinterpret_cast<struct sockaddr *>(&cli_addr), &clilen);
			if (NewSocket < 0) {
				if (GameRunning()) {
					error("AcceptorThreadLoop: Fehler %d beim Accept.\n", errno);
				}
				continue;
			}

			if (UseOwnStacks) {
				void *Stack;
				int StackNumber;
				GetCommunicationThreadStack(StackNumber, Stack);
				if (Stack == nullptr) {
					print(3, "Kein Stack-Bereich mehr frei.\n");
					if (close(NewSocket) < 0) {
						error("AcceptorThreadLoop: Fehler %d beim Schlie\xdf" "en der Socket (1).\n", errno);
					}
					continue;
				}

				IncrementActiveConnections();
				if (StartThread(HandleConnection, reinterpret_cast<void *>(static_cast<intptr_t>(StackNumber * 65536 + NewSocket)),
						Stack, 65536, true) == 0) {
					print(3, "Kann neuen Thread nicht anlegen.\n");
					if (close(NewSocket) < 0) {
						error("AcceptorThreadLoop: Fehler %d beim Schlie\xdf" "en der Socket (2).\n", errno);
					}
					DecrementActiveConnections();
					ReleaseCommunicationThreadStack(StackNumber);
				}
			} else {
				if (ActiveConnections >= 1100) {
					print(3, "Keine Verbindung mehr frei.\n");
					if (close(NewSocket) < 0) {
						error("AcceptorThreadLoop: Fehler %d beim Schlie\xdf" "en der Socket (3).\n", errno);
					}
					continue;
				}

				IncrementActiveConnections();
				if (StartThread(HandleConnection, reinterpret_cast<void *>(static_cast<intptr_t>(NewSocket)), 65536, true) == 0) {
					print(3, "Kann neuen Thread nicht anlegen.\n");
					if (close(NewSocket) < 0) {
						error("AcceptorThreadLoop: Fehler %d beim Schlie\xdf" "en der Socket (4).\n", errno);
					}
					DecrementActiveConnections();
				}
			}
		} catch (RESULT r) {
			error("AcceptorThreadLoop: Nicht abgefangene Exception %d.\n", r);
		} catch (const char *s) {
			error("AcceptorThreadLoop: Nicht abgefangene Exception \"%s\".\n", s);
		} catch (...) {
			error("AcceptorThreadLoop: Nicht abgefangene Exception unbekannten Typs.\n");
		}
	}

	AcceptorThreadPID = 0;
	if (ActiveConnections > 0) {
		print(3, "Warte auf Beendigung von %d Communication-Threads...\n", ActiveConnections);
		while (ActiveConnections > 0) {
			sleep(1);
		}
	}

	return 0;
}

// 0x080dec20  communication.c:1354
void CheckThreadlibVersion() {
	UseOwnStacks = !FileExists("/etc/image-release");
	if (UseOwnStacks) {
		print(2, "Verwende eigene Stacks.\n");
	} else {
		print(2, "Verwende verkleinerte Bibliotheks-Stacks.\n");
	}
}

// 0x080dec60  communication.c:1364
// The original is declared throw(const char *), which C++17 no longer has.
void InitCommunication() {
	CheckThreadlibVersion();
	InitCommunicationThreadStacks();
	QueryManagerConnectionPool.init();

	ActiveConnections = 0;
	TCPSocket = -1;
	OpenSocket();
	if (TCPSocket == -1) {
		throw "cannot open socket";
	}

	InitLoadHistory();
	WaitinglistHead = nullptr;
	PrivateKey.init(RSA_PRIME_P, RSA_PRIME_Q);

	AcceptorThread = StartThread(AcceptorThreadLoop, nullptr, false);
	if (AcceptorThread == 0) {
		throw "cannot start acceptor thread";
	}
}

// 0x080dee30  communication.c:1384
void ExitCommunication() {
	print(3, "Beende alle Verbindungen...\n");
	TConnection *Connection = GetFirstConnection();
	while (Connection != nullptr) {
		// The original calls kill(Connection->GetPID(), SIGHUP), under
		// LinuxThreads a signal to the thread that serves the connection.
		tgkill(getpid(), Connection->GetPID(), SIGHUP);
		Connection = GetNextConnection();
	}

	ProcessConnections();
	print(3, "Alle Verbindungen beendet.\n");

	if (TCPSocket != -1) {
		if (close(TCPSocket) < 0) {
			error("ExitCommunication: Fehler %d beim Schlie\xdf" "en der Socket.\n", errno);
		}
	}

	if (AcceptorThread != 0) {
		if (AcceptorThreadPID != 0) {
			// The original calls kill(AcceptorThreadPID, SIGHUP).
			tgkill(getpid(), AcceptorThreadPID, SIGHUP);
		}
		JoinThread(AcceptorThread);
		AcceptorThread = 0;
	}

	QueryManagerConnectionPool.exit();
	ExitCommunicationThreadStacks();
}
