// monsterdata.hh: the monster races, the raids and the monster homes as
// monsters.toml, raids.toml and monster_homes.toml define them
// (monsterdata.cc), and their loaders. The game fills RaceData, its raids and
// its homes from them (crmain.cc, crnonpl.cc); tools/convert reads the files
// it wrote back with the same loaders. The loaders need no game state.
//
// A race is named by its key: the name of its original file without ".mon"
// ("demon", "illusion", "slime2"), 1 to MAX_MONSTER_KEY_LENGTH characters of
// a-z, 0-9 and '_'. Keys and race numbers are unique in monsters.toml. A raid
// is named by its key the same way (the original file without ".evt").
//
// The files (the converter writes this as each file's comment header):
//
// monsters.toml, one [[monster]] per race, in the order of the race numbers:
//   key             The race's key.
//   number          Its race number, 1 to MAX_RACES - 1 (crmain.hh).
//   name            The name players see, Latin-1, 1 to 29 characters.
//   article         "a", "an" or "" (at most 2 characters).
//   outfit          { id = <outfit>, colors = [<head>, <body>, <legs>,
//                   <feet>] } (id 1 to 65535, colors 0 to 255) or { id = 0,
//                   object_type = <type> } (0 to 65535).
//   corpse          The object type of its corpse; female_corpse (optional)
//                   the female one, the same as corpse when missing.
//   blood           "blood", "slime", "bones", "fire" or "energy".
//   experience, summon_cost, flee_threshold, attack, defend, armor, poison,
//   lose_target     Numbers, as the original's keys.
//   strategy        [<nearest>, <weakest>, <most damage>, <random>], the
//                   chance of each in percent.
//   flags           (optional) Names of the race's flags: "kick_boxes",
//                   "kick_creatures", "see_invisible", "unpushable",
//                   "distance_fighting", "no_summon", "no_convince",
//                   "no_illusion", "no_burning", "no_poison", "no_energy",
//                   "no_hit", "no_life_drain", "no_paralyze".
//   skills          (optional) { skill = <name>, actual, minimum, maximum,
//                   next_level, factor_percent, add_level }, the skill named
//                   as GetSkillByName (crskill.cc) takes it ("hitpoints",
//                   "gostrength", ...).
//   talk            (optional) The texts it says, Latin-1; one starting with
//                   '#' is said loud, from its fourth character on.
//   inventory       (optional) { type, maximum, chance }: an object type, the
//                   most it carries of it, and the chance in 1/1000.
//   script          (optional) "monsters/<key>.lua", its hooks
//                   (scripting.hh).
//   [[monster.spell]] (optional, any number) One spell, in order: its keys
//                   as scripting.hh's "Monsters" lists them (shape, impact,
//                   one_in and the shape's and impact's keys); the loader
//                   keeps them as they are, the spell library checks them.
//
// raids.toml, one [[raid]] per raid, in the original game's order:
//   key             The raid's key; the start_monsterraid command names it.
//   description     (optional) A text the game does not use.
//   type            "small" or "big" (SmallRaid, BigRaid).
//   interval        The mean time between two of its raids in seconds; or
//   date            the time of its one raid (seconds since 1970). Exactly
//                   one of the two.
//   script          (optional) "raids/<key>.lua", its hooks.
//   [[raid.wave]]   One wave or more, in order:
//     delay         Seconds after the raid's start.
//     position      [x, y, z], where its monsters appear.
//     spread        How far from position, in fields along x and y.
//     race          The race's key.
//     count         [<min>, <max>]: how many, 0 <= min <= max, max >= 1.
//     radius        (optional) How far the monsters walk; none: no limit.
//     lifetime      (optional) Seconds until they vanish; 0 or none: never.
//     message       (optional) A text broadcast when the wave comes.
//     location      (optional) A text the game does not use.
//     inventory     (optional) { type, maximum, chance } as for a race: the
//                   objects one of the wave's monsters gets.
//
// monster_homes.toml, the key homes, an array of one inline table per home,
// in monster.db's order:
//   { race = <key>, position = [x, y, z], radius = <fields>, amount = <the
//   most monsters>, regeneration = <seconds> }
//
// An unknown key, a missing one, a value of the wrong type or out of range
// is an error, as is an unknown race key in raids.toml and
// monster_homes.toml.

#ifndef PATELLA_MONSTERDATA_HH
#define PATELLA_MONSTERDATA_HH

#include <string>
#include <vector>

#include "dbfuncs.hh"
#include "scripting.hh"

const int MAX_MONSTER_KEY_LENGTH = 30;

// An object a race carries or a wave's monster gets (inventory).
struct TMonsterItemData {
	int Type;
	int Maximum;
	int Chance;
};

struct TMonsterSkillData {
	std::string Skill;
	int Actual, Minimum, Maximum, NextLevel, FactorPercent, AddLevel;
	std::string Where;
};

// A race. Flags as their names in the file's order; Spells as their tables
// (TScriptValue::TABLE, the keys in the file's order) with the position of
// each ("<file>:<line>:<column>"); Script empty for none; Where the position
// of the entry.
struct TMonsterData {
	std::string Key;
	int Number;
	std::string Name;
	std::string Article;
	TOutfit Outfit;
	int Corpse;
	int FemaleCorpse;
	std::string Blood;
	int Experience, SummonCost, FleeThreshold, Attack, Defend, Armor, Poison, LoseTarget;
	int Strategy[4];
	std::vector<std::string> Flags;
	std::vector<TMonsterSkillData> Skills;
	std::vector<std::string> Talk;
	std::vector<TMonsterItemData> Inventory;
	std::vector<TScriptValue> Spells;
	std::vector<std::string> SpellWhere;
	std::string Script;
	std::string Where;
};

// A wave of a raid. Radius INT_MAX when missing (TAttackWave's default),
// Lifetime 0 when missing; Message and Location empty when missing.
struct TRaidWaveData {
	int Delay;
	int x, y, z;
	int Spread;
	std::string Race;
	int MinCount, MaxCount;
	int Radius;
	int Lifetime;
	std::string Message;
	std::string Location;
	std::vector<TMonsterItemData> Inventory;
	std::string Where;
};

// A raid. Big for type "big"; Interval and Date 0 for the one not given.
struct TRaidData {
	std::string Key;
	std::string Description;
	bool Big;
	int Interval;
	int Date;
	std::string Script;
	std::vector<TRaidWaveData> Waves;
	std::string Where;
};

struct TMonsterHomeData {
	std::string Race;
	int x, y, z;
	int Radius;
	int Amount;
	int Regeneration;
	std::string Where;
};

// Read the files (FileName) into the vector (cleared first), in the file's
// order. Each throws a const char * "<file>:<line>:<column>: <what>"
// (tomlfile.hh) for a missing file, a syntax error, an unknown or missing
// key, a value of the wrong type or out of range, a key or race number given
// twice. Race keys in raids and homes are checked against Races (the races
// loaded before; LoadRaids and LoadMonsterHomes take them).
void LoadMonsterRaces(const char *FileName, std::vector<TMonsterData> &Races);
void LoadRaids(const char *FileName, const std::vector<TMonsterData> &Races, std::vector<TRaidData> &Raids);
void LoadMonsterHomes(const char *FileName, const std::vector<TMonsterData> &Races,
		std::vector<TMonsterHomeData> &Homes);

#endif
