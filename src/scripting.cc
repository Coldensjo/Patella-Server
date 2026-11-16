// scripting.cc: the Lua engine (scripting.hh, scripting_api.hh). One LuaJIT
// state with the sandbox of rule 7, strict shared globals and API tables, the
// core API (Game, print, debugPrint, math.random, the RETURNVALUE_
// constants), the objects (Creature, Player, Npc, Item, Vocation, ItemType),
// the Position,
// the read-only constants, the restricted require, the loaded scripts with
// their environments, and the calls of their handlers (TScriptCall,
// TScriptEvent).
//
// The functions Lua calls (the core API, the metamethods, require, the message
// handlers) use only Lua's C API and plain C data: they throw no C++ exception
// and hold no C++ object with a destructor when they raise a Lua error.
//
// The shared tables (the API tables and the libraries string, table, math,
// bit, coroutine) reach the scripts as read-only proxies: an empty userdata
// whose metatable reads from the real table (__index), refuses writes
// (__newindex) and hides itself (__metatable). A userdata, not a table: the
// table library writes raw (table.insert(t, v) is a lua_rawseti), so a proxy
// table would take raw writes that every script then reads; a userdata it
// refuses ("table expected"). type() of a shared table is therefore
// "userdata".

#include <cassert>
#include <cerrno>
#include <climits>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <pthread.h>
#include <string>
#include <sys/stat.h>
#include <sys/types.h>
#include <vector>

#include "objects.hh"
#include "map.hh"
#include "info.hh"
#include "utils.hh"
#include "crmain.hh"
#include "scripting_api.hh"

// The longest script name LoadScript and require accept.
const int MAX_SCRIPT_NAME = 256;

// The registry keys of the engine's own Lua values.
static const char *const RESULT_METATABLE = "patella.result";
static const char *const ENVIRONMENT_METATABLE = "patella.environment";
static const char *const MODULES = "patella.modules";
static const char *const REQUIRE_STACK = "patella.require_stack";
// The real API tables, by name (the shared globals hold their proxies).
static const char *const API_TABLES = "patella.api_tables";
// The names of the read-only constants (SetScriptConstant), each true.
static const char *const CONSTANTS = "patella.constants";
// The kinds of the objects by their metatables (TScriptObjectKind numbers).
static const char *const OBJECT_KINDS = "patella.object_kinds";
// The metatable of the Positions.
static const char *const POSITION_METATABLE = "patella.position";
// The pcall message handler of the entry points (MessageHandler), made once
// so that a call pushes it without allocating.
static const char *const MESSAGE_HANDLER = "patella.message_handler";
// The last error a module raised that the module message handler traced: a
// table whose [1] is the error value and [2] its text with the traceback
// (made with room for both, so clearing it never allocates).
static const char *const TRACED_ERROR = "patella.traced_error";

// A loaded script: its name and the registry reference of its environment,
// which holds its handlers.
struct TScript {
	std::string Name;
	int Environment;
};

static sol::state *State = nullptr;
static pthread_t MainThread;
static std::string Folder;
static std::vector<TScript> Scripts;
// The text of the last const char * thrown by LoadScript or RequireHandler.
static std::string ErrorText;
// The number of modules require is loading now: the entries 1 to
// RequireStackDepth of the registry's REQUIRE_STACK.
static int RequireStackDepth = 0;

// The scripts' names of RESULT (info.hh), RETURNVALUE_ and TFS's name where
// it means the same (RETURNVALUE_NOTPOSSIBLE is NOTACCESSIBLE), our own
// otherwise, in the order of their numbers. The checks below fail to compile
// when an entry is missing, misplaced or out of step with ERROR and LOWLEVEL,
// the first and last values of RESULT.
struct TResultName {
	RESULT Code;
	const char *Name;
};

#define RESULT_NAME(Code, Name) {Code, "RETURNVALUE_" #Name}
static constexpr TResultName ResultNames[] = {
	RESULT_NAME(ERROR, ERROR),
	RESULT_NAME(NOERROR, NOERROR),
	RESULT_NAME(NOTACCESSIBLE, NOTPOSSIBLE),
	RESULT_NAME(NOTMOVABLE, NOTMOVEABLE),
	RESULT_NAME(NOTTAKABLE, CANNOTPICKUP),
	RESULT_NAME(NOROOM, NOTENOUGHROOM),
	RESULT_NAME(OUTOFRANGE, DESTINATIONOUTOFREACH),
	RESULT_NAME(OUTOFHOME, OUTOFHOME),
	RESULT_NAME(CANNOTTHROW, CANNOTTHROW),
	RESULT_NAME(TOOHEAVY, NOTENOUGHCAPACITY),
	RESULT_NAME(CROSSREFERENCE, THISISIMPOSSIBLE),
	RESULT_NAME(CONTAINERFULL, CONTAINERNOTENOUGHROOM),
	RESULT_NAME(WRONGPOSITION, PUTTHISOBJECTINYOURHAND),
	RESULT_NAME(WRONGPOSITION2, PUTTHISOBJECTINBOTHHANDS),
	RESULT_NAME(WRONGCLOTHES, CANNOTBEDRESSED),
	RESULT_NAME(HANDSNOTFREE, BOTHHANDSNEEDTOBEFREE),
	RESULT_NAME(HANDBLOCKED, DROPTWOHANDEDITEM),
	RESULT_NAME(ONEWEAPONONLY, CANONLYUSEONEWEAPON),
	RESULT_NAME(NOMATCH, NOMATCH),
	RESULT_NAME(NOTCUMULABLE, NOTCUMULABLE),
	RESULT_NAME(TOOMANYPARTS, TOOMANYPARTS),
	RESULT_NAME(EMPTYCONTAINER, EMPTYCONTAINER),
	RESULT_NAME(SPLITOBJECT, SPLITOBJECT),
	RESULT_NAME(NOKEYMATCH, NOKEYMATCH),
	RESULT_NAME(UPSTAIRS, FIRSTGOUPSTAIRS),
	RESULT_NAME(DOWNSTAIRS, FIRSTGODOWNSTAIRS),
	RESULT_NAME(CREATURENOTEXISTING, CREATUREDOESNOTEXIST),
	RESULT_NAME(PLAYERNOTEXISTING, PLAYERNOTEXISTING),
	RESULT_NAME(PLAYERNOTONLINE, PLAYERWITHTHISNAMEISNOTONLINE),
	RESULT_NAME(NAMEAMBIGUOUS, NAMEISTOOAMBIGUOUS),
	RESULT_NAME(NOTUSABLE, CANNOTUSETHISOBJECT),
	RESULT_NAME(FEDUP, FEDUP),
	RESULT_NAME(DESTROYED, DESTROYED),
	RESULT_NAME(SPELLUNKNOWN, YOUNEEDTOLEARNTHISSPELL),
	RESULT_NAME(LOWMAGICLEVEL, NOTENOUGHMAGICLEVEL),
	RESULT_NAME(MAGICITEM, YOUNEEDAMAGICITEMTOCASTSPELL),
	RESULT_NAME(NOTENOUGHMANA, NOTENOUGHMANA),
	RESULT_NAME(NOSKILL, NOSKILL),
	RESULT_NAME(TARGETLOST, TARGETLOST),
	RESULT_NAME(OUTOFAMMO, OUTOFAMMO),
	RESULT_NAME(NOCREATURE, CANONLYUSETHISRUNEONCREATURES),
	RESULT_NAME(TOOLONG, TOOLONG),
	RESULT_NAME(TARGETOUTOFRANGE, TARGETOUTOFRANGE),
	RESULT_NAME(TARGETHIDDEN, TARGETHIDDEN),
	RESULT_NAME(ATTACKNOTALLOWED, YOUMAYNOTATTACKTHISPLAYER),
	RESULT_NAME(NOWAY, THEREISNOWAY),
	RESULT_NAME(LOGINERROR, LOGINERROR),
	RESULT_NAME(LOGINABORT, LOGINABORT),
	RESULT_NAME(PROTECTIONZONE, ACTIONNOTPERMITTEDINPROTECTIONZONE),
	RESULT_NAME(ENTERPROTECTIONZONE, ENTERPROTECTIONZONE),
	RESULT_NAME(EXHAUSTED, YOUAREEXHAUSTED),
	RESULT_NAME(NOTINVITED, NOTINVITED),
	RESULT_NAME(NOPREMIUMACCOUNT, YOUNEEDPREMIUMACCOUNT),
	RESULT_NAME(MOVENOTPOSSIBLE, MOVENOTPOSSIBLE),
	RESULT_NAME(ALREADYTRADING, YOUAREALREADYTRADING),
	RESULT_NAME(PARTNERTRADING, THISPLAYERISALREADYTRADING),
	RESULT_NAME(TOOMANYOBJECTS, TOOMANYOBJECTS),
	RESULT_NAME(TOOMANYSLAVES, TOOMANYSLAVES),
	RESULT_NAME(NOTTURNABLE, NOTTURNABLE),
	RESULT_NAME(SECUREMODE, TURNSECUREMODETOATTACKUNMARKEDPLAYERS),
	RESULT_NAME(NOTENOUGHSOULPOINTS, NOTENOUGHSOUL),
	RESULT_NAME(LOWLEVEL, NOTENOUGHLEVEL),
};
#undef RESULT_NAME

const int NUMBER_OF_RESULTS = static_cast<int>(sizeof(ResultNames) / sizeof(ResultNames[0]));

// Whether entry i of ResultNames has the number ERROR + i.
static constexpr bool ResultNamesInOrder() {
	for (int i = 0; i < NUMBER_OF_RESULTS; i++) {
		if (static_cast<int>(ResultNames[i].Code) != static_cast<int>(ERROR) + i) {
			return false;
		}
	}
	return true;
}

static_assert(NUMBER_OF_RESULTS == static_cast<int>(LOWLEVEL) - static_cast<int>(ERROR) + 1,
		"ResultNames must name every value of RESULT from ERROR to LOWLEVEL");
static_assert(ResultNames[NUMBER_OF_RESULTS - 1].Code == LOWLEVEL, "ResultNames must end with LOWLEVEL");
static_assert(ResultNamesInOrder(), "ResultNames must be in the order of the numbers of RESULT");

// Asserts (debug builds) that the engine is used from the thread that ran
// InitScripting (rule 1), while it is active. The entry points that need an
// active engine assert ScriptingActive before this.
static inline void AssertMainThread() {
	assert(State == nullptr || pthread_equal(pthread_self(), MainThread));
}

