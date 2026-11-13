#ifndef PATELLA_CRINC_HH
#define PATELLA_CRINC_HH

#include <cstdint>
#include <cstdlib>

#include "objects.hh"
#include "dbfuncs.hh"
#include "containr.hh"

enum BloodType {
	BT_BLOOD = 0,
	BT_SLIME = 1,
	BT_BONES = 2,
	BT_FIRE = 3,
	BT_ENERGY = 4,
};

enum SpellShapeType {
	SHAPE_ACTOR = 0,
	SHAPE_VICTIM = 1,
	SHAPE_ORIGIN = 2,
	SHAPE_DESTINATION = 3,
	SHAPE_ANGLE = 4,
};

enum SpellImpactType {
	IMPACT_DAMAGE = 0,
	IMPACT_FIELD = 1,
	IMPACT_HEALING = 2,
	IMPACT_SPEED = 3,
	IMPACT_DRUNKEN = 4,
	IMPACT_STRENGTH = 5,
	IMPACT_OUTFIT = 6,
	IMPACT_SUMMON = 7,
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
	// inline  crinc.h:51
	TItemData() { abort(); /* STUB */ }
};

struct TSpellData {
	SpellShapeType Shape;
	int ShapeParam1;
	int ShapeParam2;
	int ShapeParam3;
	int ShapeParam4;
	SpellImpactType Impact;
	int ImpactParam1;
	int ImpactParam2;
	int ImpactParam3;
	int ImpactParam4;
	int Delay;
};

struct TRaceData {
	TRaceData();
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
	vector<TSpellData> Spell;
};

extern TRaceData RaceData[512];
extern priority_queue<uint32_t, uint32_t> ToDoQueue;

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
};

#endif
