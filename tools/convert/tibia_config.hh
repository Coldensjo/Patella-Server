// tibia_config.hh: the reader of the original .tibia config (tibia_config.cc),
// the game's ReadConfig as it was before the game read config.toml.

#ifndef PATELLA_CONVERT_TIBIA_CONFIG_HH
#define PATELLA_CONVERT_TIBIA_CONFIG_HH

#include <string>
#include <vector>

#include "query.hh"

// What the conversion needs besides the game's settings.
struct TTibiaConfigExtras {
	// The query manager passwords as the file writes them; the settings keep
	// them disguised.
	std::string Password[MAX_QUERY_MANAGERS];
	// The entries the game read and dropped (adminport, webdatabase, ...),
	// by their lower-case names, in file order.
	std::vector<std::string> Dropped;
};

// Reads the .tibia file FileName into the game's settings (config.hh) over
// SetConfigDefaults, as the game's old ReadConfig did. Throws a const char *
// as it did: the script reader's errors, "cannot find config-file", and
// DisguisePassword's for a password longer than its key.
void ReadTibiaConfig(const char *FileName, TTibiaConfigExtras &Extras);

#endif
