// test_scripting.cc: the Lua engine (scripting.cc) on its own. Loading scripts
// and its errors, handlers, the sandbox, require, result codes in both
// directions, the logging of other errors, the calls a script makes, the core
// API (randomness, integer arithmetic), the results of runNumber and
// runBoolean, the script environments, and a restart of the engine. The
// scripts are written into a scratch folder that is removed at the end.
// Prints what failed; exits 1 on any failure, 0 when all pass.

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

// test.throw_text(): throws the const char * "plain text".
static void TestThrowText() {
	throw "plain text";
}

// test.throw_exception(): throws std::runtime_error("what text").
static void TestThrowException() {
	throw std::runtime_error("what text");
}

static void RegisterTestAPI() {
	sol::table API = ScriptAPITable("test");
	API.set_function("count", Guarded(&TestCount));
	API.set_function("record", Guarded(&TestRecord));
	API.set_function("exhaust", Guarded(&TestExhaust));
	API.set_function("twice", Guarded(&TestTwice));
	API.set_function("throw_text", Guarded(&TestThrowText));
	API.set_function("throw_exception", Guarded(&TestThrowException));
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

// Calls Handler of Script with the arguments Arguments (numbers as ints).
static TOutcome Call(int Script, const char *Handler, TRun Kind, const std::vector<int> &Arguments = {}) {
	TOutcome Outcome = {false, NOERROR, 0, false};
	TScriptCall ScriptCall(Script, Handler);
	for (int Argument : Arguments) {
		ScriptCall.addNumber(Argument);
	}
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
	WriteScript("good.lua", "test.count()\nreturn { on_a = function() return 1 end }\n");
	CountCalls = 0;
	int Good = Load("good.lua");
	CHECK(Good >= 0);
	CHECK(CountCalls == 1);
	CHECK(LoadScript("good.lua", nullptr) == Good);
	CHECK(CountCalls == 1);
	CHECK(ScriptName(Good) != nullptr && strcmp(ScriptName(Good), "good.lua") == 0);

	WriteScript("sub/dir/nested.lua", "return {}\n");
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

	WriteScript("syntax.lua", "local x = 1\nlocal y = = 2\nreturn {}\n");
	std::string Syntax = LoadError("syntax.lua", "data/x.toml:3:5");
	CHECK(Syntax.compare(0, 17, "data/x.toml:3:5: ") == 0);
	CHECK_CONTAINS(Syntax, "syntax.lua:2:");
	CHECK(Syntax.find("[string") == std::string::npos);

	WriteScript("toprun.lua", "local t = nil\nt.x = 1\nreturn {}\n");
	std::string TopRun = LoadError("toprun.lua", "data/x.toml:4:1");
	CHECK(TopRun.compare(0, 17, "data/x.toml:4:1: ") == 0);
	CHECK_CONTAINS(TopRun, "toprun.lua:2:");
	CHECK_CONTAINS(TopRun, "stack traceback:");

	WriteScript("number.lua", "return 5\n");
	CHECK_CONTAINS(LoadError("number.lua"), "number.lua returns number, not a table");
	WriteScript("nothing.lua", "local x = 1\n");
	CHECK_CONTAINS(LoadError("nothing.lua"), "nothing.lua returns nil, not a table");

	WriteScript("unknown_global.lua", "local x = 1\nlocal y = undefined_thing\nreturn {}\n");
	std::string Unknown = LoadError("unknown_global.lua", "data/x.toml:2:2");
	CHECK_CONTAINS(Unknown, "data/x.toml:2:2: unknown_global.lua:2: unknown name 'undefined_thing'");

	WriteScript("unknown_api.lua", "local f = game.no_such_function\nreturn {}\n");
	CHECK_CONTAINS(LoadError("unknown_api.lua"), "unknown_api.lua:1: unknown name 'game.no_such_function'");
	WriteScript("unknown_result.lua", "local r = result.NOSUCHRESULT\nreturn {}\n");
	CHECK_CONTAINS(LoadError("unknown_result.lua"), "unknown name 'result.NOSUCHRESULT'");
	WriteScript("unknown_test.lua", "local r = test.nothing\nreturn {}\n");
	CHECK_CONTAINS(LoadError("unknown_test.lua"), "unknown name 'test.nothing'");

	WriteScript("top_throw.lua", "game.throw(result.NOROOM)\nreturn {}\n");
	CHECK_CONTAINS(LoadError("top_throw.lua"), "result NOROOM");

	WriteScript("binary.lua", std::string("\033LJ\002") + "junk");
	CHECK_CONTAINS(LoadError("binary.lua"), "binary chunks are not allowed");

	// A failed load is not cached: once fixed, the script loads.
	WriteScript("number.lua", "return {}\n");
	CHECK(Load("number.lua") >= 0);
}

static void TestHandlers() {
	WriteScript("handlers.lua", "return { on_a = function() end, on_b = 5 }\n");
	int Script = Load("handlers.lua");
	CHECK(HasHandler(Script, "on_a"));
	CHECK(!HasHandler(Script, "on_b"));
	CHECK(!HasHandler(Script, "on_c"));
	CHECK(RequireError(Script, "on_a", "data/x.toml:1:2") == "");
	CHECK(RequireError(Script, "on_c", "data/x.toml:1:2") == "data/x.toml:1:2: handlers.lua has no function on_c");
	CHECK(RequireError(Script, "on_b", nullptr) == "handlers.lua has no function on_b");

	// A missing handler at run time is a non-result error.
	TakeErrors();
	TOutcome Outcome = Call(Script, "on_c", RUN);
	CHECK(Outcome.Thrown && Outcome.Code == ERROR);
	CHECK_CONTAINS(TakeErrors(), "Script error in handlers.lua (on_c): handlers.lua has no function on_c");
}

static void TestSandbox() {
	const char *Unknown[] = {"io", "os", "debug", "ffi", "jit", "package", "dofile", "loadfile", "load",
			"loadstring", "print", "module", "_G", "getfenv", "setfenv", "rawset", "newproxy", "collectgarbage",
			"gcinfo", "unknown"};
	int Number = 0;
	for (const char *Name : Unknown) {
		// Named by number: not every global's name is a valid script name.
		std::string Script = "sandbox_" + std::to_string(Number++) + ".lua";
		WriteScript(Script, std::string("local x = ") + Name + "\nreturn {}\n");
		CHECK_CONTAINS(LoadError(Script.c_str()), Script + ":1: unknown name '" + Name + "'");
	}

	WriteScript("sandbox_ok.lua",
			"return {\n"
			"  on_removed = function()\n"
			"    return string.dump == nil and math.random == nil and math.randomseed == nil\n"
			"      and (\"x\").dump == nil\n"
			"  end,\n"
			"  on_works = function()\n"
			"    local t = {}\n"
			"    table.insert(t, 3)\n"
			"    table.insert(t, 4)\n"
			"    local f = coroutine.wrap(function() coroutine.yield(7) end)\n"
			"    return bit.band(6, 3) == 2 and bit.bor(1, 4) == 5 and string.format(\"%d-%s\", 5, \"x\") == \"5-x\"\n"
			"      and #t == 2 and t[2] == 4 and math.floor(2.5) == 2 and f() == 7\n"
			"      and type(pcall) == \"function\" and type(require) == \"function\"\n"
			"  end,\n"
			"}\n");
	int Script = Load("sandbox_ok.lua");
	TOutcome Removed = Call(Script, "on_removed", RUN_BOOLEAN);
	CHECK(!Removed.Thrown && Removed.Boolean);
	TOutcome Works = Call(Script, "on_works", RUN_BOOLEAN);
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
			"return { on_check = function() return a == b and a.value == 42 and n == true and i == 'inner' end }\n");
	WriteScript("req2.lua",
			"local a = require('lib.counted')\n"
			"return { on_value = function() return a.value end }\n");
	CountCalls = 0;
	int Req1 = Load("req1.lua");
	int Req2 = Load("req2.lua");
	CHECK(CountCalls == 1);
	TOutcome Check = Call(Req1, "on_check", RUN_BOOLEAN);
	CHECK(!Check.Thrown && Check.Boolean);
	TOutcome Value = Call(Req2, "on_value", RUN_NUMBER);
	CHECK(!Value.Thrown && Value.Number == 42);

	WriteScript("lib/cyc_a.lua", "require('lib.cyc_b')\nreturn 1\n");
	WriteScript("lib/cyc_b.lua", "require('lib.cyc_a')\nreturn 2\n");
	WriteScript("cycle.lua", "require('lib.cyc_a')\nreturn {}\n");
	std::string Cycle = LoadError("cycle.lua");
	CHECK_CONTAINS(Cycle, "lib/cyc_b.lua:1: require: 'lib.cyc_a' requires itself: lib/cyc_a.lua -> lib/cyc_b.lua -> lib/cyc_a.lua");
	WriteScript("self.lua", "require('lib.self')\nreturn {}\n");
	WriteScript("lib/self.lua", "require('lib.self')\n");
	CHECK_CONTAINS(LoadError("self.lua"), "requires itself: lib/self.lua -> lib/self.lua");

	// A module whose loading failed is neither cached nor left as "being
	// loaded": once fixed, it loads.
	WriteScript("lib/mid.lua", "local b = require('lib.boom')\nreturn b + 1\n");
	WriteScript("lib/boom.lua", "error('boom')\n");
	WriteScript("uses_mid.lua", "local m = require('lib.mid')\nreturn { on_m = function() return m end }\n");
	CHECK_CONTAINS(LoadError("uses_mid.lua"), "lib/boom.lua:1: boom");
	WriteScript("lib/boom.lua", "return 10\n");
	int UsesMid = Load("uses_mid.lua");
	TOutcome Mid = Call(UsesMid, "on_m", RUN_NUMBER);
	CHECK(!Mid.Thrown && Mid.Number == 11);

	// A failed require leaves the module off the stack of modules being
	// loaded: a second try gives the module's own error again, not a cycle,
	// and other modules load after it.
	WriteScript("lib/bad.lua", "local x = 1\nerror('bad module')\n");
	WriteScript("req_retry.lua",
			"return { on_retry = function()\n"
			"  local ok1, e1 = pcall(require, 'lib.bad')\n"
			"  local ok2, e2 = pcall(require, 'lib.bad')\n"
			"  test.record(tostring(e1))\n"
			"  test.record(tostring(e2))\n"
			"  local c = require('lib.counted')\n"
			"  return not ok1 and not ok2 and c.value == 42\n"
			"end }\n");
	int Retry = Load("req_retry.lua");
	Records.clear();
	TOutcome Retried = Call(Retry, "on_retry", RUN_BOOLEAN);
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
			"if ok1 or ok2 or string.find(e2, 'requires itself', 1, true) then error('retry failed') end\n"
			"return {}\n");
	CHECK(Load("req_retry_top.lua") >= 0);
	WriteScript("lib/bad.lua", "return 3\n");
	WriteScript("req_fixed.lua", "local b = require('lib.bad')\nreturn { on_b = function() return b end }\n");
	TOutcome Fixed = Call(Load("req_fixed.lua"), "on_b", RUN_NUMBER);
	CHECK(!Fixed.Thrown && Fixed.Number == 3);
	CHECK(TakeErrors() == "");

	WriteScript("lib/missing_user.lua", "return 1\n");
	WriteScript("req_missing.lua", "require('lib.not_there')\nreturn {}\n");
	CHECK_CONTAINS(LoadError("req_missing.lua"), "req_missing.lua:1: require 'lib.not_there': cannot open lib/not_there.lua");
	WriteScript("lib/bad_syntax.lua", "x = = 1\n");
	WriteScript("req_syntax.lua", "require('lib.bad_syntax')\nreturn {}\n");
	CHECK_CONTAINS(LoadError("req_syntax.lua"), "lib/bad_syntax.lua:1:");

	WriteScript("req_names.lua",
			"return { on_refused = function(i)\n"
			"  local names = { '..x', '../x', '/etc/passwd', 'lib.X', 'lib..counted', '.lib', 'lib.', 'lib/counted', 'a b', '' }\n"
			"  local ok, err = pcall(require, names[i])\n"
			"  return not ok and string.find(err, 'is not allowed', 1, true) ~= nil\n"
			"end }\n");
	int Names = Load("req_names.lua");
	for (int i = 1; i <= 10; i++) {
		TOutcome Refused = Call(Names, "on_refused", RUN_BOOLEAN, {i});
		CHECK(!Refused.Thrown && Refused.Boolean);
		if (Refused.Thrown || !Refused.Boolean) {
			fprintf(stderr, "  require name number %d was not refused\n", i);
		}
	}
	CHECK(TakeErrors() == "");
}

