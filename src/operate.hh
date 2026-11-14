// operate.hh: the operations of the game world (operate.cc), the talk modes
// and chat channels, and the numbers of the client's magic effects, missiles
// and colours.

#ifndef PATELLA_OPERATE_HH
#define PATELLA_OPERATE_HH

#include <cstdint>

#include "objects.hh"
#include "containr.hh"

struct Object;
struct TChannel;
struct TListener;
struct TParty;
struct TPlayer;
struct TReadScriptFile;
struct TStatement;

enum TALK_MODE {
	TALK_SAY = 1,
	TALK_WHISPER = 2,
	TALK_YELL = 3,
	TALK_PRIVATE_MESSAGE = 4,
	TALK_CHANNEL_CALL = 5,
	TALK_GAMEMASTER_REQUEST = 6,
	TALK_GAMEMASTER_ANSWER = 7,
	TALK_PLAYER_ANSWER = 8,
	TALK_GAMEMASTER_BROADCAST = 9,
	TALK_GAMEMASTER_CHANNELCALL = 10,
	TALK_GAMEMASTER_MESSAGE = 11,
	TALK_HIGHLIGHT_CHANNELCALL = 12,
	TALK_ANONYMOUS_BROADCAST = 13,
	TALK_ANONYMOUS_CHANNELCALL = 14,
	TALK_ANONYMOUS_MESSAGE = 15,
	TALK_ANIMAL_LOW = 16,
	TALK_ANIMAL_LOUD = 17,
	TALK_ADMIN_MESSAGE = 18,
	TALK_EVENT_MESSAGE = 19,
	TALK_LOGIN_MESSAGE = 20,
	TALK_STATUS_MESSAGE = 21,
	TALK_INFO_MESSAGE = 22,
	TALK_FAILURE_MESSAGE = 23,
};

enum CHANNEL {
	GUILD_CHANNEL = 0,
	GAMEMASTER_CHANNEL = 1,
	TUTOR_CHANNEL = 2,
	REQUEST_QUEUE = 3,
	GAME_CHANNEL = 4,
	TRADE_CHANNEL = 5,
	REALLIFE_CHANNEL = 6,
	HELP_CHANNEL = 7,
};

const int PUBLIC_CHANNELS = 8;

// What happened to an object: the Type of AnnounceChangedObject (and of
// AnnounceChangedField, AnnounceChangedContainer, AnnounceChangedInventory),
// of NotifyAllCreatures and of CreatureMoveStimulus. OBJECT_MOVED is only
// NotifyAllCreatures': it tells the creatures near the old and the new place
// with OBJECT_CHANGED.
const int OBJECT_DELETED = 0;
const int OBJECT_CREATED = 1;
const int OBJECT_CHANGED = 2;
const int OBJECT_MOVED = 3;

// What changed about a creature: the Type of AnnounceChangedCreature, each
// sent by the Send function of the same name (SendCreatureHealth, ...).
const int CREATURE_HEALTH = 1;
const int CREATURE_LIGHT = 2;
const int CREATURE_OUTFIT = 3;
const int CREATURE_SPEED = 4;
const int CREATURE_SKULL = 5;
const int CREATURE_PARTY = 6;

// The client's magic effects (GraphicalEffect), named by what they show and
// where the server uses them. Teleports and the energy area are one effect.
const int EFFECT_BLOOD_HIT = 1;
const int EFFECT_MANA_HIT = 2;
const int EFFECT_POFF = 3;
const int EFFECT_BLOCK_HIT = 4;
const int EFFECT_EXPLOSION_AREA = 5;
const int EFFECT_EXPLOSION = 6;
const int EFFECT_FIRE_AREA = 7;
const int EFFECT_POISON = 9;
const int EFFECT_HIT_AREA = 10;
const int EFFECT_TELEPORT = 11;
const int EFFECT_ENERGY_HIT = 12;
const int EFFECT_MAGIC_BLUE = 13;
const int EFFECT_MAGIC_RED = 14;
const int EFFECT_MAGIC_GREEN = 15;
const int EFFECT_FIRE_HIT = 16;
const int EFFECT_POISON_HIT = 17;
const int EFFECT_DEATH = 18;

// The client's missiles (Missile) that the code names itself; the others come
// from the data (THROWMISSILE, AMMOMISSILE, WANDMISSILE, the spells of the
// monsters).
const int ANIMATION_NONE = 0;
const int ANIMATION_FIRE = 4;
const int ANIMATION_ENERGY = 5;
const int ANIMATION_DEATH = 11;

// Colours of the client's 216-colour palette (36 * red + 6 * green + blue,
// each 0 to 5): the Color of TextualEffect and SendMarkCreature, and of the
// world light (GetAmbiente).
const int COLOR_BLACK = 0;
const int COLOR_BLUE = 5;
const int COLOR_GREEN = 30;
const int COLOR_LIGHTBLUE = 35;
const int COLOR_LIGHTGRAY = 129;
const int COLOR_RED = 180;
const int COLOR_ORANGE = 198;
const int COLOR_WHITE = 215;

struct TReportedStatement {
	uint32_t StatementID;
	uint32_t TimeStamp;
	uint32_t CharacterID;
	int Mode;
	int Channel;
	char Text[256];
};

