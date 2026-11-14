// circles_writer.hh: writes the game's circles as circles.toml
// (circles_writer.cc).

#ifndef PATELLA_CONVERT_CIRCLES_WRITER_HH
#define PATELLA_CONVERT_CIRCLES_WRITER_HH

#include <string>

// The text of circles.toml for the circles as the game's Circle holds them:
// the smallest square around the center that holds every field. Throws a
// const char * for what circles.toml cannot hold: a field farther than 10
// from the center, a field in two circles or twice in one, or a circle whose
// fields are not in the order the rows give (north to south, each row west to
// east).
std::string CirclesToToml();

#endif
