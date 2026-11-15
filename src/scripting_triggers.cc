// scripting_triggers.cc: the API of the trigger library (scripting.hh,
// "Trigger rules"): the methods of Item, Creature and Player and the Game
// functions the conditions and actions of the trigger rules use, and the
// constants (TYPEID_CREATURE, SKILL_LAST, MESSAGE_INFO_DESCR, FLAG_,
// ITEMTYPE_ATTRIBUTE_, ITEM_ATTRIBUTE_). RegisterTriggerAPI binds them; InitAll
// calls it after RegisterMagicAPI, before InitTriggers loads the library.
//
// What every binding here does alike:
//  - An Item parameter, self included, is an Item object or nil; the binding
//    passes its handle on as Object(handle), nil as NONE (Object(0)), and
//    never checks it itself: the C++ function does for a handle that is gone
//    or NONE what it always did (AccessObject logs "Invalid object number
//    zero" and gives the empty object, GetObjectCoordinates logs "Given
//    object does not exist", ...), because the C++ conditions and actions
//    called the same functions on NONE in the same places. So the library
//    may call a method as a function of the class with nil, Item.getId(obj).
//  - An Item result is nil for NONE, an Item holding the handle otherwise.
//  - A Creature or Player self is resolved with GetCreature on every call;
//    every method here dereferences it, so a creature that is gone is logged
//    as "<Lua name>: Creature does not exist." and raises ERROR ("nullptr:
//    checked"); a Player method on a creature that is no player throws a
//    const char * (binding's check), which Guarded turns into a logged Lua
//    error and ERROR. The library calls them only for a creature that exists
//    (and is a player for the Player methods), as the C++ code checked.
//  - A number the C++ function takes as uint32_t (an instance attribute's
//    value, a type's value, a dynamic string's number) is converted from the
//    Lua number as the C++ code converted the int it held: modulo 2^32, so a
//    negative int gives what (uint32_t) gave. A uint32_t result (an
//    attribute, CronInfo) is given as it is, 0 to 2^32 - 1: the C++ code
//    compared some as int (Compare) and some as uint32_t, which is the
//    library's to do.
//  - A string passed on as a char * is a copy (the whole string; TextEffect
//    and AddDynamicString take texts up to the longest dynamic string), so
//    the C++ function never touches Lua's string.
//  - The binding's own checks (a flag, attribute, skill, right or house
//    number out of range) guard what the C++ code never got from its checked
//    tables and would read out of bounds; they throw a const char *.
//  - What the C++ function throws (a RESULT) is raised as the result code
//    (Guarded).

#include <cstdint>
#include <string>

#include "objects.hh"
#include "map.hh"
#include "info.hh"
#include "cr.hh"
#include "crskill.hh"
#include "crmain.hh"
#include "crnonpl.hh"
#include "houses.hh"
#include "magicdata.hh"
#include "operate.hh"
#include "sending.hh"
#include "strings.hh"
#include "utils.hh"
#include "writer.hh"
#include "triggers.hh"
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

// A writable copy of a string for a C++ function that takes a char *.
static std::string Copy(const std::string &Text) {
	return std::string(Text.c_str());
}

// ---------------------------------------------------------------------------
// Item

// item:getId() -> Obj.getObjectType().getTypeID(), the object's type. Throws
// nothing.
static int ItemGetId(sol::optional<TScriptItem> Self) {
	return ItemObject(Self).getObjectType().getTypeID();
}

// item:isCreature() -> Obj.isCreature(). Throws nothing.
static bool ItemIsCreature(sol::optional<TScriptItem> Self) {
	return ItemObject(Self).isCreature();
}

// item:getCreature() -> GetCreature(Obj) (by Obj.getCreatureID(), which logs
// for an object that is no creature) as a Creature or a Player, nil for
// nullptr. Throws nothing.
static TScriptCreature ItemGetCreature(sol::optional<TScriptItem> Self) {
	TCreature *cr = GetCreature(ItemObject(Self));
	return TScriptCreature{cr != nullptr ? cr->ID : 0};
}

// item:getCreatureId() -> Obj.getCreatureID() (0, logged, for an object that
// is no creature). Throws nothing.
static uint32_t ItemGetCreatureId(sol::optional<TScriptItem> Self) {
	return ItemObject(Self).getCreatureID();
}

// item:hasFlag(flag) -> Obj.getFlag(flag), flag a FLAG number (FLAG_
// constants), 0 to FLAGS - 1 (binding's check). Throws nothing else.
static bool ItemHasFlag(sol::optional<TScriptItem> Self, int Flag) {
	if (Flag < 0 || Flag >= FLAGS) {
		throw "Item.hasFlag: no such flag";
	}
	return ItemObject(Self).getFlag(static_cast<FLAG>(Flag));
}

// item:getTypeAttribute(attribute) -> Obj.getAttribute(TYPEATTRIBUTE), the
// type's attribute (ITEMTYPE_ATTRIBUTE_ constants), 0 to TYPEATTRIBUTES - 1
// (binding's check). Returns the uint32_t. Throws nothing else.
static uint32_t ItemGetTypeAttribute(sol::optional<TScriptItem> Self, int Attribute) {
	if (Attribute < 0 || Attribute >= TYPEATTRIBUTES) {
		throw "Item.getTypeAttribute: no such type attribute";
	}
	return ItemObject(Self).getAttribute(static_cast<TYPEATTRIBUTE>(Attribute));
}

