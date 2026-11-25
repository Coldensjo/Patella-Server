// reload.hh: the reload of the scripts and the data while the game runs
// (phase 8; the driver in reload.cc). A gamemaster with the right
// RELOAD_SCRIPTS gives the command Reload Scripts (commands.toml, the script
// commands/reload_scripts.lua, whose binding Command.reloadScripts calls
// RequestScriptReload); AdvanceGame (main.cc) calls ProcessScriptReload after
// ProcessCommand, where no script runs, and does the reload there.
//
// What a reload reads again: every script of the scripts folder (a new Lua
// state, built and loaded completely before it replaces the old one), and
// items.toml (the object types and the [[use]] list), conversion.toml,
// circles.toml, spells.toml, commands.toml, triggers.toml, monsters.toml,
// raids.toml and events.toml. Only at start: config.toml, map.toml,
// houses.toml, npcs.toml (an NPC reloads only its script) and
// monster_homes.toml. All or nothing: when anything fails, everything stays
// as it was before the reload; the errors go to the log and to the
// gamemaster.
//
// What a reload refuses (the part's read throws, with the position): an
// object type that existed and is gone (removed or renumbered); an existing
// type whose instance attribute layout would change (its flags CONTAINER,
// CHEST, CUMULATIVE, KEY, KEYDOOR, LEVELDOOR, QUESTDOOR, RUNE, TEXT,
// LIQUIDCONTAINER, LIQUIDPOOL, TELEPORTABSOLUTE, MAGICFIELD, EXPIRE,
// EXPIRESTOP, WEAROUT) or whose stack order would change (BANK, CLIP,
// BOTTOM, TOP); an existing container type's CAPACITY lowered; a special
// meaning (SPECIAL_) that was defined and is gone or belongs to another
// type; a spell or command number that existed and is gone; a race number
// that existed and is gone, or whose key changed. Anything else may change
// or be added.
//
// What changes for what lives in the game: an object type's flags,
// attributes and names apply to every object of it at once (they are read
// when needed); worn objects' combat values and skill boosts follow at once
// (RefreshCreaturesAfterReload, crmain.hh, for every creature after the
// reload: NotifyChangeInventory's work where something changed); a race's
// data applies to its living monsters from their next action, except what a
// monster copies when it is made (name, outfit, skills: max hit points,
// speed; its inventory, the loot it drops), which only new monsters take;
// waves of a raid already queued keep
// their data, and their hooks follow the raid's key (none after the raid is
// gone); raids are scheduled only at start, the start command uses the
// reloaded raids; a living NPC keeps its data and its conversation and runs
// its reloaded script from its next stimulus.
//
// The steps of a reload, with the parts below in this order (the order of
// the start): items, item uses, events, triggers, magic (circles, spells,
// commands and their scripts), races, raids, NPC scripts.
//   1. For each part: read, then install (so a later part's read sees the
//      earlier parts' new data, as at start).
//   2. BeginScriptReload (scripting.hh): a new Lua state, made current; the
//      Register*API functions of InitAll on it.
//   3. For each part: loadScripts (into the new state; Lua at load time sees
//      the installed data).
//   4. All went well: finish for each part, then CommitScriptReload (the old
//      state is closed), then RefreshCreaturesAfterReload (crmain.hh: the
//      creatures' cached combat slots and skill boosts). One failed (a const char * thrown by read or
//      loadScripts; its text is copied at once): uninstall for each part
//      installed, in reverse order, discard for each part read,
//      DiscardScriptReload when it began.
// Nothing else runs between the steps: a reload that fails cannot be seen
// by the game, but for its log lines. The log gets one summary: who asked,
// the result, each part's summary(), the time it took; the gamemaster gets
// the result line (and the first error).
//
// The start uses the same parts at the places of its old Init calls (the
// part's read, install, loadScripts and finish one after the other, the
// engine current from InitScripting on, no Begin/Commit), with what only the
// start does kept apart (the per-race and per-NPC log lines, scheduling the
// raids, making the NPCs, spawning the homes, ...), so that the start makes
// the same calls in the same order as before: the same random draws, log
// lines, dynamic string numbers and creature IDs.

#ifndef PATELLA_RELOAD_HH
#define PATELLA_RELOAD_HH

#include <cstdint>
#include <string>

// One part of what a reload reads again. Every function but read and
// loadScripts cannot fail (memory a part needs is taken in read). A part is
// used once per reload (or per start), in the order read, install,
// loadScripts, finish, or read [install [loadScripts]], uninstall (if
// installed), discard.
class TReloadPart {
public:
	virtual ~TReloadPart() {}
	// The part's name in the log ("items", "triggers", ...).
	virtual const char *name() const = 0;
	// Reads the part's files into staged data; nothing live changes. Checks
	// what the start checks and what a reload refuses (above; at start
	// nothing exists to compare with). Throws a const char *
	// "<file>:<line>:<column>: <what>" (or "<what>"), valid until the next
	// call of the part.
	virtual void read() = 0;
	// Makes the staged data the live data and keeps the live data aside.
	virtual void install() = 0;
	// Loads and prepares the part's scripts in the current engine (the
	// scripts the installed data names), keeping their numbers staged. Throws
	// a const char * as LoadScript does, with the entry's position.
	virtual void loadScripts() = 0;
	// Makes the staged script numbers live, frees the data kept aside
	// (deleting its dynamic strings) and remaps what refers to the old data.
	virtual void finish() = 0;
	// Makes the data kept aside the live data again (undoes install).
	virtual void uninstall() = 0;
	// Forgets the staged data (and its dynamic strings) and script numbers.
	virtual void discard() = 0;
	// What the last read changed, for the reload's summary
	// ("items.toml: 3 types changed, 1 added").
	virtual std::string summary() const = 0;
};

// The parts (each in the unit of its data).
TReloadPart &ItemsReloadPart();       // objects.cc: items.toml, conversion.toml
TReloadPart &ItemUsesReloadPart();    // scripting_items.cc: the [[use]] scripts
TReloadPart &EventsReloadPart();      // scripting_events.cc: events.toml
TReloadPart &TriggersReloadPart();    // triggers.cc: triggers.toml, the library
TReloadPart &MagicReloadPart();       // magic.cc: circles, spells, commands
TReloadPart &RacesReloadPart();       // crmain.cc: monsters.toml, lib/monsters.lua
TReloadPart &RaidsReloadPart();       // crmain.cc: raids.toml
TReloadPart &NpcScriptsReloadPart();  // crnonpl.cc: the NPCs' scripts
TReloadPart &AdminReloadPart();       // scripting_admin.cc: admin.toml

// Asks for a reload by the player PlayerID (Command.reloadScripts). Gives
// false when one is asked already and not done yet (the second is refused).
bool RequestScriptReload(uint32_t PlayerID);

// Does the reload asked for, if any (AdvanceGame): returns at once without a
// request.
void ProcessScriptReload();

#endif
