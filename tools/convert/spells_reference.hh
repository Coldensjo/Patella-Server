// spells_reference.hh: the spell table the game had in its code
// (spells_reference.cc), the game's InitSpells as it was before the game read
// spells.toml.

#ifndef PATELLA_CONVERT_SPELLS_REFERENCE_HH
#define PATELLA_CONVERT_SPELLS_REFERENCE_HH

// Fills the game's SpellList (magicdata.hh) as the game's old InitSpells did:
// spells 1 to 255 emptied, then the spells defined in the code. Throws a
// const char * as it did, for a spell of more than 10 syllables.
void InitReferenceSpells();

#endif
