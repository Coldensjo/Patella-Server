// test_items.cc: the round trip of the object types. The object type table the
// game ends up with from an objects.srv, through the old reader
// (ReadObjectsSrv), and from the items.toml patella-convert makes of it,
// through the game's LoadItems, must be the same, field by field; LoadItems
// must refuse what is wrong with the file and line. Each items.toml must end
// with the list [[use]] of the built-in uses of objects, which LoadItems
// reads in the order of the game's code (flags, scripts, places), and every
// change of one of its entries must show.
//
//   convert_test_items
//     With made-up files in a temporary folder.
//   convert_test_items --original OBJECTS_SRV ITEMS_TOML
//     With the original objects.srv and the committed items.toml: ITEMS_TOML
//     must be exactly what patella-convert makes of OBJECTS_SRV, and give the
//     same table and list [[use]]; each script of the list must be a file in
//     the scripts folder next to ITEMS_TOML that defines onUse. Prints the
//     load times of both. Exit code 77 (skipped) when OBJECTS_SRV is not
//     there.
//
// Every load runs in a child process of its own, so that each starts from the
// table as the game starts with it. The table is compared twice: once as it
// is (its index range, which grows with the highest type), and once grown to
// all 65536 types and filled with a byte pattern before the load, so that the
// entries a loader fills, and only those, stand out; each filled entry is
// compared field by field (name, description, flags, the 62 type attributes,
// the 18 instance attribute offsets), and so are the special objects.

#include <algorithm>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <functional>
#include <map>
#include <sstream>
#include <string>
#include <vector>

#include <sys/wait.h>
#include <unistd.h>

#include "items_writer.hh"
#include "objects.hh"
#include "objects_srv.hh"
#include "tomlfile.hh"
#include "tomlwrite.hh"

#include "test_common.hh"

static const unsigned char POISON = 0xA5;
static const int ALL_TYPES = 65536;

static std::string Printable(const std::string &Text) {
	std::string Out;
	for (unsigned char c : Text) {
		if (c >= 0x20 && c < 0x7F && c != '\\') {
			Out += static_cast<char>(c);
		} else {
			char Code[8];
			snprintf(Code, sizeof(Code), "\\x%02X", c);
			Out += Code;
		}
	}
	return Out;
}

// The two loaders, each filling the game's ObjectTypes and SpecialObjects.
static std::function<void()> OldLoader(const fs::path &File) {
	return [File]() {
		TObjectsSrvExtras Extras;
		ReadObjectsSrv(File.c_str(), Extras);
	};
}

static std::function<void()> NewLoader(const fs::path &File) {
	return [File]() {
		LoadItems(File.c_str());
	};
}

static bool Untouched(const TObjectType &Type) {
	const unsigned char *Bytes = reinterpret_cast<const unsigned char *>(&Type);
	for (size_t i = 0; i < sizeof(Type); i++) {
		if (Bytes[i] != POISON) {
			return false;
		}
	}
	return true;
}

// The table as "field\tvalue" lines: the filled entries field by field, the
// number of entries left alone, and the special objects.
static std::string DumpTable() {
	std::string Out;
	char Line[256];
	int Alone = 0;
	for (int TypeID = 0; TypeID < ALL_TYPES; TypeID++) {
		const TObjectType &Type = ObjectTypes(TypeID);
		if (Untouched(Type)) {
			Alone++;
			continue;
		}
		std::string Prefix = "type " + std::to_string(TypeID) + " ";
		Out += Prefix + "name\t" + (Type.Name == nullptr ? "(null)" : "\"" + Printable(Type.Name) + "\"") + "\n";
		Out += Prefix + "description\t"
				+ (Type.Description == nullptr ? "(null)" : "\"" + Printable(Type.Description) + "\"") + "\n";
		std::string Flags;
		for (size_t i = 0; i < sizeof(Type.Flags); i++) {
			snprintf(Line, sizeof(Line), "%02X", Type.Flags[i]);
			Flags += Line;
		}
		for (int Flag = 0; Flag < FLAGS; Flag++) {
			if ((Type.Flags[Flag / 8] & (1 << (Flag % 8))) != 0) {
				Flags += std::string(" ") + FlagNames[Flag];
			}
		}
		Out += Prefix + "flags\t" + Flags + "\n";
		for (int i = 0; i < TYPEATTRIBUTES; i++) {
			Out += Prefix + "attribute " + TypeAttributeNames[i] + "\t" + std::to_string(Type.Attributes[i]) + "\n";
		}
		for (int i = 0; i < INSTANCEATTRIBUTES; i++) {
			Out += Prefix + "offset " + std::to_string(i) + "\t" + std::to_string(Type.AttributeOffsets[i]) + "\n";
		}
	}
	Out += "entries left alone\t" + std::to_string(Alone) + "\n";
	for (int Meaning = 0; Meaning < SPECIALMEANINGS; Meaning++) {
		Out += "special object " + std::to_string(Meaning) + "\t"
				+ std::to_string(SpecialObjects[Meaning].getTypeID()) + "\n";
	}
	return Out;
}

