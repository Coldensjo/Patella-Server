// test_houses.cc: the round trip of the houses. The areas, houses and house
// fields the game ends up with from a houseareas.dat and a houses.dat,
// through the old readers (ReadHouseAreasDat, ReadHousesDat), and from the
// houses.toml patella-convert makes of them, through the game's
// LoadHouseData, must be the same, field by field; LoadHouseData must refuse
// what is wrong with the file and line.
//
//   convert_test_houses
//     With made-up files in a temporary folder.
//   convert_test_houses --original HOUSEAREAS_DAT HOUSES_DAT HOUSES_TOML
//     With the original houseareas.dat and houses.dat and the committed
//     houses.toml: HOUSES_TOML must be exactly what patella-convert makes of
//     the two, and give the same tables; and every change of one value or
//     entry of it must give other tables, or an error. Exit code 77 (skipped)
//     when HOUSES_DAT or HOUSEAREAS_DAT is not there.
//
// The round trips run each load in a child process of its own, on tables
// whose first entries are filled with a byte pattern first. They compare the
// tables whole: the counts, every area (ID, SQMPrice, DepotNr) and every
// house (every member but the subowner and guest lists, which neither loader
// touches; Name, Description and OwnerName byte by byte) up to one past the
// last, MaxHouseX and MaxHouseY, every field handed to SetHouseID with its
// house, in order, and the error lines logged (the exits and centers not
// set).

#include <climits>
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>

#include "houses_dat.hh"
#include "houses_writer.hh"
#include "housesdata.hh"
#include "tomlfile.hh"
#include "utils.hh"

#include "test_common.hh"

static const unsigned char POISON = 0xA5;

// The entries filled with the pattern before a load: more than the original
// files and the made-up ones have.
static const int POISONED_AREAS = 200;
static const int POISONED_HOUSES = 1000;

// The fields handed to SetHouseID and the error lines, of the current load.
static std::vector<int> FieldCalls;
static std::string ErrorLines;

static void RecordField(int x, int y, int z, unsigned short HouseID) {
	FieldCalls.push_back(x);
	FieldCalls.push_back(y);
	FieldCalls.push_back(z);
	FieldCalls.push_back(HouseID);
}

static void RecordError(char *Text) {
	ErrorLines += Text;
}

static void Fill(void *Memory, size_t Size) {
	memset(Memory, POISON, Size);
}

static void Poison() {
	for (int i = 0; i < POISONED_AREAS; i++) {
		Fill(&HouseArea(i), sizeof(THouseArea));
	}
	for (int i = 0; i < POISONED_HOUSES; i++) {
		THouse &H = House(i);
		Fill(&H.ID, sizeof(H.ID));
		Fill(H.Name, sizeof(H.Name));
		Fill(H.Description, sizeof(H.Description));
		Fill(&H.Size, sizeof(H.Size));
		Fill(&H.Rent, sizeof(H.Rent));
		Fill(&H.DepotNr, sizeof(H.DepotNr));
		Fill(&H.NoAuction, sizeof(H.NoAuction));
		Fill(&H.GuildHouse, sizeof(H.GuildHouse));
		Fill(&H.ExitX, sizeof(H.ExitX));
		Fill(&H.ExitY, sizeof(H.ExitY));
		Fill(&H.ExitZ, sizeof(H.ExitZ));
		Fill(&H.CenterX, sizeof(H.CenterX));
		Fill(&H.CenterY, sizeof(H.CenterY));
		Fill(&H.CenterZ, sizeof(H.CenterZ));
		Fill(&H.OwnerID, sizeof(H.OwnerID));
		Fill(H.OwnerName, sizeof(H.OwnerName));
		Fill(&H.LastTransition, sizeof(H.LastTransition));
		Fill(&H.PaidUntil, sizeof(H.PaidUntil));
		Fill(&H.Subowners, sizeof(H.Subowners));
		Fill(&H.Guests, sizeof(H.Guests));
		Fill(&H.Help, sizeof(H.Help));
	}
	Fill(&HouseAreas, sizeof(HouseAreas));
	Fill(&Houses, sizeof(Houses));
	Fill(&MaxHouseX, sizeof(MaxHouseX));
	Fill(&MaxHouseY, sizeof(MaxHouseY));
}

// A buffer as hex, all of it (Whole) or up to its first zero byte.
static std::string Bytes(const char *Buffer, size_t Size, bool Whole) {
	std::string Out;
	for (size_t i = 0; i < Size; i++) {
		char Code[4];
		snprintf(Code, sizeof(Code), "%02X", static_cast<unsigned char>(Buffer[i]));
		Out += Code;
		if (!Whole && Buffer[i] == 0) {
			break;
		}
	}
	return Out;
}

// A bool as the byte it is, which the pattern may have left as neither 0 nor
// 1.
static std::string Byte(const bool &Value) {
	unsigned char Raw;
	memcpy(&Raw, &Value, 1);
	return std::to_string(Raw);
}

