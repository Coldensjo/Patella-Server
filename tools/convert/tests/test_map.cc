// test_map.cc: the round trip of the map settings. The settings the game ends
// up with from a map.dat and the mem.dat it includes, through the old reader
// (ReadMapDat), and from the map.toml patella-convert makes of them, through
// the game's LoadMapData, must be the same, field by field; LoadMapData must
// refuse what is wrong with the file and line.
//
//   convert_test_map
//     With made-up files in a temporary folder.
//   convert_test_map --original MAP_DAT MAP_TOML
//     With the original map.dat (and the mem.dat next to it) and the
//     committed map.toml: MAP_TOML must be exactly what patella-convert makes
//     of MAP_DAT, and give the same settings; and every change of one value
//     or entry of it must give other settings, or an error. Exit code 77
//     (skipped) when MAP_DAT is not there.
//
// Every load runs in a child process of its own, on variables filled with a
// byte pattern first; every new[] of the test (the entries of DepotInfo and
// Mark as they grow) is filled with the pattern too, so that what neither
// loader sets compares the same. The variables are compared whole: the
// sectors' range, the refreshed cylinders, Objects (HashTableSize) and the
// blocks of the cache (OBCount), both start positions, Marks and every mark
// up to one past the last (the name byte by byte), and DepotInfo's range of
// numbers with every entry in it (the town byte by byte), set or not.

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <new>
#include <string>
#include <vector>

#include "map_dat.hh"
#include "map_writer.hh"
#include "mapdata.hh"
#include "tomlfile.hh"

#include "test_common.hh"

static const unsigned char POISON = 0xA5;

void *operator new[](size_t Size) {
	void *Memory = malloc(Size != 0 ? Size : 1);
	if (Memory == nullptr) {
		throw std::bad_alloc();
	}
	memset(Memory, POISON, Size);
	return Memory;
}

void operator delete[](void *Memory) noexcept {
	free(Memory);
}

void operator delete[](void *Memory, size_t) noexcept {
	free(Memory);
}

static void Fill(void *Memory, size_t Size) {
	memset(Memory, POISON, Size);
}

static void Poison() {
	int *Scalars[] = {&SectorXMin, &SectorXMax, &SectorYMin, &SectorYMax, &SectorZMin, &SectorZMax,
			&RefreshedCylinders, &HashTableSize, &OBCount, &Marks, &NewbieStartPositionX, &NewbieStartPositionY,
			&NewbieStartPositionZ, &VeteranStartPositionX, &VeteranStartPositionY, &VeteranStartPositionZ};
	for (int *Scalar : Scalars) {
		Fill(Scalar, sizeof(*Scalar));
	}
	for (int i = DepotInfo.low(); i <= DepotInfo.high(); i++) {
		Fill(&DepotInfo(i), sizeof(TDepotInfo));
	}
	for (int i = Mark.low(); i <= Mark.high(); i++) {
		Fill(&Mark(i), sizeof(TMark));
	}
}

// A buffer as hex, all of it.
static std::string Bytes(const char *Buffer, size_t Size) {
	std::string Out;
	for (size_t i = 0; i < Size; i++) {
		char Code[4];
		snprintf(Code, sizeof(Code), "%02X", static_cast<unsigned char>(Buffer[i]));
		Out += Code;
	}
	return Out;
}

static std::string Three(int a, int b, int c) {
	return std::to_string(a) + " " + std::to_string(b) + " " + std::to_string(c);
}

