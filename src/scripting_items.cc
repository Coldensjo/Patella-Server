// scripting_items.cc: the API of the built-in uses of objects (scripting.hh,
// "Built-in uses of objects"): the ItemType methods, the methods of Item,
// Creature and Player and the Game and Magic functions the scripts of the
// list [[use]] of items.toml call, and the constants (LIQUID_, INFORMATION_,
// WEAPON_, SPECIAL_, RIGHT_, the blessings' quests, MAX_QUESTS,
// MAX_OPEN_CONTAINERS, LAST_BODY_POSITION). RegisterItemAPI binds them; InitAll
// calls it after RegisterTriggerAPI. InitItemUses loads the scripts of the
// list, after InitObjects has read it, through ItemUsesReloadPart, which a
// reload uses too (reload.hh); Use (operate.cc) calls them.
//
// What every binding here does alike (as in scripting_triggers.cc):
//  - An Item parameter, self included, is an Item object or nil; the binding
//    passes its handle on as Object(handle), nil as NONE, and never checks it
//    itself: the C++ function does for a handle that is gone or NONE what it
//    always did. An Item result is nil for NONE.
//  - An ItemType holds the type number ObjectType(id) made of the script's id
//    when the object was made (ItemType(id); a number that is no type was
//    logged then by ObjectType::setTypeID and became 0), so its methods work
//    on ObjectType(number) of a valid number, which logs nothing.
//  - A Creature or Player self is resolved on every call; a method that
//    dereferences it logs "<Lua name>: Creature does not exist." for a
//    creature that is gone and raises ERROR ("nullptr: checked"); a Player
//    method on a creature that is no player throws a const char * (binding's
//    check). Where the C++ function takes the creature ID and checks it itself,
//    the binding passes the ID on unchecked ("ID: passed on").
//  - A number the C++ function takes as uint32_t is converted from the Lua
//    number modulo 2^32, as the C++ code converted the int it held.
//  - A string passed on as a char * is a copy, so the C++ function never
//    touches Lua's string.
//  - The binding's own checks (a flag, attribute, skill, meaning or window
//    number out of range) guard what the C++ code never got from its constant
//    arguments and would read out of bounds or wrap; they throw a const
//    char *, which Guarded turns into a logged Lua error and ERROR.
//  - What the C++ function throws (a RESULT) is raised as the result code
//    (Guarded).

#include <cstdint>
#include <cstdio>
#include <string>
#include <tuple>
#include <vector>

#include "config.hh"
#include "objects.hh"
#include "map.hh"
#include "info.hh"
#include "cr.hh"
#include "crskill.hh"
#include "crmain.hh"
#include "crplayer.hh"
#include "dbfuncs.hh"
#include "houses.hh"
#include "magic.hh"
#include "magicdata.hh"
#include "operate.hh"
#include "sending.hh"
#include "time.hh"
#include "triggers.hh"
#include "utils.hh"
#include "reload.hh"
#include "scripting_api.hh"

// The object an Item parameter names: Object(handle), NONE for nil.
static Object ItemObject(const sol::optional<TScriptItem> &Item) {
	return Item.has_value() ? Object(Item->Handle) : NONE;
}

// An object as an Item result: nil for NONE.
static sol::optional<TScriptItem> ItemResult(Object Obj) {
	if (Obj == NONE) {
		return sol::nullopt;
	}
	return TScriptItem{Obj.getObjectID()};
}

// The creature ID names; for a missing one logs "<Name>: Creature does not
// exist." and throws ERROR.
static TCreature *ExistingCreature(uint32_t ID, const char *Name) {
	TCreature *cr = GetCreature(ID);
	if (cr == nullptr) {
		error("%s: Creature does not exist.\n", Name);
		throw ERROR;
	}
	return cr;
}

// The player the ID names: "nullptr: checked"; a creature that is no player
// throws NotPlayer (binding's check).
static TPlayer *ExistingPlayer(uint32_t ID, const char *Name, const char *NotPlayer) {
	TCreature *cr = ExistingCreature(ID, Name);
	if (cr->Type != PLAYER) {
		throw NotPlayer;
	}
	return static_cast<TPlayer *>(cr);
}

// The object a parameter that takes an Item, a Creature or nil names: an
// Item's object, a creature's object (GetCreatureObject; a creature that is
// gone is "nullptr: checked" under Name), NONE for nil.
static Object ThingObject(const sol::optional<TScriptThing> &Thing, const char *Name) {
	if (!Thing.has_value()) {
		return NONE;
	}
	if (Thing->IsCreature) {
		return ExistingCreature(Thing->Value, Name)->GetCreatureObject();
	}
	return Object(Thing->Value);
}

