// spells_reference.cc: the spell table the game had in its code
// (spells_reference.hh), the reference spells.toml is written from. This is
// the game's CreateSpell and InitSpells as they were before spells.toml,
// moved here unchanged but for their names: InitSpells is
// InitReferenceSpells, and CreateSpell is static. GetReferenceScriptName
// names the scripts after the constants of the spell numbers.

#include <cctype>
#include <cstdarg>
#include <cstring>
#include <string>

#include "magicdata.hh"
#include "utils.hh"

#include "spells_reference.hh"

static TSpellList *CreateSpell(int SpellNr, ...) {
	va_list va;
	va_start(va, SpellNr);
	int syl = 0;
	while (true) {
		char *Syl = va_arg(va, char *);
		if (Syl[0] == 0) {
			break;
		}

		// Syl is not empty, so the empty entries of SpellSyllable never match
		// it.
		for (int i = 1; i <= MAX_SYLLABLES - 1; i++) {
			if (strcmp(Syl, SpellSyllable[i]) == 0) {
				// Quirk: an eleventh syllable is written to Syllable[10], one
				// past the array (where RuneGr follows), before the error below.
				SpellList[SpellNr].Syllable[syl] = (unsigned char)i;
				syl++;
				if (syl > MAX_SPELL_SYLLABLES) {
					error("CreateSpell: Syllable count exceeded at spell %d.\n", SpellNr);
					throw "Spell has too many syllables";
				}
				break;
			}
		}
	}
	va_end(va);
	return &SpellList[SpellNr];
}

