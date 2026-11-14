// circles_writer.cc: writes the game's circles as circles.toml
// (circles_writer.hh).

#include "circles_writer.hh"

#include <algorithm>
#include <cstdlib>
#include <vector>

#include "magicdata.hh"

// How far a field of circles.toml may lie from the center: its 21 rows.
const int MAX_DISTANCE = 10;

static std::string Header() {
	std::string Text;
	Text += "# circles.toml: the circles of the game server, the area shapes its spells and\n";
	Text += "# the monsters' spells hit, converted from the original circles.dat by\n";
	Text += "# patella-convert. The game reads circles.toml in its data folder ([paths]\n";
	Text += "# data of config.toml). Anything but rows, or a row that breaks the rules\n";
	Text += "# below, is an error.\n";
	Text += "#\n";
	Text += "# rows is a square of fields, as many rows as fields in a row (an odd number,\n";
	Text += "# at most 21), whose middle field is the center: the caster, or the field a\n";
	Text += "# spell is aimed at. The rows go from north to south, the fields of a row,\n";
	Text += "# separated by single spaces, from west to east. A digit puts the field into\n";
	Text += "# the circle of that radius (0 to 9, each of at most 32 fields), \".\" into\n";
	Text += "# none. A spell of radius N hits the circles up to N, one after the other,\n";
	Text += "# and the fields of a circle in the order they stand here, row by row; that\n";
	Text += "# order decides which creature is hit first.\n";
	return Text;
}

std::string CirclesToToml() {
	int Half = 0;
	std::vector<int> Grid((2 * MAX_DISTANCE + 1) * (2 * MAX_DISTANCE + 1), -1);
	for (int Radius = 0; Radius < MAX_CIRCLES; Radius++) {
		const TCircle &C = Circle[Radius];
		if (C.Count < 0 || C.Count > MAX_CIRCLE_FIELDS) {
			throw "a circle with a field count outside 0 to 32";
		}
		for (int j = 0; j < C.Count; j++) {
			if (std::abs(C.x[j]) > MAX_DISTANCE || std::abs(C.y[j]) > MAX_DISTANCE) {
				throw "a field farther than 10 from the center, which circles.toml cannot hold";
			}
			if (j > 0 && (C.y[j] < C.y[j - 1] || (C.y[j] == C.y[j - 1] && C.x[j] <= C.x[j - 1]))) {
				throw "a circle whose fields are not in the order of the rows, which circles.toml cannot hold";
			}
			int &Field = Grid[(C.y[j] + MAX_DISTANCE) * (2 * MAX_DISTANCE + 1) + C.x[j] + MAX_DISTANCE];
			if (Field != -1) {
				throw "a field in two circles, which circles.toml cannot hold";
			}
			Field = Radius;
			Half = std::max(Half, std::max(std::abs(C.x[j]), std::abs(C.y[j])));
		}
	}

	std::string Text = Header();
	Text += "\nrows = [\n";
	for (int y = -Half; y <= Half; y++) {
		Text += "\t\"";
		for (int x = -Half; x <= Half; x++) {
			int Field = Grid[(y + MAX_DISTANCE) * (2 * MAX_DISTANCE + 1) + x + MAX_DISTANCE];
			Text += (x > -Half ? " " : "");
			Text += Field == -1 ? '.' : static_cast<char>('0' + Field);
		}
		Text += "\",\n";
	}
	Text += "]\n";
	return Text;
}
