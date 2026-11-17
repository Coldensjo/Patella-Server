// test_triggers_reload.cc: the staged tables of the trigger rules
// (triggersdata.cc), which the reload's triggers part (triggers.cc,
// TriggersReloadPart) reads, swaps in and out and empties, without the game.
// ReadStagedTriggerRules fills the staged set as LoadTriggerRules fills the
// live one, its texts dynamic strings in the file's order, and leaves the live
// set alone; CompareStagedTriggerRules counts the rules changed, added and
// removed; SwapStagedTriggerRules twice gives back the live set byte for byte
// (the containers' own bytes included); ClearStagedTriggerRules deletes the
// staged texts; a wrong file deletes the texts it made. The rules name no
// object type, so no items.toml is needed. The files are written into a
// scratch folder that is removed at the end. Prints what failed; exits 1 on
// any failure, 0 when all pass.

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>
#include <unistd.h>

#include "strings.hh"
#include "triggersdata.hh"

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

// The raw bytes of an object.
template<typename T>
static void AddBytes(std::vector<unsigned char> &Bytes, const T &Object) {
	const unsigned char *p = reinterpret_cast<const unsigned char *>(&Object);
	Bytes.insert(Bytes.end(), p, p + sizeof(T));
}

// The live tables byte for byte: the containers' own bytes (their memory
// and bounds), the used entries, the counts and the lists.
static std::vector<unsigned char> Snapshot() {
	std::vector<unsigned char> Bytes;
	AddBytes(Bytes, TriggerConditions);
	AddBytes(Bytes, TriggerActions);
	AddBytes(Bytes, NumberOfTriggerConditions);
	AddBytes(Bytes, NumberOfTriggerActions);
	for (int i = 1; i <= NumberOfTriggerConditions; i++) {
		AddBytes(Bytes, TriggerConditions(i));
	}
	for (int i = 1; i <= NumberOfTriggerActions; i++) {
		AddBytes(Bytes, TriggerActions(i));
	}
	for (int Event = 0; Event < NUMBER_OF_EVENT_TYPES; Event++) {
		AddBytes(Bytes, TriggerDatabases[Event]);
		for (int r = 1; r <= TriggerDatabases[Event].NumberOfRules; r++) {
			AddBytes(Bytes, TriggerDatabases[Event].Rules(r));
		}
	}
	for (const TTriggerRuleEntry &Entry : TriggerRules) {
		AddBytes(Bytes, Entry);
	}
	for (const TTriggerScript &Script : TriggerScripts) {
		std::string Text = Script.Name + "|" + Script.Where + "|" + std::to_string(Script.Number) + "\n";
		Bytes.insert(Bytes.end(), Text.begin(), Text.end());
	}
	for (const std::vector<std::string> *Places : {&TriggerConditionPlaces, &TriggerActionPlaces}) {
		for (const std::string &Place : *Places) {
			Bytes.insert(Bytes.end(), Place.begin(), Place.end());
			Bytes.push_back('\n');
		}
	}
	for (uint32_t Text : TriggerTexts) {
		AddBytes(Bytes, Text);
	}
	return Bytes;
}

// The texts of the live rules, in the order made.
static std::vector<std::string> Texts() {
	std::vector<std::string> Result;
	for (uint32_t Number : TriggerTexts) {
		const char *Text = GetDynamicString(Number);
		Result.push_back(Text != nullptr ? Text : "(none)");
	}
	return Result;
}

// The live rules as text, a text by its content: what two loads that make
// different dynamic strings must agree on.
static std::string Dump() {
	std::string Out;
	for (size_t k = 0; k < TriggerRules.size(); k++) {
		const TTriggerRuleEntry &Entry = TriggerRules[k];
		const TTriggerRule &Rule = TriggerDatabases[Entry.Event].Rules(Entry.Rule);
		Out += "rule " + std::to_string(Rule.Number) + " event " + std::to_string(Entry.Event) + " #"
				+ std::to_string(Entry.Rule) + " script " + std::to_string(Rule.Script) + " conditions "
				+ std::to_string(Rule.FirstCondition) + "-" + std::to_string(Rule.LastCondition) + " actions "
				+ std::to_string(Rule.FirstAction) + "-" + std::to_string(Rule.LastAction) + "\n";
	}
	for (const std::string &Text : Texts()) {
		Out += "text " + Text + "\n";
	}
	return Out;
}

