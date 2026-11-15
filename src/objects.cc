// objects.cc: the object types: their table (ObjectTypes), loaded from
// items.toml (data/items.toml in the repository, converted from the original
// objects.srv by tools/convert), the names of their flags and attributes, the
// special objects (GetSpecialObject), the list of built-in uses of objects
// (UseEntries, the [[use]] of items.toml), and the conversion of an old group and
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
const char *const SkillKeys[] = {
	"level",
	"magic_level",
	"hit_points",
	"mana",
	"go_strength",
	"carry_strength",
	"shielding",
	"distance_fighting",
	"sword_fighting",
	"club_fighting",
	"axe_fighting",
	"fist_fighting",
	"magic_defense",
	"fishing",
	"eating",
	"shining",
	"illusion",
	"poison",
	"burning",
	"energy",
	"drunken",
	"mana_shield",
	"soul_points",
};
const int NUMBER_OF_SKILL_KEYS = sizeof(SkillKeys) / sizeof(SkillKeys[0]);

// The values of the named attributes, by value or by bit, as the constants of
// the code name them.

// BODYPOSITION: objects.hh's BODY_ (both_hands takes both hands and is no
// slot).
static const char *const BodyPositionKeys[] = {
	"both_hands", "head", "neck", "container", "torso", "right_hand",
	"left_hand", "legs", "feet", "finger", "ammo",
};

// WEAPONTYPE: cr.hh's WEAPON_SWORD to WEAPON_AXE.
static const char *const WeaponTypeKeys[] = {
	nullptr, "sword", "club", "axe",
};

// BOWAMMOTYPE and AMMOTYPE: a bow shoots the ammunition of its own ammunition
// type (crcombat.cc). The crossbow and the bolts are 1, the bow and the
// arrows 2.
static const char *const AmmoTypeKeys[] = {
	nullptr, "bolt", "arrow",
};

// INFORMATIONTYPE: objects.hh's INFORMATION_.
static const char *const InformationTypeKeys[] = {
	nullptr, "date", "time", "blessings", "spellbook",
};

// The bits of the damage types (WANDDAMAGETYPE, AVOIDDAMAGETYPES,
// PROTECTIONDAMAGETYPES): cr.hh's DAMAGE_. Bit 4 (16) is no damage type of
// the game (neither the code nor a monster does such damage); the original
// data sets it among the protections.
static const char *const DamageTypeKeys[] = {
	"physical", "poison", "fire", "energy", "unused", "poison_condition",
	"fire_condition", "energy_condition", "life_drain", "mana_drain",
};

// The bits of PROFESSIONS, one for each profession (cr.hh's PROFESSION_NONE
// to PROFESSION_DRUID).
static const char *const ProfessionKeys[] = {
	"none", "knight", "paladin", "sorcerer", "druid",
};

// SOURCELIQUIDTYPE: objects.hh's LIQUID_.
static const char *const LiquidKeys[] = {
	"none", "water", "wine", "beer", "mud", "blood", "slime", "oil", "urine",
	"milk", "mana_fluid", "life_fluid", "lemonade",
};

// ABSTELEPORTEFFECT and RELTELEPORTEFFECT: the client's magic effects,
// operate.hh's EFFECT_ (0 shows none).
static const char *const EffectKeys[] = {
	"none", "blood_hit", "mana_hit", "poff", "block_hit", "explosion_area",
	"explosion", "fire_area", nullptr, "poison", "hit_area", "teleport",
	"energy_hit", "magic_blue", "magic_red", "magic_green", "fire_hit",
	"poison_hit", "death",
};

// THROWMISSILE, AMMOMISSILE and WANDMISSILE: the client's missiles
// (operate.hh's ANIMATION_ are four of them), named by what flies.
static const char *const MissileKeys[] = {
	"none", "spear", "bolt", "arrow", "fire", "energy", "poison_arrow",
	"burst_arrow", "throwing_star", "throwing_knife", "small_stone", "death",
	"large_rock", "snowball", "power_bolt", "poison",
};

// THROWSPECIALEFFECT and AMMOSPECIALEFFECT: crcombat.cc's SPECIALEFFECT_.
static const char *const SpecialEffectKeys[] = {
	"none", "poison", "burst",
};

// CORPSETYPE: the corpses players leave (0), which CleanupField leaves alone,
// and those of monsters (1), the only ones RaiseDead raises.
static const char *const CorpseTypeKeys[] = {
	"player", "monster",
};

// MEANING: objects.hh's SPECIALMEANING.
static const char *const MeaningKeys[SPECIALMEANINGS] = {
	nullptr,
	"money_one", "money_hundred", "money_ten_thousand",
	nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
	"inventory_right_hand", "inventory_left_hand", "inventory_body_male",
	"inventory_body_female", "inventory_container", "inventory_food",
	nullptr, nullptr, nullptr, nullptr,
	"depot_locker", "depot_chest", "parcel_new", "parcel_stamped",
	"parcel_label", "letter_new", "letter_stamped",
	nullptr, nullptr, nullptr,
	"blood_splash", "blood_pool",
	nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
	"rune_blank", "magic_field_fire_dangerous", "magic_field_fire_harmless",
	"magic_field_poison_dangerous", "magic_field_poison_harmless",
	"magic_field_energy_dangerous", "magic_field_energy_harmless",
	"magic_wall", "rushwood",
};

