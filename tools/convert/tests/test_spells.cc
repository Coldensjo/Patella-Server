// test_spells.cc: the round trip of the spells. The spell table the game ends
// up with from the table it had in its code (InitReferenceSpells, the old
// InitSpells) without the commands (al and om, test_commands.cc) and from the
// spells.toml patella-convert writes of it, through the game's LoadSpells,
// must be the same, field by field; LoadSpells must refuse what is wrong with
// the file and line.
//
//   convert_test_spells
//     With made-up tables and files in a temporary folder.
//   convert_test_spells --original SPELLS_TOML
//     With the committed spells.toml: it must be exactly what patella-convert
//     spells writes, and give the same table as the code did; and every
//     change of one value of it must give another table or an error.
//
// Every load runs in a child process of its own, on a table filled with a
// byte pattern first. The whole table is compared, spells 0 to 255 (0 is
// never filled): the 10 syllables, rune group and number, name, level, rune
// level, flags, mana, soul points and charges.

#include <climits>
#include <cstdio>
#include <cstring>
#include <regex>
#include <string>
#include <vector>

#include "magicdata.hh"
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

// The table as "field\tvalue" lines.
static std::string DumpSpells() {
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
	}
	return Out;
}

static std::string Snapshot(const std::function<void()> &Fill) {
	return InChild([&]() {
		memset(static_cast<void *>(SpellList), POISON, sizeof(SpellList));
		Fill();
		return DumpSpells();
	});
}

static std::function<void()> NewLoader(const fs::path &File) {
	return [File]() {
		LoadSpells(File.c_str());
	};
}

// The spells.toml patella-convert writes of the table Fill makes.
static std::string Convert(const std::function<void()> &Fill) {
	return InChild([&]() {
		Fill();
		return SpellsToToml();
	});
}

// The reference table without its commands, the spells spells.toml holds.
static void InitReferenceSpellsOnly() {
	InitReferenceSpells();
	for (int i = 1; i < MAX_SPELLS; i++) {
		if (SpellList[i].Syllable[0] == SYLLABLE_AL || SpellList[i].Syllable[0] == SYLLABLE_OM) {
			memset(&SpellList[i], 0, sizeof(TSpellList));
		}
	}
}

