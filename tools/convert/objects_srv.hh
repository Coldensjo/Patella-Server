// objects_srv.hh: the reader of the original objects.srv (objects_srv.cc), the
// game's LoadObjects as it was before the game read items.toml.

#ifndef PATELLA_CONVERT_OBJECTS_SRV_HH
#define PATELLA_CONVERT_OBJECTS_SRV_HH

#include <map>
#include <vector>

// What the conversion needs besides the object type table: the types the file
// defines, and for each the type attributes it gives, in the order the file
// writes them (the table cannot tell an attribute written as 0 from one left
// out). A type defined twice counts as defined once, with the attributes of
// its last definition.
struct TObjectsSrvExtras {
	std::map<int, std::vector<int>> Types;
};

// Reads the objects.srv file FileName into the game's ObjectTypes and
// SpecialObjects (objects.hh), as the game's old LoadObjects did. Throws a
// const char * as it did: the script reader's errors ("Unknown flag",
// "Special object already defined", ...).
void ReadObjectsSrv(const char *FileName, TObjectsSrvExtras &Extras);

#endif
