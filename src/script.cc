// script.cc: the readers and writers of the data files. TReadScriptFile splits
// a text file into tokens (identifiers, numbers, strings, byte sequences,
// coordinates, special characters) and follows the files it includes;
// TWriteScriptFile writes such a file. TReadBinaryFile and TWriteBinaryFile
// read and write binary files. SaveFile keeps a copy of a file that could not
// be read.

#include <cerrno>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#include "script.hh"
#include "mudinc.hh"
#include "utils.hh"

// The text of the errors the file classes throw. It takes file names of up to
// 4095 characters: the script-file classes format it with snprintf, which
// truncates; quirk: the binary-file classes format it with sprintf, unbounded.

static char ErrorString[100];

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

TReadScriptFile::TReadScriptFile() {
	RecursionDepth = -1;
	Bytes = (unsigned char *)String;
}

TReadScriptFile::~TReadScriptFile() {
	if (RecursionDepth != -1) {
		::error("TReadScriptFile::~TReadScriptFile: Datei ist noch offen.\n");
		while (RecursionDepth != -1) {
			close();
		}
	}
}

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
				// unsigned: an overlong number wraps around
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

char *TReadScriptFile::readIdentifier() {
	nextToken();
	if (Token != IDENTIFIER) {
		error("identifier expected");
	}
	return getIdentifier();
}

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

char *TReadScriptFile::readString() {
	nextToken();
	if (Token != STRING) {
		error("string expected");
	}
	return getString();
}

unsigned char *TReadScriptFile::readBytesequence() {
	nextToken();
	if (Token != BYTES) {
		error("byte-sequence expected");
	}
	return getBytesequence();
}

void TReadScriptFile::readCoordinate(int &x, int &y, int &z) {
	nextToken();
	if (Token != COORDINATE) {
		error("coordinates expected");
	}
	getCoordinate(x, y, z);
}

char TReadScriptFile::readSpecial() {
	nextToken();
	if (Token != SPECIAL) {
		error("special-char expected");
	}
	return getSpecial();
}

void TReadScriptFile::readSymbol(char Symbol) {
	nextToken();
	if (Token != SPECIAL) {
		error("special-char expected");
	}
	if (getSpecial() != Symbol) {
		error("special-char expected");
	}
}

char *TReadScriptFile::getIdentifier() {
	if (Token != IDENTIFIER) {
		error("identifier expected");
	}
	strLower(String);
	return String;
}

int TReadScriptFile::getNumber() {
	if (Token != NUMBER) {
		error("number expected");
	}
	return Number;
}

char *TReadScriptFile::getString() {
	if (Token != STRING) {
		error("string expected");
	}
	return String;
}

unsigned char *TReadScriptFile::getBytesequence() {
	if (Token != BYTES) {
		error("byte-sequence expected");
	}
	return Bytes;
}

void TReadScriptFile::getCoordinate(int &x, int &y, int &z) {
	if (Token != COORDINATE) {
		error("coordinates expected");
	}
	x = CoordX;
	y = CoordY;
	z = CoordZ;
}

char TReadScriptFile::getSpecial() {
	if (Token != SPECIAL) {
		error("special-char expected");
	}
	return Special;
}

TWriteScriptFile::TWriteScriptFile() {
	File = nullptr;
}

TWriteScriptFile::~TWriteScriptFile() {
	if (File != nullptr) {
		::error("TWriteScriptFile::~TWriteScriptFile: Datei %s ist noch offen.\n", Filename);
		if (fclose(File) != 0) {
			::error("TWriteScriptFile::~TWriteScriptFile: Fehler %d beim Schlie\xdf" "en der Datei.\n", errno);
		}
	}
}

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

	strcpy(Filename, FileName);
}

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

void TWriteScriptFile::writeLn() {
	if (File == nullptr) {
		::error("TWriteScriptFile::writeLn: Kein Skript zum Schreiben ge\xf6" "ffnet.\n");
		throw "Cannot write linefeed";
	}

	putc('\n', File);
}

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

