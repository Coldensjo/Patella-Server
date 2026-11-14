// communication.cc: the game server's side of the network. The acceptor thread takes the clients' connections and
// starts a communication thread for each, which reads the login (RSA) and the client's packets (XTEA), sends the data
// the game queued for it, and acts on the signals the game and the socket send it. With them: the threads' own stacks,
// the network load statistics, the waiting list, and the pool of query manager connections the threads use.

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
#include "sending.hh"
#include "receiving.hh"
#include "vlong.hh"
#include "TRSAPublicKey.hh"
#include "utils.hh"
#include "time.hh"
#include "TXTEASymmetricKey.hh"
#include "communication.hh"
#include "TRSAPrivateKey.hh"
#include "rsakeyfile.hh"
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
};
int TCPSocket;
Thread AcceptorThread;
pid_t AcceptorThreadPID;
int ActiveConnections;
Semaphore CommunicationThreadMutex(1);
bool UseOwnStacks;
unsigned char CommunicationThreadStacks[MAX_COMMUNICATION_THREADS * COMMUNICATION_THREAD_STACK_SIZE];
pid_t LastUsingCommunicationThread[MAX_COMMUNICATION_THREADS];
int FreeCommunicationThreadStacks[MAX_COMMUNICATION_THREADS];
int NumberOfFreeCommunicationThreadStacks;
TRSAPrivateKey PrivateKey;
// The primes of PrivateKey, from the key file (ReadPrivateKey).
static TRSAKeyFile PrivateKeyFile;
Semaphore RSAMutex(1);
TQueryManagerConnectionPool QueryManagerConnectionPool(10);
int LoadHistory[LOAD_HISTORY_SIZE];
int LoadHistoryPointer;
int TotalLoad;
int TotalSend;
int TotalRecv;
uint32_t LagEnd;
uint32_t EarliestFreeAccountAdmissionRound;
store<TWaitinglistEntry, 100> Waitinglist;
TWaitinglistEntry *WaitinglistHead;

// The login time-out timer of each communication thread, a POSIX timer that
// sends SIGALRM to the thread itself (alarm() is the one timer of the process,
// which the game thread uses for its beat), and whether timer_create gave this
// thread one: without it the connection has no login time-out.
static thread_local timer_t LoginTimer;
static thread_local bool LoginTimerCreated;

void InitCommunicationThreadStacks() {
	if (!UseOwnStacks) {
		return;
	}

	memset(CommunicationThreadStacks, 0xAA, sizeof(CommunicationThreadStacks));
	for (int i = 0; i < MAX_COMMUNICATION_THREADS; i++) {
		LastUsingCommunicationThread[i] = 0;
		FreeCommunicationThreadStacks[i] = i;
	}
	NumberOfFreeCommunicationThreadStacks = MAX_COMMUNICATION_THREADS;
}

void GetCommunicationThreadStack(int &StackNumber, void *&Stack) {
	if (!UseOwnStacks) {
		error("GetCommunicationThreadStack: Library does not support own stacks.\n");
		StackNumber = -1;
		Stack = nullptr;
		return;
	}

	CommunicationThreadMutex.down();
	StackNumber = -1;
	Stack = nullptr;
	for (int i = 0; i < NumberOfFreeCommunicationThreadStacks; i++) {
		int Number = FreeCommunicationThreadStacks[i];
		// A stack is free once the thread that last ran on it has ended: the
		// stored value is that thread's ID, and tgkill with signal 0 fails
		// when the thread is gone.
		if (LastUsingCommunicationThread[Number] == 0
				|| tgkill(getpid(), LastUsingCommunicationThread[Number], 0) == -1) {
			StackNumber = Number;
			Stack = &CommunicationThreadStacks[Number * COMMUNICATION_THREAD_STACK_SIZE];
			NumberOfFreeCommunicationThreadStacks--;
			FreeCommunicationThreadStacks[i] = FreeCommunicationThreadStacks[NumberOfFreeCommunicationThreadStacks];
			break;
		}
	}
	CommunicationThreadMutex.up();
}

void AttachCommunicationThreadStack(int StackNumber) {
	// The ID of the calling thread, which GetCommunicationThreadStack tests.
	LastUsingCommunicationThread[StackNumber] = gettid();
}

void ReleaseCommunicationThreadStack(int StackNumber) {
	CommunicationThreadMutex.down();
	FreeCommunicationThreadStacks[NumberOfFreeCommunicationThreadStacks] = StackNumber;
	NumberOfFreeCommunicationThreadStacks++;
	CommunicationThreadMutex.up();
}

