// test_items_reload.cc: the items part of a reload (ItemsReloadPart,
// objects.cc) without the game. The start (InitObjects) must leave what the
// converter's loaders (LoadItems, LoadConversions, the start before the part)
// leave: the same bound, the same entries of the defined types, the same
// special objects, list [[use]] and NewType, and zero in every other entry up
// to the bound. Then reloads of changed files: a good one (changed names and
// attributes, a raised capacity, a type above the bound with a new meaning, a
// changed conversion)
// installed and uninstalled (every global as before, the bound undone) and
// finished; each refusal of reload.hh, after which nothing has changed. The
// files are written into a scratch folder that is removed at the end; with
// --data <folder> the start check also runs on that folder's items.toml and
// conversion.toml (the repository's data). Prints what failed; exits 1 on any
// failure, 0 when all pass.

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>
#include <sys/wait.h>
#include <unistd.h>

#include "config.hh"
#include "objects.hh"
#include "reload.hh"

namespace fs = std::filesystem;

// config.cc's, which the test does not link.
char DATAPATH[4096];

static int Failures = 0;

#define CHECK(Condition) \
	do { \
		if (!(Condition)) { \
			fprintf(stderr, "%s:%d: CHECK failed: %s\n", __FILE__, __LINE__, #Condition); \
			Failures++; \
		} \
	} while (0)

#define CHECK_CONTAINS(Text, Part) \
	do { \
		std::string CheckText_ = (Text); \
		if (CheckText_.find(Part) == std::string::npos) { \
			fprintf(stderr, "%s:%d: \"%s\" does not contain \"%s\"\n", __FILE__, __LINE__, CheckText_.c_str(), \
					std::string(Part).c_str()); \
			Failures++; \
		} \
	} while (0)

static void WriteFile(const fs::path &File, const std::string &Text) {
	std::ofstream Out(File, std::ios::binary);
	Out << Text;
}

// Runs Body in a child process and gives the text it returns ("error: <text>"
// for a const char * it throws).
template<class F>
static std::string InChild(F Body) {
	int Pipe[2];
	if (pipe(Pipe) != 0) {
		perror("pipe");
		exit(1);
	}
	fflush(nullptr);
	pid_t Child = fork();
	if (Child == 0) {
		close(Pipe[0]);
		std::string Out;
		try {
			Out = Body();
		} catch (const char *Error) {
			Out = std::string("error: ") + Error;
		}
		size_t Done = 0;
		while (Done < Out.size()) {
			ssize_t n = write(Pipe[1], Out.data() + Done, Out.size() - Done);
			if (n <= 0) {
				break;
			}
			Done += static_cast<size_t>(n);
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
	return Out;
}

// The highest type number ObjectTypeExists takes.
static int Bound() {
	int TypeID = 0;
	while (TypeID < 70000 && ObjectTypeExists(TypeID + 1)) {
		TypeID++;
	}
	return TypeID;
}

// One entry's fields as text (names and descriptions as their texts).
static std::string DumpType(int TypeID) {
	const TObjectType &Type = ObjectTypes(TypeID);
	std::string Out = "type " + std::to_string(TypeID) + "\t";
	Out += Type.Name == nullptr ? "(null)" : "\"" + std::string(Type.Name) + "\"";
	Out += " ";
	Out += Type.Description == nullptr ? "(null)" : "\"" + std::string(Type.Description) + "\"";
	for (size_t i = 0; i < sizeof(Type.Flags); i++) {
		Out += " " + std::to_string(Type.Flags[i]);
	}
	for (int i = 0; i < TYPEATTRIBUTES; i++) {
		Out += " " + std::to_string(Type.Attributes[i]);
	}
	for (int i = 0; i < INSTANCEATTRIBUTES; i++) {
		Out += " " + std::to_string(Type.AttributeOffsets[i]);
	}
	return Out + "\n";
}

// The special objects, the list [[use]] and NewType as text.
static std::string DumpRest() {
	std::string Out;
	for (int Meaning = 0; Meaning < SPECIALMEANINGS; Meaning++) {
		Out += "special " + std::to_string(Meaning) + "\t" + std::to_string(SpecialObjects[Meaning].getTypeID()) + "\n";
	}
	for (const TUseEntry &Entry : UseEntries) {
		Out += "use\t" + std::to_string(Entry.Flag) + " " + Entry.Script + " " + Entry.Where + " "
				+ std::to_string(Entry.ScriptNumber) + "\n";
	}
	for (int i = 0; i < 65536; i++) {
		if (NewType[i] != -1) {
			Out += "conversion " + std::to_string(i) + "\t" + std::to_string(NewType[i]) + "\n";
		}
	}
	return Out;
}

static const unsigned char POISON = 0xA5;

// The start: what the converter's loaders leave (the start before the part)
// must be what InitObjects leaves. Runs in children, so the globals stay
// untouched; Folder holds items.toml and conversion.toml.
static void TestStart(const fs::path &Folder) {
	std::string ItemsFile = (Folder / "items.toml").string();
	std::string ConversionFile = (Folder / "conversion.toml").string();

	// The old start: its bound, then the entries it filled (the table
	// poisoned before, so that the ones it left alone show) and the rest.
	std::string OldBound = InChild([&]() {
		LoadItems(ItemsFile.c_str());
		return std::to_string(ObjectTypes.high());
	});
	std::string Old = InChild([&]() {
		ObjectTypes(65535);
		for (int TypeID = 0; TypeID <= 65535; TypeID++) {
			memset(static_cast<void *>(&ObjectTypes(TypeID)), POISON, sizeof(TObjectType));
		}
		LoadItems(ItemsFile.c_str());
		LoadConversions(ConversionFile.c_str());
		std::string Out;
		for (int TypeID = 0; TypeID <= 65535; TypeID++) {
			const unsigned char *Bytes = reinterpret_cast<const unsigned char *>(&ObjectTypes(TypeID));
			bool Untouched = true;
			for (size_t i = 0; i < sizeof(TObjectType) && Untouched; i++) {
				Untouched = Bytes[i] == POISON;
			}
			if (!Untouched) {
				Out += DumpType(TypeID);
			}
		}
		return Out + DumpRest();
	});

	std::string New = InChild([&]() {
		strcpy(DATAPATH, Folder.string().c_str());
		InitObjects();
		std::string Out = "bound " + std::to_string(Bound()) + "\n";
		int Zero = 0;
		for (int TypeID = 0; TypeID <= Bound(); TypeID++) {
			if (DefinedObjectTypes[TypeID]) {
				Out += DumpType(TypeID);
				continue;
			}
			const unsigned char *Bytes = reinterpret_cast<const unsigned char *>(&ObjectTypes(TypeID));
			bool IsZero = true;
			for (size_t i = 0; i < sizeof(TObjectType) && IsZero; i++) {
				IsZero = Bytes[i] == 0;
			}
			Zero += IsZero ? 1 : 0;
			if (!IsZero) {
				Out += "not zero\t" + std::to_string(TypeID) + "\n";
			}
		}
		return Out + DumpRest() + "zero " + std::to_string(Zero) + "\n";
	});

	std::string Expected = "bound " + OldBound + "\n" + Old;
	size_t ZeroAt = New.rfind("zero ");
	CHECK(ZeroAt != std::string::npos);
	if (New.substr(0, ZeroAt) != Expected) {
		fprintf(stderr, "the start differs from the old loaders' (%s)\n", Folder.string().c_str());
		size_t i = 0;
		while (i < Expected.size() && i < ZeroAt && Expected[i] == New[i]) {
			i++;
		}
		size_t LineStart = Expected.rfind('\n', i);
		LineStart = LineStart == std::string::npos ? 0 : LineStart + 1;
		fprintf(stderr, "  old: %s\n  new: %s\n", Expected.substr(LineStart, 200).c_str(),
				New.substr(LineStart, 200).c_str());
		Failures++;
	}
	printf("start (%s): %s", Folder.string().c_str(), New.substr(ZeroAt).c_str());
}

// Every global the items part sets, exactly: the entries up to the bound
// field by field (names and descriptions by pointer), the bound, the special
// objects, the list [[use]], NewType and the defined types.
struct TState {
	int Bound;
	std::vector<TObjectType> Types;
	std::vector<int> Special;
	std::vector<std::string> Uses;
	std::vector<int> Conversions;
	std::vector<bool> Defined;
};

static TState Take() {
	TState State;
	State.Bound = Bound();
	for (int TypeID = 0; TypeID <= State.Bound; TypeID++) {
		State.Types.push_back(ObjectTypes(TypeID));
	}
	for (int Meaning = 0; Meaning < SPECIALMEANINGS; Meaning++) {
		State.Special.push_back(SpecialObjects[Meaning].getTypeID());
	}
	for (const TUseEntry &Entry : UseEntries) {
		State.Uses.push_back(std::to_string(Entry.Flag) + " " + Entry.Script + " " + Entry.Where + " "
				+ std::to_string(Entry.ScriptNumber));
	}
	State.Conversions.assign(NewType, NewType + 65536);
	State.Defined = DefinedObjectTypes;
	return State;
}

static bool Same(const TState &a, const TState &b) {
	if (a.Bound != b.Bound || a.Types.size() != b.Types.size()) {
		return false;
	}
	for (size_t i = 0; i < a.Types.size(); i++) {
		const TObjectType &x = a.Types[i];
		const TObjectType &y = b.Types[i];
		if (x.Name != y.Name || x.Description != y.Description || memcmp(x.Flags, y.Flags, sizeof(x.Flags)) != 0
				|| memcmp(x.Attributes, y.Attributes, sizeof(x.Attributes)) != 0
				|| memcmp(x.AttributeOffsets, y.AttributeOffsets, sizeof(x.AttributeOffsets)) != 0) {
			return false;
		}
	}
	return a.Special == b.Special && a.Uses == b.Uses && a.Conversions == b.Conversions && a.Defined == b.Defined;
}

// The files of the tests: Items is items.toml without the list [[use]].
static const std::string Items =
		"[[item]]\nid = 0\n\n"
		"[[item]]\nid = 100\nname = \"a bag\"\nflags = [\"container\", \"take\"]\n"
		"attributes = { capacity = 8, weight = 800 }\n\n"
		"[[item]]\nid = 101\nname = \"a gold coin\"\nflags = [\"cumulative\", \"take\"]\n"
		"attributes = { weight = 10, meaning = \"money_one\" }\n\n"
		"[[item]]\nid = 102\nname = \"a stone\"\nflags = [\"bottom\"]\n\n"
		"[[item]]\nid = 5050\nname = \"a statue\"\ndescription = \"It is old.\"\nflags = [\"unmove\"]\n";
static const std::string Uses = "\n[[use]]\nflag = \"container\"\nscript = \"items/container.lua\"\n";
static const std::string Conversions = "[group.1]\n0 = 100\n1 = 101\n";

static std::string Replace(std::string Text, const std::string &From, const std::string &To) {
	size_t At = Text.find(From);
	if (At == std::string::npos) {
		fprintf(stderr, "test error: \"%s\" not found\n", From.c_str());
		exit(1);
	}
	return Text.replace(At, From.size(), To);
}

static std::string Name(int TypeID) {
	const char *Text = ObjectTypes(TypeID).Name;
	return Text == nullptr ? "(null)" : Text;
}

// A read the part refuses: the text holds What, and nothing has changed.
static void Refused(const fs::path &Folder, const std::string &ItemsText, const std::string &ConversionText,
		const char *What) {
	WriteFile(Folder / "items.toml", ItemsText);
	WriteFile(Folder / "conversion.toml", ConversionText);
	TReloadPart &Part = ItemsReloadPart();
	TState Before = Take();
	std::string Error;
	try {
		Part.read();
	} catch (const char *Text) {
		Error = Text;
	}
	CHECK_CONTAINS(Error, What);
	CHECK(Same(Before, Take()));
	Part.discard();
	CHECK(Same(Before, Take()));
}

static void TestReload(const fs::path &Folder) {
	WriteFile(Folder / "items.toml", Items + Uses);
	WriteFile(Folder / "conversion.toml", Conversions);
	strcpy(DATAPATH, Folder.string().c_str());
	InitObjects();
	CHECK(Bound() == 5050);
	CHECK(ObjectType(100).getAttribute(CAPACITY) == 8);
	CHECK(ObjectType(100).getAttributeOffset(CONTENT) == 0);
	CHECK(ObjectType(101).getAttributeOffset(AMOUNT) == 0);
	CHECK(GetSpecialObject(MONEY_ONE).getTypeID() == 101);
	CHECK(NewType[256] == 100 && NewType[257] == 101 && NewType[258] == -1);
	CHECK(UseEntries.size() == 1 && UseEntries[0].ScriptNumber == -1);
	CHECK(Name(102) == "a stone");
	TReloadPart &Part = ItemsReloadPart();
	CHECK(std::string(Part.name()) == "items");
	CHECK(Part.summary() == "items.toml: 5 types, 1 uses; conversion.toml: 14 entries");

	// A good reload: names, descriptions and attributes changed, the
	// capacity raised, a type above the bound with a meaning no type had, a
	// flag that may change (unpass), one more use, one more conversion.
	std::string Changed = Items;
	Changed = Replace(Changed, "a bag\"", "a backpack\"");
	Changed = Replace(Changed, "capacity = 8,", "capacity = 20,");
	Changed = Replace(Changed, "weight = 10, meaning", "weight = 11, meaning");
	Changed = Replace(Changed, "flags = [\"bottom\"]", "flags = [\"bottom\", \"unpass\", \"bank\"]\n"
			"description = \"Grey.\"");
	Changed += "\n[[item]]\nid = 6000\nname = \"a new thing\"\nattributes = { meaning = \"money_hundred\" }\n";
	std::string ChangedUses = Uses + "\n[[use]]\nflag = \"food\"\nscript = \"items/food.lua\"\n";
	std::string ChangedConversions = Conversions + "2 = 6000\n";

	TState Start = Take();
	Refused(Folder, Changed + ChangedUses, ChangedConversions, "the flag bank is new");
	Changed = Replace(Changed, ", \"bank\"]", "]");
	WriteFile(Folder / "items.toml", Changed + ChangedUses);
	WriteFile(Folder / "conversion.toml", ChangedConversions);
	Part.read();
	CHECK(Same(Start, Take()));
	CHECK(!ObjectTypeExists(6000));
	CHECK(Part.summary() == "items.toml: 3 types changed, 1 added, 2 uses; conversion.toml: 1 entries changed");

	Part.install();
	CHECK(Bound() == 6000);
	CHECK(Name(100) == "a backpack");
	CHECK(ObjectType(100).getAttribute(CAPACITY) == 20);
	CHECK(ObjectType(101).getAttribute(WEIGHT) == 11);
	CHECK(ObjectType(102).getFlag(UNPASS));
	CHECK(std::string(ObjectType(102).getDescription()) == "Grey.");
	CHECK(Name(6000) == "a new thing");
	CHECK(GetSpecialObject(MONEY_ONE).getTypeID() == 101);
	CHECK(GetSpecialObject(MONEY_HUNDRED).getTypeID() == 6000);
	CHECK(NewType[258] == 6000);
	CHECK(UseEntries.size() == 2 && UseEntries[1].Script == "items/food.lua");
	CHECK(DefinedObjectTypes[6000] && !DefinedObjectTypes[5999]);
	TState Installed = Take();

	// Undone: every global as before, the bound back at 5050.
	Part.loadScripts();
	Part.uninstall();
	CHECK(Same(Start, Take()));
	CHECK(!ObjectTypeExists(6000) && ObjectTypeExists(5050));
	CHECK(GetSpecialObject(MONEY_ONE).getTypeID() == 101);
	CHECK(SpecialObjects[MONEY_HUNDRED].getTypeID() == 0);
	Part.discard();
	CHECK(Same(Start, Take()));

	// Done: the new data stays, as install made it (same texts too).
	Part.read();
	Part.install();
	Part.loadScripts();
	Part.finish();
	TState Reloaded = Take();
	CHECK(Reloaded.Bound == 6000);
	CHECK(Name(100) == "a backpack" && Name(6000) == "a new thing" && Name(5050) == "a statue");
	CHECK(ObjectType(6000).getTypeID() == 6000);
	CHECK(Installed.Special == Reloaded.Special && Installed.Conversions == Reloaded.Conversions);

	// The refusals of reload.hh, each from the reloaded state.
	const std::string Good = Changed + ChangedUses;
	Refused(Folder, Replace(Good, "[[item]]\nid = 6000\nname = \"a new thing\"\nattributes = { meaning = "
			"\"money_hundred\" }\n", ""), ChangedConversions,
			"type 6000 is gone; a reload cannot remove or renumber an object type");
	Refused(Folder, Replace(Good, "id = 6000", "id = 6001"), ChangedConversions, "type 6000 is gone");
	Refused(Folder, Replace(Good, "[\"container\", \"take\"]", "[\"take\"]"), ChangedConversions,
			"type 100: the flag container is gone");
	Refused(Folder, Replace(Good, "[\"bottom\", \"unpass\"]", "[\"bottom\", \"unpass\", \"top\"]"),
			ChangedConversions, "type 102: the flag top is new");
	Refused(Folder, Replace(Good, "[\"cumulative\", \"take\"]", "[\"take\"]"), ChangedConversions,
			"type 101: the flag cumulative is gone");
	Refused(Folder, Replace(Good, "name = \"a new thing\"\n", "name = \"a new thing\"\nflags = [\"expire\"]\n"),
			ChangedConversions, "type 6000: the flag expire is new");
	Refused(Folder, Replace(Good, ", meaning = \"money_one\"", ""), ChangedConversions,
			"the meaning money_one (1) of type 101 is gone; a reload cannot remove a special meaning");
	// A meaning moved to another type (every gold coin would stop counting).
	Refused(Folder, Replace(Replace(Good, ", meaning = \"money_one\"", ""), "description = \"Grey.\"",
			"description = \"Grey.\"\nattributes = { meaning = \"money_one\" }"), ChangedConversions,
			"the meaning money_one (1) of type 101 belongs to type 102; a reload cannot move a special meaning");
	// Two meanings swapped between their types.
	Refused(Folder, Replace(Replace(Good, "thing\"\nattributes = { meaning = \"money_hundred\"",
			"thing\"\nattributes = { meaning = \"money_one\""), "11, meaning = \"money_one\"",
			"11, meaning = \"money_hundred\""), ChangedConversions,
			"the meaning money_one (1) of type 101 belongs to type 6000");
	// A container type's capacity lowered (its containers may hold more).
	Refused(Folder, Replace(Good, "capacity = 20,", "capacity = 19,"), ChangedConversions,
			"type 100: the capacity 19 is lower than 20; a reload cannot lower a container type's capacity");
	Refused(Folder, Replace(Good, "capacity = 20, ", ""), ChangedConversions, "type 100: the capacity 0 is lower");
	Refused(Folder, Changed, ChangedConversions, "items.toml: the list [[use]] is missing");
	Refused(Folder, Replace(Good, "\"unpass\"", "\"unpasss\""), ChangedConversions, "unknown flag \"unpasss\"");
	Refused(Folder, Good, Replace(ChangedConversions, "2 = 6000", "2 = 9000"), "must lie between 1 and 8191");
	Refused(Folder, "[[item\n", ChangedConversions, "items.toml");
	CHECK(Same(Reloaded, Take()));

	// What may change: a type loses a flag no layout depends on; a container
	// type's capacity rises; a new type has any flag.
	WriteFile(Folder / "items.toml", Replace(Replace(Good, "[\"container\", \"take\"]", "[\"container\"]"),
			"capacity = 20,", "capacity = 21,") + "\n[[item]]\nid = 6001\nflags = [\"container\", \"top\"]\n");
	Part.read();
	Part.install();
	CHECK(!ObjectType(100).getFlag(TAKE) && ObjectType(6001).getFlag(TOP) && Bound() == 6001);
	CHECK(ObjectType(100).getAttribute(CAPACITY) == 21);
	Part.uninstall();
	Part.discard();
	CHECK(Same(Reloaded, Take()));
}

int main(int argc, char **argv) {
	fs::path Folder = fs::temp_directory_path() / ("patella-items-reload-" + std::to_string(getpid()));
	fs::remove_all(Folder);
	fs::create_directories(Folder / "start");
	fs::create_directories(Folder / "reload");

	WriteFile(Folder / "start" / "items.toml", Items + Uses);
	WriteFile(Folder / "start" / "conversion.toml", Conversions);
	TestStart(Folder / "start");
	if (argc == 3 && strcmp(argv[1], "--data") == 0) {
		TestStart(argv[2]);
	}
	try {
		TestReload(Folder / "reload");
	} catch (const char *Error) {
		fprintf(stderr, "unexpected error: %s\n", Error);
		Failures++;
	}

	fs::remove_all(Folder);
	if (Failures != 0) {
		fprintf(stderr, "%d checks failed\n", Failures);
		return 1;
	}
	printf("all checks passed\n");
	return 0;
}
