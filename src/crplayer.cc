#include <cstdint>
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
#include "TXTEASymmetricKey.hh"
#include "crplayer.hh"

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

vector<TPlayer *> PlayerList(0, 100, 10, nullptr);	// crplayer.c:59
int FirstFreePlayer;	// crplayer.c:60
Semaphore PlayerMutex(1);	// crplayer.c:61
TPlayerData PlayerDataPool[2000];	// crplayer.c:63
Semaphore PlayerDataPoolMutex(1);	// crplayer.c:64
store<TPlayerIndexInternalNode, 100> PlayerIndexInternalNodes;	// crplayer.c:66
store<TPlayerIndexLeafNode, 100> PlayerIndexLeafNodes;	// crplayer.c:67
TPlayerIndexInternalNode PlayerIndexHead;	// crplayer.c:68

// 0x080a7170  crplayer.c:79
int GetNumberOfPlayers() { abort(); /* STUB */ }

// 0x080a7180  crplayer.c:87
TPlayer *GetPlayer(char * /*Name*/) { abort(); /* STUB */ }

// 0x080a71e0  crplayer.c:99
TPlayer *GetPlayer(uint32_t /*CharacterID*/) { abort(); /* STUB */ }

// 0x080a7220  crplayer.c:113
bool IsPlayerOnline(char * /*Name*/) { abort(); /* STUB */ }

// 0x080a72a0  crplayer.c:130
int IdentifyPlayer(char * /*Name*/, bool /*ExactMatch*/, bool /*IgnoreGamemasters*/, TPlayer *& /*pl*/) { abort(); /* STUB */ }

// 0x080a7420  crplayer.c:167
void LogoutAllPlayers() { abort(); /* STUB */ }

// 0x080a7490  crplayer.c:188
void CloseProcessedRequests(uint32_t /*CharacterID*/) { abort(); /* STUB */ }

// 0x080a7510  crplayer.c:208
void NotifyBuddies(uint32_t /*CharacterID*/, char * /*Name*/, bool /*Login*/) { abort(); /* STUB */ }

// 0x080a76a0  crplayer.c:234
void CreatePlayerList(bool /*Online*/) { abort(); /* STUB */ }

// 0x080a7830  crplayer.c:258
void PrintPlayerPositions() { abort(); /* STUB */ }

// 0x080a7900  crplayer.c:269
void LoadDepot(TPlayerData * /*PlayerData*/, int /*DepotNr*/, Object /*Con*/) { abort(); /* STUB */ }

// 0x080a7ad0  crplayer.c:304
void SaveDepot(TPlayerData * /*PlayerData*/, int /*DepotNr*/, Object /*Con*/) { abort(); /* STUB */ }

// 0x080a7cf0  crplayer.c:348
void GetProfessionName(char * /*Buffer*/, int /*Profession*/, bool /*Article*/, bool /*Capitals*/) { abort(); /* STUB */ }

// 0x080a7df0  crplayer.c:375
void SendExistingRequests(TConnection * /*Connection*/) { abort(); /* STUB */ }

// 0x080a7f60  crplayer.c:420
void InitPlayerPool() { abort(); /* STUB */ }

// 0x080a7f90  crplayer.c:430
void SavePlayerPoolSlot(TPlayerData * /*Slot*/) { abort(); /* STUB */ }

// 0x080a7fd0  crplayer.c:445
void FreePlayerPoolSlot(TPlayerData * /*Slot*/) { abort(); /* STUB */ }

// 0x080a80b0  crplayer.c:478
TPlayerData *GetPlayerPoolSlot(uint32_t /*CharacterID*/) { abort(); /* STUB */ }

// 0x080a8100  crplayer.c:497
TPlayerData *AssignPlayerPoolSlot(uint32_t /*CharacterID*/, bool /*DontWait*/) { abort(); /* STUB */ }

// 0x080a8410  crplayer.c:573
TPlayerData *AttachPlayerPoolSlot(uint32_t /*CharacterID*/, bool /*DontWait*/) { abort(); /* STUB */ }

// 0x080a84e0  crplayer.c:603
void AttachPlayerPoolSlot(TPlayerData * /*Slot*/, bool /*DontWait*/) { abort(); /* STUB */ }

// 0x080a8580  crplayer.c:627
void IncreasePlayerPoolSlotSticky(TPlayerData * /*Slot*/) { abort(); /* STUB */ }

// 0x080a85d0  crplayer.c:641
void DecreasePlayerPoolSlotSticky(TPlayerData * /*Slot*/) { abort(); /* STUB */ }

// 0x080a8620  crplayer.c:655
void DecreasePlayerPoolSlotSticky(uint32_t /*CharacterID*/) { abort(); /* STUB */ }

