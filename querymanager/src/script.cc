// script.cc: the tokenizer for the game's text data files (script.hh).

#include "script.hh"

#include <stdexcept>

namespace qm {

namespace {

bool isDigit(int c) {
	return c >= '0' && c <= '9';
}

// isAlpha (utils.c) also lists the German umlauts, but as negative values,
// which getc never returns; so only ASCII letters count.
bool isAlpha(int c) {
	return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z');
}

bool isSpace(int c) {
	return c == ' ' || c == '\t' || c == '\r' || c == '\n' || c == '\f' || c == '\v';
}

} // namespace

ScriptReader::ScriptReader(const std::string &path)
	: Path(path), File(std::fopen(path.c_str(), "rb")), Current(Token::EndOfFile), Number(0),
	Special(0), Line(1) {
	if (File == nullptr) {
		throw std::runtime_error("cannot open " + path);
	}
}

ScriptReader::~ScriptReader() {
	if (File != nullptr) {
		std::fclose(File);
	}
}

Token ScriptReader::next() {
	Text.clear();
	Number = 0;
	Special = 0;

	int c;
	while (true) {
		c = std::getc(File);
		if (c == EOF) {
			Current = Token::EndOfFile;
			return Current;
		}
		if (c == '\n') {
			Line++;
			continue;
		}
		if (isSpace(c)) {
			continue;
		}
		if (c == '#') {
			while (c != EOF && c != '\n') {
				c = std::getc(File);
			}
			if (c == '\n') {
				Line++;
			}
			continue;
		}
		break;
	}

	if (isAlpha(c)) {
		while (c != EOF && (isAlpha(c) || isDigit(c) || c == '_')) {
			if (c >= 'A' && c <= 'Z') {
				c += 'a' - 'A';
			}
			Text.push_back(static_cast<char>(c));
			c = std::getc(File);
		}
		if (c != EOF) {
			std::ungetc(c, File);
		}
		Current = Token::Identifier;
		return Current;
	}

	if (isDigit(c)) {
		uint64_t value = 0;
		while (c != EOF && isDigit(c)) {
			value = value * 10 + static_cast<uint64_t>(c - '0');
			if (value > 0xFFFFFFFFull) {
				value = 0xFFFFFFFFull;
			}
			c = std::getc(File);
		}
		if (c != EOF) {
			std::ungetc(c, File);
		}
		Number = static_cast<uint32_t>(value);
		Current = Token::Number;
		return Current;
	}

	if (c == '"') {
		while (true) {
			c = std::getc(File);
			if (c == EOF) {
				throw std::runtime_error(Path + ": unexpected end of file in a string");
			}
			if (c == '"') {
				break;
			}
			if (c == '\\') {
				c = std::getc(File);
				if (c == EOF) {
					throw std::runtime_error(Path + ": unexpected end of file in a string");
				}
				Text.push_back(c == 'n' ? '\n' : static_cast<char>(c));
				continue;
			}
			if (c == '\n') {
				Line++;
			}
			Text.push_back(static_cast<char>(c));
		}
		Current = Token::String;
		return Current;
	}

	Special = static_cast<char>(c);
	Current = Token::Special;
	return Current;
}

} // namespace qm
