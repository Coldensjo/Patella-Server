#include <cstdint>
#include <cstdlib>

#include "config.hh"
#include "script.hh"
#include "threads.hh"
#include "dbfuncs.hh"
#include "objects.hh"
#include "map.hh"
#include "info.hh"
#include "cr.hh"
#include "containr.hh"
#include "connections.hh"
#include "operate.hh"
#include "query.hh"
#include "utils.hh"
#include "time.hh"
#include "reader.hh"
#include "TXTEASymmetricKey.hh"

struct TChannel {
	uint32_t Moderator;
	char ModeratorName[30];
	vector<uint32_t> Subscriber;
	int Subscribers;
	vector<uint32_t> InvitedPlayer;
	int InvitedPlayers;
	// 0x0809cc80  operate.c:2155
	TChannel() : Subscriber(0, 10, 10), InvitedPlayer(0, 10, 10) { abort(); /* STUB */ }
};

struct TStatement {
	uint32_t StatementID;
	uint32_t TimeStamp;
	uint32_t CharacterID;
	int Mode;
	int Channel;
	uint32_t Text;
	bool Reported;
};

struct TListener {
	uint32_t StatementID;
	uint32_t CharacterID;
};

struct TParty {
	uint32_t Leader;
	vector<uint32_t> Member;
	int Members;
	vector<uint32_t> InvitedPlayer;
	int InvitedPlayers;
	// 0x0809cd10  operate.c:2663
	TParty() : Member(0, 10, 10), InvitedPlayer(0, 10, 10) { abort(); /* STUB */ }
};

int Channels = 8;	// operate.c:2159
int StatementID = 0;	// operate.c:2507
int Parties = 0;	// operate.c:2667
vector<TChannel> Channel(0, 18, 10);	// operate.c:2158
int CurrentChannelID;	// operate.c:2160
int CurrentSubscriberNumber;	// operate.c:2161
fifo<TStatement> Statements(1024);	// operate.c:2508
fifo<TListener> Listeners(1024);	// operate.c:2509
vector<TParty> Party(0, 100, 50);	// operate.c:2666

// 0x0808e2c0  operate.c:42
void AnnounceMovingCreature(uint32_t /*CreatureID*/, Object /*Con*/) { abort(); /* STUB */ }

// 0x0808e400  operate.c:65
void AnnounceChangedCreature(uint32_t /*CreatureID*/, int /*Type*/) { abort(); /* STUB */ }

// 0x0808e530  operate.c:95
void AnnounceChangedField(Object /*Obj*/, int /*Type*/) { abort(); /* STUB */ }

// 0x0808e6d0  operate.c:122
void AnnounceChangedContainer(Object /*Obj*/, int /*Type*/) { abort(); /* STUB */ }

// 0x0808e830  operate.c:149
void AnnounceChangedInventory(Object /*Obj*/, int /*Type*/) { abort(); /* STUB */ }

// 0x0808e920  operate.c:173
void AnnounceChangedObject(Object /*Obj*/, int /*Type*/) { abort(); /* STUB */ }

// 0x0808ea10  operate.c:187
void AnnounceGraphicalEffect(int /*x*/, int /*y*/, int /*z*/, int /*Type*/) { abort(); /* STUB */ }

// 0x0808eae0  operate.c:203
void AnnounceTextualEffect(int /*x*/, int /*y*/, int /*z*/, int /*Color*/, char * /*Text*/) { abort(); /* STUB */ }

// 0x0808ebc0  operate.c:219
void AnnounceMissile(int /*sx*/, int /*sy*/, int /*sz*/, int /*zx*/, int /*zy*/, int /*zz*/, int /*Type*/) { abort(); /* STUB */ }

// 0x0808ed40  operate.c:241
void CheckTopMoveObject(uint32_t /*CreatureID*/, Object /*Obj*/, Object /*Ignore*/) { abort(); /* STUB */ }

// 0x0808f010  operate.c:265
void CheckTopUseObject(uint32_t /*CreatureID*/, Object /*Obj*/) { abort(); /* STUB */ }

