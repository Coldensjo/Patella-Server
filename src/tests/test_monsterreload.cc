// test_monsterreload.cc: the reload's checks of monsterdata.cc, without the
// game. CheckRacesReload refuses a race number that is gone (removed or
// renumbered) and a race whose key changed, and allows everything else (at
// start nothing exists to compare with); CompareRaces and CompareRaids count
// the added, changed and removed entries, ignoring where an entry stands in
// the file. The files are written into a scratch folder that is removed at
// the end. Prints what failed; exits 1 on any failure, 0 when all pass.

#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>
#include <unistd.h>

#include "monsterdata.hh"

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

// Writes the file Name in the scratch folder with Text and gives its path.
static std::string WriteFile(const std::string &Name, const std::string &Text) {
	fs::path Path = Folder / Name;
	std::ofstream Out(Path, std::ios::binary);
	Out << Text;
	return Path.string();
}

// A [[monster]] entry with the key, number and experience given.
static std::string Race(const std::string &Key, int Number, int Experience = 5) {
	return "[[monster]]\n"
			"key = \"" + Key + "\"\n"
			"number = " + std::to_string(Number) + "\n"
			"name = \"" + Key + "\"\n"
			"article = \"a\"\n"
			"outfit = { id = 21, colors = [0, 0, 0, 0] }\n"
			"corpse = 2813\n"
			"blood = \"blood\"\n"
			"experience = " + std::to_string(Experience) + "\n"
			"summon_cost = 0\n"
			"flee_threshold = 0\n"
			"attack = 1\n"
			"defend = 1\n"
			"armor = 1\n"
			"poison = 0\n"
			"lose_target = 0\n"
			"strategy = [100, 0, 0, 0]\n"
			"talk = [\"Squeak!\"]\n"
			"[[monster.spell]]\n"
			"shape = \"actor\"\n"
			"effect = 1\n"
			"impact = \"healing\"\n"
			"amount = 10\n"
			"variation = 5\n"
			"one_in = 3\n\n";
}

// Reads the races of Text (written to Name).
static std::vector<TMonsterData> Races(const std::string &Name, const std::string &Text) {
	std::vector<TMonsterData> Result;
	try {
		LoadMonsterRaces(WriteFile(Name, Text).c_str(), Result);
	} catch (const char *Error) {
		fprintf(stderr, "LoadMonsterRaces of %s failed: %s\n", Name.c_str(), Error);
		Failures++;
	}
	return Result;
}

// The text CheckRacesReload throws, "" when it throws nothing.
static std::string Refusal(const std::vector<TMonsterData> &Old, const std::vector<TMonsterData> &New) {
	try {
		CheckRacesReload("monsters.toml", Old, New);
	} catch (const char *Error) {
		return Error;
	}
	return "";
}

static bool Same(const TReloadChanges &Changes, int Added, int Changed, int Removed) {
	return Changes.Added == Added && Changes.Changed == Changed && Changes.Removed == Removed;
}

