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
	// inline  utils.h:76
	void rewind() { abort(); /* STUB */ }
	// inline  utils.h:77
	void resize(int /*size*/) { abort(); /* STUB */ }
	int getPosition();
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
	// inline  utils.h:92
	int getDataSize() { abort(); /* STUB */ }
	// inline  utils.h:93
	void rewind() { abort(); /* STUB */ }
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
	unsigned char *getData();
private:
	void resizeBuffer();
};

// inline  utils.h:118
inline bool in(int /*x*/, int /*min*/, int /*max*/) { abort(); /* STUB */ }

// 0x080721b0  utils.h:132  (RandomShuffle<int>)
template<class T>
void RandomShuffle(T * /*Buffer*/, int /*Size*/) { abort(); /* STUB */ }

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

#endif
