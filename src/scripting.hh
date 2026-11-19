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
// Objects. Creature, Player (a Creature), Npc and Monster (Creatures, phases 5
// and 6) and Item are classes whose objects hold a creature ID or an object
// handle (rule 2); a Vocation holds its player's creature ID, a House (phase
// 9) a house number. A creature ID is a Player when it is a player's (below
// FIRST_NONPLAYER_CREATURE_ID), a Creature otherwise; nil
// stands where the C++ code has no creature (nullptr). Two objects are equal
// (==) when they are of the same class and hold the same ID or handle, so two
// Vocation objects are equal only for the same player, not for two players of
// the same vocation (compare their getId()). A method is called as
// object:method(...); the classes' method tables are the shared globals
// Creature, Player, Npc, Monster, Item, Vocation, House and Position
// (Player's, Npc's and Monster's fall back to Creature's). An NPC's creature ID is an Npc only where an NPC's script gets
// its NPC (the handlers of phase 5); Game.getCreature and item:getCreature give
// a Creature for it. A creature that is gone gives, in a function that takes
// it, what the C++ function does for nullptr; where the C++ function would
// dereference nullptr, the binding logs "<binding name>: Creature does not
// exist." and raises ERROR instead. Where an object is expected, any other
// value is a Lua argument error. An Item method may be called as a function of
// the class with nil for self (Item.getId(nil)), and every Item parameter takes
// nil; both stand for NONE, which the binding passes on to the C++ function
// unchecked (it logs and gives what the C++ gives for NONE).
//
// Lua's view of the engine (the core API, bound by scripting.cc):
//   math.random(m, n)       random(m, n) of utils.cc: m + rand() % (n - m +
//                           1), m for an empty span.
//   math.random(m)          random(1, m).
//   print(...)              print(2, "%s", Text) of utils.cc (2: the level
//                           of the game's informational messages), Text the
//                           arguments separated by tabs and ended by a
//                           newline: strings, numbers, booleans, nil, the
//                           objects ("Player 123"), Positions ("Position(x,
//                           y, z)") and result errors; any
//                           other value is a Lua error. print() of utils.cc
//                           formats into 1024 bytes, so a longer text loses
//                           its end and its newline, and a zero byte ends
//                           the text; the game's log keeps 126 characters
//                           of a line, at debug level 2 or more.
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
// a copy: phase 2's bindings copy it into a char[512], as CheckForSpell's
// SpellStr holds the words, the later ones copy it whole; nil gives nullptr
// where the signature says "|nil". Each function
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
//   commands/<name>.lua  onSay(caster, command, words, talk)
//       CharacterRightSpell's or AccountRightSpell's case (only for a
//       player): caster is the Player, command the entry's number, words as
//       for spells; talk the words of the talkaction (talkactions.toml) that
//       gave the command ("/goto"), with words built as the command's own
//       words give them (each parameter word a part of the talkaction's
//       parameter), or nil for the command's own words.
//
// Magic: the spell functions of magic.cc (actor: a Creature; target: a
// Creature, or nil where it says "|nil"; obj: an Item or a Creature, whose
// object is taken (GetCreatureObject); mana, soul: what is spent, as the C++
// argument).
//   computeDamage(actor|nil, spell, damage, variation) -> int ComputeDamage
//   heal(actor, mana, soul, amount)                           Heal
//   massHeal(actor, obj, mana, soul, amount, radius)          MassHeal
//   healFriend(actor, name, mana, soul, amount)               HealFriend
//   combat(actor, obj, mana, soul, damage, effect, animation, damageType)
//                                                             Combat
//   massCombat(actor, obj, mana, soul, damage, effect, radius, damageType,
//       animation)                                            MassCombat
//   angleCombat(actor, mana, soul, damage, effect, radius, angle,
//       damageType)                                           AngleCombat
//   createField(actor, mana, soul, fieldType)                 CreateField(cr, Mana, SoulPoints, Type)
//   createFieldOn(actor, obj, mana, soul, fieldType)          CreateField(cr, Obj, Mana, SoulPoints, Type)
//   massCreateField(actor, obj, mana, soul, fieldType, radius)
//                                                             MassCreateField
//   createFieldWall(actor, obj, mana, soul, fieldType, extension)
//                                                             CreateFieldWall
//   deleteField(actor, obj, mana, soul)                       DeleteField
//   cleanupFieldOn(actor, obj|nil, mana, soul)                CleanupField(cr, Obj, Mana, SoulPoints)
//   magicRope(actor, mana, soul)                              MagicRope
//   magicClimbing(actor, mana, soul, direction)               MagicClimbing(cr, Mana, SoulPoints, Direction)
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
// Trigger rules (phase 3; scripting_triggers.cc, RegisterTriggerAPI). The
// rules of triggers.toml keep their order; HandleEvent (triggers.cc) keeps the
// loop over the rules of an event, the loop limit and "the first rule whose
// conditions hold", and calls Lua once per rule. The conditions and actions
// are the functions of the trigger library, the script lib/triggers.lua (the
// conditions; it requires the module lib/triggers_actions.lua, the actions),
// named there as triggers.toml names them (is_type, move_rel, ...). The
// library defines these global functions, which the C++ side calls:
//   prepareCondition(rule, name, negate, ignoreResult, ...)
//   prepareAction(rule, name, ...)
//       At start, for each condition and each action of each rule, in the
//       order of triggers.toml; rule is the rule's number in the file (1 for
//       the first [[rule]]), the arguments those of the entry, converted by
//       their kind (triggersdata.hh): an object as its name ("obj1", "obj2",
//       "user", "temp", "null"), a type, flag, type_attribute,
//       instance_attribute, right, skill or number as a number, a coordinate
//       or a vector as a Position, a text as the number of the dynamic string
//       the loader made of it (AddDynamicString, in the file's order, as
//       before), a comparison as its text ("<", "<=", "=", ">=", ">", "<>").
//       A name the library does not know is a Lua error, which the loader
//       reports with the entry's position.
//   evaluateRule(rule, event, user, item1, item2) -> boolean
//       HandleEvent's check of one rule: event is the event's name ("use",
//       "multi_use", "movement", "collision", "separation"), user, item1 and
//       item2 the event's objects as Items (nil for NONE). It checks the
//       rule's conditions in order (negate inverts one, ignoreResult makes
//       one hold whatever it gives; a condition may set the rule's temp
//       object), and when all hold it runs the actions in order and gives
//       true. An action that raises a result code is logged as
//       "ExecuteAction: Exception <code> (action <name>)." and the next one
//       runs, as ExecuteAction caught RESULT.
// A rule may name a script instead (script = "triggers/<name>.lua", no
// conditions or actions), for new content: its global functions
// onCheck(event, user, item1, item2) -> boolean and onRun(event, user, item1,
// item2) take the place of the conditions and the actions. None of the
// converted rules does.
//
// Position: an object holding a field's coordinates (or a vector's offsets),
// three ints; Position(x, y, z) makes one; position.x, .y, .z read them;
// == compares them; it cannot be changed (a new one is made instead).
// The objects' methods for the trigger library (Item: any object handle;
// nil stands for NONE where an argument says |nil or a result says so):
//   item:getId() -> getObjectType().getTypeID().
//   item:isCreature() -> isCreature().
//   item:getCreature() -> the creature of a creature object as a Creature
//       or Player (GetCreature(Obj)), nil when there is none.
//   item:getCreatureId() -> getCreatureID().
//   item:hasFlag(flag) -> getFlag(flag) (the FLAG number).
//   item:getTypeAttribute(attribute) -> getAttribute(TYPEATTRIBUTE).
//   item:getAttribute(attribute) -> getAttribute(INSTANCEATTRIBUTE).
//   item:setAttribute(attribute, value) -> Change(Obj, INSTANCEATTRIBUTE,
//       Value) of operate.cc (sends the change; raises what it throws).
//   item:getPosition() -> GetObjectCoordinates, as a Position.
//   item:getContainer() -> getContainer() (nil for NONE).
//   item:getMapContainer() -> GetMapContainer(Obj).
//   item:getNextObject() -> getNextObject() (nil for NONE).
//   item:getFirstObject() -> GetFirstContainerObject(Obj) (nil for NONE).
//   item:countObjects() -> CountObjectsInContainer(Obj).
//   item:getBodyPosition() -> GetObjectBodyPosition(Obj).
//   item:getDescription() -> GetInfo(Obj).
//   item:getCronInfo() -> CronInfo(Obj, false).
//   item:sendMagicEffect(effect) -> GraphicalEffect(Obj, Effect).
//   creature:isPlayer() -> Type == PLAYER.
//   creature:isPeaceful() -> IsPeaceful().
//   creature:getName() -> Name.
//   creature:probeSkill(skill, a, b) -> Skills[skill]->Probe(a, b, true);
//       skill 0 to SKILL_LAST.
//   creature:damage(attacker|nil, amount, damageType) -> Damage(atk,
//       Amount, DamageType).
//   creature:startLogout() -> StartLogout(true, true).
//   player:canLogout() -> LogoutPossible() == LOGOUT_POSSIBLE.
//   player:hasRight(right) -> CheckRight(ID, right) (the RIGHT number).
//   player:getStorageValue(number) -> GetQuestValue(Number).
//   player:setStorageValue(number, value) -> SetQuestValue(Number, Value).
//   player:isHouseOwner(houseId) -> IsOwner(HouseID, pl).
//   player:setStartPosition(position) -> the start coordinates set to
//       position, SaveData() (the action set_start logs "Setting new start
//       position for <name>." with Game.log first).
//   player:sendTextMessage(mode, text) -> SendMessage(GetConnection(), Mode,
//       "%s", Text).
// Game, the map:
//   Game.getCreature(id) -> GetCreature(ID) as a Creature or Player, nil for
//       none.
//   Game.getMapContainer(position) -> GetMapContainer(x, y, z).
//   Game.getFirstObject(position) -> GetFirstObject(x, y, z) (nil: none).
//   Game.getFirstObjectOfType(position, type) -> GetFirstSpecObject(x, y,
//       z, ObjectType(Type)) (nil: none; type TYPEID_CREATURE finds a
//       creature).
//   Game.getTopObject(position) -> GetTopObject(x, y, z, false) (nil: none).
//   Game.getInventoryObject(creatureId, type, value) ->
//       GetInventoryObject(CreatureID, ObjectType(Type), Value) (nil: none).
//   Game.isProtectionZone(position), Game.isHouse(position),
//       Game.getHouseId(position) -> IsProtectionZone, IsHouse, GetHouseID.
//   Game.createObject(container, type, value) -> CreateObject(Con,
//       ObjectType(Type), Value) of triggers.cc.
//   Game.changeObject(item, type, value) -> ChangeObject(Obj,
//       ObjectType(Type), Value) of triggers.cc.
//   Game.createMonsterOfRace(race, position) -> CreateMonster(Race, x, y, z,
//       0, 0, true).
//   Game.textEffect(text|nil, position, radius) -> TextEffect(Text, x, y,
//       z, Radius) of triggers.cc.
//   Game.getDynamicString(number) -> GetDynamicString(Number) (nil for
//       nullptr), Game.addDynamicString(text) -> AddDynamicString(Text),
//       Game.deleteDynamicString(number) -> DeleteDynamicString(Number).
//   Game.moveObject(item, container) -> MoveOneObject(Obj, Con).
//   Game.moveAllObjects(item|nil, container, exclude|nil, moveUnmovable) ->
//       MoveAllObjects(Obj, Dest, Exclude, MoveUnmovable).
//   Game.deleteAllObjects(item|nil, exclude|nil, deleteUnmovable) ->
//       DeleteAllObjects(Obj, Exclude, DeleteUnmovable).
//   Game.deleteObject(item, count) -> Delete(Obj, Count).
//   Game.deleteAtCreature(creatureId, type, amount, value) ->
//       DeleteAtCreature(CreatureID, ObjectType(Type), Amount, Data).
//   Game.loadDepotBox(creatureId, number, container),
//       Game.saveDepotBox(creatureId, number, container) -> LoadDepotBox,
//       SaveDepotBox of triggers.cc.
//   Game.sendMail(item) -> SendMail(Obj) of triggers.cc.
// Constants: TYPEID_CREATURE (the object type of creatures), SKILL_LAST
// (MAX_SKILLS - 1), MESSAGE_INFO_DESCR (TALK_INFO_MESSAGE, TFS's name), and
// the numbers of objects.hh's enums by their C++ names: FLAG_<NAME> for
// every FLAG (FLAG_CLOTHES, FLAG_MAGICFIELD, FLAG_TEXT, FLAG_UNMOVE, ...),
// ITEMTYPE_ATTRIBUTE_<NAME> for every TYPEATTRIBUTE
// (ITEMTYPE_ATTRIBUTE_TOTALEXPIRETIME, ITEMTYPE_ATTRIBUTE_BODYPOSITION, ...)
// and ITEM_ATTRIBUTE_<NAME> for every INSTANCEATTRIBUTE
// (ITEM_ATTRIBUTE_TEXTSTRING, ITEM_ATTRIBUTE_EDITOR,
// ITEM_ATTRIBUTE_RESPONSIBLE, ...).
//
// Built-in uses of objects (phase 4; scripting_items.cc, RegisterItemAPI).
// Use (operate.cc) keeps its checks (the object exists, can be reached,
// the second object can be reached or thrown at) and then tries the
// entries of the ordered list [[use]] of items.toml, each a flag (as
// items.toml names the flags) and a script (script = "items/<name>.lua"):
// for the first entry whose flag the used object has, it calls the script's
// global function
//   onUse(player, item, target, window) -> boolean
// player the user (a Player), item the used object, target the object it is
// used on (an Item, nil for none), window the container window the client
// asked for (Use's Info). The function gives true when it handled the use;
// false sends Use on to the next entry whose flag the object has (as the
// C++ chain went on to its next branch where a branch's extra condition
// did not hold: a weapon that is no sword or axe or a target that cannot be
// destroyed, a text whose font size is not 1). No entry handles it: Use
// throws NOTUSABLE. A result code the function raises goes to Use's caller.
// The delivered list follows the C++ chain: container, chest,
// liquid_container, food, write, write_once, information, rune, key,
// name_door, level_door, quest_door, weapon, change_use, use_event, text.
// What stays in C++ and is called by the scripts: opening and closing a
// container window (UseContainer), the text dialog (SendEditText), a rune's
// use (UseMagicItem, phase 2) and the trigger rules of use and multi_use
// (UseObject, UseObjects): they are protocol or have their own scripts.
//
// ItemType: an object holding an object type's number; ItemType(id) makes
// one: it builds ObjectType(id) once, when it is made (an invalid number logs
// there, as the C++ code's ObjectType did, and gives the type 0), and keeps
// the resulting number.
//   itemType:getId() -> the number.
//   itemType:hasFlag(flag) -> ObjectType(id).getFlag(flag).
//   itemType:getAttribute(attribute) -> getAttribute(TYPEATTRIBUTE).
//   itemType:getName(count) -> getName(Count) (-1: the name without amount).
// More object methods for the uses (Item self and Item parameters take nil
// for NONE, as in the trigger rules):
//   item:exists() -> exists().
//   item:getName() -> GetName(Obj).
//   item:getWeight() -> GetCompleteWeight(Obj).
//   item:transform(type, value) -> Change(Obj, ObjectType(Type), Value) of
//       operate.cc.
//   item:clearField(exclude|nil) -> ClearField(Obj, Exclude) of
//       triggers.cc.
//   item:empty(rest) -> Empty(Obj, Rest) of operate.cc.
//   creature:isRemoved() -> GetCreature(ID) == nullptr (for a Player:
//       GetPlayer(ID) == nullptr).
//   creature:say(text) -> Talk(ID, TALK_SAY, nullptr, Text, false).
//   player:getSkillTimer(skill) -> Skills[skill]->TimerValue().
//   player:getSkillMax(skill) -> Skills[skill]->GetMax().
//   player:setSkillTimer(skill, cycle, count, maxCount, additional) ->
//       SetTimer(skill, Cycle, Count, MaxCount, AdditionalValue).
//   player:useContainer(item, window) -> UseContainer(ID, Con, Window).
//   player:sendEditText(item) -> SendEditText(GetConnection(), Obj).
//   player:mayOpenDoor(door) -> MayOpenDoor(Door, pl).
// Game:
//   Game.move(creatureId, item, container, count, noMerge, ignore|nil) (item
//       an Item, or a Creature for its creature object) ->
//       Move(CreatureID, Obj, Con, Count, NoMerge, Ignore) of operate.cc.
//   Game.copyObject(container, item) -> Copy(Con, Source), the copy.
//   Game.createPool(container, type, liquidType) -> CreatePool(MapCon,
//       ObjectType(Type), LiquidType).
//   Game.getBodyObject(creatureId, position) -> GetBodyObject (nil: none),
//       Game.getBodyContainer(creatureId, position) -> GetBodyContainer.
//   Game.getInventoryWeight(creatureId) -> GetInventoryWeight.
//   Game.isLayable(position) -> CoordinateLayable(x, y, z).
//   Game.getSpecialObject(meaning) -> GetSpecialObject(Meaning), the type's
//       number.
//   Game.getDate() -> year, cycle, day (GetDate); Game.getTime() -> hour,
//       minute (GetTime).
//   Game.useObject(player, item) -> UseObject(ID, Obj),
//       Game.useObjects(player, item, target) -> UseObjects(ID, Obj1, Obj2)
//       (the trigger rules of use and multi_use).
//   Magic.useMagicItem(player, item, target) -> UseMagicItem(ID, Obj,
//       Dest) (phase 2's rune use).
//   Magic.refreshMana(actor, mana, soul, amount) -> RefreshMana.
// Constants (7.72's values): LIQUID_NONE ... LIQUID_LEMONADE (objects.hh),
// INFORMATION_DATE, INFORMATION_TIME, INFORMATION_BLESSINGS,
// INFORMATION_SPELLBOOK, FIRST_BLESSING_QUEST, LAST_BLESSING_QUEST,
// MAX_QUESTS, MAX_OPEN_CONTAINERS, LAST_BODY_POSITION, WEAPON_NONE ...
// WEAPON_THROW (cr.hh's weapon types, TFS's names), SPECIAL_<NAME> for every
// SPECIALMEANING (SPECIAL_BLOOD_POOL, SPECIAL_RUNE_BLANK, ...), RIGHT_<NAME>
// for every RIGHT of info.hh (RIGHT_UNLIMITED_CAPACITY, RIGHT_ZERO_CAPACITY,
// RIGHT_OPEN_NAMEDOORS, ...).
//
// NPCs (phase 5; scripting_npcs.cc, RegisterNpcAPI). Each [[npc]] of
// npcs.toml (the game's data folder; loaded in the file's order by InitNPCs,
// crnonpl.cc) is one NPC and names its script (script = "npcs/<name>.lua").
// TNPC keeps in C++ the stimuli, the walking, the conversation state (State,
// Interlocutor, Topic, Price, Amount, TypeID, Data, LastTalk), the queue and
// the trade helpers; where it called TBehaviourDatabase::react, it calls one
// of the script's two global functions, which the loader requires:
//   onCreatureSay(npc, creature, text, situation)
//       react(Text, Situation) for a talk situation. TalkStimulus, a player's
//       say near the NPC: NPC_SITUATION_ADDRESS (the NPC was free; before the
//       call its to-do list is cleared, State is TALKING, the speaker the
//       interlocutor and LastTalk this round), NPC_SITUATION_DEFAULT (the
//       interlocutor talks; LastTalk is this round), NPC_SITUATION_BUSY
//       (another player talks while the NPC talks or has a queue: the
//       interlocutor is that player during the call and is set back after
//       it). IdleStimulus, the first player of the queue:
//       NPC_SITUATION_ADDRESSQUEUE, text the greeting it queued (State is
//       TALKING, the interlocutor that player). npc is the NPC (an Npc),
//       creature the interlocutor at the call (TScriptCall::addCreature), text
//       the player's words.
//   onCreatureDisappear(npc, creature)
//       react("", VANISH): IdleStimulus when the interlocutor said nothing
//       for 30 rounds, CreatureMoveStimulus when the interlocutor left the
//       game or the NPC's reach (5 fields in x, 4 in y, another floor).
// What the C++ code does around the call stays there (State IDLE after a
// VANISH, the turn to the interlocutor after an ADDRESS, the queue's
// order). A result code the function raises goes to the stimulus's caller
// as one react passed on; any other error is logged and the stimulus
// carries on (TScriptCall::runCaught, phase 6).
// The converted scripts hold the NPC's rules and hand both calls to the
// rule matcher, the module lib/npc.lua:
//	local behaviour = require("lib.npc")
//	local rules = behaviour.rules { <rule>, ... }
//	function onCreatureSay(npc, creature, text, situation)
//		behaviour.react(npc, rules, text, situation)
//	end
//	function onCreatureDisappear(npc, creature)
//		behaviour.react(npc, rules, "", NPC_SITUATION_VANISH)
//	end
// A rule block (an .ndb file of the original) is the module
// lib/npcs/<name>.lua; it returns behaviour.rules { ... }, and an NPC that
// included it has that value among its rules where the include was.
//
// lib/npc.lua, the matcher (what react and evaluate of TBehaviourDatabase
// did; the module returns a table of the following, which no script
// changes):
//   behaviour.rules(list) -> rules
//       A rule list made of list, an array whose elements are rules and
//       rule lists (a block's, put in place, so its rules count as the
//       list's own in order: a previous in the first rule of a block names
//       the rule before the block). Checks every element at load (a Lua
//       error that names the rule's number for a wrong one). Two things the
//       original's parser refused are allowed: an empty action list (no
//       action, as nop) and a previous in an NPC's first rule (react logs
//       "No previous pattern" when it runs).
//   behaviour.react(npc, rules, text, situation)
//       Matches text against the rules and runs the actions of the rule it
//       chooses, as react did.
// A rule is {{<condition>, ...}, {<action>, ...}} (the conditions may be
// empty: the rule holds always, with 0 conditions). The elements, by the
// names of the module (a converted script makes locals of those it uses,
// local TOPIC = behaviour.TOPIC; the original's syntax in brackets):
//   Conditions:
//     a string                 a word the text must contain ("hello$"),
//                              searched with Game.searchWord from where the
//                              previous word or number ended.
//     ADDRESS, BUSY, VANISH, MALE, FEMALE, KNIGHT, PALADIN, SORCERER,
//     DRUID, PREMIUM, PROMOTED, PZBLOCK, NONPVP, PVPENFORCED
//                              a property (ADDRESS also holds for
//                              ADDRESSQUEUE).
//     FORCE                    the rule is taken at once if the conditions
//                              before it held (!).
//     P1, P2, P3               the first, second or third number in the text
//                              after the previous word or number (%1 to %3).
//     eq(a, b), ne(a, b), lt(a, b), gt(a, b), le(a, b), ge(a, b)
//                              a comparison of two values (=, <>, <, >, <=,
//                              >=).
//   Values (expressions; C++ ints):
//     a number                 an integer, -2^31 to 2^31 - 1.
//     P1, P2, P3, param(n)     the number a condition P1 to P3 took (%1 to
//                              %3; param(n) for another n, which evaluate
//                              refuses).
//     TOPIC, PRICE, AMOUNT, TYPE, DATA
//                              the NPC's conversation variables (Topic,
//                              Price, Amount, Type, Data).
//     LEVEL, MAGLEVEL, HP, BURNING, POISON
//                              the interlocutor's skill (Level, MagLevel, HP;
//                              Burning and Poison: the skill's timer).
//     COUNTMONEY               the interlocutor's money (CountMoney).
//     count(v), spellKnown(v), spellLevel(v), questValue(v), random(a, b)
//                              (Count, SpellKnown, SpellLevel, QuestValue,
//                              Random).
//     add(a, b), sub(a, b), mul(a, b)
//                              a + b, a - b, a * b, wrapping as C++ ints do.
//   Actions:
//     a string                 a text the NPC says (%N, %A, %P, %T).
//     setTopic(v), setPrice(v), setAmount(v), setType(v), setData(v), setHp(v)
//                              (Topic=, Price=, Amount=, Type=, Data=, HP=).
//     burning(a, b), poison(a, b), setQuestValue(a, b)
//     effectMe(v), effectOpp(v), profession(v), teachSpell(v), summon(v),
//     create(v), delete(v)
//     createMoney, deleteMoney, queue, idle, nop
//     teleport(x, y, z), startPosition(x, y, z)
//     previous                 the actions of the rule before, from its
//                              first; the actions after previous do not run
//                              (*).
// A converted script makes its locals one per line, each name it uses once,
// in this order: ADDRESS, BUSY, VANISH, MALE, FEMALE, KNIGHT, PALADIN,
// SORCERER, DRUID, PREMIUM, PROMOTED, PZBLOCK, NONPVP, PVPENFORCED, FORCE,
// P1, P2, P3, eq, ne, lt, gt, le, ge, param, TOPIC, PRICE, AMOUNT, TYPE,
// DATA, LEVEL, MAGLEVEL, HP, BURNING, POISON, COUNTMONEY, count, spellKnown,
// spellLevel, questValue, random, add, sub, mul, setTopic, setPrice,
// setAmount, setType, setData, setHp, burning, poison, setQuestValue,
// effectMe, effectOpp, profession, teachSpell, summon, create, delete,
// createMoney, deleteMoney, queue, idle, nop, teleport, startPosition,
// previous. data/scripts/npcs/cipfried.lua shows the form.
//
// The NPC's API (bindings of scripting_npcs.cc). Npc is a class of objects
// that hold an NPC's creature ID (rule 2); its method table falls back to
// Creature's, a TScriptCreature parameter takes an Npc, and an Npc equals
// only an Npc of the same ID (tostring "Npc 123"). A method on an ID that is
// no NPC's (gone, or another creature) logs "<binding name>: NPC does not
// exist." and raises ERROR.
//   npc:getInterlocutor() -> GetPlayer(Interlocutor) as a Player, nil for
//       nullptr.
//   npc:getTopic(), npc:getPrice(), npc:getAmount(), npc:getTypeId() ->
//       Topic, Price, Amount, TypeID; npc:getData() -> (int)Data.
//   npc:setTopic(v), npc:setPrice(v), npc:setAmount(v), npc:setTypeId(v) ->
//       the same set to v; npc:setData(v) -> Data = (uint32_t)v.
//   npc:getState() -> State; npc:setState(state) -> State = state (a state
//       outside 0 to 6 is the binding's error).
//   npc:setLastTalk(round) -> LastTalk = (uint32_t)round.
//   npc:toDoWait(delay) -> ToDoWait(Delay); npc:toDoSay(text) ->
//       ToDoTalk(TALK_SAY, nullptr, Text, false); npc:toDoChangeState(state)
//       -> ToDoChangeState(State); npc:toDoStart() -> ToDoStart().
//   npc:enqueue(player, text) -> Enqueue(player's ID, Text).
//   npc:giveItems(type, amount) -> GiveTo(Type, Amount); npc:takeItems(type,
//       amount) -> GetFrom(Type, Amount); npc:giveMoney(amount) ->
//       GiveMoney(Amount); npc:takeMoney(amount) -> GetMoney(Amount): the
//       trade helpers, with the interlocutor, Data and their log lines as
//       before (type an object type number, made ObjectType(Type) as the C++
//       call converted the int).
// More creature and player methods:
//   creature:getPosition() -> posx, posy, posz as a Position.
//   creature:sendMagicEffect(effect) -> GraphicalEffect(GetCreatureObject(),
//       Effect).
//   creature:getHealth() -> Skills[SKILL_HITPOINTS]->Get();
//       creature:setHealth(value) -> Skills[SKILL_HITPOINTS]->Set(Value).
//   creature:die() -> Death().
//   creature:setDamageOrigin(skill, origin|nil) -> FireDamageOrigin (skill
//       SKILL_BURNING) or PoisonDamageOrigin (SKILL_POISON) = origin's
//       creature ID, 0 for nil; another skill is the binding's error.
//   player:getSex() -> Sex.
//   player:isPzLocked() -> EarliestProtectionZoneRound > RoundNr.
//   player:getItemCount(type, data) -> CountInventoryObjects(ID, Type,
//       (uint32_t)Data).
//   player:getMoney() -> CountInventoryMoney(ID).
//   player:hasLearnedSpell(spell) -> SpellKnown(Spell);
//       player:learnSpell(spell) -> LearnSpell(Spell) (spell numbers).
//   player:setVocation(id) -> SetProfession((unsigned char)ID).
//   player:setStartPosition(position) (phase 3's, above): the start
//       coordinates set and SaveData(), what the NPC's StartPosition did.
// Game and Magic:
//   Game.searchWord(word, text, start) -> SearchForWord(Word, Text + start -
//       1) of strings.cc, as the index of the match in text (1 for its first
//       byte), nil for nullptr; Game.searchNumber(n, text, start) ->
//       SearchForNumber(N, Text + start - 1) the same way. start counts from
//       1 (below 1 is the binding's error); a start past the end of text
//       (#text + 2 or more) searches the empty text after the end (react
//       searched the bytes that followed the text in memory, another
//       string's).
//   Game.getWorldType() -> WorldType.
//   Game.getRoundNumber() -> RoundNr.
//   Game.log(name, text) -> Log(Name, "%s", Text).
//   Magic.getSpellLevel(spell) -> GetSpellLevel(Spell).
// Constants (7.72's values): NPC_SITUATION_DEFAULT 0, NPC_SITUATION_ADDRESS
// 1, NPC_SITUATION_ADDRESSQUEUE 2, NPC_SITUATION_BUSY 3,
// NPC_SITUATION_VANISH 4 (crnonpl.cc's SITUATION); NPC_STATE_SLEEPING 0,
// NPC_STATE_IDLE 1, NPC_STATE_TALKING 3, NPC_STATE_LEAVING 4 (its STATE);
// PLAYERSEX_MALE 1, PLAYERSEX_FEMALE 2 (cr.hh's SEX_, TFS's names);
// WORLD_TYPE_PVP 0, WORLD_TYPE_NO_PVP 1, WORLD_TYPE_PVP_ENFORCED 2
// (config.hh's NORMAL, NON_PVP, PVP_ENFORCED, TFS's names).
//
// Monsters, raids and monster homes (phase 6; scripting_monsters.cc,
// RegisterMonsterAPI; the data: monsterdata.{hh,cc}).
// The races are data/monsters.toml ([[monster]], one per race), the raids
// data/raids.toml ([[raid]] with its [[raid.wave]]s, in the order the original
// game read mon/*.evt, which is the order in which their start rounds are
// drawn) and the monster homes data/monster_homes.toml (one entry per home, in
// monster.db's order). A race is named everywhere by its key, the name of its
// original file without ".mon" ("demon", "illusion", "slime2"; the names that
// players see are not unique); the loaders turn a key into the race number
// (GetRaceByKey, crmain.hh) and an unknown key is a start error at its
// position. The C++ code keeps the race data (RaceData, crinc.hh), the monster
// AI (TMonster, crnonpl.cc), the raids' scheduling and waves (crmain.cc) and
// the homes; what moves to Lua is the spell loop of TMonster::IdleStimulus.
//
// The monster spells. A race's spells are [[monster.spell]] entries, each a
// table of keys: shape and impact (names of the library's shapes and impacts),
// one_in (the spell is tried when Game.rand() % one_in == 0, at least 1) and
// the keys of its shape and its impact:
//   shape "actor"        effect                    (ActorShapeSpell)
//   shape "victim"       range, animation, effect  (VictimShapeSpell)
//   shape "origin"       radius, effect            (OriginShapeSpell)
//   shape "destination"  range, animation, radius, effect
//                                                  (DestinationShapeSpell)
//   shape "angle"        angle, range, effect      (AngleShapeSpell)
//   impact "damage"      damage_type, damage, variation
//   impact "field"       field_type
//   impact "healing"     amount, variation
//   impact "speed"       percent, variation, duration
//   impact "drunken"     strength, variation, duration
//   impact "strength"    skills, percent, variation, duration
//   impact "outfit"      outfit ({ id, colors = [4] } or { id = 0,
//                        object_type }), duration
//   impact "summon"      race (a key), maximum
// (the original's Shape (p1, ...) -> Impact (q1, ...) : Delay, the parameters
// in that order). The C++ loader knows none of these names: it hands every
// spell to the spell library as a table (TScriptValue), and the library
// checks it.
// The spell library is the script lib/monsters.lua; LoadRaces (crmain.cc)
// loads it (InitMonsterSpells) and calls its global functions:
//   prepareSpell(race, spell)
//       At start, for each spell of each race, in the order of monsters.toml
//       and of the race's spells; race is the race number, spell the entry as
//       a Lua table (numbers, strings, arrays, tables; the keys as in the
//       file). A wrong spell (unknown shape or impact, a missing, unknown or
//       wrong key, an unknown race key) is a Lua error, which the loader
//       reports with the spell's position. The library keeps the prepared
//       spells per race (rule 9: a cache).
//   castSpells(monster, race, target)
//       TMonster::IdleStimulus's spell loop, called at its place in
//       IdleStimulus for a race with spells: monster the Monster, race its
//       race number, target GetCreature(Target) at the call (a Creature, a
//       Player, nil). For each
//       spell in order: rand() % Delay, the flight check
//       (the monster has no master, its hit points are at most the race's
//       FleeThreshold, and random(1, 3) != 1: the spell is skipped), the
//       impact (its ComputeDamage calls; a summon only for a monster without
//       a master), and the shape when the impact is not aggressive or the
//       monster's Target is neither 0 nor its Master (read at that moment),
//       turning to target first for victim, destination and angle (victim
//       and destination only with a target).
//
// Hooks. A race's or a raid's entry may name a script (script =
// "monsters/<key>.lua", "raids/<key>.lua"); none of the converted entries
// does. Its global functions are called where the C++ code reaches the
// point, each only if the script defines it; their results are ignored (they
// cannot refuse or change what the C++ does, but may use the API):
//   onThink(monster)
//       TMonster::IdleStimulus, after the checks of LockToDo, LoggingOut and
//       the lifetime, before anything else.
//   onHealthChange(monster, attacker, damage)
//       TMonster::DamageStimulus, at its start: attacker the creature (nil
//       for none), damage the damage it took.
//   onTarget(monster, target)
//       When the monster's Target becomes another creature (not 0), after
//       it is set: target that creature.
//   onDeath(monster, killer)
//       When the monster dies (its hit points reach 0 in TCreature::Damage),
//       after the C++ death handling at that point: killer the creature that
//       was responsible (an attacker's master for a summon; nil for none).
//       Its corpse does not exist yet (it is made when the dead monster is
//       deleted).
//   onRaid(raid)
//       When a raid's waves are queued (at start for a raid that is
//       scheduled, and by the start_monsterraid command): raid its key.
//   onWave(raid, wave)
//       ProcessMonsterRaids, after a wave's monsters and items are made: raid
//       its key, wave the wave's number in the raid (1 for its first).
// A script's failure in a hook, in castSpells or in an NPC's handler is
// logged as every script error is and the game carries on
// (TScriptCall::runCaught); a result code an API function raises goes to the
// C++ caller.
//
// Monster: a class of objects that hold a monster's creature ID (rule 2);
// its method table falls back to Creature's, a TScriptCreature parameter
// takes a Monster, and a Monster equals only a Monster of the same ID
// (tostring "Monster 123"). A method on an ID that is no monster's logs
// "<binding name>: Monster does not exist." and raises ERROR.
//   monster:getMasterId() -> Master (0 for none); monster:getTargetId() ->
//       Target (0 for none): the raw IDs, also of a creature that is gone.
//   monster:getType() -> MonsterType(Race).
// MonsterType: an object holding a race number; MonsterType(key) or
// MonsterType(number) makes one, nil for an unknown key or a number outside
// 1 to MAX_RACES - 1 or of no race.
//   monsterType:getId() -> the race number; monsterType:getKey() -> its key.
//   monsterType:name() -> Name; monsterType:runHealth() -> FleeThreshold.
// More creature methods:
//   creature:turnTo(target) -> Rotate(TCreature *Target).
// Impact: an object holding an impact of magic.hh, its actor's creature ID
// and its numbers (no pointer: the C++ impact is made at the shape's call).
//   Impact.damage(actor, damageType, power, allowDefense) -> TDamageImpact
//   Impact.field(actor, fieldType) -> TFieldImpact
//   Impact.healing(actor, power) -> THealingImpact
//   Impact.speed(actor, percent, duration) -> TSpeedImpact
//   Impact.drunken(actor, power, duration) -> TDrunkenImpact
//   Impact.strength(actor, skills, percent, duration) -> TStrengthImpact
//   Impact.outfit(actor, outfitId, colors, duration) -> TOutfitImpact, the
//       outfit made as IdleStimulus made it (colors: an array of four
//       numbers 0 to 255, the four bytes; for an object's outfit the object
//       type in the first two, the other two 0).
//   Impact.drunken logs a power above 6 and makes it 6 when the Impact is
//   made, as TDrunkenImpact's constructor did where IdleStimulus made it.
//   Impact.summon(actor, race, maximum) -> TSummonImpact (race a number).
//   impact:isAggressive() -> isAggressive() of that impact (false only for
//       healing). An Impact's tostring is "Impact <kind>" ("Impact damage").
// Magic, the shapes (the impact made at the call, its actor resolved then;
// actor gone: logged and ERROR):
//   Magic.actorShapeSpell(actor, impact, effect)
//   Magic.victimShapeSpell(actor, victim, range, animation, impact, effect)
//   Magic.originShapeSpell(actor, radius, impact, effect)
//   Magic.destinationShapeSpell(actor, victim, range, animation, radius,
//       impact, effect)
//   Magic.angleShapeSpell(actor, angle, range, impact, effect)
// (ActorShapeSpell ... AngleShapeSpell of magic.cc).
//
// Game events (phase 7; scripting_events.cc, RegisterEventAPI). data/events.toml is an ordered
// list of event scripts, [[event]] script = "events/<name>.lua" (the
// delivered file has none). InitEvents (InitAll, after the API is
// registered) loads them in the file's order; a script defines any of the
// handlers below, and for an event every script that defines its handler is
// called, in the file's order. With no script the C++ code does what it did.
// A script's failure is logged and the game carries on (runCaught). The
// handlers other than onSay cannot change what the game does through their
// result, and their callers do not take result codes: a result code one of
// them raises is logged ("<handler> of <script>: result code <n>, ignored.")
// and the next script is called. onSay's result code goes to Talk's caller,
// as a result code of Talk does. The handlers (TFS's names, our arguments):
//   onLogin(player)
//       When a player has entered the game (TConnection::JoinGame, a new
//       player): after the player is in the player list and on the map and
//       its client has the login data. Taking over a player that is still
//       in the game (a reconnect) is no login.
//   onLogout(player)
//       When a player leaves the game (a logout, a lost connection, a kick, a
//       death, the server's shutdown): right before the player is deleted
//       (ProcessCreatures, LogoutAllPlayers), so player resolves.
//   onDeath(player, killer)
//       At the end of TPlayer::Death, for a player that dies (not a god,
//       whose Death only heals): killer the creature responsible (an
//       attacker's master for a summon) when the death comes from
//       TCreature::Damage, nil otherwise. A death from TCreature::Damage has
//       the rest of Damage's handling still to come (the attacker's logout
//       block, the kill statistics, the murder record).
//   onAdvance(player, skill, oldLevel, newLevel)
//       When a skill of the player rises (where the game tells the player
//       "You advanced ..."; not when it falls): skill a SKILL_ constant
//       (SKILL_LEVEL for the level, SKILL_MAGLEVEL, the fighting skills,
//       fishing), oldLevel and newLevel the skill's level before and after
//       as the message counts them (Get(), with temporary changes; the level
//       can rise by more than one at once).
//   onSay(player, text, mode) -> boolean
//       Talk (operate.cc), for a player, right before the spell and command
//       check (CheckForSpell), which Talk skips for a player its spam check
//       mutes: text what the player said, mode a TALKTYPE_ constant. A script
//       that gives true consumes the text: the scripts after it are not
//       called and Talk returns at that point (nothing is cast or said);
//       false or nil goes on. A Talk made while an onSay runs (a script's
//       player:say) calls no onSay.
// Constants (7.72's values of operate.hh's TALK_MODE for the modes a player's
// Talk has, TFS's names where the meaning is TFS's): TALKTYPE_SAY 1,
// TALKTYPE_WHISPER 2, TALKTYPE_YELL 3, TALKTYPE_PRIVATE_TO 4 (a private
// message a player sends), TALKTYPE_CHANNEL_Y 5, TALKTYPE_BROADCAST 9,
// TALKTYPE_CHANNEL_R1 10 (a gamemaster's channel call),
// TALKTYPE_PRIVATE_RED_TO 11, TALKTYPE_CHANNEL_O 12 (a highlighted channel
// call), TALKTYPE_MONSTER_SAY 16, TALKTYPE_MONSTER_YELL 17; the others by
// their 7.72 names: TALKTYPE_GAMEMASTER_REQUEST 6, TALKTYPE_GAMEMASTER_ANSWER
// 7, TALKTYPE_PLAYER_ANSWER 8, TALKTYPE_ANONYMOUS_BROADCAST 13,
// TALKTYPE_ANONYMOUS_CHANNELCALL 14, TALKTYPE_ANONYMOUS_MESSAGE 15.
//
// Reload (phase 8; reload.hh, reload.cc). A gamemaster with the right
// RELOAD_SCRIPTS reloads the scripts and most data files while the game runs
// (reload.hh says what, how and what is refused). The engine keeps one
// current state; a reload builds a second one and makes it current
// (BeginScriptReload), loads every script into it, and then closes the old
// one (CommitScriptReload) or throws the new one away
// (DiscardScriptReload). Script numbers are those of the current state.
// What a script leaves in its globals and module caches is gone after a
// reload (rule 9). The command's binding:
//   Command.reloadScripts(actor) -> boolean
//       The reload command (commands/reload_scripts.lua): asks for a reload
//       by actor (a Player; RequestScriptReload), done by the main loop
//       after the command; false when one is asked already. It checks the
//       command's right (commands.toml's entry) as CheckCommandRight does
//       for the other commands: without it, or for an actor that is no
//       player, it asks nothing and gives true.
// Constants: RIGHT_RELOAD_SCRIPTS (with the other RIGHT_ constants).
// RIGHT_ONLINE_LIST gives the command Online List (104, "aliva mas",
// commands/online_list.lua, ours), which tells the caster who is online.
//
// Commands in Lua (phase 9; scripting_commands.cc, RegisterCommandAPI,
// which RegisterMagicAPI calls). Every command script of
// data/scripts/commands/ (but reload_scripts.lua) does in Lua what its C++
// function of magic.cc did, on the small calls below: the same calls in the
// same order with the same arguments, texts and log lines. What a script
// leaves out: the C++ function's checks that the dispatcher makes
// unreachable (a creature that is nullptr or no player: onSay gets a Player;
// a word that is nullptr or empty: a parameter word is never either). Each
// script checks its command's right where the C++ function did, with its
// onSay argument command (the entry's number), through
// player:hasCommandRight. The shared helpers are the module lib/commands.lua
// (lower, add32, scanCoordinates, identifyPlayer, findRace: its comment says
// what each does). The C++ command functions are gone; the spell functions
// some commands used stay for the spells (MagicClimbing, CleanupField with
// an object, SummonCreature).
// New methods:
//   player:hasCommandRight(command[, check]) -> CheckCommandRight(ID,
//       command, check) of magic.cc (check COMMAND_RIGHT when left out; its
//       log line for a check without a right); command 0 to MAX_SPELLS - 1
//       and check 0 to 3, else the binding's error.
//   creature:getDirection() -> Direction (the way it faces, DIRECTION_).
//   creature:isMonster() -> Type == MONSTER.
//   creature:getMapContainer() -> GetMapContainer(GetCreatureObject()), an
//       Item (the object form: it does not stamp the sector's TimeStamp as
//       Game.getMapContainer(position) does).
//   creature:setSkillMagicDelta(skill, value) ->
//       Skills[skill]->SetMDAct(value).
//   player:getStartPosition() -> startx, starty, startz as a Position.
//   player:setSex(sex) -> Sex = sex (PLAYERSEX_; another value is the
//       binding's error).
//   player:clearVocation() -> ClearProfession().
//   player:addSkillTries(skill, count) -> Skills[skill]->Increase(count) (any
//       skill, SKILL_LEVEL's experience too).
//   player:sendHouseWindow(house, listId) -> ShowGuestList (listId
//       GUEST_LIST) or ShowSubownerList (SUBOWNER_LIST) of houses.cc and
//       SendEditList with its text (one call: the dialog is protocol).
//   player:sendNameDoorWindow(door) -> ShowNameDoor of houses.cc and
//       SendEditList(EDIT_LIST_NAME_DOOR, the door's handle) (one call).
// House: an object holding a house number (rule 2). House(id) makes one for
// 1 to 65535 (nil otherwise; that a house of the number exists is checked by
// the functions that take it, as the C++ functions do); house:getId().
//   house:kickPlayer(player, target) -> KickGuest(HouseID, player, target)
//       of houses.cc (its checks, moves and texts).
// Game:
//   Game.atoi(text) -> atoi(Text) of the C library (glibc on 64 bits:
//       (int)strtol(Text, nullptr, 10), the low 32 bits of the long).
//   Game.identifyPlayer(name, exactMatch, ignoreGamemasters) -> result,
//       player: IdentifyPlayer of crplayer.cc on a copy of name: result 0
//       with the Player, -1 (not online) or -2 (ambiguous) with nil.
//   Game.getPlayers() -> an array of the Players online (PlayerList's
//       order, gamemasters with NO_STATISTICS too; Online List, ours).
//   Game.searchFreeField(position, distance, houseId, jump) -> Position or
//       nil: SearchFreeField(x, y, z, Distance, HouseID, Jump) (houseId 0 to
//       65535; 65535 any field).
//   Game.searchSummonField(position, distance) -> Position or nil:
//       SearchSummonField (its random draws).
//   Game.isOnMap(position) -> IsOnMap(x, y, z).
//   Game.getMarkPosition(name) -> Position or nil: GetMarkPosition.
//   Game.startRaid(name) -> boolean: QueueMonsterRaid(Name, (int)RoundNr)
//       of crmain.cc (its log line, the waves, onRaid).
//   Game.createAtCreature(creatureId, type, value) -> Item (nil for NONE):
//       CreateAtCreature(ID, ObjectType(Type), Value) of operate.cc.
//   Game.announceChangedCreature(creatureId, what) ->
//       AnnounceChangedCreature(ID, What) (CREATURE_HEALTH ...).
//   Game.isThrowPossible(from, to, power) -> ThrowPossible(from, to, Power).
// ItemType:
//   ItemType.fromName(name, movable) -> GetObjectTypeByName(Name, Movable),
//       the type's number (0 for none).
//   ItemType.exists(id) -> ObjectTypeExists(id).
// Position:
//   position:sendMagicEffect(effect) -> GraphicalEffect(x, y, z, Effect).
//   Position, the global, is also the method table of the Positions (a
//   callable userdata: Position(x, y, z) makes one).
// Magic:
//   Magic.getCircle(radius) -> an array of Positions: the offsets of
//       Circle[radius] (circles.toml), z 0, radius 0 to MAX_CIRCLES - 1.
//   Magic.cleanupFieldOn(actor, obj|nil, mana, soul) takes nil for NONE.
// Constants (7.72's values): MESSAGE_STATUS_SMALL (TALK_FAILURE_MESSAGE),
// MESSAGE_EVENT_ADVANCE (TALK_EVENT_MESSAGE) (TFS's names); CONST_SLOT_HEAD
// ... CONST_SLOT_AMMO (objects.hh's BODY_, TFS's names); DIRECTION_NORTH,
// DIRECTION_EAST, DIRECTION_SOUTH, DIRECTION_WEST (the creature's facing);
// CREATURE_HEALTH, CREATURE_LIGHT, CREATURE_OUTFIT, CREATURE_SPEED,
// CREATURE_SKULL, CREATURE_PARTY (operate.hh); COMMAND_RIGHT,
// MOVE_RIGHT_VERTICAL, MOVE_RIGHT_SPEED, MOVE_RIGHT_COORDINATE,
// MOVE_RIGHT_MARK (magicdata.hh); GUEST_LIST 1, SUBOWNER_LIST 2 (the lists
// of sendHouseWindow, the edit lists' numbers the client gets); MAX_AMOUNT
// (the most of a cumulative object); MAX_RACES (crmain.hh: the races are 1
// to MAX_RACES - 1).
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
#include <string>
#include <utility>
#include <vector>

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

