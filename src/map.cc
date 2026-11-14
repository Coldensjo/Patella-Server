#include <algorithm>
#include <cerrno>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <dirent.h>
#include <unistd.h>

#include "script.hh"
#include "config.hh"
#include "objects.hh"
#include "map.hh"
#include "containr.hh"
#include "utils.hh"
#include "time.hh"
#include "mudinc.hh"
#include "strings.hh"
#include "shm.hh"
#include "houses.hh"

struct TObject {
	uint32_t ObjectID;
	Object NextObject;
	Object Container;
	ObjectType Type;
	uint32_t Attributes[4];
};

// The objects are kept in blocks of OBJECTS_PER_BLOCK (map.dat's CacheSize is
// a multiple of it).
const int OBJECTS_PER_BLOCK = 32768;

struct TObjectBlock {
	TObject Object[OBJECTS_PER_BLOCK];
};

// TSector::Status. A sector is SECTOR_INITIALIZING while InitSector creates its
// fields, so that SwapSector, which only takes a sector in memory, cannot swap
// it out half made.
const int SECTOR_INITIALIZING = 0;
const int SECTOR_IN_MEMORY = 1;
const int SECTOR_SWAPPED_OUT = 2;

struct TSector {
	Object MapCon[SECTOR_SIZE][SECTOR_SIZE];
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

char InstanceAttributeNames[INSTANCEATTRIBUTES][30] = {
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
};
matrix3d<TSector *> *Sector;
TObjectBlock **ObjectBlock;
TObject *FirstFreeObject;
uintptr_t *HashTableData;
unsigned char *HashTableType;
// HashTableType of an object number: free, an object in memory (HashTableData
// points at it) or swapped out with its sector (HashTableData is the number
// of the swap file). Entry 0, object number 0 (NONE), is never given out; its
// data is the object AccessObject returns for an invalid object.
const int HASHENTRY_FREE = 0;
const int HASHENTRY_IN_MEMORY = 1;
const int HASHENTRY_SWAPPED_OUT = 2;
const int HASHENTRY_RESERVED = 255;
int HashTableSize;
uint32_t HashTableMask;
int HashTableFree;
int ObjectCounter;
vector<TCronEntry> CronEntry(0, 256, 256);
// The size of CronHashTable, which chains the cron entries by object number.
const int CRON_HASH_TABLE_SIZE = 2047;
int CronHashTable[CRON_HASH_TABLE_SIZE];
int CronEntries;
int OBCount;
int SectorXMin;
int SectorXMax;
int SectorYMin;
int SectorYMax;
int SectorZMin;
int SectorZMax;
int RefreshedCylinders;
vector<TDepotInfo> DepotInfo(0, 4, 5);
vector<TMark> Mark(0, 4, 5);
int Marks;
int NewbieStartPositionX;
int NewbieStartPositionY;
int NewbieStartPositionZ;
int VeteranStartPositionX;
int VeteranStartPositionY;
int VeteranStartPositionZ;
static TDynamicWriteBuffer HelpBuffer(65536);

TObject &AccessObject(Object Obj) {
	if (Obj == NONE) {
		error("AccessObject: Ung\xfcltige Objektnummer Null.\n");
		return *(TObject *)HashTableData[0];
	}

	uint32_t Position = Obj.getObjectID() & HashTableMask;
	if (HashTableType[Position] == HASHENTRY_SWAPPED_OUT) {
		UnswapSector((uint32_t)HashTableData[Position]);
	}

	if (HashTableType[Position] == HASHENTRY_IN_MEMORY
			&& ((TObject *)HashTableData[Position])->ObjectID == Obj.getObjectID()) {
		return *(TObject *)HashTableData[Position];
	}

	return *(TObject *)HashTableData[0];
}

bool Object::exists() {
	if (ObjectID == 0) {
		return false;
	}

	uint32_t Position = ObjectID & HashTableMask;
	if (HashTableType[Position] == HASHENTRY_SWAPPED_OUT) {
		UnswapSector((uint32_t)HashTableData[Position]);
	}

	return HashTableType[Position] == HASHENTRY_IN_MEMORY && ((TObject *)HashTableData[Position])->ObjectID == ObjectID;
}

ObjectType Object::getObjectType() {
	return AccessObject(*this).Type;
}

uint32_t Object::getAttribute(INSTANCEATTRIBUTE Attribute) {
	int Offset = getObjectType().getAttributeOffset(Attribute);
	if (Offset == -1) {
		error("Object::getAttribute: Flag f\xfcr Attribut %d bei Objekttyp %d nicht gesetzt.\n",
				Attribute, getObjectType().getTypeID());
		return 0;
	}

	if (!in(Offset, 0, 3)) {
		error("Object::getAttribute: Ung\xfcltiger Offset %d f\xfcr Attribut %d bei Objekttyp %d.\n",
				Offset, Attribute, getObjectType().getTypeID());
		return 0;
	}

	return AccessObject(*this).Attributes[Offset];
}

void Object::setAttribute(INSTANCEATTRIBUTE Attribute, uint32_t Value) {
	int Offset = getObjectType().getAttributeOffset(Attribute);
	if (Offset == -1) {
		error("Object::setAttribute: Flag f\xfcr Attribut %d bei Objekttyp %d nicht gesetzt.\n",
				Attribute, getObjectType().getTypeID());
		return;
	}

	if (!in(Offset, 0, 3)) {
		error("Object::setAttribute: Ung\xfcltiger Offset %d f\xfcr Attribut %d bei Objekttyp %d.\n",
				Offset, Attribute, getObjectType().getTypeID());
		return;
	}

	if (Value == 0 && (Attribute == AMOUNT || Attribute == POOLLIQUIDTYPE || Attribute == CHARGES)) {
		Value = 1;
	}

	if (Value == 0 && Attribute == REMAININGUSES) {
		Value = getObjectType().getAttribute(TOTALUSES);
	}

	AccessObject(*this).Attributes[Offset] = Value;
}

Object Object::getNextObject() {
	return AccessObject(*this).NextObject;
}

Object Object::getContainer() {
	return AccessObject(*this).Container;
}

uint32_t Object::getCreatureID() {
	if (!isCreature()) {
		error("Object::getCreatureID: Objekt ist keine Kreatur.\n");
		return 0;
	}

	return AccessObject(*this).Attributes[1];
}

void GetObjectCoordinates(Object Obj, int &x, int &y, int &z) {
	if (!Obj.exists()) {
		error("GetObjectCoordinates: \xdc" "bergebenes Objekt existiert nicht.\n");
		x = 0;
		y = 0;
		z = 0;
		return;
	}

	while (!Obj.isMapContainer()) {
		Obj = Obj.getContainer();
	}

	x = AccessObject(Obj).Attributes[1];
	y = AccessObject(Obj).Attributes[2];
	z = AccessObject(Obj).Attributes[3] & 0xFF;
}

unsigned char GetMapContainerFlags(Object Obj) {
	if (!Obj.exists() || !Obj.isMapContainer()) {
		error("GetMapContainerFlags: Objekt ist kein MapContainer.\n");
		return 0;
	}

	return (unsigned char)(AccessObject(Obj).Attributes[3] >> 8);
}

void CronMove(int Destination, int Source) {
	CronEntry(Destination) = CronEntry(Source);
	TCronEntry *Entry = &CronEntry(Destination);
	if (Entry->Next != -1) {
		CronEntry(Entry->Next).Previous = Destination;
	}

	if (Entry->Previous != -1) {
		CronEntry(Entry->Previous).Next = Destination;
	} else {
		CronHashTable[Entry->Obj.getObjectID() % CRON_HASH_TABLE_SIZE] = Destination;
	}
}

void CronHeapify(int Position) {
	while (Position != 1) {
		int Father = Position / 2;
		if (CronEntry(Position).RoundNr >= CronEntry(Father).RoundNr) {
			break;
		}

		CronMove(0, Position);
		CronMove(Position, Father);
		CronMove(Father, 0);
		Position = Father;
	}

	while (true) {
		int SmallerSon = Position * 2;
		if (SmallerSon > CronEntries) {
			break;
		}

		if (SmallerSon + 1 <= CronEntries
				&& CronEntry(SmallerSon + 1).RoundNr < CronEntry(SmallerSon).RoundNr) {
			SmallerSon++;
		}

		if (CronEntry(Position).RoundNr <= CronEntry(SmallerSon).RoundNr) {
			break;
		}

		CronMove(0, Position);
		CronMove(Position, SmallerSon);
		CronMove(SmallerSon, 0);
		Position = SmallerSon;
	}
}

void CronSet(Object Obj, uint32_t Delay) {
	if (!Obj.exists()) {
		error("CronSet: \xdc" "bergebenes Objekt existiert nicht.\n");
		return;
	}

	CronEntries++;
	CronEntry(CronEntries).Obj = Obj;
	CronEntry(CronEntries).RoundNr = RoundNr + Delay;
	CronEntry(CronEntries).Previous = -1;
	CronEntry(CronEntries).Next = CronHashTable[Obj.getObjectID() % CRON_HASH_TABLE_SIZE];
	CronHashTable[Obj.getObjectID() % CRON_HASH_TABLE_SIZE] = CronEntries;
	if (CronEntry(CronEntries).Next != -1) {
		CronEntry(CronEntry(CronEntries).Next).Previous = CronEntries;
	}

	CronHeapify(CronEntries);
}

void CronDelete(int Position) {
	if (!in(Position, 1, CronEntries)) {
		error("CronDelete: Ung\xfcltige Position %d.\n", Position);
		return;
	}

	TCronEntry *Entry = &CronEntry(Position);
	if (Entry->Next != -1) {
		CronEntry(Entry->Next).Previous = Entry->Previous;
	}

	if (Entry->Previous != -1) {
		CronEntry(Entry->Previous).Next = Entry->Next;
	} else {
		CronHashTable[Entry->Obj.getObjectID() % CRON_HASH_TABLE_SIZE] = Entry->Next;
	}

	CronEntries--;
	if (Position <= CronEntries) {
		CronMove(Position, CronEntries + 1);
		CronHeapify(Position);
	}
}

Object CronCheck() {
	if (CronEntries == 0) {
		return NONE;
	}

	if (CronEntry(1).RoundNr > RoundNr) {
		return NONE;
	}

	return CronEntry(1).Obj;
}

uint32_t CronInfo(Object Obj, bool Delete) {
	if (!Obj.exists()) {
		error("CronInfo: \xdc" "bergebenes Objekt existiert nicht.\n");
		return 0;
	}

	int Position = CronHashTable[Obj.getObjectID() % CRON_HASH_TABLE_SIZE];
	while (Position != -1 && CronEntry(Position).Obj != Obj) {
		Position = CronEntry(Position).Next;
	}

	if (Position == -1) {
		error("CronInfo: Objekt ist nicht im Cron-System eingetragen.\n");
		return 0;
	}

	int Delay = CronEntry(Position).RoundNr - RoundNr;
	if (Delay < 1) {
		Delay = 1;
	}

	if (Delete) {
		CronDelete(Position);
	}

	return Delay;
}

uint32_t CronStop(Object Obj) {
	if (!Obj.exists()) {
		error("CronStop: \xdc" "bergebenes Objekt existiert nicht.\n");
		return 0;
	}

	return CronInfo(Obj, true);
}

void CronExpire(Object Obj, int Delay) {
	if (!Obj.exists()) {
		error("CronExpire: \xdc" "bergebenes Objekt existiert nicht.\n");
		return;
	}

	if (Obj.getFlag(EXPIRE)) {
		if (Delay == -1) {
			Delay = Obj.getAttribute(TOTALEXPIRETIME);
		}

		CronSet(Obj, Delay);
	}
}

void CronChange(Object Obj, int NewDelay) {
	if (!Obj.exists()) {
		error("CronChange: \xdc" "bergebenes Objekt existiert nicht.\n");
		return;
	}

	int Position = CronHashTable[Obj.getObjectID() % CRON_HASH_TABLE_SIZE];
	while (Position != -1 && CronEntry(Position).Obj != Obj) {
		Position = CronEntry(Position).Next;
	}

	if (Position == -1) {
		error("CronChange: Objekt ist nicht im Cron-System eingetragen.\n");
		return;
	}

	CronEntry(Position).RoundNr = RoundNr + NewDelay;
	CronHeapify(Position);
}

Object SplitObject(Object Obj, int Count) {
	if (!Obj.exists()) {
		error("SplitObject: \xdc" "bergebenes Objekt existiert nicht.\n");
		return NONE;
	}

	if (!Obj.getFlag(CUMULATIVE)) {
		error("SplitObject: Objekt ist nicht kumulierbar.\n");
		return Obj;
	}

	if (Count <= 0 || Count > (int)Obj.getAttribute(AMOUNT)) {
		error("SplitObject: Ung\xfcltiger Z\xe4hler %d.\n", Count);
		return Obj;
	}

	if ((uint32_t)Count == Obj.getAttribute(AMOUNT)) {
		return Obj;
	}

	Object Copy = CopyObject(Obj.getContainer(), Obj);
	Obj.changeAttribute(AMOUNT, -Count);
	Copy.setAttribute(AMOUNT, Count);
	return Copy;
}

void MergeObjects(Object Obj, Object Dest) {
	if (!Obj.exists()) {
		error("MergeObjects: \xdc" "bergebenes Objekt existiert nicht.\n");
		return;
	}

	if (!Dest.exists()) {
		error("MergeObjects: \xdc" "bergebenes Ziel existiert nicht.\n");
		return;
	}

	if (Obj.getObjectType() != Dest.getObjectType()) {
		error("MergeObjects: Objekttypen %d und %d sind nicht identisch.\n",
				Obj.getObjectType().getTypeID(), Dest.getObjectType().getTypeID());
		return;
	}

	if (!Obj.getFlag(CUMULATIVE)) {
		// The original passes one argument for two %d.
		error("MergeObjects: Objekttyp %d/%d ist nicht kumulierbar.\n",
				Obj.getObjectType().getTypeID());
		return;
	}

	if (Obj.getAttribute(AMOUNT) == 0) {
		error("MergeObjects: Objekt enth\xe4lt 0 Teile.\n");
		Obj.setAttribute(AMOUNT, 1);
	}

	if (Dest.getAttribute(AMOUNT) == 0) {
		error("MergeObjects: Zielobjekt enth\xe4lt 0 Teile.\n");
		Dest.setAttribute(AMOUNT, 1);
	}

	Dest.changeAttribute(AMOUNT, Obj.getAttribute(AMOUNT));
	if (Dest.getAttribute(AMOUNT) > MAX_AMOUNT) {
		error("MergeObjects: Neues Objekt enth\xe4lt mehr als 100 Teile.\n");
		Dest.setAttribute(AMOUNT, MAX_AMOUNT);
	}

	DeleteObject(Obj);
}

void MoveObject(Object Obj, Object Con) {
	if (!Obj.exists()) {
		error("MoveObject: \xdc" "bergebenes Objekt existiert nicht.\n");
		return;
	}

	if (!Con.exists()) {
		error("MoveObject: \xdc" "bergebener Zielcontainer existiert nicht.\n");
		return;
	}

	CutObject(Obj);
	PlaceObject(Obj, Con, false);
}

void ChangeObject(Object Obj, ObjectType Type) {
	int Delay = -1;
	uint32_t Amount = 0;
	uint32_t SavedExpireTime = 0;

	if (!Obj.exists()) {
		error("ChangeObject: \xdc" "bergebenes Objekt existiert nicht.\n");
		return;
	}

	if (Obj.getObjectType() == ObjectType(0)) {
		goto newObject;
	}

	if (Obj.getFlag(CUMULATIVE)) {
		Amount = Obj.getAttribute(AMOUNT);
	}

	if (Obj.getFlag(EXPIRE)) {
		SavedExpireTime = CronStop(Obj);
		if (Type.getFlag(CUMULATIVE)) {
			Amount = 1;
		}
	}

	if (Obj.getFlag(TEXT) && !Type.getFlag(TEXT)) {
		DeleteDynamicString(Obj.getAttribute(TEXTSTRING));
		Obj.setAttribute(TEXTSTRING, 0);
		DeleteDynamicString(Obj.getAttribute(EDITOR));
		Obj.setAttribute(EDITOR, 0);
	}

	if (Obj.getFlag(EXPIRESTOP)) {
		if (Obj.getAttribute(SAVEDEXPIRETIME) != 0) {
			Delay = Obj.getAttribute(SAVEDEXPIRETIME);
		}
	}

	if (Obj.getFlag(MAGICFIELD)) {
		Obj.setAttribute(RESPONSIBLE, 0);
	}

newObject:
	AccessObject(Obj).Type = Type;

	if (Obj.getFlag(CUMULATIVE)) {
		Obj.setAttribute(AMOUNT, std::max<uint32_t>(Amount, 1));
	}

	if (Obj.getFlag(RUNE)) {
		if (Obj.getAttribute(CHARGES) == 0) {
			Obj.setAttribute(CHARGES, 1);
		}
	}

	if (Obj.getFlag(LIQUIDPOOL)) {
		if (Obj.getAttribute(POOLLIQUIDTYPE) == 0) {
			Obj.setAttribute(POOLLIQUIDTYPE, LIQUID_WATER);
		}
	}

	if (Obj.getFlag(WEAROUT)) {
		if (Obj.getAttribute(REMAININGUSES) == 0) {
			Obj.setAttribute(REMAININGUSES, Type.getAttribute(TOTALUSES));
		}
	}

	if (Obj.getFlag(EXPIRESTOP)) {
		Obj.setAttribute(SAVEDEXPIRETIME, SavedExpireTime);
	}

	CronExpire(Obj, Delay);
}

void ChangeObject(Object Obj, INSTANCEATTRIBUTE Attribute, uint32_t Value) {
	if (!Obj.exists()) {
		error("ChangeObject: \xdc" "bergebenes Objekt existiert nicht.\n");
		return;
	}

	Obj.setAttribute(Attribute, Value);
}

TObject *GetFreeObjectSlot() {
	if (FirstFreeObject == nullptr) {
		SwapSector();
	}

	TObject *Help = FirstFreeObject;
	if (Help == nullptr) {
		error("GetFreeObjectSlot: Kein freier Platz mehr.\n");
		return nullptr;
	}

	// The original keeps the pointer to the next free slot in the 32-bit
	// NextObject; here it lies over NextObject and Container.
	memcpy(&FirstFreeObject, &Help->NextObject, sizeof(TObject *));
	memset((void *)Help, 0, sizeof(TObject));
	return Help;
}

void ResizeHashTable() {
	int NewSize = HashTableSize * 2;
	int NewMask = NewSize - 1;
	error("INFO: HashTabelle zu klein. Gr\xf6\xdf" "e wird verdoppelt auf %d.\n", NewSize);

	uintptr_t *NewData = (uintptr_t *)malloc(NewSize * sizeof(uintptr_t));
	unsigned char *NewType = (unsigned char *)malloc(NewSize);
	memset(NewType, HASHENTRY_FREE, NewSize);
	NewType[0] = HASHENTRY_RESERVED;
	NewData[0] = HashTableData[0];
	for (int i = 1; i < HashTableSize; i++) {
		if (HashTableType[i] == HASHENTRY_FREE) {
			continue;
		}

		if (HashTableType[i] == HASHENTRY_SWAPPED_OUT) {
			UnswapSector((uint32_t)HashTableData[i]);
		}

		if (HashTableType[i] == HASHENTRY_IN_MEMORY) {
			TObject *Entry = (TObject *)HashTableData[i];
			uint32_t nr = Entry->ObjectID & NewMask;
			NewType[nr] = HASHENTRY_IN_MEMORY;
			NewData[nr] = (uintptr_t)Entry;
		} else {
			error("ResizeHashTable: Fehler beim Reorganisieren der HashTabelle.\n");
		}
	}

	free(HashTableType);
	free(HashTableData);
	HashTableData = NewData;
	HashTableType = NewType;
	HashTableFree += HashTableSize;
	HashTableSize = NewSize;
	HashTableMask = NewMask;
}

Object CreateObject() {
	static int Iterator = 0;

	if (HashTableFree < HashTableSize / 16) {
		ResizeHashTable();
	}

	while (HashTableType[Iterator & HashTableMask] != HASHENTRY_FREE) {
		Iterator++;
	}

	TObject *Help = GetFreeObjectSlot();
	if (Help == nullptr) {
		error("CreateObject: Kann Objekt nicht anlegen.\n");
		return NONE;
	}

	Help->ObjectID = Iterator;
	HashTableData[Iterator & HashTableMask] = (uintptr_t)Help;
	HashTableType[Iterator & HashTableMask] = HASHENTRY_IN_MEMORY;
	HashTableFree--;
	IncrementObjectCounter();
	return Object(Iterator);
}

// GetObjectPriority's results: the order of the objects in a field's list
// (PlaceObject). Two BANK, two BOTTOM or two TOP objects on one field are an
// error.
const int PRIORITY_BANK = 0;
const int PRIORITY_CLIP = 1;
const int PRIORITY_BOTTOM = 2;
const int PRIORITY_TOP = 3;
const int PRIORITY_CREATURE = 4;
const int PRIORITY_OTHER = 5;

int GetObjectPriority(Object Obj) {
	if (!Obj.exists()) {
		error("GetObjectPriority: \xdc" "bergebenes Objekt existiert nicht.\n");
		return 0x7FFFFFFF;
	}

	if (Obj.getFlag(BANK)) {
		return PRIORITY_BANK;
	}

	if (Obj.getFlag(CLIP)) {
		return PRIORITY_CLIP;
	}

	if (Obj.getFlag(BOTTOM)) {
		return PRIORITY_BOTTOM;
	}

	if (Obj.getFlag(TOP)) {
		return PRIORITY_TOP;
	}

	if (Obj.isCreature()) {
		return PRIORITY_CREATURE;
	}

	return PRIORITY_OTHER;
}

void PlaceObject(Object Obj, Object Con, bool Append) {
	Object Search;
	Object *pAddr;

	if (!Obj.exists()) {
		error("PlaceObject: \xdc" "bergebenes Objekt existiert nicht.\n");
		return;
	}

	if (!Con.exists()) {
		error("PlaceObject: \xdc" "bergebener Container existiert nicht.\n");
		return;
	}

	if (!Con.getFlag(CONTAINER) && !Con.getFlag(CHEST)) {
		error("PlaceObject: Con (%d) ist kein Container.\n", Con.getObjectType().getTypeID());
		return;
	}

	if (Con.isMapContainer()) {
		int ObjPriority = GetObjectPriority(Obj);
		pAddr = (Object *)&AccessObject(Con).Attributes[0];
		Search = GetFirstContainerObject(Con);
		while (Search != NONE) {
			int SearchPriority = GetObjectPriority(Search);
			if (SearchPriority > ObjPriority) {
				break;
			}

			if (!Append && (SearchPriority == PRIORITY_CREATURE || SearchPriority == PRIORITY_OTHER)) {
				if (SearchPriority == ObjPriority) {
					break;
				}
			} else if (SearchPriority == ObjPriority
					&& (ObjPriority == PRIORITY_BANK || ObjPriority == PRIORITY_BOTTOM
						|| ObjPriority == PRIORITY_TOP)) {
				int x, y, z;
				GetObjectCoordinates(Con, x, y, z);
				error("PlaceObject: Zwei %s-Objekte (%d und %d) auf Feld [%d,%d,%d].\n",
						ObjPriority == PRIORITY_BANK ? "BANK" : (ObjPriority == PRIORITY_BOTTOM ? "BOTTOM" : "TOP"),
						Obj.getObjectType().getTypeID(), Search.getObjectType().getTypeID(),
						x, y, z);
			}

			pAddr = &AccessObject(Search).NextObject;
			Search++;
		}
	} else {
		pAddr = (Object *)&AccessObject(Con).Attributes[0];
		Search = Con.getAttribute(CONTENT);
		if (Append) {
			while (Search != NONE) {
				pAddr = &AccessObject(Search).NextObject;
				Search++;
			}
		}
	}

	AccessObject(Obj).NextObject = Search;
	AccessObject(Obj).Container = Con;
	*pAddr = Obj;
}

Object AppendObject(Object Con, ObjectType Type) {
	if (!Con.exists()) {
		error("AppendObject: \xdc" "bergebener Container existiert nicht.\n");
		return NONE;
	}

	Object Obj = CreateObject();
	ChangeObject(Obj, Type);
	PlaceObject(Obj, Con, true);
	return Obj;
}

Object SetObject(Object Con, ObjectType Type, uint32_t CreatureID) {
	if (!Con.exists()) {
		error("SetObject: \xdc" "bergebener Container existiert nicht.\n");
		return NONE;
	}

	Object Obj = CreateObject();
	ChangeObject(Obj, Type);
	PlaceObject(Obj, Con, false);
	if (Obj.isCreature()) {
		if (CreatureID == 0) {
			error("SetObject: Ung\xfcltige Kreatur-ID.\n");
		}

		AccessObject(Obj).Attributes[1] = CreatureID;
	}

	return Obj;
}

Object CopyObject(Object Con, Object Source) {
	if (!Source.exists()) {
		error("CopyObject: Vorlage existiert nicht.\n");
		return NONE;
	}

	if (!Con.exists()) {
		error("CopyObject: Zielcontainer existiert nicht.\n");
		return NONE;
	}

	if (Source.isCreature()) {
		error("CopyObject: Kreaturen d\xfcrfen nicht kopiert werden.\n");
		return NONE;
	}

	Object Obj = SetObject(Con, Source.getObjectType(), 0);
	for (int i = 0; i < 4; i++) {
		// The original looks up the copy before the source; C++17 would
		// evaluate the right side of a plain assignment first.
		uint32_t *Attributes = AccessObject(Obj).Attributes;
		Attributes[i] = AccessObject(Source).Attributes[i];
	}

	if (Obj.getFlag(CONTAINER) || Obj.getFlag(CHEST)) {
		Obj.setAttribute(CONTENT, 0);
	}

	if (Obj.getFlag(TEXT) && Obj.getAttribute(TEXTSTRING) != 0) {
		Obj.setAttribute(TEXTSTRING, AddDynamicString(GetDynamicString(Obj.getAttribute(TEXTSTRING))));
	}

	if (Obj.getFlag(TEXT) && Obj.getAttribute(EDITOR) != 0) {
		Obj.setAttribute(EDITOR, AddDynamicString(GetDynamicString(Obj.getAttribute(EDITOR))));
	}

	return Obj;
}

void PutFreeObjectSlot(TObject *Entry) {
	if (Entry == nullptr) {
		error("PutFreeObjectSlot: Entry ist NULL.\n");
		return;
	}

	// See GetFreeObjectSlot for where the pointer is kept.
	memcpy((void *)&Entry->NextObject, &FirstFreeObject, sizeof(TObject *));
	FirstFreeObject = Entry;
}

void CutObject(Object Obj) {
	if (!Obj.exists()) {
		error("CutObject: \xdc" "bergebenes Objekt existiert nicht.\n");
		return;
	}

	Object Con = Obj.getContainer();
	Object Search = GetFirstContainerObject(Con);
	if (Search == Obj) {
		Con.setAttribute(CONTENT, Obj.getNextObject().getObjectID());
	} else {
		while (Search.getNextObject() != Obj) {
			Search++;
		}

		AccessObject(Search).NextObject = Obj.getNextObject();
	}

	AccessObject(Obj).NextObject = NONE;
	AccessObject(Obj).Container = NONE;
}

void DestroyObject(Object Obj) {
	if (!Obj.exists()) {
		error("DestroyObject: \xdc" "bergebenes Objekt existiert nicht.\n");
		return;
	}

	if (Obj.getFlag(TEXT)) {
		DeleteDynamicString(Obj.getAttribute(TEXTSTRING));
		DeleteDynamicString(Obj.getAttribute(EDITOR));
	}

	if (Obj.getFlag(CONTAINER) || Obj.getFlag(CHEST)) {
		while (Obj.getAttribute(CONTENT) != 0) {
			DeleteObject(Object(Obj.getAttribute(CONTENT)));
		}
	}

	if (Obj.getFlag(EXPIRE)) {
		CronStop(Obj);
	}

	uint32_t nr = Obj.getObjectID();
	if (nr == 0) {
		error("DestroyObject: Ung\xfcltige Objektnummer %d.\n", nr);
		return;
	}

	nr &= HashTableMask;
	if (HashTableType[nr] != HASHENTRY_IN_MEMORY) {
		error("DestroyObject: Objekt steht nicht im Speicher.\n");
		return;
	}

	PutFreeObjectSlot((TObject *)HashTableData[nr]);
	HashTableType[nr] = HASHENTRY_FREE;
	HashTableFree++;
	DecrementObjectCounter();
}

void DeleteObject(Object Obj) {
	if (!Obj.exists()) {
		error("DeleteObject: \xdc" "bergebenes Objekt existiert nicht.\n");
		return;
	}

	CutObject(Obj);
	DestroyObject(Obj);
}

Object GetMapContainer(int x, int y, int z) {
	int sx = x / SECTOR_SIZE;
	int sy = y / SECTOR_SIZE;
	if (!IsOnMap(x, y, z)) {
		return NONE;
	}

	if ((*Sector)(sx, sy, z) == nullptr) {
		return NONE;
	}

	(*Sector)(sx, sy, z)->TimeStamp = RoundNr;
	return (*Sector)(sx, sy, z)->MapCon[x % SECTOR_SIZE][y % SECTOR_SIZE];
}

Object GetMapContainer(Object Obj) {
	if (!Obj.exists()) {
		error("GetMapContainer: \xfc" "bergebenes Objekt existiert nicht\n");
		return NONE;
	}

	while (!Obj.isMapContainer()) {
		Obj = Obj.getContainer();
	}

	return Obj;
}

Object GetFirstObject(int x, int y, int z) {
	Object MapCon = GetMapContainer(x, y, z);
	if (MapCon != NONE) {
		return Object(MapCon.getAttribute(CONTENT));
	} else {
		return NONE;
	}
}

Object GetFirstSpecObject(int x, int y, int z, ObjectType Type) {
	for (Object Obj = GetFirstObject(x, y, z); Obj != NONE; Obj++) {
		if (Obj.getObjectType() == Type) {
			return Obj;
		}
	}

	return NONE;
}

Object GetFirstContainerObject(Object Con) {
	if (Con == NONE) {
		error("GetFirstContainerObject: \xdc" "bergebener Container existiert nicht.\n");
		return NONE;
	}

	if (!Con.getFlag(CONTAINER) && !Con.getFlag(CHEST)) {
		error("GetFirstContainerObject: Con (%d) ist kein Container.\n", Con.getObjectType().getTypeID());
		return NONE;
	}

	return Object(Con.getAttribute(CONTENT));
}

Object GetContainerObject(Object Con, int nr) {
	if (!Con.exists()) {
		error("GetContainerObject: \xdc" "bergebener Container existiert nicht.\n");
		return NONE;
	}

	if (nr < 0) {
		error("GetContainerObject: Ung\xfcltige laufende Nummer %d.\n", nr);
		return NONE;
	}

	Object Obj = GetFirstContainerObject(Con);
	while (Obj != NONE) {
		if (nr == 0) {
			return Obj;
		}

		nr--;
		Obj++;
	}

	return NONE;
}

bool IsOnMap(int x, int y, int z) {
	int sx = x / SECTOR_SIZE;
	int sy = y / SECTOR_SIZE;
	return in(sx, SectorXMin, SectorXMax)
			&& in(sy, SectorYMin, SectorYMax)
			&& in(z, SectorZMin, SectorZMax);
}

bool CoordinateFlag(int x, int y, int z, FLAG Flag) {
	for (Object Obj = GetFirstObject(x, y, z); Obj != NONE; Obj++) {
		if (Obj.getFlag(Flag)) {
			return true;
		}
	}

	return false;
}

bool IsPremiumArea(int x, int y, int) {
	if (x / SECTOR_SIZE >= 1036) {
		return true;
	}

	if (x / SECTOR_SIZE >= 1032 && y / SECTOR_SIZE <= 998) {
		return true;
	}

	if (x / SECTOR_SIZE >= 1030 && y / SECTOR_SIZE >= 1008) {
		return true;
	}

	if (x / SECTOR_SIZE >= 1029 && y / SECTOR_SIZE >= 1009) {
		return true;
	}

	if (x / SECTOR_SIZE >= 1022 && y / SECTOR_SIZE >= 1014) {
		return true;
	}

	return y / SECTOR_SIZE >= 1016;
}

bool IsProtectionZone(int x, int y, int z) {
	Object Con = GetMapContainer(x, y, z);
	if (Con == NONE) {
		return false;
	}

	return (GetMapContainerFlags(Con) & MAPFLAG_PROTECTIONZONE) != 0;
}

bool IsNoLogoutField(int x, int y, int z) {
	Object Con = GetMapContainer(x, y, z);
	if (Con == NONE) {
		return false;
	}

	return (GetMapContainerFlags(Con) & MAPFLAG_NOLOGOUT) != 0;
}

bool IsHouse(int x, int y, int z) {
	return GetHouseID(x, y, z) != 0;
}

unsigned short GetHouseID(int x, int y, int z) {
	Object Con = GetMapContainer(x, y, z);
	if (Con == NONE) {
		return 0;
	}

	if (!Con.exists()) {
		error("GetHouseID: Kartencontainer f\xfcr Punkt [%d,%d,%d] existiert nicht.\n", x, y, z);
		return 0;
	}

	return (unsigned short)(AccessObject(Con).Attributes[3] >> 16);
}

void SetHouseID(int x, int y, int z, unsigned short ID) {
	if (!IsOnMap(x, y, z)) {
		return;
	}

	Object Con = GetMapContainer(x, y, z);
	if (Con == NONE || !Con.exists()) {
		print(3, "SetHouseID: Kartencontainer f\xfcr Punkt [%d,%d,%d] existiert nicht.\n", x, y, z);
		return;
	}

	if (GetHouseID(x, y, z) != 0) {
		// The original passes no arguments; what its three stack slots still
		// hold at this point is y, z and z, so that is what it always logs.
		error("SetHouseID: Feld [%d,%d,%d] geh\xf6rt schon zu einem Haus.\n", y, z, z);
		return;
	}

	AccessObject(Con).Attributes[3] += (uint32_t)ID << 16;
}

int GetDepotNumber(char *Town) {
	if (Town == nullptr) {
		error("GetDepotNumber: Town ist NULL.\n");
		return -1;
	}

	if (Town[0] == 0) {
		return -1;
	}

	for (int i = DepotInfo.low(); i <= DepotInfo.high(); i++) {
		if (stricmp(DepotInfo(i).Town, Town, -1) == 0) {
			return i;
		}
	}

	return -1;
}

char *GetDepotName(int DepotNumber) {
	if (DepotNumber < DepotInfo.low() || DepotNumber > DepotInfo.high()) {
		error("GetDepotName: Ung\xfcltige Depotnummer %d.\n", DepotNumber);
		return "unknown";
	}

	// The original tests Town, the first member, against NULL; written with
	// the entry's address because g++ warns about the array form.
	if (&DepotInfo(DepotNumber) == nullptr) {
		error("GetDepotName: Ung\xfcltiger Name f\xfcr Depot %d.\n", DepotNumber);
		return "unknown";
	}

	return DepotInfo(DepotNumber).Town;
}

int GetDepotSize(int DepotNumber, bool PremiumAccount) {
	if (DepotNumber < DepotInfo.low() || DepotNumber > DepotInfo.high()) {
		error("GetDepotSize: Ung\xfcltige Depotnummer %d.\n", DepotNumber);
		return 1;
	}

	if (DepotInfo(DepotNumber).Size < 1) {
		error("GetDepotSize: Ung\xfcltige Depotgr\xf6\xdf" "e %d f\xfcr Depot %d.\n",
				DepotInfo(DepotNumber).Size, DepotNumber);
		DepotInfo(DepotNumber).Size = 1;
	}

	if (PremiumAccount) {
		return DepotInfo(DepotNumber).Size * 2;
	} else {
		return DepotInfo(DepotNumber).Size;
	}
}

void GetStartPosition(int &x, int &y, int &z, bool Newbie) {
	if (Newbie) {
		x = NewbieStartPositionX;
		y = NewbieStartPositionY;
		z = NewbieStartPositionZ;
	} else {
		x = VeteranStartPositionX;
		y = VeteranStartPositionY;
		z = VeteranStartPositionZ;
	}
}

bool GetMarkPosition(char *Name, int &x, int &y, int &z) {
	for (int i = 0; i < Marks; i++) {
		if (stricmp(Mark(i).Name, Name, -1) == 0) {
			x = Mark(i).x;
			y = Mark(i).y;
			z = Mark(i).z;
			return true;
		}
	}

	return false;
}

void InitSector(int x, int y, int z) {
	if ((*Sector)(x, y, z) != nullptr) {
		error("InitSector: Sektor %d/%d/%d existiert schon.\n", x, y, z);
		return;
	}

	TSector *Sec = (TSector *)malloc(sizeof(TSector));
	(*Sector)(x, y, z) = Sec;
	memset((void *)Sec, 0, sizeof(TSector));
	Sec->Status = SECTOR_INITIALIZING;
	Sec->TimeStamp = RoundNr;
	for (int xx = 0; xx < SECTOR_SIZE; xx++) {
		for (int yy = 0; yy < SECTOR_SIZE; yy++) {
			Object Obj = CreateObject();
			AccessObject(Obj).Attributes[1] = x * SECTOR_SIZE + xx;
			AccessObject(Obj).Attributes[2] = y * SECTOR_SIZE + yy;
			AccessObject(Obj).Attributes[3] = z;
			Sec->MapCon[xx][yy] = Obj;
		}
	}

	Sec->MapFlags = 0;
	Sec->Status = SECTOR_IN_MEMORY;
}

void SwapObject(TWriteBinaryFile &File, Object Obj, uint32_t FileNumber) {
	if (!Obj.exists()) {
		error("SwapObject: \xdc" "bergebenes Objekt existiert nicht.\n");
		return;
	}

	TObject *Entry = (TObject *)HashTableData[Obj.getObjectID() & HashTableMask];
	File.writeBytes((unsigned char *)Entry, sizeof(TObject));
	if (Obj.getFlag(CONTAINER) || Obj.getFlag(CHEST)) {
		Object Search = Object(Obj.getAttribute(CONTENT));
		while (Search != NONE) {
			Object Help = Search;
			Search++;
			SwapObject(File, Help, FileNumber);
		}
	}

	PutFreeObjectSlot(Entry);
	HashTableType[Obj.getObjectID() & HashTableMask] = HASHENTRY_SWAPPED_OUT;
	HashTableData[Obj.getObjectID() & HashTableMask] = FileNumber;
}

void SwapSector() {
	static uint32_t FileNumber = 0;
	char FileName[4096];
	int SecX, SecY, SecZ;

	TSector *Sec = nullptr;
	uint32_t Oldest = RoundNr + 1;
	SecX = 0;
	SecY = 0;
	SecZ = 0;
	for (int x = SectorXMin; x <= SectorXMax; x++) {
		for (int y = SectorYMin; y <= SectorYMax; y++) {
			for (int z = SectorZMin; z <= SectorZMax; z++) {
				TSector *Help = (*Sector)(x, y, z);
				if (Help != nullptr && Help->Status == SECTOR_IN_MEMORY && Help->TimeStamp < Oldest) {
					Sec = Help;
					SecX = x;
					SecY = y;
					SecZ = z;
					Oldest = Help->TimeStamp;
				}
			}
		}
	}

	if (Sec == nullptr) {
		error("FATAL ERROR in SwapSector: Es kann kein Sektor ausgelagert werden.\n");
		abort();
	}

again:
	FileNumber++;
	if (FileNumber > 99999999) {
		FileNumber = 1;
	}

	sprintf(FileName, "%s/%08ld.swp", SAVEPATH, (long)(int32_t)FileNumber);
	if (FileExists(FileName)) {
		goto again;
	}

	TWriteBinaryFile File;
	try {
		File.open(FileName);
		print(2, "Lagere Sektor %d/%d/%d aus...\n", SecX, SecY, SecZ);
		File.writeQuad(SecX);
		File.writeQuad(SecY);
		File.writeQuad(SecZ);
		for (int xx = 0; xx < SECTOR_SIZE; xx++) {
			for (int yy = 0; yy < SECTOR_SIZE; yy++) {
				SwapObject(File, Sec->MapCon[xx][yy], FileNumber);
			}
		}

		Sec->Status = SECTOR_SWAPPED_OUT;
		File.close();
	} catch (const char *s) {
		error("FATAL ERROR in SwapSector: Kann Datei \"%s\" nicht anlegen.\n", FileName);
		error("# Fehler: %s\n", s);
		abort();
	}
}

void UnswapSector(uint32_t FileNumber) {
	char FileName[4096];
	sprintf(FileName, "%s/%08ld.swp", SAVEPATH, (long)(int32_t)FileNumber);

	TReadBinaryFile File;
	try {
		File.open(FileName);
		int FileSize = File.getSize();
		int SecX = File.readQuad();
		int SecY = File.readQuad();
		int SecZ = File.readQuad();
		print(2, "Lagere Sector %d/%d/%d ein...\n", SecX, SecY, SecZ);
		TSector *Sec = (*Sector)(SecX, SecY, SecZ);
		if (Sec == nullptr) {
			error("UnswapSector: Sektor %d/%d/%d existiert nicht.\n", SecX, SecY, SecZ);
			File.close();
			return;
		}

		if (Sec->Status != SECTOR_SWAPPED_OUT) {
			error("UnswapSector: Sektor %d/%d/%d ist nicht ausgelagert.\n", SecX, SecY, SecZ);
			File.close();
			return;
		}

		while (File.getPosition() < FileSize) {
			TObject *Entry = GetFreeObjectSlot();
			File.readBytes((unsigned char *)Entry, sizeof(TObject));
			uint32_t nr = Entry->ObjectID & HashTableMask;
			if (HashTableType[nr] == HASHENTRY_SWAPPED_OUT) {
				HashTableData[nr] = (uintptr_t)Entry;
				HashTableType[nr] = HASHENTRY_IN_MEMORY;
			} else {
				error("UnswapSector: Objekt %ld existiert schon.\n", (long)(int32_t)Entry->ObjectID);
			}
		}

		Sec->Status = SECTOR_IN_MEMORY;
		File.close();
		unlink(FileName);
	} catch (const char *s) {
		error("FATAL ERROR in UnswapSector: Kann Datei \"%s\" nicht lesen.\n", FileName);
		error("# Fehler: %s\n", s);
		abort();
	}
}

void DeleteSwappedSectors() {
	dirent *DirEntry;
	DIR *WorkDir = opendir(SAVEPATH);
	if (WorkDir == nullptr) {
		error("DeleteSwappedSectors: Unterverzeichnis %s nicht gefunden\n", SAVEPATH);
		return;
	}

	while ((DirEntry = readdir(WorkDir)) != nullptr) {
		char *s = DirEntry->d_name;
		if (strlen(s) == 12 && strncmp(s + 8, ".swp", 4) == 0) {
			char FileName[4096];
			sprintf(FileName, "%s/%s", SAVEPATH, s);
			unlink(FileName);
		}
	}

	closedir(WorkDir);
}

int GetInstanceAttributeByName(char *Name) {
	for (int i = 0; i < INSTANCEATTRIBUTES; i++) {
		if (stricmp(InstanceAttributeNames[i], Name, -1) == 0) {
			return i;
		}
	}

	return -1;
}

void LoadObjects(TReadScriptFile &Script, TWriteStream &Stream, bool Skip) {
	int Depth = 1;
	Script.readSymbol('{');
	bool ProcessObjects = true;
	Script.nextToken();
	while (Depth > 0) {
		if (ProcessObjects) {
			unsigned short TypeID;
			if (Script.Token == SPECIAL && Script.getSpecial() == '}') {
				if (!Skip) {
					Stream.writeWord(END_OF_OBJECTS);
				}

				Depth--;
				ProcessObjects = false;
				if (Depth <= 0) {
					break;
				}
			} else {
				if (Script.Token == SPECIAL && Script.getSpecial() == ',') {
					Script.nextToken();
				}

				TypeID = Script.getNumber();
				if (!ObjectTypeExists(TypeID)) {
					throw "unknown object type";
				}

				if (!Skip) {
					Stream.writeWord(TypeID);
				}

				ProcessObjects = false;
			}

			Script.nextToken();
		} else {
			int Attribute;
			if (Script.Token == SPECIAL) {
				if (!Skip) {
					Stream.writeByte(END_OF_ATTRIBUTES);
				}

				ProcessObjects = true;
			} else {
				Attribute = GetInstanceAttributeByName(Script.getIdentifier());
				if (Attribute == -1) {
					throw "unknown attribute";
				}

				Script.readSymbol('=');
				if (!Skip) {
					Stream.writeByte((unsigned char)Attribute);
				}

				if (Attribute == CONTENT) {
					Depth++;
					Script.readSymbol('{');
					ProcessObjects = true;
				} else if (Attribute == TEXTSTRING || Attribute == EDITOR) {
					if (!Skip) {
						Stream.writeString(Script.readString());
					} else {
						Script.readString();
					}
				} else {
					if (!Skip) {
						Stream.writeQuad(Script.readNumber());
					} else {
						Script.readNumber();
					}
				}

				Script.nextToken();
			}
		}
	}
}

void LoadObjects(TReadStream &Stream, Object Con) {
	int Depth = 1;
	bool ProcessObjects = true;
	Object Obj = NONE;
	while (Depth > 0) {
		if (ProcessObjects) {
			unsigned short TypeID = Stream.readWord();
			if (TypeID == END_OF_OBJECTS) {
				Depth--;
				Obj = Con;
				Con = Con.getContainer();
			} else {
				ObjectCounter++;
				ObjectType Type(TypeID);
				if (Type.isBodyContainer()) {
					// A body container's type number is its body position.
					Obj = GetContainerObject(Con, TypeID - FIRST_BODY_POSITION);
				} else {
					Obj = AppendObject(Con, Type);
				}
			}

			ProcessObjects = false;
		} else {
			unsigned char Attribute = Stream.readByte();
			if (Attribute == END_OF_ATTRIBUTES) {
				ProcessObjects = true;
			} else if (Attribute == CONTENT) {
				Depth++;
				Con = Obj;
				ProcessObjects = true;
			} else if (Attribute == TEXTSTRING || Attribute == EDITOR) {
				char Help[4096];
				Stream.readString(Help, 4096);
				Obj.setAttribute((INSTANCEATTRIBUTE)Attribute, AddDynamicString(Help));
			} else if (Attribute == REMAININGEXPIRETIME) {
				uint32_t Value = Stream.readQuad();
				if (Value != 0) {
					CronChange(Obj, Value);
				}
			} else {
				uint32_t Value = Stream.readQuad();
				Obj.setAttribute((INSTANCEATTRIBUTE)Attribute, Value);
			}
		}
	}
}

void LoadSector(char *FileName, int x, int y, int z) {
	if (!in(x, SectorXMin, SectorXMax) || !in(y, SectorYMin, SectorYMax) || !in(z, SectorZMin, SectorZMax)) {
		return;
	}

	InitSector(x, y, z);

	TReadScriptFile Script;
	try {
		Script.open(FileName);
		print(1, "Lade Sektor %d/%d/%d ...\n", x, y, z);
		TSector *Sec = (*Sector)(x, y, z);
		int xx = -1;
		int yy = -1;
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
				continue;
			}

			if (Script.Token != IDENTIFIER) {
				Script.error("next map point expected");
			}

			if (xx == -1) {
				Script.error("coordinate expected");
			}

			if (strcmp(Script.getIdentifier(), "refresh") == 0) {
				Sec->MapFlags |= MAPFLAG_REFRESH;
				AccessObject(Sec->MapCon[xx][yy]).Attributes[3] |= MAPFLAG_REFRESH << 8;
			} else if (strcmp(Script.getIdentifier(), "nologout") == 0) {
				Sec->MapFlags |= MAPFLAG_NOLOGOUT;
				AccessObject(Sec->MapCon[xx][yy]).Attributes[3] |= MAPFLAG_NOLOGOUT << 8;
			} else if (strcmp(Script.getIdentifier(), "protectionzone") == 0) {
				Sec->MapFlags |= MAPFLAG_PROTECTIONZONE;
				AccessObject(Sec->MapCon[xx][yy]).Attributes[3] |= MAPFLAG_PROTECTIONZONE << 8;
			} else if (strcmp(Script.getIdentifier(), "content") == 0) {
				Script.readSymbol('=');
				HelpBuffer.rewind();
				LoadObjects(Script, HelpBuffer, false);
				TReadBuffer ReadBuffer(HelpBuffer.getData(), HelpBuffer.getDataSize());
				LoadObjects(ReadBuffer, GetMapContainer(x * SECTOR_SIZE + xx, y * SECTOR_SIZE + yy, z));
			} else {
				Script.error("unknown map flag");
			}
		}

