#ifndef PATELLA_MUDINC_HH
#define PATELLA_MUDINC_HH

void (*handler(int signr, void (*sighandler)(int)))(int);
bool isSpace(int c);
bool isAlpha(int c);
bool isDigit(int c);
int toLower(int c);
int toUpper(int c);
char *strLower(char *s);
char *strUpper(char *s);
int stricmp(char *s1, char *s2, int Pos);
char *findLast(char *s, char c);

#endif