// item:getAttribute(attribute) -> Obj.getAttribute(INSTANCEATTRIBUTE), the
// object's own attribute (ITEM_ATTRIBUTE_ constants), 0 to
// INSTANCEATTRIBUTES - 1 (binding's check). Returns the uint32_t (0, logged,
// for a type without it). Throws nothing else.
static uint32_t ItemGetAttribute(sol::optional<TScriptItem> Self, int Attribute) {
	if (Attribute < 0 || Attribute >= INSTANCEATTRIBUTES) {
		throw "Item.getAttribute: no such attribute";
	}
	return ItemObject(Self).getAttribute(static_cast<INSTANCEATTRIBUTE>(Attribute));
}

// item:setAttribute(attribute, value) -> Change(Obj, INSTANCEATTRIBUTE,
// Value) of operate.cc: sets the attribute and sends the change. attribute 0
// to INSTANCEATTRIBUTES - 1 (binding's check). Throws what Change throws.
static void ItemSetAttribute(sol::optional<TScriptItem> Self, int Attribute, uint32_t Value) {
	if (Attribute < 0 || Attribute >= INSTANCEATTRIBUTES) {
		throw "Item.setAttribute: no such attribute";
	}
	Change(ItemObject(Self), static_cast<INSTANCEATTRIBUTE>(Attribute), Value);
}

// item:getPosition() -> GetObjectCoordinates(Obj, x, y, z) as a Position
// ([0, 0, 0], logged, for an object that does not exist). Throws nothing.
static TScriptPosition ItemGetPosition(sol::optional<TScriptItem> Self) {
	TScriptPosition Position;
	GetObjectCoordinates(ItemObject(Self), Position.x, Position.y, Position.z);
	return Position;
}

// item:getContainer() -> Obj.getContainer(), nil for NONE. Throws nothing.
static sol::optional<TScriptItem> ItemGetContainer(sol::optional<TScriptItem> Self) {
	return ItemResult(ItemObject(Self).getContainer());
}

// item:getMapContainer() -> GetMapContainer(Obj), the map field the object
// lies on (in a container: the container's), nil for NONE. Throws nothing.
static sol::optional<TScriptItem> ItemGetMapContainer(sol::optional<TScriptItem> Self) {
	return ItemResult(GetMapContainer(ItemObject(Self)));
}

// item:getNextObject() -> Obj.getNextObject(), nil for NONE. Throws nothing.
static sol::optional<TScriptItem> ItemGetNextObject(sol::optional<TScriptItem> Self) {
	return ItemResult(ItemObject(Self).getNextObject());
}

// item:getFirstObject() -> GetFirstContainerObject(Obj), the first object in
// the container, nil for NONE. Throws nothing.
static sol::optional<TScriptItem> ItemGetFirstObject(sol::optional<TScriptItem> Self) {
	return ItemResult(GetFirstContainerObject(ItemObject(Self)));
}

// item:countObjects() -> CountObjectsInContainer(Obj). Throws nothing.
static int ItemCountObjects(sol::optional<TScriptItem> Self) {
	return CountObjectsInContainer(ItemObject(Self));
}

// item:getBodyPosition() -> GetObjectBodyPosition(Obj). Throws nothing.
static int ItemGetBodyPosition(sol::optional<TScriptItem> Self) {
	return GetObjectBodyPosition(ItemObject(Self));
}

// item:getDescription() -> GetInfo(Obj), the type's description, nil for
// nullptr (an object that does not exist, logged). Throws nothing.
static sol::optional<std::string> ItemGetDescription(sol::optional<TScriptItem> Self) {
	const char *Info = GetInfo(ItemObject(Self));
	if (Info == nullptr) {
		return sol::nullopt;
	}
	return std::string(Info);
}

// item:getCronInfo() -> CronInfo(Obj, false), the object's remaining expire
// time. Returns the uint32_t. Throws nothing.
static uint32_t ItemGetCronInfo(sol::optional<TScriptItem> Self) {
	return CronInfo(ItemObject(Self), false);
}

// item:sendMagicEffect(effect) -> GraphicalEffect(Obj, Effect) of operate.cc
// (CONST_ME_ constants). Throws what GraphicalEffect throws.
static void ItemSendMagicEffect(sol::optional<TScriptItem> Self, int Effect) {
	GraphicalEffect(ItemObject(Self), Effect);
}

// ---------------------------------------------------------------------------
// Creature, Player

// creature:isPlayer() -> Type == PLAYER. nullptr: checked.
static bool CreatureIsPlayer(TScriptCreature Self) {
	return ExistingCreature(Self.ID, "Creature.isPlayer")->Type == PLAYER;
}

// creature:isPeaceful() -> IsPeaceful(). nullptr: checked.
static bool CreatureIsPeaceful(TScriptCreature Self) {
	return ExistingCreature(Self.ID, "Creature.isPeaceful")->IsPeaceful();
}

// creature:getName() -> Name, a copy. nullptr: checked.
static std::string CreatureGetName(TScriptCreature Self) {
	return ExistingCreature(Self.ID, "Creature.getName")->Name;
}

// creature:probeSkill(skill, a, b) -> Skills[skill]->Probe(a, b, true), a
// test of the skill (draws random numbers). nullptr: checked. skill 0 to
// SKILL_LAST and present for the creature (binding's check). Throws nothing
// else.
static bool CreatureProbeSkill(TScriptCreature Self, int SkillNr, int Value, int Probability) {
	TCreature *cr = ExistingCreature(Self.ID, "Creature.probeSkill");
	if (SkillNr < 0 || SkillNr >= MAX_SKILLS || cr->Skills[SkillNr] == nullptr) {
		throw "Creature.probeSkill: no such skill";
	}
	return cr->Skills[SkillNr]->Probe(Value, Probability, true);
}

