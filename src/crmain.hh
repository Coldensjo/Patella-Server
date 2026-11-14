#ifndef PATELLA_CRMAIN_HH
#define PATELLA_CRMAIN_HH

#include <cstdint>

#include "containr.hh"

struct Object;
struct TAttackWave;
struct TCreature;
struct TOutfit;

// The size of the race tables (RaceData, KilledCreatures, KilledPlayers):
// races are 1 to 511, 0 is none.
const int MAX_RACES = 512;

// The size of HashList: a creature is in the list of its ID modulo this.
const int HASH_LIST_SIZE = 1000;

// The race RaiseDead and MassRaiseDead create (skeleton.mon).
const int RACE_SKELETON = 33;

// Players have their character ID as creature ID, below this number; monsters
// and NPCs get IDs from here on (NextCreatureID, IsCreaturePlayer).
const int FIRST_NONPLAYER_CREATURE_ID = 0x40000000;

TCreature *GetCreature(uint32_t id);
TCreature *GetCreature(Object Obj);
bool IsCreaturePlayer(uint32_t CreatureID);
void ProcessSkills();
void ProcessCreatures();
void MoveCreatures(int Delay);
void DeleteChainCreature(TCreature *cr);
void InsertChainCreature(TCreature *cr, int newx, int newy);
void MoveChainCreature(TCreature *cr, int newx, int newy);
void InitKillStatistics();
void WriteKillStatistics();
void AddKillStatistics(int AttackerRace, int DefenderRace);
void ExitKillStatistics();
char *GetRaceName(int Race);
TOutfit GetRaceOutfit(int Race);
bool GetRaceNoSummon(int Race);
bool GetRaceNoConvince(int Race);
bool GetRaceNoIllusion(int Race);
bool GetRaceNoParalyze(int Race);
int GetRaceSummonCost(int Race);
int GetRacePoison(int Race);
bool GetRaceUnpushable(int Race);
void LoadRace(char *FileName);
void LoadRaces();
void LoadMonsterRaid(char *FileName, int Start, bool *Type, int *Date, int *Interval, int *Duration);
void LoadMonsterRaids();
void ProcessMonsterRaids();
void InitCr();
void ExitCr();

extern TCreature *HashList[HASH_LIST_SIZE];
extern vector<TCreature *> CreatureList;
extern int FirstFreeCreature;
extern uint32_t NextCreatureID;
extern priority_queue<uint32_t, TAttackWave *> AttackWaveQueue;
extern matrix<uint32_t> *FirstChainCreature;
extern int KilledCreatures[MAX_RACES];
extern int KilledPlayers[MAX_RACES];

#endif