static const char *const RulesA =
		"[[rule]]\n"
		"event = \"use\"\n"
		"conditions = [{ name = \"is_creature\", args = [\"user\"] }]\n"
		"actions = [{ name = \"text\", args = [\"obj1\", \"Hello\", 1] }]\n"
		"\n"
		"[[rule]]\n"
		"event = \"movement\"\n"
		"script = \"triggers/gate.lua\"\n"
		"\n"
		"[[rule]]\n"
		"event = \"collision\"\n"
		"conditions = [{ name = \"random\", args = [50] }]\n"
		"actions = [\n"
		"\t{ name = \"effect_on_map\", args = [[32000, 32000, 7], 3] },\n"
		"\t{ name = \"text_on_map\", args = [[32000, 32001, 7], \"Boom\", 2] },\n"
		"]\n";
// Rule 1's text changed, rules 2 and 3 the same, rule 4 added.
static const char *const RulesB =
		"[[rule]]\n"
		"event = \"use\"\n"
		"conditions = [{ name = \"is_creature\", args = [\"user\"] }]\n"
		"actions = [{ name = \"text\", args = [\"obj1\", \"Hello there\", 1] }]\n"
		"\n"
		"[[rule]]\n"
		"event = \"movement\"\n"
		"script = \"triggers/gate.lua\"\n"
		"\n"
		"[[rule]]\n"
		"event = \"collision\"\n"
		"conditions = [{ name = \"random\", args = [50] }]\n"
		"actions = [\n"
		"\t{ name = \"effect_on_map\", args = [[32000, 32000, 7], 3] },\n"
		"\t{ name = \"text_on_map\", args = [[32000, 32001, 7], \"Boom\", 2] },\n"
		"]\n"
		"\n"
		"[[rule]]\n"
		"event = \"use\"\n"
		"conditions = [{ name = \"is_player\", args = [\"user\"], negate = true }]\n"
		"actions = [{ name = \"nop\" }]\n";
// Rules 1 and 2 of A, the second with another script: one changed, one
// removed.
static const char *const RulesC =
		"[[rule]]\n"
		"event = \"use\"\n"
		"conditions = [{ name = \"is_creature\", args = [\"user\"] }]\n"
		"actions = [{ name = \"text\", args = [\"obj1\", \"Hello\", 1] }]\n"
		"\n"
		"[[rule]]\n"
		"event = \"movement\"\n"
		"script = \"triggers/door.lua\"\n";
// A text, then an error on line 8.
static const char *const RulesWrong =
		"[[rule]]\n"
		"event = \"use\"\n"
		"conditions = [{ name = \"is_creature\", args = [\"user\"] }]\n"
		"actions = [{ name = \"text\", args = [\"obj1\", \"Lost\", 1] }]\n"
		"\n"
		"[[rule]]\n"
		"event = \"use\"\n"
		"conditions = [{ name = \"is_creatur\", args = [\"user\"] }]\n"
		"actions = [{ name = \"nop\" }]\n";

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

// The number the next dynamic string gets: made, deleted and cleaned up.
static uint32_t NextDynamicString() {
	CleanupDynamicStrings();
	uint32_t Number = AddDynamicString(const_cast<char *>("probe"));
	DeleteDynamicString(Number);
	CleanupDynamicStrings();
	return Number;
}