static void TestResults() {
	WriteScript("results.lua",
			"return {\n"
			"  on_number = function() game.throw(result.NOROOM) end,\n"
			"  on_name = function() game.throw('NOROOM') end,\n"
			"  on_error = function() game.throw(result.ERROR) end,\n"
			"  on_uncaught = function() test.exhaust() end,\n"
			"  on_caught = function()\n"
			"    local ok, err = pcall(test.exhaust)\n"
			"    return not ok and game.result_of(err) == result.EXHAUSTED and result[49] == 'EXHAUSTED'\n"
			"      and tostring(err) == 'result EXHAUSTED' and err.code == 49\n"
			"  end,\n"
			"  on_rethrow = function() local ok, err = pcall(test.exhaust); error(err) end,\n"
			"  on_wrapped = function() local f = coroutine.wrap(function() test.exhaust() end); f() end,\n"
			"  on_twice = function(n) return test.twice(n) end,\n"
			"  on_names = function()\n"
			"    return result.ERROR == -1 and result.NOERROR == 0 and result.NOROOM == 4 and result.LOWLEVEL == 60\n"
			"      and result[-1] == 'ERROR' and result[60] == 'LOWLEVEL' and result[4] == 'NOROOM'\n"
			"      and game.result_of('NOROOM') == nil and game.result_of(4) == nil and game.result_of() == nil\n"
			"  end,\n"
			"  on_bad_throw = function() game.throw(61) end,\n"
			"  on_bad_name = function() game.throw('NOSUCH') end,\n"
			"  on_bad_index = function() return result[61] end,\n"
			"}\n");
	int Script = Load("results.lua");
	TakeErrors();

	TOutcome Number = Call(Script, "on_number", RUN);
	CHECK(Number.Thrown && Number.Code == NOROOM);
	TOutcome Name = Call(Script, "on_name", RUN);
	CHECK(Name.Thrown && Name.Code == NOROOM);
	TOutcome ErrorCode = Call(Script, "on_error", RUN);
	CHECK(ErrorCode.Thrown && ErrorCode.Code == ERROR);
	TOutcome Uncaught = Call(Script, "on_uncaught", RUN);
	CHECK(Uncaught.Thrown && Uncaught.Code == EXHAUSTED);
	TOutcome Caught = Call(Script, "on_caught", RUN_BOOLEAN);
	CHECK(!Caught.Thrown && Caught.Boolean);
	TOutcome Rethrow = Call(Script, "on_rethrow", RUN);
	CHECK(Rethrow.Thrown && Rethrow.Code == EXHAUSTED);
	TOutcome Wrapped = Call(Script, "on_wrapped", RUN);
	CHECK(Wrapped.Thrown && Wrapped.Code == EXHAUSTED);
	TOutcome Twice = Call(Script, "on_twice", RUN_NUMBER, {21});
	CHECK(!Twice.Thrown && Twice.Number == 42);
	TOutcome TwiceThrown = Call(Script, "on_twice", RUN_NUMBER, {-1});
	CHECK(TwiceThrown.Thrown && TwiceThrown.Code == NOROOM);
	TOutcome Names = Call(Script, "on_names", RUN_BOOLEAN);
	CHECK(!Names.Thrown && Names.Boolean);
	// None of these is logged: result codes are not script errors.
	CHECK(TakeErrors() == "");

	TOutcome BadThrow = Call(Script, "on_bad_throw", RUN);
	CHECK(BadThrow.Thrown && BadThrow.Code == ERROR);
	CHECK_CONTAINS(TakeErrors(), "game.throw: 61 is not a result code");
	TOutcome BadName = Call(Script, "on_bad_name", RUN);
	CHECK(BadName.Thrown && BadName.Code == ERROR);
	CHECK_CONTAINS(TakeErrors(), "game.throw: unknown result 'NOSUCH'");
	TOutcome BadIndex = Call(Script, "on_bad_index", RUN_NUMBER);
	CHECK(BadIndex.Thrown && BadIndex.Code == ERROR);
	CHECK_CONTAINS(TakeErrors(), "unknown name 'result.61'");
}

