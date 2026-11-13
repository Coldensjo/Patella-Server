#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <signal.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "config.hh"
#include "script.hh"
#include "threads.hh"
#include "dbfuncs.hh"
#include "objects.hh"
#include "map.hh"
#include "info.hh"
#include "cr.hh"
#include "containr.hh"
#include "operate.hh"
#include "connections.hh"
#include "utils.hh"
#include "time.hh"
#include "communication.hh"
#include "TXTEASymmetricKey.hh"
#include "crmain.hh"
#include "crplayer.hh"
#include "sending.hh"
#include "shm.hh"
#include "writer.hh"

const int PINGTIME = 30;
const int IDLETIME = 900;

TConnection Connections[1100];	// connections.c:32
TConnection *FirstSendingConnection;	// connections.c:33
Semaphore ConnectionMutex(1);	// connections.c:34
int ConnectionIterator;	// connections.c:515

// 0x080cad10  connections.c:45
TConnection::TConnection() {
	Free();
}

// 0x080cad40  connections.c:53
void TConnection::Process() {
	if (State == CONNECTION_LOGIN) {
		if (!GameRunning() || !ConnectionIsOk) {
			Disconnect();
		}
	}

	if (State == CONNECTION_GAME || State == CONNECTION_DEAD) {
		int Time = static_cast<int>(RoundNr - TimeStamp);

		if (Time == PINGTIME) {
			SendPing(this);
		}

		if (Time == 2 * PINGTIME) {
			SendPing(this);
		}

		if (RoundNr - IDLETIME == TimeStampAction && !CheckRight(CharacterID, NO_LOGOUT_BLOCK)) {
			SendMessage(this, TALK_ADMIN_MESSAGE,
				"You have been idle for %d minutes. You will be disconnected in one minute if you are still idle then.",
				IDLETIME / 60);
		}

		if (!GameRunning()
				|| (RoundNr - TimeStampAction >= static_cast<uint32_t>(IDLETIME + 60)
					&& !CheckRight(CharacterID, NO_LOGOUT_BLOCK))) {
			Logout(0, true);
		} else if (!ConnectionIsOk || Time >= 3 * PINGTIME) {
			Logout(0, false);
		}
	}

	if (State == CONNECTION_LOGOUT) {
		if (!GameRunning() || !ConnectionIsOk || RoundNr >= TimeStamp) {
			Disconnect();
		}
	}
}

// 0x080cb0c0  connections.c:75
void TConnection::ResetTimer(int Command) {
	if (State == CONNECTION_GAME || State == CONNECTION_DEAD) {
		TimeStamp = RoundNr;
		if (Command != 105 && Command != 190 && Command != 201 && Command != 202 && Command != 30) {
			TimeStampAction = RoundNr;
		}
	}
}

// 0x080cb120  connections.c:85
void TConnection::EmergencyPing() {
	if (State == CONNECTION_GAME || State == CONNECTION_DEAD) {
		if (RoundNr - TimeStamp < 80) {
			TimeStamp = RoundNr - 100;
		}
		SendPing(this);
	}
}

// 0x080cb170  connections.c:95
pid_t TConnection::GetPID() {
	if (State == CONNECTION_FREE) {
		error("TConnection::GetPID: Verbindung ist nicht zugewiesen.\n");
		return 0;
	}

	return PID;
}

// 0x080cb1a0  connections.c:107
int TConnection::GetSocket() {
	if (State < CONNECTION_CONNECTED) {
		error("TConnection::GetSocket: Verbindung ist nicht angeschlossen.\n");
		return -1;
	}

	return Socket;
}

// 0x080cb1e0  connections.c:119
char *TConnection::GetIPAddress() {
	if (State < CONNECTION_CONNECTED) {
		error("TConnection::GetIPAddress: Verbindung ist nicht angeschlossen.\n");
		return "Unknown";
	}

	return IPAddress;
}

// 0x080cb220  connections.c:131
void TConnection::Free() {
	State = CONNECTION_FREE;
}

// 0x080cb230  connections.c:139
void TConnection::Assign() {
	if (State != CONNECTION_FREE) {
		error("TConnection::Assign: Verbindung ist nicht frei.\n");
	}

	State = CONNECTION_ASSIGNED;
	// The original stores getpid(), which under LinuxThreads is the ID of the
	// calling communication thread.
	PID = gettid();
}

