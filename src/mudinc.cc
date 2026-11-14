// mudinc.cc: installing a signal handler, and helpers for Latin-1 text:
// character classes, case, comparison without case, and the last occurrence
// of a character.

#include <csignal>
#include <cstring>

#include "mudinc.hh"

void (*handler(int signr, void (*sighandler)(int)))(int) {
	struct sigaction neu_handler, alt_handler;

	neu_handler.sa_handler = sighandler;
	sigemptyset(&neu_handler.sa_mask);
	neu_handler.sa_flags = 0;
	if (signr == SIGALRM) {
		neu_handler.sa_flags |= SA_INTERRUPT;
	} else {
		neu_handler.sa_flags |= SA_RESTART;
	}
	if (sigaction(signr, &neu_handler, &alt_handler) < 0) {
		return SIG_ERR;
	}
	return alt_handler.sa_handler;
}

bool isSpace(int c) {
	return c == ' ' || c == '\f' || c == '\n' || c == '\r' || c == '\t' || c == '\v';
}

// Quirk: the umlaut literals are plain (signed) char, so they are negative:
// a sign-extended byte matches, the same byte as an int in 128..255 does not.
bool isAlpha(int c) {
	return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z')
		|| c == '\xe4' || c == '\xf6' || c == '\xfc'
		|| c == '\xc4' || c == '\xd6' || c == '\xdc' || c == '\xdf';
}

bool isDigit(int c) {
	return c >= '0' && c <= '9';
}

// The Latin-1 range is tested on the low byte only. Quirk: it includes 0xD7
// (the multiplication sign), which becomes 0xF7 (the division sign).
int toLower(int c) {
	if ((c >= 'A' && c <= 'Z')
			|| (static_cast<unsigned char>(c) >= 0xC0 && static_cast<unsigned char>(c) <= 0xDE)) {
		return c + 32;
	}
	return c;
}

// The Latin-1 range is tested on the low byte only. Quirk: it includes 0xF7
// (the division sign), which becomes 0xD7 (the multiplication sign).
int toUpper(int c) {
	if ((c >= 'a' && c <= 'z')
			|| (static_cast<unsigned char>(c) >= 0xE0 && static_cast<unsigned char>(c) <= 0xFE)) {
		return c - 32;
	}
	return c;
}

char *strLower(char *s) {
	for (int i = 0; s[i] != 0; i++) {
		s[i] = static_cast<char>(toLower(s[i]));
	}
	return s;
}

char *strUpper(char *s) {
	for (int i = 0; s[i] != 0; i++) {
		s[i] = static_cast<char>(toUpper(s[i]));
	}
	return s;
}

int stricmp(char *s1, char *s2, int Pos) {
	for (int i = 0; i != Pos; i++) {
		if (toLower(s1[i]) > toLower(s2[i])) {
			return 1;
		}
		if (toLower(s1[i]) < toLower(s2[i])) {
			return -1;
		}
		if (s1[i] == 0) {
			return 0;
		}
	}
	return 0;
}

char *findLast(char *s, char c) {
	char *p = nullptr;
	while ((s = strchr(s, c)) != nullptr) {
		p = s;
		s++;
	}
	return p;
}
