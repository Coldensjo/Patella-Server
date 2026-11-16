// convert.cc: patella-convert, which turns the original installation's files
// into the TOML files the game reads, with the game's own reader of each
// original file, so that the game ends up with the same values from either.
//
//   patella-convert config [--base DIR] TIBIA OUTPUT
//     The server config: .tibia to config.toml. Paths inside DIR (where the
//     installation sat, /game for the original) are written relative to it;
//     config.toml then belongs in the installation's folder. OUTPUT "-" is
//     standard output. Entries the game read and dropped are named on
//     standard error and left out.
//
//   patella-convert items OBJECTS_SRV OUTPUT
//     The object types: objects.srv to items.toml, which belongs in the
//     installation's data folder. OUTPUT "-" is standard output.
//
//   patella-convert circles CIRCLES_DAT OUTPUT
//     The circles, the area shapes of the spells: circles.dat to circles.toml,
//     which belongs in the installation's data folder. OUTPUT "-" is standard
//     output.
//
//   patella-convert spells OUTPUT
//     The spells: the spell table the game had in its code (its old
//     InitSpells, compiled into patella-convert) to spells.toml, which belongs
//     in the installation's data folder. OUTPUT "-" is standard output. The
//     commands (the spells of the classes al and om) are left out; they go
//     into commands.toml.
//
//   patella-convert commands OUTPUT
//     The commands: the commands of that spell table, with the rights the
//     code checked for them (compiled into patella-convert), to
//     commands.toml, which belongs in the installation's data folder. OUTPUT
//     "-" is standard output.
//
//   patella-convert triggers ITEMS_TOML MOVEUSE_DAT OUTPUT
//     The trigger rules: moveuse.dat to triggers.toml, which belongs in the
//     installation's data folder. The object types come from ITEMS_TOML (the
//     installation's items.toml): the rules may only name types that exist,
//     and the comments give their names. OUTPUT "-" is standard output.
//
//   patella-convert houses HOUSEAREAS_DAT HOUSES_DAT OUTPUT
//     The houses: houseareas.dat (the areas, their prices and depots) and
//     houses.dat (the houses) to houses.toml, which belongs in the
//     installation's data folder. OUTPUT "-" is standard output. owners.dat,
//     the owners the game writes, is not part of it.
//
//   patella-convert map MAP_DAT OUTPUT
//     The map settings (the range of the sectors, the refreshed cylinders,
//     the start positions, the marks and the depots): map.dat, with the
//     mem.dat it includes (the memory settings, found next to it), to
//     map.toml, which belongs in the installation's data folder. OUTPUT "-"
//     is standard output.
//
//   patella-convert conversion CONVERSION_LST OUTPUT
//     The old numbers of the object types (group and number) and their types:
//     conversion.lst to conversion.toml, which belongs in the installation's
//     data folder. OUTPUT "-" is standard output.
//
//   patella-convert npcs NPC_FOLDER DATA_FOLDER
//     The NPCs: every .npc of NPC_FOLDER (the installation's npc/), in the
//     order readdir gives them as the game read them, to DATA_FOLDER/npcs.toml
//     (their data) and DATA_FOLDER/scripts/npcs/<name>.lua (their rules), and
//     every .ndb there (the rule blocks the .npc files include) to
//     DATA_FOLDER/scripts/lib/npcs/<name>.lua. Run it on a folder of the
//     file system the game ran on: its readdir order is the NPCs' order.
//
//   patella-convert monsters MON_FOLDER MONSTER_DB DATA_FOLDER
//     The monster races, the raids and the monster homes: every .mon of
//     MON_FOLDER (the installation's mon/) to DATA_FOLDER/monsters.toml, in
//     the order of the race numbers; every .evt there, in the order readdir
//     gives them as the game read them, to DATA_FOLDER/raids.toml, with their
//     comments; MONSTER_DB (the installation's dat/monster.db) to
//     DATA_FOLDER/monster_homes.toml. What the game read and ignored (an
//     unknown key or flag, a wave's location, ...) is named on standard error.
//     Run it on a folder of the file system the game ran on: its readdir
//     order is the raids' order.
//
//   patella-convert usr-export CONFIG_TOML OUTPUT_FOLDER [CHARACTER_ID...]
//     The player data the query manager has of the world's characters (or of
//     the characters given), as player files (OUTPUT_FOLDER/NN/<id>.usr),
//     written with the game's old writer of them. CONFIG_TOML is the
//     installation's config.toml: its world name, query manager entries and
//     data folder (for the object types). Player files are imported with
//     querymanager-seed --game-root.
//
//   patella-convert usr-check [--read-only] CONFIG_TOML [USER_FOLDER]
//     The proof that the player data survives the way through the query
//     manager, for every player file in USER_FOLDER (default: the config's
//     [paths] user): the file read with the game's old reader must give the
//     same TPlayerData, field by field, as loadPlayerData gives after
//     querymanager-seed imported the file; and the same again after
//     savePlayerData and loadPlayerData (but for the murders older than 30
//     days, which a save drops). It also counts the imported files that the
//     old writer gives back byte for byte. A file the import left out fails
//     the check. It prints only counts. Without --read-only it saves every
//     character's data: use a scratch database. Its temporary file (the old
//     writer's output, a player's data) is in TMPDIR, else /tmp.

