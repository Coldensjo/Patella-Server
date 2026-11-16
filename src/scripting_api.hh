// scripting_api.hh: what the units that bind C++ functions for the scripts
// use (scripting.hh has the engine as the game code sees it). Includes sol2.
//
// Names follow TFS (The Forgotten Server) in its style: tables and classes
// PascalCase (Game, Magic, Creature, Player), functions and methods
// camelCase (Magic.massCombat, player:getLevel), constants UPPER_CASE
// globals (CONST_ME_FIREAREA, RETURNVALUE_NOTENOUGHROOM). A TFS name is used
// only where it means the same; the values are 7.72's; what 7.72 lacks has no
// name; what TFS lacks gets an own name in that style. The reference is the
// mapping note (Patella-Gameserver, notes/lua/tfs-name-mapping.md).
//
// A binding is registered in an API table (Game, Magic, ...) or a class's
// method table (Creature, Player, ...), wrapped with Guarded so that a RESULT
// the C++ function throws becomes a Lua error that carries the code (rule 5
// of scripting.hh), never a C++ exception crossing Lua frames:
//
//	// Heal(cr, Mana, SoulPoints, Amount): heals the creature; throws what Heal throws.
//	API.set_function("heal", Guarded(&HealBinding));
//
// Every binding has a comment that says what it does, its arguments, what it
// returns and what it throws. A binding resolves creature IDs and object
// handles again on every call (rule 2): GetCreature(ID), Object(ID).exists().
// It never keeps a sol object beyond the call.
//
// A binding's parameters are numbers (integral or floating types, bool),
// strings (const char *, std::string), the objects below (TScriptCreature,
// TScriptPlayer, TScriptNpc, TScriptMonster, TScriptItem, TScriptThing,
// TScriptVocation, TScriptItemType, TScriptMonsterType), a Position
// (TScriptPosition), an Impact (TScriptImpact), an outfit's colors
// (TScriptColors) or sol::optional of those ("|nil"); Guarded refuses any
// other parameter type at compile time. A script can so pass only numbers,
// booleans, strings, nil, the objects, Positions, Impacts and the colors'
// arrays; any other value where an object is expected (a table, a shared
// table's proxy, a result error) is a Lua argument error. No other table
// reaches a binding: the shared tables are userdata proxies, and sol would
// accept one where it expects a table; TScriptColors takes only a real table
// and reads it raw.
//
// Include it after the game headers that declare RESULT (info.hh, which needs
// objects.hh and map.hh before it), as the game's units include info.hh.

#ifndef PATELLA_SCRIPTING_API_HH
#define PATELLA_SCRIPTING_API_HH

#include <cstdint>
#include <exception>
#include <string>
#include <type_traits>

#include <sol/sol.hpp>

#include "scripting.hh"

// The state InitScripting made. Main thread only; asserts ScriptingActive in
// debug builds.
sol::state &ScriptState();

// An API table of the shared globals: the table behind Name's read-only proxy,
// made empty and strict (reading a field it lacks is a Lua error "unknown name
// '<Name>.<field>'"; writing one from a script is the error "<Name>.<field> is
// read-only") when it is not there yet. The classes' method tables (Creature,
// Player, Npc, Item, Vocation) are API tables too, made by InitScripting.
// Registration functions add their bindings to the returned table. Throws a
// const char * when Name is a shared global that is not an API table (a library
// such as "string", a function, a constant), or a name the sandbox removed.
sol::table ScriptAPITable(const char *Name);

// Sets the shared global Name to the constant Value: scripts read it, and
// writing it, also as a global of a script's own, is the Lua error "<Name> is
// read-only". Throws a const char * when the shared global Name exists
// already or is a name the sandbox removed.
void SetScriptConstant(const char *Name, int Value);

// Raises the result code Code as a Lua error value that carries it
// (Game.getResultOf gives it back, TScriptCall throws it as RESULT). Must be
// called directly by a function Lua called (through sol), outside any C++
// catch block: it does not return (lua_error). LuaJIT on x64 unwinds the C++
// frames as an exception would, so the destructors of objects alive there
// run.
[[noreturn]] void RaiseResult(lua_State *L, int Code);

