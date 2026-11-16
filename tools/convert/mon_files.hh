// mon_files.hh: the reader of the original monster files (mon_files.cc): the
// races (mon/*.mon), the raids (mon/*.evt) and the monster homes
// (dat/monster.db), as the game's LoadRace, LoadMonsterRaid and
// LoadMonsterhomes read them before monsters.toml, raids.toml and
// monster_homes.toml. It gives them as the loaders of monsterdata.hh give
// them from those files, with the comments and the layout of the raid files
// that raids.toml keeps.

#ifndef PATELLA_CONVERT_MON_FILES_HH
#define PATELLA_CONVERT_MON_FILES_HH

#include <string>
#include <vector>

#include "monsterdata.hh"

// A comment of a raid file: its text after the '#' (Latin-1, a CR at its end
// dropped), its line, and whether a token stands before it on that line.
struct TRaidComment {
	std::string Text;
	int Line = 0;
	bool Trailing = false;
};

// A key of a raid or of a wave as the .evt gives it: its name in raids.toml
// ("description", "type", "interval", "date"; "delay", "position", "spread",
// "race", "count", "radius", "lifetime", "message", "location",
// "inventory"), the lines of its first and of its last token, and for
// inventory the line of each object's '('.
struct TRaidKey {
	std::string Name;
	int Line = 0;
	int EndLine = 0;
	std::vector<int> ItemLines;
};

// The layout of a raid file: the raid's keys and each wave's keys in the
// file's order (a wave's first is delay), its comments and the lines that
// hold nothing but blanks.
struct TRaidLayout {
	std::vector<TRaidKey> RaidKeys;
	std::vector<std::vector<TRaidKey>> WaveKeys;
	std::vector<TRaidComment> Comments;
	std::vector<int> BlankLines;
};

// A raid file: the raid as LoadRaids gives it from raids.toml (Where: "<file>,
// line <N>" of the .evt), and its layout.
struct TRaidFile {
	TRaidData Raid;
	TRaidLayout Layout;
};

// The mon/ folder: the races by race number, the raids in the order readdir
// gives the .evt files, and what the game read and ignored ("<file>, line
// <N>: <what>"), in the order it was met.
struct TMonsterFolder {
	std::vector<TMonsterData> Races;
	std::vector<TRaidFile> Raids;
	std::vector<std::string> Ignored;
};

// Reads the folder Folder as LoadRaces and LoadMonsterRaids did: every entry
// whose name is longer than 4 characters and ends in ".mon", in the order
// readdir gives them, then every such ".evt" entry in that order. The races
// are given as LoadMonsterRaces gives them (every key the .mon lacks with the
// game's default; the spells as the tables of scripting.hh's "Monsters", a
// summon's race as its key), the raids as LoadRaids gives them (race numbers
// as keys; Interval or Date as the .evt gives it, the other 0; Radius INT_MAX
// and Lifetime 0 when missing). Throws a const char * "<file>: <what>",
// <what> the game's text (TReadScriptFile's "error in script-file \"<file>\",
// line <N>: <what>" with LoadRace's and LoadMonsterRaid's texts: "race number
// expected", "illegal race number", "race already defined", "unknown blood
// type", "unknown skill name", "unknown spell shape", "unknown spell impact",
// "zero spell delay", "type expected", "date or interval expected", "delay
// expected", "mincount greater than maxcount", "illegal number of monsters",
// "position expected", "race expected", "count expected", ...) or, in that
// same form, what the conversion refuses besides (mon_files.cc).
void ReadMonsterFolder(const char *Folder, TMonsterFolder &Result);

// Reads one .mon file (Path) into Race, as LoadRace did for a race not yet
// defined. A summon's race stays a number (TScriptValue::NUMBER) until
// ResolveRaceKeys. Notes what the game ignored in Ignored. Throws as
// ReadMonsterFolder.
void ReadRaceFile(const std::string &Path, TMonsterData &Race, std::vector<std::string> &Ignored);

// Turns the race numbers of the summons' spells into the races' keys. Throws
// as ReadMonsterFolder for a number of no race.
void ResolveRaceKeys(std::vector<TMonsterData> &Races);

// Reads one .evt file (Path), its race numbers turned into the keys of
// Races. Throws as ReadMonsterFolder.
TRaidFile ReadRaidFile(const std::string &Path, const std::vector<TMonsterData> &Races,
		std::vector<std::string> &Ignored);

// Reads monster.db (FileName) as LoadMonsterhomes did: the homes up to the
// race 0 that ends them, their race numbers turned into the keys of Races.
// Throws as ReadMonsterFolder.
void ReadMonsterHomes(const char *FileName, const std::vector<TMonsterData> &Races,
		std::vector<TMonsterHomeData> &Homes, std::vector<std::string> &Ignored);

#endif
