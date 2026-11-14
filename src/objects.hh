#ifndef PATELLA_OBJECTS_HH
#define PATELLA_OBJECTS_HH

#include <cstdint>
#include <cstdlib>

#include "containr.hh"

struct TObjectType;

enum FLAG {
	BANK = 0,
	CLIP = 1,
	BOTTOM = 2,
	TOP = 3,
	CONTAINER = 4,
	CHEST = 5,
	CUMULATIVE = 6,
	USEEVENT = 7,
	CHANGEUSE = 8,
	FORCEUSE = 9,
	MULTIUSE = 10,
	DISTUSE = 11,
	MOVEMENTEVENT = 12,
	COLLISIONEVENT = 13,
	SEPARATIONEVENT = 14,
	KEY = 15,
	KEYDOOR = 16,
	NAMEDOOR = 17,
	LEVELDOOR = 18,
	QUESTDOOR = 19,
	BED = 20,
	FOOD = 21,
	RUNE = 22,
	INFORMATION = 23,
	TEXT = 24,
	WRITE = 25,
	WRITEONCE = 26,
	LIQUIDCONTAINER = 27,
	LIQUIDSOURCE = 28,
	LIQUIDPOOL = 29,
	TELEPORTABSOLUTE = 30,
	TELEPORTRELATIVE = 31,
	UNPASS = 32,
	UNMOVE = 33,
	UNTHROW = 34,
	UNLAY = 35,
	AVOID = 36,
	MAGICFIELD = 37,
	RESTRICTLEVEL = 38,
	RESTRICTPROFESSION = 39,
	TAKE = 40,
	HANG = 41,
	HOOKSOUTH = 42,
	HOOKEAST = 43,
	ROTATE = 44,
	DESTROY = 45,
	CLOTHES = 46,
	SKILLBOOST = 47,
	PROTECTION = 48,
	LIGHT = 49,
	ROPESPOT = 50,
	CORPSE = 51,
	EXPIRE = 52,
	EXPIRESTOP = 53,
	WEAROUT = 54,
	WEAPON = 55,
	SHIELD = 56,
	BOW = 57,
	THROW = 58,
	WAND = 59,
	AMMO = 60,
	ARMOR = 61,
	HEIGHT = 62,
	DISGUISE = 63,
	SHOWDETAIL = 64,
	SPECIALOBJECT = 65,
};

const int FLAGS = 66;

enum TYPEATTRIBUTE {
	WAYPOINTS = 0,
	CAPACITY = 1,
	CHANGETARGET = 2,
	KEYDOORTARGET = 3,
	NAMEDOORTARGET = 4,
	LEVELDOORTARGET = 5,
	QUESTDOORTARGET = 6,
	NUTRITION = 7,
	INFORMATIONTYPE = 8,
	FONTSIZE = 9,
	MAXLENGTH = 10,
	MAXLENGTHONCE = 11,
	SOURCELIQUIDTYPE = 12,
	ABSTELEPORTEFFECT = 13,
	RELTELEPORTDISPLACEMENT = 14,
	RELTELEPORTEFFECT = 15,
	AVOIDDAMAGETYPES = 16,
	MINIMUMLEVEL = 17,
	PROFESSIONS = 18,
	WEIGHT = 19,
	ROTATETARGET = 20,
	DESTROYTARGET = 21,
	BODYPOSITION = 22,
	SKILLNUMBER = 23,
	SKILLMODIFICATION = 24,
	PROTECTIONDAMAGETYPES = 25,
	DAMAGEREDUCTION = 26,
	BRIGHTNESS = 27,
	LIGHTCOLOR = 28,
	CORPSETYPE = 29,
	TOTALEXPIRETIME = 30,
	EXPIRETARGET = 31,
	TOTALUSES = 32,
	WEAROUTTARGET = 33,
	WEAPONTYPE = 34,
	WEAPONATTACKVALUE = 35,
	WEAPONDEFENDVALUE = 36,
	SHIELDDEFENDVALUE = 37,
	BOWRANGE = 38,
	BOWAMMOTYPE = 39,
	THROWRANGE = 40,
	THROWATTACKVALUE = 41,
	THROWDEFENDVALUE = 42,
	THROWMISSILE = 43,
	THROWSPECIALEFFECT = 44,
	THROWEFFECTSTRENGTH = 45,
	THROWFRAGILITY = 46,
	WANDRANGE = 47,
	WANDMANACONSUMPTION = 48,
	WANDATTACKSTRENGTH = 49,
	WANDATTACKVARIATION = 50,
	WANDDAMAGETYPE = 51,
	WANDMISSILE = 52,
	AMMOTYPE = 53,
	AMMOATTACKVALUE = 54,
	AMMOMISSILE = 55,
	AMMOSPECIALEFFECT = 56,
	AMMOEFFECTSTRENGTH = 57,
	ARMORVALUE = 58,
	ELEVATION = 59,
	DISGUISETARGET = 60,
	MEANING = 61,
};

