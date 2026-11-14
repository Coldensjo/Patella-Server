// test_circles.cc: the round trip of the circles. The circles the game ends up
// with from a circles.dat, through the old reader (ReadCirclesDat), and from
// the circles.toml patella-convert makes of it, through the game's
// LoadCircles, must be the same, field by field and in the same order;
// LoadCircles must refuse what is wrong with the file and line.
//
//   convert_test_circles
//     With made-up files in a temporary folder.
//   convert_test_circles --original CIRCLES_DAT CIRCLES_TOML
//     With the original circles.dat and the committed circles.toml:
//     CIRCLES_TOML must be exactly what patella-convert makes of CIRCLES_DAT,
//     and give the same circles; and every change of one field of it must
//     give other circles. Exit code 77 (skipped) when CIRCLES_DAT is not
//     there.
//
// Every load runs in a child process of its own, on a table filled with a
// byte pattern first. The whole table is compared: for each of the 10 circles
// its count and all 32 entries of x and y, used or not.

#include <cstdio>
#include <cstring>
#include <string>
#include <vector>

#include "circles_dat.hh"
#include "circles_writer.hh"
#include "magicdata.hh"
#include "tomlfile.hh"

#include "test_common.hh"

static const unsigned char POISON = 0xA5;

static std::function<void()> OldLoader(const fs::path &File) {
	return [File]() {
		ReadCirclesDat(File.c_str());
	};
}

static std::function<void()> NewLoader(const fs::path &File) {
	return [File]() {
		LoadCircles(File.c_str());
	};
}

// The table as "field\tvalue" lines.
static std::string DumpCircles() {
	std::string Out;
	for (int Radius = 0; Radius < MAX_CIRCLES; Radius++) {
		const TCircle &C = Circle[Radius];
		std::string Prefix = "circle " + std::to_string(Radius) + " ";
		Out += Prefix + "count\t" + std::to_string(C.Count) + "\n";
		for (int j = 0; j < MAX_CIRCLE_FIELDS; j++) {
			Out += Prefix + "field " + std::to_string(j) + "\t" + std::to_string(C.x[j]) + "," + std::to_string(C.y[j])
					+ "\n";
		}
	}
	return Out;
}

static std::string Snapshot(const std::function<void()> &Load) {
	return InChild([&]() {
		memset(static_cast<void *>(Circle), POISON, sizeof(Circle));
		Load();
		return DumpCircles();
	});
}

// The circles.toml patella-convert makes of a circles.dat file.
static std::string Convert(const fs::path &File) {
	return InChild([&]() {
		ReadCirclesDat(File.c_str());
		return CirclesToToml();
	});
}

// Converts a circles.dat text, loads both and compares them. Returns the
// circles.toml text.
static std::string RoundTrip(const fs::path &Folder, const char *Name, const std::string &CirclesDat) {
	fs::path Original = Folder / (std::string(Name) + ".dat");
	fs::path Converted = Folder / (std::string(Name) + ".toml");
	WriteFile(Original, CirclesDat);
	std::string Text = Convert(Original);
	if (IsError(Text)) {
		fprintf(stderr, "%s: conversion: %s\n", Name, Text.c_str());
		Failures++;
		return Text;
	}
	WriteFile(Converted, Text);
	std::string Expected = Snapshot(OldLoader(Original));
	std::string Actual = Snapshot(NewLoader(Converted));
	if (IsError(Expected) || IsError(Actual)) {
		fprintf(stderr, "%s: a load failed:\n%s\n%s\n", Name, Expected.substr(0, 300).c_str(),
				Actual.substr(0, 300).c_str());
		Failures++;
		return Text;
	}
	if (!Same(Expected, Actual, Name, "circles.dat", "circles.toml")) {
		fprintf(stderr, "%s: circles.toml was:\n%s", Name, Text.c_str());
	}
	return Text;
}

// A circles.dat of X by Y fields with the given center, the fields from
// Values (row by row).
static std::string CirclesDat(int X, int Y, int Center, const std::vector<int> &Values) {
	std::string Text = std::to_string(X) + " " + std::to_string(Y) + " " + std::to_string(Center) + "\n";
	for (size_t i = 0; i < Values.size(); i++) {
		Text += std::to_string(Values[i]) + ((i + 1) % static_cast<size_t>(X) == 0 ? "\n" : " ");
	}
	return Text;
}

// The rows of a circles.toml after its "rows = [" line.
static std::string Rows(const std::string &Toml) {
	size_t Start = Toml.find("rows = [\n");
	return Start == std::string::npos ? Toml : Toml.substr(Start);
}

