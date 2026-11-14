// commands_reference.hh: the rights the game's code checked for its commands
// (commands_reference.cc), as it did before it read them from commands.toml.

#ifndef PATELLA_CONVERT_COMMANDS_REFERENCE_HH
#define PATELLA_CONVERT_COMMANDS_REFERENCE_HH

// Fills the game's CommandRight (magicdata.hh) with the rights the code named
// in each right check of a command, and NO_RIGHT everywhere else.
void InitReferenceCommandRights();

#endif
