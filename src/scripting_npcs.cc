// scripting_npcs.cc: the API of the NPCs' scripts (scripting.hh, "NPCs"): the
// methods of Npc, the creature and player methods and the Game and Magic
// functions lib/npc.lua uses, and the constants (NPC_SITUATION_, NPC_STATE_,
// PLAYERSEX_, WORLD_TYPE_). RegisterNpcAPI binds them; InitAll calls it after
// RegisterItemAPI. TNPC (crnonpl.cc) calls the scripts' handlers.
//
// What every binding here does alike (as in scripting_triggers.cc and
// scripting_items.cc):
//  - An Npc self is resolved with GetCreature on every call; for an ID that is
//    no NPC's (gone, or another creature) the binding logs "<Lua name>: NPC
//    does not exist." and raises ERROR.
//  - A Creature or Player self is resolved on every call; a method that
//    dereferences it logs "<Lua name>: Creature does not exist." for a
//    creature that is gone and raises ERROR ("nullptr: checked"); a Player
//    method on a creature that is no player throws a const char * (binding's
//    check). Where the C++ function takes the creature ID and checks it
//    itself, the binding passes the ID on unchecked ("ID: passed on").
//  - A number the C++ code held as uint32_t (Data, LastTalk, a count's data)
//    is converted from the Lua number modulo 2^32, as the C++ code converted
//    the int it held; an int result of a uint32_t is (int) of it.
//  - A string passed on as a char * is a copy (the whole string), so the C++
//    function never touches Lua's string.
//  - The binding's own checks (a state or a skill the C++ code never got from
//    its constants, a start before the text) throw a const char *, which
//    Guarded turns into a logged Lua error and ERROR.
//  - What the C++ function throws (a RESULT) is raised as the result code
//    (Guarded).

#include <cstdint>
#include <string>

#include "config.hh"
#include "objects.hh"
#include "map.hh"
#include "info.hh"
#include "cr.hh"
#include "crskill.hh"
#include "crmain.hh"
#include "crplayer.hh"
#include "crnonpl.hh"
#include "magic.hh"
#include "operate.hh"
#include "strings.hh"
#include "time.hh"
#include "utils.hh"
#include "writer.hh"
#include "scripting_api.hh"

