// test_conversion.cc: the round trip of the conversion of old object numbers.
// The table the game ends up with from a conversion.lst, through the old
// reader (ReadConversionLst), and from the conversion.toml patella-convert
// makes of it, through the game's LoadConversions, must be the same, entry by
// entry; LoadConversions must refuse what is wrong with the file and line.
//
//   convert_test_conversion
//     With made-up files in a temporary folder.
//   convert_test_conversion --original CONVERSION_LST CONVERSION_TOML
//     With the original conversion.lst and the committed conversion.toml:
//     CONVERSION_TOML must be exactly what patella-convert makes of
//     CONVERSION_LST, and give the same table; and every change of one entry
//     of it must give another table, or an error. Exit code 77 (skipped) when
//     CONVERSION_LST is not there.
//
// Every load runs in a child process of its own, on a table filled with a
// byte pattern first. The whole table is compared: all 65536 entries of
// NewType (256 groups of 256 numbers), the ones without a type (-1)
// included.

#include <cstdio>
#include <cstring>
#include <string>
#include <vector>

#include "conversion_lst.hh"
#include "conversion_writer.hh"
#include "objects.hh"
#include "tomlfile.hh"

#include "test_common.hh"

static const unsigned char POISON = 0xA5;

static std::function<void()> OldLoader(const fs::path &File) {
	return [File]() {
		ReadConversionLst(File.c_str());
	};
}

static std::function<void()> NewLoader(const fs::path &File) {
	return [File]() {
		LoadConversions(File.c_str());
	};
}

// The table as "field\tvalue" lines: the entries with a type and their
// count. An entry without one (-1) is left out, so that two tables differ in
// a field exactly where one has a type the other has not or another one.
static std::string DumpTable() {
	std::string Out;
	int Count = 0;
	for (int Place = 0; Place < 65536; Place++) {
		if (NewType[Place] != -1) {
			Out += std::to_string(Place / 256) + "/" + std::to_string(Place % 256) + "\t"
					+ std::to_string(NewType[Place]) + "\n";
			Count++;
		}
	}
	return "entries\t" + std::to_string(Count) + "\n" + Out;
}

static std::string Snapshot(const std::function<void()> &Load) {
	return InChild([&]() {
		memset(NewType, POISON, sizeof(NewType));
		Load();
		return DumpTable();
	});
}

// The conversion.toml patella-convert makes of a conversion.lst file.
static std::string Convert(const fs::path &File) {
	return InChild([&]() {
		ReadConversionLst(File.c_str());
		return ConversionToToml();
	});
}

// Converts a conversion.lst text, loads both and compares them. Returns the
// conversion.toml text.
static std::string RoundTrip(const fs::path &Folder, const char *Name, const std::string &ConversionLst) {
	fs::path Original = Folder / (std::string(Name) + ".lst");
	fs::path Converted = Folder / (std::string(Name) + ".toml");
	WriteFile(Original, ConversionLst);
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
	if (!Same(Expected, Actual, Name, "conversion.lst", "conversion.toml")) {
		fprintf(stderr, "%s: conversion.toml was:\n%s", Name, Text.c_str());
	}
	return Text;
}

// The part of a conversion.toml after its comments.
static std::string Body(const std::string &Toml) {
	size_t Start = Toml.find("\n[group.");
	return Start == std::string::npos ? "" : Toml.substr(Start + 1);
}

