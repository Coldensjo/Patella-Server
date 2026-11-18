// magicdata.hh: the data of the spells (magicdata.cc): the spell table,
// loaded from spells.toml and, for the commands (the spells of the classes al
// and om), from commands.toml, with the syllables of the spells' words; the
// rights the commands need, also from commands.toml; the scripts that do what
// the spells and commands do, named by the entries of both files (magic.cc
// loads them and calls them with the spell functions it holds); and the
// circles, the area shapes the spells and the monsters' spells hit, loaded
// from circles.toml.

#ifndef PATELLA_MAGICDATA_HH
#define PATELLA_MAGICDATA_HH

#include "dbfuncs.hh"

// The words a spell can have, its class syllable included (TSpellList's
// Syllable, CheckForSpell); a shorter spell ends with syllable 0.
const int MAX_SPELL_SYLLABLES = 10;

// TSpellList's Flags. A spell's damage or healing is multiplied by a factor
// of 2 * level + 3 * magic level percent (ComputeDamage), which two flags
// bound.
const int SPELLFLAG_AGGRESSIVE = 1;
const int SPELLFLAG_PREMIUM = 2;
const int SPELLFLAG_FACTOR_MAX_100 = 4;
const int SPELLFLAG_FACTOR_MIN_100 = 8;

struct TSpellList {
	unsigned char Syllable[MAX_SPELL_SYLLABLES];
	unsigned char RuneGr;
	unsigned char RuneNr;
	const char *Comment;
	unsigned short Level;
	unsigned short RuneLevel;
	unsigned short Flags;
	int Mana;
	int SoulPoints;
	int Amount;
};

// The syllables TypeOfSpell, CheckForSpell and FindSpell look at by number:
// the first syllable of a spell is its class (al: character rights, ad:
// runes, ex and ut: spells cast at once, om: account rights), and "para"
// stands for a word the caster chooses (a name, a number).
const int SYLLABLE_AL = 1;
const int SYLLABLE_AD = 2;
const int SYLLABLE_EX = 3;
const int SYLLABLE_UT = 4;
const int SYLLABLE_OM = 5;
const int SYLLABLE_PARA = 6;

// The size of SpellSyllable: the syllables 1 to 50 (0 is the empty one).
const int MAX_SYLLABLES = 51;

// The spell numbers (SpellList) the code still names, after the spells' names
// in spells.toml and commands.toml. What a spell or command does is its script
// (the key script of its entry), not its number. The code names the commands
// that have right checks (GetCommandRightChecks, and tools/convert's
// reference rights, commands_reference.cc; their scripts check them with
// player:hasCommandRight, CheckCommandRight) and three spells whose cost the
// spellbook shows as a text instead of their mana (GetSpellbook: Summon
// Creature, Convince Creature, Berserk). The numbers of the other spells and
// commands are in the reference spell table of tools/convert
// (spells_reference.hh).
const int SPELL_SUMMON_CREATURE = 9;
const int SPELL_CONVINCE_CREATURE = 12;
const int SPELL_GET_ITEM_COUNT = 34;
const int SPELL_GET_ITEM = 35;
const int SPELL_MOVE = 37;
const int SPELL_GET_EXPERIENCE = 40;
const int SPELL_CHANGE_DATA = 41;
const int SPELL_GET_SKILL_EXPERIENCE = 46;
const int SPELL_TELEPORT_TO_FRIEND = 47;
const int SPELL_RETRIEVE_FRIEND = 52;
const int SPELL_SUMMON_WILD_CREATURE = 53;
const int SPELL_GET_POSITION = 58;
const int SPELL_TEMPLE_TELEPORT = 60;
const int SPELL_CREATE_GOLD = 63;
const int SPELL_CHANGE_PROFESSION_OR_SEX = 64;
const int SPELL_KICK_PLAYER = 67;
const int SPELL_BERSERK = 80;
const int SPELL_GET_QUEST_VALUE = 96;
const int SPELL_SET_QUEST_VALUE = 97;
const int SPELL_DESINTEGRATE_SPELL = 98;
const int SPELL_LEVITATE_GAMEMASTER = 99;
const int SPELL_CLEAR_QUEST_VALUES = 100;
const int SPELL_KILL_ALL_CREATURES = 101;
const int SPELL_START_MONSTERRAID = 102;
// Ours, not the original's: the command that reloads the scripts and the data
// (reload.hh), whose right Command.reloadScripts checks.
const int SPELL_RELOAD_SCRIPTS = 103;

// The longest name of a spell (TSpellList's Comment) spells.toml and
// commands.toml may give.
const int MAX_SPELL_NAME_LENGTH = 30;

// The rights (RIGHT in info.hh), 0 to NUMBER_OF_RIGHTS - 1, which
// commands.toml names by RightNames.
const int NUMBER_OF_RIGHTS = 83;