static void TestRoundTrips(const fs::path &Folder) {
	// Diamonds as in the original file: 21 by 21 with the center in the
	// middle, radius r on the fields at distance r, 99 elsewhere.
	std::vector<int> Values;
	for (int y = 0; y < 21; y++) {
		for (int x = 0; x < 21; x++) {
			int Distance = std::abs(x - 10) + std::abs(y - 10);
			Values.push_back(Distance <= 7 ? Distance : 99);
		}
	}
	std::string Toml = RoundTrip(Folder, "diamond", CirclesDat(21, 21, 10, Values));
	CHECK(Toml.find("\trows") == std::string::npos);
	CHECK(Rows(Toml).rfind("rows = [\n\t\". . . . . . . 7 . . . . . . .\",\n", 0) == 0);
	CHECK(Toml.find("\t\"7 6 5 4 3 2 1 0 1 2 3 4 5 6 7\",\n") != std::string::npos);

	// Not square, the center not in the middle, radii 10 and above (which the
	// game leaves out), every radius 0 to 9.
	RoundTrip(Folder, "oblong", CirclesDat(5, 3, 1, {9, 8, 7, 6, 5, 4, 3, 2, 1, 0, 10, 99, 1000, 2147483647, 0}));

	// Center 0: every field lies south-east of the center.
	Toml = RoundTrip(Folder, "corner", CirclesDat(3, 2, 0, {0, 1, 2, 3, 4, 5}));
	CHECK(Rows(Toml) == "rows = [\n\t\". . . . .\",\n\t\". . . . .\",\n\t\". . 0 1 2\",\n\t\". . 3 4 5\",\n"
			"\t\". . . . .\",\n]\n");

	// The quirk: an entry that cannot be read, and every one after it, takes
	// the last radius read; and a file that ends early.
	RoundTrip(Folder, "unreadable", "3 3 1\n1 2 x 0 0 0 0 0 0\n");
	RoundTrip(Folder, "short", "3 3 1\n0 1\n");
	RoundTrip(Folder, "nothing-read", "3 3 1\nx\n");

	// No field in any circle: one row with the center.
	Toml = RoundTrip(Folder, "empty", CirclesDat(3, 3, 1, std::vector<int>(9, 99)));
	CHECK(Rows(Toml) == "rows = [\n\t\".\",\n]\n");

	// A single field; the corners of a 21 by 21 grid; a full circle of 32
	// fields.
	RoundTrip(Folder, "single", CirclesDat(1, 1, 0, {3}));
	Values.assign(21 * 21, 99);
	Values[0] = 1;
	Values[20] = 2;
	Values[21 * 20] = 3;
	Values[21 * 21 - 1] = 4;
	Toml = RoundTrip(Folder, "corners", CirclesDat(21, 21, 10, Values));
	CHECK(Rows(Toml).rfind("rows = [\n\t\"1 . . . . . . . . . . . . . . . . . . . 2\",\n", 0) == 0);
	Values.assign(21 * 21, 99);
	for (int i = 0; i < 32; i++) {
		Values[static_cast<size_t>(i * 13)] = 9;
	}
	RoundTrip(Folder, "full", CirclesDat(21, 21, 10, Values));
}

// What a conversion refuses.
static void TestRefusals(const fs::path &Folder) {
	auto Refused = [&](const char *Name, const std::string &Text, const char *What) {
		WriteFile(Folder / Name, Text);
		std::string Out = Convert(Folder / Name);
		if (Out.find(std::string("error: ") + What) != 0) {
			fprintf(stderr, "%s: expected \"error: %s...\", got \"%s\"\n", Name, What, Out.substr(0, 200).c_str());
			Failures++;
		}
	};
	Refused("negative.dat", CirclesDat(2, 1, 0, {0, -1}), "a radius below 0");
	std::vector<int> Values(21 * 21, 99);
	for (int i = 0; i < 33; i++) {
		Values[static_cast<size_t>(i * 13)] = 9;
	}
	Refused("overfull.dat", CirclesDat(21, 21, 10, Values), "a circle of more than 32 fields");
	Refused("far.dat", CirclesDat(3, 1, 12, {99, 99, 0}), "a field farther than 10");
	Refused("far-south.dat", CirclesDat(1, 3, -9, {99, 99, 0}), "a field farther than 10");
	Refused("wide.dat", CirclesDat(22, 1, 0, std::vector<int>(22, 99)), "Cannot process");
	Refused("zero.dat", "0 3 0\n", "Cannot process");
	CHECK(Convert(Folder / "missing.dat").find("error: Cannot open") == 0);
}

