// moveuse_writer.hh: writes the game's move and use rules as moveuse.toml
// (moveuse_writer.cc).

#ifndef PATELLA_CONVERT_MOVEUSE_WRITER_HH
#define PATELLA_CONVERT_MOVEUSE_WRITER_HH

#include <string>
#include <vector>

#include "moveuse_dat.hh"

// The object types the items.toml file ItemsToml defines (its [[item]] ids),
// by number, for MoveUseToToml's comments. Throws a const char * for a file
// that cannot be read or parsed.
std::vector<bool> DefinedTypes(const char *ItemsToml);

// The text of moveuse.toml for the rules as the game's tables hold them
// (MoveUseDatabases, MoveUseConditions, MoveUseActions and the dynamic
// strings), in the order of Extras, with its sections and notes. Each object
// type a rule names gets its name from the loaded object types as a comment,
// or "(not in items.toml)" for one that Defined does not hold.
// Throws a const char * for what moveuse.toml cannot hold: sections it cannot
// tell apart (one without rules, or one that ends and starts again under the
// same name with no rule between), an END without its BEGIN or a BEGIN
// without its END, and values the game does not know (a condition or an
// action of an unknown name, a comparison other than the six, a modifier, an
// object, flag, attribute, skill or right out of range).
std::string MoveUseToToml(const TMoveUseDatExtras &Extras, const std::vector<bool> &Defined);

#endif