static void TestErrors() {
	WriteScript("fail.lua",
			"local function inner(t)\n"
			"  return t.x\n"
			"end\n"
			"return { on_fail = function() local v = inner(nil) return v end,\n"
			"  on_unknown = function() return not_defined_anywhere end,\n"
			"  on_table = function() error({}) end,\n"
			"  on_string_error = function() error('custom message') end }\n");
	int Script = Load("fail.lua");
	TakeErrors();
	TOutcome Fail = Call(Script, "on_fail", RUN);
	CHECK(Fail.Thrown && Fail.Code == ERROR);
	std::string Logged = TakeErrors();
	CHECK(Logged.compare(0, 37, "Script error in fail.lua (on_fail): f") == 0);
	CHECK_CONTAINS(Logged, "Script error in fail.lua (on_fail): fail.lua:2:");
	CHECK_CONTAINS(Logged, "\nstack traceback:\n");
	CHECK_CONTAINS(Logged, "fail.lua:4:");

	TOutcome Unknown = Call(Script, "on_unknown", RUN);
	CHECK(Unknown.Thrown && Unknown.Code == ERROR);
	CHECK_CONTAINS(TakeErrors(), "Script error in fail.lua (on_unknown): fail.lua:5: unknown name 'not_defined_anywhere'");

	TOutcome Table = Call(Script, "on_table", RUN);
	CHECK(Table.Thrown && Table.Code == ERROR);
	CHECK_CONTAINS(TakeErrors(), "(error object is a table value)");

	TOutcome Custom = Call(Script, "on_string_error", RUN);
	CHECK(Custom.Thrown && Custom.Code == ERROR);
	CHECK_CONTAINS(TakeErrors(), "Script error in fail.lua (on_string_error): fail.lua:7: custom message");
}

