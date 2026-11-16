// crmain.hh: the functions and globals of crmain.cc: the creatures by ID
// (HashList) and in a list each round goes through (CreatureList), the chains
// of creatures per map block, the kill statistics, the races' values, the
// loading of races (monsters.toml) and monster raids (raids.toml), and InitCr
// and ExitCr.

#ifndef PATELLA_CRMAIN_HH
#define PATELLA_CRMAIN_HH

#include <cstdint>
#include <vector>

#include "containr.hh"

struct Object;
struct TAttackWave;
struct TCreature;
struct TMonsterData;
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
// The race number of a race key ("demon"), 0 for an unknown key or nullptr.
int GetRaceByKey(const char *Key);
// The races as LoadRaces read them from monsters.toml, in the file's order
// (LoadRaids and LoadMonsterHomes check race keys against them).
const std::vector<TMonsterData> &GetMonsterRaces();
// Fills RaceData from <DATAPATH>/monsters.toml, loads the races' scripts and
// prepares their spells. Throws a const char * (a start error).
void LoadRaces();
// Queues the waves of the raid Key (a key of raids.toml) to start in round
// Start and calls its onRaid. Gives false, and does nothing, for an unknown
// key.
bool QueueMonsterRaid(const char *Key, int Start);
// Reads <DATAPATH>/raids.toml, loads the raids' scripts and queues the raids
// that come before the next reboot. Throws a const char * (a start error).
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
