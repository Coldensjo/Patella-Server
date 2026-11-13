#ifndef PATELLA_WRITER_HH
#define PATELLA_WRITER_HH

#include <cstdint>

#include "containr.hh"

struct TCreature;
struct TPlayer;
struct TReportedStatement;

void InitProtocol();
void InsertProtocolOrder(char *ProtocolName, char *Text);
void GetProtocolOrder(char *ProtocolName, char *Text);
void WriteProtocol(char *ProtocolName, char *Text);
int ProtocolThreadLoop(void *);
void InitLog(char *ProtocolName);
void Log(char *ProtocolName, char *Text, ...);
void InitWriterBuffers();
int GetOrderBufferSpace();
void DirectReply(uint32_t CharacterID, char *Text, ...);
void LogoutReply(char *PlayerName);
void TerminateWriterOrder();
void LogoutOrder(TPlayer *Player);
void PlayerlistOrder(int NumberOfPlayers, char *PlayerNames, int *PlayerLevels, int *PlayerProfessions);
void KillStatisticsOrder(int NumberOfRaces, char *RaceNames, int *KilledPlayers, int *KilledCreatures);
void PunishmentOrder(TCreature *cr, char *Name, char *IPAddress, int Reason, int Action, char *Comment, int NumberOfStatements, vector<TReportedStatement> *ReportedStatements, uint32_t StatementID, bool IPBanishment);
void CharacterDeathOrder(TCreature *cr, int OldLevel, uint32_t Offender, char *Remark, bool Unjustified);
void AddBuddyOrder(TCreature *cr, uint32_t Buddy);
void RemoveBuddyOrder(TCreature *cr, uint32_t Buddy);
void DecrementIsOnlineOrder(uint32_t CharacterID);
void SavePlayerDataOrder();
void ProcessLogoutOrder(void *RawData);
void ProcessPlayerlistOrder(void *RawData);
void ProcessKillStatisticsOrder(void *RawData);
void ProcessPunishmentOrder(void *RawData);
void ProcessCharacterDeathOrder(void *RawData);
void ProcessAddBuddyOrder(void *RawData);
void ProcessRemoveBuddyOrder(void *RawData);
void ProcessDecrementIsOnlineOrder(void *RawData);
int WriterThreadLoop(void *);
void ProcessBroadcastReply(void *RawData);
void ProcessDirectReply(void *RawData);
void ProcessLogoutReply(void *RawData);
void ProcessWriterThreadReplies();
void ClearPlayers();
void InitWriter();
void AbortWriter();
void ExitWriter();

#endif
