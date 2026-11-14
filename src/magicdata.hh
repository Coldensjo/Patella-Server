// magicdata.hh: the data of the spells (magicdata.cc): the circles, the area
// shapes the spells and the monsters' spells hit, loaded from circles.toml.
// magic.cc holds what the spells do.

#ifndef PATELLA_MAGICDATA_HH
#define PATELLA_MAGICDATA_HH

// The circles a shape can have (Circle): radius 0 to 9.
const int MAX_CIRCLES = 10;

// The fields one circle can hold (TCircle's x and y).
const int MAX_CIRCLE_FIELDS = 32;

// One circle: the offsets of its fields from the center, in the order the
// shapes visit them (row by row from north to south, each row from west to
// east).
struct TCircle {
	int x[MAX_CIRCLE_FIELDS];
	int y[MAX_CIRCLE_FIELDS];
	int Count;
};

// Fills Circle from the circles.toml file FileName. Throws a const char *
// "<file>:<line>:<column>: <what>" for what is wrong with it.
void LoadCircles(const char *FileName);

// LoadCircles of circles.toml in the data folder (DATAPATH).
void InitCircles();

extern TCircle Circle[MAX_CIRCLES];

#endif