// The skill SkillNr of the creature, for the Player methods that read or set
// a skill's timer: 0 to MAX_SKILLS - 1 and present (binding's check, which
// throws Message).
static TSkill *ExistingSkill(TCreature *cr, int SkillNr, const char *Message) {
	if (SkillNr < 0 || SkillNr >= MAX_SKILLS || cr->Skills[SkillNr] == nullptr) {
		throw Message;
	}
	return cr->Skills[SkillNr];
}

// ItemType(id) makes its type number with this (SetItemTypeConstructor):
// ObjectType(id).getTypeID(), the number an ObjectType holds after its
// constructor, so a number that is no type is logged by
// ObjectType::setTypeID ("Invalid type <id>") and gives 0, at the point the
// object is made, as the C++ code's ObjectType Target = ... did. Throws
// nothing.
static int ItemTypeNumber(int TypeID) {
	return ObjectType(TypeID).getTypeID();
}

// ---------------------------------------------------------------------------
// ItemType

// itemType:getId() -> the type number the object holds (ObjectType(id) made
// of the script's id: 0 for a number that is no type). Throws nothing.
static int ItemTypeGetId(TScriptItemType Self) {
	return Self.TypeID;
}

// itemType:hasFlag(flag) -> ObjectType(id).getFlag(flag), flag a FLAG number
// (FLAG_ constants), 0 to FLAGS - 1 (binding's check). Throws nothing else.
static bool ItemTypeHasFlag(TScriptItemType Self, int Flag) {
	if (Flag < 0 || Flag >= FLAGS) {
		throw "ItemType.hasFlag: no such flag";
	}
	return ObjectType(Self.TypeID).getFlag(static_cast<FLAG>(Flag));
}

// itemType:getAttribute(attribute) -> ObjectType(id).getAttribute(
// TYPEATTRIBUTE) (ITEMTYPE_ATTRIBUTE_ constants), 0 to TYPEATTRIBUTES - 1
// (binding's check). Returns the uint32_t (0, logged, for a type without the
// attribute's flag). Throws nothing else.
static uint32_t ItemTypeGetAttribute(TScriptItemType Self, int Attribute) {
	if (Attribute < 0 || Attribute >= TYPEATTRIBUTES) {
		throw "ItemType.getAttribute: no such type attribute";
	}
	return ObjectType(Self.TypeID).getAttribute(static_cast<TYPEATTRIBUTE>(Attribute));
}

// itemType:getName(count) -> ObjectType(id).getName(Count), the name for
// Count objects (-1: the name without amount), a copy; nil for nullptr (the
// creature type, logged). Throws nothing.
static sol::optional<std::string> ItemTypeGetName(TScriptItemType Self, int Count) {
	const char *Name = ObjectType(Self.TypeID).getName(Count);
	if (Name == nullptr) {
		return sol::nullopt;
	}
	return std::string(Name);
}

// ---------------------------------------------------------------------------
// Item

// item:exists() -> Obj.exists() (false for nil). Throws nothing.
static bool ItemExists(sol::optional<TScriptItem> Self) {
	return ItemObject(Self).exists();
}

// item:getName() -> GetName(Obj), the name with its amount ("a sword", "3
// gold coins"; a creature's name), a copy; nil for nullptr. Throws nothing.
static sol::optional<std::string> ItemGetName(sol::optional<TScriptItem> Self) {
	const char *Name = GetName(ItemObject(Self));
	if (Name == nullptr) {
		return sol::nullopt;
	}
	return std::string(Name);
}

// item:getWeight() -> GetCompleteWeight(Obj), with the contents. Throws
// nothing.
static int ItemGetWeight(sol::optional<TScriptItem> Self) {
	return GetCompleteWeight(ItemObject(Self));
}

// item:transform(type, value) -> Change(Obj, ObjectType(Type), Value) of
// operate.cc: changes the object's type and sends the change. Throws what
// Change throws.
static void ItemTransform(sol::optional<TScriptItem> Self, int Type, uint32_t Value) {
	Change(ItemObject(Self), ObjectType(Type), Value);
}

// item:clearField(exclude|nil) -> ClearField(Obj, Exclude) of triggers.cc:
// moves what lies on the object's field away (Exclude stays). Throws what
// ClearField throws.
static void ItemClearField(sol::optional<TScriptItem> Self, sol::optional<TScriptItem> Exclude) {
	ClearField(ItemObject(Self), ItemObject(Exclude));
}

// item:empty(rest) -> Empty(Obj, Rest) of operate.cc: empties the container
// down to Rest objects. Throws what Empty throws.
static void ItemEmpty(sol::optional<TScriptItem> Self, int Rest) {
	Empty(ItemObject(Self), Rest);
}

// ---------------------------------------------------------------------------
// Creature, Player

// creature:isRemoved() -> GetCreature(ID) == nullptr. Throws nothing.
static bool CreatureIsRemoved(TScriptCreature Self) {
	return GetCreature(Self.ID) == nullptr;
}

