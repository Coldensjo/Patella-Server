#include <cerrno>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#include "script.hh"
#include "mudinc.hh"
#include "utils.hh"

// The original formats file names of up to 4095 characters into ErrorString:
// unbounded with sprintf in the binary-file classes, truncating with snprintf
// in the script-file classes.

static char ErrorString[100];	// script.c:14

// 0x0804ea20  script.c:21
void SaveFile(char *Filename) {
	if (Filename == nullptr) {
		error("SaveFile: Filename ist NULL.\n");
		return;
	}

	char BackupFilename[4096];
	strcpy(BackupFilename, Filename);
	strcat(BackupFilename, "#");

	FILE *Source = fopen(Filename, "rb");
	if (Source == nullptr) {
		error("SaveFile: Quelldatei %s existiert nicht.\n", Filename);
		return;
	}

	FILE *Dest = fopen(BackupFilename, "wb");
	if (Dest == nullptr) {
		error("SaveFile: Kann Zieldatei %s nicht anlegen.\n", BackupFilename);
		return;
	}

	fseek(Source, 0, SEEK_END);
	size_t Size = ftell(Source);
	fseek(Source, 0, SEEK_SET);

	unsigned char Buffer[Size];
	if (fread(Buffer, 1, Size, Source) != Size) {
		error("SaveFile: Fehler beim Lesen der Quelldatei %s.\n", Filename);
	} else if (fwrite(Buffer, 1, Size, Dest) != Size) {
		error("SaveFile: Fehler beim Schreiben der Zieldatei %s.\n", Filename);
	}

	if (fclose(Source) != 0) {
		error("SaveFile: Fehler %d beim Schlie\xdf" "en der Quelldatei.\n", errno);
	}
	if (fclose(Dest) != 0) {
		error("SaveFile: Fehler %d beim Schlie\xdf" "en der Zieldatei.\n", errno);
	}
}

// 0x0804ec30  script.c:63
TReadScriptFile::TReadScriptFile() {
	RecursionDepth = -1;
	Bytes = (unsigned char *)String;
}

// 0x0804ecc0  script.c:72
TReadScriptFile::~TReadScriptFile() {
	if (RecursionDepth != -1) {
		::error("TReadScriptFile::~TReadScriptFile: Datei ist noch offen.\n");
		while (RecursionDepth != -1) {
			close();
		}
	}
}

// 0x0804ed30  script.c:83
void TReadScriptFile::open(char *FileName) {
	RecursionDepth++;
	if (RecursionDepth == MAXIMUMRECURSIONDEPTH) {
		::error("TReadScriptFile::open: Rekursionstiefe zu gro\xdf.\n");
		throw "Recursion depth too high";
	}

	if (RecursionDepth > 0 && FileName[0] != '/') {
		strcpy(Filename[RecursionDepth], Filename[RecursionDepth - 1]);
		char *p = findLast(Filename[RecursionDepth], '/');
		if (p != nullptr) {
			strcpy(p + 1, FileName);
		} else {
			strcpy(Filename[RecursionDepth], FileName);
		}
	} else {
		strcpy(Filename[RecursionDepth], FileName);
	}

	File[RecursionDepth] = fopen(Filename[RecursionDepth], "rb");
	if (File[RecursionDepth] == nullptr) {
		int ErrorCode = errno;
		::error("TReadScriptFile::open: Kann Datei %s nicht \xf6" "ffnen.\n", Filename[RecursionDepth]);
		::error("Fehler %d: %s.\n", ErrorCode, strerror(ErrorCode));
		RecursionDepth--;
		throw "Cannot open script-file";
	}

	Line[RecursionDepth] = 1;
}

// 0x0804eec0  script.c:110
void TReadScriptFile::close() {
	if (RecursionDepth == -1) {
		::error("TReadScriptFile::close: Keine Datei offen.\n");
		return;
	}

	if (fclose(File[RecursionDepth]) != 0) {
		::error("TReadScriptFile::close: Fehler %d beim Schlie\xdf" "en der Datei.\n", errno);
	}
	RecursionDepth--;
}

