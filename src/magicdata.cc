// magicdata.cc: the data of the spells (magicdata.hh): the circles, loaded
// from circles.toml (data/circles.toml in the repository, converted from the
// original circles.dat by tools/convert).

#include "tomlfile.hh"

#include <cstdio>
#include <cstring>
#include <string>

#include "config.hh"
#include "magicdata.hh"

TCircle Circle[MAX_CIRCLES];

// The most rows circles.toml may have (and fields in a row): the size
// circles.dat allowed.
const int MAX_CIRCLE_ROWS = 21;

void LoadCircles(const char *FileName) {
	for (int i = 0; i < MAX_CIRCLES; i++) {
		memset(&Circle[i], 0, sizeof(TCircle));
	}

	toml::table File = TomlParseFile(FileName);
	TomlCheckKeys(File, "the top level", {"rows"});
	const toml::array *Rows = TomlGetArray(File, "rows");
	if (Rows == nullptr) {
		TomlError(File.source(), "rows is missing");
	}

	// A square of fields with the center in the middle: as many rows as
	// fields in a row, an odd number.
	int Size = static_cast<int>(Rows->size());
	if (Size % 2 == 0 || Size > MAX_CIRCLE_ROWS) {
		TomlError(Rows->source(), "rows must hold an odd number of rows, at most %d", MAX_CIRCLE_ROWS);
	}

	int Center = Size / 2;
	for (int y = 0; y < Size; y++) {
		const toml::node &RowNode = *Rows->get(static_cast<size_t>(y));
		if (!RowNode.is_string()) {
			TomlError(RowNode.source(), "a row must be a string");
		}

		// The fields, one character each, separated by single spaces.
		const std::string &Row = RowNode.as_string()->get();
		bool Square = static_cast<int>(Row.size()) == 2 * Size - 1;
		for (int i = 1; Square && i < 2 * Size - 1; i += 2) {
			Square = Row[static_cast<size_t>(i)] == ' ';
		}
		if (!Square) {
			TomlError(RowNode.source(), "row %d must be %d fields separated by single spaces, one for each row", y + 1,
					Size);
		}

		for (int x = 0; x < Size; x++) {
			char Field = Row[static_cast<size_t>(2 * x)];
			if (Field == '.') {
				continue;
			}
			if (Field < '0' || Field > '9') {
				TomlError(RowNode.source(), "row %d, field %d is neither a radius 0 to 9 nor \".\"", y + 1, x + 1);
			}

			int Radius = Field - '0';
			if (Circle[Radius].Count == MAX_CIRCLE_FIELDS) {
				TomlError(RowNode.source(), "row %d, field %d: circle %d has more than %d fields", y + 1, x + 1,
						Radius, MAX_CIRCLE_FIELDS);
			}
			Circle[Radius].x[Circle[Radius].Count] = x - Center;
			Circle[Radius].y[Circle[Radius].Count] = y - Center;
			Circle[Radius].Count++;
		}
	}
}

void InitCircles() {
	char FileName[4096];
	sprintf(FileName, "%s/circles.toml", DATAPATH);
	LoadCircles(FileName);
}
