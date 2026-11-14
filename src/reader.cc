// reader.cc: the reader thread, which loads files away from the game thread:
// the sectors of the pristine map that a refresh restores, and the data of an
// offline character whose mail is to be delivered. Orders and replies pass
// through two ring buffers; the game thread handles the replies in
// ProcessReaderThreadReplies.

#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <unistd.h>

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
#include "crplayer.hh"
#include "shm.hh"

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

// The buffer functions are not static, but reader.hh cannot declare them: they take an enum declared in this file.
void InsertOrder(TReaderThreadOrderType OrderType, int SectorX, int SectorY, int SectorZ, uint32_t CharacterID);
void GetOrder(TReaderThreadOrderType &OrderType, int &SectorX, int &SectorY, int &SectorZ, uint32_t &CharacterID);
void InsertReply(TReaderThreadReplyType ReplyType, int SectorX, int SectorY, int SectorZ, unsigned char *Data, int Size);
bool GetReply(TReaderThreadReplyType &ReplyType, int &SectorX, int &SectorY, int &SectorZ, unsigned char *&Data, int &Size);

static Thread ReaderThread = 0;
static TReaderThreadOrder OrderBuffer[ORDER_BUFFER_SIZE];
static TReaderThreadReply ReplyBuffer[REPLY_BUFFER_SIZE];
static int OrderPointerWrite;
static int OrderPointerRead;
static int ReplyPointerWrite;
static int ReplyPointerRead;
static Semaphore OrderBufferEmpty(ORDER_BUFFER_SIZE);
static Semaphore OrderBufferFull(0);
static TDynamicWriteBuffer HelpBuffer(65536);

void InitReaderBuffers() {
	OrderPointerWrite = 0;
	OrderPointerRead = 0;
	ReplyPointerWrite = 0;
	ReplyPointerRead = 0;
}

void InsertOrder(TReaderThreadOrderType OrderType, int SectorX, int SectorY, int SectorZ, uint32_t CharacterID) {
	if (OrderPointerWrite - OrderPointerRead >= ORDER_BUFFER_SIZE) {
		error("InsertOrder (Reader): Order-Puffer ist voll => Vergr\xf6\xdf" "ern.\n");
	}

	OrderBufferEmpty.down();
	OrderBuffer[OrderPointerWrite % ORDER_BUFFER_SIZE].OrderType = OrderType;
	OrderBuffer[OrderPointerWrite % ORDER_BUFFER_SIZE].SectorX = SectorX;
	OrderBuffer[OrderPointerWrite % ORDER_BUFFER_SIZE].SectorY = SectorY;
	OrderBuffer[OrderPointerWrite % ORDER_BUFFER_SIZE].SectorZ = SectorZ;
	OrderBuffer[OrderPointerWrite % ORDER_BUFFER_SIZE].CharacterID = CharacterID;
	OrderPointerWrite++;
	OrderBufferFull.up();
}

void GetOrder(TReaderThreadOrderType &OrderType, int &SectorX, int &SectorY, int &SectorZ, uint32_t &CharacterID) {
	OrderBufferFull.down();
	OrderType = OrderBuffer[OrderPointerRead % ORDER_BUFFER_SIZE].OrderType;
	SectorX = OrderBuffer[OrderPointerRead % ORDER_BUFFER_SIZE].SectorX;
	SectorY = OrderBuffer[OrderPointerRead % ORDER_BUFFER_SIZE].SectorY;
	SectorZ = OrderBuffer[OrderPointerRead % ORDER_BUFFER_SIZE].SectorZ;
	CharacterID = OrderBuffer[OrderPointerRead % ORDER_BUFFER_SIZE].CharacterID;
	OrderPointerRead++;
	OrderBufferEmpty.up();
}