// A value of a data file handed to a script as it is (a spell of
// monsters.toml): a number (an int), a text (Latin-1), an array of values or
// a table of keys and values (in the file's order). TScriptCall::addValue
// makes it a Lua value: a number, a string, an array (index 1 first) or a
// table.
struct TScriptValue {
	enum TKind { NUMBER, TEXT, ARRAY, TABLE };
	TKind Kind = NUMBER;
	int Number = 0;
	std::string Text;
	std::vector<TScriptValue> Elements;
	std::vector<std::pair<std::string, TScriptValue>> Fields;
};

// A call of a script's handler, its arguments added in order:
//	TScriptCall Call(Script, "onCastSpell");
//	Call.addPlayer(CreatureID);
//	Call.addString(Words);
//	Call.run();
// Arguments are numbers (ints, uint32_t, all exact in a double), booleans,
// strings (copied; nullptr gives nil), arrays of strings (copied; a new Lua
// table per call), the objects (a Player, a creature as a Creature or a Player,
// an Npc, an Item; a new object per call) and nil. run calls the handler (which
// must exist: RequireHandler at load time; a missing one at run time is a
// non-result error) and gives back nothing; runNumber and runBoolean give its
// first result (a number that is not an integer, or not a number/boolean, is a
// non-result error; nil gives 0 or false). When the handler raises a result
// code, run throws it as RESULT; any other error is logged with error() as
// "Script error in <script name> (<handler>): <message>" and Lua's traceback on
// the following lines (taken where the error was raised, also inside a module
// that require loaded; the handlers call no metamethod of an error value, which
// is described by its type when it is not a string or a number), and run throws
// ERROR. A TScriptCall is run once. It holds no Lua references between its
// construction and run, so it may be built and run anywhere in the main thread.
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
	// An Item object holding the object handle (Object::getObjectID); nil
	// for 0 (NONE).
	void addItem(uint32_t ObjectHandle);
	// An Npc object holding the NPC's creature ID.
	void addNpc(uint32_t CreatureID);
	// A Position object (a new one per call).
	void addPosition(int x, int y, int z);
	// A Monster object holding the monster's creature ID.
	void addMonster(uint32_t CreatureID);
	// The value as a Lua value (TScriptValue, below; copied).
	void addValue(const TScriptValue &Value);
	void run();
	// As run, for the calls whose failure the game survives (phase 6: the
	// stimuli of NPCs and monsters, the hooks): gives false when the handler
	// failed with a non-result error (logged as run logs it; failure() gives
	// its message), true otherwise; a result code is thrown as run throws it.
	bool runCaught();
	// As runCaught, for a handler whose result is a boolean: on success
	// Value is its first result (nil gives false); a result that is not a
	// boolean is a failure, logged as runBoolean words it (Value false).
	bool runCaughtBoolean(bool &Value);
	int runNumber();
	bool runBoolean();
	// After a run that threw ERROR for a non-result error: that error's
	// message as it was logged, without the traceback ("lib/triggers.lua:12:
	// unknown condition 'x'"); nullptr when no handler call failed so since
	// this call began running. Valid until the next failure.
	const char *failure() const;
