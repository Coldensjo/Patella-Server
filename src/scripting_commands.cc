// scripting_commands.cc: the API of the commands' scripts (scripting.hh,
// "Commands in Lua"): the methods of Creature, Player, House and Position, the
// Game, ItemType and Magic functions the scripts of data/scripts/commands/
// (and their module lib/commands.lua) call where the command functions of
// the original magic.cc (the faithful branch) called the C++ functions, and
// the constants (MESSAGE_STATUS_SMALL, MESSAGE_EVENT_ADVANCE, CONST_SLOT_,
// DIRECTION_, CREATURE_, COMMAND_RIGHT, MOVE_RIGHT_, GUEST_LIST,
// SUBOWNER_LIST, MAX_AMOUNT, MAX_RACES). RegisterCommandAPI
// binds them; RegisterMagicAPI calls it. The class House (its objects, House(id))
// and Position's method table are the engine's (scripting.cc); Game.atoi is
// the core API's.
//
// What every binding here does alike (as in scripting_npcs.cc and
// scripting_items.cc):
//  - A Creature or Player self is resolved on every call; a method that
//    dereferences it logs "<Lua name>: Creature does not exist." for a
//    creature that is gone and raises ERROR ("nullptr: checked"); a Player
//    method on a creature that is no player throws a const char * (binding's
//    check). Where the C++ function takes the creature ID and checks it itself,
//    the binding passes the ID on unchecked ("ID: passed on"); where it takes
//    a TPlayer * and checks it for nullptr itself, the binding passes the
//    player GetPlayer gives, nullptr for one that is gone ("nullptr: passed
//    on").
//  - An Item parameter is an Item object or nil; the binding passes its
//    handle on as Object(handle), nil as NONE, unchecked. An Item result is
//    nil for NONE.
//  - A number the C++ function takes as uint32_t is converted from the Lua
//    number modulo 2^32, as the C++ code converted the int it held; an
//    ObjectType parameter is ObjectType(Type) of the number, as the C++ call
//    converted the int.
//  - A string passed on as a char * is a copy (the whole string up to its
//    first zero byte), so a C++ function that changes it (IdentifyPlayer cuts
//    a '~') never touches Lua's string.
//  - The binding's own checks (a command, check, skill, sex, list or radius
//    out of range, a house number that is no unsigned short) guard what the
//    C++ code never got from its constant arguments and would read out of
//    bounds or wrap; they throw a const char *, which Guarded turns into a
//    logged Lua error and ERROR.
//  - What the C++ function throws (a RESULT) is raised as the result code
//    (Guarded).

#include <cstdint>
#include <string>
#include <tuple>
#include <vector>

#include "objects.hh"
#include "map.hh"
#include "info.hh"
#include "cr.hh"
#include "crskill.hh"
#include "crmain.hh"
#include "crplayer.hh"
#include "houses.hh"
#include "magic.hh"
#include "magicdata.hh"
#include "operate.hh"
#include "sending.hh"
#include "time.hh"
#include "utils.hh"
#include "scripting_api.hh"

// The lists of player:sendHouseWindow, by the numbers of the edit lists
// SendEditList sends for them (sending.hh).
const int GUEST_LIST = EDIT_LIST_HOUSE_GUESTS;
const int SUBOWNER_LIST = EDIT_LIST_HOUSE_SUBOWNERS;

// The size of the text buffers the house dialogs were written into
// (EditGuests, EditSubowners, EditNameDoor of the original magic.cc: char
// Buffer[4096]).
const int HOUSE_DIALOG_BUFFER = 4096;

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

// The skill SkillNr of the creature: 0 to MAX_SKILLS - 1 and present
// (binding's check, which throws Message).
static TSkill *ExistingSkill(TCreature *cr, int SkillNr, const char *Message) {
	if (SkillNr < 0 || SkillNr >= MAX_SKILLS || cr->Skills[SkillNr] == nullptr) {
		throw Message;
	}
	return cr->Skills[SkillNr];
}

// A writable copy of a string for a C++ function that takes a char *.
static std::string Copy(const std::string &Text) {
	return std::string(Text.c_str());
}

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