static void TestRoundTrips(const fs::path &Folder) {
	// Out of order, with CRLF, tabs and runs of spaces, an entry twice (the
	// last counts), a number above 255 that runs into the next group, the
	// game's own entries (which it sets after the file), group 0 above them,
	// the corners of the table, the types' limits, no newline at the end.
	std::string Toml = RoundTrip(Folder, "mixed",
			"10 1   103\r\n1 0   100\r\n10 0\t102\n  1 1 101\n10 1 4000\n"
			"3 256 7\n0 5 123\n250 0 5\n0 11 11\n0 0 1\n255 255 8191\n200 7 1");
	CHECK(Body(Toml) == "[group.0]\n11 = 11\n\n[group.1]\n0 = 100\n1 = 101\n\n[group.4]\n0 = 7\n\n"
			"[group.10]\n0 = 102\n1 = 4000\n\n[group.200]\n7 = 1\n\n[group.255]\n255 = 8191\n");

	// Nothing: only the game's own entries.
	Toml = RoundTrip(Folder, "empty", "");
	CHECK(Body(Toml) == "");
	RoundTrip(Folder, "blank", "\r\n  \n");
	RoundTrip(Folder, "own", "0 0 5\n0 10 5\n250 0 1\n");

	// A full group, a full table but for the game's own entries.
	std::string Full;
	for (int Number = 0; Number < 256; Number++) {
		Full += "7 " + std::to_string(Number) + " " + std::to_string(Number + 1) + "\n";
	}
	RoundTrip(Folder, "group", Full);
	Full.clear();
	for (int Place = 0; Place < 65536; Place++) {
		Full += std::to_string(Place / 256) + " " + std::to_string(Place % 256) + " "
				+ std::to_string(Place % 8191 + 1) + "\n";
	}
	RoundTrip(Folder, "all", Full);
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
	Refused("type-0.lst", "1 0 100\n1 1 0\n", "Error while loading conversion.lst");
	Refused("type-8192.lst", "1 0 8192\n", "Error while loading conversion.lst");
	Refused("type-negative.lst", "1 0 -1\n", "Error while loading conversion.lst");
	Refused("word.lst", "1 0 100\n1 x 101\n", "an entry that is not three numbers");
	Refused("comment.lst", "# types\n1 0 100\n", "an entry that is not three numbers");
	Refused("short.lst", "1 0 100\n1 1", "an entry that is not three numbers");
	Refused("outside.lst", "256 0 5\n", "a group and number outside the table");
	Refused("negative.lst", "0 -1 5\n", "a group and number outside the table");
	Refused("huge.lst", "16777216 0 5\n", "a group and number outside the table");
	CHECK(Convert(Folder / "missing.lst").find("error: Cannot open conversion.lst") == 0);
}

static void TestErrors(const fs::path &Folder) {
	fs::path File = Folder / "errors" / "conversion.toml";
	auto Load = [&](const std::string &Text) {
		WriteFile(File, Text);
		return InChild([&]() {
			LoadConversions(File.c_str());
			return std::string();
		});
	};
	auto Expect = [&](const std::string &Text, int Line, const char *What) {
		ExpectError(Load, File, Text, Line, What);
	};
	Expect("groups = 1\n", 1, "unknown key \"groups\" in the top level");
	Expect("group = 1\n", 1, "group must be a table");
	Expect("[group]\n1 = 5\n", 2, "group 1 must be a table");
	Expect("[group.256]\n0 = 5\n", 1, "group \"256\" must be a number from 0 to 255 without leading zeros");
	Expect("[group.01]\n0 = 5\n", 1, "group \"01\" must be a number");
	Expect("[group.1000]\n0 = 5\n", 1, "group \"1000\" must be a number");
	Expect("[group.x]\n0 = 5\n", 1, "group \"x\" must be a number");
	Expect("[group.-1]\n0 = 5\n", 1, "group \"-1\" must be a number");
	Expect("[group.\"\"]\n0 = 5\n", 1, "group \"\" must be a number");
	Expect("[group.1]\n0 = 100\n256 = 5\n", 3, "number \"256\" must be a number from 0 to 255 without leading zeros");
	Expect("[group.1]\n007 = 5\n", 2, "number \"007\" must be a number");
	Expect("[group.1]\n00 = 5\n", 2, "number \"00\" must be a number");
	Expect("[group.1]\n1_0 = 5\n", 2, "number \"1_0\" must be a number");
	Expect("[group.1]\n0 = \"5\"\n", 2, "the type of 1/0 must be an integer");
	Expect("[group.1]\n0 = 5.0\n", 2, "the type of 1/0 must be an integer");
	Expect("[group.1]\n0 = 0\n", 2, "the type of 1/0 must lie between 1 and 8191");
	Expect("[group.1]\n0 = 8192\n", 2, "the type of 1/0 must lie between 1 and 8191");
	Expect("[group.1]\n0 = -1\n", 2, "the type of 1/0 must lie between 1 and 8191");
	Expect("[group.0]\n5 = 100\n", 2, "0/5 is the game's own (0/0 to 0/10 and 250/0)");
	Expect("[group.0]\n0 = 100\n", 2, "0/0 is the game's own");
	Expect("[group.0]\n10 = 100\n", 2, "0/10 is the game's own");
	Expect("[group.250]\n0 = 100\n", 2, "250/0 is the game's own");
	Expect("[group.1]\n0 = 100\n0 = 101\n", 3, "");
	Expect("[group.1]\n0 = 100\n[group.1]\n1 = 101\n", 3, "");
	Expect("[group.1]\n0 = \n", 2, "");

	// What loads: nothing; an empty group; the corners; group 0 above the
	// game's own entries and 250 beside them; inline tables.
	CHECK(Load("") == "");
	CHECK(Load("[group.3]\n") == "");
	CHECK(Load("[group.0]\n11 = 1\n[group.255]\n255 = 8191\n[group.250]\n1 = 99\n") == "");
	CHECK(Load("group = { 1 = { 0 = 100, 1 = 101 }, 2 = {} }\n") == "");

	std::string Missing = InChild([&]() {
		LoadConversions((Folder / "missing.toml").c_str());
		return std::string();
	});
	CHECK(IsError(Missing) && Missing.find((Folder / "missing.toml").string()) != std::string::npos);
}

