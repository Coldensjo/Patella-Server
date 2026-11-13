#include <cstdint>
#include <cstdlib>

#include "containr.hh"
#include "utils.hh"
#include "strings.hh"

struct TStaticStringTableBlock {
	int TotalTextLength;
	char Text[65536];
};

struct TDynamicStringTableBlock {
	int FreeEntries;
	int TotalTextLength;
	bool Dirty;
	unsigned char EntryType[256];
	unsigned short StringOffset[256];
	char Text[32768];
};

list<TStaticStringTableBlock> StaticStringTable;	// strings.c:33
list<TDynamicStringTableBlock> DynamicStringTable;	// strings.c:34

// 0x08051900  strings.c:46
const char *AddStaticString(const char * /*String*/) { abort(); /* STUB */ }

// 0x08051a20  strings.c:77
uint32_t AddDynamicString(char * /*String*/) { abort(); /* STUB */ }

// 0x08051be0  strings.c:124
char *GetDynamicString(uint32_t /*Number*/) { abort(); /* STUB */ }

// 0x08051c80  strings.c:151
void DeleteDynamicString(uint32_t /*Number*/) { abort(); /* STUB */ }

// 0x08051d10  strings.c:182
void CleanupDynamicStrings() { abort(); /* STUB */ }

// 0x08051e30  strings.c:216
bool IsCountable(const char * /*s*/) { abort(); /* STUB */ }

// 0x08051ea0  strings.c:234
// local static: char ObjectNameString[50]  (strings.c:235, 0x08128bc0 .bss)
char *Plural(const char * /*s*/, int /*Count*/) { abort(); /* STUB */ }

// 0x08052330  strings.c:314
char *SearchForWord(char * /*Pattern*/, char * /*Text*/) { abort(); /* STUB */ }

// 0x08052450  strings.c:350
char *SearchForNumber(int /*Count*/, char * /*Text*/) { abort(); /* STUB */ }

// 0x08052520  strings.c:386
bool MatchString(char * /*Pattern*/, char * /*String*/) { abort(); /* STUB */ }

// 0x08052730  strings.c:420
void AddSlashes(char * /*Destination*/, const char * /*Source*/) { abort(); /* STUB */ }

// 0x08052790  strings.c:446
void Trim(char * /*Text*/) { abort(); /* STUB */ }

// 0x08052840  strings.c:467
void Trim(char * /*Destination*/, char * /*Source*/) { abort(); /* STUB */ }

// 0x080528f0  strings.c:491
void ChangeWildcards(char * /*String*/) { abort(); /* STUB */ }

// 0x08052960  strings.c:506
char *Capitals(char * /*Text*/) { abort(); /* STUB */ }

// 0x080529d0  strings.c:526
void InitStrings() { abort(); /* STUB */ }

// 0x080529e0  strings.c:534
void ExitStrings() { abort(); /* STUB */ }
