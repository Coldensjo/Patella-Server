// npcs_writer.hh: the writers of the converted NPCs (npcs_writer.cc): the
// game's npcs.toml, each NPC's script npcs/<name>.lua and each rule block's
// module lib/npcs/<name>.lua (scripting.hh, "NPCs"; data/npcs.toml and
// data/scripts/npcs/cipfried.lua show the form).

#ifndef PATELLA_CONVERT_NPCS_WRITER_HH
#define PATELLA_CONVERT_NPCS_WRITER_HH

#include <string>
#include <vector>

#include "npc_files.hh"

// npcs.toml: its header, then one [[npc]] per NPC in the folder's order.
std::string NpcsToToml(const TNpcFolder &Folder);

// The script npcs/<name>.lua of an NPC.
std::string NpcScript(const TNpcFile &Npc);

// The module lib/npcs/<name>.lua of a rule block.
std::string BlockModule(const TNpcBlock &Block);

// Writes DataFolder/npcs.toml, DataFolder/scripts/npcs/<name>.lua for each
// NPC and DataFolder/scripts/lib/npcs/<name>.lua for each block, making the
// folders that are missing. Throws a const char * when a file cannot be
// written.
void WriteNpcFolder(const TNpcFolder &Folder, const char *DataFolder);

// A Latin-1 text as a Lua string in double quotes: the printable ASCII bytes
// (32 to 126) as they are but '"' and '\' escaped with '\', a line feed as
// \n, every other byte as \ddd (three decimal digits).
std::string LuaString(const std::string &Latin1);

// An element as a script writes it: a string as LuaString gives it, a number
// as a decimal int, a name, a call as "<name>(<argument>, ...)".
std::string LuaElement(const TNpcElement &Element);

// The canonical text of a rule list: one line per rule, "<condition>, ...
// -> <action>, ...", each element as LuaElement writes it.
std::string CanonicalRules(const std::vector<TNpcRule> &Rules);

#endif
