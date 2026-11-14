#ifndef PATELLA_SENDING_HH
#define PATELLA_SENDING_HH

#include <cstdint>

#include "info.hh"

struct Object;
struct TConnection;

// The first byte of each packet the game server sends, named after the
// function that sends it (SendInitGame is SV_INIT_GAME). SV_LOGIN_ERROR,
// SV_LOGIN_MESSAGE and SV_LOGIN_WAITINGLIST are the Type of SendLoginMessage
// (communication.cc).
const int SV_INIT_GAME = 0x0A;
const int SV_RIGHTS = 0x0B;
const int SV_LOGIN_ERROR = 0x14;
const int SV_LOGIN_MESSAGE = 0x15;
const int SV_LOGIN_WAITINGLIST = 0x16;
const int SV_PING = 0x1E;
const int SV_FULL_SCREEN = 0x64;
const int SV_ROW_NORTH = 0x65;
const int SV_ROW_EAST = 0x66;
const int SV_ROW_SOUTH = 0x67;
const int SV_ROW_WEST = 0x68;
const int SV_FIELD_DATA = 0x69;
const int SV_ADD_FIELD = 0x6A;
const int SV_CHANGE_FIELD = 0x6B;
const int SV_DELETE_FIELD = 0x6C;
const int SV_MOVE_CREATURE = 0x6D;
const int SV_CONTAINER = 0x6E;
const int SV_CLOSE_CONTAINER = 0x6F;
const int SV_CREATE_IN_CONTAINER = 0x70;
const int SV_CHANGE_IN_CONTAINER = 0x71;
const int SV_DELETE_IN_CONTAINER = 0x72;
const int SV_SET_INVENTORY = 0x78;
const int SV_DELETE_INVENTORY = 0x79;
const int SV_TRADE_OFFER_OWN = 0x7D;
const int SV_TRADE_OFFER_PARTNER = 0x7E;
const int SV_CLOSE_TRADE = 0x7F;
const int SV_AMBIENTE = 0x82;
const int SV_GRAPHICAL_EFFECT = 0x83;
const int SV_TEXTUAL_EFFECT = 0x84;
const int SV_MISSILE_EFFECT = 0x85;
const int SV_MARK_CREATURE = 0x86;
const int SV_CREATURE_HEALTH = 0x8C;
const int SV_CREATURE_LIGHT = 0x8D;
const int SV_CREATURE_OUTFIT = 0x8E;
const int SV_CREATURE_SPEED = 0x8F;
const int SV_CREATURE_SKULL = 0x90;
const int SV_CREATURE_PARTY = 0x91;
const int SV_EDIT_TEXT = 0x96;
const int SV_EDIT_LIST = 0x97;
const int SV_PLAYER_DATA = 0xA0;
const int SV_PLAYER_SKILLS = 0xA1;
const int SV_PLAYER_STATE = 0xA2;
const int SV_CLEAR_TARGET = 0xA3;
const int SV_TALK = 0xAA;
const int SV_CHANNELS = 0xAB;
const int SV_OPEN_CHANNEL = 0xAC;
const int SV_PRIVATE_CHANNEL = 0xAD;
const int SV_OPEN_REQUEST_QUEUE = 0xAE;
const int SV_DELETE_REQUEST = 0xAF;
const int SV_FINISH_REQUEST = 0xB0;
const int SV_CLOSE_REQUEST = 0xB1;
const int SV_OPEN_OWN_CHANNEL = 0xB2;
const int SV_CLOSE_CHANNEL = 0xB3;
const int SV_MESSAGE = 0xB4;
const int SV_SNAPBACK = 0xB5;
const int SV_FLOORS_UP = 0xBE;
const int SV_FLOORS_DOWN = 0xBF;
const int SV_OUTFIT = 0xC8;
const int SV_BUDDY_DATA = 0xD2;
const int SV_BUDDY_ONLINE = 0xD3;
const int SV_BUDDY_OFFLINE = 0xD4;

// The word SendMapObject sends in place of a creature's object type: a
// creature the client does not know yet (with all its data), one whose data
// is outdated, or objects.hh's TYPEID_CREATURE (0x63), one it knows. The names
// are the client's.
const int UNKNOWNCREATURE = 0x61;
const int OUTDATEDCREATURE = 0x62;

bool BeginSendData(TConnection *Connection);
void FinishSendData(TConnection *Connection);
void SendAll();
void SkipFlush(TConnection *Connection);
void SendMapObject(TConnection *Connection, Object Obj);
void SendMapPoint(TConnection *Connection, int x, int y, int z);
void SendResult(TConnection *Connection, RESULT r);
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
// The lists SendEditList sends and receiving.cc's CEditList changes: a house's
// guests, its subowners (both by house ID) and the names on a name door (by
// object ID).
const int EDIT_LIST_HOUSE_GUESTS = 1;
const int EDIT_LIST_HOUSE_SUBOWNERS = 2;
const int EDIT_LIST_NAME_DOOR = 3;

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
