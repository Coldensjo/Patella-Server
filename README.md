# Patella-Server

A C++ rebuild of the 2006 Tibia game server (`bin/game`), with its own query
manager (SQLite) and a login server for 7.72 clients.

This repository contains no CipSoft files apart from what `data/` holds: the
original installation's config, object types, circles, trigger rules, houses,
map settings and old object numbers and the original game's spell table,
converted to TOML, and its original map (`origmap.zip`, unzipped into the folder
`[paths] origmap` names; start `map/` as a copy of it). To run the server you
still need your own copy of the original installation for the rest (data files,
monsters, NPCs).

## Build

On Linux with g++, CMake 3.20+ and SQLite 3:

```
cmake -S . -B build
cmake --build build -j
ctest --test-dir build
```

## RSA key

The game server and the login server read their RSA private key from a PEM
file when they start; no key is built into the programs. The game reads the
file `rsa_key` in `config.toml` names (relative to the folder of
`config.toml`, by default `key.pem` there), the login server the file
`--key FILE` names (by default `key.pem` in its working folder). Give both
the same file: the client encrypts its logins for both with one modulus, the
`RsaModulus=` of its config. Without a usable key either server stops at once
with an error that names the file.

The key must be an unencrypted RSA key, PKCS#1 (`-----BEGIN RSA PRIVATE
KEY-----`) or PKCS#8 (`-----BEGIN PRIVATE KEY-----`), with a modulus of 1024
bits and the public exponent 65537, which the 7.x client requires. `rsakeygen`
(`build/keygen/rsakeygen`) makes and reads such files and prints the modulus,
never the primes:

```
rsakeygen key.pem                                # a new key
rsakeygen --modulus key.pem                      # the RsaModulus= of a key file
rsakeygen --from-header RSA_PrivateKey.hh key.pem  # a key in the old header format
```

`--from-header` turns a header with the primes as `RSA_PRIME_P` and
`RSA_PRIME_Q` (the format the original server's key came in) into a key
file. `rsakeygen` creates the file with mode 0600 and never overwrites one.
Keep the key file private: whoever has it can read the passwords clients
send. It does not belong in git (`.gitignore` leaves out `*.pem`).

## Config

The game reads its settings from `config.toml`: the file given with
`--config FILE`, or else `config.toml` in its home folder (`$HOME`).
`data/config.toml` is the original installation's `.tibia` converted with
`patella-convert config --base /game .tibia config.toml` (`tools/convert`);
its paths are relative to the file's folder, so it goes into the folder of the
installation. Keep the installation's `.tibia` there too: the original scripts
and operator tools in its `bin/` still read it (`reboot-daily`, which the game
runs for its daily reboot, and `gamelogs-backup` from `/game/.tibia`;
`highscores`, `deloldusr`, `pl` and the other programs from `$HOME/.tibia`).

`[decay]` is not from the `.tibia`: its `monster_corpse`, `player_corpse` and
`blood` multiply how long the corpses of monsters, the corpses of players and
the pools of blood (the splash of a hit, the pool of a death, a spilled
liquid) lie before they decay, each stage of them. 1.0, the default, gives the
times of `items.toml`; the game reads them at its start.

`[rates]` is not from the `.tibia` either: its whole-number multipliers
`experience`, `magic_level` and `skill` (1 to 1000, default 1) multiply the
experience slain monsters give (5 makes a rat give 25 instead of 5; a slain
player's experience stays as it is), the mana spent that counts for the magic
level, and the tries of the other skills (fighting, shielding, fishing).

`[spawn]` (not from the `.tibia` either) sets how fast the monster homes bring
killed monsters back (`speed`: 4.0 divides the home's regeneration time by 4)
and how many monsters each holds (`amount`: 2.0 doubles it), 0.01 to 1000,
default 1.0. `[spawn.monsters]` gives single races their own values by their
key in `monsters.toml`, e.g. `rat = { speed = 4.0 }`; a value a race leaves
out is the global one. The game reads them at its start; an unknown race key
stops the start.

`[loot]` multiplies the chance of every item monsters carry (`chance`, 0.01 to
1000, default 1.0; at most certain), raids' extra items included;
`[loot.monsters]` gives single races their own, e.g. `rat = { chance = 4.0 }`.

`[death]` sets what a player loses by dying: `penalty` multiplies the loss of
experience, magic level and skills (0 to 10, default 1.0; promotion and
blessings keep their rules), `item_drop` is the percent chance of each worn
item to drop (default 10) and `container_drop` whether the backpack always
drops (default true).

`[regeneration]` multiplies the health (`hp`) and mana (`mana`) a fed player
regains each time, whole numbers 1 to 1000, default 1; a bed's offline
regeneration too. The food lasts as long as before.

`[soul]` sets the soul point regeneration in seconds: after a kill worth at
least the player's level in experience, one point per `interval` (120) or
`promoted_interval` (15) for `duration` (240); in a bed one per
`offline_interval` (900) offline.

`[pvp]` sets player killing: `fight_lock` (60) and `kill_lock` (900), the
seconds a fight or a kill of a player keeps a player from logging out (and out
of protection zones), `red_skull_days` (30), and the unjustified kills within a
day, a week and 30 days that give a red skull (`red_skull`, 3/5/10) or a
banishment (`banishment`, 6/10/20; each at most 20, the kills a player's data
keeps).

`[idle]` `minutes` (1 to 1440, default 15) is how long a player may do nothing
before being warned, and logged out a minute later.

`[vocations]` sets what a player gains per level by vocation (`none`,
`knight`, `paladin`, `sorcerer`, `druid`; promoted ones count as theirs):
`hp`, `mana` and `capacity`, e.g. `knight = { hp = 20 }`. A value given counts
from each character's next level on (past levels stay); one not given stays
what the character got with its vocation. `data/config.toml` lists the
original values, commented out.

## Object types

The game reads the object types (items, grounds, walls, corpses, ...) from
`items.toml` in its data folder (`[paths] data`). `data/items.toml` is the
original installation's `dat/objects.srv` converted with
`patella-convert items objects.srv items.toml`; copy it into the
installation's data folder. The comments at its top explain the fields. Keep
`objects.srv` there too: the original operator tools (`highscores`,
`deloldusr`, `mgrep`, `ugrep`, `tempel`) still read it. What happens when an
object is used is a script in `data/scripts/items/`: the list `[[use]]` at the
end of `items.toml` names a flag and a script for each built-in use, and the
game runs the global function `onUse` of the first script in the list whose
flag the used object has (one that gives false hands the use on to the next).
The converter writes that list the same for every `objects.srv`, in the order
the game's code had.

## Spells, commands and circles

The game reads its spells (name, words, level, mana, soul points, flags, the
rune a rune spell makes and the script) from `spells.toml`, the gamemasters'
commands (the spells of the classes al and om: name, words, the rights each
needs and the script) from `commands.toml`, and the circles, the area shapes
the spells hit, from `circles.toml`, all in its data folder. What a spell or
command does is its script, a Lua file in the scripts folder (see Scripts)
that its entry names: `spells/<name>.lua` for a spell cast at once (ex, ut)
and `runes/<name>.lua` for a rune spell (ad), whose global function
`onCastSpell` runs when the spell is cast or its rune is used;
`commands/<name>.lua` for a command, whose global function `onSay` runs when
it is given. The scripts in
`data/scripts/spells`, `runes` and `commands` are named after the cases of the
code they replaced (`SPELL_LIGHT_HEALING` is `spells/light_healing.lua`).
`data/spells.toml` and `data/commands.toml` are the spell table the game had
in its code, with the rights its code checked for the commands and these
scripts, written with `patella-convert spells spells.toml` and
`patella-convert commands commands.toml`; `data/circles.toml` is the original
installation's `dat/circles.dat` converted with
`patella-convert circles circles.dat circles.toml`. Copy all three into the
installation's data folder. The comments at the top of each file explain the
fields.

## Scripts

The game runs Lua scripts with LuaJIT, from the folder `scripts` in its data
folder: what the spells, runes and commands do, the trigger rules' conditions
and actions, the built-in uses of objects and what the NPCs say and do.
`data/scripts/` holds them, `spells/` for the spells cast at once, `runes/` for
the rune spells, `commands/` for the commands, `items/` for the uses of
objects, `npcs/` for the NPCs and `lib/` for the trigger library, the NPC rule
matcher (`lib/npc.lua`) and the NPCs' shared rule blocks (`lib/npcs/`); copy it
into the installation's data folder. The game loads the scripts
`spells.toml`, `commands.toml`, `items.toml`, `npcs.toml` and the trigger rules
name when it starts, and stops with an error that names the file, line and
column when one does not load. A script's handlers are global functions,
`onCastSpell` for the spells and runes, `onSay` for the commands, `onUse` for
the uses of objects and `onCreatureSay` and `onCreatureDisappear` for the NPCs,
and the script returns nothing. The Lua API uses the names and style of The Forgotten Server (TFS):
camelCase functions and methods, PascalCase tables and classes, upper-case
constants, with the values of 7.72, so scripts written for TFS need fewer
changes; the handlers' arguments, the `Magic` and `Command` functions and the
vocation numbers are Patella's own. `src/scripting.hh` lists the API; the
rules of the engine, which every script keeps, are in the comment at its top.

## Trigger rules

The game reads the trigger rules, what happens when an object is used, used on
something, moved, or meets or leaves another object on a field (beds, levers,
doors, fishing, teleporters, ...), from `triggers.toml` in its data folder.
`data/triggers.toml` is the original installation's `dat/moveuse.dat` converted
with `patella-convert triggers items.toml moveuse.dat triggers.toml` (the object
types come from `items.toml`); copy it into the installation's data folder. The
comments at its top explain the fields. The order of the rules matters: for each
event the game runs the first rule whose conditions hold. The names in the
conditions and actions are the functions of the trigger library,
`data/scripts/lib/triggers.lua` and `lib/triggers_actions.lua`, which the game
hands each rule at start; a rule may instead name a script of its own,
`script = "triggers/<name>.lua"`, whose `onCheck` and `onRun` take the place of
its conditions and actions.

## NPCs

The game reads the NPCs (name, sex, race, outfit, home, how far they walk and
how fast, and the script of each) from `npcs.toml` in its data folder, and
makes them in the order of the file, which gives them their creature IDs.
What an NPC says and does is its script, `data/scripts/npcs/<name>.lua`: the
rules of the original NPC file (the words the player's text must contain, the
properties and values a rule tests, and the texts and actions of the rule that
wins), which `lib/npc.lua` matches and runs as the original game did; the
original's shared rule blocks (`.ndb` files: banks, post offices, traders) are
the modules `lib/npcs/<name>.lua`, put in place where an NPC included them.
`data/npcs.toml` and those scripts are the original installation's `npc/`
folder converted with `patella-convert npcs npc data` (it writes
`data/npcs.toml`, `data/scripts/npcs/` and `data/scripts/lib/npcs/`). The
converter reads the folder in the order the file system gives, as the game
did; the committed files were converted from the original installation on
WSL's ext4, so their order is the one the original game has there. Copy them
into the installation's data folder. The comments at the top of `npcs.toml`,
`lib/npc.lua` and `src/scripting.hh` (section "NPCs") explain the fields, the
rules and the API.

## Houses

The game reads the houses (name, description, rent offset, guild hall or not,
exit, center and fields) and the house areas they lie in (price per field, town
depot) from `houses.toml` in its data folder. `data/houses.toml` is the
original installation's `dat/houseareas.dat` and `dat/houses.dat` converted
with `patella-convert houses houseareas.dat houses.dat houses.toml`; copy it
into the installation's data folder. The comments at its top explain the
fields. Who owns or rents a house stays in `owners.dat` in the data folder,
which the game reads at start-up and writes; it is not part of this
repository.

## Map settings

The game reads the map settings (the range of the sectors, how many cylinders
it refreshes each minute, the room for the objects, the start positions, the
marks gamemasters teleport to and the towns' depots) from `map.toml` in its
data folder. `data/map.toml` is the original installation's `dat/map.dat` and
the `dat/mem.dat` it includes converted with
`patella-convert map map.dat map.toml` (`mem.dat` is read from the folder of
`map.dat`); copy it into the installation's data folder. The comments at its
top explain the fields. Keep `map.dat` and `mem.dat` there too: the original
operator tools (`highscores`, `deloldusr`, `mgrep`, `ugrep`, `tempel`) still
read them.

## Old object numbers

Before the object types had numbers of their own, an object was known by a
group and a number; the code and `spells.toml` still name the runes, food,
arrows and bolts the spells make that way. The game reads the type each old
group and number stands for from `conversion.toml` in its data folder.
`data/conversion.toml` is the original installation's `dat/conversion.lst`
converted with `patella-convert conversion conversion.lst conversion.toml`;
copy it into the installation's data folder. The comments at its top explain
the layout. Keep `conversion.lst` there too: the original operator tools
(`highscores`, `deloldusr`, `mgrep`, `ugrep`, `tempel`) still read it.

## Player data

The game keeps what it knows of a character between logins (race,
profession, outfits, last login and logout, start and current position,
player-killer end, skills, spells, quest values, murders, inventory and the
nine depots) in the query manager's database, not in files. It loads a
character's data with the request loadPlayerData and saves it with
savePlayerData; the record they carry is described in
`querymanager/src/playerdata.hh`, the tables (`player_data` with one row per
character, `player_skills`, `player_spells`, `player_quest_values`,
`player_murders` and `player_depots`) in `querymanager/src/sqlite_storage.cc`.
The inventory and each depot are stored as the item text the player files had
for them. A character the database has no data of starts as a new character.
A database of an older schema is brought up to date when the query manager
opens it.

The original installation's player files (`usr/NN/<id>.usr`) are imported
with the characters by `querymanager-seed --game-root`, which reads them with
the game's old reader of them; it needs the object types (`items.toml`, or the
original `objects.srv`) in the installation's data folder. `--no-player-data`
leaves the player data out, and data the database has already is kept.
An existing database (schema version 1) that a game with player files used
gets the new tables when the query manager opens it, but no player data: run
`querymanager-seed --game-root` on that installation once more before the
new game starts. Otherwise every character starts as a new one, and the
game's save of that new character is the data a later import keeps.
`patella-convert usr-export CONFIG_TOML FOLDER [ID...]` writes the database's
player data as player files again. `patella-convert usr-check CONFIG_TOML`
proves that every player file comes back the same from the database, field
by field; it saves each file's data into the database (unless
`--read-only`), so run it on a scratch database only.
`tools/playerdata_check.py` runs it on a copy of an installation. The game
itself no longer reads or writes player files; `[paths] user` in `config.toml`
is read only by `querymanager-seed`, to find the files to import.

When a save does not reach the query manager, the game keeps the data in the
save folder (`[paths] save`) as `playerdata-<id>.rec` and logs where. It sends
that file before it loads the character again and at its next start; the next
successful save makes it obsolete. A query manager that takes the connection
but does not answer makes a save wait 30 seconds; after such a save the game
keeps its saves in files at once for a minute, so that saving many characters
(at shutdown, say) does not take hours. The query manager does not store a
save the game has given up on and closed the connection of, so such a late save
never replaces newer data saved since. While the file cannot be sent, the
character cannot log in: a load that fails refuses the login, so that a
character never starts as new because its data could not be read.

## Play

`tools/play.sh` starts the query manager, the login server (port 7171) and
the game (port 7172) for a 7.72 client; Ctrl-C stops them, the game first.
On first use it makes a play copy of your original installation
(`PLAY_INSTALL`, a tar or a folder) laid out with this repository's `data/`,
and a database with the installation's characters and player data. The copy
gets the key file `key.pem`, which the game and the login server read:
`PLAY_KEY` (an existing key file) when it is set, else a new key from the
build's `rsakeygen`.
`tools/play.sh seed --add-account N --password P --new-character NAME --world
Zanera` adds an account, `tools/play.sh modulus` prints the `RsaModulus=` of
the copy's key, which the client needs, and `tools/play.sh reset` deletes the
copy, its key and the database. The comments at the top of the script list its
settings.

## License

MIT, see [LICENSE](LICENSE). Tibia and its files belong to CipSoft GmbH and are
not covered by it. `extern/tomlplusplus` is toml++ by Mark Gillard, under its own
MIT licence (`extern/tomlplusplus/LICENSE`). `extern/luajit` is LuaJIT by Mike
Pall, under its MIT licence (`extern/luajit/COPYRIGHT`), and `extern/sol2` is
sol2 by Rapptz, ThePhD and contributors, under its MIT licence
(`extern/sol2/LICENSE.txt`).
