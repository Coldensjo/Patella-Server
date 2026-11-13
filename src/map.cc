#include <cstdint>
#include <cstdlib>

#include "script.hh"
#include "config.hh"
#include "objects.hh"
#include "map.hh"
#include "containr.hh"
#include "utils.hh"
#include "time.hh"

struct TObject {
	uint32_t ObjectID;
	Object NextObject;
	Object Container;
	ObjectType Type;
	uint32_t Attributes[4];
};

struct TObjectBlock {
	TObject Object[32768];
};

struct TSector {
	Object MapCon[32][32];
	uint32_t TimeStamp;
	unsigned char Status;
	unsigned char MapFlags;
};

struct TCronEntry {
	Object Obj;
	uint32_t RoundNr;
	int Previous;
	int Next;
};

struct TDepotInfo {
	char Town[20];
	int Size;
};

struct TMark {
	char Name[20];
	int x;
	int y;
	int z;
};

char InstanceAttributeNames[18][30] = {
	"Content",
	"ChestQuestNumber",
	"Amount",
	"KeyNumber",
	"KeyholeNumber",
	"Level",
	"DoorQuestNumber",
	"DoorQuestValue",
	"Charges",
	"String",
	"Editor",
	"ContainerLiquidType",
	"PoolLiquidType",
	"AbsTeleportDestination",
	"Responsible",
	"RemainingExpireTime",
	"SavedExpireTime",
	"RemainingUses",
};	// map.c:38
matrix3d<TSector *> *Sector;	// map.c:81
TObjectBlock **ObjectBlock;	// map.c:82
TObject *FirstFreeObject;	// map.c:83
uint32_t *HashTableData;	// map.c:84
unsigned char *HashTableType;	// map.c:85
int HashTableSize;	// map.c:86
uint32_t HashTableMask;	// map.c:87
int HashTableFree;	// map.c:88
int ObjectCounter;	// map.c:89
vector<TCronEntry> CronEntry(0, 256, 256);	// map.c:90
int CronHashTable[2047];	// map.c:91
int CronEntries;	// map.c:92
int OBCount;	// map.c:93
int SectorXMin;	// map.c:94
int SectorXMax;	// map.c:95
int SectorYMin;	// map.c:96
int SectorYMax;	// map.c:97
int SectorZMin;	// map.c:98
int SectorZMax;	// map.c:99
int RefreshedCylinders;	// map.c:100
vector<TDepotInfo> DepotInfo(0, 4, 5);	// map.c:101
vector<TMark> Mark(0, 4, 5);	// map.c:102
int Marks;	// map.c:103
int NewbieStartPositionX;	// map.c:104
int NewbieStartPositionY;	// map.c:105
int NewbieStartPositionZ;	// map.c:106
int VeteranStartPositionX;	// map.c:107
int VeteranStartPositionY;	// map.c:108
int VeteranStartPositionZ;	// map.c:109
[[maybe_unused]] static TDynamicWriteBuffer HelpBuffer(65536);	// map.c:110

// 0x080572f0  map.c:128
TObject &AccessObject(Object /*Obj*/) { abort(); /* STUB */ }

// 0x08057390  map.c:143
bool Object::exists() { abort(); /* STUB */ }

// 0x08057400  map.c:155
ObjectType Object::getObjectType() { abort(); /* STUB */ }

// 0x08057440  map.c:163
uint32_t Object::getAttribute(INSTANCEATTRIBUTE /*Attribute*/) { abort(); /* STUB */ }

// 0x08057540  map.c:180
void Object::setAttribute(INSTANCEATTRIBUTE /*Attribute*/, uint32_t /*Value*/) { abort(); /* STUB */ }

// 0x080576c0  map.c:199
Object Object::getNextObject() { abort(); /* STUB */ }

// 0x080576f0  map.c:207
Object Object::getContainer() { abort(); /* STUB */ }

// 0x08057720  map.c:215
uint32_t Object::getCreatureID() { abort(); /* STUB */ }

// 0x08057790  map.c:227
void GetObjectCoordinates(Object /*Obj*/, int & /*x*/, int & /*y*/, int & /*z*/) { abort(); /* STUB */ }

// 0x080578d0  map.c:243
unsigned char GetMapContainerFlags(Object /*Obj*/) { abort(); /* STUB */ }

// 0x080579a0  map.c:261
void CronMove(int /*Destination*/, int /*Source*/) { abort(); /* STUB */ }

// 0x08057a80  map.c:273
void CronHeapify(int /*Position*/) { abort(); /* STUB */ }