void TWriteScriptFile::writeNumber(int Number) {
	if (File == nullptr) {
		::error("TWriteScriptFile::writeNumber: Kein Skript zum Schreiben ge\xf6" "ffnet.\n");
		throw "Cannot write number";
	}

	char s[20];
	sprintf(s, "%d", Number);
	writeText(s);
}

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

TReadBinaryFile::TReadBinaryFile() {
	File = nullptr;
}

TReadBinaryFile::~TReadBinaryFile() {
	if (File != nullptr) {
		::error("TReadBinaryFile::~TReadBinaryFile: Datei %s ist noch offen.\n", Filename);
		if (fclose(File) != 0) {
			::error("TReadBinaryFile::~TReadBinaryFile: Fehler %d beim Schlie\xdf" "en der Datei.\n", errno);
		}
	}
}

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

void TReadBinaryFile::close() {
	if (fclose(File) != 0) {
		int ErrorCode = errno;
		::error("TReadBinaryFile::close: Fehler beim Schlie\xdf" "en der Datei.\n");
		::error("# Datei: %s, Fehlercode: %d (%s)\n", Filename, ErrorCode, strerror(ErrorCode));
	}
	File = nullptr;
}

void TReadBinaryFile::seek(int32_t Offset) {
	if (File == nullptr) {
		error("File not open for seek");
	}

	if (Offset < 0) {
		error("Negative offset for seek");
	}

	fseek(File, Offset, SEEK_SET);
}

int TReadBinaryFile::getSize() {
	if (FileSize == -1) {
		int Position = (int)ftell(File);
		fseek(File, 0, SEEK_END);
		FileSize = (int)ftell(File);
		fseek(File, Position, SEEK_SET);
	}
	return FileSize;
}

int TReadBinaryFile::getPosition() {
	return (int)ftell(File);
}

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

void TReadBinaryFile::readBytes(unsigned char *Buffer, int Count) {
	int Result = (int)fread(Buffer, 1, Count, File);
	if (Result != Count) {
		int ErrorCode = errno;
		::error("TReadBinaryFile::readBytes: Fehler beim Lesen von %d Bytes\n", Count);
		::error("# Datei: %s, Position %d, R\xfc" "ckgabewert: %d, Fehlercode: %d (%s)\n",
				Filename, (int)ftell(File), Result, ErrorCode, strerror(ErrorCode));
		if (fclose(File) != 0) {
			// quirk: the message names readByte, not readBytes
			::error("TReadBinaryFile::readByte: Fehler %d beim Schlie\xdf" "en der Datei.\n", errno);
		}
		File = nullptr;
		SaveFile(Filename);
		error("Error while reading bytes");
	}
}

bool TReadBinaryFile::eof() {
	if (File == nullptr) {
		error("File not open for eof check");
	}

	return getPosition() >= getSize();
}

void TReadBinaryFile::skip(int Count) {
	if (File == nullptr) {
		error("File not open for skip");
	}

	seek(getPosition() + Count);
}

TWriteBinaryFile::TWriteBinaryFile() {
	File = nullptr;
}

TWriteBinaryFile::~TWriteBinaryFile() {
	if (File != nullptr) {
		::error("TWriteBinaryFile::~TWriteBinaryFile: Datei %s ist noch offen.\n", Filename);
		if (fclose(File) != 0) {
			::error("TWriteBinaryFile::~TWriteBinaryFile: Fehler %d beim Schlie\xdf" "en der Datei.\n", errno);
		}
	}
}

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

void TWriteBinaryFile::close() {
	if (fclose(File) != 0) {
		int ErrorCode = errno;
		::error("TWriteBinaryFile::close: Fehler beim Schlie\xdf" "en der Datei.\n");
		::error("# Datei: %s, Fehlercode: %d (%s)\n", Filename, ErrorCode, strerror(ErrorCode));
	}
	File = nullptr;
}

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
