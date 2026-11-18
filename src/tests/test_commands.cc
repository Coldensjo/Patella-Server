// test_commands.cc: the commands' API (scripting_commands.cc) on the Lua
// engine, without the game: the game functions its bindings call are stubs
// here that record what they were given and answer as the test needs, and no
// creature exists (GetCreature and GetPlayer give nullptr). Checked: the
// constants; each binding that takes no creature calls its C++ function with
// the arguments converted
// as the C++ code converted them (copies of the strings it may change, the
// house number as an unsigned short, the round as an int, the value as a
// uint32_t) and gives back what it gives; the binding's checks; the methods
// on a creature that is gone (logged, ERROR). With --scripts <folder> (the
// game's data/scripts), the module lib/commands.lua is loaded from a copy of
// its lib/ and its scanCoordinates is run over a corpus against the C
// library's sscanf(Param, "%d,%d,%d") and sscanf(Param, "[%d,%d,%d]"), and,
// when the module has them, its lower against strLower and its add32 against
// the C++ int addition (-fwrapv). The scripts are written into a scratch folder
// that is removed at the end. Prints what failed; exits 1 on any failure, 0
// when all pass.

#include <climits>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <string>
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
#include "mudinc.hh"
#include "operate.hh"
#include "sending.hh"
#include "time.hh"
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

// What the stubs were given, one line per call ("SearchFreeField 10 20 7 1
// 65535 1"), and what the scripts recorded.
static std::vector<std::string> Calls;

// ---------------------------------------------------------------------------
// The game's functions the bindings call: stubs.

uint32_t RoundNr = 4000000000u;
TCircle Circle[MAX_CIRCLES];

TCreature *GetCreature(uint32_t) {
	return nullptr;
}

TPlayer *GetPlayer(uint32_t) {
	return nullptr;
}

// Cuts a '~' at the end as the game's IdentifyPlayer does (so the test sees
// whether the binding gave a copy); -2 for "amb~", -1 for any other name.
int IdentifyPlayer(char *Name, bool ExactMatch, bool IgnoreGamemasters, TPlayer *&pl) {
	Calls.push_back(std::string("IdentifyPlayer ") + Name + " " + std::to_string(ExactMatch) + " "
			+ std::to_string(IgnoreGamemasters));
	size_t Length = strlen(Name);
	if (!ExactMatch && Length > 0 && Name[Length - 1] == '~') {
		Name[Length - 1] = 0;
	}
	pl = nullptr;
	return strcmp(Name, "amb") == 0 ? -2 : -1;
}

// No player online: Game.getPlayers reads PlayerList only below
// GetNumberOfPlayers.
vector<TPlayer *> PlayerList(0, 10, 10, nullptr);

int GetNumberOfPlayers() {
	Calls.push_back("GetNumberOfPlayers");
	return 0;
}

// True for the command 37 (Move).
bool CheckCommandRight(uint32_t CharacterID, int SpellNr, int Check) {
	Calls.push_back("CheckCommandRight " + std::to_string(CharacterID) + " " + std::to_string(SpellNr) + " "
			+ std::to_string(Check));
	return SpellNr == 37;
}

void TSkill::SetMDAct(int) {
	Calls.push_back("SetMDAct");
}

void TPlayer::ClearProfession() {
	Calls.push_back("ClearProfession");
}

Object GetMapContainer(Object) {
	Calls.push_back("GetMapContainer");
	return NONE;
}

void ObjectType::setTypeID(int _TypeID) {
	TypeID = _TypeID;
}

// Finds (x + 1, y + 2) for the distance 1, nothing otherwise.
bool SearchFreeField(int &x, int &y, int &z, int Distance, unsigned short HouseID, bool Jump) {
	Calls.push_back("SearchFreeField " + std::to_string(x) + " " + std::to_string(y) + " " + std::to_string(z) + " "
			+ std::to_string(Distance) + " " + std::to_string(HouseID) + " " + std::to_string(Jump));
	if (Distance != 1) {
		return false;
	}
	x += 1;
	y += 2;
	return true;
}

// Finds (x - 1, y) for the distance 2, nothing otherwise.
bool SearchSummonField(int &x, int &y, int &z, int Distance) {
	Calls.push_back("SearchSummonField " + std::to_string(x) + " " + std::to_string(y) + " " + std::to_string(z)
			+ " " + std::to_string(Distance));
	if (Distance != 2) {
		return false;
	}
	x -= 1;
	return true;
}

// True for the power 1.
bool ThrowPossible(int sx, int sy, int sz, int zx, int zy, int zz, int Power) {
	Calls.push_back("ThrowPossible " + std::to_string(sx) + " " + std::to_string(sy) + " " + std::to_string(sz) + " "
			+ std::to_string(zx) + " " + std::to_string(zy) + " " + std::to_string(zz) + " " + std::to_string(Power));
	return Power == 1;
}

// True for x 100.
bool IsOnMap(int x, int y, int z) {
	Calls.push_back("IsOnMap " + std::to_string(x) + " " + std::to_string(y) + " " + std::to_string(z));
	return x == 100;
}