// Writes the table Fill makes as spells.toml, loads it and compares. Returns
// the spells.toml text.
static std::string RoundTrip(const fs::path &Folder, const char *Name, const std::function<void()> &Fill) {
	std::string Text = Convert(Fill);
	if (IsError(Text)) {
		fprintf(stderr, "%s: conversion: %s\n", Name, Text.c_str());
		Failures++;
		return Text;
	}
	fs::path File = Folder / (std::string(Name) + ".toml");
	WriteFile(File, Text);
	std::string Expected = Snapshot([&]() {
		for (int i = 1; i < MAX_SPELLS; i++) {
			memset(&SpellList[i], 0, sizeof(TSpellList));
		}
		Fill();
	});
	std::string Actual = Snapshot(NewLoader(File));
	if (IsError(Expected) || IsError(Actual)) {
		fprintf(stderr, "%s: a load failed:\n%s\n%s\n", Name, Expected.substr(0, 300).c_str(),
				Actual.substr(0, 300).c_str());
		Failures++;
		return Text;
	}
	if (!Same(Expected, Actual, Name, "the table", "spells.toml")) {
		fprintf(stderr, "%s: spells.toml was:\n%s", Name, Text.c_str());
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
	// Every syllable, the most syllables, a spell of one syllable, "para"
	// anywhere, every flag, the largest values, rune number 0, spell 255, an
	// empty name and a name with quotes, a backslash and Latin-1.
	std::string Toml = RoundTrip(Folder, "made-up", []() {
		TSpellList *S = &SpellList[1];
		SetWords(*S, {"ex", "ura", "gran"});
		S->Comment = "Healing";
		S->Mana = 25;
		S->Level = 9;
		S->Flags = SPELLFLAG_FACTOR_MIN_100;
		S = &SpellList[2];
		SetWords(*S, {"ex"});
		S->Comment = "";
		S = &SpellList[3];
		SetWords(*S, {"ut", "ana", "evo", "ori", "mort", "lux", "liber", "vita", "flam", "pox"});
		S->Comment = "a \"quoted\" back\\slash \xe4\xf6\xfc\xdf";
		S->Flags = SPELLFLAG_AGGRESSIVE | SPELLFLAG_PREMIUM | SPELLFLAG_FACTOR_MAX_100 | SPELLFLAG_FACTOR_MIN_100;
		S = &SpellList[4];
		SetWords(*S, {"ad", "para", "hur", "para"});
		S->Comment = "123456789012345678901234567890";
		S->RuneGr = 255;
		S->RuneNr = 255;
		S->RuneLevel = USHRT_MAX;
		S->Level = USHRT_MAX;
		S->Amount = INT_MAX;
		S->Mana = INT_MAX;
		S->SoulPoints = INT_MAX;
		S = &SpellList[255];
		SetWords(*S, {"ex", "moe", "ani", "ina", "eta", "amo", "hora", "cogni", "res", "mas"});
		S->Comment = "Last";
		S->RuneGr = 1;
		S = &SpellList[100];
		SetWords(*S, {"ut", "vis", "som", "aqua", "frigo", "tera", "ura", "sio", "grav", "ito"});
		S->Comment = "x";
		S = &SpellList[101];
		SetWords(*S, {"ex", "pan", "vid", "isa", "iva", "con", "al", "ad", "ex", "ut"});
		S->Comment = "y";
		S->SoulPoints = 1;
		S = &SpellList[102];
		SetWords(*S, {"ex", "om", "para"});
		S->Comment = "z";
		S->Flags = SPELLFLAG_PREMIUM;
	});
	CHECK(Toml.find("\n[[spell]]\nnumber = 1\nname = \"Healing\"\nwords = \"exura gran\"\nlevel = 9\nmana = 25\n"
			"soul_points = 0\nflags = [\"factor_min_100\"]\n\n") != std::string::npos);
	CHECK(Toml.find("number = 2\nname = \"\"\nwords = \"ex\"\nlevel = 0\nmana = 0\nsoul_points = 0\n\n")
			!= std::string::npos);
	CHECK(Toml.find("words = \"utana evo ori mort lux liber vita flam pox\"\n") != std::string::npos);
	CHECK(Toml.find("name = \"a \\\"quoted\\\" back\\\\slash \xc3\xa4\xc3\xb6\xc3\xbc\xc3\x9f\"\n")
			!= std::string::npos);
	CHECK(Toml.find("flags = [\"aggressive\", \"premium\", \"factor_max_100\", \"factor_min_100\"]\n")
			!= std::string::npos);
	CHECK(Toml.find("words = \"adpara hur para\"\nlevel = 65535\nmana = 2147483647\nsoul_points = 2147483647\n"
			"rune = { group = 255, number = 255, magic_level = 65535, charges = 2147483647 }\n") != std::string::npos);
	CHECK(Toml.find("number = 255\nname = \"Last\"\n") != std::string::npos);
	CHECK(Toml.find("rune = { group = 1, number = 0, magic_level = 0, charges = 0 }\n") != std::string::npos);
	CHECK(Toml.find("number = 102\n") < Toml.find("number = 255\n"));

	// Each flag on its own; and the empty table.
	for (int Flag : {SPELLFLAG_AGGRESSIVE, SPELLFLAG_PREMIUM, SPELLFLAG_FACTOR_MAX_100, SPELLFLAG_FACTOR_MIN_100}) {
		RoundTrip(Folder, ("flag-" + std::to_string(Flag)).c_str(), [Flag]() {
			SetWords(SpellList[7], {"ut", "evo", "lux"});
			SpellList[7].Comment = "Light";
			SpellList[7].Flags = static_cast<unsigned short>(Flag);
		});
	}
	Toml = RoundTrip(Folder, "empty", []() {});
	CHECK(Toml.find("\n[[spell]]\n") == std::string::npos);

	// The reference table, with its own words, too, without its commands.
	RoundTrip(Folder, "reference", InitReferenceSpellsOnly);

	// The commands of a table are left out.
	std::string WithCommands = Convert([]() {
		SetWords(SpellList[5], {"ex", "ura"});
		SpellList[5].Comment = "Healing";
		SetWords(SpellList[6], {"al", "evo", "para"});
		SpellList[6].Comment = "Get";
		SetWords(SpellList[7], {"om", "ana", "para"});
		SpellList[7].Comment = "Kick";
	});
	CHECK(WithCommands.find("number = 5\n") != std::string::npos);
	CHECK(WithCommands.find("number = 6\n") == std::string::npos);
	CHECK(WithCommands.find("number = 7\n") == std::string::npos);
}

// What a conversion refuses.
static void TestRefusals() {
	auto Refused = [](const char *Name, const std::function<void(TSpellList &)> &Change, const char *What) {
		std::string Out = Convert([&]() {
			TSpellList &S = SpellList[10];
			SetWords(S, {"ut", "evo", "lux"});
			S.Comment = "Light";
			Change(S);
		});
		if (Out.find(std::string("error: ") + What) != 0) {
			fprintf(stderr, "%s: expected \"error: %s...\", got \"%s\"\n", Name, What, Out.substr(0, 200).c_str());
			Failures++;
		}
	};
	Refused("no name", [](TSpellList &S) { S.Comment = nullptr; }, "a spell without a name");
	Refused("long name", [](TSpellList &S) { S.Comment = "1234567890123456789012345678901"; },
			"a spell name longer than 30");
	Refused("no words", [](TSpellList &S) { memset(S.Syllable, 0, sizeof(S.Syllable)); },
			"a spell whose words do not start");
	Refused("para first", [](TSpellList &S) { S.Syllable[0] = SYLLABLE_PARA; }, "a spell whose words do not start");
	Refused("gap", [](TSpellList &S) { S.Syllable[1] = 0; }, "a spell with an unknown syllable or a gap");
	Refused("empty syllable", [](TSpellList &S) { S.Syllable[2] = 41; }, "a spell with an unknown syllable");
	Refused("syllable past the table", [](TSpellList &S) { S.Syllable[2] = MAX_SYLLABLES; },
			"a spell with an unknown syllable");
	Refused("same words", [](TSpellList &) {
		SetWords(SpellList[11], {"ut", "evo", "lux"});
		SpellList[11].Comment = "Light again";
	}, "two spells with the same words");
	Refused("negative mana", [](TSpellList &S) { S.Mana = -1; }, "a negative mana");
	Refused("negative soul points", [](TSpellList &S) { S.SoulPoints = -1; }, "a negative mana");
	Refused("negative charges", [](TSpellList &S) { S.RuneGr = 1; S.Amount = -1; }, "a negative mana");
	Refused("unknown flag", [](TSpellList &S) { S.Flags = 16; }, "a spell flag without a name");
	Refused("rune number without group", [](TSpellList &S) { S.RuneNr = 1; }, "rune values without a rune group");
	Refused("rune level without group", [](TSpellList &S) { S.RuneLevel = 1; }, "rune values without a rune group");
	Refused("charges without group", [](TSpellList &S) { S.Amount = 1; }, "rune values without a rune group");
	for (const char *Class : {"al", "om"}) {
		Refused("command with a level", [Class](TSpellList &S) { SetWords(S, {Class, "evo"}); S.Level = 1; },
				"a command (al or om) with a level");
		Refused("command with mana", [Class](TSpellList &S) { SetWords(S, {Class, "evo"}); S.Mana = 1; },
				"a command (al or om) with a level");
		Refused("command with soul points", [Class](TSpellList &S) { SetWords(S, {Class, "evo"}); S.SoulPoints = 1; },
				"a command (al or om) with a level");
		Refused("command with a flag", [Class](TSpellList &S) { SetWords(S, {Class, "evo"}); S.Flags = 2; },
				"a command (al or om) with a level");
		Refused("command with a rune", [Class](TSpellList &S) { SetWords(S, {Class, "evo"}); S.RuneGr = 1; },
				"a command (al or om) with a level");
	}
	Refused("only a level", [](TSpellList &S) {
		memset(S.Syllable, 0, sizeof(S.Syllable));
		S.Comment = nullptr;
		S.Level = 3;
	}, "a spell without a name");
}

static void TestErrors(const fs::path &Folder) {
	fs::path File = Folder / "errors" / "spells.toml";
	auto Load = [&](const std::string &Text) {
		WriteFile(File, Text);
		return InChild([&]() {
			LoadSpells(File.c_str());
			return std::string();
		});
	};
	auto Expect = [&](const std::string &Text, int Line, const char *What) {
		ExpectError(Load, File, Text, Line, What);
	};
	const std::string Spell = "[[spell]]\nnumber = 10\nname = \"Light\"\nwords = \"utevo lux\"\n";
	Expect("spells = 1\n", 1, "unknown key \"spells\" in the top level");
	Expect("spell = 1\n", 1, "spell must be an array");
	Expect("spell = [1]\n", 1, "an entry of spell must be a table");
	Expect(Spell + "nmae = \"x\"\n", 5, "unknown key \"nmae\" in [[spell]]");
	Expect("[[spell]]\nname = \"x\"\nwords = \"exura\"\n", 1, "an entry of spell needs number");
	Expect("[[spell]]\nnumber = 1\nwords = \"exura\"\n", 1, "an entry of spell needs name");
	Expect("[[spell]]\nnumber = 1\nname = \"x\"\n", 1, "an entry of spell needs words");
	Expect("[[spell]]\nnumber = \"1\"\nname = \"x\"\nwords = \"exura\"\n", 2, "number must be an integer");
	Expect("[[spell]]\nnumber = 0\nname = \"x\"\nwords = \"exura\"\n", 2, "number must lie between 1 and 255");
	Expect("[[spell]]\nnumber = 256\nname = \"x\"\nwords = \"exura\"\n", 2, "number must lie between 1 and 255");
	Expect(Spell + "[[spell]]\nnumber = 10\nname = \"x\"\nwords = \"exura\"\n", 6, "spell 10 is defined twice");
	Expect("[[spell]]\nnumber = 1\nname = 5\nwords = \"exura\"\n", 3, "name must be a string");
	Expect("[[spell]]\nnumber = 1\nname = \"1234567890123456789012345678901\"\nwords = \"exura\"\n", 3,
			"name is longer than 30 characters");
	Expect("[[spell]]\nnumber = 1\nname = \"\xc4\x80\"\nwords = \"exura\"\n", 3,
			"name has a character outside Latin-1");
	Expect("[[spell]]\nnumber = 1\nname = \"a\\u0000b\"\nwords = \"exura\"\n", 3, "name has a zero character");
	auto Words = [](const std::string &Text) {
		return "[[spell]]\nnumber = 1\nname = \"x\"\nwords = " + Text + "\n";
	};
	Expect(Words("5"), 4, "words must be a string");
	Expect(Words("\"\""), 4, "words must be syllables separated by single spaces");
	Expect(Words("\" exura\""), 4, "words must be syllables separated by single spaces");
	Expect(Words("\"exura \""), 4, "words must be syllables separated by single spaces");
	Expect(Words("\"exura  gran\""), 4, "words must be syllables separated by single spaces");
	Expect(Words("\"ex ura\""), 4, "words must be syllables separated by single spaces");
	Expect(Words("\"ura\""), 4, "words must start with ad, ex or ut (al and om start commands, in commands.toml)");
	Expect(Words("\"e\""), 4, "words must start with ad, ex or ut (al and om start commands, in commands.toml)");
	Expect(Words("\"Exura\""), 4, "words must start with ad, ex or ut (al and om start commands, in commands.toml)");
	Expect(Words("\"paraura\""), 4, "words must start with ad, ex or ut (al and om start commands, in commands.toml)");
	Expect(Words("\"alevo para\""), 4, "words must start with ad, ex or ut (al and om start commands, in commands.toml)");
	Expect(Words("\"omana\""), 4, "words must start with ad, ex or ut (al and om start commands, in commands.toml)");
	Expect(Words("\"al\""), 4, "words must start with ad, ex or ut (al and om start commands, in commands.toml)");
	Expect(Words("\"exura grna\""), 4, "unknown syllable \"grna\" in words");
	Expect(Words("\"exUra\""), 4, "unknown syllable \"Ura\" in words");
	Expect(Words("\"exura\\tgran\""), 4, "unknown syllable \"ura\tgran\" in words");
	Expect(Words("\"exura gran gran gran gran gran gran gran gran gran\""), 4, "words have more than 10 syllables");
	Expect(Spell + "[[spell]]\nnumber = 11\nname = \"x\"\nwords = \"utevo lux\"\n", 8,
			"these words are already the ones of spell 10");
	Expect(Spell + "level = -1\n", 5, "level must lie between 0 and 65535");
	Expect(Spell + "level = 65536\n", 5, "level must lie between 0 and 65535");
	Expect(Spell + "mana = -1\n", 5, "mana must lie between 0 and 2147483647");
	Expect(Spell + "mana = 2147483648\n", 5, "mana must lie between 0 and 2147483647");
	Expect(Spell + "soul_points = \"1\"\n", 5, "soul_points must be an integer");
	Expect(Spell + "soul_points = -1\n", 5, "soul_points must lie between 0 and 2147483647");
	Expect(Spell + "flags = \"premium\"\n", 5, "flags must be an array");
	Expect(Spell + "flags = [\"premium\", 1]\n", 5, "a flag must be a string");
	Expect(Spell + "flags = [\"Premium\"]\n", 5, "unknown flag \"Premium\"");
	Expect(Spell + "flags = [\"aggresive\"]\n", 5, "unknown flag \"aggresive\"");
	Expect(Spell + "rune = 79\n", 5, "rune must be a table");
	Expect(Spell + "rune = { group = 79, number = 5, level = 1 }\n", 5, "unknown key \"level\" in rune");
	Expect(Spell + "rune = { number = 5 }\n", 5, "rune needs group");
	Expect(Spell + "rune = { group = 79 }\n", 5, "rune needs number");
	Expect(Spell + "rune = { group = 0, number = 5 }\n", 5, "group must lie between 1 and 255");
	Expect(Spell + "rune = { group = 256, number = 5 }\n", 5, "group must lie between 1 and 255");
	Expect(Spell + "rune = { group = 79, number = -1 }\n", 5, "number must lie between 0 and 255");
	Expect(Spell + "rune = { group = 79, number = 256 }\n", 5, "number must lie between 0 and 255");
	Expect(Spell + "rune = { group = 79, number = 5, magic_level = 65536 }\n", 5,
			"magic_level must lie between 0 and 65535");
	Expect(Spell + "rune = { group = 79, number = 5, charges = -1 }\n", 5, "charges must lie between 0 and 2147483647");
	Expect(Spell + "[spell.rune]\ngroup = 79\nnumber = 5\ncharges = 1.5\n", 8, "charges must be an integer");
	Expect(Spell + "level =\n", 5, "");

	// What loads: no file content, the smallest spell, the rune as a table of
	// its own, a flag given twice, the most syllables.
	CHECK(Load("") == "");
	CHECK(Load(Spell) == "");
	CHECK(Load(Spell + "[spell.rune]\ngroup = 79\nnumber = 5\n") == "");
	CHECK(Load(Spell + "flags = [\"premium\", \"premium\"]\n") == "");
	CHECK(Load(Words("\"exura gran gran gran gran gran gran gran gran\"")) == "");

	// The same spell written two ways gives the same table.
	WriteFile(Folder / "inline.toml",
			Spell + "flags = [\"premium\"]\nrune = { group = 79, number = 5, charges = 2 }\n");
	WriteFile(Folder / "table.toml",
			Spell + "flags = [\"premium\", \"premium\"]\nlevel = 0\n[spell.rune]\ncharges = 2\nnumber = 5\ngroup = 79\n"
			"magic_level = 0\n");
	Same(Snapshot(NewLoader(Folder / "inline.toml")), Snapshot(NewLoader(Folder / "table.toml")), "two ways", "inline",
			"table");

	std::string Missing = InChild([&]() {
		LoadSpells((Folder / "missing.toml").c_str());
		return std::string();
	});
	CHECK(IsError(Missing) && Missing.find((Folder / "missing.toml").string()) != std::string::npos);
}

// The spells of a spells.toml text: its header (up to the first "[[spell]]"),
// and each "[[spell]]" block up to the empty line before the next one.
static std::vector<std::string> SplitSpells(const std::string &Toml, std::string &Header) {
	std::vector<std::string> Blocks;
	size_t Start = Toml.find("\n[[spell]]\n");
	Start = Start == std::string::npos ? Toml.size() : Start + 1;
	Header = Toml.substr(0, Start);
	while (Start < Toml.size()) {
		size_t Next = Toml.find("\n[[spell]]\n", Start);
		Blocks.push_back(Toml.substr(Start, Next == std::string::npos ? std::string::npos : Next - Start));
		Start = Next == std::string::npos ? Toml.size() : Next + 1;
	}
	return Blocks;
}

// The changes of one value of a [[spell]] block: each number one up, the name
// and the words changed, each flag taken away or added, the rune taken away
// or added, the block left out.
static std::vector<std::pair<std::string, std::string>> Mutations(const std::string &Block) {
	std::vector<std::pair<std::string, std::string>> Result;
	std::regex Number("([a-z_]+) = (\\d+)");
	for (auto i = std::sregex_iterator(Block.begin(), Block.end(), Number); i != std::sregex_iterator(); ++i) {
		long Value = std::stol((*i)[2].str());
		std::string Mutated = Block;
		Mutated.replace(static_cast<size_t>(i->position(2)), static_cast<size_t>(i->length(2)),
				std::to_string(Value + 1));
		Result.push_back({(*i)[1].str() + " + 1", Mutated});
		if ((*i)[1].str() == "number" && Block.compare(static_cast<size_t>(i->position(0)) - 1, 1, "\n") == 0) {
			Mutated = Block;
			Mutated.replace(static_cast<size_t>(i->position(2)), static_cast<size_t>(i->length(2)), "254");
			Result.push_back({"number to 254", Mutated});
		}
	}
	size_t Name = Block.find("\nname = \"");
	Result.push_back({"name", std::string(Block).insert(Name + 9, "x")});
	size_t Words = Block.find("\nwords = \"");
	size_t WordsEnd = Block.find('"', Words + 10);
	Result.push_back({"words + vis", std::string(Block).insert(WordsEnd, " vis")});
	std::string Class = Block.substr(Words + 10, 2);
	Result.push_back({"class", std::string(Block).replace(Words + 10, 2, Class == "ex" ? "ut" : "ex")});
	size_t Last = Block.rfind(' ', WordsEnd);
	if (Last > Words + 10) {
		Result.push_back({"last syllable left out", std::string(Block).erase(Last, WordsEnd - Last)});
	}
	size_t Flags = Block.find("\nflags = [");
	for (const char *Flag : {"aggressive", "premium", "factor_max_100", "factor_min_100"}) {
		std::string Quoted = std::string("\"") + Flag + "\"";
		std::string Mutated = Block;
		if (Flags == std::string::npos) {
			size_t Soul = Block.find('\n', Block.find("\nsoul_points = ") + 1);
			Mutated.insert(Soul + 1, "flags = [" + Quoted + "]\n");
		} else {
			size_t End = Block.find(']', Flags);
			size_t At = Block.find(Quoted, Flags);
			if (At != std::string::npos && At < End) {
				Mutated = std::regex_replace(Block, std::regex(", " + Quoted + "|" + Quoted + "(, )?"), "");
				Mutated = std::regex_replace(Mutated, std::regex("\nflags = \\[\\]"), "");
			} else {
				Mutated.insert(End, ", " + Quoted);
			}
		}
		Result.push_back({std::string("flag ") + Flag, Mutated});
	}
	size_t Rune = Block.find("\nrune = {");
	if (Rune != std::string::npos) {
		Result.push_back({"rune left out", std::string(Block).erase(Rune, Block.find('\n', Rune + 1) - Rune)});
	} else {
		Result.push_back({"rune added", Block + "rune = { group = 79, number = 99 }\n"});
	}
	Result.push_back({"left out", ""});
	return Result;
}

// Every change of one value of Toml must give another table or an error.
// Returns the number of changes made.
static int TestMutations(const fs::path &Folder, const std::string &Toml, const std::string &Expected) {
	std::string Header;
	std::vector<std::string> Blocks = SplitSpells(Toml, Header);
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
			std::string Actual = Snapshot(NewLoader(File));
			if (Mutated == Toml || (!IsError(Actual) && Differences(Expected, Actual, "", "", "", false) == 0)) {
				fprintf(stderr, "mutation not caught: spell block %d: %s\n", static_cast<int>(b),
						Mutation.first.c_str());
				Failures++;
			}
		}
	}
	return Changes;
}

