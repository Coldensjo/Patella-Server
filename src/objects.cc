// objects.cc: the object types: their table (ObjectTypes), loaded from
// objects.srv, the names of their flags and attributes, the special objects
// (GetSpecialObject), and the conversion of an old group and number to a type
// number (conversion.lst).

#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>

#include "config.hh"
#include "script.hh"
#include "objects.hh"
#include "containr.hh"
#include "mudinc.hh"
#include "strings.hh"
#include "utils.hh"

struct TObjectType {
	const char *Name;
	const char *Description;
	unsigned char Flags[(FLAGS + 7) / 8];
	uint32_t Attributes[TYPEATTRIBUTES];
	int AttributeOffsets[INSTANCEATTRIBUTES];
};

char FlagNames[FLAGS][20] = {
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
};
char TypeAttributeNames[TYPEATTRIBUTES][30] = {
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
};
FLAG TypeAttributeFlags[TYPEATTRIBUTES] = {
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
};
FLAG InstanceAttributeFlags[INSTANCEATTRIBUTES] = {
	CONTAINER, CHEST, CUMULATIVE, KEY,
	KEYDOOR, LEVELDOOR, QUESTDOOR, QUESTDOOR,
	RUNE, TEXT, TEXT, LIQUIDCONTAINER,
	LIQUIDPOOL, TELEPORTABSOLUTE, MAGICFIELD, EXPIRE,
	EXPIRESTOP, WEAROUT,
};
vector<TObjectType> ObjectTypes(0, 5000, 1000);

ObjectType SpecialObjects[SPECIALMEANINGS];
int NewType[65536];

int GetFlagByName(char *Name) {
	for (int i = 0; i < FLAGS; i++) {
		if (stricmp(FlagNames[i], Name, -1) == 0) {
			return i;
		}
	}
	return -1;
}

int GetTypeAttributeByName(char *Name) {
	for (int i = 0; i < TYPEATTRIBUTES; i++) {
		if (stricmp(TypeAttributeNames[i], Name, -1) == 0) {
			return i;
		}
	}
	return -1;
}

void LoadObjects() {
	char FileName[4096];
	sprintf(FileName, "%s/objects.srv", DATAPATH);

	TReadScriptFile Script;
	Script.open(FileName);

	int TypeID = -1;
	while (true) {
		Script.nextToken();
		if (Script.Token == ENDOFFILE) {
			break;
		}

		if (Script.Token != IDENTIFIER) {
			Script.error("Identifier expected");
		}

		char Ident[30];
		strcpy(Ident, Script.getIdentifier());
		Script.readSymbol('=');

		if (strcmp(Ident, "typeid") == 0) {
			TypeID = Script.readNumber();
			ObjectTypes(TypeID).Name = nullptr;
			ObjectTypes(TypeID).Description = nullptr;
			memset(ObjectTypes(TypeID).Flags, 0, sizeof(ObjectTypes(TypeID).Flags));
			for (int i = 0; i < TYPEATTRIBUTES; i++) {
				ObjectTypes(TypeID).Attributes[i] = 0;
			}
			for (int i = 0; i < INSTANCEATTRIBUTES; i++) {
				ObjectTypes(TypeID).AttributeOffsets[i] = -1;
			}
		}

		if (TypeID < 0) {
			Script.error("Invalid type id");
		}

		if (strcmp(Ident, "name") == 0) {
			ObjectTypes(TypeID).Name = AddStaticString(Script.readString());
		}

		if (strcmp(Ident, "description") == 0) {
			ObjectTypes(TypeID).Description = AddStaticString(Script.readString());
		}

		if (strcmp(Ident, "flags") == 0) {
			Script.readSymbol('{');
			while (true) {
				Script.nextToken();
				if (Script.Token == SPECIAL && Script.getSpecial() == '}') {
					break;
				}
				if (Script.Token == SPECIAL && Script.getSpecial() == ',') {
					continue;
				}

				int Flag = GetFlagByName(Script.getIdentifier());
				if (Flag == -1) {
					Script.error("Unknown flag");
				}
				ObjectTypes(TypeID).Flags[Flag / 8] |= 1 << (Flag % 8);
			}

			for (int Offset = 0, i = 0; i < INSTANCEATTRIBUTES; i++) {
				if (ObjectType(TypeID).getFlag(InstanceAttributeFlags[i])
						|| (i == CONTENT && ObjectType(TypeID).getFlag(CHEST))) {
					ObjectTypes(TypeID).AttributeOffsets[i] = Offset;
					Offset++;
				}
			}
		}

		if (strcmp(Ident, "attributes") == 0) {
			Script.readSymbol('{');
			while (true) {
				Script.nextToken();
				if (Script.Token == SPECIAL && Script.getSpecial() == '}') {
					break;
				}
				if (Script.Token == SPECIAL && Script.getSpecial() == ',') {
					continue;
				}

				int Attribute = GetTypeAttributeByName(Script.getIdentifier());
				if (Attribute == -1) {
					Script.error("Unknown attribute");
				}
				Script.readSymbol('=');
				ObjectTypes(TypeID).Attributes[Attribute] = Script.readNumber();

				if (Attribute == MEANING) {
					int Meaning = ObjectTypes(TypeID).Attributes[MEANING];
					// Quirk: 49 passes here and in GetSpecialObject, one past
					// the last element of SpecialObjects.
					if (!in(Meaning, 1, SPECIALMEANINGS)) {
						Script.error("Unknown meaning");
					}
					if (SpecialObjects[Meaning] != ObjectType(0)) {
						Script.error("Special object already defined");
					}
					SpecialObjects[Meaning] = ObjectType(TypeID);
				}
			}
		}
	}

	Script.close();
}

