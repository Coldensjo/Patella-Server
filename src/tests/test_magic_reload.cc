// test_magic_reload.cc: the staged tables of the magic data (magicdata.cc),
// which the reload's magic part (magic.cc, MagicReloadPart) reads, swaps in
// and out and empties, without the game. ReadStagedMagicData fills the staged
// set as the live loaders fill the live one and leaves the live set alone;
// CompareStagedMagicData counts what changed; SwapStagedMagicData twice gives
// back the live set byte for byte; a spell or command number the live set has
// and the files lack is refused; a name an earlier read made is taken again;
// the reload command's right. The files are written into a scratch folder
// that is removed at the end. Prints what failed; exits 1 on any failure, 0
// when all pass.

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>
#include <unistd.h>

#include "magicdata.hh"

namespace fs = std::filesystem;

static int Failures = 0;

#define CHECK(Condition) \
	do { \
		if (!(Condition)) { \
			fprintf(stderr, "%s:%d: CHECK failed: %s\n", __FILE__, __LINE__, #Condition); \
			Failures++; \
		} \
	} while (0)

// Like CHECK, for a text that must contain Part; prints the text otherwise.
#define CHECK_CONTAINS(Text, Part) \
	do { \
		std::string CheckText_ = (Text); \
		if (CheckText_.find(Part) == std::string::npos) { \
			fprintf(stderr, "%s:%d: CHECK failed: \"%s\" does not contain \"%s\"\n", __FILE__, __LINE__, \
					CheckText_.c_str(), std::string(Part).c_str()); \
			Failures++; \
		} \
	} while (0)

static fs::path Folder;

// Writes the file Name in the scratch folder and gives its path.
static std::string WriteFile(const std::string &Name, const std::string &Text) {
	fs::path Path = Folder / Name;
	std::ofstream Out(Path, std::ios::binary);
	Out << Text;
	return Path.string();
}

// The live tables, byte for byte, and the scripts' names and places.
struct TSnapshot {
	std::vector<unsigned char> Bytes;
	std::vector<std::string> Scripts;

	bool operator==(const TSnapshot &Other) const {
		return Bytes == Other.Bytes && Scripts == Other.Scripts;
	}
};

static TSnapshot Snapshot() {
	TSnapshot Result;
	auto Add = [&](const void *Data, size_t Size) {
		const unsigned char *p = static_cast<const unsigned char *>(Data);
		Result.Bytes.insert(Result.Bytes.end(), p, p + Size);
	};
	Add(SpellList, sizeof(SpellList));
	Add(CommandRight, sizeof(CommandRight));
	Add(Circle, sizeof(Circle));
	for (int i = 0; i < MAX_SPELLS; i++) {
		const char *Name = GetSpellScript(i);
		const char *Where = GetSpellScriptPosition(i);
		Result.Scripts.push_back(std::string(Name != nullptr ? Name : "-") + " " + (Where != nullptr ? Where : "-"));
	}
	return Result;
}

// The live tables as text, the names by their text: what two loads that
// make different static strings must agree on.
static std::string Dump() {
	std::string Out;
	for (int i = 0; i < MAX_SPELLS; i++) {
		const TSpellList &S = SpellList[i];
		if (S.Syllable[0] == 0) {
			continue;
		}
		char Line[512];
		snprintf(Line, sizeof(Line), "%d %s %d %d %d %d %d %d %d %d %d", i, S.Comment, S.Syllable[0], S.Syllable[1],
				S.Level, S.Mana, S.SoulPoints, S.Flags, S.RuneGr, S.RuneNr, S.Amount);
		Out += Line;
		for (int j = 0; j < MAX_COMMAND_RIGHTS; j++) {
			Out += " " + std::to_string(CommandRight[i][j]);
		}
		const char *Script = GetSpellScript(i);
		Out += std::string(" ") + (Script != nullptr ? Script : "-") + "\n";
	}
	for (int r = 0; r < MAX_CIRCLES; r++) {
		for (int k = 0; k < Circle[r].Count; k++) {
			Out += std::to_string(r) + ":" + std::to_string(Circle[r].x[k]) + "," + std::to_string(Circle[r].y[k])
					+ " ";
		}
	}
	return Out;
}

static const char *const CirclesA =
		"rows = [\n"
		"\t\". 1 .\",\n"
		"\t\"1 0 1\",\n"
		"\t\". 1 .\",\n"
		"]\n";
static const char *const CirclesB =
		"rows = [\n"
		"\t\"1 1 .\",\n"
		"\t\"1 0 1\",\n"
		"\t\". 1 .\",\n"
		"]\n";
static const char *const SpellsA =
		"[[spell]]\nnumber = 5\nname = \"Healing\"\nwords = \"exura\"\nlevel = 9\nmana = 20\n"
		"script = \"spells/light_healing.lua\"\n"
		"[[spell]]\nnumber = 10\nname = \"Light\"\nwords = \"utevo lux\"\nmana = 20\n";
// Spell 5 changed (mana), 10 the same, 11 added.
static const char *const SpellsB =
		"[[spell]]\nnumber = 5\nname = \"Healing\"\nwords = \"exura\"\nlevel = 9\nmana = 25\n"
		"script = \"spells/light_healing.lua\"\n"
		"[[spell]]\nnumber = 10\nname = \"Light\"\nwords = \"utevo lux\"\nmana = 20\n"
		"[[spell]]\nnumber = 11\nname = \"Intense Healing\"\nwords = \"exura gran\"\nmana = 40\n";
// Spell 10 gone.
static const char *const SpellsC =
		"[[spell]]\nnumber = 5\nname = \"Healing\"\nwords = \"exura\"\nlevel = 9\nmana = 20\n"
		"script = \"spells/light_healing.lua\"\n";
static const char *const CommandsA =
		"[[command]]\nnumber = 71\nname = \"Invite Guests\"\nwords = \"aleta sio\"\n"
		"[[command]]\nnumber = 103\nname = \"Reload Scripts\"\nwords = \"omevo gran cogni\"\n"
		"right = \"RELOAD_SCRIPTS\"\nscript = \"commands/reload_scripts.lua\"\n";
// Command 35 added.
static const char *const CommandsB =
		"[[command]]\nnumber = 71\nname = \"Invite Guests\"\nwords = \"aleta sio\"\n"
		"[[command]]\nnumber = 103\nname = \"Reload Scripts\"\nwords = \"omevo gran cogni\"\n"
		"right = \"RELOAD_SCRIPTS\"\nscript = \"commands/reload_scripts.lua\"\n"
		"[[command]]\nnumber = 35\nname = \"Get Item\"\nwords = \"alevo para\"\nright = \"CREATE_OBJECTS\"\n"
		"script = \"commands/get_item.lua\"\n";
// Command 71 gone.
static const char *const CommandsD =
		"[[command]]\nnumber = 103\nname = \"Reload Scripts\"\nwords = \"omevo gran cogni\"\n"
		"right = \"RELOAD_SCRIPTS\"\nscript = \"commands/reload_scripts.lua\"\n";

// The error text of a call that throws a const char *, "" when it does not.
template<typename F>
static std::string ErrorOf(F Call) {
	try {
		Call();
	} catch (const char *Error) {
		return Error;
	}
	return "";
}

static void Test(const std::string &CirclesFileA, const std::string &SpellsFileA, const std::string &CommandsFileA) {
	std::string CirclesFileB = WriteFile("circles_b.toml", CirclesB);
	std::string SpellsFileB = WriteFile("spells_b.toml", SpellsB);
	std::string CommandsFileB = WriteFile("commands_b.toml", CommandsB);

	// The start: nothing live, the staged set read and swapped in, as the
	// live loaders would have filled the live set; the names new static
	// strings in the files' order.
	CHECK(ErrorOf([&]() { ReadStagedMagicData(CirclesFileA.c_str(), SpellsFileA.c_str(), CommandsFileA.c_str()); })
			== "");
	TMagicChanges Start = CompareStagedMagicData();
	CHECK(Start.SpellsAdded == 2 && Start.CommandsAdded == 2 && Start.SpellsChanged == 0
			&& Start.CommandsChanged == 0 && Start.CirclesChanged);
	SwapStagedMagicData();
	ClearStagedMagicData();
	std::string StartDump = Dump();
	CHECK(SpellList[5].Comment < SpellList[10].Comment);
	CHECK(SpellList[10].Comment < SpellList[71].Comment);
	CHECK(SpellList[71].Comment < SpellList[103].Comment);
	CHECK(CommandRight[103][COMMAND_RIGHT] == GetRightByName("RELOAD_SCRIPTS"));
	CHECK(GetRightByName("RELOAD_SCRIPTS") == NUMBER_OF_RIGHTS - 2);
	CHECK(GetCommandRightChecks(SPELL_RELOAD_SCRIPTS).Count == 1);
	CHECK(GetSpellScript(103) != nullptr && strcmp(GetSpellScript(103), "commands/reload_scripts.lua") == 0);
	const char *StartLight = SpellList[10].Comment;

	// The live loaders give the same tables, with names of their own.
	LoadCircles(CirclesFileA.c_str());
	LoadSpells(SpellsFileA.c_str());
	LoadCommands(CommandsFileA.c_str());
	CHECK(Dump() == StartDump);
	CHECK(SpellList[10].Comment != StartLight);
	TSnapshot Live = Snapshot();

	// A read leaves the live set alone and counts the changes.
	CHECK(ErrorOf([&]() { ReadStagedMagicData(CirclesFileB.c_str(), SpellsFileB.c_str(), CommandsFileB.c_str()); })
			== "");
	CHECK(Snapshot() == Live);
	TMagicChanges Changes = CompareStagedMagicData();
	CHECK(Changes.SpellsChanged == 1);
	CHECK(Changes.SpellsAdded == 1);
	CHECK(Changes.CommandsChanged == 0);
	CHECK(Changes.CommandsAdded == 1);
	CHECK(Changes.CirclesChanged);

	// Installed: the live set is the new one; the unchanged name is a
	// string made before (the first one with its text).
	SwapStagedMagicData();
	CHECK(SpellList[5].Mana == 25);
	CHECK(SpellList[11].Syllable[0] == SYLLABLE_EX && strcmp(SpellList[11].Comment, "Intense Healing") == 0);
	CHECK(SpellList[35].Syllable[0] == SYLLABLE_AL
			&& CommandRight[35][COMMAND_RIGHT] == GetRightByName("CREATE_OBJECTS"));
	CHECK(GetSpellScript(35) != nullptr && strcmp(GetSpellScript(35), "commands/get_item.lua") == 0);
	CHECK(Circle[1].Count == 5);
	CHECK(SpellList[10].Comment == StartLight);
	const char *Intense = SpellList[11].Comment;

	// Uninstalled: the live set as it was, byte for byte.
	SwapStagedMagicData();
	CHECK(Snapshot() == Live);
	ClearStagedMagicData();
	CHECK(Snapshot() == Live);

	// A second read of the same files makes no new name.
	CHECK(ErrorOf([&]() { ReadStagedMagicData(CirclesFileB.c_str(), SpellsFileB.c_str(), CommandsFileB.c_str()); })
			== "");
	SwapStagedMagicData();
	CHECK(SpellList[11].Comment == Intense);
	SwapStagedMagicData();
	ClearStagedMagicData();
	CHECK(Snapshot() == Live);

	// The same files: nothing changed.
	CHECK(ErrorOf([&]() { ReadStagedMagicData(CirclesFileA.c_str(), SpellsFileA.c_str(), CommandsFileA.c_str()); })
			== "");
	TMagicChanges None = CompareStagedMagicData();
	CHECK(None.SpellsChanged == 0 && None.SpellsAdded == 0 && None.CommandsChanged == 0 && None.CommandsAdded == 0
			&& !None.CirclesChanged);
	ClearStagedMagicData();

	// A spell or a command that is gone is refused; the live set stays.
	std::string SpellsFileC = WriteFile("spells_c.toml", SpellsC);
	std::string Error = ErrorOf([&]() {
		ReadStagedMagicData(CirclesFileA.c_str(), SpellsFileC.c_str(), CommandsFileA.c_str());
	});
	CHECK_CONTAINS(Error, SpellsFileC + ": spell 10 (Light) is gone");
	ClearStagedMagicData();
	CHECK(Snapshot() == Live);
	std::string CommandsFileD = WriteFile("commands_d.toml", CommandsD);
	Error = ErrorOf([&]() {
		ReadStagedMagicData(CirclesFileA.c_str(), SpellsFileA.c_str(), CommandsFileD.c_str());
	});
	CHECK_CONTAINS(Error, CommandsFileD + ": command 71 (Invite Guests) is gone");
	ClearStagedMagicData();
	CHECK(Snapshot() == Live);

	// A wrong file: the loaders' error with its position; the live set stays.
	std::string Wrong = WriteFile("spells_wrong.toml",
			"[[spell]]\nnumber = 5\nname = \"Healing\"\nwords = \"exuraa\"\n");
	Error = ErrorOf([&]() {
		ReadStagedMagicData(CirclesFileA.c_str(), Wrong.c_str(), CommandsFileA.c_str());
	});
	CHECK_CONTAINS(Error, Wrong + ":4:");
	CHECK_CONTAINS(Error, "unknown syllable");
	ClearStagedMagicData();
	CHECK(Snapshot() == Live);

	// A command checks against the staged spells, not the live ones: with
	// B live, 11 is a live spell's number, which the staged spells of A lack,
	// so a command may take it (the spell is not gone, it became a command).
	LoadCircles(CirclesFileB.c_str());
	LoadSpells(SpellsFileB.c_str());
	LoadCommands(CommandsFileB.c_str());
	Live = Snapshot();
	std::string CommandsEleven = WriteFile("commands_11.toml",
			std::string(CommandsB) + "[[command]]\nnumber = 11\nname = \"x\"\nwords = \"alani sio\"\n");
	CHECK(ErrorOf([&]() {
		ReadStagedMagicData(CirclesFileA.c_str(), SpellsFileA.c_str(), CommandsEleven.c_str());
	}) == "");
	TMagicChanges Moved = CompareStagedMagicData();
	CHECK(Moved.SpellsChanged == 1 && Moved.CommandsChanged == 1 && Moved.SpellsAdded == 0
			&& Moved.CommandsAdded == 0);
	ClearStagedMagicData();
	CHECK(Snapshot() == Live);
	std::string Twice = WriteFile("commands_10.toml",
			std::string(CommandsA) + "[[command]]\nnumber = 10\nname = \"x\"\nwords = \"alani sio\"\n");
	CHECK_CONTAINS(ErrorOf([&]() {
		ReadStagedMagicData(CirclesFileA.c_str(), SpellsFileA.c_str(), Twice.c_str());
	}), "10 is the number of a spell in spells.toml");
	ClearStagedMagicData();
	CHECK(Snapshot() == Live);
}

int main() {
	Folder = fs::temp_directory_path() / ("patella-test-magic-reload-" + std::to_string(getpid()));
	fs::create_directories(Folder);
	try {
		Test(WriteFile("circles_a.toml", CirclesA), WriteFile("spells_a.toml", SpellsA),
				WriteFile("commands_a.toml", CommandsA));
	} catch (const char *Error) {
		fprintf(stderr, "unexpected error: %s\n", Error);
		Failures++;
	}
	fs::remove_all(Folder);
	if (Failures != 0) {
		fprintf(stderr, "%d failures\n", Failures);
		return 1;
	}
	printf("all magic reload tests passed\n");
	return 0;
}
