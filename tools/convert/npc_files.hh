// npc_files.hh: the reader of the original NPC files (npc_files.cc): each
// .npc file of the npc/ folder (an NPC's data and its behaviour block) and the
// .ndb rule blocks they include, as the game's TNPC::TNPC,
// TBehaviourDatabase::TBehaviourDatabase and InitNPCs read them before
// npcs.toml and the Lua scripts, with the comments and the layout the written
// scripts keep.

#ifndef PATELLA_CONVERT_NPC_FILES_HH
#define PATELLA_CONVERT_NPC_FILES_HH

#include <string>
#include <vector>

#include "dbfuncs.hh"

// One element of a rule, by the names of lib/npc.lua (scripting.hh, "NPCs"):
// a string (a word of a condition, a text of an action), a number (a value),
// a name (a property, FORCE, P1 to P3, a value such as TOPIC, an action such
// as idle or previous) or a call of a constructor with its arguments (eq(a,
// b), add(a, b), count(v), param(n), setTopic(v), teleport(x, y, z), ...).
struct TNpcElement {
	enum TKind {STRING, NUMBER, NAME, CALL};
	TKind Kind = NAME;
	std::string Text;   // STRING: its Latin-1 bytes; NAME and CALL: the name
	int Number = 0;     // NUMBER: the int the tokenizer made
	std::vector<TNpcElement> Arguments;   // CALL
};

// A rule: its conditions (none: the rule holds always) and its actions (at
// least one; NOP, which the game stored as no action, is the action nop).
struct TNpcRule {
	std::vector<TNpcElement> Conditions;
	std::vector<TNpcElement> Actions;
};

// One line of a written rule list: a rule, the rule block an include put
// there, a comment line of the source, or an empty line (for a run of blank
// lines of the source between two of the others).
struct TNpcListItem {
	enum TKind {RULE, BLOCK, COMMENT, BLANK};
	TKind Kind = RULE;
	// RULE: the rule's index in the file's Rules; BLOCK: the index of the
	// block's first rule there, and RuleCount the number of its rules.
	int Rule = -1;
	int RuleCount = 0;
	std::string Block;      // BLOCK: the block's name (its file without ".ndb")
	// COMMENT: the comment's text after its '#'; RULE and BLOCK: the comment
	// after it on its line, if HasComment. Latin-1, without a CR at the end.
	std::string Comment;
	bool HasComment = false;
	// RULE: the comments inside the rule (a rule over several lines), which
	// the script writes on lines of their own before it.
	std::vector<std::string> CommentsBefore;
};

// A rule list with the comments of its file: those for the script's header
// (an .npc's outside its behaviour block, before it; an .ndb's before its
// first rule), the list's lines, and the comments after the behaviour
// block's closing brace (an .npc's only).
struct TNpcList {
	std::vector<std::string> HeaderComments;
	std::vector<TNpcListItem> Items;
	std::vector<std::string> TrailerComments;
};

// An .ndb file: a rule block, read on its own (its rules as an NPC that
// includes it has them in its own place).
struct TNpcBlock {
	std::string Name;   // the file name without ".ndb"
	std::vector<TNpcRule> Rules;
	TNpcList List;
};

// An .npc file: the NPC's data as TNPC::TNPC set it, and its rules as
// TBehaviourDatabase made them, the included blocks' in their places.
struct TNpcFile {
	std::string FileName;   // the file name without ".npc"
	std::string Name;       // Latin-1
	int Sex = 0;            // SEX_MALE or SEX_FEMALE
	int Race = 0;
	TOutfit Outfit;         // as TOutfit::load filled it, the rest 0
	int HomeX = 0, HomeY = 0, HomeZ = 0;
	int Radius = 0;
	int GoStrength = 0;
	std::vector<TNpcRule> Rules;   // all of them, the blocks' included
	int OwnRules = 0;              // those of the .npc itself
	TNpcList List;                 // RULE items index Rules
	std::vector<std::string> Blocks;   // the included blocks, in the order of their first include
};

// The npc/ folder: the NPCs in the order the game read them, the blocks in
// the order of their names.
struct TNpcFolder {
	std::vector<TNpcFile> Npcs;
	std::vector<TNpcBlock> Blocks;
};

// Reads the folder Folder as InitNPCs did: every entry whose name is longer
// than 4 characters and ends in ".npc", in the order readdir gives them; and
// every such ".ndb" entry, sorted by name, each read on its own. Each NPC's
// included blocks must give it exactly the rules the block gives on its own.
// Throws a const char * "<file>: <what>", <what> the game's text
// (TReadScriptFile's "error in script-file \"<file>\", line <N>: <what>",
// "gostrength before race in npc-script-file", "behaviour database specified
// twice for NPC", "no name specified for NPC", "no startpoint specified for
// NPC", "no behaviour database specified for NPC") or, in that same form,
// what the conversion refuses besides (npc_files.cc).
void ReadNpcFolder(const char *Folder, TNpcFolder &Result);

// Reads one .npc file (Path), its includes from its folder; FileName is set
// from Path. Throws as ReadNpcFolder.
TNpcFile ReadNpcFile(const std::string &Path);

// Reads one .ndb file (Path) on its own. Throws as ReadNpcFolder.
TNpcBlock ReadNpcBlock(const std::string &Path);

// The name of the local that holds a block in a script: the block's name in
// camelCase, its parts between '-' joined with the first letter of each part
// after the first upper case (gen-t-furniture-chairs-s ->
// genTFurnitureChairsS).
std::string BlockLocalName(const std::string &Block);

#endif