void InitReferenceSpells() {
	for (int i = 1; i < MAX_SPELLS; i++) {
		memset(&SpellList[i], 0, sizeof(TSpellList));
	}

	TSpellList *S;
	S = CreateSpell(SPELL_LIGHT_HEALING, "ex", "ura", "");
	S->Mana = 25;
	S->Level = 9;
	S->Flags = SPELLFLAG_FACTOR_MIN_100;
	S->Comment = "Light Healing";
	S = CreateSpell(SPELL_INTENSE_HEALING, "ex", "ura", "gran", "");
	S->Mana = 40;
	S->Level = 11;
	S->Flags = SPELLFLAG_FACTOR_MIN_100;
	S->Comment = "Intense Healing";
	S = CreateSpell(SPELL_ULTIMATE_HEALING, "ex", "ura", "vita", "");
	S->Mana = 160;
	S->Level = 20;
	S->Flags = SPELLFLAG_FACTOR_MIN_100;
	S->Comment = "Ultimate Healing";
	S = CreateSpell(SPELL_INTENSE_HEALING_RUNE, "ad", "ura", "gran", "");
	S->Mana = 240;
	S->Level = 15;
	S->RuneGr = 79;
	S->RuneNr = 5;
	S->Flags = SPELLFLAG_FACTOR_MIN_100;
	S->Amount = 1;
	S->RuneLevel = 1;
	S->SoulPoints = 2;
	S->Comment = "Intense Healing Rune";
	S = CreateSpell(SPELL_ULTIMATE_HEALING_RUNE, "ad", "ura", "vita", "");
	S->Mana = 400;
	S->Level = 24;
	S->RuneGr = 79;
	S->RuneNr = 13;
	S->Flags = SPELLFLAG_FACTOR_MIN_100;
	S->Amount = 1;
	S->RuneLevel = 4;
	S->SoulPoints = 3;
	S->Comment = "Ultimate Healing Rune";
	S = CreateSpell(SPELL_HASTE, "ut", "ani", "hur", "");
	S->Mana = 60;
	S->Level = 14;
	S->Flags = SPELLFLAG_PREMIUM;
	S->Comment = "Haste";
	S = CreateSpell(SPELL_LIGHT_MAGIC_MISSILE, "ad", "ori", "");
	S->Mana = 120;
	S->Level = 15;
	S->RuneGr = 79;
	S->RuneNr = 27;
	S->Flags = SPELLFLAG_AGGRESSIVE | SPELLFLAG_FACTOR_MIN_100;
	S->Amount = 5;
	S->RuneLevel = 0;
	S->SoulPoints = 1;
	S->Comment = "Light Magic Missile";
	S = CreateSpell(SPELL_HEAVY_MAGIC_MISSILE, "ad", "ori", "gran", "");
	S->Mana = 280;
	S->Level = 25;
	S->RuneGr = 79;
	S->RuneNr = 51;
	S->Flags = SPELLFLAG_AGGRESSIVE | SPELLFLAG_FACTOR_MIN_100;
	S->Amount = 5;
	S->RuneLevel = 4;
	S->SoulPoints = 2;
	S->Comment = "Heavy Magic Missile";
	S = CreateSpell(SPELL_SUMMON_CREATURE, "ut", "evo", "res", "para", "");
	S->Mana = 0;
	S->Level = 25;
	S->Flags = SPELLFLAG_AGGRESSIVE;
	S->Comment = "Summon Creature";
	S = CreateSpell(SPELL_LIGHT, "ut", "evo", "lux", "");
	S->Mana = 20;
	S->Level = 8;
	S->Flags = 0;
	S->Comment = "Light";
	S = CreateSpell(SPELL_GREAT_LIGHT, "ut", "evo", "gran", "lux", "");
	S->Mana = 60;
	S->Level = 13;
	S->Flags = 0;
	S->Comment = "Great Light";
	S = CreateSpell(SPELL_CONVINCE_CREATURE, "ad", "eta", "sio", "");
	S->Mana = 200;
	S->Level = 16;
	S->RuneGr = 79;
	S->RuneNr = 30;
	S->Flags = SPELLFLAG_AGGRESSIVE;
	S->Amount = 1;
	S->RuneLevel = 5;
	S->SoulPoints = 3;
	S->Comment = "Convince Creature";
	S = CreateSpell(SPELL_ENERGY_WAVE, "ex", "evo", "mort", "hur", "");
	S->Mana = 250;
	S->Level = 38;
	S->Flags = SPELLFLAG_AGGRESSIVE;
	S->Comment = "Energy Wave";
	S = CreateSpell(SPELL_CHAMELEON, "ad", "evo", "ina", "");
	S->Mana = 600;
	S->Level = 27;
	S->RuneGr = 79;
	S->RuneNr = 31;
	S->Flags = 0;
	S->Amount = 1;
	S->RuneLevel = 4;
	S->SoulPoints = 2;
	S->Comment = "Chameleon";
	S = CreateSpell(SPELL_FIREBALL, "ad", "ori", "flam", "");
	S->Mana = 160;
	S->Level = 17;
	S->RuneGr = 79;
	S->RuneNr = 42;
	S->Flags = SPELLFLAG_AGGRESSIVE | SPELLFLAG_FACTOR_MIN_100;
	S->Amount = 2;
	S->RuneLevel = 2;
	S->SoulPoints = 2;
	S->Comment = "Fireball";
	S = CreateSpell(SPELL_GREAT_FIREBALL, "ad", "ori", "gran", "flam", "");
	S->Mana = 480;
	S->Level = 23;
	S->RuneGr = 79;
	S->RuneNr = 44;
	S->Flags = SPELLFLAG_AGGRESSIVE | SPELLFLAG_FACTOR_MIN_100;
	S->Amount = 2;
	S->RuneLevel = 4;
	S->SoulPoints = 3;
	S->Comment = "Great Fireball";
	S = CreateSpell(SPELL_FIREBOMB, "ad", "evo", "mas", "flam", "");
	S->Mana = 600;
	S->Level = 27;
	S->RuneGr = 79;
	S->RuneNr = 45;
	S->Flags = SPELLFLAG_AGGRESSIVE;
	S->Amount = 2;
	S->RuneLevel = 5;
	S->SoulPoints = 4;
	S->Comment = "Firebomb";
	S = CreateSpell(SPELL_EXPLOSION, "ad", "evo", "mas", "hur", "");
	S->Mana = 720;
	S->Level = 31;
	S->RuneGr = 79;
	S->RuneNr = 53;
	S->Flags = SPELLFLAG_AGGRESSIVE;
	S->Amount = 3;
	S->RuneLevel = 6;
	S->SoulPoints = 4;
	S->Comment = "Explosion";
	S = CreateSpell(SPELL_FIRE_WAVE, "ex", "evo", "flam", "hur", "");
	S->Mana = 80;
	S->Level = 18;
	S->Flags = SPELLFLAG_AGGRESSIVE | SPELLFLAG_FACTOR_MIN_100;
	S->Comment = "Fire Wave";
	S = CreateSpell(SPELL_FIND_PERSON, "ex", "iva", "para", "");
	S->Mana = 20;
	S->Level = 8;
	S->Flags = 0;
	S->Comment = "Find Person";
	S = CreateSpell(SPELL_SUDDEN_DEATH, "ad", "ori", "vita", "vis", "");
	S->Mana = 880;
	S->Level = 45;
	S->RuneGr = 79;
	S->RuneNr = 8;
	S->Flags = SPELLFLAG_AGGRESSIVE;
	S->Amount = 1;
	S->RuneLevel = 15;
	S->SoulPoints = 5;
	S->Comment = "Sudden Death";
	S = CreateSpell(SPELL_ENERGY_BEAM, "ex", "evo", "vis", "lux", "");
	S->Mana = 100;
	S->Level = 23;
	S->Flags = SPELLFLAG_AGGRESSIVE;
	S->Comment = "Energy Beam";
	S = CreateSpell(SPELL_GREAT_ENERGY_BEAM, "ex", "evo", "gran", "vis", "lux", "");
	S->Mana = 200;
	S->Level = 29;
	S->Flags = SPELLFLAG_AGGRESSIVE;
	S->Comment = "Great Energy Beam";
	S = CreateSpell(SPELL_ULTIMATE_EXPLOSION, "ex", "evo", "gran", "mas", "vis", "");
	S->Mana = 1200;
	S->Level = 60;
	S->Flags = SPELLFLAG_AGGRESSIVE | SPELLFLAG_PREMIUM;
	S->Comment = "Ultimate Explosion";
	S = CreateSpell(SPELL_FIRE_FIELD, "ad", "evo", "grav", "flam", "");
	S->Mana = 240;
	S->Level = 15;
	S->RuneGr = 79;
	S->RuneNr = 41;
	S->Flags = SPELLFLAG_AGGRESSIVE;
	S->Amount = 3;
	S->RuneLevel = 1;
	S->SoulPoints = 1;
	S->Comment = "Fire Field";
	S = CreateSpell(SPELL_POISON_FIELD, "ad", "evo", "grav", "pox", "");
	S->Mana = 200;
	S->Level = 14;
	S->RuneGr = 79;
	S->RuneNr = 25;
	S->Flags = SPELLFLAG_AGGRESSIVE;
	S->Amount = 3;
	S->RuneLevel = 0;
	S->SoulPoints = 1;
	S->Comment = "Poison Field";
	S = CreateSpell(SPELL_ENERGY_FIELD, "ad", "evo", "grav", "vis", "");
	S->Mana = 320;
	S->Level = 18;
	S->RuneGr = 79;
	S->RuneNr = 17;
	S->Flags = SPELLFLAG_AGGRESSIVE;
	S->Amount = 3;
	S->RuneLevel = 3;
	S->SoulPoints = 2;
	S->Comment = "Energy Field";
	S = CreateSpell(SPELL_FIRE_WALL, "ad", "evo", "mas", "grav", "flam", "");
	S->Mana = 780;
	S->Level = 33;
	S->RuneGr = 79;
	S->RuneNr = 43;
	S->Flags = SPELLFLAG_AGGRESSIVE;
	S->Amount = 4;
	S->RuneLevel = 6;
	S->SoulPoints = 4;
	S->Comment = "Fire Wall";
	S = CreateSpell(SPELL_ANTIDOTE, "ex", "ana", "pox", "");
	S->Mana = 30;
	S->Level = 10;
	S->Flags = 0;
	S->Comment = "Antidote";
	S = CreateSpell(SPELL_DESTROY_FIELD, "ad", "ito", "grav", "");
	S->Mana = 120;
	S->Level = 17;
	S->RuneGr = 79;
	S->RuneNr = 1;
	S->Flags = 0;
	S->Amount = 3;
	S->RuneLevel = 3;
	S->SoulPoints = 2;
	S->Comment = "Destroy Field";
	S = CreateSpell(SPELL_ANTIDOTE_RUNE, "ad", "ana", "pox", "");
	S->Mana = 200;
	S->Level = 15;
	S->RuneGr = 79;
	S->RuneNr = 6;
	S->Flags = 0;
	S->Amount = 1;
	S->RuneLevel = 0;
	S->SoulPoints = 1;
	S->Comment = "Antidote Rune";
	S = CreateSpell(SPELL_POISON_WALL, "ad", "evo", "mas", "grav", "pox", "");
	S->Mana = 640;
	S->Level = 29;
	S->RuneGr = 79;
	S->RuneNr = 29;
	S->Flags = SPELLFLAG_AGGRESSIVE;
	S->Amount = 4;
	S->RuneLevel = 5;
	S->SoulPoints = 3;
	S->Comment = "Poison Wall";
	S = CreateSpell(SPELL_ENERGY_WALL, "ad", "evo", "mas", "grav", "vis", "");
	S->Mana = 1000;
	S->Level = 41;
	S->RuneGr = 79;
	S->RuneNr = 19;
	S->Flags = SPELLFLAG_AGGRESSIVE;
	S->Amount = 4;
	S->RuneLevel = 9;
	S->SoulPoints = 5;
	S->Comment = "Energy Wall";
	S = CreateSpell(SPELL_GET_ITEM_COUNT, "al", "evo", "para", "para", "");
	S->Mana = 0;
	S->Level = 0;
	S->Flags = 0;
	S->Comment = "Get Item";
	S = CreateSpell(SPELL_GET_ITEM, "al", "evo", "para", "");
	S->Mana = 0;
	S->Level = 0;
	S->Flags = 0;
	S->Comment = "Get Item";
	S = CreateSpell(SPELL_MOVE, "al", "ani", "para", "");
	S->Mana = 0;
	S->Level = 0;
	S->Flags = 0;
	S->Comment = "Move";
	S = CreateSpell(SPELL_CREATURE_ILLUSION, "ut", "evo", "res", "ina", "para", "");
	S->Mana = 100;
	S->Level = 23;
	S->Flags = 0;
	S->Comment = "Creature Illusion";
	S = CreateSpell(SPELL_STRONG_HASTE, "ut", "ani", "gran", "hur", "");
	S->Mana = 100;
	S->Level = 20;
	S->Flags = SPELLFLAG_PREMIUM;
	S->Comment = "Strong Haste";
	S = CreateSpell(SPELL_GET_EXPERIENCE, "al", "evo", "cogni", "para", "");
	S->Mana = 0;
	S->Level = 0;
	S->Flags = 0;
	S->Comment = "Get Experience";
	S = CreateSpell(SPELL_CHANGE_DATA, "al", "eta", "para", "");
	S->Mana = 0;
	S->Level = 0;
	S->Flags = 0;
	S->Comment = "Change Data";
	S = CreateSpell(SPELL_FOOD, "ex", "evo", "pan", "");
	S->Mana = 120;
	S->Level = 14;
	S->SoulPoints = 1;
	S->Flags = 0;
	S->Comment = "Food";
	S = CreateSpell(SPELL_MAGIC_SHIELD, "ut", "amo", "vita", "");
	S->Mana = 50;
	S->Level = 14;
	S->Flags = 0;
	S->Comment = "Magic Shield";
	S = CreateSpell(SPELL_INVISIBLE, "ut", "ana", "vid", "");
	S->Mana = 440;
	S->Level = 35;
	S->Flags = 0;
	S->Comment = "Invisible";
	S = CreateSpell(SPELL_GET_SKILL_EXPERIENCE, "al", "evo", "cogni", "para", "para", "");
	S->Mana = 0;
	S->Level = 0;
	S->Flags = 0;
	S->Comment = "Get Skill Experience";
	S = CreateSpell(SPELL_TELEPORT_TO_FRIEND, "al", "ani", "sio", "para", "");
	S->Mana = 0;
	S->Level = 0;
	S->Flags = 0;
	S->Comment = "Teleport to Friend";
	S = CreateSpell(SPELL_POISONED_ARROW, "ex", "evo", "con", "pox", "");
	S->Mana = 130;
	S->Level = 16;
	S->SoulPoints = 2;
	S->Flags = 0;
	S->Comment = "Poisoned Arrow";
	S = CreateSpell(SPELL_EXPLOSIVE_ARROW, "ex", "evo", "con", "flam", "");
	S->Mana = 290;
	S->Level = 25;
	S->SoulPoints = 3;
	S->Flags = 0;
	S->Comment = "Explosive Arrow";
	S = CreateSpell(SPELL_SOULFIRE, "ad", "evo", "res", "flam", "");
	S->Mana = 600;
	S->Level = 27;
	S->RuneGr = 79;
	S->RuneNr = 48;
	S->Flags = SPELLFLAG_AGGRESSIVE | SPELLFLAG_PREMIUM;
	S->Amount = 2;
	S->RuneLevel = 7;
	S->SoulPoints = 3;
	S->Comment = "Soulfire";
	S = CreateSpell(SPELL_CONJURE_ARROW, "ex", "evo", "con", "");
	S->Mana = 100;
	S->Level = 13;
	S->SoulPoints = 1;
	S->Flags = 0;
	S->Comment = "Conjure Arrow";
	S = CreateSpell(SPELL_RETRIEVE_FRIEND, "al", "liber", "sio", "para", "");
	S->Mana = 0;
	S->Level = 0;
	S->Flags = 0;
	S->Comment = "Retrieve Friend";
	S = CreateSpell(SPELL_SUMMON_WILD_CREATURE, "al", "evo", "res", "para", "");
	S->Mana = 0;
	S->Level = 0;
	S->Flags = 0;
	S->Comment = "Summon Wild Creature";
	S = CreateSpell(SPELL_PARALYZE, "ad", "ana", "ani", "");
	S->Mana = 1400;
	S->Level = 54;
	S->RuneGr = 79;
	S->RuneNr = 18;
	S->Flags = SPELLFLAG_AGGRESSIVE | SPELLFLAG_PREMIUM;
	S->Amount = 1;
	S->RuneLevel = 18;
	S->SoulPoints = 3;
	S->Comment = "Paralyze";
	S = CreateSpell(SPELL_ENERGYBOMB, "ad", "evo", "mas", "vis", "");
	S->Mana = 880;
	S->Level = 37;
	S->RuneGr = 79;
	S->RuneNr = 2;
	S->Flags = SPELLFLAG_AGGRESSIVE | SPELLFLAG_PREMIUM;
	S->Amount = 2;
	S->RuneLevel = 10;
	S->SoulPoints = 5;
	S->Comment = "Energybomb";
	S = CreateSpell(SPELL_POISON_STORM, "ex", "evo", "gran", "mas", "pox", "");
	S->Mana = 600;
	S->Level = 50;
	S->Flags = SPELLFLAG_AGGRESSIVE | SPELLFLAG_PREMIUM;
	S->Comment = "Poison Storm";
	S = CreateSpell(SPELL_BANISH_ACCOUNT, "om", "ana", "liber", "para", "para", "para", "");
	S->Mana = 0;
	S->Level = 0;
	S->Flags = 0;
	S->Comment = "Banish Account";
	S = CreateSpell(SPELL_GET_POSITION, "al", "iva", "tera", "");
	S->Mana = 0;
	S->Level = 0;
	S->Flags = 0;
	S->Comment = "Get Position";
	S = CreateSpell(SPELL_TEMPLE_TELEPORT, "om", "ani", "para", "");
	S->Mana = 0;
	S->Level = 0;
	S->Flags = 0;
	S->Comment = "Temple Teleport";
	S = CreateSpell(SPELL_DELETE_ACCOUNT, "om", "ana", "gran", "liber", "para", "para", "");
	S->Mana = 0;
	S->Level = 0;
	S->Flags = 0;
	S->Comment = "Delete Account";
	S = CreateSpell(SPELL_SET_NAMERULE, "om", "amo", "para", "");
	S->Mana = 0;
	S->Level = 0;
	S->Flags = 0;
	S->Comment = "Set Namerule";
	S = CreateSpell(SPELL_CREATE_GOLD, "al", "evo", "vis", "para", "");
	S->Mana = 0;
	S->Level = 0;
	S->Flags = 0;
	S->Comment = "Create Gold";
	S = CreateSpell(SPELL_CHANGE_PROFESSION_OR_SEX, "al", "eta", "vita", "para", "");
	S->Mana = 0;
	S->Level = 0;
	S->Flags = 0;
	S->Comment = "Change Profession or Sex";
	S = CreateSpell(SPELL_ENTRY_IN_CRIMINAL_RECORD, "om", "isa", "para", "para", "");
	S->Mana = 0;
	S->Level = 0;
	S->Flags = 0;
	S->Comment = "Entry in Criminal Record";
	S = CreateSpell(SPELL_NAMELOCK, "om", "ana", "hora", "para", "");
	S->Mana = 0;
	S->Level = 0;
	S->Flags = 0;
	S->Comment = "Namelock";
	S = CreateSpell(SPELL_KICK_PLAYER, "om", "ana", "para", "");
	S->Mana = 0;
	S->Level = 0;
	S->Flags = 0;
	S->Comment = "Kick Player";
	S = CreateSpell(SPELL_DELETE_CHARACTER, "om", "ana", "gran", "res", "para", "para", "");
	S->Mana = 0;
	S->Level = 0;
	S->Flags = 0;
	S->Comment = "Delete Character";
	S = CreateSpell(SPELL_BANISH_IP_ADDRESS, "om", "ana", "vis", "para", "para", "");
	S->Mana = 0;
	S->Level = 0;
	S->Flags = 0;
	S->Comment = "Banish IP Address";
	S = CreateSpell(SPELL_BANISH_CHARACTER, "om", "ana", "res", "para", "para", "para", "");
	S->Mana = 0;
	S->Level = 0;
	S->Flags = 0;
	S->Comment = "Banish Character";
	S = CreateSpell(SPELL_INVITE_GUESTS, "al", "eta", "sio", "");
	S->Mana = 0;
	S->Level = 0;
	S->Flags = 0;
	S->Comment = "Invite Guests";
	S = CreateSpell(SPELL_INVITE_SUBOWNERS, "al", "eta", "som", "");
	S->Mana = 0;
	S->Level = 0;
	S->Flags = 0;
	S->Comment = "Invite Subowners";
	S = CreateSpell(SPELL_KICK_GUEST, "al", "ana", "sio", "para", "");
	S->Mana = 0;
	S->Level = 0;
	S->Flags = 0;
	S->Comment = "Kick Guest";
	S = CreateSpell(SPELL_EDIT_DOOR, "al", "eta", "grav", "");
	S->Mana = 0;
	S->Level = 0;
	S->Flags = 0;
	S->Comment = "Edit Door";
	S = CreateSpell(SPELL_ULTIMATE_LIGHT, "ut", "evo", "vis", "lux", "");
	S->Mana = 140;
	S->Level = 26;
	S->Flags = SPELLFLAG_PREMIUM;
	S->Comment = "Ultimate Light";
	S = CreateSpell(SPELL_MAGIC_ROPE, "ex", "ani", "tera", "");
	S->Mana = 20;
	S->Level = 9;
	S->Flags = SPELLFLAG_PREMIUM;
	S->Comment = "Magic Rope";
	S = CreateSpell(SPELL_ENVENOM, "ad", "evo", "res", "pox", "");
	S->Mana = 400;
	S->Level = 21;
	S->RuneGr = 79;
	S->RuneNr = 32;
	S->Flags = SPELLFLAG_AGGRESSIVE | SPELLFLAG_PREMIUM;
	S->Amount = 1;
	S->RuneLevel = 4;
	S->SoulPoints = 2;
	S->Comment = "Envenom";
	S = CreateSpell(SPELL_DESINTEGRATE, "ad", "ito", "tera", "");
	S->Mana = 200;
	S->Level = 21;
	S->RuneGr = 79;
	S->RuneNr = 50;
	S->Amount = 3;
	S->RuneLevel = 4;
	S->SoulPoints = 3;
	S->Flags = SPELLFLAG_AGGRESSIVE | SPELLFLAG_PREMIUM;
	S->Comment = "Desintegrate";
	S = CreateSpell(SPELL_CONJURE_BOLT, "ex", "evo", "con", "mort", "");
	S->Mana = 140;
	S->Level = 17;
	S->SoulPoints = 2;
	S->Flags = SPELLFLAG_PREMIUM;
	S->Comment = "Conjure Bolt";
	S = CreateSpell(SPELL_BERSERK, "ex", "ori", "");
	S->Mana = 0;
	S->Level = 35;
	S->Flags = SPELLFLAG_AGGRESSIVE | SPELLFLAG_PREMIUM | SPELLFLAG_FACTOR_MAX_100;
	S->Comment = "Berserk";
	S = CreateSpell(SPELL_LEVITATE, "ex", "ani", "hur", "para", "");
	S->Mana = 50;
	S->Level = 12;
	S->Flags = SPELLFLAG_PREMIUM;
	S->Comment = "Levitate";
	S = CreateSpell(SPELL_MASS_HEALING, "ex", "ura", "gran", "mas", "res", "");
	S->Mana = 150;
	S->Level = 36;
	S->Flags = SPELLFLAG_PREMIUM | SPELLFLAG_FACTOR_MIN_100;
	S->Comment = "Mass Healing";
	S = CreateSpell(SPELL_ANIMATE_DEAD, "ad", "ana", "mort", "");
	S->Mana = 600;
	S->Level = 27;
	S->RuneGr = 79;
	S->RuneNr = 56;
	S->Amount = 1;
	S->RuneLevel = 4;
	S->SoulPoints = 5;
	S->Flags = SPELLFLAG_AGGRESSIVE | SPELLFLAG_PREMIUM;
	S->Comment = "Animate Dead";
	S = CreateSpell(SPELL_HEAL_FRIEND, "ex", "ura", "sio", "para", "");
	S->Mana = 70;
	S->Level = 18;
	S->Flags = SPELLFLAG_PREMIUM | SPELLFLAG_FACTOR_MIN_100;
	S->Comment = "Heal Friend";
	S = CreateSpell(SPELL_UNDEAD_LEGION, "ex", "ana", "mas", "mort", "");
	S->Mana = 500;
	S->Level = 30;
	S->Flags = SPELLFLAG_AGGRESSIVE | SPELLFLAG_PREMIUM;
	S->Comment = "Undead Legion";
	S = CreateSpell(SPELL_MAGIC_WALL, "ad", "evo", "grav", "tera", "");
	S->Mana = 750;
	S->Level = 32;
	S->RuneGr = 79;
	S->RuneNr = 33;
	S->Amount = 3;
	S->RuneLevel = 9;
	S->SoulPoints = 5;
	S->Flags = SPELLFLAG_AGGRESSIVE | SPELLFLAG_PREMIUM;
	S->Comment = "Magic Wall";
	S = CreateSpell(SPELL_FORCE_STRIKE, "ex", "ori", "mort", "");
	S->Mana = 20;
	S->Level = 11;
	S->Flags = SPELLFLAG_AGGRESSIVE | SPELLFLAG_PREMIUM;
	S->Comment = "Force Strike";
	S = CreateSpell(SPELL_ENERGY_STRIKE, "ex", "ori", "vis", "");
	S->Mana = 20;
	S->Level = 12;
	S->Flags = SPELLFLAG_AGGRESSIVE | SPELLFLAG_PREMIUM;
	S->Comment = "Energy Strike";
	S = CreateSpell(SPELL_FLAME_STRIKE, "ex", "ori", "flam", "");
	S->Mana = 20;
	S->Level = 12;
	S->Flags = SPELLFLAG_AGGRESSIVE | SPELLFLAG_PREMIUM;
	S->Comment = "Flame Strike";
	S = CreateSpell(SPELL_CANCEL_INVISIBILITY, "ex", "ana", "ina", "");
	S->Mana = 200;
	S->Level = 26;
	S->Flags = SPELLFLAG_PREMIUM;
	S->Comment = "Cancel Invisibility";
	S = CreateSpell(SPELL_POISONBOMB, "ad", "evo", "mas", "pox", "");
	S->Mana = 520;
	S->Level = 25;
	S->RuneGr = 79;
	S->RuneNr = 26;
	S->Flags = SPELLFLAG_AGGRESSIVE | SPELLFLAG_PREMIUM;
	S->Amount = 2;
	S->RuneLevel = 4;
	S->SoulPoints = 2;
	S->Comment = "Poisonbomb";
	S = CreateSpell(SPELL_ENCHANT_STAFF, "ex", "eta", "vis", "");
	S->Mana = 80;
	S->Level = 41;
	S->Flags = SPELLFLAG_PREMIUM;
	S->Comment = "Enchant Staff";
	S = CreateSpell(SPELL_CHALLENGE, "ex", "eta", "res", "");
	S->Mana = 30;
	S->Level = 20;
	S->Flags = SPELLFLAG_AGGRESSIVE | SPELLFLAG_PREMIUM;
	S->Comment = "Challenge";
	S = CreateSpell(SPELL_WILD_GROWTH, "ex", "evo", "grav", "vita", "");
	S->Mana = 220;
	S->Level = 27;
	S->Flags = SPELLFLAG_AGGRESSIVE | SPELLFLAG_PREMIUM;
	S->Comment = "Wild Growth";
	S = CreateSpell(SPELL_POWER_BOLT, "ex", "evo", "con", "vis", "");
	S->Mana = 800;
	S->Level = 59;
	S->SoulPoints = 3;
	S->Flags = SPELLFLAG_PREMIUM;
	S->Comment = "Power Bolt";
	S = CreateSpell(SPELL_GET_QUEST_VALUE, "al", "iva", "cogni", "para", "");
	S->Mana = 0;
	S->Level = 0;
	S->Flags = 0;
	S->Comment = "Get Quest Value";
	S = CreateSpell(SPELL_SET_QUEST_VALUE, "al", "eta", "cogni", "para", "para", "");
	S->Mana = 0;
	S->Level = 0;
	S->Flags = 0;
	S->Comment = "Set Quest Value";
	S = CreateSpell(SPELL_DESINTEGRATE_SPELL, "al", "ito", "tera", "");
	S->Mana = 0;
	S->Level = 0;
	S->Flags = 0;
	S->Comment = "Desintegrate Spell";
	S = CreateSpell(SPELL_LEVITATE_GAMEMASTER, "al", "ani", "hur", "para", "");
	S->Mana = 0;
	S->Level = 0;
	S->Flags = 0;
	S->Comment = "Levitate Gamemaster";
	S = CreateSpell(SPELL_CLEAR_QUEST_VALUES, "al", "ana", "cogni", "");
	S->Mana = 0;
	S->Level = 0;
	S->Flags = 0;
	S->Comment = "Clear Quest Values";
	S = CreateSpell(SPELL_KILL_ALL_CREATURES, "al", "ito", "mas", "res", "");
	S->Mana = 0;
	S->Level = 0;
	S->Flags = 0;
	S->Comment = "Kill All Creatures";
	S = CreateSpell(SPELL_START_MONSTERRAID, "al", "evo", "mas", "res", "para", "");
	S->Mana = 0;
	S->Level = 0;
	S->Flags = 0;
	S->Comment = "Start Monsterraid";
}