// ---------------------------------------------------------------------------
// Creature, Player

// creature:getDirection() -> Direction, the way the creature faces
// (DIRECTION_ constants). nullptr: checked.
static int CreatureGetDirection(TScriptCreature Self) {
	return ExistingCreature(Self.ID, "Creature.getDirection")->Direction;
}

// creature:isMonster() -> Type == MONSTER. nullptr: checked.
static bool CreatureIsMonster(TScriptCreature Self) {
	return ExistingCreature(Self.ID, "Creature.isMonster")->Type == MONSTER;
}

// creature:getMapContainer() -> GetMapContainer(GetCreatureObject()) of
// map.cc, the map field the creature stands on, nil for NONE (the object form
// of GetMapContainer: it stamps no sector's TimeStamp). nullptr: checked.
// Throws nothing else (GetMapContainer logs an object that is gone).
static sol::optional<TScriptItem> CreatureGetMapContainer(TScriptCreature Self) {
	TCreature *cr = ExistingCreature(Self.ID, "Creature.getMapContainer");
	return ItemResult(GetMapContainer(cr->GetCreatureObject()));
}

// creature:setSkillMagicDelta(skill, value) -> Skills[skill]->SetMDAct(Value),
// the skill's magic change (a gamemaster's speed: SKILL_GOSTRENGTH). nullptr:
// checked; skill out of range or missing: binding's check. Throws nothing
// else.
static void CreatureSetSkillMagicDelta(TScriptCreature Self, int SkillNr, int Value) {
	TCreature *cr = ExistingCreature(Self.ID, "Creature.setSkillMagicDelta");
	ExistingSkill(cr, SkillNr, "Creature.setSkillMagicDelta: no such skill")->SetMDAct(Value);
}

// player:hasCommandRight(command[, check]) -> CheckCommandRight(ID, Command,
// Check) of magic.cc: whether the player has the right commands.toml gives
// the command's check (COMMAND_RIGHT when check is left out; MOVE_RIGHT_ for
// the Move command); a check without a right is logged ("CheckCommandRight:
// Command <n> has no right for check <c>.") and false. ID: passed on
// (CheckRight logs a player that is gone and gives false). command 0 to
// MAX_SPELLS - 1, check 0 to MAX_COMMAND_RIGHTS - 1 (binding's check).
static bool PlayerHasCommandRightCheck(TScriptPlayer Self, int Command, int Check) {
	if (Command < 0 || Command >= MAX_SPELLS) {
		throw "Player.hasCommandRight: no such command";
	}
	if (Check < 0 || Check >= MAX_COMMAND_RIGHTS) {
		throw "Player.hasCommandRight: no such right check";
	}
	return CheckCommandRight(Self.ID, Command, Check);
}

// player:hasCommandRight(command): the same with COMMAND_RIGHT.
static bool PlayerHasCommandRight(TScriptPlayer Self, int Command) {
	return PlayerHasCommandRightCheck(Self, Command, COMMAND_RIGHT);
}

// player:getStartPosition() -> startx, starty, startz as a Position, the
// player's temple. nullptr: checked; no player: binding's check.
static TScriptPosition PlayerGetStartPosition(TScriptPlayer Self) {
	TPlayer *pl = ExistingPlayer(Self.ID, "Player.getStartPosition",
			"Player.getStartPosition: the creature is not a player");
	return TScriptPosition{pl->startx, pl->starty, pl->startz};
}

// player:setSex(sex) -> Sex = sex, sex PLAYERSEX_MALE or PLAYERSEX_FEMALE
// (binding's check: the C++ code set only those). nullptr: checked; no
// player: binding's check. Throws nothing else.
static void PlayerSetSex(TScriptPlayer Self, int Sex) {
	TPlayer *pl = ExistingPlayer(Self.ID, "Player.setSex", "Player.setSex: the creature is not a player");
	if (Sex != SEX_MALE && Sex != SEX_FEMALE) {
		throw "Player.setSex: sex must be PLAYERSEX_MALE or PLAYERSEX_FEMALE";
	}
	pl->Sex = Sex;
}