// The tables as "field\tvalue" lines; the buffers whole (Whole) or up to
// their terminating zero (between loads in one process, where what lies
// after it is left from an earlier load). The fields handed to SetHouseID
// are not part of it.
static std::string DumpTables(bool Whole) {
	std::string Out;
	Out += "areas\t" + std::to_string(HouseAreas) + "\n";
	for (int i = 0; i <= HouseAreas && i < POISONED_AREAS; i++) {
		const THouseArea &A = HouseArea(i);
		Out += "area " + std::to_string(i) + "\t" + std::to_string(A.ID) + " " + std::to_string(A.SQMPrice) + " "
				+ std::to_string(A.DepotNr) + "\n";
	}
	Out += "houses\t" + std::to_string(Houses) + "\n";
	for (int i = 0; i <= Houses && i < POISONED_HOUSES; i++) {
		const THouse &H = House(i);
		std::string Prefix = "house " + std::to_string(i) + " ";
		Out += Prefix + "id\t" + std::to_string(H.ID) + "\n";
		Out += Prefix + "name\t" + Bytes(H.Name, sizeof(H.Name), Whole) + "\n";
		Out += Prefix + "description\t" + Bytes(H.Description, sizeof(H.Description), Whole) + "\n";
		Out += Prefix + "size\t" + std::to_string(H.Size) + "\n";
		Out += Prefix + "rent\t" + std::to_string(H.Rent) + "\n";
		Out += Prefix + "depot\t" + std::to_string(H.DepotNr) + "\n";
		Out += Prefix + "no auction\t" + Byte(H.NoAuction) + "\n";
		Out += Prefix + "guild house\t" + Byte(H.GuildHouse) + "\n";
		Out += Prefix + "exit\t" + std::to_string(H.ExitX) + " " + std::to_string(H.ExitY) + " "
				+ std::to_string(H.ExitZ) + "\n";
		Out += Prefix + "center\t" + std::to_string(H.CenterX) + " " + std::to_string(H.CenterY) + " "
				+ std::to_string(H.CenterZ) + "\n";
		Out += Prefix + "owner id\t" + std::to_string(H.OwnerID) + "\n";
		Out += Prefix + "owner name\t" + Bytes(H.OwnerName, sizeof(H.OwnerName), Whole) + "\n";
		Out += Prefix + "last transition\t" + std::to_string(H.LastTransition) + "\n";
		Out += Prefix + "paid until\t" + std::to_string(H.PaidUntil) + "\n";
		Out += Prefix + "subowners\t" + std::to_string(H.Subowners) + "\n";
		Out += Prefix + "guests\t" + std::to_string(H.Guests) + "\n";
		Out += Prefix + "help\t" + std::to_string(H.Help) + "\n";
	}
	Out += "max house x\t" + std::to_string(MaxHouseX) + "\n";
	Out += "max house y\t" + std::to_string(MaxHouseY) + "\n";
	return Out;
}

// The fields handed to SetHouseID as "field\tvalue" lines.
static std::string DumpFieldCalls() {
	std::string Out = "fields\t" + std::to_string(FieldCalls.size() / 4) + "\n";
	for (size_t i = 0; i + 3 < FieldCalls.size(); i += 4) {
		Out += "field " + std::to_string(i / 4) + "\t" + std::to_string(FieldCalls[i]) + " "
				+ std::to_string(FieldCalls[i + 1]) + " " + std::to_string(FieldCalls[i + 2]) + " house "
				+ std::to_string(FieldCalls[i + 3]) + "\n";
	}
	return Out;
}

static std::string Snapshot(const std::function<void()> &Load) {
	return InChild([&]() {
		Poison();
		FieldCalls.clear();
		ErrorLines.clear();
		SetErrorFunction(RecordError);
		Load();
		return DumpTables(true) + DumpFieldCalls() + "errors\t" + ErrorLines + "\n";
	});
}

// The old way: LoadHouseAreas, then LoadHouses, which marked each field on
// the map with SetHouseID as it read it; the old reader notes the fields, in
// that order.
static std::function<void()> OldLoader(const fs::path &Areas, const fs::path &HousesDat) {
	return [Areas, HousesDat]() {
		THousesDatExtras Extras;
		ReadHouseAreasDat(Areas.c_str(), Extras);
		ReadHousesDat(HousesDat.c_str(), Extras);
		for (int i = 0; i < Houses; i++) {
			for (const THouseDatField &Field : Extras.Houses[static_cast<size_t>(i)].Fields) {
				RecordField(Field.x, Field.y, Field.z, House(i).ID);
			}
		}
	};
}

// The game's way: LoadHouses hands SetHouseID to LoadHouseData.
static std::function<void()> NewLoader(const fs::path &File) {
	return [File]() {
		LoadHouseData(File.c_str(), RecordField);
	};
}

