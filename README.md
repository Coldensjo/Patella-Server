# Patella-Server

A C++ rebuild of the 2006 Tibia game server (`bin/game`), with its own query
manager (SQLite) and a login server for 7.72 clients.

This repository contains no CipSoft files apart from what `data/` holds: the
original installation's config and object types, converted to TOML, and its
original map (`origmap.zip`, unzipped into the folder `[paths] origmap` names;
start `map/` as a copy of it). To run the server you still need your own copy
of the original installation for the rest (data files, monsters, NPCs).

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

## License

MIT, see [LICENSE](LICENSE). Tibia and its files belong to CipSoft GmbH and are
not covered by it. `extern/tomlplusplus` is toml++ by Mark Gillard, under its own
MIT licence (`extern/tomlplusplus/LICENSE`).