void ExitCommunicationThreadStacks() {
	if (!UseOwnStacks) {
		return;
	}

	if (NumberOfFreeCommunicationThreadStacks != MAX_COMMUNICATION_THREADS) {
		error("ExitCommunicationThreadStacks: Not all stacks freed.\n");
	}

	int HighestStackAddress = -1;
	int LowestStackAddress = COMMUNICATION_THREAD_STACK_SIZE;
	for (int Stack = 0; Stack < MAX_COMMUNICATION_THREADS; Stack++) {
		for (int i = 0; i < COMMUNICATION_THREAD_STACK_SIZE; i++) {
			if (CommunicationThreadStacks[Stack * COMMUNICATION_THREAD_STACK_SIZE + i] != 0xAA) {
				if (i > HighestStackAddress) {
					HighestStackAddress = i;
				}
				if (i < LowestStackAddress) {
					LowestStackAddress = i;
				}
			}
		}
	}

	if (COMMUNICATION_THREAD_STACK_SIZE - (HighestStackAddress - LowestStackAddress) < 32768) {
		error("Maximum stack extent: %d..%d\n", LowestStackAddress, HighestStackAddress);
	}
}

void InitLoadHistory() {
	for (int i = 0; i < LOAD_HISTORY_SIZE; i++) {
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

bool LagDetected() {
	return RoundNr <= LagEnd;
}

void NetLoad(int Amount, bool Send) {
	CommunicationThreadMutex.down();
	if (Send) {
		TotalSend += Amount;
	} else {
		TotalRecv += Amount;
	}
	CommunicationThreadMutex.up();
}

void NetLoadSummary() {
	CommunicationThreadMutex.down();
	Log("netload", "gesendet:  %d Bytes.\n", TotalSend);
	Log("netload", "empfangen: %d Bytes.\n", TotalRecv);
	TotalSend = 0;
	TotalRecv = 0;
	CommunicationThreadMutex.up();
}

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
	if (LoadHistoryPointer == LOAD_HISTORY_SIZE) {
		LoadHistoryPointer = 0;
	}

	if (RoundNr < 3600 || GetPlayersOnline() < 50) {
		return;
	}

	if (CurrentLoad < TotalLoad / 720) {
		Log("game", "Lag detected!\n");
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
			Log("game", "Connection on socket %d broke down.\n", Connection->GetSocket());
			return false;
		}

		if (nwritten <= 0) {
			error("WriteToSocket: Error %d while sending to socket %d.\n", errno, Connection->GetSocket());
			return false;
		}

		nleft -= nwritten;
		Buffer += nwritten;
	}

	NetLoad(Size + 2 + 48, true);
	return true;
}

bool SendLoginMessage(TConnection *Connection, int Type, char *Message, int WaitingTime) {
	if (Type < SV_LOGIN_ERROR || Type > SV_LOGIN_WAITINGLIST) {
		error("SendLoginMessage: Invalid message type %d.\n", Type);
		return true;
	}

	if (Message == nullptr) {
		error("SendLoginMessage: Message is NULL.\n");
		return true;
	}

	if (Type == SV_LOGIN_WAITINGLIST && !in(WaitingTime, 1, 255)) {
		error("SendLoginMessage: Invalid waiting time %d.\n", WaitingTime);
		return true;
	}

	try {
		unsigned char Data[302];
		TWriteBuffer Buffer(Data + 2, 300);
		Buffer.writeWord(0);
		Buffer.writeByte(static_cast<unsigned char>(Type));
		if (strlen(Message) > 290) {
			error("SendLoginMessage: Message too long (%s).\n", Message);
			return true;
		}

		Buffer.writeString(Message);
		if (Type == SV_LOGIN_WAITINGLIST) {
			Buffer.writeByte(static_cast<unsigned char>(WaitingTime));
		}

		int Size = Buffer.getDataSize();
		Buffer.rewind();
		Buffer.writeWord(static_cast<unsigned short>(Size - 2));
		return WriteToSocket(Connection, Data, Size);
	} catch (const char *s) {
		error("SendLoginMessage: Error while filling the buffer (%s)\n", s);
	}

	return true;
}

bool SendData(TConnection *Connection) {
	if (Connection == nullptr) {
		error("SendData: Connection is NULL.\n");
		return false;
	}

	int DataSize = Connection->NextToCommit - Connection->NextToSend;
	int PacketSize = (DataSize + 7) / 8 * 8 + 2;
	// PacketSize is never a multiple of 8, so WriteToSocket pads it with 6
	// more bytes, behind the PacketSize + 2 bytes the write buffer is limited
	// to. The array has room for them.
	unsigned char Buffer[PacketSize + 2 + 6];
	TWriteBuffer WriteBuffer(Buffer, PacketSize + 2);
	WriteBuffer.writeWord(0);
	WriteBuffer.writeWord(static_cast<unsigned short>(DataSize));

	int Offset = Connection->NextToSend % OUT_DATA_SIZE;
	if (Offset + DataSize > OUT_DATA_SIZE) {
		WriteBuffer.writeBytes(&Connection->OutData[Offset], OUT_DATA_SIZE - Offset);
		WriteBuffer.writeBytes(Connection->OutData, Offset + DataSize - OUT_DATA_SIZE);
	} else {
		WriteBuffer.writeBytes(&Connection->OutData[Offset], DataSize);
	}

	if (!WriteToSocket(Connection, Buffer, PacketSize)) {
		return false;
	}

	Connection->NextToSend += DataSize;
	return true;
}

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

	Log("queue", "Adding %s to the queue.\n", Name);
}

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