// 0x08057bc0  map.c:300
void CronSet(Object /*Obj*/, uint32_t /*Delay*/) { abort(); /* STUB */ }

// 0x08057d80  map.c:320
void CronDelete(int /*Position*/) { abort(); /* STUB */ }

// 0x08057e90  map.c:341
Object CronCheck() { abort(); /* STUB */ }

// 0x08057ef0  map.c:351
uint32_t CronInfo(Object /*Obj*/, bool /*Delete*/) { abort(); /* STUB */ }

// 0x08058040  map.c:373
uint32_t CronStop(Object /*Obj*/) { abort(); /* STUB */ }

// 0x080580e0  map.c:386
void CronExpire(Object /*Obj*/, int /*Delay*/) { abort(); /* STUB */ }

// 0x08058200  map.c:400
void CronChange(Object /*Obj*/, int /*NewDelay*/) { abort(); /* STUB */ }

// 0x08058330  map.c:425
Object SplitObject(Object /*Obj*/, int /*Count*/) { abort(); /* STUB */ }

// 0x080584e0  map.c:449
void MergeObjects(Object /*Obj*/, Object /*Dest*/) { abort(); /* STUB */ }

// 0x080588b0  map.c:486
void MoveObject(Object /*Obj*/, Object /*Con*/) { abort(); /* STUB */ }

// 0x080589f0  map.c:503
void ChangeObject(Object /*Obj*/, ObjectType /*Type*/) { abort(); /* STUB */ }

// 0x08058fe0  map.c:544
void ChangeObject(Object /*Obj*/, INSTANCEATTRIBUTE /*Attribute*/, uint32_t /*Value*/) { abort(); /* STUB */ }

// 0x08059090  map.c:561
TObject *GetFreeObjectSlot() { abort(); /* STUB */ }

// 0x08059110  map.c:577
void ResizeHashTable() { abort(); /* STUB */ }

// 0x08059260  map.c:619
// local static: int Iterator = 0  (map.c:620, 0x0812793c .data)
Object CreateObject() { abort(); /* STUB */ }

// 0x080593b0  map.c:641
int GetObjectPriority(Object /*Obj*/) { abort(); /* STUB */ }

// 0x08059560  map.c:659
void PlaceObject(Object /*Obj*/, Object /*Con*/, bool /*Append*/) { abort(); /* STUB */ }

// 0x08059aa0  map.c:714
Object AppendObject(Object /*Con*/, ObjectType /*Type*/) { abort(); /* STUB */ }

// 0x08059ba0  map.c:731
Object SetObject(Object /*Con*/, ObjectType /*Type*/, uint32_t /*CreatureID*/) { abort(); /* STUB */ }

// 0x08059cf0  map.c:751
Object CopyObject(Object /*Con*/, Object /*Source*/) { abort(); /* STUB */ }

// 0x0805a070  map.c:782
void PutFreeObjectSlot(TObject * /*Entry*/) { abort(); /* STUB */ }

// 0x0805a0b0  map.c:795
void CutObject(Object /*Obj*/) { abort(); /* STUB */ }

// 0x0805a300  map.c:815
void DestroyObject(Object /*Obj*/) { abort(); /* STUB */ }

// 0x0805a5d0  map.c:848
void DeleteObject(Object /*Obj*/) { abort(); /* STUB */ }

// 0x0805a670  map.c:866
Object GetMapContainer(int /*x*/, int /*y*/, int /*z*/) { abort(); /* STUB */ }

// 0x0805a9a0  map.c:879
Object GetMapContainer(Object /*Obj*/) { abort(); /* STUB */ }

// 0x0805aa80  map.c:892
Object GetFirstObject(int /*x*/, int /*y*/, int /*z*/) { abort(); /* STUB */ }

// 0x0805aaf0  map.c:901
Object GetFirstSpecObject(int /*x*/, int /*y*/, int /*z*/, ObjectType /*Type*/) { abort(); /* STUB */ }

// 0x0805abb0  map.c:911
Object GetFirstContainerObject(Object /*Con*/) { abort(); /* STUB */ }

// 0x0805acc0  map.c:927
Object GetContainerObject(Object /*Con*/, int /*nr*/) { abort(); /* STUB */ }

// 0x0805adf0  map.c:954
bool IsOnMap(int /*x*/, int /*y*/, int /*z*/) { abort(); /* STUB */ }

// 0x0805aea0  map.c:964
bool CoordinateFlag(int /*x*/, int /*y*/, int /*z*/, FLAG /*Flag*/) { abort(); /* STUB */ }

// 0x0805af60  map.c:974
bool IsPremiumArea(int /*x*/, int /*y*/, int) { abort(); /* STUB */ }