// An error raised inside a module (require) is logged with the traceback
// taken where it was raised, once: the frames inside the modules are there.
static void TestModuleTraceback() {
	WriteScript("lib/n3.lua", "local x = 1\nerror('n3 failed')\n");
	WriteScript("lib/n2.lua", "require('lib.n3')\nreturn 2\n");
	WriteScript("lib/n1.lua", "require('lib.n2')\nreturn 1\n");
	WriteScript("nested_top.lua", "require('lib.n1')\nreturn {}\n");
	std::string Loaded = LoadError("nested_top.lua", "data/x.toml:9:1");
	CHECK(StartsWith(Loaded, "data/x.toml:9:1: lib/n3.lua:2: n3 failed\nstack traceback:\n"));
	CHECK(Occurrences(Loaded, "stack traceback:") == 1);
	std::string Trace = Traceback(Loaded);
	CHECK_CONTAINS(Trace, "\tlib/n3.lua:2:");
	CHECK_CONTAINS(Trace, "\tlib/n2.lua:1:");
	CHECK_CONTAINS(Trace, "\tlib/n1.lua:1:");
	CHECK_CONTAINS(Trace, "\tnested_top.lua:1:");

	WriteScript("nested_call.lua", "return { on_req = function()\n  require('lib.n1')\nend }\n");
	int Script = Load("nested_call.lua");
	TakeErrors();
	TOutcome Outcome = Call(Script, "on_req", RUN);
	CHECK(Outcome.Thrown && Outcome.Code == ERROR);
	std::string Logged = TakeErrors();
	CHECK(StartsWith(Logged, "Script error in nested_call.lua (on_req): lib/n3.lua:2: n3 failed\nstack traceback:\n"));
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
	WriteScript("lib/res_error.lua", "game.throw(result.NOROOM)\n");
	WriteScript("nested_again.lua",
			"return {\n"
			"  on_again = function()\n"
			"    local ok, err = pcall(require, 'lib.n1')\n"
			"    error(err, 0)\n"
			"  end,\n"
			"  on_number = function()\n"
			"    local ok, err = pcall(require, 'lib.num_error')\n"
			"    return not ok and type(err) == 'number' and err == 42\n"
			"  end,\n"
			"  on_result = function() require('lib.res_error') end,\n"
			"}\n");
	int Again = Load("nested_again.lua");
	TakeErrors();
	TOutcome AgainOutcome = Call(Again, "on_again", RUN);
	CHECK(AgainOutcome.Thrown && AgainOutcome.Code == ERROR);
	Logged = TakeErrors();
	CHECK_CONTAINS(Logged, "Script error in nested_again.lua (on_again): lib/n3.lua:2: n3 failed\nstack traceback:\n");
	CHECK(Occurrences(Logged, "stack traceback:") == 1);
	CHECK_CONTAINS(Traceback(Logged), "\tlib/n3.lua:2:");
	TOutcome Number = Call(Again, "on_number", RUN_BOOLEAN);
	CHECK(!Number.Thrown && Number.Boolean);
	TOutcome Result = Call(Again, "on_result", RUN);
	CHECK(Result.Thrown && Result.Code == NOROOM);
	CHECK(TakeErrors() == "");
	WriteScript("num_top.lua", "require('lib.num_error')\nreturn {}\n");
	std::string NumberLoaded = LoadError("num_top.lua");
	CHECK(StartsWith(NumberLoaded, "42\nstack traceback:\n"));
	CHECK_CONTAINS(Traceback(NumberLoaded), "\tlib/num_error.lua:1:");
}