static void TestErrors(const fs::path &Folder) {
	fs::path File = Folder / "errors" / "circles.toml";
	auto Load = [&](const std::string &Text) {
		WriteFile(File, Text);
		return InChild([&]() {
			LoadCircles(File.c_str());
			return std::string();
		});
	};
	auto Expect = [&](const std::string &Text, int Line, const char *What) {
		ExpectError(Load, File, Text, Line, What);
	};
	Expect("circles = 1\n", 1, "unknown key \"circles\" in the top level");
	Expect("", 1, "rows is missing");
	Expect("# nothing\n", 1, "rows is missing");
	Expect("rows = 1\n", 1, "rows must be an array");
	Expect("rows = []\n", 1, "rows must hold an odd number of rows, at most 21");
	Expect("rows = [\".\", \".\"]\n", 1, "rows must hold an odd number of rows, at most 21");
	std::string Big = "rows = [\n";
	for (int i = 0; i < 23; i++) {
		Big += "\t\"" + std::string(45, '.') + "\",\n";
	}
	Expect(Big + "]\n", 1, "rows must hold an odd number of rows, at most 21");
	Expect("rows = [\n\t1,\n]\n", 2, "a row must be a string");
	Expect("rows = [\n\t\". . .\",\n\t\". . .\",\n\t\". .\",\n]\n", 4,
			"row 3 must be 3 fields separated by single spaces, one for each row");
	Expect("rows = [\n\t\". . .\",\n\t\".  . \",\n\t\". . .\",\n]\n", 3, "row 2 must be 3 fields");
	Expect("rows = [\n\t\". . .\",\n\t\". . . .\",\n\t\". . .\",\n]\n", 3, "row 2 must be 3 fields");
	Expect("rows = [\n\t\".,.,.\",\n\t\". . .\",\n\t\". . .\",\n]\n", 2, "row 1 must be 3 fields");
	Expect("rows = [\n\t\". . .\",\n\t\". 0 a\",\n\t\". . .\",\n]\n", 3,
			"row 2, field 3 is neither a radius 0 to 9 nor \".\"");
	Expect("rows = [\n\t\". . .\",\n\t\". - .\",\n\t\". . .\",\n]\n", 3, "row 2, field 2 is neither");
	Expect("rows = [\n\t\". . .\",\n\t\". \xc3\xa4.\",\n\t\". . .\",\n]\n", 3, "row 2 must be 3 fields");
	std::string Full = "rows = [\n";
	for (int y = 0; y < 7; y++) {
		Full += "\t\"" + std::string(y < 4 ? "1 1 1 1 1 1 1" : y == 4 ? "1 1 1 1 1 . ." : ". . . . . . .") + "\",\n";
	}
	Expect(Full + "]\n", 6, "row 5, field 5: circle 1 has more than 32 fields");
	Expect("rows = [\n", 1, "encountered end-of-file");

	// What loads: one row, all ten radii, 21 rows, 32 fields in one circle.
	CHECK(Load("rows = [\".\"]\n") == "");
	CHECK(Load("rows = [\n\t\"0 1 2\",\n\t\"3 4 5\",\n\t\"6 7 8\",\n]\n") == "");
	Big = "rows = [\n";
	for (int i = 0; i < 21; i++) {
		Big += "\t\"" + std::string(i == 10 ? ". . . . . . . . . . 0 . . . . . . . . . ." : ". . . . . . . . . . . . . "
				". . . . . . . .") + "\",\n";
	}
	CHECK(Load(Big + "]\n") == "");
	Full = "rows = [\n";
	for (int y = 0; y < 7; y++) {
		Full += "\t\"" + std::string(y < 4 ? "1 1 1 1 1 1 1" : y == 4 ? "1 1 1 1 . . ." : ". . . . . . .") + "\",\n";
	}
	CHECK(Load(Full + "]\n") == "");

	std::string Missing = InChild([&]() {
		LoadCircles((Folder / "missing.toml").c_str());
		return std::string();
	});
	CHECK(IsError(Missing) && Missing.find((Folder / "missing.toml").string()) != std::string::npos);
}