private:
	struct TArgument;
	int Script;
	const char *Handler;
	TArgument *Arguments;
	int NumberOfArguments;
	int Capacity;
	bool Done;
	unsigned FailuresBefore;
};

// The calls of one event's trigger functions (HandleEvent), made cheap for the
// many rules of an event: the constructor pushes the event's objects user,
// obj1 and obj2 as Items (nil for 0, NONE) once, and every call passes those
// same three values, so a call allocates nothing on the C++ heap and makes no
// Lua object of its own (the message handler is made once at start, the
// event's name an existing string). check calls Handler of Script with
// (Rule, Event, user, obj1, obj2), or (Event, user, obj1, obj2) for Rule 0,
// and gives its result as runBoolean does; run calls it with (Event, user,
// obj1, obj2) and gives nothing. Errors as for TScriptCall::run (a result
// code thrown as RESULT, any other error logged and thrown as ERROR). The
// three values stay on Lua's stack until the destructor, so instances nest
// (an action's event inside a rule's call) and must be destroyed in reverse
// order of construction, as local variables are. Event must stay valid.
// The constructor throws ERROR (logged) when the engine is not active.
class TScriptEvent {
public:
	TScriptEvent(const char *Event, uint32_t User, uint32_t Obj1, uint32_t Obj2);
	~TScriptEvent();
	TScriptEvent(const TScriptEvent &) = delete;
	TScriptEvent &operator=(const TScriptEvent &) = delete;
	bool check(int Script, const char *Handler, int Rule);
	void run(int Script, const char *Handler);
private:
	const char *Event;
	int Base;
};