// 0x0804ef30  script.c:123
void TReadScriptFile::error(char *Text) {
	char *p = findLast(Filename[RecursionDepth], '/');
	if (p != nullptr) {
		p++;
	} else {
		p = Filename[RecursionDepth];
	}

	snprintf(ErrorString, sizeof(ErrorString), "error in script-file \"%s\", line %d: %s", p, Line[RecursionDepth], Text);

	while (RecursionDepth != -1) {
		close();
	}

	throw ErrorString;
}

// 0x0804f040  script.c:137
void TReadScriptFile::nextToken() {
	if (RecursionDepth == -1) {
		::error("TReadScriptFile::nextToken: Kein Skript zum Lesen ge\xf6" "ffnet.\n");
		Token = ENDOFFILE;
		return;
	}

	int State, pos;
	int c;
	FILE *f;
	int Sign;

again:
	State = 0;
	pos = 0;
	memset(String, 0, sizeof(String));
	Number = 0;
	f = File[RecursionDepth];
	Sign = 1;

	while (true) {
		if (pos == MAXIMUMSTRINGLENGTH - 1) {
			error("string too long");
		}

		switch (State) {
		case 0:
			c = getc(f);
			if (c == EOF) {
				if (RecursionDepth <= 0) {
					Token = ENDOFFILE;
					return;
				}
				close();
				goto again;
			}
			if (c == '\n') {
				Line[RecursionDepth]++;
			} else if (!isSpace(c)) {
				if (c == '#') {
					State = 1;
				} else if (c == '@') {
					State = 30;
				} else if (isAlpha(c)) {
					State = 2;
					String[pos++] = (char)c;
				} else if (isDigit(c)) {
					Number = c - '0';
					State = 3;
				} else if (c == '"') {
					State = 6;
				} else if (c == '[') {
					State = 11;
				} else if (c == '<') {
					State = 22;
				} else if (c == '>') {
					State = 25;
				} else if (c == '-') {
					State = 27;
				} else {
					State = 10;
					Special = (char)c;
				}
			}
			break;

		case 1:
			c = getc(f);
			if (c == EOF) {
				if (RecursionDepth <= 0) {
					Token = ENDOFFILE;
					return;
				}
				close();
				goto again;
			}
			if (c == '\n') {
				Line[RecursionDepth]++;
				State = 0;
			}
			break;

		case 2:
			c = getc(f);
			if (pos == MAXIMUMIDENTIFIERLENGTH) {
				error("identifier too long");
			}
			if (c == EOF) {
				Token = IDENTIFIER;
				return;
			}
			if (isAlpha(c) || isDigit(c) || c == '_') {
				String[pos++] = (char)c;
			} else {
				ungetc(c, f);
				Token = IDENTIFIER;
				return;
			}
			break;

		case 3:
			c = getc(f);
			if (c == EOF) {
				Token = NUMBER;
				return;
			}
			if (isDigit(c)) {
				// unsigned: an overlong number wraps around as in the original
				Number = (int)((unsigned int)Number * 10 + (c - '0'));
			} else if (c == '-') {
				Bytes[pos++] = (unsigned char)Number;
				State = 4;
			} else {
				ungetc(c, f);
				Token = NUMBER;
				return;
			}
			break;

		case 4:
			c = getc(f);
			if (c == EOF) {
				error("unexpected end of file");
			}
			if (isDigit(c)) {
				Number = c - '0';
				State = 5;
			} else {
				error("syntax error");
			}
			break;

		case 5:
			c = getc(f);
			if (c == EOF) {
				Bytes[pos] = (unsigned char)Number;
				Token = BYTES;
				return;
			}
			if (isDigit(c)) {
				Number = (int)((unsigned int)Number * 10 + (c - '0'));
			} else if (c == '-') {
				Bytes[pos++] = (unsigned char)Number;
				State = 4;
			} else {
				ungetc(c, f);
				Bytes[pos] = (unsigned char)Number;
				Token = BYTES;
				return;
			}
			break;

		case 6:
			c = getc(f);
			if (c == EOF) {
				error("unexpected end of file");
			}
			if (c == '"') {
				State = 8;
			} else if (c == '\\') {
				State = 7;
			} else {
				if (c == '\n') {
					Line[RecursionDepth]++;
				}
				String[pos++] = (char)c;
			}
			break;

		case 7:
			c = getc(f);
			if (c == EOF) {
				error("unexpected end of file");
			}
			if (c == 'n') {
				String[pos++] = '\n';
			} else {
				String[pos++] = (char)c;
			}
			State = 6;
			break;

		case 8:
			Token = STRING;
			return;

		case 10:
			Token = SPECIAL;
			return;

		case 11:
			c = getc(f);
			Special = '[';
			if (c == EOF) {
				Token = SPECIAL;
				return;
			}
			if (isDigit(c)) {
				Sign = 1;
				Number = c - '0';
				State = 12;
			} else if (c == '-') {
				Sign = -1;
				Number = 0;
				State = 12;
			} else {
				ungetc(c, f);
				Token = SPECIAL;
				return;
			}
			break;

		case 12:
			c = getc(f);
			if (c == EOF) {
				error("unexpected end of file");
			}
			if (isDigit(c)) {
				Number = (int)((unsigned int)Number * 10 + (c - '0'));
			} else if (c == ',') {
				CoordX = (int)((unsigned int)Sign * (unsigned int)Number);
				State = 13;
			} else {
				error("syntax error");
			}
			break;

		case 13:
			c = getc(f);
			if (c == EOF) {
				error("unexpected end of file");
			}
			if (isDigit(c)) {
				Sign = 1;
				Number = c - '0';
				State = 14;
			} else if (c == '-') {
				Sign = -1;
				Number = 0;
				State = 14;
			} else {
				error("syntax error");
			}
			break;

		case 14:
			c = getc(f);
			if (c == EOF) {
				error("unexpected end of file");
			}
			if (isDigit(c)) {
				Number = (int)((unsigned int)Number * 10 + (c - '0'));
			} else if (c == ',') {
				CoordY = (int)((unsigned int)Sign * (unsigned int)Number);
				State = 15;
			} else {
				error("syntax error");
			}
			break;

		case 15:
			c = getc(f);
			if (c == EOF) {
				error("unexpected end of file");
			}
			if (isDigit(c)) {
				Sign = 1;
				Number = c - '0';
				State = 16;
			} else if (c == '-') {
				Sign = -1;
				Number = 0;
				State = 16;
			} else {
				error("syntax error");
			}
			break;

		case 16:
			c = getc(f);
			if (c == EOF) {
				error("unexpected end of file");
			}
			if (isDigit(c)) {
				Number = (int)((unsigned int)Number * 10 + (c - '0'));
			} else if (c == ']') {
				CoordZ = (int)((unsigned int)Sign * (unsigned int)Number);
				State = 17;
			} else {
				error("syntax error");
			}
			break;

		case 17:
			Token = COORDINATE;
			return;

		case 22:
			c = getc(f);
			Special = '<';
			if (c == EOF) {
				Token = SPECIAL;
				return;
			}
			if (c == '=') {
				State = 23;
			} else if (c == '>') {
				State = 24;
			} else {
				ungetc(c, f);
				Token = SPECIAL;
				return;
			}
			break;

		case 23:
			Special = 'L';
			Token = SPECIAL;
			return;

		case 24:
			Special = 'N';
			Token = SPECIAL;
			return;

		case 25:
			c = getc(f);
			Special = '>';
			if (c == EOF) {
				Token = SPECIAL;
				return;
			}
			if (c == '=') {
				State = 26;
			} else {
				ungetc(c, f);
				Token = SPECIAL;
				return;
			}
			break;

		case 26:
			Special = 'G';
			Token = SPECIAL;
			return;

		case 27:
			c = getc(f);
			Special = '-';
			if (c == EOF) {
				Token = SPECIAL;
				return;
			}
			if (c == '>') {
				State = 28;
			} else {
				ungetc(c, f);
				Token = SPECIAL;
				return;
			}
			break;

		case 28:
			Special = 'I';
			Token = SPECIAL;
			return;

		case 30:
			c = getc(f);
			if (c == EOF) {
				error("unexpected end of file");
			}
			if (c == '"') {
				State = 31;
			} else {
				error("syntax error");
			}
			break;

		case 31:
			c = getc(f);
			if (c == EOF) {
				error("unexpected end of file");
			}
			if (c == '"') {
				State = 32;
			} else {
				String[pos++] = (char)c;
			}
			break;

		case 32:
			open(String);
			goto again;

		default:
			::error("TReadScriptFile::nextToken: Ung\xfcltiger Zustand.\n");
			State = 0;
			break;
		}
	}
}