// creature:damage(attacker|nil, amount, damageType) -> Damage(atk, Amount,
// DamageType), atk the attacker's creature (nullptr for nil or one that is
// gone). Returns what Damage returns. nullptr (self): checked. Throws what
// Damage throws.
static int CreatureDamage(TScriptCreature Self, sol::optional<TScriptCreature> Attacker, int Amount,
		int DamageType) {
	TCreature *cr = ExistingCreature(Self.ID, "Creature.damage");
	TCreature *atk = Attacker.has_value() ? GetCreature(Attacker->ID) : nullptr;
	return cr->Damage(atk, Amount, DamageType);
}

// creature:startLogout() -> StartLogout(true, true). nullptr: checked.
// Throws what StartLogout throws.
static void CreatureStartLogout(TScriptCreature Self) {
	ExistingCreature(Self.ID, "Creature.startLogout")->StartLogout(true, true);
}

// player:canLogout() -> LogoutPossible() == LOGOUT_POSSIBLE. nullptr:
// checked; no player: binding's check.
static bool PlayerCanLogout(TScriptPlayer Self) {
	return ExistingPlayer(Self.ID, "Player.canLogout", "Player.canLogout: the creature is not a player")
			->LogoutPossible() == LOGOUT_POSSIBLE;
}

// player:hasRight(right) -> CheckRight(ID, right), right a RIGHT number, 0 to
// NUMBER_OF_RIGHTS - 1 (binding's check). nullptr: checked (CheckRight would
// log "Player does not exist" and give false; the C++ code called it only
// for an existing player); no player: binding's check.
static bool PlayerHasRight(TScriptPlayer Self, int Right) {
	ExistingPlayer(Self.ID, "Player.hasRight", "Player.hasRight: the creature is not a player");
	if (Right < 0 || Right >= NUMBER_OF_RIGHTS) {
		throw "Player.hasRight: no such right";
	}
	return CheckRight(Self.ID, static_cast<RIGHT>(Right));
}

// player:getStorageValue(number) -> GetQuestValue(Number). nullptr: checked;
// no player: binding's check. Throws nothing else.
static int PlayerGetStorageValue(TScriptPlayer Self, int Number) {
	return ExistingPlayer(Self.ID, "Player.getStorageValue", "Player.getStorageValue: the creature is not a player")
			->GetQuestValue(Number);
}

// player:setStorageValue(number, value) -> SetQuestValue(Number, Value).
// nullptr: checked; no player: binding's check. Throws nothing else.
static void PlayerSetStorageValue(TScriptPlayer Self, int Number, int Value) {
	ExistingPlayer(Self.ID, "Player.setStorageValue", "Player.setStorageValue: the creature is not a player")
			->SetQuestValue(Number, Value);
}

// player:isHouseOwner(houseId) -> IsOwner(HouseID, pl), houseId 0 to 65535
// (an unsigned short, as GetHouseID gives it; binding's check). nullptr:
// checked; no player: binding's check.
static bool PlayerIsHouseOwner(TScriptPlayer Self, int HouseID) {
	TPlayer *pl = ExistingPlayer(Self.ID, "Player.isHouseOwner", "Player.isHouseOwner: the creature is not a player");
	if (HouseID < 0 || HouseID > 0xFFFF) {
		throw "Player.isHouseOwner: no such house number";
	}
	return IsOwner(static_cast<unsigned short>(HouseID), pl);
}

// player:setStartPosition(position): what the action set_start did for a
// player: Log("game", "Setting new start position for %s.\n", Name), the
// start coordinates set to position, SaveData(). nullptr: checked; no
// player: binding's check. Throws what SaveData throws.
static void PlayerSetStartPosition(TScriptPlayer Self, TScriptPosition Position) {
	TPlayer *pl = ExistingPlayer(Self.ID, "Player.setStartPosition",
			"Player.setStartPosition: the creature is not a player");
	Log("game", "Setting new start position for %s.\n", pl->Name);
	pl->startx = Position.x;
	pl->starty = Position.y;
	pl->startz = Position.z;
	pl->SaveData();
}

// player:sendTextMessage(mode, text) -> SendMessage(GetConnection(), Mode,
// "%s", Text) (MESSAGE_INFO_DESCR is TALK_INFO_MESSAGE), Text a copy.
// nullptr: checked; no player: binding's check. Throws what SendMessage
// throws.
static void PlayerSendTextMessage(TScriptPlayer Self, int Mode, std::string Text) {
	TPlayer *pl = ExistingPlayer(Self.ID, "Player.sendTextMessage",
			"Player.sendTextMessage: the creature is not a player");
	std::string Message = Copy(Text);
	SendMessage(pl->GetConnection(), Mode, "%s", Message.c_str());
}

// ---------------------------------------------------------------------------
// Game

// Game.getCreature(id) -> GetCreature(ID) as a Creature or a Player, nil for
// nullptr. Throws nothing.
static TScriptCreature GameGetCreature(uint32_t ID) {
	TCreature *cr = GetCreature(ID);
	return TScriptCreature{cr != nullptr ? cr->ID : 0};
}

