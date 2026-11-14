// test_items.cc: the round trip of the object types. The object type table the
// game ends up with from an objects.srv, through the old reader
// (ReadObjectsSrv), and from the items.toml patella-convert makes of it,
// through the game's LoadItems, must be the same, field by field; LoadItems
// must refuse what is wrong with the file and line.
//
//   convert_test_items
//     With made-up files in a temporary folder.
//   convert_test_items --original OBJECTS_SRV ITEMS_TOML
//     With the original objects.srv and the committed items.toml: ITEMS_TOML
//     must be exactly what patella-convert makes of OBJECTS_SRV, and give the
//     same table. Prints the load times of both. Exit code 77 (skipped) when
//     OBJECTS_SRV is not there.
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

namespace fs = std::filesystem;

static int Failures = 0;

#define CHECK(Condition) \
	do { \
		if (!(Condition)) { \
			fprintf(stderr, "%s:%d: CHECK failed: %s\n", __FILE__, __LINE__, #Condition); \
			Failures++; \
		} \
	} while (0)

static const unsigned char POISON = 0xA5;
static const int ALL_TYPES = 65536;

static void WriteFile(const fs::path &Path, const std::string &Text) {
	fs::create_directories(Path.parent_path());
	std::ofstream Out(Path, std::ios::binary);
	Out << Text;
}

static std::string ReadFile(const fs::path &Path) {
	std::ifstream In(Path, std::ios::binary);
	std::stringstream Text;
	Text << In.rdbuf();
	return Text.str();
}

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

// Runs Work in a child process and returns what it returned there, or
// "error: <text>" for a const char * it threw, or "crashed" for a child that
// did not end normally.
static std::string InChild(const std::function<std::string()> &Work) {
	fflush(stdout);
	fflush(stderr);
	int Pipe[2];
	if (pipe(Pipe) != 0) {
		perror("pipe");
		exit(1);
	}
	pid_t Child = fork();
	if (Child < 0) {
		perror("fork");
		exit(1);
	}
	if (Child == 0) {
		close(Pipe[0]);
		std::string Out;
		try {
			Out = Work();
		} catch (const char *Error) {
			Out = std::string("error: ") + Error + "\n";
		}
		size_t Written = 0;
		while (Written < Out.size()) {
			ssize_t n = write(Pipe[1], Out.data() + Written, Out.size() - Written);
			if (n <= 0) {
				_exit(1);
			}
			Written += static_cast<size_t>(n);
		}
		close(Pipe[1]);
		_exit(0);
	}
	close(Pipe[1]);
	std::string Out;
	char Buffer[65536];
	ssize_t n;
	while ((n = read(Pipe[0], Buffer, sizeof(Buffer))) > 0) {
		Out.append(Buffer, static_cast<size_t>(n));
	}
	close(Pipe[0]);
	int Status = 0;
	waitpid(Child, &Status, 0);
	if (!WIFEXITED(Status) || WEXITSTATUS(Status) != 0) {
		return "crashed";
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

static std::map<std::string, std::string> Fields(const std::string &Dump) {
	std::map<std::string, std::string> Result;
	std::istringstream In(Dump);
	std::string Line;
	while (std::getline(In, Line)) {
		size_t Tab = Line.find('\t');
		if (Tab == std::string::npos) {
			Result["(line)"] += Line + " ";
		} else {
			Result[Line.substr(0, Tab)] = Line.substr(Tab + 1);
		}
	}
	return Result;
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
	// values of their own (the meaning the highest there is).
	Text += "\nTypeID = 4000\nName = \"everything\"\nFlags = {";
	for (int Flag = FLAGS - 1; Flag >= 0; Flag--) {
		Text += std::string(FlagNames[Flag]) + (Flag > 0 ? "," : "}\n");
	}
	Text += "Attributes = {";
	for (int Attribute = TYPEATTRIBUTES - 1; Attribute >= 0; Attribute--) {
		int Value = Attribute == MEANING ? SPECIALMEANINGS - 1 : (Attribute % 2 == 0 ? 1 : -1) * (Attribute * 1000 + 7);
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
	CHECK(Toml.find("attributes = { weight = 2, meaning = 1 }\n") != std::string::npos);
	CHECK(Toml.find("rel_teleport_displacement = -2145189880") != std::string::npos);
	CHECK(Toml.find("id = 120\nname = \"nothing else\"\n\n") != std::string::npos);
	CHECK(Toml.find("id = 121\nflags = [\"bank\"]\n") != std::string::npos);
	CHECK(Toml.find("id = 122\nname = \"empty lists\"\n\n") != std::string::npos);
	// The types in order of their numbers.
	CHECK(Toml.find("id = 122\n") < Toml.find("id = 4000\n") && Toml.find("id = 4000\n") < Toml.find("id = 5500\n"));

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
	Expect(Item + "attributes = { meaning = 0 }\n", 3, "meaning must lie between 1 and 48");
	Expect(Item + "attributes = { meaning = 49 }\n", 3, "meaning must lie between 1 and 48");
	Expect(Item + "attributes = { meaning = 3 }\n[[item]]\nid = 101\nattributes = { meaning = 3 }\n", 6,
			"meaning 3 is already the one of type 100");
	Expect(Item + "name =\n", 3, "");

	// What loads: no file content, the longest name, both ways of writing a
	// 32-bit number, a flag given twice.
	CHECK(Load("") == "");
	CHECK(Load(Item + "name = \"" + std::string(3998, 'n') + "\"\n") == "");
	CHECK(Load(Item + "flags = [\"take\", \"take\"]\nattributes = { weight = 4294967295, capacity = -2147483648 }\n")
			== "");
	WriteFile(Folder / "signed.toml", Item + "attributes = { weight = -1 }\n");
	WriteFile(Folder / "unsigned.toml", Item + "attributes = { weight = 4294967295 }\n");
	Same(Snapshot(NewLoader(Folder / "signed.toml")), Snapshot(NewLoader(Folder / "unsigned.toml")), "signed");

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
