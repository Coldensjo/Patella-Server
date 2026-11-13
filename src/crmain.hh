#ifndef PATELLA_CRMAIN_HH
#define PATELLA_CRMAIN_HH

#include <cstdint>

#include "containr.hh"

struct Object;
struct TAttackWave;
struct TCreature;
struct TOutfit;

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

extern TCreature *HashList[1000];
extern vector<TCreature *> CreatureList;
extern int FirstFreeCreature;
extern uint32_t NextCreatureID;
extern priority_queue<uint32_t, TAttackWave *> AttackWaveQueue;
extern matrix<uint32_t> *FirstChainCreature;
extern int KilledCreatures[512];
extern int KilledPlayers[512];

#endif
