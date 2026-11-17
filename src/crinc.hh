// crinc.hh: the races and the monster raids, shared by the creature sources.
// TRaceData holds one race of monsters.toml (key, outfit, corpse, blood,
// fighting values, flags, skills, talk, loot, the number of its spells and its
// script), RaceData all of them; TAttackWave is one queued wave of a raid of
// raids.toml. ToDoQueue orders the creatures by the time of their next to-do
// entry.

#ifndef PATELLA_CRINC_HH
#define PATELLA_CRINC_HH

#include <cstdint>
#include <cstdlib>
#include <string>

#include "objects.hh"
#include "dbfuncs.hh"
#include "containr.hh"
#include "crmain.hh"
#include "monsterdata.hh"

enum BloodType {
	BT_BLOOD = 0,
	BT_SLIME = 1,
	BT_BONES = 2,
	BT_FIRE = 3,
	BT_ENERGY = 4,
};

struct TSkillData {
	int Nr;
	int Actual;
	int Minimum;
	int Maximum;
	int NextLevel;
	int FactorPercent;
	int AddLevel;
};

struct TItemData {
	ObjectType Type;
	int Maximum;
	int Probability;
	TItemData() {}
};

// Spells is the number of the race's spells, which the spell library keeps
// (scripting.hh, "Monsters"); Script the number of its script (LoadScript),
// -1 for none.
struct TRaceData {
	TRaceData();
	char Key[MAX_MONSTER_KEY_LENGTH + 1];
	char Name[30];
	char Article[3];
	TOutfit Outfit;
	ObjectType MaleCorpse;
	ObjectType FemaleCorpse;
	BloodType Blood;
	int ExperiencePoints;
	int FleeThreshold;
	int Attack;
	int Defend;
	int Armor;
	int Poison;
	int SummonCost;
	int LoseTarget;
	int Strategy[4];
	bool KickBoxes;
	bool KickCreatures;
	bool SeeInvisible;
	bool Unpushable;
	bool DistanceFighting;
	bool NoSummon;
	bool NoIllusion;
	bool NoConvince;
	bool NoBurning;
	bool NoPoison;
	bool NoEnergy;
	bool NoHit;
	bool NoLifeDrain;
	bool NoParalyze;
	int Skills;
	vector<TSkillData> Skill;
	int Talks;
	vector<uint32_t> Talk;
	int Items;
	vector<TItemData> Item;
	int Spells;
	int Script;
};

extern TRaceData RaceData[512];
extern priority_queue<uint32_t, uint32_t> ToDoQueue;

// Raid is the key of the wave's raid (raids.toml), by which its hooks find
// the raid when the wave comes (a raid gone by a reload has none), Wave the
// wave's number in the raid (1 for its first).
struct TAttackWave {
	TAttackWave();
	~TAttackWave();
	int x;
	int y;
	int z;
	int Spread;
	int Race;
	int MinCount;
	int MaxCount;
	int Radius;
	int Lifetime;
	uint32_t Message;
	int ExtraItems;
	vector<TItemData> ExtraItem;
	std::string Raid;
	int Wave;
};

#endif
