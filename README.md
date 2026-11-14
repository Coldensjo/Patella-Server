# Patella-Server

A C++ rebuild of the 2006 Tibia game server (`bin/game`), with its own query
manager (SQLite) and a login server for 7.72 clients.

This repository contains no CipSoft files apart from what `data/` holds: the
original installation's config, converted to TOML. To run the server you need
your own copy of the original installation (data, maps, monsters, NPCs).

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

## License

MIT, see [LICENSE](LICENSE). Tibia and its files belong to CipSoft GmbH and are
not covered by it. `extern/tomlplusplus` is toml++ by Mark Gillard, under its own
MIT licence (`extern/tomlplusplus/LICENSE`).
