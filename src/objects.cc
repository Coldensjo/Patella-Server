#include <cstdint>
#include <cstdlib>

#include "config.hh"
#include "script.hh"
#include "objects.hh"
#include "containr.hh"
#include "utils.hh"

struct TObjectType {
	const char *Name;
	const char *Description;
	unsigned char Flags[9];
	uint32_t Attributes[62];
	int AttributeOffsets[18];
};

char FlagNames[66][20] = {
	"Bank",
	"Clip",
	"Bottom",
	"Top",
	"Container",
	"Chest",
	"Cumulative",
	"UseEvent",
	"ChangeUse",
	"ForceUse",
	"MultiUse",
	"DistUse",
	"MovementEvent",
	"CollisionEvent",
	"SeparationEvent",
	"Key",
	"KeyDoor",
	"NameDoor",
	"LevelDoor",
	"QuestDoor",
	"Bed",
	"Food",
	"Rune",
	"Information",
	"Text",
	"Write",
	"WriteOnce",
	"LiquidContainer",
	"LiquidSource",
	"LiquidPool",
	"TeleportAbsolute",
	"TeleportRelative",
	"Unpass",
	"Unmove",
	"Unthrow",
	"Unlay",
	"Avoid",
	"MagicField",
	"RestrictLevel",
	"RestrictProfession",
	"Take",
	"Hang",
	"HookSouth",
	"HookEast",
	"Rotate",
	"Destroy",
	"Clothes",
	"SkillBoost",
	"Protection",
	"Light",
	"RopeSpot",
	"Corpse",
	"Expire",
	"ExpireStop",
	"WearOut",
	"Weapon",
	"Shield",
	"Bow",
	"Throw",
	"Wand",
	"Ammo",
	"Armor",
	"Height",
	"Disguise",
	"ShowDetail",
	"Special",
};	// objects.c:19
char TypeAttributeNames[62][30] = {
	"Waypoints",
	"Capacity",
	"ChangeTarget",
	"KeydoorTarget",
	"NamedoorTarget",
	"LeveldoorTarget",
	"QuestdoorTarget",
	"Nutrition",
	"InformationType",
	"FontSize",
	"MaxLength",
	"MaxLengthOnce",
	"SourceLiquidType",
	"AbsTeleportEffect",
	"RelTeleportDisplacement",
	"RelTeleportEffect",
	"AvoidDamageTypes",
	"MinimumLevel",
	"Professions",
	"Weight",
	"RotateTarget",
	"DestroyTarget",
	"BodyPosition",
	"SkillNumber",
	"SkillModification",
	"ProtectionDamageTypes",
	"DamageReduction",
	"Brightness",
	"LightColor",
	"CorpseType",
	"TotalExpireTime",
	"ExpireTarget",
	"TotalUses",
	"WearoutTarget",
	"WeaponType",
	"WeaponAttackValue",
	"WeaponDefendValue",
	"ShieldDefendValue",
	"BowRange",
	"BowAmmoType",
	"ThrowRange",
	"ThrowAttackValue",
	"ThrowDefendValue",
	"ThrowMissile",
	"ThrowSpecialEffect",
	"ThrowEffectStrength",
	"ThrowFragility",
	"WandRange",
	"WandManaConsumption",
	"WandAttackStrength",
	"WandAttackVariation",
	"WandDamageType",
	"WandMissile",
	"AmmoType",
	"AmmoAttackValue",
	"AmmoMissile",
	"AmmoSpecialEffect",
	"AmmoEffectStrength",
	"ArmorValue",
	"Elevation",
	"DisguiseTarget",
	"Meaning",
};	// objects.c:32
FLAG TypeAttributeFlags[62] = {
	BANK, CONTAINER, CHANGEUSE, KEYDOOR,
	NAMEDOOR, LEVELDOOR, QUESTDOOR, FOOD,
	INFORMATION, TEXT, WRITE, WRITEONCE,
	LIQUIDSOURCE, TELEPORTABSOLUTE, TELEPORTRELATIVE, TELEPORTRELATIVE,
	AVOID, RESTRICTLEVEL, RESTRICTPROFESSION, TAKE,
	ROTATE, DESTROY, CLOTHES, SKILLBOOST,
	SKILLBOOST, PROTECTION, PROTECTION, LIGHT,
	LIGHT, CORPSE, EXPIRE, EXPIRE,
	WEAROUT, WEAROUT, WEAPON, WEAPON,
	WEAPON, SHIELD, BOW, BOW,
	THROW, THROW, THROW, THROW,
	THROW, THROW, THROW, WAND,
	WAND, WAND, WAND, WAND,
	WAND, AMMO, AMMO, AMMO,
	AMMO, AMMO, ARMOR, HEIGHT,
	DISGUISE, BANK,
};	// objects.c:45
FLAG InstanceAttributeFlags[18] = {
	CONTAINER, CHEST, CUMULATIVE, KEY,
	KEYDOOR, LEVELDOOR, QUESTDOOR, QUESTDOOR,
	RUNE, TEXT, TEXT, LIQUIDCONTAINER,
	LIQUIDPOOL, TELEPORTABSOLUTE, MAGICFIELD, EXPIRE,
	EXPIRESTOP, WEAROUT,
};	// objects.c:57
vector<TObjectType> ObjectTypes(0, 5000, 1000);	// objects.c:72
ObjectType SpecialObjects[49];	// objects.c:74
unsigned char OldGroup[8192];	// objects.c:182
unsigned char OldNumber[8192];	// objects.c:183
int NewType[65536];	// objects.c:184

