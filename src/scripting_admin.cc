// scripting_admin.cc: the commands of the admin port (admin.hh; scripting.hh,
// section "The admin port"): admin.toml and its scripts (InitAdminCommands,
// with the steps of the admin part of a reload, AdminReloadPart), the
// carrying out of a request (ProcessAdminRequests: the words of its line, the
// command's onCommand, a run over every character, help), the character a
// command has open (online or offline, its inventory and depots) and the API
// table Admin (RegisterAdminAPI).
//
// A request's line is split into words; the first names the command
// ("help": the list), the others go to its onCommand as an array. What the
// script tells (Admin.reply, Admin.fail) is the reply. A script error is
// logged and replied ("Script error: ..."), a result code replied with its
// name; either makes the reply an ERROR. A command that calls
// Admin.runOverCharacters has onCharacter(characterId, words) called for each
// character of the player index, as many each beat as a share of it allows
// (RunBudget), and then onRunEnd(words, total, characters), total the sum of
// what onCharacter
// returned; the client waits for the reply until the run ends. A reload during
// a run ends it. A character a script left open is closed without its changes
// after each handler call (and an error is logged when the handler did not
// fail).
//
// The open character (Admin.openCharacter): online, a player in the game
// (GetPlayer), whose inventory is its body containers and whose open depot is
// the depot box it stands at; the other depots are loaded from its player
// data into depot lockers made on the newbie start field (as houses.cc does
// for the depots of house owners) when a script asks for them. Offline, its
// player data slot is assigned (loaded from the query manager as a login
// loads it) and its inventory and depots are loaded into such lockers too.
// Admin.closeCharacter writes back what changed (each inventory and depot
// compared with what it was as written) into the player data, saves an
// offline character's slot at once (SavePlayerPoolSlot) and releases it, and
// deletes the lockers. A depot locker never holds the depot of a player's
// open depot box (that one is used as it is).

#include "tomlfile.hh"

#include <algorithm>
#include <chrono>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <strings.h>
#include <string>
#include <tuple>
#include <vector>

#include "config.hh"
#include "objects.hh"
#include "map.hh"
#include "mapdata.hh"
#include "info.hh"
#include "cr.hh"
#include "crplayer.hh"
#include "dbfuncs.hh"
#include "operate.hh"
#include "utils.hh"
#include "writer.hh"
#include "admin.hh"
#include "communication.hh"
#include "crskill.hh"
#include "houses.hh"
#include "main.hh"
#include "query.hh"
#include "sending.hh"
#include "writer.hh"
#include "reload.hh"
#include "scripting_api.hh"

// The share of each beat a run over the characters may take: one beat in
// ADMIN_RUN_SHARE, at least ADMIN_RUN_MINIMUM milliseconds (an offline
// character costs a load from the query manager, and a save when it changed;
// the beat's other work must still fit).
static const int ADMIN_RUN_SHARE = 4;
static const int ADMIN_RUN_MINIMUM = 5;

static std::chrono::milliseconds RunBudget() {
	return std::chrono::milliseconds(std::max(Beat / ADMIN_RUN_SHARE, ADMIN_RUN_MINIMUM));
}

// The most words of a request's line and the longest word.
static const int MAX_ADMIN_WORDS = 16;
static const int MAX_ADMIN_WORD = 255;

// The longest name of a command, and of its script.
static const int MAX_ADMIN_NAME = 30;
static const int MAX_ADMIN_SCRIPT_LENGTH = 256;

// The size the depot and inventory streams start with (as SaveDepot's).
static const int ADMIN_STREAM_SIZE = 16384;

// ---------------------------------------------------------------------------
// admin.toml

struct TAdminCommand {
	std::string Name;
	std::string Usage;
	std::string Description;
	std::string Script;
	std::string Where;
};

// The commands in use and their scripts' numbers (one per command, in the
// same order).
static std::vector<TAdminCommand> Commands;
static std::vector<int> CommandScripts;

// A town of admin.toml: its name and the start field of its citizens (the
// temple), for sethome.
struct TAdminTown {
	std::string Name;
	int x = 0;
	int y = 0;
	int z = 0;
};

static std::vector<TAdminTown> Towns;

// "<file>:<line>:<column>" of a place in a file.
static std::string PlaceText(const toml::source_region &Where) {
	char Text[4096 + 64];
	snprintf(Text, sizeof(Text), "%s:%u:%u", Where.path != nullptr ? Where.path->c_str() : "(unknown file)",
			Where.begin.line, Where.begin.column);
	return Text;
}

