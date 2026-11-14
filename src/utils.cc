// utils.cc: the helpers every part of the server uses: print and error, which
// call the handlers set with SetPrintFunction and SetErrorFunction; the byte
// streams (TReadStream, TWriteStream) and their buffers (TReadBuffer,
// TWriteBuffer, TDynamicWriteBuffer); random; FileExists; and the packing of
// coordinates into one number.

#include <cerrno>
#include <cstdarg>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <sys/stat.h>
#include <sys/types.h>

#include "utils.hh"

TPrintFunction *PrintFunction = nullptr;
TErrorFunction *ErrorFunction = nullptr;

// The length word of a string in a stream: a string of this length or longer
// has this word followed by its length as a quad.
const int LONG_STRING_MARKER = 0xFFFF;

void SetPrintFunction(TPrintFunction *Function) {
	PrintFunction = Function;
}

void SetErrorFunction(TErrorFunction *Function) {
	ErrorFunction = Function;
}

void print(int Level, char *Text, ...) {
	va_list args;
	char s[1024];

	va_start(args, Text);
	vsnprintf(s, sizeof(s), Text, args);
	va_end(args);

	if (PrintFunction != nullptr) {
		PrintFunction(Level, s);
	} else {
		printf("%s", s);
	}
}

void error(char *Text, ...) {
	va_list args;
	char s[1024];

	va_start(args, Text);
	vsnprintf(s, sizeof(s), Text, args);
	va_end(args);

	if (ErrorFunction != nullptr) {
		ErrorFunction(s);
	} else {
		printf("%s", s);
	}
}

unsigned short TReadStream::readWord() {
	unsigned char Lo = readByte();
	unsigned char Hi = readByte();
	return static_cast<unsigned short>(Lo | (Hi << 8));
}

uint32_t TReadStream::readQuad() {
	unsigned char Lo = readByte();
	unsigned char lo = readByte();
	unsigned char hi = readByte();
	unsigned char Hi = readByte();
	return static_cast<uint32_t>(Lo)
		| (static_cast<uint32_t>(lo) << 8)
		| (static_cast<uint32_t>(hi) << 16)
		| (static_cast<uint32_t>(Hi) << 24);
}

void TReadStream::readString(char *Buffer, int MaxLength) {
	if (Buffer == nullptr) {
		error("TReadStream::readString: Given buffer does not exist.\n");
		throw "internal error";
	}

	int Length = readWord();
	if (Length == LONG_STRING_MARKER) {
		Length = static_cast<int>(readQuad());
	}

	Buffer[0] = 0;
	if (Length == 0) {
		return;
	}

	if (MaxLength < 0 || Length < MaxLength) {
		readBytes(reinterpret_cast<unsigned char *>(Buffer), Length);
		Buffer[Length] = 0;
	} else {
		readBytes(reinterpret_cast<unsigned char *>(Buffer), MaxLength - 1);
		Buffer[MaxLength - 1] = 0;
		skip(Length - MaxLength + 1);
	}
}

void TReadStream::readBytes(unsigned char *Buffer, int Count) {
	if (Buffer == nullptr) {
		error("TReadStream::readBytes: Given buffer does not exist.\n");
		throw "internal error";
	}

	for (int i = 0; i < Count; i++) {
		Buffer[i] = readByte();
	}
}

void TWriteStream::writeWord(unsigned short w) {
	writeByte(static_cast<unsigned char>(w));
	writeByte(static_cast<unsigned char>(w >> 8));
}

void TWriteStream::writeQuad(uint32_t q) {
	writeByte(static_cast<unsigned char>(q));
	writeByte(static_cast<unsigned char>(q >> 8));
	writeByte(static_cast<unsigned char>(q >> 16));
	writeByte(static_cast<unsigned char>(q >> 24));
}