// 0x0804f950  script.c:311
char *TReadScriptFile::readIdentifier() {
	nextToken();
	if (Token != IDENTIFIER) {
		error("identifier expected");
	}
	return getIdentifier();
}

// 0x0804f9d0  script.c:322
int TReadScriptFile::readNumber() {
	int sgn = 1;
	nextToken();
	if (Token == SPECIAL && getSpecial() == '-') {
		sgn = -1;
		nextToken();
	}
	if (Token != NUMBER) {
		error("number expected");
	}
	return (int)((unsigned int)sgn * (unsigned int)getNumber());
}

// 0x0804fa80  script.c:338
char *TReadScriptFile::readString() {
	nextToken();
	if (Token != STRING) {
		error("string expected");
	}
	return getString();
}

// 0x0804faf0  script.c:349
unsigned char *TReadScriptFile::readBytesequence() {
	nextToken();
	if (Token != BYTES) {
		error("byte-sequence expected");
	}
	return getBytesequence();
}

// 0x0804fb60  script.c:360
void TReadScriptFile::readCoordinate(int &x, int &y, int &z) {
	nextToken();
	if (Token != COORDINATE) {
		error("coordinates expected");
	}
	getCoordinate(x, y, z);
}

// 0x0804fbf0  script.c:371
char TReadScriptFile::readSpecial() {
	nextToken();
	if (Token != SPECIAL) {
		error("special-char expected");
	}
	return getSpecial();
}

