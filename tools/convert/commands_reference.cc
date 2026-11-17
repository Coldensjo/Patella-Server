// commands_reference.cc: the rights the game's code checked for its commands
// (commands_reference.hh), which the reference commands.toml is written with.
// Each command function of magic.cc called CheckRight with one of these
// rights where it now calls CheckCommandRight with the command and the check.
// After them the commands that are ours, not the original code's (Reload
// Scripts), with their rights and scripts.

#include <cstdint>
#include <cstring>

#include "objects.hh"
#include "map.hh"
#include "info.hh"
#include "magicdata.hh"

#include "commands_reference.hh"

void InitReferenceCommandRights() {
	for (int i = 0; i < MAX_SPELLS; i++) {
		for (int j = 0; j < MAX_COMMAND_RIGHTS; j++) {
			CommandRight[i][j] = NO_RIGHT;
		}
	}

	// CreateThing, with and without a count.
	CommandRight[SPELL_GET_ITEM_COUNT][COMMAND_RIGHT] = CREATE_OBJECTS;
	CommandRight[SPELL_GET_ITEM][COMMAND_RIGHT] = CREATE_OBJECTS;
	// Teleport: up and down; fast, fastest, slow and normal; coordinates; a
	// mark.
	CommandRight[SPELL_MOVE][MOVE_RIGHT_VERTICAL] = TELEPORT_VERTICAL;
	CommandRight[SPELL_MOVE][MOVE_RIGHT_SPEED] = MODIFY_GOSTRENGTH;
	CommandRight[SPELL_MOVE][MOVE_RIGHT_COORDINATE] = TELEPORT_TO_COORDINATE;
	CommandRight[SPELL_MOVE][MOVE_RIGHT_MARK] = TELEPORT_TO_MARK;
	// CreateKnowledge, of the level and of a skill.
	CommandRight[SPELL_GET_EXPERIENCE][COMMAND_RIGHT] = CHANGE_SKILLS;
	CommandRight[SPELL_GET_SKILL_EXPERIENCE][COMMAND_RIGHT] = CHANGE_SKILLS;
	// ChangeData.
	CommandRight[SPELL_CHANGE_DATA][COMMAND_RIGHT] = CREATE_OBJECTS;
	// TeleportToCreature and TeleportPlayerToMe.
	CommandRight[SPELL_TELEPORT_TO_FRIEND][COMMAND_RIGHT] = TELEPORT_TO_CHARACTER;
	CommandRight[SPELL_RETRIEVE_FRIEND][COMMAND_RIGHT] = RETRIEVE;
	// SummonCreature of a god (not the spell Summon Creature).
	CommandRight[SPELL_SUMMON_WILD_CREATURE][COMMAND_RIGHT] = CREATE_MONSTERS;
	// GetPosition.
	CommandRight[SPELL_GET_POSITION][COMMAND_RIGHT] = SHOW_COORDINATE;
	// HomeTeleport.
	CommandRight[SPELL_TEMPLE_TELEPORT][COMMAND_RIGHT] = HOME_TELEPORT;
	// CreateMoney.
	CommandRight[SPELL_CREATE_GOLD][COMMAND_RIGHT] = CREATE_MONEY;
	// ChangeProfession.
	CommandRight[SPELL_CHANGE_PROFESSION_OR_SEX][COMMAND_RIGHT] = CHANGE_PROFESSION;
	// KickPlayer.
	CommandRight[SPELL_KICK_PLAYER][COMMAND_RIGHT] = KICK;
	// GetQuestValue, SetQuestValue and ClearQuestValues.
	CommandRight[SPELL_GET_QUEST_VALUE][COMMAND_RIGHT] = CHANGE_SKILLS;
	CommandRight[SPELL_SET_QUEST_VALUE][COMMAND_RIGHT] = CHANGE_SKILLS;
	CommandRight[SPELL_CLEAR_QUEST_VALUES][COMMAND_RIGHT] = CHANGE_SKILLS;
	// CleanupField of the field in front.
	CommandRight[SPELL_DESINTEGRATE_SPELL][COMMAND_RIGHT] = CLEANUP_FIELDS;
	// MagicClimbing of a gamemaster.
	CommandRight[SPELL_LEVITATE_GAMEMASTER][COMMAND_RIGHT] = LEVITATE;
	// KillAllMonsters and StartMonsterraid.
	CommandRight[SPELL_KILL_ALL_CREATURES][COMMAND_RIGHT] = CREATE_MONSTERS;
	CommandRight[SPELL_START_MONSTERRAID][COMMAND_RIGHT] = CREATE_MONSTERS;

	// Ours, not the original code's: Reload Scripts, which reloads the
	// scripts and the data while the game runs (reload.hh).
	TSpellList &Reload = SpellList[SPELL_RELOAD_SCRIPTS];
	memset(&Reload, 0, sizeof(Reload));
	const char *const ReloadWords[] = {"om", "evo", "gran", "cogni"};
	int Count = 0;
	for (const char *Word : ReloadWords) {
		for (int i = 1; i < MAX_SYLLABLES; i++) {
			if (strcmp(Word, SpellSyllable[i]) == 0) {
				Reload.Syllable[Count++] = static_cast<unsigned char>(i);
				break;
			}
		}
	}
	Reload.Comment = "Reload Scripts";
	CommandRight[SPELL_RELOAD_SCRIPTS][COMMAND_RIGHT] = RELOAD_SCRIPTS;
}

const char *GetOwnCommandScriptName(int SpellNr) {
	if (SpellNr == SPELL_RELOAD_SCRIPTS && SpellList[SpellNr].Syllable[0] == SYLLABLE_OM) {
		return "reload_scripts";
	}
	return nullptr;
}