// 0x080cb280  connections.c:150
void TConnection::Connect(int _Socket) {
	if (State != CONNECTION_ASSIGNED) {
		error("TConnection::Connect: Verbindung ist keinem Thread zugewiesen .\n");
	}

	State = CONNECTION_CONNECTED;
	ConnectionIsOk = true;
	ClosingIsDelayed = true;
	Socket = _Socket;

	struct sockaddr peer;
	struct sockaddr_in inaddr;
	socklen_t addrlen = sizeof(peer);
	getpeername(Socket, &peer, &addrlen);
	memcpy(&inaddr, &peer, sizeof(inaddr));
	strcpy(IPAddress, inet_ntoa(inaddr.sin_addr));

	RandomSeed = rand();
}

// 0x080cb330  connections.c:172
void TConnection::Login() {
	if (State != CONNECTION_CONNECTED) {
		error("TConnection::Connect: Ung\xfcltiger Verbindungs-Zustand %d.\n", State);
	}

	State = CONNECTION_LOGIN;
}

// 0x080cb370  connections.c:183
bool TConnection::JoinGame(TReadBuffer &Buffer) {
	if (State != CONNECTION_LOGIN) {
		error("TConnection::JoinGame: Ung\xfcltiger Verbindungszustand %d.\n", State);
	}

	ClearKnownCreatureTable(false);

	try {
		TerminalType = Buffer.readWord();
		TerminalVersion = Buffer.readWord();
	} catch (const char *s) {
		error("TConnection::JoinGame: Fehler beim Auslesen des Puffers (%s).\n", s);
		return false;
	}

	switch (TerminalType) {
		case 0:
		case 1:
		case 2:
			break;

		default:
			error("TConnection::JoinGame: Unbekannter Terminal-Typ %d.\n", TerminalType);
			return false;
	}

	TerminalOffsetX = 8;
	TerminalOffsetY = 6;
	TerminalWidth = 18;
	TerminalHeight = 14;

	try {
		CharacterID = Buffer.readQuad();
	} catch (const char *s) {
		error("TConnection::JoinGame: Fehler beim Auslesen des Puffers (%s).\n", s);
		return false;
	}

	TPlayer *pl = ::GetPlayer(CharacterID);
	if (pl != nullptr) {
		if (pl->IsDead) {
			Log("game", "Spieler %s ist gerade am Sterben - Einloggen gescheitert.\n", pl->Name);
			DecreasePlayerPoolSlotSticky(CharacterID);
			return false;
		}

		if (pl->IsLoggingOut() && pl->LogoutPossible() == 0) {
			Log("game", "Spieler %s loggt gerade aus - Einloggen gescheitert.\n", pl->Name);
			DecreasePlayerPoolSlotSticky(CharacterID);
			return false;
		}

		TConnection *OldConnection = pl->GetConnection();
		pl->ClearConnection();
		if (OldConnection != nullptr) {
			OldConnection->CharacterID = 0;
			OldConnection->Logout(0, true);
		}

		DecrementIsOnlineOrder(CharacterID);
		pl->TakeOver(this);
	} else {
		pl = new TPlayer(this, CharacterID);
		if (pl == nullptr) {
			return false;
		}

		if (pl->ConstructError != NOERROR) {
			delete pl;
			return false;
		}
	}

	strcpy(Name, pl->Name);
	TimeStamp = RoundNr;
	TimeStampAction = RoundNr;
	return true;
}

// 0x080cb6c0  connections.c:270
void TConnection::EnterGame() {
	if (State != CONNECTION_LOGIN) {
		error("TConnection::EnterGame: Ung\xfcltiger Verbindungszustand %d.\n", State);
	}

	State = CONNECTION_GAME;
}

// 0x080cb700  connections.c:280
void TConnection::Die() {
	if (State == CONNECTION_GAME) {
		State = CONNECTION_DEAD;
	}
}

// 0x080cb720  connections.c:290
void TConnection::Logout(int Delay, bool StopFight) {
	if (State != CONNECTION_GAME && State != CONNECTION_DEAD && State != CONNECTION_LOGOUT) {
		error("TConnection::Logout: Ung\xfcltiger Verbindungszustand %d.\n", State);
	}

	State = CONNECTION_LOGOUT;

	TPlayer *pl = GetPlayer();
	if (pl != nullptr) {
		pl->ClearConnection();
		pl->StartLogout(false, StopFight);
	}

	CharacterID = 0;
	TimeStamp = RoundNr + Delay;
	ClosingIsDelayed = false;
}

