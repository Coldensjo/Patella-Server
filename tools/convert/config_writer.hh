// config_writer.hh: writes the game's settings as config.toml
// (config_writer.cc), the file the game's LoadConfig reads.

#ifndef PATELLA_CONVERT_CONFIG_WRITER_HH
#define PATELLA_CONVERT_CONFIG_WRITER_HH

#include <string>

#include "tibia_config.hh"

// The config.toml text of the settings ReadTibiaConfig filled in (config.hh)
// and of its extras. Every key is written, those the .tibia left out with
// their default. A path inside Base (an absolute folder, the one the
// installation sits in) is written relative to it, so that the config works
// from the folder it is placed in; with Base nullptr or empty, and for paths
// outside it, paths are written as they are.
std::string ConfigToToml(const TTibiaConfigExtras &Extras, const char *Base);

#endif