// player:isRemoved() -> GetPlayer(ID) == nullptr. Throws nothing.
static bool PlayerIsRemoved(TScriptPlayer Self) {
	return GetPlayer(Self.ID) == nullptr;
}

// creature:say(text) -> Talk(ID, TALK_SAY, nullptr, Text, false) of
// operate.cc, Text a copy. ID: passed on (Talk logs a creature that is gone
// and throws ERROR). Throws what Talk throws.
static void CreatureSay(TScriptCreature Self, std::string Text) {
	std::string Buffer(Text.c_str());
	Talk(Self.ID, TALK_SAY, nullptr, &Buffer[0], false);
}

// player:getSkillTimer(skill) -> Skills[skill]->TimerValue(). nullptr:
// checked; no player, skill out of range or missing: binding's check.
static int PlayerGetSkillTimer(TScriptPlayer Self, int SkillNr) {
	TPlayer *pl = ExistingPlayer(Self.ID, "Player.getSkillTimer", "Player.getSkillTimer: the creature is not a player");
	return ExistingSkill(pl, SkillNr, "Player.getSkillTimer: no such skill")->TimerValue();
}

// player:getSkillMax(skill) -> Skills[skill]->GetMax(). nullptr: checked; no
// player, skill out of range or missing: binding's check.
static int PlayerGetSkillMax(TScriptPlayer Self, int SkillNr) {
	TPlayer *pl = ExistingPlayer(Self.ID, "Player.getSkillMax", "Player.getSkillMax: the creature is not a player");
	return ExistingSkill(pl, SkillNr, "Player.getSkillMax: no such skill")->GetMax();
}

// player:setSkillTimer(skill, cycle, count, maxCount, additional) ->
// SetTimer(skill, Cycle, Count, MaxCount, AdditionalValue), what it returns.
// nullptr: checked; no player, skill out of range or missing: binding's
// check. Throws nothing else.
static bool PlayerSetSkillTimer(TScriptPlayer Self, int SkillNr, int Cycle, int Count, int MaxCount,
		int AdditionalValue) {
	TPlayer *pl = ExistingPlayer(Self.ID, "Player.setSkillTimer", "Player.setSkillTimer: the creature is not a player");
	ExistingSkill(pl, SkillNr, "Player.setSkillTimer: no such skill");
	return pl->SetTimer(static_cast<unsigned short>(SkillNr), Cycle, Count, MaxCount, AdditionalValue);
}

// player:useContainer(item, window) -> UseContainer(ID, Con, Window) of
// triggers.cc: opens the container in the window, or closes it where it is
// open. ID: passed on (UseContainer checks the object first, then logs a
// player that is gone and throws ERROR). window 0 to 255, an unsigned char
// as Use's Info (binding's check; UseContainer checks it against
// MAX_OPEN_CONTAINERS). Throws what UseContainer throws.
static void PlayerUseContainer(TScriptPlayer Self, sol::optional<TScriptItem> Container, int Window) {
	if (Window < 0 || Window > 255) {
		throw "Player.useContainer: window must be 0 to 255";
	}
	UseContainer(Self.ID, ItemObject(Container), static_cast<unsigned char>(Window));
}

// player:sendEditText(item) -> SendEditText(GetConnection(), Obj) of
// sending.cc: the text dialog of the object. nullptr: checked; no player:
// binding's check. Throws what SendEditText throws.
static void PlayerSendEditText(TScriptPlayer Self, sol::optional<TScriptItem> Item) {
	TPlayer *pl = ExistingPlayer(Self.ID, "Player.sendEditText", "Player.sendEditText: the creature is not a player");
	SendEditText(pl->GetConnection(), ItemObject(Item));
}

// player:mayOpenDoor(door) -> MayOpenDoor(Door, pl) of houses.cc: whether
// the name door's list lets the player in. nullptr: checked; no player:
// binding's check. Throws what MayOpenDoor throws.
static bool PlayerMayOpenDoor(TScriptPlayer Self, sol::optional<TScriptItem> Door) {
	TPlayer *pl = ExistingPlayer(Self.ID, "Player.mayOpenDoor", "Player.mayOpenDoor: the creature is not a player");
	return MayOpenDoor(ItemObject(Door), pl);
}

// ---------------------------------------------------------------------------
// Game

// Game.move(creatureId, item, container, count, noMerge, ignore|nil) ->
// Move(CreatureID, Obj, Con, Count, NoMerge, Ignore) of operate.cc; item an
// Item, or a Creature or Player whose object is moved (GetCreatureObject; a
// creature that is gone: nullptr checked), nil for NONE. Throws what Move
// throws.
static void GameMove(uint32_t CreatureID, sol::optional<TScriptThing> Item, sol::optional<TScriptItem> Container,
		int Count, bool NoMerge, sol::optional<TScriptItem> Ignore) {
	Object Obj = ThingObject(Item, "Game.move");
	Move(CreatureID, Obj, ItemObject(Container), Count, NoMerge, ItemObject(Ignore));
}