// The mark "temple" at 1000, 2000, 7; changes the name it was given (so the
// test sees whether the binding gave a copy).
bool GetMarkPosition(char *Name, int &x, int &y, int &z) {
	Calls.push_back(std::string("GetMarkPosition ") + Name);
	bool Found = strcmp(Name, "temple") == 0;
	Name[0] = 'X';
	if (!Found) {
		return false;
	}
	x = 1000;
	y = 2000;
	z = 7;
	return true;
}

// The raid "orcs" exists.
bool QueueMonsterRaid(const char *Key, int Start) {
	Calls.push_back(std::string("QueueMonsterRaid ") + Key + " " + std::to_string(Start));
	return strcmp(Key, "orcs") == 0;
}

// The object 77 for the type 3031, NONE otherwise.
Object CreateAtCreature(uint32_t CreatureID, ObjectType Type, uint32_t Value) {
	Calls.push_back("CreateAtCreature " + std::to_string(CreatureID) + " " + std::to_string(Type.getTypeID()) + " "
			+ std::to_string(Value));
	return Type.getTypeID() == 3031 ? Object(77) : NONE;
}

void AnnounceChangedCreature(uint32_t CreatureID, int Type) {
	Calls.push_back("AnnounceChangedCreature " + std::to_string(CreatureID) + " " + std::to_string(Type));
}

void GraphicalEffect(int x, int y, int z, int Type) {
	Calls.push_back("GraphicalEffect " + std::to_string(x) + " " + std::to_string(y) + " " + std::to_string(z) + " "
			+ std::to_string(Type));
}

// The type 3264 for "sword", 0 otherwise; changes the name it was given.
ObjectType GetObjectTypeByName(char *SearchName, bool Movable) {
	Calls.push_back(std::string("GetObjectTypeByName ") + SearchName + " " + std::to_string(Movable));
	bool Sword = strcmp(SearchName, "sword") == 0;
	SearchName[0] = 'X';
	return ObjectType(Sword ? 3264 : 0);
}

// The types 100 to 5000 exist.
bool ObjectTypeExists(int TypeID) {
	return TypeID >= 100 && TypeID <= 5000;
}

void ShowGuestList(unsigned short HouseID, TPlayer *, char *Buffer) {
	Calls.push_back("ShowGuestList " + std::to_string(HouseID));
	strcpy(Buffer, "# Guests\n");
}

void ShowSubownerList(unsigned short HouseID, TPlayer *, char *Buffer) {
	Calls.push_back("ShowSubownerList " + std::to_string(HouseID));
	strcpy(Buffer, "# Subowners\n");
}

void ShowNameDoor(Object Door, TPlayer *, char *Buffer) {
	Calls.push_back("ShowNameDoor " + std::to_string(Door.getObjectID()));
	strcpy(Buffer, "# Door\n");
}

void KickGuest(unsigned short HouseID, TPlayer *Host, TPlayer *Guest) {
	Calls.push_back("KickGuest " + std::to_string(HouseID) + " " + (Host == nullptr ? "null" : "host") + " "
			+ (Guest == nullptr ? "null" : "guest"));
}

void SendEditList(TConnection *, unsigned char Type, uint32_t ID, char *Text) {
	Calls.push_back("SendEditList " + std::to_string(Type) + " " + std::to_string(ID) + " " + Text);
}

// ---------------------------------------------------------------------------
// The test's own API and helpers.

static fs::path Folder;

// Writes the script Name (a path below the scratch folder) with Text.
static void WriteScript(const std::string &Name, const std::string &Text) {
	fs::path Path = Folder / Name;
	fs::create_directories(Path.parent_path());
	std::ofstream Out(Path, std::ios::binary);
	Out << Text;
}

// What error() was given since the last TakeErrors.
static std::string Errors;

static void CaptureError(char *Text) {
	Errors += Text;
}

static void CapturePrint(int, char *) {
}

// The errors logged since the last call, cleared.
static std::string TakeErrors() {
	std::string Result = Errors;
	Errors.clear();
	return Result;
}

// The stubs' calls since the last call, cleared.
static std::vector<std::string> TakeCalls() {
	std::vector<std::string> Result = Calls;
	Calls.clear();
	return Result;
}

// Prints the calls when they are not the expected ones.
static bool SameCalls(const std::vector<std::string> &Got, const std::vector<std::string> &Expected) {
	if (Got == Expected) {
		return true;
	}
	fprintf(stderr, "  calls:\n");
	for (const std::string &Line : Got) {
		fprintf(stderr, "    %s\n", Line.c_str());
	}
	fprintf(stderr, "  expected:\n");
	for (const std::string &Line : Expected) {
		fprintf(stderr, "    %s\n", Line.c_str());
	}
	return false;
}

// test.record(text): records the text as a call.
static void TestRecord(std::string Text) {
	Calls.push_back(Text);
}

// test.makePlayer(id), test.makeCreature(id): the objects.
static TScriptPlayer TestMakePlayer(uint32_t ID) {
	return TScriptPlayer{ID};
}

