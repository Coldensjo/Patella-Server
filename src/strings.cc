#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#include "containr.hh"
#include "mudinc.hh"
#include "utils.hh"
#include "strings.hh"

// The text bytes of a static string block, the strings of a dynamic string
// block and its text bytes. A dynamic string's number is its block's index
// times DYNAMIC_STRING_BLOCK_ENTRIES plus its entry plus 1 (0 is no string).
const int STATIC_STRING_BLOCK_SIZE = 65536;
const int DYNAMIC_STRING_BLOCK_ENTRIES = 256;
const int DYNAMIC_STRING_BLOCK_SIZE = 32768;

// TDynamicStringTableBlock::EntryType: a free entry, a string, and a deleted
// string whose text CleanupDynamicStrings removes.
const int DYNAMIC_STRING_FREE = 0;
const int DYNAMIC_STRING_USED = 1;
const int DYNAMIC_STRING_DELETED = 2;

struct TStaticStringTableBlock {
	int TotalTextLength;
	char Text[STATIC_STRING_BLOCK_SIZE];
};

struct TDynamicStringTableBlock {
	int FreeEntries;
	int TotalTextLength;
	bool Dirty;
	unsigned char EntryType[DYNAMIC_STRING_BLOCK_ENTRIES];
	unsigned short StringOffset[DYNAMIC_STRING_BLOCK_ENTRIES];
	char Text[DYNAMIC_STRING_BLOCK_SIZE];
};

list<TStaticStringTableBlock> StaticStringTable;
list<TDynamicStringTableBlock> DynamicStringTable;

const char *AddStaticString(const char *String) {
	int Length = static_cast<int>(strlen(String));
	if (Length >= STATIC_STRING_BLOCK_SIZE) {
		error("AddStaticString: String zu lang (%d).\n", Length);
		return nullptr;
	}

	if (Length == 0) {
		return "";
	}

	listIterator<TStaticStringTableBlock> iter(StaticStringTable.first());
	while (iter() != nullptr) {
		if (STATIC_STRING_BLOCK_SIZE - (*iter)->TotalTextLength > Length + 1) {
			break;
		}
		iter++;
	}

	if (iter() == nullptr) {
		iter = StaticStringTable.append();
		(*iter)->TotalTextLength = 0;
	}

	char *p = (*iter)->Text + (*iter)->TotalTextLength;
	memcpy(p, String, Length + 1);
	(*iter)->TotalTextLength += Length + 1;
	return p;
}

uint32_t AddDynamicString(char *String) {
	int Length = static_cast<int>(strlen(String));
	if (Length >= DYNAMIC_STRING_BLOCK_SIZE) {
		error("AddDynamicString: String zu lang (%d)\n", Length);
		return 0;
	}

	if (Length == 0) {
		return 0;
	}

	listIterator<TDynamicStringTableBlock> iter(DynamicStringTable.first());
	int count = 0;
	while (iter() != nullptr) {
		if ((*iter)->FreeEntries > 0
				&& DYNAMIC_STRING_BLOCK_SIZE - (*iter)->TotalTextLength > Length + 1) {
			break;
		}
		iter++;
		count++;
	}

	if (iter() == nullptr) {
		iter = DynamicStringTable.append();
		TDynamicStringTableBlock *p = *iter;
		p->FreeEntries = DYNAMIC_STRING_BLOCK_ENTRIES;
		p->TotalTextLength = 0;
		p->Dirty = false;
		for (int i = 0; i < DYNAMIC_STRING_BLOCK_ENTRIES; i++) {
			p->EntryType[i] = DYNAMIC_STRING_FREE;
		}
	}

	TDynamicStringTableBlock *p = *iter;
	int pos = -1;
	for (int i = 0; i < DYNAMIC_STRING_BLOCK_ENTRIES; i++) {
		if (p->EntryType[i] == DYNAMIC_STRING_FREE) {
			pos = i;
			break;
		}
	}

	if (pos == -1) {
		error("AddDynamicString: Keinen freien Platz gefunden.\n");
		return 0;
	}

	p->EntryType[pos] = DYNAMIC_STRING_USED;
	p->FreeEntries--;
	p->StringOffset[pos] = static_cast<unsigned short>(p->TotalTextLength);
	memcpy(p->Text + p->TotalTextLength, String, Length + 1);
	p->TotalTextLength += Length + 1;
	return count * DYNAMIC_STRING_BLOCK_ENTRIES + pos + 1;
}

