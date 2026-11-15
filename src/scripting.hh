// scripting.hh: the Lua engine (LuaJIT 2.1, extern/luajit; bound with sol2,
// extern/sol2) as the game code sees it: one Lua state, created at start,
// the scripts of data/scripts/ loaded into it and cached, and the calls of
// their handlers. Nothing here needs sol2; the units that bind C++ functions
// for the scripts include scripting_api.hh.
//
// The rules every script and binding keeps (they make a script provable
// against the C++ code it replaces):
//  1. One state, main thread only: everything here is called from the main
//     loop (LaunchGame) or from InitAll/ExitAll, never from a signal handler,
//     the reader, writer, protocol, acceptor or connection threads. Debug
//     builds assert it.
//  2. Scripts hold creature IDs and object handles (Object::getObjectID),
//     never pointers; every API function resolves them again.
//  3. Randomness only through game.random and game.rand, which call the C++
//     random and rand; math.random and math.randomseed do not exist.
//  4. No behaviour may depend on pairs order (pairs, next, table.foreach);
//     arrays and ipairs only. Nothing may depend on tostring of a table, a
//     function or a userdata either (it shows an address).
//  5. Result codes (RESULT) cross the boundary unchanged: an API function
//     that throws one raises a Lua error that carries it, and the C++ caller
//     of a handler throws it again. Any other Lua error is logged with its
//     file:line and traceback and thrown as ERROR.
//  6. A script makes the same calls, in the same order, with the same
//     arguments as the C++ code it replaces.
//  7. The sandbox: base (without dofile, loadfile, load, loadstring, print,
//     _G, getfenv, setfenv, rawset, newproxy, collectgarbage, gcinfo), table,
//     string (without dump), math (without random, randomseed), bit;
//     coroutine comes with base. No io, os, debug, ffi, jit, package. require
//     loads only from the scripts folder. The shared tables are read-only to
//     scripts: the API tables (game, result, ...) and the libraries (string,
//     table, math, bit, coroutine) are proxies (userdata, so type(game) is
//     "userdata" and the raw writes of table.insert cannot reach them) whose
//     writes are Lua errors ("<table>.<field> is read-only"), and the
//     strings' metatable cannot be read or changed (getmetatable("") gives
//     false). Scripts use no __gc and no __mode (weak tables): nothing may
//     depend on when the garbage collector runs.
//  8. Errors in scripts are found when they are loaded, at start.
//  9. Scripts keep no state that must survive; their globals may cache. What
//     a module returns (require) is shared by every script that requires it:
//     no script changes it or its tables after it is loaded, so no script's
//     behaviour depends on whether another one ran.
//
// Scripts. A script is a file below the scripts folder (InitScripting's
// Folder, "<DATAPATH>/scripts" in the game), named by its path relative to
// that folder with '/' and the ending ".lua" ("spells/light.lua"); the name may
// contain only a-z, 0-9, '_', '-', '/' and '.', and no part may be "." or "..".
// Its chunk name, which Lua puts in front of messages and tracebacks, is that
// name, so an error reads "spells/light.lua:12: <what>". A script runs once,
// when it is first loaded, in an environment of its own whose reads fall back
// to the shared globals (the API tables and the sandbox's libraries); its own
// globals stay in that environment. It must return a table; the table's
// functions named on_<what> are its handlers. A global name that is neither
// in the script's environment nor shared is an error when it is read
// ("unknown name 'x'"), and so is an unknown field of an API table: a script
// that uses one at its top level fails to load, at start.
//
// require(name) loads "lib/x.lua" for require("lib.x") (the dots become
// slashes, ".lua" is added), with the same rules, once: later calls give the
// value the first one returned (any value but nil; nil counts as true, as in
// Lua). A module that requires itself, directly or through others, is an
// error.
//
// Lua's view of the engine (the core API, bound by scripting.cc):
//   game.random(low, high)  random(low, high) of utils.cc: low + rand() %
//                           (high - low + 1), low for an empty span.
//   game.rand()             rand(), as the C++ code calls it directly.
//   game.idiv(a, b)         a / b of C++ ints (truncated toward zero); b == 0
//                           is a Lua error.
//   game.imod(a, b)         a % b of C++ ints (the sign of a); b == 0 is a
//                           Lua error.
//   game.print(level, text) print(Level, "%s", text) of utils.cc.
//   game.error(text)        error("%s", text) of utils.cc, with each '%' of
//                           text doubled (see below).
//   game.throw(code)        raises the result code (a number of RESULT, or
//                           its name as in result below) as a C++ throw of it
//                           would.
//   game.result_of(err)     the result code an error value carries (from
//                           pcall), as a number, or nil for another error.
//                           A result error's tostring is "result <NAME>",
//                           and err.code is its number too.
//   result.<NAME>           the numbers of RESULT, by name (result.NOROOM ==
//                           4, result.ERROR == -1, ...); result[4] gives the
//                           name "NOROOM".
// The areas' APIs follow, each bound by a Register...API function in a unit
// of its own (scripting_api.hh). Their functions take creature IDs and object
// handles (rule 2). An actor or target ID that names no creature (0
// included) gives what the C++ function does when it is given nullptr; where
// the C++ function would dereference nullptr, the binding logs
// "<binding name>: Creature does not exist." and raises ERROR instead. A
// string a binding passes on as a char * is copied into a char[512] first,
// as CheckForSpell's SpellStr holds the words; nil gives nullptr where the
// signature says "|nil". Each function raises what the C++ function throws.
//
// Spells, runes and commands (phase 2; scripting_magic.cc, RegisterMagicAPI).
// An entry of spells.toml or commands.toml names its script with script =
// "<path>"; the script is named after the SPELL_ case of the C++ switch it
// replaces, lower case without SPELL_ (SPELL_LIGHT_HEALING ->
// "spells/light_healing.lua", SPELL_FIREBALL -> "runes/fireball.lua",
// SPELL_GET_ITEM_COUNT -> "commands/get_item_count.lua"). An entry whose
// case did not exist has no script and does what the switch did without a
// case. The handlers:
//   spells/<name>.lua    on_cast(caster, spell, mana, soul_points, words)
//       CastSpell's case, after the checks: caster is the creature's ID,
//       spell the entry's number, mana and soul_points its mana and soul
//       points (SpellList[SpellNr].Mana, .SoulPoints), words an array with
//       words[k] = SpellStr[k] for k = 1 to the last entry CheckForSpell set
//       (the empty one that ends them included).
//   runes/<name>.lua     on_use(user, spell, mana, soul_points, target,
//                        target_creature)
//       UseMagicItem's case, after the checks and the choice of the target:
//       user is the player's ID, target the object Dest, target_creature
//       the ID of dest, 0 where dest is nullptr.
//   commands/<name>.lua  on_say(caster, command, words)
//       CharacterRightSpell's or AccountRightSpell's case (only for a
//       player); words as for on_cast.
//
// magic: the spell functions of magic.cc (actor, target: creature IDs; obj:
// an object handle; mana, soul: what is spent, as the C++ argument).
//   compute_damage(actor, spell, damage, variation) -> int     ComputeDamage
//   heal(actor, mana, soul, amount)                            Heal
//   mass_heal(actor, obj, mana, soul, amount, radius)          MassHeal
//   heal_friend(actor, name, mana, soul, amount)               HealFriend
//   combat(actor, obj, mana, soul, damage, effect, animation, damage_type)
//                                                              Combat
//   mass_combat(actor, obj, mana, soul, damage, effect, radius, damage_type,
//       animation)                                             MassCombat
//   angle_combat(actor, mana, soul, damage, effect, radius, angle,
//       damage_type)                                           AngleCombat
//   kill_all_monsters(actor, effect, radius)                   KillAllMonsters
//   create_field(actor, mana, soul, field_type)                CreateField(cr, Mana, SoulPoints, Type)
//   create_field_on(actor, obj, mana, soul, field_type)        CreateField(cr, Obj, Mana, SoulPoints, Type)
//   mass_create_field(actor, obj, mana, soul, field_type, radius)
//                                                              MassCreateField
//   create_field_wall(actor, obj, mana, soul, field_type, extension)
//                                                              CreateFieldWall
//   delete_field(actor, obj, mana, soul)                       DeleteField
//   cleanup_field(actor)                                       CleanupField(cr)
//   cleanup_field_on(actor, obj, mana, soul)                   CleanupField(cr, Obj, Mana, SoulPoints)
//   magic_rope(actor, mana, soul)                              MagicRope
//   magic_climbing(actor, mana, soul, direction)               MagicClimbing(cr, Mana, SoulPoints, Direction)
//   magic_climbing_free(actor, direction)                      MagicClimbing(cr, Direction)
//   create_food(actor, mana, soul)                             CreateFood
//   create_arrows(actor, mana, soul, ammo, amount)             CreateArrows
//   summon_creature(actor, mana, race_name, god)               SummonCreature(cr, Mana, RaceName, God)
//   raise_dead(actor, obj, mana, soul)                         RaiseDead
//   mass_raise_dead(actor, obj, mana, soul, radius)            MassRaiseDead
//   magic_go_strength(actor, target, mana, soul, percent, duration)
//                                                              MagicGoStrength
//   shielding(actor, mana, soul, duration)                     Shielding
//   negate_poison(actor, mana, soul, target)                   NegatePoison
//   enlight(actor, mana, soul, radius, duration)               Enlight
//   invisibility(actor, mana, soul, duration)                  Invisibility
//   cancel_invisibility(actor, obj, mana, soul, radius)        CancelInvisibility
//   creature_illusion(actor, mana, soul, race_name, duration)  CreatureIllusion
//   object_illusion(actor, mana, soul, obj, duration)          ObjectIllusion
//   enchant_object(actor, mana, soul, origin_type, target_type)
//                                                              EnchantObject (object type IDs)
//   convince(actor, target)                                    Convince
//   challenge(actor, mana, soul, radius)                       Challenge
//   find_person(actor, mana, soul, name)                       FindPerson
// command: the commands' functions of magic.cc (actor: the caster's ID; the
// other arguments are the words, as the C++ case passed SpellStr).
//   create_thing(actor, p1, p2|nil)           CreateThing
//   teleport(actor, p)                        Teleport
//   create_knowledge(actor, p1, p2|nil)       CreateKnowledge
//   change_data(actor, p)                     ChangeData
//   teleport_to_creature(actor, name)         TeleportToCreature
//   teleport_player_to_me(actor, name)        TeleportPlayerToMe
//   get_position(actor)                       GetPosition
//   create_money(actor, p)                    CreateMoney
//   change_profession(actor, p)               ChangeProfession
//   edit_guests(actor)                        EditGuests
//   edit_subowners(actor)                     EditSubowners
//   kick_guest(actor, name)                   KickGuest
//   edit_name_door(actor)                     EditNameDoor
//   get_quest_value(actor, p)                 GetQuestValue
//   set_quest_value(actor, p1, p2)            SetQuestValue
//   clear_quest_values(actor)                 ClearQuestValues
//   start_monsterraid(actor, name)            StartMonsterraid
//   home_teleport(actor, name)                HomeTeleport
//   banish_account(actor, name, n, reason)    BanishAccount
//   delete_account(actor, name, reason)       DeleteAccount
//   set_name_rule(actor, name)                SetNameRule
//   notation(actor, name, comment)            Notation
//   name_lock(actor, name)                    NameLock
//   kick_player(actor, name)                  KickPlayer
//   delete_character(actor, name, reason)     DeleteCharacter
//   ip_banishment(actor, name, reason)        IPBanishment
//   banish_character(actor, name, n, reason)  BanishCharacter
// game.to_int(text) -> int: (int)std::clamp(strtol(text, nullptr, 10),
//   (long)INT32_MIN, (long)INT32_MAX), as AccountRightSpell read numbers.
// creature.object(id) -> the handle of the creature's object
//   (GetCreatureObject).
// creature.skill(id, skill) -> Skills[skill]->Get().
// creature.effective_profession(id) -> GetEffectiveProfession().
// object.new_type(group, number) -> GetNewObjectType(Group, Number), the
//   type's ID.
// Constants (read-only tables of numbers): effect.<X> (EFFECT_X of
// operate.hh, e.g. effect.DEATH), animation.<X> (ANIMATION_X), damage.<X>
// (DAMAGE_X of cr.hh), field.<X> (FIELD_X of info.hh), ammo.<X> (the
// CONJURE_X of magic.cc: ARROW, POISONED_ARROW, EXPLOSIVE_ARROW, BOLT,
// POWER_BOLT), skill.<X> (SKILL_X of crskill.hh), profession.<X>
// (PROFESSION_X of cr.hh).
//
// Logged texts: the game's error handler (shm.cc's ErrorHandler) hands the
// text to Log as its format string, so every text the engine logs with
// error() that comes from a script or from Lua (game.error, the script errors
// TScriptCall logs, LoadScript's messages) has each '%' doubled. Log then
// writes one '%'; the console (VerboseOutput) shows the doubled text.
//
// Integer arithmetic: Lua numbers are doubles. A script uses game.idiv and
// game.imod where the C++ code divides or takes the remainder of ints that
// may be negative; for non-negative ints math.floor(a / b) and a % b give the
// same.

