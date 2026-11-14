# Patella-Server

A C++ rebuild of the 2006 Tibia game server (`bin/game`), with its own query
manager (SQLite) and a login server for 7.72 clients.

This repository contains no CipSoft files apart from what `data/` holds: the
original installation's config, object types, circles, move and use rules,
houses, map settings and old object numbers and the original game's spell
table, converted to TOML, and its original map (`origmap.zip`, unzipped into
the folder `[paths] origmap` names; start `map/` as a copy of it). To run the
server you still need your own copy of the original installation for the rest
(data files, monsters, NPCs).

## Build

On Linux with g++, CMake 3.20+ and SQLite 3:

```
cmake -S . -B build
cmake --build build -j
ctest --test-dir build
```

The first build makes a new RSA key and prints the modulus clients need
(`RsaModulus=...`).

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

## Object types

The game reads the object types (items, grounds, walls, corpses, ...) from
`items.toml` in its data folder (`[paths] data`). `data/items.toml` is the
original installation's `dat/objects.srv` converted with
`patella-convert items objects.srv items.toml`; copy it into the
installation's data folder. The comments at its top explain the fields. Keep
`objects.srv` there too: the original operator tools (`highscores`,
`deloldusr`, `mgrep`, `ugrep`, `tempel`) still read it.

## Spells and circles

The game reads its spells (name, words, level, mana, soul points, flags and the
rune a rune spell makes) from `spells.toml` and the circles, the area shapes
the spells hit, from `circles.toml`, both in its data folder. What a spell does
stays in the code, which knows the spells by their numbers. `data/spells.toml`
is the spell table the game had in its code, written with
`patella-convert spells spells.toml`; `data/circles.toml` is the original
installation's `dat/circles.dat` converted with
`patella-convert circles circles.dat circles.toml`. Copy both into the
installation's data folder. The comments at the top of each file explain the
fields.

## Move and use rules

The game reads the move and use rules, what happens when an object is used,
used on something, moved, or meets or leaves another object on a field (beds,
levers, doors, fishing, teleporters, ...), from `moveuse.toml` in its data
folder. `data/moveuse.toml` is the original installation's `dat/moveuse.dat`
converted with `patella-convert moveuse items.toml moveuse.dat moveuse.toml`
(the object types come from `items.toml`); copy it into the installation's data
folder. The comments at its top explain the fields. The order of the rules
matters: for each event the game runs the first rule whose conditions hold.

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

## License

MIT, see [LICENSE](LICENSE). Tibia and its files belong to CipSoft GmbH and are
not covered by it. `extern/tomlplusplus` is toml++ by Mark Gillard, under its own
MIT licence (`extern/tomlplusplus/LICENSE`).