// The NPC the ID names; for one that is gone or no NPC logs "<Name>: NPC
// does not exist." and throws ERROR.
static TNPC *ExistingNPC(uint32_t ID, const char *Name) {
	TCreature *cr = GetCreature(ID);
	if (cr == nullptr || cr->Type != NPC) {
		error("%s: NPC does not exist.\n", Name);
		throw ERROR;
	}
	return static_cast<TNPC *>(cr);
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
// Npc

// npc:getInterlocutor() -> GetPlayer(Interlocutor) as a Player (GetPlayer
// logs a creature that is no player), nil for nullptr. Throws nothing else.
static sol::optional<TScriptPlayer> NpcGetInterlocutor(TScriptNpc Self) {
	TPlayer *pl = GetPlayer(ExistingNPC(Self.ID, "Npc.getInterlocutor")->Interlocutor);
	if (pl == nullptr) {
		return sol::nullopt;
	}
	return TScriptPlayer{pl->ID};
}

// npc:getTopic() -> Topic. Throws nothing else.
static int NpcGetTopic(TScriptNpc Self) {
	return ExistingNPC(Self.ID, "Npc.getTopic")->Topic;
}

// npc:setTopic(v) -> Topic = v. Throws nothing else.
static void NpcSetTopic(TScriptNpc Self, int Value) {
	ExistingNPC(Self.ID, "Npc.setTopic")->Topic = Value;
}

// npc:getPrice() -> Price. Throws nothing else.
static int NpcGetPrice(TScriptNpc Self) {
	return ExistingNPC(Self.ID, "Npc.getPrice")->Price;
}

// npc:setPrice(v) -> Price = v. Throws nothing else.
static void NpcSetPrice(TScriptNpc Self, int Value) {
	ExistingNPC(Self.ID, "Npc.setPrice")->Price = Value;
}

// npc:getAmount() -> Amount. Throws nothing else.
static int NpcGetAmount(TScriptNpc Self) {
	return ExistingNPC(Self.ID, "Npc.getAmount")->Amount;
}

// npc:setAmount(v) -> Amount = v. Throws nothing else.
static void NpcSetAmount(TScriptNpc Self, int Value) {
	ExistingNPC(Self.ID, "Npc.setAmount")->Amount = Value;
}

// npc:getTypeId() -> TypeID, the object type the rules set. Throws nothing
// else.
static int NpcGetTypeId(TScriptNpc Self) {
	return ExistingNPC(Self.ID, "Npc.getTypeId")->TypeID;
}

// npc:setTypeId(v) -> TypeID = v. Throws nothing else.
static void NpcSetTypeId(TScriptNpc Self, int Value) {
	ExistingNPC(Self.ID, "Npc.setTypeId")->TypeID = Value;
}

// npc:getData() -> (int)Data. Throws nothing else.
static int NpcGetData(TScriptNpc Self) {
	return static_cast<int>(ExistingNPC(Self.ID, "Npc.getData")->Data);
}

// npc:setData(v) -> Data = (uint32_t)v. Throws nothing else.
static void NpcSetData(TScriptNpc Self, int Value) {
	ExistingNPC(Self.ID, "Npc.setData")->Data = static_cast<uint32_t>(Value);
}

// npc:getState() -> State (NPC_STATE_ constants). Throws nothing else.
static int NpcGetState(TScriptNpc Self) {
	return static_cast<int>(ExistingNPC(Self.ID, "Npc.getState")->GetState());
}

// npc:setState(state) -> State = state (ChangeState(state, false), which
// sets it and does nothing else). state SLEEPING to PANIC, the values of
// STATE (binding's check: react set only IDLE and LEAVING). Throws nothing
// else.
static void NpcSetState(TScriptNpc Self, int State) {
	TNPC *npc = ExistingNPC(Self.ID, "Npc.setState");
	if (State < SLEEPING || State > PANIC) {
		throw "Npc.setState: no such state";
	}
	npc->ChangeState(static_cast<STATE>(State), false);
}

// npc:setLastTalk(round) -> LastTalk = (uint32_t)round, the round modulo
// 2^32 (RoundNr + TalkDelay / 1000 was computed in uint32_t). Throws nothing
// else.
static void NpcSetLastTalk(TScriptNpc Self, int Round) {
	ExistingNPC(Self.ID, "Npc.setLastTalk")->LastTalk = static_cast<uint32_t>(Round);
}

// npc:toDoWait(delay) -> ToDoWait(Delay), a wait of Delay milliseconds on the
// NPC's to-do list. Throws what ToDoWait throws.
static void NpcToDoWait(TScriptNpc Self, int Delay) {
	ExistingNPC(Self.ID, "Npc.toDoWait")->ToDoWait(Delay);
}

// npc:toDoSay(text) -> ToDoTalk(TALK_SAY, nullptr, Text, false), Text a
// copy: the NPC says the text when its to-do list gets there. Throws what
// ToDoTalk throws.
static void NpcToDoSay(TScriptNpc Self, std::string Text) {
	TNPC *npc = ExistingNPC(Self.ID, "Npc.toDoSay");
	std::string Buffer = Copy(Text);
	npc->ToDoTalk(TALK_SAY, nullptr, &Buffer[0], false);
}

// npc:toDoChangeState(state) -> ToDoChangeState(State): the state change
// when the to-do list gets there (ChangeNPCState). Throws what
// ToDoChangeState throws.
static void NpcToDoChangeState(TScriptNpc Self, int State) {
	ExistingNPC(Self.ID, "Npc.toDoChangeState")->ToDoChangeState(State);
}

// npc:toDoStart() -> ToDoStart(): starts the to-do list. Throws what
// ToDoStart throws.
static void NpcToDoStart(TScriptNpc Self) {
	ExistingNPC(Self.ID, "Npc.toDoStart")->ToDoStart();
}

// npc:enqueue(player, text) -> Enqueue(player's ID, Text), Text a copy: the
// player waits in the NPC's queue with the greeting text (an empty text is
// queued as AddDynamicString's 0, no string). Throws nothing else.
static void NpcEnqueue(TScriptNpc Self, TScriptPlayer Player, std::string Text) {
	TNPC *npc = ExistingNPC(Self.ID, "Npc.enqueue");
	std::string Buffer = Copy(Text);
	npc->Enqueue(Player.ID, &Buffer[0]);
}

// npc:giveItems(type, amount) -> GiveTo(ObjectType(Type), Amount): creates
// Amount objects of the type (each with Data, or as cumulative piles) at the
// interlocutor and logs it ("npc" log); ObjectType(Type) logs a number that
// is no type, as the C++ call's conversion of the int did. Throws nothing
// else (GiveTo logs a RESULT).
static void NpcGiveItems(TScriptNpc Self, int Type, int Amount) {
	ExistingNPC(Self.ID, "Npc.giveItems")->GiveTo(ObjectType(Type), Amount);
}

// npc:takeItems(type, amount) -> GetFrom(ObjectType(Type), Amount): deletes
// Amount objects of the type (with Data) at the interlocutor and logs it.
// Throws nothing else (GetFrom logs a RESULT).
static void NpcTakeItems(TScriptNpc Self, int Type, int Amount) {
	ExistingNPC(Self.ID, "Npc.takeItems")->GetFrom(ObjectType(Type), Amount);
}

// npc:giveMoney(amount) -> GiveMoney(Amount): the coins of Amount gold to the
// interlocutor. Throws nothing else.
static void NpcGiveMoney(TScriptNpc Self, int Amount) {
	ExistingNPC(Self.ID, "Npc.giveMoney")->GiveMoney(Amount);
}

// npc:takeMoney(amount) -> GetMoney(Amount): Amount gold from the
// interlocutor, with the change. Throws nothing else.
static void NpcTakeMoney(TScriptNpc Self, int Amount) {
	ExistingNPC(Self.ID, "Npc.takeMoney")->GetMoney(Amount);
}

// ---------------------------------------------------------------------------
// Creature, Player

// creature:getPosition() -> posx, posy, posz as a Position. nullptr:
// checked.
static TScriptPosition CreatureGetPosition(TScriptCreature Self) {
	TCreature *cr = ExistingCreature(Self.ID, "Creature.getPosition");
	return TScriptPosition{cr->posx, cr->posy, cr->posz};
}

// creature:sendMagicEffect(effect) -> GraphicalEffect(GetCreatureObject(),
// Effect) of operate.cc (CONST_ME_ constants). nullptr: checked. Throws what
// GraphicalEffect throws.
static void CreatureSendMagicEffect(TScriptCreature Self, int Effect) {
	TCreature *cr = ExistingCreature(Self.ID, "Creature.sendMagicEffect");
	GraphicalEffect(cr->GetCreatureObject(), Effect);
}

// creature:getHealth() -> Skills[SKILL_HITPOINTS]->Get(). nullptr: checked.
static int CreatureGetHealth(TScriptCreature Self) {
	return ExistingCreature(Self.ID, "Creature.getHealth")->Skills[SKILL_HITPOINTS]->Get();
}

// creature:setHealth(value) -> Skills[SKILL_HITPOINTS]->Set(Value). nullptr:
// checked. Throws what Set throws.
static void CreatureSetHealth(TScriptCreature Self, int Value) {
	ExistingCreature(Self.ID, "Creature.setHealth")->Skills[SKILL_HITPOINTS]->Set(Value);
}

// creature:die() -> Death(). nullptr: checked. Throws what Death throws.
static void CreatureDie(TScriptCreature Self) {
	ExistingCreature(Self.ID, "Creature.die")->Death();
}

// creature:setDamageOrigin(skill, origin|nil) -> FireDamageOrigin (skill
// SKILL_BURNING) or PoisonDamageOrigin (SKILL_POISON) = the ID origin holds,
// 0 for nil: who is to blame for the damage of the timer. nullptr (self):
// checked; another skill: binding's check.
static void CreatureSetDamageOrigin(TScriptCreature Self, int SkillNr, sol::optional<TScriptCreature> Origin) {
	TCreature *cr = ExistingCreature(Self.ID, "Creature.setDamageOrigin");
	uint32_t OriginID = Origin.has_value() ? Origin->ID : 0;
	if (SkillNr == SKILL_BURNING) {
		cr->FireDamageOrigin = OriginID;
	} else if (SkillNr == SKILL_POISON) {
		cr->PoisonDamageOrigin = OriginID;
	} else {
		throw "Creature.setDamageOrigin: skill must be SKILL_BURNING or SKILL_POISON";
	}
}

// player:getSex() -> Sex (PLAYERSEX_ constants). nullptr: checked; no
// player: binding's check.
static int PlayerGetSex(TScriptPlayer Self) {
	return ExistingPlayer(Self.ID, "Player.getSex", "Player.getSex: the creature is not a player")->Sex;
}

// player:isPzLocked() -> EarliestProtectionZoneRound > RoundNr: whether the
// player may not enter a protection zone. nullptr: checked; no player:
// binding's check.
static bool PlayerIsPzLocked(TScriptPlayer Self) {
	return ExistingPlayer(Self.ID, "Player.isPzLocked", "Player.isPzLocked: the creature is not a player")
			->EarliestProtectionZoneRound > RoundNr;
}

// player:getItemCount(type, data) -> CountInventoryObjects(ID,
// ObjectType(Type), (uint32_t)Data) of info.cc: the objects of the type
// (with the data, for a type that has some) the player carries. ID: passed on
// (CountInventoryObjects logs a creature that is gone and gives 0).
// ObjectType(Type) logs a number that is no type. Throws nothing.
static int PlayerGetItemCount(TScriptPlayer Self, int Type, int Data) {
	return CountInventoryObjects(Self.ID, ObjectType(Type), static_cast<uint32_t>(Data));
}

// player:getMoney() -> CountInventoryMoney(ID) of info.cc, the gold the
// player carries. ID: passed on (logged and 0 for a creature that is gone).
// Throws nothing.
static int PlayerGetMoney(TScriptPlayer Self) {
	return CountInventoryMoney(Self.ID);
}

// player:hasLearnedSpell(spell) -> SpellKnown(Spell) (a spell number; logged
// and false for one out of range). nullptr: checked; no player: binding's
// check.
static bool PlayerHasLearnedSpell(TScriptPlayer Self, int Spell) {
	return ExistingPlayer(Self.ID, "Player.hasLearnedSpell", "Player.hasLearnedSpell: the creature is not a player")
			->SpellKnown(Spell);
}

// player:learnSpell(spell) -> LearnSpell(Spell) (logged for a number out of
// range or a spell the player knows). nullptr: checked; no player: binding's
// check.
static void PlayerLearnSpell(TScriptPlayer Self, int Spell) {
	ExistingPlayer(Self.ID, "Player.learnSpell", "Player.learnSpell: the creature is not a player")
			->LearnSpell(Spell);
}

// player:setVocation(id) -> SetProfession((unsigned char)ID) (VOCATION_
// constants; VOCATION_PROMOTION promotes). nullptr: checked; no player:
// binding's check. Throws what SetProfession throws.
static void PlayerSetVocation(TScriptPlayer Self, int ID) {
	ExistingPlayer(Self.ID, "Player.setVocation", "Player.setVocation: the creature is not a player")
			->SetProfession(static_cast<unsigned char>(ID));
}

// ---------------------------------------------------------------------------
// Game, Magic

// Where a search of Game.searchWord or Game.searchNumber in Text begins:
// Text from the 1-based Start on (Start #Text + 1: the terminating zero, the
// empty text), or Empty for a start past that, the empty text after the end.
// A start below 1 throws Refused (binding's check). Text and Empty stay the
// caller's.
static char *SearchStart(std::string &Text, int Start, char *Empty, const char *Refused) {
	if (Start < 1) {
		throw Refused;
	}
	if (static_cast<size_t>(Start) - 1 > Text.size()) {
		return Empty;
	}
	return &Text[0] + (Start - 1);
}

// Game.searchWord(word, text, start) -> SearchForWord(Word, Text + start - 1)
// of strings.cc, Word and Text copies: the 1-based index in text of the first
// word from start on that begins with word (word ending in '$': that is the
// whole word), nil for nullptr. start 1 or more (binding's check); a start
// past #text + 1 searches the empty text (react searched the bytes after the
// text's end there; scripting.hh). SearchForWord logs an empty word. Throws
// nothing else.
static sol::optional<int> GameSearchWord(std::string Word, std::string Text, int Start) {
	char Empty[1] = {0};
	char *Begin = SearchStart(Text, Start, Empty, "Game.searchWord: start must be 1 or more");
	std::string Pattern = Copy(Word);
	char *Found = SearchForWord(&Pattern[0], Begin);
	if (Found == nullptr || Begin == Empty) {
		return sol::nullopt;
	}
	return static_cast<int>(Found - &Text[0]) + 1;
}

// Game.searchNumber(n, text, start) -> SearchForNumber(N, Text + start - 1)
// of strings.cc, Text a copy: the 1-based index in text of the n-th number
// from start on, nil for nullptr. start as for Game.searchWord.
// SearchForNumber logs an n below 1. Throws nothing else.
static sol::optional<int> GameSearchNumber(int Count, std::string Text, int Start) {
	char Empty[1] = {0};
	char *Begin = SearchStart(Text, Start, Empty, "Game.searchNumber: start must be 1 or more");
	char *Found = SearchForNumber(Count, Begin);
	if (Found == nullptr || Begin == Empty) {
		return sol::nullopt;
	}
	return static_cast<int>(Found - &Text[0]) + 1;
}

// Game.getWorldType() -> WorldType (WORLD_TYPE_ constants). Throws nothing.
static int GameGetWorldType() {
	return static_cast<int>(WorldType);
}

// Game.getRoundNumber() -> RoundNr, the round (a second of the game) it is.
// Throws nothing.
static uint32_t GameGetRoundNumber() {
	return RoundNr;
}

// Game.log(name, text) -> Log(Name, "%s", Text) of writer.cc: the text in the
// log <name>.log (and on the console at debug level 2), Name a copy. Throws
// nothing.
static void GameLog(std::string Name, std::string Text) {
	std::string Buffer = Copy(Name);
	Log(&Buffer[0], "%s", Text.c_str());
}

// Magic.getSpellLevel(spell) -> GetSpellLevel(Spell) of magic.cc, the level
// the spell needs (logged and 1 for a number out of range). Throws nothing.
static int MagicGetSpellLevel(int Spell) {
	return GetSpellLevel(Spell);
}

// ---------------------------------------------------------------------------
// The constants

// A constant of the NPCs' scripts: its name and its value.
struct TNpcConstant {
	const char *Name;
	int Value;
};

static const TNpcConstant Constants[] = {
	// The situations of SITUATION (crnonpl.hh).
	{"NPC_SITUATION_DEFAULT", DEFAULT},
	{"NPC_SITUATION_ADDRESS", ADDRESS},
	{"NPC_SITUATION_ADDRESSQUEUE", ADDRESSQUEUE},
	{"NPC_SITUATION_BUSY", BUSY},
	{"NPC_SITUATION_VANISH", VANISH},

	// The NPCs' states of STATE (crnonpl.hh).
	{"NPC_STATE_SLEEPING", SLEEPING},
	{"NPC_STATE_IDLE", IDLE},
	{"NPC_STATE_TALKING", TALKING},
	{"NPC_STATE_LEAVING", LEAVING},

	// The sexes of cr.hh, TFS's names.
	{"PLAYERSEX_MALE", SEX_MALE},
	{"PLAYERSEX_FEMALE", SEX_FEMALE},

	// The world types of config.hh (TWorldType), TFS's names.
	{"WORLD_TYPE_PVP", NORMAL},
	{"WORLD_TYPE_NO_PVP", NON_PVP},
	{"WORLD_TYPE_PVP_ENFORCED", PVP_ENFORCED},
};

// The values scripting.hh gives them, 7.72's.
static_assert(DEFAULT == 0 && ADDRESS == 1 && ADDRESSQUEUE == 2 && BUSY == 3 && VANISH == 4,
		"the NPC_SITUATION_ constants are 0 to 4");
static_assert(SLEEPING == 0 && IDLE == 1 && TALKING == 3 && LEAVING == 4,
		"the NPC_STATE_ constants are 0, 1, 3 and 4");
static_assert(SEX_MALE == 1 && SEX_FEMALE == 2, "PLAYERSEX_MALE is 1, PLAYERSEX_FEMALE 2");
static_assert(NORMAL == 0 && NON_PVP == 1 && PVP_ENFORCED == 2, "the WORLD_TYPE_ constants are 0 to 2");

void RegisterNpcAPI() {
	sol::table Npc = ScriptAPITable("Npc");
	Npc.set_function("getInterlocutor", Guarded(&NpcGetInterlocutor));
	Npc.set_function("getTopic", Guarded(&NpcGetTopic));
	Npc.set_function("setTopic", Guarded(&NpcSetTopic));
	Npc.set_function("getPrice", Guarded(&NpcGetPrice));
	Npc.set_function("setPrice", Guarded(&NpcSetPrice));
	Npc.set_function("getAmount", Guarded(&NpcGetAmount));
	Npc.set_function("setAmount", Guarded(&NpcSetAmount));
	Npc.set_function("getTypeId", Guarded(&NpcGetTypeId));
	Npc.set_function("setTypeId", Guarded(&NpcSetTypeId));
	Npc.set_function("getData", Guarded(&NpcGetData));
	Npc.set_function("setData", Guarded(&NpcSetData));
	Npc.set_function("getState", Guarded(&NpcGetState));
	Npc.set_function("setState", Guarded(&NpcSetState));
	Npc.set_function("setLastTalk", Guarded(&NpcSetLastTalk));
	Npc.set_function("toDoWait", Guarded(&NpcToDoWait));
	Npc.set_function("toDoSay", Guarded(&NpcToDoSay));
	Npc.set_function("toDoChangeState", Guarded(&NpcToDoChangeState));
	Npc.set_function("toDoStart", Guarded(&NpcToDoStart));
	Npc.set_function("enqueue", Guarded(&NpcEnqueue));
	Npc.set_function("giveItems", Guarded(&NpcGiveItems));
	Npc.set_function("takeItems", Guarded(&NpcTakeItems));
	Npc.set_function("giveMoney", Guarded(&NpcGiveMoney));
	Npc.set_function("takeMoney", Guarded(&NpcTakeMoney));

	sol::table Creature = ScriptAPITable("Creature");
	Creature.set_function("getPosition", Guarded(&CreatureGetPosition));
	Creature.set_function("sendMagicEffect", Guarded(&CreatureSendMagicEffect));
	Creature.set_function("getHealth", Guarded(&CreatureGetHealth));
	Creature.set_function("setHealth", Guarded(&CreatureSetHealth));
	Creature.set_function("die", Guarded(&CreatureDie));
	Creature.set_function("setDamageOrigin", Guarded(&CreatureSetDamageOrigin));

	sol::table Player = ScriptAPITable("Player");
	Player.set_function("getSex", Guarded(&PlayerGetSex));
	Player.set_function("isPzLocked", Guarded(&PlayerIsPzLocked));
	Player.set_function("getItemCount", Guarded(&PlayerGetItemCount));
	Player.set_function("getMoney", Guarded(&PlayerGetMoney));
	Player.set_function("hasLearnedSpell", Guarded(&PlayerHasLearnedSpell));
	Player.set_function("learnSpell", Guarded(&PlayerLearnSpell));
	Player.set_function("setVocation", Guarded(&PlayerSetVocation));

	sol::table Game = ScriptAPITable("Game");
	Game.set_function("searchWord", Guarded(&GameSearchWord));
	Game.set_function("searchNumber", Guarded(&GameSearchNumber));
	Game.set_function("getWorldType", Guarded(&GameGetWorldType));
	Game.set_function("getRoundNumber", Guarded(&GameGetRoundNumber));
	Game.set_function("log", Guarded(&GameLog));

	sol::table Magic = ScriptAPITable("Magic");
	Magic.set_function("getSpellLevel", Guarded(&MagicGetSpellLevel));

	for (const TNpcConstant &Constant : Constants) {
		SetScriptConstant(Constant.Name, Constant.Value);
	}
}