// Game.getMapContainer(position) -> GetMapContainer(x, y, z), the field, nil
// for NONE. Throws nothing.
static sol::optional<TScriptItem> GameGetMapContainer(TScriptPosition Position) {
	return ItemResult(GetMapContainer(Position.x, Position.y, Position.z));
}

// Game.getFirstObject(position) -> GetFirstObject(x, y, z), nil for NONE.
// Throws nothing.
static sol::optional<TScriptItem> GameGetFirstObject(TScriptPosition Position) {
	return ItemResult(GetFirstObject(Position.x, Position.y, Position.z));
}

// Game.getFirstObjectOfType(position, type) -> GetFirstSpecObject(x, y, z,
// ObjectType(Type)), the first object of the type on the field (type
// TYPEID_CREATURE: the first creature), nil for NONE. Throws nothing.
static sol::optional<TScriptItem> GameGetFirstObjectOfType(TScriptPosition Position, int Type) {
	return ItemResult(GetFirstSpecObject(Position.x, Position.y, Position.z, ObjectType(Type)));
}

// Game.getTopObject(position) -> GetTopObject(x, y, z, false), nil for NONE.
// Throws nothing.
static sol::optional<TScriptItem> GameGetTopObject(TScriptPosition Position) {
	return ItemResult(GetTopObject(Position.x, Position.y, Position.z, false));
}

// Game.getInventoryObject(creatureId, type, value) ->
// GetInventoryObject(CreatureID, ObjectType(Type), Value), nil for NONE.
// Throws nothing.
static sol::optional<TScriptItem> GameGetInventoryObject(uint32_t CreatureID, int Type, uint32_t Value) {
	return ItemResult(GetInventoryObject(CreatureID, ObjectType(Type), Value));
}

// Game.isProtectionZone(position) -> IsProtectionZone(x, y, z). Throws
// nothing.
static bool GameIsProtectionZone(TScriptPosition Position) {
	return IsProtectionZone(Position.x, Position.y, Position.z);
}

// Game.isHouse(position) -> IsHouse(x, y, z). Throws nothing.
static bool GameIsHouse(TScriptPosition Position) {
	return IsHouse(Position.x, Position.y, Position.z);
}

// Game.getHouseId(position) -> GetHouseID(x, y, z). Throws nothing.
static int GameGetHouseId(TScriptPosition Position) {
	return GetHouseID(Position.x, Position.y, Position.z);
}

// Game.createObject(container, type, value) -> CreateObject(Con,
// ObjectType(Type), Value) of triggers.cc, the new object, nil for NONE (a
// failure it logs). Throws what CreateObject throws.
static sol::optional<TScriptItem> GameCreateObject(sol::optional<TScriptItem> Container, int Type,
		uint32_t Value) {
	return ItemResult(CreateObject(ItemObject(Container), ObjectType(Type), Value));
}

// Game.changeObject(item, type, value) -> ChangeObject(Obj, ObjectType(Type),
// Value) of triggers.cc. Throws what ChangeObject throws.
static void GameChangeObject(sol::optional<TScriptItem> Item, int Type, uint32_t Value) {
	ChangeObject(ItemObject(Item), ObjectType(Type), Value);
}

// Game.createMonsterOfRace(race, position) -> CreateMonster(Race, x, y, z, 0,
// 0, true), the monster as a Creature, nil for nullptr. Throws what
// CreateMonster throws.
static TScriptCreature GameCreateMonsterOfRace(int Race, TScriptPosition Position) {
	TCreature *cr = CreateMonster(Race, Position.x, Position.y, Position.z, 0, 0, true);
	return TScriptCreature{cr != nullptr ? cr->ID : 0};
}

// Game.textEffect(text|nil, position, radius) -> TextEffect(Text, x, y, z,
// Radius) of triggers.cc, Text a copy, nullptr for nil (TextEffect logs it).
// Throws what TextEffect throws.
static void GameTextEffect(sol::optional<std::string> Text, TScriptPosition Position, int Radius) {
	std::string Buffer = Text.has_value() ? Copy(*Text) : std::string();
	TextEffect(Text.has_value() ? &Buffer[0] : nullptr, Position.x, Position.y, Position.z, Radius);
}

// Game.getDynamicString(number) -> GetDynamicString(Number), a copy, nil for
// nullptr. Throws nothing.
static sol::optional<std::string> GameGetDynamicString(uint32_t Number) {
	const char *Text = GetDynamicString(Number);
	if (Text == nullptr) {
		return sol::nullopt;
	}
	return std::string(Text);
}

// Game.addDynamicString(text) -> AddDynamicString(Text), Text a copy; the new
// string's number. Throws what AddDynamicString throws.
static uint32_t GameAddDynamicString(std::string Text) {
	std::string Buffer = Copy(Text);
	return AddDynamicString(&Buffer[0]);
}

// Game.deleteDynamicString(number) -> DeleteDynamicString(Number). Throws
// nothing.
static void GameDeleteDynamicString(uint32_t Number) {
	DeleteDynamicString(Number);
}

// Game.moveObject(item, container) -> MoveOneObject(Obj, Con) of
// triggers.cc. Throws what MoveOneObject throws.
static void GameMoveObject(sol::optional<TScriptItem> Item, sol::optional<TScriptItem> Container) {
	MoveOneObject(ItemObject(Item), ItemObject(Container));
}