// 0x0804fc60  script.c:382
void TReadScriptFile::readSymbol(char Symbol) {
	nextToken();
	if (Token != SPECIAL) {
		error("special-char expected");
	}
	if (getSpecial() != Symbol) {
		error("special-char expected");
	}
}

// 0x0804fd00  script.c:392
char *TReadScriptFile::getIdentifier() {
	if (Token != IDENTIFIER) {
		error("identifier expected");
	}
	strLower(String);
	return String;
}

// 0x0804fd50  script.c:402
int TReadScriptFile::getNumber() {
	if (Token != NUMBER) {
		error("number expected");
	}
	return Number;
}

// 0x0804fda0  script.c:411
char *TReadScriptFile::getString() {
	if (Token != STRING) {
		error("string expected");
	}
	return String;
}

// 0x0804fdf0  script.c:420
unsigned char *TReadScriptFile::getBytesequence() {
	if (Token != BYTES) {
		error("byte-sequence expected");
	}
	return Bytes;
}

// 0x0804fe40  script.c:429
void TReadScriptFile::getCoordinate(int &x, int &y, int &z) {
	if (Token != COORDINATE) {
		error("coordinates expected");
	}
	x = CoordX;
	y = CoordY;
	z = CoordZ;
}

// 0x0804feb0  script.c:440
char TReadScriptFile::getSpecial() {
	if (Token != SPECIAL) {
		error("special-char expected");
	}
	return Special;
}

// 0x0804ff10  script.c:454
TWriteScriptFile::TWriteScriptFile() {
	File = nullptr;
}

// 0x0804ff90  script.c:462
TWriteScriptFile::~TWriteScriptFile() {
	if (File != nullptr) {
		::error("TWriteScriptFile::~TWriteScriptFile: Datei %s ist noch offen.\n", Filename);
		if (fclose(File) != 0) {
			::error("TWriteScriptFile::~TWriteScriptFile: Fehler %d beim Schlie\xdf" "en der Datei.\n", errno);
		}
	}
}

// 0x08050000  script.c:473
void TWriteScriptFile::open(char *FileName) {
	if (File != nullptr) {
		::error("TWriteScriptFile::open: Altes Skript ist noch offen.\n");
		if (fclose(File) != 0) {
			::error("TWriteScriptFile::open: Fehler %d beim Schlie\xdf" "en der Datei.\n", errno);
		}
		File = nullptr;
	}

	File = fopen(FileName, "wb");
	if (File == nullptr) {
		int ErrorCode = errno;
		::error("TWriteScriptFile: Kann Datei %s nicht anlegen.\n", FileName);
		::error("Fehler %d: %s.\n", ErrorCode, strerror(ErrorCode));
		throw "Cannot create script-file";
	}

	Line = 0;
	strcpy(Filename, FileName);
}