void InsertReply(TReaderThreadReplyType ReplyType, int SectorX, int SectorY, int SectorZ, unsigned char *Data, int Size) {
	while (ReplyPointerWrite - ReplyPointerRead >= REPLY_BUFFER_SIZE) {
		error("InsertReply (Reader): Puffer ist voll; warte...\n");
		DelayThread(5, 0);
	}

	ReplyBuffer[ReplyPointerWrite % REPLY_BUFFER_SIZE].ReplyType = ReplyType;
	ReplyBuffer[ReplyPointerWrite % REPLY_BUFFER_SIZE].SectorX = SectorX;
	ReplyBuffer[ReplyPointerWrite % REPLY_BUFFER_SIZE].SectorY = SectorY;
	ReplyBuffer[ReplyPointerWrite % REPLY_BUFFER_SIZE].SectorZ = SectorZ;
	ReplyBuffer[ReplyPointerWrite % REPLY_BUFFER_SIZE].Data = Data;
	ReplyBuffer[ReplyPointerWrite % REPLY_BUFFER_SIZE].Size = Size;
	ReplyPointerWrite++;
}

bool GetReply(TReaderThreadReplyType &ReplyType, int &SectorX, int &SectorY, int &SectorZ, unsigned char *&Data, int &Size) {
	if (ReplyPointerRead == ReplyPointerWrite) {
		return false;
	}

	ReplyType = ReplyBuffer[ReplyPointerRead % REPLY_BUFFER_SIZE].ReplyType;
	SectorX = ReplyBuffer[ReplyPointerRead % REPLY_BUFFER_SIZE].SectorX;
	SectorY = ReplyBuffer[ReplyPointerRead % REPLY_BUFFER_SIZE].SectorY;
	SectorZ = ReplyBuffer[ReplyPointerRead % REPLY_BUFFER_SIZE].SectorZ;
	Data = ReplyBuffer[ReplyPointerRead % REPLY_BUFFER_SIZE].Data;
	Size = ReplyBuffer[ReplyPointerRead % REPLY_BUFFER_SIZE].Size;
	ReplyPointerRead++;
	return true;
}

void TerminateReaderOrder() {
	InsertOrder(ORDER_TERMINATE, 0, 0, 0, 0);
}

void LoadSectorOrder(int SectorX, int SectorY, int SectorZ) {
	InsertOrder(ORDER_LOADSECTOR, SectorX, SectorY, SectorZ, 0);
}

void LoadCharacterOrder(uint32_t CharacterID) {
	InsertOrder(ORDER_LOADCHARACTER, 0, 0, 0, CharacterID);
}

void ProcessLoadSectorOrder(int SectorX, int SectorY, int SectorZ) {
	char FileName[4096];

	sprintf(FileName, "%s/%04u-%04u-%02u.sec", ORIGMAPPATH, SectorX, SectorY, SectorZ);
	if (!FileExists(FileName)) {
		return;
	}

	try {
		TReadScriptFile Script;

		Script.open(FileName);

		int xx = -1, yy = -1;
		bool Refreshable = false;
		HelpBuffer.rewind();
		while (true) {
			Script.nextToken();
			if (Script.Token == ENDOFFILE) {
				break;
			}

			if (Script.Token == SPECIAL && Script.getSpecial() == ',') {
				continue;
			}

			if (Script.Token == BYTES) {
				unsigned char *Help = Script.getBytesequence();
				xx = Help[0];
				yy = Help[1];
				Script.readSymbol(':');
				Refreshable = false;
			} else if (Script.Token == IDENTIFIER) {
				if (xx == -1) {
					Script.error("coordinate expected");
				}

				if (strcmp(Script.getIdentifier(), "refresh") == 0) {
					Refreshable = true;
				}

				if (strcmp(Script.getIdentifier(), "content") == 0) {
					Script.readSymbol('=');
					if (Refreshable) {
						HelpBuffer.writeByte((unsigned char)xx);
						HelpBuffer.writeByte((unsigned char)yy);
					}
					LoadObjects(Script, HelpBuffer, !Refreshable);
				}
			}
		}

		Script.close();

		int DataSize = HelpBuffer.getDataSize();
		if (DataSize != 0) {
			unsigned char *Data = new unsigned char[DataSize];
			memcpy(Data, HelpBuffer.getData(), DataSize);
			InsertReply(REPLY_SECTORDATA, SectorX, SectorY, SectorZ, Data, DataSize);
		}
	} catch (const char *s) {
		error("ProcessLoadSectorOrder: Fehler beim Laden der Datei (%s).\n", s);
	}
}