// player:clearVocation() -> ClearProfession(): the player has no vocation
// (nothing for one that has none). nullptr: checked; no player: binding's
// check. Throws nothing else.
static void PlayerClearVocation(TScriptPlayer Self) {
	ExistingPlayer(Self.ID, "Player.clearVocation", "Player.clearVocation: the creature is not a player")
			->ClearProfession();
}

// player:addSkillTries(skill, count) -> Skills[skill]->Increase(Count): the
// skill's experience rises by count (SKILL_LEVEL: experience points; the
// fighting skills, magic level and fishing: tries; the other skills do
// nothing), with the advances it brings. nullptr: checked; no player, skill
// out of range or missing: binding's check. Throws what Increase throws.
static void PlayerAddSkillTries(TScriptPlayer Self, int SkillNr, int Count) {
	TPlayer *pl = ExistingPlayer(Self.ID, "Player.addSkillTries", "Player.addSkillTries: the creature is not a player");
	ExistingSkill(pl, SkillNr, "Player.addSkillTries: no such skill")->Increase(Count);
}

// player:sendHouseWindow(house, listId) -> the list dialog of the house,
// what EditGuests and EditSubowners of the original magic.cc did after
// finding the house:
// for GUEST_LIST ShowGuestList(HouseID, pl, Buffer), for SUBOWNER_LIST
// ShowSubownerList(HouseID, pl, Buffer) of houses.cc (each checks the house,
// the player's right to the list and logs the edit at level 3), then
// SendEditList(GetConnection(), EDIT_LIST_HOUSE_GUESTS or
// EDIT_LIST_HOUSE_SUBOWNERS, HouseID, Buffer) of sending.cc; Buffer as large
// as the C++ code's. nullptr: checked; no player, another listId: binding's
// check. Throws what ShowGuestList and ShowSubownerList throw (ERROR for a
// house that does not exist, NOTACCESSIBLE without the right).
static void PlayerSendHouseWindow(TScriptPlayer Self, TScriptHouse House, int ListId) {
	TPlayer *pl = ExistingPlayer(Self.ID, "Player.sendHouseWindow",
			"Player.sendHouseWindow: the creature is not a player");
	if (ListId != GUEST_LIST && ListId != SUBOWNER_LIST) {
		throw "Player.sendHouseWindow: listId must be GUEST_LIST or SUBOWNER_LIST";
	}
	unsigned short HouseID = static_cast<unsigned short>(House.ID);
	char Buffer[HOUSE_DIALOG_BUFFER];
	if (ListId == GUEST_LIST) {
		ShowGuestList(HouseID, pl, Buffer);
		SendEditList(pl->GetConnection(), EDIT_LIST_HOUSE_GUESTS, HouseID, Buffer);
	} else {
		ShowSubownerList(HouseID, pl, Buffer);
		SendEditList(pl->GetConnection(), EDIT_LIST_HOUSE_SUBOWNERS, HouseID, Buffer);
	}
}

// player:sendNameDoorWindow(door) -> the list dialog of the name door, what
// EditNameDoor of the original magic.cc did after finding the door:
// ShowNameDoor(Door, pl,
// Buffer) of houses.cc (it checks the door, its house and that the player is
// the tenant, and logs at level 3), then SendEditList(GetConnection(),
// EDIT_LIST_NAME_DOOR, Door.getObjectID(), Buffer); Buffer as large as the
// C++ code's; door nil is NONE (ShowNameDoor logs and throws ERROR).
// nullptr: checked; no player: binding's check. Throws what ShowNameDoor
// throws.
static void PlayerSendNameDoorWindow(TScriptPlayer Self, sol::optional<TScriptItem> Door) {
	TPlayer *pl = ExistingPlayer(Self.ID, "Player.sendNameDoorWindow",
			"Player.sendNameDoorWindow: the creature is not a player");
	Object DoorObject = ItemObject(Door);
	char Buffer[HOUSE_DIALOG_BUFFER];
	ShowNameDoor(DoorObject, pl, Buffer);
	SendEditList(pl->GetConnection(), EDIT_LIST_NAME_DOOR, DoorObject.getObjectID(), Buffer);
}

// ---------------------------------------------------------------------------
// House