// The SPELL_ constants by name, each spell and command of the reference table
// once.
#define SCRIPT_NAME(Name) {SPELL_##Name, #Name}
static const struct {
	int SpellNr;
	const char *Constant;
} ScriptNames[] = {
	SCRIPT_NAME(LIGHT_HEALING),
	SCRIPT_NAME(INTENSE_HEALING),
	SCRIPT_NAME(ULTIMATE_HEALING),
	SCRIPT_NAME(INTENSE_HEALING_RUNE),
	SCRIPT_NAME(ULTIMATE_HEALING_RUNE),
	SCRIPT_NAME(HASTE),
	SCRIPT_NAME(LIGHT_MAGIC_MISSILE),
	SCRIPT_NAME(HEAVY_MAGIC_MISSILE),
	SCRIPT_NAME(SUMMON_CREATURE),
	SCRIPT_NAME(LIGHT),
	SCRIPT_NAME(GREAT_LIGHT),
	SCRIPT_NAME(CONVINCE_CREATURE),
	SCRIPT_NAME(ENERGY_WAVE),
	SCRIPT_NAME(CHAMELEON),
	SCRIPT_NAME(FIREBALL),
	SCRIPT_NAME(GREAT_FIREBALL),
	SCRIPT_NAME(FIREBOMB),
	SCRIPT_NAME(EXPLOSION),
	SCRIPT_NAME(FIRE_WAVE),
	SCRIPT_NAME(FIND_PERSON),
	SCRIPT_NAME(SUDDEN_DEATH),
	SCRIPT_NAME(ENERGY_BEAM),
	SCRIPT_NAME(GREAT_ENERGY_BEAM),
	SCRIPT_NAME(ULTIMATE_EXPLOSION),
	SCRIPT_NAME(FIRE_FIELD),
	SCRIPT_NAME(POISON_FIELD),
	SCRIPT_NAME(ENERGY_FIELD),
	SCRIPT_NAME(FIRE_WALL),
	SCRIPT_NAME(ANTIDOTE),
	SCRIPT_NAME(DESTROY_FIELD),
	SCRIPT_NAME(ANTIDOTE_RUNE),
	SCRIPT_NAME(POISON_WALL),
	SCRIPT_NAME(ENERGY_WALL),
	SCRIPT_NAME(GET_ITEM_COUNT),
	SCRIPT_NAME(GET_ITEM),
	SCRIPT_NAME(MOVE),
	SCRIPT_NAME(CREATURE_ILLUSION),
	SCRIPT_NAME(STRONG_HASTE),
	SCRIPT_NAME(GET_EXPERIENCE),
	SCRIPT_NAME(CHANGE_DATA),
	SCRIPT_NAME(FOOD),
	SCRIPT_NAME(MAGIC_SHIELD),
	SCRIPT_NAME(INVISIBLE),
	SCRIPT_NAME(GET_SKILL_EXPERIENCE),
	SCRIPT_NAME(TELEPORT_TO_FRIEND),
	SCRIPT_NAME(POISONED_ARROW),
	SCRIPT_NAME(EXPLOSIVE_ARROW),
	SCRIPT_NAME(SOULFIRE),
	SCRIPT_NAME(CONJURE_ARROW),
	SCRIPT_NAME(RETRIEVE_FRIEND),
	SCRIPT_NAME(SUMMON_WILD_CREATURE),
	SCRIPT_NAME(PARALYZE),
	SCRIPT_NAME(ENERGYBOMB),
	SCRIPT_NAME(POISON_STORM),
	SCRIPT_NAME(BANISH_ACCOUNT),
	SCRIPT_NAME(GET_POSITION),
	SCRIPT_NAME(TEMPLE_TELEPORT),
	SCRIPT_NAME(DELETE_ACCOUNT),
	SCRIPT_NAME(SET_NAMERULE),
	SCRIPT_NAME(CREATE_GOLD),
	SCRIPT_NAME(CHANGE_PROFESSION_OR_SEX),
	SCRIPT_NAME(ENTRY_IN_CRIMINAL_RECORD),
	SCRIPT_NAME(NAMELOCK),
	SCRIPT_NAME(KICK_PLAYER),
	SCRIPT_NAME(DELETE_CHARACTER),
	SCRIPT_NAME(BANISH_IP_ADDRESS),
	SCRIPT_NAME(BANISH_CHARACTER),
	SCRIPT_NAME(INVITE_GUESTS),
	SCRIPT_NAME(INVITE_SUBOWNERS),
	SCRIPT_NAME(KICK_GUEST),
	SCRIPT_NAME(EDIT_DOOR),
	SCRIPT_NAME(ULTIMATE_LIGHT),
	SCRIPT_NAME(MAGIC_ROPE),
	SCRIPT_NAME(ENVENOM),
	SCRIPT_NAME(DESINTEGRATE),
	SCRIPT_NAME(CONJURE_BOLT),
	SCRIPT_NAME(BERSERK),
	SCRIPT_NAME(LEVITATE),
	SCRIPT_NAME(MASS_HEALING),
	SCRIPT_NAME(ANIMATE_DEAD),
	SCRIPT_NAME(HEAL_FRIEND),
	SCRIPT_NAME(UNDEAD_LEGION),
	SCRIPT_NAME(MAGIC_WALL),
	SCRIPT_NAME(FORCE_STRIKE),
	SCRIPT_NAME(ENERGY_STRIKE),
	SCRIPT_NAME(FLAME_STRIKE),
	SCRIPT_NAME(CANCEL_INVISIBILITY),
	SCRIPT_NAME(POISONBOMB),
	SCRIPT_NAME(ENCHANT_STAFF),
	SCRIPT_NAME(CHALLENGE),
	SCRIPT_NAME(WILD_GROWTH),
	SCRIPT_NAME(POWER_BOLT),
	SCRIPT_NAME(GET_QUEST_VALUE),
	SCRIPT_NAME(SET_QUEST_VALUE),
	SCRIPT_NAME(DESINTEGRATE_SPELL),
	SCRIPT_NAME(LEVITATE_GAMEMASTER),
	SCRIPT_NAME(CLEAR_QUEST_VALUES),
	SCRIPT_NAME(KILL_ALL_CREATURES),
	SCRIPT_NAME(START_MONSTERRAID),
};
#undef SCRIPT_NAME

const char *GetReferenceScriptName(int SpellNr) {
	static std::string Names[MAX_SPELLS];
	static bool Filled = false;
	if (!Filled) {
		for (const auto &Entry : ScriptNames) {
			std::string &Name = Names[Entry.SpellNr];
			for (const char *c = Entry.Constant; *c != 0; c++) {
				Name += static_cast<char>(tolower(static_cast<unsigned char>(*c)));
			}
		}
		Filled = true;
	}
	if (SpellNr < 0 || SpellNr >= MAX_SPELLS || Names[SpellNr].empty()) {
		return nullptr;
	}
	return Names[SpellNr].c_str();
}