// The message handlers call no metamethod of an error value: a __tostring
// that calls game.rand() does not run (no rand() is consumed), and the value
// is named by its type.
static void TestErrorObject() {
	const char *Raise = "error(setmetatable({}, { __tostring = function() game.rand() return 'called' end }))";
	WriteScript("tostring_error.lua", std::string("return { on_fail = function()\n  ") + Raise + "\nend }\n");
	WriteScript("lib/tostring_error.lua", std::string(Raise) + "\n");
	WriteScript("tostring_module.lua", "return { on_fail = function()\n  require('lib.tostring_error')\nend }\n");
	WriteScript("tostring_top.lua", std::string("local x = 1\n") + Raise + "\nreturn {}\n");
	int Script = Load("tostring_error.lua");
	int Module = Load("tostring_module.lua");
	TakeErrors();

	srand(77);
	TOutcome Outcome = Call(Script, "on_fail", RUN);
	int After = rand();
	srand(77);
	CHECK(After == rand());
	CHECK(Outcome.Thrown && Outcome.Code == ERROR);
	std::string Logged = TakeErrors();
	CHECK_CONTAINS(Logged, "Script error in tostring_error.lua (on_fail): (error object is a table value)\nstack traceback:\n");
	CHECK_CONTAINS(Traceback(Logged), "tostring_error.lua:2:");
	CHECK(Logged.find("called") == std::string::npos);

	srand(77);
	TOutcome ModuleOutcome = Call(Module, "on_fail", RUN);
	After = rand();
	srand(77);
	CHECK(After == rand());
	CHECK(ModuleOutcome.Thrown && ModuleOutcome.Code == ERROR);
	Logged = TakeErrors();
	CHECK_CONTAINS(Logged, "Script error in tostring_module.lua (on_fail): (error object is a table value)\nstack traceback:\n");
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
			"return { on_calls = function(id, words, flag, nothing, absent, negative)\n"
			"  test.record('first ' .. id)\n"
			"  test.record('second ' .. words)\n"
			"  test.record('third ' .. tostring(flag) .. ' ' .. tostring(nothing) .. ' ' .. tostring(absent))\n"
			"  test.record('fourth ' .. negative)\n"
			"end }\n");
	int Script = Load("calls.lua");
	Records.clear();
	TScriptCall ScriptCall(Script, "on_calls");
	ScriptCall.addNumber(static_cast<uint32_t>(0x80000001u));
	ScriptCall.addString("utevo lux");
	ScriptCall.addBoolean(true);
	ScriptCall.addNil();
	ScriptCall.addString(nullptr);
	ScriptCall.addNumber(-17);
	bool Thrown = false;
	try {
		ScriptCall.run();
	} catch (RESULT) {
		Thrown = true;
	}
	CHECK(!Thrown);
	std::vector<std::string> Expected = {"first 2147483649", "second utevo lux", "third true nil nil",
			"fourth -17"};
	CHECK(Records == Expected);
	CHECK(TakeErrors() == "");

	WriteScript("calls2.lua",
			"return { on_calls = function(a, b)\n"
			"  test.record('a ' .. a)\n"
			"  test.record('b ' .. b)\n"
			"  test.record('sum ' .. (a + b))\n"
			"end }\n");
	int Script2 = Load("calls2.lua");
	Records.clear();
	TOutcome Outcome = Call(Script2, "on_calls", RUN, {3, 4});
	CHECK(!Outcome.Thrown);
	CHECK((Records == std::vector<std::string>{"a 3", "b 4", "sum 7"}));
	CHECK(TakeErrors() == "");

	// Many arguments (the array grows).
	WriteScript("many.lua",
			"return { on_sum = function(...)\n"
			"  local s = 0\n"
			"  for _, v in ipairs({...}) do s = s + v end\n"
			"  return s + select('#', ...) * 1000\n"
			"end }\n");
	int Many = Load("many.lua");
	std::vector<int> Arguments;
	int Sum = 0;
	for (int i = 1; i <= 40; i++) {
		Arguments.push_back(i);
		Sum += i;
	}
	TOutcome ManyOutcome = Call(Many, "on_sum", RUN_NUMBER, Arguments);
	CHECK(!ManyOutcome.Thrown && ManyOutcome.Number == Sum + 40000);

	// game.print and game.error.
	WriteScript("print.lua", "return { on_print = function() game.print(2, 'hello\\n'); game.error('bad %s thing\\n') end }\n");
	int Print = Load("print.lua");
	Prints.clear();
	TakeErrors();
	TOutcome PrintOutcome = Call(Print, "on_print", RUN);
	CHECK(!PrintOutcome.Thrown);
	CHECK((Prints == std::vector<std::string>{"2:hello\n"}));
	// Its '%' are doubled: error() hands the text to Log as its format.
	CHECK(TakeErrors() == "bad %%s thing\n");
}

static void TestRandom() {
	WriteScript("random.lua",
			"return {\n"
			"  on_random = function(low, high) return game.random(low, high) end,\n"
			"  on_rand = function() return game.rand() end,\n"
			"}\n");
	int Script = Load("random.lua");
	struct TCase { bool Rand; int Low; int High; };
	std::vector<TCase> Cases = {{false, 1, 100}, {true, 0, 0}, {false, -50, 50}, {false, 5, 5}, {false, 10, 1},
			{true, 0, 0}, {false, 0, 1}, {false, INT_MIN, INT_MAX}, {false, 0, INT_MAX}, {false, 1, 6}};
	srand(1234);
	std::vector<int> Expected;
	for (int Round = 0; Round < 5; Round++) {
		for (const TCase &Case : Cases) {
			Expected.push_back(Case.Rand ? rand() : random(Case.Low, Case.High));
		}
	}
	srand(1234);
	std::vector<int> Got;
	bool Thrown = false;
	for (int Round = 0; Round < 5; Round++) {
		for (const TCase &Case : Cases) {
			TOutcome Outcome = Case.Rand ? Call(Script, "on_rand", RUN_NUMBER)
					: Call(Script, "on_random", RUN_NUMBER, {Case.Low, Case.High});
			Thrown = Thrown || Outcome.Thrown;
			Got.push_back(Outcome.Number);
		}
	}
	CHECK(!Thrown);
	CHECK(Got == Expected);
	CHECK(TakeErrors() == "");
}