// 0x080a8690  crplayer.c:671
void ReleasePlayerPoolSlot(TPlayerData * /*Slot*/) { abort(); /* STUB */ }

// 0x080a8710  crplayer.c:693
void SavePlayerPoolSlots() { abort(); /* STUB */ }

// 0x080a8860  crplayer.c:713
void ExitPlayerPool() { abort(); /* STUB */ }

// 0x080a8a90  crplayer.c:745
int GetPlayerIndexEntryNumber(char * /*Name*/, int /*Position*/) { abort(); /* STUB */ }

// 0x080a8b00  crplayer.c:762
void InsertPlayerIndex(TPlayerIndexInternalNode * /*Node*/, int /*Position*/, char * /*Name*/, uint32_t /*CharacterID*/) { abort(); /* STUB */ }

// 0x080a8ce0  crplayer.c:811
TPlayerIndexEntry *SearchPlayerIndex(char * /*Name*/) { abort(); /* STUB */ }

// 0x080a8d80  crplayer.c:835
void InitPlayerIndex() { abort(); /* STUB */ }

// 0x080a8ef0  crplayer.c:869
bool PlayerExists(char * /*Name*/) { abort(); /* STUB */ }

// 0x080a8f30  crplayer.c:883
uint32_t GetCharacterID(char * /*Name*/) { abort(); /* STUB */ }

// 0x080a8f70  crplayer.c:898
char *GetCharacterName(char * /*Name*/) { abort(); /* STUB */ }

// 0x080a9a60  crplayer.c:917
TPlayer::TPlayer(TConnection * /*Connection*/, uint32_t /*CharacterID*/) : AttackedPlayers(0, 10, 10), FormerAttackedPlayers(0, 10, 10) { abort(); /* STUB */ }

// 0x080aa9e0  crplayer.c:1073
TPlayer::~TPlayer() { abort(); /* STUB */ }

// 0x080ab380  crplayer.c:1136
void TPlayer::SetInList() { abort(); /* STUB */ }

// 0x080ab410  crplayer.c:1151
void TPlayer::DelInList() { abort(); /* STUB */ }

// 0x080ab500  crplayer.c:1169
void TPlayer::ClearConnection() { abort(); /* STUB */ }

// 0x080ab590  crplayer.c:1185
void TPlayer::LoadData() { abort(); /* STUB */ }

// 0x080ab840  crplayer.c:1231
void TPlayer::SaveData() { abort(); /* STUB */ }

// 0x080abac0  crplayer.c:1268
void TPlayer::LoadInventory(bool /*SetStandardInventory*/) { abort(); /* STUB */ }

// 0x080abdf0  crplayer.c:1308
void TPlayer::SaveInventory() { abort(); /* STUB */ }

// 0x080abfd0  crplayer.c:1345
void TPlayer::StartCoordinates() { abort(); /* STUB */ }

// 0x080ac010  crplayer.c:1353
void TPlayer::TakeOver(TConnection * /*Connection*/) { abort(); /* STUB */ }

// 0x080ac450  crplayer.c:1419
void TPlayer::Death() { abort(); /* STUB */ }

// 0x080ac7e0  crplayer.c:1462
void TPlayer::AttackStimulus(uint32_t) { abort(); /* STUB */ }

// 0x080ac810  crplayer.c:1469
void TPlayer::DamageStimulus(uint32_t, int, int) { abort(); /* STUB */ }

// 0x080ac840  crplayer.c:1477
void TPlayer::IdleStimulus() { abort(); /* STUB */ }

// 0x080ac8e0  crplayer.c:1496
void TPlayer::SetOpenContainer(int /*Window*/, Object /*Con*/) { abort(); /* STUB */ }

// 0x080ac990  crplayer.c:1512
Object TPlayer::GetOpenContainer(int /*Window*/) { abort(); /* STUB */ }

// 0x080ac9e0  crplayer.c:1524
void TPlayer::CloseAllContainers() { abort(); /* STUB */ }

// 0x080aca70  crplayer.c:1535
Object TPlayer::InspectTrade(bool /*OwnOffer*/, int /*Position*/) { abort(); /* STUB */ }

// 0x080acbd0  crplayer.c:1564
void TPlayer::AcceptTrade() { abort(); /* STUB */ }

// 0x080ad5e0  crplayer.c:1665
void TPlayer::RejectTrade() { abort(); /* STUB */ }

// 0x080ad6c0  crplayer.c:1683
void TPlayer::ClearProfession() { abort(); /* STUB */ }

// 0x080ad700  crplayer.c:1697
void TPlayer::SetProfession(unsigned char /*prof*/) { abort(); /* STUB */ }

