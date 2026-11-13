#ifndef PATELLA_CRPLAYER_HH
#define PATELLA_CRPLAYER_HH

#include <cstdint>

#include "containr.hh"

struct Object;
struct Semaphore;
struct TConnection;
struct TPlayer;
struct TPlayerData;
struct TPlayerIndexEntry;
struct TPlayerIndexInternalNode;
struct TPlayerIndexLeafNode;

int GetNumberOfPlayers();
TPlayer *GetPlayer(char *Name);
TPlayer *GetPlayer(uint32_t CharacterID);
bool IsPlayerOnline(char *Name);
int IdentifyPlayer(char *Name, bool ExactMatch, bool IgnoreGamemasters, TPlayer *&pl);
void LogoutAllPlayers();
void CloseProcessedRequests(uint32_t CharacterID);
void NotifyBuddies(uint32_t CharacterID, char *Name, bool Login);
void CreatePlayerList(bool Online);
void PrintPlayerPositions();
void LoadDepot(TPlayerData *PlayerData, int DepotNr, Object Con);
void SaveDepot(TPlayerData *PlayerData, int DepotNr, Object Con);
void GetProfessionName(char *Buffer, int Profession, bool Article, bool Capitals);
void SendExistingRequests(TConnection *Connection);
void InitPlayerPool();
void SavePlayerPoolSlot(TPlayerData *Slot);
void FreePlayerPoolSlot(TPlayerData *Slot);
TPlayerData *GetPlayerPoolSlot(uint32_t CharacterID);
TPlayerData *AssignPlayerPoolSlot(uint32_t CharacterID, bool DontWait);
TPlayerData *AttachPlayerPoolSlot(uint32_t CharacterID, bool DontWait);
void AttachPlayerPoolSlot(TPlayerData *Slot, bool DontWait);
void IncreasePlayerPoolSlotSticky(TPlayerData *Slot);
void DecreasePlayerPoolSlotSticky(TPlayerData *Slot);
void DecreasePlayerPoolSlotSticky(uint32_t CharacterID);
void ReleasePlayerPoolSlot(TPlayerData *Slot);
void SavePlayerPoolSlots();
void ExitPlayerPool();
int GetPlayerIndexEntryNumber(char *Name, int Position);
void InsertPlayerIndex(TPlayerIndexInternalNode *Node, int Position, char *Name, uint32_t CharacterID);
TPlayerIndexEntry *SearchPlayerIndex(char *Name);
void InitPlayerIndex();
bool PlayerExists(char *Name);
uint32_t GetCharacterID(char *Name);
char *GetCharacterName(char *Name);
void InitPlayer();
void ExitPlayer();

extern vector<TPlayer *> PlayerList;
extern int FirstFreePlayer;
extern Semaphore PlayerMutex;
extern TPlayerData PlayerDataPool[2000];
extern Semaphore PlayerDataPoolMutex;
extern store<TPlayerIndexInternalNode, 100> PlayerIndexInternalNodes;
extern store<TPlayerIndexLeafNode, 100> PlayerIndexLeafNodes;
extern TPlayerIndexInternalNode PlayerIndexHead;

#endif