// 0x080cb7e0  connections.c:309
void TConnection::Close(bool Delay) {
	if (State < CONNECTION_CONNECTED) {
		error("TConnection::Close: Ung\xfcltiger Verbindungszustand %d.\n", State);
	}

	ConnectionIsOk = false;
	ClosingIsDelayed = Delay;
	if (State == CONNECTION_CONNECTED) {
		State = CONNECTION_DISCONNECTED;
	}
}

// 0x080cb850  connections.c:321
void TConnection::Disconnect() {
	if (State < CONNECTION_CONNECTED) {
		error("TConnection::Close: Ung\xfcltiger Verbindungszustand %d.\n", State);
	}

	ClearKnownCreatureTable(true);
	ConnectionIsOk = false;
	State = CONNECTION_DISCONNECTED;
	// The original calls kill(PID, SIGHUP); under LinuxThreads that reaches
	// exactly the communication thread that serves this connection.
	tgkill(getpid(), PID, SIGHUP);
}

// 0x080cb900  connections.c:334
TPlayer *TConnection::GetPlayer() {
	if (!IsOnline()) {
		error("TConnection::GetPlayer: Ung\xfcltiger Verbindungszustand %d.\n", State);
		return nullptr;
	}

	if (CharacterID == 0) {
		return nullptr;
	}

	return ::GetPlayer(CharacterID);
}

// 0x080cb950  connections.c:347
const char *TConnection::GetName() {
	if (!IsOnline()) {
		error("TConnection::GetName: Ung\xfcltiger Verbindungszustand %d.\n", State);
		return "";
	}

	return Name;
}

// 0x080cb990  connections.c:359
void TConnection::GetPosition(int &x, int &y, int &z) {
	TPlayer *pl = GetPlayer();
	if (pl == nullptr) {
		x = 0;
		y = 0;
		z = 0;
	} else {
		x = pl->posx;
		y = pl->posy;
		z = pl->posz;
	}
}

// 0x080cba30  connections.c:376
bool TConnection::IsVisible(int x, int y, int z) {
	int posx, posy, posz;
	GetPosition(posx, posy, posz);

	if (posz <= 7) {
		if (z > 7) {
			return false;
		}
	} else {
		if (z - posz < -2 || z - posz > 2) {
			return false;
		}
	}

	int dz = posz - z;
	x = x + TerminalOffsetX - posx - dz;
	y = y + TerminalOffsetY - posy - dz;
	return x >= 0 && x < TerminalWidth && y >= 0 && y < TerminalHeight;
}

// 0x080cbae0  connections.c:393
KNOWNCREATURESTATE TConnection::KnownCreature(uint32_t ID, bool UpdateFollows) {
	for (int i = 0; i < 150; i++) {
		TKnownCreature &K = KnownCreatureTable[i];
		if (K.CreatureID == ID) {
			KNOWNCREATURESTATE State = K.State;
			if (State == KNOWNCREATURE_OUTDATED && UpdateFollows) {
				K.State = KNOWNCREATURE_UPTODATE;
			}
			return State;
		}
	}

	return KNOWNCREATURE_FREE;
}

