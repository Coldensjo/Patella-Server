#ifndef PATELLA_STRINGS_HH
#define PATELLA_STRINGS_HH

#include <cstdint>

#include "containr.hh"

struct TDynamicStringTableBlock;
struct TStaticStringTableBlock;

const char *AddStaticString(const char *String);
uint32_t AddDynamicString(char *String);
char *GetDynamicString(uint32_t Number);
void DeleteDynamicString(uint32_t Number);
void CleanupDynamicStrings();
bool IsCountable(const char *s);
char *Plural(const char *s, int Count);
char *SearchForWord(char *Pattern, char *Text);
char *SearchForNumber(int Count, char *Text);
bool MatchString(char *Pattern, char *String);
void AddSlashes(char *Destination, const char *Source);
void Trim(char *Text);
char *Capitals(char *Text);
void InitStrings();
void ExitStrings();

extern list<TStaticStringTableBlock> StaticStringTable;
extern list<TDynamicStringTableBlock> DynamicStringTable;

#endif