// The variables as "field\tvalue" lines.
static std::string DumpMap() {
	std::string Out;
	Out += "sectors x\t" + std::to_string(SectorXMin) + " " + std::to_string(SectorXMax) + "\n";
	Out += "sectors y\t" + std::to_string(SectorYMin) + " " + std::to_string(SectorYMax) + "\n";
	Out += "sectors z\t" + std::to_string(SectorZMin) + " " + std::to_string(SectorZMax) + "\n";
	Out += "refreshed cylinders\t" + std::to_string(RefreshedCylinders) + "\n";
	Out += "objects\t" + std::to_string(HashTableSize) + "\n";
	Out += "cache blocks\t" + std::to_string(OBCount) + "\n";
	Out += "newbie start\t" + Three(NewbieStartPositionX, NewbieStartPositionY, NewbieStartPositionZ) + "\n";
	Out += "veteran start\t" + Three(VeteranStartPositionX, VeteranStartPositionY, VeteranStartPositionZ) + "\n";
	Out += "marks\t" + std::to_string(Marks) + "\n";
	if (Marks >= 0 && Marks < 100000) {
		for (int i = 0; i <= Marks; i++) {
			const TMark &M = Mark(i);
			Out += "mark " + std::to_string(i) + "\t" + Bytes(M.Name, sizeof(M.Name)) + " " + Three(M.x, M.y, M.z)
					+ "\n";
		}
	}
	int Low = DepotInfo.low(), High = DepotInfo.high();
	Out += "depot numbers\t" + std::to_string(Low) + " " + std::to_string(High) + "\n";
	for (int i = Low; i <= High; i++) {
		const TDepotInfo &D = DepotInfo(i);
		Out += "depot " + std::to_string(i) + "\t" + Bytes(D.Town, sizeof(D.Town)) + " " + std::to_string(D.Size)
				+ "\n";
	}
	return Out;
}

static std::string Snapshot(const std::function<void()> &Load) {
	return InChild([&]() {
		Poison();
		Load();
		return DumpMap();
	});
}

static std::function<void()> OldLoader(const fs::path &File) {
	return [File]() {
		TMapDatExtras Extras;
		ReadMapDat(File.c_str(), Extras);
	};
}

static std::function<void()> NewLoader(const fs::path &File) {
	return [File]() {
		LoadMapData(File.c_str());
	};
}

// The map.toml patella-convert makes of a map.dat file.
static std::string Convert(const fs::path &File) {
	return InChild([&]() {
		TMapDatExtras Extras;
		ReadMapDat(File.c_str(), Extras);
		return MapToToml(Extras);
	});
}

// Converts a map.dat text (with a mem.dat next to it, if MemDat is not
// empty), loads both and compares them. Returns the map.toml text.
static std::string RoundTrip(const fs::path &Folder, const char *Name, const std::string &MapDat,
		const std::string &MemDat = "") {
	fs::path Original = Folder / Name / "map.dat";
	fs::path Converted = Folder / Name / "map.toml";
	WriteFile(Original, MapDat);
	if (!MemDat.empty()) {
		WriteFile(Folder / Name / "mem.dat", MemDat);
	}
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
	if (!Same(Expected, Actual, Name, "map.dat", "map.toml")) {
		fprintf(stderr, "%s: map.toml was:\n%s", Name, Text.c_str());
	}
	return Text;
}

// The part of a map.toml after its comments.
static std::string Body(const std::string &Toml) {
	size_t Start = Toml.find("\nrefreshed_cylinders");
	return Start == std::string::npos ? Toml : Toml.substr(Start + 1);
}

static const char *const STARTS = "NewbieStart = [32097,32219,7]\nVeteranStart = [32369,32241,7]\n";