// 0x08050110  script.c:494
void TWriteScriptFile::close() {
	if (File == nullptr) {
		::error("TWriteScriptFile::close: Kein Skript offen.\n");
		return;
	}

	if (fclose(File) != 0) {
		::error("TWriteScriptFile::close: Fehler %d beim Schlie\xdf" "en der Datei.\n", errno);
	}
	File = nullptr;
}

// 0x08050180  script.c:507
void TWriteScriptFile::error(char *Text) {
	if (File != nullptr) {
		if (fclose(File) != 0) {
			::error("TWriteScriptFile::error: Fehler %d beim Schlie\xdf" "en der Datei.\n", errno);
		}
	}
	File = nullptr;

	snprintf(ErrorString, sizeof(ErrorString), "error in script-file \"%s\", line %d: %s", Filename, Line, Text);
	throw ErrorString;
}

// 0x08050240  script.c:520
void TWriteScriptFile::writeLn() {
	if (File == nullptr) {
		::error("TWriteScriptFile::writeLn: Kein Skript zum Schreiben ge\xf6" "ffnet.\n");
		throw "Cannot write linefeed";
	}

	putc('\n', File);
}

// 0x080502b0  script.c:532
void TWriteScriptFile::writeText(char *Text) {
	if (File == nullptr) {
		::error("TWriteScriptFile::writeText: Kein Skript zum Schreiben ge\xf6" "ffnet.\n");
		throw "Cannot write text";
	}

	if (Text == nullptr) {
		::error("TWriteScriptFile::writeText: Text ist NULL.\n");
		throw "Cannot write text";
	}

	while (*Text != 0) {
		putc(*Text, File);
		Text++;
	}
}

// 0x08050350  script.c:549
void TWriteScriptFile::writeString(char *Text) {
	if (File == nullptr) {
		::error("TWriteScriptFile::writeString: Kein Skript zum Schreiben ge\xf6" "ffnet.\n");
		throw "Cannot write string";
	}

	if (Text == nullptr) {
		::error("TWriteScriptFile::writeString: Text ist NULL.\n");
		throw "Cannot write string";
	}

	putc('"', File);
	while (*Text != 0) {
		if (*Text == '"' || *Text == '\\') {
			putc('\\', File);
		}
		if (*Text == '\n') {
			putc('\\', File);
			putc('n', File);
		} else {
			putc(*Text, File);
		}
		Text++;
	}
	putc('"', File);
}

// 0x08050470  script.c:574
void TWriteScriptFile::writeNumber(int Number) {
	if (File == nullptr) {
		::error("TWriteScriptFile::writeNumber: Kein Skript zum Schreiben ge\xf6" "ffnet.\n");
		throw "Cannot write number";
	}

	char s[20];
	sprintf(s, "%d", Number);
	writeText(s);
}

// 0x08050510  script.c:589
void TWriteScriptFile::writeBytesequence(unsigned char *Sequence, int Length) {
	if (File == nullptr) {
		::error("TWriteScriptFile::writeBytesequence: Kein Skript zum Schreiben ge\xf6" "ffnet.\n");
		throw "Cannot write bytesequence";
	}

	if (Sequence == nullptr) {
		::error("TWriteScriptFile::writeBytesequence: Sequence ist NULL.\n");
		throw "Cannot write bytesequence";
	}

	if (Length <= 0) {
		::error("TWriteScriptFile::writeBytesequence: Ung\xfcltige Sequenzl\xe4nge.\n");
		throw "Cannot write bytesequence";
	}

	for (int i = 0; i < Length; i++) {
		if (i != 0) {
			writeText("-");
		}
		writeNumber(Sequence[i]);
	}
}

// 0x08050660  script.c:612
void TWriteScriptFile::writeCoordinate(int x, int y, int z) {
	if (File == nullptr) {
		::error("TWriteScriptFile::writeCoordinate: Kein Skript zum Schreiben ge\xf6" "ffnet.\n");
		throw "Cannot write coordinate";
	}

	if (x < 0 || y < 0 || z < 0) {
		::error("TWriteScriptFile::writeCoordinate: Ung\xfcltige Koordinaten [%d,%d,%d].\n", x, y, z);
		throw "Invalid coordinates";
	}

	char s[20];
	sprintf(s, "[%u,%u,%u]", x, y, z);
	writeText(s);
}