static TScriptCreature TestMakeCreature(uint32_t ID) {
	return TScriptCreature{ID};
}

// test.makeItem(handle): an Item.
static TScriptItem TestMakeItem(uint32_t Handle) {
	return TScriptItem{Handle};
}

// What scanCoordinates, lower and add32 of lib/commands.lua gave.
struct TScanned {
	bool Matched;
	double x, y, z;
	int Count;
};
static TScanned Scanned;
static std::string Lowered;
static double Added;

// test.scanned(matched, x, y, z, count): what scanCoordinates gave.
static void TestScanned(bool Matched, double x, double y, double z, int Count) {
	Scanned = TScanned{Matched, x, y, z, Count};
}

// test.lowered(text): what lower gave.
static void TestLowered(std::string Text) {
	Lowered = Text;
}

// test.added(sum): what add32 gave.
static void TestAdded(double Sum) {
	Added = Sum;
}

static void RegisterTestAPI() {
	sol::table API = ScriptAPITable("test");
	API.set_function("record", Guarded(&TestRecord));
	API.set_function("makePlayer", Guarded(&TestMakePlayer));
	API.set_function("makeCreature", Guarded(&TestMakeCreature));
	API.set_function("makeItem", Guarded(&TestMakeItem));
	API.set_function("scanned", Guarded(&TestScanned));
	API.set_function("lowered", Guarded(&TestLowered));
	API.set_function("added", Guarded(&TestAdded));
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

// How a handler call ended: thrown (a RESULT, which), or its boolean.
struct TOutcome {
	bool Thrown;
	RESULT Code;
	bool Boolean;
};

// Runs Handler of Script with the number Number (when not 0) as runBoolean.
static TOutcome Call(int Script, const char *Handler, int Number = 0) {
	TOutcome Outcome = {false, NOERROR, false};
	TScriptCall ScriptCall(Script, Handler);
	if (Number != 0) {
		ScriptCall.addNumber(Number);
	}
	try {
		Outcome.Boolean = ScriptCall.runBoolean();
	} catch (RESULT r) {
		Outcome.Thrown = true;
		Outcome.Code = r;
	}
	return Outcome;
}

// Runs the refusals of a script's onRefused(i) for i = 1 to the count of
// Expected: each call must fail with a Lua error containing its text (the
// script records tostring(err) with test.record), and nothing may be logged.
static void CheckRefusals(int Script, const std::vector<const char *> &Expected, const char *What) {
	for (size_t i = 0; i < Expected.size(); i++) {
		TakeCalls();
		TOutcome Outcome = Call(Script, "onRefused", static_cast<int>(i + 1));
		CHECK(!Outcome.Thrown && !Outcome.Boolean);
		if (Outcome.Thrown || Outcome.Boolean) {
			fprintf(stderr, "  %s refusal number %d was not refused\n", What, static_cast<int>(i + 1));
		}
		std::vector<std::string> Recorded = TakeCalls();
		CHECK(Recorded.size() == 1);
		if (Recorded.size() == 1) {
			CHECK_CONTAINS(Recorded[0], Expected[i]);
		} else {
			fprintf(stderr, "  %s refusal number %d recorded %d lines\n", What, static_cast<int>(i + 1),
					static_cast<int>(Recorded.size()));
		}
	}
	CHECK(TakeErrors() == "");
}

// ---------------------------------------------------------------------------
// The tests

// The constants scripting.hh lists, with their values.
static void TestConstants() {
	WriteScript("constants.lua",
			"function onCheck()\n"
			"  return MESSAGE_STATUS_SMALL == 23 and MESSAGE_EVENT_ADVANCE == 19\n"
			"    and CONST_SLOT_HEAD == 1 and CONST_SLOT_NECKLACE == 2 and CONST_SLOT_BACKPACK == 3\n"
			"    and CONST_SLOT_ARMOR == 4 and CONST_SLOT_RIGHT == 5 and CONST_SLOT_LEFT == 6\n"
			"    and CONST_SLOT_LEGS == 7 and CONST_SLOT_FEET == 8 and CONST_SLOT_RING == 9 and CONST_SLOT_AMMO == 10\n"
			"    and DIRECTION_NORTH == 0 and DIRECTION_EAST == 1 and DIRECTION_SOUTH == 2 and DIRECTION_WEST == 3\n"
			"    and CREATURE_HEALTH == 1 and CREATURE_LIGHT == 2 and CREATURE_OUTFIT == 3 and CREATURE_SPEED == 4\n"
			"    and CREATURE_SKULL == 5 and CREATURE_PARTY == 6\n"
			"    and COMMAND_RIGHT == 0 and MOVE_RIGHT_VERTICAL == 0 and MOVE_RIGHT_SPEED == 1\n"
			"    and MOVE_RIGHT_COORDINATE == 2 and MOVE_RIGHT_MARK == 3\n"
			"    and GUEST_LIST == 1 and SUBOWNER_LIST == 2 and MAX_AMOUNT == 100 and MAX_RACES == 512\n"
			"end\n");
	int Script = Load("constants.lua");
	TOutcome Outcome = Call(Script, "onCheck");
	CHECK(!Outcome.Thrown && Outcome.Boolean);
	CHECK(TakeErrors() == "");
}

// The methods of Creature and Player on a creature that is gone: each logs
// "<name>: Creature does not exist." and raises ERROR, and calls nothing.
static void TestGoneCreature() {
	WriteScript("gone.lua",
			"local p = test.makePlayer(5)\n"
			"local c = test.makeCreature(0x40000005)\n"
			"local calls = {\n"
			"  function() return c:getDirection() end,\n"
			"  function() return c:isMonster() end,\n"
			"  function() return c:getMapContainer() end,\n"
			"  function() return c:setSkillMagicDelta(SKILL_GOSTRENGTH, 100) end,\n"
			"  function() return p:getStartPosition() end,\n"
			"  function() return p:setSex(PLAYERSEX_MALE) end,\n"
			"  function() return p:clearVocation() end,\n"
			"  function() return p:addSkillTries(0, 5) end,\n"
			"  function() return p:sendHouseWindow(House(7), GUEST_LIST) end,\n"
			"  function() return p:sendNameDoorWindow(test.makeItem(9)) end,\n"
			"}\n"
			"function onCall(i) calls[i]() end\n");
	// SKILL_GOSTRENGTH and PLAYERSEX_MALE are other areas' constants.
	SetScriptConstant("SKILL_GOSTRENGTH", 12);
	SetScriptConstant("PLAYERSEX_MALE", 1);
	int Script = Load("gone.lua");
	const char *Names[] = {"Creature.getDirection", "Creature.isMonster", "Creature.getMapContainer",
			"Creature.setSkillMagicDelta", "Player.getStartPosition", "Player.setSex", "Player.clearVocation",
			"Player.addSkillTries", "Player.sendHouseWindow", "Player.sendNameDoorWindow"};
	int Count = static_cast<int>(sizeof(Names) / sizeof(Names[0]));
	for (int i = 1; i <= Count; i++) {
		TakeCalls();
		TScriptCall Gone(Script, "onCall");
		Gone.addNumber(i);
		bool Thrown = false;
		RESULT Code = NOERROR;
		try {
			Gone.run();
		} catch (RESULT r) {
			Thrown = true;
			Code = r;
		}
		CHECK(Thrown && Code == ERROR);
		std::string Logged = TakeErrors();
		CHECK(Logged.rfind(std::string(Names[i - 1]) + ": Creature does not exist.\n", 0) == 0);
		if (Logged.rfind(std::string(Names[i - 1]) + ": Creature does not exist.\n", 0) != 0) {
			fprintf(stderr, "  call %d logged: %s\n", i, Logged.c_str());
		}
		CHECK(TakeCalls().empty());
	}
}

// player:hasCommandRight: CheckCommandRight with the player's ID, the command
// and the check (COMMAND_RIGHT when left out), its result; the binding's
// checks.
static void TestCommandRight() {
	WriteScript("command_right.lua",
			"local p = test.makePlayer(5)\n"
			"function onRight()\n"
			"  return p:hasCommandRight(37) == true and p:hasCommandRight(40) == false\n"
			"    and p:hasCommandRight(37, MOVE_RIGHT_MARK) == true and p:hasCommandRight(255, 1) == false\n"
			"    and p:hasCommandRight(0) == false\n"
			"end\n"
			"function onRefused(i)\n"
			"  local calls = {\n"
			"    function() return p:hasCommandRight(-1) end,\n"
			"    function() return p:hasCommandRight(256) end,\n"
			"    function() return p:hasCommandRight(37, -1) end,\n"
			"    function() return p:hasCommandRight(37, 4) end,\n"
			"    function() return p:hasCommandRight(37, 'x') end,\n"
			"    function() return p:hasCommandRight(37.5) end,\n"
			"    function() return p:hasCommandRight() end,\n"
			"    function() return test.makeCreature(0x40000005):hasCommandRight(37) end,\n"
			"    function() return Player.hasCommandRight(test.makeCreature(0x40000005), 37) end,\n"
			"  }\n"
			"  local ok, err = pcall(calls[i])\n"
			"  test.record(tostring(err))\n"
			"  return ok\n"
			"end\n");
	int Script = Load("command_right.lua");
	TakeCalls();
	TOutcome Outcome = Call(Script, "onRight");
	CHECK(!Outcome.Thrown && Outcome.Boolean);
	CHECK(SameCalls(TakeCalls(), {"CheckCommandRight 5 37 0", "CheckCommandRight 5 40 0", "CheckCommandRight 5 37 3",
			"CheckCommandRight 5 255 1", "CheckCommandRight 5 0 0"}));
	CHECK(TakeErrors() == "");
	// The binding's checks and sol's argument checks (no overload matches)
	// raise Lua errors, which pcall catches: nothing is logged or called.
	CheckRefusals(Script, {"no such command", "no such command", "no such right check", "no such right check",
			"matching", "matching", "matching", "unknown name 'Creature.hasCommandRight'", "matching"},
			"hasCommandRight");
}

// House, Position and the Game, ItemType and Magic functions: the C++
// calls with their arguments and what the bindings give back.
static void TestBindings() {
	WriteScript("bindings.lua",
			"function onHouse()\n"
			"  local h = House(7)\n"
			"  h:kickPlayer(test.makePlayer(5), test.makePlayer(6))\n"
			"  return h:getId() == 7 and House.getId(House(65535)) == 65535\n"
			"end\n"
			"function onEffect()\n"
			"  local p = Position(1, 2, 3)\n"
			"  p:sendMagicEffect(13)\n"
			"  Position.sendMagicEffect(Position(-1, 70000, 15), 0)\n"
			"  return true\n"
			"end\n"
			"function onIdentify()\n"
			"  local name = 'Name~'\n"
			"  local r1, p1 = Game.identifyPlayer(name, false, true)\n"
			"  local r2, p2 = Game.identifyPlayer('amb~', false, false)\n"
			"  local r3, p3 = Game.identifyPlayer('Exact~', true, false)\n"
			"  return r1 == -1 and p1 == nil and r2 == -2 and p2 == nil and r3 == -1 and p3 == nil\n"
			"    and name == 'Name~'\n"
			"    and select('#', Game.identifyPlayer('x', false, false)) == 2\n"
			"end\n"
			"function onPlayers()\n"
			"  local players = Game.getPlayers()\n"
			"  return type(players) == 'table' and #players == 0 and next(players) == nil\n"
			"end\n"
			"function onFields()\n"
			"  local from = Position(10, 20, 7)\n"
			"  return Game.searchFreeField(from, 1, 65535, true) == Position(11, 22, 7)\n"
			"    and Game.searchFreeField(from, 0, 0, false) == nil\n"
			"    and Game.searchFreeField(from, 1, 4711, false) == Position(11, 22, 7)\n"
			"    and Game.searchSummonField(from, 2) == Position(9, 20, 7) and Game.searchSummonField(from, 3) == nil\n"
			"    and select('#', Game.searchSummonField(from, 3)) == 1\n"
			"    and Game.isOnMap(Position(100, 1, 2)) == true and Game.isOnMap(Position(99, 1, 2)) == false\n"
			"    and Game.isThrowPossible(Position(1, 2, 3), Position(4, 5, 6), 1) == true\n"
			"    and Game.isThrowPossible(Position(1, 2, 3), Position(4, 5, 6), 0) == false\n"
			"end\n"
			"function onMarks()\n"
			"  local name = 'temple'\n"
			"  return Game.getMarkPosition(name) == Position(1000, 2000, 7) and name == 'temple'\n"
			"    and Game.getMarkPosition('nowhere') == nil\n"
			"end\n"
			"function onRaid()\n"
			"  return Game.startRaid('orcs') == true and Game.startRaid('trolls') == false\n"
			"end\n"
			"function onCreate()\n"
			"  local made = Game.createAtCreature(5, 3031, 4294967295)\n"
			"  local none = Game.createAtCreature(0x40000005, 1, 0)\n"
			"  Game.createAtCreature(5, 2, -1)\n"
			"  Game.announceChangedCreature(5, CREATURE_SPEED)\n"
			"  return tostring(made) == 'Item 77' and none == nil\n"
			"end\n"
			"function onItemTypes()\n"
			"  local name = 'sword'\n"
			"  return ItemType.fromName(name, true) == 3264 and name == 'sword'\n"
			"    and ItemType.fromName('nothing', false) == 0\n"
			"    and ItemType.exists(100) == true and ItemType.exists(5000) == true and ItemType.exists(99) == false\n"
			"    and ItemType.exists(5001) == false\n"
			"end\n"
			"function onCircles()\n"
			"  local c0, c2, again = Magic.getCircle(0), Magic.getCircle(2), Magic.getCircle(2)\n"
			"  return type(c0) == 'table' and #c0 == 1 and c0[1] == Position(0, 0, 0)\n"
			"    and #c2 == 3 and c2[1] == Position(-2, 0, 0) and c2[2] == Position(0, 2, 0)\n"
			"    and c2[3] == Position(2, -1, 0)\n"
			"    and not rawequal(c2, again) and again[3] == c2[3] and #Magic.getCircle(9) == 0\n"
			"end\n"
			"function onRefused(i)\n"
			"  local calls = {\n"
			"    function() return Game.searchFreeField(Position(1, 2, 3), 1, 65536, true) end,\n"
			"    function() return Game.searchFreeField(Position(1, 2, 3), 1, -1, true) end,\n"
			"    function() return Magic.getCircle(-1) end,\n"
			"    function() return Magic.getCircle(10) end,\n"
			"    function() return House(7):kickPlayer(test.makeCreature(0x40000005), test.makePlayer(6)) end,\n"
			"    function() return Position(1, 2, 3).sendMagicEffect(House(1), 1) end,\n"
			"    function() return Game.identifyPlayer(nil, false, false) end,\n"
			"    function() return ItemType.fromName(3031, true) end,\n"
			"    function() return Game.isOnMap(House(1)) end,\n"
			"  }\n"
			"  local ok, err = pcall(calls[i])\n"
			"  test.record(tostring(err))\n"
			"  return ok\n"
			"end\n");
	int Script = Load("bindings.lua");
	TakeCalls();

	TOutcome House = Call(Script, "onHouse");
	CHECK(!House.Thrown && House.Boolean);
	CHECK(SameCalls(TakeCalls(), {"KickGuest 7 null null"}));

	TOutcome Effect = Call(Script, "onEffect");
	CHECK(!Effect.Thrown && Effect.Boolean);
	CHECK(SameCalls(TakeCalls(), {"GraphicalEffect 1 2 3 13", "GraphicalEffect -1 70000 15 0"}));

	TOutcome Identify = Call(Script, "onIdentify");
	CHECK(!Identify.Thrown && Identify.Boolean);
	CHECK(SameCalls(TakeCalls(), {"IdentifyPlayer Name~ 0 1", "IdentifyPlayer amb~ 0 0", "IdentifyPlayer Exact~ 1 0",
			"IdentifyPlayer x 0 0"}));

	TOutcome Players = Call(Script, "onPlayers");
	CHECK(!Players.Thrown && Players.Boolean);
	CHECK(SameCalls(TakeCalls(), {"GetNumberOfPlayers"}));

	TOutcome Fields = Call(Script, "onFields");
	CHECK(!Fields.Thrown && Fields.Boolean);
	CHECK(SameCalls(TakeCalls(), {"SearchFreeField 10 20 7 1 65535 1", "SearchFreeField 10 20 7 0 0 0",
			"SearchFreeField 10 20 7 1 4711 0", "SearchSummonField 10 20 7 2", "SearchSummonField 10 20 7 3",
			"SearchSummonField 10 20 7 3", "IsOnMap 100 1 2", "IsOnMap 99 1 2", "ThrowPossible 1 2 3 4 5 6 1",
			"ThrowPossible 1 2 3 4 5 6 0"}));

	TOutcome Marks = Call(Script, "onMarks");
	CHECK(!Marks.Thrown && Marks.Boolean);
	CHECK(SameCalls(TakeCalls(), {"GetMarkPosition temple", "GetMarkPosition nowhere"}));

	// The round as the C++ code's (int)RoundNr: 4000000000 wraps.
	TOutcome Raid = Call(Script, "onRaid");
	CHECK(!Raid.Thrown && Raid.Boolean);
	CHECK(SameCalls(TakeCalls(), {"QueueMonsterRaid orcs -294967296", "QueueMonsterRaid trolls -294967296"}));

	TOutcome Create = Call(Script, "onCreate");
	CHECK(!Create.Thrown && Create.Boolean);
	CHECK(SameCalls(TakeCalls(), {"CreateAtCreature 5 3031 4294967295", "CreateAtCreature 1073741829 1 0",
			"CreateAtCreature 5 2 4294967295", "AnnounceChangedCreature 5 4"}));

	TOutcome ItemTypes = Call(Script, "onItemTypes");
	CHECK(!ItemTypes.Thrown && ItemTypes.Boolean);
	CHECK(SameCalls(TakeCalls(), {"GetObjectTypeByName sword 1", "GetObjectTypeByName nothing 0"}));

	TOutcome Circles = Call(Script, "onCircles");
	CHECK(!Circles.Thrown && Circles.Boolean);
	CHECK(TakeCalls().empty());
	CHECK(TakeErrors() == "");

	CheckRefusals(Script, {"houseId must be 0 to 65535", "houseId must be 0 to 65535", "radius must be 0",
			"radius must be 0", "Player expected", "Position expected", "expected string, received nil",
			"expected string, received number", "Position expected"}, "binding");
	CHECK(TakeCalls().empty());
}

// sol's argument checks of the house dialogs: a wrong argument is refused
// before the binding runs (so before the player is looked up); nothing is
// logged or called.
static void TestHouseWindowChecks() {
	WriteScript("house_window.lua",
			"function onRefused(i)\n"
			"  local p = test.makePlayer(5)\n"
			"  local calls = {\n"
			"    function() return p:sendHouseWindow(7, GUEST_LIST) end,\n"
			"    function() return p:sendHouseWindow(House(7), 'x') end,\n"
			"    function() return p:sendNameDoorWindow(House(7)) end,\n"
			"  }\n"
			"  local ok, err = pcall(calls[i])\n"
			"  test.record(tostring(err))\n"
			"  return ok\n"
			"end\n");
	int Script = Load("house_window.lua");
	CheckRefusals(Script, {"House expected", "expected number, received string", "Item expected"}, "house window");
}

// The C++ code's coordinates (Teleport): sscanf(Param, "%d,%d,%d") == 3 or
// sscanf(Param, "[%d,%d,%d]") == 3, with x, y, z as sscanf leaves them
// (they start as the caster's position there; here a value no corpus entry
// gives).
static bool ScanCoordinates(const char *Text, int &x, int &y, int &z) {
	x = y = z = 123456789;
	return sscanf(Text, "%d,%d,%d", &x, &y, &z) == 3 || sscanf(Text, "[%d,%d,%d]", &x, &y, &z) == 3;
}

// lib/commands.lua from the game's scripts folder ScriptsFolder: copies its
// lib/ into the scratch folder and checks scanCoordinates against
// ScanCoordinates over a corpus, lower against strLower and add32 against the
// C++ int addition, each when the module has it. Skipped (and said so) when
// the module is not there.
static void TestCommandsModule(const char *ScriptsFolder) {
	if (ScriptsFolder == nullptr) {
		printf("lib/commands.lua: no --scripts folder given, its checks skipped\n");
		return;
	}
	fs::path Lib = fs::path(ScriptsFolder) / "lib";
	if (!fs::exists(Lib / "commands.lua")) {
		printf("lib/commands.lua: not in %s, its checks skipped\n", Lib.string().c_str());
		return;
	}
	std::error_code Failed;
	fs::copy(Lib, Folder / "lib", fs::copy_options::recursive | fs::copy_options::overwrite_existing, Failed);
	CHECK(!Failed);

	WriteScript("module.lua",
			"local commands = require('lib.commands')\n"
			"function onHas(i)\n"
			"  local names = {'scanCoordinates', 'lower', 'add32'}\n"
			"  return type(commands[names[i]]) == 'function'\n"
			"end\n"
			"function onScan(text)\n"
			"  local results = {commands.scanCoordinates(text)}\n"
			"  local count = select('#', commands.scanCoordinates(text))\n"
			"  if results[1] == nil then\n"
			"    test.scanned(false, 0, 0, 0, count)\n"
			"  else\n"
			"    test.scanned(true, results[1], results[2], results[3], count)\n"
			"  end\n"
			"end\n"
			"function onLower(text) test.lowered(commands.lower(text)) end\n"
			"function onAdd(a, b) test.added(commands.add32(a, b)) end\n");
	int Script;
	try {
		Script = LoadScript("module.lua", nullptr);
	} catch (const char *Text) {
		fprintf(stderr, "lib/commands.lua does not load with the core and the commands' API alone: %s\n", Text);
		Failures++;
		return;
	}

	bool Has[3];
	for (int i = 0; i < 3; i++) {
		TOutcome Outcome = Call(Script, "onHas", i + 1);
		Has[i] = !Outcome.Thrown && Outcome.Boolean;
	}
	CHECK(Has[0]);
	int Checked = 0;

	if (Has[0]) {
		const char *Corpus[] = {"1,2,3", "[1,2,3]", " 1, 2, 3", "1 ,2,3", "1,2 ,3", "1,2,3]", "[1,2,3", " [1,2,3]",
				"[ 1, 2, 3 ]", "[1 ,2,3]", "+1,-2,+3", "-0,0,0", "1,2", "1,2,", ",1,2,3", "1,,2,3", "a,b,c",
				"1,2,3,4", "1,2,3abc", "0x10,2,3", "1.5,2,3", "32000,31000,7", "[32000,31000,7]",
				"4294967297,1,1", "-2147483649,0,0", "2147483648,-2147483648,2147483647",
				"99999999999999999999,1,2", "-99999999999999999999,1,2", "[99999999999999999999,1,2]",
				"9223372036854775808,1,2", "00000000000000000000000000001,2,3", "\t1,\n2,\v3", "\f\r1,2,3",
				"1,2,\xA0" "3", "\xA0" "1,2,3", "\xE9", "", " ", "[", "[]", "[1,2]", "[1,2,3]x", "1, -2, - 3",
				"1,2,-", "--1,2,3", "+-1,2,3", "-+1,2,3", "1,2,3 ", "[-1,-2,-3]", "1e3,2,3", "\xD9\xA1,2,3",
				"1,2,+", "+,1,2", "[+1,+2,+3]", "[1,2,3,4]", "1;2;3", "1,2;3", "[1,2,3]]", "[[1,2,3]",
				"12345678901,2,3", "-1,-1,-1", "0,0,0", "1\xC0,2,3", "\xFF" "1,2,3", "1,2,3\xFF",
				"[ -5 , 6, 7]", "[-5, 6 ,7]", "  -5,  6,  7", "\n[1,2,3]", "1,\t+2,\t-3"};
		for (const char *Text : Corpus) {
			int x, y, z;
			bool Matched = ScanCoordinates(Text, x, y, z);
			Scanned = TScanned{false, -1, -1, -1, -1};
			TScriptCall Scan(Script, "onScan");
			Scan.addString(Text);
			bool Thrown = false;
			try {
				Scan.run();
			} catch (RESULT) {
				Thrown = true;
			}
			bool Same = !Thrown && Scanned.Matched == Matched
					&& (Matched ? Scanned.x == x && Scanned.y == y && Scanned.z == z && Scanned.Count == 3
						: Scanned.Count <= 1);
			CHECK(Same);
			if (!Same) {
				fprintf(stderr, "  scanCoordinates(\"%s\"): Lua %s %.17g %.17g %.17g (%d values), C %s %d %d %d%s\n",
						Text, Scanned.Matched ? "match" : "none", Scanned.x, Scanned.y, Scanned.z, Scanned.Count,
						Matched ? "match" : "none", x, y, z, Thrown ? " (the call failed)" : "");
			}
			Checked++;
		}
	}

	if (Has[1]) {
		std::vector<std::string> Texts;
		for (int c = 1; c < 256; c++) {
			Texts.push_back(std::string(1, static_cast<char>(c)));
		}
		Texts.push_back("Orc Raid");
		Texts.push_back("\xC4\xD6\xDC\xDF\xD7\xDE\xDF\xE4");
		Texts.push_back("DEMON_ODIN-2");
		for (const std::string &Text : Texts) {
			std::string Expected = Text;
			strLower(&Expected[0]);
			Lowered = "(none)";
			TScriptCall Lower(Script, "onLower");
			Lower.addString(Text.c_str());
			bool Thrown = false;
			try {
				Lower.run();
			} catch (RESULT) {
				Thrown = true;
			}
			CHECK(!Thrown && Lowered == Expected);
			if (Thrown || Lowered != Expected) {
				fprintf(stderr, "  lower of byte %d...: Lua \"%s\", strLower \"%s\"\n",
						static_cast<unsigned char>(Text[0]), Lowered.c_str(), Expected.c_str());
			}
			Checked++;
		}
	} else {
		printf("lib/commands.lua has no lower, its check skipped\n");
	}

	if (Has[2]) {
		int Pairs[][2] = {{1, 2}, {-1, 1}, {INT_MAX, 1}, {INT_MIN, -1}, {INT_MAX, INT_MAX}, {INT_MIN, INT_MIN},
				{32000, -40000}, {2000000000, 2000000000}, {-2000000000, -2000000000}, {0, 0}, {INT_MAX, INT_MIN}};
		for (auto &Pair : Pairs) {
			// The C++ int addition with -fwrapv: the sum modulo 2^32 as an int.
			int Expected = static_cast<int>(static_cast<uint32_t>(Pair[0]) + static_cast<uint32_t>(Pair[1]));
			Added = 0.5;
			TScriptCall Add(Script, "onAdd");
			Add.addNumber(Pair[0]);
			Add.addNumber(Pair[1]);
			bool Thrown = false;
			try {
				Add.run();
			} catch (RESULT) {
				Thrown = true;
			}
			CHECK(!Thrown && Added == Expected);
			if (Thrown || Added != Expected) {
				fprintf(stderr, "  add32(%d, %d): Lua %.17g, C++ %d\n", Pair[0], Pair[1], Added, Expected);
			}
			Checked++;
		}
	} else {
		printf("lib/commands.lua has no add32, its check skipped\n");
	}
	CHECK(TakeErrors() == "");
	printf("lib/commands.lua: %d values compared\n", Checked);
}

int main(int argc, char **argv) {
	const char *ScriptsFolder = nullptr;
	for (int i = 1; i < argc; i++) {
		if (strcmp(argv[i], "--scripts") == 0 && i + 1 < argc) {
			ScriptsFolder = argv[++i];
		} else {
			fprintf(stderr, "usage: %s [--scripts <data/scripts folder>]\n", argv[0]);
			return 2;
		}
	}

	const char *Temporary = getenv("TMPDIR");
	std::string Template = std::string(Temporary != nullptr && Temporary[0] != 0 ? Temporary : "/tmp")
			+ "/patella-commands-XXXXXX";
	std::vector<char> Buffer(Template.begin(), Template.end());
	Buffer.push_back(0);
	if (mkdtemp(Buffer.data()) == nullptr) {
		perror("mkdtemp");
		return 1;
	}
	Folder = Buffer.data();

	// Circle[0]: the center; Circle[2]: three fields; the others empty.
	Circle[0].Count = 1;
	Circle[0].x[0] = 0;
	Circle[0].y[0] = 0;
	Circle[2].Count = 3;
	Circle[2].x[0] = -2;
	Circle[2].y[0] = 0;
	Circle[2].x[1] = 0;
	Circle[2].y[1] = 2;
	Circle[2].x[2] = 2;
	Circle[2].y[2] = -1;

	SetErrorFunction(CaptureError);
	SetPrintFunction(CapturePrint);
	InitScripting(Folder.c_str());
	RegisterCommandAPI();
	RegisterTestAPI();
	int BaseTop = lua_gettop(ScriptState().lua_state());

	TestConstants();
	TestGoneCreature();
	TestCommandRight();
	TestBindings();
	TestHouseWindowChecks();
	TestCommandsModule(ScriptsFolder);
	CHECK(lua_gettop(ScriptState().lua_state()) == BaseTop);

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