// The houses.toml patella-convert makes of a houseareas.dat and a houses.dat.
static std::string Convert(const fs::path &Areas, const fs::path &HousesDat) {
	return InChild([&]() {
		SetErrorFunction(RecordError);
		THousesDatExtras Extras;
		ReadHouseAreasDat(Areas.c_str(), Extras);
		ReadHousesDat(HousesDat.c_str(), Extras);
		return HousesToToml(Extras);
	});
}

// Converts the two texts, loads both ways and compares. Returns the
// houses.toml text.
static std::string RoundTrip(const fs::path &Folder, const char *Name, const std::string &AreasDat,
		const std::string &HousesDat) {
	fs::path Areas = Folder / (std::string(Name) + "-houseareas.dat");
	fs::path Original = Folder / (std::string(Name) + "-houses.dat");
	fs::path Converted = Folder / (std::string(Name) + ".toml");
	WriteFile(Areas, AreasDat);
	WriteFile(Original, HousesDat);
	std::string Text = Convert(Areas, Original);
	if (IsError(Text)) {
		fprintf(stderr, "%s: conversion: %s\n", Name, Text.c_str());
		Failures++;
		return Text;
	}
	WriteFile(Converted, Text);
	std::string Expected = Snapshot(OldLoader(Areas, Original));
	std::string Actual = Snapshot(NewLoader(Converted));
	if (IsError(Expected) || IsError(Actual)) {
		fprintf(stderr, "%s: a load failed:\n%s\n%s\n", Name, Expected.substr(0, 300).c_str(),
				Actual.substr(0, 300).c_str());
		Failures++;
		return Text;
	}
	if (!Same(Expected, Actual, Name, "houses.dat", "houses.toml")) {
		fprintf(stderr, "%s: houses.toml was:\n%s", Name, Text.c_str());
	}
	return Text;
}

// The conversion of the two texts must fail with an error containing What.
static void ExpectRefusal(const fs::path &Folder, const std::string &AreasDat, const std::string &HousesDat,
		const char *What) {
	fs::path Areas = Folder / "refused-houseareas.dat";
	fs::path Original = Folder / "refused-houses.dat";
	WriteFile(Areas, AreasDat);
	WriteFile(Original, HousesDat);
	std::string Text = Convert(Areas, Original);
	if (!IsError(Text) || Text.find(What) == std::string::npos) {
		fprintf(stderr, "expected a refusal \"%s\" for:\n%s\n%s\ngot: %s\n", What, AreasDat.c_str(),
				HousesDat.c_str(), Text.substr(0, 300).c_str());
		Failures++;
	}
}

// One house of houses.dat.
static std::string DatHouse(const std::string &ID, const std::string &Name, const std::string &Description,
		const std::string &RentOffset, const std::string &Area, const std::string &GuildHouse,
		const std::string &Exit, const std::string &Center, const std::string &Fields) {
	return "ID = " + ID + "\nName = \"" + Name + "\"\nDescription = \"" + Description + "\"\nRentOffset = "
			+ RentOffset + "\nArea = " + Area + "\nGuildHouse = " + GuildHouse + "\nExit = " + Exit
			+ "\nCenter = " + Center + "\nFields = {" + Fields + "}\n\n";
}

