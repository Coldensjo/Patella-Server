// scripting_events.cc: the game events (scripting.hh, "Game events"). InitEvents
// reads the ordered list of event scripts of events.toml and loads them; the
// Event functions call the handlers onLogin, onLogout, onDeath, onAdvance and
// onSay of every listed script that defines them, in the file's order.
// RegisterEventAPI registers the TALKTYPE_ constants; InitAll calls it after
// RegisterMonsterAPI.
//
// Each Event function returns at once when no script defines its handler, so
// with no event script (the delivered events.toml) the game does what it did:
// no Lua, no random numbers, no logs. A handler's failure is logged and the
// next script is called (runCaught). A result code raised by onLogin,
// onLogout, onDeath or onAdvance is logged and the next script is called
// (their callers take none); one raised by onSay goes to Talk's caller, and
// the scripts after it are not called.

#include "tomlfile.hh"

#include <cstdint>
#include <cstdio>
#include <string>
#include <vector>

#include "objects.hh"
#include "map.hh"
#include "info.hh"
#include "operate.hh"
#include "scripting_api.hh"

// The longest script name of an [[event]]: the longest LoadScript takes.
static const int MAX_EVENT_SCRIPT_LENGTH = 256;

// The handlers, in the order of the lists below.
enum TEventHandler {
	EVENT_LOGIN,
	EVENT_LOGOUT,
	EVENT_DEATH,
	EVENT_ADVANCE,
	EVENT_SAY,
	NUMBER_OF_EVENT_HANDLERS
};

static const char *const HandlerNames[NUMBER_OF_EVENT_HANDLERS] = {
	"onLogin",
	"onLogout",
	"onDeath",
	"onAdvance",
	"onSay",
};

// Per handler, the numbers of the listed scripts that define it, in the order
// of events.toml.
static std::vector<int> HandlerScripts[NUMBER_OF_EVENT_HANDLERS];

struct TEventConstant {
	const char *Name;
	int Value;
};

// The talk modes of a player's Talk (operate.hh's TALK_MODE from TALK_SAY to
// TALK_ANIMAL_LOUD; the modes after them are the classes of the game's own
// messages) under TFS's names where the meaning is TFS's, under their 7.72
// names otherwise.
static const TEventConstant TalkTypes[] = {
	{"TALKTYPE_SAY", TALK_SAY},
	{"TALKTYPE_WHISPER", TALK_WHISPER},
	{"TALKTYPE_YELL", TALK_YELL},
	{"TALKTYPE_PRIVATE_TO", TALK_PRIVATE_MESSAGE},
	{"TALKTYPE_CHANNEL_Y", TALK_CHANNEL_CALL},
	{"TALKTYPE_GAMEMASTER_REQUEST", TALK_GAMEMASTER_REQUEST},
	{"TALKTYPE_GAMEMASTER_ANSWER", TALK_GAMEMASTER_ANSWER},
	{"TALKTYPE_PLAYER_ANSWER", TALK_PLAYER_ANSWER},
	{"TALKTYPE_BROADCAST", TALK_GAMEMASTER_BROADCAST},
	{"TALKTYPE_CHANNEL_R1", TALK_GAMEMASTER_CHANNELCALL},
	{"TALKTYPE_PRIVATE_RED_TO", TALK_GAMEMASTER_MESSAGE},
	{"TALKTYPE_CHANNEL_O", TALK_HIGHLIGHT_CHANNELCALL},
	{"TALKTYPE_ANONYMOUS_BROADCAST", TALK_ANONYMOUS_BROADCAST},
	{"TALKTYPE_ANONYMOUS_CHANNELCALL", TALK_ANONYMOUS_CHANNELCALL},
	{"TALKTYPE_ANONYMOUS_MESSAGE", TALK_ANONYMOUS_MESSAGE},
	{"TALKTYPE_MONSTER_SAY", TALK_ANIMAL_LOW},
	{"TALKTYPE_MONSTER_YELL", TALK_ANIMAL_LOUD},
};

