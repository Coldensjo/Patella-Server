#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <time.h>
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
#include "TXTEASymmetricKey.hh"
#include "crplayer.hh"
#include "crmain.hh"
#include "sending.hh"
#include "writer.hh"
#include "houses.hh"
#include "moveuse.hh"
#include "shm.hh"
#include "strings.hh"
#include "mudinc.hh"

// LoadDepot, SaveDepot and TPlayer::MovePossible are declared throw(RESULT)
// in the original, and InitPlayer has an exception specification as well.
// C++17 has no dynamic exception specifications, so an exception of another
// type now propagates instead of ending in std::unexpected().

const int PLAYERINDEX_LEAFSIZE = 10;
const int PLAYERDATA_POOLSIZE = 2000;
const int PLAYERKILLINGMARK_DELAY = 5;
const int TALKBUFFER_LINES = 4;
const int TALKBUFFER_MILLISECONDS_PER_LINE = 2500;

struct TPlayerIndexEntry {
	char Name[30];
	uint32_t CharacterID;
};

struct TPlayerIndexNode {
	bool InternalNode;
};

struct TPlayerIndexInternalNode : public TPlayerIndexNode {
	TPlayerIndexNode *Child[27];
};

struct TPlayerIndexLeafNode : public TPlayerIndexNode {
	int Count;
	TPlayerIndexEntry Entry[10];
};

vector<TPlayer *> PlayerList(0, 100, 10, nullptr);
int FirstFreePlayer;
Semaphore PlayerMutex(1);
TPlayerData PlayerDataPool[2000];
Semaphore PlayerDataPoolMutex(1);
store<TPlayerIndexInternalNode, 100> PlayerIndexInternalNodes;
store<TPlayerIndexLeafNode, 100> PlayerIndexLeafNodes;
TPlayerIndexInternalNode PlayerIndexHead;

int GetNumberOfPlayers() {
	return FirstFreePlayer;
}

TPlayer *GetPlayer(char *Name) {
	for (int i = 0; i < FirstFreePlayer; i++) {
		TPlayer *pl = PlayerList(i);
		if (stricmp(pl->Name, Name, -1) == 0) {
			return pl;
		}
	}

	return nullptr;
}

TPlayer *GetPlayer(uint32_t CharacterID) {
	TCreature *cr = GetCreature(CharacterID);
	if (cr == nullptr) {
		return nullptr;
	}

	if (cr->Type != PLAYER) {
		error("GetPlayer: Kreatur ist kein Spieler.\n");
		return nullptr;
	}

	return (TPlayer *)cr;
}

bool IsPlayerOnline(char *Name) {
	PlayerMutex.down();
	bool IsOnline = GetPlayer(Name) != nullptr;
	PlayerMutex.up();
	return IsOnline;
}

int IdentifyPlayer(char *Name, bool ExactMatch, bool IgnoreGamemasters, TPlayer *&pl) {
	if (Name == nullptr) {
		error("IdentifyPlayer: Name ist NULL.\n");
		return -1;
	}

	if (Name[0] == 0) {
		error("IdentifyPlayer: Name ist leer.\n");
		return -1;
	}

	if (!ExactMatch) {
		if (Name[strlen(Name) - 1] == '~') {
			Name[strlen(Name) - 1] = 0;
		} else {
			ExactMatch = true;
		}
	}

	int Hits = 0;
	for (int i = 0; i < FirstFreePlayer; i++) {
		char *TestName = PlayerList(i)->Name;
		if (stricmp(TestName, Name, -1) == 0) {
			pl = PlayerList(i);
			return 0;
		}

		if (!ExactMatch && stricmp(TestName, Name, (int)strlen(Name)) == 0) {
			if (IgnoreGamemasters && CheckRight(PlayerList(i)->ID, NO_STATISTICS)) {
				continue;
			}

			Hits++;
			pl = PlayerList(i);
		}
	}

	if (Hits == 1) {
		return 0;
	}

	if (Hits > 1) {
		return -2;
	}

	return -1;
}

void LogoutAllPlayers() {
	print(1, "LogoutAllPlayers: Werde alle Spieler ausloggen!\n");

	int i = 0;
	while (i < FirstFreePlayer) {
		TPlayer *pl = PlayerList(i);
		if (pl == nullptr) {
			error("LogoutAllPlayers: Eintrag %d in der Playerlist ist NULL.\n", i);
			i++;
			continue;
		}

		// The destructor takes the player out of the list, so i stays.
		delete pl;
	}
}

void CloseProcessedRequests(uint32_t CharacterID) {
	for (int i = 0; i < FirstFreePlayer; i++) {
		TPlayer *pl = PlayerList(i);
		if (pl == nullptr) {
			error("CloseProcessedRequests: Spieler %d existiert nicht.\n", i);
			continue;
		}

		if (pl->Request != 0 && pl->RequestProcessingGamemaster == CharacterID) {
			SendCloseRequest(pl->GetConnection());
			pl->Request = 0;
		}
	}
}

void NotifyBuddies(uint32_t CharacterID, char *Name, bool Login) {
	if (Name == nullptr || Name[0] == 0) {
		error("NotifyBuddies: Name existiert nicht.\n");
		return;
	}

	bool Hide = CheckRight(CharacterID, NO_STATISTICS);
	for (int i = 0; i < FirstFreePlayer; i++) {
		TPlayer *Player = PlayerList(i);
		if (Player->GetConnection() == nullptr || Player->PlayerData == nullptr) {
			continue;
		}

		if (Hide && !CheckRight(Player->ID, READ_GAMEMASTER_CHANNEL)) {
			continue;
		}

		for (int j = 0; j < Player->PlayerData->Buddies; j++) {
			if (Player->PlayerData->Buddy[j] != CharacterID) {
				continue;
			}

			if (strcmp(Player->PlayerData->BuddyName[j], Name) == 0) {
				SendBuddyStatus(Player->GetConnection(), CharacterID, Login);
			} else {
				SendBuddyData(Player->GetConnection(), CharacterID, Name, Login);
				strcpy(Player->PlayerData->BuddyName[j], Name);
			}
		}
	}
}

void CreatePlayerList(bool Online) {
	char *PlayerNames = new char[FirstFreePlayer * 30];
	int *PlayerLevels = new int[FirstFreePlayer];
	int *PlayerProfessions = new int[FirstFreePlayer];
	int NumberOfPlayers = -1;
	if (Online) {
		NumberOfPlayers = 0;
		for (int i = 0; i < FirstFreePlayer; i++) {
			TPlayer *Player = PlayerList(i);
			if (CheckRight(Player->ID, NO_STATISTICS)) {
				continue;
			}

			strcpy(&PlayerNames[NumberOfPlayers * 30], Player->Name);
			PlayerLevels[NumberOfPlayers] = Player->Skills[0]->Get();
			PlayerProfessions[NumberOfPlayers] = Player->GetActiveProfession();
			NumberOfPlayers++;
		}
	}

	PlayerlistOrder(NumberOfPlayers, PlayerNames, PlayerLevels, PlayerProfessions);

	if (Online) {
		Log("load", "%ld %d\n", (long)static_cast<int32_t>(time(nullptr)), FirstFreePlayer);
	}
}

void PrintPlayerPositions() {
	for (int i = 0; i < FirstFreePlayer; i++) {
		TPlayer *Player = PlayerList(i);
		Log("players", "[%d,%d,%d] %d %d\n", Player->posx, Player->posy, Player->posz,
				Player->Skills[0]->Get(), Player->GetActiveProfession());
	}
}

void LoadDepot(TPlayerData *PlayerData, int DepotNr, Object Con) {
	if (PlayerData == nullptr) {
		error("LoadDepot: PlayerData ist NULL.\n");
		throw ERROR;
	}

	if (!Con.exists()) {
		error("LoadDepot: \xdc" "bergebener Container existiert nicht.\n");
		throw ERROR;
	}

	if (!Con.getFlag(CONTAINER)) {
		error("LoadDepot: \xdc" "bergebenes Objekt ist kein Container.\n");
		throw ERROR;
	}

	if (DepotNr < 0 || DepotNr > 8) {
		error("LoadDepot: Ung\xfcltige Depotnummer %d.\n", DepotNr);
		throw ERROR;
	}

	if (PlayerData->Depot[DepotNr] == nullptr) {
		Create(Con, GetSpecialObject(DEPOT_CHEST), 0);
		return;
	}

	try {
		TReadBuffer Buffer(PlayerData->Depot[DepotNr], PlayerData->DepotSize[DepotNr]);
		LoadObjects(Buffer, Con);
	} catch (const char *s) {
		error("LoadDepot: Kann Depot nicht lesen (%s).\n", s);
		throw ERROR;
	}
}

void SaveDepot(TPlayerData *PlayerData, int DepotNr, Object Con) {
	if (PlayerData == nullptr) {
		error("SaveDepot: PlayerData ist NULL.\n");
		throw ERROR;
	}

	if (!Con.exists()) {
		error("SaveDepot: \xdc" "bergebener Container existiert nicht.\n");
		throw ERROR;
	}

	if (!Con.getFlag(CONTAINER)) {
		error("SaveDepot: \xdc" "bergebenes Objekt ist kein Container.\n");
		throw ERROR;
	}

	if (DepotNr < 0 || DepotNr > 8) {
		error("SaveDepot: Ung\xfcltige Depotnummer %d.\n", DepotNr);
		throw ERROR;
	}

	PlayerData->Dirty = true;
	if (PlayerData->Depot[DepotNr] != nullptr) {
		delete[] PlayerData->Depot[DepotNr];
		PlayerData->Depot[DepotNr] = nullptr;
	}

	try {
		TDynamicWriteBuffer HelpBuffer(16384);
		SaveObjects(GetFirstContainerObject(Con), HelpBuffer, false);
		PlayerData->DepotSize[DepotNr] = HelpBuffer.getDataSize();
		PlayerData->Depot[DepotNr] = new unsigned char[PlayerData->DepotSize[DepotNr]];
		memcpy(PlayerData->Depot[DepotNr], HelpBuffer.getData(), PlayerData->DepotSize[DepotNr]);
	} catch (const char *s) {
		error("SaveDepot: Kann Depot nicht schreiben (%s).\n", s);
		PlayerData->Depot[DepotNr] = nullptr;
		throw ERROR;
	}
}