static void TestRoundTrips(const fs::path &Folder) {
	// Comments, names and texts with escapes, Latin-1 and control
	// characters, an area number that wraps (70000 is 4464), numbers at their
	// limits, a rent that wraps, a house number that wraps (65537 is 1),
	// misnamed keys, GuildHouse in upper case and as neither true nor false,
	// fields with doubled and missing commas, twice in a house and in two
	// houses, no fields, exits and centers with x 0 (error lines), the
	// longest name and description, coordinates at their limits.
	std::string Areas =
		"# Tibia - graphical Multi User Dungeon\n"
		"# houseareas.dat: Gebiete\n"
		"\n"
		"Area = (100,\"Wilderness\",45,0)\n"
		"Area = (200,\"Gr\xfcne \\\"Wiese\\\" \\\\ Nord\",-5,1)\n"
		"Area=(0,\"\",2147483647,-3)\n"
		"Area = (70000,\"wraps\tto 4464\\n\",10,8)\n"
		"Gebiet = (300,\"misnamed\",-2147483648,2147483647)\n";
	std::string Houses =
		"# houses.dat\r\n"
		+ DatHouse("65537", "Wraps to One", "", "-100", "100", "true", "[32000,32000,7]", "[32001,32001,7]",
				"[32000,32000,7],[32001,32000,7],,[32002,32000,7] [32000,32000,7]")
		+ "Nummer = 10\nNom = \"Latin-1 \xe4\xf6\xfc\xdf\"\nText = \"a \\\"quoted\\\" description\\nwith two "
		"lines \\\\ and a\ttab\r\"\nOffset = 2147483647\nGebiet = 200\nGilde = TRUE\nAusgang = [0,0,0]\n"
		"Mitte = [0,32000,15]\nFelder = {}\n\n"
		+ DatHouse("11", std::string(49, 'N'), std::string(499, 'D'), "0", "70000", "yes", "[65535,65535,15]",
				"[32768,32768,8]", "[65535,65535,15],[0,0,0],[32000,32000,7]")
		+ DatHouse("12", "Rent Wraps", "x", "7", "0", "false", "[32100,32100,7]", "[32100,32100,0]",
				"[32100,32100,7],[32101,32100,7],[32100,32090,7]")
		+ DatHouse("40000", "Minimum Price", "y", "-2147483648", "300", "true", "[32000,32000,7]",
				"[32000,32000,7]", "[31000,33000,7]");
	std::string Text = RoundTrip(Folder, "everything", Areas, Houses);
	CHECK(Text.find("area = 200  # Gr\xc3\xbcne \"Wiese\" \\ Nord\n") != std::string::npos);
	CHECK(Text.find("area = 4464  # wraps to 4464 \n") != std::string::npos);
	CHECK(Text.find("name = \"wraps\\tto 4464\\n\"\n") != std::string::npos);
	CHECK(Text.find("fields = []\n") != std::string::npos);
	CHECK(Text.find("\n[[house]]\nid = 1\n") != std::string::npos);

	// Nothing; areas without houses.
	RoundTrip(Folder, "empty", "", "");
	RoundTrip(Folder, "areas-only", Areas, "# no houses\n");

	// More areas and houses than the tables first hold, and one house of
	// many fields over several lines.
	std::string ManyAreas, ManyHouses;
	for (int i = 0; i < 120; i++) {
		ManyAreas += "Area = (" + std::to_string(1000 + i) + ",\"Area " + std::to_string(i) + "\","
				+ std::to_string(i * 7 - 50) + "," + std::to_string(i % 9) + ")\n";
	}
	for (int i = 0; i < 250; i++) {
		std::string Fields;
		for (int j = 0; j < i % 13; j++) {
			Fields += (j > 0 ? "," : "") + std::string("[") + std::to_string(32000 + i) + ","
					+ std::to_string(31000 + j * (i % 5)) + "," + std::to_string(j % 16) + "]";
		}
		ManyHouses += DatHouse(std::to_string(100 + i * 3), "House " + std::to_string(i), "", std::to_string(i * 11),
				std::to_string(1000 + (i * 37) % 120), i % 3 == 0 ? "true" : "false",
				"[" + std::to_string(32000 + i) + ",31000,7]", "[" + std::to_string(32001 + i) + ",31002,6]", Fields);
	}
	std::string Big;
	for (int j = 0; j < 2000; j++) {
		Big += (j > 0 ? "," : "") + std::string("[") + std::to_string(33000 + j % 50) + ","
				+ std::to_string(32000 + j / 50) + "," + std::to_string(j % 3 + 5) + "]";
	}
	ManyHouses += DatHouse("60000", "Big", "a big one", "0", "1000", "true", "[33000,32000,7]", "[33025,32020,6]",
			Big);
	RoundTrip(Folder, "many", ManyAreas, ManyHouses);
}

static void TestRefusals(const fs::path &Folder) {
	std::string Areas = "Area = (100,\"A\",10,0)\n";
	auto One = [](const std::string &ID, const std::string &Name, const std::string &Description,
			const std::string &Area, const std::string &Exit, const std::string &Fields) {
		return DatHouse(ID, Name, Description, "0", Area, "false", Exit, "[32000,32000,7]", Fields);
	};
	ExpectRefusal(Folder, Areas, One("1", std::string(50, 'N'), "", "100", "[32000,32000,7]", ""),
			"name longer than 49");
	ExpectRefusal(Folder, Areas, One("1", "N", std::string(500, 'D'), "100", "[32000,32000,7]", ""),
			"description longer than 499");
	ExpectRefusal(Folder, Areas + "Area = (100,\"B\",20,1)\n", One("1", "N", "", "100", "[32000,32000,7]", ""),
			"an area number twice");
	ExpectRefusal(Folder, Areas + "Area = (65636,\"wraps to 100\",20,1)\n", "", "an area number twice");
	ExpectRefusal(Folder, Areas, One("1", "N", "", "100", "[32000,32000,7]", "[-1,32000,7]"),
			"a coordinate outside");
	ExpectRefusal(Folder, Areas, One("1", "N", "", "100", "[32000,32000,16]", ""), "a coordinate outside");
	ExpectRefusal(Folder, Areas, One("1", "N", "", "100", "[65536,32000,7]", ""), "a coordinate outside");
	ExpectRefusal(Folder, Areas, DatHouse("1", "N", "", "0", "100", "false", "[32000,32000,7]",
			"[32000,32000,-1]", ""), "a coordinate outside");
	// The old reader's own errors.
	ExpectRefusal(Folder, Areas, One("0", "N", "", "100", "[32000,32000,7]", ""), "Cannot load houses");
	ExpectRefusal(Folder, Areas, One("65536", "N", "", "100", "[32000,32000,7]", ""), "Cannot load houses");
	ExpectRefusal(Folder, Areas, One("5", "N", "", "100", "[32000,32000,7]", "")
			+ One("5", "M", "", "100", "[32000,32000,7]", ""), "Cannot load houses");
	ExpectRefusal(Folder, Areas, One("5", "N", "", "100", "[32000,32000,7]", "")
			+ One("4", "M", "", "100", "[32000,32000,7]", ""), "Cannot load houses");
	ExpectRefusal(Folder, Areas, One("1", "N", "", "101", "[32000,32000,7]", ""), "Cannot load houses");
	ExpectRefusal(Folder, Areas, One("1", "N", "", "100", "[32000,32000,7]", "32000"), "coordinates expected");
	ExpectRefusal(Folder, "Area = 100\n", "", "special-char expected");

	fs::path Missing = Folder / "missing.dat";
	fs::path AreasFile = Folder / "refused-houseareas.dat";
	WriteFile(AreasFile, Areas);
	CHECK(IsError(Convert(AreasFile, Missing)));
	CHECK(IsError(Convert(Missing, AreasFile)));
}