// 0x080adb80  crplayer.c:1781
unsigned char TPlayer::GetRealProfession() { abort(); /* STUB */ }

// 0x080adb90  crplayer.c:1789
unsigned char TPlayer::GetEffectiveProfession() { abort(); /* STUB */ }

// 0x080adbb0  crplayer.c:1797
unsigned char TPlayer::GetActiveProfession() { abort(); /* STUB */ }

// 0x080adc00  crplayer.c:1805
bool TPlayer::GetActivePromotion() { abort(); /* STUB */ }

// 0x080adc50  crplayer.c:1813
bool TPlayer::SpellKnown(int /*SpellNumber*/) { abort(); /* STUB */ }

// 0x080adc90  crplayer.c:1825
void TPlayer::LearnSpell(int /*SpellNumber*/) { abort(); /* STUB */ }

// 0x080adcd0  crplayer.c:1841
int TPlayer::GetQuestValue(int /*Number*/) { abort(); /* STUB */ }

// 0x080add40  crplayer.c:1854
void TPlayer::SetQuestValue(int /*Number*/, int /*Value*/) { abort(); /* STUB */ }

// 0x080addc0  crplayer.c:1868
void TPlayer::CheckOutfit() { abort(); /* STUB */ }

// 0x080adec0  crplayer.c:1903
void TPlayer::CheckState() { abort(); /* STUB */ }

// 0x080adfc0  crplayer.c:1925
bool TPlayer::MovePossible(int /*x*/, int /*y*/, int /*z*/, bool /*Execute*/, bool /*Jump*/) { abort(); /* STUB */ }

// 0x080ae1b0  crplayer.c:1941
void TPlayer::AddBuddy(char * /*Name*/) { abort(); /* STUB */ }

// 0x080ae480  crplayer.c:1994
void TPlayer::RemoveBuddy(uint32_t /*CharacterID*/) { abort(); /* STUB */ }

// 0x080ae550  crplayer.c:2013
void TPlayer::SendBuddies() { abort(); /* STUB */ }

// 0x080ae6c0  crplayer.c:2030
void TPlayer::Regenerate() { abort(); /* STUB */ }

// 0x080ae990  crplayer.c:2072
bool TPlayer::IsAttacker(uint32_t /*Victim*/, bool /*CheckFormer*/) { abort(); /* STUB */ }

// 0x080aea50  crplayer.c:2085
bool TPlayer::IsAggressor(bool /*CheckFormer*/) { abort(); /* STUB */ }

// 0x080aeaa0  crplayer.c:2096
bool TPlayer::IsAttackJustified(uint32_t /*Victim*/) { abort(); /* STUB */ }

// 0x080aebf0  crplayer.c:2113
void TPlayer::RecordAttack(uint32_t /*Victim*/) { abort(); /* STUB */ }

// 0x080aee30  crplayer.c:2139
void TPlayer::RecordMurder(uint32_t /*Victim*/) { abort(); /* STUB */ }

// 0x080af030  crplayer.c:2169
int TPlayer::CheckPlayerkilling() { abort(); /* STUB */ }

// 0x080af0f0  crplayer.c:2189
void TPlayer::ClearAttacker(uint32_t /*Victim*/) { abort(); /* STUB */ }

// 0x080af220  crplayer.c:2207
void TPlayer::ClearPlayerkillingMarks() { abort(); /* STUB */ }

// 0x080af470  crplayer.c:2248
int TPlayer::GetPlayerkillingMark(TPlayer * /*Observer*/) { abort(); /* STUB */ }

// 0x080af550  crplayer.c:2265
uint32_t TPlayer::GetPartyLeader(bool /*CheckFormer*/) { abort(); /* STUB */ }

// 0x080af590  crplayer.c:2276
void TPlayer::JoinParty(uint32_t /*Leader*/) { abort(); /* STUB */ }

// 0x080af5b0  crplayer.c:2285
void TPlayer::LeaveParty() { abort(); /* STUB */ }

// 0x080af5d0  crplayer.c:2293
int TPlayer::GetPartyMark(TPlayer * /*Observer*/) { abort(); /* STUB */ }

// 0x080af6e0  crplayer.c:2310
int TPlayer::RecordTalk() { abort(); /* STUB */ }

// 0x080af740  crplayer.c:2330
int TPlayer::RecordMessage(uint32_t /*Addressee*/) { abort(); /* STUB */ }

// 0x080af7d0  crplayer.c:2356
int TPlayer::CheckForMuting() { abort(); /* STUB */ }

// 0x080af800  crplayer.c:2370
void InitPlayer() { abort(); /* STUB */ }

// 0x080af860  crplayer.c:2381
void ExitPlayer() { abort(); /* STUB */ }