void GetProfessionName(char *Buffer, int Profession, bool Article, bool Capitals) {
	if (Article) {
		if (Profession == 11 || Profession == 14) {
			strcpy(Buffer, "an ");
		} else {
			strcpy(Buffer, "a ");
		}
	} else {
		Buffer[0] = 0;
	}

	switch (Profession) {
		case 0:
			strcat(Buffer, "None");
			break;
		case 1:
			strcat(Buffer, "Knight");
			break;
		case 2:
			strcat(Buffer, "Paladin");
			break;
		case 3:
			strcat(Buffer, "Sorcerer");
			break;
		case 4:
			strcat(Buffer, "Druid");
			break;
		case 11:
			strcat(Buffer, "Elite Knight");
			break;
		case 12:
			strcat(Buffer, "Royal Paladin");
			break;
		case 13:
			strcat(Buffer, "Master Sorcerer");
			break;
		case 14:
			strcat(Buffer, "Elder Druid");
			break;
		default:
			error("GetProfessionName: Unbekannter Beruf %d.\n", Profession);
			strcat(Buffer, "None");
			break;
	}

	if (!Capitals) {
		strLower(Buffer);
	}
}

void SendExistingRequests(TConnection *Connection) {
	if (Connection == nullptr) {
		error("SendExistingRequests: Verbindung ist NULL.\n");
		return;
	}

	int Requests = 0;
	TPlayer *Player[FirstFreePlayer];
	for (int i = 0; i < FirstFreePlayer; i++) {
		TPlayer *pl = PlayerList(i);
		if (pl == nullptr) {
			error("SendExistingRequests: Spieler %d existiert nicht.\n", i);
			continue;
		}

		if (pl->Request != 0 && pl->RequestProcessingGamemaster == 0) {
			Player[Requests] = pl;
			Requests++;
		}
	}

	for (int i = 0; i < Requests - 1; i++) {
		for (int j = i + 1; j < Requests; j++) {
			if (Player[i]->RequestTimestamp > Player[j]->RequestTimestamp) {
				TPlayer *Help = Player[i];
				Player[i] = Player[j];
				Player[j] = Help;
			}
		}
	}

	for (int i = 0; i < Requests; i++) {
		SendTalk(Connection, 0, Player[i]->Name, TALK_GAMEMASTER_REQUEST,
				GetDynamicString(Player[i]->Request), (int)(RoundNr - Player[i]->RequestTimestamp));
	}
}

void InitPlayerPool() {
	for (int i = 0; i < PLAYERDATA_POOLSIZE; i++) {
		PlayerDataPool[i].CharacterID = 0;
	}
}

void SavePlayerPoolSlot(TPlayerData *Slot) {
	if (Slot == nullptr) {
		error("SavePlayerPoolSlot: Slot existiert nicht.\n");
		return;
	}

	if (Slot->Dirty) {
		SavePlayerData(Slot);
		Slot->Dirty = false;
	}
}

void FreePlayerPoolSlot(TPlayerData *Slot) {
	if (Slot == nullptr) {
		error("FreePlayerPoolSlot: Slot ist NULL.\n");
		return;
	}

	print(3, "Gebe Slot von Charakter %ld frei.\n", (long)Slot->CharacterID);
	if (Slot->CharacterID == 0) {
		return;
	}

	if (Slot->Sticky > 0) {
		error("FreePlayerPoolSlot: Slot wird noch ben\xf6tigt; darf nicht freigegeben werden.\n");
		return;
	}

	if (Slot->Locked != 0 && Slot->Locked != gettid()) {
		error("FreePlayerPoolSlot: Slot ist von einem anderen Thread gesperrt.\n");
		return;
	}

	SavePlayerPoolSlot(Slot);

	if (Slot->Inventory != nullptr) {
		delete[] Slot->Inventory;
	}

	for (int i = 0; i < 9; i++) {
		if (Slot->Depot[i] != nullptr) {
			delete[] Slot->Depot[i];
		}
	}

	Slot->CharacterID = 0;
}

TPlayerData *GetPlayerPoolSlot(uint32_t CharacterID) {
	if (CharacterID == 0) {
		error("GetPlayerPoolSlot: CharacterID ist Null.\n");
		return nullptr;
	}

	for (int i = 0; i < PLAYERDATA_POOLSIZE; i++) {
		if (PlayerDataPool[i].CharacterID == CharacterID) {
			return &PlayerDataPool[i];
		}
	}

	return nullptr;
}

TPlayerData *AssignPlayerPoolSlot(uint32_t CharacterID, bool DontWait) {
	if (CharacterID == 0) {
		error("AssignPlayerPoolSlot: CharacterID ist Null.\n");
		return nullptr;
	}

	print(3, "Reserviere Slot f\xfcr Charakter %ld.\n", (long)CharacterID);

	TPlayerData *Slot;
again:
	PlayerDataPoolMutex.down();
	Slot = GetPlayerPoolSlot(CharacterID);
	if (Slot != nullptr) {
		if (Slot->Locked != 0) {
			if (DontWait) {
				Slot->Sticky++;
			}
			PlayerDataPoolMutex.up();
			if (DontWait) {
				return Slot;
			}
			DelayThread(0, 100);
			goto again;
		}

		Slot->Locked = gettid();
		PlayerDataPoolMutex.up();
		return Slot;
	}

	for (int i = 0; i < PLAYERDATA_POOLSIZE; i++) {
		if (PlayerDataPool[i].CharacterID == 0) {
			Slot = &PlayerDataPool[i];
			memset(Slot, 0, sizeof(TPlayerData));
			Slot->CharacterID = CharacterID;
			Slot->Locked = gettid();
			Slot->Dirty = false;
			Slot->Sticky = 0;
			PlayerDataPoolMutex.up();
			break;
		}
	}

	if (Slot == nullptr) {
		for (int i = 0; i < PLAYERDATA_POOLSIZE; i++) {
			if (PlayerDataPool[i].Sticky == 0 && !PlayerDataPool[i].Dirty && PlayerDataPool[i].Locked == 0) {
				Slot = &PlayerDataPool[i];
				FreePlayerPoolSlot(Slot);
				memset(Slot, 0, sizeof(TPlayerData));
				Slot->CharacterID = CharacterID;
				Slot->Locked = gettid();
				PlayerDataPoolMutex.up();
				break;
			}
		}
	}

	if (Slot == nullptr) {
		for (int i = 0; i < PLAYERDATA_POOLSIZE; i++) {
			if (PlayerDataPool[i].Sticky == 0 && PlayerDataPool[i].Locked == 0) {
				Slot = &PlayerDataPool[i];
				FreePlayerPoolSlot(Slot);
				memset(Slot, 0, sizeof(TPlayerData));
				Slot->CharacterID = CharacterID;
				Slot->Locked = gettid();
				PlayerDataPoolMutex.up();
				break;
			}
		}
	}

	if (Slot == nullptr) {
		error("AssignPlayerPoolSlot: Kein Slot mehr frei.\n");
		PlayerDataPoolMutex.up();
		return nullptr;
	}

	print(3, "Lade Daten f\xfcr Spieler %ld.\n", (long)CharacterID);
	if (!PlayerDataExists(CharacterID)) {
		Log("game", "Spieler %ld loggt sich zum ersten Mal ein.\n", (long)CharacterID);
	}

	if (!LoadPlayerData(Slot)) {
		Slot->CharacterID = 0;
		Slot->Locked = 0;
		return nullptr;
	}

	return Slot;
}

TPlayerData *AttachPlayerPoolSlot(uint32_t CharacterID, bool DontWait) {
	if (CharacterID == 0) {
		error("AttachPlayerPoolSlot: CharacterID ist Null.\n");
		return nullptr;
	}

	print(3, "Attache Slot von Spieler %ld.\n", (long)CharacterID);

	TPlayerData *Slot;
again:
	PlayerDataPoolMutex.down();
	Slot = GetPlayerPoolSlot(CharacterID);
	if (Slot == nullptr) {
		PlayerDataPoolMutex.up();
		error("AttachPlayerPoolSlot: Daten des Charakters sind nicht vorhanden.\n");
		return nullptr;
	}

	if (Slot->Locked != 0) {
		PlayerDataPoolMutex.up();
		if (DontWait) {
			return nullptr;
		}
		DelayThread(0, 100);
		goto again;
	}

	Slot->Locked = gettid();
	PlayerDataPoolMutex.up();
	return Slot;
}

void AttachPlayerPoolSlot(TPlayerData *Slot, bool DontWait) {
	if (Slot == nullptr) {
		error("AttachPlayerPoolSlot: Slot ist NULL.\n");
		return;
	}

	print(3, "Attache Slot von Spieler %ld.\n", (long)Slot->CharacterID);

again:
	PlayerDataPoolMutex.down();
	if (Slot->Locked != 0) {
		PlayerDataPoolMutex.up();
		if (DontWait) {
			return;
		}
		DelayThread(0, 100);
		goto again;
	}

	Slot->Locked = gettid();
	PlayerDataPoolMutex.up();
}

void IncreasePlayerPoolSlotSticky(TPlayerData *Slot) {
	if (Slot == nullptr) {
		error("IncreasePlayerPoolSlotSticky: Slot ist NULL.\n");
		return;
	}

	PlayerDataPoolMutex.down();
	Slot->Sticky++;
	PlayerDataPoolMutex.up();
}

void DecreasePlayerPoolSlotSticky(TPlayerData *Slot) {
	if (Slot == nullptr) {
		error("DecreasePlayerPoolSlotSticky: Slot ist NULL.\n");
		return;
	}

	PlayerDataPoolMutex.down();
	Slot->Sticky--;
	PlayerDataPoolMutex.up();
}

void DecreasePlayerPoolSlotSticky(uint32_t CharacterID) {
	if (CharacterID == 0) {
		error("DecreasePlayerPoolSlotSticky: CharacterID ist Null.\n");
		return;
	}

	PlayerDataPoolMutex.down();
	TPlayerData *Slot = GetPlayerPoolSlot(CharacterID);
	if (Slot != nullptr) {
		Slot->Sticky--;
	} else {
		error("DecreasePlayerPoolSlotSticky: Slot von Spieler %ld nicht gefunden.\n", (long)CharacterID);
	}
	PlayerDataPoolMutex.up();
}

void ReleasePlayerPoolSlot(TPlayerData *Slot) {
	if (Slot == nullptr) {
		error("ReleasePlayerPoolSlot: Slot ist NULL.\n");
		return;
	}

	print(3, "Gebe Slot von Spieler %ld frei.\n", (long)Slot->CharacterID);
	if (Slot->Locked == 0) {
		error("ReleasePlayerPoolSlot: Slot ist nicht gesperrt.\n");
		return;
	}

	if (Slot->Locked != gettid()) {
		error("ReleasePlayerPoolSlot: Slot ist von einem anderen Thread gesperrt.\n");
		return;
	}

	Slot->Locked = 0;
}

