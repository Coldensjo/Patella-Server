// test_triggers.cc: the round trip of the trigger rules. The tables the game
// ends up with from a moveuse.dat, through the old reader (ReadMoveUseDat), and
// from the triggers.toml patella-convert makes of it, through the game's
// LoadTriggerRules, must be the same, entry by entry; LoadTriggerRules must
// refuse what is wrong with the file and line.
//
//   convert_test_triggers
//     With made-up files in a temporary folder.
//   convert_test_triggers --original ITEMS_TOML MOVEUSE_DAT TRIGGERS_TOML
//     With the original moveuse.dat, the committed items.toml for the object
//     types and the committed triggers.toml: TRIGGERS_TOML must be exactly what
//     patella-convert makes of MOVEUSE_DAT, and give the same tables; and every
//     change of one value, name or entry of it must give rules that behave
//     differently, or an error. Exit code 77 (skipped) when MOVEUSE_DAT is not
//     there.
//
// The round trips run each load in a child process of its own, on tables
// whose first entries are filled with a byte pattern first. They compare the
// tables whole: the counts, every condition (modifier, type, all five
// parameters, used or not) and action (type, all five parameters) up to one
// past the last, every rule of the five databases up to one past the last,
// and the text of every text parameter (the parameter itself is the dynamic
// string's number, so the texts must also be added in the same order).

#include <climits>
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>

#include "map.hh"
#include "moveuse_dat.hh"
#include "triggers_writer.hh"
#include "triggersdata.hh"
#include "objects.hh"
#include "strings.hh"
#include "tomlfile.hh"

#include "test_common.hh"

static const unsigned char POISON = 0xA5;

// The entries filled with the pattern before a load: more than the original
// file has.
static const int POISONED_ENTRIES = 12000;
static const int POISONED_RULES = 3000;

static std::string Printable(const char *Text) {
	if (Text == nullptr) {
		return "(none)";
	}
	std::string Out = "\"";
	for (const unsigned char *c = reinterpret_cast<const unsigned char *>(Text); *c != 0; c++) {
		if (*c >= 0x20 && *c < 0x7F && *c != '\\') {
			Out += static_cast<char>(*c);
		} else {
			char Code[8];
			snprintf(Code, sizeof(Code), "\\x%02X", *c);
			Out += Code;
		}
	}
	return Out + "\"";
}

// The signature of a condition or action type, or nullptr for a number that
// is none.
static const TTriggerSignature *ConditionSignature(int Type) {
	return Type >= 0 && Type < NUMBER_OF_CONDITION_TYPES ? &ConditionSignatures[Type] : nullptr;
}

static const TTriggerSignature *ActionSignature(int Type) {
	return Type >= 0 && Type < NUMBER_OF_ACTION_TYPES ? &ActionSignatures[Type] : nullptr;
}

// The parameters as "field\tvalue" lines, all five, with the texts of the
// text parameters.
static std::string DumpParameters(const std::string &Prefix, const int *Parameters,
		const TTriggerSignature *Signature) {
	std::string Out;
	for (int j = 0; j < MAX_PARAMETERS; j++) {
		Out += Prefix + "parameter " + std::to_string(j) + "\t" + std::to_string(Parameters[j]) + "\n";
		if (Signature != nullptr && j < Signature->NumberOfParameters
				&& Signature->Parameters[j] == PARAMETER_TEXT) {
			Out += Prefix + "text " + std::to_string(j) + "\t"
					+ Printable(GetDynamicString(static_cast<uint32_t>(Parameters[j]))) + "\n";
		}
	}
	return Out;
}

// The tables as "field\tvalue" lines.
static std::string DumpTables() {
	std::string Out;
	Out += "conditions\t" + std::to_string(NumberOfTriggerConditions) + "\n";
	Out += "actions\t" + std::to_string(NumberOfTriggerActions) + "\n";
	for (int i = 1; i <= NumberOfTriggerConditions + 1 && i <= POISONED_ENTRIES; i++) {
		const TTriggerCondition &C = TriggerConditions(i);
		std::string Prefix = "condition " + std::to_string(i) + " ";
		Out += Prefix + "modifier\t" + std::to_string(static_cast<int>(C.Modifier)) + "\n";
		Out += Prefix + "type\t" + std::to_string(static_cast<int>(C.Condition)) + "\n";
		Out += DumpParameters(Prefix, C.Parameters, ConditionSignature(C.Condition));
	}
	for (int i = 1; i <= NumberOfTriggerActions + 1 && i <= POISONED_ENTRIES; i++) {
		const TTriggerAction &A = TriggerActions(i);
		std::string Prefix = "action " + std::to_string(i) + " ";
		Out += Prefix + "type\t" + std::to_string(static_cast<int>(A.Action)) + "\n";
		Out += DumpParameters(Prefix, A.Parameters, ActionSignature(A.Action));
	}
	for (int Event = 0; Event < NUMBER_OF_EVENT_TYPES; Event++) {
		TTriggerDatabase &Database = TriggerDatabases[Event];
		std::string Prefix = "event " + std::to_string(Event) + " ";
		Out += Prefix + "rules\t" + std::to_string(Database.NumberOfRules) + "\n";
		for (int r = 1; r <= Database.NumberOfRules + 1 && r <= POISONED_RULES; r++) {
			const TTriggerRule &Rule = Database.Rules(r);
			Out += Prefix + "rule " + std::to_string(r) + "\t" + std::to_string(Rule.FirstCondition) + " "
					+ std::to_string(Rule.LastCondition) + " " + std::to_string(Rule.FirstAction) + " "
					+ std::to_string(Rule.LastAction) + "\n";
		}
	}
	return Out;
}