#define KEYS(Keys) Keys, static_cast<int>(sizeof(Keys) / sizeof(Keys[0]))

static const TNamedAttribute NamedAttributes[] = {
	{INFORMATIONTYPE, NAMED_VALUE, KEYS(InformationTypeKeys)},
	{SOURCELIQUIDTYPE, NAMED_VALUE, KEYS(LiquidKeys)},
	{ABSTELEPORTEFFECT, NAMED_VALUE, KEYS(EffectKeys)},
	{RELTELEPORTEFFECT, NAMED_VALUE, KEYS(EffectKeys)},
	{AVOIDDAMAGETYPES, NAMED_BITS, KEYS(DamageTypeKeys)},
	{PROFESSIONS, NAMED_BITS, KEYS(ProfessionKeys)},
	{BODYPOSITION, NAMED_VALUE, KEYS(BodyPositionKeys)},
	{SKILLNUMBER, NAMED_VALUE, SkillKeys, NUMBER_OF_SKILL_KEYS},
	{PROTECTIONDAMAGETYPES, NAMED_BITS, KEYS(DamageTypeKeys)},
	{CORPSETYPE, NAMED_VALUE, KEYS(CorpseTypeKeys)},
	{WEAPONTYPE, NAMED_VALUE, KEYS(WeaponTypeKeys)},
	{BOWAMMOTYPE, NAMED_VALUE, KEYS(AmmoTypeKeys)},
	{THROWMISSILE, NAMED_VALUE, KEYS(MissileKeys)},
	{THROWSPECIALEFFECT, NAMED_VALUE, KEYS(SpecialEffectKeys)},
	{WANDDAMAGETYPE, NAMED_BIT, KEYS(DamageTypeKeys)},
	{WANDMISSILE, NAMED_VALUE, KEYS(MissileKeys)},
	{AMMOTYPE, NAMED_VALUE, KEYS(AmmoTypeKeys)},
	{AMMOMISSILE, NAMED_VALUE, KEYS(MissileKeys)},
	{AMMOSPECIALEFFECT, NAMED_VALUE, KEYS(SpecialEffectKeys)},
	{MEANING, NAMED_VALUE, KEYS(MeaningKeys)},
};

#undef KEYS

const TNamedAttribute *GetNamedAttribute(int Attribute) {
	for (const TNamedAttribute &Named : NamedAttributes) {
		if (Named.Attribute == Attribute) {
			return &Named;
		}
	}
	return nullptr;
}

const char *GetValueKey(const char *const *Keys, int Count, int64_t Value) {
	if (Value < 0 || Value >= Count) {
		return nullptr;
	}
	return Keys[Value];
}

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

// The index of the name Node gives in the keys of Named. Key is the
// attribute's name.
static int ReadValueName(const TNamedAttribute &Named, const std::string &Key, const toml::node &Node) {
	if (!Node.is_string()) {
		TomlError(Node.source(), Named.Form == NAMED_BITS ? "%s must be an array of strings" : "%s must be a string",
				Key.c_str());
	}
	const std::string &Name = Node.as_string()->get();
	for (int i = 0; i < Named.Count; i++) {
		if (Named.Keys[i] != nullptr && Name == Named.Keys[i]) {
			return i;
		}
	}
	TomlError(Node.source(), "%s: \"%s\" is unknown", Key.c_str(), Name.c_str());
}

// The value of a named attribute: its name, the name of its bit, or the
// array of the names of its bits (each may be given more than once).
static uint32_t ReadNamedValue(const TNamedAttribute &Named, const std::string &Key, const toml::node &Node) {
	if (Named.Form != NAMED_BITS) {
		int Index = ReadValueName(Named, Key, Node);
		return Named.Form == NAMED_BIT ? UINT32_C(1) << Index : static_cast<uint32_t>(Index);
	}

	const toml::array *Names = Node.as_array();
	if (Names == nullptr) {
		TomlError(Node.source(), "%s must be an array of strings", Key.c_str());
	}
	uint32_t Value = 0;
	for (const toml::node &Name : *Names) {
		Value |= UINT32_C(1) << ReadValueName(Named, Key, Name);
	}
	return Value;
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
			if (const TNamedAttribute *Named = GetNamedAttribute(Attribute)) {
				Type.Attributes[Attribute] = ReadNamedValue(*Named, Key, Pair.second);
			} else {
				// A 32-bit number, written signed or unsigned.
				if (!Pair.second.is_integer()) {
					TomlError(Pair.second.source(), "%s must be an integer", Key.c_str());
				}
				int64_t Value = Pair.second.as_integer()->get();
				if (Value < INT32_MIN || Value > static_cast<int64_t>(UINT32_MAX)) {
					TomlError(Pair.second.source(), "%s must lie between %d and %u", Key.c_str(), INT32_MIN,
							UINT32_MAX);
				}
				Type.Attributes[Attribute] = static_cast<uint32_t>(Value);
			}

			// The names of meaning are those of 1 to SPECIALMEANINGS - 1.
			if (Attribute == MEANING) {
				int Meaning = static_cast<int>(Type.Attributes[MEANING]);
				if (SpecialObjects[Meaning] != ObjectType(0)) {
					TomlError(Pair.second.source(), "meaning %d is already the one of type %d", Meaning,
							SpecialObjects[Meaning].getTypeID());
				}
				SpecialObjects[Meaning] = ObjectType(TypeID);
			}
		}
	}
}