void SavePlayerPoolSlots() {
	print(3, "Speichere alle Spielerdaten...\n");
	for (int i = 0; i < PLAYERDATA_POOLSIZE; i++) {
		TPlayerData &P = PlayerDataPool[i];
		if (P.CharacterID == 0 || P.Sticky > 0 || P.Locked != 0 || !P.Dirty) {
			continue;
		}

		if (static_cast<int32_t>(P.LastLogoutTime) > static_cast<int32_t>(time(nullptr)) - 900) {
			continue;
		}

		AttachPlayerPoolSlot(&P, true);
		if (P.Locked == gettid()) {
			SavePlayerPoolSlot(&P);
			ReleasePlayerPoolSlot(&P);
		}
	}
}

void ExitPlayerPool() {
	for (int i = 0; i < PLAYERDATA_POOLSIZE; i++) {
		uint32_t CharacterID = PlayerDataPool[i].CharacterID;
		if (CharacterID == 0) {
			continue;
		}

		while (PlayerDataPool[i].Locked != 0) {
			print(2, "Warte auf Freigabe von Slot %d...\n", i);
			DelayThread(1, 0);
		}

		while (PlayerDataPool[i].Sticky > 0) {
			DelayThread(1, 0);
			AttachPlayerPoolSlot(&PlayerDataPool[i], false);
			SendMails(&PlayerDataPool[i]);
			DecreasePlayerPoolSlotSticky(&PlayerDataPool[i]);
			ReleasePlayerPoolSlot(&PlayerDataPool[i]);
		}

		AttachPlayerPoolSlot(&PlayerDataPool[i], false);
		FreePlayerPoolSlot(&PlayerDataPool[i]);
		ReleasePlayerPoolSlot(&PlayerDataPool[i]);
	}

	print(1, "Alle Spielerdaten gespeichert.\n");
}

int GetPlayerIndexEntryNumber(char *Name, int Position) {
	if (Name == nullptr) {
		error("GetPlayerIndexEntryNumber: Name ist NULL.\n");
		return 0;
	}

	if (Position >= (int)strlen(Name)) {
		return 0;
	}

	char c = Name[Position];
	if (c >= 'A' && c <= 'Z') {
		return c - 'A' + 1;
	}

	if (c >= 'a' && c <= 'z') {
		return c - 'a' + 1;
	}

	return 0;
}

void InsertPlayerIndex(TPlayerIndexInternalNode *Node, int Position, char *Name, uint32_t CharacterID) {
	if (Node == nullptr) {
		error("InsertPlayerIndex: Node ist NULL.\n");
		return;
	}

	if (Name == nullptr) {
		error("InsertPlayerIndex: Name ist NULL.\n");
		return;
	}

	int Number = GetPlayerIndexEntryNumber(Name, Position);
	TPlayerIndexNode *Child = Node->Child[Number];
	if (Child == nullptr) {
		TPlayerIndexLeafNode *Leaf = PlayerIndexLeafNodes.getFreeItem();
		Leaf->InternalNode = false;
		strcpy(Leaf->Entry[0].Name, Name);
		Leaf->Count = 1;
		Leaf->Entry[0].CharacterID = CharacterID;
		Node->Child[Number] = Leaf;
		return;
	}

	if (Child->InternalNode) {
		InsertPlayerIndex((TPlayerIndexInternalNode *)Child, Position + 1, Name, CharacterID);
		return;
	}

	TPlayerIndexLeafNode *LeafNode = (TPlayerIndexLeafNode *)Child;
	for (int i = 0; i < LeafNode->Count; i++) {
		if (strcmp(LeafNode->Entry[i].Name, Name) == 0) {
			return;
		}
	}

	if (LeafNode->Count < PLAYERINDEX_LEAFSIZE) {
		strcpy(LeafNode->Entry[LeafNode->Count].Name, Name);
		LeafNode->Entry[LeafNode->Count].CharacterID = CharacterID;
		LeafNode->Count++;
		return;
	}

	TPlayerIndexInternalNode *InternalNode = PlayerIndexInternalNodes.getFreeItem();
	InternalNode->InternalNode = true;
	for (int i = 0; i < 27; i++) {
		InternalNode->Child[i] = nullptr;
	}
	Node->Child[Number] = InternalNode;

	for (int i = 0; i < LeafNode->Count; i++) {
		InsertPlayerIndex(InternalNode, Position + 1, LeafNode->Entry[i].Name, LeafNode->Entry[i].CharacterID);
	}

	PlayerIndexLeafNodes.deleteItem(LeafNode);
	InsertPlayerIndex(InternalNode, Position + 1, Name, CharacterID);
}

TPlayerIndexEntry *SearchPlayerIndex(char *Name) {
	if (Name == nullptr) {
		error("SearchPlayerIndex: Name ist NULL.\n");
		return nullptr;
	}

	TPlayerIndexNode *Node = &PlayerIndexHead;
	int Position = 0;
	while (Node->InternalNode) {
		Node = ((TPlayerIndexInternalNode *)Node)->Child[GetPlayerIndexEntryNumber(Name, Position)];
		if (Node == nullptr) {
			return nullptr;
		}
		Position++;
	}

	TPlayerIndexLeafNode *LeafNode = (TPlayerIndexLeafNode *)Node;
	for (int i = 0; i < LeafNode->Count; i++) {
		if (stricmp(LeafNode->Entry[i].Name, Name, -1) == 0) {
			return &LeafNode->Entry[i];
		}
	}

	return nullptr;
}

void InitPlayerIndex() {
	PlayerIndexHead.InternalNode = true;
	for (int i = 0; i < 27; i++) {
		PlayerIndexHead.Child[i] = nullptr;
	}

	// The original does not delete the connection object on this path either.
	TQueryManagerConnection *Connection = new TQueryManagerConnection(360007);
	if (!Connection->isConnected()) {
		error("InitPlayerIndex: Kann nicht zum Query-Manager verbinden.\n");
		return;
	}

	uint32_t MinimumCharacterID = 0;
	int NumberOfPlayers;
	char Names[10000][30];
	uint32_t CharacterIDs[10000];
	while (true) {
		if (Connection->loadPlayers(MinimumCharacterID, NumberOfPlayers, Names, CharacterIDs) != 0) {
			error("InitPlayerIndex: Kann Spielerdaten nicht ermitteln.\n");
			break;
		}

		for (int i = 0; i < NumberOfPlayers; i++) {
			InsertPlayerIndex(&PlayerIndexHead, 0, Names[i], CharacterIDs[i]);
		}

		print(2, "%d Spielernamen geladen.\n", NumberOfPlayers);
		if (NumberOfPlayers < 10000) {
			break;
		}

		MinimumCharacterID = CharacterIDs[9999] + 1;
	}

	delete Connection;
}

bool PlayerExists(char *Name) {
	if (Name == nullptr) {
		error("PlayerExists: Name ist NULL.\n");
		return false;
	}

	TPlayerIndexEntry *Entry = SearchPlayerIndex(Name);
	return Entry != nullptr;
}

uint32_t GetCharacterID(char *Name) {
	if (Name == nullptr) {
		error("GetCharacterID: Name ist NULL.\n");
		return 0;
	}

	TPlayerIndexEntry *Entry = SearchPlayerIndex(Name);
	if (Entry == nullptr) {
		return 0;
	}

	return Entry->CharacterID;
}

char *GetCharacterName(char *Name) {
	if (Name == nullptr) {
		error("GetCharacterName: Name ist NULL.\n");
		return nullptr;
	}

	TPlayerIndexEntry *Entry = SearchPlayerIndex(Name);
	if (Entry == nullptr) {
		error("GetCharacterName: Spieler existiert nicht.\n");
		return "Unknown";
	}

	return Entry->Name;
}