static void Poison() {
	for (int i = 1; i <= POISONED_ENTRIES; i++) {
		memset(static_cast<void *>(&TriggerConditions(i)), POISON, sizeof(TTriggerCondition));
		memset(static_cast<void *>(&TriggerActions(i)), POISON, sizeof(TTriggerAction));
	}
	for (int Event = 0; Event < NUMBER_OF_EVENT_TYPES; Event++) {
		for (int r = 1; r <= POISONED_RULES; r++) {
			memset(static_cast<void *>(&TriggerDatabases[Event].Rules(r)), POISON, sizeof(TTriggerRule));
		}
		memset(static_cast<void *>(&TriggerDatabases[Event].NumberOfRules), POISON, sizeof(int));
	}
	memset(static_cast<void *>(&NumberOfTriggerConditions), POISON, sizeof(int));
	memset(static_cast<void *>(&NumberOfTriggerActions), POISON, sizeof(int));
}

static std::function<void()> OldLoader(const fs::path &File) {
	return [File]() {
		TMoveUseDatExtras Extras;
		ReadMoveUseDat(File.c_str(), Extras);
	};
}

// The game's way: InitTriggers sets the counts to 0, LoadTriggers loads.
static std::function<void()> NewLoader(const fs::path &File) {
	return [File]() {
		NumberOfTriggerConditions = 0;
		NumberOfTriggerActions = 0;
		LoadTriggerRules(File.c_str());
	};
}

static std::string Snapshot(const std::function<void()> &Load) {
	return InChild([&]() {
		Poison();
		Load();
		return DumpTables();
	});
}

// The object types defined in the items.toml loaded in this process.
static std::vector<bool> Defined;

// The triggers.toml patella-convert makes of a moveuse.dat file (with the
// object types loaded in this process).
static std::string Convert(const fs::path &File) {
	return InChild([&]() {
		TMoveUseDatExtras Extras;
		ReadMoveUseDat(File.c_str(), Extras);
		return TriggersToToml(Extras, Defined);
	});
}

// Converts a moveuse.dat text, loads both and compares them. Returns the
// triggers.toml text.
static std::string RoundTrip(const fs::path &Folder, const char *Name, const std::string &MoveUseDat) {
	fs::path Original = Folder / (std::string(Name) + ".dat");
	fs::path Converted = Folder / (std::string(Name) + ".toml");
	WriteFile(Original, MoveUseDat);
	std::string Text = Convert(Original);
	if (IsError(Text)) {
		fprintf(stderr, "%s: conversion: %s\n", Name, Text.c_str());
		Failures++;
		return Text;
	}
	WriteFile(Converted, Text);
	std::string Expected = Snapshot(OldLoader(Original));
	std::string Actual = Snapshot(NewLoader(Converted));
	if (IsError(Expected) || IsError(Actual)) {
		fprintf(stderr, "%s: a load failed:\n%s\n%s\n", Name, Expected.substr(0, 300).c_str(),
				Actual.substr(0, 300).c_str());
		Failures++;
		return Text;
	}
	if (!Same(Expected, Actual, Name, "moveuse.dat", "triggers.toml")) {
		fprintf(stderr, "%s: triggers.toml was:\n%s", Name, Text.c_str());
	}
	return Text;
}

// The rules of a triggers.toml text: what follows the header.
static std::string Rules(const std::string &Toml) {
	size_t Start = Toml.find("\n[[rule]]\n");
	return Start == std::string::npos ? std::string() : Toml.substr(Start);
}

// The object types of the made-up files: 100 to 103 with names (one Latin-1,
// one with a control character), 104 named "", 105 without a name.
static const char *const TestItems =
	"[[item]]\nid = 100\nname = \"a lever\"\n\n"
	"[[item]]\nid = 101\nname = \"a gr\xc3\xbc" "ner stone\"\n\n"
	"[[item]]\nid = 102\nname = \"two\\nlines\"\n\n"
	"[[item]]\nid = 103\nname = \"a key\"\n\n"
	"[[item]]\nid = 104\nname = \"\"\n\n"
	"[[item]]\nid = 105\n";

// The skill names of the old format, by skill number (GetSkillByName).
static const char *const OldSkillNames[] = {
	"Level", "MagicLevel", "HitPoints", "Mana", "GoStrength", "CarryStrength", "Shielding", "DistanceFighting",
	"SwordFighting", "ClubFighting", "AxeFighting", "FistFighting", "MagicDefense", "Fishing", "Eating", "Shining",
	"Illusion", "Poison", "Burning", "Energy", "Drunken", "ManaShield", "SoulPoints",
};