static void TestRoundTrips(const fs::path &Folder) {
	// Everything at once: keys in any case and given twice, the limits of
	// each value, the memory settings in an included file, marks with
	// escapes, Latin-1, empty and 19-character names, one name three times,
	// depots out of order, with gaps, one twice, with odd sizes and towns.
	std::string Toml = RoundTrip(Folder, "full",
			"# GIMUD - Graphical Interface Multi User Dungeon\r\n"
			"# map.dat: Daten der Karte\r\n"
			"SectorXMin = 5\r\n"
			"SECTORXMIN = 1\r\n"
			"sectorxmax =    2046\r\n"
			"SectorYMin = 2046\r\n"
			"SectorYMax = 2046\r\n"
			"SectorZMin = 15\r\n"
			"SectorZMax = 15\r\n"
			"RefreshedCylinders = 2147483647\r\n"
			"@\"mem.dat\"\r\n"
			"NewbieStart = [1,0,0]\r\n"
			"VeteranStart = [65535,65535,15]\r\n"
			"Mark = (\"Thais\",[32369,32215,7])\r\n"
			"Mark = (\"\",[0,0,0])\r\n"
			"Mark = (\"abcdefghijklmnopqrs\",[65535,65535,15])\r\n"
			"Mark = (\"Quote\\\"Back\\\\slash\\n\",[1,2,3])\r\n"
			"Mark = (\"K\xe4se \xdf\xff\",[4,5,6])\r\n"
			"Mark = (\"THAIS\",[7,8,9])\r\n"
			"Mark = (\"Thais\",[10,11,12])\r\n"
			"Depot = (8,\"Port Hope\",1000)\r\n"
			"Depot = (0,\"Thais\",0)\r\n"
			"Depot = (3,\"abcdefghijklmnopqrs\",-7)\r\n"
			"Depot = (0,\"Thais again\",2147483647)\r\n"
			"Depot = (5,\"\",-2147483648)\r\n"
			"Depot = (6,\"Stra\xdf" "e \\\"7\\\"\",1)\r\n",
			"# mem.dat\nObjects   = 1\nCacheSize = 32768\n");
	CHECK(Body(Toml).rfind("refreshed_cylinders = 2147483647\nnewbie_start = [1, 0, 0]\n"
			"veteran_start = [65535, 65535, 15]\n\n[sectors]\nx_min = 1\nx_max = 2046\n", 0) == 0);
	CHECK(Toml.find("\n[memory]\nobjects = 1\ncache_size = 32768\n") != std::string::npos);
	CHECK(Toml.find("name = \"Quote\\\"Back\\\\slash\\n\"\n") != std::string::npos);
	CHECK(Toml.find("name = \"K\xc3\xa4se \xc3\x9f\xc3\xbf\"\n") != std::string::npos);
	CHECK(Toml.find("[[depot]]\nid = 0\ntown = \"Thais again\"\nsize = 2147483647\n\n[[depot]]\nid = 3\n")
			!= std::string::npos);
	std::string Last = "\n[[depot]]\nid = 8\ntown = \"Port Hope\"\nsize = 1000\n";
	CHECK(Toml.size() > Last.size() && Toml.compare(Toml.size() - Last.size(), Last.size(), Last) == 0);

	// Only the start positions: the old reader's defaults for the rest.
	Toml = RoundTrip(Folder, "minimal", STARTS);
	CHECK(Toml.find("\n[sectors]\nx_min = 1000\nx_max = 1015\ny_min = 1000\ny_max = 1015\nz_min = 0\nz_max = 15\n"
			"\n[memory]\nobjects = 1048576\ncache_size = 655360\n") != std::string::npos);
	CHECK(Toml.find("refreshed_cylinders = 1\n") != std::string::npos);
	CHECK(Toml.find("\n[[mark]]\n") == std::string::npos && Toml.find("\n[[depot]]\n") == std::string::npos);

	// The largest memory settings, given in map.dat itself; the refreshed
	// cylinders at their other end; a single depot that leaves DepotInfo's
	// first entries unset.
	RoundTrip(Folder, "large", std::string(STARTS) + "Objects = 1073741824\nCacheSize = 2147450880\n"
			"RefreshedCylinders = -2147483648\nDepot = (2,\"Kazordoon\",5)\n");

	// Many marks, so that Mark grows many times; all depots.
	std::string Many = STARTS;
	for (int i = 0; i < 300; i++) {
		Many += "Mark = (\"Mark" + std::to_string(i) + "\",[" + std::to_string(32000 + i) + ","
				+ std::to_string(31000 + i) + "," + std::to_string(i % 16) + "])\n";
	}
	for (int i = 8; i >= 0; i--) {
		Many += "Depot = (" + std::to_string(i) + ",\"Town" + std::to_string(i) + "\"," + std::to_string(i * 100)
				+ ")\n";
	}
	RoundTrip(Folder, "many", Many);

	// The original layout: the start positions after the include, the marks
	// before the depots.
	RoundTrip(Folder, "layout",
			"SectorXMin =  996\nSectorXMax = 1043\nSectorYMin =  984\nSectorYMax = 1031\nSectorZMin =    0\n"
			"SectorZMax =   15\nRefreshedCylinders = 8\n@\"mem.dat\"\nNewbieStart = [32097,32219,7]\n"
			"VeteranStart = [32369,32241,7]\nMark = (\"Thais\",[32369,32215,7])\nDepot = (0,\"Thais\",1000)\n",
			"Objects   = 33554432\nCacheSize = 20971520\n");
}

