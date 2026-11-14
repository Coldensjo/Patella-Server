// spells_writer.hh: writes the game's spell table as spells.toml
// (spells_writer.cc).

#ifndef PATELLA_CONVERT_SPELLS_WRITER_HH
#define PATELLA_CONVERT_SPELLS_WRITER_HH

#include <string>

// The words of a spell as spells.toml writes them: the class syllable and the
// next syllable as one word, every further syllable a word of its own (as the
// game's GetSpellString writes them, which also drops the last letter of a
// spell of one syllable).
std::string SpellWords(const unsigned char *Syllable);

// The text of spells.toml for the spells of the game's SpellList (spell
// numbers 1 to 255; an entry that is all zero is no spell), in the order of
// their numbers: number, name, words, level, mana and soul points; the flags
// it has; its rune, if it makes one. Throws a const char * for what
// spells.toml cannot hold: a spell without a name, words that do not start
// with a class syllable or have an unknown syllable or a gap, a name longer
// than 30 characters, two spells with the same words, a negative mana, soul
// points or charges, an unknown flag, or rune values without a rune group.
std::string SpellsToToml();

#endif