static void TestRoundTrips(const fs::path &Folder) {
	// Every condition and every action, the modifiers, upper case, numbers
	// that wrap, coordinates and vectors out of range, texts with escapes and
	// Latin-1, notes.
	std::string Text =
		"# a comment\n"
		"BEGIN \"Every Condition\"\n"
		"Use, IsPosition (Obj1,[32000,31000,7]), IsType (Obj1,100), IsCreature (User), IsPlayer (Obj2), "
		"HasFlag (Obj1,Clip), HasTypeAttribute (Obj1,Weight,<,100), HasInstanceAttribute (Obj1,Charges,<=,5), "
		"HasText (Obj1,\"Hello \\\"World\\\"\\n\\\\ \xe4\"), IsPeaceful (User), MayLogout (User), "
		"HasProfession (User,2), HasLevel (User,>=,20), HasRight (User,PREMIUM_ACCOUNT) -> NOP\n"
		"MultiUse, HasQuestValue (User,123,=,1), TestSkill (User,Fishing,80,50), CountObjects (Obj2,>,3), "
		"CountObjectsOnMap ([32000,32001,7],<>,0), IsObjectThere ([32000,32000,7],101), "
		"IsCreatureThere ([32000,32000,6]), IsPlayerThere ([32001,32000,7]), "
		"IsObjectInInventory (User,102,2), IsProtectionZone (User), IsHouse (Obj1), IsHouseOwner (Obj1,User), "
		"IsDressed (Temp), Random (50) -> Nop \"a note\"\n"
		"END\n"
		"BEGIN \"Every Action\"\n"
		"Collision, IsType (Obj1,100) -> CreateOnMap([32000,32000,7],103,0), Create(Obj2,103,5), "
		"MonsterOnMap([32000,32000,7],35), Monster(Obj1,-1), EffectOnMap([32000,32000,7],13), Effect(Obj1,3), "
		"TextOnMap([32000,32000,7],\"Hi\",2), Text(User,\"\",1), ChangeOnMap([32000,32000,7],100,101,0), "
		"Change(Obj1,104,0), ChangeRel(Obj1,[0,-1,0],100,105,7) \"\"\n"
		"Separation, IsType (Obj1,100) -> SetAttribute(Obj1,ContainerLiquidType,5), "
		"ChangeAttribute(Obj1,SavedExpireTime,-60), SetQuestValue(User,200,1), Damage(Obj1,User,2,100), "
		"SetStart(User,[32000,32000,7]), WriteName(User,\"%N\",Obj1), WriteText(\"text\",Obj1), Logout(User), "
		"MoveAllOnMap([32000,32000,7],[32001,32000,7]), MoveAll(Obj1,[32000,32000,7]), "
		"MoveAllRel(Obj1,[1,0,0]), MoveTopOnMap([32000,32000,7],100,[32001,32000,7]) \"Gr\xfc\xdf" "e \\\"q\\\"\"\n"
		"Movement, IsType (Obj1,100) -> MoveTop(Obj1,[32000,32000,7]), MoveTopRel(Obj1,[0,0,-1]), "
		"Move(Obj1,[32000,32000,7]), MoveRel(User,Obj1,[0,0,0]), Retrieve(Obj1,[1,1,0],[-1,-1,0]), "
		"DeleteAllOnMap([32000,32000,7]), DeleteTopOnMap([32000,32000,7],100), DeleteOnMap([32000,32000,7],101), "
		"Delete(Obj1), DeleteInInventory(User,102,1), Description(Obj1,User), "
		"LoadDepot([32000,32000,7],103,User,1), SaveDepot([32000,32000,7],103,User,2), SendMail(Obj1), Nop\n"
		"END\n"
		"BEGIN \"Modifiers and Quirks\"\n"
		"Use, !IsHouse (Obj1), ~IsObjectThere ([32000,32000,7],100), ! IsType (Obj1,101) -> Nop\n"
		"USE, ISTYPE (OBJ1,100), HASFLAG (OBJ1,CLIP), HASRIGHT (USER,special_moveuse) -> EFFECT(OBJ1,3)\n"
		"Use, Random (4294967295), Random (2147483648), Random (-2147483648), Random (-0) -> "
		"Effect(Obj1,4294967296), Effect(Obj1,-5)\n"
		"Use, IsPosition (Obj1,[0,0,0]), IsPosition (Obj1,[65535,65535,15]), IsPosition (Obj1,[32000,32000,16]), "
		"IsPosition (Obj1,[-1,5,7]), IsPosition (Obj1,[24576,40959,0]) -> MoveRel(User,Obj1,[-8192,8191,-8]), "
		"MoveRel(User,Obj1,[0,0,7]), MoveRel(User,Obj1,[100000,0,0]), MoveRel(User,Obj1,[0,0,8])\n"
		"Use, IsType (Obj1,5000), IsType (Obj1,0), HasText (Obj1,\"\\t\x01\x7f\xff\") -> Text(Obj1,\"" +
		std::string(3998, 'x') + "\",0)\n"
		"END\n";
	std::string Toml = RoundTrip(Folder, "everything", Text);
	CHECK(Toml.find("\t{ name = \"is_type\", args = [\"obj1\", 100] },  # 100: a lever\n") != std::string::npos);
	CHECK(Toml.find("# 101: a gr\xc3\xbcner stone\n") != std::string::npos);
	CHECK(Toml.find("# 102: two?lines\n") != std::string::npos);
	CHECK(Toml.find("args = [\"obj1\", 104, 0] },  # 104: (no name)\n") != std::string::npos);
	CHECK(Toml.find("[\"obj1\", [0, -1, 0], 100, 105, 7] },  # 100: a lever; 105: (no name)\n")
			!= std::string::npos);
	CHECK(Toml.find("\t{ name = \"is_house\", negate = true, args = [\"obj1\"] },\n") != std::string::npos);
	CHECK(Toml.find("\t{ name = \"is_object_there\", ignore_result = true, args = [[32000, 32000, 7], 100] },")
			!= std::string::npos);
	CHECK(Toml.find("\t{ name = \"nop\" },\n") != std::string::npos);
	CHECK(Toml.find("note = \"a note\"\n") != std::string::npos);
	CHECK(Toml.find("note = \"\"\n") != std::string::npos);
	CHECK(Toml.find("note = \"Gr\xc3\xbc\xc3\x9f" "e \\\"q\\\"\"\n") != std::string::npos);
	CHECK(Toml.find("\"Hello \\\"World\\\"\\n\\\\ \xc3\xa4\"") != std::string::npos);
	CHECK(Toml.find("args = [\"obj1\", -1] }") != std::string::npos);
	CHECK(Toml.find("args = [-2147483648] }") != std::string::npos);
	CHECK(Toml.find("[\"user\", \"special_moveuse\"]") != std::string::npos);
	CHECK(Toml.find("[\"user\", \"fishing\", 80, 50]") != std::string::npos);
	CHECK(Toml.find("[[32000, 32001, 7], \"<>\", 0]") != std::string::npos);
	CHECK(Toml.find("[\"user\", \"obj1\", [-8192, 8191, -8]]") != std::string::npos);
	CHECK(Toml.find("event = \"multi_use\"\n") != std::string::npos);
	CHECK(Toml.find("section = [\"Modifiers and Quirks\"]\nevent = \"use\"\nconditions = [\n") != std::string::npos);
	CHECK(Toml.find("[\"obj1\", 5000] },  # 5000: (not in items.toml)\n") != std::string::npos);
	CHECK(Toml.find("# a comment") == std::string::npos);

	// Every flag, type attribute, instance attribute, skill and comparison.
	Text = "";
	for (int i = 0; i < FLAGS; i++) {
		Text += std::string("Use, HasFlag (Obj1,") + FlagNames[i] + ") -> Nop\n";
	}
	for (int i = 0; i < TYPEATTRIBUTES; i++) {
		Text += std::string("Use, HasTypeAttribute (Obj1,") + TypeAttributeNames[i] + ",=," + std::to_string(i)
				+ ") -> Nop\n";
	}
	for (int i = 0; i < INSTANCEATTRIBUTES; i++) {
		Text += std::string("Use, HasInstanceAttribute (Obj1,") + InstanceAttributeNames[i] + ",<,1) -> "
				+ "SetAttribute(Obj1," + InstanceAttributeNames[i] + ",2), ChangeAttribute(Obj1,"
				+ InstanceAttributeNames[i] + ",3)\n";
	}
	for (const char *Skill : OldSkillNames) {
		Text += std::string("Use, TestSkill (User,") + Skill + ",1,2) -> Nop\n";
	}
	for (const char *Comparison : {"<", "<=", "=", ">=", ">", "<>"}) {
		Text += std::string("Use, HasLevel (User,") + Comparison + ",1) -> Nop\n";
	}
	Toml = RoundTrip(Folder, "names", Text);
	for (int i = 0; i < NUMBER_OF_SKILL_KEYS; i++) {
		CHECK(Toml.find(std::string("[\"user\", \"") + SkillKeys[i] + "\", 1, 2]") != std::string::npos);
	}
	for (int i = 0; i < INSTANCEATTRIBUTES; i++) {
		CHECK(Toml.find(std::string("[\"obj1\", \"") + InstanceAttributeKeys[i] + "\", 3]") != std::string::npos);
	}

	// Sections: rules outside any, nested ones, a section with rules before
	// and after a subsection, names with quotes, slashes, Latin-1 and none;
	// events taking turns.
	Text =
		"Use, IsType (Obj1,100) -> Nop\n"
		"BEGIN \"A\"\n"
		"Collision, IsType (Obj1,100) -> Nop\n"
		"BEGIN \"B/C \\\"d\\\" \xe9\"\n"
		"BEGIN \"\"\n"
		"Use, IsType (Obj1,101) -> Nop\n"
		"END\n"
		"Use, IsType (Obj1,102) -> Nop\n"
		"END\n"
		"Separation, IsType (Obj1,100) -> Nop\n"
		"BEGIN \"B\"\n"
		"Use, IsType (Obj1,103) -> Nop\n"
		"END\n"
		"END\n"
		"Use, IsType (Obj1,104) -> Nop\n"
		"BEGIN \"A\"\n"
		"BEGIN \"B\"\n"
		"MultiUse, IsType (Obj1,100) -> Nop\n"
		"END\n"
		"END\n"
		"Use, IsType (Obj1,100) -> Nop\n";
	// The two top-level sections "A", with a rule outside them between, stay
	// apart; so does the top-level rule after them.
	Text = std::string("BEGIN \"Z\"\nMovement, IsType (Obj1,100) -> Nop\nEND\n") + Text;
	Toml = RoundTrip(Folder, "sections", Text);
	CHECK(Rules(Toml).rfind("\n[[rule]]\nsection = [\"Z\"]\nevent = \"movement\"\n", 0) == 0);
	CHECK(Toml.find("\n[[rule]]\nevent = \"use\"\nconditions = [\n\t{ name = \"is_type\", args = [\"obj1\", 100] }")
			!= std::string::npos);
	CHECK(Toml.find("section = [\"A\", \"B/C \\\"d\\\" \xc3\xa9\", \"\"]\n") != std::string::npos);
	CHECK(Toml.find("section = [\"A\", \"B/C \\\"d\\\" \xc3\xa9\"]\nevent = \"use\"\n") != std::string::npos);
	CHECK(Toml.find("section = [\"A\", \"B\"]\nevent = \"use\"\n") != std::string::npos);
	CHECK(Toml.find("section = [\"A\", \"B\"]\nevent = \"multi_use\"\n") != std::string::npos);

	// No rules at all; a single one.
	Toml = RoundTrip(Folder, "empty", "");
	CHECK(Rules(Toml).empty());
	RoundTrip(Folder, "single", "Separation, Random (1) -> Delete(Obj1)");
}

