#ifndef PATELLA_UTILS_HH
#define PATELLA_UTILS_HH

#include <cstdint>
#include <cstdlib>

typedef void TPrintFunction(int, char *);

typedef void TErrorFunction(char *);

typedef void TFileHandlerFunction(char *, char *, char *);

struct TReadStream {
	virtual bool readFlag();
	virtual unsigned char readByte() = 0;
	virtual unsigned short readWord();
	virtual uint32_t readQuad();
	virtual void readString(char *Buffer, int MaxLength);
	virtual void readBytes(unsigned char *Buffer, int Count);
	virtual bool eof() = 0;
	virtual void skip(int) = 0;
};

struct TWriteStream {
	virtual void writeFlag(bool f);
	virtual void writeByte(unsigned char) = 0;
	virtual void writeWord(unsigned short w);
	virtual void writeQuad(uint32_t q);
	virtual void writeString(char *s);
	virtual void writeBytes(unsigned char *Buffer, int Count);
};

struct TReadBuffer : public TReadStream {
	TReadBuffer(unsigned char *data, int size);
	virtual unsigned char readByte();
	virtual unsigned short readWord();
	virtual uint32_t readQuad();
	virtual void readBytes(unsigned char *Buffer, int Count);
	virtual bool eof();
	virtual void skip(int Count);
	void rewind() { Position = 0; }
	void resize(int size) { Size = size; }
	int getPosition() { return Position; }
private:
	unsigned char *Data;
	int Size;
	int Position;
};

struct TWriteBuffer : public TWriteStream {
	TWriteBuffer(unsigned char *data, int size);
	virtual void writeByte(unsigned char b);
	virtual void writeWord(unsigned short w);
	virtual void writeQuad(uint32_t q);
	virtual void writeBytes(unsigned char *Buffer, int Count);
	int getDataSize() { return Position; }
	void rewind() { Position = 0; }
protected:
	unsigned char *Data;
	int Size;
	int Position;
};

struct TDynamicWriteBuffer : public TWriteBuffer {
	TDynamicWriteBuffer(int InitialSize);
	virtual ~TDynamicWriteBuffer();
	virtual void writeByte(unsigned char b);
	virtual void writeWord(unsigned short w);
	virtual void writeQuad(uint32_t q);
	virtual void writeBytes(unsigned char *Buffer, int Count);
	unsigned char *getData() { return Data; }
private:
	void resizeBuffer();
};

inline bool in(int x, int min, int max) { return x >= min && x <= max; }

void SetPrintFunction(TPrintFunction *Function);
void SetErrorFunction(TErrorFunction *Function);
void SilentHandler(int, char *);
void SilentHandler(char *);
void LogfileHandler(int, char *Text);
void LogfileHandler(char *Text);
void print(int Level, char *Text, ...);
void error(char *Text, ...);
int val(char c);
int random(int low, int high);
bool FileExists(char *FileName);
void TreatAllFiles(char *Directory, char *Extension, TFileHandlerFunction *Function);
void SendMail(char *Address, char *Subject, char *Text);
void RemoveZombie();
uint32_t PackAbsoluteCoordinate(int x, int y, int z);
uint32_t PackRelativeCoordinate(int dx, int dy, int dz);
void UnpackAbsoluteCoordinate(uint32_t Code, int &x, int &y, int &z);
void UnpackRelativeCoordinate(uint32_t Code, int &dx, int &dy, int &dz);

extern TPrintFunction *PrintFunction;
extern TErrorFunction *ErrorFunction;
extern char StandardLogfile[4096];

// Defined below the prototypes it uses; the original header declares them first.
template<class T>
void RandomShuffle(T *Buffer, int Size) {
	if (Buffer == nullptr) {
		error("RandomShuffle: Buffer ist NULL.\n");
		return;
	}

	for (int Position = 0; Position < Size - 1; Position++) {
		int Exchange = random(Position, Size - 1);
		if (Exchange != Position) {
			T Help = Buffer[Position];
			Buffer[Position] = Buffer[Exchange];
			Buffer[Exchange] = Help;
		}
	}
}

#endif