// Raises a Lua error with the text "<where>: Text", where is the position
// of the Lua code that called the function Lua called (luaL_where level 1),
// as luaL_error does: TScriptCall logs it as a non-result error and throws
// ERROR. The same conditions as RaiseResult; Text is copied first.
[[noreturn]] void RaiseError(lua_State *L, const char *Text);

// The objects (scripting.hh, "Objects"): a userdata holding a creature ID
// (a Creature, Player, Npc or Monster), an object handle, for a Vocation its
// player's creature ID, for an ItemType an object type's number (an int, kept
// as its uint32_t bits), or for a MonsterType a race number.
enum TScriptObjectKind {
	SCRIPT_NO_OBJECT,
	SCRIPT_CREATURE,
	SCRIPT_PLAYER,
	SCRIPT_ITEM,
	SCRIPT_VOCATION,
	SCRIPT_ITEMTYPE,
	SCRIPT_NPC,
	SCRIPT_MONSTER,
	SCRIPT_MONSTERTYPE,
};

// The last kind (the kinds are numbered from SCRIPT_NO_OBJECT, 0, on).
const TScriptObjectKind LAST_SCRIPT_OBJECT_KIND = SCRIPT_MONSTERTYPE;

// The kind of the value at Index: SCRIPT_NO_OBJECT for anything that is not
// one of the objects. Calls no metamethod.
TScriptObjectKind ScriptObjectKind(lua_State *L, int Index);

// The ID or handle the object at Index holds (0 for a value that is not an
// object).
uint32_t ScriptObjectValue(lua_State *L, int Index);

// Pushes a new object of the kind Kind (not SCRIPT_NO_OBJECT) holding Value.
void PushScriptObject(lua_State *L, TScriptObjectKind Kind, uint32_t Value);

// Pushes the creature ID as an object: nil for 0, a Player for a player's ID
// (below FIRST_NONPLAYER_CREATURE_ID, as IsCreaturePlayer says), a Creature
// otherwise.
void PushScriptCreature(lua_State *L, uint32_t ID);

// Sets the function ItemType(id) makes an ItemType's type number with: the
// script's id goes in, the number the object holds comes out. It runs inside
// a function Lua called, so it must not throw (error() and print() are
// fine). nullptr (the default, and after ExitScripting) keeps id as it is.
// RegisterItemAPI sets ObjectType(id).getTypeID().
void SetItemTypeConstructor(int (*Constructor)(int));

// A Position (scripting.hh, "Position"): a userdata holding three ints, not
// one of the objects above (ScriptObjectKind gives SCRIPT_NO_OBJECT for it).
// Whether the value at Index is a Position (calls no metamethod); its
// coordinates; pushing a new one.
bool IsScriptPosition(lua_State *L, int Index);
void GetScriptPosition(lua_State *L, int Index, int *x, int *y, int *z);
void PushScriptPosition(lua_State *L, int x, int y, int z);

// An Impact (scripting.hh, "Monster", Impact): a userdata holding the kind of
// an impact of magic.hh, its actor's creature ID and its numbers, not one of
// the objects above (ScriptObjectKind gives SCRIPT_NO_OBJECT for it). Its
// methods are those of the API table Impact (impact:isAggressive()), which
// also holds the constructors (Impact.damage, ...); it cannot be changed, two
// are equal (==) when kind, actor, numbers and colors are, and its tostring
// is "Impact <kind>" ("Impact damage"). The numbers by kind (Values, the rest
// 0): damage {damage type, power, allow defense (0 or 1)}, field {field
// type}, healing {power}, speed {percent, duration}, drunken {power,
// duration}, strength {skills, percent, duration}, outfit {outfit ID,
// duration} and the four color bytes (Colors, all 0 for the others), summon
// {race, maximum}.
enum TScriptImpactKind {
	SCRIPT_IMPACT_DAMAGE,
	SCRIPT_IMPACT_FIELD,
	SCRIPT_IMPACT_HEALING,
	SCRIPT_IMPACT_SPEED,
	SCRIPT_IMPACT_DRUNKEN,
	SCRIPT_IMPACT_STRENGTH,
	SCRIPT_IMPACT_OUTFIT,
	SCRIPT_IMPACT_SUMMON,
};

