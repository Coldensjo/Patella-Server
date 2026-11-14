// script.cc: the readers and writers of the data files. TReadScriptFile splits
// a text file, or such a text in memory, into tokens (identifiers, numbers,
// strings, byte sequences, coordinates, special characters) and follows the
// files it includes; TWriteScriptFile writes such a file or text.
// TReadBinaryFile and TWriteBinaryFile read and write binary files. SaveFile
// keeps a copy of a file that could not be read.

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
		error("SaveFile: Filename is NULL.\n");
		return;
	}

	char BackupFilename[4096];
	strcpy(BackupFilename, Filename);
	strcat(BackupFilename, "#");

	FILE *Source = fopen(Filename, "rb");
	if (Source == nullptr) {
		error("SaveFile: Source file %s does not exist.\n", Filename);
		return;
	}

	FILE *Dest = fopen(BackupFilename, "wb");
	if (Dest == nullptr) {
		error("SaveFile: Cannot create target file %s.\n", BackupFilename);
		return;
	}

	fseek(Source, 0, SEEK_END);
	size_t Size = ftell(Source);
	fseek(Source, 0, SEEK_SET);

	unsigned char Buffer[Size];
	if (fread(Buffer, 1, Size, Source) != Size) {
		error("SaveFile: Error while reading the source file %s.\n", Filename);
	} else if (fwrite(Buffer, 1, Size, Dest) != Size) {
		error("SaveFile: Error while writing the target file %s.\n", Filename);
	}

	if (fclose(Source) != 0) {
		error("SaveFile: Error %d while closing the source file.\n", errno);
	}
	if (fclose(Dest) != 0) {
		error("SaveFile: Error %d while closing the target file.\n", errno);
	}
}

TReadScriptFile::TReadScriptFile() {
	RecursionDepth = -1;
	Bytes = (unsigned char *)String;
}

TReadScriptFile::~TReadScriptFile() {
	if (RecursionDepth != -1) {
		::error("TReadScriptFile::~TReadScriptFile: File is still open.\n");
		while (RecursionDepth != -1) {
			close();
		}
	}
}

void TReadScriptFile::open(char *FileName) {
	RecursionDepth++;
	if (RecursionDepth == MAXIMUMRECURSIONDEPTH) {
		::error("TReadScriptFile::open: Recursion depth too large.\n");
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
		::error("TReadScriptFile::open: Cannot open file %s.\n", Filename[RecursionDepth]);
		::error("Error %d: %s.\n", ErrorCode, strerror(ErrorCode));
		RecursionDepth--;
		throw "Cannot open script-file";
	}

	Line[RecursionDepth] = 1;
}

void TReadScriptFile::openText(char *Name, const char *Text, int Length) {
	RecursionDepth++;
	if (RecursionDepth == MAXIMUMRECURSIONDEPTH) {
		::error("TReadScriptFile::openText: Recursion depth too large.\n");
		throw "Recursion depth too high";
	}

	snprintf(Filename[RecursionDepth], sizeof(Filename[RecursionDepth]), "%s", Name);

	// fmemopen refuses an empty buffer; a single blank reads as the end of the
	// text just as well.
	if (Length > 0) {
		File[RecursionDepth] = fmemopen(const_cast<char *>(Text), (size_t)Length, "rb");
	} else {
		File[RecursionDepth] = fmemopen(const_cast<char *>(" "), 1, "rb");
	}
	if (File[RecursionDepth] == nullptr) {
		int ErrorCode = errno;
		::error("TReadScriptFile::openText: Cannot open text %s.\n", Filename[RecursionDepth]);
		::error("Error %d: %s.\n", ErrorCode, strerror(ErrorCode));
		RecursionDepth--;
		throw "Cannot open script-text";
	}

	Line[RecursionDepth] = 1;
}