void ProcessLoadCharacterOrder(uint32_t CharacterID) {
again:
	TPlayerData *PlayerData = AssignPlayerPoolSlot(CharacterID, true);
	if (PlayerData == nullptr) {
		error("ProcessLoadCharacterOrder: Kann keinen Slot f\xfcr Spielerdaten zuweisen.\n");
		return;
	}

	// Locked is the thread ID (gettid) of the thread that holds the slot. If the game thread holds it, the order is
	// dropped; if another thread does, this one waits and tries again.
	if (PlayerData->Locked != gettid()) {
		if (PlayerData->Locked != GetGameThreadPID()) {
			DecreasePlayerPoolSlotSticky(PlayerData);
			DelayThread(1, 0);
			goto again;
		}
		return;
	}

	IncreasePlayerPoolSlotSticky(PlayerData);
	ReleasePlayerPoolSlot(PlayerData);
	InsertReply(REPLY_CHARACTERDATA, 0, 0, 0, nullptr, (int)CharacterID);
}

int ReaderThreadLoop(void *) {
	while (true) {
		TReaderThreadOrderType OrderType;
		int SectorX, SectorY, SectorZ;
		uint32_t CharacterID;

		GetOrder(OrderType, SectorX, SectorY, SectorZ, CharacterID);
		switch (OrderType) {
			case ORDER_TERMINATE:
				return 0;

			case ORDER_LOADSECTOR:
				ProcessLoadSectorOrder(SectorX, SectorY, SectorZ);
				break;

			case ORDER_LOADCHARACTER:
				ProcessLoadCharacterOrder(CharacterID);
				break;

			default:
				error("ReaderThreadLoop: Unbekanntes Kommando %d.\n", OrderType);
				break;
		}
	}
}

void ProcessSectorReply(TRefreshSectorFunction *RefreshSector, int SectorX, int SectorY, int SectorZ, unsigned char *Data, int Size) {
	RefreshSector(SectorX, SectorY, SectorZ, Data, Size);
	delete[] Data;
}

void ProcessCharacterReply(TSendMailsFunction *SendMails, uint32_t CharacterID) {
	TPlayerData *PlayerData = AttachPlayerPoolSlot(CharacterID, true);
	if (PlayerData == nullptr) {
		DecreasePlayerPoolSlotSticky(CharacterID);
		return;
	}

	SendMails(PlayerData);
	DecreasePlayerPoolSlotSticky(PlayerData);
	ReleasePlayerPoolSlot(PlayerData);
}

void ProcessReaderThreadReplies(TRefreshSectorFunction *RefreshSector, TSendMailsFunction *SendMails) {
	while (true) {
		TReaderThreadReplyType ReplyType;
		int SectorX, SectorY, SectorZ;
		unsigned char *Data;
		int Size;

		if (!GetReply(ReplyType, SectorX, SectorY, SectorZ, Data, Size)) {
			break;
		}

		switch (ReplyType) {
			case REPLY_SECTORDATA:
				ProcessSectorReply(RefreshSector, SectorX, SectorY, SectorZ, Data, Size);
				break;

			case REPLY_CHARACTERDATA:
				ProcessCharacterReply(SendMails, (uint32_t)Size);
				break;

			default:
				error("ProcessReaderThreadReplies: Unbekannte R\xfc" "ckmeldung %d.\n", ReplyType);
				break;
		}
	}
}

void InitReader() {
	InitReaderBuffers();
	ReaderThread = StartThread(ReaderThreadLoop, nullptr, false);
	if (ReaderThread == 0) {
		throw "cannot start reader thread";
	}
}

void ExitReader() {
	if (ReaderThread != 0) {
		TerminateReaderOrder();
		JoinThread(ReaderThread);
		ReaderThread = 0;
	}
}