// What a conversion refuses.
static void TestRefusals(const fs::path &Folder) {
	auto Refused = [&](const char *Name, const std::string &Text, const char *What) {
		WriteFile(Folder / Name, Text);
		std::string Out = Convert(Folder / Name);
		if (Out.find(std::string("error: ") + What) != 0) {
			fprintf(stderr, "%s: expected \"error: %s...\", got \"%s\"\n", Name, What, Out.substr(0, 200).c_str());
			Failures++;
		}
	};
	const char *Rule = "Use, IsType (Obj1,100) -> Nop\n";
	Refused("empty-section.dat", std::string("BEGIN \"A\"\nEND\n") + Rule, "a section without rules");
	Refused("empty-inner.dat", std::string("BEGIN \"A\"\n") + Rule + "BEGIN \"B\"\nEND\nEND\n",
			"a section without rules");
	Refused("empty-last.dat", std::string(Rule) + "BEGIN \"A\"\nEND\n", "a section without rules");
	Refused("only-empty.dat", "BEGIN \"A\"\nEND\n", "a section without rules");
	Refused("same-again.dat", std::string("BEGIN \"A\"\n") + Rule + "END\nBEGIN \"A\"\n" + Rule + "END\n",
			"a section without rules, or two neighbouring sections of the same name");
	Refused("same-outer.dat", std::string("BEGIN \"A\"\nBEGIN \"B\"\n") + Rule + "END\nEND\nBEGIN \"A\"\n"
			+ "BEGIN \"C\"\n" + Rule + "END\nEND\n", "a section without rules, or two neighbouring sections");
	Refused("end.dat", std::string(Rule) + "END\n", "an END without its BEGIN");
	Refused("begin.dat", std::string("BEGIN \"A\"\n") + Rule, "a BEGIN without its END");
	Refused("unknown-condition.dat", "Use, Foo, IsType (Obj1,100) -> Nop\n", "a condition the game does not know");
	Refused("unknown-action.dat", "Use, IsType (Obj1,100) -> Foo\nUse, IsType (Obj1,100) -> Nop\n",
			"an action the game does not know");
	Refused("syntax.dat", "Use, IsType (Obj1,100) Nop\n", "error in script-file");
	Refused("type.dat", "Use, IsType (Obj1,70000) -> Nop\n", "error in script-file");
	CHECK(Convert(Folder / "missing.dat").find("error: Cannot open") == 0);
}