// 0x08050760  script.c:635
TReadBinaryFile::TReadBinaryFile() {
	File = nullptr;
}

// 0x080507f0  script.c:643
TReadBinaryFile::~TReadBinaryFile() {
	if (File != nullptr) {
		::error("TReadBinaryFile::~TReadBinaryFile: Datei %s ist noch offen.\n", Filename);
		if (fclose(File) != 0) {
			::error("TReadBinaryFile::~TReadBinaryFile: Fehler %d beim Schlie\xdf" "en der Datei.\n", errno);
		}
	}
}

// 0x080508d0  script.c:654
void TReadBinaryFile::open(char *FileName) {
	if (File != nullptr) {
		error("File still open");
	}

	File = fopen(FileName, "rb");
	if (File == nullptr) {
		sprintf(ErrorString, "Cannot open file %s", FileName);
		throw ErrorString;
	}

	strcpy(Filename, FileName);
	FileSize = -1;
}

// 0x08050a00  script.c:669
void TReadBinaryFile::close() {
	if (fclose(File) != 0) {
		int ErrorCode = errno;
		::error("TReadBinaryFile::close: Fehler beim Schlie\xdf" "en der Datei.\n");
		::error("# Datei: %s, Fehlercode: %d (%s)\n", Filename, ErrorCode, strerror(ErrorCode));
	}
	File = nullptr;
}

// 0x08050a70  script.c:682
void TReadBinaryFile::seek(int32_t Offset) {
	if (File == nullptr) {
		error("File not open for seek");
	}

	if (Offset < 0) {
		error("Negative offset for seek");
	}

	fseek(File, Offset, SEEK_SET);
}

// 0x08050ba0  script.c:692
int TReadBinaryFile::getSize() {
	if (FileSize == -1) {
		int Position = (int)ftell(File);
		fseek(File, 0, SEEK_END);
		FileSize = (int)ftell(File);
		fseek(File, Position, SEEK_SET);
	}
	return FileSize;
}

// 0x08050c20  script.c:706
int TReadBinaryFile::getPosition() {
	return (int)ftell(File);
}

// 0x08050c40  script.c:714
void TReadBinaryFile::error(char *Text) {
	if (File != nullptr) {
		if (fclose(File) != 0) {
			::error("TReadBinaryFile::error: Fehler %d beim Schlie\xdf" "en der Datei.\n", errno);
		}
	}
	File = nullptr;

	sprintf(ErrorString, "error in binary-file \"%s\": %s.", Filename, Text);
	throw ErrorString;
}

// 0x08050cf0  script.c:727
unsigned char TReadBinaryFile::readByte() {
	unsigned char Help;
	int Result = (int)fread(&Help, 1, 1, File);
	if (Result != 1) {
		int ErrorCode = errno;
		::error("TReadBinaryFile::readByte: Fehler beim Lesen eines Bytes\n");
		::error("# Datei: %s, Position: %d, R\xfc" "ckgabewert: %d, Fehlercode: %d (%s)\n",
				Filename, (int)ftell(File), Result, ErrorCode, strerror(ErrorCode));
		if (fclose(File) != 0) {
			::error("TReadBinaryFile::readByte: Fehler %d beim Schlie\xdf" "en der Datei.\n", errno);
		}
		File = nullptr;
		SaveFile(Filename);
		error("Error while reading byte");
	}
	return Help;
}

// 0x08050e60  script.c:746
void TReadBinaryFile::readBytes(unsigned char *Buffer, int Count) {
	int Result = (int)fread(Buffer, 1, Count, File);
	if (Result != Count) {
		int ErrorCode = errno;
		::error("TReadBinaryFile::readBytes: Fehler beim Lesen von %d Bytes\n", Count);
		::error("# Datei: %s, Position %d, R\xfc" "ckgabewert: %d, Fehlercode: %d (%s)\n",
				Filename, (int)ftell(File), Result, ErrorCode, strerror(ErrorCode));
		if (fclose(File) != 0) {
			// the original names readByte here
			::error("TReadBinaryFile::readByte: Fehler %d beim Schlie\xdf" "en der Datei.\n", errno);
		}
		File = nullptr;
		SaveFile(Filename);
		error("Error while reading bytes");
	}
}