TPlayer::TPlayer(TConnection *Connection, uint32_t CharacterID) : AttackedPlayers(0, 10, 10), FormerAttackedPlayers(0, 10, 10) {
	Profession = 0;
	ConstructError = NOERROR;
	Type = PLAYER;
	Depot = NONE;
	TradeObject = NONE;
	memset((void *)OpenContainer, 0, sizeof(OpenContainer));
	OldState = 0;
	memset(SpellList, 0, sizeof(SpellList));
	memset(QuestValues, 0, sizeof(QuestValues));
	Request = 0;
	RequestTimestamp = 0;
	RequestProcessingGamemaster = 0;
	TutorActivities = 0;
	LoseInventory = 1;
	NumberOfAttackedPlayers = 0;
	NumberOfFormerAttackedPlayers = 0;
	NumberOfMutings = 0;
	Guild[0] = 0;
	Rank[0] = 0;
	Title[0] = 0;
	Aggressor = false;
	FormerAggressor = false;
	PartyLeader = 0;
	TalkBufferFullTime = 0;
	MutingEndRound = 0;
	for (int i = 0; i < 20; i++) {
		Addressees[i] = 0;
	}

	PlayerData = AttachPlayerPoolSlot(CharacterID, false);
	if (PlayerData == nullptr) {
		error("TPlayer::TPlayer: PlayerData-Slot nicht gefunden.\n");
		ConstructError = ERROR;
		return;
	}

	SendMails(PlayerData);
	try {
		SetConnection(Connection);
		strcpy(Name, PlayerData->Name);
		SetID(CharacterID);
		InsertPlayerIndex(&PlayerIndexHead, 0, Name, CharacterID);
		LoadData();
		AccountID = PlayerData->AccountID;
		strcpy(IPAddress, Connection->GetIPAddress());
		memcpy(Rights, PlayerData->Rights, sizeof(Rights));
		if (CheckRight(ID, PREMIUM_ACCOUNT)) {
			Log("game", "Spieler besitzt Premium Account.\n");
		}

		Sex = PlayerData->Sex;
		strcpy(Guild, PlayerData->Guild);
		strcpy(Rank, PlayerData->Rank);
		strcpy(Title, PlayerData->Title);
		Request = 0;
		RequestTimestamp = 0;
		RequestProcessingGamemaster = 0;
		if (PlayerData->PlayerkillerEnd < static_cast<int32_t>(time(nullptr))) {
			PlayerData->PlayerkillerEnd = 0;
		}

		CheckOutfit();
		SetInList();

		Skills[22]->SetMax(GetActivePromotion() ? 200 : 100);
		Skills[22]->Check();
		int Rest = Skills[22]->TimerValue();
		if (Rest > 14) {
			int Interval = GetActivePromotion() ? 15 : 120;
			SetTimer(22, (Rest - 1) / Interval + 1, (Rest - 1) % Interval + 1, Interval, -1);
		}

		TSkill *sk = Skills[2];
		if (sk->Get() <= 0) {
			sk->Set(sk->GetMax());
			posx = startx;
			posy = starty;
			posz = startz;
			for (int i = 0; i < 25; i++) {
				DelTimer((unsigned short)i);
				Skills[i]->SetMDAct(0);
				Skills[i]->SetDAct(0);
			}
			Outfit = OrgOutfit;
			Skills[3]->Set(Skills[3]->GetMax());
		}

		if (PlayerData->LastLoginTime == 0 && CheckRight(ID, GAMEMASTER_OUTFIT)) {
			Log("game", "Gamemaster-Charakter %s loggt zum ersten Mal ein -> Level 2 setzen.\n", Name);
			Skills[0]->SetAct(2);
			Skills[0]->SetExp(100);
			Skills[0]->SetLastLevel(100);
			Skills[0]->SetNextLevel(200);
		}

		if (CoordinateFlag(posx, posy, posz, BED)) {
			Regenerate();
		}

		unsigned short HouseID = GetHouseID(posx, posy, posz);
		if (HouseID != 0
				&& !IsInvited(HouseID, this, static_cast<int32_t>(PlayerData->LastLogoutTime))
				&& !CheckRight(ID, ENTER_HOUSES)) {
			GetExitPosition(HouseID, posx, posy, posz);
		}

		if (!CheckRight(ID, PREMIUM_ACCOUNT)) {
			if (IsPremiumArea(startx, starty, startz)) {
				Log("game", "Spieler %s wird aus PayArea-Stadt ausgeb\xfcrgert und erh\xe4lt neue Heimatstadt.\n", Name);
				GetStartPosition(startx, starty, startz, GetEffectiveProfession() == 0);
			}

			if (IsPremiumArea(posx, posy, posz)) {
				Log("game", "Spieler %s wird aus PayArea geworfen und in seine Heimatstadt gesetzt.\n", Name);
				posx = startx;
				posy = starty;
				posz = startz;
			}
		}

		SetOnMap();
		Connection->EnterGame();
		SendInitGame(GetConnection(), ID);
		SendRights(GetConnection());
		SendFullScreen(GetConnection());
		GraphicalEffect(GetCreatureObject(), 11);
		LoadInventory(PlayerData->LastLoginTime == 0);
		NotifyChangeInventory();
		SendAmbiente(GetConnection());
		if (GetCreature(ID) == nullptr) {
			error("TPlayer::TPlayer: Kreatur nicht korrekt angelegt.\n");
		} else {
			AnnounceChangedCreature(ID, 2);
		}

		SendPlayerSkills(GetConnection());
		OldState = 0;
		CheckState();
		if (PlayerData->LastLoginTime == 0 && !CheckRight(ID, GAMEMASTER_OUTFIT)) {
			Log("game", "Spieler %s loggt zum ersten Mal ein -> Outfitwahl.\n", Name);
			SendMessage(GetConnection(), TALK_LOGIN_MESSAGE, "Welcome to Tibia! Please choose your outfit.");
			SendOutfit(GetConnection());
		}

		SendBuddies();
		if (PlayerData->LastLoginTime != 0) {
			char TimeString[100];
			struct tm *t = localtime(&PlayerData->LastLoginTime);
			strftime(TimeString, 100, "%d. %b %Y %X %Z", t);
			SendMessage(GetConnection(), TALK_LOGIN_MESSAGE, "Your last visit in Tibia: %s.", TimeString);
		}

		PlayerData->LastLoginTime = time(nullptr);
	} catch (RESULT r) {
		ConstructError = r;
	}
}

TPlayer::~TPlayer() {
	LogoutOrder(this);
	if (ConstructError != NOERROR) {
		DelInList();
		return;
	}

	PlayerData->LastLogoutTime = time(nullptr);
	SaveData();
	if (!IsDead) {
		Log("game", "Spieler %s loggt aus.\n", Name);
		GraphicalEffect(posx, posy, posz, 3);
		SaveInventory();
	} else {
		Log("game", "Spieler %s ist gestorben.\n", Name);
		if (PlayerData == nullptr) {
			error("TPlayer::~TPlayer: PlayerData ist NULL.\n");
		}

		if (PlayerData != nullptr && PlayerData->Inventory != nullptr) {
			delete[] PlayerData->Inventory;
			PlayerData->Inventory = nullptr;
		}

		if ((Profession != 0 && Skills[0]->Get() <= 5) || Skills[2]->GetMax() <= 0) {
			// The level line is logged in both cases, as in the original.
			if (Skills[2]->GetMax() <= 0) {
				Log("game", "Setze Spieler %s komplett zur\xfc" "ck wegen MaxHitpoints.\n", Name);
			}
			Log("game", "Setze Spieler %s komplett zur\xfc" "ck wegen Level.\n", Name);
			if (PlayerData != nullptr) {
				PlayerData->CurrentOutfit = PlayerData->OriginalOutfit;
				PlayerData->startx = 0;
				PlayerData->starty = 0;
				PlayerData->startz = 0;
				PlayerData->posx = 0;
				PlayerData->posy = 0;
				PlayerData->posz = 0;
				PlayerData->Profession = 0;
				memset(PlayerData->SpellList, 0, sizeof(PlayerData->SpellList));
				memset(PlayerData->QuestValues, 0, sizeof(PlayerData->QuestValues));
				for (int i = 0; i < 25; i++) {
					PlayerData->Minimum[i] = INT32_MIN;
				}
			}
			LoseInventory = 2;
		}

		if (PlayerData->PlayerkillerEnd != 0) {
			LoseInventory = 2;
		}

		if (CheckRight(ID, KEEP_INVENTORY)) {
			LoseInventory = 0;
		}
	}

	if (CheckRight(ID, READ_GAMEMASTER_CHANNEL)) {
		CloseProcessedRequests(ID);
	}

	if (Request != 0) {
		if (RequestProcessingGamemaster == 0) {
			DeleteGamemasterRequest(Name);
		} else {
			TPlayer *Gamemaster = GetPlayer(RequestProcessingGamemaster);
			if (Gamemaster != nullptr) {
				SendFinishRequest(Gamemaster->GetConnection(), Name);
			}
		}
		Request = 0;
	}

	LeaveAllChannels(ID);
	if (GetPartyLeader(false) != 0) {
		::LeaveParty(ID, true);
	}

	ClearPlayerkillingMarks();
	DelInList();
	PlayerData->Dirty = true;
	DecreasePlayerPoolSlotSticky(PlayerData);
	ReleasePlayerPoolSlot(PlayerData);
}

void TPlayer::SetInList() {
	SetInCrList();
	PlayerMutex.down();
	PlayerList(FirstFreePlayer) = this;
	FirstFreePlayer++;
	PlayerMutex.up();
	NotifyBuddies(ID, Name, true);
	IncrementPlayersOnline();
	if (Profession == 0) {
		IncrementNewbiesOnline();
	}
}

void TPlayer::DelInList() {
	NotifyBuddies(ID, Name, false);
	for (int i = 0; i < FirstFreePlayer; i++) {
		if (PlayerList(i) == this) {
			PlayerMutex.down();
			PlayerList(i) = PlayerList(FirstFreePlayer - 1);
			PlayerList(FirstFreePlayer - 1) = nullptr;
			FirstFreePlayer--;
			PlayerMutex.up();
			DecrementPlayersOnline();
			if (Profession == 0) {
				DecrementNewbiesOnline();
			}
			return;
		}
	}
}

void TPlayer::ClearConnection() {
	SetConnection(nullptr);
	if (Request != 0) {
		int GamemasterID = (int)RequestProcessingGamemaster;
		if (GamemasterID != 0) {
			TPlayer *Gamemaster = GetPlayer((uint32_t)GamemasterID);
			if (Gamemaster != nullptr) {
				SendFinishRequest(Gamemaster->GetConnection(), Name);
			}
		} else {
			DeleteGamemasterRequest(Name);
		}
		Request = 0;
	}
}

void TPlayer::LoadData() {
	if (PlayerData == nullptr) {
		error("TPlayer::LoadData: PlayerData ist NULL.\n");
		return;
	}

	Race = PlayerData->Race;
	OrgOutfit = PlayerData->OriginalOutfit;
	StartCoordinates();
	posx = startx;
	posy = starty;
	posz = startz;
	Direction = 2;
	SetSkills(Race);
	Outfit = PlayerData->CurrentOutfit;
	if (PlayerData->startx != 0) {
		startx = PlayerData->startx;
		starty = PlayerData->starty;
		startz = PlayerData->startz;
	}

	if (PlayerData->posx != 0) {
		posx = PlayerData->posx;
		posy = PlayerData->posy;
		posz = PlayerData->posz;
	}

	Profession = (unsigned char)PlayerData->Profession;
	memcpy(SpellList, PlayerData->SpellList, sizeof(SpellList));
	memcpy(QuestValues, PlayerData->QuestValues, sizeof(QuestValues));
	for (int SkillNr = 0; SkillNr < 25; SkillNr++) {
		if (PlayerData->Minimum[SkillNr] != INT32_MIN) {
			Skills[SkillNr]->Load(PlayerData->Actual[SkillNr], PlayerData->Maximum[SkillNr],
					PlayerData->Minimum[SkillNr], PlayerData->DeltaAct[SkillNr],
					PlayerData->MagicDeltaAct[SkillNr], PlayerData->Cycle[SkillNr],
					PlayerData->MaxCycle[SkillNr], PlayerData->Count[SkillNr],
					PlayerData->MaxCount[SkillNr], PlayerData->AddLevel[SkillNr],
					PlayerData->Experience[SkillNr], PlayerData->FactorPercent[SkillNr],
					PlayerData->NextLevel[SkillNr], PlayerData->Delta[SkillNr]);
		}
	}

	EarliestYellRound = PlayerData->EarliestYellRound;
	EarliestTradeChannelRound = PlayerData->EarliestTradeChannelRound;
	EarliestSpellTime = PlayerData->EarliestSpellTime;
	EarliestMultiuseTime = PlayerData->EarliestMultiuseTime;
	TalkBufferFullTime = PlayerData->TalkBufferFullTime;
	MutingEndRound = PlayerData->MutingEndRound;
	NumberOfMutings = PlayerData->NumberOfMutings;
}