// What a conversion refuses.
static void TestRefusals(const fs::path &Folder) {
	auto Refused = [&](const char *Name, const std::string &Text, const char *What) {
		fs::path File = Folder / "refusals" / Name / "map.dat";
		WriteFile(File, Text);
		std::string Out = Convert(File);
		if (Out.compare(0, 7, "error: ") != 0 || Out.find(What) == std::string::npos) {
			fprintf(stderr, "%s: expected an error with \"%s\", got \"%s\"\n", Name, What, Out.substr(0, 200).c_str());
			Failures++;
		}
	};
	// Where the old reader wrote past its buffers, or what map.toml cannot
	// hold.
	Refused("depot-9", std::string(STARTS) + "Depot = (9,\"Nowhere\",1000)\n",
			"error in script-file \"map.dat\", line 3: depot number outside 0 to 8");
	Refused("mark-name", std::string(STARTS) + "Mark = (\"abcdefghijklmnopqrst\",[1,2,3])\n",
			"line 3: mark name too long");
	Refused("identifier", std::string(STARTS) + "abcdefghijklmnopqrstuvwxyzabcd = 5\n",
			"line 3: identifier too long");
	Refused("mark-x", std::string(STARTS) + "Mark = (\"Far\",[65536,2,3])\n", "a field outside x and y 0 to 65535");
	Refused("mark-z", std::string(STARTS) + "Mark = (\"Deep\",[1,2,16])\n", "a field outside x and y 0 to 65535");
	Refused("start-z", "NewbieStart = [32097,32219,16]\nVeteranStart = [32369,32241,7]\n",
			"a field outside x and y 0 to 65535");
	Refused("start-y", "NewbieStart = [32097,32219,7]\nVeteranStart = [32369,65536,7]\n",
			"a field outside x and y 0 to 65535");
	// The old reader's own checks.
	Refused("town", std::string(STARTS) + "Depot = (1,\"abcdefghijklmnopqrst\",1000)\n", "town name too long");
	Refused("no-newbie", "VeteranStart = [32369,32241,7]\n", "no start position for newbies specified");
	Refused("no-veteran", "NewbieStart = [32097,32219,7]\n", "no start position for veterans specified");
	Refused("sector-x", std::string(STARTS) + "SectorXMin = 0\n", "illegal value for SectorXMin");
	Refused("sector-x-max", std::string(STARTS) + "SectorXMax = 2047\n", "illegal value for SectorXMax");
	Refused("sector-z", std::string(STARTS) + "SectorZMax = 16\n", "illegal value for SectorZMax");
	Refused("sector-order", std::string(STARTS) + "SectorYMin = 1020\n", "SectorYMin is greater than SectorYMax");
	Refused("cache", std::string(STARTS) + "CacheSize = 1000\n", "CacheSize must be a multiple of 32768");
	Refused("cache-zero", std::string(STARTS) + "CacheSize = 0\n", "illegal value for CacheSize");
	Refused("objects", std::string(STARTS) + "Objects = 3\n", "Objects must be a power of 2");
	Refused("unknown", std::string(STARTS) + "Unknown = 5\n", "line 3: Identifier expected");
	Refused("include", std::string(STARTS) + "@\"nothing.dat\"\n", "Cannot open script-file");
	CHECK(Convert(Folder / "missing" / "map.dat").find("error: Cannot open script-file") == 0);
}

