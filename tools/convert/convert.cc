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

#include <cstdio>
#include <cstring>
#include <string>

#include "config_writer.hh"
#include "tibia_config.hh"
#include "tomlwrite.hh"

static int Usage() {
	fprintf(stderr, "usage: patella-convert config [--base DIR] TIBIA OUTPUT\n");
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

int main(int argc, char **argv) {
	if (argc < 2) {
		return Usage();
	}

	try {
		if (strcmp(argv[1], "config") == 0) {
			return ConvertConfig(argc - 2, argv + 2);
		}
	} catch (const char *Text) {
		fprintf(stderr, "patella-convert: %s\n", Text);
		return 1;
	}
	return Usage();
}