// Game.copyObject(container, item) -> Copy(Con, Source) of operate.cc, the
// copy, nil for NONE. Throws what Copy throws.
static sol::optional<TScriptItem> GameCopyObject(sol::optional<TScriptItem> Container,
		sol::optional<TScriptItem> Source) {
	return ItemResult(Copy(ItemObject(Container), ItemObject(Source)));
}

// Game.createPool(container, type, liquidType) -> CreatePool(MapCon,
// ObjectType(Type), LiquidType) of operate.cc. Throws what CreatePool throws.
static void GameCreatePool(sol::optional<TScriptItem> MapContainer, int Type, uint32_t LiquidType) {
	CreatePool(ItemObject(MapContainer), ObjectType(Type), LiquidType);
}

// Game.getBodyObject(creatureId, position) -> GetBodyObject(CreatureID,
// Position), the object in the body position, nil for NONE (GetBodyObject
// logs a position or creature that does not exist). Throws nothing.
static sol::optional<TScriptItem> GameGetBodyObject(uint32_t CreatureID, int Position) {
	return ItemResult(GetBodyObject(CreatureID, Position));
}

// Game.getBodyContainer(creatureId, position) -> GetBodyContainer(CreatureID,
// Position), the body position's container, nil for NONE (logged as by
// GetBodyContainer). Throws nothing.
static sol::optional<TScriptItem> GameGetBodyContainer(uint32_t CreatureID, int Position) {
	return ItemResult(GetBodyContainer(CreatureID, Position));
}

// Game.getInventoryWeight(creatureId) -> GetInventoryWeight(CreatureID) (0,
// logged, for a creature that is gone). Throws nothing.
static int GameGetInventoryWeight(uint32_t CreatureID) {
	return GetInventoryWeight(CreatureID);
}

// Game.isLayable(position) -> CoordinateLayable(x, y, z). Throws nothing.
static bool GameIsLayable(TScriptPosition Position) {
	return CoordinateLayable(Position.x, Position.y, Position.z);
}

// Game.getSpecialObject(meaning) -> GetSpecialObject(Meaning).getTypeID(),
// the type with the meaning (SPECIAL_ constants; 0, logged, for a meaning no
// type has). meaning 0 to SPECIALMEANINGS - 1 (binding's check: the C++
// function would read SpecialObjects[SPECIALMEANINGS]). Throws nothing else.
static int GameGetSpecialObject(int Meaning) {
	if (Meaning < 0 || Meaning >= SPECIALMEANINGS) {
		throw "Game.getSpecialObject: no such meaning";
	}
	return GetSpecialObject(static_cast<SPECIALMEANING>(Meaning)).getTypeID();
}

// Game.getDate() -> year, cycle, day (GetDate of time.cc), three results.
// Throws nothing.
static std::tuple<int, int, int> GameGetDate() {
	int Year, Cycle, Day;
	GetDate(Year, Cycle, Day);
	return std::make_tuple(Year, Cycle, Day);
}

// Game.getTime() -> hour, minute (GetTime of time.cc), two results. Throws
// nothing.
static std::tuple<int, int> GameGetTime() {
	int Hour, Minute;
	GetTime(Hour, Minute);
	return std::make_tuple(Hour, Minute);
}

// Game.useObject(player, item) -> UseObject(ID, Obj) of triggers.cc: the
// trigger rules of the event use (NOTUSABLE when none holds). ID: passed on.
// Throws what UseObject throws.
static void GameUseObject(TScriptPlayer Player, sol::optional<TScriptItem> Item) {
	UseObject(Player.ID, ItemObject(Item));
}

// Game.useObjects(player, item, target) -> UseObjects(ID, Obj1, Obj2) of
// triggers.cc: the trigger rules of the event multi_use (NOTUSABLE when none
// holds). ID: passed on. Throws what UseObjects throws.
static void GameUseObjects(TScriptPlayer Player, sol::optional<TScriptItem> Item,
		sol::optional<TScriptItem> Target) {
	UseObjects(Player.ID, ItemObject(Item), ItemObject(Target));
}

// ---------------------------------------------------------------------------
// Magic

// Magic.useMagicItem(player, item, target) -> UseMagicItem(ID, Obj, Dest) of
// magic.cc: a rune's use (its checks, then its script, phase 2); target nil
// for NONE. ID: passed on. Throws what UseMagicItem throws.
static void MagicUseMagicItem(TScriptPlayer Player, sol::optional<TScriptItem> Item,
		sol::optional<TScriptItem> Target) {
	UseMagicItem(Player.ID, ItemObject(Item), ItemObject(Target));
}

