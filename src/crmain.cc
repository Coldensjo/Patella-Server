#include <cstdint>
#include <cstdlib>

#include "script.hh"
#include "config.hh"
#include "threads.hh"
#include "dbfuncs.hh"
#include "objects.hh"
#include "map.hh"
#include "info.hh"
#include "cr.hh"
#include "containr.hh"
#include "crinc.hh"
#include "operate.hh"
#include "connections.hh"
#include "utils.hh"
#include "time.hh"
#include "communication.hh"
#include "TXTEASymmetricKey.hh"
#include "crmain.hh"

TCreature *HashList[1000];	// crmain.c:31
vector<TCreature *> CreatureList(0, 10000, 1000, nullptr);	// crmain.c:32
int FirstFreeCreature;	// crmain.c:33
TRaceData RaceData[512];	// crmain.c:34
uint32_t NextCreatureID;	// crmain.c:35
priority_queue<uint32_t, uint32_t> ToDoQueue(5000, 1000);	// crmain.c:36
priority_queue<uint32_t, TAttackWave *> AttackWaveQueue(100, 100);	// crmain.c:37
matrix<uint32_t> *FirstChainCreature;	// crmain.c:160
int KilledCreatures[512];	// crmain.c:323
int KilledPlayers[512];	// crmain.c:324

// 0x0809df10  crmain.c:48
TCreature *GetCreature(uint32_t /*id*/) { abort(); /* STUB */ }

// 0x0809df70  crmain.c:66
TCreature *GetCreature(Object /*Obj*/) { abort(); /* STUB */ }

// 0x0809df90  crmain.c:74
bool IsCreaturePlayer(uint32_t /*CreatureID*/) { abort(); /* STUB */ }

// 0x0809dfb0  crmain.c:82
void ProcessSkills() { abort(); /* STUB */ }

// 0x0809e020  crmain.c:97
void ProcessCreatures() { abort(); /* STUB */ }

// 0x0809e230  crmain.c:136
void MoveCreatures(int /*Delay*/) { abort(); /* STUB */ }

// 0x0809e2d0  crmain.c:166
void DeleteChainCreature(TCreature * /*cr*/) { abort(); /* STUB */ }

// 0x0809e430  crmain.c:198
void InsertChainCreature(TCreature * /*cr*/, int /*newx*/, int /*newy*/) { abort(); /* STUB */ }

// 0x0809e5a0  crmain.c:213
void MoveChainCreature(TCreature * /*cr*/, int /*newx*/, int /*newy*/) { abort(); /* STUB */ }

// 0x0809e6d0  crmain.c:229
TFindCreatures::TFindCreatures(int /*radx*/, int /*rady*/, int /*x*/, int /*y*/, int /*Mask*/) { abort(); /* STUB */ }

// 0x0809e7f0  crmain.c:238
TFindCreatures::TFindCreatures(int /*radx*/, int /*rady*/, uint32_t /*CreatureID*/, int /*Mask*/) { abort(); /* STUB */ }

// 0x0809e960  crmain.c:254
TFindCreatures::TFindCreatures(int /*radx*/, int /*rady*/, Object /*Obj*/, int /*Mask*/) { abort(); /* STUB */ }

// 0x0809ea30  crmain.c:269
void TFindCreatures::initSearch(int /*radx*/, int /*rady*/, int /*x*/, int /*y*/, int /*mask*/) { abort(); /* STUB */ }

// 0x0809eab0  crmain.c:286
uint32_t TFindCreatures::getNext() { abort(); /* STUB */ }

// 0x0809ecb0  crmain.c:330
void InitKillStatistics() { abort(); /* STUB */ }

// 0x0809ece0  crmain.c:342
void WriteKillStatistics() { abort(); /* STUB */ }

// 0x0809ee50  crmain.c:370
void AddKillStatistics(int /*AttackerRace*/, int /*DefenderRace*/) { abort(); /* STUB */ }

// 0x0809eed0  crmain.c:379
void ExitKillStatistics() { abort(); /* STUB */ }

// 0x0809f0c0  crmain.c:389
TCreature::TCreature() : ToDoList(0, 20, 10) { abort(); /* STUB */ }

// 0x0809f960  crmain.c:431
TCreature::~TCreature() { abort(); /* STUB */ }