void LoadConversionList() {
	std::ifstream IN;
	char FileName[4096];
	int Group, Number, Type;

	for (int i = 0; i < 65536; i++) {
		NewType[i] = -1;
	}

	sprintf(FileName, "%s/conversion.lst", DATAPATH);
	IN.open(FileName, std::ios::in);
	if (IN.fail()) {
		// Quirk: no argument is passed for %s.
		error("LoadConversionList: Kann Datei %s nicht \xf6" "ffnen.\n");
		throw "Cannot open conversion.lst";
	}

	while (true) {
		IN >> Group;
		if (IN.eof()) {
			break;
		}
		IN >> Number >> Type;

		if (Type < 1 || Type > CONVERSION_TYPES - 1) {
			error("LoadConversionList: Ung\xfcltiger Typ %d.\n", Type);
			throw "Error while loading conversion.lst";
		}

		NewType[Group * 256 + Number] = Type;
	}

	IN.close();

	// No type (0) and the body containers keep their numbers, in group 0.
	for (int i = 0; i <= LAST_BODY_POSITION; i++) {
		NewType[i] = i;
	}

	NewType[250 * 256 + 0] = TYPEID_CREATURE;
}

ObjectType GetNewObjectType(unsigned char Group, unsigned char Number) {
	int Type = NewType[Group * 256 + Number];
	if (Type == -1) {
		error("GetNewObjectType: Objekttyp %d/%d existiert nicht.\n", Group, Number);
		return ObjectType(0);
	}
	return ObjectType(Type);
}

void ObjectType::setTypeID(int _TypeID) {
	if (!in(_TypeID, ObjectTypes.low(), ObjectTypes.high())) {
		error("ObjectType::setTypeID: Ung\xfcltiger Typ %d.\n", _TypeID);
		_TypeID = 0;
	}
	TypeID = _TypeID;
}

const char *ObjectType::getName(int Count) {
	char ObjectName[50];
	ObjectName[0] = 0;

	if (isCreature()) {
		error("ObjectType::getName: Der Kreaturtyp hat keinen Namen.\n");
		return nullptr;
	}

	const char *Help = ObjectTypes(TypeID).Name;
	if (Help != nullptr) {
		strcpy(ObjectName, Help);
	}
	return Plural(ObjectName, Count);
}

const char *ObjectType::getDescription() {
	return ObjectTypes(getTypeID()).Description;
}

bool ObjectType::getFlag(FLAG Flag) {
	unsigned char b = static_cast<unsigned char>(1 << (Flag % 8));
	return (ObjectTypes(getTypeID()).Flags[Flag / 8] & b) == b;
}

uint32_t ObjectType::getAttribute(TYPEATTRIBUTE Attribute) {
	if (!getFlag(TypeAttributeFlags[Attribute])) {
		error("ObjectType::getAttribute: Typ %d hat kein Flag %d f\xfc" "r Attribut %d.\n",
				getTypeID(), TypeAttributeFlags[Attribute], Attribute);
		return 0;
	}
	return ObjectTypes(getTypeID()).Attributes[Attribute];
}

int ObjectType::getAttributeOffset(INSTANCEATTRIBUTE Attribute) {
	if (!getFlag(InstanceAttributeFlags[Attribute])
			&& (Attribute != CONTENT || !getFlag(CHEST))) {
		return -1;
	}
	return ObjectTypes(getTypeID()).AttributeOffsets[Attribute];
}

bool ObjectTypeExists(int TypeID) {
	return in(TypeID, ObjectTypes.low(), ObjectTypes.high());
}

ObjectType GetSpecialObject(SPECIALMEANING Meaning) {
	if (!in(Meaning, 1, SPECIALMEANINGS)) {
		error("GetSpecialObject: Ung\xfcltige Bedeutung %d.\n", Meaning);
		return ObjectType(0);
	}

	if (SpecialObjects[Meaning] == ObjectType(0)) {
		error("GetSpecialObject: Kein Objekttyp mit Bedeutung %d definiert.\n", Meaning);
		return ObjectType(0);
	}

	return SpecialObjects[Meaning];
}

ObjectType GetObjectTypeByName(char *SearchName, bool Movable) {
	if (SearchName == nullptr) {
		error("GetObjectTypeByName: SearchName ist NULL.\n");
		return ObjectType(0);
	}

	char Pattern[50];
	snprintf(Pattern, sizeof(Pattern), " %s ", SearchName);

	ObjectType BestType(0);
	for (int TypeID = ObjectType(TYPEID_CREATURE).getTypeID() + 1; TypeID <= ObjectTypes.high(); TypeID++) {
		ObjectType Type(TypeID);
		if (Movable && Type.getFlag(UNMOVE)) {
			continue;
		}

		const char *ObjectName = Type.getName(-1);
		if (ObjectName == nullptr || ObjectName[0] == 0) {
			continue;
		}

		if (stricmp(const_cast<char *>(ObjectName), SearchName, -1) == 0) {
			return Type;
		}

		if (BestType != ObjectType(0)) {
			continue;
		}

		char Help[50];
		snprintf(Help, sizeof(Help), " %s ", ObjectName);
		if (strstr(Help, Pattern) != nullptr) {
			BestType = Type;
		}
	}

	return BestType;
}

void InitObjects() {
	LoadObjects();
	LoadConversionList();
}
