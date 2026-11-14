#ifndef PATELLA_CRSKILL_HH
#define PATELLA_CRSKILL_HH

// The skill numbers: the index into TSkillBase::Skills and the skill tables
// of TPlayerData. The names are the words GetSkillByName knows (the skill
// names of the monster, NPC and moveuse scripts).
const int SKILL_LEVEL = 0;
const int SKILL_MAGICLEVEL = 1;
const int SKILL_HITPOINTS = 2;
const int SKILL_MANA = 3;
const int SKILL_GOSTRENGTH = 4;
const int SKILL_CARRYSTRENGTH = 5;
const int SKILL_SHIELDING = 6;
const int SKILL_DISTANCEFIGHTING = 7;
const int SKILL_SWORDFIGHTING = 8;
const int SKILL_CLUBFIGHTING = 9;
const int SKILL_AXEFIGHTING = 10;
const int SKILL_FISTFIGHTING = 11;
const int SKILL_MAGICDEFENSE = 12;
const int SKILL_FISHING = 13;
const int SKILL_EATING = 14;
const int SKILL_SHINING = 15;
const int SKILL_ILLUSION = 16;
const int SKILL_POISON = 17;
const int SKILL_BURNING = 18;
const int SKILL_ENERGY = 19;
const int SKILL_DRUNKEN = 20;
const int SKILL_MANASHIELD = 21;
const int SKILL_SOULPOINTS = 22;

// The size of the skill tables (TSkillBase::Skills and TimerList, the skill
// arrays of TPlayerData). Only the skills 0 to 22 above exist.
const int MAX_SKILLS = 25;

int GetSkillByName(char *Name);
void InitCrskill();
void ExitCrskill();

#endif