std::vector<TUseEntry> UseEntries;

// The longest script name of a [[use]]: the longest LoadScript takes.
const int MAX_USE_SCRIPT_LENGTH = 256;

// Whether Name is the script name of a [[use]]: "items/<name>.lua" and a
// script name as scripting.hh defines it (only a-z, 0-9, '_', '-', '/' and
// '.'; no part empty, "." or ".."; a file name that is more than ".lua"; at
// most MAX_USE_SCRIPT_LENGTH characters).
static bool ValidUseScript(const std::string &Name) {
	const std::string Prefix = "items/";
	const std::string Ending = ".lua";
	if (Name.size() > static_cast<size_t>(MAX_USE_SCRIPT_LENGTH) || Name.size() <= Prefix.size() + Ending.size()
			|| Name.compare(0, Prefix.size(), Prefix) != 0
			|| Name.compare(Name.size() - Ending.size(), Ending.size(), Ending) != 0) {
		return false;
	}
	size_t PartStart = 0;
	for (size_t i = 0; i <= Name.size(); i++) {
		char c = i < Name.size() ? Name[i] : 0;
		if (c == '/' || c == 0) {
			std::string Part = Name.substr(PartStart, i - PartStart);
			if (Part.empty() || Part == "." || Part == ".." || (c == 0 && Part == Ending)) {
				return false;
			}
			PartStart = i + 1;
		} else if (!((c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || c == '_' || c == '-' || c == '.')) {
			return false;
		}
	}
	return true;
}

// One [[use]] of items.toml: flag, a flag as [[item]] names them, and
// script, the script that handles the use; both needed, no other keys.
// Appended to UseEntries.
static void ReadUse(const toml::node &Node) {
	if (!Node.is_table()) {
		TomlError(Node.source(), "an entry of use must be a table");
	}

	const toml::table &Entry = *Node.as_table();
	TomlCheckKeys(Entry, "[[use]]", {"flag", "script"});

	auto FlagFound = Entry.find("flag");
	if (FlagFound == Entry.end()) {
		TomlError(Node.source(), "an entry of use needs flag");
	}
	if (!FlagFound->second.is_string()) {
		TomlError(FlagFound->second.source(), "flag must be a string");
	}
	const std::string &FlagKey = FlagFound->second.as_string()->get();
	int Flag = GetFlagByKey(FlagKey);
	if (Flag == -1) {
		TomlError(FlagFound->second.source(), "unknown flag \"%s\"", FlagKey.c_str());
	}

	auto ScriptFound = Entry.find("script");
	if (ScriptFound == Entry.end()) {
		TomlError(Node.source(), "an entry of use needs script");
	}
	// The errors and Where name the key's place, as for the scripts of
	// spells.toml.
	const toml::source_region &Where = ScriptFound->first.source();
	if (!ScriptFound->second.is_string()) {
		TomlError(Where, "script must be a string");
	}
	const std::string &Name = ScriptFound->second.as_string()->get();
	if (!ValidUseScript(Name)) {
		TomlError(Where, "script \"%s\" must be a script name \"items/<name>.lua\" (a-z, 0-9, _, -, / and ., "
				"no part . or .., at most %d characters)", Name.c_str(), MAX_USE_SCRIPT_LENGTH);
	}

	char Position[4096 + 64];
	snprintf(Position, sizeof(Position), "%s:%u:%u", Where.path != nullptr ? Where.path->c_str() : "(unknown file)",
			Where.begin.line, Where.begin.column);
	UseEntries.push_back(TUseEntry{static_cast<FLAG>(Flag), Name, Position, -1});
}

void LoadItems(const char *FileName) {
	toml::table Items = TomlParseFile(FileName);
	TomlCheckKeys(Items, "the top level", {"item", "use"});

	std::vector<bool> Defined(MAX_TYPEID + 1, false);
	if (const toml::array *Entries = TomlGetArray(Items, "item")) {
		for (const toml::node &Node : *Entries) {
			ReadItem(Node, Defined);
		}
	}

	UseEntries.clear();
	if (const toml::array *Entries = TomlGetArray(Items, "use")) {
		for (const toml::node &Node : *Entries) {
			ReadUse(Node);
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