// 0x080cbb30  connections.c:409
uint32_t TConnection::NewKnownCreature(uint32_t NewID) {
	uint32_t OldID = 0;
	int FreeEntry = -1;

	for (int i = 0; i < 150; i++) {
		if (KnownCreatureTable[i].CreatureID == NewID) {
			FreeEntry = i;
			OldID = NewID;
			break;
		}
	}

	if (FreeEntry == -1) {
		for (int i = 0; i < 150; i++) {
			if (KnownCreatureTable[i].State == KNOWNCREATURE_FREE) {
				FreeEntry = i;
				OldID = KnownCreatureTable[i].CreatureID;
				break;
			}
		}
	}

	if (FreeEntry == -1) {
		for (int i = 0; i < 150; i++) {
			TCreature *cr = GetCreature(KnownCreatureTable[i].CreatureID);
			if (cr == nullptr) {
				error("TUserCom::NewKnownCreature: Bekannte Kreatur existiert nicht.\n");
			} else if (IsVisible(cr->posx, cr->posy, cr->posz)) {
				continue;
			}

			// The binary asks IsVisible a second time for a creature that the
			// first call reported as out of view.
			if (cr == nullptr || !IsVisible(cr->posx, cr->posy, cr->posz)) {
				FreeEntry = i;
				OldID = KnownCreatureTable[i].CreatureID;
				UnchainKnownCreature(OldID);
				break;
			}
		}
	}

	if (FreeEntry == -1) {
		print(3, "KnownCreatureTable ausgelastet.\n");
		return 0;
	}

	if (KnownCreatureTable[FreeEntry].State != KNOWNCREATURE_FREE) {
		error("TUserCom::NewKnownCreature: Slot ist nicht gel\xf6scht.\n");
	}

	KnownCreatureTable[FreeEntry].CreatureID = NewID;
	KnownCreatureTable[FreeEntry].State = KNOWNCREATURE_UPTODATE;

	TCreature *cr = GetCreature(NewID);
	if (cr == nullptr) {
		error("TUserCom::NewKnownCreature: Kreatur %lu existiert nicht.\n", static_cast<unsigned long>(NewID));
	} else {
		KnownCreatureTable[FreeEntry].Next = cr->FirstKnowingConnection;
		cr->FirstKnowingConnection = &KnownCreatureTable[FreeEntry];
	}

	return OldID;
}

// 0x080cbd40  connections.c:465
void TConnection::ClearKnownCreatureTable(bool Unchain) {
	for (int i = 0; i < 150; i++) {
		TKnownCreature &K = KnownCreatureTable[i];
		if (Unchain && K.State != KNOWNCREATURE_FREE) {
			UnchainKnownCreature(K.CreatureID);
		}

		K.Connection = this;
		K.State = KNOWNCREATURE_FREE;
		K.CreatureID = 0;
	}
}

// 0x080cbdb0  connections.c:479
void TConnection::UnchainKnownCreature(uint32_t ID) {
	TCreature *cr = GetCreature(ID);
	if (cr == nullptr) {
		error("TUserCom::UnchainKnownCreature: Kreatur %lu existiert nicht.\n", static_cast<unsigned long>(ID));
		return;
	}

	TKnownCreature *KnownCreature = cr->FirstKnowingConnection;
	if (KnownCreature == nullptr) {
		error("TUserCom::UnchainKnownCreature: Kreatur %lu kennt niemand.\n", static_cast<unsigned long>(ID));
		return;
	}

	if (KnownCreature->Connection == this) {
		KnownCreature->State = KNOWNCREATURE_FREE;
		cr->FirstKnowingConnection = KnownCreature->Next;
		return;
	}

	while (KnownCreature->Next != nullptr) {
		if (KnownCreature->Next->Connection == this) {
			KnownCreature->Next->State = KNOWNCREATURE_FREE;
			KnownCreature->Next = KnownCreature->Next->Next;
			return;
		}
		KnownCreature = KnownCreature->Next;
	}

	error("TUserCom::UnchainKnownCreature: Kreatur %lu ist nicht bekannt.\n", static_cast<unsigned long>(ID));
}

// 0x080cbe50  connections.c:522
TConnection *AssignFreeConnection() {
	ConnectionMutex.down();
	for (int i = 0; i < 1100; i++) {
		if (Connections[i].GetState() == CONNECTION_FREE) {
			Connections[i].Assign();
			ConnectionMutex.up();
			return &Connections[i];
		}
	}

	ConnectionMutex.up();
	return nullptr;
}

// 0x080cbec0  connections.c:537
TConnection *GetFirstConnection() {
	ConnectionIterator = -1;
	return GetNextConnection();
}

// 0x080cbf10  connections.c:546
TConnection *GetNextConnection() {
	while (ConnectionIterator < 1099) {
		ConnectionIterator++;
		if (Connections[ConnectionIterator].GetState() != CONNECTION_FREE) {
			return &Connections[ConnectionIterator];
		}
	}

	return nullptr;
}

// 0x080cbf60  connections.c:558
void ProcessConnections() {
	TConnection *Connection = GetFirstConnection();
	while (Connection != nullptr) {
		Connection->Process();
		Connection = GetNextConnection();
	}
}

// 0x080cbfd0  connections.c:572
void InitConnections() {
	for (int i = 0; i < 1100; i++) {
		Connections[i].Free();
	}

	FirstSendingConnection = nullptr;
}

// 0x080cc000  connections.c:581
void ExitConnections() {
}