const TScriptImpactKind LAST_SCRIPT_IMPACT_KIND = SCRIPT_IMPACT_SUMMON;

struct TScriptImpact {
	TScriptImpactKind Kind;
	uint32_t Actor;
	int Values[4];
	unsigned char Colors[4];
};

// Whether the value at Index is an Impact (calls no metamethod); the Impact
// it holds (kind damage, all numbers 0, for another value); pushing a new
// one.
bool IsScriptImpact(lua_State *L, int Index);
TScriptImpact GetScriptImpact(lua_State *L, int Index);
void PushScriptImpact(lua_State *L, const TScriptImpact &Impact);

// The parameter and result types of the objects. TScriptCreature takes a
// Creature, a Player, an Npc or a Monster (all are Creatures); TScriptPlayer
// only a Player; TScriptNpc only an Npc; TScriptMonster only a Monster;
// TScriptItem only an Item; TScriptThing an Item or any of the creatures (for
// the C++ functions that take an Object); TScriptVocation a Vocation;
// TScriptItemType an ItemType; TScriptMonsterType a MonsterType. A
// TScriptCreature result is nil for 0, a Player for a player's ID and a
// Creature otherwise (PushScriptCreature), also for an NPC's or a monster's
// ID.
struct TScriptCreature {
	uint32_t ID;
};

struct TScriptPlayer {
	uint32_t ID;
};

struct TScriptNpc {
	uint32_t ID;
};

struct TScriptMonster {
	uint32_t ID;
};

struct TScriptItem {
	uint32_t Handle;
};

struct TScriptThing {
	bool IsCreature;
	uint32_t Value;  // the creature ID, or the object handle
};

struct TScriptVocation {
	uint32_t PlayerID;
};

struct TScriptItemType {
	int TypeID;
};

struct TScriptMonsterType {
	int Race;
};

// The colors of an outfit as a parameter: a Lua array of four integers (a
// real table, read raw: elements 1 to 4, each a number without fraction in
// the range of int, and no element 5); any other value is the argument error
// "array of four colors expected". Values[0] is element 1.
struct TScriptColors {
	int Values[4];
};

// A Position as a parameter or result.
struct TScriptPosition {
	int x;
	int y;
	int z;
};

// How each object type is checked, read and pushed (sol's customization
// points below use it).
template<typename T>
struct TScriptObjectType;

template<>
struct TScriptObjectType<TScriptCreature> {
	static bool accepts(TScriptObjectKind Kind) {
		return Kind == SCRIPT_CREATURE || Kind == SCRIPT_PLAYER || Kind == SCRIPT_NPC || Kind == SCRIPT_MONSTER;
	}
	static const char *expected() { return "Creature expected"; }
	static TScriptCreature make(TScriptObjectKind, uint32_t Value) { return TScriptCreature{Value}; }
	static void push(lua_State *L, const TScriptCreature &Value) { PushScriptCreature(L, Value.ID); }
};

template<>
struct TScriptObjectType<TScriptPlayer> {
	static bool accepts(TScriptObjectKind Kind) { return Kind == SCRIPT_PLAYER; }
	static const char *expected() { return "Player expected"; }
	static TScriptPlayer make(TScriptObjectKind, uint32_t Value) { return TScriptPlayer{Value}; }
	static void push(lua_State *L, const TScriptPlayer &Value) { PushScriptObject(L, SCRIPT_PLAYER, Value.ID); }
};

template<>
struct TScriptObjectType<TScriptNpc> {
	static bool accepts(TScriptObjectKind Kind) { return Kind == SCRIPT_NPC; }
	static const char *expected() { return "Npc expected"; }
	static TScriptNpc make(TScriptObjectKind, uint32_t Value) { return TScriptNpc{Value}; }
	static void push(lua_State *L, const TScriptNpc &Value) { PushScriptObject(L, SCRIPT_NPC, Value.ID); }
};

