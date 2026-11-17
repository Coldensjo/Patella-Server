// test_commands.cc: the round trip of the commands, the spells of the classes
// al and om, and their rights. The spell table and the rights of the commands
// the game ends up with from the table it had in its code
// (InitReferenceSpells) and the rights its code checked
// (InitReferenceCommandRights), and from the spells.toml and commands.toml
// patella-convert writes of them, through the game's LoadSpells and
// LoadCommands, must be the same, field by field; LoadCommands must refuse
// what is wrong with the file and line.
//
//   convert_test_commands
//     With made-up tables and files in a temporary folder.
//   convert_test_commands --original SPELLS_TOML COMMANDS_TOML
//     With the committed files: commands.toml must be exactly what
//     patella-convert commands writes, and the two must give the same table
//     and rights as the code did; every change of one value of commands.toml
//     must give another table or other rights, or an error; and every script
//     commands.toml names must be a file in the scripts folder next to it
//     with the handler onSay.
//
// Every load runs in a child process of its own, on a table and rights filled
// with a byte pattern first. The whole table is compared, spells 0 to 255, as
// test_spells.cc compares it (with the scripts), and every right check of
// every number.

#include <climits>
#include <cstdio>
#include <cstring>
#include <regex>
#include <string>
#include <vector>

#include "magicdata.hh"
#include "commands_reference.hh"
#include "spells_reference.hh"
#include "spells_writer.hh"
#include "tomlfile.hh"

#include "test_common.hh"

static const unsigned char POISON = 0xA5;

static std::string Printable(const char *Text) {
	std::string Out;
	for (const unsigned char *c = reinterpret_cast<const unsigned char *>(Text); *c != 0; c++) {
		if (*c >= 0x20 && *c < 0x7F && *c != '\\') {
			Out += static_cast<char>(*c);
		} else {
			char Code[8];
			snprintf(Code, sizeof(Code), "\\x%02X", *c);
			Out += Code;
		}
	}
	return Out;
}

// The script of spell or command SpellNr: of the loaded files (Loaded), else
// the one spells.toml or commands.toml is written with; "(none)" for none.
static std::string Script(int SpellNr, bool Loaded) {
	if (Loaded) {
		const char *Name = GetSpellScript(SpellNr);
		return Name == nullptr ? "(none)" : "\"" + std::string(Name) + "\"";
	}
	if (SpellNr == 0 || SpellList[SpellNr].Syllable[0] == 0) {
		return "(none)";
	}
	return "\"" + SpellScript(SpellNr) + "\"";
}

// The table and the rights as "field\tvalue" lines.
static std::string Dump(bool Loaded) {
	std::string Out;
	const char *Poisoned;
	memset(static_cast<void *>(&Poisoned), POISON, sizeof(Poisoned));
	for (int SpellNr = 0; SpellNr < MAX_SPELLS; SpellNr++) {
		const TSpellList &Spell = SpellList[SpellNr];
		std::string Prefix = "spell " + std::to_string(SpellNr) + " ";
		for (int i = 0; i < MAX_SPELL_SYLLABLES; i++) {
			Out += Prefix + "syllable " + std::to_string(i) + "\t" + std::to_string(Spell.Syllable[i]) + "\n";
		}
		Out += Prefix + "rune group\t" + std::to_string(Spell.RuneGr) + "\n";
		Out += Prefix + "rune number\t" + std::to_string(Spell.RuneNr) + "\n";
		Out += Prefix + "name\t" + (Spell.Comment == nullptr ? "(null)" : Spell.Comment == Poisoned ? "(pattern)"
				: "\"" + Printable(Spell.Comment) + "\"") + "\n";
		Out += Prefix + "level\t" + std::to_string(Spell.Level) + "\n";
		Out += Prefix + "rune level\t" + std::to_string(Spell.RuneLevel) + "\n";
		Out += Prefix + "flags\t" + std::to_string(Spell.Flags) + "\n";
		Out += Prefix + "mana\t" + std::to_string(Spell.Mana) + "\n";
		Out += Prefix + "soul points\t" + std::to_string(Spell.SoulPoints) + "\n";
		Out += Prefix + "charges\t" + std::to_string(Spell.Amount) + "\n";
		Out += Prefix + "script\t" + Script(SpellNr, Loaded) + "\n";
		for (int i = 0; i < MAX_COMMAND_RIGHTS; i++) {
			Out += Prefix + "right " + std::to_string(i) + "\t" + std::to_string(CommandRight[SpellNr][i]) + "\n";
		}
	}
	return Out;
}