void TPlayer::SaveData() {
	if (PlayerData == nullptr) {
		error("TPlayer::SaveData: PlayerData ist NULL.\n");
		return;
	}

	PlayerData->OriginalOutfit = OrgOutfit;
	PlayerData->CurrentOutfit = Outfit;
	PlayerData->startx = startx;
	PlayerData->starty = starty;
	PlayerData->startz = startz;
	PlayerData->posx = posx;
	PlayerData->posy = posy;
	PlayerData->posz = posz;
	PlayerData->Profession = Profession;
	memcpy(PlayerData->SpellList, SpellList, sizeof(SpellList));
	memcpy(PlayerData->QuestValues, QuestValues, sizeof(QuestValues));
	for (int SkillNr = 0; SkillNr < 25; SkillNr++) {
		if (Skills[SkillNr] == nullptr) {
			PlayerData->Minimum[SkillNr] = INT32_MIN;
		} else {
			Skills[SkillNr]->Save(PlayerData->Actual[SkillNr], PlayerData->Maximum[SkillNr],
					PlayerData->Minimum[SkillNr], PlayerData->DeltaAct[SkillNr],
					PlayerData->MagicDeltaAct[SkillNr], PlayerData->Cycle[SkillNr],
					PlayerData->MaxCycle[SkillNr], PlayerData->Count[SkillNr],
					PlayerData->MaxCount[SkillNr], PlayerData->AddLevel[SkillNr],
					PlayerData->Experience[SkillNr], PlayerData->FactorPercent[SkillNr],
					PlayerData->NextLevel[SkillNr], PlayerData->Delta[SkillNr]);
		}
	}

	PlayerData->EarliestYellRound = EarliestYellRound;
	PlayerData->EarliestTradeChannelRound = EarliestTradeChannelRound;
	PlayerData->EarliestSpellTime = EarliestSpellTime;
	PlayerData->EarliestMultiuseTime = EarliestMultiuseTime;
	PlayerData->TalkBufferFullTime = TalkBufferFullTime;
	PlayerData->MutingEndRound = MutingEndRound;
	PlayerData->NumberOfMutings = NumberOfMutings;
}

void TPlayer::LoadInventory(bool SetStandardInventory) {
	if (PlayerData == nullptr) {
		error("TPlayer::LoadInventory: PlayerData ist NULL.\n");
		return;
	}

	if (PlayerData->Inventory == nullptr) {
		if (Profession == 0 && !CheckRight(ID, ZERO_CAPACITY) && SetStandardInventory) {
			try {
				Create(GetBodyContainer(ID, 5), GetSpecialObject(INVENTORY_RIGHTHAND), 0);
				Create(GetBodyContainer(ID, 6), GetSpecialObject(INVENTORY_LEFTHAND), 0);
				Create(GetBodyContainer(ID, 4),
						GetSpecialObject(Sex == 1 ? INVENTORY_BODY_MALE : INVENTORY_BODY_FEMALE), 0);
				Object Con = Create(GetBodyContainer(ID, 3), GetSpecialObject(INVENTORY_CONTAINER), 0);
				Create(Con, GetSpecialObject(INVENTORY_FOOD), 1);
			} catch (RESULT r) {
				error("TPlayer::LoadInventory: Exception %d beim Erstellen des Standard-Inventorys.\n", r);
			}
		}
	} else {
		try {
			TReadBuffer Buffer(PlayerData->Inventory, PlayerData->InventorySize);
			while (true) {
				int Position = Buffer.readByte();
				if (Position == 0xFF) {
					break;
				}

				LoadObjects(Buffer, GetBodyContainer(ID, Position));
				Object Obj = GetBodyObject(ID, Position);
				if (Obj != NONE) {
					SendSetInventory(GetConnection(), Position, Obj);
				}
			}
		} catch (const char *s) {
			error("TPlayer::LoadInventory: Kann Inventory von Spieler %s nicht lesen.\n", Name);
			error("# Fehler: %s\n", s);
		}
	}
}

void TPlayer::SaveInventory() {
	if (PlayerData == nullptr) {
		error("TPlayer::SaveInventory: PlayerData ist NULL.\n");
		return;
	}

	if (PlayerData->Inventory != nullptr) {
		delete[] PlayerData->Inventory;
		PlayerData->Inventory = nullptr;
	}

	PlayerData->InventorySize = 0;
	if (GetCreatureObject() == NONE) {
		return;
	}

	try {
		TDynamicWriteBuffer HelpBuffer(16384);
		for (int con = 1; con <= 10; con++) {
			Object Obj = GetBodyObject(ID, con);
			if (Obj.exists()) {
				HelpBuffer.writeByte((unsigned char)con);
				SaveObjects(Obj, HelpBuffer, false);
			}
		}

		if (HelpBuffer.getDataSize() != 0) {
			HelpBuffer.writeByte(0xFF);
			PlayerData->InventorySize = HelpBuffer.getDataSize();
			PlayerData->Inventory = new unsigned char[PlayerData->InventorySize];
			memcpy(PlayerData->Inventory, HelpBuffer.getData(), PlayerData->InventorySize);
		}
	} catch (const char *s) {
		// Original bug: the player name for %s is not passed.
		error("TPlayer::SaveInventory: Kann Inventory von Spieler %s nicht schreiben.\n");
		error("# Fehler: %s\n", s);
	}
}

void TPlayer::StartCoordinates() {
	GetStartPosition(startx, starty, startz, true);
}

void TPlayer::TakeOver(TConnection *Connection) {
	Log("game", "Spieler %s \xfc" "bernimmt Verbindung.\n", Name);
	LoggingOut = false;
	LogoutAllowed = false;
	SetConnection(Connection);
	Connection->EnterGame();

	TPlayerData *PlayerData = GetPlayerPoolSlot(ID);
	if (PlayerData == nullptr) {
		error("TPlayer::TakeOver: PlayerData-Slot nicht gefunden.\n");
		return;
	}

	if (PlayerData->Locked != gettid()) {
		error("TPlayer::TakeOver: PlayerData-Slot ist nicht korrekt gesperrt (%d).\n", PlayerData->Locked);
	}

	if (PlayerData->Sticky < 2) {
		error("TPlayer::TakeOver: Falscher Sticky-Wert %d.\n", PlayerData->Sticky);
	} else {
		DecreasePlayerPoolSlotSticky(PlayerData);
	}

	strcpy(Name, PlayerData->Name);
	InsertPlayerIndex(&PlayerIndexHead, 0, Name, ID);
	strcpy(IPAddress, Connection->GetIPAddress());
	memcpy(Rights, PlayerData->Rights, sizeof(Rights));
	Sex = PlayerData->Sex;
	strcpy(Guild, PlayerData->Guild);
	strcpy(Rank, PlayerData->Rank);
	strcpy(Title, PlayerData->Title);
	CheckOutfit();
	CloseAllContainers();
	Combat.StopAttack(0);
	LeaveAllChannels(ID);
	if (Request != 0) {
		if (RequestProcessingGamemaster == 0) {
			DeleteGamemasterRequest(Name);
		} else {
			TPlayer *Gamemaster = GetPlayer(RequestProcessingGamemaster);
			if (Gamemaster != nullptr) {
				SendFinishRequest(Gamemaster->GetConnection(), Name);
			}
		}
		Request = 0;
	}

	RequestTimestamp = 0;
	RequestProcessingGamemaster = 0;
	RejectTrade();
	if (CheckRight(ID, READ_GAMEMASTER_CHANNEL)) {
		CloseProcessedRequests(ID);
	}

	SendInitGame(GetConnection(), ID);
	SendRights(GetConnection());
	SendFullScreen(GetConnection());
	SendBodyInventory(GetConnection(), ID);
	SendAmbiente(GetConnection());
	SendPlayerData(GetConnection());
	SendPlayerSkills(GetConnection());
	OldState = 0;
	CheckState();
	SendBuddies();
}

void TPlayer::Death() {
	if (CheckRight(ID, INVULNERABLE)) {
		error("TPlayer::Death: Aha, so geht das aber nicht!! Goetter kann man nicht toeten!!\n");
		Skills[2]->Set(Skills[2]->GetMax());
		return;
	}

	if (GetConnection() != nullptr) {
		SendPlayerData(GetConnection());
		SendMessage(GetConnection(), TALK_EVENT_MESSAGE, "You are dead.\n");
		GetConnection()->Die();
	}

	TCreature::Death();
	if (WorldType == PVP_ENFORCED) {
		Combat.DistributeExperiencePoints((uint32_t)(Skills[0]->GetExp() / 20));
	}

	int Percent = GetActivePromotion() ? 7 : 10;
	for (int Quest = 101; Quest <= 105; Quest++) {
		if (GetQuestValue(Quest) != 0) {
			Percent--;
			SetQuestValue(Quest, 0);
		}
	}

	TSkill *sk = Skills[8];
	sk->Decrease((int)((int64_t)sk->GetExp() * Percent / 100));
	sk = Skills[10];
	sk->Decrease((int)((int64_t)sk->GetExp() * Percent / 100));
	sk = Skills[9];
	sk->Decrease((int)((int64_t)sk->GetExp() * Percent / 100));
	sk = Skills[7];
	sk->Decrease((int)((int64_t)sk->GetExp() * Percent / 100));
	sk = Skills[11];
	sk->Decrease((int)((int64_t)sk->GetExp() * Percent / 100));
	sk = Skills[6];
	sk->Decrease((int)((int64_t)sk->GetExp() * Percent / 100));
	sk = Skills[1];
	sk->Decrease((int)((int64_t)sk->GetExp() * Percent / 100));
	sk = Skills[13];
	sk->Decrease((int)((int64_t)sk->GetExp() * Percent / 100));
	sk = Skills[0];
	sk->Decrease((int)((int64_t)sk->GetExp() * Percent / 100));
}

void TPlayer::AttackStimulus(uint32_t) {
	if (!IsDead) {
		BlockLogout(60, false);
	}
}

void TPlayer::DamageStimulus(uint32_t, int, int) {
	if (!IsDead) {
		BlockLogout(60, false);
	}
}

void TPlayer::IdleStimulus() {
	if (Combat.GetAttackDest() == 0) {
		return;
	}

	try {
		ToDoAttack();
		ToDoStart();
	} catch (RESULT r) {
		ToDoClear();
		if (r != NOERROR) {
			if (r != NOWAY) {
				SendResult(GetConnection(), r);
			}
			ToDoWait(1000);
			ToDoStart();
		}
	}
}

void TPlayer::SetOpenContainer(int Window, Object Con) {
	if (Window < 0 || Window > 15) {
		// Original bug: the window number for %d is not passed.
		error("TPlayer::SetOpenContainer: Ung\xfcltige Fensternummer %d.\n");
		return;
	}

	if (Con != NONE && (!Con.exists() || !Con.getFlag(CONTAINER))) {
		error("TPlayer::SetOpenContainer: Container existiert nicht.\n");
		return;
	}

	OpenContainer[Window] = Con;
}

