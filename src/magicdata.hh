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

// The spell numbers (SpellList) that the code uses, named after the spells'
// names in spells.toml and commands.toml. What a spell or command does is its
// script (the key script of its entry), not its number.
const int SPELL_LIGHT_HEALING = 1;
const int SPELL_INTENSE_HEALING = 2;
const int SPELL_ULTIMATE_HEALING = 3;
const int SPELL_INTENSE_HEALING_RUNE = 4;
const int SPELL_ULTIMATE_HEALING_RUNE = 5;
const int SPELL_HASTE = 6;
const int SPELL_LIGHT_MAGIC_MISSILE = 7;
const int SPELL_HEAVY_MAGIC_MISSILE = 8;
const int SPELL_SUMMON_CREATURE = 9;
const int SPELL_LIGHT = 10;
const int SPELL_GREAT_LIGHT = 11;
const int SPELL_CONVINCE_CREATURE = 12;
const int SPELL_ENERGY_WAVE = 13;
const int SPELL_CHAMELEON = 14;
const int SPELL_FIREBALL = 15;
const int SPELL_GREAT_FIREBALL = 16;
const int SPELL_FIREBOMB = 17;
const int SPELL_EXPLOSION = 18;
const int SPELL_FIRE_WAVE = 19;
const int SPELL_FIND_PERSON = 20;
const int SPELL_SUDDEN_DEATH = 21;
const int SPELL_ENERGY_BEAM = 22;
const int SPELL_GREAT_ENERGY_BEAM = 23;
const int SPELL_ULTIMATE_EXPLOSION = 24;
const int SPELL_FIRE_FIELD = 25;
const int SPELL_POISON_FIELD = 26;
const int SPELL_ENERGY_FIELD = 27;
const int SPELL_FIRE_WALL = 28;
const int SPELL_ANTIDOTE = 29;
const int SPELL_DESTROY_FIELD = 30;
const int SPELL_ANTIDOTE_RUNE = 31;
const int SPELL_POISON_WALL = 32;
const int SPELL_ENERGY_WALL = 33;
const int SPELL_GET_ITEM_COUNT = 34;
const int SPELL_GET_ITEM = 35;
const int SPELL_MOVE = 37;
const int SPELL_CREATURE_ILLUSION = 38;
const int SPELL_STRONG_HASTE = 39;
const int SPELL_GET_EXPERIENCE = 40;
const int SPELL_CHANGE_DATA = 41;
const int SPELL_FOOD = 42;
const int SPELL_MAGIC_SHIELD = 44;
const int SPELL_INVISIBLE = 45;
const int SPELL_GET_SKILL_EXPERIENCE = 46;
const int SPELL_TELEPORT_TO_FRIEND = 47;
const int SPELL_POISONED_ARROW = 48;
const int SPELL_EXPLOSIVE_ARROW = 49;
const int SPELL_SOULFIRE = 50;
const int SPELL_CONJURE_ARROW = 51;
const int SPELL_RETRIEVE_FRIEND = 52;
const int SPELL_SUMMON_WILD_CREATURE = 53;
const int SPELL_PARALYZE = 54;
const int SPELL_ENERGYBOMB = 55;
const int SPELL_POISON_STORM = 56;
const int SPELL_BANISH_ACCOUNT = 57;
const int SPELL_GET_POSITION = 58;
const int SPELL_TEMPLE_TELEPORT = 60;
const int SPELL_DELETE_ACCOUNT = 61;
const int SPELL_SET_NAMERULE = 62;
const int SPELL_CREATE_GOLD = 63;
const int SPELL_CHANGE_PROFESSION_OR_SEX = 64;
const int SPELL_ENTRY_IN_CRIMINAL_RECORD = 65;
const int SPELL_NAMELOCK = 66;
const int SPELL_KICK_PLAYER = 67;
const int SPELL_DELETE_CHARACTER = 68;
const int SPELL_BANISH_IP_ADDRESS = 69;
const int SPELL_BANISH_CHARACTER = 70;
const int SPELL_INVITE_GUESTS = 71;
const int SPELL_INVITE_SUBOWNERS = 72;
const int SPELL_KICK_GUEST = 73;
const int SPELL_EDIT_DOOR = 74;
const int SPELL_ULTIMATE_LIGHT = 75;
const int SPELL_MAGIC_ROPE = 76;
const int SPELL_ENVENOM = 77;
const int SPELL_DESINTEGRATE = 78;
const int SPELL_CONJURE_BOLT = 79;
const int SPELL_BERSERK = 80;
const int SPELL_LEVITATE = 81;
const int SPELL_MASS_HEALING = 82;
const int SPELL_ANIMATE_DEAD = 83;
const int SPELL_HEAL_FRIEND = 84;
const int SPELL_UNDEAD_LEGION = 85;
const int SPELL_MAGIC_WALL = 86;
const int SPELL_FORCE_STRIKE = 87;
const int SPELL_ENERGY_STRIKE = 88;
const int SPELL_FLAME_STRIKE = 89;
const int SPELL_CANCEL_INVISIBILITY = 90;
const int SPELL_POISONBOMB = 91;
const int SPELL_ENCHANT_STAFF = 92;
const int SPELL_CHALLENGE = 93;
const int SPELL_WILD_GROWTH = 94;
const int SPELL_POWER_BOLT = 95;
const int SPELL_GET_QUEST_VALUE = 96;
const int SPELL_SET_QUEST_VALUE = 97;
const int SPELL_DESINTEGRATE_SPELL = 98;
const int SPELL_LEVITATE_GAMEMASTER = 99;
const int SPELL_CLEAR_QUEST_VALUES = 100;
const int SPELL_KILL_ALL_CREATURES = 101;
const int SPELL_START_MONSTERRAID = 102;

// The longest name of a spell (TSpellList's Comment) spells.toml and
// commands.toml may give.
const int MAX_SPELL_NAME_LENGTH = 30;

// The rights (RIGHT in info.hh), 0 to NUMBER_OF_RIGHTS - 1, which
// commands.toml names by RightNames.
const int NUMBER_OF_RIGHTS = 82;

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

// LoadSpells of spells.toml in the data folder (DATAPATH).
void InitSpells();

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

// LoadCommands of commands.toml in the data folder (DATAPATH).
void InitCommands();

// Fills Circle from the circles.toml file FileName. Throws a const char *
// "<file>:<line>:<column>: <what>" for what is wrong with it.
void LoadCircles(const char *FileName);

// LoadCircles of circles.toml in the data folder (DATAPATH).
void InitCircles();

extern char SpellSyllable[MAX_SYLLABLES][6];
extern TSpellList SpellList[MAX_SPELLS];
extern int CommandRight[MAX_SPELLS][MAX_COMMAND_RIGHTS];
extern const char *const RightNames[NUMBER_OF_RIGHTS];
extern TCircle Circle[MAX_CIRCLES];

#endif