// A houses.toml that loads, line by line, for the malformed variants.
static const char *const GoodToml[] = {
	"[[area]]",                    // 1
	"id = 100",                    // 2
	"name = \"A\"",                // 3
	"sqm_price = 10",              // 4
	"depot = 0",                   // 5
	"",                            // 6
	"[[house]]",                   // 7
	"id = 1",                      // 8
	"name = \"H\"",                // 9
	"description = \"D\"",         // 10
	"rent_offset = 0",             // 11
	"area = 100",                  // 12
	"guild_house = false",         // 13
	"exit = [100, 100, 7]",        // 14
	"center = [100, 100, 7]",      // 15
	"fields = [[100, 100, 7]]",    // 16
};
static const int GOOD_TOML_LINES = sizeof(GoodToml) / sizeof(GoodToml[0]);

// GoodToml with line Line (from 1) replaced by Text ("" leaves it empty),
// and Extra after it all.
static std::string Variant(int Line, const std::string &Text, const std::string &Extra = "") {
	std::string Out;
	for (int i = 0; i < GOOD_TOML_LINES; i++) {
		Out += (i + 1 == Line ? Text : std::string(GoodToml[i])) + "\n";
	}
	return Out + Extra;
}

static void TestMalformed(const fs::path &Folder) {
	fs::path File = Folder / "malformed.toml";
	auto Load = [&](const std::string &Text) {
		WriteFile(File, Text);
		return Snapshot(NewLoader(File));
	};
	auto Expect = [&](const std::string &Text, int Line, const char *What) {
		ExpectError(Load, File, Text, Line, What);
	};

	// The good one loads.
	CHECK(!IsError(Load(Variant(0, ""))));

	std::string NextHouse = "\n[[house]]\nid = 2\nname = \"H2\"\ndescription = \"\"\nrent_offset = 0\n"
			"area = 100\nguild_house = true\nexit = [1, 1, 1]\ncenter = [1, 1, 1]\nfields = []\n";
	std::string NextArea = "\n[[area]]\nid = 200\nname = \"B\"\nsqm_price = 1\ndepot = 1\n";

	// The top level and the entries.
	Expect(Variant(0, "", "\n[paths]\ndata = 1\n"), 18, "unknown key \"paths\" in the top level");
	Expect("houses = 1\n", 1, "unknown key \"houses\" in the top level");
	Expect("area = 1\n", 1, "area must be an array");
	Expect("house = [1]\n", 1, "an entry of house must be a table");
	Expect("area = [\"a\"]\n", 1, "an entry of area must be a table");

	// [[area]].
	Expect(Variant(5, "depot = 0\nprice = 1"), 6, "unknown key \"price\" in [[area]]");
	Expect(Variant(2, ""), 1, "an area needs id");
	Expect(Variant(3, ""), 1, "[[area]] 100 needs name");
	Expect(Variant(4, ""), 1, "[[area]] 100 needs sqm_price");
	Expect(Variant(5, ""), 1, "[[area]] 100 needs depot");
	Expect(Variant(2, "id = -1"), 2, "id must lie between 0 and 65535");
	Expect(Variant(2, "id = 65536"), 2, "id must lie between 0 and 65535");
	Expect(Variant(2, "id = \"100\""), 2, "id must be an integer");
	Expect(Variant(0, "", NextArea + "\n[[area]]\nid = 100\nname = \"C\"\nsqm_price = 1\ndepot = 1\n"), 25,
			"area 100 is defined twice");
	Expect(Variant(3, "name = 1"), 3, "name must be a Latin-1 string");
	Expect(Variant(3, "name = \"\xe2\x82\xac\""), 3, "name must be a Latin-1 string");
	Expect(Variant(4, "sqm_price = 1.5"), 4, "sqm_price must be an integer");
	Expect(Variant(4, "sqm_price = 2147483648"), 4, "sqm_price must lie between");
	Expect(Variant(5, "depot = -2147483649"), 5, "depot must lie between");

	// [[house]].
	Expect(Variant(16, "fields = []\nowner = \"x\""), 17, "unknown key \"owner\" in [[house]]");
	Expect(Variant(8, ""), 7, "a house needs id");
	Expect(Variant(9, ""), 7, "[[house]] 1 needs name");
	Expect(Variant(10, ""), 7, "[[house]] 1 needs description");
	Expect(Variant(11, ""), 7, "[[house]] 1 needs rent_offset");
	Expect(Variant(12, ""), 7, "[[house]] 1 needs area");
	Expect(Variant(13, ""), 7, "[[house]] 1 needs guild_house");
	Expect(Variant(14, ""), 7, "[[house]] 1 needs exit");
	Expect(Variant(15, ""), 7, "[[house]] 1 needs center");
	Expect(Variant(16, ""), 7, "[[house]] 1 needs fields");
	Expect(Variant(8, "id = 0"), 8, "id must lie between 1 and 65535");
	Expect(Variant(8, "id = 65536"), 8, "id must lie between 1 and 65535");
	Expect(Variant(8, "id = 2", NextHouse), 19, "house 2 comes after house 2: the ids must ascend");
	Expect(Variant(8, "id = 3", NextHouse), 19, "house 2 comes after house 3: the ids must ascend");
	Expect(Variant(9, "name = \"" + std::string(50, 'N') + "\""), 9, "name is longer than 49 characters");
	Expect(Variant(10, "description = \"" + std::string(500, 'D') + "\""), 10,
			"description is longer than 499 characters");
	Expect(Variant(9, "name = \"a\\u0000b\""), 9, "name has a zero character");
	Expect(Variant(10, "description = \"\xe2\x82\xac\""), 10, "description has a character outside Latin-1");
	Expect(Variant(9, "name = 5"), 9, "name must be a string");
	Expect(Variant(11, "rent_offset = \"0\""), 11, "rent_offset must be an integer");
	Expect(Variant(11, "rent_offset = 2147483648"), 11, "rent_offset must lie between");
	Expect(Variant(12, "area = 101"), 12, "area 101 does not exist");
	Expect(Variant(12, "area = -1"), 12, "area must lie between 0 and 65535");
	Expect(Variant(13, "guild_house = \"true\""), 13, "guild_house must be true or false");
	Expect(Variant(13, "guild_house = 1"), 13, "guild_house must be true or false");
	Expect(Variant(14, "exit = 5"), 14, "exit must be an array [x, y, z]");
	Expect(Variant(14, "exit = [100, 100]"), 14, "exit must be an array [x, y, z]");
	Expect(Variant(14, "exit = [100, 100, 7, 1]"), 14, "exit must be an array [x, y, z]");
	Expect(Variant(14, "exit = [-1, 100, 7]"), 14, "x of exit must lie between 0 and 65535");
	Expect(Variant(14, "exit = [100, 65536, 7]"), 14, "y of exit must lie between 0 and 65535");
	Expect(Variant(14, "exit = [100, 100, 16]"), 14, "z of exit must lie between 0 and 15");
	Expect(Variant(14, "exit = [100, 100, \"7\"]"), 14, "z of exit must be an integer");
	Expect(Variant(15, "center = [100, 100, -1]"), 15, "z of center must lie between 0 and 15");
	Expect(Variant(15, "center = {x = 1}"), 15, "center must be an array [x, y, z]");
	Expect(Variant(16, "fields = 5"), 16, "fields must be an array");
	Expect(Variant(16, "fields = [100, 100, 7]"), 16, "a field must be an array [x, y, z]");
	Expect(Variant(16, "fields = [[100, 100, 7], [1, 2]]"), 16, "a field must be an array [x, y, z]");
	Expect(Variant(16, "fields = [[100, 100, 7],\n[1, 2, 16]]"), 17, "z of a field must lie between 0 and 15");
	Expect(Variant(16, "fields = [[1.0, 2, 3]]"), 16, "x of a field must be an integer");
	Expect(Variant(16, "fields = [[100, 100, 7]"), 16, "");

	// What loads: the keys in another order, the areas after the houses, the
	// limits, no fields, the longest name and description, an area without
	// houses, an empty file.
	CHECK(!IsError(Load(
			"[[house]]\nfields = []\ncenter = [0, 0, 0]\nexit = [65535, 65535, 15]\nguild_house = true\n"
			"area = 0\nrent_offset = -2147483648\ndescription = \"" + std::string(499, 'D') + "\"\nname = \""
			+ std::string(49, 'N') + "\"\nid = 65535\n\n[[area]]\ndepot = -2147483648\nsqm_price = 2147483647\n"
			"name = \"\"\nid = 0\n\n[[area]]\nid = 65535\nname = \"unused\"\nsqm_price = 0\ndepot = 0\n")));
	std::string Empty = Load("");
	CHECK(!IsError(Empty) && Empty.find("houses\t0\n") != std::string::npos);
	std::string Logged = Load(Variant(14, "exit = [0, 100, 7]"));
	CHECK(Logged.find("errors\tLoadHouses: Ausgang f\xfcr Haus 1 nicht gesetzt.\n") != std::string::npos);
}

