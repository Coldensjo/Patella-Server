#include <cstdint>
#include <cstdlib>

#include "script.hh"
#include "config.hh"
#include "threads.hh"
#include "dbfuncs.hh"
#include "objects.hh"
#include "map.hh"
#include "info.hh"
#include "cr.hh"
#include "containr.hh"
#include "utils.hh"
#include "reader.hh"
#include "time.hh"

enum TReaderThreadOrderType {
	ORDER_TERMINATE = 0,
	ORDER_LOADSECTOR = 1,
	ORDER_LOADCHARACTER = 2,
};

struct TReaderThreadOrder {
	TReaderThreadOrderType OrderType;
	int SectorX;
	int SectorY;
	int SectorZ;
	uint32_t CharacterID;
};

enum TReaderThreadReplyType {
	REPLY_SECTORDATA = 0,
	REPLY_CHARACTERDATA = 1,
};

struct TReaderThreadReply {
	TReaderThreadReplyType ReplyType;
	int SectorX;
	int SectorY;
	int SectorZ;
	unsigned char *Data;
	int Size;
};

const int ORDER_BUFFER_SIZE = 200;
const int REPLY_BUFFER_SIZE = 200;

// external functions that reader.hh cannot declare: they take an enum declared in this file
void InsertOrder(TReaderThreadOrderType OrderType, int SectorX, int SectorY, int SectorZ, uint32_t CharacterID);
void GetOrder(TReaderThreadOrderType &OrderType, int &SectorX, int &SectorY, int &SectorZ, uint32_t &CharacterID);
void InsertReply(TReaderThreadReplyType ReplyType, int SectorX, int SectorY, int SectorZ, unsigned char *Data, int Size);
bool GetReply(TReaderThreadReplyType &ReplyType, int &SectorX, int &SectorY, int &SectorZ, unsigned char *&Data, int &Size);

[[maybe_unused]] static Thread ReaderThread = 0;	// reader.c:60
[[maybe_unused]] static TReaderThreadOrder OrderBuffer[200];	// reader.c:50
[[maybe_unused]] static TReaderThreadReply ReplyBuffer[200];	// reader.c:51
[[maybe_unused]] static int OrderPointerWrite;	// reader.c:52
[[maybe_unused]] static int OrderPointerRead;	// reader.c:53
[[maybe_unused]] static int ReplyPointerWrite;	// reader.c:54
[[maybe_unused]] static int ReplyPointerRead;	// reader.c:55
[[maybe_unused]] static Semaphore OrderBufferEmpty(200);	// reader.c:57
[[maybe_unused]] static Semaphore OrderBufferFull(0);	// reader.c:58
[[maybe_unused]] static TDynamicWriteBuffer HelpBuffer(65536);	// reader.c:61

// 0x080df270  reader.c:73
void InitReaderBuffers() { abort(); /* STUB */ }

// 0x080df2a0  reader.c:85
void InsertOrder(TReaderThreadOrderType /*OrderType*/, int /*SectorX*/, int /*SectorY*/, int /*SectorZ*/, uint32_t /*CharacterID*/) { abort(); /* STUB */ }

// 0x080df370  reader.c:103
void GetOrder(TReaderThreadOrderType & /*OrderType*/, int & /*SectorX*/, int & /*SectorY*/, int & /*SectorZ*/, uint32_t & /*CharacterID*/) { abort(); /* STUB */ }

// 0x080df490  reader.c:119
void InsertReply(TReaderThreadReplyType /*ReplyType*/, int /*SectorX*/, int /*SectorY*/, int /*SectorZ*/, unsigned char * /*Data*/, int /*Size*/) { abort(); /* STUB */ }

// 0x080df560  reader.c:139
bool GetReply(TReaderThreadReplyType & /*ReplyType*/, int & /*SectorX*/, int & /*SectorY*/, int & /*SectorZ*/, unsigned char *& /*Data*/, int & /*Size*/) { abort(); /* STUB */ }

// 0x080df670  reader.c:162
void TerminateReaderOrder() { abort(); /* STUB */ }

// 0x080df720  reader.c:171
void LoadSectorOrder(int /*SectorX*/, int /*SectorY*/, int /*SectorZ*/) { abort(); /* STUB */ }

// 0x080df7f0  reader.c:179
void LoadCharacterOrder(uint32_t /*CharacterID*/) { abort(); /* STUB */ }

// 0x080df8a0  reader.c:193
void ProcessLoadSectorOrder(int /*SectorX*/, int /*SectorY*/, int /*SectorZ*/) { abort(); /* STUB */ }

// 0x080dfc10  reader.c:247
void ProcessLoadCharacterOrder(uint32_t /*CharacterID*/) { abort(); /* STUB */ }

// 0x080dfd50  reader.c:271
int ReaderThreadLoop(void *) { abort(); /* STUB */ }

// 0x080dfe30  reader.c:296
void ProcessSectorReply(TRefreshSectorFunction * /*RefreshSector*/, int /*SectorX*/, int /*SectorY*/, int /*SectorZ*/, unsigned char * /*Data*/, int /*Size*/) { abort(); /* STUB */ }

// 0x080dfe80  reader.c:307
void ProcessCharacterReply(TSendMailsFunction * /*SendMails*/, uint32_t /*CharacterID*/) { abort(); /* STUB */ }

// 0x080dfef0  reader.c:323
void ProcessReaderThreadReplies(TRefreshSectorFunction * /*RefreshSector*/, TSendMailsFunction * /*SendMails*/) { abort(); /* STUB */ }

// 0x080e0030  reader.c:349
void InitReader() { abort(); /* STUB */ }

// 0x080e00c0  reader.c:360
void ExitReader() { abort(); /* STUB */ }
