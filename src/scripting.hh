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
//     never pointers: the objects (Creature, Player, Item, Vocation) hold
//     only an ID or a handle, and every API function resolves it again.
//  3. One source of randomness, the engine's: math.random(m, n) is random(m,
//     n) of utils.cc, math.random(m) is random(1, m), and Game.rand() is
//     rand(), as the C++ code calls them; math.random() without arguments is
//     an error, and math.randomseed does not exist.
//  4. No behaviour may depend on pairs order (pairs, next, table.foreach);
//     arrays and ipairs only. Nothing may depend on tostring of a table, a
//     function or a userdata either (it shows an address); the objects'
//     tostring has none ("Player 123", "Creature 123", "Item 456"), and print
//     refuses what would show one.
//  5. Result codes (RESULT) cross the boundary unchanged: an API function
//     that throws one raises a Lua error that carries it, and the C++ caller
//     of a handler throws it again. Any other Lua error is logged with its
//     file:line and traceback and thrown as ERROR.
//  6. A script makes the same calls, in the same order, with the same
//     arguments as the C++ code it replaces.
//  7. The sandbox: base (without dofile, loadfile, load, loadstring, _G,
//     getfenv, setfenv, rawset, newproxy, collectgarbage, gcinfo; print is
//     the engine's, below), table, string (without dump), math (random as in
//     rule 3, without randomseed), bit; coroutine comes with base. No io, os,
//     debug, ffi, jit, package. require loads only from the scripts folder.
//     The shared tables are read-only to scripts: the API tables (Game,
//     Magic, ...), the classes' method tables (Creature, Player, ...) and the
//     libraries (string, table, math, bit, coroutine) are proxies (userdata,
//     so type(Game) is "userdata" and the raw writes of table.insert cannot
//     reach them) whose writes are Lua errors ("<table>.<field> is
//     read-only"); the constants cannot be set, not even as a script's own
//     global ("<NAME> is read-only"); the strings' and the objects'
//     metatables cannot be read or changed (getmetatable gives false).
//     Scripts use no __gc and no __mode (weak tables): nothing may depend on
//     when the garbage collector runs.
//  8. Errors in scripts are found when they are loaded, at start; a load
//     error names the TOML position of the script's entry.
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
// to the shared globals (the API tables, the classes, the constants and the
// sandbox's libraries); its own globals stay in that environment. Its
// handlers are global functions of its own (function onCastSpell(...) ...
// end); it returns nothing (a value other than nil is a load error). A global
// name that is neither in the script's environment nor shared is an error
// when it is read ("unknown name 'x'"), and so is an unknown field of an API
// table or an unknown method: a script that uses one at its top level fails
// to load, at start.
//
// require(name) loads "lib/x.lua" for require("lib.x") (the dots become
// slashes, ".lua" is added), with the same rules, once: later calls give the
// value the first one returned (any value but nil; nil counts as true, as in
// Lua). A module that requires itself, directly or through others, is an
// error.
//
// Names. The API takes TFS's names and style (scripting_api.hh): tables and
// classes PascalCase, functions, methods and handlers camelCase, constants
// UPPER_CASE globals. Values are 7.72's; what 7.72 lacks does not exist.
//
// Objects. Creature, Player (a Creature) and Item are classes whose objects
// hold a creature ID or an object handle (rule 2); a Vocation holds its
// player's creature ID. A creature ID is a Player when it is a player's
// (below FIRST_NONPLAYER_CREATURE_ID), a Creature otherwise; nil stands
// where the C++ code has no creature (nullptr). Two objects are equal (==)
// when they are of the same class and hold the same ID or handle. A method
// is called as object:method(...); the classes' method tables are the
// shared globals Creature, Player, Item and Vocation (Player's falls back to
// Creature's). A creature that is gone gives, in a function that takes it,
// what the C++ function does for nullptr; where the C++ function would
// dereference nullptr, the binding logs "<binding name>: Creature does not
// exist." and raises ERROR instead. Where an object is expected, any other
// value is a Lua argument error.
//
// Lua's view of the engine (the core API, bound by scripting.cc):
//   math.random(m, n)       random(m, n) of utils.cc: m + rand() % (n - m +
//                           1), m for an empty span.
//   math.random(m)          random(1, m).
//   print(...)              print(2, "%s", Text) of utils.cc (2: the level
//                           of the game's informational messages), Text the
//                           arguments separated by tabs and ended by a
//                           newline: strings, numbers, booleans, nil, the
//                           objects ("Player 123") and result errors; any
//                           other value is a Lua error.
//   debugPrint(text)        error("%s", text) of utils.cc, with each '%' of
//                           text doubled (see below).
//   Game.rand()             rand(), as the C++ code calls it directly.
//   Game.idiv(a, b)         a / b of C++ ints (truncated toward zero); b == 0
//                           is a Lua error.
//   Game.imod(a, b)         a % b of C++ ints (the sign of a); b == 0 is a
//                           Lua error.
//   Game.print(level, text) print(Level, "%s", text) of utils.cc.
//   Game.throwResult(code)  raises the result code (a number of RESULT, or
//                           its RETURNVALUE_ name as a string) as a C++
//                           throw of it would.
//   Game.getResultOf(err)   the result code an error value carries (from
//                           pcall), as a number, or nil for another error.
//                           A result error's tostring is "result
//                           RETURNVALUE_<NAME>", and err.code is its number
//                           too.
//   RETURNVALUE_<NAME>      the numbers of RESULT (info.hh): TFS's name where
//                           it means the same, our own otherwise:
//     ERROR -1 RETURNVALUE_ERROR, NOERROR 0 RETURNVALUE_NOERROR,
//     NOTACCESSIBLE RETURNVALUE_NOTPOSSIBLE, NOTMOVABLE RETURNVALUE_NOTMOVEABLE,
//     NOTTAKABLE RETURNVALUE_CANNOTPICKUP, NOROOM RETURNVALUE_NOTENOUGHROOM,
//     OUTOFRANGE RETURNVALUE_DESTINATIONOUTOFREACH, OUTOFHOME
//     RETURNVALUE_OUTOFHOME, CANNOTTHROW RETURNVALUE_CANNOTTHROW, TOOHEAVY
//     RETURNVALUE_NOTENOUGHCAPACITY, CROSSREFERENCE RETURNVALUE_THISISIMPOSSIBLE,
//     CONTAINERFULL RETURNVALUE_CONTAINERNOTENOUGHROOM, WRONGPOSITION
//     RETURNVALUE_PUTTHISOBJECTINYOURHAND, WRONGPOSITION2
//     RETURNVALUE_PUTTHISOBJECTINBOTHHANDS, WRONGCLOTHES
//     RETURNVALUE_CANNOTBEDRESSED, HANDSNOTFREE RETURNVALUE_BOTHHANDSNEEDTOBEFREE,
//     HANDBLOCKED RETURNVALUE_DROPTWOHANDEDITEM, ONEWEAPONONLY
//     RETURNVALUE_CANONLYUSEONEWEAPON, NOMATCH RETURNVALUE_NOMATCH, NOTCUMULABLE
//     RETURNVALUE_NOTCUMULABLE, TOOMANYPARTS RETURNVALUE_TOOMANYPARTS,
//     EMPTYCONTAINER RETURNVALUE_EMPTYCONTAINER, SPLITOBJECT
//     RETURNVALUE_SPLITOBJECT, NOKEYMATCH RETURNVALUE_NOKEYMATCH, UPSTAIRS
//     RETURNVALUE_FIRSTGOUPSTAIRS, DOWNSTAIRS RETURNVALUE_FIRSTGODOWNSTAIRS,
//     CREATURENOTEXISTING RETURNVALUE_CREATUREDOESNOTEXIST, PLAYERNOTEXISTING
//     RETURNVALUE_PLAYERNOTEXISTING, PLAYERNOTONLINE
//     RETURNVALUE_PLAYERWITHTHISNAMEISNOTONLINE, NAMEAMBIGUOUS
//     RETURNVALUE_NAMEISTOOAMBIGUOUS, NOTUSABLE RETURNVALUE_CANNOTUSETHISOBJECT,
//     FEDUP RETURNVALUE_FEDUP, DESTROYED RETURNVALUE_DESTROYED, SPELLUNKNOWN
//     RETURNVALUE_YOUNEEDTOLEARNTHISSPELL, LOWMAGICLEVEL
//     RETURNVALUE_NOTENOUGHMAGICLEVEL, MAGICITEM
//     RETURNVALUE_YOUNEEDAMAGICITEMTOCASTSPELL, NOTENOUGHMANA
//     RETURNVALUE_NOTENOUGHMANA, NOSKILL RETURNVALUE_NOSKILL, TARGETLOST
//     RETURNVALUE_TARGETLOST, OUTOFAMMO RETURNVALUE_OUTOFAMMO, NOCREATURE
//     RETURNVALUE_CANONLYUSETHISRUNEONCREATURES, TOOLONG RETURNVALUE_TOOLONG,
//     TARGETOUTOFRANGE RETURNVALUE_TARGETOUTOFRANGE, TARGETHIDDEN
//     RETURNVALUE_TARGETHIDDEN, ATTACKNOTALLOWED
//     RETURNVALUE_YOUMAYNOTATTACKTHISPLAYER, NOWAY RETURNVALUE_THEREISNOWAY,
//     LOGINERROR RETURNVALUE_LOGINERROR, LOGINABORT RETURNVALUE_LOGINABORT,
//     PROTECTIONZONE RETURNVALUE_ACTIONNOTPERMITTEDINPROTECTIONZONE,
//     ENTERPROTECTIONZONE RETURNVALUE_ENTERPROTECTIONZONE, EXHAUSTED
//     RETURNVALUE_YOUAREEXHAUSTED, NOTINVITED RETURNVALUE_NOTINVITED,
//     NOPREMIUMACCOUNT RETURNVALUE_YOUNEEDPREMIUMACCOUNT, MOVENOTPOSSIBLE
//     RETURNVALUE_MOVENOTPOSSIBLE, ALREADYTRADING RETURNVALUE_YOUAREALREADYTRADING,
//     PARTNERTRADING RETURNVALUE_THISPLAYERISALREADYTRADING, TOOMANYOBJECTS
//     RETURNVALUE_TOOMANYOBJECTS, TOOMANYSLAVES RETURNVALUE_TOOMANYSLAVES,
//     NOTTURNABLE RETURNVALUE_NOTTURNABLE, SECUREMODE
//     RETURNVALUE_TURNSECUREMODETOATTACKUNMARKEDPLAYERS, NOTENOUGHSOULPOINTS
//     RETURNVALUE_NOTENOUGHSOUL, LOWLEVEL 60 RETURNVALUE_NOTENOUGHLEVEL.
//   creature:getId()        the creature ID the object holds.
// The areas' APIs follow, each bound by a Register...API function in a unit
// of its own (scripting_api.hh). A string a binding passes on as a char * is
// copied into a char[512] first, as CheckForSpell's SpellStr holds the
// words; nil gives nullptr where the signature says "|nil". Each function
// raises what the C++ function throws.
//
// Spells, runes and commands (phase 2; scripting_magic.cc, RegisterMagicAPI).
// An entry of spells.toml or commands.toml names its script with script =
// "<path>"; the script is named after the SPELL_ case of the C++ switch it
// replaces, lower case without SPELL_ (SPELL_LIGHT_HEALING ->
// "spells/light_healing.lua", SPELL_FIREBALL -> "runes/fireball.lua",
// SPELL_GET_ITEM_COUNT -> "commands/get_item_count.lua"). An entry whose
// case did not exist has no script and does what the switch did without a
// case. The handlers:
//   spells/<name>.lua    onCastSpell(caster, spell, mana, soulPoints, words)
//       CastSpell's case, after the checks: caster is the Player, spell the
//       entry's number, mana and soulPoints its mana and soul points
//       (SpellList[SpellNr].Mana, .SoulPoints), words an array with words[k]
//       = SpellStr[k] for k = 1 to the last entry CheckForSpell set (the
//       empty one that ends them included).
//   runes/<name>.lua     onCastSpell(user, spell, mana, soulPoints, target,
//                        targetCreature)
//       UseMagicItem's case, after the checks and the choice of the target:
//       user is the Player, target the Item of the object Dest (ground and
//       fields included), targetCreature the creature dest (a Player for a
//       player), nil where dest is nullptr.
//   commands/<name>.lua  onSay(caster, command, words)
//       CharacterRightSpell's or AccountRightSpell's case (only for a
//       player): caster is the Player, command the entry's number, words as
//       for spells.
//
// Magic: the spell functions of magic.cc (actor: a Creature; target: a
// Creature, or nil where it says "|nil"; obj: an Item or a Creature, whose
// object is taken (GetCreatureObject); mana, soul: what is spent, as the C++
// argument).
//   computeDamage(actor, spell, damage, variation) -> int     ComputeDamage
//   heal(actor, mana, soul, amount)                           Heal
//   massHeal(actor, obj, mana, soul, amount, radius)          MassHeal
//   healFriend(actor, name, mana, soul, amount)               HealFriend
//   combat(actor, obj, mana, soul, damage, effect, animation, damageType)
//                                                             Combat
//   massCombat(actor, obj, mana, soul, damage, effect, radius, damageType,
//       animation)                                            MassCombat
//   angleCombat(actor, mana, soul, damage, effect, radius, angle,
//       damageType)                                           AngleCombat
//   killAllMonsters(actor, effect, radius)                    KillAllMonsters
//   createField(actor, mana, soul, fieldType)                 CreateField(cr, Mana, SoulPoints, Type)
//   createFieldOn(actor, obj, mana, soul, fieldType)          CreateField(cr, Obj, Mana, SoulPoints, Type)
//   massCreateField(actor, obj, mana, soul, fieldType, radius)
//                                                             MassCreateField
//   createFieldWall(actor, obj, mana, soul, fieldType, extension)
//                                                             CreateFieldWall
//   deleteField(actor, obj, mana, soul)                       DeleteField
//   cleanupField(actor)                                       CleanupField(cr)
//   cleanupFieldOn(actor, obj, mana, soul)                    CleanupField(cr, Obj, Mana, SoulPoints)
//   magicRope(actor, mana, soul)                              MagicRope
//   magicClimbing(actor, mana, soul, direction)               MagicClimbing(cr, Mana, SoulPoints, Direction)
//   magicClimbingFree(actor, direction)                       MagicClimbing(cr, Direction)
//   createFood(actor, mana, soul)                             CreateFood
//   createArrows(actor, mana, soul, ammo, amount)             CreateArrows
//   summonCreature(actor, mana, raceName, god)                SummonCreature(cr, Mana, RaceName, God)
//   raiseDead(actor, obj, mana, soul)                         RaiseDead
//   massRaiseDead(actor, obj, mana, soul, radius)             MassRaiseDead
//   magicGoStrength(actor, target|nil, mana, soul, percent, duration)
//                                                             MagicGoStrength
//   shielding(actor, mana, soul, duration)                    Shielding
//   negatePoison(actor, mana, soul, target|nil)               NegatePoison
//   enlight(actor, mana, soul, radius, duration)              Enlight
//   invisibility(actor, mana, soul, duration)                 Invisibility
//   cancelInvisibility(actor, obj, mana, soul, radius)        CancelInvisibility
//   creatureIllusion(actor, mana, soul, raceName, duration)   CreatureIllusion
//   objectIllusion(actor, mana, soul, obj, duration)          ObjectIllusion
//   enchantObject(actor, mana, soul, originType, targetType)  EnchantObject (object type IDs)
//   convince(actor, target|nil)                               Convince
//   challenge(actor, mana, soul, radius)                      Challenge
//   findPerson(actor, mana, soul, name)                       FindPerson
// Command: the commands' functions of magic.cc (actor: the caster, a
// Creature; the other arguments are the words, as the C++ case passed
// SpellStr).
//   createThing(actor, p1, p2|nil)           CreateThing
//   teleport(actor, p)                       Teleport
//   createKnowledge(actor, p1, p2|nil)       CreateKnowledge
//   changeData(actor, p)                     ChangeData
//   teleportToCreature(actor, name)          TeleportToCreature
//   teleportPlayerToMe(actor, name)          TeleportPlayerToMe
//   showPosition(actor)                      GetPosition
//   createMoney(actor, p)                    CreateMoney
//   changeProfession(actor, p)               ChangeProfession
//   editGuests(actor)                        EditGuests
//   editSubowners(actor)                     EditSubowners
//   kickGuest(actor, name)                   KickGuest
//   editNameDoor(actor)                      EditNameDoor
//   getQuestValue(actor, p)                  GetQuestValue
//   setQuestValue(actor, p1, p2)             SetQuestValue
//   clearQuestValues(actor)                  ClearQuestValues
//   startMonsterraid(actor, name)            StartMonsterraid
//   homeTeleport(actor, name)                HomeTeleport
//   banishAccount(actor, name, n, reason)    BanishAccount
//   deleteAccount(actor, name, reason)       DeleteAccount
//   setNameRule(actor, name)                 SetNameRule
//   notation(actor, name, comment)           Notation
//   nameLock(actor, name)                    NameLock
//   kickPlayer(actor, name)                  KickPlayer
//   deleteCharacter(actor, name, reason)     DeleteCharacter
//   ipBanishment(actor, name, reason)        IPBanishment
//   banishCharacter(actor, name, n, reason)  BanishCharacter
// Game.toInt(text) -> int: (int)std::clamp(strtol(text, nullptr, 10),
//   (long)INT32_MIN, (long)INT32_MAX), as AccountRightSpell read numbers.
// creature:getSkillValue(skill) -> Skills[skill]->Get().
// player:getLevel() -> Skills[SKILL_LEVEL]->Get().
// player:getVocation() -> the player's Vocation:
//   vocation:getId() -> GetRealProfession() (with the promotion: 11 for an
//   elite knight), vocation:getBaseId() -> GetEffectiveProfession()
//   (without it: 1).
// ItemType.fromOldNumber(group, number) -> GetNewObjectType(Group,
//   Number), the type's ID.
// Constants (read-only globals, 7.72's values):
//   CONST_ME_NONE 0, CONST_ME_DRAWBLOOD 1 to CONST_ME_SOUND_WHITE 25 (the
//   effects; operate.hh's EFFECT_ where it names one), CONST_ANI_NONE 0 to
//   CONST_ANI_POISON 15 (the projectiles; ANIMATION_), COMBAT_PHYSICALDAMAGE,
//   COMBAT_POISONDAMAGE = COMBAT_EARTHDAMAGE, COMBAT_FIREDAMAGE,
//   COMBAT_ENERGYDAMAGE, COMBAT_POISONCONDITION, COMBAT_FIRECONDITION,
//   COMBAT_ENERGYCONDITION, COMBAT_LIFEDRAIN, COMBAT_MANADRAIN (cr.hh's
//   DAMAGE_), FIELD_FIRE, FIELD_POISON, FIELD_ENERGY, FIELD_MAGICWALL,
//   FIELD_RUSHWOOD (info.hh), CONJURE_ARROW, CONJURE_POISONED_ARROW,
//   CONJURE_EXPLOSIVE_ARROW, CONJURE_BOLT, CONJURE_POWER_BOLT (magic.hh),
//   SKILL_LEVEL, SKILL_MAGLEVEL, SKILL_SHIELD, SKILL_DISTANCE, SKILL_SWORD,
//   SKILL_CLUB, SKILL_AXE, SKILL_FIST, SKILL_FISHING (TFS's names),
//   SKILL_HITPOINTS, SKILL_MANA, SKILL_GOSTRENGTH, SKILL_CARRYSTRENGTH,
//   SKILL_MAGICDEFENSE, SKILL_EATING, SKILL_SHINING, SKILL_ILLUSION,
//   SKILL_POISON, SKILL_BURNING, SKILL_ENERGY, SKILL_DRUNKEN,
//   SKILL_MANASHIELD, SKILL_SOULPOINTS (crskill.hh), VOCATION_NONE,
//   VOCATION_KNIGHT, VOCATION_PALADIN, VOCATION_SORCERER, VOCATION_DRUID,
//   VOCATION_PROMOTION (cr.hh's PROFESSION_).
//
// Logged texts: the game's error handler (shm.cc's ErrorHandler) hands the
// text to Log as its format string, so every text the engine logs with
// error() that comes from a script or from Lua (debugPrint, the script errors
// TScriptCall logs, LoadScript's messages) has each '%' doubled. Log then
// writes one '%'; the console (VerboseOutput) shows the doubled text.
//
// Integer arithmetic: Lua numbers are doubles. A script uses Game.idiv and
// Game.imod where the C++ code divides or takes the remainder of ints that
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
// returns a value other than nil ("<name>: a script returns nothing; its
// handlers are global functions"). The text stays valid until the next call
// that throws one.
int LoadScript(const char *Name, const char *Where);