static int Original(const char *SpellsToml) {
	std::string Text = Convert(InitReferenceSpells);
	std::string Committed = ReadFile(SpellsToml);
	if (Committed != Text) {
		fprintf(stderr, "%s is not what patella-convert spells writes (%s)\n", SpellsToml,
				IsError(Text) ? Text.c_str() : "a different text");
		Failures++;
	}

	std::string Expected = Snapshot(InitReferenceSpellsOnly);
	std::string Actual = Snapshot(NewLoader(SpellsToml));
	if (IsError(Expected) || IsError(Actual)) {
		fprintf(stderr, "a load failed: %s / %s\n", Expected.substr(0, 200).c_str(), Actual.substr(0, 200).c_str());
		return 1;
	}
	if (Same(Expected, Actual, "original", "the code", "spells.toml")) {
		std::map<std::string, std::string> Values = Fields(Actual);
		int Spells = 0, Runes = 0;
		for (int SpellNr = 1; SpellNr < MAX_SPELLS; SpellNr++) {
			std::string Prefix = "spell " + std::to_string(SpellNr) + " ";
			Spells += Values[Prefix + "name"] != "(null)";
			Runes += Values[Prefix + "rune group"] != "0";
		}
		printf("the code (without its commands) and %s give the same table: %d spells, %d of them runes; %d fields "
				"compared\n", SpellsToml, Spells, Runes, static_cast<int>(Values.size()));
	}

	fs::path Folder = TemporaryFolder("convert-test-spells-original");
	int Changes = TestMutations(Folder, Committed, Expected);
	fs::remove_all(Folder);
	printf("%d changes of single values of %s, each caught\n", Changes, SpellsToml);
	return Failures == 0 ? 0 : 1;
}

int main(int argc, char **argv) {
	if (argc == 3 && strcmp(argv[1], "--original") == 0) {
		int Result = Original(argv[2]);
		if (Failures != 0) {
			fprintf(stderr, "%d failures\n", Failures);
		}
		return Result;
	}
	if (argc != 1) {
		fprintf(stderr, "usage: convert_test_spells [--original SPELLS_TOML]\n");
		return 2;
	}

	fs::path Folder = TemporaryFolder("convert-test-spells");
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