// The rights a command checks, at most MAX_COMMAND_RIGHTS (CommandRight). A
// command checks one right, COMMAND_RIGHT, which it needs to do anything;
// SPELL_MOVE checks one of four, by the word the caster gives: "up" and
// "down", "fast", "fastest", "slow" and "normal", coordinates, or the name of
// a mark.
const int MAX_COMMAND_RIGHTS = 4;
const int COMMAND_RIGHT = 0;
const int MOVE_RIGHT_VERTICAL = 0;
const int MOVE_RIGHT_SPEED = 1;
const int MOVE_RIGHT_COORDINATE = 2;
const int MOVE_RIGHT_MARK = 3;

// CommandRight of a right check that has no right.
const int NO_RIGHT = -1;

// The circles a shape can have (Circle): radius 0 to 9.
const int MAX_CIRCLES = 10;

// The fields one circle can hold (TCircle's x and y).
const int MAX_CIRCLE_FIELDS = 32;

// One circle: the offsets of its fields from the center, in the order the
// shapes visit them (row by row from north to south, each row from west to
// east).
struct TCircle {
	int x[MAX_CIRCLE_FIELDS];
	int y[MAX_CIRCLE_FIELDS];
	int Count;
};

// Fills SpellList from the spells.toml file FileName: the spells it defines
// (of the classes ad, ex and ut), and every other spell number empty, for the
// commands too (LoadCommands). Throws a const char *
// "<file>:<line>:<column>: <what>" for what is wrong with it.
void LoadSpells(const char *FileName);

// The right checks of a command: their names in commands.toml, by their
// index in CommandRight, or none. Names[0] is "" for a command that checks
// only COMMAND_RIGHT, whose right commands.toml gives as a string.
struct TCommandRightChecks {
	int Count;
	const char *Names[MAX_COMMAND_RIGHTS];
};
TCommandRightChecks GetCommandRightChecks(int SpellNr);

// The script that spells.toml or commands.toml names for the spell or command
// SpellNr (the key script of its entry: a script name below the scripts folder
// such as "spells/light_healing.lua", in "spells/" for the classes ex and ut,
// "runes/" for ad and "commands/" for al and om), or nullptr for none. Set by
// LoadSpells and LoadCommands, which check the name.
const char *GetSpellScript(int SpellNr);

// Where that key is, "<file>:<line>:<column>", or nullptr for none.
const char *GetSpellScriptPosition(int SpellNr);

// The right of a name in commands.toml (PREMIUM_ACCOUNT, ...), or NO_RIGHT
// for none.
int GetRightByName(const char *Name);

// Fills CommandRight and the commands' entries in SpellList from the
// commands.toml file FileName; the spells must be loaded already (LoadSpells),
// since a command may not have the number or the words of a spell. Throws a
// const char * "<file>:<line>:<column>: <what>" for what is wrong with it.
void LoadCommands(const char *FileName);

// Fills Circle from the circles.toml file FileName. Throws a const char *
// "<file>:<line>:<column>: <what>" for what is wrong with it.
void LoadCircles(const char *FileName);

// The reload (phase 8; reload.hh, magic.cc's MagicReloadPart): a second set
// of the tables above (SpellList, CommandRight, Circle and the scripts'
// names), the staged set, which a reload fills while the live set stays as it
// is, and which then changes places with it.
//
// ReadStagedMagicData empties the staged set and reads the circles.toml,
// spells.toml and commands.toml files (in this order) into it as LoadCircles,
// LoadSpells and LoadCommands read them into the live set, with their checks
// (the commands' against the staged spells). It refuses a spell or command
// number the live set has and the staged set has not: throws a const char *
// "<file>: <spell or command> <number> (<name>) is gone ..." (<file> the
// spells or commands file, by its class in the live set). The names of the
// spells and commands are static strings (AddStaticString, never freed): a
// name that an earlier read (any loader's, the start's included) made already
// takes that string again, so that reloads add static strings only for names
// not seen before. At start no name was read before, so every name is a new
// static string, in the file's order, as LoadSpells and LoadCommands make
// them. Throws as the loaders do; the staged set may then be filled in part
// (ClearStagedMagicData empties it).
void ReadStagedMagicData(const char *CirclesFile, const char *SpellsFile, const char *CommandsFile);

// Swaps the staged set and the live set, entry by entry (the install of a
// reload, and its uninstall after it).
void SwapStagedMagicData();

// Empties the staged set: every spell empty, every right check NO_RIGHT,
// every script none, every circle without fields.
void ClearStagedMagicData();

// What the staged set changes against the live set, number by number: the
// spells (classes ad, ex and ut) and commands (al and om) whose entry
// differs (words, name, values, flags, rune, rights or script) or is new, and
// whether a circle differs.
struct TMagicChanges {
	int SpellsChanged;
	int SpellsAdded;
	int CommandsChanged;
	int CommandsAdded;
	bool CirclesChanged;
};
TMagicChanges CompareStagedMagicData();

extern char SpellSyllable[MAX_SYLLABLES][6];
extern TSpellList SpellList[MAX_SPELLS];
extern int CommandRight[MAX_SPELLS][MAX_COMMAND_RIGHTS];
extern const char *const RightNames[NUMBER_OF_RIGHTS];
extern TCircle Circle[MAX_CIRCLES];

#endif