// Game.moveAllObjects(item|nil, container, exclude|nil, moveUnmovable) ->
// MoveAllObjects(Obj, Dest, Exclude, MoveUnmovable) of triggers.cc. Throws
// what MoveAllObjects throws.
static void GameMoveAllObjects(sol::optional<TScriptItem> Item, sol::optional<TScriptItem> Container,
		sol::optional<TScriptItem> Exclude, bool MoveUnmovable) {
	MoveAllObjects(ItemObject(Item), ItemObject(Container), ItemObject(Exclude), MoveUnmovable);
}

// Game.deleteAllObjects(item|nil, exclude|nil, deleteUnmovable) ->
// DeleteAllObjects(Obj, Exclude, DeleteUnmovable) of triggers.cc. Throws what
// DeleteAllObjects throws.
static void GameDeleteAllObjects(sol::optional<TScriptItem> Item, sol::optional<TScriptItem> Exclude,
		bool DeleteUnmovable) {
	DeleteAllObjects(ItemObject(Item), ItemObject(Exclude), DeleteUnmovable);
}

// Game.deleteObject(item, count) -> Delete(Obj, Count) of operate.cc (-1:
// all). Throws what Delete throws.
static void GameDeleteObject(sol::optional<TScriptItem> Item, int Count) {
	Delete(ItemObject(Item), Count);
}

// Game.deleteAtCreature(creatureId, type, amount, value) ->
// DeleteAtCreature(CreatureID, ObjectType(Type), Amount, Data) of operate.cc.
// Throws what DeleteAtCreature throws.
static void GameDeleteAtCreature(uint32_t CreatureID, int Type, int Amount, uint32_t Data) {
	DeleteAtCreature(CreatureID, ObjectType(Type), Amount, Data);
}

// Game.loadDepotBox(creatureId, number, container) -> LoadDepotBox(CreatureID,
// Nr, Con) of triggers.cc. Throws what LoadDepotBox throws.
static void GameLoadDepotBox(uint32_t CreatureID, int Number, sol::optional<TScriptItem> Container) {
	LoadDepotBox(CreatureID, Number, ItemObject(Container));
}

// Game.saveDepotBox(creatureId, number, container) -> SaveDepotBox(CreatureID,
// Nr, Con) of triggers.cc. Throws what SaveDepotBox throws.
static void GameSaveDepotBox(uint32_t CreatureID, int Number, sol::optional<TScriptItem> Container) {
	SaveDepotBox(CreatureID, Number, ItemObject(Container));
}

// Game.sendMail(item) -> SendMail(Obj) of triggers.cc. Throws what SendMail
// throws.
static void GameSendMail(sol::optional<TScriptItem> Item) {
	SendMail(ItemObject(Item));
}

// ---------------------------------------------------------------------------
// The constants

// A constant of the trigger library: its name and its value.
struct TTriggerConstant {
	const char *Name;
	int Value;
};

static const TTriggerConstant Constants[] = {
	{"TYPEID_CREATURE", TYPEID_CREATURE},
	{"SKILL_LAST", MAX_SKILLS - 1},
	{"MESSAGE_INFO_DESCR", TALK_INFO_MESSAGE},
};

// The values of objects.hh's enums FLAG, TYPEATTRIBUTE and INSTANCEATTRIBUTE
// by their C++ names, in the order of their numbers. The checks below fail to
// compile when a list does not name every value from 0 to the enum's count
// (FLAGS, TYPEATTRIBUTES, INSTANCEATTRIBUTES) in order: an entry missing,
// misplaced or added to the enum but not here.
#define FLAG_CONSTANT(Name) {"FLAG_" #Name, Name}
static constexpr TTriggerConstant FlagConstants[] = {
	FLAG_CONSTANT(BANK),
	FLAG_CONSTANT(CLIP),
	FLAG_CONSTANT(BOTTOM),
	FLAG_CONSTANT(TOP),
	FLAG_CONSTANT(CONTAINER),
	FLAG_CONSTANT(CHEST),
	FLAG_CONSTANT(CUMULATIVE),
	FLAG_CONSTANT(USEEVENT),
	FLAG_CONSTANT(CHANGEUSE),
	FLAG_CONSTANT(FORCEUSE),
	FLAG_CONSTANT(MULTIUSE),
	FLAG_CONSTANT(DISTUSE),
	FLAG_CONSTANT(MOVEMENTEVENT),
	FLAG_CONSTANT(COLLISIONEVENT),
	FLAG_CONSTANT(SEPARATIONEVENT),
	FLAG_CONSTANT(KEY),
	FLAG_CONSTANT(KEYDOOR),
	FLAG_CONSTANT(NAMEDOOR),
	FLAG_CONSTANT(LEVELDOOR),
	FLAG_CONSTANT(QUESTDOOR),
	FLAG_CONSTANT(BED),
	FLAG_CONSTANT(FOOD),
	FLAG_CONSTANT(RUNE),
	FLAG_CONSTANT(INFORMATION),
	FLAG_CONSTANT(TEXT),
	FLAG_CONSTANT(WRITE),
	FLAG_CONSTANT(WRITEONCE),
	FLAG_CONSTANT(LIQUIDCONTAINER),
	FLAG_CONSTANT(LIQUIDSOURCE),
	FLAG_CONSTANT(LIQUIDPOOL),
	FLAG_CONSTANT(TELEPORTABSOLUTE),
	FLAG_CONSTANT(TELEPORTRELATIVE),
	FLAG_CONSTANT(UNPASS),
	FLAG_CONSTANT(UNMOVE),
	FLAG_CONSTANT(UNTHROW),
	FLAG_CONSTANT(UNLAY),
	FLAG_CONSTANT(AVOID),
	FLAG_CONSTANT(MAGICFIELD),
	FLAG_CONSTANT(RESTRICTLEVEL),
	FLAG_CONSTANT(RESTRICTPROFESSION),
	FLAG_CONSTANT(TAKE),
	FLAG_CONSTANT(HANG),
	FLAG_CONSTANT(HOOKSOUTH),
	FLAG_CONSTANT(HOOKEAST),
	FLAG_CONSTANT(ROTATE),
	FLAG_CONSTANT(DESTROY),
	FLAG_CONSTANT(CLOTHES),
	FLAG_CONSTANT(SKILLBOOST),
	FLAG_CONSTANT(PROTECTION),
	FLAG_CONSTANT(LIGHT),
	FLAG_CONSTANT(ROPESPOT),
	FLAG_CONSTANT(CORPSE),
	FLAG_CONSTANT(EXPIRE),
	FLAG_CONSTANT(EXPIRESTOP),
	FLAG_CONSTANT(WEAROUT),
	FLAG_CONSTANT(WEAPON),
	FLAG_CONSTANT(SHIELD),
	FLAG_CONSTANT(BOW),
	FLAG_CONSTANT(THROW),
	FLAG_CONSTANT(WAND),
	FLAG_CONSTANT(AMMO),
	FLAG_CONSTANT(ARMOR),
	FLAG_CONSTANT(HEIGHT),
	FLAG_CONSTANT(DISGUISE),
	FLAG_CONSTANT(SHOWDETAIL),
	FLAG_CONSTANT(SPECIALOBJECT),
};
#undef FLAG_CONSTANT