static void TestErrors(const fs::path &Folder) {
	fs::path File = Folder / "errors" / "triggers.toml";
	auto Load = [&](const std::string &Text) {
		WriteFile(File, Text);
		return InChild([&]() {
			NumberOfTriggerConditions = 0;
			NumberOfTriggerActions = 0;
			LoadTriggerRules(File.c_str());
			return std::string();
		});
	};
	auto Expect = [&](const std::string &Text, int Line, const char *What) {
		ExpectError(Load, File, Text, Line, What);
	};
	// A rule with the given condition and action lines.
	auto Rule = [](const std::string &Condition, const std::string &Action) {
		return "[[rule]]\nevent = \"use\"\nconditions = [\n\t" + Condition + ",\n]\nactions = [\n\t" + Action
				+ ",\n]\n";
	};
	auto C = [&](const std::string &Condition) {
		return Rule(Condition, "{ name = \"nop\" }");
	};
	auto A = [&](const std::string &Action) {
		return Rule("{ name = \"random\", args = [1] }", Action);
	};

	Expect("rules = 1\n", 1, "unknown key \"rules\" in the top level");
	Expect("rule = 1\n", 1, "rule must be an array");
	Expect("rule = [1]\n", 1, "an entry of rule must be a table");
	Expect("[[rule]]\nevent = \"use\"\nfoo = 1\n", 3, "unknown key \"foo\" in [[rule]]");
	Expect("[[rule]]\nconditions = [{ name = \"random\", args = [1] }]\nactions = [{ name = \"nop\" }]\n", 1,
			"a rule needs event");
	Expect("[[rule]]\nevent = 1\n", 2, "event must be a string");
	Expect("[[rule]]\nevent = \"walk\"\n", 2, "event: \"walk\" is unknown");
	Expect("[[rule]]\nevent = \"use\"\nactions = [{ name = \"nop\" }]\n", 1, "a rule needs conditions");
	Expect("[[rule]]\nevent = \"use\"\nconditions = [{ name = \"random\", args = [1] }]\n", 1,
			"a rule needs actions");
	Expect("[[rule]]\nevent = \"use\"\nconditions = []\nactions = [{ name = \"nop\" }]\n", 3,
			"conditions must not be empty");
	Expect("[[rule]]\nevent = \"use\"\nconditions = [{ name = \"random\", args = [1] }]\nactions = []\n", 4,
			"actions must not be empty");
	Expect("[[rule]]\nevent = \"use\"\nconditions = 1\n", 3, "conditions must be an array");
	Expect(C("1"), 4, "an entry of conditions must be a table");
	Expect(C("{ args = [1] }"), 4, "an entry of conditions needs name");
	Expect(C("{ name = 1 }"), 4, "name must be a string");
	Expect(C("{ name = \"isposition\" }"), 4, "unknown condition \"isposition\"");
	Expect(C("{ name = \"random\", foo = 1 }"), 4, "unknown key \"foo\" in an entry of conditions");
	Expect(C("{ name = \"random\" }"), 4, "random takes 1 arguments, not 0");
	Expect(C("{ name = \"random\", args = [1, 2] }"), 4, "random takes 1 arguments, not 2");
	Expect(C("{ name = \"random\", args = 1 }"), 4, "args must be an array");
	Expect(C("{ name = \"random\", args = [1.5] }"), 4, "argument 1 of random must be an integer");
	Expect(C("{ name = \"random\", args = [\"1\"] }"), 4, "argument 1 of random must be an integer");
	Expect(C("{ name = \"random\", args = [2147483648] }"), 4,
			"argument 1 of random must lie between -2147483648 and 2147483647");
	Expect(C("{ name = \"random\", args = [-2147483649] }"), 4, "argument 1 of random must lie between");
	Expect(C("{ name = \"is_house\", args = [\"obj3\"] }"), 4, "argument 1 of is_house: \"obj3\" is unknown");
	Expect(C("{ name = \"is_house\", args = [\"Obj1\"] }"), 4, "argument 1 of is_house: \"Obj1\" is unknown");
	Expect(C("{ name = \"is_house\", args = [1] }"), 4, "argument 1 of is_house must be a string");
	Expect(C("{ name = \"is_type\", args = [\"obj1\", 70000] }"), 4,
			"argument 2 of is_type: object type 70000 does not exist");
	Expect(C("{ name = \"is_type\", args = [\"obj1\", -1] }"), 4, "object type -1 does not exist");
	Expect(C("{ name = \"is_type\", args = [\"obj1\", \"100\"] }"), 4, "argument 2 of is_type must be an integer");
	Expect(C("{ name = \"has_flag\", args = [\"obj1\", \"Clip\"] }"), 4, "argument 2 of has_flag: \"Clip\" is unknown");
	Expect(C("{ name = \"has_type_attribute\", args = [\"obj1\", \"keydoor_target\", \"=\", 1] }"), 4,
			"argument 2 of has_type_attribute: \"keydoor_target\" is unknown");
	Expect(C("{ name = \"has_type_attribute\", args = [\"obj1\", \"weight\", \"==\", 1] }"), 4,
			"argument 3 of has_type_attribute: \"==\" is unknown");
	Expect(C("{ name = \"has_instance_attribute\", args = [\"obj1\", \"Charges\", \"=\", 1] }"), 4,
			"argument 2 of has_instance_attribute: \"Charges\" is unknown");
	Expect(C("{ name = \"has_right\", args = [\"user\", \"gamemaster\"] }"), 4,
			"argument 2 of has_right: \"gamemaster\" is unknown");
	Expect(C("{ name = \"test_skill\", args = [\"user\", \"magiclevel\", 1, 2] }"), 4,
			"argument 2 of test_skill: \"magiclevel\" is unknown");
	Expect(C("{ name = \"has_text\", args = [\"obj1\", \"\xe2\x82\xac\"] }"), 4,
			"argument 2 of has_text has a character outside Latin-1");
	Expect(C("{ name = \"has_text\", args = [\"obj1\", \"a\\u0000b\"] }"), 4,
			"argument 2 of has_text has a zero character");
	Expect(C("{ name = \"has_text\", args = [\"obj1\", \"" + std::string(3999, 'x') + "\"] }"), 4,
			"argument 2 of has_text is longer than 3998 characters");
	Expect(C("{ name = \"has_text\", args = [\"obj1\", 1] }"), 4, "argument 2 of has_text must be a string");
	Expect(C("{ name = \"is_position\", args = [\"obj1\", 1] }"), 4,
			"argument 2 of is_position must be an array [x, y, z]");
	Expect(C("{ name = \"is_position\", args = [\"obj1\", [32000, 32000]] }"), 4,
			"argument 2 of is_position must be an array [x, y, z]");
	Expect(C("{ name = \"is_position\", args = [\"obj1\", [32000, 32000, 7, 1]] }"), 4,
			"argument 2 of is_position must be an array [x, y, z]");
	Expect(C("{ name = \"is_position\", args = [\"obj1\", [24575, 32000, 7]] }"), 4,
			"x of argument 2 of is_position must lie between 24576 and 40959");
	Expect(C("{ name = \"is_position\", args = [\"obj1\", [32000, 40960, 7]] }"), 4,
			"y of argument 2 of is_position must lie between 24576 and 40959");
	Expect(C("{ name = \"is_position\", args = [\"obj1\", [32000, 32000, 16]] }"), 4,
			"z of argument 2 of is_position must lie between 0 and 15");
	Expect(C("{ name = \"is_position\", args = [\"obj1\", [32000, \"a\", 7]] }"), 4,
			"y of argument 2 of is_position must be an integer");
	Expect(C("{ name = \"random\", args = [1], negate = 1 }"), 4, "negate must be true or false");
	Expect(C("{ name = \"random\", args = [1], ignore_result = \"yes\" }"), 4, "ignore_result must be true or false");
	Expect(C("{ name = \"random\", args = [1], negate = true, ignore_result = true }"), 4,
			"a condition cannot have both negate and ignore_result");
	Expect(A("1"), 7, "an entry of actions must be a table");
	Expect(A("{ name = \"nop\", negate = true }"), 7, "unknown key \"negate\" in an entry of actions");
	Expect(A("{ name = \"nop\", args = [\"obj1\"] }"), 7, "nop takes 0 arguments, not 1");
	Expect(A("{ name = \"random\", args = [1] }"), 7, "unknown action \"random\"");
	Expect(A("{ name = \"move_rel\", args = [\"user\", \"obj1\", [8192, 0, 0]] }"), 7,
			"x of argument 3 of move_rel must lie between -8192 and 8191");
	Expect(A("{ name = \"move_rel\", args = [\"user\", \"obj1\", [0, -8193, 0]] }"), 7,
			"y of argument 3 of move_rel must lie between -8192 and 8191");
	Expect(A("{ name = \"move_rel\", args = [\"user\", \"obj1\", [0, 0, 8]] }"), 7,
			"z of argument 3 of move_rel must lie between -8 and 7");
	Expect(A("{ name = \"change\", args = [\"obj1\", 70000, 0] }"), 7, "object type 70000 does not exist");
	Expect("[[rule]]\nsection = \"A\"\n", 2, "section must be an array");
	Expect("[[rule]]\nsection = [1]\n", 2, "a section name must be a Latin-1 string");
	Expect("[[rule]]\nsection = [\"\xe2\x82\xac\"]\n", 2, "a section name must be a Latin-1 string");
	Expect("[[rule]]\nnote = 1\n", 2, "note must be a Latin-1 string");
	Expect("[[rule]]\nevent = \"use\"\nconditions = [\n", 3, "encountered end-of-file");
	Expect(C("{ name = \"random\", args = [1] }") + "\n" + C("{ name = \"random\", args = [x] }"), 13, "");

	// What loads: no rules; nop with args = []; every name the header lists.
	CHECK(Load("") == "");
	CHECK(Load("# nothing\n") == "");
	CHECK(Load(A("{ name = \"nop\", args = [] }")) == "");
	CHECK(Load(C("{ name = \"random\", args = [1], negate = false, ignore_result = false }")) == "");
	CHECK(Load(C("{ name = \"is_position\", args = [\"obj1\", [24576, 40959, 15]] }")) == "");
	CHECK(Load(A("{ name = \"move_rel\", args = [\"user\", \"obj1\", [-8192, 8191, -8]] }")) == "");
	CHECK(Load(C("{ name = \"has_text\", args = [\"obj1\", \"" + std::string(3998, 'x') + "\"] }")) == "");

	std::string Missing = InChild([&]() {
		LoadTriggerRules((Folder / "missing.toml").c_str());
		return std::string();
	});
	CHECK(IsError(Missing) && Missing.find((Folder / "missing.toml").string()) != std::string::npos);
}

