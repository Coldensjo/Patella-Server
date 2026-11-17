// commands_reference.hh: the rights the game's code checked for its commands
// (commands_reference.cc), as it did before it read them from commands.toml,
// and the commands that are ours, not the original code's.

#ifndef PATELLA_CONVERT_COMMANDS_REFERENCE_HH
#define PATELLA_CONVERT_COMMANDS_REFERENCE_HH

// Fills the game's CommandRight (magicdata.hh) with the rights the code named
// in each right check of a command, and NO_RIGHT everywhere else; then adds
// the commands that are ours to SpellList, with their rights: 103 Reload
// Scripts ("omevo gran cogni", RELOAD_SCRIPTS).
void InitReferenceCommandRights();

// The script name of a command that is ours ("reload_scripts", without the
// folder and ".lua", as GetReferenceScriptName gives them), or nullptr: its
// number with words of its class (om for Reload Scripts) in SpellList; any
// other entry of that number (a spell, a command of the other class) has no
// script name.
const char *GetOwnCommandScriptName(int SpellNr);

#endif