// 0x0805b000  map.c:991
bool IsProtectionZone(int /*x*/, int /*y*/, int /*z*/) { abort(); /* STUB */ }

// 0x0805b070  map.c:1001
bool IsNoLogoutField(int /*x*/, int /*y*/, int /*z*/) { abort(); /* STUB */ }

// 0x0805b0e0  map.c:1011
bool IsHouse(int /*x*/, int /*y*/, int /*z*/) { abort(); /* STUB */ }

// 0x0805b180  map.c:1019
unsigned short GetHouseID(int /*x*/, int /*y*/, int /*z*/) { abort(); /* STUB */ }

// 0x0805b260  map.c:1033
void SetHouseID(int /*x*/, int /*y*/, int /*z*/, unsigned short /*ID*/) { abort(); /* STUB */ }

// 0x0805b430  map.c:1052
int GetDepotNumber(char * /*Town*/) { abort(); /* STUB */ }

// 0x0805b4c0  map.c:1067
char *GetDepotName(int /*DepotNumber*/) { abort(); /* STUB */ }

// 0x0805b540  map.c:1083
int GetDepotSize(int /*DepotNumber*/, bool /*PremiumAccount*/) { abort(); /* STUB */ }

// 0x0805b610  map.c:1099
void GetStartPosition(int & /*x*/, int & /*y*/, int & /*z*/, bool /*Newbie*/) { abort(); /* STUB */ }

// 0x0805b660  map.c:1115
bool GetMarkPosition(char * /*Name*/, int & /*x*/, int & /*y*/, int & /*z*/) { abort(); /* STUB */ }

// 0x0805b710  map.c:1136
void InitSector(int /*x*/, int /*y*/, int /*z*/) { abort(); /* STUB */ }

// 0x0805b970  map.c:1163
void SwapObject(TWriteBinaryFile & /*File*/, Object /*Obj*/, uint32_t /*FileNumber*/) { abort(); /* STUB */ }

// 0x0805bb60  map.c:1187
// local static: uint32_t FileNumber = 0  (map.c:1188, 0x08127940 .data)
void SwapSector() { abort(); /* STUB */ }

// 0x0805bf30  map.c:1241
void UnswapSector(uint32_t /*FileNumber*/) { abort(); /* STUB */ }

// 0x0805c2b0  map.c:1288
void DeleteSwappedSectors() { abort(); /* STUB */ }

// 0x0805c390  map.c:1315
int GetInstanceAttributeByName(char * /*Name*/) { abort(); /* STUB */ }

// 0x0805c3f0  map.c:1326
void LoadObjects(TReadScriptFile & /*Script*/, TWriteStream & /*Stream*/, bool /*Skip*/) { abort(); /* STUB */ }

// 0x0805c680  map.c:1376
void LoadObjects(TReadStream & /*Stream*/, Object /*Con*/) { abort(); /* STUB */ }

// 0x0805c890  map.c:1423
void LoadSector(char * /*FileName*/, int /*x*/, int /*y*/, int /*z*/) { abort(); /* STUB */ }

// 0x0805ce00  map.c:1483
void LoadMap() { abort(); /* STUB */ }

// 0x0805cfd0  map.c:1514
void SaveObjects(Object /*Obj*/, TWriteStream & /*Stream*/, bool /*Stop*/) { abort(); /* STUB */ }

// 0x0805d370  map.c:1575
void SaveObjects(TReadStream & /*Stream*/, TWriteScriptFile & /*Script*/) { abort(); /* STUB */ }

// 0x0805d530  map.c:1622
void SaveSector(char * /*FileName*/, int /*x*/, int /*y*/, int /*z*/) { abort(); /* STUB */ }

// 0x0805d980  map.c:1682
// local static: bool SavingMap = false  (map.c:1683, 0x08127944 .data)
void SaveMap() { abort(); /* STUB */ }

// 0x0805daa0  map.c:1707
void RefreshSector(int /*x*/, int /*y*/, int /*z*/, TReadStream & /*Stream*/) { abort(); /* STUB */ }

// 0x0805dcf0  map.c:1737
void PatchSector(int /*x*/, int /*y*/, int /*z*/, bool /*FullSector*/, TReadScriptFile & /*Script*/, bool /*SaveHouses*/) { abort(); /* STUB */ }

// 0x0805f050  map.c:1929
void ReadMapConfig() { abort(); /* STUB */ }

// 0x0805f860  map.c:2020
void InitMap() { abort(); /* STUB */ }

// 0x0805faf0  map.c:2062
void ExitMap(bool /*save*/) { abort(); /* STUB */ }