// Every change of one field of the rows (a digit to the next one, "." to 9,
// a digit to "."), and the swap of two different fields of a row, must give
// other circles. Returns the number of changes made.
static int TestMutations(const fs::path &Folder, const std::string &Toml, const std::string &Expected) {
	size_t Start = Toml.find("rows = [\n");
	std::vector<size_t> Positions;
	for (size_t i = Start; i < Toml.size(); i++) {
		char c = Toml[i];
		if ((c == '.' || (c >= '0' && c <= '9')) && (Toml[i - 1] == '"' || Toml[i - 1] == ' ')) {
			Positions.push_back(i);
		}
	}
	int Changes = 0;
	fs::path File = Folder / "mutated.toml";
	auto Try = [&](const std::string &Mutated, const std::string &What) {
		Changes++;
		WriteFile(File, Mutated);
		std::string Actual = Snapshot(NewLoader(File));
		if (!IsError(Actual) && Differences(Expected, Actual, "", "", "", false) == 0) {
			fprintf(stderr, "mutation not caught: %s\n", What.c_str());
			Failures++;
		}
	};
	for (size_t Position : Positions) {
		char c = Toml[Position];
		std::vector<char> Replacements;
		if (c == '.') {
			Replacements = {'9'};
		} else {
			Replacements = {'.', static_cast<char>(c == '9' ? '0' : c + 1)};
		}
		for (char r : Replacements) {
			std::string Mutated = Toml;
			Mutated[Position] = r;
			Try(Mutated, "field at byte " + std::to_string(Position) + " to " + r);
		}
	}
	for (size_t i = 0; i + 1 < Positions.size(); i++) {
		size_t a = Positions[i], b = Positions[i + 1];
		if (Toml[a] != Toml[b] && Toml.find('\n', a) > b) {
			std::string Mutated = Toml;
			std::swap(Mutated[a], Mutated[b]);
			Try(Mutated, "swap of the fields at bytes " + std::to_string(a) + " and " + std::to_string(b));
		}
	}
	return Changes;
}

static int Original(const char *CirclesDatFile, const char *CirclesToml) {
	if (!fs::is_regular_file(CirclesDatFile)) {
		printf("skipped: no %s\n", CirclesDatFile);
		return 77;
	}

	std::string Text = Convert(CirclesDatFile);
	std::string Committed = ReadFile(CirclesToml);
	if (Committed != Text) {
		fprintf(stderr, "%s is not what patella-convert circles makes of %s (%s)\n", CirclesToml, CirclesDatFile,
				IsError(Text) ? Text.c_str() : "a different text");
		Failures++;
	}

	std::string Expected = Snapshot(OldLoader(CirclesDatFile));
	std::string Actual = Snapshot(NewLoader(CirclesToml));
	if (IsError(Expected) || IsError(Actual)) {
		fprintf(stderr, "a load failed: %s / %s\n", Expected.substr(0, 200).c_str(), Actual.substr(0, 200).c_str());
		return 1;
	}
	if (Same(Expected, Actual, "original", "circles.dat", "circles.toml")) {
		std::map<std::string, std::string> Values = Fields(Actual);
		std::string Counts;
		int Used = 0;
		for (int Radius = 0; Radius < MAX_CIRCLES; Radius++) {
			std::string Count = Values["circle " + std::to_string(Radius) + " count"];
			Counts += (Radius > 0 ? " " : "") + Count;
			Used += atoi(Count.c_str());
		}
		printf("%s and %s give the same circles: %d fields, per radius 0-9: %s; %d values compared\n",
				CirclesDatFile, CirclesToml, Used, Counts.c_str(), static_cast<int>(Values.size()));
	}

	fs::path Folder = TemporaryFolder("convert-test-circles-original");
	int Changes = TestMutations(Folder, Committed, Expected);
	fs::remove_all(Folder);
	printf("%d changes of single fields of %s, each caught\n", Changes, CirclesToml);
	return Failures == 0 ? 0 : 1;
}

int main(int argc, char **argv) {
	if (argc == 4 && strcmp(argv[1], "--original") == 0) {
		int Result = Original(argv[2], argv[3]);
		if (Failures != 0) {
			fprintf(stderr, "%d failures\n", Failures);
		}
		return Result;
	}
	if (argc != 1) {
		fprintf(stderr, "usage: convert_test_circles [--original CIRCLES_DAT CIRCLES_TOML]\n");
		return 2;
	}

	fs::path Folder = TemporaryFolder("convert-test-circles");
	try {
		TestRoundTrips(Folder);
		TestRefusals(Folder);
		TestErrors(Folder);
	} catch (const char *Error) {
		fprintf(stderr, "unexpected error: %s\n", Error);
		Failures++;
	}
	fs::remove_all(Folder);
	if (Failures != 0) {
		fprintf(stderr, "%d failures\n", Failures);
		return 1;
	}
	printf("all passed\n");
	return 0;
}