		Script.close();
	} catch (const char *s) {
		error("LoadSector: Kann Datei \"%s\" nicht lesen.\n", FileName);
		error("# Fehler: %s\n", s);
		throw "Cannot load sector";
	}
}

void LoadMap() {
	dirent *DirEntry;
	char FileName[4096];
	int x, y, z;

	DIR *WorkDir = opendir(MAPPATH);
	if (WorkDir == nullptr) {
		error("LoadMap: Unterverzeichnis %s nicht gefunden\n", MAPPATH);
		throw "Cannot load map";
	}

	print(1, "Lade Karte ...\n");
	ObjectCounter = 0;
	int SectorCounter = 0;
	while ((DirEntry = readdir(WorkDir)) != nullptr) {
		if (strlen(DirEntry->d_name) > 8
				&& strcmp(DirEntry->d_name + strlen(DirEntry->d_name) - 4, ".sec") == 0) {
			if (sscanf(DirEntry->d_name, "%d-%d-%d.sec", &x, &y, &z) == 3) {
				sprintf(FileName, "%s/%s", MAPPATH, DirEntry->d_name);
				LoadSector(FileName, x, y, z);
				SectorCounter++;
			}
		}
	}

	closedir(WorkDir);
	print(1, "%d Sektoren geladen.\n", SectorCounter);
	print(1, "%d Objekte geladen.\n", ObjectCounter);
}