#define TYPE_ATTRIBUTE_CONSTANT(Name) {"ITEMTYPE_ATTRIBUTE_" #Name, Name}
static constexpr TTriggerConstant TypeAttributeConstants[] = {
	TYPE_ATTRIBUTE_CONSTANT(WAYPOINTS),
	TYPE_ATTRIBUTE_CONSTANT(CAPACITY),
	TYPE_ATTRIBUTE_CONSTANT(CHANGETARGET),
	TYPE_ATTRIBUTE_CONSTANT(KEYDOORTARGET),
	TYPE_ATTRIBUTE_CONSTANT(NAMEDOORTARGET),
	TYPE_ATTRIBUTE_CONSTANT(LEVELDOORTARGET),
	TYPE_ATTRIBUTE_CONSTANT(QUESTDOORTARGET),
	TYPE_ATTRIBUTE_CONSTANT(NUTRITION),
	TYPE_ATTRIBUTE_CONSTANT(INFORMATIONTYPE),
	TYPE_ATTRIBUTE_CONSTANT(FONTSIZE),
	TYPE_ATTRIBUTE_CONSTANT(MAXLENGTH),
	TYPE_ATTRIBUTE_CONSTANT(MAXLENGTHONCE),
	TYPE_ATTRIBUTE_CONSTANT(SOURCELIQUIDTYPE),
	TYPE_ATTRIBUTE_CONSTANT(ABSTELEPORTEFFECT),
	TYPE_ATTRIBUTE_CONSTANT(RELTELEPORTDISPLACEMENT),
	TYPE_ATTRIBUTE_CONSTANT(RELTELEPORTEFFECT),
	TYPE_ATTRIBUTE_CONSTANT(AVOIDDAMAGETYPES),
	TYPE_ATTRIBUTE_CONSTANT(MINIMUMLEVEL),
	TYPE_ATTRIBUTE_CONSTANT(PROFESSIONS),
	TYPE_ATTRIBUTE_CONSTANT(WEIGHT),
	TYPE_ATTRIBUTE_CONSTANT(ROTATETARGET),
	TYPE_ATTRIBUTE_CONSTANT(DESTROYTARGET),
	TYPE_ATTRIBUTE_CONSTANT(BODYPOSITION),
	TYPE_ATTRIBUTE_CONSTANT(SKILLNUMBER),
	TYPE_ATTRIBUTE_CONSTANT(SKILLMODIFICATION),
	TYPE_ATTRIBUTE_CONSTANT(PROTECTIONDAMAGETYPES),
	TYPE_ATTRIBUTE_CONSTANT(DAMAGEREDUCTION),
	TYPE_ATTRIBUTE_CONSTANT(BRIGHTNESS),
	TYPE_ATTRIBUTE_CONSTANT(LIGHTCOLOR),
	TYPE_ATTRIBUTE_CONSTANT(CORPSETYPE),
	TYPE_ATTRIBUTE_CONSTANT(TOTALEXPIRETIME),
	TYPE_ATTRIBUTE_CONSTANT(EXPIRETARGET),
	TYPE_ATTRIBUTE_CONSTANT(TOTALUSES),
	TYPE_ATTRIBUTE_CONSTANT(WEAROUTTARGET),
	TYPE_ATTRIBUTE_CONSTANT(WEAPONTYPE),
	TYPE_ATTRIBUTE_CONSTANT(WEAPONATTACKVALUE),
	TYPE_ATTRIBUTE_CONSTANT(WEAPONDEFENDVALUE),
	TYPE_ATTRIBUTE_CONSTANT(SHIELDDEFENDVALUE),
	TYPE_ATTRIBUTE_CONSTANT(BOWRANGE),
	TYPE_ATTRIBUTE_CONSTANT(BOWAMMOTYPE),
	TYPE_ATTRIBUTE_CONSTANT(THROWRANGE),
	TYPE_ATTRIBUTE_CONSTANT(THROWATTACKVALUE),
	TYPE_ATTRIBUTE_CONSTANT(THROWDEFENDVALUE),
	TYPE_ATTRIBUTE_CONSTANT(THROWMISSILE),
	TYPE_ATTRIBUTE_CONSTANT(THROWSPECIALEFFECT),
	TYPE_ATTRIBUTE_CONSTANT(THROWEFFECTSTRENGTH),
	TYPE_ATTRIBUTE_CONSTANT(THROWFRAGILITY),
	TYPE_ATTRIBUTE_CONSTANT(WANDRANGE),
	TYPE_ATTRIBUTE_CONSTANT(WANDMANACONSUMPTION),
	TYPE_ATTRIBUTE_CONSTANT(WANDATTACKSTRENGTH),
	TYPE_ATTRIBUTE_CONSTANT(WANDATTACKVARIATION),
	TYPE_ATTRIBUTE_CONSTANT(WANDDAMAGETYPE),
	TYPE_ATTRIBUTE_CONSTANT(WANDMISSILE),
	TYPE_ATTRIBUTE_CONSTANT(AMMOTYPE),
	TYPE_ATTRIBUTE_CONSTANT(AMMOATTACKVALUE),
	TYPE_ATTRIBUTE_CONSTANT(AMMOMISSILE),
	TYPE_ATTRIBUTE_CONSTANT(AMMOSPECIALEFFECT),
	TYPE_ATTRIBUTE_CONSTANT(AMMOEFFECTSTRENGTH),
	TYPE_ATTRIBUTE_CONSTANT(ARMORVALUE),
	TYPE_ATTRIBUTE_CONSTANT(ELEVATION),
	TYPE_ATTRIBUTE_CONSTANT(DISGUISETARGET),
	TYPE_ATTRIBUTE_CONSTANT(MEANING),
};
#undef TYPE_ATTRIBUTE_CONSTANT

