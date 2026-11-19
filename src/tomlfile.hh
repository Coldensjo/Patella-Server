// tomlfile.hh: the helpers the game's TOML readers share (tomlfile.cc): opening
// a file, errors with file:line:column, checking that a table has only known
// keys, and reading integers, numbers with a fraction and Latin-1 strings with
// their limits.
//
// The game's texts are Latin-1 and TOML files are UTF-8: a string is turned
// back into Latin-1 on reading, and a character outside Latin-1 is an error.
// Errors are thrown as a const char *, as the game's other loaders do; the
// text says "<file>:<line>:<column>: <what>".

#ifndef PATELLA_TOMLFILE_HH
#define PATELLA_TOMLFILE_HH

#include <initializer_list>
#include <string>

#include <toml++/toml.hpp>

// Parses a TOML file. Throws for a file that does not exist or cannot be
// read and for a syntax error.
toml::table TomlParseFile(const char *FileName);

// Throws "<file>:<line>:<column>: <text>" for a place in a file.
[[noreturn]] void TomlError(const toml::source_region &Where, const char *Format, ...)
	__attribute__((format(printf, 2, 3)));

// Throws for the first key of Table that is not one of Keys. What names the
// table in the error text ("the top level", "[paths]", ...).
void TomlCheckKeys(const toml::table &Table, const char *What, std::initializer_list<const char *> Keys);

// The table under Key, or nullptr when there is none. Throws when the value
// is not a table.
const toml::table *TomlGetTable(const toml::table &Table, const char *Key);

// The array under Key, or nullptr when there is none. Throws when the value
// is not an array.
const toml::array *TomlGetArray(const toml::table &Table, const char *Key);

// Sets Value to the integer under Key if there is one, and leaves it as it is
// otherwise. Throws when the value is not an integer or lies outside
// Min..Max.
void TomlGetNumber(const toml::table &Table, const char *Key, int &Value, int Min, int Max);

// Sets Value to the number under Key, written with or without a fraction
// (2.5 or 2), if there is one, and leaves it as it is otherwise. Throws when
// the value is not a number or lies outside Min..Max (inf and nan do).
void TomlGetFloat(const toml::table &Table, const char *Key, double &Value, double Min, double Max);

// Copies the string under Key, as Latin-1, into Buffer (Size bytes with the
// terminating zero) if there is one, and leaves Buffer as it is otherwise.
// Returns whether there was one. Throws when the value is not a string, has a
// character outside Latin-1 or a zero byte, or does not fit.
bool TomlGetString(const toml::table &Table, const char *Key, char *Buffer, int Size);

// The Latin-1 bytes of a UTF-8 string. Returns false when it has a character
// above U+00FF.
bool Utf8ToLatin1(const std::string &Text, std::string &Latin1);

// The UTF-8 form of a Latin-1 string.
std::string Latin1ToUtf8(const std::string &Latin1);

#endif