Object TPlayer::GetOpenContainer(int Window) {
	if (Window < 0 || Window > 15) {
		// Original bug: the window number for %d is not passed.
		error("TPlayer::GetOpenContainer: Ung\xfcltige Fensternummer %d.\n");
		return NONE;
	}

	return OpenContainer[Window];
}

void TPlayer::CloseAllContainers() {
	for (int i = 0; i < 16; i++) {
		if (GetOpenContainer(i) != NONE) {
			SetOpenContainer(i, NONE);
			SendCloseContainer(GetConnection(), i);
		}
	}
}

Object TPlayer::InspectTrade(bool OwnOffer, int Position) {
	if (TradeObject == NONE) {
		return NONE;
	}

	Object Obj;
	if (OwnOffer) {
		Obj = TradeObject;
	} else {
		if (TradePartner == 0) {
			return NONE;
		}

		TPlayer *Partner = GetPlayer(TradePartner);
		if (Partner == nullptr) {
			return NONE;
		}

		if (Partner->TradeObject == NONE || Partner->TradePartner != ID) {
			return NONE;
		}

		Obj = Partner->TradeObject;
	}

	while (Obj != NONE && Position > 0) {
		int Count = CountObjects(Obj);
		if (Position < Count) {
			Position--;
			Obj = GetFirstContainerObject(Obj);
		} else {
			Position -= Count;
			Obj++;
		}
	}

	return Obj;
}

void TPlayer::AcceptTrade() {
	if (TradeObject == NONE) {
		return;
	}

	TPlayer *Partner = GetPlayer(TradePartner);
	if (Partner == nullptr) {
		return;
	}

	if (Partner->TradeObject == NONE || Partner->TradePartner != ID) {
		return;
	}

	TradeAccepted = true;
	if (!Partner->TradeAccepted) {
		return;
	}

	Object Obj[2], Con[2];
	TPlayer *Player[2];
	Obj[0] = TradeObject;
	Obj[1] = Partner->TradeObject;
	Player[0] = this;
	Player[1] = Partner;
	Con[0] = NONE;
	Con[1] = NONE;
	for (int p = 0; p < 2; p++) {
		try {
			if (!ObjectAccessible(Player[p]->ID, Obj[p], 1)) {
				throw NOTACCESSIBLE;
			}

			if (Obj[p].getFlag(UNMOVE)) {
				throw NOTMOVABLE;
			}

			if (!Obj[p].getFlag(TAKE)) {
				throw NOTTAKABLE;
			}

			if (CheckRight(Player[1 - p]->ID, ZERO_CAPACITY)) {
				throw TOOHEAVY;
			}

			if (!CheckRight(Player[1 - p]->ID, UNLIMITED_CAPACITY)) {
				TSkill *sk = Player[1 - p]->Skills[5];
				if (sk == nullptr) {
					error("TPlayer::AcceptTrade: Skill CARRYSTRENGTH existiert nicht.\n");
					throw ERROR;
				}

				int NewWeight = GetInventoryWeight(Player[1 - p]->ID) + GetCompleteWeight(Obj[p]);
				if (GetObjectCreatureID(Obj[1 - p]) == Player[1 - p]->ID) {
					NewWeight -= GetCompleteWeight(Obj[1 - p]);
				}

				if (NewWeight > sk->Get() * 100) {
					throw TOOHEAVY;
				}
			}

			int pos = 0;
			bool first = true;
			bool invert = Obj[p].getFlag(MOVEMENTEVENT);
		again:
			pos++;
			try {
				Object Help = GetBodyObject(Player[1 - p]->ID, pos);
				if (first != invert && Help == NONE) {
					CheckInventoryPlace(Obj[p].getObjectType(), GetBodyContainer(Player[1 - p]->ID, pos), Obj[1 - p]);
					Con[1 - p] = GetBodyContainer(Player[1 - p]->ID, pos);
				}

				if (first == invert && Help != NONE && Help.getFlag(CONTAINER) && Help != Obj[1 - p]) {
					// The original asks for the container of Obj[1 - p] before it
					// counts the objects in Help.
					int Count = 0;
					if (Obj[1 - p].getContainer() == Help) {
						Count = -1;
					}
					Count += CountObjectsInContainer(Help);
					if (Count >= (int)Help.getAttribute(CAPACITY)) {
						throw CONTAINERFULL;
					}

					Con[1 - p] = Help;
				}
			} catch (RESULT) {
			}

			if (Con[1 - p] == NONE) {
				if (pos <= 9) {
					goto again;
				}

				if (!first) {
					throw NOROOM;
				}

				first = false;
				pos = 0;
				goto again;
			}
		} catch (RESULT r) {
			if (r == TOOHEAVY || r == NOROOM) {
				SendResult(Player[1 - p]->GetConnection(), r);
			} else {
				SendResult(Player[p]->GetConnection(), r);
			}

			TradeObject = NONE;
			Partner->TradeObject = NONE;
			SendCloseTrade(GetConnection());
			SendCloseTrade(Partner->GetConnection());
			return;
		}
	}

	TradeObject = NONE;
	Partner->TradeObject = NONE;
	SendCloseTrade(GetConnection());
	SendCloseTrade(Partner->GetConnection());

	int Step = 1;
	ObjectType Type0 = Obj[0].getObjectType();
	ObjectType Type1 = Obj[1].getObjectType();
	try {
		::Move(0, Obj[0], GetMapContainer(Player[1]->GetCreatureObject()), -1, true, NONE);
		Step = 2;
		::Move(0, Obj[1], GetMapContainer(Player[0]->GetCreatureObject()), -1, true, NONE);
		Step = 3;
		::Move(0, Obj[0], Con[1], -1, true, NONE);
		Step = 4;
		::Move(0, Obj[1], Con[0], -1, true, NONE);
	} catch (RESULT r) {
		error("TPlayer::AcceptTrade: Exception %d bei Schritt %d.\n", r, Step);
		error("# Handelspartner: %s und %s\n", Name, Partner->Name);
		error("# Objekte: %d gegen %d an [%d,%d,%d]\n", Type0.getTypeID(), Type1.getTypeID(), posx, posy, posz);
		error("# Zielcontainer: %d und %d\n", Con[0].getObjectType().getTypeID(), Con[1].getObjectType().getTypeID());
	}
}

void TPlayer::RejectTrade() {
	if (TradeObject == NONE) {
		return;
	}

	TradeObject = NONE;
	TPlayer *Partner = GetPlayer(TradePartner);
	if (Partner != nullptr && Partner->TradeObject != NONE && Partner->TradePartner == ID) {
		Partner->TradeObject = NONE;
		SendCloseTrade(Partner->GetConnection());
		SendMessage(Partner->GetConnection(), TALK_FAILURE_MESSAGE, "Trade cancelled.");
	}
}

void TPlayer::ClearProfession() {
	if (Profession == 0) {
		return;
	}

	Profession = 0;
	Combat.CheckCombatValues();
	IncrementNewbiesOnline();
}

void TPlayer::SetProfession(unsigned char prof) {
	if (prof == 10) {
		if (Profession == 0) {
			error("TPlayer::SetProfession: Spieler hat noch keinen Beruf f\xfcr Veredelung.\n");
			return;
		}

		if (Profession >= 10) {
			error("TPlayer::SetProfession: Spieler hat seinen Beruf schon veredelt.\n");
			return;
		}

		Profession += 10;
		Combat.CheckCombatValues();
		Skills[22]->SetMax(200);
		int Rest = Skills[22]->TimerValue();
		if (Rest > 14) {
			SetTimer(22, (Rest - 1) / 15 + 1, (Rest - 1) % 15 + 1, 15, -1);
		}
		return;
	}

	if (Profession != 0) {
		error("TPlayer::SetProfession: Player '%s' hat bereits einen Beruf!\n", Name);
		return;
	}

	switch (prof) {
		case 1:
			Skills[8]->ChangeSkill(1100, 50);
			Skills[10]->ChangeSkill(1100, 50);
			Skills[9]->ChangeSkill(1100, 50);
			Skills[11]->ChangeSkill(1100, 50);
			Skills[7]->ChangeSkill(1400, 30);
			Skills[6]->ChangeSkill(1100, 100);
			Skills[2]->SetAddLevel(15);
			Skills[5]->SetAddLevel(25);
			Skills[3]->SetAddLevel(5);
			Skills[1]->ChangeSkill(3000, 1600);
			break;
		case 2:
			Skills[8]->ChangeSkill(1200, 50);
			Skills[10]->ChangeSkill(1200, 50);
			Skills[9]->ChangeSkill(1200, 50);
			Skills[11]->ChangeSkill(1200, 50);
			Skills[7]->ChangeSkill(1100, 30);
			Skills[6]->ChangeSkill(1100, 100);
			Skills[2]->SetAddLevel(10);
			Skills[5]->SetAddLevel(20);
			Skills[3]->SetAddLevel(15);
			Skills[1]->ChangeSkill(1400, 1600);
			break;
		case 3:
			Skills[8]->ChangeSkill(2000, 50);
			Skills[10]->ChangeSkill(2000, 50);
			Skills[9]->ChangeSkill(2000, 50);
			Skills[11]->ChangeSkill(1500, 50);
			Skills[7]->ChangeSkill(2000, 30);
			Skills[6]->ChangeSkill(1500, 100);
			Skills[2]->SetAddLevel(5);
			Skills[5]->SetAddLevel(10);
			Skills[3]->SetAddLevel(30);
			Skills[1]->ChangeSkill(1100, 1600);
			break;
		case 4:
			Skills[8]->ChangeSkill(1800, 50);
			Skills[10]->ChangeSkill(1800, 50);
			Skills[9]->ChangeSkill(1800, 50);
			Skills[11]->ChangeSkill(1500, 50);
			Skills[7]->ChangeSkill(1800, 30);
			Skills[6]->ChangeSkill(1500, 100);
			Skills[2]->SetAddLevel(5);
			Skills[5]->SetAddLevel(10);
			Skills[3]->SetAddLevel(30);
			Skills[1]->ChangeSkill(1100, 1600);
			break;
		default:
			error("TPlayer::SetProfession: Beruf %d existiert nicht!\n", prof);
			return;
	}

	Profession = prof;
	Combat.CheckCombatValues();
	DecrementNewbiesOnline();
}

unsigned char TPlayer::GetRealProfession() {
	return Profession;
}

unsigned char TPlayer::GetEffectiveProfession() {
	if (Profession >= 10) {
		return Profession - 10;
	}

	return Profession;
}

unsigned char TPlayer::GetActiveProfession() {
	if (CheckRight(ID, PREMIUM_ACCOUNT)) {
		return Profession;
	}

	return GetEffectiveProfession();
}