const int TYPEATTRIBUTES = 62;

// What an INFORMATION object tells, its INFORMATIONTYPE (moveuse.cc's
// UseAnnouncer): the date and the time, the time, the blessings received, or
// the player's spells (the spellbook; sending.cc's SendEditText shows them).
const int INFORMATION_DATE = 1;
const int INFORMATION_TIME = 2;
const int INFORMATION_BLESSINGS = 3;
const int INFORMATION_SPELLBOOK = 4;

enum INSTANCEATTRIBUTE {
	CONTENT = 0,
	CHESTQUESTNUMBER = 1,
	AMOUNT = 2,
	KEYNUMBER = 3,
	KEYHOLENUMBER = 4,
	DOORLEVEL = 5,
	DOORQUESTNUMBER = 6,
	DOORQUESTVALUE = 7,
	CHARGES = 8,
	TEXTSTRING = 9,
	EDITOR = 10,
	CONTAINERLIQUIDTYPE = 11,
	POOLLIQUIDTYPE = 12,
	ABSTELEPORTDESTINATION = 13,
	RESPONSIBLE = 14,
	REMAININGEXPIRETIME = 15,
	SAVEDEXPIRETIME = 16,
	REMAININGUSES = 17,
};

const int INSTANCEATTRIBUTES = 18;

// The most pieces a cumulative object can hold (its AMOUNT).
const int MAX_AMOUNT = 100;

enum SPECIALMEANING {
	MONEY_ONE = 1,
	MONEY_HUNDRED = 2,
	MONEY_TENTHOUSAND = 3,
	INVENTORY_RIGHTHAND = 10,
	INVENTORY_LEFTHAND = 11,
	INVENTORY_BODY_MALE = 12,
	INVENTORY_BODY_FEMALE = 13,
	INVENTORY_CONTAINER = 14,
	INVENTORY_FOOD = 15,
	DEPOT_LOCKER = 20,
	DEPOT_CHEST = 21,
	PARCEL_NEW = 22,
	PARCEL_STAMPED = 23,
	PARCEL_LABEL = 24,
	LETTER_NEW = 25,
	LETTER_STAMPED = 26,
	BLOOD_SPLASH = 30,
	BLOOD_POOL = 31,
	RUNE_BLANK = 40,
	MAGICFIELD_FIRE_DANGEROUS = 41,
	MAGICFIELD_FIRE_HARMLESS = 42,
	MAGICFIELD_POISON_DANGEROUS = 43,
	MAGICFIELD_POISON_HARMLESS = 44,
	MAGICFIELD_ENERGY_DANGEROUS = 45,
	MAGICFIELD_ENERGY_HARMLESS = 46,
	MAGICFIELD_MAGICWALL = 47,
	MAGICFIELD_RUSHWOOD = 48,
};

const int SPECIALMEANINGS = 49;