static void TestRaces() {
	std::vector<TMonsterData> Old = Races("old.toml", Race("rat", 1) + Race("demon", 2));
	CHECK(Old.size() == 2);

	// The start: nothing to compare with.
	CHECK(Refusal({}, Old).empty());
	CHECK(Same(CompareRaces({}, Old), 2, 0, 0));

	// The same races, standing elsewhere in the file: nothing changed.
	std::vector<TMonsterData> Moved = Races("moved.toml", "# a comment\n\n" + Race("rat", 1) + Race("demon", 2));
	CHECK(Refusal(Old, Moved).empty());
	CHECK(Same(CompareRaces(Old, Moved), 0, 0, 0));

	// A race added, one changed.
	std::vector<TMonsterData> More = Races("more.toml", Race("rat", 1) + Race("demon", 2, 6000) + Race("orc", 3));
	CHECK(Refusal(Old, More).empty());
	CHECK(Same(CompareRaces(Old, More), 1, 1, 0));

	// A race number gone.
	std::vector<TMonsterData> Gone = Races("gone.toml", Race("rat", 1));
	std::string Text = Refusal(Old, Gone);
	CHECK_CONTAINS(Text, "monsters.toml: race number 2 (\"demon\") is gone");

	// A race renumbered: its old number is gone.
	std::vector<TMonsterData> Renumbered = Races("renumbered.toml", Race("rat", 1) + Race("demon", 5));
	CHECK_CONTAINS(Refusal(Old, Renumbered), "race number 2 (\"demon\") is gone");

	// A race's key changed: at the new entry's position.
	std::vector<TMonsterData> Rekeyed = Races("rekeyed.toml", Race("rat", 1) + Race("demon2", 2));
	Text = Refusal(Old, Rekeyed);
	CHECK_CONTAINS(Text, "rekeyed.toml:");
	CHECK_CONTAINS(Text, "race number 2 had the key \"demon\", now \"demon2\"");

	// Two keys exchanged between numbers: refused at the first.
	std::vector<TMonsterData> Exchanged = Races("exchanged.toml", Race("demon", 1) + Race("rat", 2));
	CHECK_CONTAINS(Refusal(Old, Exchanged), "race number 1 had the key \"rat\", now \"demon\"");

	// A changed spell, talk line or inventory counts as a change.
	std::string Changed = Race("rat", 1) + Race("demon", 2);
	size_t Amount = Changed.rfind("amount = 10");
	Changed.replace(Amount, 11, "amount = 11");
	CHECK(Same(CompareRaces(Old, Races("spell.toml", Changed)), 0, 1, 0));
	Changed = Race("rat", 1) + Race("demon", 2);
	Changed.replace(Changed.rfind("Squeak!"), 7, "Squeal!");
	CHECK(Same(CompareRaces(Old, Races("talk.toml", Changed)), 0, 1, 0));

	// Removed (counted, though CheckRacesReload refuses it).
	CHECK(Same(CompareRaces(Old, Gone), 0, 0, 1));
}

// A [[raid]] entry with one wave of rats.
static std::string Raid(const std::string &Key, int Interval, const std::string &Message = "Rats!") {
	return "[[raid]]\n"
			"key = \"" + Key + "\"\n"
			"type = \"small\"\n"
			"interval = " + std::to_string(Interval) + "\n"
			"[[raid.wave]]\n"
			"delay = 0\n"
			"position = [32000, 32000, 7]\n"
			"spread = 5\n"
			"race = \"rat\"\n"
			"count = [1, 3]\n"
			"message = \"" + Message + "\"\n\n";
}

static std::vector<TRaidData> Raids(const std::vector<TMonsterData> &Known, const std::string &Name,
		const std::string &Text) {
	std::vector<TRaidData> Result;
	try {
		LoadRaids(WriteFile(Name, Text).c_str(), Known, Result);
	} catch (const char *Error) {
		fprintf(stderr, "LoadRaids of %s failed: %s\n", Name.c_str(), Error);
		Failures++;
	}
	return Result;
}

static void TestRaids() {
	std::vector<TMonsterData> Known = Races("races.toml", Race("rat", 1));
	std::vector<TRaidData> Old = Raids(Known, "raids_old.toml", Raid("one", 1000) + Raid("two", 2000));
	CHECK(Old.size() == 2);
	CHECK(Same(CompareRaids({}, Old), 2, 0, 0));
	CHECK(Same(CompareRaids(Old, Raids(Known, "raids_moved.toml", "\n\n" + Raid("one", 1000) + Raid("two", 2000))),
			0, 0, 0));
	// "two" changed (its wave's message), "one" removed, "three" added.
	std::vector<TRaidData> New = Raids(Known, "raids_new.toml", Raid("two", 2000, "More rats!") + Raid("three", 10));
	CHECK(Same(CompareRaids(Old, New), 1, 1, 1));
	// A raid's interval changed.
	CHECK(Same(CompareRaids(Old, Raids(Known, "raids_interval.toml", Raid("one", 1001) + Raid("two", 2000))), 0, 1,
			0));

	// A raid naming a race that is not among the races is refused by the
	// loader (a reload checks against the installed races).
	std::vector<TRaidData> Result;
	std::string Text = Raid("one", 1000);
	Text.replace(Text.find("\"rat\""), 5, "\"orc\"");
	bool Refused = false;
	try {
		LoadRaids(WriteFile("raids_orc.toml", Text).c_str(), Known, Result);
	} catch (const char *) {
		Refused = true;
	}
	CHECK(Refused);
}

int main() {
	Folder = fs::temp_directory_path() / ("patella-test-monsterreload-" + std::to_string(getpid()));
	fs::create_directories(Folder);
	TestRaces();
	TestRaids();
	fs::remove_all(Folder);
	if (Failures != 0) {
		fprintf(stderr, "%d checks failed.\n", Failures);
		return 1;
	}
	printf("All checks passed.\n");
	return 0;
}
