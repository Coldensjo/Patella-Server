// circles_dat.hh: the reader of the original circles.dat (circles_dat.cc), the
// game's InitCircles as it was before the game read circles.toml.

#ifndef PATELLA_CONVERT_CIRCLES_DAT_HH
#define PATELLA_CONVERT_CIRCLES_DAT_HH

// Reads the circles.dat file FileName into the game's Circle (magicdata.hh),
// as the game's old InitCircles did. Throws a const char * as it did ("Cannot
// open ...", "Cannot process ..."), and for a radius below 0 or a 33rd field
// in one circle, where the old reader wrote past the table.
void ReadCirclesDat(const char *FileName);

#endif
