// conversion_lst.hh: the reader of the original conversion.lst
// (conversion_lst.cc), the game's LoadConversionList as it was before the
// game read conversion.toml.

#ifndef PATELLA_CONVERT_CONVERSION_LST_HH
#define PATELLA_CONVERT_CONVERSION_LST_HH

// Reads the conversion.lst file FileName into the game's NewType
// (objects.hh), as the game's old LoadConversionList did. Throws a const
// char * as it did ("Cannot open conversion.lst", "Error while loading
// conversion.lst" for a type outside 1 to 8191), and for an entry that is
// not three numbers and a group and number that lie outside NewType, where
// the old reader went on forever or wrote past NewType.
void ReadConversionLst(const char *FileName);

#endif