char *GetDynamicString(uint32_t Number) {
	if (Number == 0) {
		return nullptr;
	}

	Number--;
	int Block = Number / DYNAMIC_STRING_BLOCK_ENTRIES;
	int pos = Number % DYNAMIC_STRING_BLOCK_ENTRIES;
	int count = 0;
	listIterator<TDynamicStringTableBlock> iter(DynamicStringTable.first());
	while (count < Block && iter() != nullptr) {
		iter++;
		count++;
	}

	if (iter() == nullptr) {
		error("GetDynamicString: Block f\xfcr String %u existiert nicht\n", Number + 1);
		return nullptr;
	}

	TDynamicStringTableBlock *p = *iter;
	if (p->EntryType[pos] != DYNAMIC_STRING_USED) {
		error("GetDynamicString: Eintrag f\xfcr String %u existiert nicht\n", Number + 1);
		return nullptr;
	}

	return p->Text + p->StringOffset[pos];
}

void DeleteDynamicString(uint32_t Number) {
	if (Number == 0) {
		return;
	}

	Number--;
	int Block = Number / DYNAMIC_STRING_BLOCK_ENTRIES;
	int pos = Number % DYNAMIC_STRING_BLOCK_ENTRIES;
	int count = 0;
	listIterator<TDynamicStringTableBlock> iter(DynamicStringTable.first());
	while (count < Block && iter() != nullptr) {
		iter++;
		count++;
	}

	if (iter() == nullptr) {
		error("DeleteDynamicString: Block f\xfcr String %u existiert nicht\n", Number + 1);
		return;
	}

	TDynamicStringTableBlock *p = *iter;
	if (p->EntryType[pos] != DYNAMIC_STRING_USED) {
		error("DeleteDynamicString: Eintrag f\xfcr String %u existiert nicht\n", Number + 1);
		return;
	}

	p->EntryType[pos] = DYNAMIC_STRING_DELETED;
	p->Dirty = true;
}

void CleanupDynamicStrings() {
	listIterator<TDynamicStringTableBlock> iter(DynamicStringTable.first());
	while (iter() != nullptr) {
		TDynamicStringTableBlock *p = *iter;
		if (p->Dirty) {
			for (int i = 0; i < DYNAMIC_STRING_BLOCK_ENTRIES; i++) {
				if (p->EntryType[i] != DYNAMIC_STRING_DELETED) {
					continue;
				}

				int Offset = p->StringOffset[i];
				int Length = static_cast<int>(strlen(p->Text + Offset)) + 1;
				p->TotalTextLength -= Length;
				p->EntryType[i] = DYNAMIC_STRING_FREE;
				if (Offset + Length > DYNAMIC_STRING_BLOCK_SIZE) {
					error("CleanupDynamicStrings: Stringende fehlt\n");
					continue;
				}

				int Help = DYNAMIC_STRING_BLOCK_SIZE - Offset - Length;
				if (Help > 0) {
					memmove(p->Text + Offset, p->Text + Offset + Length, Help);
				}
				p->FreeEntries++;
				for (int j = 0; j < DYNAMIC_STRING_BLOCK_ENTRIES; j++) {
					if (p->EntryType[j] != DYNAMIC_STRING_FREE && p->StringOffset[j] > Offset) {
						p->StringOffset[j] -= static_cast<unsigned short>(Length);
					}
				}
			}
			p->Dirty = false;
		}
		iter++;
	}
}

bool IsCountable(const char *s) {
	if (strncmp(s, "some ", 5) == 0) {
		return false;
	}

	return strncmp(s, "a ", 2) == 0 || strncmp(s, "an ", 3) == 0;
}