// house:getId() -> the house number the object holds (1 to 65535). Throws
// nothing.
static int HouseGetId(TScriptHouse Self) {
	return Self.ID;
}

// house:kickPlayer(player, target) -> KickGuest(HouseID, Host, Guest) of
// houses.cc: player (the host) puts target out of the house (its checks: the
// house exists, target is in it, player may kick and target may be kicked,
// or kicks itself; then the move to the exit with its effects). nullptr:
// passed on (a player that is gone is nullptr, which KickGuest logs and
// throws ERROR for, after its house check). Throws what KickGuest throws.
static void HouseKickPlayer(TScriptHouse Self, TScriptPlayer Host, TScriptPlayer Guest) {
	KickGuest(static_cast<unsigned short>(Self.ID), GetPlayer(Host.ID), GetPlayer(Guest.ID));
}

// ---------------------------------------------------------------------------
// Position

// position:sendMagicEffect(effect) -> GraphicalEffect(x, y, z, Effect) of
// operate.cc (CONST_ME_ constants): the effect on the field, for the players
// who see it. Throws what GraphicalEffect throws.
static void PositionSendMagicEffect(TScriptPosition Self, int Effect) {
	GraphicalEffect(Self.x, Self.y, Self.z, Effect);
}

// ---------------------------------------------------------------------------
// Game

// Game.identifyPlayer(name, exactMatch, ignoreGamemasters) -> result, player:
// IdentifyPlayer(Name, ExactMatch, IgnoreGamemasters, pl) of crplayer.cc on a
// copy of name (IdentifyPlayer cuts a '~' at its end; without exactMatch the
// '~' asks for a match of the name's beginning, and a name without one is
// matched exactly). result 0 with the Player found, -1 (no such player
// online; also for an empty name, logged) or -2 (the beginning matches more
// than one) with nil. Throws nothing.
static std::tuple<int, sol::optional<TScriptPlayer>> GameIdentifyPlayer(std::string Name, bool ExactMatch,
		bool IgnoreGamemasters) {
	std::string Buffer = Copy(Name);
	TPlayer *pl = nullptr;
	int Result = IdentifyPlayer(&Buffer[0], ExactMatch, IgnoreGamemasters, pl);
	if (Result != 0 || pl == nullptr) {
		return std::make_tuple(Result, sol::optional<TScriptPlayer>());
	}
	return std::make_tuple(Result, sol::optional<TScriptPlayer>(TScriptPlayer{pl->ID}));
}

// Game.getPlayers() -> a new array of the Players online, in the order of
// PlayerList (crplayer.cc; the order they joined, but that one leaving puts
// the last in its place), gamemasters with NO_STATISTICS too. Throws nothing.
static sol::as_table_t<std::vector<TScriptPlayer>> GameGetPlayers() {
	std::vector<TScriptPlayer> Players;
	for (int i = 0; i < GetNumberOfPlayers(); i++) {
		Players.push_back(TScriptPlayer{PlayerList(i)->ID});
	}
	return sol::as_table(std::move(Players));
}

// Game.searchFreeField(position, distance, houseId, jump) -> the field
// SearchFreeField(x, y, z, Distance, HouseID, Jump) of info.cc finds from
// position on (in its spiral, at most distance away; on the floor of
// position): a Position, nil when it finds none (the C++ code's x, y, z stay
// position's then). houseId 0 to 65535 (an unsigned short; 0xFFFF: any
// field, 0: no house field; binding's check). Throws nothing else.
static sol::optional<TScriptPosition> GameSearchFreeField(TScriptPosition Position, int Distance, int HouseID,
		bool Jump) {
	if (HouseID < 0 || HouseID > 0xFFFF) {
		throw "Game.searchFreeField: houseId must be 0 to 65535";
	}
	int x = Position.x;
	int y = Position.y;
	int z = Position.z;
	if (!SearchFreeField(x, y, z, Distance, static_cast<unsigned short>(HouseID), Jump)) {
		return sol::nullopt;
	}
	return TScriptPosition{x, y, z};
}