static void TestArithmetic() {
	WriteScript("arith.lua",
			"return {\n"
			"  on_idiv = function(a, b) return game.idiv(a, b) end,\n"
			"  on_imod = function(a, b) return game.imod(a, b) end,\n"
			"  on_div0 = function()\n"
			"    local ok1, e1 = pcall(game.idiv, 1, 0)\n"
			"    local ok2, e2 = pcall(game.imod, 1, 0)\n"
			"    local ok3 = pcall(game.idiv, 1.5, 1)\n"
			"    local ok4 = pcall(game.idiv, 'x', 1)\n"
			"    local ok5 = pcall(game.random, 1)\n"
			"    local ok6 = pcall(game.random, 1, 2^31)\n"
			"    return not ok1 and not ok2 and not ok3 and not ok4 and not ok5 and not ok6\n"
			"      and string.find(e1, 'division by zero', 1, true) ~= nil\n"
			"      and string.find(e2, 'division by zero', 1, true) ~= nil\n"
			"  end,\n"
			"}\n");
	int Script = Load("arith.lua");
	int Pairs[][2] = {{7, 2}, {-7, 2}, {7, -2}, {-7, -2}, {0, 5}, {-1, 3}, {INT_MAX, -1}, {INT_MIN, 2},
			{INT_MIN, INT_MAX}, {-9, 10}, {100, 7}};
	for (auto &Pair : Pairs) {
		TOutcome Div = Call(Script, "on_idiv", RUN_NUMBER, {Pair[0], Pair[1]});
		TOutcome Mod = Call(Script, "on_imod", RUN_NUMBER, {Pair[0], Pair[1]});
		CHECK(!Div.Thrown && Div.Number == Pair[0] / Pair[1]);
		CHECK(!Mod.Thrown && Mod.Number == Pair[0] % Pair[1]);
	}
	TOutcome Div0 = Call(Script, "on_div0", RUN_BOOLEAN);
	CHECK(!Div0.Thrown && Div0.Boolean);
	TakeErrors();
	TOutcome Direct = Call(Script, "on_idiv", RUN_NUMBER, {1, 0});
	CHECK(Direct.Thrown && Direct.Code == ERROR);
	CHECK_CONTAINS(TakeErrors(), "game.idiv: division by zero");
	TOutcome Overflow = Call(Script, "on_idiv", RUN_NUMBER, {INT_MIN, -1});
	CHECK(Overflow.Thrown && Overflow.Code == ERROR);
	CHECK_CONTAINS(TakeErrors(), "game.idiv: overflow");
}

static void TestRunResults() {
	WriteScript("returns.lua",
			"return {\n"
			"  on_five = function() return 5 end,\n"
			"  on_negative = function() return -2147483648 end,\n"
			"  on_nil = function() end,\n"
			"  on_true = function() return true end,\n"
			"  on_false = function() return false end,\n"
			"  on_fraction = function() return 2.5 end,\n"
			"  on_big = function() return 3000000000 end,\n"
			"  on_string = function() return 'x' end,\n"
			"  on_one = function() return 1 end,\n"
			"  on_two = function() return 7, 8 end,\n"
			"}\n");
	int Script = Load("returns.lua");
	TakeErrors();
	TOutcome Five = Call(Script, "on_five", RUN_NUMBER);
	CHECK(!Five.Thrown && Five.Number == 5);
	TOutcome Negative = Call(Script, "on_negative", RUN_NUMBER);
	CHECK(!Negative.Thrown && Negative.Number == INT_MIN);
	TOutcome Two = Call(Script, "on_two", RUN_NUMBER);
	CHECK(!Two.Thrown && Two.Number == 7);
	TOutcome NilNumber = Call(Script, "on_nil", RUN_NUMBER);
	CHECK(!NilNumber.Thrown && NilNumber.Number == 0);
	TOutcome NilBoolean = Call(Script, "on_nil", RUN_BOOLEAN);
	CHECK(!NilBoolean.Thrown && !NilBoolean.Boolean);
	TOutcome True = Call(Script, "on_true", RUN_BOOLEAN);
	CHECK(!True.Thrown && True.Boolean);
	TOutcome False = Call(Script, "on_false", RUN_BOOLEAN);
	CHECK(!False.Thrown && !False.Boolean);
	TOutcome Run = Call(Script, "on_five", RUN);
	CHECK(!Run.Thrown);
	CHECK(TakeErrors() == "");

	TOutcome Fraction = Call(Script, "on_fraction", RUN_NUMBER);
	CHECK(Fraction.Thrown && Fraction.Code == ERROR);
	CHECK_CONTAINS(TakeErrors(), "Script error in returns.lua (on_fraction): on_fraction returned 2.5, not an integer");
	TOutcome Big = Call(Script, "on_big", RUN_NUMBER);
	CHECK(Big.Thrown && Big.Code == ERROR);
	CHECK_CONTAINS(TakeErrors(), "on_big returned 3000000000, not an integer");
	TOutcome StringNumber = Call(Script, "on_string", RUN_NUMBER);
	CHECK(StringNumber.Thrown && StringNumber.Code == ERROR);
	CHECK_CONTAINS(TakeErrors(), "on_string returned a string, not an integer");
	TOutcome BooleanNumber = Call(Script, "on_true", RUN_NUMBER);
	CHECK(BooleanNumber.Thrown && BooleanNumber.Code == ERROR);
	CHECK_CONTAINS(TakeErrors(), "on_true returned true, not an integer");
	TOutcome NumberBoolean = Call(Script, "on_one", RUN_BOOLEAN);
	CHECK(NumberBoolean.Thrown && NumberBoolean.Code == ERROR);
	CHECK_CONTAINS(TakeErrors(), "Script error in returns.lua (on_one): on_one returned 1, not a boolean");
}