template<>
struct TScriptObjectType<TScriptMonster> {
	static bool accepts(TScriptObjectKind Kind) { return Kind == SCRIPT_MONSTER; }
	static const char *expected() { return "Monster expected"; }
	static TScriptMonster make(TScriptObjectKind, uint32_t Value) { return TScriptMonster{Value}; }
	static void push(lua_State *L, const TScriptMonster &Value) { PushScriptObject(L, SCRIPT_MONSTER, Value.ID); }
};

template<>
struct TScriptObjectType<TScriptItem> {
	static bool accepts(TScriptObjectKind Kind) { return Kind == SCRIPT_ITEM; }
	static const char *expected() { return "Item expected"; }
	static TScriptItem make(TScriptObjectKind, uint32_t Value) { return TScriptItem{Value}; }
	static void push(lua_State *L, const TScriptItem &Value) { PushScriptObject(L, SCRIPT_ITEM, Value.Handle); }
};

template<>
struct TScriptObjectType<TScriptThing> {
	static bool accepts(TScriptObjectKind Kind) {
		return Kind == SCRIPT_ITEM || Kind == SCRIPT_CREATURE || Kind == SCRIPT_PLAYER || Kind == SCRIPT_NPC
				|| Kind == SCRIPT_MONSTER;
	}
	static const char *expected() { return "Item or Creature expected"; }
	static TScriptThing make(TScriptObjectKind Kind, uint32_t Value) {
		return TScriptThing{Kind != SCRIPT_ITEM, Value};
	}
	static void push(lua_State *L, const TScriptThing &Value) {
		if (Value.IsCreature) {
			PushScriptCreature(L, Value.Value);
		} else {
			PushScriptObject(L, SCRIPT_ITEM, Value.Value);
		}
	}
};

template<>
struct TScriptObjectType<TScriptVocation> {
	static bool accepts(TScriptObjectKind Kind) { return Kind == SCRIPT_VOCATION; }
	static const char *expected() { return "Vocation expected"; }
	static TScriptVocation make(TScriptObjectKind, uint32_t Value) { return TScriptVocation{Value}; }
	static void push(lua_State *L, const TScriptVocation &Value) {
		PushScriptObject(L, SCRIPT_VOCATION, Value.PlayerID);
	}
};

template<>
struct TScriptObjectType<TScriptItemType> {
	static bool accepts(TScriptObjectKind Kind) { return Kind == SCRIPT_ITEMTYPE; }
	static const char *expected() { return "ItemType expected"; }
	static TScriptItemType make(TScriptObjectKind, uint32_t Value) {
		return TScriptItemType{static_cast<int>(Value)};
	}
	static void push(lua_State *L, const TScriptItemType &Value) {
		PushScriptObject(L, SCRIPT_ITEMTYPE, static_cast<uint32_t>(Value.TypeID));
	}
};

template<>
struct TScriptObjectType<TScriptMonsterType> {
	static bool accepts(TScriptObjectKind Kind) { return Kind == SCRIPT_MONSTERTYPE; }
	static const char *expected() { return "MonsterType expected"; }
	static TScriptMonsterType make(TScriptObjectKind, uint32_t Value) {
		return TScriptMonsterType{static_cast<int>(Value)};
	}
	static void push(lua_State *L, const TScriptMonsterType &Value) {
		PushScriptObject(L, SCRIPT_MONSTERTYPE, static_cast<uint32_t>(Value.Race));
	}
};

// sol's check of an object parameter: the kind must be one T accepts;
// otherwise the handler raises sol's argument error.
template<typename T, typename Handler>
bool CheckScriptObject(lua_State *L, int Index, Handler &&handler, sol::stack::record &tracking) {
	tracking.use(1);
	if (TScriptObjectType<T>::accepts(ScriptObjectKind(L, Index))) {
		return true;
	}
	handler(L, Index, sol::type::userdata, sol::type_of(L, Index), TScriptObjectType<T>::expected());
	return false;
}

template<typename T>
T GetScriptObject(lua_State *L, int Index, sol::stack::record &tracking) {
	tracking.use(1);
	return TScriptObjectType<T>::make(ScriptObjectKind(L, Index), ScriptObjectValue(L, Index));
}