// What a loader leaves: the table's index range as the game has it, then the
// whole table grown to every type and filled with POISON before the load.
static std::string Snapshot(const std::function<void()> &Load) {
	std::string Range = InChild([&]() {
		Load();
		return "range\t" + std::to_string(ObjectTypes.low()) + " to " + std::to_string(ObjectTypes.high()) + "\n";
	});
	std::string Table = InChild([&]() {
		ObjectTypes(ALL_TYPES - 1);
		for (int TypeID = 0; TypeID < ALL_TYPES; TypeID++) {
			memset(static_cast<void *>(&ObjectTypes(TypeID)), POISON, sizeof(TObjectType));
		}
		Load();
		return DumpTable();
	});
	return Range + Table;
}

// Prints the fields that differ; returns whether none do. Fields gives the
// number of fields compared.
static bool Same(const std::string &Expected, const std::string &Actual, const char *What, size_t *Compared = nullptr) {
	std::map<std::string, std::string> A = Fields(Expected), B = Fields(Actual);
	int Differences = 0;
	auto Report = [&](const std::string &Key, const std::string &X, const std::string &Y) {
		if (Differences < 30) {
			fprintf(stderr, "%s: %s: objects.srv gives %s, items.toml %s\n", What, Key.c_str(), X.c_str(), Y.c_str());
		}
		Differences++;
	};
	for (const auto &Pair : A) {
		auto Other = B.find(Pair.first);
		if (Other == B.end()) {
			Report(Pair.first, Pair.second, "(nothing)");
		} else if (Other->second != Pair.second) {
			Report(Pair.first, Pair.second, Other->second);
		}
	}
	for (const auto &Pair : B) {
		if (A.find(Pair.first) == A.end()) {
			Report(Pair.first, "(nothing)", Pair.second);
		}
	}
	if (Differences != 0) {
		fprintf(stderr, "%s: %d fields differ\n", What, Differences);
		Failures++;
	}
	if (Compared != nullptr) {
		*Compared = A.size();
	}
	return Differences == 0;
}

// The items.toml patella-convert makes of an objects.srv file.
static std::string Convert(const fs::path &File) {
	return InChild([&]() {
		TObjectsSrvExtras Extras;
		ReadObjectsSrv(File.c_str(), Extras);
		return ItemsToToml(Extras);
	});
}

// The list [[use]] that patella-convert must write, the built-in uses of
// objects in the order in which the game's code tried them (Use of
// operate.cc before the uses became scripts): the flag and the script of
// each entry. Stated here on its own, so that a change of the writer's list
// shows.
static const struct {
	const char *Flag;
	const char *Script;
} ExpectedUses[] = {
	{"container", "items/container.lua"},
	{"chest", "items/chest.lua"},
	{"liquid_container", "items/liquid_container.lua"},
	{"food", "items/food.lua"},
	{"write", "items/text_object.lua"},
	{"write_once", "items/text_object.lua"},
	{"information", "items/announcer.lua"},
	{"rune", "items/rune.lua"},
	{"key", "items/key_door.lua"},
	{"name_door", "items/name_door.lua"},
	{"level_door", "items/level_door.lua"},
	{"quest_door", "items/quest_door.lua"},
	{"weapon", "items/weapon.lua"},
	{"change_use", "items/change_use.lua"},
	{"use_event", "items/use_event.lua"},
	{"text", "items/readable_text.lua"},
};
static const int EXPECTED_USES = static_cast<int>(sizeof(ExpectedUses) / sizeof(ExpectedUses[0]));

// The list [[use]] as the last LoadItems read it, as "field\tvalue" lines:
// the number of entries, and each entry's flag, script, place and script
// number.
static std::string DumpUses() {
	std::string Out = "uses\t" + std::to_string(UseEntries.size()) + "\n";
	for (size_t i = 0; i < UseEntries.size(); i++) {
		std::string Prefix = "use " + std::to_string(i) + " ";
		Out += Prefix + "flag\t" + FlagKeys[UseEntries[i].Flag] + "\n";
		Out += Prefix + "script\t" + UseEntries[i].Script + "\n";
		Out += Prefix + "place\t" + UseEntries[i].Where + "\n";
		Out += Prefix + "script number\t" + std::to_string(UseEntries[i].ScriptNumber) + "\n";
	}
	return Out;
}

// What LoadItems reads of File's list [[use]], in a child (DumpUses, or the
// error).
static std::string LoadUses(const fs::path &File) {
	return InChild([&]() {
		LoadItems(File.c_str());
		return DumpUses();
	});
}

// The lines of a DumpUses dump that say what the list does: the number of
// entries and their flags and scripts, in order (not their places).
static std::string UseOrder(const std::string &Dump) {
	std::istringstream In(Dump);
	std::string Line, Out;
	while (std::getline(In, Line)) {
		size_t Tab = Line.find('\t');
		std::string Field = Line.substr(0, Tab);
		if (Field == "uses" || (Field.size() > 5 && Field.compare(Field.size() - 5, 5, " flag") == 0)
				|| (Field.size() > 7 && Field.compare(Field.size() - 7, 7, " script") == 0)) {
			Out += Line + "\n";
		}
	}
	return Out;
}