static_assert(sizeof(TalkTypes) / sizeof(TalkTypes[0]) == TALK_ANIMAL_LOUD,
		"TalkTypes must name every talk mode from 1 to TALK_ANIMAL_LOUD");

void RegisterEventAPI() {
	for (const TEventConstant &Constant : TalkTypes) {
		SetScriptConstant(Constant.Name, Constant.Value);
	}
}

// Whether Name is the script name of an [[event]]: "events/<name>.lua" and a
// script name as scripting.hh defines it (only a-z, 0-9, '_', '-', '/' and
// '.'; no part empty, "." or ".."; a file name that is more than ".lua"; at
// most MAX_EVENT_SCRIPT_LENGTH characters).
static bool ValidEventScript(const std::string &Name) {
	const std::string Prefix = "events/";
	const std::string Ending = ".lua";
	if (Name.size() > static_cast<size_t>(MAX_EVENT_SCRIPT_LENGTH) || Name.size() <= Prefix.size() + Ending.size()
			|| Name.compare(0, Prefix.size(), Prefix) != 0
			|| Name.compare(Name.size() - Ending.size(), Ending.size(), Ending) != 0) {
		return false;
	}
	size_t PartStart = 0;
	for (size_t i = 0; i <= Name.size(); i++) {
		char c = i < Name.size() ? Name[i] : 0;
		if (c == 0 && i < Name.size()) {
			return false;
		}
		if (c == '/' || c == 0) {
			std::string Part = Name.substr(PartStart, i - PartStart);
			if (Part.empty() || Part == "." || Part == ".." || (c == 0 && Part == Ending)) {
				return false;
			}
			PartStart = i + 1;
		} else if (!((c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || c == '_' || c == '-' || c == '.')) {
			return false;
		}
	}
	return true;
}

// "<file>:<line>:<column>" of a place in a file.
static std::string Position(const toml::source_region &Where) {
	char Text[4096 + 64];
	snprintf(Text, sizeof(Text), "%s:%u:%u", Where.path != nullptr ? Where.path->c_str() : "(unknown file)",
			Where.begin.line, Where.begin.column);
	return Text;
}

// An entry of the list: the script's name and the place of its script key.
struct TEventEntry {
	std::string Script;
	std::string Where;
};

// One [[event]]: script needed, no other keys.
static TEventEntry ReadEvent(const toml::node &Node) {
	if (!Node.is_table()) {
		TomlError(Node.source(), "an entry of event must be a table");
	}
	const toml::table &Entry = *Node.as_table();
	TomlCheckKeys(Entry, "[[event]]", {"script"});

	auto ScriptFound = Entry.find("script");
	if (ScriptFound == Entry.end()) {
		TomlError(Node.source(), "an entry of event needs script");
	}
	// The errors and Where name the key's place, as for the scripts of
	// spells.toml and items.toml.
	const toml::source_region &Where = ScriptFound->first.source();
	if (!ScriptFound->second.is_string()) {
		TomlError(Where, "script must be a string");
	}
	const std::string &Name = ScriptFound->second.as_string()->get();
	if (!ValidEventScript(Name)) {
		TomlError(Where, "script \"%s\" must be a script name \"events/<name>.lua\" (a-z, 0-9, _, -, / and ., "
				"no part . or .., at most %d characters)", Name.c_str(), MAX_EVENT_SCRIPT_LENGTH);
	}
	return TEventEntry{Name, Position(Where)};
}

// The text InitEvents throws for a script named twice.
static std::string EventError;

void InitEvents(const char *FileName) {
	toml::table File = TomlParseFile(FileName);
	TomlCheckKeys(File, "the top level", {"event"});

	std::vector<TEventEntry> Entries;
	if (const toml::array *List = TomlGetArray(File, "event")) {
		for (const toml::node &Node : *List) {
			TEventEntry Entry = ReadEvent(Node);
			for (const TEventEntry &Earlier : Entries) {
				if (Earlier.Script == Entry.Script) {
					EventError = Entry.Where + ": script \"" + Entry.Script + "\" is listed twice (first at "
							+ Earlier.Where + ")";
					throw EventError.c_str();
				}
			}
			Entries.push_back(Entry);
		}
	}

	// The lists change only when every script has loaded.
	std::vector<int> Scripts[NUMBER_OF_EVENT_HANDLERS];
	for (const TEventEntry &Entry : Entries) {
		int Script = LoadScript(Entry.Script.c_str(), Entry.Where.c_str());
		for (int Handler = 0; Handler < NUMBER_OF_EVENT_HANDLERS; Handler++) {
			if (HasHandler(Script, HandlerNames[Handler])) {
				Scripts[Handler].push_back(Script);
			}
		}
	}
	for (int Handler = 0; Handler < NUMBER_OF_EVENT_HANDLERS; Handler++) {
		HandlerScripts[Handler].swap(Scripts[Handler]);
	}
}

void ExitEvents() {
	for (int Handler = 0; Handler < NUMBER_OF_EVENT_HANDLERS; Handler++) {
		HandlerScripts[Handler].clear();
	}
}

// Runs the call of a notify-only handler: a failure is logged (runCaught), and
// so is a result code, which its callers do not take.
static void RunNotify(TScriptCall &Call, int Script, TEventHandler Handler) {
	try {
		Call.runCaught();
	} catch (RESULT r) {
		error("%s of %s: result code %d, ignored.\n", HandlerNames[Handler], ScriptName(Script), static_cast<int>(r));
	}
}

void EventLogin(uint32_t PlayerID) {
	for (int Script : HandlerScripts[EVENT_LOGIN]) {
		TScriptCall Call(Script, HandlerNames[EVENT_LOGIN]);
		Call.addPlayer(PlayerID);
		RunNotify(Call, Script, EVENT_LOGIN);
	}
}

void EventLogout(uint32_t PlayerID) {
	for (int Script : HandlerScripts[EVENT_LOGOUT]) {
		TScriptCall Call(Script, HandlerNames[EVENT_LOGOUT]);
		Call.addPlayer(PlayerID);
		RunNotify(Call, Script, EVENT_LOGOUT);
	}
}

void EventDeath(uint32_t PlayerID, uint32_t KillerID) {
	for (int Script : HandlerScripts[EVENT_DEATH]) {
		TScriptCall Call(Script, HandlerNames[EVENT_DEATH]);
		Call.addPlayer(PlayerID);
		Call.addCreature(KillerID);
		RunNotify(Call, Script, EVENT_DEATH);
	}
}

void EventAdvance(uint32_t PlayerID, int Skill, int OldLevel, int NewLevel) {
	for (int Script : HandlerScripts[EVENT_ADVANCE]) {
		TScriptCall Call(Script, HandlerNames[EVENT_ADVANCE]);
		Call.addPlayer(PlayerID);
		Call.addNumber(Skill);
		Call.addNumber(OldLevel);
		Call.addNumber(NewLevel);
		RunNotify(Call, Script, EVENT_ADVANCE);
	}
}

// Whether an onSay is running: a Talk made by its script (player:say) calls
// no onSay, which would otherwise call itself without end.
static bool SayRunning = false;

// onSay's result with runCaughtBoolean: a failure (an error, a result that is
// not a boolean) is logged and counts as false; a result code goes to the
// caller.
bool EventSay(uint32_t PlayerID, const char *Text, int Mode) {
	if (SayRunning) {
		return false;
	}
	for (int Script : HandlerScripts[EVENT_SAY]) {
		TScriptCall Call(Script, HandlerNames[EVENT_SAY]);
		Call.addPlayer(PlayerID);
		Call.addString(Text);
		Call.addNumber(Mode);
		bool Consumed = false;
		SayRunning = true;
		try {
			Call.runCaughtBoolean(Consumed);
		} catch (...) {
			SayRunning = false;
			throw;
		}
		SayRunning = false;
		if (Consumed) {
			return true;
		}
	}
	return false;
}
