// circles_dat.cc: the reader of the original circles.dat (circles_dat.hh).
// This is the game's InitCircles as it was before circles.toml, moved here
// unchanged but for two things: the file name is given instead of made from
// DATAPATH, and a radius below 0 or a 33rd field in one circle is refused
// where the old reader wrote past the table.

#include <cstring>
#include <fstream>

#include "magicdata.hh"
#include "utils.hh"

#include "circles_dat.hh"

void ReadCirclesDat(const char *FileName) {
	std::ifstream IN;
	int X, Y, Center;
	for (int i = 0; i < 10; i++) {
		memset(&Circle[i], 0, sizeof(TCircle));
	}

	IN.open(FileName);
	if (IN.fail()) {
		error("InitCircles: Kann Datei %s nicht \xf6" "ffnen.\n", FileName);
		throw "Cannot open \"circles.dat\"";
	}

	IN >> X;
	IN >> Y;
	IN >> Center;
	if (X < 1 || X > 21 || Y < 1 || Y > 21) {
		error("InitCircles: Ung\xfcltiges Dateiformat.\n");
		throw "Cannot process \"circles.dat\"";
	}

	// Quirk: an entry that cannot be read, and every one after it (the stream
	// stays failed), takes the last radius read, or 0 before the first.
	int rad = 0;
	for (int y = 0; y < Y; y++) {
		for (int x = 0; x < X; x++) {
			int Value;
			if (IN >> Value) {
				rad = Value;
			}
			if (rad < 0) {
				throw "a radius below 0, where the old reader wrote before the table";
			}
			if (rad < 10) {
				if (Circle[rad].Count == MAX_CIRCLE_FIELDS) {
					throw "a circle of more than 32 fields, where the old reader wrote past the table";
				}
				Circle[rad].x[Circle[rad].Count] = x - Center;
				Circle[rad].y[Circle[rad].Count] = y - Center;
				Circle[rad].Count++;
			}
		}
	}
}