// sol's check of an object parameter that may be nil (sol::optional): nil or
// no value passes (counted as sol counts them), an object as above; any other
// value is an argument error. (sol's own check of an optional takes any value
// and gives nullopt for one of the wrong type.)
template<typename T, typename Handler>
bool CheckOptionalScriptObject(lua_State *L, int Index, Handler &&handler, sol::stack::record &tracking) {
	if (lua_isnoneornil(L, Index)) {
		tracking.use(lua_isnone(L, Index) ? 0 : 1);
		return true;
	}
	return CheckScriptObject<T>(L, Index, std::forward<Handler>(handler), tracking);
}

template<typename T>
sol::optional<T> GetOptionalScriptObject(lua_State *L, int Index, sol::stack::record &tracking) {
	if (lua_isnoneornil(L, Index)) {
		tracking.use(lua_isnone(L, Index) ? 0 : 1);
		return sol::nullopt;
	}
	return GetScriptObject<T>(L, Index, tracking);
}

// sol's customization points (found by argument-dependent lookup), for T and
// sol::optional<T>.
#define SCRIPT_OBJECT_TYPE(T) \
	template<typename Handler> \
	bool sol_lua_check(sol::types<T>, lua_State *L, int Index, Handler &&handler, sol::stack::record &tracking) { \
		return CheckScriptObject<T>(L, Index, std::forward<Handler>(handler), tracking); \
	} \
	inline T sol_lua_get(sol::types<T>, lua_State *L, int Index, sol::stack::record &tracking) { \
		return GetScriptObject<T>(L, Index, tracking); \
	} \
	inline int sol_lua_push(sol::types<T>, lua_State *L, const T &Value) { \
		TScriptObjectType<T>::push(L, Value); \
		return 1; \
	} \
	template<typename Handler> \
	bool sol_lua_check(sol::types<sol::optional<T>>, lua_State *L, int Index, Handler &&handler, \
			sol::stack::record &tracking) { \
		return CheckOptionalScriptObject<T>(L, Index, std::forward<Handler>(handler), tracking); \
	} \
	inline sol::optional<T> sol_lua_get(sol::types<sol::optional<T>>, lua_State *L, int Index, \
			sol::stack::record &tracking) { \
		return GetOptionalScriptObject<T>(L, Index, tracking); \
	}

SCRIPT_OBJECT_TYPE(TScriptCreature)
SCRIPT_OBJECT_TYPE(TScriptPlayer)
SCRIPT_OBJECT_TYPE(TScriptNpc)
SCRIPT_OBJECT_TYPE(TScriptMonster)
SCRIPT_OBJECT_TYPE(TScriptItem)
SCRIPT_OBJECT_TYPE(TScriptThing)
SCRIPT_OBJECT_TYPE(TScriptVocation)
SCRIPT_OBJECT_TYPE(TScriptItemType)
SCRIPT_OBJECT_TYPE(TScriptMonsterType)

#undef SCRIPT_OBJECT_TYPE

// sol's customization points of a Position parameter (only a Position; any
// other value is the argument error "Position expected") and result.
template<typename Handler>
bool sol_lua_check(sol::types<TScriptPosition>, lua_State *L, int Index, Handler &&handler,
		sol::stack::record &tracking) {
	tracking.use(1);
	if (IsScriptPosition(L, Index)) {
		return true;
	}
	handler(L, Index, sol::type::userdata, sol::type_of(L, Index), "Position expected");
	return false;
}

inline TScriptPosition sol_lua_get(sol::types<TScriptPosition>, lua_State *L, int Index,
		sol::stack::record &tracking) {
	tracking.use(1);
	TScriptPosition Position;
	GetScriptPosition(L, Index, &Position.x, &Position.y, &Position.z);
	return Position;
}

inline int sol_lua_push(sol::types<TScriptPosition>, lua_State *L, const TScriptPosition &Value) {
	PushScriptPosition(L, Value.x, Value.y, Value.z);
	return 1;
}

// sol's customization points of an Impact parameter (only an Impact; any
// other value is the argument error "Impact expected") and result.
template<typename Handler>
bool sol_lua_check(sol::types<TScriptImpact>, lua_State *L, int Index, Handler &&handler,
		sol::stack::record &tracking) {
	tracking.use(1);
	if (IsScriptImpact(L, Index)) {
		return true;
	}
	handler(L, Index, sol::type::userdata, sol::type_of(L, Index), "Impact expected");
	return false;
}

