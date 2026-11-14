// moveuse_dat.hh: the reader of the original moveuse.dat (moveuse_dat.cc), the
// game's LoadDataBase as it was before the game read moveuse.toml.

#ifndef PATELLA_CONVERT_MOVEUSE_DAT_HH
#define PATELLA_CONVERT_MOVEUSE_DAT_HH

#include <string>
#include <vector>

// One entry of moveuse.dat in the order of the file: the start of a section
// (BEGIN "name"), its end (END), or a rule, by its event type and its number
// in that event's rule database, with the text that may follow its actions
// (a note the game skips).
struct TMoveUseDatEntry {
	enum {BEGIN, END, RULE} Kind = RULE;
	std::string Name;
	int Event = 0;
	int Rule = 0;
	bool HasNote = false;
	std::string Note;
};

// What the conversion needs besides the game's tables: the sections and the
// order of the rules across the event types.
struct TMoveUseDatExtras {
	std::vector<TMoveUseDatEntry> Entries;
};

// Reads the moveuse.dat file FileName into the game's MoveUseConditions,
// MoveUseActions and MoveUseDatabases (moveusedata.hh), and its texts into
// the dynamic strings, as the game's old InitMoveUse and LoadDataBase did. The
// object types must be loaded. Throws a const char * as it did: the script
// reader's errors ("Unknown object type", "'->' expected", ...).
void ReadMoveUseDat(const char *FileName, TMoveUseDatExtras &Extras);

#endif
