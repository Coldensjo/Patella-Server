// items_writer.hh: writes the game's object type table as items.toml
// (items_writer.cc).

#ifndef PATELLA_CONVERT_ITEMS_WRITER_HH
#define PATELLA_CONVERT_ITEMS_WRITER_HH

#include <string>

#include "objects_srv.hh"

// The text of items.toml for the types Extras names, as the game's
// ObjectTypes holds them: name and description where the type has one, the
// flags it has, and the attributes objects.srv gave, in the order it gave
// them; then the list [[use]] of the built-in uses of objects, the same for
// every objects.srv. Throws a const char * for what items.toml cannot hold: a
// type above 65535, a meaning of 49, or a special object whose type was
// defined again without its meaning (SpecialObjects keeps it, the type no
// longer says so).
std::string ItemsToToml(const TObjectsSrvExtras &Extras);

#endif