// 0x08054d10  objects.c:91
int GetFlagByName(char * /*Name*/) { abort(); /* STUB */ }

// 0x08054d70  objects.c:101
int GetTypeAttributeByName(char * /*Name*/) { abort(); /* STUB */ }

// 0x08054dd0  objects.c:112
void LoadObjects() { abort(); /* STUB */ }

// 0x08055650  objects.c:190
void LoadConversionList() { abort(); /* STUB */ }

// 0x08055a40  objects.c:231
ObjectType GetNewObjectType(unsigned char /*Group*/, unsigned char /*Number*/) { abort(); /* STUB */ }

// 0x08055b00  objects.c:244
void GetOldObjectType(ObjectType /*Type*/, unsigned char & /*Group*/, unsigned char & /*Number*/) { abort(); /* STUB */ }

// 0x08055b80  objects.c:257
bool ObjectTypeExists(unsigned char /*Group*/, unsigned char /*Number*/) { abort(); /* STUB */ }

// 0x08055ba0  objects.c:270
void ObjectType::setTypeID(int /*_TypeID*/) { abort(); /* STUB */ }

// 0x08055bf0  objects.c:282
const char *ObjectType::getName(int /*Count*/) { abort(); /* STUB */ }

// 0x08055c60  objects.c:298
const char *ObjectType::getDescription() { abort(); /* STUB */ }

// 0x08055c90  objects.c:306
bool ObjectType::getFlag(FLAG /*Flag*/) { abort(); /* STUB */ }

// 0x08055cf0  objects.c:315
uint32_t ObjectType::getAttribute(TYPEATTRIBUTE /*Attribute*/) { abort(); /* STUB */ }

// 0x08055da0  objects.c:328
int ObjectType::getAttributeOffset(INSTANCEATTRIBUTE /*Attribute*/) { abort(); /* STUB */ }

// 0x08055e50  objects.c:337
bool ObjectTypeExists(int /*TypeID*/) { abort(); /* STUB */ }

// 0x08055e80  objects.c:345
ObjectType GetSpecialObject(SPECIALMEANING /*Meaning*/) { abort(); /* STUB */ }

// 0x08055f20  objects.c:362
ObjectType GetObjectTypeByName(char * /*SearchName*/, bool /*Movable*/) { abort(); /* STUB */ }

// 0x08056120  objects.c:394
void InitObjects() { abort(); /* STUB */ }

// 0x08056150  objects.c:403
void ExitObjects() { abort(); /* STUB */ }