bool TPlayer::GetActivePromotion() {
	return CheckRight(ID, PREMIUM_ACCOUNT) && Profession >= 10;
}

bool TPlayer::SpellKnown(int SpellNumber) {
	if (SpellNumber < 0 || SpellNumber > 255) {
		error("TPlayer::SpellKnown: Ung\xfcltige Spruchnummer %d.\n", SpellNumber);
		return false;
	}

	return SpellList[SpellNumber] != 0;
}

void TPlayer::LearnSpell(int SpellNumber) {
	if (SpellNumber < 0 || SpellNumber > 255) {
		error("TPlayer::LearnSpell: Ung\xfcltige Spruchnummer %d.\n", SpellNumber);
		return;
	}

	if (SpellList[SpellNumber] != 0) {
		error("TPlayer::LearnSpell: Der Spieler kennt den Spruch schon.\n");
		return;
	}

	SpellList[SpellNumber] = 1;
}

int TPlayer::GetQuestValue(int Number) {
	if (Number < 0 || Number > 499) {
		error("TPlayer::GetQuestValue: Ung\xfcltige Nummer %d.\n", Number);
		return 0;
	}

	print(3, "Wert der Questvariablen %d von %s: %d.\n", Number, Name, QuestValues[Number]);
	return QuestValues[Number];
}

void TPlayer::SetQuestValue(int Number, int Value) {
	if (Number < 0 || Number > 499) {
		error("TPlayer::SetQuestValue: Ung\xfcltige Nummer %d.\n", Number);
		return;
	}

	print(3, "Neuer Wert f\xfcr Questvariable %d von %s: %d.\n", Number, Name, Value);
	QuestValues[Number] = Value;
}

void TPlayer::CheckOutfit() {
	if (CheckRight(ID, GAMEMASTER_OUTFIT)) {
		if (OrgOutfit.OutfitID == 75) {
			return;
		}

		OrgOutfit.OutfitID = 75;
		OrgOutfit.Colors[0] = 0;
		OrgOutfit.Colors[1] = 0;
		OrgOutfit.Colors[2] = 0;
		OrgOutfit.Colors[3] = 0;
	} else if (Sex == 1) {
		if (in(OrgOutfit.OutfitID, 128, 134)) {
			return;
		}

		OrgOutfit.OutfitID = 128;
		OrgOutfit.Colors[0] = 78;
		OrgOutfit.Colors[1] = 69;
		OrgOutfit.Colors[2] = 58;
		OrgOutfit.Colors[3] = 76;
	} else {
		if (in(OrgOutfit.OutfitID, 136, 142)) {
			return;
		}

		OrgOutfit.OutfitID = 136;
		OrgOutfit.Colors[0] = 78;
		OrgOutfit.Colors[1] = 69;
		OrgOutfit.Colors[2] = 58;
		OrgOutfit.Colors[3] = 76;
	}

	Outfit = OrgOutfit;
}

void TPlayer::CheckState() {
	if (GetConnection() == nullptr) {
		return;
	}

	int NewState = 0;
	if (Skills[17]->TimerValue() > 0) {
		NewState |= 1;
	}

	if (Skills[18]->TimerValue() > 0) {
		NewState |= 2;
	}

	if (Skills[19]->TimerValue() > 0) {
		NewState |= 4;
	}

	if (Skills[20]->TimerValue() > 0 && Skills[20]->Get() == 0) {
		NewState |= 8;
	}

	if (Skills[21]->TimerValue() > 0 || Skills[21]->Get() > 0) {
		NewState |= 16;
	}

	if (Skills[4]->GetMDAct() < 0) {
		NewState |= 32;
	}

	if (Skills[4]->GetMDAct() > 0) {
		NewState |= 64;
	}

	if (RoundNr < EarliestLogoutRound) {
		NewState |= 128;
	}

	if (NewState != OldState) {
		SendPlayerState(GetConnection(), (unsigned char)NewState);
		OldState = NewState;
	}
}

bool TPlayer::MovePossible(int x, int y, int z, bool Execute, bool Jump) {
	if (Jump) {
		if (!JumpPossible(x, y, z, false)) {
			return false;
		}
	} else {
		if (!::MovePossible(x, y, z)) {
			return false;
		}
	}

	if (Execute) {
		if (RoundNr < EarliestProtectionZoneRound
				&& IsProtectionZone(x, y, z)
				&& !IsProtectionZone(posx, posy, posz)) {
			throw ENTERPROTECTIONZONE;
		}

		unsigned short HouseID = GetHouseID(x, y, z);
		if (HouseID != 0 && !IsInvited(HouseID, this, INT32_MAX) && !CheckRight(ID, ENTER_HOUSES)) {
			throw NOTINVITED;
		}
	} else {
		if (CoordinateAvoid(x, y, z)) {
			return false;
		}
	}

	return true;
}

void TPlayer::AddBuddy(char *Name) {
	if (Name == nullptr) {
		error("TPlayer::AddBuddy: Name ist NULL.\n");
		return;
	}

	if (PlayerData == nullptr) {
		error("TPlayer::AddBuddy: PlayerData ist NULL.\n");
		return;
	}

	if (PlayerData->Buddies >= 100
			|| (!CheckRight(ID, READ_GAMEMASTER_CHANNEL) && PlayerData->Buddies >= 100)
			|| (!CheckRight(ID, PREMIUM_ACCOUNT) && PlayerData->Buddies >= 20)) {
		SendMessage(GetConnection(), TALK_FAILURE_MESSAGE, "You cannot add more buddies.");
		return;
	}

	TPlayer *pl;
	uint32_t CharacterID;
	char RealName[30];
	bool Online;
	if (IdentifyPlayer(Name, true, true, pl) == 0) {
		CharacterID = pl->ID;
		strcpy(RealName, pl->Name);
		Online = true;
	} else {
		CharacterID = GetCharacterID(Name);
		if (CharacterID == 0) {
			SendResult(GetConnection(), PLAYERNOTEXISTING);
			return;
		}

		strcpy(RealName, GetCharacterName(Name));
		Online = false;
	}

	for (int i = 0; i < PlayerData->Buddies; i++) {
		if (PlayerData->Buddy[i] == CharacterID) {
			SendMessage(GetConnection(), TALK_FAILURE_MESSAGE, "This player is already in your list.");
			return;
		}
	}

	SendBuddyData(GetConnection(), CharacterID, RealName,
			Online && (!CheckRight(CharacterID, NO_STATISTICS) || CheckRight(ID, READ_GAMEMASTER_CHANNEL)));
	AddBuddyOrder(this, CharacterID);
	PlayerData->Buddy[PlayerData->Buddies] = CharacterID;
	strcpy(PlayerData->BuddyName[PlayerData->Buddies], RealName);
	PlayerData->Buddies++;
}

void TPlayer::RemoveBuddy(uint32_t CharacterID) {
	if (PlayerData == nullptr) {
		error("TPlayer::RemoveBuddy: PlayerData ist NULL.\n");
		return;
	}

	int Position = -1;
	for (int i = 0; i < PlayerData->Buddies; i++) {
		if (PlayerData->Buddy[i] == CharacterID) {
			Position = i;
			break;
		}
	}

	if (Position == -1) {
		return;
	}

	PlayerData->Buddy[Position] = PlayerData->Buddy[PlayerData->Buddies - 1];
	strcpy(PlayerData->BuddyName[Position], PlayerData->BuddyName[PlayerData->Buddies - 1]);
	PlayerData->Buddies--;
	RemoveBuddyOrder(this, CharacterID);
}

void TPlayer::SendBuddies() {
	if (PlayerData == nullptr) {
		error("TPlayer::SendBuddies: PlayerData ist NULL.\n");
		return;
	}

	bool ViewAll = CheckRight(ID, READ_GAMEMASTER_CHANNEL);
	for (int i = 0; i < PlayerData->Buddies; i++) {
		TPlayer *pl = GetPlayer(PlayerData->Buddy[i]);
		if (pl != nullptr) {
			SendBuddyData(GetConnection(), PlayerData->Buddy[i], pl->Name,
					ViewAll || !CheckRight(pl->ID, NO_STATISTICS));
		} else {
			SendBuddyData(GetConnection(), PlayerData->Buddy[i], PlayerData->BuddyName[i], false);
		}
	}
}

void TPlayer::Regenerate() {
	Object Bed = GetFirstObject(posx, posy, posz);
	while (Bed != NONE) {
		if (Bed.getFlag(BED)) {
			break;
		}
		Bed++;
	}

	if (Bed == NONE) {
		error("TPlayer::Regenerate: Bett nicht gefunden.\n");
		return;
	}

	if (!Bed.getFlag(TEXT)) {
		error("TPlayer::Regenerate: Bett tr\xe4gt keinen Text.\n");
		return;
	}

	if (Bed.getAttribute(TEXTSTRING) == 0) {
		return;
	}

	if (stricmp(GetDynamicString(Bed.getAttribute(TEXTSTRING)), Name, -1) != 0) {
		return;
	}

	time_t OfflineTime = 0;
	if (PlayerData != nullptr && PlayerData->LastLogoutTime != 0) {
		OfflineTime = static_cast<int32_t>(time(nullptr) - PlayerData->LastLogoutTime);
	}

	int Regeneration = (int)(OfflineTime / 15);
	int Fed = Skills[14]->TimerValue() / 3;
	if (Fed < Regeneration) {
		Regeneration = Fed;
	}

	if (Regeneration > 0) {
		Skills[2]->Change(Regeneration / 4);
		Skills[3]->Change(Regeneration);
		SetTimer(14, Skills[14]->TimerValue() - Regeneration * 3, 0, 0, -1);
	}

	if (OfflineTime >= 900) {
		Skills[22]->Change((int)(OfflineTime / 900));
	}

	try {
		UseObjects(0, Bed, Bed);
	} catch (RESULT r) {
		error("TPlayer::Regenerate: Exception %d beim Aufr\xe4umen des Bettes.\n", r);
	}
}

bool TPlayer::IsAttacker(uint32_t Victim, bool CheckFormer) {
	for (int i = 0; i < NumberOfAttackedPlayers; i++) {
		if (AttackedPlayers(i) == Victim) {
			return true;
		}
	}

	if (CheckFormer && RoundNr <= FormerLogoutRound + PLAYERKILLINGMARK_DELAY) {
		for (int i = 0; i < NumberOfFormerAttackedPlayers; i++) {
			if (FormerAttackedPlayers(i) == Victim) {
				return true;
			}
		}
	}

	return false;
}

bool TPlayer::IsAggressor(bool CheckFormer) {
	return Aggressor
			|| (CheckFormer && RoundNr <= FormerLogoutRound + PLAYERKILLINGMARK_DELAY && FormerAggressor);
}