// The dump DumpUses must give for the items.toml text Text, the file File:
// ExpectedUses, each entry in the place of its script key, the line of the
// n-th "script = " after the first [[use]], column 1.
static std::string ExpectedUseDump(const fs::path &File, const std::string &Text) {
	std::string Out = "uses\t" + std::to_string(EXPECTED_USES) + "\n";
	size_t At = Text.find("\n[[use]]\n");
	for (int i = 0; i < EXPECTED_USES; i++) {
		std::string Prefix = "use " + std::to_string(i) + " ";
		Out += Prefix + "flag\t" + ExpectedUses[i].Flag + "\n";
		Out += Prefix + "script\t" + ExpectedUses[i].Script + "\n";
		size_t Script = At == std::string::npos ? std::string::npos : Text.find("\nscript = ", At);
		long Line = Script == std::string::npos ? 0
				: std::count(Text.begin(), Text.begin() + static_cast<long>(Script) + 1, '\n') + 1;
		Out += Prefix + "place\t" + File.string() + ":" + std::to_string(Line) + ":1\n";
		Out += Prefix + "script number\t-1\n";
		At = Script == std::string::npos ? std::string::npos : Script + 1;
	}
	return Out;
}

// Checks that File, whose text is Text, ends with the list [[use]], after its
// [[item]] tables, and that LoadItems reads it as ExpectedUses, each entry in
// its place.
static void CheckUses(const fs::path &File, const std::string &Text, const char *What) {
	size_t FirstUse = Text.find("\n[[use]]\n");
	size_t LastItem = Text.rfind("\n[[item]]\n");
	if (FirstUse == std::string::npos || (LastItem != std::string::npos && LastItem > FirstUse)) {
		fprintf(stderr, "%s: the list [[use]] is not at the end of the file\n", What);
		Failures++;
	}
	std::string Actual = LoadUses(File);
	if (IsError(Actual)) {
		fprintf(stderr, "%s: loading the list [[use]]: %s\n", What, Actual.c_str());
		Failures++;
		return;
	}
	Same(ExpectedUseDump(File, Text), Actual, What, "the list", "items.toml");
}

// Every change of one entry of the list [[use]] of Text (its flag changed,
// unknown or left out, its script changed or left out, the entry left out,
// given twice or swapped with the next) must give another list or an error.
// Returns the number of changes made.
static int TestUseMutations(const fs::path &Folder, const std::string &Text) {
	size_t FirstUse = Text.find("\n[[use]]\n");
	if (FirstUse == std::string::npos) {
		fprintf(stderr, "no list [[use]] to change\n");
		Failures++;
		return 0;
	}
	std::string Head = Text.substr(0, FirstUse);
	std::vector<std::string> Blocks;
	for (size_t At = FirstUse; At != std::string::npos;) {
		size_t Next = Text.find("\n[[use]]\n", At + 1);
		Blocks.push_back(Text.substr(At, Next == std::string::npos ? std::string::npos : Next - At));
		At = Next;
	}

	fs::path File = Folder / "mutated-uses.toml";
	WriteFile(File, Text);
	std::string Expected = UseOrder(LoadUses(File));
	int Changes = 0;
	auto Try = [&](size_t Entry, const char *What, const std::vector<std::string> &Changed) {
		std::string Mutated = Head;
		for (const std::string &Block : Changed) {
			Mutated += Block;
		}
		Changes++;
		WriteFile(File, Mutated);
		std::string Actual = LoadUses(File);
		if (Mutated == Text || (!IsError(Actual) && UseOrder(Actual) == Expected)) {
			fprintf(stderr, "mutation not caught: use entry %d: %s\n", static_cast<int>(Entry), What);
			Failures++;
		}
	};
	for (size_t i = 0; i < Blocks.size(); i++) {
		const std::string &Block = Blocks[i];
		size_t Flag = Block.find("\nflag = \"");
		size_t Script = Block.find("\nscript = \"");
		if (Flag == std::string::npos || Script == std::string::npos) {
			fprintf(stderr, "use entry %d has no flag or script line\n", static_cast<int>(i));
			Failures++;
			continue;
		}
		size_t FlagStart = Flag + 9;
		std::string FlagKey = Block.substr(FlagStart, Block.find('"', FlagStart) - FlagStart);
		int FlagNumber = 0;
		while (FlagNumber < FLAGS && FlagKey != FlagKeys[FlagNumber]) {
			FlagNumber++;
		}
		std::string OtherFlag = FlagKeys[(FlagNumber + 1) % FLAGS];
		size_t ScriptEnd = Block.find('"', Script + 11);

		auto With = [&](const std::string &Changed) {
			std::vector<std::string> Result = Blocks;
			Result[i] = Changed;
			return Result;
		};
		std::string Changed = Block;
		Try(i, "flag changed", With(Changed.replace(FlagStart, FlagKey.size(), OtherFlag)));
		Changed = Block;
		Try(i, "flag unknown", With(Changed.insert(FlagStart, "x")));
		Changed = Block;
		Try(i, "flag left out", With(Changed.erase(Flag, Block.find('\n', Flag + 1) - Flag)));
		Changed = Block;
		Try(i, "script changed", With(Changed.insert(ScriptEnd - 4, "x")));
		Changed = Block;
		Try(i, "script left out", With(Changed.erase(Script, Block.find('\n', Script + 1) - Script)));
		Try(i, "left out", With(""));
		Try(i, "given twice", With(Block + Block));
		if (i + 1 < Blocks.size()) {
			std::vector<std::string> Swapped = Blocks;
			std::swap(Swapped[i], Swapped[i + 1]);
			Try(i, "swapped with the next", Swapped);
		}
	}
	return Changes;
}