// Whether Name is a command's name: a-z, 0-9 and '_', 1 to MAX_ADMIN_NAME
// characters, not "help".
static bool ValidCommandName(const std::string &Name) {
	if (Name.empty() || Name.size() > static_cast<size_t>(MAX_ADMIN_NAME) || Name == "help") {
		return false;
	}
	for (char c : Name) {
		if (!((c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || c == '_')) {
			return false;
		}
	}
	return true;
}

// Whether Name is the script name of a command: "admin/<name>.lua" and a
// script name as scripting.hh defines it (only a-z, 0-9, '_', '-', '/' and
// '.'; no part empty, "." or ".."; a file name that is more than ".lua"; at
// most MAX_ADMIN_SCRIPT_LENGTH characters).
static bool ValidAdminScript(const std::string &Name) {
	const std::string Prefix = "admin/";
	const std::string Ending = ".lua";
	if (Name.size() > static_cast<size_t>(MAX_ADMIN_SCRIPT_LENGTH) || Name.size() <= Prefix.size() + Ending.size()
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

// The text of a string key of Entry, which must be there.
static std::string RequiredText(const toml::table &Entry, const toml::node &Node, const char *Key) {
	auto Found = Entry.find(Key);
	if (Found == Entry.end()) {
		TomlError(Node.source(), "an entry of command needs %s", Key);
	}
	if (!Found->second.is_string()) {
		TomlError(Found->second.source(), "%s must be a string", Key);
	}
	std::string Latin1;
	if (!Utf8ToLatin1(Found->second.as_string()->get(), Latin1)) {
		TomlError(Found->second.source(), "%s has a character outside Latin-1", Key);
	}
	return Latin1;
}

// One [[town]]: name (at most 29 characters) and temple ([x, y, z]), no
// other keys.
static TAdminTown ReadTown(const toml::node &Node) {
	if (!Node.is_table()) {
		TomlError(Node.source(), "an entry of town must be a table");
	}
	const toml::table &Entry = *Node.as_table();
	TomlCheckKeys(Entry, "[[town]]", {"name", "temple"});
	TAdminTown Town;
	char Name[30];
	if (!TomlGetString(Entry, "name", Name, sizeof(Name))) {
		TomlError(Node.source(), "an entry of town needs name");
	}
	Town.Name = Name;
	const toml::array *Temple = TomlGetArray(Entry, "temple");
	if (Temple == nullptr) {
		TomlError(Node.source(), "an entry of town needs temple");
	}
	int Values[3] = {0, 0, 0};
	if (Temple->size() != 3) {
		TomlError(Entry.find("temple")->second.source(), "temple must be [x, y, z]");
	}
	for (size_t i = 0; i < 3; i++) {
		const toml::node &Value = (*Temple)[i];
		if (!Value.is_integer() || Value.as_integer()->get() < 0
				|| Value.as_integer()->get() > (i < 2 ? 65535 : 15)) {
			TomlError(Value.source(), "temple must be [x, y, z], x and y 0 to 65535, z 0 to 15");
		}
		Values[i] = static_cast<int>(Value.as_integer()->get());
	}
	Town.x = Values[0];
	Town.y = Values[1];
	Town.z = Values[2];
	return Town;
}

// One [[command]]: name, usage, description and script needed, no other keys.
static TAdminCommand ReadCommand(const toml::node &Node) {
	if (!Node.is_table()) {
		TomlError(Node.source(), "an entry of command must be a table");
	}
	const toml::table &Entry = *Node.as_table();
	TomlCheckKeys(Entry, "[[command]]", {"name", "usage", "description", "script"});

	TAdminCommand Command;
	Command.Name = RequiredText(Entry, Node, "name");
	if (!ValidCommandName(Command.Name)) {
		TomlError(Entry.find("name")->second.source(),
				"name \"%s\" must be a-z, 0-9 and _, at most %d characters, not help", Command.Name.c_str(),
				MAX_ADMIN_NAME);
	}
	Command.Usage = RequiredText(Entry, Node, "usage");
	Command.Description = RequiredText(Entry, Node, "description");
	Command.Script = RequiredText(Entry, Node, "script");
	const toml::source_region &Where = Entry.find("script")->first.source();
	if (!ValidAdminScript(Command.Script)) {
		TomlError(Where, "script \"%s\" must be a script name \"admin/<name>.lua\" (a-z, 0-9, _, -, / and ., "
				"no part . or .., at most %d characters)", Command.Script.c_str(), MAX_ADMIN_SCRIPT_LENGTH);
	}
	Command.Where = PlaceText(Where);
	return Command;
}

// ---------------------------------------------------------------------------
// The request and the run

// The request being carried out: its reply so far and whether it failed.
static bool RequestActive = false;
static std::string Reply;
static bool ReplyFailed = false;
static std::string RequestText;

// Whether onCommand is running (Admin.runOverCharacters and Admin.retry may
// be called), whether it asked for a run or a retry, and the retries before
// this call of it.
static bool InCommand = false;
static bool RunAsked = false;
static bool RetryAsked = false;
static int CommandTries = 0;

// A command waiting to be called again (Admin.retry): its script and words,
// the round of the call and the retries so far.
struct TAdminRetry {
	bool Active = false;
	int Script = 0;
	std::vector<std::string> Words;
	uint32_t Round = 0;
	int Tries = 0;
};

static TAdminRetry Retry;

// The rounds a command waits for its retry, and the most retries it gets.
static const int ADMIN_RETRY_ROUNDS = 2;
static const int MAX_ADMIN_RETRIES = 5;

// The run over the characters: the command's script and words, the
// characters (taken when it began), the next one, the sum of onCharacter's
// results and the characters whose onCharacter failed.
struct TAdminRun {
	bool Active = false;
	int Script = 0;
	std::vector<std::string> Words;
	std::vector<uint32_t> Characters;
	size_t Next = 0;
	long long Total = 0;
	int Failures = 0;
};

static TAdminRun Run;

// The open character (see the top).
struct TAdminSession {
	bool Open = false;
	uint32_t CharacterID = 0;
	bool Online = false;
	// The player data: an offline character's assigned slot, a player's own.
	TPlayerData *Slot = nullptr;
	// An offline character's inventory: per body position a locker holding
	// that position's object.
	Object Inventory[LAST_BODY_POSITION + 1];
	// The depots loaded into lockers (NONE: not loaded), and whether the
	// player's open depot box was handed out.
	Object Depot[MAX_DEPOTS];
	bool LiveDepotUsed = false;
	// An offline character's new saved position.
	bool PositionSet = false;
	int x = 0;
	int y = 0;
	int z = 0;
	// An offline character's new end of its muting (MutingEndRound) and new
	// start field (its temple).
	bool MutingSet = false;
	uint32_t MutingEndRound = 0;
	bool StartSet = false;
	int StartX = 0;
	int StartY = 0;
	int StartZ = 0;
};

static TAdminSession Session;

static void CloseSession(bool WriteBack);

// Ends the request with its reply and logs it.
static void FinishRequest() {
	Log("admin", "%s -> %s\n", RequestText.c_str(), ReplyFailed ? "failed" : "done");
	AnswerAdminRequest(!ReplyFailed, Reply);
	RequestActive = false;
	Reply.clear();
	RequestText.clear();
	ReplyFailed = false;
}

// Adds a line to the reply.
static void ReplyLine(const std::string &Text) {
	Reply += Text;
	Reply += '\n';
}

// Splits Line into words: separated by spaces and tabs, a word in double
// quotes may hold them. false (with Error set) for an unclosed quote, too
// many words or a word too long.
static bool SplitWords(const std::string &Line, std::vector<std::string> &Words, std::string &Error) {
	Words.clear();
	size_t i = 0;
	while (true) {
		while (i < Line.size() && (Line[i] == ' ' || Line[i] == '\t')) {
			i++;
		}
		if (i >= Line.size()) {
			return true;
		}
		std::string Word;
		if (Line[i] == '"') {
			size_t End = Line.find('"', i + 1);
			if (End == std::string::npos) {
				Error = "A quote is not closed.";
				return false;
			}
			Word = Line.substr(i + 1, End - i - 1);
			i = End + 1;
		} else {
			size_t Start = i;
			while (i < Line.size() && Line[i] != ' ' && Line[i] != '\t') {
				i++;
			}
			Word = Line.substr(Start, i - Start);
		}
		if (Word.size() > static_cast<size_t>(MAX_ADMIN_WORD)) {
			Error = "A word is longer than " + std::to_string(MAX_ADMIN_WORD) + " characters.";
			return false;
		}
		if (Words.size() >= static_cast<size_t>(MAX_ADMIN_WORDS)) {
			Error = "More than " + std::to_string(MAX_ADMIN_WORDS) + " words.";
			return false;
		}
		Words.push_back(Word);
	}
}

// The C strings of Words from From on, for addStringArray.
static std::vector<const char *> WordPointers(const std::vector<std::string> &Words, size_t From) {
	std::vector<const char *> Pointers;
	for (size_t i = From; i < Words.size(); i++) {
		Pointers.push_back(Words[i].c_str());
	}
	return Pointers;
}

// Runs a handler call; false when it failed: a script error (logged by the
// engine) is replied as "Script error: <message>", a result code as "Failed:
// <its name>". Number is the handler's number for a call with WantNumber.
// Closes a character the handler left open without its changes (and logs
// that when the handler did not fail).
static bool RunHandler(TScriptCall &Call, int Script, const char *Handler, bool WantNumber, int &Number) {
	Number = 0;
	bool Ok = true;
	try {
		if (WantNumber) {
			Number = Call.runNumber();
		} else {
			Call.run();
		}
	} catch (RESULT r) {
		Ok = false;
		const char *Failure = Call.failure();
		if (Failure != nullptr) {
			ReplyLine(std::string("Script error: ") + Failure);
		} else {
			const char *Name = ScriptResultName(static_cast<int>(r));
			ReplyLine(std::string("Failed: ") + (Name != nullptr ? Name : std::to_string(static_cast<int>(r))));
		}
	}
	if (Session.Open) {
		if (Ok) {
			error("Admin: %s of %s left character %lu open; its changes are dropped.\n", Handler,
					ScriptName(Script), static_cast<unsigned long>(Session.CharacterID));
		}
		CloseSession(false);
	}
	return Ok;
}

// The help: every command with its usage and description.
static void ReplyHelp() {
	ReplyLine("help");
	ReplyLine("    Lists the commands.");
	for (const TAdminCommand &Command : Commands) {
		ReplyLine(Command.Usage);
		ReplyLine("    " + Command.Description);
	}
}

// Calls the command's onCommand with Words (Tries: the retries before this
// call); then starts a run when it asked for one, waits ADMIN_RETRY_ROUNDS
// when it asked for a retry (Admin.retry), else answers.
static void RunCommand(int Script, const std::vector<std::string> &Words, int Tries) {
	std::vector<const char *> Pointers = WordPointers(Words, 0);
	TScriptCall Call(Script, "onCommand");
	Call.addStringArray(Pointers.data(), static_cast<int>(Pointers.size()));
	InCommand = true;
	RunAsked = false;
	RetryAsked = false;
	CommandTries = Tries;
	int Unused;
	if (!RunHandler(Call, Script, "onCommand", false, Unused)) {
		ReplyFailed = true;
	}
	InCommand = false;

	if (RetryAsked && !ReplyFailed) {
		RetryAsked = false;
		RunAsked = false;
		Retry.Active = true;
		Retry.Script = Script;
		Retry.Words = Words;
		Retry.Round = RoundNr + ADMIN_RETRY_ROUNDS;
		Retry.Tries = Tries + 1;
		return;
	}
	RetryAsked = false;

	if (!RunAsked || ReplyFailed) {
		RunAsked = false;
		FinishRequest();
		return;
	}
	if (!HasHandler(Script, "onCharacter")) {
		RunAsked = false;
		ReplyFailed = true;
		ReplyLine(std::string("Script error: ") + ScriptName(Script) + " asks for a run but has no onCharacter.");
		FinishRequest();
		return;
	}

	RunAsked = false;
	Run.Active = true;
	Run.Script = Script;
	Run.Words = Words;
	GetAllCharacterIDs(Run.Characters);
	Run.Next = 0;
	Run.Total = 0;
	Run.Failures = 0;
	print(2, "Admin: going over %d characters.\n", static_cast<int>(Run.Characters.size()));
}

// Carries out a request's line: help, or the command's onCommand; starts a
// run when it asked for one, else answers.
static void ExecuteRequest(const std::string &Line) {
	RequestActive = true;
	RequestText = Line;
	Reply.clear();
	ReplyFailed = false;

	std::vector<std::string> Words;
	std::string Error;
	if (!SplitWords(Line, Words, Error)) {
		ReplyFailed = true;
		ReplyLine(Error);
		FinishRequest();
		return;
	}
	if (Words.empty()) {
		ReplyFailed = true;
		ReplyLine("No command given. \"help\" lists the commands.");
		FinishRequest();
		return;
	}

	std::string Name = Words[0];
	for (char &c : Name) {
		if (c >= 'A' && c <= 'Z') {
			c = static_cast<char>(c - 'A' + 'a');
		}
	}
	if (Name == "help") {
		ReplyHelp();
		FinishRequest();
		return;
	}

	int Script = -1;
	for (size_t i = 0; i < Commands.size(); i++) {
		if (Commands[i].Name == Name) {
			Script = CommandScripts[i];
		}
	}
	if (Script < 0) {
		ReplyFailed = true;
		ReplyLine("Unknown command \"" + Words[0] + "\". \"help\" lists the commands.");
		FinishRequest();
		return;
	}

	RunCommand(Script, std::vector<std::string>(Words.begin() + 1, Words.end()), 0);
}

// Ends the run with onRunEnd (if the script has it) and answers.
static void EndRun() {
	int Script = Run.Script;
	if (HasHandler(Script, "onRunEnd")) {
		std::vector<const char *> Pointers = WordPointers(Run.Words, 0);
		TScriptCall Call(Script, "onRunEnd");
		Call.addStringArray(Pointers.data(), static_cast<int>(Pointers.size()));
		Call.addNumber(static_cast<int>(Run.Total));
		Call.addNumber(static_cast<int>(Run.Characters.size()));
		int Unused;
		if (!RunHandler(Call, Script, "onRunEnd", false, Unused)) {
			ReplyFailed = true;
		}
	}
	if (Run.Failures > 0) {
		ReplyFailed = true;
		ReplyLine(std::to_string(Run.Failures) + " characters failed (the game's error log has why).");
	}
	Run = TAdminRun();
	FinishRequest();
}

// Calls onCharacter for the next characters, at least one and then as long
// as the beat's share (RunBudget) lasts; ends the run after the last.
static void ContinueRun() {
	auto Start = std::chrono::steady_clock::now();
	for (int i = 0; Run.Next < Run.Characters.size(); i++) {
		if (i > 0 && std::chrono::steady_clock::now() - Start >= RunBudget()) {
			break;
		}
		uint32_t CharacterID = Run.Characters[Run.Next++];
		std::vector<const char *> Pointers = WordPointers(Run.Words, 0);
		TScriptCall Call(Run.Script, "onCharacter");
		Call.addNumber(CharacterID);
		Call.addStringArray(Pointers.data(), static_cast<int>(Pointers.size()));
		// One character's failure is counted and replied; the run goes on.
		int Result;
		if (RunHandler(Call, Run.Script, "onCharacter", true, Result)) {
			Run.Total += Result;
		} else {
			Run.Failures++;
		}
	}
	if (Run.Next >= Run.Characters.size()) {
		EndRun();
	}
}

void ProcessAdminRequests() {
	if (!AdminEnabled) {
		return;
	}
	if (Run.Active) {
		ContinueRun();
		return;
	}
	if (Retry.Active) {
		if (RoundNr >= Retry.Round) {
			TAdminRetry Again = Retry;
			Retry = TAdminRetry();
			RunCommand(Again.Script, Again.Words, Again.Tries);
		}
		return;
	}
	std::string Line;
	if (TakeAdminRequest(Line)) {
		ExecuteRequest(Line);
	}
}

// Ends a run that the reload or the end of the game stops.
static void StopRun(const char *Why) {
	if (Retry.Active) {
		Retry = TAdminRetry();
		ReplyFailed = true;
		ReplyLine(std::string("Stopped: ") + Why);
		FinishRequest();
		return;
	}
	if (!Run.Active) {
		return;
	}
	ReplyFailed = true;
	ReplyLine("Stopped after " + std::to_string(Run.Next) + " of " + std::to_string(Run.Characters.size())
			+ " characters: " + Why);
	Run = TAdminRun();
	FinishRequest();
}

// ---------------------------------------------------------------------------
// The open character


// A new depot locker on the newbie start field, where houses.cc makes the
// lockers of house owners' depots.
static Object NewLocker() {
	int x;
	int y;
	int z;
	GetStartPosition(x, y, z, true);
	return SetObject(GetMapContainer(x, y, z), GetSpecialObject(DEPOT_LOCKER), 0);
}

// The objects of Con as SaveObjects writes them.
static std::string StreamOf(Object Con) {
	TDynamicWriteBuffer Buffer(ADMIN_STREAM_SIZE);
	SaveObjects(GetFirstContainerObject(Con), Buffer, false);
	return std::string(reinterpret_cast<const char *>(Buffer.getData()), Buffer.getDataSize());
}

// The inventory of the lockers as SaveInventory writes it ("" for none).
static std::string InventoryStream() {
	TDynamicWriteBuffer Buffer(ADMIN_STREAM_SIZE);
	for (int Position = FIRST_BODY_POSITION; Position <= LAST_BODY_POSITION; Position++) {
		Object Obj = GetFirstContainerObject(Session.Inventory[Position]);
		if (Obj != NONE) {
			Buffer.writeByte(static_cast<unsigned char>(Position));
			SaveObjects(Obj, Buffer, false);
		}
	}
	if (Buffer.getDataSize() != 0) {
		Buffer.writeByte(END_OF_INVENTORY);
	}
	return std::string(reinterpret_cast<const char *>(Buffer.getData()), Buffer.getDataSize());
}

// Whether the bytes differ from a stored stream (nullptr: none).
static bool Differs(const std::string &Bytes, const unsigned char *Stored, int StoredSize) {
	if (Stored == nullptr) {
		return !Bytes.empty();
	}
	return Bytes.size() != static_cast<size_t>(StoredSize) || memcmp(Bytes.data(), Stored, Bytes.size()) != 0;
}

// Whether a depot locker holds only what LoadDepot makes of a depot that has
// no data: one empty depot chest.
static bool OnlyEmptyChest(Object Locker) {
	Object First = GetFirstContainerObject(Locker);
	return First != NONE && First.getNextObject() == NONE && First.getObjectType() == GetSpecialObject(DEPOT_CHEST)
			&& GetFirstContainerObject(First) == NONE;
}

// Deletes the lockers and forgets the session; an offline slot is released.
static void ForgetSession() {
	for (Object &Locker : Session.Inventory) {
		if (Locker != NONE && Locker.exists()) {
			DeleteObject(Locker);
		}
		Locker = NONE;
	}
	for (Object &Locker : Session.Depot) {
		if (Locker != NONE && Locker.exists()) {
			DeleteObject(Locker);
		}
		Locker = NONE;
	}
	if (!Session.Online && Session.Slot != nullptr) {
		ReleasePlayerPoolSlot(Session.Slot);
	}
	Session = TAdminSession();
}

// Writes back what changed (WriteBack) and closes the open character;
// nothing when none is open.
static void CloseSession(bool WriteBack) {
	if (!Session.Open) {
		return;
	}
	if (!WriteBack) {
		ForgetSession();
		return;
	}

	TPlayerData *Slot = Session.Slot;
	bool Changed = false;
	try {
		if (!Session.Online) {
			std::string Bytes = InventoryStream();
			if (Differs(Bytes, Slot->Inventory, Slot->InventorySize)) {
				unsigned char *NewInventory = nullptr;
				if (!Bytes.empty()) {
					NewInventory = new unsigned char[Bytes.size()];
					memcpy(NewInventory, Bytes.data(), Bytes.size());
				}
				delete[] Slot->Inventory;
				Slot->Inventory = NewInventory;
				Slot->InventorySize = static_cast<int>(Bytes.size());
				Changed = true;
			}
			if (Session.PositionSet) {
				Slot->posx = Session.x;
				Slot->posy = Session.y;
				Slot->posz = Session.z;
				Changed = true;
			}
			if (Session.MutingSet) {
				Slot->MutingEndRound = Session.MutingEndRound;
				Changed = true;
			}
			if (Session.StartSet) {
				Slot->startx = Session.StartX;
				Slot->starty = Session.StartY;
				Slot->startz = Session.StartZ;
				Changed = true;
			}
		}

		for (int Nr = 0; Nr < MAX_DEPOTS; Nr++) {
			Object Locker = Session.Depot[Nr];
			if (Locker == NONE) {
				continue;
			}
			bool Different = Slot->Depot[Nr] == nullptr ? !OnlyEmptyChest(Locker)
					: Differs(StreamOf(Locker), Slot->Depot[Nr], Slot->DepotSize[Nr]);
			if (Different) {
				SaveDepot(Slot, Nr, Locker);
				Changed = true;
			}
		}
	} catch (const char *s) {
		error("Admin.closeCharacter: Cannot write the data of character %lu (%s).\n",
				static_cast<unsigned long>(Session.CharacterID), s);
		ForgetSession();
		throw;
	} catch (RESULT r) {
		error("Admin.closeCharacter: Cannot write the data of character %lu (result %d).\n",
				static_cast<unsigned long>(Session.CharacterID), static_cast<int>(r));
		ForgetSession();
		throw;
	}

	if (Session.Online && Session.LiveDepotUsed) {
		TPlayer *pl = GetPlayer(Session.CharacterID);
		if (pl != nullptr && pl->Depot != NONE && pl->Depot.exists()) {
			pl->DepotSpace = GetDepotSize(pl->DepotNr, CheckRight(pl->ID, PREMIUM_ACCOUNT)) - (CountObjects(pl->Depot) - 1);
		}
	}

	if (Changed) {
		Slot->Dirty = true;
		if (!Session.Online) {
			SavePlayerPoolSlot(Slot);
		}
	}
	ForgetSession();
}

// The open character's player, nullptr when offline; throws when it left.
static TPlayer *SessionPlayer(const char *Name) {
	if (!Session.Online) {
		return nullptr;
	}
	TPlayer *pl = GetPlayer(Session.CharacterID);
	if (pl == nullptr) {
		std::string Text = std::string(Name) + ": the player has left the game";
		static std::string Message;
		Message = Text;
		throw Message.c_str();
	}
	return pl;
}

// Throws unless a request is being carried out and a character is open.
static void RequireSession(const char *Name) {
	if (!RequestActive) {
		throw "Admin: only the admin port's commands may call this";
	}
	if (!Session.Open) {
		static std::string Message;
		Message = std::string(Name) + ": no character is open (Admin.openCharacter)";
		throw Message.c_str();
	}
}

// Throws unless a request is being carried out.
static void RequireRequest() {
	if (!RequestActive) {
		throw "Admin: only the admin port's commands may call this";
	}
}

// ---------------------------------------------------------------------------
// The API

// A writable copy of a string for a C++ function that takes a char *.
static std::string Copy(const std::string &Text) {
	return std::string(Text.c_str());
}

// An object as an Item result: nil for NONE.
static sol::optional<TScriptItem> ItemResult(Object Obj) {
	if (Obj == NONE) {
		return sol::nullopt;
	}
	return TScriptItem{Obj.getObjectID()};
}

// Admin.reply(text): adds text as a line to the reply.
static void AdminReply(std::string Text) {
	RequireRequest();
	ReplyLine(Text);
}

// Admin.fail(text): adds text as a line to the reply and makes it an ERROR.
static void AdminFail(std::string Text) {
	RequireRequest();
	ReplyFailed = true;
	ReplyLine(Text);
}

// Admin.runOverCharacters(): asks for a run over every character after
// onCommand; only in onCommand, of a script with onCharacter.
static void AdminRunOverCharacters() {
	RequireRequest();
	if (!InCommand) {
		throw "Admin.runOverCharacters: only in onCommand";
	}
	RunAsked = true;
}

// Admin.findCharacter(name) -> the character's ID and its name as the player
// index has it, or nil: GetCharacterID of a copy (any case).
static std::tuple<sol::optional<uint32_t>, sol::optional<std::string>> AdminFindCharacter(std::string Name) {
	std::string Buffer = Copy(Name);
	if (Buffer.empty() || Buffer.size() >= 30) {
		return std::make_tuple(sol::nullopt, sol::nullopt);
	}
	uint32_t CharacterID = GetCharacterID(&Buffer[0]);
	if (CharacterID == 0) {
		return std::make_tuple(sol::nullopt, sol::nullopt);
	}
	return std::make_tuple(CharacterID, std::string(GetCharacterName(&Buffer[0])));
}

// Admin.openCharacter(characterId) -> whether it is online: opens the
// character (one at a time). Throws (a Lua error) when one is open already,
// for a player who is dying, or when its data cannot be loaded.
static bool AdminOpenCharacter(uint32_t CharacterID) {
	RequireRequest();
	if (Session.Open) {
		throw "Admin.openCharacter: a character is open already (Admin.closeCharacter first)";
	}
	if (CharacterID == 0) {
		throw "Admin.openCharacter: no character 0";
	}

	TPlayer *pl = GetPlayer(CharacterID);
	if (pl != nullptr) {
		if (pl->IsDead || pl->PlayerData == nullptr) {
			throw "Admin.openCharacter: the player is dying";
		}
		Session.Open = true;
		Session.CharacterID = CharacterID;
		Session.Online = true;
		Session.Slot = pl->PlayerData;
		return true;
	}

	TPlayerData *Slot = AssignPlayerPoolSlot(CharacterID, false);
	if (Slot == nullptr) {
		throw "Admin.openCharacter: cannot load the character's data";
	}
	Session.Open = true;
	Session.CharacterID = CharacterID;
	Session.Online = false;
	Session.Slot = Slot;
	try {
		for (int Position = FIRST_BODY_POSITION; Position <= LAST_BODY_POSITION; Position++) {
			Session.Inventory[Position] = NewLocker();
		}
		if (Slot->Inventory != nullptr) {
			TReadBuffer Buffer(Slot->Inventory, Slot->InventorySize);
			while (true) {
				int Position = Buffer.readByte();
				if (Position == END_OF_INVENTORY) {
					break;
				}
				if (Position < FIRST_BODY_POSITION || Position > LAST_BODY_POSITION) {
					throw "invalid body position";
				}
				LoadObjects(Buffer, Session.Inventory[Position]);
			}
		}
	} catch (const char *s) {
		error("Admin.openCharacter: Cannot read the inventory of character %lu (%s).\n",
				static_cast<unsigned long>(CharacterID), s);
		ForgetSession();
		throw "Admin.openCharacter: cannot read the character's inventory";
	}
	return false;
}

// Admin.closeCharacter(): writes back what changed and closes the character.
static void AdminCloseCharacter() {
	RequireSession("Admin.closeCharacter");
	CloseSession(true);
}

// Admin.getPlayer() -> the open character's Player, nil when it is offline.
static sol::optional<TScriptPlayer> AdminGetPlayer() {
	RequireSession("Admin.getPlayer");
	TPlayer *pl = SessionPlayer("Admin.getPlayer");
	if (pl == nullptr) {
		return sol::nullopt;
	}
	return TScriptPlayer{pl->ID};
}

// Admin.getName() -> the open character's name ("" when its data has none).
static std::string AdminGetName() {
	RequireSession("Admin.getName");
	TPlayer *pl = SessionPlayer("Admin.getName");
	return pl != nullptr ? std::string(pl->Name) : std::string(Session.Slot->Name);
}

// Admin.getInventoryContainer(slot) -> the container holding the open
// character's object at body position slot (CONST_SLOT_HEAD to
// CONST_SLOT_AMMO): a player's body container, an offline character's
// locker.
static TScriptItem AdminGetInventoryContainer(int Position) {
	RequireSession("Admin.getInventoryContainer");
	if (Position < FIRST_BODY_POSITION || Position > LAST_BODY_POSITION) {
		throw "Admin.getInventoryContainer: slot must be CONST_SLOT_HEAD to CONST_SLOT_AMMO";
	}
	TPlayer *pl = SessionPlayer("Admin.getInventoryContainer");
	if (pl != nullptr) {
		return TScriptItem{GetBodyContainer(pl->ID, Position).getObjectID()};
	}
	return TScriptItem{Session.Inventory[Position].getObjectID()};
}

// The locker (or open depot box) of depot Nr of the open character, loaded
// when first asked for.
static Object SessionDepot(int Nr, const char *Name) {
	if (Nr < 0 || Nr >= MAX_DEPOTS) {
		static std::string Message;
		Message = std::string(Name) + ": the depot number must be 0 to " + std::to_string(MAX_DEPOTS - 1);
		throw Message.c_str();
	}
	TPlayer *pl = SessionPlayer(Name);
	if (pl != nullptr && pl->Depot != NONE && pl->Depot.exists() && pl->DepotNr == Nr) {
		Session.LiveDepotUsed = true;
		return pl->Depot;
	}
	if (Session.Depot[Nr] == NONE) {
		Object Locker = NewLocker();
		Session.Depot[Nr] = Locker;
		LoadDepot(Session.Slot, Nr, Locker);
	}
	return Session.Depot[Nr];
}

// Admin.getDepotContainer(depot) -> the container of the open character's
// depot (0 to MAX_DEPOTS - 1): its locker, the one the player stands at for
// the depot open.
static TScriptItem AdminGetDepotContainer(int Nr) {
	RequireSession("Admin.getDepotContainer");
	return TScriptItem{SessionDepot(Nr, "Admin.getDepotContainer").getObjectID()};
}

// Admin.addToDepot(depot, type, value): puts a new object (Create's value)
// into the open character's depot: into its depot chest, else the locker,
// else, when both are full, into the locker anyway as mail is (the amount of
// a cumulative one set). Never onto the map.
static void AdminAddToDepot(int Nr, int Type, uint32_t Value) {
	RequireSession("Admin.addToDepot");
	Object Locker = SessionDepot(Nr, "Admin.addToDepot");
	ObjectType ObjType(Type);
	Object Chest = GetFirstContainerObject(Locker);
	while (Chest != NONE && Chest.getObjectType() != GetSpecialObject(DEPOT_CHEST)) {
		Chest = Chest.getNextObject();
	}
	if (Chest != NONE) {
		try {
			Create(Chest, ObjType, Value);
			return;
		} catch (RESULT) {
		}
	}
	try {
		Create(Locker, ObjType, Value);
		return;
	} catch (RESULT) {
	}
	Object Obj = SetObject(Locker, ObjType, 0);
	if (Obj.getFlag(CUMULATIVE)) {
		ChangeObject(Obj, AMOUNT, Value == 0 ? 1 : Value);
	}
}

// Admin.create(container, type, value) -> the Item Create(Con, Type, Value)
// of operate.cc makes, or nil when the place refuses it (no room, too heavy,
// not takeable; nothing is put elsewhere).
static sol::optional<TScriptItem> AdminCreate(TScriptItem Container, int Type, uint32_t Value) {
	RequireRequest();
	try {
		return ItemResult(Create(Object(Container.Handle), ObjectType(Type), Value));
	} catch (RESULT) {
		return sol::nullopt;
	}
}

// Admin.getCarrier(item) -> the creature ID of the creature carrying the
// item (GetObjectCreatureID), 0 for none.
static uint32_t AdminGetCarrier(TScriptItem Item) {
	Object Obj(Item.Handle);
	if (!Obj.exists()) {
		return 0;
	}
	return GetObjectCreatureID(Obj);
}

// Admin.getStartPosition() -> the open character's start field (its temple):
// a player's, an offline character's from its data (the newbie start when
// its data has none).
static TScriptPosition AdminGetStartPosition() {
	RequireSession("Admin.getStartPosition");
	TPlayer *pl = SessionPlayer("Admin.getStartPosition");
	if (pl != nullptr) {
		return TScriptPosition{pl->startx, pl->starty, pl->startz};
	}
	if (Session.Slot->startx != 0) {
		return TScriptPosition{Session.Slot->startx, Session.Slot->starty, Session.Slot->startz};
	}
	int x;
	int y;
	int z;
	GetStartPosition(x, y, z, true);
	return TScriptPosition{x, y, z};
}

// Admin.getPosition() -> where the open character is: a player's field, an
// offline character's saved position (as set, else from its data; the start
// field when its data has none).
static TScriptPosition AdminGetPosition() {
	RequireSession("Admin.getPosition");
	TPlayer *pl = SessionPlayer("Admin.getPosition");
	if (pl != nullptr) {
		return TScriptPosition{pl->posx, pl->posy, pl->posz};
	}
	if (Session.PositionSet) {
		return TScriptPosition{Session.x, Session.y, Session.z};
	}
	if (Session.Slot->posx != 0) {
		return TScriptPosition{Session.Slot->posx, Session.Slot->posy, Session.Slot->posz};
	}
	return AdminGetStartPosition();
}

// Admin.setSavedPosition(position): where the open offline character logs in
// next (written by Admin.closeCharacter); a field of the map. Not for a
// player online.
static void AdminSetSavedPosition(TScriptPosition Position) {
	RequireSession("Admin.setSavedPosition");
	if (Session.Online) {
		throw "Admin.setSavedPosition: the character is online";
	}
	if (!IsOnMap(Position.x, Position.y, Position.z)) {
		throw "Admin.setSavedPosition: the position is not on the map";
	}
	Session.PositionSet = true;
	Session.x = Position.x;
	Session.y = Position.y;
	Session.z = Position.z;
}

// Admin.getDepotTown(depot) -> the town of a depot number (map.toml), nil for
// a number without a depot.
static sol::optional<std::string> AdminGetDepotTown(int Nr) {
	if (Nr < DepotInfo.low() || Nr > DepotInfo.high() || DepotInfo(Nr).Town[0] == 0) {
		return sol::nullopt;
	}
	return std::string(DepotInfo(Nr).Town);
}

// Admin.retry(): asks for onCommand to be called again with the same words
// after ADMIN_RETRY_ROUNDS rounds (a kicked player has left by then); only in
// onCommand. Throws after MAX_ADMIN_RETRIES retries.
static void AdminRetry() {
	RequireRequest();
	if (!InCommand) {
		throw "Admin.retry: only in onCommand";
	}
	if (CommandTries >= MAX_ADMIN_RETRIES) {
		throw "Admin.retry: gave up after the retries";
	}
	RetryAsked = true;
}

// Admin.getTries() -> how many times onCommand was called again before this
// call (Admin.retry).
static int AdminGetTries() {
	RequireRequest();
	return CommandTries;
}

// Admin.broadcast(text): BroadcastMessage(TALK_ADMIN_MESSAGE, "%s", text), the
// red message in every player's screen.
static void AdminBroadcast(std::string Text) {
	RequireRequest();
	BroadcastMessage(TALK_ADMIN_MESSAGE, "%s", Text.c_str());
}

// Admin.getIPAddress(player) -> the address the player is connected from, nil
// for a player who has left.
static sol::optional<std::string> AdminGetIPAddress(TScriptPlayer Player) {
	TPlayer *pl = GetPlayer(Player.ID);
	if (pl == nullptr) {
		return sol::nullopt;
	}
	return std::string(pl->IPAddress);
}

// Admin.heal(): the open character, a player online, gets full hitpoints and
// mana and loses its poison, burning, energy and drunkenness; the clients
// are told. Not for an offline character.
static void AdminHeal() {
	RequireSession("Admin.heal");
	TPlayer *pl = SessionPlayer("Admin.heal");
	if (pl == nullptr) {
		throw "Admin.heal: the character is offline";
	}
	for (int SkillNr : {SKILL_POISON, SKILL_BURNING, SKILL_ENERGY, SKILL_DRUNKEN}) {
		if (pl->Skills[SkillNr] != nullptr) {
			pl->DelTimer(static_cast<unsigned short>(SkillNr));
		}
	}
	pl->Skills[SKILL_HITPOINTS]->Set(pl->Skills[SKILL_HITPOINTS]->GetMax());
	pl->Skills[SKILL_MANA]->Set(pl->Skills[SKILL_MANA]->GetMax());
	SendPlayerData(pl->GetConnection());
	AnnounceChangedCreature(pl->ID, CREATURE_HEALTH);
	pl->CheckState();
}

// Admin.setMuting(minutes): the open character may not talk (say, whisper,
// yell, private messages, the public channels) for minutes minutes from now
// (0: no longer), as a muting by the game does; offline, written by
// Admin.closeCharacter. The game counts it in rounds of the running game.
static void AdminSetMuting(int Minutes) {
	RequireSession("Admin.setMuting");
	if (Minutes < 0 || Minutes > 525600) {
		throw "Admin.setMuting: minutes must be 0 to 525600";
	}
	uint32_t End = Minutes == 0 ? 0 : RoundNr + static_cast<uint32_t>(Minutes) * 60;
	TPlayer *pl = SessionPlayer("Admin.setMuting");
	if (pl != nullptr) {
		pl->SetMutingEndRound(End);
		return;
	}
	Session.MutingSet = true;
	Session.MutingEndRound = End;
}

// Admin.setStartPosition(position): the open character's start field (its
// temple; where it comes back after a death and where temple sends it); a
// field of the map. Offline, written by Admin.closeCharacter.
static void AdminSetStartPosition(TScriptPosition Position) {
	RequireSession("Admin.setStartPosition");
	if (!IsOnMap(Position.x, Position.y, Position.z)) {
		throw "Admin.setStartPosition: the position is not on the map";
	}
	TPlayer *pl = SessionPlayer("Admin.setStartPosition");
	if (pl != nullptr) {
		pl->startx = Position.x;
		pl->starty = Position.y;
		pl->startz = Position.z;
		return;
	}
	Session.StartSet = true;
	Session.StartX = Position.x;
	Session.StartY = Position.y;
	Session.StartZ = Position.z;
}

// Admin.getTownTemple(name) -> the temple of a town of admin.toml (any case),
// nil for none.
static sol::optional<TScriptPosition> AdminGetTownTemple(std::string Name) {
	for (const TAdminTown &Town : Towns) {
		if (strcasecmp(Town.Name.c_str(), Name.c_str()) == 0) {
			return TScriptPosition{Town.x, Town.y, Town.z};
		}
	}
	return sol::nullopt;
}

// Admin.getTowns() -> the names of the towns of admin.toml, in its order.
static sol::as_table_t<std::vector<std::string>> AdminGetTowns() {
	std::vector<std::string> Names;
	for (const TAdminTown &Town : Towns) {
		Names.push_back(Town.Name);
	}
	return sol::as_table(std::move(Names));
}

// A connection of the query manager pool for the admin port's requests;
// throws when there is none.
struct TAdminQuery {
	TQueryManagerPoolConnection Connection{&QueryManagerConnectionPool};
	TQueryManagerConnection *operator->() {
		if (Connection.operator->() == nullptr) {
			throw "Admin: cannot connect to the query manager";
		}
		return Connection.operator->();
	}
};

// Admin.banishAccount(name, days, reason) -> code, until: banishAccountFor of
// the query manager for the account of the character named (days 0: for
// good). code 0 done (until the end, 4294967295 for good), 1 no such
// character (or one without an account), 2 a gamemaster's account, -1 the
// request failed.
static std::tuple<int, uint32_t> AdminBanishAccount(std::string Name, int Days, std::string Reason) {
	RequireRequest();
	if (Days < 0) {
		throw "Admin.banishAccount: days must not be negative";
	}
	TAdminQuery Query;
	uint32_t BanishmentID = 0;
	uint32_t Until = 0;
	int Result = Query->banishAccountFor(0, Name.c_str(), Reason.c_str(), "admin port", static_cast<uint32_t>(Days),
			BanishmentID, Until);
	return std::make_tuple(Result, Until);
}

// Admin.liftBanishment(name) -> code: liftBanishment of the query manager. 0
// done, 1 no such character (or one without an account), 2 not banished, -1
// the request failed.
static int AdminLiftBanishment(std::string Name) {
	RequireRequest();
	TAdminQuery Query;
	return Query->liftBanishment(Name.c_str());
}

// Admin.banishIPAddress(address, days, reason) -> code, until:
// banishIPAddressFor of the query manager (days 0: for good). 0 done, 1 no
// IPv4 address, -1 the request failed.
static std::tuple<int, uint32_t> AdminBanishIPAddress(std::string Address, int Days, std::string Reason) {
	RequireRequest();
	if (Days < 0) {
		throw "Admin.banishIPAddress: days must not be negative";
	}
	TAdminQuery Query;
	uint32_t Until = 0;
	int Result = Query->banishIPAddressFor(0, Address.c_str(), Reason.c_str(), "admin port",
			static_cast<uint32_t>(Days), Until);
	return std::make_tuple(Result, Until);
}

// Admin.namelock(name, reason) -> code: setNamelock of the query manager. 0
// done, 1 no such character, 3 namelocked already, -1 the request failed.
static int AdminNamelock(std::string Name, std::string Reason) {
	RequireRequest();
	TAdminQuery Query;
	std::string Buffer = Copy(Name);
	std::string ReasonBuffer = Copy(Reason);
	char Empty[1] = "";
	char Comment[] = "admin port";
	return Query->setNamelock(0, &Buffer[0], Empty, &ReasonBuffer[0], Comment);
}

// Admin.addNotation(name, text) -> code: setNotation of the query manager,
// text the comment. 0 done, 1 no such character, 2 a gamemaster's, -1 the
// request failed.
static int AdminAddNotation(std::string Name, std::string Text) {
	RequireRequest();
	TAdminQuery Query;
	std::string Buffer = Copy(Name);
	std::string Comment = Copy(Text);
	char Empty[1] = "";
	char Reason[] = "admin port";
	uint32_t NotationID = 0;
	return Query->setNotation(0, &Buffer[0], Empty, Reason, &Comment[0], NotationID);
}

// Admin.renameCharacter(name, newName) -> code: renameCharacter of the query
// manager for the character the player index has under name (any case),
// then the game's own records of the name (the player index, a player data
// slot, the houses' owner names). Only for a character that is not in the
// game (a Lua error otherwise). 0 done, 1 no such character, 2 the name is
// taken, 3 an empty or too long name, -1 the request failed.
static int AdminRenameCharacter(std::string Name, std::string NewName) {
	RequireRequest();
	if (Session.Open) {
		throw "Admin.renameCharacter: close the character first";
	}
	std::string Buffer = Copy(Name);
	if (Buffer.empty() || Buffer.size() >= 30) {
		return 1;
	}
	uint32_t CharacterID = GetCharacterID(&Buffer[0]);
	if (CharacterID == 0) {
		return 1;
	}
	std::string OldName = GetCharacterName(&Buffer[0]);
	if (GetPlayer(CharacterID) != nullptr) {
		throw "Admin.renameCharacter: the character is in the game";
	}
	TAdminQuery Query;
	int Result = Query->renameCharacter(CharacterID, NewName.c_str());
	if (Result == 0) {
		RenameCharacterInGame(CharacterID, OldName.c_str(), NewName.c_str());
	}
	return Result;
}

// Admin.save() -> players, failed: writes the data of every player online
// into its slot (SaveData, SaveInventory) and saves it with the query manager
// at once (SavePlayerData; one it cannot take is kept in a file, failed
// counts those), orders the save of the other slots (SavePlayerDataOrder,
// the writer thread) and saves the map (SaveMap: the houses' contents).
static std::tuple<int, int> AdminSave() {
	RequireRequest();
	int Saved = 0;
	int Failed = 0;
	for (int i = 0; i < GetNumberOfPlayers(); i++) {
		TPlayer *pl = PlayerList(i);
		if (pl == nullptr || pl->IsDead || pl->PlayerData == nullptr) {
			continue;
		}
		pl->SaveData();
		pl->SaveInventory();
		if (SavePlayerData(pl->PlayerData)) {
			Saved++;
		} else {
			Failed++;
		}
	}
	SavePlayerDataOrder();
	SaveMap();
	return std::make_tuple(Saved, Failed);
}

// Admin.scheduleShutdown(minutes, restart): ScheduleShutdown of main.cc
// (minutes 0 to 1440).
static void AdminScheduleShutdown(int Minutes, bool Restart) {
	RequireRequest();
	if (Minutes < 0 || Minutes > 1440) {
		throw "Admin.scheduleShutdown: minutes must be 0 to 1440";
	}
	ScheduleShutdown(Minutes, Restart);
}

// Admin.cancelShutdown() -> whether one was scheduled.
static bool AdminCancelShutdown() {
	RequireRequest();
	return CancelShutdown();
}

// Admin.getShutdown() -> the seconds until the scheduled shutdown and whether
// it restarts, or nil.
static std::tuple<sol::optional<int>, bool> AdminGetShutdown() {
	int Left = ShutdownSecondsLeft();
	if (Left < 0) {
		return std::make_tuple(sol::optional<int>(), false);
	}
	return std::make_tuple(sol::optional<int>(Left), ShutdownRestarts());
}

void RegisterAdminAPI() {
	sol::table Admin = ScriptAPITable("Admin");
	Admin.set_function("reply", Guarded(&AdminReply));
	Admin.set_function("fail", Guarded(&AdminFail));
	Admin.set_function("runOverCharacters", Guarded(&AdminRunOverCharacters));
	Admin.set_function("findCharacter", Guarded(&AdminFindCharacter));
	Admin.set_function("openCharacter", Guarded(&AdminOpenCharacter));
	Admin.set_function("closeCharacter", Guarded(&AdminCloseCharacter));
	Admin.set_function("getPlayer", Guarded(&AdminGetPlayer));
	Admin.set_function("getName", Guarded(&AdminGetName));
	Admin.set_function("getInventoryContainer", Guarded(&AdminGetInventoryContainer));
	Admin.set_function("getDepotContainer", Guarded(&AdminGetDepotContainer));
	Admin.set_function("addToDepot", Guarded(&AdminAddToDepot));
	Admin.set_function("create", Guarded(&AdminCreate));
	Admin.set_function("getCarrier", Guarded(&AdminGetCarrier));
	Admin.set_function("getStartPosition", Guarded(&AdminGetStartPosition));
	Admin.set_function("getPosition", Guarded(&AdminGetPosition));
	Admin.set_function("setSavedPosition", Guarded(&AdminSetSavedPosition));
	Admin.set_function("getDepotTown", Guarded(&AdminGetDepotTown));
	Admin.set_function("retry", Guarded(&AdminRetry));
	Admin.set_function("getTries", Guarded(&AdminGetTries));
	Admin.set_function("broadcast", Guarded(&AdminBroadcast));
	Admin.set_function("getIPAddress", Guarded(&AdminGetIPAddress));
	Admin.set_function("heal", Guarded(&AdminHeal));
	Admin.set_function("setMuting", Guarded(&AdminSetMuting));
	Admin.set_function("setStartPosition", Guarded(&AdminSetStartPosition));
	Admin.set_function("getTownTemple", Guarded(&AdminGetTownTemple));
	Admin.set_function("getTowns", Guarded(&AdminGetTowns));
	Admin.set_function("banishAccount", Guarded(&AdminBanishAccount));
	Admin.set_function("liftBanishment", Guarded(&AdminLiftBanishment));
	Admin.set_function("banishIPAddress", Guarded(&AdminBanishIPAddress));
	Admin.set_function("namelock", Guarded(&AdminNamelock));
	Admin.set_function("addNotation", Guarded(&AdminAddNotation));
	Admin.set_function("renameCharacter", Guarded(&AdminRenameCharacter));
	Admin.set_function("save", Guarded(&AdminSave));
	Admin.set_function("scheduleShutdown", Guarded(&AdminScheduleShutdown));
	Admin.set_function("cancelShutdown", Guarded(&AdminCancelShutdown));
	Admin.set_function("getShutdown", Guarded(&AdminGetShutdown));
}

// ---------------------------------------------------------------------------
// The admin part of a reload

// read parses admin.toml into the staged list; install makes it the list in
// use; loadScripts loads its scripts (each needs onCommand) into the current
// engine; finish makes their numbers CommandScripts (a run going on is
// stopped: its script number is the old engine's); uninstall gives back the
// list in use before; discard forgets what is staged.
class TAdminReloadPart : public TReloadPart {
public:
	const char *name() const override {
		return "admin";
	}
	void read() override;
	void install() override {
		Commands.swap(Staged);
		Towns.swap(StagedTowns);
	}
	void loadScripts() override;
	void finish() override;
	void uninstall() override {
		Commands.swap(Staged);
		Towns.swap(StagedTowns);
	}
	void discard() override {
		Staged.clear();
		StagedScripts.clear();
		StagedTowns.clear();
	}
	std::string summary() const override {
		return Summary;
	}

	// Forgets every list and the file (ExitAdminCommands).
	void forget() {
		Commands.clear();
		CommandScripts.clear();
		Towns.clear();
		discard();
		FileName.clear();
	}

	// The file read reads (InitAdminCommands sets it).
	std::string FileName;

private:
	std::vector<TAdminCommand> Staged;
	std::vector<int> StagedScripts;
	std::vector<TAdminTown> StagedTowns;
	std::string Error;
	std::string Summary;
};

void TAdminReloadPart::read() {
	toml::table File = TomlParseFile(FileName.c_str());
	TomlCheckKeys(File, "the top level", {"command", "town"});

	std::vector<TAdminCommand> Read;
	if (const toml::array *List = TomlGetArray(File, "command")) {
		for (const toml::node &Node : *List) {
			TAdminCommand Command = ReadCommand(Node);
			for (const TAdminCommand &Earlier : Read) {
				if (Earlier.Name == Command.Name) {
					Error = Command.Where + ": command \"" + Command.Name + "\" is listed twice (first at "
							+ Earlier.Where + ")";
					throw Error.c_str();
				}
			}
			Read.push_back(Command);
		}
	}
	std::vector<TAdminTown> ReadTowns;
	if (const toml::array *List = TomlGetArray(File, "town")) {
		for (const toml::node &Node : *List) {
			TAdminTown Town = ReadTown(Node);
			for (const TAdminTown &Earlier : ReadTowns) {
				if (strcasecmp(Earlier.Name.c_str(), Town.Name.c_str()) == 0) {
					Error = PlaceText(Node.source()) + ": town \"" + Town.Name + "\" is listed twice";
					throw Error.c_str();
				}
			}
			ReadTowns.push_back(Town);
		}
	}
	Summary = "admin.toml: " + std::to_string(Read.size()) + " commands, " + std::to_string(ReadTowns.size())
			+ " towns";
	Staged.swap(Read);
	StagedTowns.swap(ReadTowns);
}

void TAdminReloadPart::loadScripts() {
	StagedScripts.clear();
	for (const TAdminCommand &Command : Commands) {
		int Script = LoadScript(Command.Script.c_str(), Command.Where.c_str());
		RequireHandler(Script, "onCommand", Command.Where.c_str());
		StagedScripts.push_back(Script);
	}
}

void TAdminReloadPart::finish() {
	StopRun("the scripts were reloaded.");
	CommandScripts.swap(StagedScripts);
	StagedScripts.clear();
	Staged.clear();
	StagedTowns.clear();
}

static TAdminReloadPart AdminPart;

TReloadPart &AdminReloadPart() {
	return AdminPart;
}

void InitAdminCommands(const char *FileName) {
	AdminPart.FileName = FileName;
	bool Installed = false;
	try {
		AdminPart.read();
		AdminPart.install();
		Installed = true;
		AdminPart.loadScripts();
	} catch (const char *) {
		if (Installed) {
			AdminPart.uninstall();
		}
		AdminPart.discard();
		throw;
	}
	AdminPart.finish();
}

void ExitAdminCommands() {
	CloseSession(false);
	StopRun("the game server is shutting down.");
	AdminPart.forget();
}
