// Reader for the game's text data files (.tibia, usr/NN/<id>.usr,
// dat/owners.dat), as far as seeding needs it.
//
// The token rules are those of TReadScriptFile::nextToken (script.c): '#'
// starts a comment that runs to the end of the line; identifiers start with
// a letter and continue with letters, digits and '_'; numbers are decimal;
// strings are in double quotes, with "\n" for a newline and a backslash
// before any other character standing for that character. Identifiers are
// compared in lower case by the game, so they are returned in lower case.
// Everything else is returned as a one-character special token; the game's
// composite tokens (coordinates in brackets, byte sequences such as
// 3-86-3-86) come out as their parts.

#ifndef QM_SCRIPT_HH
#define QM_SCRIPT_HH

#include <cstdint>
#include <cstdio>
#include <string>

namespace qm {

enum class Token {
	EndOfFile,
	Identifier,
	Number,
	String,
	Special,
};

class ScriptReader {
public:
	// Throws std::runtime_error if the file cannot be opened.
	explicit ScriptReader(const std::string &path);
	~ScriptReader();

	ScriptReader(const ScriptReader &) = delete;
	ScriptReader &operator=(const ScriptReader &) = delete;

	// Reads the next token and returns its kind.
	// Throws std::runtime_error on an unterminated string.
	Token next();

	Token token() const { return Current; }
	// Identifier (lower case) or string contents (file bytes, ISO-8859-1).
	const std::string &text() const { return Text; }
	uint32_t number() const { return Number; }
	char special() const { return Special; }
	int line() const { return Line; }
	const std::string &path() const { return Path; }

	// True if the current token is the given special character.
	bool isSpecial(char c) const { return Current == Token::Special && Special == c; }

private:
	std::string Path;
	FILE *File;
	Token Current;
	std::string Text;
	uint32_t Number;
	char Special;
	int Line;
};

} // namespace qm

#endif
