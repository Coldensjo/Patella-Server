// config_writer.cc: writes the game's settings as config.toml
// (config_writer.hh).

#include "config_writer.hh"

#include <string>

#include "config.hh"
#include "tomlwrite.hh"

// Path as written in config.toml: relative to Base if it lies inside it.
static std::string ConfigPath(const char *Path, const char *Base) {
	std::string Value = Path;
	if (Base == nullptr || Base[0] == 0) {
		return Value;
	}

	std::string Folder = Base;
	while (Folder.size() > 1 && Folder.back() == '/') {
		Folder.pop_back();
	}
	if (Value == Folder) {
		return ".";
	}
	if (Folder != "/" && Value.compare(0, Folder.size() + 1, Folder + "/") == 0
			&& Value.size() > Folder.size() + 1) {
		return Value.substr(Folder.size() + 1);
	}
	return Value;
}

std::string ConfigToToml(const TTibiaConfigExtras &Extras, const char *Base) {
	std::string Text;
	Text += "# config.toml: the settings of the game server, converted from the original\n";
	Text += "# .tibia by patella-convert. The game reads the file given with --config FILE,\n";
	Text += "# or else config.toml in its home folder ($HOME). A key left out keeps its\n";
	Text += "# default; an unknown key or a value of the wrong type is an error.\n";
	Text += "\n";
	Text += "# How much the game prints to its console and log: messages of a level above\n";
	Text += "# this are left out (default 1).\n";
	Text += "debug_level = " + std::to_string(DebugLevel) + "\n";
	Text += "\n";
	Text += "# The beat: how often the game is advanced, in milliseconds (default 200).\n";
	Text += "beat = " + std::to_string(Beat) + "\n";
	Text += "\n";
	Text += "# Key of the shared memory segment through which the operator tools watch\n";
	Text += "# the game (default 0).\n";
	Text += "shm_key = " + std::to_string(SHMKey) + "\n";
	Text += "\n";
	Text += "# The file of the server's RSA private key, for the clients' logins: PEM,\n";
	Text += "# PKCS#1 or PKCS#8, unencrypted, 1024 bits, exponent 65537 (rsakeygen makes\n";
	Text += "# one). A relative path is relative to the folder this file is in (default\n";
	Text += "# \"key.pem\"). Keep the file private.\n";
	Text += "rsa_key = \"key.pem\"\n";
	Text += "\n";
	Text += "[world]\n";
	Text += "# The world's name; the game logs in to the query manager with it (default\n";
	Text += "# empty).\n";
	Text += "name = " + TomlQuote(WorldName) + "\n";
	Text += "# \"public\", or \"private\": the query manager then lets in only invited\n";
	Text += "# accounts (default \"public\").\n";
	Text += std::string("state = ") + (PrivateWorld ? "\"private\"" : "\"public\"") + "\n";
	Text += "\n";
	// The .tibia has nothing of the kind: the defaults.
	Text += "# How long corpses and blood lie before they decay, as multipliers of the times\n";
	Text += "# in items.toml (total_expire_time): 2.0 makes them lie twice as long, 0.5 half\n";
	Text += "# as long. Each is 0.01 to 1000 (default 1.0, the original times), and counts\n";
	Text += "# for every stage of the decay. What already lies when the game starts keeps\n";
	Text += "# the time it had left.\n";
	Text += "[decay]\n";
	Text += "monster_corpse = 1.0  # the corpses of monsters\n";
	Text += "player_corpse = 1.0   # the corpses of players\n";
	Text += "blood = 1.0           # the splash of a hit, the pool of a death, a spilled liquid\n";
	Text += "\n";
	Text += "# Folders. A relative path is relative to the folder this file is in (default:\n";
	Text += "# empty for each).\n";
	Text += "[paths]\n";
	const struct {
		const char *Key;
		const char *Path;
		const char *Comment;
	} Paths[] = {
		{"bin", BINPATH, "reboot-daily, run when the game ends for a reboot"},
		{"map", MAPPATH, "the map sectors (*.sec) the game loads and saves"},
		{"origmap", ORIGMAPPATH, "the original map sectors"},
		{"data", DATAPATH, "items.toml, map.toml and the other data"},
		{"user", USERPATH, "the old player files (NN/<id>.usr) querymanager-seed imports"},
		{"log", LOGPATH, "the log files"},
		{"save", SAVEPATH, "game.pid, swapped-out sectors, patches, unsent player data"},
	};
	const size_t NumberOfPaths = sizeof(Paths) / sizeof(Paths[0]);
	std::string Lines[NumberOfPaths];
	size_t Width = 0;
	for (size_t i = 0; i < NumberOfPaths; i++) {
		Lines[i] = std::string(Paths[i].Key) + " = " + TomlQuote(ConfigPath(Paths[i].Path, Base));
		Width = Lines[i].size() > Width ? Lines[i].size() : Width;
	}
	for (size_t i = 0; i < NumberOfPaths; i++) {
		Text += Lines[i] + std::string(Width - Lines[i].size() + 2, ' ') + "# " + Paths[i].Comment + "\n";
	}
	Text += "\n";
	Text += "# The query managers, tried in this order (at most 10). password: the game's\n";
	Text += "# password at the query manager as the original .tibia writes it; the game\n";
	Text += "# disguises it before sending it, so it can have at most 8 characters.\n";
	for (int i = 0; i < NumberOfQueryManagers; i++) {
		Text += "[[query_manager]]\n";
		Text += "host = " + TomlQuote(QUERY_MANAGER[i].Host) + "\n";
		Text += "port = " + std::to_string(QUERY_MANAGER[i].Port) + "\n";
		Text += "password = " + TomlQuote(Extras.Password[i]) + "\n";
		if (i + 1 < NumberOfQueryManagers) {
			Text += "\n";
		}
	}
	return Text;
}