#include <cstdio>
#include <cstring>
#include <exception>
#include <string>
#include <vector>

#include "circles_dat.hh"
#include "circles_writer.hh"
#include "config_writer.hh"
#include "conversion_lst.hh"
#include "conversion_writer.hh"
#include "houses_dat.hh"
#include "houses_writer.hh"
#include "items_writer.hh"
#include "map_dat.hh"
#include "map_writer.hh"
#include "mon_files.hh"
#include "monsterdata.hh"
#include "monsters_writer.hh"
#include "npc_files.hh"
#include "npcs_writer.hh"
#include "moveuse_dat.hh"
#include "triggers_writer.hh"
#include "objects.hh"
#include "objects_srv.hh"
#include "commands_reference.hh"
#include "spells_reference.hh"
#include "spells_writer.hh"
#include "tibia_config.hh"
#include "tomlwrite.hh"
#include "usr_commands.hh"

static int Usage() {
	fprintf(stderr, "usage: patella-convert config [--base DIR] TIBIA OUTPUT\n"
			"       patella-convert items OBJECTS_SRV OUTPUT\n"
			"       patella-convert circles CIRCLES_DAT OUTPUT\n"
			"       patella-convert spells OUTPUT\n"
			"       patella-convert commands OUTPUT\n"
			"       patella-convert triggers ITEMS_TOML MOVEUSE_DAT OUTPUT\n"
			"       patella-convert houses HOUSEAREAS_DAT HOUSES_DAT OUTPUT\n"
			"       patella-convert map MAP_DAT OUTPUT\n"
			"       patella-convert conversion CONVERSION_LST OUTPUT\n"
			"       patella-convert npcs NPC_FOLDER DATA_FOLDER\n"
			"       patella-convert monsters MON_FOLDER MONSTER_DB DATA_FOLDER\n"
			"       patella-convert usr-export CONFIG_TOML OUTPUT_FOLDER [CHARACTER_ID...]\n"
			"       patella-convert usr-check [--read-only] CONFIG_TOML [USER_FOLDER]\n");
	return 2;
}

static int ConvertConfig(int argc, char **argv) {
	const char *Base = nullptr;
	const char *Files[2] = {nullptr, nullptr};
	int NumberOfFiles = 0;
	for (int i = 0; i < argc; i++) {
		if (strcmp(argv[i], "--base") == 0 && i + 1 < argc) {
			Base = argv[++i];
		} else if (argv[i][0] == '-' && argv[i][1] != 0) {
			return Usage();
		} else if (NumberOfFiles < 2) {
			Files[NumberOfFiles++] = argv[i];
		} else {
			return Usage();
		}
	}
	if (NumberOfFiles != 2) {
		return Usage();
	}

	TTibiaConfigExtras Extras;
	ReadTibiaConfig(Files[0], Extras);
	for (const std::string &Name : Extras.Dropped) {
		fprintf(stderr, "patella-convert: %s: \"%s\" left out: the game read it and dropped it\n",
				Files[0], Name.c_str());
	}
	WriteTextFile(Files[1], ConfigToToml(Extras, Base));
	return 0;
}

static int ConvertItems(int argc, char **argv) {
	if (argc != 2 || (argv[0][0] == '-' && argv[0][1] != 0)) {
		return Usage();
	}

	TObjectsSrvExtras Extras;
	ReadObjectsSrv(argv[0], Extras);
	WriteTextFile(argv[1], ItemsToToml(Extras));
	return 0;
}

static int ConvertCircles(int argc, char **argv) {
	if (argc != 2 || (argv[0][0] == '-' && argv[0][1] != 0)) {
		return Usage();
	}

	ReadCirclesDat(argv[0]);
	WriteTextFile(argv[1], CirclesToToml());
	return 0;
}

static int ConvertSpells(int argc, char **argv) {
	if (argc != 1 || (argv[0][0] == '-' && argv[0][1] != 0)) {
		return Usage();
	}

	InitReferenceSpells();
	WriteTextFile(argv[0], SpellsToToml());
	return 0;
}

static int ConvertCommands(int argc, char **argv) {
	if (argc != 1 || (argv[0][0] == '-' && argv[0][1] != 0)) {
		return Usage();
	}

	InitReferenceSpells();
	InitReferenceCommandRights();
	WriteTextFile(argv[0], CommandsToToml());
	return 0;
}

static int ConvertTriggers(int argc, char **argv) {
	if (argc != 3 || (argv[0][0] == '-' && argv[0][1] != 0) || (argv[1][0] == '-' && argv[1][1] != 0)) {
		return Usage();
	}

	LoadItems(argv[0]);
	TMoveUseDatExtras Extras;
	ReadMoveUseDat(argv[1], Extras);
	WriteTextFile(argv[2], TriggersToToml(Extras, DefinedTypes(argv[0])));
	return 0;
}