#ifndef PATELLA_SCRIPTING_HH
#define PATELLA_SCRIPTING_HH

#include <cstdint>

// Creates the Lua state with the sandbox and the core API, for the scripts
// below Folder (a folder path without a trailing '/'; it need not exist until
// a script is loaded). Records the calling thread as the only one that may
// use the engine. Throws a const char * on failure (as InitAll's other Init
// functions), also when Lua cannot create the state. Calling it twice
// without ExitScripting is an error (throws).
void InitScripting(const char *Folder);

// Closes the state and forgets every loaded script. Safe without
// InitScripting.
void ExitScripting();

// Whether InitScripting has run and ExitScripting has not.
bool ScriptingActive();

// Loads the script Name (see above) once and gives its number, by which the
// handlers are called; a second LoadScript of the same name gives the same
// number without running it again. Where is the place that names the script,
// "<file>:<line>:<column>" of a TOML entry's script key, or nullptr. Throws a
// const char * "<Where>: <what>" (or "<what>" without Where) when the name is
// not allowed, the file is missing or cannot be read, the script does not
// compile ("<what>" then contains Lua's "spells/light.lua:12: <message>"),
// fails while it runs its top level (with the message and the traceback), or
// does not return a table. The text stays valid until the next call that
// throws one.
int LoadScript(const char *Name, const char *Where);