// The rules as the game uses them, as one text: for each event its rules in
// order, each with its conditions (modifier, type, the parameters it uses,
// texts as their text) and actions. Two loads that give the same text behave
// the same. Deletes the texts afterwards.
static std::string RulesInUse() {
	std::string Out;
	std::vector<uint32_t> Texts;
	auto Parameters = [&](const int *Values, const TTriggerSignature *Signature) {
		for (int j = 0; Signature != nullptr && j < Signature->NumberOfParameters; j++) {
			if (Signature->Parameters[j] == PARAMETER_TEXT) {
				Out += " " + Printable(GetDynamicString(static_cast<uint32_t>(Values[j])));
				Texts.push_back(static_cast<uint32_t>(Values[j]));
			} else {
				Out += " " + std::to_string(Values[j]);
			}
		}
	};
	for (int Event = 0; Event < NUMBER_OF_EVENT_TYPES; Event++) {
		TTriggerDatabase &Database = TriggerDatabases[Event];
		for (int r = 1; r <= Database.NumberOfRules; r++) {
			const TTriggerRule &Rule = Database.Rules(r);
			Out += "rule " + std::to_string(Event) + "\n";
			for (int i = Rule.FirstCondition; i <= Rule.LastCondition; i++) {
				const TTriggerCondition &C = TriggerConditions(i);
				Out += "c " + std::to_string(static_cast<int>(C.Modifier)) + " "
						+ std::to_string(static_cast<int>(C.Condition));
				Parameters(C.Parameters, ConditionSignature(C.Condition));
				Out += "\n";
			}
			for (int i = Rule.FirstAction; i <= Rule.LastAction; i++) {
				const TTriggerAction &A = TriggerActions(i);
				Out += "a " + std::to_string(static_cast<int>(A.Action));
				Parameters(A.Parameters, ActionSignature(A.Action));
				Out += "\n";
			}
		}
	}
	for (uint32_t Text : Texts) {
		DeleteDynamicString(Text);
	}
	CleanupDynamicStrings();
	return Out;
}