// Magic.refreshMana(actor, mana, soul, amount) -> RefreshMana(cr, Mana,
// SoulPoints, AddMana) of magic.cc: spends mana and soul points, adds AddMana
// mana, shows the blue sparkles. nullptr: passed on (RefreshMana logs it and
// throws ERROR). Throws what RefreshMana throws.
static void MagicRefreshMana(TScriptCreature Actor, int Mana, int SoulPoints, int AddMana) {
	RefreshMana(GetCreature(Actor.ID), Mana, SoulPoints, AddMana);
}

// ---------------------------------------------------------------------------
// The constants

// A constant of the uses' scripts: its name and its value.
struct TItemConstant {
	const char *Name;
	int Value;
};

static const TItemConstant Constants[] = {
	// The liquids of objects.hh.
	{"LIQUID_NONE", LIQUID_NONE},
	{"LIQUID_WATER", LIQUID_WATER},
	{"LIQUID_WINE", LIQUID_WINE},
	{"LIQUID_BEER", LIQUID_BEER},
	{"LIQUID_MUD", LIQUID_MUD},
	{"LIQUID_BLOOD", LIQUID_BLOOD},
	{"LIQUID_SLIME", LIQUID_SLIME},
	{"LIQUID_OIL", LIQUID_OIL},
	{"LIQUID_URINE", LIQUID_URINE},
	{"LIQUID_MILK", LIQUID_MILK},
	{"LIQUID_MANAFLUID", LIQUID_MANAFLUID},
	{"LIQUID_LIFEFLUID", LIQUID_LIFEFLUID},
	{"LIQUID_LEMONADE", LIQUID_LEMONADE},

	// The values of INFORMATIONTYPE (objects.hh).
	{"INFORMATION_DATE", INFORMATION_DATE},
	{"INFORMATION_TIME", INFORMATION_TIME},
	{"INFORMATION_BLESSINGS", INFORMATION_BLESSINGS},
	{"INFORMATION_SPELLBOOK", INFORMATION_SPELLBOOK},

	// The quest values of the blessings (cr.hh), the number of quest values
	// (dbfuncs.hh), the container windows (cr.hh), the last body position
	// (objects.hh).
	{"FIRST_BLESSING_QUEST", FIRST_BLESSING_QUEST},
	{"LAST_BLESSING_QUEST", LAST_BLESSING_QUEST},
	{"MAX_QUESTS", MAX_QUESTS},
	{"MAX_OPEN_CONTAINERS", MAX_OPEN_CONTAINERS},
	{"LAST_BODY_POSITION", LAST_BODY_POSITION},

	// The weapon types of cr.hh (the values of WEAPONTYPE), TFS's names.
	{"WEAPON_NONE", WEAPON_NONE},
	{"WEAPON_SWORD", WEAPON_SWORD},
	{"WEAPON_CLUB", WEAPON_CLUB},
	{"WEAPON_AXE", WEAPON_AXE},
	{"WEAPON_SHIELD", WEAPON_SHIELD},
	{"WEAPON_BOW", WEAPON_BOW},
	{"WEAPON_THROW", WEAPON_THROW},
};

// The values of SPECIALMEANING (objects.hh) by their C++ names, in the order
// of their numbers. The checks below fail to compile when an entry is out of
// order, when the count differs from the enum's (27 meanings) or when the
// last is not SPECIALMEANINGS - 1 (a meaning appended to the enum).
#define SPECIAL_CONSTANT(Name) {"SPECIAL_" #Name, Name}
static constexpr TItemConstant SpecialConstants[] = {
	SPECIAL_CONSTANT(MONEY_ONE),
	SPECIAL_CONSTANT(MONEY_HUNDRED),
	SPECIAL_CONSTANT(MONEY_TENTHOUSAND),
	SPECIAL_CONSTANT(INVENTORY_RIGHTHAND),
	SPECIAL_CONSTANT(INVENTORY_LEFTHAND),
	SPECIAL_CONSTANT(INVENTORY_BODY_MALE),
	SPECIAL_CONSTANT(INVENTORY_BODY_FEMALE),
	SPECIAL_CONSTANT(INVENTORY_CONTAINER),
	SPECIAL_CONSTANT(INVENTORY_FOOD),
	SPECIAL_CONSTANT(DEPOT_LOCKER),
	SPECIAL_CONSTANT(DEPOT_CHEST),
	SPECIAL_CONSTANT(PARCEL_NEW),
	SPECIAL_CONSTANT(PARCEL_STAMPED),
	SPECIAL_CONSTANT(PARCEL_LABEL),
	SPECIAL_CONSTANT(LETTER_NEW),
	SPECIAL_CONSTANT(LETTER_STAMPED),
	SPECIAL_CONSTANT(BLOOD_SPLASH),
	SPECIAL_CONSTANT(BLOOD_POOL),
	SPECIAL_CONSTANT(RUNE_BLANK),
	SPECIAL_CONSTANT(MAGICFIELD_FIRE_DANGEROUS),
	SPECIAL_CONSTANT(MAGICFIELD_FIRE_HARMLESS),
	SPECIAL_CONSTANT(MAGICFIELD_POISON_DANGEROUS),
	SPECIAL_CONSTANT(MAGICFIELD_POISON_HARMLESS),
	SPECIAL_CONSTANT(MAGICFIELD_ENERGY_DANGEROUS),
	SPECIAL_CONSTANT(MAGICFIELD_ENERGY_HARMLESS),
	SPECIAL_CONSTANT(MAGICFIELD_MAGICWALL),
	SPECIAL_CONSTANT(MAGICFIELD_RUSHWOOD),
};
#undef SPECIAL_CONSTANT