// Converts an objects.srv text, loads both and compares them. Returns the
// items.toml text.
static std::string RoundTrip(const fs::path &Folder, const char *Name, const std::string &ObjectsSrv) {
	fs::path Original = Folder / (std::string(Name) + ".srv");
	fs::path Converted = Folder / (std::string(Name) + ".toml");
	WriteFile(Original, ObjectsSrv);
	std::string Text = Convert(Original);
	if (Text.compare(0, 7, "error: ") == 0 || Text == "crashed") {
		fprintf(stderr, "%s: conversion: %s\n", Name, Text.c_str());
		Failures++;
		return Text;
	}
	WriteFile(Converted, Text);
	CheckUses(Converted, Text, Name);
	std::string Expected = Snapshot(OldLoader(Original));
	std::string Actual = Snapshot(NewLoader(Converted));
	if (Expected.find("error: ") != std::string::npos || Actual.find("error: ") != std::string::npos) {
		fprintf(stderr, "%s: a load failed:\n%s\n%s\n", Name, Expected.substr(0, 300).c_str(),
				Actual.substr(0, 300).c_str());
		Failures++;
		return Text;
	}
	if (!Same(Expected, Actual, Name)) {
		fprintf(stderr, "%s: items.toml was:\n%s", Name, Text.c_str());
	}
	return Text;
}