// 0x08050fd0  script.c:763
bool TReadBinaryFile::eof() {
	if (File == nullptr) {
		error("File not open for eof check");
	}

	return getPosition() >= getSize();
}

// 0x080510f0  script.c:772
void TReadBinaryFile::skip(int Count) {
	if (File == nullptr) {
		error("File not open for skip");
	}

	seek(getPosition() + Count);
}

// 0x080511c0  script.c:786
TWriteBinaryFile::TWriteBinaryFile() {
	File = nullptr;
}

// 0x08051250  script.c:794
TWriteBinaryFile::~TWriteBinaryFile() {
	if (File != nullptr) {
		::error("TWriteBinaryFile::~TWriteBinaryFile: Datei %s ist noch offen.\n", Filename);
		if (fclose(File) != 0) {
			::error("TWriteBinaryFile::~TWriteBinaryFile: Fehler %d beim Schlie\xdf" "en der Datei.\n", errno);
		}
	}
}

// 0x08051330  script.c:805
void TWriteBinaryFile::open(char *FileName) {
	if (File != nullptr) {
		error("File still open");
	}

	File = fopen(FileName, "wb");
	if (File == nullptr) {
		int ErrorCode = errno;
		::error("TWriteBinaryFile::open: Kann Datei %s nicht anlegen.\n", FileName);
		::error("Fehler %d: %s.\n", ErrorCode, strerror(ErrorCode));
		sprintf(ErrorString, "Cannot create file %s.", FileName);
		throw ErrorString;
	}

	strcpy(Filename, FileName);
}

// 0x08051490  script.c:822
void TWriteBinaryFile::close() {
	if (fclose(File) != 0) {
		int ErrorCode = errno;
		::error("TWriteBinaryFile::close: Fehler beim Schlie\xdf" "en der Datei.\n");
		::error("# Datei: %s, Fehlercode: %d (%s)\n", Filename, ErrorCode, strerror(ErrorCode));
	}
	File = nullptr;
}

// 0x08051500  script.c:835
void TWriteBinaryFile::error(char *Text) {
	if (File != nullptr) {
		if (fclose(File) != 0) {
			::error("TWriteBinaryFile::error: Fehler %d beim Schlie\xdf" "en der Datei.\n", errno);
		}
	}
	File = nullptr;

	sprintf(ErrorString, "error in binary-file \"%s\": %s.", Filename, Text);
	throw ErrorString;
}

// 0x080515b0  script.c:848
void TWriteBinaryFile::writeByte(unsigned char Number) {
	unsigned char Help = Number;
	int Result = (int)fwrite(&Help, 1, 1, File);
	if (Result != 1) {
		int ErrorCode = errno;
		::error("TWriteBinaryFile::writeByte: Fehler beim Schreiben eines Bytes\n");
		::error("# Datei: %s, R\xfc" "ckgabewert: %d, Fehlercode: %d (%s)\n",
				Filename, Result, ErrorCode, strerror(ErrorCode));
		if (fclose(File) != 0) {
			::error("TWriteBinaryFile::writeByte: Fehler %d beim Schlie\xdf" "en der Datei.\n", errno);
		}
		File = nullptr;
		SaveFile(Filename);
		error("Error while writing byte");
	}
}

// 0x08051710  script.c:866
void TWriteBinaryFile::writeBytes(unsigned char *Buffer, int Count) {
	int Result = (int)fwrite(Buffer, 1, Count, File);
	if (Result != Count) {
		int ErrorCode = errno;
		::error("TWriteBinaryFile::writeBytes: Fehler beim Schreiben von %d Bytes\n", Count);
		::error("# Datei: %s, R\xfc" "ckgabewert: %d, Fehlercode: %d (%s)\n",
				Filename, Result, ErrorCode, strerror(ErrorCode));
		if (fclose(File) != 0) {
			::error("TWriteBinaryFile::writeBytes: Fehler %d beim Schlie\xdf" "en der Datei.\n", errno);
		}
		File = nullptr;
		SaveFile(Filename);
		error("Error while writing bytes");
	}
}
