#ifndef PATELLA_DBFUNCS_HH
#define PATELLA_DBFUNCS_HH

#include <cstdint>
#include <sys/types.h>
#include <time.h>

#include "crskill.hh"

struct TReadScriptFile;
struct TWriteScriptFile;

// The sizes of TPlayerData's tables: the spells (SpellList, also in TPlayer;
// the spells are 1 to 255, 0 is none), the quest values (QuestValues, also in
// TPlayer; 0 to 499), the depots (0 to 8) and the buddies.
const int MAX_SPELLS = 256;
const int MAX_QUESTS = 500;
const int MAX_DEPOTS = 9;
const int MAX_BUDDIES = 100;

struct TOutfit {
	int OutfitID;
	union {
		unsigned short ObjectType;
		unsigned char Colors[4];
	};
	void load(TReadScriptFile &Script);
	void save(TWriteScriptFile &Script);
};

struct TPlayerData {
	uint32_t CharacterID;
	pid_t Locked;
	int Sticky;
	bool Dirty;
	int Race;
	TOutfit OriginalOutfit;
	TOutfit CurrentOutfit;
	time_t LastLoginTime;
	time_t LastLogoutTime;
	int startx;
	int starty;
	int startz;
	int posx;
	int posy;
	int posz;
	int Profession;
	int PlayerkillerEnd;
	int Actual[25];
	int Maximum[25];
	int Minimum[25];
	int DeltaAct[25];
	int MagicDeltaAct[25];
	int Cycle[25];
	int MaxCycle[25];
	int Count[25];
	int MaxCount[25];
	int AddLevel[25];
	int Experience[25];
	int FactorPercent[25];
	int NextLevel[25];
	int Delta[25];
	unsigned char SpellList[256];
	int QuestValues[500];
	int MurderTimestamps[20];
	unsigned char *Inventory;
	int InventorySize;
	unsigned char *Depot[9];
	int DepotSize[9];
	uint32_t AccountID;
	int Sex;
	char Name[30];
	unsigned char Rights[12];
	char Guild[31];
	char Rank[31];
	char Title[31];
	int Buddies;
	uint32_t Buddy[100];
	char BuddyName[100][30];
	uint32_t EarliestYellRound;
	uint32_t EarliestTradeChannelRound;
	uint32_t EarliestSpellTime;
	uint32_t EarliestMultiuseTime;
	uint32_t TalkBufferFullTime;
	uint32_t MutingEndRound;
	uint32_t Addressees[20];
	uint32_t AddresseesTimes[20];
	int NumberOfMutings;
};

void PlayerDataPath(uint32_t CharacterID, char *Extension, char *Path);
bool PlayerDataExists(uint32_t CharacterID);
bool LoadPlayerData(TPlayerData *Slot);
void SavePlayerData(TPlayerData *Slot);
void UnlinkPlayerData(uint32_t CharacterID);

#endif