char *Plural(const char *s, int Count) {
	static char ObjectNameString[50];
	char Help[50];
	char Last, Prev;
	char *p;

	ObjectNameString[0] = 0;
	if (s == nullptr) {
		error("Plural: \xdc" "bergebener String existiert nicht.\n");
		return ObjectNameString;
	}

	strcpy(ObjectNameString, s);
	if (Count == 1) {
		return ObjectNameString;
	}

	if (!IsCountable(ObjectNameString)) {
		return ObjectNameString;
	}

	if (ObjectNameString[1] == 'n') {
		memmove(ObjectNameString, ObjectNameString + 3, 47);
	} else {
		memmove(ObjectNameString, ObjectNameString + 2, 48);
	}

	if (Count < 0) {
		return ObjectNameString;
	}

	p = strstr(ObjectNameString, " of ");
	if (p != nullptr) {
		strcpy(Help, p);
		*p = 0;
	} else {
		Help[0] = 0;
	}

	if (strcmp(ObjectNameString, "knife") == 0) {
		strcpy(ObjectNameString, "knives");
		goto cont;
	}

	if (strcmp(ObjectNameString, "throwing knife") == 0) {
		strcpy(ObjectNameString, "throwing knives");
		goto cont;
	}

	if (strcmp(ObjectNameString, "orc spearman") == 0) {
		strcpy(ObjectNameString, "orc spearmen");
		goto cont;
	}

	if (strcmp(ObjectNameString, "deer") == 0
			|| strcmp(ObjectNameString, "dead deer") == 0
			|| strcmp(ObjectNameString, "sheep") == 0
			|| strcmp(ObjectNameString, "black sheep") == 0
			|| strcmp(ObjectNameString, "fish") == 0) {
		goto cont;
	}

	if (strlen(ObjectNameString) < 2) {
		goto cont;
	}

	Last = static_cast<char>(toLower(ObjectNameString[strlen(ObjectNameString) - 1]));
	Prev = static_cast<char>(toLower(ObjectNameString[strlen(ObjectNameString) - 2]));
	if (Last == 's' || Last == 'x'
			|| (Last == 'h' && (Prev == 's' || Prev == 'c'))) {
		strcat(ObjectNameString, "es");
	} else if (Last == 'y' && Prev != 'a' && Prev != 'e' && Prev != 'i'
			&& Prev != 'o' && Prev != 'u') {
		ObjectNameString[strlen(ObjectNameString) - 1] = 'i';
		strcat(ObjectNameString, "es");
	} else if (Last == 'f' && Prev == 'l') {
		ObjectNameString[strlen(ObjectNameString) - 1] = 'v';
		strcat(ObjectNameString, "es");
	} else if (Last == 'o') {
		strcat(ObjectNameString, "es");
	} else {
		strcat(ObjectNameString, "s");
	}

cont:
	strcat(ObjectNameString, Help);
	strcpy(Help, ObjectNameString);
	if (Count != 0) {
		snprintf(ObjectNameString, 50, "%d %s", Count, Help);
	}

	return ObjectNameString;
}

char *SearchForWord(char *Pattern, char *Text) {
	if (Pattern == nullptr || Pattern[0] == 0) {
		error("SearchForWord: \xdc" "bergebenes Suchwort existiert nicht.\n");
		return nullptr;
	}

	if (Text == nullptr) {
		error("SearchForWord: \xdc" "bergebener Text existiert nicht.\n");
		return nullptr;
	}

	bool start = true;
	unsigned int PatternLength = static_cast<unsigned int>(strlen(Pattern));
	bool WholeWord = false;
	if (Pattern[PatternLength - 1] == '$') {
		PatternLength--;
		WholeWord = true;
	}

	for (char *c = Text; *c != 0; c++) {
		if (isSpace(*c)) {
			start = true;
			continue;
		}

		if (!start) {
			continue;
		}

		for (unsigned int count = 0; count <= PatternLength; count++) {
			if (count == PatternLength) {
				if (!WholeWord || (!isAlpha(c[count]) && !isDigit(c[count]))) {
					return c;
				}
				start = false;
				break;
			}

			if (toLower(c[count]) != toLower(Pattern[count])) {
				start = false;
				break;
			}
		}
	}

	return nullptr;
}