void TReadScriptFile::close() {
	if (RecursionDepth == -1) {
		::error("TReadScriptFile::close: No file open.\n");
		return;
	}

	if (fclose(File[RecursionDepth]) != 0) {
		::error("TReadScriptFile::close: Error %d while closing the file.\n", errno);
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
		::error("TReadScriptFile::nextToken: No script opened for reading.\n");
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
			::error("TReadScriptFile::nextToken: Invalid state.\n");
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
	Text = nullptr;
	TextLength = 0;
}

TWriteScriptFile::~TWriteScriptFile() {
	if (File != nullptr) {
		::error("TWriteScriptFile::~TWriteScriptFile: File %s is still open.\n", Filename);
		if (fclose(File) != 0) {
			::error("TWriteScriptFile::~TWriteScriptFile: Error %d while closing the file.\n", errno);
		}
	}

	// The text openText wrote; open_memstream leaves it to the caller.
	free(Text);
}

void TWriteScriptFile::open(char *FileName) {
	if (File != nullptr) {
		::error("TWriteScriptFile::open: Old script is still open.\n");
		if (fclose(File) != 0) {
			::error("TWriteScriptFile::open: Error %d while closing the file.\n", errno);
		}
		File = nullptr;
	}

	free(Text);
	Text = nullptr;
	TextLength = 0;
	File = fopen(FileName, "wb");
	if (File == nullptr) {
		int ErrorCode = errno;
		::error("TWriteScriptFile::open: Cannot create file %s.\n", FileName);
		::error("Error %d: %s.\n", ErrorCode, strerror(ErrorCode));
		throw "Cannot create script-file";
	}

	strcpy(Filename, FileName);
}

void TWriteScriptFile::openText() {
	if (File != nullptr) {
		::error("TWriteScriptFile::openText: Old script is still open.\n");
		if (fclose(File) != 0) {
			::error("TWriteScriptFile::openText: Error %d while closing the file.\n", errno);
		}
		File = nullptr;
	}

	free(Text);
	Text = nullptr;
	TextLength = 0;
	File = open_memstream(&Text, &TextLength);
	if (File == nullptr) {
		int ErrorCode = errno;
		::error("TWriteScriptFile::openText: Cannot create a text.\n");
		::error("Error %d: %s.\n", ErrorCode, strerror(ErrorCode));
		throw "Cannot create script-text";
	}

	strcpy(Filename, "(text)");
}

void TWriteScriptFile::close() {
	if (File == nullptr) {
		::error("TWriteScriptFile::close: No script open.\n");
		return;
	}

	if (fclose(File) != 0) {
		::error("TWriteScriptFile::close: Error %d while closing the file.\n", errno);
	}
	File = nullptr;
}

void TWriteScriptFile::writeLn() {
	if (File == nullptr) {
		::error("TWriteScriptFile::writeLn: No script opened for writing.\n");
		throw "Cannot write linefeed";
	}

	putc('\n', File);
}

void TWriteScriptFile::writeText(char *Text) {
	if (File == nullptr) {
		::error("TWriteScriptFile::writeText: No script opened for writing.\n");
		throw "Cannot write text";
	}

	if (Text == nullptr) {
		::error("TWriteScriptFile::writeText: Text is NULL.\n");
		throw "Cannot write text";
	}

	while (*Text != 0) {
		putc(*Text, File);
		Text++;
	}
}

void TWriteScriptFile::writeString(char *Text) {
	if (File == nullptr) {
		::error("TWriteScriptFile::writeString: No script opened for writing.\n");
		throw "Cannot write string";
	}

	if (Text == nullptr) {
		::error("TWriteScriptFile::writeString: Text is NULL.\n");
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
		::error("TWriteScriptFile::writeNumber: No script opened for writing.\n");
		throw "Cannot write number";
	}

	char s[20];
	sprintf(s, "%d", Number);
	writeText(s);
}

void TWriteScriptFile::writeBytesequence(unsigned char *Sequence, int Length) {
	if (File == nullptr) {
		::error("TWriteScriptFile::writeBytesequence: No script opened for writing.\n");
		throw "Cannot write bytesequence";
	}

	if (Sequence == nullptr) {
		::error("TWriteScriptFile::writeBytesequence: Sequence is NULL.\n");
		throw "Cannot write bytesequence";
	}

	if (Length <= 0) {
		::error("TWriteScriptFile::writeBytesequence: Invalid sequence length.\n");
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
		::error("TWriteScriptFile::writeCoordinate: No script opened for writing.\n");
		throw "Cannot write coordinate";
	}

	if (x < 0 || y < 0 || z < 0) {
		::error("TWriteScriptFile::writeCoordinate: Invalid coordinates [%d,%d,%d].\n", x, y, z);
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
		::error("TReadBinaryFile::~TReadBinaryFile: File %s is still open.\n", Filename);
		if (fclose(File) != 0) {
			::error("TReadBinaryFile::~TReadBinaryFile: Error %d while closing the file.\n", errno);
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
		::error("TReadBinaryFile::close: Error while closing the file.\n");
		::error("# File: %s, error code: %d (%s)\n", Filename, ErrorCode, strerror(ErrorCode));
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
			::error("TReadBinaryFile::error: Error %d while closing the file.\n", errno);
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
		::error("TReadBinaryFile::readByte: Error while reading a byte\n");
		::error("# File: %s, position: %d, return value: %d, error code: %d (%s)\n",
				Filename, (int)ftell(File), Result, ErrorCode, strerror(ErrorCode));
		if (fclose(File) != 0) {
			::error("TReadBinaryFile::readByte: Error %d while closing the file.\n", errno);
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
		::error("TReadBinaryFile::readBytes: Error while reading %d bytes\n", Count);
		::error("# File: %s, position %d, return value: %d, error code: %d (%s)\n",
				Filename, (int)ftell(File), Result, ErrorCode, strerror(ErrorCode));
		if (fclose(File) != 0) {
			// quirk: the message names readByte, not readBytes
			::error("TReadBinaryFile::readBytes: Error %d while closing the file.\n", errno);
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
		::error("TWriteBinaryFile::~TWriteBinaryFile: File %s is still open.\n", Filename);
		if (fclose(File) != 0) {
			::error("TWriteBinaryFile::~TWriteBinaryFile: Error %d while closing the file.\n", errno);
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
		::error("TWriteBinaryFile::open: Cannot create file %s.\n", FileName);
		::error("Error %d: %s.\n", ErrorCode, strerror(ErrorCode));
		sprintf(ErrorString, "Cannot create file %s.", FileName);
		throw ErrorString;
	}

	strcpy(Filename, FileName);
}

void TWriteBinaryFile::close() {
	if (fclose(File) != 0) {
		int ErrorCode = errno;
		::error("TWriteBinaryFile::close: Error while closing the file.\n");
		::error("# File: %s, error code: %d (%s)\n", Filename, ErrorCode, strerror(ErrorCode));
	}
	File = nullptr;
}

void TWriteBinaryFile::error(char *Text) {
	if (File != nullptr) {
		if (fclose(File) != 0) {
			::error("TWriteBinaryFile::error: Error %d while closing the file.\n", errno);
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
		::error("TWriteBinaryFile::writeByte: Error while writing a byte\n");
		::error("# File: %s, return value: %d, error code: %d (%s)\n",
				Filename, Result, ErrorCode, strerror(ErrorCode));
		if (fclose(File) != 0) {
			::error("TWriteBinaryFile::writeByte: Error %d while closing the file.\n", errno);
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
		::error("TWriteBinaryFile::writeBytes: Error while writing %d bytes\n", Count);
		::error("# File: %s, return value: %d, error code: %d (%s)\n",
				Filename, Result, ErrorCode, strerror(ErrorCode));
		if (fclose(File) != 0) {
			::error("TWriteBinaryFile::writeBytes: Error %d while closing the file.\n", errno);
		}
		File = nullptr;
		SaveFile(Filename);
		error("Error while writing bytes");
	}
}