// The name of a loaded script (as given to LoadScript).
const char *ScriptName(int Script);

// Whether the table a script returned has a function Handler ("on_cast").
bool HasHandler(int Script, const char *Handler);

// Throws a const char * "<Where>: <script name> has no function <Handler>"
// (without "<Where>: " for nullptr) when the script lacks the handler. The
// loaders call it for every handler their entries need.
void RequireHandler(int Script, const char *Handler, const char *Where);

// A call of a script's handler, its arguments added in order:
//	TScriptCall Call(Script, "on_cast");
//	Call.addNumber(CreatureID);
//	Call.addString(Words);
//	Call.run();
// Arguments are numbers (ints, uint32_t such as creature IDs and object
// handles, all exact in a double), booleans, strings (copied; nullptr gives
// nil), arrays of strings (copied; a new Lua table per call) and nil. run calls the handler (which must exist: RequireHandler at
// load time; a missing one at run time is a non-result error) and gives back
// nothing; runNumber and runBoolean give its first result (a number that is
// not an integer, or not a number/boolean, is a non-result error; nil gives
// 0 or false). When the handler raises a result code, run throws it as
// RESULT; any other error is logged with error() as
// "Script error in <script name> (<handler>): <message>" and Lua's traceback
// on the following lines (taken where the error was raised, also inside a
// module that require loaded; the handlers call no metamethod of an error
// value, which is described by its type when it is not a string or a
// number), and run throws ERROR. A TScriptCall is run once.
// It holds no Lua references between its construction and run, so it may be
// built and run anywhere in the main thread.
class TScriptCall {
public:
	TScriptCall(int Script, const char *Handler);
	~TScriptCall();
	TScriptCall(const TScriptCall &) = delete;
	TScriptCall &operator=(const TScriptCall &) = delete;
	void addNumber(int Value);
	void addNumber(uint32_t Value);
	void addBoolean(bool Value);
	void addString(const char *Value);
	void addNil();
	// The Count strings of Values as one Lua array (Values[0] is index 1).
	void addStringArray(const char *const *Values, int Count);
	void run();
	int runNumber();
	bool runBoolean();
private:
	struct TArgument;
	int Script;
	const char *Handler;
	TArgument *Arguments;
	int NumberOfArguments;
	int Capacity;
	bool Done;
};

#endif
