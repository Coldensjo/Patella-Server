// test_scripting.cc: the Lua engine (scripting.cc) on its own. Loading scripts
// and its errors, global handlers, the sandbox, require, result codes in both
// directions and their RETURNVALUE_ names, the logging of other errors, the
// calls a script makes, the objects, the Npcs, the Monsters, MonsterTypes and
// Impacts, the ItemTypes, the Positions, the constants, the core API
// (math.random, print, integer arithmetic), the results of runNumber and
// runBoolean, the calls of an event's rules (TScriptEvent), the message of a
// failed call (TScriptCall::failure), runCaught, the data values of addValue,
// the hooks of the monsters and raids, the script environments, the second
// state of a reload, and a restart of the engine. The scripts are written into
// a scratch folder that is removed at the end. Prints what failed; exits 1 on
// any failure, 0 when all pass.

#include <algorithm>
#include <climits>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <stdexcept>
#include <string>
#include <vector>

#include "objects.hh"
#include "map.hh"
#include "info.hh"
#include "utils.hh"
#include "scripting_api.hh"

namespace fs = std::filesystem;

static int Failures = 0;

#define CHECK(Condition) \
	do { \
		if (!(Condition)) { \
			fprintf(stderr, "%s:%d: CHECK failed: %s\n", __FILE__, __LINE__, #Condition); \
			Failures++; \
		} \
	} while (0)

// Like CHECK, for a text that must contain Part; prints the text otherwise.
#define CHECK_CONTAINS(Text, Part) \
	do { \
		std::string CheckText_ = (Text); \
		if (CheckText_.find(Part) == std::string::npos) { \
			fprintf(stderr, "%s:%d: CHECK failed: \"%s\" does not contain \"%s\"\n", __FILE__, __LINE__, \
					CheckText_.c_str(), std::string(Part).c_str()); \
			Failures++; \
		} \
	} while (0)

static fs::path Folder;

// How often Part occurs in Text.
static int Occurrences(const std::string &Text, const std::string &Part) {
	int Count = 0;
	for (size_t At = Text.find(Part); At != std::string::npos; At = Text.find(Part, At + Part.size())) {
		Count++;
	}
	return Count;
}

// Whether Text begins with Prefix.
static bool StartsWith(const std::string &Text, const std::string &Prefix) {
	return Text.compare(0, Prefix.size(), Prefix) == 0;
}

// The part of Text from "stack traceback:" on, or "" when there is none.
static std::string Traceback(const std::string &Text) {
	size_t At = Text.find("stack traceback:");
	return At == std::string::npos ? std::string() : Text.substr(At);
}

// Writes the script Name (a path below the scratch folder) with Text.
static void WriteScript(const std::string &Name, const std::string &Text) {
	fs::path Path = Folder / Name;
	fs::create_directories(Path.parent_path());
	std::ofstream Out(Path, std::ios::binary);
	Out << Text;
}

// The text LoadScript throws for Name, or "" when it loads.
static std::string LoadError(const char *Name, const char *Where = nullptr) {
	try {
		LoadScript(Name, Where);
	} catch (const char *Text) {
		return Text;
	}
	return "";
}

// The text RequireHandler throws, or "" when the handler is there.
static std::string RequireError(int Script, const char *Handler, const char *Where) {
	try {
		RequireHandler(Script, Handler, Where);
	} catch (const char *Text) {
		return Text;
	}
	return "";
}

// What error() was given since the last TakeErrors.
static std::string Errors;
// What print() was given: level and text.
static std::vector<std::string> Prints;

static void CaptureError(char *Text) {
	Errors += Text;
}

static void CapturePrint(int Level, char *Text) {
	Prints.push_back(std::to_string(Level) + ":" + Text);
}

// The errors logged since the last call, cleared.
static std::string TakeErrors() {
	std::string Result = Errors;
	Errors.clear();
	return Result;
}

// The test API: functions the scripts call, recording what they were given.
static int CountCalls = 0;
static std::vector<std::string> Records;
// The creature IDs that exist for Creature.testHealth.
static std::vector<uint32_t> Alive;

// test.count(): counts the calls.
static void TestCount() {
	CountCalls++;
}

// test.record(text): records the text.
static void TestRecord(std::string Text) {
	Records.push_back(Text);
}

// test.exhaust(): throws EXHAUSTED.
static void TestExhaust() {
	throw EXHAUSTED;
}

// test.twice(n): 2 * n; throws NOROOM for a negative n.
static int TestTwice(int n) {
	if (n < 0) {
		throw NOROOM;
	}
	return 2 * n;
}

// test.throwText(): throws the const char * "plain text".
static void TestThrowText() {
	throw "plain text";
}

// test.throwException(): throws std::runtime_error("what text").
static void TestThrowException() {
	throw std::runtime_error("what text");
}

// test.creatureId(creature): the ID of a Creature or a Player.
static uint32_t TestCreatureId(TScriptCreature Creature) {
	return Creature.ID;
}

// test.playerId(player): the ID of a Player.
static uint32_t TestPlayerId(TScriptPlayer Player) {
	return Player.ID;
}

// test.itemHandle(item): the handle of an Item.
static uint32_t TestItemHandle(TScriptItem Item) {
	return Item.Handle;
}

// test.thing(obj): "creature <ID>" or "item <handle>".
static std::string TestThing(TScriptThing Thing) {
	return std::string(Thing.IsCreature ? "creature " : "item ") + std::to_string(Thing.Value);
}

// test.optionalCreature(creature|nil): its ID, or -1 for nil.
static double TestOptionalCreature(sol::optional<TScriptCreature> Creature) {
	return Creature.has_value() ? static_cast<double>(Creature->ID) : -1;
}

// test.makeCreature(id): the creature ID as an object (nil for 0).
static TScriptCreature TestMakeCreature(uint32_t ID) {
	return TScriptCreature{ID};
}

// test.makeItem(handle): an Item.
static TScriptItem TestMakeItem(uint32_t Handle) {
	return TScriptItem{Handle};
}

// test.npcId(npc): the ID of an Npc.
static uint32_t TestNpcId(TScriptNpc Npc) {
	return Npc.ID;
}

// test.makeNpc(id): an Npc holding the ID.
static TScriptNpc TestMakeNpc(uint32_t ID) {
	return TScriptNpc{ID};
}

// npc:testNpcName(): "npc <ID>", a method of Npc.
static std::string NpcTestNpcName(TScriptNpc Self) {
	return "npc " + std::to_string(Self.ID);
}

// test.monsterId(monster): the ID of a Monster.
static uint32_t TestMonsterId(TScriptMonster Monster) {
	return Monster.ID;
}

// test.makeMonster(id): a Monster holding the ID.
static TScriptMonster TestMakeMonster(uint32_t ID) {
	return TScriptMonster{ID};
}

// monster:testMonsterName(): "monster <ID>", a method of Monster.
static std::string MonsterTestMonsterName(TScriptMonster Self) {
	return "monster " + std::to_string(Self.ID);
}

// test.makeMonsterType(race): a MonsterType holding the race number.
static TScriptMonsterType TestMakeMonsterType(int Race) {
	return TScriptMonsterType{Race};
}

// test.monsterTypeRace(monsterType): the race number of a MonsterType.
static int TestMonsterTypeRace(TScriptMonsterType MonsterType) {
	return MonsterType.Race;
}

// test.makeImpact(kind, actor, a, b, c, d): an Impact of the kind (a
// TScriptImpactKind number) for the actor's ID, with the numbers a to d and
// the colors 1, 2, 3, 4 for an outfit.
static TScriptImpact TestMakeImpact(int Kind, TScriptCreature Actor, int a, int b, int c, int d) {
	TScriptImpact Impact;
	memset(&Impact, 0, sizeof(Impact));
	Impact.Kind = static_cast<TScriptImpactKind>(Kind);
	Impact.Actor = Actor.ID;
	Impact.Values[0] = a;
	Impact.Values[1] = b;
	Impact.Values[2] = c;
	Impact.Values[3] = d;
	if (Impact.Kind == SCRIPT_IMPACT_OUTFIT) {
		for (int i = 0; i < 4; i++) {
			Impact.Colors[i] = static_cast<unsigned char>(i + 1);
		}
	}
	return Impact;
}

// test.impactText(impact): "<kind> <actor> <a> <b> <c> <d> <colors>".
static std::string TestImpactText(TScriptImpact Impact) {
	std::string Text = std::to_string(static_cast<int>(Impact.Kind)) + " " + std::to_string(Impact.Actor);
	for (int i = 0; i < 4; i++) {
		Text += " " + std::to_string(Impact.Values[i]);
	}
	Text += " ";
	for (int i = 0; i < 4; i++) {
		Text += std::to_string(static_cast<int>(Impact.Colors[i]));
	}
	return Text;
}

// test.colors(colors): "<c1> <c2> <c3> <c4>" of an array of four colors.
static std::string TestColors(TScriptColors Colors) {
	return std::to_string(Colors.Values[0]) + " " + std::to_string(Colors.Values[1]) + " "
			+ std::to_string(Colors.Values[2]) + " " + std::to_string(Colors.Values[3]);
}

// test.vocation(player): the player's Vocation.
static TScriptVocation TestVocation(TScriptPlayer Player) {
	return TScriptVocation{Player.ID};
}

// test.vocationPlayer(vocation): the ID of the Vocation's player.
static uint32_t TestVocationPlayer(TScriptVocation Vocation) {
	return Vocation.PlayerID;
}

// test.itemTypeId(itemType): the type number of an ItemType.
static int TestItemTypeId(TScriptItemType ItemType) {
	return ItemType.TypeID;
}

// test.makeItemType(id): an ItemType holding id as it is.
static TScriptItemType TestMakeItemType(int TypeID) {
	return TScriptItemType{TypeID};
}

// itemType:testDouble(): twice the type number, a method of ItemType.
static int ItemTypeTestDouble(TScriptItemType Self) {
	return 2 * Self.TypeID;
}

// creature:testName(): "creature <ID>", a method of Creature.
static std::string CreatureTestName(TScriptCreature Self) {
	return "creature " + std::to_string(Self.ID);
}

// creature:testHealth(): 100 for a creature in Alive; for one that is gone
// logs "Creature.testHealth: Creature does not exist." and throws ERROR, as
// the game's bindings do.
static int CreatureTestHealth(TScriptCreature Self) {
	if (std::find(Alive.begin(), Alive.end(), Self.ID) == Alive.end()) {
		error("Creature.testHealth: Creature does not exist.\n");
		throw ERROR;
	}
	return 100;
}

// test.positionText(position): "x y z".
static std::string TestPositionText(TScriptPosition Position) {
	return std::to_string(Position.x) + " " + std::to_string(Position.y) + " " + std::to_string(Position.z);
}

// test.makePosition(x, y, z): a Position.
static TScriptPosition TestMakePosition(int x, int y, int z) {
	return TScriptPosition{x, y, z};
}

// The script test.nestedEvent calls, set by TestEvents.
static int NestedEventScript = -1;

// test.nestedEvent(): a TScriptEvent ("separation", objects 0, 21, 0) inside
// a call, checking rule 7 with onRule of NestedEventScript; its result.
static bool TestNestedEvent() {
	TScriptEvent Event("separation", 0, 21, 0);
	return Event.check(NestedEventScript, "onRule", 7);
}

static void RegisterTestAPI() {
	sol::table API = ScriptAPITable("test");
	API.set_function("count", Guarded(&TestCount));
	API.set_function("record", Guarded(&TestRecord));
	API.set_function("exhaust", Guarded(&TestExhaust));
	API.set_function("twice", Guarded(&TestTwice));
	API.set_function("throwText", Guarded(&TestThrowText));
	API.set_function("throwException", Guarded(&TestThrowException));
	API.set_function("creatureId", Guarded(&TestCreatureId));
	API.set_function("playerId", Guarded(&TestPlayerId));
	API.set_function("itemHandle", Guarded(&TestItemHandle));
	API.set_function("thing", Guarded(&TestThing));
	API.set_function("optionalCreature", Guarded(&TestOptionalCreature));
	API.set_function("makeCreature", Guarded(&TestMakeCreature));
	API.set_function("makeItem", Guarded(&TestMakeItem));
	API.set_function("vocation", Guarded(&TestVocation));
	API.set_function("vocationPlayer", Guarded(&TestVocationPlayer));
	API.set_function("positionText", Guarded(&TestPositionText));
	API.set_function("makePosition", Guarded(&TestMakePosition));
	API.set_function("nestedEvent", Guarded(&TestNestedEvent));
	API.set_function("itemTypeId", Guarded(&TestItemTypeId));
	API.set_function("makeItemType", Guarded(&TestMakeItemType));
	API.set_function("npcId", Guarded(&TestNpcId));
	API.set_function("makeNpc", Guarded(&TestMakeNpc));
	API.set_function("monsterId", Guarded(&TestMonsterId));
	API.set_function("makeMonster", Guarded(&TestMakeMonster));
	API.set_function("makeMonsterType", Guarded(&TestMakeMonsterType));
	API.set_function("monsterTypeRace", Guarded(&TestMonsterTypeRace));
	API.set_function("makeImpact", Guarded(&TestMakeImpact));
	API.set_function("impactText", Guarded(&TestImpactText));
	API.set_function("colors", Guarded(&TestColors));
	sol::table Npc = ScriptAPITable("Npc");
	Npc.set_function("testNpcName", Guarded(&NpcTestNpcName));
	sol::table Monster = ScriptAPITable("Monster");
	Monster.set_function("testMonsterName", Guarded(&MonsterTestMonsterName));
	sol::table ItemType = ScriptAPITable("ItemType");
	ItemType.set_function("testDouble", Guarded(&ItemTypeTestDouble));
	sol::table Creature = ScriptAPITable("Creature");
	Creature.set_function("testName", Guarded(&CreatureTestName));
	Creature.set_function("testHealth", Guarded(&CreatureTestHealth));
	SetScriptConstant("CONST_ME_FIREAREA", 7);
}

// How a handler call ended.
struct TOutcome {
	bool Thrown;    // a RESULT was thrown
	RESULT Code;    // which
	int Number;     // runNumber's result
	bool Boolean;   // runBoolean's result
};

// The kinds of handler call.
enum TRun { RUN, RUN_NUMBER, RUN_BOOLEAN };

// Runs ScriptCall as Kind says.
static TOutcome Run(TScriptCall &ScriptCall, TRun Kind) {
	TOutcome Outcome = {false, NOERROR, 0, false};
	try {
		if (Kind == RUN) {
			ScriptCall.run();
		} else if (Kind == RUN_NUMBER) {
			Outcome.Number = ScriptCall.runNumber();
		} else {
			Outcome.Boolean = ScriptCall.runBoolean();
		}
	} catch (RESULT r) {
		Outcome.Thrown = true;
		Outcome.Code = r;
	}
	return Outcome;
}

// Calls Handler of Script with the arguments Arguments (numbers as ints).
static TOutcome Call(int Script, const char *Handler, TRun Kind, const std::vector<int> &Arguments = {}) {
	TScriptCall ScriptCall(Script, Handler);
	for (int Argument : Arguments) {
		ScriptCall.addNumber(Argument);
	}
	return Run(ScriptCall, Kind);
}

// Loads Name, which must load; -1 (and a failure) otherwise.
static int Load(const char *Name) {
	try {
		return LoadScript(Name, nullptr);
	} catch (const char *Text) {
		fprintf(stderr, "LoadScript(%s) failed: %s\n", Name, Text);
		Failures++;
		return -1;
	}
}

static void TestLoading() {
	WriteScript("good.lua", "test.count()\nfunction onA() return 1 end\n");
	CountCalls = 0;
	int Good = Load("good.lua");
	CHECK(Good >= 0);
	CHECK(CountCalls == 1);
	CHECK(LoadScript("good.lua", nullptr) == Good);
	CHECK(CountCalls == 1);
	CHECK(ScriptName(Good) != nullptr && strcmp(ScriptName(Good), "good.lua") == 0);

	WriteScript("sub/dir/nested.lua", "local x = 1\n");
	CHECK(Load("sub/dir/nested.lua") >= 0);
	CHECK(Load("sub/dir/nested.lua") != Good);

	std::string Missing = LoadError("missing.lua", "data/x.toml:3:5");
	CHECK_CONTAINS(Missing, "data/x.toml:3:5: ");
	CHECK_CONTAINS(Missing, "cannot open missing.lua");
	CHECK_CONTAINS(LoadError("missing.lua"), "cannot open missing.lua");
	CHECK(LoadError("missing.lua").compare(0, 4, "data") != 0);

	const char *BadNames[] = {"../x.lua", "/x.lua", "x.txt", "a/./b.lua", "A.lua", "a//b.lua", ".lua", "a/.lua",
			"x.lua/", "x", "", "a b.lua", "a/../good.lua"};
	for (const char *Name : BadNames) {
		std::string Text = LoadError(Name, "data/x.toml:1:1");
		CHECK_CONTAINS(Text, "data/x.toml:1:1: script name '");
		CHECK_CONTAINS(Text, "' is not allowed");
	}
	CHECK_CONTAINS(LoadError("../x.lua"), "script name '../x.lua' is not allowed");

	WriteScript("syntax.lua", "local x = 1\nlocal y = = 2\n");
	std::string Syntax = LoadError("syntax.lua", "data/x.toml:3:5");
	CHECK(Syntax.compare(0, 17, "data/x.toml:3:5: ") == 0);
	CHECK_CONTAINS(Syntax, "syntax.lua:2:");
	CHECK(Syntax.find("[string") == std::string::npos);

	WriteScript("toprun.lua", "local t = nil\nt.x = 1\n");
	std::string TopRun = LoadError("toprun.lua", "data/x.toml:4:1");
	CHECK(TopRun.compare(0, 17, "data/x.toml:4:1: ") == 0);
	CHECK_CONTAINS(TopRun, "toprun.lua:2:");
	CHECK_CONTAINS(TopRun, "stack traceback:");

	// A script returns nothing: its handlers are global functions.
	WriteScript("table.lua", "return { onA = function() end }\n");
	CHECK(LoadError("table.lua", "data/x.toml:5:1")
			== "data/x.toml:5:1: table.lua: a script returns nothing; its handlers are global functions");
	WriteScript("number.lua", "return 5\n");
	CHECK(LoadError("number.lua") == "number.lua: a script returns nothing; its handlers are global functions");
	WriteScript("second.lua", "return nil, 5\n");
	CHECK_CONTAINS(LoadError("second.lua"), "second.lua: a script returns nothing");
	WriteScript("nothing.lua", "local x = 1\n");
	CHECK(Load("nothing.lua") >= 0);
	WriteScript("return_nil.lua", "function onA() end\nreturn nil\n");
	CHECK(Load("return_nil.lua") >= 0);

	WriteScript("unknown_global.lua", "local x = 1\nlocal y = undefined_thing\n");
	std::string Unknown = LoadError("unknown_global.lua", "data/x.toml:2:2");
	CHECK_CONTAINS(Unknown, "data/x.toml:2:2: unknown_global.lua:2: unknown name 'undefined_thing'");

	WriteScript("unknown_api.lua", "local f = Game.noSuchFunction\n");
	CHECK_CONTAINS(LoadError("unknown_api.lua"), "unknown_api.lua:1: unknown name 'Game.noSuchFunction'");
	WriteScript("unknown_result.lua", "local r = RETURNVALUE_NOSUCHRESULT\n");
	CHECK_CONTAINS(LoadError("unknown_result.lua"), "unknown name 'RETURNVALUE_NOSUCHRESULT'");
	WriteScript("unknown_test.lua", "local r = test.nothing\n");
	CHECK_CONTAINS(LoadError("unknown_test.lua"), "unknown name 'test.nothing'");
	WriteScript("unknown_method.lua", "local m = Player.noSuchMethod\n");
	CHECK_CONTAINS(LoadError("unknown_method.lua"), "unknown name 'Player.noSuchMethod'");

	// The old lower-case tables are gone.
	const char *OldNames[] = {"game", "result", "magic", "command", "creature", "object", "effect", "animation",
			"damage", "field", "ammo", "skill", "profession"};
	int Number = 0;
	for (const char *Name : OldNames) {
		std::string Script = "old_" + std::to_string(Number++) + ".lua";
		WriteScript(Script, std::string("local x = ") + Name + "\n");
		CHECK_CONTAINS(LoadError(Script.c_str()), Script + ":1: unknown name '" + Name + "'");
	}

	WriteScript("top_throw.lua", "Game.throwResult(RETURNVALUE_NOTENOUGHROOM)\n");
	CHECK_CONTAINS(LoadError("top_throw.lua"), "result RETURNVALUE_NOTENOUGHROOM");

	WriteScript("binary.lua", std::string("\033LJ\002") + "junk");
	CHECK_CONTAINS(LoadError("binary.lua"), "binary chunks are not allowed");

	// A failed load is not cached: once fixed, the script loads.
	WriteScript("number.lua", "function onA() end\n");
	CHECK(Load("number.lua") >= 0);
}

static void TestHandlers() {
	WriteScript("handlers.lua",
			"function onA() end\n"
			"onB = 5\n"
			"local function onD() end\n"
			"onE = function() end\n");
	int Script = Load("handlers.lua");
	CHECK(HasHandler(Script, "onA"));
	CHECK(!HasHandler(Script, "onB"));
	CHECK(!HasHandler(Script, "onC"));
	CHECK(!HasHandler(Script, "onD"));
	CHECK(HasHandler(Script, "onE"));
	// A shared global is not the script's handler.
	CHECK(!HasHandler(Script, "print"));
	CHECK(!HasHandler(Script, "pcall"));
	CHECK(RequireError(Script, "onA", "data/x.toml:1:2") == "");
	CHECK(RequireError(Script, "onC", "data/x.toml:1:2") == "data/x.toml:1:2: handlers.lua has no function onC");
	CHECK(RequireError(Script, "onB", "data/x.toml:7:3") == "data/x.toml:7:3: handlers.lua has no function onB");
	CHECK(RequireError(Script, "onB", nullptr) == "handlers.lua has no function onB");
	CHECK(RequireError(Script, "print", nullptr) == "handlers.lua has no function print");

	// A missing handler at run time is a non-result error.
	TakeErrors();
	TOutcome Outcome = Call(Script, "onC", RUN);
	CHECK(Outcome.Thrown && Outcome.Code == ERROR);
	CHECK_CONTAINS(TakeErrors(), "Script error in handlers.lua (onC): handlers.lua has no function onC");

	// A handler a script sets later (from another handler) counts then.
	WriteScript("handlers_late.lua", "function onSet() function onLater() return 3 end end\n");
	int Late = Load("handlers_late.lua");
	CHECK(!HasHandler(Late, "onLater"));
	CHECK(!Call(Late, "onSet", RUN).Thrown);
	TOutcome Later = Call(Late, "onLater", RUN_NUMBER);
	CHECK(!Later.Thrown && Later.Number == 3);
}

static void TestSandbox() {
	const char *Unknown[] = {"io", "os", "debug", "ffi", "jit", "package", "dofile", "loadfile", "load",
			"loadstring", "module", "_G", "getfenv", "setfenv", "rawset", "newproxy", "collectgarbage", "gcinfo",
			"unknown"};
	int Number = 0;
	for (const char *Name : Unknown) {
		// Named by number: not every global's name is a valid script name.
		std::string Script = "sandbox_" + std::to_string(Number++) + ".lua";
		WriteScript(Script, std::string("local x = ") + Name + "\n");
		CHECK_CONTAINS(LoadError(Script.c_str()), Script + ":1: unknown name '" + Name + "'");
	}

	WriteScript("sandbox_ok.lua",
			"function onRemoved()\n"
			"  return string.dump == nil and math.randomseed == nil and (\"x\").dump == nil\n"
			"    and type(math.random) == 'function' and type(print) == 'function'\n"
			"    and type(debugPrint) == 'function'\n"
			"end\n"
			"function onWorks()\n"
			"  local t = {}\n"
			"  table.insert(t, 3)\n"
			"  table.insert(t, 4)\n"
			"  local f = coroutine.wrap(function() coroutine.yield(7) end)\n"
			"  return bit.band(6, 3) == 2 and bit.bor(1, 4) == 5 and string.format(\"%d-%s\", 5, \"x\") == \"5-x\"\n"
			"    and #t == 2 and t[2] == 4 and math.floor(2.5) == 2 and f() == 7\n"
			"    and type(pcall) == \"function\" and type(require) == \"function\"\n"
			"end\n");
	int Script = Load("sandbox_ok.lua");
	TOutcome Removed = Call(Script, "onRemoved", RUN_BOOLEAN);
	CHECK(!Removed.Thrown && Removed.Boolean);
	TOutcome Works = Call(Script, "onWorks", RUN_BOOLEAN);
	CHECK(!Works.Thrown && Works.Boolean);
	CHECK(TakeErrors() == "");

	// The JIT compiler is on: luaopen_jit ran (its module stays in the
	// registry's _LOADED, out of the scripts' reach) and jit.status() says so.
	lua_State *L = ScriptState().lua_state();
	int Top = lua_gettop(L);
	lua_getfield(L, LUA_REGISTRYINDEX, "_LOADED");
	CHECK(lua_istable(L, -1));
	lua_getfield(L, -1, "jit");
	CHECK(lua_istable(L, -1));
	lua_getfield(L, -1, "status");
	CHECK(lua_isfunction(L, -1));
	CHECK(lua_pcall(L, 0, 1, 0) == 0);
	CHECK(lua_toboolean(L, -1));
	lua_getfield(L, LUA_REGISTRYINDEX, "_LOADED");
	lua_getfield(L, -1, "io");
	CHECK(lua_isnil(L, -1));
	lua_getfield(L, -2, "os");
	CHECK(lua_isnil(L, -1));
	lua_getfield(L, -3, "debug");
	CHECK(lua_isnil(L, -1));
	lua_getfield(L, -4, "package");
	CHECK(lua_isnil(L, -1));
	lua_settop(L, Top);
}

static void TestRequire() {
	WriteScript("lib/counted.lua", "test.count()\nreturn { value = 42 }\n");
	WriteScript("lib/nothing.lua", "local x = 1\n");
	WriteScript("lib/deep/inner.lua", "return 'inner'\n");
	WriteScript("req1.lua",
			"local a = require('lib.counted')\n"
			"local b = require('lib.counted')\n"
			"local n = require('lib.nothing')\n"
			"local i = require('lib.deep.inner')\n"
			"own_global = 1\n"
			"function onCheck() return a == b and a.value == 42 and n == true and i == 'inner' end\n");
	WriteScript("req2.lua",
			"local a = require('lib.counted')\n"
			"function onValue() return a.value end\n");
	CountCalls = 0;
	int Req1 = Load("req1.lua");
	int Req2 = Load("req2.lua");
	CHECK(CountCalls == 1);
	TOutcome Check = Call(Req1, "onCheck", RUN_BOOLEAN);
	CHECK(!Check.Thrown && Check.Boolean);
	TOutcome Value = Call(Req2, "onValue", RUN_NUMBER);
	CHECK(!Value.Thrown && Value.Number == 42);

	WriteScript("lib/cyc_a.lua", "require('lib.cyc_b')\nreturn 1\n");
	WriteScript("lib/cyc_b.lua", "require('lib.cyc_a')\nreturn 2\n");
	WriteScript("cycle.lua", "require('lib.cyc_a')\n");
	std::string Cycle = LoadError("cycle.lua");
	CHECK_CONTAINS(Cycle, "lib/cyc_b.lua:1: require: 'lib.cyc_a' requires itself: lib/cyc_a.lua -> lib/cyc_b.lua -> lib/cyc_a.lua");
	WriteScript("self.lua", "require('lib.self')\n");
	WriteScript("lib/self.lua", "require('lib.self')\n");
	CHECK_CONTAINS(LoadError("self.lua"), "requires itself: lib/self.lua -> lib/self.lua");

	// A module whose loading failed is neither cached nor left as "being
	// loaded": once fixed, it loads.
	WriteScript("lib/mid.lua", "local b = require('lib.boom')\nreturn b + 1\n");
	WriteScript("lib/boom.lua", "error('boom')\n");
	WriteScript("uses_mid.lua", "local m = require('lib.mid')\nfunction onM() return m end\n");
	CHECK_CONTAINS(LoadError("uses_mid.lua"), "lib/boom.lua:1: boom");
	WriteScript("lib/boom.lua", "return 10\n");
	int UsesMid = Load("uses_mid.lua");
	TOutcome Mid = Call(UsesMid, "onM", RUN_NUMBER);
	CHECK(!Mid.Thrown && Mid.Number == 11);

	// A failed require leaves the module off the stack of modules being
	// loaded: a second try gives the module's own error again, not a cycle,
	// and other modules load after it.
	WriteScript("lib/bad.lua", "local x = 1\nerror('bad module')\n");
	WriteScript("req_retry.lua",
			"function onRetry()\n"
			"  local ok1, e1 = pcall(require, 'lib.bad')\n"
			"  local ok2, e2 = pcall(require, 'lib.bad')\n"
			"  test.record(tostring(e1))\n"
			"  test.record(tostring(e2))\n"
			"  local c = require('lib.counted')\n"
			"  return not ok1 and not ok2 and c.value == 42\n"
			"end\n");
	int Retry = Load("req_retry.lua");
	Records.clear();
	TOutcome Retried = Call(Retry, "onRetry", RUN_BOOLEAN);
	CHECK(!Retried.Thrown && Retried.Boolean);
	CHECK(Records.size() == 2);
	if (Records.size() == 2) {
		CHECK_CONTAINS(Records[0], "lib/bad.lua:2: bad module");
		CHECK_CONTAINS(Records[1], "lib/bad.lua:2: bad module");
		CHECK(Records[1].find("requires itself") == std::string::npos);
		// A script's pcall of require gets what the module raised, untraced.
		CHECK(Records[0] == "lib/bad.lua:2: bad module");
		CHECK(Records[1] == "lib/bad.lua:2: bad module");
	}
	// The same at a script's top level, and the module loads once fixed.
	WriteScript("req_retry_top.lua",
			"local ok1 = pcall(require, 'lib.bad')\n"
			"local ok2, e2 = pcall(require, 'lib.bad')\n"
			"if ok1 or ok2 or string.find(e2, 'requires itself', 1, true) then error('retry failed') end\n");
	CHECK(Load("req_retry_top.lua") >= 0);
	WriteScript("lib/bad.lua", "return 3\n");
	WriteScript("req_fixed.lua", "local b = require('lib.bad')\nfunction onB() return b end\n");
	TOutcome Fixed = Call(Load("req_fixed.lua"), "onB", RUN_NUMBER);
	CHECK(!Fixed.Thrown && Fixed.Number == 3);
	CHECK(TakeErrors() == "");

	WriteScript("lib/missing_user.lua", "return 1\n");
	WriteScript("req_missing.lua", "require('lib.not_there')\n");
	CHECK_CONTAINS(LoadError("req_missing.lua"), "req_missing.lua:1: require 'lib.not_there': cannot open lib/not_there.lua");
	WriteScript("lib/bad_syntax.lua", "x = = 1\n");
	WriteScript("req_syntax.lua", "require('lib.bad_syntax')\n");
	CHECK_CONTAINS(LoadError("req_syntax.lua"), "lib/bad_syntax.lua:1:");

	WriteScript("req_names.lua",
			"function onRefused(i)\n"
			"  local names = { '..x', '../x', '/etc/passwd', 'lib.X', 'lib..counted', '.lib', 'lib.', 'lib/counted', 'a b', '' }\n"
			"  local ok, err = pcall(require, names[i])\n"
			"  return not ok and string.find(err, 'is not allowed', 1, true) ~= nil\n"
			"end\n");
	int Names = Load("req_names.lua");
	for (int i = 1; i <= 10; i++) {
		TOutcome Refused = Call(Names, "onRefused", RUN_BOOLEAN, {i});
		CHECK(!Refused.Thrown && Refused.Boolean);
		if (Refused.Thrown || !Refused.Boolean) {
			fprintf(stderr, "  require name number %d was not refused\n", i);
		}
	}
	CHECK(TakeErrors() == "");
}

// The number of the shared global Name, read raw from C++; INT_MIN when it is
// not a number.
static int SharedNumber(const char *Name) {
	lua_State *L = ScriptState().lua_state();
	lua_pushstring(L, Name);
	lua_rawget(L, LUA_GLOBALSINDEX);
	int Value = lua_type(L, -1) == LUA_TNUMBER ? static_cast<int>(lua_tointeger(L, -1)) : INT_MIN;
	lua_pop(L, 1);
	return Value;
}

// Every result code has its RETURNVALUE_ name (the mapping's).
static void TestResultNames() {
	struct TName { RESULT Code; const char *Name; };
	const TName Names[] = {
		{ERROR, "RETURNVALUE_ERROR"}, {NOERROR, "RETURNVALUE_NOERROR"},
		{NOTACCESSIBLE, "RETURNVALUE_NOTPOSSIBLE"}, {NOTMOVABLE, "RETURNVALUE_NOTMOVEABLE"},
		{NOTTAKABLE, "RETURNVALUE_CANNOTPICKUP"}, {NOROOM, "RETURNVALUE_NOTENOUGHROOM"},
		{OUTOFRANGE, "RETURNVALUE_DESTINATIONOUTOFREACH"}, {OUTOFHOME, "RETURNVALUE_OUTOFHOME"},
		{CANNOTTHROW, "RETURNVALUE_CANNOTTHROW"}, {TOOHEAVY, "RETURNVALUE_NOTENOUGHCAPACITY"},
		{CROSSREFERENCE, "RETURNVALUE_THISISIMPOSSIBLE"}, {CONTAINERFULL, "RETURNVALUE_CONTAINERNOTENOUGHROOM"},
		{WRONGPOSITION, "RETURNVALUE_PUTTHISOBJECTINYOURHAND"},
		{WRONGPOSITION2, "RETURNVALUE_PUTTHISOBJECTINBOTHHANDS"}, {WRONGCLOTHES, "RETURNVALUE_CANNOTBEDRESSED"},
		{HANDSNOTFREE, "RETURNVALUE_BOTHHANDSNEEDTOBEFREE"}, {HANDBLOCKED, "RETURNVALUE_DROPTWOHANDEDITEM"},
		{ONEWEAPONONLY, "RETURNVALUE_CANONLYUSEONEWEAPON"}, {NOMATCH, "RETURNVALUE_NOMATCH"},
		{NOTCUMULABLE, "RETURNVALUE_NOTCUMULABLE"}, {TOOMANYPARTS, "RETURNVALUE_TOOMANYPARTS"},
		{EMPTYCONTAINER, "RETURNVALUE_EMPTYCONTAINER"}, {SPLITOBJECT, "RETURNVALUE_SPLITOBJECT"},
		{NOKEYMATCH, "RETURNVALUE_NOKEYMATCH"}, {UPSTAIRS, "RETURNVALUE_FIRSTGOUPSTAIRS"},
		{DOWNSTAIRS, "RETURNVALUE_FIRSTGODOWNSTAIRS"}, {CREATURENOTEXISTING, "RETURNVALUE_CREATUREDOESNOTEXIST"},
		{PLAYERNOTEXISTING, "RETURNVALUE_PLAYERNOTEXISTING"},
		{PLAYERNOTONLINE, "RETURNVALUE_PLAYERWITHTHISNAMEISNOTONLINE"},
		{NAMEAMBIGUOUS, "RETURNVALUE_NAMEISTOOAMBIGUOUS"}, {NOTUSABLE, "RETURNVALUE_CANNOTUSETHISOBJECT"},
		{FEDUP, "RETURNVALUE_FEDUP"}, {DESTROYED, "RETURNVALUE_DESTROYED"},
		{SPELLUNKNOWN, "RETURNVALUE_YOUNEEDTOLEARNTHISSPELL"}, {LOWMAGICLEVEL, "RETURNVALUE_NOTENOUGHMAGICLEVEL"},
		{MAGICITEM, "RETURNVALUE_YOUNEEDAMAGICITEMTOCASTSPELL"}, {NOTENOUGHMANA, "RETURNVALUE_NOTENOUGHMANA"},
		{NOSKILL, "RETURNVALUE_NOSKILL"}, {TARGETLOST, "RETURNVALUE_TARGETLOST"},
		{OUTOFAMMO, "RETURNVALUE_OUTOFAMMO"}, {NOCREATURE, "RETURNVALUE_CANONLYUSETHISRUNEONCREATURES"},
		{TOOLONG, "RETURNVALUE_TOOLONG"}, {TARGETOUTOFRANGE, "RETURNVALUE_TARGETOUTOFRANGE"},
		{TARGETHIDDEN, "RETURNVALUE_TARGETHIDDEN"}, {ATTACKNOTALLOWED, "RETURNVALUE_YOUMAYNOTATTACKTHISPLAYER"},
		{NOWAY, "RETURNVALUE_THEREISNOWAY"}, {LOGINERROR, "RETURNVALUE_LOGINERROR"},
		{LOGINABORT, "RETURNVALUE_LOGINABORT"},
		{PROTECTIONZONE, "RETURNVALUE_ACTIONNOTPERMITTEDINPROTECTIONZONE"},
		{ENTERPROTECTIONZONE, "RETURNVALUE_ENTERPROTECTIONZONE"}, {EXHAUSTED, "RETURNVALUE_YOUAREEXHAUSTED"},
		{NOTINVITED, "RETURNVALUE_NOTINVITED"}, {NOPREMIUMACCOUNT, "RETURNVALUE_YOUNEEDPREMIUMACCOUNT"},
		{MOVENOTPOSSIBLE, "RETURNVALUE_MOVENOTPOSSIBLE"}, {ALREADYTRADING, "RETURNVALUE_YOUAREALREADYTRADING"},
		{PARTNERTRADING, "RETURNVALUE_THISPLAYERISALREADYTRADING"}, {TOOMANYOBJECTS, "RETURNVALUE_TOOMANYOBJECTS"},
		{TOOMANYSLAVES, "RETURNVALUE_TOOMANYSLAVES"}, {NOTTURNABLE, "RETURNVALUE_NOTTURNABLE"},
		{SECUREMODE, "RETURNVALUE_TURNSECUREMODETOATTACKUNMARKEDPLAYERS"},
		{NOTENOUGHSOULPOINTS, "RETURNVALUE_NOTENOUGHSOUL"}, {LOWLEVEL, "RETURNVALUE_NOTENOUGHLEVEL"},
	};
	CHECK(sizeof(Names) / sizeof(Names[0]) == static_cast<size_t>(LOWLEVEL - ERROR + 1));
	WriteScript("result_names.lua",
			"function onName(name) Game.throwResult(name) end\n"
			"function onText(code) local ok, err = pcall(Game.throwResult, code) test.record(tostring(err)) end\n");
	int Script = Load("result_names.lua");
	for (const TName &Name : Names) {
		CHECK(SharedNumber(Name.Name) == static_cast<int>(Name.Code));
		if (SharedNumber(Name.Name) != static_cast<int>(Name.Code)) {
			fprintf(stderr, "  %s is not %d\n", Name.Name, static_cast<int>(Name.Code));
		}
		TScriptCall ByName(Script, "onName");
		ByName.addString(Name.Name);
		TOutcome Outcome = Run(ByName, RUN);
		CHECK(Outcome.Thrown && Outcome.Code == Name.Code);
		Records.clear();
		CHECK(!Call(Script, "onText", RUN, {static_cast<int>(Name.Code)}).Thrown);
		CHECK(Records.size() == 1 && Records[0] == std::string("result ") + Name.Name);
	}
	CHECK(TakeErrors() == "");
}

static void TestResults() {
	WriteScript("results.lua",
			"function onNumber() Game.throwResult(RETURNVALUE_NOTENOUGHROOM) end\n"
			"function onName() Game.throwResult('RETURNVALUE_NOTENOUGHROOM') end\n"
			"function onError() Game.throwResult(RETURNVALUE_ERROR) end\n"
			"function onUncaught() test.exhaust() end\n"
			"function onCaught()\n"
			"  local ok, err = pcall(test.exhaust)\n"
			"  return not ok and Game.getResultOf(err) == RETURNVALUE_YOUAREEXHAUSTED\n"
			"    and tostring(err) == 'result RETURNVALUE_YOUAREEXHAUSTED' and err.code == 49\n"
			"end\n"
			"function onRethrow() local ok, err = pcall(test.exhaust); error(err) end\n"
			"function onWrapped() local f = coroutine.wrap(function() test.exhaust() end); f() end\n"
			"function onTwice(n) return test.twice(n) end\n"
			"function onNames()\n"
			"  return RETURNVALUE_ERROR == -1 and RETURNVALUE_NOERROR == 0 and RETURNVALUE_NOTENOUGHROOM == 4\n"
			"    and RETURNVALUE_NOTENOUGHLEVEL == 60 and RETURNVALUE_NOTPOSSIBLE == 1\n"
			"    and Game.getResultOf('RETURNVALUE_NOTENOUGHROOM') == nil and Game.getResultOf(4) == nil\n"
			"    and Game.getResultOf() == nil\n"
			"end\n"
			"function onBadThrow() Game.throwResult(61) end\n"
			"function onBadName() Game.throwResult('NOROOM') end\n"
			"function onBadType() Game.throwResult({}) end\n"
			"function onUnknownName() return RETURNVALUE_NOSUCH end\n");
	int Script = Load("results.lua");
	TakeErrors();

	TOutcome Number = Call(Script, "onNumber", RUN);
	CHECK(Number.Thrown && Number.Code == NOROOM);
	TOutcome Name = Call(Script, "onName", RUN);
	CHECK(Name.Thrown && Name.Code == NOROOM);
	TOutcome ErrorCode = Call(Script, "onError", RUN);
	CHECK(ErrorCode.Thrown && ErrorCode.Code == ERROR);
	TOutcome Uncaught = Call(Script, "onUncaught", RUN);
	CHECK(Uncaught.Thrown && Uncaught.Code == EXHAUSTED);
	TOutcome Caught = Call(Script, "onCaught", RUN_BOOLEAN);
	CHECK(!Caught.Thrown && Caught.Boolean);
	TOutcome Rethrow = Call(Script, "onRethrow", RUN);
	CHECK(Rethrow.Thrown && Rethrow.Code == EXHAUSTED);
	TOutcome Wrapped = Call(Script, "onWrapped", RUN);
	CHECK(Wrapped.Thrown && Wrapped.Code == EXHAUSTED);
	TOutcome Twice = Call(Script, "onTwice", RUN_NUMBER, {21});
	CHECK(!Twice.Thrown && Twice.Number == 42);
	TOutcome TwiceThrown = Call(Script, "onTwice", RUN_NUMBER, {-1});
	CHECK(TwiceThrown.Thrown && TwiceThrown.Code == NOROOM);
	TOutcome Names = Call(Script, "onNames", RUN_BOOLEAN);
	CHECK(!Names.Thrown && Names.Boolean);
	// None of these is logged: result codes are not script errors.
	CHECK(TakeErrors() == "");

	TOutcome BadThrow = Call(Script, "onBadThrow", RUN);
	CHECK(BadThrow.Thrown && BadThrow.Code == ERROR);
	CHECK_CONTAINS(TakeErrors(), "Game.throwResult: 61 is not a result code");
	TOutcome BadName = Call(Script, "onBadName", RUN);
	CHECK(BadName.Thrown && BadName.Code == ERROR);
	CHECK_CONTAINS(TakeErrors(), "Game.throwResult: unknown result 'NOROOM'");
	TOutcome BadType = Call(Script, "onBadType", RUN);
	CHECK(BadType.Thrown && BadType.Code == ERROR);
	CHECK_CONTAINS(TakeErrors(), "result code expected");
	TOutcome UnknownName = Call(Script, "onUnknownName", RUN_NUMBER);
	CHECK(UnknownName.Thrown && UnknownName.Code == ERROR);
	CHECK_CONTAINS(TakeErrors(), "unknown name 'RETURNVALUE_NOSUCH'");
}

static void TestErrors() {
	WriteScript("fail.lua",
			"local function inner(t)\n"
			"  return t.x\n"
			"end\n"
			"function onFail() local v = inner(nil) return v end\n"
			"function onUnknown() return not_defined_anywhere end\n"
			"function onTable() error({}) end\n"
			"function onStringError() error('custom message') end\n");
	int Script = Load("fail.lua");
	TakeErrors();
	TOutcome Fail = Call(Script, "onFail", RUN);
	CHECK(Fail.Thrown && Fail.Code == ERROR);
	std::string Logged = TakeErrors();
	CHECK(StartsWith(Logged, "Script error in fail.lua (onFail): fail.lua:2:"));
	CHECK_CONTAINS(Logged, "\nstack traceback:\n");
	CHECK_CONTAINS(Logged, "fail.lua:4:");

	TOutcome Unknown = Call(Script, "onUnknown", RUN);
	CHECK(Unknown.Thrown && Unknown.Code == ERROR);
	CHECK_CONTAINS(TakeErrors(), "Script error in fail.lua (onUnknown): fail.lua:5: unknown name 'not_defined_anywhere'");

	TOutcome Table = Call(Script, "onTable", RUN);
	CHECK(Table.Thrown && Table.Code == ERROR);
	CHECK_CONTAINS(TakeErrors(), "(error object is a table value)");

	TOutcome Custom = Call(Script, "onStringError", RUN);
	CHECK(Custom.Thrown && Custom.Code == ERROR);
	CHECK_CONTAINS(TakeErrors(), "Script error in fail.lua (onStringError): fail.lua:7: custom message");
}

// An error raised inside a module (require) is logged with the traceback
// taken where it was raised, once: the frames inside the modules are there.
static void TestModuleTraceback() {
	WriteScript("lib/n3.lua", "local x = 1\nerror('n3 failed')\n");
	WriteScript("lib/n2.lua", "require('lib.n3')\nreturn 2\n");
	WriteScript("lib/n1.lua", "require('lib.n2')\nreturn 1\n");
	WriteScript("nested_top.lua", "require('lib.n1')\n");
	std::string Loaded = LoadError("nested_top.lua", "data/x.toml:9:1");
	CHECK(StartsWith(Loaded, "data/x.toml:9:1: lib/n3.lua:2: n3 failed\nstack traceback:\n"));
	CHECK(Occurrences(Loaded, "stack traceback:") == 1);
	std::string Trace = Traceback(Loaded);
	CHECK_CONTAINS(Trace, "\tlib/n3.lua:2:");
	CHECK_CONTAINS(Trace, "\tlib/n2.lua:1:");
	CHECK_CONTAINS(Trace, "\tlib/n1.lua:1:");
	CHECK_CONTAINS(Trace, "\tnested_top.lua:1:");

	WriteScript("nested_call.lua", "function onReq()\n  require('lib.n1')\nend\n");
	int Script = Load("nested_call.lua");
	TakeErrors();
	TOutcome Outcome = Call(Script, "onReq", RUN);
	CHECK(Outcome.Thrown && Outcome.Code == ERROR);
	std::string Logged = TakeErrors();
	CHECK(StartsWith(Logged, "Script error in nested_call.lua (onReq): lib/n3.lua:2: n3 failed\nstack traceback:\n"));
	CHECK(Occurrences(Logged, "stack traceback:") == 1);
	Trace = Traceback(Logged);
	CHECK_CONTAINS(Trace, "\tlib/n3.lua:2:");
	CHECK_CONTAINS(Trace, "\tlib/n2.lua:1:");
	CHECK_CONTAINS(Trace, "\tlib/n1.lua:1:");
	CHECK_CONTAINS(Trace, "\tnested_call.lua:2:");

	// The module's frames also when a script catches the error and raises it
	// again unchanged; a result code a module raises stays a result code.
	// Level 0: error() would turn the number into a string with the position.
	WriteScript("lib/num_error.lua", "error(42, 0)\n");
	WriteScript("lib/res_error.lua", "Game.throwResult(RETURNVALUE_NOTENOUGHROOM)\n");
	WriteScript("nested_again.lua",
			"function onAgain()\n"
			"  local ok, err = pcall(require, 'lib.n1')\n"
			"  error(err, 0)\n"
			"end\n"
			"function onNumber()\n"
			"  local ok, err = pcall(require, 'lib.num_error')\n"
			"  return not ok and type(err) == 'number' and err == 42\n"
			"end\n"
			"function onResult() require('lib.res_error') end\n");
	int Again = Load("nested_again.lua");
	TakeErrors();
	TOutcome AgainOutcome = Call(Again, "onAgain", RUN);
	CHECK(AgainOutcome.Thrown && AgainOutcome.Code == ERROR);
	Logged = TakeErrors();
	CHECK_CONTAINS(Logged, "Script error in nested_again.lua (onAgain): lib/n3.lua:2: n3 failed\nstack traceback:\n");
	CHECK(Occurrences(Logged, "stack traceback:") == 1);
	CHECK_CONTAINS(Traceback(Logged), "\tlib/n3.lua:2:");
	TOutcome Number = Call(Again, "onNumber", RUN_BOOLEAN);
	CHECK(!Number.Thrown && Number.Boolean);
	TOutcome Result = Call(Again, "onResult", RUN);
	CHECK(Result.Thrown && Result.Code == NOROOM);
	CHECK(TakeErrors() == "");
	WriteScript("num_top.lua", "require('lib.num_error')\n");
	std::string NumberLoaded = LoadError("num_top.lua");
	CHECK(StartsWith(NumberLoaded, "42\nstack traceback:\n"));
	CHECK_CONTAINS(Traceback(NumberLoaded), "\tlib/num_error.lua:1:");
}

// The message handlers call no metamethod of an error value: a __tostring
// that calls Game.rand() does not run (no rand() is consumed), and the value
// is named by its type.
static void TestErrorObject() {
	const char *Raise = "error(setmetatable({}, { __tostring = function() Game.rand() return 'called' end }))";
	WriteScript("tostring_error.lua", std::string("function onFail()\n  ") + Raise + "\nend\n");
	WriteScript("lib/tostring_error.lua", std::string(Raise) + "\n");
	WriteScript("tostring_module.lua", "function onFail()\n  require('lib.tostring_error')\nend\n");
	WriteScript("tostring_top.lua", std::string("local x = 1\n") + Raise + "\n");
	int Script = Load("tostring_error.lua");
	int Module = Load("tostring_module.lua");
	TakeErrors();

	srand(77);
	TOutcome Outcome = Call(Script, "onFail", RUN);
	int After = rand();
	srand(77);
	CHECK(After == rand());
	CHECK(Outcome.Thrown && Outcome.Code == ERROR);
	std::string Logged = TakeErrors();
	CHECK_CONTAINS(Logged, "Script error in tostring_error.lua (onFail): (error object is a table value)\nstack traceback:\n");
	CHECK_CONTAINS(Traceback(Logged), "tostring_error.lua:2:");
	CHECK(Logged.find("called") == std::string::npos);

	srand(77);
	TOutcome ModuleOutcome = Call(Module, "onFail", RUN);
	After = rand();
	srand(77);
	CHECK(After == rand());
	CHECK(ModuleOutcome.Thrown && ModuleOutcome.Code == ERROR);
	Logged = TakeErrors();
	CHECK_CONTAINS(Logged, "Script error in tostring_module.lua (onFail): (error object is a table value)\nstack traceback:\n");
	CHECK(Occurrences(Logged, "stack traceback:") == 1);
	CHECK_CONTAINS(Traceback(Logged), "lib/tostring_error.lua:1:");
	CHECK(Logged.find("called") == std::string::npos);

	srand(77);
	std::string Loaded = LoadError("tostring_top.lua");
	After = rand();
	srand(77);
	CHECK(After == rand());
	CHECK(StartsWith(Loaded, "(error object is a table value)\nstack traceback:\n"));
	CHECK_CONTAINS(Traceback(Loaded), "tostring_top.lua:2:");
}

static void TestCalls() {
	WriteScript("calls.lua",
			"function onCalls(id, words, flag, nothing, absent, negative)\n"
			"  test.record('first ' .. id)\n"
			"  test.record('second ' .. words)\n"
			"  test.record('third ' .. tostring(flag) .. ' ' .. tostring(nothing) .. ' ' .. tostring(absent))\n"
			"  test.record('fourth ' .. negative)\n"
			"end\n");
	int Script = Load("calls.lua");
	Records.clear();
	TScriptCall ScriptCall(Script, "onCalls");
	ScriptCall.addNumber(static_cast<uint32_t>(0x80000001u));
	ScriptCall.addString("utevo lux");
	ScriptCall.addBoolean(true);
	ScriptCall.addNil();
	ScriptCall.addString(nullptr);
	ScriptCall.addNumber(-17);
	CHECK(!Run(ScriptCall, RUN).Thrown);
	std::vector<std::string> Expected = {"first 2147483649", "second utevo lux", "third true nil nil",
			"fourth -17"};
	CHECK(Records == Expected);
	CHECK(TakeErrors() == "");

	WriteScript("calls2.lua",
			"function onCalls(a, b)\n"
			"  test.record('a ' .. a)\n"
			"  test.record('b ' .. b)\n"
			"  test.record('sum ' .. (a + b))\n"
			"end\n");
	int Script2 = Load("calls2.lua");
	Records.clear();
	TOutcome Outcome = Call(Script2, "onCalls", RUN, {3, 4});
	CHECK(!Outcome.Thrown);
	CHECK((Records == std::vector<std::string>{"a 3", "b 4", "sum 7"}));
	CHECK(TakeErrors() == "");

	// Many arguments (the array grows).
	WriteScript("many.lua",
			"function onSum(...)\n"
			"  local s = 0\n"
			"  for _, v in ipairs({...}) do s = s + v end\n"
			"  return s + select('#', ...) * 1000\n"
			"end\n");
	int Many = Load("many.lua");
	std::vector<int> Arguments;
	int Sum = 0;
	for (int i = 1; i <= 40; i++) {
		Arguments.push_back(i);
		Sum += i;
	}
	TOutcome ManyOutcome = Call(Many, "onSum", RUN_NUMBER, Arguments);
	CHECK(!ManyOutcome.Thrown && ManyOutcome.Number == Sum + 40000);

	// Game.print and debugPrint.
	WriteScript("print.lua", "function onPrint() Game.print(3, 'hello\\n'); debugPrint('bad %s thing\\n') end\n");
	int Print = Load("print.lua");
	Prints.clear();
	TakeErrors();
	TOutcome PrintOutcome = Call(Print, "onPrint", RUN);
	CHECK(!PrintOutcome.Thrown);
	CHECK((Prints == std::vector<std::string>{"3:hello\n"}));
	// Its '%' are doubled: error() hands the text to Log as its format.
	CHECK(TakeErrors() == "bad %%s thing\n");
}

// print(...): the arguments separated by tabs, a newline, at level 2; values
// that would show an address are refused.
static void TestPrint() {
	WriteScript("lua_print.lua",
			"function onPrint(p, c, item)\n"
			"  print('a', 1, true, nil, 2.5, -3)\n"
			"  print()\n"
			"  print(p, c, item, 'x % y')\n"
			"  local ok, err = pcall(test.exhaust)\n"
			"  print(err)\n"
			"end\n"
			"function onRefused(i)\n"
			"  local values = { {}, print, Game, coroutine.create(function() end), test.vocation(test.makeCreature(5)) }\n"
			"  local ok, err = pcall(print, 'before', values[i])\n"
			"  test.record(tostring(err))\n"
			"  return ok\n"
			"end\n");
	int Script = Load("lua_print.lua");
	Prints.clear();
	TScriptCall ScriptCall(Script, "onPrint");
	ScriptCall.addPlayer(7);
	ScriptCall.addCreature(0x40000001u);
	ScriptCall.addItem(9);
	CHECK(!Run(ScriptCall, RUN).Thrown);
	std::vector<std::string> Expected = {"2:a\t1\ttrue\tnil\t2.5\t-3\n", "2:\n",
			"2:Player 7\tCreature 1073741825\tItem 9\tx % y\n", "2:result RETURNVALUE_YOUAREEXHAUSTED\n"};
	CHECK(Prints == Expected);
	if (Prints != Expected) {
		for (const std::string &Text : Prints) {
			fprintf(stderr, "  printed: [%s]\n", Text.c_str());
		}
	}

	const char *Types[] = {"table", "function", "userdata", "thread"};
	for (int i = 1; i <= 4; i++) {
		Prints.clear();
		Records.clear();
		TOutcome Refused = Call(Script, "onRefused", RUN_BOOLEAN, {i});
		CHECK(!Refused.Thrown && !Refused.Boolean);
		CHECK(Prints.empty());
		CHECK(Records.size() == 1);
		if (Records.size() == 1) {
			CHECK_CONTAINS(Records[0], std::string("print: argument 2 is a ") + Types[i - 1] + ", which has no text");
		}
	}
	// A Vocation is an object: printed by its text.
	Prints.clear();
	Records.clear();
	TOutcome Vocation = Call(Script, "onRefused", RUN_BOOLEAN, {5});
	CHECK(!Vocation.Thrown && Vocation.Boolean);
	CHECK((Prints == std::vector<std::string>{"2:before\tVocation of Player 5\n"}));
	CHECK(TakeErrors() == "");
}

// addStringArray: the strings as a new array table per argument and call,
// Values[0] at index 1; Count 0 gives an empty table.
static void TestStringArray() {
	WriteScript("arrays.lua",
			"previous = false\n"
			"function onWords(words, again, empty, after)\n"
			"  test.record('count ' .. #words .. ' ' .. #again .. ' ' .. #empty .. ' ' .. after)\n"
			"  for i, w in ipairs(words) do test.record(i .. ' [' .. w .. ']') end\n"
			"  test.record('distinct ' .. tostring(not rawequal(words, again)))\n"
			"  test.record('new ' .. tostring(not rawequal(words, previous)))\n"
			"  test.record('first ' .. tostring(words[1]))\n"
			"  previous = words\n"
			"  words[1] = 'changed'\n"
			"end\n");
	int Script = Load("arrays.lua");
	char SpellStr[4][512] = {"exura", "sio", "Some Name", ""};
	const char *Words[3] = {SpellStr[1], SpellStr[2], SpellStr[3]};
	std::vector<std::string> Expected = {"count 3 3 0 7", "1 [sio]", "2 [Some Name]", "3 []", "distinct true",
			"new true", "first sio"};
	for (int Round = 0; Round < 2; Round++) {
		Records.clear();
		TScriptCall ScriptCall(Script, "onWords");
		ScriptCall.addStringArray(Words, 3);
		ScriptCall.addStringArray(Words, 3);
		ScriptCall.addStringArray(nullptr, 0);
		ScriptCall.addNumber(7);
		CHECK(!Run(ScriptCall, RUN).Thrown);
		// The second round gets a new table: the change of the first is not
		// seen, and the table is not the one kept.
		CHECK(Records == Expected);
		CHECK(TakeErrors() == "");
	}
	// The script changed its copy only.
	CHECK(strcmp(SpellStr[1], "sio") == 0);
}

// math.random and Game.rand give the numbers random() and rand() of the C++
// code give, in the same order.
static void TestRandom() {
	WriteScript("random.lua",
			"function onRandom(low, high) return math.random(low, high) end\n"
			"function onRandomOne(high) return math.random(high) end\n"
			"function onRand() return Game.rand() end\n"
			"function onRefused()\n"
			"  local ok1, e1 = pcall(math.random)\n"
			"  local ok2 = pcall(math.random, 1, 2, 3)\n"
			"  local ok3 = pcall(math.random, 1.5)\n"
			"  local ok4 = pcall(math.random, 1, 2^31)\n"
			"  local ok5 = pcall(math.random, 'x')\n"
			"  return not ok1 and not ok2 and not ok3 and not ok4 and not ok5\n"
			"    and string.find(e1, 'math.random: one or two arguments expected', 1, true) ~= nil\n"
			"end\n");
	int Script = Load("random.lua");
	// Kind 0: rand(); 1: random(Low, High); 2: random(1, High).
	struct TCase { int Kind; int Low; int High; };
	std::vector<TCase> Cases = {{1, 1, 100}, {0, 0, 0}, {1, -50, 50}, {2, 0, 6}, {1, 5, 5}, {1, 10, 1},
			{2, 0, 1}, {0, 0, 0}, {1, 0, 1}, {1, INT_MIN, INT_MAX}, {2, 0, 0}, {2, 0, -5}, {1, 0, INT_MAX},
			{2, 0, INT_MAX}, {1, 1, 6}, {2, 0, 100}};
	srand(1234);
	std::vector<int> Expected;
	for (int Round = 0; Round < 5; Round++) {
		for (const TCase &Case : Cases) {
			Expected.push_back(Case.Kind == 0 ? rand() : Case.Kind == 1 ? random(Case.Low, Case.High)
					: random(1, Case.High));
		}
	}
	srand(1234);
	std::vector<int> Got;
	bool Thrown = false;
	for (int Round = 0; Round < 5; Round++) {
		for (const TCase &Case : Cases) {
			TOutcome Outcome = Case.Kind == 0 ? Call(Script, "onRand", RUN_NUMBER)
					: Case.Kind == 1 ? Call(Script, "onRandom", RUN_NUMBER, {Case.Low, Case.High})
					: Call(Script, "onRandomOne", RUN_NUMBER, {Case.High});
			Thrown = Thrown || Outcome.Thrown;
			Got.push_back(Outcome.Number);
		}
	}
	CHECK(!Thrown);
	CHECK(Got == Expected);

	// The refused calls draw no number.
	srand(99);
	TOutcome Refused = Call(Script, "onRefused", RUN_BOOLEAN);
	int After = rand();
	srand(99);
	CHECK(After == rand());
	CHECK(!Refused.Thrown && Refused.Boolean);
	CHECK(TakeErrors() == "");
}

static void TestArithmetic() {
	WriteScript("arith.lua",
			"function onIdiv(a, b) return Game.idiv(a, b) end\n"
			"function onImod(a, b) return Game.imod(a, b) end\n"
			"function onDiv0()\n"
			"  local ok1, e1 = pcall(Game.idiv, 1, 0)\n"
			"  local ok2, e2 = pcall(Game.imod, 1, 0)\n"
			"  local ok3 = pcall(Game.idiv, 1.5, 1)\n"
			"  local ok4 = pcall(Game.idiv, 'x', 1)\n"
			"  return not ok1 and not ok2 and not ok3 and not ok4\n"
			"    and string.find(e1, 'division by zero', 1, true) ~= nil\n"
			"    and string.find(e2, 'division by zero', 1, true) ~= nil\n"
			"end\n");
	int Script = Load("arith.lua");
	int Pairs[][2] = {{7, 2}, {-7, 2}, {7, -2}, {-7, -2}, {0, 5}, {-1, 3}, {INT_MAX, -1}, {INT_MIN, 2},
			{INT_MIN, INT_MAX}, {-9, 10}, {100, 7}};
	for (auto &Pair : Pairs) {
		TOutcome Div = Call(Script, "onIdiv", RUN_NUMBER, {Pair[0], Pair[1]});
		TOutcome Mod = Call(Script, "onImod", RUN_NUMBER, {Pair[0], Pair[1]});
		CHECK(!Div.Thrown && Div.Number == Pair[0] / Pair[1]);
		CHECK(!Mod.Thrown && Mod.Number == Pair[0] % Pair[1]);
	}
	TOutcome Div0 = Call(Script, "onDiv0", RUN_BOOLEAN);
	CHECK(!Div0.Thrown && Div0.Boolean);
	TakeErrors();
	TOutcome Direct = Call(Script, "onIdiv", RUN_NUMBER, {1, 0});
	CHECK(Direct.Thrown && Direct.Code == ERROR);
	CHECK_CONTAINS(TakeErrors(), "Game.idiv: division by zero");
	TOutcome Overflow = Call(Script, "onIdiv", RUN_NUMBER, {INT_MIN, -1});
	CHECK(Overflow.Thrown && Overflow.Code == ERROR);
	CHECK_CONTAINS(TakeErrors(), "Game.idiv: overflow");
	TOutcome ModOverflow = Call(Script, "onImod", RUN_NUMBER, {INT_MIN, -1});
	CHECK(ModOverflow.Thrown && ModOverflow.Code == ERROR);
	CHECK_CONTAINS(TakeErrors(), "Game.imod: overflow");
}

static void TestRunResults() {
	WriteScript("returns.lua",
			"function onFive() return 5 end\n"
			"function onNegative() return -2147483648 end\n"
			"function onNil() end\n"
			"function onTrue() return true end\n"
			"function onFalse() return false end\n"
			"function onFraction() return 2.5 end\n"
			"function onBig() return 3000000000 end\n"
			"function onString() return 'x' end\n"
			"function onOne() return 1 end\n"
			"function onTwo() return 7, 8 end\n");
	int Script = Load("returns.lua");
	TakeErrors();
	TOutcome Five = Call(Script, "onFive", RUN_NUMBER);
	CHECK(!Five.Thrown && Five.Number == 5);
	TOutcome Negative = Call(Script, "onNegative", RUN_NUMBER);
	CHECK(!Negative.Thrown && Negative.Number == INT_MIN);
	TOutcome Two = Call(Script, "onTwo", RUN_NUMBER);
	CHECK(!Two.Thrown && Two.Number == 7);
	TOutcome NilNumber = Call(Script, "onNil", RUN_NUMBER);
	CHECK(!NilNumber.Thrown && NilNumber.Number == 0);
	TOutcome NilBoolean = Call(Script, "onNil", RUN_BOOLEAN);
	CHECK(!NilBoolean.Thrown && !NilBoolean.Boolean);
	TOutcome True = Call(Script, "onTrue", RUN_BOOLEAN);
	CHECK(!True.Thrown && True.Boolean);
	TOutcome False = Call(Script, "onFalse", RUN_BOOLEAN);
	CHECK(!False.Thrown && !False.Boolean);
	TOutcome Run = Call(Script, "onFive", RUN);
	CHECK(!Run.Thrown);
	CHECK(TakeErrors() == "");

	TOutcome Fraction = Call(Script, "onFraction", RUN_NUMBER);
	CHECK(Fraction.Thrown && Fraction.Code == ERROR);
	CHECK_CONTAINS(TakeErrors(), "Script error in returns.lua (onFraction): onFraction returned 2.5, not an integer");
	TOutcome Big = Call(Script, "onBig", RUN_NUMBER);
	CHECK(Big.Thrown && Big.Code == ERROR);
	CHECK_CONTAINS(TakeErrors(), "onBig returned 3000000000, not an integer");
	TOutcome StringNumber = Call(Script, "onString", RUN_NUMBER);
	CHECK(StringNumber.Thrown && StringNumber.Code == ERROR);
	CHECK_CONTAINS(TakeErrors(), "onString returned a string, not an integer");
	TOutcome BooleanNumber = Call(Script, "onTrue", RUN_NUMBER);
	CHECK(BooleanNumber.Thrown && BooleanNumber.Code == ERROR);
	CHECK_CONTAINS(TakeErrors(), "onTrue returned true, not an integer");
	TOutcome NumberBoolean = Call(Script, "onOne", RUN_BOOLEAN);
	CHECK(NumberBoolean.Thrown && NumberBoolean.Code == ERROR);
	CHECK_CONTAINS(TakeErrors(), "Script error in returns.lua (onOne): onOne returned 1, not a boolean");
}

// The objects: what they hold, their text, equality, methods, and the values
// refused where an object is expected.
static void TestObjects() {
	WriteScript("objects.lua",
			"function onObjects(p, c, item, none, p2, cp, item2)\n"
			"  test.record(tostring(p))\n"
			"  test.record(tostring(c))\n"
			"  test.record(tostring(item))\n"
			"  test.record(tostring(cp))\n"
			"  return none == nil and type(p) == 'userdata' and getmetatable(p) == false\n"
			"    and getmetatable(item) == false\n"
			"    and p == p2 and not rawequal(p, p2) and p == cp and p ~= c and item == item2\n"
			"    and p ~= test.makeItem(7) and item ~= test.makeCreature(9) and c == test.makeCreature(0x40000001)\n"
			"    and p:getId() == 7 and c:getId() == 0x40000001 and Player.getId(p) == 7 and Creature.getId(p) == 7\n"
			"    and test.creatureId(p) == 7 and test.creatureId(c) == 0x40000001 and test.playerId(p) == 7\n"
			"    and test.itemHandle(item) == 9 and test.thing(p) == 'creature 7' and test.thing(c) == 'creature 1073741825'\n"
			"    and test.thing(item) == 'item 9' and p:testName() == 'creature 7' and c:testName() == 'creature 1073741825'\n"
			"    and test.optionalCreature(nil) == -1 and test.optionalCreature() == -1 and test.optionalCreature(p) == 7\n"
			"    and test.makeCreature(0) == nil and tostring(test.makeCreature(7)) == 'Player 7'\n"
			"    and tostring(test.makeCreature(0x40000005)) == 'Creature 1073741829'\n"
			"    and tostring(test.vocation(p)) == 'Vocation of Player 7' and test.vocationPlayer(test.vocation(p)) == 7\n"
			"    and test.vocation(p) == test.vocation(p2) and test.vocation(p) ~= p\n"
			"end\n"
			"function onRefused(i, p, c, item)\n"
			"  local ok, result = pcall(test.exhaust)\n"
			"  local calls = {\n"
			"    function() return test.creatureId({}) end,\n"
			"    function() return test.creatureId(Game) end,\n"
			"    function() return test.creatureId(item) end,\n"
			"    function() return test.creatureId(5) end,\n"
			"    function() return test.creatureId(nil) end,\n"
			"    function() return test.creatureId(result) end,\n"
			"    function() return test.playerId(c) end,\n"
			"    function() return test.itemHandle(p) end,\n"
			"    function() return test.thing(test.vocation(p)) end,\n"
			"    function() return test.thing('x') end,\n"
			"    function() return test.optionalCreature({}) end,\n"
			"    function() return test.optionalCreature(Game) end,\n"
			"    function() return test.optionalCreature(item) end,\n"
			"    function() return Creature.getId({}) end,\n"
			"    function() return Creature.getId(item) end,\n"
			"    function() return item:getId() end,\n"
			"    function() return p.getId() end,\n"
			"    function() p.x = 1 end,\n"
			"    function() Player.getId = nil end,\n"
			"    function() return p:noSuchMethod() end,\n"
			"  }\n"
			"  local ok2, err = pcall(calls[i])\n"
			"  test.record(tostring(err))\n"
			"  return ok2\n"
			"end\n"
			"function onGone(c) test.record(tostring(c)) return c:testHealth() end\n");
	int Script = Load("objects.lua");
	TakeErrors();

	Records.clear();
	TScriptCall Objects(Script, "onObjects");
	Objects.addPlayer(7);
	Objects.addCreature(0x40000001u);
	Objects.addItem(9);
	Objects.addCreature(0);
	Objects.addPlayer(7);
	Objects.addCreature(7);
	Objects.addItem(9);
	TOutcome Outcome = Run(Objects, RUN_BOOLEAN);
	CHECK(!Outcome.Thrown && Outcome.Boolean);
	CHECK((Records == std::vector<std::string>{"Player 7", "Creature 1073741825", "Item 9", "Player 7"}));
	CHECK(TakeErrors() == "");

	const char *Refusals[] = {"Creature expected", "Creature expected", "Creature expected", "Creature expected",
			"Creature expected", "Creature expected", "Player expected", "Item expected",
			"Item or Creature expected", "Item or Creature expected", "Creature expected", "Creature expected",
			"Creature expected", "Creature expected",
			"Creature expected", "unknown name 'Item.getId'", "Creature expected", "Player.x is read-only",
			"Player.getId is read-only", "unknown name 'Player.noSuchMethod'"};
	int Count = static_cast<int>(sizeof(Refusals) / sizeof(Refusals[0]));
	for (int i = 1; i <= Count; i++) {
		Records.clear();
		TScriptCall Refused(Script, "onRefused");
		Refused.addNumber(i);
		Refused.addPlayer(7);
		Refused.addCreature(0x40000001u);
		Refused.addItem(9);
		TOutcome RefusedOutcome = Run(Refused, RUN_BOOLEAN);
		CHECK(!RefusedOutcome.Thrown && !RefusedOutcome.Boolean);
		if (RefusedOutcome.Thrown || RefusedOutcome.Boolean) {
			fprintf(stderr, "  refusal number %d was not refused\n", i);
		}
		CHECK(Records.size() == 1);
		if (Records.size() == 1) {
			CHECK_CONTAINS(Records[0], Refusals[i - 1]);
		}
	}
	CHECK(TakeErrors() == "");

	// A creature that is gone: the object stays (its text, its ID); the
	// binding logs and raises ERROR where the C++ code would dereference it.
	Alive = {12};
	Records.clear();
	TScriptCall Present(Script, "onGone");
	Present.addPlayer(12);
	TOutcome PresentOutcome = Run(Present, RUN_NUMBER);
	CHECK(!PresentOutcome.Thrown && PresentOutcome.Number == 100);
	Alive.clear();
	TScriptCall Gone(Script, "onGone");
	Gone.addPlayer(12);
	TOutcome GoneOutcome = Run(Gone, RUN_NUMBER);
	CHECK(GoneOutcome.Thrown && GoneOutcome.Code == ERROR);
	CHECK((Records == std::vector<std::string>{"Player 12", "Player 12"}));
	// ERROR is a result code: raised as one, not logged as a script error.
	CHECK(TakeErrors() == "Creature.testHealth: Creature does not exist.\n");
}

// The Npcs: made by TScriptCall::addNpc and a binding; an Npc is a Creature
// (its methods fall back to Creature's, a Creature parameter and an Item or
// Creature parameter take it, creature:getId gives its ID), equal only to an
// Npc of the same ID (not to the Creature of that ID), named "Npc <id>" by
// tostring and print, read-only with a hidden metatable; what an Npc
// parameter refuses.
static void TestNpcs() {
	WriteScript("npcs.lua",
			"function onNpc(npc, c, p, npc2)\n"
			"  print(npc)\n"
			"  return type(npc) == 'userdata' and getmetatable(npc) == false\n"
			"    and tostring(npc) == 'Npc 1073741825' and npc == npc2 and not rawequal(npc, npc2)\n"
			"    and npc == test.makeNpc(0x40000001) and npc ~= test.makeNpc(0x40000002)\n"
			"    and npc ~= c and c == test.makeCreature(0x40000001) and npc ~= p and npc ~= 0x40000001\n"
			"    and npc:getId() == 0x40000001 and Creature.getId(npc) == 0x40000001 and Npc.getId(npc) == 0x40000001\n"
			"    and npc:testName() == 'creature 1073741825' and Npc.testName(npc) == 'creature 1073741825'\n"
			"    and npc:testNpcName() == 'npc 1073741825' and test.npcId(npc) == 0x40000001\n"
			"    and test.creatureId(npc) == 0x40000001 and test.optionalCreature(npc) == 0x40000001\n"
			"    and test.thing(npc) == 'creature 1073741825'\n"
			"end\n"
			"function onRefused(i, npc, c, p)\n"
			"  local calls = {\n"
			"    function() return test.npcId(c) end,\n"
			"    function() return test.npcId(p) end,\n"
			"    function() return test.npcId(test.makeItem(9)) end,\n"
			"    function() return test.npcId(nil) end,\n"
			"    function() return test.npcId({}) end,\n"
			"    function() return test.playerId(npc) end,\n"
			"    function() return test.itemHandle(npc) end,\n"
			"    function() return c:testNpcName() end,\n"
			"    function() return Npc.testNpcName(c) end,\n"
			"    function() return npc:noSuchMethod() end,\n"
			"    function() npc.x = 1 end,\n"
			"    function() Npc.testNpcName = nil end,\n"
			"  }\n"
			"  local ok, err = pcall(calls[i])\n"
			"  test.record(tostring(err))\n"
			"  return ok\n"
			"end\n");
	int Script = Load("npcs.lua");
	TakeErrors();

	Prints.clear();
	TScriptCall Npcs(Script, "onNpc");
	Npcs.addNpc(0x40000001u);
	Npcs.addCreature(0x40000001u);
	Npcs.addPlayer(7);
	Npcs.addNpc(0x40000001u);
	TOutcome Outcome = Run(Npcs, RUN_BOOLEAN);
	CHECK(!Outcome.Thrown && Outcome.Boolean);
	CHECK((Prints == std::vector<std::string>{"2:Npc 1073741825\n"}));
	CHECK(TakeErrors() == "");

	const char *Refusals[] = {"Npc expected", "Npc expected", "Npc expected", "Npc expected", "Npc expected",
			"Player expected", "Item expected", "unknown name 'Creature.testNpcName'", "Npc expected",
			"unknown name 'Npc.noSuchMethod'", "Npc.x is read-only", "Npc.testNpcName is read-only"};
	int Count = static_cast<int>(sizeof(Refusals) / sizeof(Refusals[0]));
	for (int i = 1; i <= Count; i++) {
		Records.clear();
		TScriptCall Refused(Script, "onRefused");
		Refused.addNumber(i);
		Refused.addNpc(0x40000001u);
		Refused.addCreature(0x40000001u);
		Refused.addPlayer(7);
		TOutcome RefusedOutcome = Run(Refused, RUN_BOOLEAN);
		CHECK(!RefusedOutcome.Thrown && !RefusedOutcome.Boolean);
		if (RefusedOutcome.Thrown || RefusedOutcome.Boolean) {
			fprintf(stderr, "  Npc refusal number %d was not refused\n", i);
		}
		CHECK(Records.size() == 1);
		if (Records.size() == 1) {
			CHECK_CONTAINS(Records[0], Refusals[i - 1]);
		}
	}
	CHECK(TakeErrors() == "");
}

// The Monster objects (a Creature with a method table of its own), the
// MonsterTypes, the Impacts and the colors' arrays.
static void TestMonsterObjects() {
	WriteScript("monster_objects.lua",
			"function onMonster(m, c, p, n, m2)\n"
			"  print(m, test.makeMonsterType(12))\n"
			"  return type(m) == 'userdata' and getmetatable(m) == false\n"
			"    and tostring(m) == 'Monster 1073741830' and m == m2 and not rawequal(m, m2)\n"
			"    and m == test.makeMonster(0x40000006) and m ~= test.makeMonster(0x40000007)\n"
			"    and m ~= c and c == test.makeCreature(0x40000006) and m ~= n and m ~= p and m ~= 0x40000006\n"
			"    and m:getId() == 0x40000006 and Creature.getId(m) == 0x40000006 and Monster.getId(m) == 0x40000006\n"
			"    and m:testName() == 'creature 1073741830' and Monster.testName(m) == 'creature 1073741830'\n"
			"    and m:testMonsterName() == 'monster 1073741830' and test.monsterId(m) == 0x40000006\n"
			"    and test.creatureId(m) == 0x40000006 and test.optionalCreature(m) == 0x40000006\n"
			"    and test.thing(m) == 'creature 1073741830'\n"
			"    and tostring(test.makeMonsterType(12)) == 'MonsterType 12'\n"
			"    and test.makeMonsterType(12) == test.makeMonsterType(12)\n"
			"    and test.makeMonsterType(12) ~= test.makeMonsterType(13)\n"
			"    and test.monsterTypeRace(test.makeMonsterType(12)) == 12 and getmetatable(test.makeMonsterType(1)) == false\n"
			"end\n"
			"function onImpact(m)\n"
			"  local damage = test.makeImpact(0, m, 1, -20, 1, 0)\n"
			"  local outfit = test.makeImpact(6, m, 35, 4000, 0, 0)\n"
			"  print(damage, outfit)\n"
			"  return type(damage) == 'userdata' and getmetatable(damage) == false\n"
			"    and tostring(damage) == 'Impact damage' and tostring(test.makeImpact(7, m, 5, 2, 0, 0)) == 'Impact summon'\n"
			"    and damage == test.makeImpact(0, m, 1, -20, 1, 0) and not rawequal(damage, test.makeImpact(0, m, 1, -20, 1, 0))\n"
			"    and damage ~= test.makeImpact(0, m, 1, -21, 1, 0) and damage ~= test.makeImpact(1, m, 1, -20, 1, 0)\n"
			"    and damage ~= test.makeImpact(0, test.makeCreature(9), 1, -20, 1, 0) and damage ~= outfit\n"
			"    and test.impactText(damage) == '0 1073741830 1 -20 1 0 0000'\n"
			"    and test.impactText(outfit) == '6 1073741830 35 4000 0 0 1234'\n"
			"    and test.colors({1, 2, 255, -1}) == '1 2 255 -1'\n"
			"end\n"
			"function onRefused(i, m, c, p, n)\n"
			"  local damage = test.makeImpact(0, m, 1, 2, 3, 4)\n"
			"  local calls = {\n"
			"    function() return test.monsterId(c) end,\n"
			"    function() return test.monsterId(p) end,\n"
			"    function() return test.monsterId(n) end,\n"
			"    function() return test.monsterId(nil) end,\n"
			"    function() return test.npcId(m) end,\n"
			"    function() return test.playerId(m) end,\n"
			"    function() return c:testMonsterName() end,\n"
			"    function() return m:noSuchMethod() end,\n"
			"    function() m.x = 1 end,\n"
			"    function() return test.monsterTypeRace(m) end,\n"
			"    function() return test.creatureId(test.makeMonsterType(3)) end,\n"
			"    function() return test.impactText(m) end,\n"
			"    function() return test.impactText({}) end,\n"
			"    function() return test.creatureId(damage) end,\n"
			"    function() damage.kind = 1 end,\n"
			"    function() return damage:noSuchMethod() end,\n"
			"    function() return test.colors({1, 2, 3}) end,\n"
			"    function() return test.colors({1, 2, 3, 4, 5}) end,\n"
			"    function() return test.colors({1, 2, 3.5, 4}) end,\n"
			"    function() return test.colors({1, 2, '3', 4}) end,\n"
			"    function() return test.colors(string) end,\n"
			"    function() return test.colors(1234) end,\n"
			"  }\n"
			"  local ok, err = pcall(calls[i])\n"
			"  test.record(tostring(err))\n"
			"  return ok\n"
			"end\n");
	int Script = Load("monster_objects.lua");
	TakeErrors();

	Prints.clear();
	TScriptCall Monsters(Script, "onMonster");
	Monsters.addMonster(0x40000006u);
	Monsters.addCreature(0x40000006u);
	Monsters.addPlayer(7);
	Monsters.addNpc(0x40000006u);
	Monsters.addMonster(0x40000006u);
	TOutcome Outcome = Run(Monsters, RUN_BOOLEAN);
	CHECK(!Outcome.Thrown && Outcome.Boolean);
	CHECK((Prints == std::vector<std::string>{"2:Monster 1073741830\tMonsterType 12\n"}));
	CHECK(TakeErrors() == "");

	Prints.clear();
	TScriptCall Impacts(Script, "onImpact");
	Impacts.addMonster(0x40000006u);
	TOutcome ImpactOutcome = Run(Impacts, RUN_BOOLEAN);
	CHECK(!ImpactOutcome.Thrown && ImpactOutcome.Boolean);
	CHECK((Prints == std::vector<std::string>{"2:Impact damage\tImpact outfit\n"}));
	CHECK(TakeErrors() == "");

	const char *Refusals[] = {"Monster expected", "Monster expected", "Monster expected", "Monster expected",
			"Npc expected", "Player expected", "unknown name 'Creature.testMonsterName'",
			"unknown name 'Monster.noSuchMethod'", "Monster.x is read-only", "MonsterType expected",
			"Creature expected", "Impact expected", "Impact expected", "Creature expected",
			"Impact.kind is read-only", "unknown name 'Impact.noSuchMethod'", "array of four colors expected",
			"array of four colors expected", "array of four colors expected", "array of four colors expected",
			"array of four colors expected", "array of four colors expected"};
	int Count = static_cast<int>(sizeof(Refusals) / sizeof(Refusals[0]));
	for (int i = 1; i <= Count; i++) {
		Records.clear();
		TScriptCall Refused(Script, "onRefused");
		Refused.addNumber(i);
		Refused.addMonster(0x40000006u);
		Refused.addCreature(0x40000006u);
		Refused.addPlayer(7);
		Refused.addNpc(0x40000006u);
		TOutcome RefusedOutcome = Run(Refused, RUN_BOOLEAN);
		CHECK(!RefusedOutcome.Thrown && !RefusedOutcome.Boolean);
		if (RefusedOutcome.Thrown || RefusedOutcome.Boolean) {
			fprintf(stderr, "  Monster refusal number %d was not refused\n", i);
		}
		CHECK(Records.size() == 1);
		if (Records.size() == 1) {
			CHECK_CONTAINS(Records[0], Refusals[i - 1]);
		}
	}
	CHECK(TakeErrors() == "");
}

// addValue: numbers, texts, arrays and tables of a data file, nested, as Lua
// values (new tables per call).
static void TestValues() {
	WriteScript("values.lua",
			"local kept\n"
			"function onValue(n, t, a, empty, spell, again)\n"
			"  local fresh = kept == nil or not rawequal(kept, spell)\n"
			"  kept = spell\n"
			"  return n == -7 and type(n) == 'number' and t == 'fire\\0\\233' and type(a) == 'table'\n"
			"    and #a == 3 and a[1] == 1 and a[2] == 'two' and a[3][1] == 3 and #a[3] == 1\n"
			"    and type(empty) == 'table' and next(empty) == nil\n"
			"    and spell.shape == 'victim' and spell.range == 7 and spell.one_in == 2147483647\n"
			"    and spell.damage == -2147483648 and spell.outfit.id == 0 and spell.outfit.colors[4] == 255\n"
			"    and #spell.outfit.colors == 4 and spell.missing == nil and spell ~= again\n"
			"    and fresh\n"
			"end\n");
	int Script = Load("values.lua");
	TakeErrors();

	TScriptValue Number;
	Number.Kind = TScriptValue::NUMBER;
	Number.Number = -7;
	TScriptValue Text;
	Text.Kind = TScriptValue::TEXT;
	Text.Text = std::string("fire\0\xe9", 6);
	TScriptValue Three;
	Three.Kind = TScriptValue::NUMBER;
	Three.Number = 3;
	TScriptValue Inner;
	Inner.Kind = TScriptValue::ARRAY;
	Inner.Elements.push_back(Three);
	TScriptValue One;
	One.Kind = TScriptValue::NUMBER;
	One.Number = 1;
	TScriptValue Two;
	Two.Kind = TScriptValue::TEXT;
	Two.Text = "two";
	TScriptValue Array;
	Array.Kind = TScriptValue::ARRAY;
	Array.Elements = {One, Two, Inner};
	TScriptValue Empty;
	Empty.Kind = TScriptValue::TABLE;

	auto Field = [](const char *Key, TScriptValue Value) {
		return std::pair<std::string, TScriptValue>(Key, Value);
	};
	auto MakeNumber = [](int Value) {
		TScriptValue Result;
		Result.Kind = TScriptValue::NUMBER;
		Result.Number = Value;
		return Result;
	};
	TScriptValue Shape;
	Shape.Kind = TScriptValue::TEXT;
	Shape.Text = "victim";
	TScriptValue Colors;
	Colors.Kind = TScriptValue::ARRAY;
	Colors.Elements = {MakeNumber(1), MakeNumber(2), MakeNumber(3), MakeNumber(255)};
	TScriptValue Outfit;
	Outfit.Kind = TScriptValue::TABLE;
	Outfit.Fields = {Field("id", MakeNumber(0)), Field("colors", Colors)};
	TScriptValue Spell;
	Spell.Kind = TScriptValue::TABLE;
	Spell.Fields = {Field("shape", Shape), Field("range", MakeNumber(7)), Field("one_in", MakeNumber(INT_MAX)),
			Field("damage", MakeNumber(INT_MIN)), Field("outfit", Outfit)};

	for (int Round = 0; Round < 2; Round++) {
		TScriptCall Call(Script, "onValue");
		Call.addValue(Number);
		Call.addValue(Text);
		Call.addValue(Array);
		Call.addValue(Empty);
		Call.addValue(Spell);
		Call.addValue(Spell);
		TOutcome Outcome = Run(Call, RUN_BOOLEAN);
		CHECK(!Outcome.Thrown && Outcome.Boolean);
	}
	CHECK(TakeErrors() == "");
}

// runCaught: true for a call that ends, false for a non-result failure
// (logged as run logs it, failure() gives its message), the result code a
// handler raises thrown as run throws it.
static void TestRunCaught() {
	WriteScript("caught.lua",
			"function onFail(n) error('failed ' .. n) end\n"
			"function onResult() test.exhaust() end\n"
			"function onErrorResult() Game.throwResult(RETURNVALUE_ERROR) end\n"
			"function onFine(n) test.record('fine ' .. n) return 'ignored' end\n");
	int Script = Load("caught.lua");
	TakeErrors();

	TScriptCall Fail(Script, "onFail");
	Fail.addNumber(5);
	bool Thrown = false;
	bool Result = true;
	try {
		Result = Fail.runCaught();
	} catch (RESULT) {
		Thrown = true;
	}
	CHECK(!Thrown && !Result);
	CHECK(Fail.failure() != nullptr && std::string(Fail.failure()) == "caught.lua:1: failed 5");
	std::string Logged = TakeErrors();
	CHECK_CONTAINS(Logged, "Script error in caught.lua (onFail): caught.lua:1: failed 5");
	CHECK_CONTAINS(Logged, "stack traceback:");

	TScriptCall Missing(Script, "onMissing");
	Thrown = false;
	Result = true;
	try {
		Result = Missing.runCaught();
	} catch (RESULT) {
		Thrown = true;
	}
	CHECK(!Thrown && !Result);
	CHECK_CONTAINS(TakeErrors(), "Script error in caught.lua (onMissing): caught.lua has no function onMissing");

	const char *ResultHandlers[] = {"onResult", "onErrorResult"};
	RESULT Codes[] = {EXHAUSTED, ERROR};
	for (int i = 0; i < 2; i++) {
		TScriptCall Raised(Script, ResultHandlers[i]);
		Thrown = false;
		RESULT Code = NOERROR;
		try {
			Raised.runCaught();
		} catch (RESULT r) {
			Thrown = true;
			Code = r;
		}
		CHECK(Thrown && Code == Codes[i]);
		CHECK(Raised.failure() == nullptr);
	}
	CHECK(TakeErrors() == "");

	Records.clear();
	TScriptCall Fine(Script, "onFine");
	Fine.addNumber(3);
	Thrown = false;
	Result = false;
	try {
		Result = Fine.runCaught();
	} catch (RESULT) {
		Thrown = true;
	}
	CHECK(!Thrown && Result && Fine.failure() == nullptr);
	CHECK((Records == std::vector<std::string>{"fine 3"}));
	CHECK(TakeErrors() == "");
}

// What a hook call threw: NOERROR for none.
template<typename F>
static RESULT HookThrown(F &&Hook) {
	try {
		Hook();
	} catch (RESULT r) {
		return r;
	}
	return NOERROR;
}

// The hooks: each calls its handler with its arguments (a creature of 0 as
// nil), only when the script defines it, never for Script -1;
// a failure is logged and the caller carries on; a result code is thrown.
static void TestHooks() {
	const char *Text =
			"local function show(v)\n"
			"  if v == nil then return 'nil' end\n"
			"  return type(v) .. ':' .. tostring(v)\n"
			"end\n"
			"function onThink(monster) test.record('think ' .. show(monster)) end\n"
			"function onHealthChange(monster, attacker, damage)\n"
			"  test.record('health ' .. show(monster) .. ' ' .. show(attacker) .. ' ' .. show(damage))\n"
			"end\n"
			"function onTarget(monster, target) test.record('target ' .. show(monster) .. ' ' .. show(target)) end\n"
			"function onDeath(monster, killer)\n"
			"  test.record('death ' .. show(monster) .. ' ' .. show(killer))\n"
			"end\n"
			"function onRaid(raid) test.record('raid ' .. show(raid)) end\n"
			"function onWave(raid, wave) test.record('wave ' .. show(raid) .. ' ' .. show(wave)) end\n";
	WriteScript("monsters/full.lua", Text);
	WriteScript("monsters/think_only.lua", "function onThink(monster) test.record('only ' .. tostring(monster)) end\n"
			"onTarget = 'not a function'\n");
	WriteScript("monsters/failing.lua",
			"function onThink(monster) error('think failed') end\n"
			"function onHealthChange(monster) test.exhaust() end\n"
			"function onTarget(monster, target) test.record('after failure') end\n");
	int Full = Load("monsters/full.lua");
	int ThinkOnly = Load("monsters/think_only.lua");
	int Failing = Load("monsters/failing.lua");
	TakeErrors();

	Records.clear();
	CHECK(HookThrown([&] { MonsterThinkHook(Full, 0x40000010u); }) == NOERROR);
	CHECK(HookThrown([&] { MonsterHealthChangeHook(Full, 0x40000010u, 7, 25); }) == NOERROR);
	CHECK(HookThrown([&] { MonsterHealthChangeHook(Full, 0x40000010u, 0, 0); }) == NOERROR);
	CHECK(HookThrown([&] { MonsterHealthChangeHook(Full, 0x40000010u, 0x40000011u, -3); }) == NOERROR);
	CHECK(HookThrown([&] { MonsterTargetHook(Full, 0x40000010u, 8); }) == NOERROR);
	CHECK(HookThrown([&] { MonsterTargetHook(Full, 0x40000010u, 0x40000012u); }) == NOERROR);
	CHECK(HookThrown([&] { MonsterDeathHook(Full, 0x40000010u, 9); }) == NOERROR);
	CHECK(HookThrown([&] { MonsterDeathHook(Full, 0x40000010u, 0); }) == NOERROR);
	CHECK(HookThrown([&] { RaidHook(Full, "orc_raid"); }) == NOERROR);
	CHECK(HookThrown([&] { RaidWaveHook(Full, "orc_raid", 3); }) == NOERROR);
	std::vector<std::string> Expected = {
		"think userdata:Monster 1073741840",
		"health userdata:Monster 1073741840 userdata:Player 7 number:25",
		"health userdata:Monster 1073741840 nil number:0",
		"health userdata:Monster 1073741840 userdata:Creature 1073741841 number:-3",
		"target userdata:Monster 1073741840 userdata:Player 8",
		"target userdata:Monster 1073741840 userdata:Creature 1073741842",
		"death userdata:Monster 1073741840 userdata:Player 9",
		"death userdata:Monster 1073741840 nil",
		"raid string:orc_raid",
		"wave string:orc_raid number:3",
	};
	CHECK(Records == Expected);
	if (Records != Expected) {
		for (const std::string &Record : Records) {
			fprintf(stderr, "  hook record: %s\n", Record.c_str());
		}
	}
	CHECK(TakeErrors() == "");

	// Only the handlers the script defines (a global that is no function is
	// none), and nothing for Script -1.
	Records.clear();
	MonsterThinkHook(ThinkOnly, 0x40000010u);
	MonsterHealthChangeHook(ThinkOnly, 0x40000010u, 7, 1);
	MonsterTargetHook(ThinkOnly, 0x40000010u, 7);
	MonsterDeathHook(ThinkOnly, 0x40000010u, 7);
	RaidHook(ThinkOnly, "x");
	RaidWaveHook(ThinkOnly, "x", 1);
	MonsterThinkHook(-1, 0x40000010u);
	MonsterHealthChangeHook(-1, 0x40000010u, 7, 1);
	MonsterTargetHook(-1, 0x40000010u, 7);
	MonsterDeathHook(-1, 0x40000010u, 7);
	RaidHook(-1, "x");
	RaidWaveHook(-1, "x", 1);
	CHECK((Records == std::vector<std::string>{"only Monster 1073741840"}));
	CHECK(TakeErrors() == "");

	// A failure is logged and the caller goes on; a result code is thrown.
	Records.clear();
	CHECK(HookThrown([&] { MonsterThinkHook(Failing, 0x40000010u); }) == NOERROR);
	CHECK_CONTAINS(TakeErrors(), "Script error in monsters/failing.lua (onThink): monsters/failing.lua:1: think failed");
	CHECK(HookThrown([&] { MonsterHealthChangeHook(Failing, 0x40000010u, 7, 1); }) == EXHAUSTED);
	CHECK(TakeErrors() == "");
	CHECK(HookThrown([&] { MonsterTargetHook(Failing, 0x40000010u, 7); }) == NOERROR);
	CHECK((Records == std::vector<std::string>{"after failure"}));
	CHECK(TakeErrors() == "");
}

// The constructor of the ItemTypes in TestItemTypes: a negative number is
// logged and gives 0, as ObjectType(id) does for a number that is no type.
static int TestItemTypeConstructor(int TypeID) {
	if (TypeID < 0) {
		error("Invalid type %d.\n", TypeID);
		return 0;
	}
	return TypeID;
}

// The ItemTypes: made by ItemType(id) and a binding; ==, tostring and print,
// read-only, hidden metatable, methods; what an ItemType parameter refuses and
// what ItemType(...) refuses; the constructor SetItemTypeConstructor sets.
static void TestItemTypes() {
	WriteScript("itemtypes.lua",
			"function onItemType(item, p)\n"
			"  local t = ItemType(3031)\n"
			"  print(t, ItemType(-1))\n"
			"  return type(t) == 'userdata' and getmetatable(t) == false and t == ItemType(3031)\n"
			"    and not rawequal(t, ItemType(3031)) and t ~= ItemType(3032) and t ~= item and t ~= p\n"
			"    and ItemType(9) ~= test.makeItem(9) and ItemType(7) ~= p and t ~= 3031 and t ~= nil\n"
			"    and tostring(t) == 'ItemType 3031' and tostring(ItemType(-1)) == 'ItemType -1'\n"
			"    and test.itemTypeId(t) == 3031 and test.makeItemType(5) == ItemType(5)\n"
			"    and test.itemTypeId(ItemType(-2147483648)) == -2147483648 and t:testDouble() == 6062\n"
			"    and ItemType.testDouble(ItemType(4)) == 8\n"
			"end\n"
			"function onRefused(i, item)\n"
			"  local t = ItemType(1)\n"
			"  local calls = {\n"
			"    function() return ItemType() end,\n"
			"    function() return ItemType(1, 2) end,\n"
			"    function() return ItemType(1.5) end,\n"
			"    function() return ItemType('1') end,\n"
			"    function() return ItemType(2147483648) end,\n"
			"    function() return test.itemTypeId(item) end,\n"
			"    function() return test.itemTypeId(1) end,\n"
			"    function() return test.itemTypeId(nil) end,\n"
			"    function() return test.itemHandle(t) end,\n"
			"    function() return test.thing(t) end,\n"
			"    function() t.x = 1 end,\n"
			"    function() ItemType.testDouble = nil end,\n"
			"    function() return t:noSuchMethod() end,\n"
			"    function() return ItemType.testDouble(item) end,\n"
			"  }\n"
			"  local ok, err = pcall(calls[i])\n"
			"  test.record(tostring(err))\n"
			"  return ok\n"
			"end\n"
			"function onMadeNegative() return test.itemTypeId(ItemType(-5)) end\n"
			"function onMadePositive() return test.itemTypeId(ItemType(17)) end\n");
	int Script = Load("itemtypes.lua");
	TakeErrors();

	Prints.clear();
	TScriptCall ItemTypes(Script, "onItemType");
	ItemTypes.addItem(3031);
	ItemTypes.addPlayer(3031);
	TOutcome Outcome = Run(ItemTypes, RUN_BOOLEAN);
	CHECK(!Outcome.Thrown && Outcome.Boolean);
	CHECK((Prints == std::vector<std::string>{"2:ItemType 3031	ItemType -1\n"}));
	CHECK(TakeErrors() == "");

	const char *Refusals[] = {"one argument expected", "one argument expected", "integer expected",
			"number expected", "integer expected", "ItemType expected", "ItemType expected", "ItemType expected",
			"Item expected", "Item or Creature expected", "ItemType.x is read-only", "ItemType.testDouble is read-only",
			"unknown name 'ItemType.noSuchMethod'", "ItemType expected"};
	int Count = static_cast<int>(sizeof(Refusals) / sizeof(Refusals[0]));
	for (int i = 1; i <= Count; i++) {
		Records.clear();
		TScriptCall Refused(Script, "onRefused");
		Refused.addNumber(i);
		Refused.addItem(9);
		TOutcome RefusedOutcome = Run(Refused, RUN_BOOLEAN);
		CHECK(!RefusedOutcome.Thrown && !RefusedOutcome.Boolean);
		if (RefusedOutcome.Thrown || RefusedOutcome.Boolean) {
			fprintf(stderr, "  item type refusal number %d was not refused\n", i);
		}
		CHECK(Records.size() == 1);
		if (Records.size() == 1) {
			CHECK_CONTAINS(Records[0], Refusals[i - 1]);
		}
	}
	CHECK(TakeErrors() == "");

	// The constructor makes the number once, when the object is made: it
	// logs there, and the object holds what it gave.
	SetItemTypeConstructor(&TestItemTypeConstructor);
	TOutcome Negative = Call(Script, "onMadeNegative", RUN_NUMBER);
	CHECK(!Negative.Thrown && Negative.Number == 0);
	CHECK(TakeErrors() == "Invalid type -5.\n");
	TOutcome Positive = Call(Script, "onMadePositive", RUN_NUMBER);
	CHECK(!Positive.Thrown && Positive.Number == 17);
	CHECK(TakeErrors() == "");
	SetItemTypeConstructor(nullptr);
	TOutcome Unchanged = Call(Script, "onMadeNegative", RUN_NUMBER);
	CHECK(!Unchanged.Thrown && Unchanged.Number == -5);
	CHECK(TakeErrors() == "");
}

// The Positions: made by TScriptCall::addPosition, Position(x, y, z) and a
// binding; their fields, ==, tostring and print, read-only, hidden metatable;
// what a Position parameter refuses. addItem(0) gives nil.
static void TestPositions() {
	WriteScript("positions.lua",
			"function onPosition(p, q, item)\n"
			"  print(p, Position(-1, 0, 15))\n"
			"  return type(p) == 'userdata' and p.x == 32000 and p.y == 31000 and p.z == 7\n"
			"    and p == q and not rawequal(p, q) and getmetatable(p) == false\n"
			"    and tostring(p) == 'Position(32000, 31000, 7)' and tostring(Position(-1, -2, -3)) == 'Position(-1, -2, -3)'\n"
			"    and Position(1, 2, 3) == Position(1, 2, 3) and Position(1, 2, 3) ~= Position(1, 2, 4)\n"
			"    and Position(1, 2, 3) ~= Position(0, 2, 3) and Position(1, 2, 3) ~= Position(1, 0, 3)\n"
			"    and p ~= item and p ~= nil and p ~= 32000\n"
			"    and test.positionText(p) == '32000 31000 7' and test.makePosition(1, 2, 3) == Position(1, 2, 3)\n"
			"    and test.positionText(Position(2147483647, -2147483648, 0)) == '2147483647 -2147483648 0'\n"
			"end\n"
			"function onRefused(i, p, item)\n"
			"  local calls = {\n"
			"    function() p.x = 1 end,\n"
			"    function() return p.w end,\n"
			"    function() return p[1] end,\n"
			"    function() return Position(1, 2) end,\n"
			"    function() return Position(1, 2, 3, 4) end,\n"
			"    function() return Position(1.5, 2, 3) end,\n"
			"    function() return Position('1', 2, 3) end,\n"
			"    function() return Position(1, 2, 2147483648) end,\n"
			"    function() return test.positionText(item) end,\n"
			"    function() return test.positionText({x = 1, y = 2, z = 3}) end,\n"
			"    function() return test.positionText(nil) end,\n"
			"    function() return test.itemHandle(p) end,\n"
			"  }\n"
			"  local ok, err = pcall(calls[i])\n"
			"  test.record(tostring(err))\n"
			"  return ok\n"
			"end\n"
			"function onNil(...)\n"
			"  return select('#', ...) == 3 and select(1, ...) == nil and tostring((select(2, ...))) == 'Item 9'\n"
			"    and select(3, ...) == nil\n"
			"end\n");
	int Script = Load("positions.lua");
	TakeErrors();

	Prints.clear();
	TScriptCall Positions(Script, "onPosition");
	Positions.addPosition(32000, 31000, 7);
	Positions.addPosition(32000, 31000, 7);
	Positions.addItem(9);
	TOutcome Outcome = Run(Positions, RUN_BOOLEAN);
	CHECK(!Outcome.Thrown && Outcome.Boolean);
	CHECK((Prints == std::vector<std::string>{"2:Position(32000, 31000, 7)\tPosition(-1, 0, 15)\n"}));
	CHECK(TakeErrors() == "");

	const char *Refusals[] = {"Position.x is read-only", "unknown name 'Position.w'", "unknown name 'Position.1'",
			"three arguments expected", "three arguments expected", "integer expected", "number expected",
			"integer expected", "Position expected", "Position expected", "Position expected", "Item expected"};
	int Count = static_cast<int>(sizeof(Refusals) / sizeof(Refusals[0]));
	for (int i = 1; i <= Count; i++) {
		Records.clear();
		TScriptCall Refused(Script, "onRefused");
		Refused.addNumber(i);
		Refused.addPosition(1, 2, 3);
		Refused.addItem(9);
		TOutcome RefusedOutcome = Run(Refused, RUN_BOOLEAN);
		CHECK(!RefusedOutcome.Thrown && !RefusedOutcome.Boolean);
		if (RefusedOutcome.Thrown || RefusedOutcome.Boolean) {
			fprintf(stderr, "  position refusal number %d was not refused\n", i);
		}
		CHECK(Records.size() == 1);
		if (Records.size() == 1) {
			CHECK_CONTAINS(Records[0], Refusals[i - 1]);
		}
	}
	CHECK(TakeErrors() == "");

	TScriptCall Nil(Script, "onNil");
	Nil.addItem(0);
	Nil.addItem(9);
	Nil.addItem(0);
	TOutcome NilOutcome = Run(Nil, RUN_BOOLEAN);
	CHECK(!NilOutcome.Thrown && NilOutcome.Boolean);
}

// TScriptEvent: the rule number (or none), the event's name and the three
// objects (nil for 0), the same values for every call; results as
// runBoolean; errors; an event inside a call; the stack as before.
static void TestEvents() {
	WriteScript("events.lua",
			"seen = {}\n"
			"function onRule(rule, event, user, item1, item2)\n"
			"  test.record(tostring(rule) .. ' ' .. event .. ' ' .. tostring(user) .. ' ' .. tostring(item1) .. ' '\n"
			"      .. tostring(item2))\n"
			"  if rule == 1 then seen[1] = item1 end\n"
			"  if rule == 2 then return seen[1] == item1 and rawequal(seen[1], item1) end\n"
			"  if rule == 3 then return nil end\n"
			"  if rule == 4 then return 1 end\n"
			"  if rule == 5 then test.exhaust() end\n"
			"  if rule == 6 then error('rule six') end\n"
			"  if rule == 8 then return test.nestedEvent() end\n"
			"  return rule == 7\n"
			"end\n"
			"function onCheck(...)\n"
			"  test.record(select('#', ...) .. ' ' .. table.concat({tostring((select(1, ...))), tostring((select(2, ...))),\n"
			"      tostring((select(3, ...))), tostring((select(4, ...)))}, ' '))\n"
			"  return true\n"
			"end\n"
			"function onRun(...) test.record('run ' .. select('#', ...)) end\n");
	int Script = Load("events.lua");
	NestedEventScript = Script;
	lua_State *L = ScriptState().lua_state();
	int Top = lua_gettop(L);
	TakeErrors();
	Records.clear();
	{
		TScriptEvent Event("collision", 0, 9, 10);
		CHECK(lua_gettop(L) == Top + 3);
		CHECK(!Event.check(Script, "onRule", 1));
		CHECK(Event.check(Script, "onRule", 2));
		CHECK(!Event.check(Script, "onRule", 3));
		CHECK(Event.check(Script, "onRule", 7));
		CHECK(Event.check(Script, "onCheck", 0));
		Event.run(Script, "onRun");
		CHECK(Event.check(Script, "onRule", 8));
		CHECK(lua_gettop(L) == Top + 3);
		CHECK(TakeErrors() == "");

		bool Thrown = false;
		try {
			Event.check(Script, "onRule", 4);
		} catch (RESULT r) {
			Thrown = r == ERROR;
		}
		CHECK(Thrown);
		CHECK_CONTAINS(TakeErrors(), "Script error in events.lua (onRule): onRule returned 1, not a boolean");
		Thrown = false;
		try {
			Event.check(Script, "onRule", 5);
		} catch (RESULT r) {
			Thrown = r == EXHAUSTED;
		}
		CHECK(Thrown);
		CHECK(TakeErrors() == "");
		Thrown = false;
		try {
			Event.check(Script, "onRule", 6);
		} catch (RESULT r) {
			Thrown = r == ERROR;
		}
		CHECK(Thrown);
		std::string Logged = TakeErrors();
		CHECK(StartsWith(Logged, "Script error in events.lua (onRule): events.lua:10: rule six"));
		CHECK_CONTAINS(Logged, "stack traceback:");
		Thrown = false;
		try {
			Event.check(Script, "noSuchHandler", 1);
		} catch (RESULT r) {
			Thrown = r == ERROR;
		}
		CHECK(Thrown);
		CHECK_CONTAINS(TakeErrors(), "events.lua has no function noSuchHandler");
		CHECK(lua_gettop(L) == Top + 3);
	}
	CHECK(lua_gettop(L) == Top);
	CHECK((Records == std::vector<std::string>{
			"1 collision nil Item 9 Item 10",
			"2 collision nil Item 9 Item 10",
			"3 collision nil Item 9 Item 10",
			"7 collision nil Item 9 Item 10",
			"4 collision nil Item 9 Item 10",
			"run 4",
			"8 collision nil Item 9 Item 10",
			"7 separation nil Item 21 nil",
			"4 collision nil Item 9 Item 10",
			"5 collision nil Item 9 Item 10",
			"6 collision nil Item 9 Item 10",
	}));
}

// TScriptCall::failure: Lua's message of a failed call without the traceback;
// nullptr after a call that ran or raised a result code.
static void TestFailure() {
	WriteScript("failure.lua",
			"function onFail() error('it failed') end\n"
			"function onResult() test.exhaust() end\n"
			"function onFine() end\n");
	int Script = Load("failure.lua");
	TScriptCall Fail(Script, "onFail");
	CHECK(Fail.failure() == nullptr);
	TOutcome FailOutcome = Run(Fail, RUN);
	CHECK(FailOutcome.Thrown && FailOutcome.Code == ERROR);
	CHECK(Fail.failure() != nullptr && std::string(Fail.failure()) == "failure.lua:1: it failed");
	CHECK_CONTAINS(TakeErrors(), "stack traceback:");
	TScriptCall Result(Script, "onResult");
	TOutcome ResultOutcome = Run(Result, RUN);
	CHECK(ResultOutcome.Thrown && ResultOutcome.Code == EXHAUSTED);
	CHECK(Result.failure() == nullptr);
	TScriptCall Fine(Script, "onFine");
	TOutcome FineOutcome = Run(Fine, RUN);
	CHECK(!FineOutcome.Thrown && Fine.failure() == nullptr);
	CHECK(TakeErrors() == "");
}

// The constants are read-only, also as a script's own global; SetScriptConstant
// refuses a name that is taken.
static void TestConstants() {
	CHECK(SharedNumber("CONST_ME_FIREAREA") == 7);
	WriteScript("constant_top.lua", "local x = CONST_ME_FIREAREA\nCONST_ME_FIREAREA = 1\n");
	CHECK_CONTAINS(LoadError("constant_top.lua", "data/x.toml:2:3"),
			"data/x.toml:2:3: constant_top.lua:2: CONST_ME_FIREAREA is read-only");
	WriteScript("constant_run.lua",
			"function onWrite() RETURNVALUE_NOTENOUGHROOM = 5 end\n"
			"function onRead() local CONST_ME_FIREAREA = 1 return CONST_ME_FIREAREA end\n"
			"function onValue() return CONST_ME_FIREAREA end\n");
	int Script = Load("constant_run.lua");
	TakeErrors();
	TOutcome Write = Call(Script, "onWrite", RUN);
	CHECK(Write.Thrown && Write.Code == ERROR);
	CHECK_CONTAINS(TakeErrors(), "constant_run.lua:1: RETURNVALUE_NOTENOUGHROOM is read-only");
	TOutcome Local = Call(Script, "onRead", RUN_NUMBER);
	CHECK(!Local.Thrown && Local.Number == 1);
	TOutcome Value = Call(Script, "onValue", RUN_NUMBER);
	CHECK(!Value.Thrown && Value.Number == 7);
	CHECK(SharedNumber("RETURNVALUE_NOTENOUGHROOM") == 4);

	const char *Taken[] = {"CONST_ME_FIREAREA", "RETURNVALUE_NOERROR", "Game", "Player", "print", "debugPrint",
			"pcall", "dofile", "io"};
	for (const char *Name : Taken) {
		std::string Text;
		try {
			SetScriptConstant(Name, 1);
		} catch (const char *s) {
			Text = s;
		}
		CHECK(Text == std::string("SetScriptConstant: the name ") + Name + " is taken or removed by the sandbox");
	}
	CHECK(SharedNumber("CONST_ME_FIREAREA") == 7);
	CHECK(TakeErrors() == "");
}

static void TestEnvironments() {
	WriteScript("env_a.lua",
			"shared_name = 1\n"
			"local function get() return shared_name end\n"
			"onGet = get\n"
			"function onSet(v) shared_name = v end\n");
	WriteScript("env_b.lua", "function onGet() return shared_name end\n");
	WriteScript("env_c.lua", "Game = 5\nfunction onGame() return Game end\n");
	// The shared tables reach scripts as read-only proxies, userdata.
	WriteScript("env_d.lua",
			"function onGame() return type(Game) == 'userdata' and type(Player) == 'userdata' end\n");
	int A = Load("env_a.lua");
	int B = Load("env_b.lua");
	int C = Load("env_c.lua");
	int D = Load("env_d.lua");
	TakeErrors();
	TOutcome GetA = Call(A, "onGet", RUN_NUMBER);
	CHECK(!GetA.Thrown && GetA.Number == 1);
	TOutcome SetA = Call(A, "onSet", RUN, {9});
	CHECK(!SetA.Thrown);
	TOutcome GetA2 = Call(A, "onGet", RUN_NUMBER);
	CHECK(!GetA2.Thrown && GetA2.Number == 9);
	TOutcome GetB = Call(B, "onGet", RUN_NUMBER);
	CHECK(GetB.Thrown && GetB.Code == ERROR);
	CHECK_CONTAINS(TakeErrors(), "env_b.lua:1: unknown name 'shared_name'");
	TOutcome GameC = Call(C, "onGame", RUN_NUMBER);
	CHECK(!GameC.Thrown && GameC.Number == 5);
	TOutcome GameD = Call(D, "onGame", RUN_BOOLEAN);
	CHECK(!GameD.Thrown && GameD.Boolean);
	CHECK(TakeErrors() == "");

	// ScriptAPITable gives the same table again.
	{
		sol::table First = ScriptAPITable("Extra");
		First.set_function("seven", Guarded(+[]() -> int { return 7; }));
		sol::table Second = ScriptAPITable("Extra");
		CHECK(Second.raw_get<sol::object>("seven").get_type() == sol::type::function);
	}
	WriteScript("extra.lua", "function onSeven() return Extra.seven() end\n");
	TOutcome Seven = Call(Load("extra.lua"), "onSeven", RUN_NUMBER);
	CHECK(!Seven.Thrown && Seven.Number == 7);

	// An API table made after scripts were loaded is read-only to them too.
	{
		sol::table Late = ScriptAPITable("Late");
		Late.set_function("eight", Guarded(+[]() -> int { return 8; }));
	}
	WriteScript("late.lua",
			"function onLate()\n"
			"  local ok, err = pcall(function() Late.eight = nil end)\n"
			"  return not ok and string.find(err, 'Late.eight is read-only', 1, true) ~= nil and Late.eight() == 8\n"
			"    and getmetatable(Late) == false\n"
			"end\n");
	TOutcome LateOutcome = Call(Load("late.lua"), "onLate", RUN_BOOLEAN);
	CHECK(!LateOutcome.Thrown && LateOutcome.Boolean);

	// ScriptAPITable refuses the shared globals that are not API tables.
	const char *NotAPI[] = {"string", "table", "math", "bit", "coroutine", "require", "pcall", "print", "debugPrint",
			"RETURNVALUE_NOERROR", "CONST_ME_FIREAREA"};
	for (const char *Name : NotAPI) {
		std::string Text;
		try {
			ScriptAPITable(Name);
		} catch (const char *s) {
			Text = s;
		}
		CHECK_CONTAINS(Text, std::string("ScriptAPITable: the shared global ") + Name + " is not an API table");
	}

	// It refuses the names the sandbox removed or never opened, so a binding
	// unit cannot bring one back.
	const char *Removed[] = {"dofile", "loadfile", "load", "loadstring", "_G", "getfenv", "setfenv", "rawset",
			"newproxy", "collectgarbage", "gcinfo", "jit", "io", "os", "debug", "ffi", "package", "module"};
	int Number = 0;
	for (const char *Name : Removed) {
		std::string Text;
		try {
			ScriptAPITable(Name);
		} catch (const char *s) {
			Text = s;
		}
		CHECK(Text == std::string("ScriptAPITable: the name ") + Name + " is removed by the sandbox");
		std::string Script = "removed_" + std::to_string(Number++) + ".lua";
		WriteScript(Script, std::string("local x = ") + Name + "\n");
		CHECK_CONTAINS(LoadError(Script.c_str()), Script + ":1: unknown name '" + Name + "'");
	}
	CHECK(TakeErrors() == "");
}

// The shared tables are read-only to scripts (rule 7 of scripting.hh).
static void TestReadOnly() {
	WriteScript("readonly.lua",
			"local writes = {\n"
			"  function() Game.x = 1 end,\n"
			"  function() Game.rand = nil end,\n"
			"  function() RETURNVALUE_NOTENOUGHROOM = 5 end,\n"
			"  function() string.upper = nil end,\n"
			"  function() table.insert = nil end,\n"
			"  function() math.floor = nil end,\n"
			"  function() bit.band = nil end,\n"
			"  function() coroutine.wrap = nil end,\n"
			"  function() test.count = nil end,\n"
			"  function() Creature.getId = nil end,\n"
			"  function() table.insert(Game, 'x') end,\n"
			"  function() table.insert(string, 'x') end,\n"
			"  function() getmetatable('').__index = {} end,\n"
			"  function() setmetatable(Game, {}) end,\n"
			"  function() setmetatable(string, {}) end,\n"
			"  function() math.random = nil end,\n"
			"  function() CONST_ME_FIREAREA = 1 end,\n"
			"  function() table.insert(Player, 'x') end,\n"
			"}\n"
			"function onWrite(i)\n"
			"  local ok, err = pcall(writes[i])\n"
			"  test.record(tostring(err))\n"
			"  return ok\n"
			"end\n");
	int Script = Load("readonly.lua");
	const char *Expected[] = {
		"readonly.lua:2: Game.x is read-only",
		"readonly.lua:3: Game.rand is read-only",
		"readonly.lua:4: RETURNVALUE_NOTENOUGHROOM is read-only",
		"readonly.lua:5: string.upper is read-only",
		"readonly.lua:6: table.insert is read-only",
		"readonly.lua:7: math.floor is read-only",
		"readonly.lua:8: bit.band is read-only",
		"readonly.lua:9: coroutine.wrap is read-only",
		"readonly.lua:10: test.count is read-only",
		"readonly.lua:11: Creature.getId is read-only",
		"table expected",
		"table expected",
		"readonly.lua:14:",
		"table expected",
		"table expected",
		"readonly.lua:17: math.random is read-only",
		"readonly.lua:18: CONST_ME_FIREAREA is read-only",
		"table expected",
	};
	int Count = static_cast<int>(sizeof(Expected) / sizeof(Expected[0]));
	for (int i = 0; i < Count; i++) {
		Records.clear();
		TOutcome Outcome = Call(Script, "onWrite", RUN_BOOLEAN, {i + 1});
		CHECK(!Outcome.Thrown && !Outcome.Boolean);
		CHECK(Records.size() == 1);
		if (Records.size() == 1) {
			CHECK_CONTAINS(Records[0], Expected[i]);
		}
	}

	// Another script sees every value as it was.
	WriteScript("readonly_check.lua",
			"function onCheck()\n"
			"  return type(Game.rand) == 'function' and RETURNVALUE_NOTENOUGHROOM == 4 and string.upper('a') == 'A'\n"
			"    and type(table.insert) == 'function' and math.floor(2.5) == 2 and bit.band(6, 3) == 2\n"
			"    and type(coroutine.wrap) == 'function' and type(test.count) == 'function'\n"
			"    and type(Creature.getId) == 'function' and type(math.random) == 'function'\n"
			"    and CONST_ME_FIREAREA == 7 and not pcall(function() return Game.x end)\n"
			"    and string[1] == nil and ('x'):upper() == 'X' and ('x').upper == string.upper\n"
			"end\n"
			"function onMeta()\n"
			"  return getmetatable('') == false and getmetatable(Game) == false and getmetatable(Creature) == false\n"
			"    and getmetatable(string) == false and getmetatable(table) == false and getmetatable(math) == false\n"
			"    and getmetatable(bit) == false and getmetatable(coroutine) == false and getmetatable(test) == false\n"
			"    and getmetatable(Player) == false and getmetatable(Item) == false and getmetatable(Vocation) == false\n"
			"end\n");
	int Check = Load("readonly_check.lua");
	TOutcome Values = Call(Check, "onCheck", RUN_BOOLEAN);
	CHECK(!Values.Thrown && Values.Boolean);
	TOutcome Meta = Call(Check, "onMeta", RUN_BOOLEAN);
	CHECK(!Meta.Thrown && Meta.Boolean);

	// A write at a script's top level fails its load.
	WriteScript("readonly_top.lua", "local x = 1\nstring.upper = string.lower\n");
	CHECK_CONTAINS(LoadError("readonly_top.lua"), "readonly_top.lua:2: string.upper is read-only");
	TOutcome After = Call(Check, "onCheck", RUN_BOOLEAN);
	CHECK(!After.Thrown && After.Boolean);

	// A script's global assignment stays in its own environment: the shared
	// globals are not written, so no other script sees the name.
	WriteScript("global_set.lua",
			"pcall = 1\nnew_name = 2\nfunction onGet() return pcall + new_name end\n");
	WriteScript("global_get.lua",
			"function onGet()\n"
			"  return type(pcall) == 'function' and not pcall(function() return new_name end)\n"
			"end\n");
	TOutcome Set = Call(Load("global_set.lua"), "onGet", RUN_NUMBER);
	CHECK(!Set.Thrown && Set.Number == 3);
	TOutcome Get = Call(Load("global_get.lua"), "onGet", RUN_BOOLEAN);
	CHECK(!Get.Thrown && Get.Boolean);
	CHECK(TakeErrors() == "");
}

// Bindings wrapped with Guarded that throw a text, and sol's argument checks:
// logged non-result errors, ERROR.
static void TestGuarded() {
	WriteScript("guarded.lua",
			"function onText()\n"
			"  test.throwText()\n"
			"end\n"
			"function onException()\n"
			"  test.throwException()\n"
			"end\n"
			"function onWrongType()\n"
			"  local v = test.twice('x')\n"
			"  return v\n"
			"end\n"
			"function onCaught()\n"
			"  local ok, err = pcall(test.throwText)\n"
			"  return not ok and Game.getResultOf(err) == nil and string.find(err, 'plain text', 1, true) ~= nil\n"
			"end\n");
	int Script = Load("guarded.lua");
	TakeErrors();
	TOutcome Text = Call(Script, "onText", RUN);
	CHECK(Text.Thrown && Text.Code == ERROR);
	std::string Logged = TakeErrors();
	CHECK(StartsWith(Logged, "Script error in guarded.lua (onText): "));
	CHECK_CONTAINS(Logged, "Script error in guarded.lua (onText): guarded.lua:2: plain text\nstack traceback:\n");

	TOutcome Exception = Call(Script, "onException", RUN);
	CHECK(Exception.Thrown && Exception.Code == ERROR);
	CHECK_CONTAINS(TakeErrors(),
			"Script error in guarded.lua (onException): guarded.lua:5: what text\nstack traceback:\n");

	TOutcome Wrong = Call(Script, "onWrongType", RUN_NUMBER);
	CHECK(Wrong.Thrown && Wrong.Code == ERROR);
	Logged = TakeErrors();
	// sol's message has no position of its own; the traceback has the line.
	CHECK_CONTAINS(Logged, "Script error in guarded.lua (onWrongType): stack index 1, expected number");
	CHECK_CONTAINS(Logged, "\nstack traceback:\n");
	CHECK_CONTAINS(Logged, "guarded.lua:8:");

	TOutcome Caught = Call(Script, "onCaught", RUN_BOOLEAN);
	CHECK(!Caught.Thrown && Caught.Boolean);
	CHECK(TakeErrors() == "");
}

// Every text from a script or from Lua that the engine logs, or LoadScript
// throws, has each '%' doubled (scripting.hh, "Logged texts").
static void TestPercent() {
	WriteScript("percent.lua",
			"function onLog() debugPrint('a % b %s c\\n') end\n"
			"function onFail() error('50% of %s') end\n");
	int Script = Load("percent.lua");
	TakeErrors();
	TOutcome Log = Call(Script, "onLog", RUN);
	CHECK(!Log.Thrown);
	CHECK(TakeErrors() == "a %% b %%s c\n");
	TOutcome Fail = Call(Script, "onFail", RUN);
	CHECK(Fail.Thrown && Fail.Code == ERROR);
	std::string Logged = TakeErrors();
	CHECK_CONTAINS(Logged, "Script error in percent.lua (onFail): percent.lua:2: 50%% of %%s\nstack traceback:");
	CHECK(Logged.find("50% of") == std::string::npos);

	// The text LoadScript throws (InitAll logs it with error("...%s...")).
	WriteScript("percent_load.lua", "error('load 7% bad')\n");
	std::string Thrown = LoadError("percent_load.lua", "data/x%y.toml:1:2");
	CHECK_CONTAINS(Thrown, "data/x%%y.toml:1:2: percent_load.lua:1: load 7%% bad");
	CHECK(Thrown.find("7% bad") == std::string::npos);
}

// The engine's second state (phase 8): BeginScriptReload makes a new state,
// set up as InitScripting sets one up and current, with no script; the old
// one stays as it was; DiscardScriptReload goes back to it and
// CommitScriptReload closes it. The modules, the globals of the scripts and
// the API tables are the state's own.
static void TestReload() {
	CHECK(!ScriptReloadActive());
	// Commit and Discard without a reload change nothing (they assert in a
	// debug build, so only a release build tries them).
#ifdef NDEBUG
	CommitScriptReload();
	DiscardScriptReload();
	CHECK(!ScriptReloadActive());
#endif

	WriteScript("reload_counter.lua",
			"local lib = require('lib.reload_module')\n"
			"calls = 0\n"
			"function onCount() calls = calls + 1 return calls * 100 + lib.loads end\n");
	WriteScript("lib/reload_module.lua", "test.count()\nreturn {loads = 1}\n");
	sol::state *OldState = &ScriptState();
	int OldTop = lua_gettop(OldState->lua_state());
	CountCalls = 0;
	int OldCounter = Load("reload_counter.lua");
	CHECK(CountCalls == 1);
	CHECK(Call(OldCounter, "onCount", RUN_NUMBER).Number == 101);
	int OldGood = LoadScript("good.lua", nullptr);

	// Begin: a new current state with no script; the API is set up as at
	// start (the core API, the classes, the constants) but for what the
	// caller registers.
	srand(7);
	int First = rand();
	srand(7);
	BeginScriptReload();
	CHECK(rand() == First);
	CHECK(ScriptReloadActive());
	CHECK(ScriptingActive());
	CHECK(&ScriptState() != OldState);
	CHECK(lua_gettop(ScriptState().lua_state()) == 0);
	// Not a second time.
	bool Refused = false;
	try {
		BeginScriptReload();
	} catch (const char *Text) {
		Refused = true;
		CHECK_CONTAINS(Text, "BeginScriptReload: a reload has begun already");
	}
	CHECK(Refused);
	CHECK(ScriptReloadActive());

	// The new state lacks the test API until it is registered.
	CHECK_CONTAINS(LoadError("reload_counter.lua"), "unknown name 'test'");
	CHECK(TakeErrors() == "");
	RegisterTestAPI();
	// The modules are the new state's: lib.reload_module runs again.
	CountCalls = 0;
	int NewCounter = Load("reload_counter.lua");
	CHECK(NewCounter == 0);
	CHECK(CountCalls == 1);
	CHECK(ScriptName(NewCounter) != nullptr && strcmp(ScriptName(NewCounter), "reload_counter.lua") == 0);
	CHECK(Call(NewCounter, "onCount", RUN_NUMBER).Number == 101);
	CHECK(Call(NewCounter, "onCount", RUN_NUMBER).Number == 201);
	CHECK(!HasHandler(NewCounter, "onA"));
	CHECK(Load("good.lua") == 1);
	CHECK(HasHandler(1, "onA"));

	// Discard: the old state is current again, as it was.
	DiscardScriptReload();
	CHECK(!ScriptReloadActive());
	CHECK(&ScriptState() == OldState);
	CHECK(lua_gettop(OldState->lua_state()) == OldTop);
	CHECK(LoadScript("reload_counter.lua", nullptr) == OldCounter);
	CHECK(LoadScript("good.lua", nullptr) == OldGood);
	CHECK(Call(OldCounter, "onCount", RUN_NUMBER).Number == 201);

	// Begin and commit: the new state replaces the old one for good.
	BeginScriptReload();
	RegisterTestAPI();
	CountCalls = 0;
	NewCounter = Load("reload_counter.lua");
	CHECK(NewCounter == 0);
	CHECK(CountCalls == 1);
	CommitScriptReload();
	CHECK(!ScriptReloadActive());
	CHECK(ScriptingActive());
	CHECK(&ScriptState() != OldState);
	CHECK(Call(NewCounter, "onCount", RUN_NUMBER).Number == 101);
	CHECK(LoadScript("reload_counter.lua", nullptr) == NewCounter);
	// Set up as at start: the core API, the classes, the constants, and what
	// was registered.
	WriteScript("reload_plain.lua", "function onType()\n"
			"  test.record(type(Game) .. ' ' .. RETURNVALUE_NOTPOSSIBLE .. ' ' .. type(test) .. ' '\n"
			"    .. CONST_ME_FIREAREA .. ' ' .. tostring(Position(1, 2, 3)) .. ' ' .. math.random(5, 5))\n"
			"end\n");
	int Plain = Load("reload_plain.lua");
	Records.clear();
	CHECK(!Call(Plain, "onType", RUN).Thrown);
	CHECK(Records.size() == 1 && Records[0] == "userdata 1 userdata 7 Position(1, 2, 3) 5");
	CHECK(TakeErrors() == "");

	// A failure in the new state is logged with the new state's names, and
	// failure() sees it (the failures are counted for both states).
	WriteScript("reload_fail.lua", "function onFail() error('new state') end\n");
	int Fail = Load("reload_fail.lua");
	TScriptCall FailCall(Fail, "onFail");
	CHECK(!FailCall.runCaught());
	CHECK(FailCall.failure() != nullptr && std::string(FailCall.failure()) == "reload_fail.lua:1: new state");
	CHECK_CONTAINS(TakeErrors(), "Script error in reload_fail.lua (onFail): reload_fail.lua:1: new state");

	// ExitScripting during a reload closes both states.
	BeginScriptReload();
	CHECK(ScriptReloadActive());
	ExitScripting();
	CHECK(!ScriptingActive());
	CHECK(!ScriptReloadActive());
	bool Inactive = false;
	try {
		BeginScriptReload();
	} catch (const char *Text) {
		Inactive = true;
		CHECK_CONTAINS(Text, "BeginScriptReload: the scripting engine is not active");
	}
	CHECK(Inactive);
	InitScripting(Folder.c_str());
	RegisterTestAPI();
	CHECK(Load("reload_counter.lua") == 0);
}

static void TestRestart() {
	CHECK(ScriptingActive());
	bool Thrown = false;
	try {
		InitScripting(Folder.c_str());
	} catch (const char *) {
		Thrown = true;
	}
	CHECK(Thrown);

	ExitScripting();
	CHECK(!ScriptingActive());
	ExitScripting();
	CHECK(!ScriptingActive());

	// InitScripting does not consume rand().
	srand(1);
	int First = rand();
	srand(1);
	InitScripting(Folder.c_str());
	int Second = rand();
	CHECK(First == Second);
	CHECK(ScriptingActive());
	RegisterTestAPI();
	CountCalls = 0;
	int Good = Load("good.lua");
	CHECK(Good == 0);
	CHECK(CountCalls == 1);
	TOutcome A = Call(Good, "onA", RUN_NUMBER);
	CHECK(!A.Thrown && A.Number == 1);
	// The modules are forgotten too.
	CountCalls = 0;
	CHECK(Load("req2.lua") >= 0);
	CHECK(CountCalls == 1);
}

int main() {
	const char *Temporary = getenv("TMPDIR");
	std::string Template = std::string(Temporary != nullptr && Temporary[0] != 0 ? Temporary : "/tmp")
			+ "/patella-scripting-XXXXXX";
	std::vector<char> Buffer(Template.begin(), Template.end());
	Buffer.push_back(0);
	if (mkdtemp(Buffer.data()) == nullptr) {
		perror("mkdtemp");
		return 1;
	}
	Folder = Buffer.data();

	SetErrorFunction(CaptureError);
	SetPrintFunction(CapturePrint);

	CHECK(!ScriptingActive());
	ExitScripting();
	InitScripting(Folder.c_str());
	RegisterTestAPI();
	int BaseTop = lua_gettop(ScriptState().lua_state());

	TestLoading();
	TestHandlers();
	TestSandbox();
	TestRequire();
	TestResultNames();
	TestResults();
	TestErrors();
	TestModuleTraceback();
	TestErrorObject();
	TestCalls();
	TestPrint();
	TestStringArray();
	TestRandom();
	TestArithmetic();
	TestRunResults();
	TestObjects();
	TestNpcs();
	TestMonsterObjects();
	TestValues();
	TestRunCaught();
	TestHooks();
	TestPositions();
	TestItemTypes();
	TestEvents();
	TestFailure();
	TestConstants();
	TestEnvironments();
	TestReadOnly();
	TestGuarded();
	TestPercent();
	// Every call left Lua's stack as it found it.
	CHECK(lua_gettop(ScriptState().lua_state()) == BaseTop);
	TestReload();
	TestRestart();

	ExitScripting();
	SetErrorFunction(nullptr);
	SetPrintFunction(nullptr);
	std::error_code Ignored;
	fs::remove_all(Folder, Ignored);

	if (Failures > 0) {
		fprintf(stderr, "%d check(s) failed\n", Failures);
		return 1;
	}
	printf("all checks passed\n");
	return 0;
}
