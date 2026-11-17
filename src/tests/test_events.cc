// test_events.cc: the game events (scripting_events.cc) on the Lua engine,
// without the game. InitEvents with good and wrong events.toml files, the
// TALKTYPE_ constants, and the Event functions: each calls the handler of
// every listed script that defines it, in the file's order, with its
// arguments; onSay's true stops the rest; a failure is logged and the next
// script runs; a result code goes to the caller; the events part of a reload
// (EventsReloadPart) with a new state. The scripts and files are
// written into a scratch folder that is removed at the end. Prints what
// failed; exits 1 on any failure, 0 when all pass.

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
#include "utils.hh"
#include "scripting_api.hh"
#include "reload.hh"

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

// The scratch folder; the scripts are in its scripts/, events.toml in it.
static fs::path Folder;

// Writes the file Name (a path below Folder) with Text.
static void WriteFile(const std::string &Name, const std::string &Text) {
	fs::path Path = Folder / Name;
	fs::create_directories(Path.parent_path());
	std::ofstream Out(Path, std::ios::binary);
	Out << Text;
}

// The path of events.toml in the scratch folder.
static std::string EventsFile() {
	return (Folder / "events.toml").string();
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

// What the scripts recorded with test.record.
static std::vector<std::string> Records;

// test.record(text): records the text.
static void TestRecord(std::string Text) {
	Records.push_back(Text);
}

// test.exhaust(): throws EXHAUSTED.
static void TestExhaust() {
	throw EXHAUSTED;
}

// test.say(text): EventSay(7, text, 1) as a Talk of player 7 made by a
// script, its result.
static bool TestSay(std::string Text) {
	return EventSay(7, Text.c_str(), 1);
}

static void RegisterTestAPI() {
	sol::table API = ScriptAPITable("test");
	API.set_function("record", Guarded(&TestRecord));
	API.set_function("exhaust", Guarded(&TestExhaust));
	API.set_function("say", Guarded(&TestSay));
}

// Writes events.toml with Text and runs InitEvents on it: the text it throws,
// or "" when it loads.
static std::string InitError(const std::string &Text) {
	WriteFile("events.toml", Text);
	try {
		InitEvents(EventsFile().c_str());
	} catch (const char *Message) {
		return Message;
	}
	return "";
}

// Prints the records when they are not the expected ones.
static void CheckRecords(const std::vector<std::string> &Expected, int Line) {
	if (Records != Expected) {
		fprintf(stderr, "%s:%d: CHECK failed: records differ\n", __FILE__, Line);
		for (const std::string &Record : Records) {
			fprintf(stderr, "  record: %s\n", Record.c_str());
		}
		Failures++;
	}
}

#define CHECK_RECORDS(...) CheckRecords(std::vector<std::string>__VA_ARGS__, __LINE__)

// What an Event call threw: NOERROR for none.
template<typename F>
static RESULT Thrown(F &&Event) {
	try {
		Event();
	} catch (RESULT r) {
		return r;
	}
	return NOERROR;
}

// The values of the TALKTYPE_ constants.
static void TestConstants() {
	WriteFile("scripts/talktypes.lua",
			"test.record(table.concat({TALKTYPE_SAY, TALKTYPE_WHISPER, TALKTYPE_YELL, TALKTYPE_PRIVATE_TO,\n"
			"  TALKTYPE_CHANNEL_Y, TALKTYPE_GAMEMASTER_REQUEST, TALKTYPE_GAMEMASTER_ANSWER,\n"
			"  TALKTYPE_PLAYER_ANSWER, TALKTYPE_BROADCAST, TALKTYPE_CHANNEL_R1, TALKTYPE_PRIVATE_RED_TO,\n"
			"  TALKTYPE_CHANNEL_O, TALKTYPE_ANONYMOUS_BROADCAST, TALKTYPE_ANONYMOUS_CHANNELCALL,\n"
			"  TALKTYPE_ANONYMOUS_MESSAGE, TALKTYPE_MONSTER_SAY, TALKTYPE_MONSTER_YELL}, ' '))\n"
			"test.record(tostring((pcall(function() return TALKTYPE_ADMIN_MESSAGE end))))\n"
			"local ok, message = pcall(function() TALKTYPE_SAY = 2 end)\n"
			"test.record(tostring(ok) .. ' ' .. message)\n");
	Records.clear();
	try {
		LoadScript("talktypes.lua", nullptr);
	} catch (const char *Text) {
		fprintf(stderr, "talktypes.lua: %s\n", Text);
		Failures++;
	}
	CHECK_RECORDS({"1 2 3 4 5 6 7 8 9 10 11 12 13 14 15 16 17", "false",
			"false talktypes.lua:7: TALKTYPE_SAY is read-only"});
	CHECK(TakeErrors() == "");
}

// The scripts the tests list: all handlers that record their arguments
// (all.lua), only onLogin and an onSay that gives false (login.lua), an onSay
// that consumes texts beginning with '!' (bang.lua), none of the handlers
// (none.lua), and handlers that fail or raise result codes (failing.lua).
static void WriteScripts() {
	const char *Show =
			"local function show(v)\n"
			"  if v == nil then return 'nil' end\n"
			"  return type(v) .. ':' .. tostring(v)\n"
			"end\n";
	WriteFile("scripts/events/all.lua", std::string(Show) +
			"function onLogin(player) test.record('all login ' .. show(player)) end\n"
			"function onLogout(player) test.record('all logout ' .. show(player)) end\n"
			"function onDeath(player, killer) test.record('all death ' .. show(player) .. ' ' .. show(killer)) end\n"
			"function onAdvance(player, skill, oldLevel, newLevel)\n"
			"  test.record('all advance ' .. show(player) .. ' ' .. show(skill) .. ' ' .. show(oldLevel) .. ' '\n"
			"    .. show(newLevel))\n"
			"end\n"
			"function onSay(player, text, mode)\n"
			"  test.record('all say ' .. show(player) .. ' ' .. show(text) .. ' ' .. show(mode))\n"
			"end\n");
	WriteFile("scripts/events/login.lua",
			"function onLogin(player) test.record('login login ' .. tostring(player)) end\n"
			"function onSay(player, text, mode) test.record('login say ' .. text) return false end\n"
			"onLogout = 'not a function'\n");
	WriteFile("scripts/events/bang.lua",
			"function onSay(player, text, mode)\n"
			"  test.record('bang say ' .. text)\n"
			"  return text:sub(1, 1) == '!'\n"
			"end\n");
	WriteFile("scripts/events/none.lua", "local x = 1\n");
	WriteFile("scripts/events/failing.lua",
			"function onLogin(player) error('login failed') end\n"
			"function onLogout(player) test.exhaust() end\n"
			"function onDeath(player, killer) Game.throwResult(RETURNVALUE_ERROR) end\n"
			"function onAdvance(player) error('advance failed') end\n"
			"function onSay(player, text, mode)\n"
			"  if text == 'number' then return 5 end\n"
			"  if text == 'exhaust' then test.exhaust() end\n"
			"  if text == 'result error' then Game.throwResult(RETURNVALUE_ERROR) end\n"
			"  error('say failed')\n"
			"end\n");
	WriteFile("scripts/events/broken.lua", "function onLogin(player)\n");
}

// No scripts: no Event function calls anything, EventSay gives false.
static void CheckNothingCalled() {
	Records.clear();
	EventLogin(7);
	EventLogout(7);
	EventDeath(7, 8);
	EventAdvance(7, 0, 1, 2);
	CHECK(!EventSay(7, "!hi", 1));
	CHECK(Records.empty());
	CHECK(TakeErrors() == "");
}

// InitEvents with good files and the Event functions on their scripts.
static void TestCalls() {
	// The delivered form: a header, no entry.
	CHECK(InitError("# events.toml: no event scripts\n") == "");
	CheckNothingCalled();
	CHECK(InitError("") == "");
	CheckNothingCalled();

	CHECK(InitError("[[event]]\nscript = \"events/all.lua\"\n\n"
			"[[event]]\nscript = \"events/none.lua\"\n\n"
			"[[event]]\nscript = \"events/login.lua\"\n\n"
			"[[event]]\nscript = \"events/bang.lua\"\n") == "");
	CHECK(TakeErrors() == "");

	// Each script that defines the handler, in the file's order, with the
	// arguments (a creature of 0 as nil, a Player for a player's ID, a
	// Creature otherwise).
	Records.clear();
	CHECK(Thrown([] { EventLogin(7); }) == NOERROR);
	CHECK(Thrown([] { EventLogout(7); }) == NOERROR);
	CHECK(Thrown([] { EventDeath(7, 8); }) == NOERROR);
	CHECK(Thrown([] { EventDeath(7, 0x40000010u); }) == NOERROR);
	CHECK(Thrown([] { EventDeath(7, 0); }) == NOERROR);
	CHECK(Thrown([] { EventAdvance(7, 0, 8, 10); }) == NOERROR);
	CHECK(Thrown([] { EventAdvance(7, 13, 10, 11); }) == NOERROR);
	CHECK_RECORDS({
		"all login userdata:Player 7",
		"login login Player 7",
		"all logout userdata:Player 7",
		"all death userdata:Player 7 userdata:Player 8",
		"all death userdata:Player 7 userdata:Creature 1073741840",
		"all death userdata:Player 7 nil",
		"all advance userdata:Player 7 number:0 number:8 number:10",
		"all advance userdata:Player 7 number:13 number:10 number:11",
	});

	// onSay: nil and false go on, true stops the rest and gives true.
	Records.clear();
	bool Consumed = true;
	CHECK(Thrown([&] { Consumed = EventSay(7, "hello", 1); }) == NOERROR);
	CHECK(!Consumed);
	CHECK(Thrown([&] { Consumed = EventSay(7, "!go", 3); }) == NOERROR);
	CHECK(Consumed);
	CHECK_RECORDS({
		"all say userdata:Player 7 string:hello number:1",
		"login say hello",
		"bang say hello",
		"all say userdata:Player 7 string:!go number:3",
		"login say !go",
		"bang say !go",
	});
	CHECK(TakeErrors() == "");

	// A script that consumes stops the scripts after it.
	CHECK(InitError("[[event]]\nscript = \"events/bang.lua\"\n\n"
			"[[event]]\nscript = \"events/all.lua\"\n") == "");
	Records.clear();
	CHECK(EventSay(7, "!x", 1));
	CHECK(!EventSay(7, "y", 2));
	CHECK_RECORDS({"bang say !x", "bang say y", "all say userdata:Player 7 string:y number:2"});
	CHECK(TakeErrors() == "");

	// A new InitEvents replaces the lists; an empty file leaves none.
	CHECK(InitError("") == "");
	CheckNothingCalled();
}

// Failures are logged and the next script runs; a result code goes to the
// caller and the scripts after it are not called.
static void TestFailures() {
	CHECK(InitError("[[event]]\nscript = \"events/failing.lua\"\n\n"
			"[[event]]\nscript = \"events/all.lua\"\n") == "");
	CHECK(TakeErrors() == "");

	Records.clear();
	CHECK(Thrown([] { EventLogin(7); }) == NOERROR);
	CHECK_CONTAINS(TakeErrors(), "Script error in events/failing.lua (onLogin): events/failing.lua:1: login failed");
	CHECK(Thrown([] { EventAdvance(7, 1, 2, 3); }) == NOERROR);
	CHECK_CONTAINS(TakeErrors(), "Script error in events/failing.lua (onAdvance): events/failing.lua:4: advance failed");
	CHECK_RECORDS({
		"all login userdata:Player 7",
		"all advance userdata:Player 7 number:1 number:2 number:3",
	});

	// Result codes of the notify-only handlers: logged, the next script runs,
	// nothing is thrown.
	Records.clear();
	CHECK(Thrown([] { EventLogout(7); }) == NOERROR);
	CHECK_CONTAINS(TakeErrors(), "onLogout of events/failing.lua: result code ");
	CHECK(Thrown([] { EventDeath(7, 8); }) == NOERROR);
	CHECK_CONTAINS(TakeErrors(), "onDeath of events/failing.lua: result code ");
	CHECK_RECORDS({
		"all logout userdata:Player 7",
		"all death userdata:Player 7 userdata:Player 8",
	});

	// onSay: an error and a result that is not a boolean are logged and count
	// as false; a result code is thrown, RETURNVALUE_ERROR too.
	Records.clear();
	bool Consumed = true;
	CHECK(Thrown([&] { Consumed = EventSay(7, "plain", 1); }) == NOERROR);
	CHECK(!Consumed);
	CHECK_CONTAINS(TakeErrors(), "Script error in events/failing.lua (onSay): events/failing.lua:9: say failed");
	Consumed = true;
	CHECK(Thrown([&] { Consumed = EventSay(7, "number", 1); }) == NOERROR);
	CHECK(!Consumed);
	CHECK_CONTAINS(TakeErrors(), "Script error in events/failing.lua (onSay): onSay returned 5, not a boolean");
	CHECK_RECORDS({
		"all say userdata:Player 7 string:plain number:1",
		"all say userdata:Player 7 string:number number:1",
	});
	Records.clear();
	CHECK(Thrown([] { EventSay(7, "exhaust", 1); }) == EXHAUSTED);
	CHECK(Thrown([] { EventSay(7, "result error", 1); }) == ERROR);
	CHECK(Records.empty());
	CHECK(TakeErrors() == "");
}

// A Talk made while an onSay runs calls no onSay: the inner EventSay gives
// false and records nothing; the next say runs onSay again.
static void TestReentry() {
	WriteFile("scripts/events/reenter.lua",
			"function onSay(player, text, mode)\n"
			"  test.record('say ' .. text)\n"
			"  if text == 'outer' then test.record('inner ' .. tostring(test.say('inner'))) end\n"
			"end\n");
	CHECK(InitError("[[event]]\nscript = \"events/reenter.lua\"\n") == "");
	Records.clear();
	CHECK(!EventSay(7, "outer", 1));
	CHECK(!EventSay(7, "again", 1));
	CHECK_RECORDS({"say outer", "inner false", "say again"});
	CHECK(TakeErrors() == "");
}

// The errors of InitEvents: each names its place; a failed InitEvents keeps
// the lists it had.
static void TestErrors() {
	CHECK(InitError("[[event]]\nscript = \"events/bang.lua\"\n") == "");
	std::string File = EventsFile();

	CHECK_CONTAINS(InitError("[[events]]\nscript = \"events/all.lua\"\n"), "the top level");
	CHECK_CONTAINS(InitError("[[events]]\nscript = \"events/all.lua\"\n"), "unknown key");
	CHECK_CONTAINS(InitError("[[event]]\nscript = \"events/all.lua\"\nname = \"x\"\n"), File + ":");
	CHECK_CONTAINS(InitError("[[event]]\nscript = \"events/all.lua\"\nname = \"x\"\n"), "[[event]]");
	CHECK_CONTAINS(InitError("[[event]]\nname = \"x\"\n"), "unknown key");
	CHECK_CONTAINS(InitError("[[event]]\n"), File + ":1:1: an entry of event needs script");
	CHECK_CONTAINS(InitError("[[event]]\nscript = 5\n"), File + ":2:1: script must be a string");
	CHECK_CONTAINS(InitError("event = 5\n"), File + ":1:");
	CHECK_CONTAINS(InitError("event = [5]\n"), File + ":1:10: an entry of event must be a table");
	CHECK_CONTAINS(InitError("[[event]]\nscript = \"items/all.lua\"\n"),
			File + ":2:1: script \"items/all.lua\" must be a script name \"events/<name>.lua\"");
	CHECK_CONTAINS(InitError("[[event]]\nscript = \"events/../all.lua\"\n"), "must be a script name");
	CHECK_CONTAINS(InitError("[[event]]\nscript = \"events/.lua\"\n"), "must be a script name");
	CHECK_CONTAINS(InitError("[[event]]\nscript = \"events/missing.lua\"\n"), File + ":2:1: ");
	CHECK_CONTAINS(InitError("[[event]]\nscript = \"events/missing.lua\"\n"), "events/missing.lua");
	CHECK_CONTAINS(InitError("[[event]]\nscript = \"events/broken.lua\"\n"), File + ":2:1: ");
	CHECK_CONTAINS(InitError("[[event]]\nscript = \"events/broken.lua\"\n"), "events/broken.lua:");
	CHECK(InitError("[[event]]\nscript = \"events/all.lua\"\n\n[[event]]\nscript = \"events/login.lua\"\n\n"
			"[[event]]\nscript = \"events/all.lua\"\n")
			== File + ":8:1: script \"events/all.lua\" is listed twice (first at " + File + ":2:1)");
	CHECK_CONTAINS(InitError("[[event]\n"), File + ":1:");
	TakeErrors();

	// A missing file.
	fs::remove(Folder / "events.toml");
	bool Missing = false;
	try {
		InitEvents(EventsFile().c_str());
	} catch (const char *Text) {
		Missing = true;
		CHECK_CONTAINS(Text, "events.toml");
	}
	CHECK(Missing);

	// The list of the last good file is still there.
	Records.clear();
	CHECK(EventSay(7, "!still", 1));
	CHECK(!EventSay(7, "still", 1));
	CHECK_RECORDS({"bang say !still", "bang say still"});
	CHECK(TakeErrors() == "");

	CHECK(InitError("") == "");
}

// The events part of a reload (EventsReloadPart): read stages the list and
// says what changed, install and loadScripts change nothing the Event
// functions use, finish makes the new lists theirs; a failure undone with
// uninstall and discard leaves the old lists and the old state working.
static void TestReloadPart() {
	CHECK(InitError("[[event]]\nscript = \"events/all.lua\"\n\n[[event]]\nscript = \"events/login.lua\"\n") == "");
	TReloadPart &Part = EventsReloadPart();
	CHECK(std::string(Part.name()) == "events");

	// A reload to bang.lua and all.lua, all steps.
	WriteFile("events.toml", "[[event]]\nscript = \"events/bang.lua\"\n\n[[event]]\nscript = \"events/all.lua\"\n");
	Part.read();
	CHECK(Part.summary() == "events.toml: 2 listed, 1 added, 1 removed");
	Part.install();
	BeginScriptReload();
	RegisterTestAPI();
	RegisterEventAPI();
	Part.loadScripts();
	Part.finish();
	CommitScriptReload();
	Records.clear();
	CHECK(EventSay(7, "!x", 1));
	EventLogin(7);
	CHECK_RECORDS({"bang say !x", "all login userdata:Player 7"});
	CHECK(TakeErrors() == "");

	// A script that fails to load: undone, the lists and the state as before.
	WriteFile("events.toml", "[[event]]\nscript = \"events/login.lua\"\n\n[[event]]\nscript = \"events/broken.lua\"\n");
	Part.read();
	CHECK(Part.summary() == "events.toml: 2 listed, 2 added, 2 removed");
	Part.install();
	BeginScriptReload();
	RegisterTestAPI();
	RegisterEventAPI();
	std::string Error;
	try {
		Part.loadScripts();
	} catch (const char *Text) {
		Error = Text;
	}
	CHECK_CONTAINS(Error, EventsFile() + ":5:1: ");
	CHECK_CONTAINS(Error, "events/broken.lua:");
	Part.uninstall();
	Part.discard();
	DiscardScriptReload();
	Records.clear();
	CHECK(EventSay(7, "!y", 1));
	EventLogin(7);
	CHECK_RECORDS({"bang say !y", "all login userdata:Player 7"});
	CHECK(TakeErrors() == "");

	// A file that fails to read: discarded, nothing changed; the next read
	// compares with the list in use.
	WriteFile("events.toml", "[[event]]\nscript = \"events/all.lua\"\n\n[[event]]\nscript = \"events/all.lua\"\n");
	Error.clear();
	try {
		Part.read();
	} catch (const char *Text) {
		Error = Text;
	}
	CHECK_CONTAINS(Error, "is listed twice");
	Part.discard();
	WriteFile("events.toml", "[[event]]\nscript = \"events/all.lua\"\n");
	Part.read();
	CHECK(Part.summary() == "events.toml: 1 listed, 0 added, 1 removed");
	Part.discard();
	Records.clear();
	CHECK(EventSay(7, "!z", 1));
	CHECK_RECORDS({"bang say !z"});

	// InitEvents is the part's steps: an empty file leaves no lists.
	CHECK(InitError("") == "");
	CheckNothingCalled();
}

int main() {
	const char *Temporary = getenv("TMPDIR");
	std::string Template = std::string(Temporary != nullptr && Temporary[0] != 0 ? Temporary : "/tmp")
			+ "/patella-events-XXXXXX";
	std::vector<char> Buffer(Template.begin(), Template.end());
	Buffer.push_back(0);
	if (mkdtemp(Buffer.data()) == nullptr) {
		perror("mkdtemp");
		return 1;
	}
	Folder = Buffer.data();

	SetErrorFunction(CaptureError);
	SetPrintFunction(CapturePrint);

	// The Event functions do nothing before InitEvents, without the engine.
	CheckNothingCalled();

	InitScripting((Folder / "scripts").c_str());
	RegisterTestAPI();
	RegisterEventAPI();
	int BaseTop = lua_gettop(ScriptState().lua_state());

	TestConstants();
	WriteScripts();
	TestCalls();
	TestFailures();
	TestReentry();
	TestErrors();
	// Every call left Lua's stack as it found it.
	CHECK(lua_gettop(ScriptState().lua_state()) == BaseTop);
	TestReloadPart();
	CHECK(lua_gettop(ScriptState().lua_state()) == 0);

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
