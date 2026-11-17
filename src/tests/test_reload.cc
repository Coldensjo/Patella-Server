// test_reload.cc: the driver of the reload (reload.cc) with fake parts, on the
// Lua engine, without the game. The steps in their order (read and install
// part by part, the new state, the Register*API functions in InitAll's order,
// loadScripts, finish, commit), a failure in read, in loadScripts and in the
// API (uninstall in reverse, discard in reverse, the new state thrown away),
// the copy of a thrown text that the part overwrites at once, the summary in
// the log, a second request refused until the reload is done, and nothing
// done without a request. The game's functions reload.cc calls (GetPlayer,
// Log, SendMessage, the Register*API functions, RefreshCreaturesAfterReload)
// are stubs here. Prints what
// failed; exits 1 on any failure, 0 when all pass.

#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <string>
#include <vector>

#include "objects.hh"
#include "map.hh"
#include "info.hh"
#include "cr.hh"
#include "crmain.hh"
#include "crplayer.hh"
#include "sending.hh"
#include "writer.hh"
#include "utils.hh"
#include "scripting_api.hh"
#include "reload.hh"

namespace fs = std::filesystem;

static int Failures = 0;

#define CHECK(Condition) \
	do { \
		if (!(Condition)) { \
			fprintf(stderr, "%s:%d: CHECK failed: %s\n", __FILE__, __LINE__, #Condition); \
			Failures++; \
		} \
	} while (0)

// What happened, in order: "<part>.<step>", "api.<name>", "engine.<what>".
static std::vector<std::string> Steps;

// The lines Log("game", ...) was given.
static std::vector<std::string> Logged;

// Where a fake part fails: nowhere, in read or in loadScripts.
enum TFailAt { FAIL_NOWHERE, FAIL_READ, FAIL_LOAD };

// The API function that throws, or nullptr.
static const char *FailingAPI = nullptr;

// A part that records its steps, says whether the engine's reload has begun
// in loadScripts, and fails where FailAt says, throwing a text in a buffer of
// its own that it overwrites right after the throw is caught (by its next
// step), as a static error buffer would be.
class TFakePart : public TReloadPart {
public:
	explicit TFakePart(const char *Name) : Name(Name) {
	}
	const char *name() const override {
		return Name;
	}
	void read() override {
		record("read");
		if (FailAt == FAIL_READ) {
			fail("cannot read");
		}
	}
	void install() override {
		record("install");
	}
	void loadScripts() override {
		record(ScriptReloadActive() ? "loadScripts(new state)" : "loadScripts(old state)");
		if (FailAt == FAIL_LOAD) {
			fail("cannot load");
		}
	}
	void finish() override {
		record("finish");
	}
	void uninstall() override {
		record("uninstall");
		strcpy(Buffer, "overwritten");
	}
	void discard() override {
		record("discard");
		strcpy(Buffer, "overwritten");
	}
	std::string summary() const override {
		return std::string("summary of ") + Name;
	}

	TFailAt FailAt = FAIL_NOWHERE;
	// What the failure's text has at its end.
	std::string Extra;

private:
	void record(const char *Step) {
		Steps.push_back(std::string(Name) + "." + Step);
	}
	[[noreturn]] void fail(const char *What) {
		snprintf(Buffer, sizeof(Buffer), "%s.toml:3:7: %s%s", Name, What, Extra.c_str());
		throw static_cast<const char *>(Buffer);
	}

	const char *Name;
	char Buffer[1024] = "";
};

static TFakePart Items("items");
static TFakePart ItemUses("item uses");
static TFakePart Events("events");
static TFakePart Triggers("triggers");
static TFakePart Magic("magic");
static TFakePart Races("races");
static TFakePart Raids("raids");
static TFakePart NpcScripts("npc scripts");

static TFakePart *const AllParts[] = {&Items, &ItemUses, &Events, &Triggers, &Magic, &Races, &Raids, &NpcScripts};

TReloadPart &ItemsReloadPart() {
	return Items;
}

TReloadPart &ItemUsesReloadPart() {
	return ItemUses;
}

TReloadPart &EventsReloadPart() {
	return Events;
}

TReloadPart &TriggersReloadPart() {
	return Triggers;
}

TReloadPart &MagicReloadPart() {
	return Magic;
}

TReloadPart &RacesReloadPart() {
	return Races;
}

TReloadPart &RaidsReloadPart() {
	return Raids;
}

TReloadPart &NpcScriptsReloadPart() {
	return NpcScripts;
}

// The API functions: each records itself; FailingAPI throws as
// ScriptAPITable would.
static void API(const char *Name) {
	Steps.push_back(std::string("api.") + Name);
	if (FailingAPI != nullptr && strcmp(FailingAPI, Name) == 0) {
		throw "ScriptAPITable: the shared global x is not an API table";
	}
}

void RegisterMagicAPI() {
	API("magic");
}

void RegisterTriggerAPI() {
	API("trigger");
}

void RegisterItemAPI() {
	API("item");
}

void RegisterNpcAPI() {
	API("npc");
}

void RegisterMonsterAPI() {
	API("monster");
}

void RegisterEventAPI() {
	API("event");
}

// Records whether the engine's reload is committed when it is called.
void RefreshCreaturesAfterReload() {
	Steps.push_back(ScriptReloadActive() ? "creatures.refresh(not committed)" : "creatures.refresh");
}

// No player is online in this test.
TPlayer *GetPlayer(uint32_t) {
	return nullptr;
}

void SendMessage(TConnection *, int, char *, ...) {
	Steps.push_back("message");
}

void Log(char *ProtocolName, char *Text, ...) {
	char Line[1024];
	va_list Arguments;
	va_start(Arguments, Text);
	vsnprintf(Line, sizeof(Line), Text, Arguments);
	va_end(Arguments);
	CHECK(strcmp(ProtocolName, "game") == 0);
	Logged.push_back(Line);
}

static void Reset() {
	Steps.clear();
	Logged.clear();
	FailingAPI = nullptr;
	for (TFakePart *Part : AllParts) {
		Part->FailAt = FAIL_NOWHERE;
		Part->Extra.clear();
	}
}

// Prints Actual when it is not Expected.
static void CheckList(const std::vector<std::string> &Actual, const std::vector<std::string> &Expected, int Line) {
	if (Actual != Expected) {
		fprintf(stderr, "%s:%d: CHECK failed: the list differs\n", __FILE__, Line);
		for (const std::string &Entry : Actual) {
			fprintf(stderr, "  got: %s", Entry.c_str());
			if (Entry.empty() || Entry.back() != '\n') {
				fputs("\n", stderr);
			}
		}
		Failures++;
	}
}

#define CHECK_LIST(Actual, ...) CheckList(Actual, std::vector<std::string>__VA_ARGS__, __LINE__)

// The first line of the log without its time ("... done in 3 ms." becomes
// "... done in N ms.").
static std::string FirstLine() {
	if (Logged.empty()) {
		return "";
	}
	std::string Line = Logged[0];
	size_t In = Line.find(" in ");
	size_t Ms = Line.find(" ms");
	if (In != std::string::npos && Ms != std::string::npos && Ms > In) {
		Line = Line.substr(0, In + 4) + "N" + Line.substr(Ms);
	}
	return Line;
}

// Every step in order and the summary.
static void TestSuccess() {
	Reset();
	sol::state *Before = &ScriptState();
	CHECK(RequestScriptReload(7));
	// A second request before the reload is refused.
	CHECK(!RequestScriptReload(8));
	ProcessScriptReload();
	CHECK_LIST(Steps, {
		"items.read", "items.install", "item uses.read", "item uses.install", "events.read", "events.install",
		"triggers.read", "triggers.install", "magic.read", "magic.install", "races.read", "races.install",
		"raids.read", "raids.install", "npc scripts.read", "npc scripts.install",
		"api.magic", "api.trigger", "api.item", "api.npc", "api.monster", "api.event",
		"items.loadScripts(new state)", "item uses.loadScripts(new state)", "events.loadScripts(new state)",
		"triggers.loadScripts(new state)", "magic.loadScripts(new state)", "races.loadScripts(new state)",
		"raids.loadScripts(new state)", "npc scripts.loadScripts(new state)",
		"items.finish", "item uses.finish", "events.finish", "triggers.finish", "magic.finish", "races.finish",
		"raids.finish", "npc scripts.finish", "creatures.refresh",
	});
	CHECK(!ScriptReloadActive());
	CHECK(&ScriptState() != Before);
	CHECK(FirstLine() == "Reload of the scripts and data by player 7: done in N ms.\n");
	CHECK_LIST(std::vector<std::string>(Logged.begin() + 1, Logged.end()), {
		"  summary of items\n", "  summary of item uses\n", "  summary of events\n",
		"  summary of triggers\n", "  summary of magic\n", "  summary of races\n",
		"  summary of raids\n", "  summary of npc scripts\n",
	});

	// Done: a new request is taken; without one nothing happens.
	Reset();
	ProcessScriptReload();
	CHECK(Steps.empty() && Logged.empty());
	CHECK(RequestScriptReload(8));
	ProcessScriptReload();
	CHECK(Steps.size() == 39);
	CHECK(FirstLine() == "Reload of the scripts and data by player 8: done in N ms.\n");
}

// A read that fails: the parts installed before it uninstalled in reverse,
// every part read (that one too) discarded in reverse, no new state.
static void TestReadFailure() {
	Reset();
	sol::state *Before = &ScriptState();
	Triggers.FailAt = FAIL_READ;
	CHECK(RequestScriptReload(7));
	ProcessScriptReload();
	CHECK_LIST(Steps, {
		"items.read", "items.install", "item uses.read", "item uses.install", "events.read", "events.install",
		"triggers.read",
		"events.uninstall", "item uses.uninstall", "items.uninstall",
		"triggers.discard", "events.discard", "item uses.discard", "items.discard",
	});
	CHECK(!ScriptReloadActive());
	CHECK(&ScriptState() == Before);
	CHECK(FirstLine() == "Reload of the scripts and data by player 7: failed in N ms, nothing changed.\n");
	// A failed reload logs its error and no summaries (nothing changed).
	CHECK_LIST(std::vector<std::string>(Logged.begin() + 1, Logged.end()), {
		"  error: triggers: triggers.toml:3:7: cannot read\n",
	});
}

// A loadScripts that fails: everything uninstalled and discarded in reverse,
// the new state thrown away, the old one current; the text copied before the
// part overwrote it.
static void TestLoadFailure() {
	Reset();
	sol::state *Before = &ScriptState();
	Races.FailAt = FAIL_LOAD;
	CHECK(RequestScriptReload(7));
	ProcessScriptReload();
	std::vector<std::string> Expected = {
		"items.read", "items.install", "item uses.read", "item uses.install", "events.read", "events.install",
		"triggers.read", "triggers.install", "magic.read", "magic.install", "races.read", "races.install",
		"raids.read", "raids.install", "npc scripts.read", "npc scripts.install",
		"api.magic", "api.trigger", "api.item", "api.npc", "api.monster", "api.event",
		"items.loadScripts(new state)", "item uses.loadScripts(new state)", "events.loadScripts(new state)",
		"triggers.loadScripts(new state)", "magic.loadScripts(new state)", "races.loadScripts(new state)",
		"npc scripts.uninstall", "raids.uninstall", "races.uninstall", "magic.uninstall", "triggers.uninstall",
		"events.uninstall", "item uses.uninstall", "items.uninstall",
		"npc scripts.discard", "raids.discard", "races.discard", "magic.discard", "triggers.discard",
		"events.discard", "item uses.discard", "items.discard",
	};
	CheckList(Steps, Expected, __LINE__);
	CHECK(!ScriptReloadActive());
	CHECK(&ScriptState() == Before);
	CHECK(Logged.size() == 2);
	CHECK(Logged.size() > 1 && Logged[1] == "  error: races: races.toml:3:7: cannot load\n");

	// An API function that throws: the same undoing, no loadScripts.
	Reset();
	FailingAPI = "monster";
	CHECK(RequestScriptReload(7));
	ProcessScriptReload();
	CHECK(Steps.size() == 16 + 5 + 8 + 8);
	CHECK(Steps.size() > 21 && Steps[20] == "api.monster" && Steps[21] == "npc scripts.uninstall");
	CHECK(!ScriptReloadActive());
	CHECK(&ScriptState() == Before);
	CHECK(Logged.size() > 1
			&& Logged[1] == "  error: engine: ScriptAPITable: the shared global x is not an API table\n");
}

// A long error goes on over further lines of the log.
static void TestLongError() {
	Reset();
	Items.FailAt = FAIL_READ;
	Items.Extra = std::string(300, 'x');
	CHECK(RequestScriptReload(7));
	ProcessScriptReload();
	std::string Error = "error: items: items.toml:3:7: cannot read" + std::string(300, 'x');
	CHECK(Logged.size() == 3);
	CHECK(Logged.size() == 3 && Logged[1] == "  " + Error.substr(0, 200) + "\n"
			&& Logged[2] == "    " + Error.substr(200) + "\n");

	// A begun reload of the engine (another caller's) makes the reload fail
	// at BeginScriptReload, after every part is installed, and undo them.
	Reset();
	BeginScriptReload();
	CHECK(RequestScriptReload(7));
	ProcessScriptReload();
	CHECK(Steps.size() == 16 + 8 + 8);
	CHECK(Logged.size() > 1 && Logged[1] == "  error: engine: BeginScriptReload: a reload has begun already\n");
	CHECK(ScriptReloadActive());
	DiscardScriptReload();
}

int main() {
	const char *Temporary = getenv("TMPDIR");
	std::string Template = std::string(Temporary != nullptr && Temporary[0] != 0 ? Temporary : "/tmp")
			+ "/patella-reload-XXXXXX";
	std::vector<char> Buffer(Template.begin(), Template.end());
	Buffer.push_back(0);
	if (mkdtemp(Buffer.data()) == nullptr) {
		perror("mkdtemp");
		return 1;
	}
	fs::path Folder = Buffer.data();

	InitScripting(Folder.c_str());
	TestSuccess();
	TestReadFailure();
	TestLoadFailure();
	TestLongError();
	ExitScripting();

	std::error_code Ignored;
	fs::remove_all(Folder, Ignored);
	if (Failures > 0) {
		fprintf(stderr, "%d check(s) failed\n", Failures);
		return 1;
	}
	printf("all checks passed\n");
	return 0;
}