// The table a good load left, kept by the parent for the children to compare
// with.
static int ExpectedTable[65536];

// Every change of one entry of the file (its type one up, its number one up,
// the entry left out) and of one group (its number one up) must give another
// table, or an error. Returns the number of changes made.
static int TestMutations(const fs::path &Folder, const std::string &Toml) {
	int Changes = 0;
	fs::path File = Folder / "mutated.toml";
	auto Try = [&](const std::string &Mutated, const std::string &What) {
		Changes++;
		WriteFile(File, Mutated);
		std::string Result = InChild([&]() {
			LoadConversions(File.c_str());
			return std::string(memcmp(NewType, ExpectedTable, sizeof(NewType)) == 0 ? "same" : "different");
		});
		if (Result == "same") {
			fprintf(stderr, "mutation not caught: %s\n", What.c_str());
			Failures++;
		}
	};
	size_t Start = Toml.find("\n[group.");
	size_t Position = Start + 1;
	while (Position < Toml.size()) {
		size_t End = Toml.find('\n', Position);
		std::string Line = Toml.substr(Position, End - Position);
		std::string Where = "line \"" + Line + "\" at byte " + std::to_string(Position);
		if (Line.rfind("[group.", 0) == 0) {
			int Group = atoi(Line.c_str() + 7);
			std::string Mutated = Toml;
			Mutated.replace(Position, Line.size(), "[group." + std::to_string(Group + 1) + "]");
			Try(Mutated, Where + ", group one up");
		} else if (!Line.empty()) {
			size_t Equals = Line.find(" = ");
			int Number = atoi(Line.c_str());
			int Type = atoi(Line.c_str() + Equals + 3);
			std::string Mutated = Toml;
			Mutated.replace(Position, Line.size(), std::to_string(Number) + " = " + std::to_string(Type + 1));
			Try(Mutated, Where + ", type one up");
			Mutated = Toml;
			Mutated.replace(Position, Line.size(), std::to_string(Number + 1) + " = " + std::to_string(Type));
			Try(Mutated, Where + ", number one up");
			Mutated = Toml;
			Mutated.erase(Position, Line.size() + 1);
			Try(Mutated, Where + ", left out");
		}
		Position = End + 1;
	}
	return Changes;
}

static int Original(const char *ConversionLstFile, const char *ConversionToml) {
	if (!fs::is_regular_file(ConversionLstFile)) {
		printf("skipped: no %s\n", ConversionLstFile);
		return 77;
	}

	std::string Text = Convert(ConversionLstFile);
	std::string Committed = ReadFile(ConversionToml);
	if (Committed != Text) {
		fprintf(stderr, "%s is not what patella-convert conversion makes of %s (%s)\n", ConversionToml,
				ConversionLstFile, IsError(Text) ? Text.c_str() : "a different text");
		Failures++;
	}

	std::string Expected = Snapshot(OldLoader(ConversionLstFile));
	std::string Actual = Snapshot(NewLoader(ConversionToml));
	if (IsError(Expected) || IsError(Actual)) {
		fprintf(stderr, "a load failed: %s / %s\n", Expected.substr(0, 200).c_str(), Actual.substr(0, 200).c_str());
		return 1;
	}
	if (Same(Expected, Actual, "original", "conversion.lst", "conversion.toml")) {
		std::map<std::string, std::string> Values = Fields(Actual);
		printf("%s and %s give the same table: %s entries with a type (the game's own 12 included), all 65536 "
				"compared\n", ConversionLstFile, ConversionToml, Values["entries"].c_str());
	}

	LoadConversions(ConversionToml);
	memcpy(ExpectedTable, NewType, sizeof(ExpectedTable));
	fs::path Folder = TemporaryFolder("convert-test-conversion-original");
	int Changes = TestMutations(Folder, Committed);
	fs::remove_all(Folder);
	printf("%d changes of %s, each caught\n", Changes, ConversionToml);
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
		fprintf(stderr, "usage: convert_test_conversion [--original CONVERSION_LST CONVERSION_TOML]\n");
		return 2;
	}

	fs::path Folder = TemporaryFolder("convert-test-conversion");
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