// A valid map.toml, one line each, numbered from 1 in the comments.
static const std::vector<std::string> BASE = {
	"refreshed_cylinders = 8",         // 1
	"newbie_start = [32097, 32219, 7]", // 2
	"veteran_start = [32369, 32241, 7]", // 3
	"[sectors]",                       // 4
	"x_min = 996",                     // 5
	"x_max = 1043",                    // 6
	"y_min = 984",                     // 7
	"y_max = 1031",                    // 8
	"z_min = 0",                       // 9
	"z_max = 15",                      // 10
	"[memory]",                        // 11
	"objects = 33554432",              // 12
	"cache_size = 20971520",           // 13
	"[[mark]]",                        // 14
	"name = \"Thais\"",                // 15
	"position = [32369, 32215, 7]",    // 16
	"[[depot]]",                       // 17
	"id = 0",                          // 18
	"town = \"Thais\"",                // 19
	"size = 1000",                     // 20
};

static std::string Join(const std::vector<std::string> &Lines) {
	std::string Text;
	for (const std::string &Line : Lines) {
		Text += Line + "\n";
	}
	return Text;
}

// BASE with line Number (from 1) replaced by Text, or removed for nullptr;
// and line Number2 by Text2, if given.
static std::string With(int Number, const char *Text, int Number2 = 0, const char *Text2 = nullptr) {
	std::vector<std::string> Lines = BASE;
	if (Number2 != 0) {
		Lines[static_cast<size_t>(Number2 - 1)] = Text2;
	}
	if (Text == nullptr) {
		Lines.erase(Lines.begin() + (Number - 1));
	} else {
		Lines[static_cast<size_t>(Number - 1)] = Text;
	}
	return Join(Lines);
}

// BASE without lines First to Last (from 1).
static std::string Without(int First, int Last) {
	std::vector<std::string> Lines = BASE;
	Lines.erase(Lines.begin() + (First - 1), Lines.begin() + Last);
	return Join(Lines);
}

