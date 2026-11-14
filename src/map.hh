// map.hh: Object, an object by its number, and the map's interface: the
// objects and their containers, the cron of timed object changes, the fields
// and their flags, house numbers, depots, marks and start positions, and
// loading, saving, swapping, refreshing and patching the sectors.

#ifndef PATELLA_MAP_HH
#define PATELLA_MAP_HH

#include <cstdint>
#include <cstdlib>

#include "objects.hh"
#include "containr.hh"

struct TCronEntry;
struct TDepotInfo;
struct TMark;
struct TObject;
struct TObjectBlock;
struct TReadScriptFile;
struct TReadStream;
struct TSector;
struct TWriteBinaryFile;
struct TWriteScriptFile;
struct TWriteStream;

struct Object {
	Object() { ObjectID = 0; }
	Object(const Object &Other) { ObjectID = Other.ObjectID; }
	Object(uint32_t _ObjectID) { ObjectID = _ObjectID; }
	Object operator=(Object Other) { ObjectID = Other.ObjectID; return *this; }
	bool operator==(Object Other) { return ObjectID == Other.ObjectID; }
	bool operator!=(Object Other) { return !(*this == Other); }
	bool exists();
	uint32_t getObjectID() { return ObjectID; }
	ObjectType getObjectType();
	bool getFlag(FLAG Flag) { return getObjectType().getFlag(Flag); }
	uint32_t getAttribute(TYPEATTRIBUTE Attribute) { return getObjectType().getAttribute(Attribute); }
	uint32_t getAttribute(INSTANCEATTRIBUTE Attribute);
	void setAttribute(INSTANCEATTRIBUTE Attribute, uint32_t Value);
	void changeAttribute(INSTANCEATTRIBUTE Attribute, int Delta) { setAttribute(Attribute, getAttribute(Attribute) + Delta); }
	Object operator++(int) { ObjectID = getNextObject().getObjectID(); return *this; }
	Object getNextObject();
	Object getContainer();
	bool isMapContainer() { return getObjectType().isMapContainer(); }
	bool isBodyContainer() { return getObjectType().isBodyContainer(); }
	bool isCreature() { return getObjectType().isCreature(); }
	uint32_t getCreatureID();
private:
	uint32_t ObjectID;
};

static const Object NONE;

// Coordinates of an object that is not on the map, as the client sends them
// and GetObject and GetBodyContainer read them: x is POSITION_NOT_ON_MAP, y is
// a body position (objects.hh's BODY_*) or POSITION_CONTAINER_BASE plus the
// number of an open container window (0 to MAX_OPEN_CONTAINERS - 1, cr.hh),
// and z the place in that container.
const int POSITION_NOT_ON_MAP = 0xFFFF;
const int POSITION_CONTAINER_BASE = 0x40;

// The width and height of a map sector in fields (the XXXX-YYYY-ZZ.sec files).
const int SECTOR_SIZE = 32;

// The flags of a map field (GetMapContainerFlags, TSector::MapFlags), named as
// the sector files name them: Refresh, NoLogout, ProtectionZone. In the map
// container's Attributes[3] they are shifted left by 8.
const int MAPFLAG_REFRESH = 1;
const int MAPFLAG_NOLOGOUT = 2;
const int MAPFLAG_PROTECTIONZONE = 4;

// The object streams of LoadObjects and SaveObjects: each object is its type
// number (a word) followed by its attributes, each an attribute number (a
// byte) and its value, and END_OF_ATTRIBUTES. The attribute CONTENT has no
// value; the objects inside follow it, ended by END_OF_OBJECTS, and then the
// container's END_OF_ATTRIBUTES. moveuse.cc writes an empty depot chest in
// this form.
const int END_OF_OBJECTS = 0xFFFF;
const int END_OF_ATTRIBUTES = 0xFF;

// The size of CronHashTable, which chains the cron entries by object number.
const int CRON_HASH_TABLE_SIZE = 2047;

extern int SectorXMin;
extern int SectorXMax;
extern int SectorYMin;
extern int SectorYMax;
extern int SectorZMin;
extern int SectorZMax;
extern int RefreshedCylinders;