static void TestRoundTrips(const fs::path &Folder) {
	// Lower- and mixed-case keys and flags, comments, attributes before the
	// flags, a second Flags line, an attribute given twice, a type defined
	// again, types out of order and above the table's first 5000, gaps, a type
	// without flags, attributes or name, negative and wrapping numbers,
	// Latin-1, escapes, and a chest (whose CONTENT offset comes from Chest).
	std::string Text =
		"# made-up objects.srv\n"
		"TypeID      = 0 # map container\n"
		"Name        = \"\"\n"
		"Flags       = {Container}\n"
		"Attributes  = {Capacity=0}\n"
		"\n"
		"typeid = 3\n"
		"name = \"body\"\n"
		"FLAGS = {container,TAKE}\n"
		"Attributes = {Weight=0,Capacity=1}\n"
		"\n"
		"TypeID = 50\n"
		"Name = \"a \\\"chest\\\", a back\\\\slash, a\\nnew line, \xe4\xf6\xfc\xdf and a\ttab\"\n"
		"Description = \"Latin-1 \xff\xa0 and # no comment\"\n"
		"Flags = {Chest,Container,Take,Unmove}\n"
		"Attributes = {Capacity=4294967295,Weight=-1}\n"
		"\n"
		"TypeID = 60\n"
		"Attributes = {Weight=10}\n"
		"Flags = {Take}\n"
		"Flags = {Cumulative,LiquidContainer}\n"
		"Name = \"second\"\n"
		"\n"
		"TypeID = 70\n"
		"Name = \"first\"\n"
		"Flags = {Bed}\n"
		"Attributes = {Waypoints=3}\n"
		"TypeID = 70\n"
		"Name = \"defined again\"\n"
		"Flags = {Food}\n"
		"Attributes = {Nutrition=7}\n"
		"\n"
		"TypeID = 98\n"
		"Name = \"coin\"\n"
		"Flags = {Cumulative,Take}\n"
		"Attributes = {Weight=1,Meaning=1,Weight=2}\n"
		"\n"
		"TypeID = 99 # creature container\n"
		"Name = \"\"\n"
		"Flags = {Container,Unpass}\n"
		"Attributes = {Capacity=10}\n"
		"\n"
		"TypeID = 5500\n"
		"Name = \"far up\"\n"
		"Flags = {Bottom,TeleportRelative,Unmove,Avoid,Light}\n"
		"Attributes = {RelTeleportEffect=0,RelTeleportDisplacement=-2145189880,AvoidDamageTypes=0,Brightness=2,"
		"LightColor=29}\n"
		"\n"
		"TypeID = 120\n"
		"Name = \"nothing else\"\n"
		"\n"
		"TypeID = 121\n"
		"Flags = {Bank}\n"
		"Attributes = {Waypoints=100}\n"
		"\n"
		"TypeID = 122\n"
		"Name = \"empty lists\"\n"
		"Flags = {}\n"
		"Attributes = {}\n";

	// A type with every flag and every attribute, in reverse order, with
	// values of their own: for an attribute given by name its last name (the
	// meaning the highest there is), all of its bits for an array.
	Text += "\nTypeID = 4000\nName = \"everything\"\nFlags = {";
	for (int Flag = FLAGS - 1; Flag >= 0; Flag--) {
		Text += std::string(FlagNames[Flag]) + (Flag > 0 ? "," : "}\n");
	}
	Text += "Attributes = {";
	for (int Attribute = TYPEATTRIBUTES - 1; Attribute >= 0; Attribute--) {
		int64_t Value = (Attribute % 2 == 0 ? 1 : -1) * (Attribute * 1000 + 7);
		if (const TNamedAttribute *Named = GetNamedAttribute(Attribute)) {
			Value = Named->Form == NAMED_VALUE ? Named->Count - 1
					: Named->Form == NAMED_BIT ? INT64_C(1) << (Named->Count - 1) : (INT64_C(1) << Named->Count) - 1;
		}
		Text += std::string(TypeAttributeNames[Attribute]) + "=" + std::to_string(Value) + (Attribute > 0 ? "," : "}\n");
	}

	std::string Toml = RoundTrip(Folder, "made-up", Text);
	CHECK(Toml.find("\n[[item]]\nid = 3\nname = \"body\"\nflags = [\"container\", \"take\"]\n"
			"attributes = { weight = 0, capacity = 1 }\n") != std::string::npos);
	CHECK(Toml.find("name = \"a \\\"chest\\\", a back\\\\slash, a\\nnew line, \xc3\xa4\xc3\xb6\xc3\xbc\xc3\x9f and "
			"a\\ttab\"\n") != std::string::npos);
	CHECK(Toml.find("attributes = { capacity = -1, weight = -1 }\n") != std::string::npos);
	CHECK(Toml.find("id = 70\nname = \"defined again\"\nflags = [\"food\"]\nattributes = { nutrition = 7 }\n")
			!= std::string::npos);
	CHECK(Toml.find("attributes = { weight = 2, meaning = \"money_one\" }\n") != std::string::npos);
	CHECK(Toml.find("rel_teleport_effect = \"none\"") != std::string::npos);
	CHECK(Toml.find("avoid_damage_types = [], brightness = 2, light_color = 29") != std::string::npos);
	CHECK(Toml.find("meaning = \"rushwood\"") != std::string::npos);
	CHECK(Toml.find("wand_damage_type = \"mana_drain\"") != std::string::npos);
	CHECK(Toml.find("protection_damage_types = [\"physical\", \"poison\", \"fire\", \"energy\", \"unused\", "
			"\"poison_condition\", \"fire_condition\", \"energy_condition\", \"life_drain\", \"mana_drain\"]")
			!= std::string::npos);
	CHECK(Toml.find("professions = [\"none\", \"knight\", \"paladin\", \"sorcerer\", \"druid\"]") != std::string::npos);
	CHECK(Toml.find("body_position = \"ammo\"") != std::string::npos);
	CHECK(Toml.find("rel_teleport_displacement = -2145189880") != std::string::npos);
	CHECK(Toml.find("id = 120\nname = \"nothing else\"\n\n") != std::string::npos);
	CHECK(Toml.find("id = 121\nflags = [\"bank\"]\n") != std::string::npos);
	CHECK(Toml.find("id = 122\nname = \"empty lists\"\n\n") != std::string::npos);
	// The types in order of their numbers.
	CHECK(Toml.find("id = 122\n") < Toml.find("id = 4000\n") && Toml.find("id = 4000\n") < Toml.find("id = 5500\n"));
	// The list [[use]], its entries as the writer writes them, and the
	// header that explains it.
	CHECK(Toml.find("\n\n[[use]]\nflag = \"container\"\nscript = \"items/container.lua\"\n\n[[use]]\nflag = \"chest\"\n")
			!= std::string::npos);
	const std::string LastUse = "\n\n[[use]]\nflag = \"text\"\nscript = \"items/readable_text.lua\"\n";
	CHECK(Toml.size() > LastUse.size() && Toml.compare(Toml.size() - LastUse.size(), LastUse.size(), LastUse) == 0);
	CHECK(Toml.find("# Each [[use]], at the end of the file,") != std::string::npos
			&& Toml.find("# Each [[use]]") < Toml.find("\n[[item]]\n"));
	// Each change of an entry of the list is caught.
	int Changes = TestUseMutations(Folder, Toml);
	CHECK(Changes == EXPECTED_USES * 8 - 1);

	// Each flag and attribute key round trips on its own, too.
	for (int Flag = 0; Flag < FLAGS; Flag++) {
		RoundTrip(Folder, ("flag-" + std::to_string(Flag)).c_str(),
				"TypeID = 100\nName = \"x\"\nFlags = {" + std::string(FlagNames[Flag]) + "}\n");
	}

	// An empty file defines nothing.
	RoundTrip(Folder, "empty", "# nothing\n");
}