// 0x0808f2b0  operate.c:288
void CheckTopMultiuseObject(uint32_t /*CreatureID*/, Object /*Obj*/) { abort(); /* STUB */ }

// 0x0808f520  operate.c:310
void CheckMoveObject(uint32_t /*CreatureID*/, Object /*Obj*/, bool /*Take*/) { abort(); /* STUB */ }

// 0x0808f6c0  operate.c:330
void CheckMapDestination(uint32_t /*CreatureID*/, Object /*Obj*/, Object /*MapCon*/) { abort(); /* STUB */ }

// 0x0808fe90  operate.c:372
void CheckMapPlace(uint32_t /*CreatureID*/, ObjectType /*Type*/, Object /*MapCon*/) { abort(); /* STUB */ }

// 0x08090140  operate.c:389
void CheckContainerDestination(Object /*Obj*/, Object /*Con*/) { abort(); /* STUB */ }

// 0x08090260  operate.c:404
void CheckContainerPlace(ObjectType /*Type*/, Object /*Con*/, Object /*oldObj*/) { abort(); /* STUB */ }

// 0x080903b0  operate.c:418
void CheckDepotSpace(uint32_t /*CreatureID*/, Object /*Source*/, Object /*Destination*/, int /*Count*/) { abort(); /* STUB */ }

// 0x08090560  operate.c:446
void CheckInventoryDestination(Object /*Obj*/, Object /*Con*/, bool /*Split*/) { abort(); /* STUB */ }

// 0x080909e0  operate.c:478
void CheckInventoryPlace(ObjectType /*Type*/, Object /*Con*/, Object /*OldObj*/) { abort(); /* STUB */ }

// 0x08090ec0  operate.c:507
void CheckWeight(uint32_t /*CreatureID*/, Object /*Obj*/, int /*Count*/) { abort(); /* STUB */ }

// 0x08091090  operate.c:531
void CheckWeight(uint32_t /*CreatureID*/, ObjectType /*Type*/, uint32_t /*Data*/, int /*oldWeight*/) { abort(); /* STUB */ }

// 0x08091210  operate.c:561
void NotifyCreature(uint32_t /*CreatureID*/, Object /*Obj*/, bool /*Inventory*/) { abort(); /* STUB */ }

// 0x080912c0  operate.c:577
void NotifyCreature(uint32_t /*CreatureID*/, ObjectType /*Type*/, bool /*Inventory*/) { abort(); /* STUB */ }

// 0x08091360  operate.c:594
void NotifyAllCreatures(Object /*Obj*/, int /*Type*/, Object /*OldCon*/) { abort(); /* STUB */ }

// 0x080916a0  operate.c:644
void NotifyTrades(Object /*Obj*/) { abort(); /* STUB */ }

// 0x080918c0  operate.c:680
void NotifyDepot(uint32_t /*CreatureID*/, Object /*Obj*/, int /*Count*/) { abort(); /* STUB */ }

// 0x08091a00  operate.c:711
void CloseContainer(Object /*Con*/, bool /*Force*/) { abort(); /* STUB */ }

// 0x08091bf0  operate.c:752
Object Create(Object /*Con*/, ObjectType /*Type*/, uint32_t /*Value*/) { abort(); /* STUB */ }

// 0x08092350  operate.c:821
Object Copy(Object /*Con*/, Object /*Source*/) { abort(); /* STUB */ }

// 0x08092750  operate.c:872
void Move(uint32_t /*CreatureID*/, Object /*Obj*/, Object /*Con*/, int /*Count*/, bool /*NoMerge*/, Object /*Ignore*/) { abort(); /* STUB */ }

// 0x08093130  operate.c:974
void Merge(uint32_t /*CreatureID*/, Object /*Obj*/, Object /*Dest*/, int /*Count*/, Object /*Ignore*/) { abort(); /* STUB */ }

// 0x080936f0  operate.c:1036
void Change(Object /*Obj*/, ObjectType /*NewType*/, uint32_t /*Value*/) { abort(); /* STUB */ }

// 0x08093f50  operate.c:1098
void Change(Object /*Obj*/, INSTANCEATTRIBUTE /*Attribute*/, uint32_t /*Value*/) { abort(); /* STUB */ }