// Game.searchSummonField(position, distance) -> the field
// SearchSummonField(x, y, z, Distance) of info.cc chooses around position (a
// random(0, 99) per field of the square, as it draws them): a Position, nil
// when it finds none. Throws nothing.
static sol::optional<TScriptPosition> GameSearchSummonField(TScriptPosition Position, int Distance) {
	int x = Position.x;
	int y = Position.y;
	int z = Position.z;
	if (!SearchSummonField(x, y, z, Distance)) {
		return sol::nullopt;
	}
	return TScriptPosition{x, y, z};
}

// Game.isOnMap(position) -> IsOnMap(x, y, z) of map.cc: whether the field is
// in a sector of the map's range. Throws nothing.
static bool GameIsOnMap(TScriptPosition Position) {
	return IsOnMap(Position.x, Position.y, Position.z);
}

// Game.getMarkPosition(name) -> the field of the map's mark of that name
// (GetMarkPosition(Name, x, y, z) of map.cc, Name a copy, compared without
// case) as a Position, nil for none. Throws nothing.
static sol::optional<TScriptPosition> GameGetMarkPosition(std::string Name) {
	std::string Buffer = Copy(Name);
	int x = 0;
	int y = 0;
	int z = 0;
	if (!GetMarkPosition(&Buffer[0], x, y, z)) {
		return sol::nullopt;
	}
	return TScriptPosition{x, y, z};
}

// Game.startRaid(name) -> QueueMonsterRaid(Name, (int)RoundNr) of crmain.cc,
// Name a copy: queues the waves of the raid whose key is name, from this
// round on (with the raid's onRaid hook); false for no raid of that key.
// Throws nothing.
static bool GameStartRaid(std::string Name) {
	std::string Buffer = Copy(Name);
	return QueueMonsterRaid(Buffer.c_str(), static_cast<int>(RoundNr));
}

// Game.createAtCreature(creatureId, type, value) -> CreateAtCreature(ID,
// ObjectType(Type), Value) of operate.cc: the new object in the creature's
// hands or inventory, or on its field when it cannot carry it (nil for NONE).
// ID: passed on (CreateAtCreature logs a creature that is gone and throws
// ERROR). Throws what CreateAtCreature throws.
static sol::optional<TScriptItem> GameCreateAtCreature(uint32_t CreatureID, int Type, uint32_t Value) {
	return ItemResult(CreateAtCreature(CreatureID, ObjectType(Type), Value));
}

// Game.announceChangedCreature(creatureId, what) ->
// AnnounceChangedCreature(ID, What) of operate.cc: tells the players who see
// the creature what changed (CREATURE_ constants). ID: passed on
// (AnnounceChangedCreature logs a creature that is gone). Throws nothing.
static void GameAnnounceChangedCreature(uint32_t CreatureID, int What) {
	AnnounceChangedCreature(CreatureID, What);
}

// Game.isThrowPossible(from, to, power) -> ThrowPossible(from.x, from.y,
// from.z, to.x, to.y, to.z, Power) of info.cc: whether something thrown from
// from with that power reaches to. Throws nothing.
static bool GameIsThrowPossible(TScriptPosition From, TScriptPosition To, int Power) {
	return ThrowPossible(From.x, From.y, From.z, To.x, To.y, To.z, Power);
}

// ---------------------------------------------------------------------------
// ItemType

// ItemType.fromName(name, movable) -> GetObjectTypeByName(Name,
// Movable).getTypeID() of objects.cc, Name a copy: the number of the type
// whose name is name (without case), else of the first whose name contains it
// as words, 0 for none (movable: only types without the flag UNMOVE). Throws
// nothing.
static int ItemTypeFromName(std::string Name, bool Movable) {
	std::string Buffer = Copy(Name);
	return GetObjectTypeByName(&Buffer[0], Movable).getTypeID();
}

// ItemType.exists(id) -> ObjectTypeExists(id) of objects.cc: whether id is a
// type number of the object types (from the lowest to the highest). Throws
// nothing.
static bool ItemTypeExists(int TypeID) {
	return ObjectTypeExists(TypeID);
}

// ---------------------------------------------------------------------------
// Magic

