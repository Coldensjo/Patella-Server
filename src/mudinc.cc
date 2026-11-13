#include <cstdlib>

#include "mudinc.hh"

// 0x0804cc80  mudinc.c:22
void (*handler(int /*signr*/, void (* /*sighandler*/)(int)))(int) { abort(); /* STUB */ }

// 0x0804ccf0  mudinc.c:50
bool isSpace(int /*c*/) { abort(); /* STUB */ }

// 0x0804cd30  mudinc.c:58
bool isAlpha(int /*c*/) { abort(); /* STUB */ }

// 0x0804cd80  mudinc.c:68
bool isEngAlpha(int /*c*/) { abort(); /* STUB */ }

// 0x0804cdb0  mudinc.c:76
bool isDigit(int /*c*/) { abort(); /* STUB */ }

// 0x0804cdd0  mudinc.c:84
int toLower(int /*c*/) { abort(); /* STUB */ }

// 0x0804cdf0  mudinc.c:93
int toUpper(int /*c*/) { abort(); /* STUB */ }

// 0x0804ce10  mudinc.c:103
char *strLower(char * /*s*/) { abort(); /* STUB */ }

// 0x0804ce60  mudinc.c:117
char *strUpper(char * /*s*/) { abort(); /* STUB */ }

// 0x0804ceb0  mudinc.c:132
int stricmp(char * /*s1*/, char * /*s2*/, int /*Pos*/) { abort(); /* STUB */ }

// 0x0804cf60  mudinc.c:148
char *findFirst(char * /*s*/, char /*c*/) { abort(); /* STUB */ }

// 0x0804cf80  mudinc.c:157
char *findLast(char * /*s*/, char /*c*/) { abort(); /* STUB */ }