#define INSTANCE_ATTRIBUTE_CONSTANT(Name) {"ITEM_ATTRIBUTE_" #Name, Name}
static constexpr TTriggerConstant InstanceAttributeConstants[] = {
	INSTANCE_ATTRIBUTE_CONSTANT(CONTENT),
	INSTANCE_ATTRIBUTE_CONSTANT(CHESTQUESTNUMBER),
	INSTANCE_ATTRIBUTE_CONSTANT(AMOUNT),
	INSTANCE_ATTRIBUTE_CONSTANT(KEYNUMBER),
	INSTANCE_ATTRIBUTE_CONSTANT(KEYHOLENUMBER),
	INSTANCE_ATTRIBUTE_CONSTANT(DOORLEVEL),
	INSTANCE_ATTRIBUTE_CONSTANT(DOORQUESTNUMBER),
	INSTANCE_ATTRIBUTE_CONSTANT(DOORQUESTVALUE),
	INSTANCE_ATTRIBUTE_CONSTANT(CHARGES),
	INSTANCE_ATTRIBUTE_CONSTANT(TEXTSTRING),
	INSTANCE_ATTRIBUTE_CONSTANT(EDITOR),
	INSTANCE_ATTRIBUTE_CONSTANT(CONTAINERLIQUIDTYPE),
	INSTANCE_ATTRIBUTE_CONSTANT(POOLLIQUIDTYPE),
	INSTANCE_ATTRIBUTE_CONSTANT(ABSTELEPORTDESTINATION),
	INSTANCE_ATTRIBUTE_CONSTANT(RESPONSIBLE),
	INSTANCE_ATTRIBUTE_CONSTANT(REMAININGEXPIRETIME),
	INSTANCE_ATTRIBUTE_CONSTANT(SAVEDEXPIRETIME),
	INSTANCE_ATTRIBUTE_CONSTANT(REMAININGUSES),
};
#undef INSTANCE_ATTRIBUTE_CONSTANT

// Whether the Count entries of List have the values 0 to Count - 1 in order.
template<size_t Count>
static constexpr bool NumberedInOrder(const TTriggerConstant (&List)[Count]) {
	for (size_t i = 0; i < Count; i++) {
		if (List[i].Value != static_cast<int>(i)) {
			return false;
		}
	}
	return true;
}

static_assert(sizeof(FlagConstants) / sizeof(FlagConstants[0]) == FLAGS, "FlagConstants must name every FLAG");
static_assert(NumberedInOrder(FlagConstants), "FlagConstants must be in the order of the numbers of FLAG");
static_assert(sizeof(TypeAttributeConstants) / sizeof(TypeAttributeConstants[0]) == TYPEATTRIBUTES,
		"TypeAttributeConstants must name every TYPEATTRIBUTE");
static_assert(NumberedInOrder(TypeAttributeConstants),
		"TypeAttributeConstants must be in the order of the numbers of TYPEATTRIBUTE");
static_assert(sizeof(InstanceAttributeConstants) / sizeof(InstanceAttributeConstants[0]) == INSTANCEATTRIBUTES,
		"InstanceAttributeConstants must name every INSTANCEATTRIBUTE");
static_assert(NumberedInOrder(InstanceAttributeConstants),
		"InstanceAttributeConstants must be in the order of the numbers of INSTANCEATTRIBUTE");