inline TScriptImpact sol_lua_get(sol::types<TScriptImpact>, lua_State *L, int Index,
		sol::stack::record &tracking) {
	tracking.use(1);
	return GetScriptImpact(L, Index);
}

inline int sol_lua_push(sol::types<TScriptImpact>, lua_State *L, const TScriptImpact &Value) {
	PushScriptImpact(L, Value);
	return 1;
}

// Whether the value at Index is an array of four colors (TScriptColors),
// read raw (no metamethod runs); sets *Colors when it is and Colors is not
// nullptr.
bool ReadScriptColors(lua_State *L, int Index, TScriptColors *Colors);

// sol's customization points of a colors parameter (TScriptColors).
template<typename Handler>
bool sol_lua_check(sol::types<TScriptColors>, lua_State *L, int Index, Handler &&handler,
		sol::stack::record &tracking) {
	tracking.use(1);
	if (ReadScriptColors(L, Index, nullptr)) {
		return true;
	}
	handler(L, Index, sol::type::table, sol::type_of(L, Index), "array of four colors expected");
	return false;
}

inline TScriptColors sol_lua_get(sol::types<TScriptColors>, lua_State *L, int Index,
		sol::stack::record &tracking) {
	tracking.use(1);
	TScriptColors Colors = {{0, 0, 0, 0}};
	ReadScriptColors(L, Index, &Colors);
	return Colors;
}

// Guarded(f): a function for sol's set_function that calls f with the same
// arguments and gives its result. A RESULT f throws is caught and raised with
// RaiseResult after the catch block has ended. A const char * or a
// std::exception f throws is caught and raised with RaiseError (its text, or
// what()), so it is logged with its place and the traceback and the C++
// caller of the handler throws ERROR; in the C++ code such an exception went
// on to the caller's caller, so a binding whose function may throw one says
// so in its comment. No other exception may leave f.
// Whether a binding may take a parameter of type T (see the top).
template<typename T>
struct TScriptArgument : std::integral_constant<bool, std::is_arithmetic<T>::value
		|| std::is_same<T, const char *>::value || std::is_same<T, std::string>::value
		|| std::is_same<T, TScriptCreature>::value || std::is_same<T, TScriptPlayer>::value
		|| std::is_same<T, TScriptNpc>::value || std::is_same<T, TScriptMonster>::value
		|| std::is_same<T, TScriptItem>::value || std::is_same<T, TScriptThing>::value
		|| std::is_same<T, TScriptVocation>::value || std::is_same<T, TScriptItemType>::value
		|| std::is_same<T, TScriptMonsterType>::value || std::is_same<T, TScriptPosition>::value
		|| std::is_same<T, TScriptImpact>::value || std::is_same<T, TScriptColors>::value> {};
template<typename T>
struct TScriptArgument<sol::optional<T>> : TScriptArgument<std::decay_t<T>> {};

template<typename R, typename... A>
auto Guarded(R (*Function)(A...)) {
	static_assert((TScriptArgument<std::decay_t<A>>::value && ...),
			"a binding takes only numbers, bool, strings, the script objects, Positions, Impacts, colors and"
				" sol::optional of those");
	return [Function](sol::this_state State, A... Arguments) -> R {
		int Code = 0;
		bool Text = false;
		std::string Message;
		try {
			return Function(Arguments...);
		} catch (RESULT r) {
			Code = static_cast<int>(r);
		} catch (const char *s) {
			Text = true;
			Message = (s != nullptr ? s : "(null)");
		} catch (const std::exception &e) {
			Text = true;
			Message = e.what();
		}
		if (Text) {
			RaiseError(State, Message.c_str());
		}
		RaiseResult(State, Code);
	};
}

// The registration of each area's API, called by InitAll right after
// InitScripting, in this order (scripting.hh lists the functions).
void RegisterMagicAPI();
void RegisterTriggerAPI();
void RegisterItemAPI();
void RegisterNpcAPI();
void RegisterMonsterAPI();

#endif