void SaveObjects(Object Obj, TWriteStream &Stream, bool Stop) {
	Object LastObj = NONE;
	int Depth = 1;
	bool ProcessObjects = true;
	while (Depth > 0) {
		if (ProcessObjects) {
			if (Obj == NONE) {
				Stream.writeWord(END_OF_OBJECTS);
				Depth--;
				if (Depth <= 0) {
					break;
				}

				Stream.writeByte(END_OF_ATTRIBUTES);
				ObjectCounter++;
				if (Stop && Depth == 1) {
					break;
				}

				if (LastObj == NONE) {
					error("LastObj ist NONE (1)\n");
				}

				LastObj = LastObj.getContainer();
				if (LastObj == NONE) {
					error("LastObj ist NONE (2)\n");
				}

				Obj = LastObj.getNextObject();
			} else {
				// The original repeats the test although it cannot hold here.
				if (Obj == NONE) {
					error("Obj ist NONE (1)\n");
				}

				if (Obj.isCreature()) {
					if (Stop && Depth == 1) {
						break;
					}

					LastObj = Obj;
					Obj++;
				} else {
					Stream.writeWord(Obj.getObjectType().getTypeID());
					ProcessObjects = false;
				}
			}
		} else {
			if (Obj == NONE) {
				error("Obj ist NONE (2)\n");
			}

			for (int Attribute = 0; Attribute <= INSTANCEATTRIBUTES - 1; Attribute++) {
				if (Attribute == CONTENT) {
					continue;
				}

				if (Obj.getObjectType().getAttributeOffset((INSTANCEATTRIBUTE)Attribute) != -1) {
					uint32_t Value;
					if (Attribute == REMAININGEXPIRETIME) {
						Value = CronInfo(Obj, false);
					} else {
						Value = Obj.getAttribute((INSTANCEATTRIBUTE)Attribute);
					}

					if (Value != 0) {
						Stream.writeByte((unsigned char)Attribute);
						if (Attribute == TEXTSTRING || Attribute == EDITOR) {
							Stream.writeString(GetDynamicString(Value));
						} else {
							Stream.writeQuad(Value);
						}
					}
				}
			}

			if (Obj.getObjectType().getAttributeOffset(CONTENT) != -1 && Obj.getAttribute(CONTENT) != 0) {
				Depth++;
				LastObj = NONE;
				Obj = GetFirstContainerObject(Obj);
				Stream.writeByte(CONTENT);
			} else {
				Stream.writeByte(END_OF_ATTRIBUTES);
				ObjectCounter++;
				if (Stop && Depth == 1) {
					break;
				}

				LastObj = Obj;
				Obj++;
			}

			ProcessObjects = true;
		}
	}
}

