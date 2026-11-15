// spells_reference.hh: the spell table the game had in its code
// (spells_reference.cc), the game's InitSpells as it was before the game read
// spells.toml, with the numbers of its spells and commands and the names of
// the scripts that do what they do.

#ifndef PATELLA_CONVERT_SPELLS_REFERENCE_HH
#define PATELLA_CONVERT_SPELLS_REFERENCE_HH

#include "magicdata.hh"

// The spell numbers that only the reference table names, after the spells'
// names in spells.toml and commands.toml; the game's code used them to pick
// what a spell or command does, which their scripts do now. The numbers the
// game's code still names are in magicdata.hh.
const int SPELL_LIGHT_HEALING = 1;
const int SPELL_INTENSE_HEALING = 2;
const int SPELL_ULTIMATE_HEALING = 3;
const int SPELL_INTENSE_HEALING_RUNE = 4;
const int SPELL_ULTIMATE_HEALING_RUNE = 5;
const int SPELL_HASTE = 6;
const int SPELL_LIGHT_MAGIC_MISSILE = 7;
const int SPELL_HEAVY_MAGIC_MISSILE = 8;
const int SPELL_LIGHT = 10;
const int SPELL_GREAT_LIGHT = 11;
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
const int SPELL_CREATURE_ILLUSION = 38;
const int SPELL_STRONG_HASTE = 39;
const int SPELL_FOOD = 42;
const int SPELL_MAGIC_SHIELD = 44;
const int SPELL_INVISIBLE = 45;
const int SPELL_POISONED_ARROW = 48;
const int SPELL_EXPLOSIVE_ARROW = 49;
const int SPELL_SOULFIRE = 50;
const int SPELL_CONJURE_ARROW = 51;
const int SPELL_PARALYZE = 54;
const int SPELL_ENERGYBOMB = 55;
const int SPELL_POISON_STORM = 56;
const int SPELL_BANISH_ACCOUNT = 57;
const int SPELL_DELETE_ACCOUNT = 61;
const int SPELL_SET_NAMERULE = 62;
const int SPELL_ENTRY_IN_CRIMINAL_RECORD = 65;
const int SPELL_NAMELOCK = 66;
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

// Fills the game's SpellList (magicdata.hh) as the game's old InitSpells did:
// spells 1 to 255 emptied, then the spells defined in the code. Throws a
// const char * as it did, for a spell of more than 10 syllables.
void InitReferenceSpells();

// The name of the script of the spell or command SpellNr: the name of its
// SPELL_ constant in lower case without SPELL_ ("light_healing" for
// SPELL_LIGHT_HEALING), or nullptr for a number without one.
const char *GetReferenceScriptName(int SpellNr);

#endif