static void TestErrors(const fs::path &Folder) {
	fs::path File = Folder / "errors" / "map.toml";
	auto Load = [&](const std::string &Text) {
		WriteFile(File, Text);
		return InChild([&]() {
			LoadMapData(File.c_str());
			return std::string();
		});
	};
	auto Expect = [&](const std::string &Text, int Line, const char *What) {
		ExpectError(Load, File, Text, Line, What);
	};
	std::string Base = Join(BASE);
	CHECK(Load(Base) == "");

	// The top level.
	Expect(With(1, "refreshed = 8"), 1, "unknown key \"refreshed\" in the top level");
	Expect("mem = 1\n" + Base, 1, "unknown key \"mem\" in the top level");
	Expect(Base + "mem = 1\n", 21, "unknown key \"mem\" in [[depot]]");
	Expect(With(1, nullptr), 1, "the top level needs refreshed_cylinders");
	Expect(With(2, nullptr), 1, "the top level needs newbie_start");
	Expect(With(3, nullptr), 1, "the top level needs veteran_start");
	Expect(Without(4, 10), 1, "the top level needs sectors");
	Expect(Without(11, 13), 1, "the top level needs memory");
	Expect("sectors = 1\n" + Without(4, 10), 1, "sectors must be a table");
	Expect("memory = [1]\n" + Without(11, 13), 1, "memory must be a table");
	Expect(With(1, "refreshed_cylinders = \"8\""), 1, "refreshed_cylinders must be an integer");
	Expect(With(1, "refreshed_cylinders = 2147483648"), 1,
			"refreshed_cylinders must lie between -2147483648 and 2147483647");
	Expect(With(1, "refreshed_cylinders = -2147483649"), 1, "refreshed_cylinders must lie between");
	Expect(With(2, "newbie_start = [0, 32219, 7]"), 2, "x of newbie_start must lie between 1 and 65535");
	Expect(With(2, "newbie_start = [65536, 32219, 7]"), 2, "x of newbie_start must lie between 1 and 65535");
	Expect(With(2, "newbie_start = [1, -1, 7]"), 2, "y of newbie_start must lie between 0 and 65535");
	Expect(With(2, "newbie_start = [1, 65536, 7]"), 2, "y of newbie_start must lie between 0 and 65535");
	Expect(With(3, "veteran_start = [1, 2, 16]"), 3, "z of veteran_start must lie between 0 and 15");
	Expect(With(3, "veteran_start = [1, 2, \"7\"]"), 3, "z of veteran_start must be an integer");
	Expect(With(3, "veteran_start = [1, 2]"), 3, "veteran_start must be an array [x, y, z]");
	Expect(With(3, "veteran_start = [1, 2, 3, 4]"), 3, "veteran_start must be an array [x, y, z]");
	Expect(With(3, "veteran_start = 5"), 3, "veteran_start must be an array [x, y, z]");
	Expect(With(1, "refreshed_cylinders = "), 1, "");

	// [sectors].
	Expect(With(5, "xmin = 996"), 5, "unknown key \"xmin\" in [sectors]");
	Expect(With(5, nullptr), 4, "[sectors] needs x_min");
	Expect(With(6, nullptr), 4, "[sectors] needs x_max");
	Expect(With(10, nullptr), 4, "[sectors] needs z_max");
	Expect(With(5, "x_min = 0"), 5, "x_min must lie between 1 and 2046");
	Expect(With(6, "x_max = 2047"), 6, "x_max must lie between 1 and 2046");
	Expect(With(7, "y_min = 0"), 7, "y_min must lie between 1 and 2046");
	Expect(With(8, "y_max = 2047"), 8, "y_max must lie between 1 and 2046");
	Expect(With(9, "z_min = -1"), 9, "z_min must lie between 0 and 15");
	Expect(With(10, "z_max = 16"), 10, "z_max must lie between 0 and 15");
	Expect(With(5, "x_min = 1044"), 5, "x_min 1044 is greater than x_max 1043");
	Expect(With(7, "y_min = 1032"), 7, "y_min 1032 is greater than y_max 1031");
	Expect(With(9, "z_min = 1", 10, "z_max = 0"), 9, "z_min 1 is greater than z_max 0");
	Expect(With(5, "x_min = 996.0"), 5, "x_min must be an integer");

	// [memory].
	Expect(With(12, "object = 1"), 12, "unknown key \"object\" in [memory]");
	Expect(With(12, nullptr), 11, "[memory] needs objects");
	Expect(With(13, nullptr), 11, "[memory] needs cache_size");
	Expect(With(12, "objects = 3"), 12, "objects must be a power of 2");
	Expect(With(12, "objects = 33554431"), 12, "objects must be a power of 2");
	Expect(With(12, "objects = 0"), 12, "objects must lie between 1 and 2147483647");
	Expect(With(12, "objects = 2147483648"), 12, "objects must lie between 1 and 2147483647");
	Expect(With(13, "cache_size = 32769"), 13, "cache_size must be a multiple of 32768");
	Expect(With(13, "cache_size = 0"), 13, "cache_size must lie between 32768 and 2147483647");
	Expect(With(13, "cache_size = 2147483648"), 13, "cache_size must lie between 32768 and 2147483647");

	// [[mark]].
	Expect(With(15, "names = \"Thais\""), 15, "unknown key \"names\" in [[mark]]");
	Expect(With(15, nullptr), 14, "a mark needs name");
	Expect(With(16, nullptr), 14, "a mark needs position");
	Expect(With(15, "name = 5"), 15, "name must be a string");
	Expect(With(15, "name = \"abcdefghijklmnopqrst\""), 15, "name is longer than 19 characters");
	Expect(With(15, "name = \"\\u0100\""), 15, "name has a character outside Latin-1");
	Expect(With(15, "name = \"a\\u0000b\""), 15, "name has a zero character");
	Expect(With(16, "position = [65536, 2, 3]"), 16, "x of position must lie between 0 and 65535");
	Expect(With(16, "position = [1, 2, 16]"), 16, "z of position must lie between 0 and 15");
	Expect(With(16, "position = \"here\""), 16, "position must be an array [x, y, z]");
	Expect("mark = 5\n" + Without(14, 16), 1, "mark must be an array");
	Expect("mark = [1]\n" + Without(14, 16), 1, "an entry of mark must be a table");

	// [[depot]].
	Expect(With(18, "number = 0"), 18, "unknown key \"number\" in [[depot]]");
	Expect(With(18, nullptr), 17, "a depot needs id");
	Expect(With(19, nullptr), 17, "depot 0 needs town");
	Expect(With(20, nullptr), 17, "depot 0 needs size");
	Expect(With(18, "id = 9"), 18, "id must lie between 0 and 8");
	Expect(With(18, "id = -1"), 18, "id must lie between 0 and 8");
	Expect(With(18, "id = \"0\""), 18, "id must be an integer");
	Expect(Base + "[[depot]]\nid = 1\ntown = \"Carlin\"\nsize = 1\n[[depot]]\nid = 0\n", 26,
			"depot 0 is defined twice");
	Expect(With(19, "town = \"abcdefghijklmnopqrst\""), 19, "town is longer than 19 characters");
	Expect(With(19, "town = 0"), 19, "town must be a string");
	Expect(With(20, "size = 2147483648"), 20, "size must lie between -2147483648 and 2147483647");
	Expect(With(20, "size = 1.5"), 20, "size must be an integer");
	Expect("depot = {}\n" + Without(17, 20), 1, "depot must be an array");

	// What loads: no marks and no depots; the limits of each value.
	CHECK(Load(Without(14, 20)) == "");
	CHECK(Load(With(15, "name = \"abcdefghijklmnopqrs\"")) == "");
	CHECK(Load(With(15, "name = \"\"")) == "");
	CHECK(Load(With(15, "name = \"\xc3\xbf\"")) == "");
	CHECK(Load(With(20, "size = -2147483648")) == "");
	CHECK(Load(With(18, "id = 8")) == "");
	CHECK(Load(With(12, "objects = 1073741824")) == "");
	CHECK(Load(With(12, "objects = 1")) == "");
	CHECK(Load(With(13, "cache_size = 2147450880")) == "");
	CHECK(Load(With(2, "newbie_start = [1, 0, 0]")) == "");
	CHECK(Load(With(16, "position = [0, 65535, 15]")) == "");
	CHECK(Load(Base + "[[mark]]\nname = \"Thais\"\nposition = [1, 2, 3]\n") == "");

	std::string Missing = InChild([&]() {
		LoadMapData((Folder / "missing.toml").c_str());
		return std::string();
	});
	CHECK(IsError(Missing) && Missing.find((Folder / "missing.toml").string()) != std::string::npos);
}

