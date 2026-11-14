// conversion_writer.hh: writes the game's conversion of old object numbers as
// conversion.toml (conversion_writer.cc).

#ifndef PATELLA_CONVERT_CONVERSION_WRITER_HH
#define PATELLA_CONVERT_CONVERSION_WRITER_HH

#include <string>

// The text of conversion.toml for the game's NewType (objects.hh): every old
// group and number with a type, but the game's own (0/0 to 0/10 and 250/0),
// which it sets itself. Throws a const char * for a table the old reader
// cannot have left: a type outside 1 to 8191, or the game's own entries not
// as the game sets them.
std::string ConversionToToml();

#endif