// What a conversion refuses.
static void TestRefusals(const fs::path &Folder) {
	WriteFile(Folder / "meaning49.srv", "TypeID = 100\nAttributes = {Meaning=49}\n");
	CHECK(Convert(Folder / "meaning49.srv").find("error: a meaning of 49") == 0);
	WriteFile(Folder / "lost-meaning.srv",
			"TypeID = 100\nAttributes = {Meaning=3}\nTypeID = 100\nAttributes = {Weight=3}\n");
	CHECK(Convert(Folder / "lost-meaning.srv").find("error: a special object whose type was defined again") == 0);
	// Values and bits without a name.
	const char *const Unnamed[] = {
		"BodyPosition=11", "BodyPosition=-1", "WeaponType=0", "AbsTeleportEffect=8", "Professions=32",
		"ProtectionDamageTypes=1024", "WandDamageType=0", "WandDamageType=3", "Meaning=4",
	};
	for (const char *Attribute : Unnamed) {
		WriteFile(Folder / "unnamed.srv", std::string("TypeID = 100\nAttributes = {") + Attribute + "}\n");
		std::string Error = Convert(Folder / "unnamed.srv");
		if (Error.find("error: a value of ") != 0 || Error.find(" without a name") == std::string::npos) {
			fprintf(stderr, "%s: %s\n", Attribute, Error.c_str());
			Failures++;
		}
	}
	WriteFile(Folder / "unknown-flag.srv", "TypeID = 100\nFlags = {Bnak}\n");
	CHECK(Convert(Folder / "unknown-flag.srv").find("error: ") == 0);
}

