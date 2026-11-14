// tomlwrite.hh: what the converters share for writing TOML (tomlwrite.cc).

#ifndef PATELLA_CONVERT_TOMLWRITE_HH
#define PATELLA_CONVERT_TOMLWRITE_HH

#include <string>

// A Latin-1 text as a TOML basic string, quotes included, in UTF-8: '"' and
// '\' are escaped, control characters are written as \b, \t, \n, \f, \r or
// \uXXXX.
std::string TomlQuote(const std::string &Latin1);

// Writes Text to the file FileName, or to standard output for "-". Throws a
// const char * when the file cannot be written.
void WriteTextFile(const char *FileName, const std::string &Text);

#endif