void SaveObjects(TReadStream &Stream, TWriteScriptFile &Script) {
	int Depth = 1;
	bool ProcessObjects = true;
	Script.writeText("{");
	bool FirstObject = true;
	while (Depth > 0) {
		if (ProcessObjects) {
			unsigned short TypeID = Stream.readWord();
			if (TypeID == END_OF_OBJECTS) {
				Depth--;
				ProcessObjects = false;
				Script.writeText("}");
			} else {
				if (!FirstObject) {
					Script.writeText(", ");
				}

				Script.writeNumber(TypeID);
				ProcessObjects = false;
			}
		} else {
			unsigned char Attribute = Stream.readByte();
			if (Attribute == END_OF_ATTRIBUTES) {
				ProcessObjects = true;
				FirstObject = false;
			} else {
				Script.writeText(" ");
				Script.writeText(InstanceAttributeNames[Attribute]);
				Script.writeText("=");
				if (Attribute == CONTENT) {
					Depth++;
					ProcessObjects = true;
					Script.writeText("{");
					FirstObject = true;
				} else if (Attribute == TEXTSTRING || Attribute == EDITOR) {
					char Help[4096];
					Stream.readString(Help, 4096);
					Script.writeString(Help);
				} else {
					Script.writeNumber(Stream.readQuad());
				}
			}
		}
	}
}

