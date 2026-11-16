// monsters_writer.hh: the writers of the converted monster data
// (monsters_writer.cc): the game's monsters.toml, raids.toml and
// monster_homes.toml, in the schema of monsterdata.hh.

#ifndef PATELLA_CONVERT_MONSTERS_WRITER_HH
#define PATELLA_CONVERT_MONSTERS_WRITER_HH

#include <string>
#include <vector>

#include "mon_files.hh"

// monsters.toml: its header, then one [[monster]] per race in the order of
// Races (the race numbers'), every key written but the optional ones that
// are empty (flags, skills, talk, inventory, the spells) and female_corpse
// when it is corpse.
std::string MonstersToToml(const std::vector<TMonsterData> &Races);

// raids.toml: its header, then one [[raid]] per raid in the order of Raids,
// with the .evt's comments where they stand and a blank line where it has
// blank lines.
std::string RaidsToToml(const std::vector<TRaidFile> &Raids);

// monster_homes.toml: its header, then the array homes, one inline table per
// line in the order of Homes.
std::string MonsterHomesToToml(const std::vector<TMonsterHomeData> &Homes);

// A value of a spell as TOML: a number, a string, an array [a, b] or an inline
// table { key = value, ... }.
std::string TomlValue(const TScriptValue &Value);

// Writes DataFolder/monsters.toml, DataFolder/raids.toml and
// DataFolder/monster_homes.toml. Throws a const char * when a file cannot be
// written.
void WriteMonsterFiles(const TMonsterFolder &Folder, const std::vector<TMonsterHomeData> &Homes,
		const char *DataFolder);

#endif