// The table and rights Fill makes, which loads files (Loaded) or fills them
// in the code.
static std::string Snapshot(const std::function<void()> &Fill, bool Loaded = true) {
	return InChild([&]() {
		memset(static_cast<void *>(SpellList), POISON, sizeof(SpellList));
		memset(static_cast<void *>(CommandRight), POISON, sizeof(CommandRight));
		Fill();
		return Dump(Loaded);
	});
}

static std::function<void()> NewLoader(const fs::path &Spells, const fs::path &Commands) {
	return [Spells, Commands]() {
		LoadSpells(Spells.c_str());
		LoadCommands(Commands.c_str());
	};
}

// The table and rights as the game starts: all empty.
static void Clear() {
	for (int i = 1; i < MAX_SPELLS; i++) {
		memset(&SpellList[i], 0, sizeof(TSpellList));
	}
	for (int i = 0; i < MAX_SPELLS; i++) {
		for (int j = 0; j < MAX_COMMAND_RIGHTS; j++) {
			CommandRight[i][j] = NO_RIGHT;
		}
	}
}

// The commands.toml patella-convert writes of the table and rights Fill
// makes (on an empty table and rights).
static std::string Convert(const std::function<void()> &Fill) {
	return InChild([&]() {
		Clear();
		Fill();
		return CommandsToToml();
	});
}

static std::string ConvertSpells(const std::function<void()> &Fill) {
	return InChild([&]() {
		Clear();
		Fill();
		return SpellsToToml();
	});
}

// Writes the table and rights Fill makes as spells.toml and commands.toml,
// loads them and compares. Returns the commands.toml text.
static std::string RoundTrip(const fs::path &Folder, const char *Name, const std::function<void()> &Fill) {
	std::string Spells = ConvertSpells(Fill);
	std::string Text = Convert(Fill);
	if (IsError(Spells) || IsError(Text)) {
		fprintf(stderr, "%s: conversion: %s%s\n", Name, Spells.substr(0, 200).c_str(), Text.c_str());
		Failures++;
		return Text;
	}
	fs::path SpellsFile = Folder / (std::string(Name) + "-spells.toml");
	fs::path CommandsFile = Folder / (std::string(Name) + "-commands.toml");
	WriteFile(SpellsFile, Spells);
	WriteFile(CommandsFile, Text);
	std::string Expected = Snapshot([&]() {
		Clear();
		Fill();
	}, false);
	std::string Actual = Snapshot(NewLoader(SpellsFile, CommandsFile));
	if (IsError(Expected) || IsError(Actual)) {
		fprintf(stderr, "%s: a load failed:\n%s\n%s\n", Name, Expected.substr(0, 300).c_str(),
				Actual.substr(0, 300).c_str());
		Failures++;
		return Text;
	}
	if (!Same(Expected, Actual, Name, "the table", "the files")) {
		fprintf(stderr, "%s: commands.toml was:\n%s", Name, Text.c_str());
	}
	return Text;
}

static int Syllable(const char *Text) {
	for (int i = 1; i < MAX_SYLLABLES; i++) {
		if (strcmp(SpellSyllable[i], Text) == 0) {
			return i;
		}
	}
	fprintf(stderr, "no syllable %s\n", Text);
	exit(1);
}

static void SetWords(TSpellList &Spell, std::initializer_list<const char *> Words) {
	int i = 0;
	for (const char *Word : Words) {
		Spell.Syllable[i++] = static_cast<unsigned char>(Syllable(Word));
	}
}