static void TestErrors(const fs::path &Folder) {
	fs::path File = Folder / "errors" / "items.toml";
	std::string Name = File.string();
	auto Load = [&](const std::string &Text) {
		WriteFile(File, Text);
		return InChild([&]() {
			LoadItems(File.c_str());
			return std::string();
		});
	};
	auto Expect = [&](const std::string &Text, int Line, const char *What) {
		std::string Error = Load(Text);
		std::string Where = "error: " + Name + ":" + std::to_string(Line) + ":";
		bool Ok = Error.compare(0, Where.size(), Where) == 0 && Error.find(What) != std::string::npos;
		if (!Ok) {
			fprintf(stderr, "for:\n%s\nexpected \"%s...%s\", got \"%s\"\n", Text.c_str(), Where.c_str(), What,
					Error.c_str());
			Failures++;
		}
	};
	const std::string Item = "[[item]]\nid = 100\n";
	Expect("items = 1\n", 1, "unknown key \"items\" in the top level");
	Expect("item = 1\n", 1, "item must be an array");
	Expect("item = [1]\n", 1, "an entry of item must be a table");
	Expect(Item + "nmae = \"x\"\n", 3, "unknown key \"nmae\" in [[item]]");
	Expect("[[item]]\nname = \"x\"\n", 1, "an entry of item needs id");
	Expect("[[item]]\nid = \"1\"\n", 2, "id must be an integer");
	Expect("[[item]]\nid = -1\n", 2, "id must lie between 0 and 65535");
	Expect("[[item]]\nid = 65536\n", 2, "id must lie between 0 and 65535");
	Expect(Item + Item, 4, "type 100 is defined twice");
	Expect(Item + "name = 5\n", 3, "name must be a string");
	Expect(Item + "description = \"\xc4\x80\"\n", 3, "description has a character outside Latin-1");
	Expect(Item + "name = \"a\\u0000b\"\n", 3, "name has a zero character");
	Expect(Item + "name = \"" + std::string(3999, 'n') + "\"\n", 3, "name is longer than 3998 characters");
	Expect(Item + "flags = \"bank\"\n", 3, "flags must be an array");
	Expect(Item + "flags = [\"bank\", 1]\n", 3, "a flag must be a string");
	Expect(Item + "flags = [\"bnak\"]\n", 3, "unknown flag \"bnak\"");
	Expect(Item + "flags = [\"Bank\"]\n", 3, "unknown flag \"Bank\"");
	Expect(Item + "attributes = 5\n", 3, "attributes must be a table");
	Expect(Item + "attributes = { wieght = 1 }\n", 3, "unknown attribute \"wieght\"");
	Expect(Item + "attributes = { weight = \"1\" }\n", 3, "weight must be an integer");
	Expect(Item + "attributes = { weight = 4294967296 }\n", 3, "weight must lie between -2147483648 and 4294967295");
	Expect(Item + "attributes = { weight = -2147483649 }\n", 3, "weight must lie between");
	Expect(Item + "attributes = { meaning = 1 }\n", 3, "meaning must be a string");
	Expect(Item + "attributes = { meaning = \"money\" }\n", 3, "meaning: \"money\" is unknown");
	Expect(Item + "attributes = { meaning = \"money_ten_thousand\" }\n[[item]]\nid = 101\n"
			"attributes = { meaning = \"money_ten_thousand\" }\n", 6, "meaning 3 is already the one of type 100");
	Expect(Item + "attributes = { body_position = \"Head\" }\n", 3, "body_position: \"Head\" is unknown");
	Expect(Item + "attributes = { weapon_type = \"sword\", body_position = [\"head\"] }\n", 3,
			"body_position must be a string");
	Expect(Item + "attributes = { wand_damage_type = 4 }\n", 3, "wand_damage_type must be a string");
	Expect(Item + "attributes = { professions = \"druid\" }\n", 3, "professions must be an array of strings");
	Expect(Item + "attributes = { professions = [\"druid\", 16] }\n", 3, "professions must be an array of strings");
	Expect(Item + "attributes = { professions = [\"elder_druid\"] }\n", 3,
			"professions: \"elder_druid\" is unknown");
	Expect(Item + "name =\n", 3, "");

	// The list [[use]]: the entry's place for a missing key, the key's for
	// the rest.
	const std::string Use = "[[use]]\nflag = \"food\"\n";
	Expect("use = 1\n", 1, "use must be an array");
	Expect("use = [1]\n", 1, "an entry of use must be a table");
	Expect(Item + "[[use]]\nscript = \"items/food.lua\"\n", 3, "an entry of use needs flag");
	Expect(Item + Use, 3, "an entry of use needs script");
	Expect(Use + "script = \"items/food.lua\"\nscirpt = \"items/food.lua\"\n", 4, "unknown key \"scirpt\" in [[use]]");
	Expect("[[use]]\nflag = 21\nscript = \"items/food.lua\"\n", 2, "flag must be a string");
	Expect("[[use]]\nflag = \"bnak\"\nscript = \"items/food.lua\"\n", 2, "unknown flag \"bnak\"");
	Expect("[[use]]\nflag = \"Food\"\nscript = \"items/food.lua\"\n", 2, "unknown flag \"Food\"");
	Expect("[[use]]\nscript = \"items/food.lua\"\nflag = \"\"\n", 3, "unknown flag \"\"");
	Expect(Use + "script = 1\n", 3, "script must be a string");
	Expect(Use + "script = [\"items/food.lua\"]\n", 3, "script must be a string");
	const std::string BadScripts[] = {
		"", "food.lua", "items/food", "items/food.LUA", "items/Food.lua", "items/.lua", "items/x/.lua", "items//food.lua",
		"items/../food.lua", "items/./food.lua", "items/fo od.lua", "scripts/items/food.lua", "spells/food.lua",
		"/items/food.lua", "items\\\\food.lua", "items/" + std::string(247, 'a') + ".lua",
	};
	for (const std::string &Script : BadScripts) {
		Expect(Use + "script = \"" + Script + "\"\n", 3, "must be a script name \"items/<name>.lua\"");
	}
	Expect(Item + "[[use]]\nflag = \"food\"\nscript = \"items/food.lua\"\n" + Use + "script = \"items/Food.lua\"\n", 8,
			"script \"items/Food.lua\" must be a script name");

	// What a list [[use]] may hold: the longest script name, a script in a
	// folder below items/, a flag and a script given twice; the entries in
	// the order of the file, each in the place of its script key.
	const std::string Longest = "items/" + std::string(246, 'a') + ".lua";
	std::string Uses = InChild([&]() {
		WriteFile(File, Use + "script = \"" + Longest + "\"\n" + Item + "[[use]]\n  script = \"items/sub/x-y_1.0.lua\"\n"
				"flag = \"bank\"\n" + Use + "script = \"items/food.lua\"\n");
		LoadItems(File.c_str());
		return DumpUses();
	});
	CHECK(Uses == "uses\t3\nuse 0 flag\tfood\nuse 0 script\t" + Longest + "\nuse 0 place\t" + Name + ":3:1\n"
			"use 0 script number\t-1\nuse 1 flag\tbank\nuse 1 script\titems/sub/x-y_1.0.lua\nuse 1 place\t" + Name
			+ ":7:3\nuse 1 script number\t-1\nuse 2 flag\tfood\nuse 2 script\titems/food.lua\nuse 2 place\t" + Name
			+ ":11:1\nuse 2 script number\t-1\n");
	// A file without [[use]] has none.
	WriteFile(Folder / "no-uses.toml", Item);
	CHECK(LoadUses(Folder / "no-uses.toml") == "uses\t0\n");

	// What loads: no file content, the longest name, both ways of writing a
	// 32-bit number, a flag given twice.
	CHECK(Load("") == "");
	CHECK(Load(Item + "name = \"" + std::string(3998, 'n') + "\"\n") == "");
	CHECK(Load(Item + "flags = [\"take\", \"take\"]\nattributes = { weight = 4294967295, capacity = -2147483648 }\n")
			== "");
	WriteFile(Folder / "signed.toml", Item + "attributes = { weight = -1 }\n");
	WriteFile(Folder / "unsigned.toml", Item + "attributes = { weight = 4294967295 }\n");
	Same(Snapshot(NewLoader(Folder / "signed.toml")), Snapshot(NewLoader(Folder / "unsigned.toml")), "signed");

	// The values of names: a bit given twice counts once; an empty array is 0.
	WriteFile(Folder / "named.toml", Item + "flags = [\"restrict_profession\", \"protection\", \"wand\"]\n"
			"attributes = { professions = [\"druid\", \"none\", \"druid\"], protection_damage_types = [], "
			"wand_damage_type = \"life_drain\", wand_missile = \"poison\" }\n");
	std::string Values = InChild([&]() {
		LoadItems((Folder / "named.toml").c_str());
		ObjectType Type(100);
		return std::to_string(Type.getAttribute(PROFESSIONS)) + " "
				+ std::to_string(Type.getAttribute(PROTECTIONDAMAGETYPES)) + " "
				+ std::to_string(Type.getAttribute(WANDDAMAGETYPE)) + " " + std::to_string(Type.getAttribute(WANDMISSILE));
	});
	CHECK(Values == "17 0 256 15");

	std::string Missing = InChild([&]() {
		LoadItems((Folder / "missing.toml").c_str());
		return std::string();
	});
	CHECK(Missing.compare(0, 7, "error: ") == 0 && Missing.find((Folder / "missing.toml").string()) != std::string::npos);
}