static void TestEnvironments() {
	WriteScript("env_a.lua",
			"shared_name = 1\n"
			"local function get() return shared_name end\n"
			"return { on_get = get, on_set = function(v) shared_name = v end }\n");
	WriteScript("env_b.lua", "return { on_get = function() return shared_name end }\n");
	WriteScript("env_c.lua", "game = 5\nreturn { on_game = function() return game end }\n");
	// The shared tables reach scripts as read-only proxies, userdata.
	WriteScript("env_d.lua", "return { on_game = function() return type(game) == 'userdata' end }\n");
	int A = Load("env_a.lua");
	int B = Load("env_b.lua");
	int C = Load("env_c.lua");
	int D = Load("env_d.lua");
	TakeErrors();
	TOutcome GetA = Call(A, "on_get", RUN_NUMBER);
	CHECK(!GetA.Thrown && GetA.Number == 1);
	TOutcome SetA = Call(A, "on_set", RUN, {9});
	CHECK(!SetA.Thrown);
	TOutcome GetA2 = Call(A, "on_get", RUN_NUMBER);
	CHECK(!GetA2.Thrown && GetA2.Number == 9);
	TOutcome GetB = Call(B, "on_get", RUN_NUMBER);
	CHECK(GetB.Thrown && GetB.Code == ERROR);
	CHECK_CONTAINS(TakeErrors(), "env_b.lua:1: unknown name 'shared_name'");
	TOutcome GameC = Call(C, "on_game", RUN_NUMBER);
	CHECK(!GameC.Thrown && GameC.Number == 5);
	TOutcome GameD = Call(D, "on_game", RUN_BOOLEAN);
	CHECK(!GameD.Thrown && GameD.Boolean);
	CHECK(TakeErrors() == "");

	// ScriptAPITable gives the same table again.
	{
		sol::table First = ScriptAPITable("extra");
		First.set_function("seven", Guarded(+[]() -> int { return 7; }));
		sol::table Second = ScriptAPITable("extra");
		CHECK(Second.raw_get<sol::object>("seven").get_type() == sol::type::function);
	}
	WriteScript("extra.lua", "return { on_seven = function() return extra.seven() end }\n");
	TOutcome Seven = Call(Load("extra.lua"), "on_seven", RUN_NUMBER);
	CHECK(!Seven.Thrown && Seven.Number == 7);

	// An API table made after scripts were loaded is read-only to them too.
	{
		sol::table Late = ScriptAPITable("late");
		Late.set_function("eight", Guarded(+[]() -> int { return 8; }));
	}
	WriteScript("late.lua",
			"return { on_late = function()\n"
			"  local ok, err = pcall(function() late.eight = nil end)\n"
			"  return not ok and string.find(err, 'late.eight is read-only', 1, true) ~= nil and late.eight() == 8\n"
			"    and getmetatable(late) == false\n"
			"end }\n");
	TOutcome LateOutcome = Call(Load("late.lua"), "on_late", RUN_BOOLEAN);
	CHECK(!LateOutcome.Thrown && LateOutcome.Boolean);

	// ScriptAPITable refuses the shared globals that are not API tables.
	const char *NotAPI[] = {"string", "table", "math", "bit", "coroutine", "require", "pcall"};
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
	const char *Removed[] = {"dofile", "loadfile", "load", "loadstring", "print", "_G", "getfenv", "setfenv",
			"rawset", "newproxy", "collectgarbage", "gcinfo", "jit", "io", "os", "debug", "ffi", "package", "module"};
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
		WriteScript(Script, std::string("local x = ") + Name + "\nreturn {}\n");
		CHECK_CONTAINS(LoadError(Script.c_str()), Script + ":1: unknown name '" + Name + "'");
	}
	CHECK(TakeErrors() == "");
}

// The shared tables are read-only to scripts (rule 7 of scripting.hh).
static void TestReadOnly() {
	WriteScript("readonly.lua",
			"local writes = {\n"
			"  function() game.x = 1 end,\n"
			"  function() game.random = nil end,\n"
			"  function() result.NOROOM = 5 end,\n"
			"  function() string.upper = nil end,\n"
			"  function() table.insert = nil end,\n"
			"  function() math.floor = nil end,\n"
			"  function() bit.band = nil end,\n"
			"  function() coroutine.wrap = nil end,\n"
			"  function() test.count = nil end,\n"
			"  function() result[1] = 'x' end,\n"
			"  function() table.insert(result, 'x') end,\n"
			"  function() table.insert(string, 'x') end,\n"
			"  function() getmetatable('').__index = {} end,\n"
			"  function() setmetatable(game, {}) end,\n"
			"  function() setmetatable(string, {}) end,\n"
			"}\n"
			"return { on_write = function(i)\n"
			"  local ok, err = pcall(writes[i])\n"
			"  test.record(tostring(err))\n"
			"  return ok\n"
			"end }\n");
	int Script = Load("readonly.lua");
	const char *Expected[] = {
		"readonly.lua:2: game.x is read-only",
		"readonly.lua:3: game.random is read-only",
		"readonly.lua:4: result.NOROOM is read-only",
		"readonly.lua:5: string.upper is read-only",
		"readonly.lua:6: table.insert is read-only",
		"readonly.lua:7: math.floor is read-only",
		"readonly.lua:8: bit.band is read-only",
		"readonly.lua:9: coroutine.wrap is read-only",
		"readonly.lua:10: test.count is read-only",
		"readonly.lua:11: result.1 is read-only",
		"table expected",
		"table expected",
		"readonly.lua:14:",
		"table expected",
		"table expected",
	};
	int Count = static_cast<int>(sizeof(Expected) / sizeof(Expected[0]));
	for (int i = 0; i < Count; i++) {
		Records.clear();
		TOutcome Outcome = Call(Script, "on_write", RUN_BOOLEAN, {i + 1});
		CHECK(!Outcome.Thrown && !Outcome.Boolean);
		CHECK(Records.size() == 1);
		if (Records.size() == 1) {
			CHECK_CONTAINS(Records[0], Expected[i]);
		}
	}

	// Another script sees every value as it was.
	WriteScript("readonly_check.lua",
			"return { on_check = function()\n"
			"  return type(game.random) == 'function' and result.NOROOM == 4 and string.upper('a') == 'A'\n"
			"    and type(table.insert) == 'function' and math.floor(2.5) == 2 and bit.band(6, 3) == 2\n"
			"    and type(coroutine.wrap) == 'function' and type(test.count) == 'function'\n"
			"    and result[1] == 'NOTACCESSIBLE' and not pcall(function() return game.x end)\n"
			"    and string[1] == nil and ('x'):upper() == 'X' and ('x').upper == string.upper\n"
			"end,\n"
			"on_meta = function()\n"
			"  return getmetatable('') == false and getmetatable(game) == false and getmetatable(result) == false\n"
			"    and getmetatable(string) == false and getmetatable(table) == false and getmetatable(math) == false\n"
			"    and getmetatable(bit) == false and getmetatable(coroutine) == false and getmetatable(test) == false\n"
			"end }\n");
	int Check = Load("readonly_check.lua");
	TOutcome Values = Call(Check, "on_check", RUN_BOOLEAN);
	CHECK(!Values.Thrown && Values.Boolean);
	TOutcome Meta = Call(Check, "on_meta", RUN_BOOLEAN);
	CHECK(!Meta.Thrown && Meta.Boolean);

	// A write at a script's top level fails its load.
	WriteScript("readonly_top.lua", "local x = 1\nstring.upper = string.lower\nreturn {}\n");
	CHECK_CONTAINS(LoadError("readonly_top.lua"), "readonly_top.lua:2: string.upper is read-only");
	TOutcome After = Call(Check, "on_check", RUN_BOOLEAN);
	CHECK(!After.Thrown && After.Boolean);

	// A script's global assignment stays in its own environment: the shared
	// globals are not written, so no other script sees the name.
	WriteScript("global_set.lua",
			"pcall = 1\nnew_name = 2\nreturn { on_get = function() return pcall + new_name end }\n");
	WriteScript("global_get.lua",
			"return { on_get = function()\n"
			"  return type(pcall) == 'function' and not pcall(function() return new_name end)\n"
			"end }\n");
	TOutcome Set = Call(Load("global_set.lua"), "on_get", RUN_NUMBER);
	CHECK(!Set.Thrown && Set.Number == 3);
	TOutcome Get = Call(Load("global_get.lua"), "on_get", RUN_BOOLEAN);
	CHECK(!Get.Thrown && Get.Boolean);
	CHECK(TakeErrors() == "");
}