// The values of RIGHT (info.hh) by their C++ names, in the order of their
// numbers; the checks below fail to compile when the list does not name every
// value from 0 to NUMBER_OF_RIGHTS - 1 in order.
#define RIGHT_CONSTANT(Name) {"RIGHT_" #Name, Name}
static constexpr TItemConstant RightConstants[] = {
	RIGHT_CONSTANT(PREMIUM_ACCOUNT),
	RIGHT_CONSTANT(NOTATION),
	RIGHT_CONSTANT(NAMELOCK),
	RIGHT_CONSTANT(STATEMENT_REPORT),
	RIGHT_CONSTANT(BANISHMENT),
	RIGHT_CONSTANT(FINAL_WARNING),
	RIGHT_CONSTANT(IP_BANISHMENT),
	RIGHT_CONSTANT(KICK),
	RIGHT_CONSTANT(HOME_TELEPORT),
	RIGHT_CONSTANT(GAMEMASTER_BROADCAST),
	RIGHT_CONSTANT(ANONYMOUS_BROADCAST),
	RIGHT_CONSTANT(NO_BANISHMENT),
	RIGHT_CONSTANT(ALLOW_MULTICLIENT),
	RIGHT_CONSTANT(LOG_COMMUNICATION),
	RIGHT_CONSTANT(READ_GAMEMASTER_CHANNEL),
	RIGHT_CONSTANT(READ_TUTOR_CHANNEL),
	RIGHT_CONSTANT(HIGHLIGHT_HELP_CHANNEL),
	RIGHT_CONSTANT(SEND_BUGREPORTS),
	RIGHT_CONSTANT(NAME_INSULTING),
	RIGHT_CONSTANT(NAME_SENTENCE),
	RIGHT_CONSTANT(NAME_NONSENSICAL_LETTERS),
	RIGHT_CONSTANT(NAME_BADLY_FORMATTED),
	RIGHT_CONSTANT(NAME_NO_PERSON),
	RIGHT_CONSTANT(NAME_CELEBRITY),
	RIGHT_CONSTANT(NAME_COUNTRY),
	RIGHT_CONSTANT(NAME_FAKE_IDENTITY),
	RIGHT_CONSTANT(NAME_FAKE_POSITION),
	RIGHT_CONSTANT(STATEMENT_INSULTING),
	RIGHT_CONSTANT(STATEMENT_SPAMMING),
	RIGHT_CONSTANT(STATEMENT_ADVERT_OFFTOPIC),
	RIGHT_CONSTANT(STATEMENT_ADVERT_MONEY),
	RIGHT_CONSTANT(STATEMENT_NON_ENGLISH),
	RIGHT_CONSTANT(STATEMENT_CHANNEL_OFFTOPIC),
	RIGHT_CONSTANT(STATEMENT_VIOLATION_INCITING),
	RIGHT_CONSTANT(CHEATING_BUG_ABUSE),
	RIGHT_CONSTANT(CHEATING_GAME_WEAKNESS),
	RIGHT_CONSTANT(CHEATING_MACRO_USE),
	RIGHT_CONSTANT(CHEATING_MODIFIED_CLIENT),
	RIGHT_CONSTANT(CHEATING_HACKING),
	RIGHT_CONSTANT(CHEATING_MULTI_CLIENT),
	RIGHT_CONSTANT(CHEATING_ACCOUNT_TRADING),
	RIGHT_CONSTANT(CHEATING_ACCOUNT_SHARING),
	RIGHT_CONSTANT(GAMEMASTER_THREATENING),
	RIGHT_CONSTANT(GAMEMASTER_PRETENDING),
	RIGHT_CONSTANT(GAMEMASTER_INFLUENCE),
	RIGHT_CONSTANT(GAMEMASTER_FALSE_REPORTS),
	RIGHT_CONSTANT(KILLING_EXCESSIVE_UNJUSTIFIED),
	RIGHT_CONSTANT(DESTRUCTIVE_BEHAVIOUR),
	RIGHT_CONSTANT(SPOILING_AUCTION),
	RIGHT_CONSTANT(INVALID_PAYMENT),
	RIGHT_CONSTANT(TELEPORT_TO_CHARACTER),
	RIGHT_CONSTANT(TELEPORT_TO_MARK),
	RIGHT_CONSTANT(TELEPORT_VERTICAL),
	RIGHT_CONSTANT(TELEPORT_TO_COORDINATE),
	RIGHT_CONSTANT(LEVITATE),
	RIGHT_CONSTANT(SPECIAL_MOVEUSE),
	RIGHT_CONSTANT(MODIFY_GOSTRENGTH),
	RIGHT_CONSTANT(SHOW_COORDINATE),
	RIGHT_CONSTANT(RETRIEVE),
	RIGHT_CONSTANT(ENTER_HOUSES),
	RIGHT_CONSTANT(OPEN_NAMEDOORS),
	RIGHT_CONSTANT(INVULNERABLE),
	RIGHT_CONSTANT(UNLIMITED_MANA),
	RIGHT_CONSTANT(KEEP_INVENTORY),
	RIGHT_CONSTANT(ALL_SPELLS),
	RIGHT_CONSTANT(UNLIMITED_CAPACITY),
	RIGHT_CONSTANT(ZERO_CAPACITY),
	RIGHT_CONSTANT(ATTACK_EVERYWHERE),
	RIGHT_CONSTANT(NO_ATTACK),
	RIGHT_CONSTANT(NO_RUNES),
	RIGHT_CONSTANT(NO_LOGOUT_BLOCK),
	RIGHT_CONSTANT(GAMEMASTER_OUTFIT),
	RIGHT_CONSTANT(ILLUMINATE),
	RIGHT_CONSTANT(CHANGE_PROFESSION),
	RIGHT_CONSTANT(IGNORED_BY_MONSTERS),
	RIGHT_CONSTANT(SHOW_KEYHOLE_NUMBERS),
	RIGHT_CONSTANT(CREATE_OBJECTS),
	RIGHT_CONSTANT(CREATE_MONEY),
	RIGHT_CONSTANT(CREATE_MONSTERS),
	RIGHT_CONSTANT(CHANGE_SKILLS),
	RIGHT_CONSTANT(CLEANUP_FIELDS),
	RIGHT_CONSTANT(NO_STATISTICS),
	RIGHT_CONSTANT(RELOAD_SCRIPTS),
};
#undef RIGHT_CONSTANT