// The fastest of Runs loads, each in a child of its own, in milliseconds.
static double LoadTime(const std::function<void()> &Load, int Runs) {
	double Best = 1e9;
	for (int i = 0; i < Runs; i++) {
		std::string Out = InChild([&]() {
			auto Start = std::chrono::steady_clock::now();
			Load();
			auto End = std::chrono::steady_clock::now();
			return std::to_string(std::chrono::duration<double, std::milli>(End - Start).count());
		});
		Best = std::min(Best, atof(Out.c_str()));
	}
	return Best;
}

static int Original(const char *ObjectsSrv, const char *Items) {
	if (!fs::is_regular_file(ObjectsSrv)) {
		printf("skipped: no %s\n", ObjectsSrv);
		return 77;
	}

	std::string Text = Convert(ObjectsSrv);
	if (ReadFile(Items) != Text) {
		fprintf(stderr, "%s is not what patella-convert items makes of %s (%s)\n", Items, ObjectsSrv,
				Text.compare(0, 7, "error: ") == 0 ? Text.c_str() : "a different text");
		Failures++;
	}

	std::string Expected = Snapshot(OldLoader(ObjectsSrv));
	std::string Actual = Snapshot(NewLoader(Items));
	for (const std::string *Dump : {&Expected, &Actual}) {
		size_t Error = Dump->find("error: ");
		if (Error != std::string::npos || Dump->find("crashed") != std::string::npos) {
			fprintf(stderr, "%s failed: %s\n", Dump == &Expected ? "ReadObjectsSrv" : "LoadItems",
					Error != std::string::npos ? Dump->substr(Error, Dump->find('\n', Error) - Error).c_str()
						: "crashed");
			return 1;
		}
	}
	size_t Compared = 0;
	if (Same(Expected, Actual, "original", &Compared)) {
		std::map<std::string, std::string> Values = Fields(Actual);
		int Types = 0, Specials = 0;
		for (const auto &Pair : Values) {
			if (Pair.first.compare(0, 5, "type ") == 0 && Pair.first.size() > 5
					&& Pair.first.compare(Pair.first.size() - 5, 5, " name") == 0) {
				Types++;
			}
			if (Pair.first.compare(0, 15, "special object ") == 0 && Pair.second != "0") {
				Specials++;
			}
		}
		printf("%s and %s give the same table: %s, %d types, %d special objects, %s entries left alone; "
				"%d fields compared\n", ObjectsSrv, Items, Values["range"].c_str(), Types, Specials,
				Values["entries left alone"].c_str(), static_cast<int>(Compared));
	}

	std::string Committed = ReadFile(Items);
	CheckUses(Items, Committed, "original");
	int Files = CheckScriptFiles(Items, Committed);
	CHECK(Files == EXPECTED_USES);
	printf("%d scripts that %s names in its list [[use]], each a file with its handler\n", Files, Items);
	fs::path Folder = TemporaryFolder("convert-test-items-original");
	int Changes = TestUseMutations(Folder, Committed);
	fs::remove_all(Folder);
	printf("%d changes of single entries of the list [[use]] of %s, each caught\n", Changes, Items);

	double Old = LoadTime(OldLoader(ObjectsSrv), 5);
	double New = LoadTime(NewLoader(Items), 5);
	printf("load time (fastest of 5): objects.srv %.1f ms, items.toml %.1f ms\n", Old, New);
	return Failures == 0 ? 0 : 1;
}

int main(int argc, char **argv) {
	if (argc == 4 && strcmp(argv[1], "--original") == 0) {
		return Original(argv[2], argv[3]);
	}
	if (argc != 1) {
		fprintf(stderr, "usage: convert_test_items [--original OBJECTS_SRV ITEMS_TOML]\n");
		return 2;
	}

	char Pattern[] = "/tmp/convert-test-items-XXXXXX";
	if (mkdtemp(Pattern) == nullptr) {
		perror("mkdtemp");
		return 1;
	}
	fs::path Folder = fs::canonical(Pattern);
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