// Loads a parsed triggers.toml in this process: the rules in use, or
// "error: ..." for a const char * the load threw.
static std::string LoadInProcess(const toml::table &File) {
	NumberOfTriggerConditions = 0;
	NumberOfTriggerActions = 0;
	try {
		ReadTriggerRules(File);
	} catch (const char *Error) {
		return std::string("error: ") + Error;
	}
	return RulesInUse();
}

// The next key after Value among Count keys (the first after the last), or
// Value + "x" when it is none of them.
static std::string NextKey(const std::string &Value, const char *const *Keys, int Count) {
	for (int i = 0; i < Count; i++) {
		if (Value == Keys[i]) {
			return Keys[(i + 1) % Count];
		}
	}
	return Value + "x";
}

static std::string NextKey(const std::string &Value, const TTriggerKey *Keys, int Count) {
	std::vector<const char *> Names;
	for (int i = 0; i < Count; i++) {
		Names.push_back(Keys[i].Key);
	}
	return NextKey(Value, Names.data(), Count);
}

// Every change of one value, name or entry of the rules, made in the parsed
// file and undone after, must give other rules in use or an error. Changes:
// each event to the next; each condition and action name to the next of its
// list; each argument (an integer one up, each coordinate or vector component
// one up, a name to the next one of its kind, a text lengthened); negate and
// ignore_result added or taken away; each condition and action left out
// (where there are two or more); each rule left out; each rule swapped with
// the next rule of the same event when the two differ. Returns the number of
// changes made.
static int TestMutations(toml::table &File, const std::string &Expected) {
	int Changes = 0;
	std::string Where;
	auto Try = [&](const std::string &What) {
		Changes++;
		std::string Actual = LoadInProcess(File);
		if (Actual == Expected) {
			fprintf(stderr, "mutation not caught: %s: %s\n", Where.c_str(), What.c_str());
			Failures++;
		}
	};
	auto ChangeString = [&](toml::node &Node, const std::string &New, const std::string &What) {
		std::string Old = Node.as_string()->get();
		Node.as_string()->get() = New;
		Try(What + " \"" + Old + "\" to \"" + New + "\"");
		Node.as_string()->get() = Old;
	};
	auto ChangeInteger = [&](toml::node &Node, const std::string &What) {
		int64_t Old = Node.as_integer()->get();
		Node.as_integer()->get() = Old + 1;
		Try(What + " one up");
		Node.as_integer()->get() = Old;
	};
	auto LeaveOut = [&](toml::array &Array, size_t Index, const std::string &What) {
		toml::node *Node = Array.get(Index);
		toml::table Saved = *Node->as_table();
		Array.erase(Array.cbegin() + static_cast<std::ptrdiff_t>(Index));
		Try(What + " left out");
		Array.insert(Array.cbegin() + static_cast<std::ptrdiff_t>(Index), std::move(Saved));
	};

	toml::array &Rules = *File.get("rule")->as_array();
	for (size_t r = 0; r < Rules.size(); r++) {
		toml::table &Rule = *Rules.get(r)->as_table();
		Where = "rule " + std::to_string(r + 1);
		ChangeString(*Rule.get("event"), NextKey(Rule.get("event")->as_string()->get(), EventKeys,
				NUMBER_OF_EVENT_TYPES), "event");

		for (const char *ListKey : {"conditions", "actions"}) {
			bool IsCondition = ListKey[0] == 'c';
			toml::array &List = *Rule.get(ListKey)->as_array();
			for (size_t e = 0; e < List.size(); e++) {
				toml::table &Entry = *List.get(e)->as_table();
				std::string Prefix = std::string(IsCondition ? "condition " : "action ") + std::to_string(e + 1);
				std::string Name = Entry.get("name")->as_string()->get();
				std::vector<const char *> Names;
				const TTriggerSignature *Signature = nullptr;
				const TTriggerSignature *List2 = IsCondition ? ConditionSignatures : ActionSignatures;
				int Count = IsCondition ? NUMBER_OF_CONDITION_TYPES : NUMBER_OF_ACTION_TYPES;
				for (int i = 0; i < Count; i++) {
					Names.push_back(List2[i].Key);
					if (Name == List2[i].Key) {
						Signature = &List2[i];
					}
				}
				ChangeString(*Entry.get("name"), NextKey(Name, Names.data(), Count), Prefix + " name");

				if (IsCondition) {
					for (const char *Modifier : {"negate", "ignore_result"}) {
						if (Entry.contains(Modifier)) {
							bool Old = Entry.get(Modifier)->as_boolean()->get();
							Entry.erase(Modifier);
							Try(Prefix + " " + Modifier + " taken away");
							Entry.insert_or_assign(Modifier, Old);
							Entry.get(Modifier)->as_boolean()->get() = !Old;
							Try(Prefix + " " + Modifier + " turned");
							Entry.get(Modifier)->as_boolean()->get() = Old;
						} else {
							Entry.insert_or_assign(Modifier, true);
							Try(Prefix + " " + Modifier + " added");
							Entry.erase(Modifier);
						}
					}
				}

				toml::array *Args = Entry.contains("args") ? Entry.get("args")->as_array() : nullptr;
				for (size_t a = 0; Args != nullptr && a < Args->size(); a++) {
					toml::node &Arg = *Args->get(a);
					std::string What = Prefix + " argument " + std::to_string(a + 1);
					ParameterType Type = Signature->Parameters[a];
					if (Arg.is_integer()) {
						ChangeInteger(Arg, What);
					} else if (Arg.is_array()) {
						for (size_t i = 0; i < 3; i++) {
							ChangeInteger(*Arg.as_array()->get(i), What + " component " + std::to_string(i + 1));
						}
					} else {
						const std::string &Value = Arg.as_string()->get();
						std::string New;
						switch (Type) {
							case PARAMETER_OBJECT:
								New = NextKey(Value, EventObjectKeys, NUMBER_OF_EVENTOBJECTS);
								break;
							case PARAMETER_FLAG:
								New = NextKey(Value, FlagKeys, FLAGS);
								break;
							case PARAMETER_TYPEATTRIBUTE:
								New = NextKey(Value, TypeAttributeKeys, TYPEATTRIBUTES);
								break;
							case PARAMETER_INSTANCEATTRIBUTE:
								New = NextKey(Value, InstanceAttributeKeys, INSTANCEATTRIBUTES);
								break;
							case PARAMETER_SKILL:
								New = NextKey(Value, SkillKeys, NUMBER_OF_SKILL_KEYS);
								break;
							case PARAMETER_RIGHT:
								New = NextKey(Value, RightKeys, 2);
								break;
							case PARAMETER_COMPARISON:
								New = NextKey(Value, ComparisonKeys, 6);
								break;
							default:
								New = Value + "x";
								break;
						}
						ChangeString(Arg, New, What);
					}
				}

				if (List.size() > 1) {
					LeaveOut(List, e, Prefix);
				}
			}
		}

		LeaveOut(Rules, r, "the rule");
		// Putting the rule back made a new node of it.
		toml::table &Back = *Rules.get(r)->as_table();
		for (size_t Next = r + 1; Next < Rules.size(); Next++) {
			toml::table &Other = *Rules.get(Next)->as_table();
			if (Other.get("event")->as_string()->get() != Back.get("event")->as_string()->get()) {
				continue;
			}
			if (*Other.get("conditions")->as_array() != *Back.get("conditions")->as_array()
					|| *Other.get("actions")->as_array() != *Back.get("actions")->as_array()) {
				std::swap(*Rules.get(r)->as_table(), *Rules.get(Next)->as_table());
				Try("swapped with rule " + std::to_string(Next + 1));
				std::swap(*Rules.get(r)->as_table(), *Rules.get(Next)->as_table());
			}
			break;
		}
	}
	return Changes;
}