// Whether the values of the Count entries of List rise strictly.
template<size_t Count>
static constexpr bool Ascending(const TItemConstant (&List)[Count]) {
	for (size_t i = 1; i < Count; i++) {
		if (List[i].Value <= List[i - 1].Value) {
			return false;
		}
	}
	return true;
}

// Whether the Count entries of List have the values 0 to Count - 1 in order.
template<size_t Count>
static constexpr bool NumberedInOrder(const TItemConstant (&List)[Count]) {
	for (size_t i = 0; i < Count; i++) {
		if (List[i].Value != static_cast<int>(i)) {
			return false;
		}
	}
	return true;
}

const size_t NUMBER_OF_SPECIAL_CONSTANTS = sizeof(SpecialConstants) / sizeof(SpecialConstants[0]);
static_assert(NUMBER_OF_SPECIAL_CONSTANTS == 27, "SpecialConstants must name every SPECIALMEANING");
static_assert(Ascending(SpecialConstants), "SpecialConstants must be in the order of the numbers of SPECIALMEANING");
static_assert(SpecialConstants[0].Value >= 1, "a SPECIALMEANING is 1 or more");
static_assert(SpecialConstants[NUMBER_OF_SPECIAL_CONSTANTS - 1].Value == SPECIALMEANINGS - 1,
		"SpecialConstants must end with the last SPECIALMEANING, SPECIALMEANINGS - 1");
static_assert(sizeof(RightConstants) / sizeof(RightConstants[0]) == NUMBER_OF_RIGHTS,
		"RightConstants must name every RIGHT");
static_assert(NumberedInOrder(RightConstants), "RightConstants must be in the order of the numbers of RIGHT");
static_assert(RELOAD_SCRIPTS == NUMBER_OF_RIGHTS - 1, "NUMBER_OF_RIGHTS must follow the last RIGHT");