static void TestRoundTrips(const fs::path &Folder) {
	// A spell next to the commands; a command of each class that checks one
	// right, Move with its four (its script before them), two commands that
	// check none, the most syllables, the first and the last right, a name
	// with quotes, a backslash and Latin-1; the scripts after the numbers
	// whatever the commands there are.
	std::string Toml = RoundTrip(Folder, "made-up", []() {
		TSpellList *S = &SpellList[1];
		SetWords(*S, {"ex", "ura"});
		S->Comment = "Light Healing";
		S->Mana = 25;
		S = &SpellList[SPELL_GET_ITEM];
		SetWords(*S, {"al", "evo", "para"});
		S->Comment = "Get Item";
		CommandRight[SPELL_GET_ITEM][COMMAND_RIGHT] = 0;
		S = &SpellList[SPELL_KICK_PLAYER];
		SetWords(*S, {"om", "ana", "para"});
		S->Comment = "a \"quoted\" back\\slash \xe4\xf6\xfc\xdf";
		CommandRight[SPELL_KICK_PLAYER][COMMAND_RIGHT] = NUMBER_OF_RIGHTS - 1;
		S = &SpellList[SPELL_MOVE];
		SetWords(*S, {"al", "ani", "para"});
		S->Comment = "Move";
		CommandRight[SPELL_MOVE][MOVE_RIGHT_VERTICAL] = 52;
		CommandRight[SPELL_MOVE][MOVE_RIGHT_SPEED] = 56;
		CommandRight[SPELL_MOVE][MOVE_RIGHT_COORDINATE] = 53;
		CommandRight[SPELL_MOVE][MOVE_RIGHT_MARK] = 51;
		S = &SpellList[SPELL_INVITE_GUESTS];
		SetWords(*S, {"al", "eta", "sio"});
		S->Comment = "Invite Guests";
		S = &SpellList[SPELL_EDIT_DOOR];
		SetWords(*S, {"om", "moe", "ani", "ina", "eta", "amo", "hora", "cogni", "res", "mas"});
		S->Comment = "123456789012345678901234567890";
	});
	CHECK(Toml.find("\n[[command]]\nnumber = 35\nname = \"Get Item\"\nwords = \"alevo para\"\n"
			"right = \"PREMIUM_ACCOUNT\"\nscript = \"commands/get_item.lua\"\n\n") != std::string::npos);
	CHECK(Toml.find("words = \"omana para\"\nright = \"RELOAD_SCRIPTS\"\nscript = \"commands/kick_player.lua\"\n")
			!= std::string::npos);
	CHECK(Toml.find("name = \"a \\\"quoted\\\" back\\\\slash \xc3\xa4\xc3\xb6\xc3\xbc\xc3\x9f\"\n")
			!= std::string::npos);
	CHECK(Toml.find("number = 37\nname = \"Move\"\nwords = \"alani para\"\nscript = \"commands/move.lua\"\n"
			"[command.right]\nvertical = \"TELEPORT_VERTICAL\"\nspeed = \"MODIFY_GOSTRENGTH\"\n"
			"coordinate = \"TELEPORT_TO_COORDINATE\"\nmark = \"TELEPORT_TO_MARK\"\n") != std::string::npos);
	CHECK(Toml.find("number = 71\nname = \"Invite Guests\"\nwords = \"aleta sio\"\n"
			"script = \"commands/invite_guests.lua\"\n\n") != std::string::npos);
	CHECK(Toml.find("number = 74\nname = \"123456789012345678901234567890\"\n"
			"words = \"ommoe ani ina eta amo hora cogni res mas\"\nscript = \"commands/edit_door.lua\"\n")
			!= std::string::npos);
	CHECK(Toml.find("number = 1\n") == std::string::npos);
	CHECK(Toml.find("number = 37\n") < Toml.find("number = 71\n"));
	CHECK(Toml.find("number = 71\n") < Toml.find("number = 74\n"));

	Toml = RoundTrip(Folder, "empty", []() {});
	CHECK(Toml.find("\n[[command]]\n") == std::string::npos);

	// The reference table with the rights the code checked, and the commands
	// that are ours after the original's.
	Toml = RoundTrip(Folder, "reference", []() {
		InitReferenceSpells();
		InitReferenceCommandRights();
	});
	CHECK(Toml.find("number = 102\nname = \"Start Monsterraid\"\n") < Toml.find("number = 103\n"));
	CHECK(Toml.find("\n[[command]]\nnumber = 103\nname = \"Reload Scripts\"\nwords = \"omevo gran cogni\"\n"
			"right = \"RELOAD_SCRIPTS\"\nscript = \"commands/reload_scripts.lua\"\n") != std::string::npos);
	CHECK(Toml.find("# Command 103 (Reload Scripts) reloads the scripts and the data while the game\n")
			!= std::string::npos);

	// Number 103 with words of another class is no command of ours: it has no
	// script name.
	std::string Other = Convert([]() {
		SetWords(SpellList[SPELL_RELOAD_SCRIPTS], {"al", "evo", "gran", "cogni"});
		SpellList[SPELL_RELOAD_SCRIPTS].Comment = "Reload Scripts";
		CommandRight[SPELL_RELOAD_SCRIPTS][COMMAND_RIGHT] = NUMBER_OF_RIGHTS - 1;
	});
	CHECK(Other.find("error: a spell number without a script name") == 0);
}