// Magic.getCircle(radius) -> the offsets of Circle[radius] (circles.toml), a
// new array of Positions (dx, dy, 0) in the circle's order, as the shapes'
// loops "for j < Circle[i].Count" read them. radius 0 to MAX_CIRCLES - 1
// (binding's check). Throws nothing else.
static sol::as_table_t<std::vector<TScriptPosition>> MagicGetCircle(int Radius) {
	if (Radius < 0 || Radius >= MAX_CIRCLES) {
		throw "Magic.getCircle: radius must be 0 to MAX_CIRCLES - 1";
	}
	std::vector<TScriptPosition> Offsets;
	for (int j = 0; j < Circle[Radius].Count; j++) {
		Offsets.push_back(TScriptPosition{Circle[Radius].x[j], Circle[Radius].y[j], 0});
	}
	return sol::as_table(std::move(Offsets));
}

// ---------------------------------------------------------------------------
// The constants

// A constant of the commands' scripts: its name and its value.
struct TCommandConstant {
	const char *Name;
	int Value;
};

static const TCommandConstant Constants[] = {
	// The talk modes of the commands' messages (operate.hh), TFS's names.
	{"MESSAGE_STATUS_SMALL", TALK_FAILURE_MESSAGE},
	{"MESSAGE_EVENT_ADVANCE", TALK_EVENT_MESSAGE},

	// The body positions (objects.hh's BODY_), TFS's names.
	{"CONST_SLOT_HEAD", BODY_HEAD},
	{"CONST_SLOT_NECKLACE", BODY_NECK},
	{"CONST_SLOT_BACKPACK", BODY_CONTAINER},
	{"CONST_SLOT_ARMOR", BODY_TORSO},
	{"CONST_SLOT_RIGHT", BODY_RIGHTHAND},
	{"CONST_SLOT_LEFT", BODY_LEFTHAND},
	{"CONST_SLOT_LEGS", BODY_LEGS},
	{"CONST_SLOT_FEET", BODY_FEET},
	{"CONST_SLOT_RING", BODY_FINGER},
	{"CONST_SLOT_AMMO", BODY_AMMO},

	// The directions a creature faces (cr.hh).
	{"DIRECTION_NORTH", DIRECTION_NORTH},
	{"DIRECTION_EAST", DIRECTION_EAST},
	{"DIRECTION_SOUTH", DIRECTION_SOUTH},
	{"DIRECTION_WEST", DIRECTION_WEST},

	// What changed about a creature (operate.hh, AnnounceChangedCreature).
	{"CREATURE_HEALTH", CREATURE_HEALTH},
	{"CREATURE_LIGHT", CREATURE_LIGHT},
	{"CREATURE_OUTFIT", CREATURE_OUTFIT},
	{"CREATURE_SPEED", CREATURE_SPEED},
	{"CREATURE_SKULL", CREATURE_SKULL},
	{"CREATURE_PARTY", CREATURE_PARTY},

	// The right checks of the commands (magicdata.hh, CheckCommandRight).
	{"COMMAND_RIGHT", COMMAND_RIGHT},
	{"MOVE_RIGHT_VERTICAL", MOVE_RIGHT_VERTICAL},
	{"MOVE_RIGHT_SPEED", MOVE_RIGHT_SPEED},
	{"MOVE_RIGHT_COORDINATE", MOVE_RIGHT_COORDINATE},
	{"MOVE_RIGHT_MARK", MOVE_RIGHT_MARK},

	// The lists of player:sendHouseWindow, TFS's names.
	{"GUEST_LIST", GUEST_LIST},
	{"SUBOWNER_LIST", SUBOWNER_LIST},

	// The most objects of a cumulative type in one pile (objects.hh).
	{"MAX_AMOUNT", MAX_AMOUNT},

	// The number of race numbers (crmain.hh): the races are 1 to MAX_RACES - 1.
	{"MAX_RACES", MAX_RACES},
};

// The values scripting.hh gives them, 7.72's.
static_assert(TALK_FAILURE_MESSAGE == 23 && TALK_EVENT_MESSAGE == 19,
		"MESSAGE_STATUS_SMALL is 23, MESSAGE_EVENT_ADVANCE 19");