// 0x08094170  operate.c:1130
void Delete(Object /*Obj*/, int /*Count*/) { abort(); /* STUB */ }

// 0x08094530  operate.c:1184
void Empty(Object /*Con*/, int /*Rest*/) { abort(); /* STUB */ }

// 0x080946e0  operate.c:1212
void GraphicalEffect(int /*x*/, int /*y*/, int /*z*/, int /*Type*/) { abort(); /* STUB */ }

// 0x080946f0  operate.c:1220
void GraphicalEffect(Object /*Obj*/, int /*Type*/) { abort(); /* STUB */ }

// 0x08094760  operate.c:1231
void TextualEffect(Object /*Obj*/, int /*Color*/, char * /*Text*/, ...) { abort(); /* STUB */ }

// 0x08094850  operate.c:1253
void Missile(Object /*Start*/, Object /*Dest*/, int /*Type*/) { abort(); /* STUB */ }

// 0x08094930  operate.c:1273
void Look(uint32_t /*CreatureID*/, Object /*Obj*/) { abort(); /* STUB */ }

// 0x08095fe0  operate.c:1477
void Talk(uint32_t /*CreatureID*/, int /*Mode*/, char * /*Addressee*/, char * /*Text*/, bool /*CheckSpamming*/) { abort(); /* STUB */ }

// 0x08097090  operate.c:1660
void Use(uint32_t /*CreatureID*/, Object /*Obj1*/, Object /*Obj2*/, unsigned char /*Info*/) { abort(); /* STUB */ }

// 0x080978f0  operate.c:1712
void Turn(uint32_t /*CreatureID*/, Object /*Obj*/) { abort(); /* STUB */ }

// 0x08097a80  operate.c:1730
void CreatePool(Object /*MapCon*/, ObjectType /*Type*/, uint32_t /*LiquidType*/) { abort(); /* STUB */ }

// 0x08097c90  operate.c:1760
void EditText(uint32_t /*CreatureID*/, Object /*Obj*/, char * /*Text*/) { abort(); /* STUB */ }

// 0x08097f50  operate.c:1797
Object CreateAtCreature(uint32_t /*CreatureID*/, ObjectType /*Type*/, uint32_t /*Value*/) { abort(); /* STUB */ }

// 0x08098250  operate.c:1835
void DeleteAtCreature(uint32_t /*CreatureID*/, ObjectType /*Type*/, int /*Amount*/, uint32_t /*Data*/) { abort(); /* STUB */ }

// 0x080983f0  operate.c:1859
void ProcessCronSystem() { abort(); /* STUB */ }

// 0x080985b0  operate.c:1889
bool SectorRefreshable(int /*SectorX*/, int /*SectorY*/, int /*SectorZ*/) { abort(); /* STUB */ }

// 0x08098660  operate.c:1909
void RefreshSector(int /*SectorX*/, int /*SectorY*/, int /*SectorZ*/, unsigned char * /*Data*/, int /*Size*/) { abort(); /* STUB */ }

// 0x08098a50  operate.c:1957
void RefreshMap() { abort(); /* STUB */ }

// 0x08098df0  operate.c:2014
// local static: int RefreshX = 2147483647  (operate.c:2015, 0x08127bc8 .data)
// local static: int RefreshY = 2147483647  (operate.c:2016, 0x08127bcc .data)
void RefreshCylinders() { abort(); /* STUB */ }

// 0x08098ec0  operate.c:2032
void ApplyPatch(int /*SectorX*/, int /*SectorY*/, int /*SectorZ*/, bool /*FullSector*/, TReadScriptFile & /*Script*/, bool /*SaveHouses*/) { abort(); /* STUB */ }

// 0x08099380  operate.c:2088
void ApplyPatches() { abort(); /* STUB */ }

// 0x080997b0  operate.c:2167
int GetNumberOfChannels() { abort(); /* STUB */ }

// 0x080997c0  operate.c:2175
bool ChannelActive(int /*ChannelID*/) { abort(); /* STUB */ }