void SaveSector(char *FileName, int x, int y, int z) {
	TSector *Sec = (*Sector)(x, y, z);
	if (Sec == nullptr) {
		return;
	}

	TWriteScriptFile Script;
	try {
		Script.open(FileName);
		print(1, "Speichere Sektor %d/%d/%d ...\n", x, y, z);
		Script.writeText("# Tibia - graphical Multi-User-Dungeon");
		Script.writeLn();
		Script.writeText("# Data for sector ");
		Script.writeNumber(x);
		Script.writeText("/");
		Script.writeNumber(y);
		Script.writeText("/");
		Script.writeNumber(z);
		Script.writeLn();
		Script.writeLn();

		bool Empty = true;
		for (int xx = 0; xx < SECTOR_SIZE; xx++) {
			for (int yy = 0; yy < SECTOR_SIZE; yy++) {
				Object Obj = GetFirstObject(x * SECTOR_SIZE + xx, y * SECTOR_SIZE + yy, z);
				int Flags = GetMapContainerFlags(Sec->MapCon[xx][yy]);
				if (Obj == NONE && Flags == 0) {
					continue;
				}

				Script.writeNumber(xx);
				Script.writeText("-");
				Script.writeNumber(yy);
				Script.writeText(": ");
				if (Flags & MAPFLAG_REFRESH) {
					Script.writeText("Refresh, ");
				}

				if (Flags & MAPFLAG_NOLOGOUT) {
					Script.writeText("NoLogout, ");
				}

				if (Flags & MAPFLAG_PROTECTIONZONE) {
					Script.writeText("ProtectionZone, ");
				}

				if (Obj != NONE) {
					Script.writeText("Content=");
					HelpBuffer.rewind();
					SaveObjects(Obj, HelpBuffer, false);
					TReadBuffer ReadBuffer(HelpBuffer.getData(), HelpBuffer.getDataSize());
					SaveObjects(ReadBuffer, Script);
				}

				Script.writeLn();
				Empty = false;
			}
		}

		Script.close();
		if (Empty) {
			error("SaveSector: Sektor %d/%d/%d ist leer.\n", x, y, z);
			unlink(FileName);
		}
	} catch (const char *s) {
		error("SaveSector: Kann Datei %s nicht schreiben.\n", FileName);
		error("# Fehler: %s\n", s);
	}
}