// The tables in use, loaded from the parsed File in this process: the dump
// with the buffers up to their terminating zero, the fields handed to
// SetHouseID and the error lines, or the error.
static std::string LoadInProcess(const toml::table &File, std::vector<int> &Fields) {
	FieldCalls.clear();
	ErrorLines.clear();
	try {
		ReadHouseData(File, RecordField);
	} catch (const char *Error) {
		Fields.clear();
		return std::string("error: ") + Error;
	}
	Fields = FieldCalls;
	return DumpTables(false) + "errors\t" + ErrorLines + "\n";
}

// Every change of one value or entry of the file, made in the parsed file
// and undone after, must give other tables in use or an error. Changes: each
// area's id, sqm_price and depot one up, each area left out and swapped with
// the next; each house's id and rent_offset one up, its name and description
// lengthened, guild_house turned, its area changed to the next area that
// differs in price or depot and to one that does not exist, each component
// of exit and center one up, the house left out and its first field left
// out; one component of each field one up (x, y and z by turns). Returns the
// number of changes made.
static int TestMutations(toml::table &File, const std::string &Expected, const std::vector<int> &ExpectedFields) {
	int Changes = 0;
	std::string Where;
	std::vector<int> Fields;
	auto Try = [&](const std::string &What) {
		Changes++;
		std::string Actual = LoadInProcess(File, Fields);
		if (Actual == Expected && Fields == ExpectedFields) {
			fprintf(stderr, "mutation not caught: %s: %s\n", Where.c_str(), What.c_str());
			Failures++;
		}
	};
	auto ChangeInteger = [&](toml::node &Node, const std::string &What) {
		int64_t Old = Node.as_integer()->get();
		Node.as_integer()->get() = Old + 1;
		Try(What + " one up");
		Node.as_integer()->get() = Old;
	};
	auto Lengthen = [&](toml::node &Node, const std::string &What) {
		std::string Old = Node.as_string()->get();
		Node.as_string()->get() = Old + "x";
		Try(What + " lengthened");
		Node.as_string()->get() = Old;
	};
	// Putting an entry back makes a new node of it.
	auto LeaveOutTable = [&](toml::array &Array, size_t Index, const std::string &What) {
		toml::table Saved = *Array.get(Index)->as_table();
		Array.erase(Array.cbegin() + static_cast<std::ptrdiff_t>(Index));
		Try(What + " left out");
		Array.insert(Array.cbegin() + static_cast<std::ptrdiff_t>(Index), std::move(Saved));
	};
	auto LeaveOutArray = [&](toml::array &Array, size_t Index, const std::string &What) {
		toml::array Saved = *Array.get(Index)->as_array();
		Array.erase(Array.cbegin() + static_cast<std::ptrdiff_t>(Index));
		Try(What + " left out");
		Array.insert(Array.cbegin() + static_cast<std::ptrdiff_t>(Index), std::move(Saved));
	};

	toml::array &Areas = *File.get("area")->as_array();
	toml::array &HouseList = *File.get("house")->as_array();
	for (size_t a = 0; a < Areas.size(); a++) {
		Where = "area " + std::to_string(a + 1);
		toml::table &Area = *Areas.get(a)->as_table();
		for (const char *Key : {"id", "sqm_price", "depot"}) {
			ChangeInteger(*Area.get(Key), Key);
		}
		LeaveOutTable(Areas, a, "the area");
		if (a + 1 < Areas.size()) {
			std::swap(*Areas.get(a)->as_table(), *Areas.get(a + 1)->as_table());
			Try("swapped with the next");
			std::swap(*Areas.get(a)->as_table(), *Areas.get(a + 1)->as_table());
		}
	}

	// The areas by id, for the changes of a house's area.
	auto AreaPrice = [&](int64_t ID, int64_t &Price, int64_t &Depot) {
		for (const toml::node &Node : Areas) {
			const toml::table &Area = *Node.as_table();
			if (Area.get("id")->as_integer()->get() == ID) {
				Price = Area.get("sqm_price")->as_integer()->get();
				Depot = Area.get("depot")->as_integer()->get();
				return true;
			}
		}
		return false;
	};

	for (size_t h = 0; h < HouseList.size(); h++) {
		toml::table &Entry = *HouseList.get(h)->as_table();
		Where = "house " + std::to_string(Entry.get("id")->as_integer()->get());
		ChangeInteger(*Entry.get("id"), "id");
		ChangeInteger(*Entry.get("rent_offset"), "rent_offset");
		Lengthen(*Entry.get("name"), "name");
		Lengthen(*Entry.get("description"), "description");

		bool &Guild = Entry.get("guild_house")->as_boolean()->get();
		Guild = !Guild;
		Try("guild_house turned");
		Guild = !Guild;

		int64_t &AreaID = Entry.get("area")->as_integer()->get();
		int64_t OldArea = AreaID, Price = 0, Depot = 0;
		AreaPrice(OldArea, Price, Depot);
		for (const toml::node &Node : Areas) {
			const toml::table &Area = *Node.as_table();
			if (Area.get("sqm_price")->as_integer()->get() != Price
					|| Area.get("depot")->as_integer()->get() != Depot) {
				AreaID = Area.get("id")->as_integer()->get();
				Try("area to " + std::to_string(AreaID));
				break;
			}
		}
		int64_t None = 0, Unused = 0;
		while (AreaPrice(None, Unused, Unused)) {
			None++;
		}
		AreaID = None;
		Try("area to " + std::to_string(None) + ", which does not exist");
		AreaID = OldArea;

		for (const char *Key : {"exit", "center"}) {
			toml::array &Coordinate = *Entry.get(Key)->as_array();
			for (size_t i = 0; i < 3; i++) {
				ChangeInteger(*Coordinate.get(i), std::string(Key) + " component " + std::to_string(i + 1));
			}
		}

		toml::array &FieldList = *Entry.get("fields")->as_array();
		for (size_t f = 0; f < FieldList.size(); f++) {
			ChangeInteger(*FieldList.get(f)->as_array()->get(f % 3),
					"field " + std::to_string(f + 1) + " component " + std::to_string(f % 3 + 1));
		}
		if (!FieldList.empty()) {
			LeaveOutArray(FieldList, 0, "the first field");
		}

		LeaveOutTable(HouseList, h, "the house");
	}
	return Changes;
}