static int Original(const char *ItemsToml, const char *MoveUseDat, const char *TriggersToml) {
	if (!fs::is_regular_file(MoveUseDat)) {
		printf("skipped: no %s\n", MoveUseDat);
		return 77;
	}

	// The object types, loaded here once for every load after.
	LoadItems(ItemsToml);
	Defined = DefinedTypes(ItemsToml);

	std::string Text = Convert(MoveUseDat);
	std::string Committed = ReadFile(TriggersToml);
	if (Committed != Text) {
		fprintf(stderr, "%s is not what patella-convert triggers makes of %s (%s)\n", TriggersToml, MoveUseDat,
				IsError(Text) ? Text.c_str() : "a different text");
		Failures++;
	}

	std::string Expected = Snapshot(OldLoader(MoveUseDat));
	std::string Actual = Snapshot(NewLoader(TriggersToml));
	if (IsError(Expected) || IsError(Actual)) {
		fprintf(stderr, "a load failed: %s / %s\n", Expected.substr(0, 200).c_str(), Actual.substr(0, 200).c_str());
		return 1;
	}
	if (Same(Expected, Actual, "original", "moveuse.dat", "triggers.toml")) {
		std::map<std::string, std::string> Values = Fields(Actual);
		int Texts = 0;
		for (const auto &Value : Values) {
			Texts += Value.first.find(" text ") != std::string::npos;
		}
		std::string Rules;
		for (int Event = 0; Event < NUMBER_OF_EVENT_TYPES; Event++) {
			Rules += (Event > 0 ? "/" : "") + Values["event " + std::to_string(Event) + " rules"];
		}
		printf("%s and %s give the same tables: %s rules (use/multi_use/movement/collision/separation), "
				"%s conditions, %s actions, %d texts; %d values compared\n", MoveUseDat, TriggersToml,
				Rules.c_str(), Values["conditions"].c_str(), Values["actions"].c_str(), Texts,
				static_cast<int>(Values.size()));
	}

	toml::table File = TomlParseFile(TriggersToml);
	std::string InUse = LoadInProcess(File);
	if (IsError(InUse)) {
		fprintf(stderr, "%s\n", InUse.c_str());
		return 1;
	}
	int Changes = TestMutations(File, InUse);
	CHECK(LoadInProcess(File) == InUse);
	printf("%d changes of single values, names or entries of %s, each caught\n", Changes, TriggersToml);
	return Failures == 0 ? 0 : 1;
}

int main(int argc, char **argv) {
	if (argc == 5 && strcmp(argv[1], "--original") == 0) {
		int Result = 1;
		try {
			Result = Original(argv[2], argv[3], argv[4]);
		} catch (const char *Error) {
			fprintf(stderr, "unexpected error: %s\n", Error);
			Failures++;
		}
		if (Failures != 0) {
			fprintf(stderr, "%d failures\n", Failures);
			Result = 1;
		}
		return Result;
	}
	if (argc != 1) {
		fprintf(stderr, "usage: convert_test_triggers [--original ITEMS_TOML MOVEUSE_DAT TRIGGERS_TOML]\n");
		return 2;
	}

	fs::path Folder = TemporaryFolder("convert-test-triggers");
	try {
		WriteFile(Folder / "items.toml", TestItems);
		LoadItems((Folder / "items.toml").c_str());
		Defined = DefinedTypes((Folder / "items.toml").c_str());
		TestRoundTrips(Folder);
		TestRefusals(Folder);
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