static int ConvertHouses(int argc, char **argv) {
	if (argc != 3 || (argv[0][0] == '-' && argv[0][1] != 0) || (argv[1][0] == '-' && argv[1][1] != 0)) {
		return Usage();
	}

	THousesDatExtras Extras;
	ReadHouseAreasDat(argv[0], Extras);
	ReadHousesDat(argv[1], Extras);
	WriteTextFile(argv[2], HousesToToml(Extras));
	return 0;
}

static int ConvertMap(int argc, char **argv) {
	if (argc != 2 || (argv[0][0] == '-' && argv[0][1] != 0)) {
		return Usage();
	}

	TMapDatExtras Extras;
	ReadMapDat(argv[0], Extras);
	WriteTextFile(argv[1], MapToToml(Extras));
	return 0;
}

static int ConvertConversion(int argc, char **argv) {
	if (argc != 2 || (argv[0][0] == '-' && argv[0][1] != 0)) {
		return Usage();
	}

	ReadConversionLst(argv[0]);
	WriteTextFile(argv[1], ConversionToToml());
	return 0;
}

static int ConvertNpcs(int argc, char **argv) {
	if (argc != 2 || (argv[0][0] == '-' && argv[0][1] != 0) || (argv[1][0] == '-' && argv[1][1] != 0)) {
		return Usage();
	}

	TNpcFolder Folder;
	ReadNpcFolder(argv[0], Folder);
	WriteNpcFolder(Folder, argv[1]);
	return 0;
}

static int ConvertMonsters(int argc, char **argv) {
	if (argc != 3 || (argv[0][0] == '-' && argv[0][1] != 0) || (argv[1][0] == '-' && argv[1][1] != 0)
			|| (argv[2][0] == '-' && argv[2][1] != 0)) {
		return Usage();
	}

	TMonsterFolder Folder;
	ReadMonsterFolder(argv[0], Folder);
	std::vector<TMonsterHomeData> Homes;
	ReadMonsterHomes(argv[1], Folder.Races, Homes, Folder.Ignored);
	for (const std::string &Text : Folder.Ignored) {
		fprintf(stderr, "patella-convert: %s\n", Text.c_str());
	}
	WriteMonsterFiles(Folder, Homes, argv[2]);

	// The files written are read back with the game's loaders, so a value the
	// original game took but the loaders refuse is an error here, not at the
	// game's start.
	std::string Data = argv[2];
	std::vector<TMonsterData> Races;
	std::vector<TRaidData> Raids;
	std::vector<TMonsterHomeData> HomesRead;
	LoadMonsterRaces((Data + "/monsters.toml").c_str(), Races);
	LoadRaids((Data + "/raids.toml").c_str(), Races, Raids);
	LoadMonsterHomes((Data + "/monster_homes.toml").c_str(), Races, HomesRead);
	return 0;
}

int main(int argc, char **argv) {
	if (argc < 2) {
		return Usage();
	}

	try {
		if (strcmp(argv[1], "config") == 0) {
			return ConvertConfig(argc - 2, argv + 2);
		}
		if (strcmp(argv[1], "items") == 0) {
			return ConvertItems(argc - 2, argv + 2);
		}
		if (strcmp(argv[1], "circles") == 0) {
			return ConvertCircles(argc - 2, argv + 2);
		}
		if (strcmp(argv[1], "spells") == 0) {
			return ConvertSpells(argc - 2, argv + 2);
		}
		if (strcmp(argv[1], "commands") == 0) {
			return ConvertCommands(argc - 2, argv + 2);
		}
		if (strcmp(argv[1], "triggers") == 0) {
			return ConvertTriggers(argc - 2, argv + 2);
		}
		if (strcmp(argv[1], "houses") == 0) {
			return ConvertHouses(argc - 2, argv + 2);
		}
		if (strcmp(argv[1], "map") == 0) {
			return ConvertMap(argc - 2, argv + 2);
		}
		if (strcmp(argv[1], "conversion") == 0) {
			return ConvertConversion(argc - 2, argv + 2);
		}
		if (strcmp(argv[1], "npcs") == 0) {
			return ConvertNpcs(argc - 2, argv + 2);
		}
		if (strcmp(argv[1], "monsters") == 0) {
			return ConvertMonsters(argc - 2, argv + 2);
		}
		if (strcmp(argv[1], "usr-export") == 0) {
			return UsrExport(argc - 2, argv + 2);
		}
		if (strcmp(argv[1], "usr-check") == 0) {
			return UsrCheck(argc - 2, argv + 2);
		}
	} catch (const char *Text) {
		fprintf(stderr, "patella-convert: %s\n", Text);
		return 1;
	} catch (const std::exception &e) {
		fprintf(stderr, "patella-convert: %s\n", e.what());
		return 1;
	}
	return Usage();
}