// Text with each '%' doubled, for the texts that reach error() (and so Log)
// as part of a format string (scripting.hh, "Logged texts").
static std::string DoublePercent(const std::string &Text) {
	std::string Result;
	Result.reserve(Text.size());
	for (char c : Text) {
		Result += c;
		if (c == '%') {
			Result += '%';
		}
	}
	return Result;
}

// The name of the result code Code, or nullptr for a number that is not one.
static const char *ResultName(int Code) {
	if (Code < static_cast<int>(ERROR) || Code > static_cast<int>(LOWLEVEL)) {
		return nullptr;
	}
	return ResultNames[Code - static_cast<int>(ERROR)].Name;
}

// Sets Code to the number of the result named Name; false for a name that is
// not one.
static bool ResultCode(const char *Name, int *Code) {
	for (int i = 0; i < NUMBER_OF_RESULTS; i++) {
		if (strcmp(ResultNames[i].Name, Name) == 0) {
			*Code = static_cast<int>(ResultNames[i].Code);
			return true;
		}
	}
	return false;
}

// The text of a result code as an error value shows it: "result
// RETURNVALUE_NOTENOUGHROOM", or
// "result <number>" for a number that is not a result code.
static std::string ResultText(int Code) {
	const char *Name = ResultName(Code);
	if (Name != nullptr) {
		return std::string("result ") + Name;
	}
	return "result " + std::to_string(Code);
}

// Makes a stack index that may be relative (negative) absolute.
static int AbsoluteIndex(lua_State *L, int Index) {
	if (Index < 0 && Index > LUA_REGISTRYINDEX) {
		return lua_gettop(L) + Index + 1;
	}
	return Index;
}

// Whether the value at Index is a result-error value (RaiseResult); sets
// *Code to its code then, when Code is not nullptr.
static bool IsResultError(lua_State *L, int Index, int *Code) {
	Index = AbsoluteIndex(L, Index);
	if (lua_type(L, Index) != LUA_TUSERDATA || !lua_getmetatable(L, Index)) {
		return false;
	}
	lua_getfield(L, LUA_REGISTRYINDEX, RESULT_METATABLE);
	bool Same = lua_rawequal(L, -1, -2) != 0;
	lua_pop(L, 2);
	if (Same && Code != nullptr) {
		memcpy(Code, lua_touserdata(L, Index), sizeof(int));
	}
	return Same;
}

// The text of the error value at Index, read without calling Lua: a string or
// number as it is, a result error as ResultText, anything else as "(error
// object is a <type> value)".
static std::string ErrorValueText(lua_State *L, int Index) {
	int Code;
	if (IsResultError(L, Index, &Code)) {
		return ResultText(Code);
	}
	int Type = lua_type(L, Index);
	if (Type == LUA_TSTRING || Type == LUA_TNUMBER) {
		size_t Length;
		lua_pushvalue(L, Index);
		const char *Text = lua_tolstring(L, -1, &Length);
		std::string Result(Text, Length);
		lua_pop(L, 1);
		return Result;
	}
	return std::string("(error object is a ") + luaL_typename(L, Index) + " value)";
}

// Throws ErrorText, set to "<Where>: <What>" (or What without Where) with
// each '%' doubled, as a const char *: InitAll logs the text it catches with
// error("...%s...", Text), and error() hands that to Log as its format
// string, so the logged line shows one '%' (the text itself shows two).
[[noreturn]] static void ThrowError(const char *Where, const std::string &What) {
	if (Where != nullptr) {
		ErrorText = DoublePercent(std::string(Where) + ": " + What);
	} else {
		ErrorText = DoublePercent(What);
	}
	throw ErrorText.c_str();
}