// Loads the script of each entry of the list [[use]] of items.toml
// (UseEntries, objects.hh, read by InitObjects) and requires its onUse
// (scripting_items.cc); a script named twice is loaded once. Called by InitAll
// after RegisterItemAPI and InitObjects. Throws a const char * (a start
// error) when items.toml has no [[use]] and as LoadScript and RequireHandler
// throw, with the position of the entry's script key.
void InitItemUses();

// Monsters (phase 6; scripting_monsters.cc). InitMonsterSpells loads the
// spell library lib/monsters.lua and requires its prepareSpell and
// castSpells; LoadRaces calls it once, before the first PrepareMonsterSpell.
// PrepareMonsterSpell calls prepareSpell(Race, Spell) and throws a const char
// * "<Where>: <the script's error>" when it fails (a start error).
// CastMonsterSpells calls castSpells(monster, Race, target) (TargetID 0 or a
// creature that is gone: target nil) with runCaught. Each throws ERROR
// (logged) when the engine is not active.
void InitMonsterSpells();
// The reload's end of the spell library (phase 8): while a reload is active
// (ScriptReloadActive) InitMonsterSpells keeps the library's number staged
// and PrepareMonsterSpell uses it; CommitMonsterSpells makes it live (the
// races part's finish; nothing without a staged number),
// DiscardMonsterSpells forgets it.
void CommitMonsterSpells();
void DiscardMonsterSpells();
void PrepareMonsterSpell(int Race, const TScriptValue &Spell, const char *Where);
void CastMonsterSpells(uint32_t MonsterID, int Race, uint32_t TargetID);

