#ifndef PATELLA_SCRIPT_HH
#define PATELLA_SCRIPT_HH

#include <cstdint>
#include <stdio.h>

#include "utils.hh"

enum TOKEN {
	ENDOFFILE = 0,
	IDENTIFIER = 1,
	NUMBER = 2,
	STRING = 3,
	BYTES = 4,
	COORDINATE = 5,
	SPECIAL = 6,
};

struct TReadScriptFile {
	static const int MAXIMUMSTRINGLENGTH = 4000;
	static const int MAXIMUMIDENTIFIERLENGTH = 30;
	static const int MAXIMUMRECURSIONDEPTH = 3;
	TReadScriptFile();
	~TReadScriptFile();
	void open(char *FileName);
	void close();
	void error(char *Text);
	void nextToken();
	TOKEN Token;
	char *readIdentifier();
	int readNumber();
	char *readString();
	unsigned char *readBytesequence();
	void readCoordinate(int &x, int &y, int &z);
	char readSpecial();
	void readSymbol(char Symbol);
	char *getIdentifier();
	int getNumber();
	char *getString();
	unsigned char *getBytesequence();
	void getCoordinate(int &x, int &y, int &z);
	char getSpecial();
private:
	FILE *File[MAXIMUMRECURSIONDEPTH];
	char Filename[MAXIMUMRECURSIONDEPTH][4096];
	int Line[MAXIMUMRECURSIONDEPTH];
	char String[MAXIMUMSTRINGLENGTH];
	int RecursionDepth;
	unsigned char *Bytes;
	int Number;
	int CoordX;
	int CoordY;
	int CoordZ;
	char Special;
};

struct TWriteScriptFile {
	TWriteScriptFile();
	~TWriteScriptFile();
	void open(char *FileName);
	void close();
	void writeLn();
	void writeText(char *Text);
	void writeString(char *Text);
	void writeNumber(int Number);
	void writeBytesequence(unsigned char *Sequence, int Length);
	void writeCoordinate(int x, int y, int z);
private:
	FILE *File;
	char Filename[4096];
};

struct TReadBinaryFile : public TReadStream {
	TReadBinaryFile();
	virtual ~TReadBinaryFile();
	void open(char *FileName);
	void close();
	void seek(int32_t Offset);
	int getSize();
	int getPosition();
	void error(char *Text);
	virtual unsigned char readByte();
	virtual void readBytes(unsigned char *Buffer, int Count);
	virtual bool eof();
	virtual void skip(int Count);
private:
	FILE *File;
	char Filename[4096];
	int FileSize;
};

struct TWriteBinaryFile : public TWriteStream {
	TWriteBinaryFile();
	virtual ~TWriteBinaryFile();
	void open(char *FileName);
	void close();
	void error(char *Text);
	virtual void writeByte(unsigned char Number);
	virtual void writeBytes(unsigned char *Buffer, int Count);
private:
	FILE *File;
	char Filename[4096];
};

void SaveFile(char *Filename);

#endif