void SaveMap() {
	static bool SavingMap = false;
	char FileName[4096];

	if (SavingMap) {
		error("SaveMap: Karte wird schon gespeichert.\n");
		return;
	}

	SavingMap = true;
	print(1, "Speichere Karte ...\n");
	ObjectCounter = 0;
	for (int x = SectorXMin; x <= SectorXMax; x++) {
		for (int y = SectorYMin; y <= SectorYMax; y++) {
			for (int z = SectorZMin; z <= SectorZMax; z++) {
				sprintf(FileName, "%s/%04u-%04u-%02u.sec", MAPPATH, x, y, z);
				SaveSector(FileName, x, y, z);
			}
		}
	}

	print(1, "%d Objekte gespeichert.\n", ObjectCounter);
	SavingMap = false;
}

void RefreshSector(int x, int y, int z, TReadStream &Stream) {
	TSector *Sec = (*Sector)(x, y, z);
	if (Sec == nullptr || (Sec->MapFlags & MAPFLAG_REFRESH) == 0) {
		return;
	}

	print(3, "Refreshe Sektor %d/%d/%d ...\n", x, y, z);
	try {
		while (!Stream.eof()) {
			int xx = Stream.readByte();
			int yy = Stream.readByte();
			while (true) {
				Object Obj = GetFirstObject(x * SECTOR_SIZE + xx, y * SECTOR_SIZE + yy, z);
				while (Obj != NONE && Obj.isCreature()) {
					Obj++;
				}

				if (Obj == NONE) {
					break;
				}

				DeleteObject(Obj);
			}

			LoadObjects(Stream, GetMapContainer(x * SECTOR_SIZE + xx, y * SECTOR_SIZE + yy, z));
		}
	} catch (const char *s) {
		error("RefreshSector: Fehler beim Auslesen der Daten (%s).\n", s);
	}
}