// What a conversion refuses.
static void TestRefusals() {
	auto Refused = [](const char *Name, const std::function<void()> &Change, const char *What) {
		std::string Out = Convert([&]() {
			SetWords(SpellList[SPELL_GET_ITEM], {"al", "evo", "para"});
			SpellList[SPELL_GET_ITEM].Comment = "Get Item";
			CommandRight[SPELL_GET_ITEM][COMMAND_RIGHT] = 76;
			Change();
		});
		if (Out.find(std::string("error: ") + What) != 0) {
			fprintf(stderr, "%s: expected \"error: %s...\", got \"%s\"\n", Name, What, Out.substr(0, 200).c_str());
			Failures++;
		}
	};
	Refused("no right", []() { CommandRight[SPELL_GET_ITEM][COMMAND_RIGHT] = NO_RIGHT; },
			"a command without a right for one of its checks");
	Refused("Move without one right", []() {
		SetWords(SpellList[SPELL_MOVE], {"al", "ani", "para"});
		SpellList[SPELL_MOVE].Comment = "Move";
		CommandRight[SPELL_MOVE][MOVE_RIGHT_VERTICAL] = 52;
		CommandRight[SPELL_MOVE][MOVE_RIGHT_SPEED] = 56;
		CommandRight[SPELL_MOVE][MOVE_RIGHT_COORDINATE] = 53;
	}, "a command without a right for one of its checks");
	Refused("a second right", []() { CommandRight[SPELL_GET_ITEM][1] = 76; },
			"a right for a check that no command has");
	Refused("right of a command that checks none", []() {
		SetWords(SpellList[SPELL_INVITE_GUESTS], {"al", "eta", "sio"});
		SpellList[SPELL_INVITE_GUESTS].Comment = "Invite Guests";
		CommandRight[SPELL_INVITE_GUESTS][COMMAND_RIGHT] = 76;
	}, "a right for a check that no command has");
	Refused("right without a command", []() { CommandRight[SPELL_GET_ITEM_COUNT][COMMAND_RIGHT] = 76; },
			"a right for a check that no command has");
	Refused("right of a spell", []() {
		SetWords(SpellList[SPELL_GET_ITEM_COUNT], {"ex", "evo", "para"});
		SpellList[SPELL_GET_ITEM_COUNT].Comment = "Get Items";
		CommandRight[SPELL_GET_ITEM_COUNT][COMMAND_RIGHT] = 76;
	}, "a right for a check that no command has");
	Refused("right of spell 0", []() { CommandRight[0][0] = 76; }, "a right for a check that no command has");
	Refused("right past the names", []() { CommandRight[SPELL_GET_ITEM][COMMAND_RIGHT] = NUMBER_OF_RIGHTS; },
			"a right without a name");
	Refused("negative right", []() { CommandRight[SPELL_GET_ITEM][COMMAND_RIGHT] = -2; }, "a right without a name");
	Refused("command with mana", []() { SpellList[SPELL_GET_ITEM].Mana = 1; }, "a command (al or om) with a level");
	Refused("broken spell", []() {
		SetWords(SpellList[1], {"ex", "ura"});
		SpellList[1].Comment = nullptr;
	}, "a spell without a name");
	Refused("same words as a spell", []() {
		SetWords(SpellList[1], {"al", "evo", "para"});
		SpellList[1].Comment = "Twice";
	}, "two spells with the same words");
	for (int SpellNr : {36, 200, 255}) {
		for (const char *Class : {"al", "om"}) {
			Refused("number without a script name", [SpellNr, Class]() {
				SetWords(SpellList[SpellNr], {Class, "ura"});
				SpellList[SpellNr].Comment = "x";
			}, "a spell number without a script name");
		}
	}
}