static_assert(BODY_HEAD == 1 && BODY_NECK == 2 && BODY_CONTAINER == 3 && BODY_TORSO == 4 && BODY_RIGHTHAND == 5
		&& BODY_LEFTHAND == 6 && BODY_LEGS == 7 && BODY_FEET == 8 && BODY_FINGER == 9 && BODY_AMMO == 10,
		"the CONST_SLOT_ constants are 1 to 10");
static_assert(DIRECTION_NORTH == 0 && DIRECTION_EAST == 1 && DIRECTION_SOUTH == 2 && DIRECTION_WEST == 3,
		"the DIRECTION_ constants are 0 to 3");
static_assert(CREATURE_HEALTH == 1 && CREATURE_PARTY == 6, "the CREATURE_ constants are 1 to 6");
static_assert(COMMAND_RIGHT == 0 && MOVE_RIGHT_VERTICAL == 0 && MOVE_RIGHT_SPEED == 1 && MOVE_RIGHT_COORDINATE == 2
		&& MOVE_RIGHT_MARK == 3, "the right checks are 0 to 3");
static_assert(GUEST_LIST == 1 && SUBOWNER_LIST == 2, "GUEST_LIST is 1, SUBOWNER_LIST 2");
static_assert(MAX_AMOUNT == 100, "MAX_AMOUNT is 100");

void RegisterCommandAPI() {
	sol::table Creature = ScriptAPITable("Creature");
	Creature.set_function("getDirection", Guarded(&CreatureGetDirection));
	Creature.set_function("isMonster", Guarded(&CreatureIsMonster));
	Creature.set_function("getMapContainer", Guarded(&CreatureGetMapContainer));
	Creature.set_function("setSkillMagicDelta", Guarded(&CreatureSetSkillMagicDelta));

	sol::table Player = ScriptAPITable("Player");
	Player.set_function("hasCommandRight",
			sol::overload(Guarded(&PlayerHasCommandRight), Guarded(&PlayerHasCommandRightCheck)));
	Player.set_function("getStartPosition", Guarded(&PlayerGetStartPosition));
	Player.set_function("setSex", Guarded(&PlayerSetSex));
	Player.set_function("clearVocation", Guarded(&PlayerClearVocation));
	Player.set_function("addSkillTries", Guarded(&PlayerAddSkillTries));
	Player.set_function("sendHouseWindow", Guarded(&PlayerSendHouseWindow));
	Player.set_function("sendNameDoorWindow", Guarded(&PlayerSendNameDoorWindow));

	sol::table House = ScriptAPITable("House");
	House.set_function("getId", Guarded(&HouseGetId));
	House.set_function("kickPlayer", Guarded(&HouseKickPlayer));

	sol::table Position = ScriptAPITable("Position");
	Position.set_function("sendMagicEffect", Guarded(&PositionSendMagicEffect));

	sol::table Game = ScriptAPITable("Game");
	Game.set_function("identifyPlayer", Guarded(&GameIdentifyPlayer));
	Game.set_function("getPlayers", Guarded(&GameGetPlayers));
	Game.set_function("searchFreeField", Guarded(&GameSearchFreeField));
	Game.set_function("searchSummonField", Guarded(&GameSearchSummonField));
	Game.set_function("isOnMap", Guarded(&GameIsOnMap));
	Game.set_function("getMarkPosition", Guarded(&GameGetMarkPosition));
	Game.set_function("startRaid", Guarded(&GameStartRaid));
	Game.set_function("createAtCreature", Guarded(&GameCreateAtCreature));
	Game.set_function("announceChangedCreature", Guarded(&GameAnnounceChangedCreature));
	Game.set_function("isThrowPossible", Guarded(&GameIsThrowPossible));

	sol::table ItemType = ScriptAPITable("ItemType");
	ItemType.set_function("fromName", Guarded(&ItemTypeFromName));
	ItemType.set_function("exists", Guarded(&ItemTypeExists));

	sol::table Magic = ScriptAPITable("Magic");
	Magic.set_function("getCircle", Guarded(&MagicGetCircle));

	for (const TCommandConstant &Constant : Constants) {
		SetScriptConstant(Constant.Name, Constant.Value);
	}
}