void PatchSector(int x, int y, int z, bool FullSector, TReadScriptFile &Script, bool SaveHouses) {
	bool NewSector = ((*Sector)(x, y, z) == nullptr);
	if (NewSector) {
		print(2, "Lege Sektor %d/%d/%d neu an.\n", x, y, z);
		InitSector(x, y, z);
	}

	TSector *Sec = (*Sector)(x, y, z);
	bool FieldPatched[SECTOR_SIZE][SECTOR_SIZE], FieldTreated[SECTOR_SIZE][SECTOR_SIZE];
	int xx = -1, yy = -1;
	bool House = false;
	for (int hx = 0; hx < SECTOR_SIZE; hx++) {
		for (int hy = 0; hy < SECTOR_SIZE; hy++) {
			FieldPatched[hx][hy] = false;
			FieldTreated[hx][hy] = false;
		}
	}

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
			FieldTreated[xx][yy] = true;
			Script.readSymbol(':');
			House = IsHouse(x * SECTOR_SIZE + xx, y * SECTOR_SIZE + yy, z)
					|| (CoordinateFlag(x * SECTOR_SIZE + xx, y * SECTOR_SIZE + yy, z, HOOKSOUTH)
						&& (IsHouse(x * SECTOR_SIZE + xx - 1, y * SECTOR_SIZE + yy + 1, z)
							|| IsHouse(x * SECTOR_SIZE + xx, y * SECTOR_SIZE + yy + 1, z)
							|| IsHouse(x * SECTOR_SIZE + xx + 1, y * SECTOR_SIZE + yy + 1, z)))
					|| (CoordinateFlag(x * SECTOR_SIZE + xx, y * SECTOR_SIZE + yy, z, HOOKEAST)
						&& (IsHouse(x * SECTOR_SIZE + xx + 1, y * SECTOR_SIZE + yy - 1, z)
							|| IsHouse(x * SECTOR_SIZE + xx + 1, y * SECTOR_SIZE + yy, z)
							|| IsHouse(x * SECTOR_SIZE + xx + 1, y * SECTOR_SIZE + yy + 1, z)));
			if (House) {
				if (SaveHouses) {
					continue;
				}

				CleanHouseField(x * SECTOR_SIZE + xx, y * SECTOR_SIZE + yy, z);
			}

			while (true) {
				Object Obj = GetFirstObject(x * SECTOR_SIZE + xx, y * SECTOR_SIZE + yy, z);
				while (Obj != NONE && Obj.isCreature()) {
					Obj++;
				}

				if (Obj == NONE) {
					break;
				}

				DeleteObject(Obj);
			}

			AccessObject(Sec->MapCon[xx][yy]).Attributes[3] &= 0xFFFF00FF;
			FieldPatched[xx][yy] = true;
			continue;
		}

		if (Script.Token != IDENTIFIER) {
			Script.error("next map point expected");
		}

		if (xx == -1) {
			Script.error("coordinate expected");
		}

		if (strcmp(Script.getIdentifier(), "refresh") == 0) {
			Sec->MapFlags |= MAPFLAG_REFRESH;
			if (!House || !SaveHouses) {
				AccessObject(Sec->MapCon[xx][yy]).Attributes[3] |= MAPFLAG_REFRESH << 8;
			}
		} else if (strcmp(Script.getIdentifier(), "nologout") == 0) {
			Sec->MapFlags |= MAPFLAG_NOLOGOUT;
			if (!House || !SaveHouses) {
				AccessObject(Sec->MapCon[xx][yy]).Attributes[3] |= MAPFLAG_NOLOGOUT << 8;
			}
		} else if (strcmp(Script.getIdentifier(), "protectionzone") == 0) {
			Sec->MapFlags |= MAPFLAG_PROTECTIONZONE;
			if (!House || !SaveHouses) {
				AccessObject(Sec->MapCon[xx][yy]).Attributes[3] |= MAPFLAG_PROTECTIONZONE << 8;
			}
		} else if (strcmp(Script.getIdentifier(), "content") == 0) {
			Script.readSymbol('=');
			HelpBuffer.rewind();
			if (House && SaveHouses) {
				LoadObjects(Script, HelpBuffer, true);
			} else {
				LoadObjects(Script, HelpBuffer, false);
				TReadBuffer ReadBuffer(HelpBuffer.getData(), HelpBuffer.getDataSize());
				LoadObjects(ReadBuffer, GetMapContainer(x * SECTOR_SIZE + xx, y * SECTOR_SIZE + yy, z));
			}
		} else {
			Script.error("unknown map flag");
		}
	}

	if (FullSector) {
		for (int xx = 0; xx < SECTOR_SIZE; xx++) {
			for (int yy = 0; yy < SECTOR_SIZE; yy++) {
				if (FieldTreated[xx][yy]) {
					continue;
				}

				House = IsHouse(x * SECTOR_SIZE + xx, y * SECTOR_SIZE + yy, z)
						|| (CoordinateFlag(x * SECTOR_SIZE + xx, y * SECTOR_SIZE + yy, z, HOOKSOUTH)
							&& (IsHouse(x * SECTOR_SIZE + xx - 1, y * SECTOR_SIZE + yy + 1, z)
								|| IsHouse(x * SECTOR_SIZE + xx, y * SECTOR_SIZE + yy + 1, z)
								|| IsHouse(x * SECTOR_SIZE + xx + 1, y * SECTOR_SIZE + yy + 1, z)))
						|| (CoordinateFlag(x * SECTOR_SIZE + xx, y * SECTOR_SIZE + yy, z, HOOKEAST)
							&& (IsHouse(x * SECTOR_SIZE + xx + 1, y * SECTOR_SIZE + yy - 1, z)
								|| IsHouse(x * SECTOR_SIZE + xx + 1, y * SECTOR_SIZE + yy, z)
								|| IsHouse(x * SECTOR_SIZE + xx + 1, y * SECTOR_SIZE + yy + 1, z)));
				if (House) {
					if (SaveHouses) {
						continue;
					}

					CleanHouseField(x * SECTOR_SIZE + xx, y * SECTOR_SIZE + yy, z);
				}

				while (true) {
					Object Obj = GetFirstObject(x * SECTOR_SIZE + xx, y * SECTOR_SIZE + yy, z);
					while (Obj != NONE && Obj.isCreature()) {
						Obj++;
					}

					if (Obj == NONE) {
						break;
					}

					DeleteObject(Obj);
				}

				AccessObject(Sec->MapCon[xx][yy]).Attributes[3] &= 0xFFFF00FF;
				FieldPatched[xx][yy] = true;
			}
		}
	}

	TReadScriptFile IN;
	TWriteScriptFile OUT;
	char FileName[4096], FileNameBak[4096];
	sprintf(FileName, "%s/%04u-%04u-%02u.sec", ORIGMAPPATH, x, y, z);
	sprintf(FileNameBak, "%s/%04u-%04u-%02u.sec~", ORIGMAPPATH, x, y, z);
	if (!NewSector) {
		IN.open(FileName);
	}

	OUT.open(FileNameBak);
	OUT.writeText("# Tibia - graphical Multi-User-Dungeon");
	OUT.writeLn();
	OUT.writeText("# Data for sector ");
	OUT.writeNumber(x);
	OUT.writeText("/");
	OUT.writeNumber(y);
	OUT.writeText("/");
	OUT.writeNumber(z);
	OUT.writeLn();
	OUT.writeLn();

	if (!NewSector) {
		xx = -1;
		yy = -1;
		while (true) {
			IN.nextToken();
			if (IN.Token == ENDOFFILE) {
				IN.close();
				break;
			}

			if (IN.Token == SPECIAL && IN.getSpecial() == ',') {
				continue;
			}

			if (IN.Token == BYTES) {
				unsigned char *Help = IN.getBytesequence();
				xx = Help[0];
				yy = Help[1];
				IN.readSymbol(':');
				if (!FieldPatched[xx][yy]) {
					OUT.writeNumber(xx);
					OUT.writeText("-");
					OUT.writeNumber(yy);
					OUT.writeText(": ");
				}

				continue;
			}

			if (IN.Token != IDENTIFIER) {
				IN.error("next map point expected");
			}

			if (xx == -1) {
				IN.error("coordinate expected");
			}

			if (strcmp(IN.getIdentifier(), "refresh") == 0) {
				if (!FieldPatched[xx][yy]) {
					OUT.writeText("Refresh, ");
				}
			} else if (strcmp(IN.getIdentifier(), "nologout") == 0) {
				if (!FieldPatched[xx][yy]) {
					OUT.writeText("NoLogout, ");
				}
			} else if (strcmp(IN.getIdentifier(), "protectionzone") == 0) {
				if (!FieldPatched[xx][yy]) {
					OUT.writeText("ProtectionZone, ");
				}
			} else if (strcmp(IN.getIdentifier(), "content") == 0) {
				IN.readSymbol('=');
				HelpBuffer.rewind();
				LoadObjects(IN, HelpBuffer, false);
				if (!FieldPatched[xx][yy]) {
					OUT.writeText("Content=");
					TReadBuffer ReadBuffer(HelpBuffer.getData(), HelpBuffer.getDataSize());
					SaveObjects(ReadBuffer, OUT);
					OUT.writeLn();
				}
			} else {
				IN.error("unknown map flag");
			}
		}
	}

	for (int xx = 0; xx < SECTOR_SIZE; xx++) {
		for (int yy = 0; yy < SECTOR_SIZE; yy++) {
			if (!FieldPatched[xx][yy]) {
				continue;
			}

			Object Obj = GetFirstObject(x * SECTOR_SIZE + xx, y * SECTOR_SIZE + yy, z);
			int Flags = GetMapContainerFlags(Sec->MapCon[xx][yy]);
			if (Obj == NONE && Flags == 0) {
				continue;
			}

			OUT.writeNumber(xx);
			OUT.writeText("-");
			OUT.writeNumber(yy);
			OUT.writeText(": ");
			if (Flags & MAPFLAG_REFRESH) {
				OUT.writeText("Refresh, ");
			}

			if (Flags & MAPFLAG_NOLOGOUT) {
				OUT.writeText("NoLogout, ");
			}

			if (Flags & MAPFLAG_PROTECTIONZONE) {
				OUT.writeText("ProtectionZone, ");
			}

			if (Obj != NONE) {
				OUT.writeText("Content=");
				HelpBuffer.rewind();
				SaveObjects(Obj, HelpBuffer, false);
				TReadBuffer ReadBuffer(HelpBuffer.getData(), HelpBuffer.getDataSize());
				SaveObjects(ReadBuffer, OUT);
			}

			OUT.writeLn();
		}
	}

	OUT.close();
	if (!NewSector) {
		unlink(FileName);
	}

	int result = rename(FileNameBak, FileName);
	if (result != 0) {
		error("PatchSector: Fehler %d beim Umbenennen von %s.\n", result, FileNameBak);
		error("# Fehler %d: %s.\n", errno, strerror(errno));
		throw "cannot patch ORIGMAP";
	}
}