void AnnounceMovingCreature(uint32_t CreatureID, Object Con);
void AnnounceChangedCreature(uint32_t CreatureID, int Type);
void AnnounceChangedField(Object Obj, int Type);
void AnnounceChangedContainer(Object Obj, int Type);
void AnnounceChangedInventory(Object Obj, int Type);
void AnnounceChangedObject(Object Obj, int Type);
void AnnounceGraphicalEffect(int x, int y, int z, int Type);
void AnnounceTextualEffect(int x, int y, int z, int Color, char *Text);
void AnnounceMissile(int sx, int sy, int sz, int zx, int zy, int zz, int Type);
void CheckTopMoveObject(uint32_t CreatureID, Object Obj, Object Ignore);
void CheckTopUseObject(uint32_t CreatureID, Object Obj);
void CheckTopMultiuseObject(uint32_t CreatureID, Object Obj);
void CheckMoveObject(uint32_t CreatureID, Object Obj, bool Take);
void CheckMapDestination(uint32_t CreatureID, Object Obj, Object MapCon);
void CheckMapPlace(uint32_t CreatureID, ObjectType Type, Object MapCon);
void CheckContainerDestination(Object Obj, Object Con);
void CheckContainerPlace(ObjectType Type, Object Con, Object oldObj);
void CheckDepotSpace(uint32_t CreatureID, Object Source, Object Destination, int Count);
void CheckInventoryDestination(Object Obj, Object Con, bool Split);
void CheckInventoryPlace(ObjectType Type, Object Con, Object OldObj);
void CheckWeight(uint32_t CreatureID, Object Obj, int Count);
void CheckWeight(uint32_t CreatureID, ObjectType Type, uint32_t Data, int oldWeight);
void NotifyCreature(uint32_t CreatureID, Object Obj, bool Inventory);
void NotifyCreature(uint32_t CreatureID, ObjectType Type, bool Inventory);
void NotifyAllCreatures(Object Obj, int Type, Object OldCon);
void NotifyTrades(Object Obj);
void NotifyDepot(uint32_t CreatureID, Object Obj, int Count);
void CloseContainer(Object Con, bool Force);
Object Create(Object Con, ObjectType Type, uint32_t Value);
Object Copy(Object Con, Object Source);
void Move(uint32_t CreatureID, Object Obj, Object Con, int Count, bool NoMerge, Object Ignore);
void Merge(uint32_t CreatureID, Object Obj, Object Dest, int Count, Object Ignore);
void Change(Object Obj, ObjectType NewType, uint32_t Value);
void Change(Object Obj, INSTANCEATTRIBUTE Attribute, uint32_t Value);
void Delete(Object Obj, int Count);
void Empty(Object Con, int Rest);
void GraphicalEffect(int x, int y, int z, int Type);
void GraphicalEffect(Object Obj, int Type);
void TextualEffect(Object Obj, int Color, char *Text, ...);
void Missile(Object Start, Object Dest, int Type);
void Look(uint32_t CreatureID, Object Obj);
void Talk(uint32_t CreatureID, int Mode, char *Addressee, char *Text, bool CheckSpamming);
void Use(uint32_t CreatureID, Object Obj1, Object Obj2, unsigned char Info);
void Turn(uint32_t CreatureID, Object Obj);
void CreatePool(Object MapCon, ObjectType Type, uint32_t LiquidType);
void EditText(uint32_t CreatureID, Object Obj, char *Text);
Object CreateAtCreature(uint32_t CreatureID, ObjectType Type, uint32_t Value);
void DeleteAtCreature(uint32_t CreatureID, ObjectType Type, int Amount, uint32_t Data);
void ProcessCronSystem();
bool SectorRefreshable(int SectorX, int SectorY, int SectorZ);
void RefreshSector(int SectorX, int SectorY, int SectorZ, unsigned char *Data, int Size);
void RefreshMap();
void RefreshCylinders();
void ApplyPatch(int SectorX, int SectorY, int SectorZ, bool FullSector, TReadScriptFile &Script, bool SaveHouses);
void ApplyPatches();
int GetNumberOfChannels();
bool ChannelActive(int ChannelID);
bool ChannelAvailable(int ChannelID, uint32_t CharacterID);
char *GetChannelName(int ChannelID, uint32_t CharacterID);
bool ChannelSubscribed(int ChannelID, uint32_t CharacterID);
uint32_t GetFirstSubscriber(int ChannelID);
uint32_t GetNextSubscriber();
bool MayOpenChannel(uint32_t CharacterID);
void OpenChannel(uint32_t CharacterID);
void CloseChannel(int ChannelID);
void InviteToChannel(uint32_t CharacterID, char *Name);
void ExcludeFromChannel(uint32_t CharacterID, char *Name);
bool JoinChannel(int ChannelID, uint32_t CharacterID);
void LeaveChannel(int ChannelID, uint32_t CharacterID, bool Close);
void LeaveAllChannels(uint32_t CharacterID);
uint32_t LogCommunication(uint32_t CharacterID, int Mode, int Channel, char *Text);
uint32_t LogListener(uint32_t StatementID, TPlayer *pl);
void ProcessCommunicationControl();
int GetCommunicationContext(uint32_t CharacterID, uint32_t StatementID, int &NumberOfStatements, vector<TReportedStatement> *&ReportedStatements);
TParty *GetParty(uint32_t Leader);
bool IsInvitedToParty(uint32_t Guest, uint32_t Host);
void DisbandParty(uint32_t Leader);
void InviteToParty(uint32_t Host, uint32_t Guest);
void RevokeInvitation(uint32_t Host, uint32_t Guest);
void JoinParty(uint32_t Guest, uint32_t Host);
void PassLeadership(uint32_t OldLeader, uint32_t NewLeader);
void LeaveParty(uint32_t Member, bool Forced);

extern int Channels;
extern int StatementID;
extern int Parties;
extern vector<TChannel> Channel;
extern int CurrentChannelID;
extern int CurrentSubscriberNumber;
extern fifo<TStatement> Statements;
extern fifo<TListener> Listeners;
extern vector<TParty> Party;

#endif
