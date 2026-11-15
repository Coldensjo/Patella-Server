// scripting_api.hh: what the units that bind C++ functions for the scripts
// use (scripting.hh has the engine as the game code sees it). Includes sol2.
//
// A binding is registered in an API table (game, creature, object, map,
// player, ...) under a snake_case name, wrapped with Guarded so that a RESULT
// the C++ function throws becomes a Lua error that carries the code (rule 5
// of scripting.hh), never a C++ exception crossing Lua frames:
//
//	// Heal(CreatureID, Amount): heals the creature; throws what Heal throws.
//	API.set_function("heal", Guarded(&HealBinding));
//
// Every binding has a comment that says what it does, its arguments, what it
// returns and what it throws. A binding resolves creature IDs and object
// handles again on every call (rule 2): GetCreature(ID), Object(ID).exists().
// It never keeps a sol object beyond the call.
//
// A binding's parameters are numbers (integral or floating types, bool),
// strings (const char *, std::string) or sol::optional of those, so a script
// can pass only numbers, booleans, strings and nil; Guarded refuses any other
// parameter type at compile time. Tables and userdata never reach a binding:
// the shared tables are userdata proxies, and sol would accept one where it
// expects a table.
//
// Include it after the game headers that declare RESULT (info.hh, which needs
// objects.hh and map.hh before it), as the game's units include info.hh.

#ifndef PATELLA_SCRIPTING_API_HH
#define PATELLA_SCRIPTING_API_HH

#include <exception>
#include <string>
#include <type_traits>

#include <sol/sol.hpp>

#include "scripting.hh"

// The state InitScripting made. Main thread only; asserts ScriptingActive in
// debug builds.
sol::state &ScriptState();

// An API table of the shared globals: the table behind Name's read-only
// proxy, made empty and strict (reading a field it lacks is a Lua error
// "unknown name '<Name>.<field>'"; writing one from a script is the error
// "<Name>.<field> is read-only") when it is not there yet. Registration
// functions add their bindings to the returned table. Throws a const char *
// when Name is a shared global that is not an API table (a library such as
// "string", or a function).
sol::table ScriptAPITable(const char *Name);

// Raises the result code Code as a Lua error value that carries it
// (game.result_of gives it back, TScriptCall throws it as RESULT). Must be
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
		|| std::is_same<T, const char *>::value || std::is_same<T, std::string>::value> {};
template<typename T>
struct TScriptArgument<sol::optional<T>> : TScriptArgument<std::decay_t<T>> {};

template<typename R, typename... A>
auto Guarded(R (*Function)(A...)) {
	static_assert((TScriptArgument<std::decay_t<A>>::value && ...),
			"a binding takes only numbers, bool, strings and sol::optional of those");
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

#endif