// The positions of the values of a map.toml after its comments: the integers
// and the strings.
struct TValue {
	size_t Start;
	size_t Length;
	bool String;
};

static std::vector<TValue> Values(const std::string &Toml, size_t BodyStart) {
	std::vector<TValue> Result;
	for (size_t i = BodyStart; i < Toml.size(); i++) {
		char c = Toml[i];
		if (c == '"') {
			size_t End = i + 1;
			while (Toml[End] != '"') {
				End += Toml[End] == '\\' ? 2 : 1;
			}
			Result.push_back({i, End - i + 1, true});
			i = End;
		} else if ((c >= '0' && c <= '9') || c == '-') {
			bool Starts = Toml[i - 1] == ' ' || Toml[i - 1] == '[';
			size_t End = i + 1;
			while (End < Toml.size() && Toml[End] >= '0' && Toml[End] <= '9') {
				End++;
			}
			if (Starts) {
				Result.push_back({i, End - i, false});
			}
			i = End - 1;
		} else if (c == '\n' && i + 1 < Toml.size() && Toml[i + 1] != '[') {
			// Skip the key up to "= ".
			size_t Equals = Toml.find(" = ", i);
			if (Equals != std::string::npos && Equals < Toml.find('\n', i + 1)) {
				i = Equals + 2;
			}
		}
	}
	return Result;
}

// The starts of the [[mark]] and [[depot]] entries, and where each ends.
static std::vector<std::pair<size_t, size_t>> Entries(const std::string &Toml, const char *Header) {
	std::vector<std::pair<size_t, size_t>> Result;
	size_t Start = Toml.find(Header);
	while (Start != std::string::npos) {
		size_t End = Toml.find("\n\n", Start);
		End = End == std::string::npos ? Toml.size() : End + 2;
		Result.push_back({Start, End});
		Start = Toml.find(Header, End);
	}
	return Result;
}