bool TPlayer::IsAttackJustified(uint32_t Victim) {
	TPlayer *pl = GetPlayer(Victim);
	if (pl == nullptr) {
		error("TPlayer::IsAttackJustified: Opfer existiert nicht.\n");
		return true;
	}

	if (WorldType == PVP_ENFORCED) {
		return true;
	}

	if (pl->IsPlayerkiller()) {
		return true;
	}

	if (pl->IsAggressor(true)) {
		return true;
	}

	if (GetPartyLeader(true) != 0 && GetPartyLeader(true) == pl->GetPartyLeader(true)) {
		return true;
	}

	return pl->IsAttacker(ID, true);
}

void TPlayer::RecordAttack(uint32_t Victim) {
	if (WorldType != NORMAL) {
		return;
	}

	if (Victim == ID) {
		return;
	}

	TPlayer *pl = GetPlayer(Victim);
	if (pl == nullptr) {
		error("TPlayer::RecordAttack: Opfer existiert nicht.\n");
		return;
	}

	if (!pl->IsAttacker(ID, true)
			&& !IsAttacker(Victim, false)
			&& (GetPartyLeader(true) == 0 || GetPartyLeader(true) != pl->GetPartyLeader(true))) {
		AttackedPlayers(NumberOfAttackedPlayers) = Victim;
		NumberOfAttackedPlayers++;
		print(3, "Spieler %s ist Angreifer f\xfcr Spieler %s.\n", Name, pl->Name);
		if (pl->GetConnection() != nullptr
				&& pl->GetConnection()->KnownCreature(ID, false) == KNOWNCREATURE_UPTODATE) {
			SendCreatureSkull(pl->GetConnection(), ID);
		}
	}

	if (!IsAttackJustified(Victim) && !IsAggressor(false)) {
		Aggressor = true;
		print(3, "Spieler %s ist Aggressor.\n", Name);
		AnnounceChangedCreature(ID, 5);
	}
}

void TPlayer::RecordMurder(uint32_t Victim) {
	if (WorldType != NORMAL) {
		return;
	}

	if (Victim == ID) {
		return;
	}

	if (IsAttackJustified(Victim)) {
		return;
	}

	TPlayer *pl = GetPlayer(Victim);
	if (pl == nullptr) {
		error("TPlayer::RecordMurder: Opfer existiert nicht.\n");
		return;
	}

	print(3, "Ungerechtfertigter Mord von %s an %s.\n", Name, pl->Name);
	for (int i = 1; i < 20; i++) {
		PlayerData->MurderTimestamps[i - 1] = PlayerData->MurderTimestamps[i];
	}
	PlayerData->MurderTimestamps[19] = static_cast<int32_t>(time(nullptr));
	SendMessage(GetConnection(), TALK_ADMIN_MESSAGE, "Warning! The murder of %s was not justified.", pl->Name);

	int Playerkilling = CheckPlayerkilling();
	if (Playerkilling != 0) {
		bool Announce = !IsPlayerkiller();
		PlayerData->PlayerkillerEnd = static_cast<int32_t>(time(nullptr)) + 2592000;
		if (Announce) {
			print(3, "Spieler %s ist Playerkiller.\n", Name);
			AnnounceChangedCreature(ID, 5);
		}

		if (Playerkilling == 2) {
			PunishmentOrder(nullptr, Name, IPAddress, 28, 2,
					"Exceeding the limit of unjustified kills by 100%.", 0, nullptr, 0, false);
		}
	}
}

int TPlayer::CheckPlayerkilling() {
	int LastDay = 0;
	int LastWeek = 0;
	int LastMonth = 0;
	int Now = static_cast<int32_t>(time(nullptr));
	for (int i = 0; i < 20; i++) {
		if (PlayerData->MurderTimestamps[i] == 0) {
			continue;
		}

		if (PlayerData->MurderTimestamps[i] > Now - 86400) {
			LastDay++;
		}

		if (PlayerData->MurderTimestamps[i] > Now - 604800) {
			LastWeek++;
		}

		if (PlayerData->MurderTimestamps[i] > Now - 2592000) {
			LastMonth++;
		}
	}

	if (LastDay >= 6 || LastWeek >= 10 || LastMonth >= 20) {
		return 2;
	}

	if (LastDay >= 3 || LastWeek >= 5 || LastMonth >= 10) {
		return 1;
	}

	return 0;
}

void TPlayer::ClearAttacker(uint32_t Victim) {
	for (int i = 0; i < NumberOfAttackedPlayers; i++) {
		if (AttackedPlayers(i) == Victim) {
			AttackedPlayers(i) = AttackedPlayers(NumberOfAttackedPlayers - 1);
			NumberOfAttackedPlayers--;
			TPlayer *pl = GetPlayer(Victim);
			if (pl != nullptr) {
				print(3, "Spieler %s ist kein Angreifer mehr f\xfcr Spieler %s.\n", Name, pl->Name);
				if (pl->GetConnection() != nullptr
						&& pl->GetConnection()->KnownCreature(ID, false) == KNOWNCREATURE_UPTODATE) {
					SendCreatureSkull(pl->GetConnection(), ID);
				}
			}
			return;
		}
	}
}

void TPlayer::ClearPlayerkillingMarks() {
	print(3, "L\xf6sche Markierungen von Spieler %s.\n", Name);
	FormerLogoutRound = RoundNr;
	NumberOfFormerAttackedPlayers = NumberOfAttackedPlayers;
	FormerAggressor = Aggressor;
	for (int i = 0; i < NumberOfAttackedPlayers; i++) {
		FormerAttackedPlayers(i) = AttackedPlayers(i);
	}

	if (IsAggressor(false)) {
		Aggressor = false;
		NumberOfAttackedPlayers = 0;
		print(3, "Spieler %s ist kein Aggressor mehr.\n", Name);
		AnnounceChangedCreature(ID, 5);
	}

	if (NumberOfAttackedPlayers > 0) {
		int Count = NumberOfAttackedPlayers;
		NumberOfAttackedPlayers = 0;
		for (int i = 0; i < Count; i++) {
			TPlayer *pl = GetPlayer(AttackedPlayers(i));
			if (pl != nullptr) {
				print(3, "Spieler %s ist kein Angreifer mehr f\xfcr Spieler %s.\n", Name, pl->Name);
				if (pl->GetConnection() != nullptr
						&& pl->GetConnection()->KnownCreature(ID, false) == KNOWNCREATURE_UPTODATE) {
					SendCreatureSkull(pl->GetConnection(), ID);
				}
			}
		}
	}

	for (int i = 0; i < FirstFreePlayer; i++) {
		TPlayer *pl = PlayerList(i);
		if (pl == nullptr) {
			error("TPlayer::ClearPlayerkillingMarks: Spieler %d existiert nicht.\n", i);
		} else {
			pl->ClearAttacker(ID);
		}
	}
}

int TPlayer::GetPlayerkillingMark(TPlayer *Observer) {
	if (WorldType != NORMAL) {
		return 0;
	}

	if (Observer == nullptr) {
		error("TPlayer::GetPlayerkillingMark: Beobachter existiert nicht.\n");
		return 0;
	}

	if (IsPlayerkiller()) {
		return 4;
	}

	if (IsAggressor(false)) {
		return 3;
	}

	if (GetPartyLeader(false) != 0 && GetPartyLeader(false) == Observer->GetPartyLeader(false)) {
		return 2;
	}

	if (IsAttacker(Observer->ID, false)) {
		return 1;
	}

	return 0;
}

uint32_t TPlayer::GetPartyLeader(bool CheckFormer) {
	if (PartyLeavingRound != 0) {
		if (!CheckFormer) {
			return 0;
		}

		if (PartyLeavingRound + PLAYERKILLINGMARK_DELAY < RoundNr) {
			return 0;
		}
	}

	return PartyLeader;
}

void TPlayer::JoinParty(uint32_t Leader) {
	PartyLeader = Leader;
	PartyLeavingRound = 0;
}

void TPlayer::LeaveParty() {
	PartyLeavingRound = RoundNr;
}

int TPlayer::GetPartyMark(TPlayer *Observer) {
	if (Observer == nullptr) {
		error("TPlayer::GetPartyMark: Beobachter existiert nicht.\n");
		return 0;
	}

	if (ID == Observer->GetPartyLeader(false)) {
		return 4;
	}

	if (GetPartyLeader(false) != 0 && GetPartyLeader(false) == Observer->GetPartyLeader(false)) {
		return 3;
	}

	if (Observer->ID == Observer->GetPartyLeader(false) && IsInvitedToParty(ID, Observer->ID)) {
		return 2;
	}

	if (ID == GetPartyLeader(false) && IsInvitedToParty(Observer->ID, ID)) {
		return 1;
	}

	return 0;
}

int TPlayer::RecordTalk() {
	if (ServerMilliseconds >= TalkBufferFullTime) {
		TalkBufferFullTime = ServerMilliseconds + TALKBUFFER_MILLISECONDS_PER_LINE;
	} else if (ServerMilliseconds >= TalkBufferFullTime - (TALKBUFFER_LINES - 1) * TALKBUFFER_MILLISECONDS_PER_LINE) {
		TalkBufferFullTime += TALKBUFFER_MILLISECONDS_PER_LINE;
	} else {
		NumberOfMutings++;
		int Duration = NumberOfMutings * NumberOfMutings * 5;
		MutingEndRound = RoundNr + Duration;
		return Duration;
	}

	return 0;
}

int TPlayer::RecordMessage(uint32_t Addressee) {
	int FreeSlot = -1;
	for (int i = 0; i < 20; i++) {
		if (Addressees[i] == Addressee) {
			AddresseesTimes[i] = RoundNr;
			return 0;
		}

		if (Addressees[i] == 0 || AddresseesTimes[i] < RoundNr - 600) {
			FreeSlot = i;
		}
	}

	if (FreeSlot < 0) {
		NumberOfMutings++;
		int Duration = NumberOfMutings * NumberOfMutings * 5;
		MutingEndRound = RoundNr + Duration;
		return Duration;
	}

	Addressees[FreeSlot] = Addressee;
	AddresseesTimes[FreeSlot] = RoundNr;
	return 0;
}

int TPlayer::CheckForMuting() {
	if (RoundNr < MutingEndRound) {
		return (int)(MutingEndRound - RoundNr);
	}

	return 0;
}

void InitPlayer() {
	FirstFreePlayer = 0;
	InitPlayerPool();
	CreatePlayerList(true);
	InitPlayerIndex();
}

void ExitPlayer() {
	ExitPlayerPool();
	CreatePlayerList(false);
}