static void TestErrors(const fs::path &Folder) {
	fs::path SpellsFile = Folder / "errors" / "spells.toml";
	fs::path File = Folder / "errors" / "commands.toml";
	WriteFile(SpellsFile, "[[spell]]\nnumber = 10\nname = \"Light\"\nwords = \"utevo lux\"\n");
	auto Load = [&](const std::string &Text) {
		WriteFile(File, Text);
		return InChild([&]() {
			LoadSpells(SpellsFile.c_str());
			LoadCommands(File.c_str());
			return std::string();
		});
	};
	auto Expect = [&](const std::string &Text, int Line, const char *What) {
		ExpectError(Load, File, Text, Line, What);
	};
	// Command 71 (Invite Guests) checks no right, 35 (Get Item) one, 37 (Move)
	// four.
	const std::string None = "[[command]]\nnumber = 71\nname = \"Invite Guests\"\nwords = \"aleta sio\"\n";
	const std::string One = "[[command]]\nnumber = 35\nname = \"Get Item\"\nwords = \"alevo para\"\n";
	const std::string Move = "[[command]]\nnumber = 37\nname = \"Move\"\nwords = \"alani para\"\n";
	const std::string MoveRights = "[command.right]\nvertical = \"TELEPORT_VERTICAL\"\nspeed = \"MODIFY_GOSTRENGTH\"\n"
			"coordinate = \"TELEPORT_TO_COORDINATE\"\nmark = \"TELEPORT_TO_MARK\"\n";
	Expect("commands = 1\n", 1, "unknown key \"commands\" in the top level");
	Expect("spell = []\n", 1, "unknown key \"spell\" in the top level");
	Expect("command = 1\n", 1, "command must be an array");
	Expect("command = [1]\n", 1, "an entry of command must be a table");
	Expect(None + "nmae = \"x\"\n", 5, "unknown key \"nmae\" in [[command]]");
	Expect(None + "level = 0\n", 5, "unknown key \"level\" in [[command]]");
	Expect(None + "mana = 0\n", 5, "unknown key \"mana\" in [[command]]");
	Expect("[[command]]\nname = \"x\"\nwords = \"aleta sio\"\n", 1, "an entry of command needs number");
	Expect("[[command]]\nnumber = 71\nwords = \"aleta sio\"\n", 1, "an entry of command needs name");
	Expect("[[command]]\nnumber = 71\nname = \"x\"\n", 1, "an entry of command needs words");
	Expect("[[command]]\nnumber = \"71\"\nname = \"x\"\nwords = \"aleta sio\"\n", 2, "number must be an integer");
	Expect("[[command]]\nnumber = 0\nname = \"x\"\nwords = \"aleta sio\"\n", 2, "number must lie between 1 and 255");
	Expect("[[command]]\nnumber = 256\nname = \"x\"\nwords = \"aleta sio\"\n", 2,
			"number must lie between 1 and 255");
	Expect(None + "[[command]]\nnumber = 71\nname = \"x\"\nwords = \"aleta som\"\n", 6, "command 71 is defined twice");
	Expect("[[command]]\nnumber = 10\nname = \"x\"\nwords = \"aleta sio\"\n", 2,
			"10 is the number of a spell in spells.toml");
	Expect("[[command]]\nnumber = 71\nname = 5\nwords = \"aleta sio\"\n", 3, "name must be a string");
	Expect("[[command]]\nnumber = 71\nname = \"1234567890123456789012345678901\"\nwords = \"aleta sio\"\n", 3,
			"name is longer than 30 characters");
	Expect("[[command]]\nnumber = 71\nname = \"\xc4\x80\"\nwords = \"aleta sio\"\n", 3,
			"name has a character outside Latin-1");
	auto Words = [](const std::string &Text) {
		return "[[command]]\nnumber = 71\nname = \"x\"\nwords = " + Text + "\n";
	};
	const char *Class = "words must start with al or om (ad, ex and ut start spells, in spells.toml)";
	Expect(Words("5"), 4, "words must be a string");
	Expect(Words("\"\""), 4, "words must be syllables separated by single spaces");
	Expect(Words("\"aleta  sio\""), 4, "words must be syllables separated by single spaces");
	Expect(Words("\"exura\""), 4, Class);
	Expect(Words("\"utevo lux\""), 4, Class);
	Expect(Words("\"adori\""), 4, Class);
	Expect(Words("\"eta\""), 4, Class);
	Expect(Words("\"Aleta sio\""), 4, Class);
	Expect(Words("\"aleta sioo\""), 4, "unknown syllable \"sioo\" in words");
	Expect(Words("\"aleta sio sio sio sio sio sio sio sio sio\""), 4, "words have more than 10 syllables");
	Expect(None + "[[command]]\nnumber = 72\nname = \"x\"\nwords = \"aleta sio\"\n", 8,
			"these words are already the ones of command 71");
	// The words of a spell with another class are not the same words.
	WriteFile(SpellsFile, "[[spell]]\nnumber = 10\nname = \"Light\"\nwords = \"utevo lux\"\n"
			"[[spell]]\nnumber = 11\nname = \"x\"\nwords = \"exura\"\n");
	CHECK(Load("[[command]]\nnumber = 71\nname = \"x\"\nwords = \"alura\"\n") == "");

	// The rights.
	Expect(None + "right = \"KICK\"\n", 5, "command 71 checks no right");
	Expect("[[command]]\nnumber = 200\nname = \"x\"\nwords = \"alura vis\"\nright = \"KICK\"\n", 5,
			"command 200 checks no right");
	Expect(One, 1, "command 35 needs right");
	Expect(One + "right = 76\n", 5, "right must be a string");
	Expect(One + "right = \"create_objects\"\n", 5, "unknown right \"create_objects\"");
	Expect(One + "right = \"CREATE_OBJECT\"\n", 5, "unknown right \"CREATE_OBJECT\"");
	Expect(One + "right = \"\"\n", 5, "unknown right \"\"");
	Expect(One + "right = [\"CREATE_OBJECTS\"]\n", 5, "right must be a string");
	Expect(One + "right = { use = \"CREATE_OBJECTS\" }\n", 5, "right must be a string");
	Expect(Move, 1, "command 37 needs right");
	Expect(Move + "right = \"TELEPORT_VERTICAL\"\n", 5, "right of command 37 must be a table of its 4 right checks");
	Expect(Move + "[command.right]\nvertical = \"TELEPORT_VERTICAL\"\n", 5, "right of command 37 needs speed");
	Expect(Move + MoveRights + "up = \"TELEPORT_VERTICAL\"\n", 10, "unknown key \"up\" in right of command 37");
	Expect(Move + "[command.right]\nvertical = 52\n", 6, "vertical must be a string");
	Expect(Move + "[command.right]\nvertical = \"TELEPORT_VERTICAL\"\nspeed = \"MODIFY_GOSTRENGTH\"\n"
			"coordinate = \"TELEPORT_TO_COORDINATE\"\nmark = \"MARK\"\n", 9, "unknown right \"MARK\"");
	Expect(One + "right =\n", 5, "");

	// The script.
	Expect(None + "script = 5\n", 5, "script must be a string");
	Expect(None + "script = \"commands/Invite.lua\"\n", 5, "script \"commands/Invite.lua\" is not a script name");
	Expect(None + "script = \"commands/../x.lua\"\n", 5, "script \"commands/../x.lua\" is not a script name");
	Expect(None + "script = \"spells/invite_guests.lua\"\n", 5, "the script of a command must be in commands/");
	Expect(None + "script = \"commandsx/invite_guests.lua\"\n", 5, "the script of a command must be in commands/");
	Expect(Move + "script = \"commands/move.lua\"\n" + MoveRights + "script = \"commands/move.lua\"\n", 11,
			"unknown key \"script\" in right of command 37");

	// What loads: no file content, a command that checks none, one that
	// checks one, Move as a table of its own and inline, a number the code
	// does not know, a script, Move's before its table of rights, every right
	// by its name.
	CHECK(Load("") == "");
	CHECK(Load(None) == "");
	CHECK(Load(One + "right = \"CREATE_OBJECTS\"\n") == "");
	CHECK(Load(Move + MoveRights) == "");
	CHECK(Load(Move + "right = { mark = \"KICK\", vertical = \"KICK\", speed = \"KICK\", coordinate = \"KICK\" }\n")
			== "");
	CHECK(Load("[[command]]\nnumber = 200\nname = \"x\"\nwords = \"omvis\"\n") == "");
	CHECK(Load(None + "script = \"commands/invite_guests.lua\"\n") == "");
	CHECK(Load(Move + "script = \"commands/move.lua\"\n" + MoveRights) == "");
	for (int Right = 0; Right < NUMBER_OF_RIGHTS; Right++) {
		std::string Text = One + "right = \"" + RightNames[Right] + "\"\n";
		std::string Out = InChild([&]() {
			WriteFile(File, Text);
			LoadSpells(SpellsFile.c_str());
			LoadCommands(File.c_str());
			return std::to_string(CommandRight[SPELL_GET_ITEM][COMMAND_RIGHT]);
		});
		CHECK(Out == std::to_string(Right));
	}

	// The same commands written two ways give the same table and rights.
	WriteFile(Folder / "inline.toml", One + "right = \"KICK\"\n" + Move
			+ "right = { vertical = \"KICK\", speed = \"RETRIEVE\", coordinate = \"LEVITATE\", mark = \"NOTATION\" }\n");
	WriteFile(Folder / "table.toml", Move + "[command.right]\nmark = \"NOTATION\"\ncoordinate = \"LEVITATE\"\n"
			"speed = \"RETRIEVE\"\nvertical = \"KICK\"\n" + One + "right = \"KICK\"\n");
	Same(Snapshot(NewLoader(SpellsFile, Folder / "inline.toml")), Snapshot(NewLoader(SpellsFile, Folder / "table.toml")),
			"two ways", "inline", "table");

	std::string Missing = InChild([&]() {
		LoadSpells(SpellsFile.c_str());
		LoadCommands((Folder / "missing.toml").c_str());
		return std::string();
	});
	CHECK(IsError(Missing) && Missing.find((Folder / "missing.toml").string()) != std::string::npos);
}

