// reload.cc: the driver of the reload (reload.hh). RequestScriptReload keeps
// the request of the command Reload Scripts; ProcessScriptReload, called by
// AdvanceGame after ProcessCommand, does the steps of reload.hh with the parts
// in their order, writes one summary to the game log ("game") and tells the
// gamemaster who asked the result.

#include <chrono>
#include <cstdint>
#include <cstdio>
#include <exception>
#include <string>
#include <vector>

#include "objects.hh"
#include "map.hh"
#include "info.hh"
#include "cr.hh"
#include "crmain.hh"
#include "crplayer.hh"
#include "operate.hh"
#include "sending.hh"
#include "writer.hh"
#include "scripting_api.hh"
#include "reload.hh"
#include "admin.hh"

// The longest text a message to a player holds (Talk takes no longer one).
static const size_t MAX_MESSAGE_LENGTH = 255;

// The longest piece of text the summary writes on one line of the log (a
// line of the log holds 256 characters with its date); a longer text goes on
// over the next lines.
static const size_t MAX_LOG_PIECE = 200;

// Whether a reload is asked for, by whom (the player's creature ID and name
// when it asked).
static bool Requested = false;
static uint32_t RequesterID = 0;
static std::string RequesterName;

bool RequestScriptReload(uint32_t PlayerID) {
	if (Requested) {
		return false;
	}
	Requested = true;
	RequesterID = PlayerID;
	TPlayer *pl = GetPlayer(PlayerID);
	if (pl != nullptr) {
		RequesterName = pl->Name;
	} else {
		RequesterName = "player " + std::to_string(PlayerID);
	}
	return true;
}

// Writes Text to the game log, Indent in front, in pieces of at most
// MAX_LOG_PIECE characters, each further piece on a line of its own indented
// by two more spaces.
static void LogText(const std::string &Indent, const std::string &Text) {
	size_t Start = 0;
	std::string Prefix = Indent;
	do {
		std::string Piece = Text.substr(Start, MAX_LOG_PIECE);
		Log("game", "%s%s\n", Prefix.c_str(), Piece.c_str());
		Start += MAX_LOG_PIECE;
		Prefix = Indent + "  ";
	} while (Start < Text.size());
}

// Text cut to what a message holds.
static std::string MessageText(const std::string &Text) {
	return Text.size() <= MAX_MESSAGE_LENGTH ? Text : Text.substr(0, MAX_MESSAGE_LENGTH);
}

void ProcessScriptReload() {
	if (!Requested) {
		return;
	}
	// Requested stays set until the end, so a request made while the reload
	// runs (by a script's top level) is refused, as a second request is.
	std::chrono::steady_clock::time_point Start = std::chrono::steady_clock::now();

	// The parts in the order of the start (reload.hh).
	TReloadPart *const Parts[] = {
		&ItemsReloadPart(),
		&ItemUsesReloadPart(),
		&EventsReloadPart(),
		&TriggersReloadPart(),
		&MagicReloadPart(),
		&RacesReloadPart(),
		&RaidsReloadPart(),
		&NpcScriptsReloadPart(),
		&AdminReloadPart(),
	};
	const int NUMBER_OF_PARTS = static_cast<int>(sizeof(Parts) / sizeof(Parts[0]));

	// How many parts were read (their read called, also one that threw) and
	// installed, whether the new engine was begun, the summaries of the parts
	// read so far, and the failure: "<part>: <text>", the text copied at once
	// (it may live in a buffer the next call reuses).
	int NumberRead = 0;
	int NumberInstalled = 0;
	bool Began = false;
	std::vector<std::string> Summaries;
	std::string Failure;
	const char *Doing = "";
	try {
		for (int i = 0; i < NUMBER_OF_PARTS; i++) {
			Doing = Parts[i]->name();
			NumberRead = i + 1;
			Parts[i]->read();
			Parts[i]->install();
			NumberInstalled = i + 1;
		}
		Doing = "engine";
		BeginScriptReload();
		Began = true;
		RegisterMagicAPI();
		RegisterTriggerAPI();
		RegisterItemAPI();
		RegisterNpcAPI();
		RegisterMonsterAPI();
		RegisterEventAPI();
		RegisterAdminAPI();
		for (int i = 0; i < NUMBER_OF_PARTS; i++) {
			Doing = Parts[i]->name();
			Parts[i]->loadScripts();
		}
		// Each part's summary, with its new data installed and its scripts
		// loaded.
		for (int i = 0; i < NUMBER_OF_PARTS; i++) {
			Summaries.push_back(Parts[i]->summary());
		}
	} catch (const char *Text) {
		Failure = std::string(Doing) + ": " + (Text != nullptr ? Text : "(no text)");
	} catch (const std::exception &Exception) {
		Failure = std::string(Doing) + ": " + Exception.what();
	} catch (...) {
		Failure = std::string(Doing) + ": an exception that is no text";
	}

	if (Failure.empty()) {
		for (int i = 0; i < NUMBER_OF_PARTS; i++) {
			Parts[i]->finish();
		}
		CommitScriptReload();
		RefreshCreaturesAfterReload();
	} else {
		for (int i = NumberInstalled - 1; i >= 0; i--) {
			Parts[i]->uninstall();
		}
		for (int i = NumberRead - 1; i >= 0; i--) {
			Parts[i]->discard();
		}
		if (Began) {
			DiscardScriptReload();
		}
	}

	long Milliseconds = static_cast<long>(std::chrono::duration_cast<std::chrono::milliseconds>(
			std::chrono::steady_clock::now() - Start).count());
	char Result[128];
	if (Failure.empty()) {
		snprintf(Result, sizeof(Result), "done in %ld ms", Milliseconds);
	} else {
		snprintf(Result, sizeof(Result), "failed in %ld ms, nothing changed", Milliseconds);
	}
	LogText("", "Reload of the scripts and data by " + RequesterName + ": " + Result + ".");
	if (!Failure.empty()) {
		size_t LineStart = 0;
		bool First = true;
		while (LineStart <= Failure.size()) {
			size_t LineEnd = Failure.find('\n', LineStart);
			if (LineEnd == std::string::npos) {
				LineEnd = Failure.size();
			}
			std::string Line = Failure.substr(LineStart, LineEnd - LineStart);
			LogText(First ? "  " : "    ", First ? "error: " + Line : Line);
			First = false;
			LineStart = LineEnd + 1;
		}
	}
	for (const std::string &Summary : Summaries) {
		LogText("  ", Summary);
	}

	TPlayer *pl = GetPlayer(RequesterID);
	if (pl != nullptr) {
		std::string Line = Failure.empty() ? "Scripts and data reloaded in " + std::to_string(Milliseconds) + " ms."
				: std::string("Reload failed, nothing changed:");
		SendMessage(pl->GetConnection(), TALK_INFO_MESSAGE, "%s", MessageText(Line).c_str());
		if (!Failure.empty()) {
			SendMessage(pl->GetConnection(), TALK_INFO_MESSAGE, "%s",
					MessageText(Failure.substr(0, Failure.find('\n'))).c_str());
		}
	}
	Requested = false;
}