void TWriteStream::writeString(char *s) {
	if (s == nullptr) {
		writeWord(0);
		return;
	}

	int Length = static_cast<int>(strlen(s));
	if (Length < LONG_STRING_MARKER) {
		writeWord(static_cast<unsigned short>(Length));
	} else {
		writeWord(LONG_STRING_MARKER);
		writeQuad(static_cast<uint32_t>(Length));
	}

	if (Length != 0) {
		writeBytes(reinterpret_cast<unsigned char *>(s), Length);
	}
}

void TWriteStream::writeBytes(unsigned char *Buffer, int Count) {
	if (Buffer == nullptr) {
		error("TWriteStream::writeBytes: Given buffer does not exist.\n");
		throw "internal error";
	}

	for (int i = 0; i < Count; i++) {
		writeByte(Buffer[i]);
	}
}

TReadBuffer::TReadBuffer(unsigned char *data, int size) {
	Data = data;
	Size = size;
	Position = 0;

	if (data == nullptr) {
		error("TReadBuffer::TReadBuffer: data is NULL.\n");
		Size = 0;
	} else if (size < 0) {
		error("TReadBuffer::TReadBuffer: Invalid data size %d.\n", size);
		Size = 0;
	}
}

unsigned char TReadBuffer::readByte() {
	if (Position > Size - 1) {
		throw "buffer empty";
	}

	unsigned char b = Data[Position];
	Position += 1;
	return b;
}

unsigned short TReadBuffer::readWord() {
	if (Position > Size - 2) {
		throw "buffer empty";
	}

	unsigned short w = static_cast<unsigned short>(Data[Position] + (Data[Position + 1] << 8));
	Position += 2;
	return w;
}

uint32_t TReadBuffer::readQuad() {
	if (Position > Size - 4) {
		throw "buffer empty";
	}

	uint32_t q = static_cast<uint32_t>(Data[Position])
		+ (static_cast<uint32_t>(Data[Position + 1]) << 8)
		+ (static_cast<uint32_t>(Data[Position + 2]) << 16)
		+ (static_cast<uint32_t>(Data[Position + 3]) << 24);
	Position += 4;
	return q;
}

void TReadBuffer::readBytes(unsigned char *Buffer, int Count) {
	if (Buffer == nullptr) {
		error("TReadBuffer::readBytes: Given buffer does not exist.\n");
		throw "buffer not existing";
	}

	if (Position > Size - Count) {
		throw "buffer empty";
	}

	memcpy(Buffer, Data + Position, Count);
	Position += Count;
}

bool TReadBuffer::eof() {
	return Position >= Size;
}

void TReadBuffer::skip(int Count) {
	if (Position > Size - Count) {
		throw "buffer empty";
	}

	Position += Count;
}

TWriteBuffer::TWriteBuffer(unsigned char *data, int size) {
	Data = data;
	Size = size;
	Position = 0;

	// Unlike TReadBuffer, a negative size is only reported; Size keeps it.
	if (data == nullptr) {
		error("TWriteBuffer::TWriteBuffer: data is NULL.\n");
		Size = 0;
	} else if (size < 0) {
		error("TWriteBuffer::TWriteBuffer: Invalid data size %d.\n", size);
	}
}

void TWriteBuffer::writeByte(unsigned char b) {
	if (Position > Size - 1) {
		throw "buffer full";
	}

	Data[Position] = b;
	Position += 1;
}

void TWriteBuffer::writeWord(unsigned short w) {
	if (Position > Size - 2) {
		throw "buffer full";
	}

	Data[Position] = static_cast<unsigned char>(w);
	Data[Position + 1] = static_cast<unsigned char>(w >> 8);
	Position += 2;
}

void TWriteBuffer::writeQuad(uint32_t q) {
	if (Position > Size - 4) {
		throw "buffer full";
	}

	Data[Position] = static_cast<unsigned char>(q);
	Data[Position + 1] = static_cast<unsigned char>(q >> 8);
	Data[Position + 2] = static_cast<unsigned char>(q >> 16);
	Data[Position + 3] = static_cast<unsigned char>(q >> 24);
	Position += 4;
}