// The commands of a commands.toml text: its header (up to the first
// "[[command]]"), and each "[[command]]" block up to the empty line before the
// next one.
static std::vector<std::string> SplitCommands(const std::string &Toml, std::string &Header) {
	std::vector<std::string> Blocks;
	size_t Start = Toml.find("\n[[command]]\n");
	Start = Start == std::string::npos ? Toml.size() : Start + 1;
	Header = Toml.substr(0, Start);
	while (Start < Toml.size()) {
		size_t Next = Toml.find("\n[[command]]\n", Start);
		Blocks.push_back(Toml.substr(Start, Next == std::string::npos ? std::string::npos : Next - Start));
		Start = Next == std::string::npos ? Toml.size() : Next + 1;
	}
	return Blocks;
}

// The changes of one value of a [[command]] block: the number one up and to
// 254, the name and the words changed, the class changed, the last syllable
// left out, each right changed to the next one and left out, a right added
// to a command without one, the script changed or left out, the block left
// out.
static std::vector<std::pair<std::string, std::string>> Mutations(const std::string &Block) {
	std::vector<std::pair<std::string, std::string>> Result;
	std::smatch Match;
	std::regex Number("\nnumber = (\\d+)\n");
	std::string Search = "\n" + Block;
	if (std::regex_search(Search, Match, Number)) {
		size_t At = static_cast<size_t>(Match.position(1)) - 1;
		long Value = std::stol(Match[1].str());
		Result.push_back({"number + 1", std::string(Block).replace(At, static_cast<size_t>(Match.length(1)),
				std::to_string(Value + 1))});
		Result.push_back({"number to 254", std::string(Block).replace(At, static_cast<size_t>(Match.length(1)),
				"254")});
	}
	size_t Name = Block.find("\nname = \"");
	Result.push_back({"name", std::string(Block).insert(Name + 9, "x")});
	size_t Words = Block.find("\nwords = \"");
	size_t WordsEnd = Block.find('"', Words + 10);
	Result.push_back({"words + vis", std::string(Block).insert(WordsEnd, " vis")});
	std::string Class = Block.substr(Words + 10, 2);
	Result.push_back({"class", std::string(Block).replace(Words + 10, 2, Class == "al" ? "om" : "al")});
	size_t Last = Block.rfind(' ', WordsEnd);
	if (Last > Words + 10) {
		Result.push_back({"last syllable left out", std::string(Block).erase(Last, WordsEnd - Last)});
	}
	std::regex Right("\n([a-z]+) = \"([A-Z_]+)\"");
	bool HasRight = false;
	for (auto i = std::sregex_iterator(Block.begin(), Block.end(), Right); i != std::sregex_iterator(); ++i) {
		HasRight = true;
		int Number = GetRightByName((*i)[2].str().c_str());
		std::string Next = RightNames[(Number + 1) % NUMBER_OF_RIGHTS];
		Result.push_back({(*i)[1].str() + " to " + Next, std::string(Block).replace(static_cast<size_t>(i->position(2)),
				static_cast<size_t>(i->length(2)), Next)});
		size_t LineStart = static_cast<size_t>(i->position(0)) + 1;
		Result.push_back({(*i)[1].str() + " left out", std::string(Block).erase(LineStart,
				Block.find('\n', LineStart) + 1 - LineStart)});
	}
	if (!HasRight) {
		std::string Added = Block;
		while (!Added.empty() && Added.back() == '\n') {
			Added.pop_back();
		}
		Result.push_back({"right added", Added + "\nright = \"KICK\"\n"});
	}
	size_t Script = Block.find("\nscript = \"");
	if (Script != std::string::npos) {
		size_t ScriptEnd = Block.find('"', Script + 11);
		Result.push_back({"script changed", std::string(Block).insert(ScriptEnd - 4, "x")});
		Result.push_back({"script left out", std::string(Block).erase(Script, Block.find('\n', Script + 1) - Script)});
	}
	Result.push_back({"left out", ""});
	return Result;
}