static int Original(const char *AreasDat, const char *HousesDat, const char *HousesToml) {
	if (!fs::is_regular_file(AreasDat) || !fs::is_regular_file(HousesDat)) {
		printf("skipped: no %s or %s\n", AreasDat, HousesDat);
		return 77;
	}

	std::string Text = Convert(AreasDat, HousesDat);
	std::string Committed = ReadFile(HousesToml);
	if (Committed != Text) {
		fprintf(stderr, "%s is not what patella-convert houses makes of %s and %s (%s)\n", HousesToml, AreasDat,
				HousesDat, IsError(Text) ? Text.c_str() : "a different text");
		Failures++;
	}

	std::string Expected = Snapshot(OldLoader(AreasDat, HousesDat));
	std::string Actual = Snapshot(NewLoader(HousesToml));
	if (IsError(Expected) || IsError(Actual)) {
		fprintf(stderr, "a load failed: %s / %s\n", Expected.substr(0, 200).c_str(), Actual.substr(0, 200).c_str());
		return 1;
	}
	if (Same(Expected, Actual, "original", "houseareas.dat and houses.dat", "houses.toml")) {
		std::map<std::string, std::string> Values = Fields(Actual);
		printf("%s, %s and %s give the same tables: %s areas, %s houses, %s fields, max house x/y %s/%s, "
				"errors logged \"%s\"; %d values compared\n", AreasDat, HousesDat, HousesToml,
				Values["areas"].c_str(), Values["houses"].c_str(), Values["fields"].c_str(),
				Values["max house x"].c_str(), Values["max house y"].c_str(), Values["errors"].c_str(),
				static_cast<int>(Values.size()));
	}

	toml::table File = TomlParseFile(HousesToml);
	SetErrorFunction(RecordError);
	std::vector<int> InUseFields;
	std::string InUse = LoadInProcess(File, InUseFields);
	if (IsError(InUse)) {
		fprintf(stderr, "%s\n", InUse.c_str());
		return 1;
	}
	int Changes = TestMutations(File, InUse, InUseFields);
	std::vector<int> AfterFields;
	CHECK(LoadInProcess(File, AfterFields) == InUse && AfterFields == InUseFields);
	printf("%d changes of single values or entries of %s, each caught\n", Changes, HousesToml);
	return Failures == 0 ? 0 : 1;
}

int main(int argc, char **argv) {
	if (argc == 5 && strcmp(argv[1], "--original") == 0) {
		int Result = 1;
		try {
			Result = Original(argv[2], argv[3], argv[4]);
		} catch (const char *Error) {
			fprintf(stderr, "unexpected error: %s\n", Error);
			Failures++;
		}
		if (Failures != 0) {
			fprintf(stderr, "%d failures\n", Failures);
			Result = 1;
		}
		return Result;
	}
	if (argc != 1) {
		fprintf(stderr, "usage: convert_test_houses [--original HOUSEAREAS_DAT HOUSES_DAT HOUSES_TOML]\n");
		return 2;
	}

	fs::path Folder = TemporaryFolder("convert-test-houses");
	TestRoundTrips(Folder);
	TestRefusals(Folder);
	TestMalformed(Folder);
	fs::remove_all(Folder);
	if (Failures != 0) {
		fprintf(stderr, "%d failures\n", Failures);
		return 1;
	}
	printf("all houses round trips passed\n");
	return 0;
}
