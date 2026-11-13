#ifndef PATELLA_SENDING_HH
#define PATELLA_SENDING_HH

#include <cstdint>

#include "info.hh"

struct Object;
struct TConnection;

bool BeginSendData(TConnection *Connection);
void FinishSendData(TConnection *Connection);
void SendAll();
void SkipFlush(TConnection *Connection);
void SendMapObject(TConnection *Connection, Object Obj);
void SendMapPoint(TConnection *Connection, int x, int y, int z);
void SendResult(TConnection *Connection, RESULT r);
void SendRefresh(TConnection *Connection);
void SendInitGame(TConnection *Connection, uint32_t id);
void SendRights(TConnection *Connection);
void SendPing(TConnection *Connection);
void SendFullScreen(TConnection *Connection);
void SendRow(TConnection *Connection, int Direction);
void SendFloors(TConnection *Connection, bool Up);
void SendFieldData(TConnection *Connection, int x, int y, int z);
void SendCloseContainer(TConnection *Connection, int ContNr);
void SendContainer(TConnection *Connection, int ContNr);
void SendCreateInContainer(TConnection *Connection, int ContNr, Object Obj);
void SendChangeInContainer(TConnection *Connection, int ContNr, Object Obj);
void SendDeleteInContainer(TConnection *Connection, int ContNr, Object Obj);
void SendBodyInventory(TConnection *Connection, uint32_t CreatureID);
void SendSetInventory(TConnection *Connection, int Position, Object Obj);
void SendDeleteInventory(TConnection *Connection, int Position);
void SendTradeObjects(TConnection *Connection, Object Obj);
void SendTradeOffer(TConnection *Connection, char *Name, bool OwnOffer, Object Obj);
void SendCloseTrade(TConnection *Connection);
void SendAddField(TConnection *Connection, int x, int y, int z, Object Obj);
void SendDeleteField(TConnection *Connection, int x, int y, int z, Object Obj);
void SendChangeField(TConnection *Connection, int x, int y, int z, Object Obj);
void SendMoveCreature(TConnection *Connection, uint32_t CreatureID, int x, int y, int z);
void SendGraphicalEffect(TConnection *Connection, int x, int y, int z, int Type);
void SendTextualEffect(TConnection *Connection, int x, int y, int z, int Color, char *Text);
void SendMissileEffect(TConnection *Connection, int sx, int sy, int sz, int zx, int zy, int zz, int Type);
void SendMarkCreature(TConnection *Connection, uint32_t CreatureID, int Color);
void SendEditText(TConnection *Connection, Object Obj);
void SendEditList(TConnection *Connection, unsigned char Type, uint32_t ID, char *Text);
void SendAmbiente(TConnection *Connection);
void SendCreatureHealth(TConnection *Connection, uint32_t CreatureID);
void SendCreatureLight(TConnection *Connection, uint32_t CreatureID);
void SendCreatureOutfit(TConnection *Connection, uint32_t CreatureID);
void SendCreatureSpeed(TConnection *Connection, uint32_t CreatureID);
void SendCreatureSkull(TConnection *Connection, uint32_t CreatureID);
void SendCreatureParty(TConnection *Connection, uint32_t CreatureID);
void SendClearTarget(TConnection *Connection);
void SendPlayerData(TConnection *Connection);
void SendPlayerSkills(TConnection *Connection);
void SendPlayerState(TConnection *Connection, unsigned char State);
void SendMessage(TConnection *Connection, int Mode, char *Text, ...);
void SendSnapback(TConnection *Connection);
void SendTalk(TConnection *Connection, uint32_t StatementID, char *Sender, int Mode, char *Text, int Data);
void SendTalk(TConnection *Connection, uint32_t StatementID, char *Sender, int Mode, int Channel, char *Text);
void SendTalk(TConnection *Connection, uint32_t StatementID, char *Sender, int Mode, int x, int y, int z, char *Text);
void SendChannels(TConnection *Connection);
void SendOpenChannel(TConnection *Connection, int Channel);
void SendPrivateChannel(TConnection *Connection, char *Name);
void SendOpenRequestQueue(TConnection *Connection);
void SendDeleteRequest(TConnection *Connection, char *Name);
void SendFinishRequest(TConnection *Connection, char *Name);
void SendCloseRequest(TConnection *Connection);
void SendOpenOwnChannel(TConnection *Connection, int Channel);
void SendCloseChannel(TConnection *Connection, int Channel);
void SendOutfit(TConnection *Connection);
void SendBuddyData(TConnection *Connection, uint32_t CharacterID, char *Name, bool Online);
void SendBuddyStatus(TConnection *Connection, uint32_t CharacterID, bool Online);
void BroadcastMessage(int Mode, char *Text, ...);
void DeleteGamemasterRequest(char *Name);

#endif