// Every change of one value of Toml must give another table or other rights,
// or an error. Returns the number of changes made.
static int TestMutations(const fs::path &Folder, const fs::path &SpellsToml, const std::string &Toml,
		const std::string &Expected) {
	std::string Header;
	std::vector<std::string> Blocks = SplitCommands(Toml, Header);
	fs::path File = Folder / "mutated.toml";
	int Changes = 0;
	for (size_t b = 0; b < Blocks.size(); b++) {
		for (const auto &Mutation : Mutations(Blocks[b])) {
			std::string Mutated = Header;
			bool First = true;
			for (size_t i = 0; i < Blocks.size(); i++) {
				const std::string &Block = i == b ? Mutation.second : Blocks[i];
				if (!Block.empty()) {
					Mutated += (First ? "" : "\n") + Block;
					First = false;
				}
			}
			Changes++;
			WriteFile(File, Mutated);
			std::string Actual = Snapshot(NewLoader(SpellsToml, File));
			if (Mutated == Toml || (!IsError(Actual) && Differences(Expected, Actual, "", "", "", false) == 0)) {
				fprintf(stderr, "mutation not caught: command block %d: %s\n", static_cast<int>(b),
						Mutation.first.c_str());
				Failures++;
			}
		}
	}
	return Changes;
}

