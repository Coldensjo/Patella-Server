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
//     in the installation's data folder. OUTPUT "-" is standard output.
//
//   patella-convert moveuse ITEMS_TOML MOVEUSE_DAT OUTPUT
//     The move and use rules: moveuse.dat to moveuse.toml, which belongs in
//     the installation's data folder. The object types come from ITEMS_TOML
//     (the installation's items.toml): the rules may only name types that
//     exist, and the comments give their names. OUTPUT "-" is standard
//     output.

#include <cstdio>
#include <cstring>
#include <string>

#include "circles_dat.hh"
#include "circles_writer.hh"
#include "config_writer.hh"
#include "items_writer.hh"
#include "moveuse_dat.hh"
#include "moveuse_writer.hh"
#include "objects.hh"
#include "objects_srv.hh"
#include "spells_reference.hh"
#include "spells_writer.hh"
#include "tibia_config.hh"
#include "tomlwrite.hh"

static int Usage() {
	fprintf(stderr, "usage: patella-convert config [--base DIR] TIBIA OUTPUT\n"
			"       patella-convert items OBJECTS_SRV OUTPUT\n"
			"       patella-convert circles CIRCLES_DAT OUTPUT\n"
			"       patella-convert spells OUTPUT\n"
			"       patella-convert moveuse ITEMS_TOML MOVEUSE_DAT OUTPUT\n");
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

static int ConvertMoveUse(int argc, char **argv) {
	if (argc != 3 || (argv[0][0] == '-' && argv[0][1] != 0) || (argv[1][0] == '-' && argv[1][1] != 0)) {
		return Usage();
	}

	LoadItems(argv[0]);
	TMoveUseDatExtras Extras;
	ReadMoveUseDat(argv[1], Extras);
	WriteTextFile(argv[2], MoveUseToToml(Extras, DefinedTypes(argv[0])));
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
		if (strcmp(argv[1], "moveuse") == 0) {
			return ConvertMoveUse(argc - 2, argv + 2);
		}
	} catch (const char *Text) {
		fprintf(stderr, "patella-convert: %s\n", Text);
		return 1;
	}
	return Usage();
}