void ReadMapConfig() {
	SectorXMin = 1000;
	SectorXMax = 1015;
	SectorYMin = 1000;
	SectorYMax = 1015;
	SectorZMin = 0;
	SectorZMax = 15;
	RefreshedCylinders = 1;
	HashTableSize = 1048576;
	OBCount = 655360;
	Marks = 0;
	NewbieStartPositionX = 0;
	NewbieStartPositionY = 0;
	NewbieStartPositionZ = 0;
	VeteranStartPositionX = 0;
	VeteranStartPositionY = 0;
	VeteranStartPositionZ = 0;

	char FileName[4096];
	sprintf(FileName, "%s/map.dat", DATAPATH);

	TReadScriptFile Script;
	Script.open(FileName);
	while (true) {
		Script.nextToken();
		if (Script.Token == ENDOFFILE) {
			Script.close();
			break;
		}

		if (Script.Token != IDENTIFIER) {
			Script.error("Identifier expected");
		}

		char Ident[30];
		strcpy(Ident, Script.getIdentifier());
		Script.readSymbol('=');

		if (strcmp(Ident, "sectorxmin") == 0) {
			SectorXMin = Script.readNumber();
		}

		if (strcmp(Ident, "sectorxmax") == 0) {
			SectorXMax = Script.readNumber();
		}

		if (strcmp(Ident, "sectorymin") == 0) {
			SectorYMin = Script.readNumber();
		}

		if (strcmp(Ident, "sectorymax") == 0) {
			SectorYMax = Script.readNumber();
		}

		if (strcmp(Ident, "sectorzmin") == 0) {
			SectorZMin = Script.readNumber();
		}

		if (strcmp(Ident, "sectorzmax") == 0) {
			SectorZMax = Script.readNumber();
		}

		if (strcmp(Ident, "refreshedcylinders") == 0) {
			RefreshedCylinders = Script.readNumber();
		}

		if (strcmp(Ident, "objects") == 0) {
			HashTableSize = Script.readNumber();
		}

		if (strcmp(Ident, "cachesize") == 0) {
			OBCount = Script.readNumber();
		}

		if (strcmp(Ident, "depot") == 0) {
			Script.readSymbol('(');
			int DepotNumber = Script.readNumber();
			Script.readSymbol(',');
			Script.nextToken();
			if (Script.Token != STRING) {
				Script.error("town name expected");
			}

			if (strlen(Script.getString()) > 19) {
				Script.error("town name too long");
			}

			strcpy(DepotInfo(DepotNumber).Town, Script.getString());
			Script.readSymbol(',');
			DepotInfo(DepotNumber).Size = Script.readNumber();
			Script.readSymbol(')');
		}

		if (strcmp(Ident, "mark") == 0) {
			Script.readSymbol('(');
			strcpy(Mark(Marks).Name, Script.readString());
			Script.readSymbol(',');
			Script.readCoordinate(Mark(Marks).x, Mark(Marks).y, Mark(Marks).z);
			Script.readSymbol(')');
			Marks++;
		}

		if (strcmp(Ident, "newbiestart") == 0) {
			Script.readCoordinate(NewbieStartPositionX, NewbieStartPositionY, NewbieStartPositionZ);
		}

		if (strcmp(Ident, "veteranstart") == 0) {
			Script.readCoordinate(VeteranStartPositionX, VeteranStartPositionY, VeteranStartPositionZ);
		}
	}

	if (SectorXMin <= 0) {
		throw "illegal value for SectorXMin";
	}

	if (SectorXMax >= 2047) {
		throw "illegal value for SectorXMax";
	}

	if (SectorYMin <= 0) {
		throw "illegal value for SectorYMin";
	}

	if (SectorYMax >= 2047) {
		throw "illegal value for SectorYMax";
	}

	if (SectorZMin < 0) {
		throw "illegal value for SectorZMin";
	}

	if (SectorZMax > 15) {
		throw "illegal value for SectorZMax";
	}

	if (SectorXMin > SectorXMax) {
		throw "SectorXMin is greater than SectorXMax";
	}

	if (SectorYMin > SectorYMax) {
		throw "SectorYMin is greater than SectorYMax";
	}

	if (SectorZMin > SectorZMax) {
		throw "SectorZMin is greater than SectorZMax";
	}

	if (OBCount % OBJECTS_PER_BLOCK != 0) {
		throw "CacheSize must be a multiple of 32768";
	}

	if (OBCount <= 0) {
		throw "illegal value for CacheSize";
	}

	OBCount /= OBJECTS_PER_BLOCK;
	if (HashTableSize <= 0) {
		throw "illegal value for Objects";
	}

	if ((HashTableSize & (HashTableSize - 1)) != 0) {
		throw "Objects must be a power of 2";
	}

	if (NewbieStartPositionX == 0) {
		throw "no start position for newbies specified";
	}

	if (VeteranStartPositionX == 0) {
		throw "no start position for veterans specified";
	}
}

void InitMap() {
	ReadMapConfig();
	Sector = new matrix3d<TSector *>(SectorXMin, SectorXMax, SectorYMin, SectorYMax, SectorZMin, SectorZMax, nullptr);
	DeleteSwappedSectors();

	ObjectBlock = (TObjectBlock **)malloc(OBCount * sizeof(TObjectBlock *));
	for (int i = 0; i < OBCount; i++) {
		ObjectBlock[i] = (TObjectBlock *)malloc(sizeof(TObjectBlock));
	}

	for (int i = 0; i < OBCount; i++) {
		// See GetFreeObjectSlot for where the pointer is kept.
		TObject *Next;
		for (int j = 0; j < OBJECTS_PER_BLOCK - 1; j++) {
			Next = &ObjectBlock[i]->Object[j + 1];
			memcpy((void *)&ObjectBlock[i]->Object[j].NextObject, &Next, sizeof(TObject *));
		}

		if (i < OBCount - 1) {
			Next = ObjectBlock[i + 1]->Object;
		} else {
			Next = nullptr;
		}

		memcpy((void *)&ObjectBlock[i]->Object[OBJECTS_PER_BLOCK - 1].NextObject, &Next, sizeof(TObject *));
	}

	FirstFreeObject = ObjectBlock[0]->Object;
	HashTableMask = HashTableSize - 1;
	HashTableData = (uintptr_t *)malloc(HashTableSize * sizeof(uintptr_t));
	HashTableType = (unsigned char *)malloc(HashTableSize);
	memset(HashTableType, HASHENTRY_FREE, HashTableSize);
	HashTableFree = HashTableSize - 1;
	HashTableType[0] = HASHENTRY_RESERVED;
	HashTableData[0] = (uintptr_t)GetFreeObjectSlot();

	for (int i = 0; i < CRON_HASH_TABLE_SIZE; i++) {
		CronHashTable[i] = -1;
	}

	CronEntries = 0;
	LoadMap();
}

void ExitMap(bool save) {
	if (save) {
		SaveMap();
	}

	free(HashTableType);
	free(HashTableData);
	for (int i = 0; i < OBCount; i++) {
		free(ObjectBlock[i]);
	}

	free(ObjectBlock);
	for (int x = SectorXMin; x <= SectorXMax; x++) {
		for (int y = SectorYMin; y <= SectorYMax; y++) {
			for (int z = SectorZMin; z <= SectorZMax; z++) {
				if ((*Sector)(x, y, z) != nullptr) {
					free((*Sector)(x, y, z));
				}
			}
		}
	}

	delete Sector;
	DeleteSwappedSectors();
}