int GetWaitinglistPosition(char *Name, bool FreeAccount, bool Newbie) {
	int FreeNewbies = 0;
	int FreeVeterans = 0;
	int PremiumNewbies = 0;
	int PremiumVeterans = 0;

	CommunicationThreadMutex.down();
	while (WaitinglistHead != nullptr && RoundNr > WaitinglistHead->NextTry + 60) {
		TWaitinglistEntry *Entry = WaitinglistHead;
		Log("queue", "Removing %s from the waiting list due to inactivity.\n", Entry->Name);
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

int CheckWaitingTime(char *Name, TConnection *Connection, bool FreeAccount, bool Newbie) {
	const char *Reason = nullptr;
	int Position = GetWaitinglistPosition(Name, FreeAccount, Newbie);
	int WaitingTime = 0;

	int PlayersOnline = GetPlayersOnline();
	int NewbiesOnline = GetNewbiesOnline();

	if (PlayersOnline > GetOrderBufferSpace() - Position) {
		print(3, "Order buffer is almost full.\n");
		Reason = "The server is overloaded.";
		WaitingTime = Position / 2 + 10;
	} else if (FreeAccount && RoundNr < EarliestFreeAccountAdmissionRound) {
		print(3, "No FreeAccounts admitted after MassKick.\n");
		Reason = "The server is overloaded.\nOnly players with premium accounts\nare admitted at the moment.";
		WaitingTime = Position / 2 + static_cast<int>(EarliestFreeAccountAdmissionRound - RoundNr);
	} else if (!FreeAccount && PlayersOnline > MaxPlayers - Position) {
		print(3, "Too many players online.\n");
		Reason = "There are too many players online.";
		WaitingTime = Position / 2 + 3;
	} else if (FreeAccount && PlayersOnline > MaxPlayers - PremiumPlayerBuffer - Position) {
		print(3, "No more room for FreeAccounts.\n");
		Reason = "Too many players online.\nOnly players with premium accounts\nare admitted at the moment.";
		WaitingTime = Position / 2 + 5;
	} else if (Newbie && !FreeAccount && NewbiesOnline > MaxNewbies - Position) {
		print(3, "No more room for Newbies.\n");
		Reason = "There are too many players online\non the beginners' island, Rookgaard.";
		WaitingTime = Position / 2 + 3;
	} else if (Newbie && FreeAccount && NewbiesOnline > MaxNewbies - PremiumNewbieBuffer - Position) {
		print(3, "No more room for Newbies with FreeAccount.\n");
		Reason = "There are too many players online\non the beginners' island, Rookgaard.\nOnly players with premium accounts\nare admitted at the moment.";
		WaitingTime = Position / 2 + 5;
	}

	if (WaitingTime > 240) {
		WaitingTime = 240;
	}

	if (WaitingTime > 0) {
		char Message[250];
		snprintf(Message, 250, "%s\n\nYou are at place %d on the waiting list.", Reason, Position);
		SendLoginMessage(Connection, SV_LOGIN_WAITINGLIST, Message, WaitingTime);
	}

	return WaitingTime;
}

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

bool CallGameThread(TConnection *Connection) {
	if (GameRunning()) {
		Connection->WaitingForACK = true;
		// SIGUSR1 to the game thread. It is the main thread, so its thread ID
		// is the process ID.
		if (tgkill(getpid(), GetGameThreadPID(), SIGUSR1) != 0) {
			error("CallGameThread: Can't send SIGUSR1 to pid %d\n", GetGameThreadPID());
			SendLoginMessage(Connection, SV_LOGIN_ERROR, "The server is not online.\nPlease try again later.", -1);
			return false;
		}
	}

	return true;
}

bool CheckConnection(TConnection *Connection) {
	pollfd Status;
	Status.fd = Connection->GetSocket();
	Status.events = POLLIN;
	if (poll(&Status, 1, 0) < 0) {
		return false;
	}

	return (Status.revents & POLLIN) == 0;
}

TPlayerData *PerformRegistration(TConnection *Connection, char *PlayerName, uint32_t AccountID, char *PlayerPassword, bool GamemasterClient) {
	TQueryManagerPoolConnection QueryManagerConnection(&QueryManagerConnectionPool);
	if (QueryManagerConnection.operator->() == nullptr) {
		error("PerformRegistration: Cannot connect to the query manager.\n");
		SendLoginMessage(Connection, SV_LOGIN_ERROR, "Internal error, closing connection.", -1);
		return nullptr;
	}

	if (!CheckConnection(Connection)) {
		return nullptr;
	}

	print(3, "Player \"%s\" logs in.\n", PlayerName);

	uint32_t CharacterID;
	int Sex, NumberOfBuddies;
	bool PremiumAccountActivated;
	unsigned char Rights[12];
	uint32_t BuddyIDs[MAX_BUDDIES];
	char Guild[31], Rank[31], Title[31], BuddyNames[MAX_BUDDIES][30];
	memset(Rights, 0, sizeof(Rights));

	switch (QueryManagerConnection->loginGame(AccountID, PlayerName, PlayerPassword,
			Connection->GetIPAddress(), PrivateWorld, false, GamemasterClient,
			CharacterID, Sex, Guild, Rank, Title, NumberOfBuddies, BuddyIDs,
			BuddyNames, Rights, PremiumAccountActivated)) {
	case STATUS_OK:
		break;

	case LOGIN_CHARACTER_UNKNOWN:
		print(3, "Player does not exist.\n");
		SendLoginMessage(Connection, SV_LOGIN_ERROR, "Character doesn't exist.\nCreate a new character on the Tibia website\nat \"www.tibia.com\".", -1);
		return nullptr;

	case LOGIN_CHARACTER_DELETED:
		print(3, "Player was deleted.\n");
		SendLoginMessage(Connection, SV_LOGIN_ERROR, "Character doesn't exist.\nCreate a new character on the Tibia website.", -1);
		return nullptr;

	case LOGIN_WRONG_WORLD:
		print(3, "Player does not live on this world.\n");
		SendLoginMessage(Connection, SV_LOGIN_ERROR, "Character doesn't live on this world.\nPlease login on the right world.", -1);
		return nullptr;

	case LOGIN_NOT_INVITED:
		print(3, "Player is not invited.\n");
		SendLoginMessage(Connection, SV_LOGIN_ERROR, "This world is private and you have not been invited to play on it.", -1);
		return nullptr;

	case LOGIN_WRONG_PASSWORD:
		Log("game", "Wrong password for player %s; login from %s.\n", PlayerName, Connection->GetIPAddress());
		SendLoginMessage(Connection, SV_LOGIN_ERROR, "Accountnumber or password is not correct.", -1);
		return nullptr;

	case LOGIN_ACCOUNT_BLOCKED:
		Log("game", "Player %s blocked; login from %s.\n", PlayerName, Connection->GetIPAddress());
		SendLoginMessage(Connection, SV_LOGIN_ERROR, "Account disabled for five minutes. Please wait.", -1);
		return nullptr;

	case LOGIN_ACCOUNT_DELETED:
		Log("game", "Account of player %s was deleted.\n", PlayerName);
		SendLoginMessage(Connection, SV_LOGIN_ERROR, "Accountnumber or password is not correct.", -1);
		return nullptr;

	case LOGIN_IP_BLOCKED:
		Log("game", "IP address %s blocked for player %s.\n", Connection->GetIPAddress(), PlayerName);
		SendLoginMessage(Connection, SV_LOGIN_ERROR, "IP address blocked for 30 minutes. Please wait.", -1);
		return nullptr;

	case LOGIN_ACCOUNT_BANISHED:
		print(3, "Account is banished.\n");
		SendLoginMessage(Connection, SV_LOGIN_ERROR, "Your account is banished.", -1);
		return nullptr;

	case LOGIN_NAMELOCKED:
		print(3, "Character must be renamed.\n");
		SendLoginMessage(Connection, SV_LOGIN_ERROR, "Your character is banished because of his/her name.", -1);
		return nullptr;

	case LOGIN_IP_BANISHED:
		print(3, "IP address is blocked.\n");
		SendLoginMessage(Connection, SV_LOGIN_ERROR, "Your IP address is banished.", -1);
		return nullptr;

	case LOGIN_ALREADY_ONLINE:
		print(3, "Other characters of the same account already logged in.\n");
		SendLoginMessage(Connection, SV_LOGIN_ERROR, "You may only login with one character\nof your account at the same time.", -1);
		return nullptr;

	case LOGIN_NOT_GAMEMASTER:
		print(3, "Login with gamemaster client on non-gamemaster account.\n");
		SendLoginMessage(Connection, SV_LOGIN_ERROR, "You may only login with a Gamemaster account.", -1);
		return nullptr;

	case LOGIN_WRONG_ACCOUNT:
		Log("game", "Wrong account number %lu for %s.\n", static_cast<unsigned long>(AccountID), PlayerName);
		SendLoginMessage(Connection, SV_LOGIN_ERROR, "Login failed due to corrupt data.", -1);
		return nullptr;

	case -1:
		SendLoginMessage(Connection, SV_LOGIN_ERROR, "Internal error, closing connection.", -1);
		return nullptr;

	default:
		error("PerformRegistration: Unknown return value from QueryManager.\n");
		SendLoginMessage(Connection, SV_LOGIN_ERROR, "Internal error, closing connection.", -1);
		return nullptr;
	}

	if (!CheckConnection(Connection)) {
		QueryManagerConnection->decrementIsOnline(CharacterID);
		return nullptr;
	}

	if (AccountID == 0) {
		error("PerformRegistration: Player %s has not been assigned to an account yet.\n", PlayerName);
		SendLoginMessage(Connection, SV_LOGIN_ERROR, "Character is not assigned to an account.\nPerform this on the Tibia website\nat \"www.tibia.com\".", -1);
		return nullptr;
	}

	PlayerName[29] = 0;
	if (PremiumAccountActivated) {
		SendLoginMessage(Connection, SV_LOGIN_MESSAGE, "Your Premium Account is now activated.\nHave a lot of fun in Tibia.", -1);
	}

	Log("game", "Spieler %s loggt ein an Socket %d von %s.\n", PlayerName, Connection->GetSocket(), Connection->GetIPAddress());

	TPlayerData *PlayerData = AssignPlayerPoolSlot(CharacterID, true);
	if (PlayerData == nullptr) {
		error("PerformRegistration: Cannot assign a slot for player data.\n");
		QueryManagerConnection->decrementIsOnline(CharacterID);
		SendLoginMessage(Connection, SV_LOGIN_ERROR, "There are too many players online.\nPlease try again later.", -1);
		return nullptr;
	}

	// Locked is the ID of the thread that locked the slot.
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

bool HandleLogin(TConnection *Connection) {
	TReadBuffer Buffer(Connection->InData + 2, 2046);
	unsigned char Command;
	try {
		Command = Buffer.readByte();
	} catch (const char *s) {
		error("HandleLogin: Error while reading the command (%s).\n", s);
		return false;
	}

	if (Command != CL_LOGIN) {
		print(3, "Invalid init command %d.\n", Command);
		return false;
	}

	unsigned short TerminalType, TerminalVersion;
	bool GamemasterClient;
	uint32_t AccountID;
	char PlayerName[31];
	char PlayerPassword[31];
	// Two layouts: a 7.72 client sends terminal type and version in clear
	// before the RSA block (command, 2 words, 128 bytes: 133 bytes) and
	// leaves them out of the block; a 7.70 client sends them inside the block
	// (command, 128 bytes: 129 bytes). A packet of any other size is read as
	// the 7.70 layout.
	bool ClientLayout772 = (Connection->InDataSize == 1 + 4 + 128);
	try {
		if (ClientLayout772) {
			TerminalType = Buffer.readWord();
			TerminalVersion = Buffer.readWord();
		}
		unsigned char AsymmetricData[128];
		Buffer.readBytes(AsymmetricData, 128);

		RSAMutex.down();
		try {
			PrivateKey.decrypt(AsymmetricData);
		} catch (const char *s) {
			error("HandleLogin: Error while decrypting (%s).\n", s);
			RSAMutex.up();
			SendLoginMessage(Connection, SV_LOGIN_ERROR, "Login failed due to corrupt data.", -1);
			return false;
		}
		RSAMutex.up();

		TReadBuffer Help(AsymmetricData, 128);
		Help.readByte();
		Connection->SetSymmetricKey(AsymmetricData + Help.getPosition());
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
		print(3, "Error while reading the login data (%s).\n", s);
		SendLoginMessage(Connection, SV_LOGIN_ERROR, "Login failed due to corrupt data.", -1);
		return false;
	}

	if (PlayerName[0] == 0) {
		SendLoginMessage(Connection, SV_LOGIN_ERROR, "You must enter a character name.", -1);
		return false;
	}

	if (TerminalType >= TERMINALTYPES || TerminalVersion < TERMINALVERSION[TerminalType]) {
		SendLoginMessage(Connection, SV_LOGIN_ERROR, "Your terminal version is too old.\nPlease get a new version at\nhttp://www.tibia.com.", -1);
		return false;
	}

	if (!GameRunning()) {
		SendLoginMessage(Connection, SV_LOGIN_ERROR, "The server is not online.\nPlease try again later.", -1);
		return false;
	}

	if (GameStarting()) {
		SendLoginMessage(Connection, SV_LOGIN_ERROR, "The game is just starting.\nPlease try again later.", -1);
		return false;
	}

	if (GameEnding()) {
		SendLoginMessage(Connection, SV_LOGIN_ERROR, "The game is just going down.\nPlease try again later.", -1);
		return false;
	}

	TPlayerData *PlayerData;
	bool ReadOnly;
	uint32_t NextTry;
	bool FreeAccount, Newbie;

again:
	if (GetWaitinglistEntry(PlayerName, NextTry, FreeAccount, Newbie)) {
		print(3, "Player on waiting list.\n");
		int RemainingWaitingTime = static_cast<int>(NextTry - RoundNr);
		if (RemainingWaitingTime > 0) {
			Log("queue", "%s logs in %d seconds too early.\n", PlayerName, RemainingWaitingTime);
			SendLoginMessage(Connection, SV_LOGIN_WAITINGLIST, "It's not your turn yet.", RemainingWaitingTime);
			return false;
		}

		if (RemainingWaitingTime < -60) {
			Log("queue", "%s logs in %d seconds too late.\n", PlayerName, -RemainingWaitingTime);
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

		// Locked is the ID of the thread that locked the slot.
		ReadOnly = PlayerData->Locked != gettid();
	} else {
		print(3, "Player not on waiting list.\n");
		PlayerData = PerformRegistration(Connection, PlayerName, AccountID, PlayerPassword, GamemasterClient);
		if (PlayerData == nullptr) {
			return false;
		}

		ReadOnly = PlayerData->Locked != gettid();
		FreeAccount = (PlayerData->Rights[PREMIUM_ACCOUNT / 8] & (1 << (PREMIUM_ACCOUNT % 8))) == 0;
		Newbie = PlayerData->Profession == PROFESSION_NONE
				&& (PlayerData->Rights[NO_LOGOUT_BLOCK / 8] & (1 << (NO_LOGOUT_BLOCK % 8))) == 0;

		if (FreeAccount && (MaxPlayers == PremiumPlayerBuffer
				|| (Newbie && MaxNewbies == PremiumNewbieBuffer))) {
			SendLoginMessage(Connection, SV_LOGIN_ERROR, "Only players with premium accounts\nare allowed to enter this world.", -1);

			TQueryManagerPoolConnection QueryManagerConnection(&QueryManagerConnectionPool);
			if (QueryManagerConnection.operator->() == nullptr) {
				error("HandleLogin: Cannot connect to the query manager.\n");
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
					error("HandleLogin: Cannot connect to the query manager.\n");
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
		WriteBuffer.writeByte(CL_JOIN_GAME);
		WriteBuffer.writeWord(TerminalType);
		WriteBuffer.writeWord(TerminalVersion);
		WriteBuffer.writeQuad(PlayerData->CharacterID);
	} catch (const char *s) {
		error("HandleLogin: Error while building the login packet (%s).\n", s);
		SendLoginMessage(Connection, SV_LOGIN_ERROR, "Internal error, closing connection.", -1);
		return false;
	}

	Connection->NextToSend = 0;
	Connection->NextToCommit = 0;
	Connection->InDataSize = WriteBuffer.getDataSize();
	Connection->NextToWrite = 0;
	Connection->Login();
	return CallGameThread(Connection);
}

bool ReceiveCommand(TConnection *Connection) {
	if (Connection == nullptr) {
		error("ReceiveCommand: Connection is NULL.\n");
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
		print(3, "Too little data on socket %d.\n", Connection->GetSocket());
		return true;
	}

	Connection->InDataSize = Help[0] + Help[1] * 256;
	if (Connection->InDataSize > 2045 || Connection->InDataSize == 0) {
		int away = Connection->InDataSize;
		print(3, "Packet on socket %d too large or empty, discarded (%d bytes)\n", Connection->GetSocket(), away);
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
			SendLoginMessage(Connection, SV_LOGIN_ERROR, "Internal error, closing connection.", -1);
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
			SendLoginMessage(Connection, SV_LOGIN_ERROR, "Internal error, closing connection.", -1);
			return false;
		}

		NetLoad(Count + 48, false);
		if (Count != PacketSize) {
			goto again;
		}

		if (PacketSize % 8 != 0) {
			print(3, "Invalid packet length %d for encrypted packet from %s.\n", PacketSize, Connection->GetName());
			goto again;
		}

		for (int Offset = 0; Offset < PacketSize; Offset += 8) {
			Connection->GetSymmetricKey().decrypt(Connection->InData + Offset);
		}

		Connection->InDataSize = Connection->InData[0] + Connection->InData[1] * 256;
		if (Connection->InDataSize > 2045 || Connection->InDataSize == 0) {
			// Quirk: the message gives the size of the packet, not the size of
			// the user data just read.
			print(3, "Payload (%d bytes) of packet on socket %d too large or empty.\n", PacketSize, Connection->GetSocket());
			goto again;
		}
	}

	bool Result;
	if (Connection->GetState() == CONNECTION_CONNECTED) {
		// The login has arrived: the login time-out stops (CommunicationThread
		// starts it).
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

void IncrementActiveConnections() {
	CommunicationThreadMutex.down();
	ActiveConnections++;
	CommunicationThreadMutex.up();
}

void DecrementActiveConnections() {
	CommunicationThreadMutex.down();
	ActiveConnections--;
	CommunicationThreadMutex.up();
}

void CommunicationThread(int Socket) {
	TConnection *Connection = AssignFreeConnection();
	if (Connection == nullptr) {
		print(2, "No free connection left.\n");
		if (close(Socket) < 0) {
			error("CommunicationThread: Error %d while closing the socket (1).\n", errno);
		}
		return;
	}

	Connection->Connect(Socket);
	Connection->WaitingForACK = false;

	// The socket's SIGIO goes to this thread: F_SETOWN_EX with F_OWNER_TID
	// names one thread, where F_SETOWN with a process ID would let any thread
	// of the process take it.
	struct f_owner_ex Owner;
	Owner.type = F_OWNER_TID;
	Owner.pid = gettid();
	if (fcntl(Socket, F_SETOWN_EX, &Owner) < 0) {
		error("CommunicationThread: F_SETOWN failed for socket %d.\n", Socket);
		if (close(Socket) < 0) {
			error("CommunicationThread: Error %d while closing the socket (2).\n", errno);
		}
		Connection->Free();
		return;
	}

	if (fcntl(Socket, F_SETFL, O_NONBLOCK | O_ASYNC) < 0) {
		error("CommunicationThread: F_SETFL failed for socket %d.\n", Socket);
		if (close(Socket) < 0) {
			error("CommunicationThread: Error %d while closing the socket (3).\n", errno);
		}
		Connection->Free();
		return;
	}

	sigset_t SignalSet;
	sigfillset(&SignalSet);
	sigprocmask(SIG_SETMASK, &SignalSet, nullptr);

	// The login time-out: 5 seconds, on a timer that sends SIGALRM to this
	// thread; ReceiveCommand stops it when the login arrives. alarm() is the
	// one timer of the process, which the game thread uses for its beat.
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

	// The thread waits only for the signals it acts on. A thread in sigwait
	// also takes process-directed signals: the beat SIGALRM of the game
	// thread's setitimer and anything kill()ed to the server's PID. So each
	// of those that neither its login timer raised nor a thread of this
	// process sent (si_code does not tell tgkill from kill on every kernel,
	// si_pid does) is handed on to the main thread, whose thread ID is the
	// process ID. SIGIO (from F_SETOWN_EX) and SIGPIPE (from send) are always
	// directed at this thread.
	sigemptyset(&SignalSet);
	sigaddset(&SignalSet, SIGHUP);
	sigaddset(&SignalSet, SIGUSR1);
	sigaddset(&SignalSet, SIGUSR2);
	sigaddset(&SignalSet, SIGPIPE);
	sigaddset(&SignalSet, SIGALRM);
	sigaddset(&SignalSet, SIGIO);

	while (GameRunning() && Connection->IsOk()) {
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
				print(2, "Login timeout for socket %d.\n", Socket);
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
		error("CommunicationThread: Error %d while closing the socket (4).\n", errno);
	}
	Connection->Free();
}

int HandleConnection(void *Data) {
	// Data is no pointer: the acceptor packs the stack number and the socket
	// into it.
	int Socket = static_cast<int>(reinterpret_cast<intptr_t>(Data)) % 65536;
	int StackNumber = static_cast<int>(reinterpret_cast<intptr_t>(Data)) / 65536;

	if (UseOwnStacks) {
		AttachCommunicationThreadStack(StackNumber);
	}

	try {
		CommunicationThread(Socket);
	} catch (RESULT r) {
		error("HandleConnection: Uncaught exception %d.\n", r);
	} catch (const char *s) {
		error("HandleConnection: Uncaught exception \"%s\".\n", s);
	} catch (std::exception &e) {
		error("HandleConnection: Uncaught exception %s.\n", e.what());
	} catch (...) {
		error("HandleConnection: Uncaught exception of unknown type.\n");
	}

	// CommunicationThread deletes its login timer when it ends; after an
	// exception the timer is deleted here, since a POSIX timer outlives its
	// thread.
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

bool OpenSocket() {
	print(1, "Starting game server...\n");
	print(1, "Pid %d - listening on port %d\n", getpid(), GamePort);

	TCPSocket = socket(AF_INET, SOCK_STREAM, 0);
	if (TCPSocket < 0) {
		error("OpenSocket: Cannot open socket.\n");
		return false;
	}

	struct linger l;
	l.l_onoff = 0;
	l.l_linger = 0;
	if (setsockopt(TCPSocket, SOL_SOCKET, SO_LINGER, &l, sizeof(l)) < 0) {
		error("OpenSocket: Socket was not set to LINGER=0.\n");
		return false;
	}

	struct sockaddr_in ServerAddress;
	memset(&ServerAddress, 0, sizeof(ServerAddress));
	ServerAddress.sin_family = AF_INET;
	ServerAddress.sin_addr.s_addr = inet_addr(GameAddress);
	ServerAddress.sin_port = htons(static_cast<unsigned short>(GamePort));

	// Quirk: the address structure is passed as the value of SO_REUSEADDR.
	// Its first four bytes are not zero, so the option is switched on.
	if (setsockopt(TCPSocket, SOL_SOCKET, SO_REUSEADDR, &ServerAddress, sizeof(ServerAddress)) != 0) {
		error("OpenSocket: Error %d at setsockopt.\n", errno);
		return false;
	}

	if (bind(TCPSocket, reinterpret_cast<struct sockaddr *>(&ServerAddress), sizeof(ServerAddress)) < 0) {
		error("OpenSocket: Error %d at bind.\n", errno);
		print(1, "Bind Error Again -> Begin FloodBind :(\n");
		while (bind(TCPSocket, reinterpret_cast<struct sockaddr *>(&ServerAddress), sizeof(ServerAddress)) < 0) {
			sleep(1);
		}
	}

	errno = 0;
	if (listen(TCPSocket, 512) < 0) {
		error("OpenSocket: Error %d at listen.\n", errno);
		return false;
	}

	return true;
}

int AcceptorThreadLoop(void *) {
	// The acceptor blocks every signal but SIGHUP, which ExitCommunication
	// sends it: the game thread's beat SIGALRM (whose SA_INTERRUPT would
	// break off accept) and the signals kill()ed to the server's PID go to
	// other threads. The communication threads it starts inherit this mask.
	sigset_t SignalSet;
	sigfillset(&SignalSet);
	sigdelset(&SignalSet, SIGHUP);
	sigprocmask(SIG_SETMASK, &SignalSet, nullptr);

	// The ID of this thread, to which ExitCommunication sends SIGHUP.
	AcceptorThreadPID = gettid();
	print(1, "Waiting for clients...\n");

	socklen_t clilen;
	struct sockaddr_in cli_addr;
	while (GameRunning()) {
		try {
			clilen = sizeof(cli_addr);
			errno = 0;
			int NewSocket = accept(TCPSocket, reinterpret_cast<struct sockaddr *>(&cli_addr), &clilen);
			if (NewSocket < 0) {
				if (GameRunning()) {
					error("AcceptorThreadLoop: Error %d at accept.\n", errno);
				}
				continue;
			}

			if (UseOwnStacks) {
				void *Stack;
				int StackNumber;
				GetCommunicationThreadStack(StackNumber, Stack);
				if (Stack == nullptr) {
					print(3, "No free stack area left.\n");
					if (close(NewSocket) < 0) {
						error("AcceptorThreadLoop: Error %d while closing the socket (1).\n", errno);
					}
					continue;
				}

				IncrementActiveConnections();
				if (StartThread(HandleConnection, reinterpret_cast<void *>(static_cast<intptr_t>(StackNumber * 65536 + NewSocket)),
						Stack, COMMUNICATION_THREAD_STACK_SIZE, true) == 0) {
					print(3, "Cannot create new thread.\n");
					if (close(NewSocket) < 0) {
						error("AcceptorThreadLoop: Error %d while closing the socket (2).\n", errno);
					}
					DecrementActiveConnections();
					ReleaseCommunicationThreadStack(StackNumber);
				}
			} else {
				if (ActiveConnections >= MAX_COMMUNICATION_THREADS) {
					print(3, "No free connection left.\n");
					if (close(NewSocket) < 0) {
						error("AcceptorThreadLoop: Error %d while closing the socket (3).\n", errno);
					}
					continue;
				}

				IncrementActiveConnections();
				if (StartThread(HandleConnection, reinterpret_cast<void *>(static_cast<intptr_t>(NewSocket)), COMMUNICATION_THREAD_STACK_SIZE, true) == 0) {
					print(3, "Cannot create new thread.\n");
					if (close(NewSocket) < 0) {
						error("AcceptorThreadLoop: Error %d while closing the socket (4).\n", errno);
					}
					DecrementActiveConnections();
				}
			}
		} catch (RESULT r) {
			error("AcceptorThreadLoop: Uncaught exception %d.\n", r);
		} catch (const char *s) {
			error("AcceptorThreadLoop: Uncaught exception \"%s\".\n", s);
		} catch (...) {
			error("AcceptorThreadLoop: Uncaught exception of unknown type.\n");
		}
	}

	AcceptorThreadPID = 0;
	if (ActiveConnections > 0) {
		print(3, "Waiting for %d communication threads to end...\n", ActiveConnections);
		while (ActiveConnections > 0) {
			sleep(1);
		}
	}

	return 0;
}

void ChooseThreadStacks() {
	UseOwnStacks = !FileExists("/etc/image-release");
	if (UseOwnStacks) {
		print(2, "Using own stacks.\n");
	} else {
		print(2, "Using reduced library stacks.\n");
	}
}

void ReadPrivateKey() {
	try {
		PrivateKeyFile = ReadRSAKeyFile(RSAKEYPATH);
	} catch (const std::exception &e) {
		static char ErrorText[4096 + 256];
		snprintf(ErrorText, sizeof(ErrorText), "%s", e.what());
		throw static_cast<const char *>(ErrorText);
	}
}

// Throws a const char * when it cannot connect to the query manager, open
// the socket or start the acceptor thread. ReadPrivateKey must have read the
// key.
void InitCommunication() {
	ChooseThreadStacks();
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
	PrivateKey.init(PrivateKeyFile.PrimeP.c_str(), PrivateKeyFile.PrimeQ.c_str());

	AcceptorThread = StartThread(AcceptorThreadLoop, nullptr, false);
	if (AcceptorThread == 0) {
		throw "cannot start acceptor thread";
	}
}

void ExitCommunication() {
	print(3, "Closing all connections...\n");
	TConnection *Connection = GetFirstConnection();
	while (Connection != nullptr) {
		// SIGHUP to the thread that serves the connection: GetPID is its
		// thread ID (TConnection::Assign).
		tgkill(getpid(), Connection->GetPID(), SIGHUP);
		Connection = GetNextConnection();
	}

	ProcessConnections();
	print(3, "All connections closed.\n");

	if (TCPSocket != -1) {
		if (close(TCPSocket) < 0) {
			error("ExitCommunication: Error %d while closing the socket.\n", errno);
		}
	}

	if (AcceptorThread != 0) {
		if (AcceptorThreadPID != 0) {
			// SIGHUP to the acceptor thread, the one signal it does not block.
			tgkill(getpid(), AcceptorThreadPID, SIGHUP);
		}
		JoinThread(AcceptorThread);
		AcceptorThread = 0;
	}

	QueryManagerConnectionPool.exit();
	ExitCommunicationThreadStacks();
}