// The body positions: the inventory slots, the y of an inventory object's
// coordinates (x is POSITION_NOT_ON_MAP), the value of the BODYPOSITION
// attribute of CLOTHES, and the type numbers of the body containers
// (isBodyContainer). BODY_BOTHHANDS is the BODYPOSITION of CLOTHES that take
// both hands (two-handed weapons); it is no slot.
const int BODY_BOTHHANDS = 0;
const int BODY_HEAD = 1;
const int BODY_NECK = 2;
const int BODY_CONTAINER = 3;
const int BODY_TORSO = 4;
const int BODY_RIGHTHAND = 5;
const int BODY_LEFTHAND = 6;
const int BODY_LEGS = 7;
const int BODY_FEET = 8;
const int BODY_FINGER = 9;
const int BODY_AMMO = 10;
const int FIRST_BODY_POSITION = 1;
const int LAST_BODY_POSITION = 10;

// The liquids: the values of CONTAINERLIQUIDTYPE, POOLLIQUIDTYPE and
// SOURCELIQUIDTYPE, named as GetLiquidName names them.
const int LIQUID_NONE = 0;
const int LIQUID_WATER = 1;
const int LIQUID_WINE = 2;
const int LIQUID_BEER = 3;
const int LIQUID_MUD = 4;
const int LIQUID_BLOOD = 5;
const int LIQUID_SLIME = 6;
const int LIQUID_OIL = 7;
const int LIQUID_URINE = 8;
const int LIQUID_MILK = 9;
const int LIQUID_MANAFLUID = 10;
const int LIQUID_LIFEFLUID = 11;
const int LIQUID_LEMONADE = 12;

// The type number of every creature object (isCreature). On the wire it is
// also the word that introduces a known creature (SendMapObject), beside
// sending.hh's UNKNOWNCREATURE and OUTDATEDCREATURE.
const int TYPEID_CREATURE = 99;

struct ObjectType {
	ObjectType(const ObjectType &Other) { setTypeID(Other.TypeID); }
	ObjectType(int TypeID = 0) { setTypeID(TypeID); }
	ObjectType operator=(ObjectType Other) { setTypeID(Other.TypeID); return *this; }
	bool operator==(ObjectType Other) { return TypeID == Other.TypeID; }
	bool operator!=(ObjectType Other) { return !(*this == Other); }
	int getTypeID() { return TypeID; }
	bool isMapContainer() { return TypeID == 0; }
	bool isBodyContainer() { return in(TypeID, 1, 10); }
	bool isCreature() { return TypeID == 99; }
	const char *getName(int Count);
	const char *getDescription();
	bool getFlag(FLAG Flag);
	uint32_t getAttribute(TYPEATTRIBUTE Attribute);
	int getAttributeOffset(INSTANCEATTRIBUTE Attribute);
private:
	void setTypeID(int _TypeID);
	int TypeID;
};

int GetFlagByName(char *Name);
int GetTypeAttributeByName(char *Name);
void LoadObjects();
void LoadConversionList();
ObjectType GetNewObjectType(unsigned char Group, unsigned char Number);
void GetOldObjectType(ObjectType Type, unsigned char &Group, unsigned char &Number);
bool ObjectTypeExists(unsigned char Group, unsigned char Number);
bool ObjectTypeExists(int TypeID);
ObjectType GetSpecialObject(SPECIALMEANING Meaning);
ObjectType GetObjectTypeByName(char *SearchName, bool Movable);
void InitObjects();
void ExitObjects();

extern char FlagNames[66][20];
extern char TypeAttributeNames[62][30];
extern FLAG TypeAttributeFlags[62];
extern FLAG InstanceAttributeFlags[18];
extern vector<TObjectType> ObjectTypes;
extern ObjectType SpecialObjects[49];
extern unsigned char OldGroup[8192];
extern unsigned char OldNumber[8192];
extern int NewType[65536];

#endif
