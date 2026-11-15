// spells_writer.hh: writes the game's spell table as spells.toml and, with
// the rights of the commands, commands.toml (spells_writer.cc).

#ifndef PATELLA_CONVERT_SPELLS_WRITER_HH
#define PATELLA_CONVERT_SPELLS_WRITER_HH

#include <string>

// The words of a spell as spells.toml writes them: the class syllable and the
// next syllable as one word, every further syllable a word of its own (as the
// game's GetSpellString writes them, which also drops the last letter of a
// spell of one syllable).
std::string SpellWords(const unsigned char *Syllable);

// The script spells.toml or commands.toml gives the entry SpellNr of the
// game's SpellList: its script name (GetReferenceScriptName) in the folder of
// its class, "spells/" for ex and ut, "runes/" for ad and "commands/" for al
// and om, with ".lua" ("spells/light_healing.lua"). Throws a const char * for
// a number without a script name.
std::string SpellScript(int SpellNr);

// The text of spells.toml for the spells of the game's SpellList (spell
// numbers 1 to 255; an entry that is all zero is no spell) whose words start
// with ad, ex or ut, in the order of their numbers: number, name, words,
// level, mana and soul points; the flags it has; its rune, if it makes one;
// its script (SpellScript). Throws a const char * for what spells.toml or
// commands.toml cannot hold, anywhere in the table: a spell without a name,
// words that do not start with a class syllable or have an unknown syllable
// or a gap, a name longer than 30 characters, two spells with the same words,
// a negative mana, soul points or charges, an unknown flag, rune values
// without a rune group, a command (al or om) with a level, mana, soul points,
// flags or a rune, or a number without a script name.
std::string SpellsToToml();

// The text of commands.toml for the commands of the game's SpellList, the
// spells whose words start with al or om, in the order of their numbers:
// number, name, words, the rights of the command's right checks
// (GetCommandRightChecks) in CommandRight and its script (SpellScript), which
// comes before the rights when they are a table of their own. Throws a
// const char * for what SpellsToToml refuses, and for a right without a name,
// a right of a check that no command has, or a command without the right of
// one of its checks.
std::string CommandsToToml();

#endif