// 0x080a06e0  crmain.c:501
void TCreature::SetID(uint32_t /*CharacterID*/) { abort(); /* STUB */ }

// 0x080a07e0  crmain.c:530
void TCreature::DelID() { abort(); /* STUB */ }

// 0x080a08a0  crmain.c:555
void TCreature::SetInCrList() { abort(); /* STUB */ }

// 0x080a08d0  crmain.c:564
void TCreature::DelInCrList() { abort(); /* STUB */ }

// 0x080a0970  crmain.c:576
void TCreature::StartLogout(bool /*Force*/, bool /*StopFight*/) { abort(); /* STUB */ }

// 0x080a0a00  crmain.c:587
int TCreature::LogoutPossible() { abort(); /* STUB */ }

// 0x080a0a90  crmain.c:601
void TCreature::BlockLogout(int /*Delay*/, bool /*BlockProtectionZone*/) { abort(); /* STUB */ }

// 0x080a0b70  crmain.c:617
int TCreature::GetHealth() { abort(); /* STUB */ }

// 0x080a0bf0  crmain.c:634
int TCreature::GetSpeed() { abort(); /* STUB */ }

// 0x080a0c20  crmain.c:647
void TCreature::Death() { abort(); /* STUB */ }

// 0x080a0c40  crmain.c:659
int TCreature::Damage(TCreature * /*atk*/, int /*Damage*/, int /*DamageType*/) { abort(); /* STUB */ }

// 0x080a2010  crmain.c:950
bool TCreature::MovePossible(int /*x*/, int /*y*/, int /*z*/, bool /*Execute*/, bool /*Jump*/) { abort(); /* STUB */ }

// 0x080a22f0  crmain.c:963
TRaceData::TRaceData() : Skill(1, 5, 5), Talk(1, 5, 5, 0), Item(1, 5, 5), Spell(1, 5, 5) { abort(); /* STUB */ }

// 0x080a24e0  crmain.c:1005
char *GetRaceName(int /*Race*/) { abort(); /* STUB */ }

// 0x080a2520  crmain.c:1017
TOutfit GetRaceOutfit(int /*Race*/) { abort(); /* STUB */ }

// 0x080a2580  crmain.c:1029
bool GetRaceNoSummon(int /*Race*/) { abort(); /* STUB */ }

// 0x080a25c0  crmain.c:1041
bool GetRaceNoConvince(int /*Race*/) { abort(); /* STUB */ }

// 0x080a2600  crmain.c:1053
bool GetRaceNoIllusion(int /*Race*/) { abort(); /* STUB */ }

// 0x080a2640  crmain.c:1065
bool GetRaceNoParalyze(int /*Race*/) { abort(); /* STUB */ }

// 0x080a2680  crmain.c:1077
int GetRaceSummonCost(int /*Race*/) { abort(); /* STUB */ }

// 0x080a26c0  crmain.c:1089
int GetRacePoison(int /*Race*/) { abort(); /* STUB */ }

// 0x080a2700  crmain.c:1101
bool GetRaceUnpushable(int /*Race*/) { abort(); /* STUB */ }

// 0x080a2740  crmain.c:1114
void LoadRace(char * /*FileName*/) { abort(); /* STUB */ }

// 0x080a3a90  crmain.c:1358
void LoadRaces() { abort(); /* STUB */ }

// 0x080a3c30  crmain.c:1382
TAttackWave::TAttackWave() : ExtraItem(1, 5, 5) { abort(); /* STUB */ }

// 0x080a3d00  crmain.c:1403
TAttackWave::~TAttackWave() { abort(); /* STUB */ }

// 0x080a3d40  crmain.c:1413
void LoadMonsterRaid(char * /*FileName*/, int /*Start*/, bool * /*Type*/, int * /*Date*/, int * /*Interval*/, int * /*Duration*/) { abort(); /* STUB */ }

// 0x080a4650  crmain.c:1516
void LoadMonsterRaids() { abort(); /* STUB */ }

// 0x080a4a40  crmain.c:1570
void ProcessMonsterRaids() { abort(); /* STUB */ }

// 0x080a50f0  crmain.c:1639
void InitCr() { abort(); /* STUB */ }

// 0x080a51f0  crmain.c:1657
void ExitCr() { abort(); /* STUB */ }