// Whether Name is a script name: only a-z, 0-9, '_', '-', '/' and '.'; parts
// between single '/' that are neither empty nor "." nor ".."; the ending
// ".lua" after a file name; at most MAX_SCRIPT_NAME characters.
static bool ValidScriptName(const char *Name) {
	size_t Length = strlen(Name);
	if (Length > static_cast<size_t>(MAX_SCRIPT_NAME) || Length < 4 || strcmp(Name + Length - 4, ".lua") != 0) {
		return false;
	}

	size_t PartStart = 0;
	for (size_t i = 0; i <= Length; i++) {
		char c = Name[i];
		if (c == '/' || c == 0) {
			size_t PartLength = i - PartStart;
			const char *Part = Name + PartStart;
			if (PartLength == 0 || (PartLength == 1 && Part[0] == '.')
					|| (PartLength == 2 && Part[0] == '.' && Part[1] == '.')) {
				return false;
			}
			if (c == 0 && PartLength <= 4) {
				// The file name is only ".lua".
				return false;
			}
			PartStart = i + 1;
		} else if (!((c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || c == '_' || c == '-' || c == '.')) {
			return false;
		}
	}
	return true;
}

// Writes the script name of the module Module ("lib.x" gives "lib/x.lua")
// into Name (Size bytes); false when Module has a character other than a-z,
// 0-9, '_', '-' and '.' or the name is not a valid script name.
static bool ModuleScriptName(const char *Module, char *Name, size_t Size) {
	size_t Length = strlen(Module);
	if (Length + 5 > Size) {
		return false;
	}
	for (size_t i = 0; i < Length; i++) {
		char c = Module[i];
		if (c == '.') {
			Name[i] = '/';
		} else if ((c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || c == '_' || c == '-') {
			Name[i] = c;
		} else {
			return false;
		}
	}
	memcpy(Name + Length, ".lua", 5);
	return ValidScriptName(Name);
}


// Loads the script Name from the scripts folder as a function and pushes it;
// on failure pushes the message instead and returns false. The chunk name is
// "@<Name>", so Lua's messages and tracebacks begin "<Name>:<line>:". The
// file is read into a Lua userdata (luaL_loadbuffer then compiles it), so
// nothing leaks on a Lua error; binary (precompiled) chunks are refused.
static bool LoadChunk(lua_State *L, const char *Name) {
	char Path[4096 + MAX_SCRIPT_NAME + 2];
	int PathLength = snprintf(Path, sizeof(Path), "%s/%s", Folder.c_str(), Name);
	if (PathLength < 0 || PathLength >= static_cast<int>(sizeof(Path))) {
		lua_pushfstring(L, "cannot open %s: path too long", Name);
		return false;
	}

	struct stat FileStatus;
	if (stat(Path, &FileStatus) != 0) {
		lua_pushfstring(L, "cannot open %s: %s", Name, strerror(errno));
		return false;
	}
	if (!S_ISREG(FileStatus.st_mode)) {
		lua_pushfstring(L, "cannot open %s: not a file", Name);
		return false;
	}

	size_t Size = static_cast<size_t>(FileStatus.st_size);
	char *Buffer = static_cast<char *>(lua_newuserdata(L, Size > 0 ? Size : 1));
	FILE *File = fopen(Path, "rb");
	if (File == nullptr) {
		lua_pop(L, 1);
		lua_pushfstring(L, "cannot open %s: %s", Name, strerror(errno));
		return false;
	}
	size_t Read = fread(Buffer, 1, Size, File);
	bool Failed = ferror(File) != 0 || Read != Size;
	fclose(File);
	if (Failed) {
		lua_pop(L, 1);
		lua_pushfstring(L, "cannot read %s", Name);
		return false;
	}
	if (Size > 0 && Buffer[0] == '\033') {
		lua_pop(L, 1);
		lua_pushfstring(L, "cannot load %s: binary chunks are not allowed", Name);
		return false;
	}

	lua_pushfstring(L, "@%s", Name);
	int Status = luaL_loadbuffer(L, Buffer, Size, lua_tostring(L, -1));
	// The stack is: buffer, chunk name, function or message.
	lua_replace(L, -3);
	lua_pop(L, 1);
	return Status == 0;
}

// Pushes a new script environment: an empty table whose reads fall back to
// the shared globals (and so are strict) and whose writes of a constant's
// name are refused (EnvironmentNewIndex).
static void PushEnvironment(lua_State *L) {
	lua_newtable(L);
	lua_getfield(L, LUA_REGISTRYINDEX, ENVIRONMENT_METATABLE);
	lua_setmetatable(L, -2);
}

// Pushes the text of the key at Index for the messages of the strict tables:
// a string or number as it is, anything else as "(<type>)".
static const char *PushKeyText(lua_State *L, int Index) {
	int Type = lua_type(L, Index);
	if (Type == LUA_TSTRING || Type == LUA_TNUMBER) {
		lua_pushvalue(L, Index);
		return lua_tostring(L, -1);
	}
	return lua_pushfstring(L, "(%s)", luaL_typename(L, Index));
}

// __index of the shared globals: reading a name they lack (directly or through
// a script environment) is the Lua error "unknown name '<name>'", at the line
// that reads it.
static int StrictGlobalIndex(lua_State *L) {
	return luaL_error(L, "unknown name '%s'", PushKeyText(L, 2));
}

// __index of an API table, whose name is its upvalue: reading a field it lacks
// is the Lua error "unknown name '<table>.<field>'", at the line that reads it.
static int StrictAPIIndex(lua_State *L) {
	const char *Table = lua_tostring(L, lua_upvalueindex(1));
	return luaL_error(L, "unknown name '%s.%s'", Table, PushKeyText(L, 2));
}

// __newindex of a read-only proxy, whose table's name is its upvalue: writing
// a field is the Lua error "<table>.<field> is read-only", at the line that
// writes it.
static int ReadOnlyNewIndex(lua_State *L) {
	const char *Table = lua_tostring(L, lua_upvalueindex(1));
	return luaL_error(L, "%s.%s is read-only", Table, PushKeyText(L, 2));
}

// Pushes a read-only proxy of the table at Index, named Name in its messages:
// an empty userdata whose metatable reads from the table (so an API table's
// strict __index still applies), makes every write an error and cannot be
// read or replaced by a script (getmetatable gives false; setmetatable takes
// only tables).
static void PushProxy(lua_State *L, int Index, const char *Name) {
	Index = AbsoluteIndex(L, Index);
	lua_newuserdata(L, 0);
	lua_createtable(L, 0, 3);
	lua_pushvalue(L, Index);
	lua_setfield(L, -2, "__index");
	lua_pushstring(L, Name);
	lua_pushcclosure(L, ReadOnlyNewIndex, 1);
	lua_setfield(L, -2, "__newindex");
	lua_pushboolean(L, 0);
	lua_setfield(L, -2, "__metatable");
	lua_setmetatable(L, -2);
}

// Replaces the shared global Name, a table, by its read-only proxy.
static void MakeGlobalReadOnly(lua_State *L, const char *Name) {
	lua_pushstring(L, Name);
	lua_pushstring(L, Name);
	lua_rawget(L, LUA_GLOBALSINDEX);
	PushProxy(L, -1, Name);
	lua_remove(L, -2);
	lua_rawset(L, LUA_GLOBALSINDEX);
}

// The shared globals the libraries make that the sandbox removes. _G, getfenv
// and setfenv would let a script reach the shared globals or another
// script's environment past its own; rawset would write past the read-only
// proxies; newproxy gives userdata with __gc, whose finalizers run (and may
// call rand()) whenever the collector runs, in another handler;
// collectgarbage and gcinfo control or read the collector, which is shared.
// Lua's print (it writes to stdout) is replaced by the engine's (LuaPrint).
static const char *const RemovedGlobals[] = {"dofile", "loadfile", "load", "loadstring", "_G", "getfenv",
		"setfenv", "rawset", "newproxy", "collectgarbage", "gcinfo", LUA_JITLIBNAME};

// The globals of the libraries the sandbox does not open (rule 7).
static const char *const NeverOpenedGlobals[] = {"io", "os", "debug", "ffi", "package", "module"};

// Whether Name is a global the sandbox removed or never opened.
static bool SandboxRemovedName(const char *Name) {
	for (const char *Removed : RemovedGlobals) {
		if (strcmp(Name, Removed) == 0) {
			return true;
		}
	}
	for (const char *Removed : NeverOpenedGlobals) {
		if (strcmp(Name, Removed) == 0) {
			return true;
		}
	}
	return false;
}

// Pushes the API table Name: the real table behind the shared global Name's
// read-only proxy. When there is none yet, makes it, empty with a strict
// metatable, and sets the shared global Name to its proxy. Throws a const
// char * when the shared global Name is something other than an API table,
// or is a name the sandbox removed or never opened (the stack is as before
// then).
static void PushAPITable(lua_State *L, const char *Name) {
	lua_getfield(L, LUA_REGISTRYINDEX, API_TABLES);
	lua_getfield(L, -1, Name);
	if (lua_istable(L, -1)) {
		lua_remove(L, -2);
		return;
	}
	lua_pop(L, 1);
	if (SandboxRemovedName(Name)) {
		lua_pop(L, 1);
		ThrowError(nullptr, std::string("ScriptAPITable: the name ") + Name + " is removed by the sandbox");
	}
	lua_pushstring(L, Name);
	lua_rawget(L, LUA_GLOBALSINDEX);
	bool Taken = !lua_isnil(L, -1);
	lua_pop(L, 1);
	if (Taken) {
		lua_pop(L, 1);
		ThrowError(nullptr, std::string("ScriptAPITable: the shared global ") + Name + " is not an API table");
	}

	lua_newtable(L);
	lua_newtable(L);
	lua_pushstring(L, Name);
	lua_pushcclosure(L, StrictAPIIndex, 1);
	lua_setfield(L, -2, "__index");
	lua_pushboolean(L, 0);
	lua_setfield(L, -2, "__metatable");
	lua_setmetatable(L, -2);
	// The stack is: API_TABLES, the table.
	lua_pushvalue(L, -1);
	lua_setfield(L, -3, Name);
	lua_pushstring(L, Name);
	PushProxy(L, -2, Name);
	lua_rawset(L, LUA_GLOBALSINDEX);
	lua_remove(L, -2);
}

// Forgets the error recorded in TRACED_ERROR. Allocates nothing, so it may be
// called outside a protected call.
static void ClearTracedError(lua_State *L) {
	lua_getfield(L, LUA_REGISTRYINDEX, TRACED_ERROR);
	lua_pushnil(L);
	lua_rawseti(L, -2, 1);
	lua_pushnil(L);
	lua_rawseti(L, -2, 2);
	lua_pop(L, 1);
}

// When the error value at Index (absolute) is the one recorded in
// TRACED_ERROR, pushes its recorded text with the traceback and returns true;
// otherwise pushes nothing and returns false.
static bool PushRecordedTrace(lua_State *L, int Index) {
	lua_getfield(L, LUA_REGISTRYINDEX, TRACED_ERROR);
	lua_rawgeti(L, -1, 1);
	bool Same = !lua_isnil(L, -1) && lua_rawequal(L, -1, Index);
	lua_pop(L, 1);
	if (Same) {
		lua_rawgeti(L, -1, 2);
		lua_remove(L, -2);
		return true;
	}
	lua_pop(L, 1);
	return false;
}

// Pushes the text of the error value at stack index 1 followed by Lua's
// traceback from the function that raised it. The text is read without
// calling Lua (no __tostring, no other metamethod, so no script code runs
// while an error is handled): a string or number as it is, anything else as
// "(error object is a <type> value)".
static void PushTraceback(lua_State *L) {
	int Type = lua_type(L, 1);
	if (Type == LUA_TSTRING || Type == LUA_TNUMBER) {
		// A copy: lua_tostring turns a number into a string in place.
		lua_pushvalue(L, 1);
		lua_tostring(L, -1);
	} else {
		lua_pushfstring(L, "(error object is a %s value)", lua_typename(L, Type));
	}
	luaL_traceback(L, L, lua_tostring(L, -1), 1);
	lua_remove(L, -2);
}

// The pcall message handler of the entry points (LoadScript, TScriptCall): a
// result error stays as it is; an error a module raised that
// ModuleMessageHandler traced becomes the text it recorded (so the traceback
// is the one taken where the error was raised, inside the module, and there
// is one); any other error becomes its text followed by Lua's traceback
// (PushTraceback).
static int MessageHandler(lua_State *L) {
	lua_settop(L, 1);
	if (IsResultError(L, 1, nullptr)) {
		return 1;
	}
	if (PushRecordedTrace(L, 1)) {
		return 1;
	}
	PushTraceback(L);
	return 1;
}

// The pcall message handler of require, which runs a module: the error value
// stays as it is (require raises it again, and a script's pcall of require
// gets what the module raised). A non-result error that is not the one
// recorded already (raised again by a nested require) is recorded in
// TRACED_ERROR with its text and the traceback taken here, where it was
// raised, for MessageHandler.
static int ModuleMessageHandler(lua_State *L) {
	lua_settop(L, 1);
	if (IsResultError(L, 1, nullptr)) {
		return 1;
	}
	if (PushRecordedTrace(L, 1)) {
		lua_settop(L, 1);
		return 1;
	}
	PushTraceback(L);
	lua_getfield(L, LUA_REGISTRYINDEX, TRACED_ERROR);
	lua_pushvalue(L, 1);
	lua_rawseti(L, -2, 1);
	lua_pushvalue(L, -2);
	lua_rawseti(L, -2, 2);
	lua_settop(L, 1);
	return 1;
}

// require(module): the value of the script "lib/x.lua" for "lib.x"
// (scripting.hh). Runs the module once, in an environment of its own, and
// keeps what it returned (true for nil) for later calls. Lua errors: a module
// name that is not allowed, a module that cannot be loaded, a module that
// requires itself (directly or through others), and whatever the module
// raises (the same error value, raised again once the module has left the
// stack of modules being loaded, so a failed require can be retried). A
// non-result error of the module is traced where it was raised
// (ModuleMessageHandler).
static int LuaRequire(lua_State *L) {
	const char *Module = luaL_checkstring(L, 1);
	char Name[MAX_SCRIPT_NAME + 1];
	if (!ModuleScriptName(Module, Name, sizeof(Name))) {
		return luaL_error(L, "require: module name '%s' is not allowed", Module);
	}

	lua_settop(L, 1);
	lua_getfield(L, LUA_REGISTRYINDEX, MODULES);       // 2: the loaded modules
	lua_getfield(L, 2, Name);
	if (!lua_isnil(L, -1)) {
		return 1;
	}
	lua_pop(L, 1);

	lua_getfield(L, LUA_REGISTRYINDEX, REQUIRE_STACK); // 3: the modules being loaded
	int Depth = RequireStackDepth;
	for (int i = 1; i <= Depth; i++) {
		lua_rawgeti(L, 3, i);
		bool Same = strcmp(lua_tostring(L, -1), Name) == 0;
		lua_pop(L, 1);
		if (Same) {
			// "<where>require: 'lib.a' requires itself: lib/a.lua -> lib/b.lua -> lib/a.lua"
			luaL_checkstack(L, 2 * (Depth - i + 1) + 3, "require: too many modules");
			luaL_where(L, 1);
			lua_pushfstring(L, "require: '%s' requires itself: ", Module);
			for (int j = i; j <= Depth; j++) {
				lua_rawgeti(L, 3, j);
				lua_pushliteral(L, " -> ");
			}
			lua_pushstring(L, Name);
			lua_concat(L, 2 * (Depth - i + 1) + 3);
			return lua_error(L);
		}
	}

	if (!LoadChunk(L, Name)) {                         // 4: the message
		luaL_where(L, 1);
		lua_pushfstring(L, "require '%s': ", Module);
		lua_pushvalue(L, 4);
		lua_concat(L, 3);
		return lua_error(L);
	}
	PushEnvironment(L);
	lua_setfenv(L, 4);
	lua_pushcfunction(L, ModuleMessageHandler);
	lua_insert(L, 4);                                  // 4: the message handler, 5: the module
	lua_pushstring(L, Name);
	lua_rawseti(L, 3, Depth + 1);
	RequireStackDepth = Depth + 1;
	// An error recorded before is not this module's.
	ClearTracedError(L);
	int Status = lua_pcall(L, 0, 1, 4);                // 5: what the module returned, or the error
	lua_pushnil(L);
	lua_rawseti(L, 3, Depth + 1);
	RequireStackDepth = Depth;
	if (Status != 0) {
		return lua_error(L);
	}
	if (lua_isnil(L, 5)) {
		lua_pop(L, 1);
		lua_pushboolean(L, 1);
	}
	lua_pushvalue(L, 5);
	lua_setfield(L, 2, Name);
	return 1;
}

// The argument at Index as a C++ int: a number without fraction in the range
// of int; anything else is a Lua error.
static int CheckInt(lua_State *L, int Index) {
	if (lua_type(L, Index) != LUA_TNUMBER) {
		return luaL_typerror(L, Index, "number");
	}
	double Value = lua_tonumber(L, Index);
	if (!(Value >= INT_MIN && Value <= INT_MAX) || Value != floor(Value)) {
		return luaL_argerror(L, Index, "integer expected");
	}
	return static_cast<int>(Value);
}

// Pushes the text of a result code as an error value shows it ("result
// RETURNVALUE_NOTENOUGHROOM", or "result <number>" for a number that is not
// one), with Lua's own functions only.
static void PushResultText(lua_State *L, int Code) {
	const char *Name = ResultName(Code);
	if (Name != nullptr) {
		lua_pushfstring(L, "result %s", Name);
	} else {
		lua_pushfstring(L, "result %d", Code);
	}
}

// __tostring of a result error: "result RETURNVALUE_<NAME>".
static int ResultErrorToString(lua_State *L) {
	int Code;
	memcpy(&Code, lua_touserdata(L, 1), sizeof(int));
	PushResultText(L, Code);
	return 1;
}

// __index of a result error: err.code is its code; any other field is nil.
static int ResultErrorIndex(lua_State *L) {
	if (lua_type(L, 2) == LUA_TSTRING && strcmp(lua_tostring(L, 2), "code") == 0) {
		int Code;
		memcpy(&Code, lua_touserdata(L, 1), sizeof(int));
		lua_pushinteger(L, Code);
	} else {
		lua_pushnil(L);
	}
	return 1;
}

// ---------------------------------------------------------------------------
// The objects: a userdata of one uint32_t (a creature ID of a Creature, Player
// or Npc, an object handle, a Vocation's player ID, or an ItemType's type
// number, an int kept as its bits) whose metatable is its class's (no __gc, so
// it is trivially destructible and the collector runs no code). The metatable
// reads methods from the class's method table, an API table (so an unknown
// method is "unknown name '<class>.<method>'"), refuses writes, compares by
// kind and value (__eq), names the object without an address (__tostring) and
// is hidden from scripts (__metatable).

// The class names of the kinds (TScriptObjectKind order).
static const char *const ObjectKindNames[] = {nullptr, "Creature", "Player", "Item", "Vocation", "ItemType",
		"Npc"};

// The registry keys of the kinds' metatables.
static const char *const ObjectMetatables[] = {nullptr, "patella.object.Creature", "patella.object.Player",
		"patella.object.Item", "patella.object.Vocation", "patella.object.ItemType", "patella.object.Npc"};

static_assert(sizeof(ObjectKindNames) / sizeof(ObjectKindNames[0]) == LAST_SCRIPT_OBJECT_KIND + 1,
		"ObjectKindNames must name every TScriptObjectKind");
static_assert(sizeof(ObjectMetatables) / sizeof(ObjectMetatables[0]) == LAST_SCRIPT_OBJECT_KIND + 1,
		"ObjectMetatables must have a key for every TScriptObjectKind");

TScriptObjectKind ScriptObjectKind(lua_State *L, int Index) {
	Index = AbsoluteIndex(L, Index);
	if (lua_type(L, Index) != LUA_TUSERDATA || !lua_getmetatable(L, Index)) {
		return SCRIPT_NO_OBJECT;
	}
	lua_getfield(L, LUA_REGISTRYINDEX, OBJECT_KINDS);
	lua_insert(L, -2);
	lua_rawget(L, -2);
	int Kind = lua_type(L, -1) == LUA_TNUMBER ? static_cast<int>(lua_tointeger(L, -1)) : SCRIPT_NO_OBJECT;
	lua_pop(L, 2);
	return static_cast<TScriptObjectKind>(Kind);
}

uint32_t ScriptObjectValue(lua_State *L, int Index) {
	if (ScriptObjectKind(L, Index) == SCRIPT_NO_OBJECT) {
		return 0;
	}
	uint32_t Value;
	memcpy(&Value, lua_touserdata(L, Index), sizeof(Value));
	return Value;
}

void PushScriptObject(lua_State *L, TScriptObjectKind Kind, uint32_t Value) {
	assert(Kind > SCRIPT_NO_OBJECT && Kind <= LAST_SCRIPT_OBJECT_KIND);
	void *Data = lua_newuserdata(L, sizeof(Value));
	memcpy(Data, &Value, sizeof(Value));
	lua_getfield(L, LUA_REGISTRYINDEX, ObjectMetatables[Kind]);
	lua_setmetatable(L, -2);
}

void PushScriptCreature(lua_State *L, uint32_t ID) {
	if (ID == 0) {
		lua_pushnil(L);
	} else if (ID < static_cast<uint32_t>(FIRST_NONPLAYER_CREATURE_ID)) {
		PushScriptObject(L, SCRIPT_PLAYER, ID);
	} else {
		PushScriptObject(L, SCRIPT_CREATURE, ID);
	}
}

// Pushes the text of an object: "Creature 123", "Player 123", "Npc 123",
// "Item 456", "Vocation of Player 123", "ItemType 123" (the type number as
// the int it is, so "ItemType -1").
static void PushObjectText(lua_State *L, TScriptObjectKind Kind, uint32_t Value) {
	char Text[64];
	if (Kind == SCRIPT_VOCATION) {
		snprintf(Text, sizeof(Text), "Vocation of Player %u", static_cast<unsigned>(Value));
	} else if (Kind == SCRIPT_ITEMTYPE) {
		snprintf(Text, sizeof(Text), "ItemType %d", static_cast<int>(Value));
	} else {
		snprintf(Text, sizeof(Text), "%s %u", ObjectKindNames[Kind], static_cast<unsigned>(Value));
	}
	lua_pushstring(L, Text);
}

// __tostring of the objects (rule 4: no address).
static int ObjectToString(lua_State *L) {
	PushObjectText(L, ScriptObjectKind(L, 1), ScriptObjectValue(L, 1));
	return 1;
}

// __eq of the objects, one function in every class's metatable (Lua calls
// __eq only when both operands have the same one): the same kind and value.
static int ObjectEqual(lua_State *L) {
	TScriptObjectKind Kind = ScriptObjectKind(L, 1);
	lua_pushboolean(L, Kind != SCRIPT_NO_OBJECT && Kind == ScriptObjectKind(L, 2)
			&& ScriptObjectValue(L, 1) == ScriptObjectValue(L, 2));
	return 1;
}

// __index of the method table of a class derived from another (Player and Npc
// from Creature): a method it lacks is read from the parent's method table
// (raw, one level), and one neither has is the Lua error "unknown name
// '<class>.<field>'". Upvalues: the class's name and the parent's table.
static int DerivedClassIndex(lua_State *L) {
	lua_pushvalue(L, 2);
	lua_rawget(L, lua_upvalueindex(2));
	if (!lua_isnil(L, -1)) {
		return 1;
	}
	const char *Table = lua_tostring(L, lua_upvalueindex(1));
	return luaL_error(L, "unknown name '%s.%s'", Table, PushKeyText(L, 2));
}

// Makes the class of Kind: its method table (the API table of its name,
// derived from Parent's unless that is SCRIPT_NO_OBJECT; a parent is made
// first) and its objects' metatable, whose __eq is the function at the
// stack index Equal.
static void MakeClass(lua_State *L, TScriptObjectKind Kind, TScriptObjectKind Parent, int Equal) {
	const char *Name = ObjectKindNames[Kind];
	PushAPITable(L, Name);
	if (Parent != SCRIPT_NO_OBJECT) {
		lua_getmetatable(L, -1);
		lua_pushstring(L, Name);
		PushAPITable(L, ObjectKindNames[Parent]);
		lua_pushcclosure(L, DerivedClassIndex, 2);
		lua_setfield(L, -2, "__index");
		lua_pop(L, 1);
	}
	luaL_newmetatable(L, ObjectMetatables[Kind]);
	lua_pushvalue(L, -2);
	lua_setfield(L, -2, "__index");
	lua_pushstring(L, Name);
	lua_pushcclosure(L, ReadOnlyNewIndex, 1);
	lua_setfield(L, -2, "__newindex");
	lua_pushvalue(L, Equal);
	lua_setfield(L, -2, "__eq");
	lua_pushcfunction(L, ObjectToString);
	lua_setfield(L, -2, "__tostring");
	lua_pushboolean(L, 0);
	lua_setfield(L, -2, "__metatable");
	lua_getfield(L, LUA_REGISTRYINDEX, OBJECT_KINDS);
	lua_pushvalue(L, -2);
	lua_pushinteger(L, Kind);
	lua_rawset(L, -3);
	lua_pop(L, 3);
}

// creature:getId(): the creature ID the object holds, also for a creature
// that is gone. Argument: a Creature, a Player or an Npc. Returns the number.
static int CreatureGetId(lua_State *L) {
	TScriptObjectKind Kind = ScriptObjectKind(L, 1);
	if (Kind != SCRIPT_CREATURE && Kind != SCRIPT_PLAYER && Kind != SCRIPT_NPC) {
		return luaL_typerror(L, 1, "Creature");
	}
	lua_pushnumber(L, ScriptObjectValue(L, 1));
	return 1;
}

// The function ItemType(id) makes the type number with
// (SetItemTypeConstructor); nullptr keeps id as it is.
static int (*ItemTypeConstructor)(int) = nullptr;

void SetItemTypeConstructor(int (*Constructor)(int)) {
	AssertMainThread();
	ItemTypeConstructor = Constructor;
}

// ItemType(id), the __call of the shared global ItemType (the proxy of the
// class's method table, which is argument 1): a new ItemType holding the
// type number ItemTypeConstructor makes of id, any int (the game's is
// ObjectType(id).getTypeID(), so a number that is no type is logged there and
// gives 0). Returns it. Any other number of arguments, or an id that is not an
// int, is a Lua error.
static int ItemTypeNew(lua_State *L) {
	if (lua_gettop(L) != 2) {
		return luaL_error(L, "ItemType: one argument expected (id)");
	}
	int TypeID = CheckInt(L, 2);
	if (ItemTypeConstructor != nullptr) {
		TypeID = ItemTypeConstructor(TypeID);
	}
	PushScriptObject(L, SCRIPT_ITEMTYPE, static_cast<uint32_t>(TypeID));
	return 1;
}

// Makes the shared global ItemType callable: ItemType(id) (ItemTypeNew). The
// class (MakeClass) must exist.
static void MakeItemTypeConstructor(lua_State *L) {
	lua_pushstring(L, ObjectKindNames[SCRIPT_ITEMTYPE]);
	lua_rawget(L, LUA_GLOBALSINDEX);
	lua_getmetatable(L, -1);
	lua_pushcfunction(L, ItemTypeNew);
	lua_setfield(L, -2, "__call");
	lua_pop(L, 2);
}

// ---------------------------------------------------------------------------
// The Position: a userdata of three ints (x, y, z) whose metatable gives the
// fields x, y and z (any other field is the Lua error "unknown name
// 'Position.<field>'"), refuses writes, compares by value (__eq), names it
// without an address (__tostring "Position(x, y, z)") and is hidden from
// scripts (__metatable). No __gc.

bool IsScriptPosition(lua_State *L, int Index) {
	Index = AbsoluteIndex(L, Index);
	if (lua_type(L, Index) != LUA_TUSERDATA || !lua_getmetatable(L, Index)) {
		return false;
	}
	lua_getfield(L, LUA_REGISTRYINDEX, POSITION_METATABLE);
	bool Same = lua_rawequal(L, -1, -2) != 0;
	lua_pop(L, 2);
	return Same;
}

void GetScriptPosition(lua_State *L, int Index, int *x, int *y, int *z) {
	int Values[3] = {0, 0, 0};
	if (IsScriptPosition(L, Index)) {
		memcpy(Values, lua_touserdata(L, Index), sizeof(Values));
	}
	*x = Values[0];
	*y = Values[1];
	*z = Values[2];
}

void PushScriptPosition(lua_State *L, int x, int y, int z) {
	int Values[3] = {x, y, z};
	void *Data = lua_newuserdata(L, sizeof(Values));
	memcpy(Data, Values, sizeof(Values));
	lua_getfield(L, LUA_REGISTRYINDEX, POSITION_METATABLE);
	lua_setmetatable(L, -2);
}

// Pushes the text of the Position at Index: "Position(x, y, z)".
static void PushPositionText(lua_State *L, int Index) {
	int x, y, z;
	GetScriptPosition(L, Index, &x, &y, &z);
	lua_pushfstring(L, "Position(%d, %d, %d)", x, y, z);
}

// Position(x, y, z): a new Position. Arguments: three ints. Returns it. Any
// other number of arguments is a Lua error.
static int PositionNew(lua_State *L) {
	if (lua_gettop(L) != 3) {
		return luaL_error(L, "Position: three arguments expected (x, y, z)");
	}
	int x = CheckInt(L, 1);
	int y = CheckInt(L, 2);
	int z = CheckInt(L, 3);
	PushScriptPosition(L, x, y, z);
	return 1;
}

// __index of the Positions: position.x, .y, .z.
static int PositionIndex(lua_State *L) {
	int x, y, z;
	GetScriptPosition(L, 1, &x, &y, &z);
	if (lua_type(L, 2) == LUA_TSTRING) {
		const char *Field = lua_tostring(L, 2);
		if (strcmp(Field, "x") == 0) {
			lua_pushinteger(L, x);
			return 1;
		}
		if (strcmp(Field, "y") == 0) {
			lua_pushinteger(L, y);
			return 1;
		}
		if (strcmp(Field, "z") == 0) {
			lua_pushinteger(L, z);
			return 1;
		}
	}
	return luaL_error(L, "unknown name 'Position.%s'", PushKeyText(L, 2));
}

// __eq of the Positions: the same three coordinates.
static int PositionEqual(lua_State *L) {
	bool Equal = false;
	if (IsScriptPosition(L, 1) && IsScriptPosition(L, 2)) {
		int x1, y1, z1, x2, y2, z2;
		GetScriptPosition(L, 1, &x1, &y1, &z1);
		GetScriptPosition(L, 2, &x2, &y2, &z2);
		Equal = x1 == x2 && y1 == y2 && z1 == z2;
	}
	lua_pushboolean(L, Equal);
	return 1;
}

// __tostring of the Positions.
static int PositionToString(lua_State *L) {
	PushPositionText(L, 1);
	return 1;
}

// Makes the Positions' metatable and the shared global Position.
static void MakePosition(lua_State *L) {
	luaL_newmetatable(L, POSITION_METATABLE);
	lua_pushcfunction(L, PositionIndex);
	lua_setfield(L, -2, "__index");
	lua_pushstring(L, "Position");
	lua_pushcclosure(L, ReadOnlyNewIndex, 1);
	lua_setfield(L, -2, "__newindex");
	lua_pushcfunction(L, PositionEqual);
	lua_setfield(L, -2, "__eq");
	lua_pushcfunction(L, PositionToString);
	lua_setfield(L, -2, "__tostring");
	lua_pushboolean(L, 0);
	lua_setfield(L, -2, "__metatable");
	lua_pop(L, 1);
	lua_pushcfunction(L, PositionNew);
	lua_setfield(L, LUA_GLOBALSINDEX, "Position");
}

// ---------------------------------------------------------------------------
// The constants

// Sets the shared global Name to the constant Value and records it in
// CONSTANTS; false (and nothing set) when the shared global Name exists or is
// a name the sandbox removed.
static bool SetConstant(lua_State *L, const char *Name, int Value) {
	if (SandboxRemovedName(Name)) {
		return false;
	}
	lua_pushstring(L, Name);
	lua_rawget(L, LUA_GLOBALSINDEX);
	bool Taken = !lua_isnil(L, -1);
	lua_pop(L, 1);
	if (Taken) {
		return false;
	}
	lua_pushstring(L, Name);
	lua_pushinteger(L, Value);
	lua_rawset(L, LUA_GLOBALSINDEX);
	lua_getfield(L, LUA_REGISTRYINDEX, CONSTANTS);
	lua_pushboolean(L, 1);
	lua_setfield(L, -2, Name);
	lua_pop(L, 1);
	return true;
}

// __newindex of the script environments: a global a script sets stays in its
// environment, except a constant's name, which is the Lua error "<name> is
// read-only", at the line that writes it.
static int EnvironmentNewIndex(lua_State *L) {
	if (lua_type(L, 2) == LUA_TSTRING) {
		lua_getfield(L, LUA_REGISTRYINDEX, CONSTANTS);
		lua_pushvalue(L, 2);
		lua_rawget(L, -2);
		bool Constant = lua_toboolean(L, -1) != 0;
		lua_pop(L, 2);
		if (Constant) {
			return luaL_error(L, "%s is read-only", lua_tostring(L, 2));
		}
	}
	lua_settop(L, 3);
	lua_rawset(L, 1);
	return 0;
}

// ---------------------------------------------------------------------------
// The core API

// math.random(m, n): random(m, n) of utils.cc, m + rand() % (n - m + 1), m
// for an empty span; math.random(m): random(1, m). Arguments: ints. Returns
// the int. No arguments, or more than two, is a Lua error (rule 3).
static int MathRandom(lua_State *L) {
	int Count = lua_gettop(L);
	if (Count == 1) {
		int High = CheckInt(L, 1);
		lua_pushinteger(L, random(1, High));
		return 1;
	}
	if (Count == 2) {
		int Low = CheckInt(L, 1);
		int High = CheckInt(L, 2);
		lua_pushinteger(L, random(Low, High));
		return 1;
	}
	return luaL_error(L, "math.random: one or two arguments expected");
}

// The level print(...) writes with: 2, the level of the game's informational
// messages (the talk lines, the connection messages).
const int SCRIPT_PRINT_LEVEL = 2;

// print(...): its arguments as text, separated by tabs and ended by a
// newline, as Lua's print writes them, given to print(SCRIPT_PRINT_LEVEL,
// "%s", Text) of utils.cc. Strings and numbers as tostring gives them,
// booleans, nil, the objects ("Player 123"), the Positions ("Position(x, y,
// z)") and result errors ("result RETURNVALUE_..."); a table, a function, a coroutine or any other userdata
// is a Lua error, so no address is ever printed (rule 4). Calls no
// metamethod. Returns nothing.
static int LuaPrint(lua_State *L) {
	int Count = lua_gettop(L);
	luaL_checkstack(L, 2 * Count + 2, "print: too many arguments");
	for (int i = 1; i <= Count; i++) {
		if (i > 1) {
			lua_pushliteral(L, "\t");
		}
		int Type = lua_type(L, i);
		int Code;
		TScriptObjectKind Kind = ScriptObjectKind(L, i);
		if (Type == LUA_TSTRING || Type == LUA_TNUMBER) {
			lua_pushvalue(L, i);
			lua_tostring(L, -1);
		} else if (Type == LUA_TBOOLEAN) {
			lua_pushstring(L, lua_toboolean(L, i) ? "true" : "false");
		} else if (Type == LUA_TNIL) {
			lua_pushliteral(L, "nil");
		} else if (Kind != SCRIPT_NO_OBJECT) {
			PushObjectText(L, Kind, ScriptObjectValue(L, i));
		} else if (IsScriptPosition(L, i)) {
			PushPositionText(L, i);
		} else if (IsResultError(L, i, &Code)) {
			PushResultText(L, Code);
		} else {
			return luaL_error(L, "print: argument %d is a %s, which has no text", i, luaL_typename(L, i));
		}
	}
	lua_pushliteral(L, "\n");
	lua_concat(L, lua_gettop(L) - Count);
	print(SCRIPT_PRINT_LEVEL, "%s", lua_tostring(L, -1));
	return 0;
}

// debugPrint(text): error("%s", Text) of utils.cc, with each '%' of Text
// doubled (error() hands the text to Log as its format string). Argument: a
// string. Returns nothing.
static int DebugPrint(lua_State *L) {
	const char *Text = luaL_checkstring(L, 1);
	error("%s", luaL_gsub(L, Text, "%", "%%"));
	return 0;
}

// Game.rand(): rand(), as the C++ code calls it directly. Returns the int.
static int GameRand(lua_State *L) {
	lua_pushinteger(L, rand());
	return 1;
}

// Game.idiv(a, b): a / b of C++ ints, truncated toward zero. Arguments: ints.
// Returns the int. b == 0, and INT_MIN / -1 (which C++ cannot compute), are
// Lua errors.
static int GameIdiv(lua_State *L) {
	int a = CheckInt(L, 1);
	int b = CheckInt(L, 2);
	if (b == 0) {
		return luaL_error(L, "Game.idiv: division by zero");
	}
	if (a == INT_MIN && b == -1) {
		return luaL_error(L, "Game.idiv: overflow");
	}
	lua_pushinteger(L, a / b);
	return 1;
}

// Game.imod(a, b): a % b of C++ ints, with the sign of a. Arguments: ints.
// Returns the int. b == 0, and INT_MIN % -1 (which C++ cannot compute), are
// Lua errors.
static int GameImod(lua_State *L) {
	int a = CheckInt(L, 1);
	int b = CheckInt(L, 2);
	if (b == 0) {
		return luaL_error(L, "Game.imod: division by zero");
	}
	if (a == INT_MIN && b == -1) {
		return luaL_error(L, "Game.imod: overflow");
	}
	lua_pushinteger(L, a % b);
	return 1;
}

// Game.print(level, text): print(Level, "%s", Text) of utils.cc. Arguments:
// an int and a string. Returns nothing.
static int GamePrint(lua_State *L) {
	int Level = CheckInt(L, 1);
	const char *Text = luaL_checkstring(L, 2);
	print(Level, "%s", Text);
	return 0;
}

// Game.throwResult(code): raises the result code, a number of RESULT or its
// name ("RETURNVALUE_NOTENOUGHROOM"), as a C++ throw of it would: a Lua error
// whose value carries the code. Does not return; a value that is not a
// result code is a plain Lua error.
static int GameThrowResult(lua_State *L) {
	int Code;
	int Type = lua_type(L, 1);
	if (Type == LUA_TSTRING) {
		const char *Name = lua_tostring(L, 1);
		if (!ResultCode(Name, &Code)) {
			return luaL_error(L, "Game.throwResult: unknown result '%s'", Name);
		}
	} else if (Type == LUA_TNUMBER) {
		double Value = lua_tonumber(L, 1);
		if (!(Value >= ERROR && Value <= LOWLEVEL) || Value != floor(Value)) {
			return luaL_error(L, "Game.throwResult: %s is not a result code", lua_tostring(L, 1));
		}
		Code = static_cast<int>(Value);
	} else {
		return luaL_typerror(L, 1, "result code");
	}
	RaiseResult(L, Code);
}

// Game.getResultOf(err): the result code the error value err (from pcall)
// carries, as a number, or nil for another error.
static int GameGetResultOf(lua_State *L) {
	int Code;
	if (lua_gettop(L) >= 1 && IsResultError(L, 1, &Code)) {
		lua_pushinteger(L, Code);
	} else {
		lua_pushnil(L);
	}
	return 1;
}

// A C function and its name: the core API in the table Game, and the
// libraries of the sandbox.
struct TCoreFunction {
	const char *Name;
	lua_CFunction Function;
};

static const TCoreFunction GameFunctions[] = {
	{"rand", GameRand},
	{"idiv", GameIdiv},
	{"imod", GameImod},
	{"print", GamePrint},
	{"throwResult", GameThrowResult},
	{"getResultOf", GameGetResultOf},
};

// The libraries of the sandbox (rule 7), opened as luaL_openlibs opens them.
// luaopen_jit switches the JIT compiler on; its table jit is removed after.
static const TCoreFunction Libraries[] = {
	{"", luaopen_base},
	{LUA_TABLIBNAME, luaopen_table},
	{LUA_STRLIBNAME, luaopen_string},
	{LUA_MATHLIBNAME, luaopen_math},
	{LUA_BITLIBNAME, luaopen_bit},
	{LUA_JITLIBNAME, luaopen_jit},
};

// The libraries the scripts reach through read-only proxies.
static const char *const ReadOnlyLibraries[] = {LUA_STRLIBNAME, LUA_TABLIBNAME, LUA_MATHLIBNAME, LUA_BITLIBNAME,
		LUA_COLIBNAME};

// Builds the sandbox, the metatables, require, the core API, the classes and
// the RETURNVALUE_ constants in the new state L.
static void SetUpState(lua_State *L) {
	for (const TCoreFunction &Library : Libraries) {
		lua_pushcfunction(L, Library.Function);
		lua_pushstring(L, Library.Name);
		lua_call(L, 1, 0);
	}
	for (const char *Name : RemovedGlobals) {
		lua_pushnil(L);
		lua_setfield(L, LUA_GLOBALSINDEX, Name);
	}
	lua_pushcfunction(L, LuaPrint);
	lua_setfield(L, LUA_GLOBALSINDEX, "print");
	lua_pushcfunction(L, DebugPrint);
	lua_setfield(L, LUA_GLOBALSINDEX, "debugPrint");
	lua_getfield(L, LUA_GLOBALSINDEX, LUA_STRLIBNAME);
	lua_pushnil(L);
	lua_setfield(L, -2, "dump");
	lua_pop(L, 1);
	lua_getfield(L, LUA_GLOBALSINDEX, LUA_MATHLIBNAME);
	lua_pushcfunction(L, MathRandom);
	lua_setfield(L, -2, "random");
	lua_pushnil(L);
	lua_setfield(L, -2, "randomseed");
	lua_pop(L, 1);

	// The libraries become read-only. The strings' metatable keeps the real
	// string library as its __index (("x"):upper()) and cannot be read, so it
	// cannot be changed either.
	for (const char *Name : ReadOnlyLibraries) {
		MakeGlobalReadOnly(L, Name);
	}
	lua_pushliteral(L, "");
	lua_getmetatable(L, -1);
	lua_pushboolean(L, 0);
	lua_setfield(L, -2, "__metatable");
	lua_pop(L, 2);

	// The result errors: a userdata holding the code.
	luaL_newmetatable(L, RESULT_METATABLE);
	lua_pushcfunction(L, ResultErrorToString);
	lua_setfield(L, -2, "__tostring");
	lua_pushcfunction(L, ResultErrorIndex);
	lua_setfield(L, -2, "__index");
	lua_pushboolean(L, 0);
	lua_setfield(L, -2, "__metatable");
	lua_pop(L, 1);

	// The script environments read the shared globals and keep the
	// constants read-only.
	lua_newtable(L);
	lua_pushvalue(L, LUA_GLOBALSINDEX);
	lua_setfield(L, -2, "__index");
	lua_pushcfunction(L, EnvironmentNewIndex);
	lua_setfield(L, -2, "__newindex");
	lua_pushboolean(L, 0);
	lua_setfield(L, -2, "__metatable");
	lua_setfield(L, LUA_REGISTRYINDEX, ENVIRONMENT_METATABLE);

	lua_newtable(L);
	lua_setfield(L, LUA_REGISTRYINDEX, MODULES);
	lua_newtable(L);
	lua_setfield(L, LUA_REGISTRYINDEX, REQUIRE_STACK);
	lua_newtable(L);
	lua_setfield(L, LUA_REGISTRYINDEX, API_TABLES);
	lua_newtable(L);
	lua_setfield(L, LUA_REGISTRYINDEX, CONSTANTS);
	lua_newtable(L);
	lua_setfield(L, LUA_REGISTRYINDEX, OBJECT_KINDS);
	lua_createtable(L, 2, 0);
	lua_pushboolean(L, 0);
	lua_rawseti(L, -2, 1);
	lua_pushboolean(L, 0);
	lua_rawseti(L, -2, 2);
	lua_setfield(L, LUA_REGISTRYINDEX, TRACED_ERROR);
	lua_pushcfunction(L, LuaRequire);
	lua_setfield(L, LUA_GLOBALSINDEX, "require");
	lua_pushcfunction(L, MessageHandler);
	lua_setfield(L, LUA_REGISTRYINDEX, MESSAGE_HANDLER);

	PushAPITable(L, "Game");
	for (const TCoreFunction &Function : GameFunctions) {
		lua_pushcfunction(L, Function.Function);
		lua_setfield(L, -2, Function.Name);
	}
	lua_pop(L, 1);

	for (const TResultName &Result : ResultNames) {
		SetConstant(L, Result.Name, static_cast<int>(Result.Code));
	}

	lua_pushcfunction(L, ObjectEqual);
	int Equal = lua_gettop(L);
	MakeClass(L, SCRIPT_CREATURE, SCRIPT_NO_OBJECT, Equal);
	MakeClass(L, SCRIPT_PLAYER, SCRIPT_CREATURE, Equal);
	MakeClass(L, SCRIPT_ITEM, SCRIPT_NO_OBJECT, Equal);
	MakeClass(L, SCRIPT_VOCATION, SCRIPT_NO_OBJECT, Equal);
	MakeClass(L, SCRIPT_ITEMTYPE, SCRIPT_NO_OBJECT, Equal);
	MakeClass(L, SCRIPT_NPC, SCRIPT_CREATURE, Equal);
	lua_pop(L, 1);
	MakeItemTypeConstructor(L);
	PushAPITable(L, ObjectKindNames[SCRIPT_CREATURE]);
	lua_pushcfunction(L, CreatureGetId);
	lua_setfield(L, -2, "getId");
	lua_pop(L, 1);
	MakePosition(L);

	// Last, the shared globals become strict.
	lua_newtable(L);
	lua_pushcfunction(L, StrictGlobalIndex);
	lua_setfield(L, -2, "__index");
	lua_pushboolean(L, 0);
	lua_setfield(L, -2, "__metatable");
	lua_setmetatable(L, LUA_GLOBALSINDEX);
}

// SetUpState as a function for lua_cpcall, so that a Lua error while setting
// up (out of memory) is returned instead of ending the program.
static int ProtectedSetUpState(lua_State *L) {
	lua_settop(L, 0);
	SetUpState(L);
	return 0;
}

void InitScripting(const char *Folder) {
	if (State != nullptr) {
		throw "InitScripting: the scripting engine is active already";
	}
	// sol::state uses the state luaL_newstate gives without checking it (a
	// null state would crash), so Lua is asked for one first.
	lua_State *Probe = luaL_newstate();
	if (Probe == nullptr) {
		throw "InitScripting: Lua cannot create its state";
	}
	lua_close(Probe);
	sol::state *NewState = nullptr;
	try {
		NewState = new sol::state();
	} catch (const std::exception &) {
		throw "InitScripting: Lua cannot create its state";
	}
	// Set up in a protected call; on failure the half-made state is closed
	// and the engine stays inactive, so InitScripting may be tried again.
	lua_State *L = NewState->lua_state();
	if (lua_cpcall(L, ProtectedSetUpState, nullptr) != 0) {
		std::string Message = ErrorValueText(L, -1);
		delete NewState;
		ThrowError(nullptr, "InitScripting: Lua cannot set up its state: " + Message);
	}
	lua_settop(L, 0);
	MainThread = pthread_self();
	::Folder = Folder;
	Scripts.clear();
	RequireStackDepth = 0;
	State = NewState;
}

void ExitScripting() {
	if (State == nullptr) {
		return;
	}
	AssertMainThread();
	delete State;
	State = nullptr;
	Scripts.clear();
	Folder.clear();
	RequireStackDepth = 0;
	ItemTypeConstructor = nullptr;
}

bool ScriptingActive() {
	return State != nullptr;
}

int LoadScript(const char *Name, const char *Where) {
	assert(ScriptingActive());
	AssertMainThread();
	if (!ScriptingActive()) {
		ThrowError(Where, "the scripting engine is not active");
	}
	if (Name == nullptr || !ValidScriptName(Name)) {
		ThrowError(Where, std::string("script name '") + (Name != nullptr ? Name : "") + "' is not allowed");
	}
	for (size_t i = 0; i < Scripts.size(); i++) {
		if (Scripts[i].Name == Name) {
			return static_cast<int>(i);
		}
	}

	lua_State *L = State->lua_state();
	int Top = lua_gettop(L);
	lua_pushcfunction(L, MessageHandler);
	if (!LoadChunk(L, Name)) {
		std::string Message = ErrorValueText(L, -1);
		lua_settop(L, Top);
		ThrowError(Where, Message);
	}
	// The stack is: the message handler, the environment, the chunk.
	PushEnvironment(L);
	lua_insert(L, -2);
	lua_pushvalue(L, -2);
	lua_setfenv(L, -2);
	int Depth = RequireStackDepth;
	ClearTracedError(L);
	int Status = lua_pcall(L, 0, LUA_MULTRET, Top + 1);
	ClearTracedError(L);
	// require takes its modules off the stack whether they fail or not.
	assert(RequireStackDepth == Depth);
	(void) Depth;
	if (Status != 0) {
		std::string Message = ErrorValueText(L, -1);
		lua_settop(L, Top);
		ThrowError(Where, Message);
	}
	for (int i = Top + 3; i <= lua_gettop(L); i++) {
		if (!lua_isnil(L, i)) {
			lua_settop(L, Top);
			ThrowError(Where, std::string(Name) + ": a script returns nothing; its handlers are global functions");
		}
	}
	lua_settop(L, Top + 2);
	int Environment = luaL_ref(L, LUA_REGISTRYINDEX);
	lua_settop(L, Top);
	Scripts.push_back(TScript{Name, Environment});
	return static_cast<int>(Scripts.size()) - 1;
}

const char *ScriptName(int Script) {
	AssertMainThread();
	assert(Script >= 0 && Script < static_cast<int>(Scripts.size()));
	if (Script < 0 || Script >= static_cast<int>(Scripts.size())) {
		return nullptr;
	}
	return Scripts[Script].Name.c_str();
}

// Pushes the handler Handler of the script Script (a raw field of its
// environment, its global of that name, so no metamethod runs and no shared
// global counts), or nil when there is none.
static void PushHandler(lua_State *L, int Script, const char *Handler) {
	lua_rawgeti(L, LUA_REGISTRYINDEX, Scripts[Script].Environment);
	lua_pushstring(L, Handler);
	lua_rawget(L, -2);
	lua_remove(L, -2);
}

bool HasHandler(int Script, const char *Handler) {
	assert(ScriptingActive());
	AssertMainThread();
	assert(Script >= 0 && Script < static_cast<int>(Scripts.size()));
	if (!ScriptingActive() || Script < 0 || Script >= static_cast<int>(Scripts.size())) {
		return false;
	}
	lua_State *L = State->lua_state();
	PushHandler(L, Script, Handler);
	bool Result = lua_isfunction(L, -1);
	lua_pop(L, 1);
	return Result;
}

void RequireHandler(int Script, const char *Handler, const char *Where) {
	assert(ScriptingActive());
	AssertMainThread();
	if (!HasHandler(Script, Handler)) {
		const char *Name = ScriptName(Script);
		ThrowError(Where, std::string(Name != nullptr ? Name : "(no script)") + " has no function " + Handler);
	}
}

sol::state &ScriptState() {
	assert(ScriptingActive());
	AssertMainThread();
	return *State;
}

sol::table ScriptAPITable(const char *Name) {
	assert(ScriptingActive());
	AssertMainThread();
	if (!ScriptingActive()) {
		throw "ScriptAPITable: the scripting engine is not active";
	}
	lua_State *L = State->lua_state();
	PushAPITable(L, Name);
	sol::table Table(L, -1);
	lua_pop(L, 1);
	return Table;
}

void SetScriptConstant(const char *Name, int Value) {
	assert(ScriptingActive());
	AssertMainThread();
	if (!ScriptingActive()) {
		throw "SetScriptConstant: the scripting engine is not active";
	}
	if (!SetConstant(State->lua_state(), Name, Value)) {
		ThrowError(nullptr, std::string("SetScriptConstant: the name ") + Name + " is taken or removed by the sandbox");
	}
}

void RaiseResult(lua_State *L, int Code) {
	int *Value = static_cast<int *>(lua_newuserdata(L, sizeof(int)));
	*Value = Code;
	lua_getfield(L, LUA_REGISTRYINDEX, RESULT_METATABLE);
	lua_setmetatable(L, -2);
	lua_error(L);
	// lua_error does not return.
	abort();
}

void RaiseError(lua_State *L, const char *Text) {
	lua_pushstring(L, Text);
	luaL_where(L, 1);
	lua_insert(L, -2);
	lua_concat(L, 2);
	lua_error(L);
	// lua_error does not return.
	abort();
}

// The Type of a TScriptCall argument that is a Position (not a Lua type).
const int POSITION_ARGUMENT = -100;

// An argument of a TScriptCall: its Lua type and value (LUA_TTABLE: an array
// of the strings Strings; LUA_TUSERDATA: an object of the kind Kind holding
// Object; POSITION_ARGUMENT: a Position of the coordinates Position).
struct TScriptCall::TArgument {
	int Type;
	double Number;
	bool Boolean;
	std::string String;
	std::vector<std::string> Strings;
	TScriptObjectKind Kind = SCRIPT_NO_OBJECT;
	uint32_t Object = 0;
	int Position[3] = {0, 0, 0};

	// Pushes the value; an array as a new table, which takes one more stack
	// slot while it is filled.
	void push(lua_State *L) const {
		switch (Type) {
		case LUA_TTABLE:
			lua_createtable(L, static_cast<int>(Strings.size()), 0);
			for (size_t i = 0; i < Strings.size(); i++) {
				lua_pushlstring(L, Strings[i].data(), Strings[i].size());
				lua_rawseti(L, -2, static_cast<int>(i) + 1);
			}
			break;
		case LUA_TUSERDATA:
			PushScriptObject(L, Kind, Object);
			break;
		case POSITION_ARGUMENT:
			PushScriptPosition(L, Position[0], Position[1], Position[2]);
			break;
		case LUA_TNUMBER:
			lua_pushnumber(L, Number);
			break;
		case LUA_TBOOLEAN:
			lua_pushboolean(L, Boolean);
			break;
		case LUA_TSTRING:
			lua_pushlstring(L, String.data(), String.size());
			break;
		default:
			lua_pushnil(L);
			break;
		}
	}
};

// What a handler returned first: its Lua type and value.
struct TCallResult {
	int Type;
	double Number;
	bool Boolean;
};

// Gives a new, nil argument at the end of a TScriptCall's arguments, doubling
// the array when it is full. (A template, so it needs not name the private
// TScriptCall::TArgument.)
template<typename T>
static T &AppendArgument(T *&Arguments, int &NumberOfArguments, int &Capacity) {
	if (NumberOfArguments == Capacity) {
		int NewCapacity = Capacity > 0 ? 2 * Capacity : 4;
		T *NewArguments = new T[NewCapacity];
		for (int i = 0; i < NumberOfArguments; i++) {
			NewArguments[i] = std::move(Arguments[i]);
		}
		delete[] Arguments;
		Arguments = NewArguments;
		Capacity = NewCapacity;
	}
	T &Argument = Arguments[NumberOfArguments++];
	Argument.Type = LUA_TNIL;
	return Argument;
}

// The message of the last non-result error of a handler call (without the
// traceback) and the number of such errors so far (TScriptCall::failure).
static std::string FailureText;
static unsigned FailureCount = 0;

// Logs a non-result error of a handler call with error() as "Script error in
// <script name> (<handler>): <Message>", each '%' of Message doubled (it
// comes from Lua and reaches Log as part of the format string), records it
// for TScriptCall::failure and throws ERROR.
[[noreturn]] static void ScriptFailure(int Script, const char *Handler, const std::string &Message) {
	const char *Name = "(no script)";
	if (Script >= 0 && Script < static_cast<int>(Scripts.size())) {
		Name = Scripts[Script].Name.c_str();
	}
	error("Script error in %s (%s): %s\n", Name, Handler, DoublePercent(Message).c_str());
	FailureText = Message.substr(0, Message.find("\nstack traceback:"));
	FailureCount++;
	throw ERROR;
}

// Runs the call prepared on L's stack above Top (the message handler at Top +
// 1, the handler, NumberOfArguments arguments), sets Result to the handler's
// first result and leaves the stack at Top. Throws the result code the
// handler raised as RESULT; any other failure goes to ScriptFailure.
static void FinishCall(lua_State *L, int Top, int Script, const char *Handler, int NumberOfArguments,
		TCallResult *Result) {
	int Depth = RequireStackDepth;
	ClearTracedError(L);
	int Status = lua_pcall(L, NumberOfArguments, 1, Top + 1);
	ClearTracedError(L);
	// require takes its modules off the stack whether they fail or not.
	assert(RequireStackDepth == Depth);
	(void) Depth;
	if (Status != 0) {
		int Code;
		if (IsResultError(L, -1, &Code) && ResultName(Code) != nullptr) {
			lua_settop(L, Top);
			throw static_cast<RESULT>(Code);
		}
		std::string Message = ErrorValueText(L, -1);
		lua_settop(L, Top);
		ScriptFailure(Script, Handler, Message);
	}

	Result->Type = lua_type(L, -1);
	Result->Number = Result->Type == LUA_TNUMBER ? lua_tonumber(L, -1) : 0;
	Result->Boolean = Result->Type == LUA_TBOOLEAN && lua_toboolean(L, -1);
	lua_settop(L, Top);
}

// Pushes the message handler and the handler Handler of the script Script on
// L's stack, after checking that the engine is active, Script exists, there
// is room for NumberOfArguments more values and the handler is a function;
// returns the stack top before them. Failures go to ScriptFailure (the stack
// as before). Allocates nothing: the message handler is the one function
// made at start (MESSAGE_HANDLER), the handler's name an existing string.
static int PrepareCall(int Script, const char *Handler, int NumberOfArguments) {
	if (!ScriptingActive()) {
		ScriptFailure(Script, Handler, "the scripting engine is not active");
	}
	if (Script < 0 || Script >= static_cast<int>(Scripts.size())) {
		ScriptFailure(Script, Handler, "there is no script number " + std::to_string(Script));
	}

	lua_State *L = State->lua_state();
	int Top = lua_gettop(L);
	if (!lua_checkstack(L, NumberOfArguments + 3)) {
		ScriptFailure(Script, Handler, "too many arguments");
	}
	lua_getfield(L, LUA_REGISTRYINDEX, MESSAGE_HANDLER);
	PushHandler(L, Script, Handler);
	if (!lua_isfunction(L, -1)) {
		lua_settop(L, Top);
		ScriptFailure(Script, Handler, Scripts[Script].Name + " has no function " + Handler);
	}
	return Top;
}

// Calls the handler Handler of the script Script with the arguments and sets
// Result to its first result. Throws the result code the handler raised as
// RESULT; any other failure goes to ScriptFailure.
template<typename T>
static void CallHandler(int Script, const char *Handler, const T *Arguments, int NumberOfArguments,
		TCallResult *Result) {
	int Top = PrepareCall(Script, Handler, NumberOfArguments);
	lua_State *L = State->lua_state();
	for (int i = 0; i < NumberOfArguments; i++) {
		Arguments[i].push(L);
	}

	FinishCall(L, Top, Script, Handler, NumberOfArguments, Result);
}

// The text of a handler's result for the messages of runNumber and runBoolean:
// the value, or its type.
static std::string ResultValueText(const TCallResult &Result) {
	if (Result.Type == LUA_TNUMBER) {
		char Text[64];
		snprintf(Text, sizeof(Text), "%.17g", Result.Number);
		return Text;
	}
	if (Result.Type == LUA_TBOOLEAN) {
		return Result.Boolean ? "true" : "false";
	}
	return std::string("a ") + lua_typename(State->lua_state(), Result.Type);
}

TScriptCall::TScriptCall(int Script, const char *Handler) :
		Script(Script), Handler(Handler), Arguments(nullptr), NumberOfArguments(0), Capacity(0), Done(false),
		FailuresBefore(0) {
	AssertMainThread();
}

const char *TScriptCall::failure() const {
	AssertMainThread();
	if (!Done || FailureCount == FailuresBefore) {
		return nullptr;
	}
	return FailureText.c_str();
}

TScriptCall::~TScriptCall() {
	AssertMainThread();
	delete[] Arguments;
}

void TScriptCall::addNumber(int Value) {
	AssertMainThread();
	assert(!Done);
	TArgument &Argument = AppendArgument(Arguments, NumberOfArguments, Capacity);
	Argument.Type = LUA_TNUMBER;
	Argument.Number = Value;
}

void TScriptCall::addNumber(uint32_t Value) {
	AssertMainThread();
	assert(!Done);
	TArgument &Argument = AppendArgument(Arguments, NumberOfArguments, Capacity);
	Argument.Type = LUA_TNUMBER;
	Argument.Number = Value;
}

void TScriptCall::addBoolean(bool Value) {
	AssertMainThread();
	assert(!Done);
	TArgument &Argument = AppendArgument(Arguments, NumberOfArguments, Capacity);
	Argument.Type = LUA_TBOOLEAN;
	Argument.Boolean = Value;
}

void TScriptCall::addString(const char *Value) {
	AssertMainThread();
	assert(!Done);
	TArgument &Argument = AppendArgument(Arguments, NumberOfArguments, Capacity);
	if (Value != nullptr) {
		Argument.Type = LUA_TSTRING;
		Argument.String = Value;
	}
}

void TScriptCall::addPlayer(uint32_t CreatureID) {
	AssertMainThread();
	assert(!Done);
	TArgument &Argument = AppendArgument(Arguments, NumberOfArguments, Capacity);
	Argument.Type = LUA_TUSERDATA;
	Argument.Kind = SCRIPT_PLAYER;
	Argument.Object = CreatureID;
}

void TScriptCall::addCreature(uint32_t CreatureID) {
	AssertMainThread();
	assert(!Done);
	TArgument &Argument = AppendArgument(Arguments, NumberOfArguments, Capacity);
	if (CreatureID != 0) {
		Argument.Type = LUA_TUSERDATA;
		Argument.Kind = CreatureID < static_cast<uint32_t>(FIRST_NONPLAYER_CREATURE_ID) ? SCRIPT_PLAYER
				: SCRIPT_CREATURE;
		Argument.Object = CreatureID;
	}
}

void TScriptCall::addNpc(uint32_t CreatureID) {
	AssertMainThread();
	assert(!Done);
	TArgument &Argument = AppendArgument(Arguments, NumberOfArguments, Capacity);
	Argument.Type = LUA_TUSERDATA;
	Argument.Kind = SCRIPT_NPC;
	Argument.Object = CreatureID;
}

void TScriptCall::addItem(uint32_t ObjectHandle) {
	AssertMainThread();
	assert(!Done);
	TArgument &Argument = AppendArgument(Arguments, NumberOfArguments, Capacity);
	if (ObjectHandle != 0) {
		Argument.Type = LUA_TUSERDATA;
		Argument.Kind = SCRIPT_ITEM;
		Argument.Object = ObjectHandle;
	}
}

void TScriptCall::addPosition(int x, int y, int z) {
	AssertMainThread();
	assert(!Done);
	TArgument &Argument = AppendArgument(Arguments, NumberOfArguments, Capacity);
	Argument.Type = POSITION_ARGUMENT;
	Argument.Position[0] = x;
	Argument.Position[1] = y;
	Argument.Position[2] = z;
}

void TScriptCall::addNil() {
	AssertMainThread();
	assert(!Done);
	AppendArgument(Arguments, NumberOfArguments, Capacity);
}

void TScriptCall::addStringArray(const char *const *Values, int Count) {
	AssertMainThread();
	assert(!Done);
	assert(Count >= 0);
	TArgument &Argument = AppendArgument(Arguments, NumberOfArguments, Capacity);
	Argument.Type = LUA_TTABLE;
	Argument.Strings.clear();
	for (int i = 0; i < Count; i++) {
		// An array has no holes: a null string (not allowed) gives "".
		assert(Values[i] != nullptr);
		Argument.Strings.push_back(Values[i] != nullptr ? Values[i] : "");
	}
}

void TScriptCall::run() {
	assert(ScriptingActive());
	AssertMainThread();
	assert(!Done);
	if (Done) {
		ScriptFailure(Script, Handler, "the call has run already");
	}
	Done = true;
	FailuresBefore = FailureCount;
	TCallResult Result;
	CallHandler(Script, Handler, Arguments, NumberOfArguments, &Result);
}

int TScriptCall::runNumber() {
	assert(ScriptingActive());
	AssertMainThread();
	assert(!Done);
	if (Done) {
		ScriptFailure(Script, Handler, "the call has run already");
	}
	Done = true;
	FailuresBefore = FailureCount;
	TCallResult Result;
	CallHandler(Script, Handler, Arguments, NumberOfArguments, &Result);
	if (Result.Type == LUA_TNIL) {
		return 0;
	}
	if (Result.Type != LUA_TNUMBER || !(Result.Number >= INT_MIN && Result.Number <= INT_MAX)
			|| Result.Number != floor(Result.Number)) {
		ScriptFailure(Script, Handler, std::string(Handler) + " returned " + ResultValueText(Result) + ", not an integer");
	}
	return static_cast<int>(Result.Number);
}

bool TScriptCall::runBoolean() {
	assert(ScriptingActive());
	AssertMainThread();
	assert(!Done);
	if (Done) {
		ScriptFailure(Script, Handler, "the call has run already");
	}
	Done = true;
	FailuresBefore = FailureCount;
	TCallResult Result;
	CallHandler(Script, Handler, Arguments, NumberOfArguments, &Result);
	if (Result.Type == LUA_TNIL) {
		return false;
	}
	if (Result.Type != LUA_TBOOLEAN) {
		ScriptFailure(Script, Handler, std::string(Handler) + " returned " + ResultValueText(Result) + ", not a boolean");
	}
	return Result.Boolean;
}

// Pushes an object handle as an Item, nil for 0 (NONE).
static void PushItemOrNil(lua_State *L, uint32_t ObjectHandle) {
	if (ObjectHandle == 0) {
		lua_pushnil(L);
	} else {
		PushScriptObject(L, SCRIPT_ITEM, ObjectHandle);
	}
}

TScriptEvent::TScriptEvent(const char *Event, uint32_t User, uint32_t Obj1, uint32_t Obj2) :
		Event(Event), Base(0) {
	assert(ScriptingActive());
	AssertMainThread();
	if (!ScriptingActive()) {
		error("TScriptEvent: the scripting engine is not active.\n");
		throw ERROR;
	}
	lua_State *L = State->lua_state();
	Base = lua_gettop(L);
	if (!lua_checkstack(L, 3)) {
		error("TScriptEvent: no room on Lua's stack.\n");
		throw ERROR;
	}
	PushItemOrNil(L, User);
	PushItemOrNil(L, Obj1);
	PushItemOrNil(L, Obj2);
}

TScriptEvent::~TScriptEvent() {
	AssertMainThread();
	if (ScriptingActive()) {
		lua_settop(State->lua_state(), Base);
	}
}

bool TScriptEvent::check(int Script, const char *Handler, int Rule) {
	assert(ScriptingActive());
	AssertMainThread();
	int NumberOfArguments = Rule > 0 ? 5 : 4;
	int Top = PrepareCall(Script, Handler, NumberOfArguments);
	lua_State *L = State->lua_state();
	if (Rule > 0) {
		lua_pushinteger(L, Rule);
	}
	lua_pushstring(L, Event);
	lua_pushvalue(L, Base + 1);
	lua_pushvalue(L, Base + 2);
	lua_pushvalue(L, Base + 3);
	TCallResult Result;
	FinishCall(L, Top, Script, Handler, NumberOfArguments, &Result);
	if (Result.Type == LUA_TNIL) {
		return false;
	}
	if (Result.Type != LUA_TBOOLEAN) {
		ScriptFailure(Script, Handler, std::string(Handler) + " returned " + ResultValueText(Result) + ", not a boolean");
	}
	return Result.Boolean;
}

void TScriptEvent::run(int Script, const char *Handler) {
	assert(ScriptingActive());
	AssertMainThread();
	int Top = PrepareCall(Script, Handler, 4);
	lua_State *L = State->lua_state();
	lua_pushstring(L, Event);
	lua_pushvalue(L, Base + 1);
	lua_pushvalue(L, Base + 2);
	lua_pushvalue(L, Base + 3);
	TCallResult Result;
	FinishCall(L, Top, Script, Handler, 4, &Result);
}