char *SearchForNumber(int Count, char *Text) {
	if (Count < 1) {
		error("SearchForNumber: Illegale Suchnummer %d.\n", Count);
		return nullptr;
	}

	if (Text == nullptr) {
		error("SearchForNumber: \xdc" "bergebener Text existiert nicht.\n");
		return nullptr;
	}

	bool start = true;
	for (char *c = Text; *c != 0; c++) {
		if (isSpace(*c)) {
			start = true;
			continue;
		}

		if (!start) {
			continue;
		}

		start = false;
		if (!isDigit(*c)) {
			continue;
		}

		int ofs = 0;
		do {
			ofs++;
			if (c[ofs] == 0 || isSpace(c[ofs])) {
				Count--;
				if (Count == 0) {
					return c;
				}
				break;
			}
		} while (isDigit(c[ofs]));
	}

	return nullptr;
}

bool MatchString(char *Pattern, char *String) {
	if (Pattern == nullptr) {
		error("MatchString: Pattern ist NULL.\n");
		return false;
	}

	if (String == nullptr) {
		error("MatchString: String ist NULL.\n");
		return false;
	}

	int sl = static_cast<int>(strlen(String));
	int pl = static_cast<int>(strlen(Pattern));
	int d[sl + 1][pl + 1];
	for (int i = 0; i <= sl; i++) {
		d[i][0] = i;
	}

	for (int j = 1; j <= pl; j++) {
		char c = Pattern[j - 1];
		int edit = 1, del = 1, ins = 1;
		if (c == '*') {
			edit = 0;
			del = 0;
			ins = 0;
		}
		if (c == '?') {
			edit = 0;
		}

		d[0][j] = d[0][j - 1] + del;
		for (int i = 1; i <= sl; i++) {
			int edit2 = (toLower(String[i - 1]) == toLower(Pattern[j - 1])) ? 0 : edit;
			d[i][j] = std::min(std::min(d[i - 1][j] + ins, d[i][j - 1] + del),
					d[i - 1][j - 1] + edit2);
		}
	}

	return d[sl][pl] == 0;
}

void AddSlashes(char *Destination, const char *Source) {
	if (Source == nullptr) {
		error("AddSlashes: Source ist NULL.\n");
		return;
	}

	if (Destination == nullptr) {
		error("AddSlashes: Destination ist NULL.\n");
		return;
	}

	while (*Source != 0) {
		if (*Source == '"' || *Source == '#' || *Source == '\'' || *Source == '\\') {
			*Destination = '\\';
			Destination++;
		}
		*Destination = *Source;
		Destination++;
		Source++;
	}
	*Destination = 0;
}

void Trim(char *Text) {
	if (Text == nullptr) {
		error("Trim: Text ist NULL.\n");
		return;
	}

	int i = 0;
	while (Text[i] != 0 && isSpace(Text[i])) {
		i++;
	}

	// The length is taken from Text + 1, as in the original: an empty Text is
	// measured behind its terminator, and with leading spaces up to i - 1
	// bytes behind the terminator are moved along.
	memmove(Text, Text + i, strlen(Text + 1) + 1);

	i = static_cast<int>(strlen(Text));
	while (i > 0 && isSpace(Text[i - 1])) {
		Text[i - 1] = 0;
		i--;
	}
}

char *Capitals(char *Text) {
	bool start = true;
	for (int i = 0; i < static_cast<int>(strlen(Text)); i++) {
		if (isAlpha(Text[i])) {
			if (start) {
				Text[i] = static_cast<char>(toUpper(Text[i]));
			}
			start = false;
		} else {
			start = true;
		}
	}

	return Text;
}
