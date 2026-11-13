#include <cstdint>
#include <cstdlib>

#include "script.hh"
#include "utils.hh"

[[maybe_unused]] static char ErrorString[100];	// script.c:14

// 0x0804ea20  script.c:21
void SaveFile(char * /*Filename*/) { abort(); /* STUB */ }

// 0x0804ec30  script.c:63
TReadScriptFile::TReadScriptFile() { abort(); /* STUB */ }

// 0x0804ecc0  script.c:72
TReadScriptFile::~TReadScriptFile() { abort(); /* STUB */ }

// 0x0804ed30  script.c:83
void TReadScriptFile::open(char * /*FileName*/) { abort(); /* STUB */ }

// 0x0804eec0  script.c:110
void TReadScriptFile::close() { abort(); /* STUB */ }

// 0x0804ef30  script.c:123
void TReadScriptFile::error(char * /*Text*/) { abort(); /* STUB */ }

// 0x0804f040  script.c:137
void TReadScriptFile::nextToken() { abort(); /* STUB */ }

// 0x0804f950  script.c:311
char *TReadScriptFile::readIdentifier() { abort(); /* STUB */ }

// 0x0804f9d0  script.c:322
int TReadScriptFile::readNumber() { abort(); /* STUB */ }

// 0x0804fa80  script.c:338
char *TReadScriptFile::readString() { abort(); /* STUB */ }

// 0x0804faf0  script.c:349
unsigned char *TReadScriptFile::readBytesequence() { abort(); /* STUB */ }

// 0x0804fb60  script.c:360
void TReadScriptFile::readCoordinate(int & /*x*/, int & /*y*/, int & /*z*/) { abort(); /* STUB */ }

// 0x0804fbf0  script.c:371
char TReadScriptFile::readSpecial() { abort(); /* STUB */ }

// 0x0804fc60  script.c:382
void TReadScriptFile::readSymbol(char /*Symbol*/) { abort(); /* STUB */ }

// 0x0804fd00  script.c:392
char *TReadScriptFile::getIdentifier() { abort(); /* STUB */ }

// 0x0804fd50  script.c:402
int TReadScriptFile::getNumber() { abort(); /* STUB */ }

// 0x0804fda0  script.c:411
char *TReadScriptFile::getString() { abort(); /* STUB */ }

// 0x0804fdf0  script.c:420
unsigned char *TReadScriptFile::getBytesequence() { abort(); /* STUB */ }

// 0x0804fe40  script.c:429
void TReadScriptFile::getCoordinate(int & /*x*/, int & /*y*/, int & /*z*/) { abort(); /* STUB */ }

// 0x0804feb0  script.c:440
char TReadScriptFile::getSpecial() { abort(); /* STUB */ }

// 0x0804ff10  script.c:454
TWriteScriptFile::TWriteScriptFile() { abort(); /* STUB */ }

// 0x0804ff90  script.c:462
TWriteScriptFile::~TWriteScriptFile() { abort(); /* STUB */ }

// 0x08050000  script.c:473
void TWriteScriptFile::open(char * /*FileName*/) { abort(); /* STUB */ }

// 0x08050110  script.c:494
void TWriteScriptFile::close() { abort(); /* STUB */ }

// 0x08050180  script.c:507
void TWriteScriptFile::error(char * /*Text*/) { abort(); /* STUB */ }

// 0x08050240  script.c:520
void TWriteScriptFile::writeLn() { abort(); /* STUB */ }

// 0x080502b0  script.c:532
void TWriteScriptFile::writeText(char * /*Text*/) { abort(); /* STUB */ }

// 0x08050350  script.c:549
void TWriteScriptFile::writeString(char * /*Text*/) { abort(); /* STUB */ }

// 0x08050470  script.c:574
void TWriteScriptFile::writeNumber(int /*Number*/) { abort(); /* STUB */ }

// 0x08050510  script.c:589
void TWriteScriptFile::writeBytesequence(unsigned char * /*Sequence*/, int /*Length*/) { abort(); /* STUB */ }

// 0x08050660  script.c:612
void TWriteScriptFile::writeCoordinate(int /*x*/, int /*y*/, int /*z*/) { abort(); /* STUB */ }

// 0x08050760  script.c:635
TReadBinaryFile::TReadBinaryFile() { abort(); /* STUB */ }

// 0x080507f0  script.c:643
TReadBinaryFile::~TReadBinaryFile() { abort(); /* STUB */ }

// 0x080508d0  script.c:654
void TReadBinaryFile::open(char * /*FileName*/) { abort(); /* STUB */ }

// 0x08050a00  script.c:669
void TReadBinaryFile::close() { abort(); /* STUB */ }

// 0x08050a70  script.c:682
void TReadBinaryFile::seek(int32_t /*Offset*/) { abort(); /* STUB */ }

// 0x08050ba0  script.c:692
int TReadBinaryFile::getSize() { abort(); /* STUB */ }

// 0x08050c20  script.c:706
int TReadBinaryFile::getPosition() { abort(); /* STUB */ }

// 0x08050c40  script.c:714
void TReadBinaryFile::error(char * /*Text*/) { abort(); /* STUB */ }

// 0x08050cf0  script.c:727
unsigned char TReadBinaryFile::readByte() { abort(); /* STUB */ }

// 0x08050e60  script.c:746
void TReadBinaryFile::readBytes(unsigned char * /*Buffer*/, int /*Count*/) { abort(); /* STUB */ }

// 0x08050fd0  script.c:763
bool TReadBinaryFile::eof() { abort(); /* STUB */ }

// 0x080510f0  script.c:772
void TReadBinaryFile::skip(int /*Count*/) { abort(); /* STUB */ }

// 0x080511c0  script.c:786
TWriteBinaryFile::TWriteBinaryFile() { abort(); /* STUB */ }

// 0x08051250  script.c:794
TWriteBinaryFile::~TWriteBinaryFile() { abort(); /* STUB */ }

// 0x08051330  script.c:805
void TWriteBinaryFile::open(char * /*FileName*/) { abort(); /* STUB */ }

// 0x08051490  script.c:822
void TWriteBinaryFile::close() { abort(); /* STUB */ }

// 0x08051500  script.c:835
void TWriteBinaryFile::error(char * /*Text*/) { abort(); /* STUB */ }

// 0x080515b0  script.c:848
void TWriteBinaryFile::writeByte(unsigned char /*Number*/) { abort(); /* STUB */ }

// 0x08051710  script.c:866
void TWriteBinaryFile::writeBytes(unsigned char * /*Buffer*/, int /*Count*/) { abort(); /* STUB */ }