// The hooks (above): Script is the race's or raid's script (-1: none, and the
// call does nothing); a handler the script lacks is not called. Each runs with
// runCaught; a result code goes to the caller. A creature argument of 0 is
// nil. They are in scripting.cc (the engine's tests call them).
void MonsterThinkHook(int Script, uint32_t MonsterID);
void MonsterHealthChangeHook(int Script, uint32_t MonsterID, uint32_t AttackerID, int Damage);
void MonsterTargetHook(int Script, uint32_t MonsterID, uint32_t TargetID);
void MonsterDeathHook(int Script, uint32_t MonsterID, uint32_t KillerID);
void RaidHook(int Script, const char *RaidKey);
void RaidWaveHook(int Script, const char *RaidKey, int Wave);

// Game events (phase 7; scripting_events.cc). InitEvents reads events.toml
// (the data folder's; FileName) and loads its scripts; it throws a const char
// * (a start error, with the entry's position) as LoadScript does and for a
// file that is missing or wrong. ExitEvents forgets the scripts (ExitAll,
// before ExitScripting). The Event functions call the handler of every
// script that defines it, in order, each with runCaught; with no event script
// they return at once. EventSay gives true when a script consumed the text. A
// creature argument of 0 is nil.
void InitEvents(const char *FileName);
void ExitEvents();
void EventLogin(uint32_t PlayerID);
void EventLogout(uint32_t PlayerID);
void EventDeath(uint32_t PlayerID, uint32_t KillerID);
void EventAdvance(uint32_t PlayerID, int Skill, int OldLevel, int NewLevel);
bool EventSay(uint32_t PlayerID, const char *Text, int Mode);

// Reload (phase 8). BeginScriptReload makes a new state, set up as
// InitScripting sets one up (the sandbox and the core API; the caller then
// runs the Register*API functions on it) with no script loaded, and makes it
// current; the old state stays as it is. It throws a const char * when Lua
// cannot make the state or a reload has begun already. CommitScriptReload
// closes the old state; DiscardScriptReload closes the new one and makes the
// old one current again. ScriptReloadActive: whether a reload has begun and
// is not committed or discarded. All in the main thread, with no script
// running.
void BeginScriptReload();
void CommitScriptReload();
void DiscardScriptReload();
bool ScriptReloadActive();

#endif