TObject &AccessObject(Object Obj);
void GetObjectCoordinates(Object Obj, int &x, int &y, int &z);
unsigned char GetMapContainerFlags(Object Obj);
void CronMove(int Destination, int Source);
void CronHeapify(int Position);
void CronSet(Object Obj, uint32_t Delay);
void CronDelete(int Position);
Object CronCheck();
uint32_t CronInfo(Object Obj, bool Delete);
uint32_t CronStop(Object Obj);
void CronExpire(Object Obj, int Delay);
void CronChange(Object Obj, int NewDelay);
Object SplitObject(Object Obj, int Count);
void MergeObjects(Object Obj, Object Dest);
void MoveObject(Object Obj, Object Con);
void ChangeObject(Object Obj, ObjectType Type);
void ChangeObject(Object Obj, INSTANCEATTRIBUTE Attribute, uint32_t Value);
TObject *GetFreeObjectSlot();
void ResizeHashTable();
Object CreateObject();
int GetObjectPriority(Object Obj);
void PlaceObject(Object Obj, Object Con, bool Append);
Object AppendObject(Object Con, ObjectType Type);
Object SetObject(Object Con, ObjectType Type, uint32_t CreatureID);
Object CopyObject(Object Con, Object Source);
void PutFreeObjectSlot(TObject *Entry);
void CutObject(Object Obj);
void DestroyObject(Object Obj);
void DeleteObject(Object Obj);
Object GetMapContainer(int x, int y, int z);
Object GetMapContainer(Object Obj);
Object GetFirstObject(int x, int y, int z);
Object GetFirstSpecObject(int x, int y, int z, ObjectType Type);
Object GetFirstContainerObject(Object Con);
Object GetContainerObject(Object Con, int nr);
bool IsOnMap(int x, int y, int z);
bool CoordinateFlag(int x, int y, int z, FLAG Flag);
bool IsPremiumArea(int x, int y, int);
bool IsProtectionZone(int x, int y, int z);
bool IsNoLogoutField(int x, int y, int z);
bool IsHouse(int x, int y, int z);
unsigned short GetHouseID(int x, int y, int z);
void SetHouseID(int x, int y, int z, unsigned short ID);
int GetDepotNumber(char *Town);
char *GetDepotName(int DepotNumber);
int GetDepotSize(int DepotNumber, bool PremiumAccount);
void GetStartPosition(int &x, int &y, int &z, bool Newbie);
bool GetMarkPosition(char *Name, int &x, int &y, int &z);
void InitSector(int x, int y, int z);
void SwapObject(TWriteBinaryFile &File, Object Obj, uint32_t FileNumber);
void SwapSector();
void UnswapSector(uint32_t FileNumber);
void DeleteSwappedSectors();
int GetInstanceAttributeByName(char *Name);
void LoadObjects(TReadScriptFile &Script, TWriteStream &Stream, bool Skip);
void LoadObjects(TReadStream &Stream, Object Con);
void LoadSector(char *FileName, int x, int y, int z);
void LoadMap();
void SaveObjects(Object Obj, TWriteStream &Stream, bool Stop);
void SaveObjects(TReadStream &Stream, TWriteScriptFile &Script);
void SaveSector(char *FileName, int x, int y, int z);
void SaveMap();
void RefreshSector(int x, int y, int z, TReadStream &Stream);
void PatchSector(int x, int y, int z, bool FullSector, TReadScriptFile &Script, bool SaveHouses);
void ReadMapConfig();
void InitMap();
void ExitMap(bool save);

extern char InstanceAttributeNames[INSTANCEATTRIBUTES][30];
extern matrix3d<TSector *> *Sector;
extern TObjectBlock **ObjectBlock;
extern TObject *FirstFreeObject;
extern uintptr_t *HashTableData;
extern unsigned char *HashTableType;
extern int HashTableSize;
extern uint32_t HashTableMask;
extern int HashTableFree;
extern int ObjectCounter;
extern vector<TCronEntry> CronEntry;
extern int CronHashTable[CRON_HASH_TABLE_SIZE];
extern int CronEntries;
extern int OBCount;
extern vector<TDepotInfo> DepotInfo;
extern vector<TMark> Mark;
extern int Marks;
extern int NewbieStartPositionX;
extern int NewbieStartPositionY;
extern int NewbieStartPositionZ;
extern int VeteranStartPositionX;
extern int VeteranStartPositionY;
extern int VeteranStartPositionZ;

#endif
