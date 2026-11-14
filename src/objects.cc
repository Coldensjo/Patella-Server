// objects.cc: the object types: their table (ObjectTypes), loaded from
// items.toml (data/items.toml in the repository, converted from the original
// objects.srv by tools/convert), the names of their flags and attributes, the
// special objects (GetSpecialObject), and the conversion of an old group and
// number to a type number, loaded from conversion.toml (data/conversion.toml
// in the repository, converted from the original conversion.lst by
// tools/convert).

#include "tomlfile.hh"

#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>

#include "config.hh"
#include "script.hh"
#include "objects.hh"
#include "containr.hh"
#include "mudinc.hh"
#include "strings.hh"
#include "utils.hh"

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
// The flags and type attributes as items.toml names them: the names above in
// snake_case ("Keydoor" and "Wearout" as "key_door" and "wear_out", as the
// flags write them).
const char *const FlagKeys[FLAGS] = {
	"bank",
	"clip",
	"bottom",
	"top",
	"container",
	"chest",
	"cumulative",
	"use_event",
	"change_use",
	"force_use",
	"multi_use",
	"dist_use",
	"movement_event",
	"collision_event",
	"separation_event",
	"key",
	"key_door",
	"name_door",
	"level_door",
	"quest_door",
	"bed",
	"food",
	"rune",
	"information",
	"text",
	"write",
	"write_once",
	"liquid_container",
	"liquid_source",
	"liquid_pool",
	"teleport_absolute",
	"teleport_relative",
	"unpass",
	"unmove",
	"unthrow",
	"unlay",
	"avoid",
	"magic_field",
	"restrict_level",
	"restrict_profession",
	"take",
	"hang",
	"hook_south",
	"hook_east",
	"rotate",
	"destroy",
	"clothes",
	"skill_boost",
	"protection",
	"light",
	"rope_spot",
	"corpse",
	"expire",
	"expire_stop",
	"wear_out",
	"weapon",
	"shield",
	"bow",
	"throw",
	"wand",
	"ammo",
	"armor",
	"height",
	"disguise",
	"show_detail",
	"special",
};
const char *const TypeAttributeKeys[TYPEATTRIBUTES] = {
	"waypoints",
	"capacity",
	"change_target",
	"key_door_target",
	"name_door_target",
	"level_door_target",
	"quest_door_target",
	"nutrition",
	"information_type",
	"font_size",
	"max_length",
	"max_length_once",
	"source_liquid_type",
	"abs_teleport_effect",
	"rel_teleport_displacement",
	"rel_teleport_effect",
	"avoid_damage_types",
	"minimum_level",
	"professions",
	"weight",
	"rotate_target",
	"destroy_target",
	"body_position",
	"skill_number",
	"skill_modification",
	"protection_damage_types",
	"damage_reduction",
	"brightness",
	"light_color",
	"corpse_type",
	"total_expire_time",
	"expire_target",
	"total_uses",
	"wear_out_target",
	"weapon_type",
	"weapon_attack_value",
	"weapon_defend_value",
	"shield_defend_value",
	"bow_range",
	"bow_ammo_type",
	"throw_range",
	"throw_attack_value",
	"throw_defend_value",
	"throw_missile",
	"throw_special_effect",
	"throw_effect_strength",
	"throw_fragility",
	"wand_range",
	"wand_mana_consumption",
	"wand_attack_strength",
	"wand_attack_variation",
	"wand_damage_type",
	"wand_missile",
	"ammo_type",
	"ammo_attack_value",
	"ammo_missile",
	"ammo_special_effect",
	"ammo_effect_strength",
	"armor_value",
	"elevation",
	"disguise_target",
	"meaning",
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

// The longest name or description: the longest string the script reader of
// objects.srv took.
const int MAX_OBJECT_TEXT_LENGTH = TReadScriptFile::MAXIMUMSTRINGLENGTH - 2;

// The highest type number items.toml may give: a type is a 16-bit number on
// the wire.
const int MAX_TYPEID = 65535;

static int GetFlagByKey(const std::string &Key) {
	for (int i = 0; i < FLAGS; i++) {
		if (Key == FlagKeys[i]) {
			return i;
		}
	}
	return -1;
}

static int GetTypeAttributeByKey(const std::string &Key) {
	for (int i = 0; i < TYPEATTRIBUTES; i++) {
		if (Key == TypeAttributeKeys[i]) {
			return i;
		}
	}
	return -1;
}

// The name or the description of an [[item]] as a static string, or nullptr
// when the entry has none.
static const char *ReadObjectText(const toml::table &Entry, const char *Key) {
	static char Text[MAX_OBJECT_TEXT_LENGTH + 1];
	if (!TomlGetString(Entry, Key, Text, sizeof(Text))) {
		return nullptr;
	}
	return AddStaticString(Text);
}

// One [[item]] of items.toml. The type's entry in ObjectTypes is filled anew
// from its name, description, flags and attributes, as a TypeID of objects.srv
// did. Defined notes the types filled so far.
static void ReadItem(const toml::node &Node, std::vector<bool> &Defined) {
	if (!Node.is_table()) {
		TomlError(Node.source(), "an entry of item must be a table");
	}

	const toml::table &Entry = *Node.as_table();
	TomlCheckKeys(Entry, "[[item]]", {"id", "name", "description", "flags", "attributes"});
	if (!Entry.contains("id")) {
		TomlError(Node.source(), "an entry of item needs id");
	}

	int TypeID = -1;
	TomlGetNumber(Entry, "id", TypeID, 0, MAX_TYPEID);
	if (Defined[TypeID]) {
		TomlError(Entry.get("id")->source(), "type %d is defined twice", TypeID);
	}
	Defined[TypeID] = true;

	TObjectType &Type = ObjectTypes(TypeID);
	Type.Name = nullptr;
	Type.Description = nullptr;
	memset(Type.Flags, 0, sizeof(Type.Flags));
	for (int i = 0; i < TYPEATTRIBUTES; i++) {
		Type.Attributes[i] = 0;
	}
	for (int i = 0; i < INSTANCEATTRIBUTES; i++) {
		Type.AttributeOffsets[i] = -1;
	}

	Type.Name = ReadObjectText(Entry, "name");
	Type.Description = ReadObjectText(Entry, "description");

	if (const toml::array *Flags = TomlGetArray(Entry, "flags")) {
		for (const toml::node &FlagNode : *Flags) {
			if (!FlagNode.is_string()) {
				TomlError(FlagNode.source(), "a flag must be a string");
			}
			const std::string &Key = FlagNode.as_string()->get();
			int Flag = GetFlagByKey(Key);
			if (Flag == -1) {
				TomlError(FlagNode.source(), "unknown flag \"%s\"", Key.c_str());
			}
			Type.Flags[Flag / 8] |= 1 << (Flag % 8);
		}
	}

	// The instance attributes the flags give an object of the type, in their
	// order.
	for (int Offset = 0, i = 0; i < INSTANCEATTRIBUTES; i++) {
		if (ObjectType(TypeID).getFlag(InstanceAttributeFlags[i])
				|| (i == CONTENT && ObjectType(TypeID).getFlag(CHEST))) {
			Type.AttributeOffsets[i] = Offset;
			Offset++;
		}
	}

	if (const toml::table *Attributes = TomlGetTable(Entry, "attributes")) {
		for (const auto &Pair : *Attributes) {
			std::string Key(Pair.first.str());
			int Attribute = GetTypeAttributeByKey(Key);
			if (Attribute == -1) {
				TomlError(Pair.first.source(), "unknown attribute \"%s\"", Key.c_str());
			}
			// A 32-bit number, written signed or unsigned.
			if (!Pair.second.is_integer()) {
				TomlError(Pair.second.source(), "%s must be an integer", Key.c_str());
			}
			int64_t Value = Pair.second.as_integer()->get();
			if (Value < INT32_MIN || Value > static_cast<int64_t>(UINT32_MAX)) {
				TomlError(Pair.second.source(), "%s must lie between %d and %u", Key.c_str(), INT32_MIN, UINT32_MAX);
			}
			Type.Attributes[Attribute] = static_cast<uint32_t>(Value);

			if (Attribute == MEANING) {
				int Meaning = static_cast<int>(Type.Attributes[MEANING]);
				if (!in(Meaning, 1, SPECIALMEANINGS - 1)) {
					TomlError(Pair.second.source(), "meaning must lie between 1 and %d", SPECIALMEANINGS - 1);
				}
				if (SpecialObjects[Meaning] != ObjectType(0)) {
					TomlError(Pair.second.source(), "meaning %d is already the one of type %d", Meaning,
							SpecialObjects[Meaning].getTypeID());
				}
				SpecialObjects[Meaning] = ObjectType(TypeID);
			}
		}
	}
}

void LoadItems(const char *FileName) {
	toml::table Items = TomlParseFile(FileName);
	TomlCheckKeys(Items, "the top level", {"item"});

	std::vector<bool> Defined(MAX_TYPEID + 1, false);
	if (const toml::array *Entries = TomlGetArray(Items, "item")) {
		for (const toml::node &Node : *Entries) {
			ReadItem(Node, Defined);
		}
	}
}

void LoadObjects() {
	char FileName[4096];
	sprintf(FileName, "%s/items.toml", DATAPATH);
	LoadItems(FileName);
}

// An old group or number, a key of conversion.toml: 0 to 255, in decimal
// without leading zeros (so that "7" and "07" cannot both give one).
static int ReadOldNumber(const toml::key &Key, const char *What) {
	const std::string Text(Key.str());
	bool Valid = !Text.empty() && Text.size() <= 3 && (Text[0] != '0' || Text.size() == 1);
	for (char c : Text) {
		Valid = Valid && c >= '0' && c <= '9';
	}
	if (!Valid || atoi(Text.c_str()) > 255) {
		TomlError(Key.source(), "%s \"%s\" must be a number from 0 to 255 without leading zeros", What, Text.c_str());
	}
	return atoi(Text.c_str());
}

void LoadConversions(const char *FileName) {
	for (int i = 0; i < 65536; i++) {
		NewType[i] = -1;
	}

	toml::table File = TomlParseFile(FileName);
	TomlCheckKeys(File, "the top level", {"group"});
	if (const toml::table *Groups = TomlGetTable(File, "group")) {
		for (const auto &GroupEntry : *Groups) {
			int Group = ReadOldNumber(GroupEntry.first, "group");
			if (!GroupEntry.second.is_table()) {
				TomlError(GroupEntry.second.source(), "group %d must be a table", Group);
			}
			for (const auto &Entry : *GroupEntry.second.as_table()) {
				int Number = ReadOldNumber(Entry.first, "number");
				if ((Group == 0 && Number <= LAST_BODY_POSITION) || (Group == 250 && Number == 0)) {
					TomlError(Entry.first.source(), "%d/%d is the game's own (0/0 to 0/%d and 250/0)", Group,
							Number, LAST_BODY_POSITION);
				}
				if (!Entry.second.is_integer()) {
					TomlError(Entry.second.source(), "the type of %d/%d must be an integer", Group, Number);
				}
				int64_t Type = Entry.second.as_integer()->get();
				if (Type < 1 || Type > CONVERSION_TYPES - 1) {
					TomlError(Entry.second.source(), "the type of %d/%d must lie between 1 and %d", Group, Number,
							CONVERSION_TYPES - 1);
				}
				NewType[Group * 256 + Number] = static_cast<int>(Type);
			}
		}
	}

	// No type (0) and the body containers keep their numbers, in group 0.
	for (int i = 0; i <= LAST_BODY_POSITION; i++) {
		NewType[i] = i;
	}

	NewType[250 * 256 + 0] = TYPEID_CREATURE;
}

void LoadConversionList() {
	char FileName[4096];
	sprintf(FileName, "%s/conversion.toml", DATAPATH);
	LoadConversions(FileName);
}

ObjectType GetNewObjectType(unsigned char Group, unsigned char Number) {
	int Type = NewType[Group * 256 + Number];
	if (Type == -1) {
		error("GetNewObjectType: Object type %d/%d does not exist.\n", Group, Number);
		return ObjectType(0);
	}
	return ObjectType(Type);
}

void ObjectType::setTypeID(int _TypeID) {
	if (!in(_TypeID, ObjectTypes.low(), ObjectTypes.high())) {
		error("ObjectType::setTypeID: Invalid type %d.\n", _TypeID);
		_TypeID = 0;
	}
	TypeID = _TypeID;
}

const char *ObjectType::getName(int Count) {
	char ObjectName[50];
	ObjectName[0] = 0;

	if (isCreature()) {
		error("ObjectType::getName: The creature type has no name.\n");
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
		error("ObjectType::getAttribute: Type %d has no flag %d for attribute %d.\n",
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
		error("GetSpecialObject: Invalid meaning %d.\n", Meaning);
		return ObjectType(0);
	}

	if (SpecialObjects[Meaning] == ObjectType(0)) {
		error("GetSpecialObject: No object type with meaning %d defined.\n", Meaning);
		return ObjectType(0);
	}

	return SpecialObjects[Meaning];
}

ObjectType GetObjectTypeByName(char *SearchName, bool Movable) {
	if (SearchName == nullptr) {
		error("GetObjectTypeByName: SearchName is NULL.\n");
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