void TWriteBuffer::writeBytes(unsigned char *Buffer, int Count) {
	if (Position > Size - Count) {
		throw "buffer full";
	}

	memcpy(Data + Position, Buffer, Count);
	Position += Count;
}

TDynamicWriteBuffer::TDynamicWriteBuffer(int InitialSize) : TWriteBuffer(new unsigned char[InitialSize], InitialSize) {
}

TDynamicWriteBuffer::~TDynamicWriteBuffer() {
	delete[] Data;
}

void TDynamicWriteBuffer::resizeBuffer() {
	unsigned char *Help = new unsigned char[Size * 2];
	memcpy(Help, Data, Size);
	delete[] Data;
	Data = Help;
	Size *= 2;
}

void TDynamicWriteBuffer::writeByte(unsigned char b) {
	while (Position > Size - 1) {
		resizeBuffer();
	}

	Data[Position] = b;
	Position += 1;
}

void TDynamicWriteBuffer::writeWord(unsigned short w) {
	while (Position > Size - 2) {
		resizeBuffer();
	}

	Data[Position] = static_cast<unsigned char>(w);
	Data[Position + 1] = static_cast<unsigned char>(w >> 8);
	Position += 2;
}

void TDynamicWriteBuffer::writeQuad(uint32_t q) {
	while (Position > Size - 4) {
		resizeBuffer();
	}

	Data[Position] = static_cast<unsigned char>(q);
	Data[Position + 1] = static_cast<unsigned char>(q >> 8);
	Data[Position + 2] = static_cast<unsigned char>(q >> 16);
	Data[Position + 3] = static_cast<unsigned char>(q >> 24);
	Position += 4;
}

void TDynamicWriteBuffer::writeBytes(unsigned char *Buffer, int Count) {
	while (Position > Size - Count) {
		resizeBuffer();
	}

	memcpy(Data + Position, Buffer, Count);
	Position += Count;
}

int random(int low, int high) {
	// Computed unsigned, so a span of 2^31 or more wraps to a range of 0 or less and gives low.
	int range = static_cast<int>(static_cast<uint32_t>(high) - static_cast<uint32_t>(low) + 1);
	if (range <= 0) {
		return low;
	}
	return low + rand() % range;
}

bool FileExists(char *FileName) {
	struct stat Buffer;

	errno = 0;
	int retval = stat(FileName, &Buffer);
	if (retval == 0) {
		return true;
	}

	if (errno != ENOENT) {
		error("FileExists: Unexpected error code %d.\n", errno);
	}
	return false;
}

uint32_t PackAbsoluteCoordinate(int x, int y, int z) {
	return ((static_cast<uint32_t>(x) - 24576) << 18)
		+ ((static_cast<uint32_t>(y) - 24576) << 4)
		+ static_cast<uint32_t>(z);
}

uint32_t PackRelativeCoordinate(int dx, int dy, int dz) {
	return ((static_cast<uint32_t>(dx) + 8192) << 18)
		+ ((static_cast<uint32_t>(dy) + 8192) << 4)
		+ static_cast<uint32_t>(dz) + 8;
}

void UnpackAbsoluteCoordinate(uint32_t Code, int &x, int &y, int &z) {
	x = static_cast<int>(Code >> 18) + 24576;
	y = static_cast<int>((Code >> 4) & 0x3FFF) + 24576;
	z = static_cast<int>(Code & 0xF);
}

void UnpackRelativeCoordinate(uint32_t Code, int &dx, int &dy, int &dz) {
	dx = static_cast<int>(Code >> 18) - 8192;
	dy = static_cast<int>((Code >> 4) & 0x3FFF) - 8192;
	dz = static_cast<int>(Code & 0xF) - 8;
}
