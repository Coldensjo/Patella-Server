#ifndef PATELLA_READER_HH
#define PATELLA_READER_HH

#include <cstdint>

struct TPlayerData;

typedef void TRefreshSectorFunction(int, int, int, unsigned char *, int);

typedef void TSendMailsFunction(TPlayerData *);

void InitReaderBuffers();
void TerminateReaderOrder();
void LoadSectorOrder(int SectorX, int SectorY, int SectorZ);
void LoadCharacterOrder(uint32_t CharacterID);
void ProcessLoadSectorOrder(int SectorX, int SectorY, int SectorZ);
void ProcessLoadCharacterOrder(uint32_t CharacterID);
int ReaderThreadLoop(void *);
void ProcessSectorReply(TRefreshSectorFunction *RefreshSector, int SectorX, int SectorY, int SectorZ, unsigned char *Data, int Size);
void ProcessCharacterReply(TSendMailsFunction *SendMails, uint32_t CharacterID);
void ProcessReaderThreadReplies(TRefreshSectorFunction *RefreshSector, TSendMailsFunction *SendMails);
void InitReader();
void ExitReader();

#endif