static int Original(const char *SpellsToml, const char *CommandsToml) {
	auto Reference = []() {
		InitReferenceSpells();
		InitReferenceCommandRights();
	};
	std::string Text = Convert(Reference);
	std::string Committed = ReadFile(CommandsToml);
	if (Committed != Text) {
		fprintf(stderr, "%s is not what patella-convert commands writes (%s)\n", CommandsToml,
				IsError(Text) ? Text.c_str() : "a different text");
		Failures++;
	}

	std::string Expected = Snapshot(Reference, false);
	std::string Actual = Snapshot(NewLoader(SpellsToml, CommandsToml));
	if (IsError(Expected) || IsError(Actual)) {
		fprintf(stderr, "a load failed: %s / %s\n", Expected.substr(0, 200).c_str(), Actual.substr(0, 200).c_str());
		return 1;
	}
	if (Same(Expected, Actual, "original", "the code", "the files")) {
		std::map<std::string, std::string> Values = Fields(Actual);
		int Commands = 0, Checks = 0, Scripts = 0;
		for (int SpellNr = 1; SpellNr < MAX_SPELLS; SpellNr++) {
			std::string Prefix = "spell " + std::to_string(SpellNr) + " ";
			bool Command = Values[Prefix + "syllable 0"] == std::to_string(SYLLABLE_AL)
					|| Values[Prefix + "syllable 0"] == std::to_string(SYLLABLE_OM);
			Commands += Command;
			for (int i = 0; i < MAX_COMMAND_RIGHTS; i++) {
				Checks += Values[Prefix + "right " + std::to_string(i)] != std::to_string(NO_RIGHT);
			}
			if (Command) {
				Scripts += Values[Prefix + "script"] != "(none)";
				if (Values[Prefix + "script"] == "(none)") {
					fprintf(stderr, "command %d has no script\n", SpellNr);
					Failures++;
				}
			}
		}
		printf("the code and %s with %s give the same table and rights: %d commands, %d right checks, %d scripts; "
				"%d fields compared\n", SpellsToml, CommandsToml, Commands, Checks, Scripts,
				static_cast<int>(Values.size()));
	}

	int Files = CheckScriptFiles(CommandsToml, Committed);
	printf("%d scripts that %s names, each a file with its handler\n", Files, CommandsToml);

	fs::path Folder = TemporaryFolder("convert-test-commands-original");
	int Changes = TestMutations(Folder, SpellsToml, Committed, Expected);
	fs::remove_all(Folder);
	printf("%d changes of single values of %s, each caught\n", Changes, CommandsToml);
	return Failures == 0 ? 0 : 1;
}

int main(int argc, char **argv) {
	if (argc == 4 && strcmp(argv[1], "--original") == 0) {
		int Result = Original(argv[2], argv[3]);
		if (Failures != 0) {
			fprintf(stderr, "%d failures\n", Failures);
		}
		return Result;
	}
	if (argc != 1) {
		fprintf(stderr, "usage: convert_test_commands [--original SPELLS_TOML COMMANDS_TOML]\n");
		return 2;
	}

	fs::path Folder = TemporaryFolder("convert-test-commands");
	try {
		TestRoundTrips(Folder);
		TestRefusals();
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