void RegisterItemAPI() {
	SetItemTypeConstructor(&ItemTypeNumber);

	sol::table ItemType = ScriptAPITable("ItemType");
	ItemType.set_function("getId", Guarded(&ItemTypeGetId));
	ItemType.set_function("hasFlag", Guarded(&ItemTypeHasFlag));
	ItemType.set_function("getAttribute", Guarded(&ItemTypeGetAttribute));
	ItemType.set_function("getName", Guarded(&ItemTypeGetName));

	sol::table Item = ScriptAPITable("Item");
	Item.set_function("exists", Guarded(&ItemExists));
	Item.set_function("getName", Guarded(&ItemGetName));
	Item.set_function("getWeight", Guarded(&ItemGetWeight));
	Item.set_function("transform", Guarded(&ItemTransform));
	Item.set_function("clearField", Guarded(&ItemClearField));
	Item.set_function("empty", Guarded(&ItemEmpty));

	sol::table Creature = ScriptAPITable("Creature");
	Creature.set_function("isRemoved", Guarded(&CreatureIsRemoved));
	Creature.set_function("say", Guarded(&CreatureSay));

	sol::table Player = ScriptAPITable("Player");
	Player.set_function("isRemoved", Guarded(&PlayerIsRemoved));
	Player.set_function("getSkillTimer", Guarded(&PlayerGetSkillTimer));
	Player.set_function("getSkillMax", Guarded(&PlayerGetSkillMax));
	Player.set_function("setSkillTimer", Guarded(&PlayerSetSkillTimer));
	Player.set_function("useContainer", Guarded(&PlayerUseContainer));
	Player.set_function("sendEditText", Guarded(&PlayerSendEditText));
	Player.set_function("mayOpenDoor", Guarded(&PlayerMayOpenDoor));

	sol::table Game = ScriptAPITable("Game");
	Game.set_function("move", Guarded(&GameMove));
	Game.set_function("copyObject", Guarded(&GameCopyObject));
	Game.set_function("createPool", Guarded(&GameCreatePool));
	Game.set_function("getBodyObject", Guarded(&GameGetBodyObject));
	Game.set_function("getBodyContainer", Guarded(&GameGetBodyContainer));
	Game.set_function("getInventoryWeight", Guarded(&GameGetInventoryWeight));
	Game.set_function("isLayable", Guarded(&GameIsLayable));
	Game.set_function("getSpecialObject", Guarded(&GameGetSpecialObject));
	Game.set_function("getDate", Guarded(&GameGetDate));
	Game.set_function("getTime", Guarded(&GameGetTime));
	Game.set_function("useObject", Guarded(&GameUseObject));
	Game.set_function("useObjects", Guarded(&GameUseObjects));

	sol::table Magic = ScriptAPITable("Magic");
	Magic.set_function("useMagicItem", Guarded(&MagicUseMagicItem));
	Magic.set_function("refreshMana", Guarded(&MagicRefreshMana));

	for (const TItemConstant &Constant : Constants) {
		SetScriptConstant(Constant.Name, Constant.Value);
	}
	for (const TItemConstant &Constant : SpecialConstants) {
		SetScriptConstant(Constant.Name, Constant.Value);
	}
	for (const TItemConstant &Constant : RightConstants) {
		SetScriptConstant(Constant.Name, Constant.Value);
	}
}

namespace {

// The part of a reload for the scripts of the list [[use]] (reload.hh): the
// entries are the items part's (ItemsReloadPart, installed before), so read,
// install and uninstall have nothing to do; loadScripts loads each entry's
// script into the current engine and finish makes the numbers the entries'.
// The start uses it as well (InitItemUses).
class TItemUsesReloadPart : public TReloadPart {
public:
	const char *name() const override { return "item uses"; }
	void read() override { Numbers.clear(); }
	void install() override {}
	void loadScripts() override;
	void finish() override;
	void uninstall() override {}
	void discard() override { Numbers.clear(); }
	std::string summary() const override;
private:
	// The script number of each entry of UseEntries, in its order.
	std::vector<int> Numbers;
};

// The text loadScripts throws for a missing list.
std::string UseError;

void TItemUsesReloadPart::loadScripts() {
	// A reload's items part refuses such a file before; this is the start's
	// refusal.
	if (UseEntries.empty()) {
		UseError = std::string(DATAPATH) + "/items.toml: the list [[use]] is missing (the built-in uses of objects "
				"need it)";
		throw UseError.c_str();
	}
	Numbers.clear();
	Numbers.reserve(UseEntries.size());
	for (const TUseEntry &Entry : UseEntries) {
		int Number = LoadScript(Entry.Script.c_str(), Entry.Where.c_str());
		RequireHandler(Number, "onUse", Entry.Where.c_str());
		Numbers.push_back(Number);
	}
}

void TItemUsesReloadPart::finish() {
	for (size_t i = 0; i < UseEntries.size() && i < Numbers.size(); i++) {
		UseEntries[i].ScriptNumber = Numbers[i];
	}
	Numbers.clear();
}

std::string TItemUsesReloadPart::summary() const {
	return "[[use]]: " + std::to_string(UseEntries.size()) + " scripts";
}

} // namespace

TReloadPart &ItemUsesReloadPart() {
	static TItemUsesReloadPart Part;
	return Part;
}

// The start: the part's steps one after the other (the scripts loaded in the
// order of the list, each entry's number set when finish runs, after the
// last).
void InitItemUses() {
	TReloadPart &Part = ItemUsesReloadPart();
	Part.read();
	Part.install();
	Part.loadScripts();
	Part.finish();
}
