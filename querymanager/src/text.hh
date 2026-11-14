// Character set helpers.
//
// Strings on the wire and in the game's data files are ISO-8859-1 (the
// German log texts of the game server are stored that way, e.g. 0xF6 for
// o-umlaut).
// Inside this program a `std::string` that came from the wire or goes to it
// holds those bytes unchanged. The SQLite implementation converts to UTF-8
// for its TEXT columns and back.

#ifndef QM_TEXT_HH
#define QM_TEXT_HH

#include <string>

namespace qm {

std::string latin1ToUtf8(const std::string &latin1);
// Code points above U+00FF and malformed sequences become '?'.
std::string utf8ToLatin1(const std::string &utf8);

// Lower-case form of an ISO-8859-1 string, used as the lookup key for
// character names (names are compared without regard to case).
std::string foldLatin1(const std::string &latin1);

} // namespace qm

#endif