static void Test() {
	std::string FileA = WriteFile("a.toml", RulesA);
	std::string FileB = WriteFile("b.toml", RulesB);
	std::string FileC = WriteFile("c.toml", RulesC);

	// The start: no dynamic string yet, nothing live; the staged set read and
	// swapped in. Its texts are the dynamic strings 1 and 2, in the file's
	// order.
	CHECK(ErrorOf([&]() { ReadStagedTriggerRules(FileA.c_str()); }) == "");
	TTriggerChanges Start = CompareStagedTriggerRules();
	CHECK(Start.Changed == 0 && Start.Added == 3 && Start.Removed == 0);
	SwapStagedTriggerRules();
	ClearStagedTriggerRules();
	CHECK(TriggerTexts == std::vector<uint32_t>({1, 2}));
	CHECK(Texts() == std::vector<std::string>({"Hello", "Boom"}));
	CHECK(TriggerDatabases[EVENT_USE].NumberOfRules == 1 && TriggerDatabases[EVENT_MOVEMENT].NumberOfRules == 1
			&& TriggerDatabases[EVENT_COLLISION].NumberOfRules == 1);
	CHECK(NumberOfTriggerConditions == 2 && NumberOfTriggerActions == 3);
	CHECK(TriggerScripts.size() == 1 && TriggerScripts[0].Name == "triggers/gate.lua"
			&& TriggerScripts[0].Number == -1);
	CHECK(TriggerActionPlaces.size() == 4);
	CHECK_CONTAINS(TriggerActionPlaces[3], FileA + ":15:");
	std::string StartDump = Dump();

	// LoadTriggerRules gives the same tables, with texts of its own.
	NumberOfTriggerConditions = 0;
	NumberOfTriggerActions = 0;
	LoadTriggerRules(FileA.c_str());
	CHECK(Dump() == StartDump);
	CHECK(TriggerTexts == std::vector<uint32_t>({3, 4}));
	std::vector<unsigned char> Live = Snapshot();
	std::vector<uint32_t> LiveTexts = TriggerTexts;

	// A read leaves the live set alone and counts the changes; its texts
	// come after, in the file's order.
	uint32_t Next = NextDynamicString();
	CHECK(ErrorOf([&]() { ReadStagedTriggerRules(FileB.c_str()); }) == "");
	CHECK(Snapshot() == Live);
	TTriggerChanges Changes = CompareStagedTriggerRules();
	CHECK(Changes.Changed == 1);
	CHECK(Changes.Added == 1);
	CHECK(Changes.Removed == 0);

	// Installed: the live set is the new one.
	SwapStagedTriggerRules();
	CHECK(TriggerTexts == std::vector<uint32_t>({Next, Next + 1}));
	CHECK(Texts() == std::vector<std::string>({"Hello there", "Boom"}));
	CHECK(TriggerDatabases[EVENT_USE].NumberOfRules == 2);
	CHECK(TriggerRules.size() == 4);
	CHECK(TriggerConditions(TriggerDatabases[EVENT_USE].Rules(2).FirstCondition).Modifier == MODIFIER_INVERT);

	// Uninstalled: the live set as it was, byte for byte; the new texts still
	// there until the staged set is emptied, which deletes them and leaves
	// the live texts.
	SwapStagedTriggerRules();
	CHECK(Snapshot() == Live);
	CHECK(GetDynamicString(Next) != nullptr);
	ClearStagedTriggerRules();
	CHECK(Snapshot() == Live);
	CHECK(GetDynamicString(Next) == nullptr);
	CHECK(GetDynamicString(Next + 1) == nullptr);
	CHECK(Texts() == std::vector<std::string>({"Hello", "Boom"}));

	// Fewer rules: one changed (the script), one removed.
	CHECK(ErrorOf([&]() { ReadStagedTriggerRules(FileC.c_str()); }) == "");
	TTriggerChanges Fewer = CompareStagedTriggerRules();
	CHECK(Fewer.Changed == 1 && Fewer.Added == 0 && Fewer.Removed == 1);
	ClearStagedTriggerRules();

	// The same file: nothing changed.
	CHECK(ErrorOf([&]() { ReadStagedTriggerRules(FileA.c_str()); }) == "");
	TTriggerChanges None = CompareStagedTriggerRules();
	CHECK(None.Changed == 0 && None.Added == 0 && None.Removed == 0);
	ClearStagedTriggerRules();
	CHECK(Snapshot() == Live);

	// A wrong file: the error with its position; the text made before the
	// error is deleted again; the live set stays.
	std::string FileWrong = WriteFile("wrong.toml", RulesWrong);
	Next = NextDynamicString();
	std::string Error = ErrorOf([&]() { ReadStagedTriggerRules(FileWrong.c_str()); });
	CHECK_CONTAINS(Error, FileWrong + ":8:");
	CHECK_CONTAINS(Error, "unknown condition \"is_creatur\"");
	CHECK(GetDynamicString(Next) == nullptr);
	CHECK(Snapshot() == Live);
	ClearStagedTriggerRules();
	CHECK(Snapshot() == Live);
	CHECK_CONTAINS(ErrorOf([&]() { ReadStagedTriggerRules((Folder / "missing.toml").c_str()); }), "missing.toml");
	CHECK(Snapshot() == Live);

	// Finished: the new set installed and the old one emptied, which deletes
	// the old texts.
	CHECK(ErrorOf([&]() { ReadStagedTriggerRules(FileB.c_str()); }) == "");
	SwapStagedTriggerRules();
	ClearStagedTriggerRules();
	for (uint32_t Number : LiveTexts) {
		CHECK(GetDynamicString(Number) == nullptr);
	}
	CHECK(Texts() == std::vector<std::string>({"Hello there", "Boom"}));
	CHECK(TriggerRules.size() == 4);
}

int main() {
	Folder = fs::temp_directory_path() / ("patella-test-triggers-reload-" + std::to_string(getpid()));
	fs::create_directories(Folder);
	try {
		Test();
	} catch (const char *Error) {
		fprintf(stderr, "unexpected error: %s\n", Error);
		Failures++;
	}
	fs::remove_all(Folder);
	if (Failures != 0) {
		fprintf(stderr, "%d failures\n", Failures);
		return 1;
	}
	printf("all trigger reload tests passed\n");
	return 0;
}