// Bindings wrapped with Guarded that throw a text, and sol's argument checks:
// logged non-result errors, ERROR.
static void TestGuarded() {
	WriteScript("guarded.lua",
			"return {\n"
			"  on_text = function()\n"
			"    test.throw_text()\n"
			"  end,\n"
			"  on_exception = function()\n"
			"    test.throw_exception()\n"
			"  end,\n"
			"  on_wrong_type = function()\n"
			"    local v = test.twice('x')\n"
			"    return v\n"
			"  end,\n"
			"  on_caught = function()\n"
			"    local ok, err = pcall(test.throw_text)\n"
			"    return not ok and game.result_of(err) == nil and string.find(err, 'plain text', 1, true) ~= nil\n"
			"  end,\n"
			"}\n");
	int Script = Load("guarded.lua");
	TakeErrors();
	TOutcome Text = Call(Script, "on_text", RUN);
	CHECK(Text.Thrown && Text.Code == ERROR);
	std::string Logged = TakeErrors();
	CHECK(Logged.compare(0, 39, "Script error in guarded.lua (on_text): ") == 0);
	CHECK_CONTAINS(Logged, "Script error in guarded.lua (on_text): guarded.lua:3: plain text\nstack traceback:\n");

	TOutcome Exception = Call(Script, "on_exception", RUN);
	CHECK(Exception.Thrown && Exception.Code == ERROR);
	CHECK_CONTAINS(TakeErrors(),
			"Script error in guarded.lua (on_exception): guarded.lua:6: what text\nstack traceback:\n");

	TOutcome Wrong = Call(Script, "on_wrong_type", RUN_NUMBER);
	CHECK(Wrong.Thrown && Wrong.Code == ERROR);
	Logged = TakeErrors();
	// sol's message has no position of its own; the traceback has the line.
	CHECK_CONTAINS(Logged, "Script error in guarded.lua (on_wrong_type): stack index 1, expected number");
	CHECK_CONTAINS(Logged, "\nstack traceback:\n");
	CHECK_CONTAINS(Logged, "guarded.lua:9:");

	TOutcome Caught = Call(Script, "on_caught", RUN_BOOLEAN);
	CHECK(!Caught.Thrown && Caught.Boolean);
	CHECK(TakeErrors() == "");
}

// Every text from a script or from Lua that the engine logs, or LoadScript
// throws, has each '%' doubled (scripting.hh, "Logged texts").
static void TestPercent() {
	WriteScript("percent.lua",
			"return {\n"
			"  on_log = function() game.error('a % b %s c\\n') end,\n"
			"  on_fail = function() error('50% of %s') end,\n"
			"}\n");
	int Script = Load("percent.lua");
	TakeErrors();
	TOutcome Log = Call(Script, "on_log", RUN);
	CHECK(!Log.Thrown);
	CHECK(TakeErrors() == "a %% b %%s c\n");
	TOutcome Fail = Call(Script, "on_fail", RUN);
	CHECK(Fail.Thrown && Fail.Code == ERROR);
	std::string Logged = TakeErrors();
	CHECK_CONTAINS(Logged, "Script error in percent.lua (on_fail): percent.lua:3: 50%% of %%s\nstack traceback:");
	CHECK(Logged.find("50% of") == std::string::npos);

	// The text LoadScript throws (InitAll logs it with error("...%s...")).
	WriteScript("percent_load.lua", "error('load 7% bad')\nreturn {}\n");
	std::string Thrown = LoadError("percent_load.lua", "data/x%y.toml:1:2");
	CHECK_CONTAINS(Thrown, "data/x%%y.toml:1:2: percent_load.lua:1: load 7%% bad");
	CHECK(Thrown.find("7% bad") == std::string::npos);
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
	TOutcome A = Call(Good, "on_a", RUN_NUMBER);
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
	TestResults();
	TestErrors();
	TestModuleTraceback();
	TestErrorObject();
	TestCalls();
	TestRandom();
	TestArithmetic();
	TestRunResults();
	TestEnvironments();
	TestReadOnly();
	TestGuarded();
	TestPercent();
	// Every call left Lua's stack as it found it.
	CHECK(lua_gettop(ScriptState().lua_state()) == BaseTop);
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