// The counts are the enums' last values plus one.
static_assert(SPECIALOBJECT == FLAGS - 1, "FLAGS must follow the last FLAG");
static_assert(MEANING == TYPEATTRIBUTES - 1, "TYPEATTRIBUTES must follow the last TYPEATTRIBUTE");
static_assert(REMAININGUSES == INSTANCEATTRIBUTES - 1, "INSTANCEATTRIBUTES must follow the last INSTANCEATTRIBUTE");

void RegisterTriggerAPI() {
	sol::table Item = ScriptAPITable("Item");
	Item.set_function("getId", Guarded(&ItemGetId));
	Item.set_function("isCreature", Guarded(&ItemIsCreature));
	Item.set_function("getCreature", Guarded(&ItemGetCreature));
	Item.set_function("getCreatureId", Guarded(&ItemGetCreatureId));
	Item.set_function("hasFlag", Guarded(&ItemHasFlag));
	Item.set_function("getTypeAttribute", Guarded(&ItemGetTypeAttribute));
	Item.set_function("getAttribute", Guarded(&ItemGetAttribute));
	Item.set_function("setAttribute", Guarded(&ItemSetAttribute));
	Item.set_function("getPosition", Guarded(&ItemGetPosition));
	Item.set_function("getContainer", Guarded(&ItemGetContainer));
	Item.set_function("getMapContainer", Guarded(&ItemGetMapContainer));
	Item.set_function("getNextObject", Guarded(&ItemGetNextObject));
	Item.set_function("getFirstObject", Guarded(&ItemGetFirstObject));
	Item.set_function("countObjects", Guarded(&ItemCountObjects));
	Item.set_function("getBodyPosition", Guarded(&ItemGetBodyPosition));
	Item.set_function("getDescription", Guarded(&ItemGetDescription));
	Item.set_function("getCronInfo", Guarded(&ItemGetCronInfo));
	Item.set_function("sendMagicEffect", Guarded(&ItemSendMagicEffect));

	sol::table Creature = ScriptAPITable("Creature");
	Creature.set_function("isPlayer", Guarded(&CreatureIsPlayer));
	Creature.set_function("isPeaceful", Guarded(&CreatureIsPeaceful));
	Creature.set_function("getName", Guarded(&CreatureGetName));
	Creature.set_function("probeSkill", Guarded(&CreatureProbeSkill));
	Creature.set_function("damage", Guarded(&CreatureDamage));
	Creature.set_function("startLogout", Guarded(&CreatureStartLogout));

	sol::table Player = ScriptAPITable("Player");
	Player.set_function("canLogout", Guarded(&PlayerCanLogout));
	Player.set_function("hasRight", Guarded(&PlayerHasRight));
	Player.set_function("getStorageValue", Guarded(&PlayerGetStorageValue));
	Player.set_function("setStorageValue", Guarded(&PlayerSetStorageValue));
	Player.set_function("isHouseOwner", Guarded(&PlayerIsHouseOwner));
	Player.set_function("setStartPosition", Guarded(&PlayerSetStartPosition));
	Player.set_function("sendTextMessage", Guarded(&PlayerSendTextMessage));

	sol::table Game = ScriptAPITable("Game");
	Game.set_function("getCreature", Guarded(&GameGetCreature));
	Game.set_function("getMapContainer", Guarded(&GameGetMapContainer));
	Game.set_function("getFirstObject", Guarded(&GameGetFirstObject));
	Game.set_function("getFirstObjectOfType", Guarded(&GameGetFirstObjectOfType));
	Game.set_function("getTopObject", Guarded(&GameGetTopObject));
	Game.set_function("getInventoryObject", Guarded(&GameGetInventoryObject));
	Game.set_function("isProtectionZone", Guarded(&GameIsProtectionZone));
	Game.set_function("isHouse", Guarded(&GameIsHouse));
	Game.set_function("getHouseId", Guarded(&GameGetHouseId));
	Game.set_function("createObject", Guarded(&GameCreateObject));
	Game.set_function("changeObject", Guarded(&GameChangeObject));
	Game.set_function("createMonsterOfRace", Guarded(&GameCreateMonsterOfRace));
	Game.set_function("textEffect", Guarded(&GameTextEffect));
	Game.set_function("getDynamicString", Guarded(&GameGetDynamicString));
	Game.set_function("addDynamicString", Guarded(&GameAddDynamicString));
	Game.set_function("deleteDynamicString", Guarded(&GameDeleteDynamicString));
	Game.set_function("moveObject", Guarded(&GameMoveObject));
	Game.set_function("moveAllObjects", Guarded(&GameMoveAllObjects));
	Game.set_function("deleteAllObjects", Guarded(&GameDeleteAllObjects));
	Game.set_function("deleteObject", Guarded(&GameDeleteObject));
	Game.set_function("deleteAtCreature", Guarded(&GameDeleteAtCreature));
	Game.set_function("loadDepotBox", Guarded(&GameLoadDepotBox));
	Game.set_function("saveDepotBox", Guarded(&GameSaveDepotBox));
	Game.set_function("sendMail", Guarded(&GameSendMail));

	for (const TTriggerConstant &Constant : Constants) {
		SetScriptConstant(Constant.Name, Constant.Value);
	}
	for (const TTriggerConstant &Constant : FlagConstants) {
		SetScriptConstant(Constant.Name, Constant.Value);
	}
	for (const TTriggerConstant &Constant : TypeAttributeConstants) {
		SetScriptConstant(Constant.Name, Constant.Value);
	}
	for (const TTriggerConstant &Constant : InstanceAttributeConstants) {
		SetScriptConstant(Constant.Name, Constant.Value);
	}
}
