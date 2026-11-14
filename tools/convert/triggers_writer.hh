// triggers_writer.hh: writes the game's trigger rules as triggers.toml
// (triggers_writer.cc).

#ifndef PATELLA_CONVERT_TRIGGERS_WRITER_HH
#define PATELLA_CONVERT_TRIGGERS_WRITER_HH

#include <string>
#include <vector>

#include "moveuse_dat.hh"

// The object types the items.toml file ItemsToml defines (its [[item]] ids),
// by number, for TriggersToToml's comments. Throws a const char * for a file
// that cannot be read or parsed.
std::vector<bool> DefinedTypes(const char *ItemsToml);

// The text of triggers.toml for the rules as the game's tables hold them
// (TriggerDatabases, TriggerConditions, TriggerActions and the dynamic
// strings), in the order of Extras, with its sections and notes. Each object
// type a rule names gets its name from the loaded object types as a comment,
// or "(not in items.toml)" for one that Defined does not hold.
// Throws a const char * for what triggers.toml cannot hold: sections it cannot
// tell apart (one without rules, or one that ends and starts again under the
// same name with no rule between), an END without its BEGIN or a BEGIN
// without its END, and values the game does not know (a condition or an
// action of an unknown name, a comparison other than the six, a modifier, an
// object, flag, attribute, skill or right out of range).
std::string TriggersToToml(const TMoveUseDatExtras &Extras, const std::vector<bool> &Defined);

#endif