// 0x08099820  operate.c:2188
bool ChannelAvailable(int /*ChannelID*/, uint32_t /*CharacterID*/) { abort(); /* STUB */ }

// 0x08099950  operate.c:2218
// local static: char ChannelName[40]  (operate.c:2240, 0x081c3720 .bss)
char *GetChannelName(int /*ChannelID*/, uint32_t /*CharacterID*/) { abort(); /* STUB */ }

// 0x08099a80  operate.c:2249
bool ChannelSubscribed(int /*ChannelID*/, uint32_t /*CharacterID*/) { abort(); /* STUB */ }

// 0x08099b30  operate.c:2261
uint32_t GetFirstSubscriber(int /*ChannelID*/) { abort(); /* STUB */ }

// 0x08099c40  operate.c:2273
uint32_t GetNextSubscriber() { abort(); /* STUB */ }

// 0x08099ce0  operate.c:2284
bool MayOpenChannel(uint32_t /*CharacterID*/) { abort(); /* STUB */ }

// 0x08099d60  operate.c:2296
void OpenChannel(uint32_t /*CharacterID*/) { abort(); /* STUB */ }

// 0x08099f10  operate.c:2330
void CloseChannel(int /*ChannelID*/) { abort(); /* STUB */ }

// 0x08099f70  operate.c:2347
void InviteToChannel(uint32_t /*CharacterID*/, char * /*Name*/) { abort(); /* STUB */ }

// 0x0809a240  operate.c:2391
void ExcludeFromChannel(uint32_t /*CharacterID*/, char * /*Name*/) { abort(); /* STUB */ }

// 0x0809a520  operate.c:2441
bool JoinChannel(int /*ChannelID*/, uint32_t /*CharacterID*/) { abort(); /* STUB */ }

// 0x0809a650  operate.c:2459
void LeaveChannel(int /*ChannelID*/, uint32_t /*CharacterID*/, bool /*Close*/) { abort(); /* STUB */ }

// 0x0809a800  operate.c:2481
void LeaveAllChannels(uint32_t /*CharacterID*/) { abort(); /* STUB */ }

// 0x0809a890  operate.c:2516
uint32_t LogCommunication(uint32_t /*CharacterID*/, int /*Mode*/, int /*Channel*/, char * /*Text*/) { abort(); /* STUB */ }

// 0x0809a920  operate.c:2543
uint32_t LogListener(uint32_t /*StatementID*/, TPlayer * /*pl*/) { abort(); /* STUB */ }

// 0x0809a990  operate.c:2558
void ProcessCommunicationControl() { abort(); /* STUB */ }

// 0x0809ab60  operate.c:2584
int GetCommunicationContext(uint32_t /*CharacterID*/, uint32_t /*StatementID*/, int & /*NumberOfStatements*/, vector<TReportedStatement> *& /*ReportedStatements*/) { abort(); /* STUB */ }

// 0x0809b640  operate.c:2673
TParty *GetParty(uint32_t /*Leader*/) { abort(); /* STUB */ }

// 0x0809b6a0  operate.c:2683
bool IsInvitedToParty(uint32_t /*Guest*/, uint32_t /*Host*/) { abort(); /* STUB */ }

// 0x0809b740  operate.c:2695
void DisbandParty(uint32_t /*Leader*/) { abort(); /* STUB */ }

// 0x0809b960  operate.c:2729
void InviteToParty(uint32_t /*Host*/, uint32_t /*Guest*/) { abort(); /* STUB */ }

// 0x0809bdd0  operate.c:2799
void RevokeInvitation(uint32_t /*Host*/, uint32_t /*Guest*/) { abort(); /* STUB */ }

// 0x0809c0c0  operate.c:2840
void JoinParty(uint32_t /*Guest*/, uint32_t /*Host*/) { abort(); /* STUB */ }

// 0x0809c440  operate.c:2888
void PassLeadership(uint32_t /*OldLeader*/, uint32_t /*NewLeader*/) { abort(); /* STUB */ }

// 0x0809c740  operate.c:2941
void LeaveParty(uint32_t /*Member*/, bool /*Forced*/) { abort(); /* STUB */ }