// Every change of one value of the file (an integer one up, a string one
// character longer), every mark and depot left out, and every swap of a mark
// with the next must give other settings, or an error. The order of the
// depots does not count, so swapping them is no change. Returns the number of
// changes made.
static int TestMutations(const fs::path &Folder, const std::string &Toml, const std::string &Expected) {
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
	size_t BodyStart = Toml.find("\nrefreshed_cylinders");
	for (const TValue &Value : Values(Toml, BodyStart)) {
		std::string Mutated = Toml;
		std::string Old = Toml.substr(Value.Start, Value.Length);
		if (Value.String) {
			Mutated.insert(Value.Start + 1, "x");
		} else {
			Mutated.replace(Value.Start, Value.Length, std::to_string(std::stoll(Old) + 1));
		}
		Try(Mutated, "the value " + Old + " at byte " + std::to_string(Value.Start));
	}
	for (const char *Header : {"[[mark]]\n", "[[depot]]\n"}) {
		std::vector<std::pair<size_t, size_t>> List = Entries(Toml, Header);
		for (const auto &Entry : List) {
			std::string Mutated = Toml;
			Mutated.erase(Entry.first, Entry.second - Entry.first);
			Try(Mutated, std::string("the entry at byte ") + std::to_string(Entry.first) + " left out");
		}
		if (std::string(Header) == "[[mark]]\n") {
			for (size_t i = 0; i + 1 < List.size(); i++) {
				std::string A = Toml.substr(List[i].first, List[i].second - List[i].first);
				std::string B = Toml.substr(List[i + 1].first, List[i + 1].second - List[i + 1].first);
				if (B.back() != '\n' || B.size() < 2 || B[B.size() - 2] != '\n') {
					B += "\n";
					A.pop_back();
				}
				std::string Mutated = Toml;
				Mutated.replace(List[i].first, List[i + 1].second - List[i].first, B + A);
				Try(Mutated, "the marks at bytes " + std::to_string(List[i].first) + " and "
						+ std::to_string(List[i + 1].first) + " swapped");
			}
		}
	}
	return Changes;
}

static int Original(const char *MapDatFile, const char *MapToml) {
	if (!fs::is_regular_file(MapDatFile)) {
		printf("skipped: no %s\n", MapDatFile);
		return 77;
	}

	std::string Text = Convert(MapDatFile);
	std::string Committed = ReadFile(MapToml);
	if (Committed != Text) {
		fprintf(stderr, "%s is not what patella-convert map makes of %s (%s)\n", MapToml, MapDatFile,
				IsError(Text) ? Text.c_str() : "a different text");
		Failures++;
	}

	std::string Expected = Snapshot(OldLoader(MapDatFile));
	std::string Actual = Snapshot(NewLoader(MapToml));
	if (IsError(Expected) || IsError(Actual)) {
		fprintf(stderr, "a load failed: %s / %s\n", Expected.substr(0, 200).c_str(), Actual.substr(0, 200).c_str());
		return 1;
	}
	if (Same(Expected, Actual, "original", "map.dat", "map.toml")) {
		std::map<std::string, std::string> Values = Fields(Actual);
		printf("%s and %s give the same settings: sectors x %s, y %s, z %s; %s marks; depot numbers %s; %d "
				"values compared\n", MapDatFile, MapToml, Values["sectors x"].c_str(), Values["sectors y"].c_str(),
				Values["sectors z"].c_str(), Values["marks"].c_str(), Values["depot numbers"].c_str(),
				static_cast<int>(Values.size()));
	}

	fs::path Folder = TemporaryFolder("convert-test-map-original");
	int Changes = TestMutations(Folder, Committed, Expected);
	fs::remove_all(Folder);
	printf("%d changes of %s, each caught\n", Changes, MapToml);
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
		fprintf(stderr, "usage: convert_test_map [--original MAP_DAT MAP_TOML]\n");
		return 2;
	}

	fs::path Folder = TemporaryFolder("convert-test-map");
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