// The name of a loaded script (as given to LoadScript).
const char *ScriptName(int Script);

// Whether the script has a global function Handler ("onCastSpell") of its
// own.
bool HasHandler(int Script, const char *Handler);

// Throws a const char * "<Where>: <script name> has no function <Handler>"
// (without "<Where>: " for nullptr) when the script lacks the handler. The
// loaders call it for every handler their entries need.
void RequireHandler(int Script, const char *Handler, const char *Where);

// A call of a script's handler, its arguments added in order:
//	TScriptCall Call(Script, "onCastSpell");
//	Call.addPlayer(CreatureID);
//	Call.addString(Words);
//	Call.run();
// Arguments are numbers (ints, uint32_t, all exact in a double), booleans,
// strings (copied; nullptr gives nil), arrays of strings (copied; a new Lua
// table per call), the objects (a Player, a creature as a Creature or a
// Player, an Item; a new object per call) and nil. run calls the handler
// (which must exist: RequireHandler at load time; a missing one at run time
// is a non-result error) and gives back nothing; runNumber and runBoolean
// give its first result (a number that is not an integer, or not a
// number/boolean, is a non-result error; nil gives 0 or false). When the
// handler raises a result code, run throws it as RESULT; any other error is
// logged with error() as "Script error in <script name> (<handler>):
// <message>" and Lua's traceback on the following lines (taken where the
// error was raised, also inside a module that require loaded; the handlers
// call no metamethod of an error value, which is described by its type when
// it is not a string or a number), and run throws ERROR. A TScriptCall is
// run once. It holds no Lua references between its construction and run, so
// it may be built and run anywhere in the main thread.
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
	// A Player object holding the player's creature ID.
	void addPlayer(uint32_t CreatureID);
	// The creature as an object: nil for 0, a Player for a player's ID, a
	// Creature otherwise.
	void addCreature(uint32_t CreatureID);
	// An Item object holding the object handle (Object::getObjectID).
	void addItem(uint32_t ObjectHandle);
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
