// test_npcs.cc: the conversion of the NPCs (patella-convert npcs). The NPCs'
// data the converter's reader gets from their .npc files (ReadNpcFile, which
// reads them as the game's TNPC::TNPC and its rule parser did) and the data
// the game's LoadNPCs (npcdata.cc) gets from the npcs.toml patella-convert
// makes of them must be the same, NPC by NPC and in order; the rules the
// written scripts and block modules give through the real Lua parser (the
// engine of scripting.cc, with a lib/npc.lua of the test's own whose
// constructors make a canonical text of each element) must be those the
// reader made, element by element, the blocks' in their places; the handlers
// must hand text and situation on; and the reader must refuse what the game
// refused, and what the conversion refuses besides, with the file and line.
// (That the reader parses as the game did is shown outside this test, by
// comparing it with the game's own parser of main before phase 5.)
//
//   convert_test_npcs
//     With made-up files in a temporary folder.
//   convert_test_npcs --original TAR SCRATCH DATA_FOLDER
//     With the original npc/ folder of TAR (a tar with npc/ at its top, the
//     pristine installation), extracted into SCRATCH (a folder on the file
//     system the game ran on, whose readdir order is the NPCs' order; not
//     /mnt/c): every .npc and .ndb must convert, and the written npcs.toml,
//     scripts/npcs/*.lua and scripts/lib/npcs/*.lua must be exactly the
//     committed ones of DATA_FOLDER, none missing or extra; both round trips
//     on them; and single changes of a written script, block or entry must
//     be caught by them. Exit code 77 (skipped) when TAR is not there.

#include <algorithm>
#include <cctype>
#include <cstdio>
#include <cstring>
#include <set>
#include <string>
#include <vector>

#include <dirent.h>

#include "objects.hh"
#include "map.hh"
#include "info.hh"
#include "cr.hh"
#include "utils.hh"
#include "scripting_api.hh"
#include "npc_files.hh"
#include "npcs_writer.hh"
#include "npcdata.hh"

#include "test_common.hh"

// The test's lib/npc.lua. Each constant and constructor makes a table whose
// text is the element's canonical text (that of LuaElement); rules flattens
// the blocks in place, hands the canonical text of the whole list (that of
// CanonicalRules) to NpcTest.record and returns it with the list; react hands
// its text, situation and the rules' text to NpcTest.react.
static const char *const FIXTURE = R"LUA(-- lib/npc.lua of test_npcs.cc.
local behaviour = {}

local function quote(s)
	local out = {}
	for i = 1, #s do
		local b = s:byte(i)
		if b == 34 then
			out[i] = "\\\""
		elseif b == 92 then
			out[i] = "\\\\"
		elseif b == 10 then
			out[i] = "\\n"
		elseif b >= 32 and b <= 126 then
			out[i] = string.char(b)
		else
			out[i] = string.format("\\%03d", b)
		end
	end
	return "\"" .. table.concat(out) .. "\""
end

local function text(value)
	if type(value) == "number" then
		if value ~= math.floor(value) or value < -2147483648 or value > 2147483647 then
			error("not an int: " .. value)
		end
		return string.format("%d", value)
	elseif type(value) == "string" then
		return quote(value)
	elseif type(value) == "table" and type(value.text) == "string" and not value.isRules then
		return value.text
	end
	error("not an element: " .. type(value))
end

local NAMES = {
	"ADDRESS", "BUSY", "VANISH", "MALE", "FEMALE", "KNIGHT", "PALADIN", "SORCERER", "DRUID", "PREMIUM",
	"PROMOTED", "PZBLOCK", "NONPVP", "PVPENFORCED", "FORCE", "P1", "P2", "P3", "TOPIC", "PRICE", "AMOUNT",
	"TYPE", "DATA", "LEVEL", "MAGLEVEL", "HP", "BURNING", "POISON", "COUNTMONEY", "createMoney",
	"deleteMoney", "queue", "idle", "nop", "previous",
}
local CALLS = {
	"eq", "ne", "lt", "gt", "le", "ge", "param", "count", "spellKnown", "spellLevel", "questValue", "random",
	"add", "sub", "mul", "setTopic", "setPrice", "setAmount", "setType", "setData", "setHp", "burning",
	"poison", "setQuestValue", "effectMe", "effectOpp", "profession", "teachSpell", "summon", "create",
	"delete", "teleport", "startPosition",
}
for _, name in ipairs(NAMES) do
	behaviour[name] = {text = name}
end
for _, name in ipairs(CALLS) do
	behaviour[name] = function(...)
		local parts = {}
		for i = 1, select("#", ...) do
			parts[i] = text((select(i, ...)))
		end
		return {text = name .. "(" .. table.concat(parts, ", ") .. ")"}
	end
end

local function elements(list)
	if type(list) ~= "table" or list.text ~= nil then
		error("not a list of elements")
	end
	local parts = {}
	for i, element in ipairs(list) do
		parts[i] = text(element)
	end
	return table.concat(parts, ", ")
end

function behaviour.rules(list)
	local lines = {}
	for _, element in ipairs(list) do
		if type(element) == "table" and element.isRules then
			for _, line in ipairs(element.lines) do
				lines[#lines + 1] = line
			end
		elseif type(element) == "table" and #element == 2 then
			lines[#lines + 1] = elements(element[1]) .. " -> " .. elements(element[2]) .. "\n"
		else
			error("not a rule")
		end
	end
	local rules = {isRules = true, lines = lines, text = table.concat(lines)}
	NpcTest.record(rules.text)
	return rules
end

function behaviour.react(npc, rules, text, situation)
	NpcTest.react(text, situation, rules.text)
end

function behaviour.canonical(rules)
	return rules.text
end

return behaviour
)LUA";

// What the fixture handed to C++.
static std::string Recorded;
static std::string ReactText;
static int ReactSituation = -1;
static std::string ReactRules;

// NpcTest.record(text): the canonical text of a rule list.
static void TestRecord(std::string Text) {
	Recorded = Text;
}

// NpcTest.react(text, situation, rules): what react got.
static void TestReact(std::string Text, int Situation, std::string Rules) {
	ReactText = Text;
	ReactSituation = Situation;
	ReactRules = Rules;
}

// The engine on the scripts folder Scripts, with the fixture as its
// lib/npc.lua and the test's API.
struct TEngine {
	explicit TEngine(const fs::path &Scripts) {
		WriteFile(Scripts / "lib" / "npc.lua", FIXTURE);
		InitScripting(Scripts.c_str());
		sol::table API = ScriptAPITable("NpcTest");
		API.set_function("record", Guarded(&TestRecord));
		API.set_function("react", Guarded(&TestReact));
		try {
			SetScriptConstant("NPC_SITUATION_VANISH", 4);
		} catch (const char *) {
			// The engine has it already.
		}
	}
	~TEngine() {
		ExitScripting();
	}
};

// The first line in which two texts differ, for the reports.
static std::string FirstDifference(const std::string &Expected, const std::string &Actual) {
	std::istringstream A(Expected), B(Actual);
	std::string LineA, LineB;
	for (int Line = 1;; Line++) {
		bool MoreA = static_cast<bool>(std::getline(A, LineA));
		bool MoreB = static_cast<bool>(std::getline(B, LineB));
		if (!MoreA && !MoreB) {
			return "(none)";
		}
		if (!MoreA || !MoreB || LineA != LineB) {
			return "line " + std::to_string(Line) + ": expected \"" + (MoreA ? LineA : "(end)") + "\", got \""
					+ (MoreB ? LineB : "(end)") + "\"";
		}
	}
}

// The rule round trip: each NPC's script loaded in the engine gives its rules,
// and its handlers hand text and situation to react with them; each block's
// module gives its rules. Scripts is the folder the scripts were written to.
static void CheckRules(const fs::path &Scripts, const TNpcFolder &Folder) {
	TEngine Engine(Scripts);
	for (const TNpcFile &Npc : Folder.Npcs) {
		std::string Name = "npcs/" + Npc.FileName + ".lua";
		std::string Expected = CanonicalRules(Npc.Rules);
		Recorded = "(nothing)";
		int Script = -1;
		try {
			Script = LoadScript(Name.c_str(), nullptr);
			RequireHandler(Script, "onCreatureSay", nullptr);
			RequireHandler(Script, "onCreatureDisappear", nullptr);
		} catch (const char *Error) {
			fprintf(stderr, "%s: %s\n", Name.c_str(), Error);
			Failures++;
			continue;
		}
		if (Recorded != Expected) {
			fprintf(stderr, "%s: the rules differ from the reader's: %s\n", Name.c_str(),
					FirstDifference(Expected, Recorded).c_str());
			Failures++;
		}

		const char *Text = "h\xe9llo \"you\" %N";
		ReactText.clear();
		ReactSituation = -1;
		ReactRules.clear();
		try {
			TScriptCall Say(Script, "onCreatureSay");
			Say.addNumber(1073741825);
			Say.addNil();
			Say.addString(Text);
			Say.addNumber(3);
			Say.run();
		} catch (...) {
			fprintf(stderr, "%s: onCreatureSay failed\n", Name.c_str());
			Failures++;
		}
		if (ReactText != Text || ReactSituation != 3 || ReactRules != Expected) {
			fprintf(stderr, "%s: onCreatureSay did not hand text, situation and rules to react\n", Name.c_str());
			Failures++;
		}
		ReactText = "(nothing)";
		ReactSituation = -1;
		ReactRules.clear();
		try {
			TScriptCall Disappear(Script, "onCreatureDisappear");
			Disappear.addNumber(1073741825);
			Disappear.addNil();
			Disappear.run();
		} catch (...) {
			fprintf(stderr, "%s: onCreatureDisappear failed\n", Name.c_str());
			Failures++;
		}
		if (ReactText != "" || ReactSituation != 4 || ReactRules != Expected) {
			fprintf(stderr, "%s: onCreatureDisappear did not hand \"\" and VANISH to react\n", Name.c_str());
			Failures++;
		}
	}
	for (const TNpcBlock &Block : Folder.Blocks) {
		std::string Name = "blockcheck/" + Block.Name + ".lua";
		WriteFile(Scripts / Name, "local behaviour = require(\"lib.npc\")\n"
				"NpcTest.record(behaviour.canonical(require(\"lib.npcs." + Block.Name + "\")))\n");
		Recorded = "(nothing)";
		try {
			LoadScript(Name.c_str(), nullptr);
		} catch (const char *Error) {
			fprintf(stderr, "%s: %s\n", Name.c_str(), Error);
			Failures++;
			continue;
		}
		std::string Expected = CanonicalRules(Block.Rules);
		if (Recorded != Expected) {
			fprintf(stderr, "lib/npcs/%s.lua: the rules differ from the reader's: %s\n", Block.Name.c_str(),
					FirstDifference(Expected, Recorded).c_str());
			Failures++;
		}
	}
}

// The first difference between the NPCs LoadNPCs gave and those the old
// reader gave, or "" when there is none.
static std::string CompareData(const std::vector<TNPCData> &Loaded, const TNpcFolder &Folder) {
	if (Loaded.size() != Folder.Npcs.size()) {
		return "LoadNPCs gives " + std::to_string(Loaded.size()) + " NPCs, the old reader "
				+ std::to_string(Folder.Npcs.size());
	}
	for (size_t i = 0; i < Loaded.size(); i++) {
		const TNPCData &A = Loaded[i];
		const TNpcFile &B = Folder.Npcs[i];
		std::string What;
		if (A.Name != B.Name) {
			What = "name";
		} else if (A.Sex != B.Sex) {
			What = "sex";
		} else if (A.Race != B.Race) {
			What = "race";
		} else if (memcmp(&A.Outfit, &B.Outfit, sizeof(TOutfit)) != 0) {
			What = "outfit";
		} else if (A.HomeX != B.HomeX || A.HomeY != B.HomeY || A.HomeZ != B.HomeZ) {
			What = "home";
		} else if (A.Radius != B.Radius) {
			What = "radius";
		} else if (A.GoStrength != B.GoStrength) {
			What = "go strength";
		} else if (A.Script != "npcs/" + B.FileName + ".lua") {
			What = "script";
		} else if (A.Where.find(":") == std::string::npos) {
			What = "where";
		}
		if (!What.empty()) {
			return "NPC " + std::to_string(i + 1) + " (" + B.FileName + ".npc): " + What + " differs";
		}
	}
	return "";
}

// The TOML round trip: LoadNPCs of Toml gives the NPCs of the old reader.
static void CheckData(const fs::path &Toml, const TNpcFolder &Folder) {
	std::vector<TNPCData> Loaded;
	try {
		LoadNPCs(Toml.c_str(), Loaded);
	} catch (const char *Error) {
		fprintf(stderr, "LoadNPCs: %s\n", Error);
		Failures++;
		return;
	}
	std::string Difference = CompareData(Loaded, Folder);
	if (!Difference.empty()) {
		fprintf(stderr, "%s\n", Difference.c_str());
		Failures++;
	}
}

// The names readdir gives for Folder's .npc files, without ".npc".
static std::vector<std::string> ReaddirOrder(const fs::path &Folder) {
	std::vector<std::string> Names;
	DIR *Dir = opendir(Folder.c_str());
	while (Dir != nullptr) {
		struct dirent *Entry = readdir(Dir);
		if (Entry == nullptr) {
			break;
		}
		std::string Name = Entry->d_name;
		if (Name.size() > 4 && Name.compare(Name.size() - 4, 4, ".npc") == 0) {
			Names.push_back(Name.substr(0, Name.size() - 4));
		}
	}
	if (Dir != nullptr) {
		closedir(Dir);
	}
	return Names;
}

static void CheckOrder(const fs::path &NpcFolder, const TNpcFolder &Folder) {
	std::vector<std::string> Names;
	for (const TNpcFile &Npc : Folder.Npcs) {
		Names.push_back(Npc.FileName);
	}
	if (Names != ReaddirOrder(NpcFolder)) {
		fprintf(stderr, "the NPCs are not in the order readdir gives their files\n");
		Failures++;
	}
}

// The standard header keys of a made-up NPC.
static std::string Header(const std::string &Name = "Tester") {
	return "Name = \"" + Name + "\"\nSex = male\nRace = 1\nOutfit = (128,0-0-0-0)\nHome = [100,100,7]\nRadius = 1\n"
			"GoStrength = 10\n";
}

static const char *const BLOCK_A =
	"# A block\n"
	"# with two lines\n"
	"\n"
	"\"block\" -> \"From the block.\"   # trailing\n"
	"# between\n"
	"\n"
	"\"blockword\",%1,2<%1 -> Amount=%1, \"Two: %A\", *\n";

static const char *const BLOCK_A_LUA =
	"-- lib/npcs/a-block.lua: the rule block a-block.ndb, converted by patella-convert.\n"
	"-- An NPC that included it (@\"a-block.ndb\") has it in its rules where the include\n"
	"-- was; lib/npc.lua matches and runs them (scripting.hh, \"NPCs\"). The comments\n"
	"-- are those of a-block.ndb.\n"
	"--\n"
	"-- A block\n"
	"-- with two lines\n"
	"\n"
	"local behaviour = require(\"lib.npc\")\n"
	"local P1 = behaviour.P1\n"
	"local lt = behaviour.lt\n"
	"local setAmount = behaviour.setAmount\n"
	"local previous = behaviour.previous\n"
	"\n"
	"return behaviour.rules {\n"
	"\t{{\"block\"}, {\"From the block.\"}}, -- trailing\n"
	"\t-- between\n"
	"\n"
	"\t{{\"blockword\", P1, lt(2, P1)}, {setAmount(P1), \"Two: %A\", previous}},\n"
	"}\n";

// A block whose first rule names the rule before it; no NPC includes it.
static const char *const BLOCK_B = "\"x\" -> *\n";

static const char *const BLOCK_C = "\"two-a\" -> \"A\"\n\"two-b\" -> \"B\"\n";

// CRLF, no newline at the end, comments in every place.
static const char *const NPC_BETA =
	"# Beta\r\n"
	"\r\n"
	"Name = \"Beta\"\r\n"
	"Sex = male\r\n"
	"Race = 2\r\n"
	"Outfit = (128,1-2-3-4)\r\n"
	"Home = [1,2,3]\r\n"
	"Radius = 5\r\n"
	"GoStrength = 20\r\n"
	"Behaviour = {\r\n"
	"@\"a-block.ndb\"   # the block\r\n"
	"\r\n"
	"\r\n"
	"\"two\",          # inside\r\n"
	" \"words\" -> \"Hi\", *\r\n"
	"# last\r\n"
	"}\r\n"
	"# after\r\n"
	"# end";

static const char *const NPC_BETA_LUA =
	"-- npcs/beta.lua: the behaviour of the NPC Beta, converted from\n"
	"-- beta.npc by patella-convert. The NPC is its [[npc]] entry in npcs.toml;\n"
	"-- lib/npc.lua matches and runs the rules below, in the order of the file\n"
	"-- (scripting.hh, \"NPCs\"). The comments are those of beta.npc.\n"
	"--\n"
	"-- Beta\n"
	"\n"
	"local behaviour = require(\"lib.npc\")\n"
	"local aBlock = require(\"lib.npcs.a-block\")\n"
	"local previous = behaviour.previous\n"
	"\n"
	"local rules = behaviour.rules {\n"
	"\taBlock, -- the block\n"
	"\n"
	"\t-- inside\n"
	"\t{{\"two\", \"words\"}, {\"Hi\", previous}},\n"
	"\t-- last\n"
	"}\n"
	"-- after\n"
	"-- end\n"
	"\n"
	"function onCreatureSay(npc, creature, text, situation)\n"
	"\tbehaviour.react(npc, rules, text, situation)\n"
	"end\n"
	"\n"
	"function onCreatureDisappear(npc, creature)\n"
	"\tbehaviour.react(npc, rules, \"\", NPC_SITUATION_VANISH)\n"
	"end\n";

// Every kind of condition, value and action; Latin-1 in name, texts and
// comments; numbers that wrap; escapes; a block included twice.
static const char *const NPC_ALPHA =
	"# Alpha: f\xfcr den Test\n"
	"\n"
	"Name = \"Alpha \xe9\"\n"
	"Sex = female\n"
	"Race = 33\n"
	"Outfit = (0,3058)\n"
	"Home = [65535,0,15]\n"
	"Radius = 0\n"
	"GoStrength = 65535\n"
	"# between the keys\n"
	"\n"
	"Behaviour = {   # after the brace\n"
	"Address,\"hello$\",! -> \"Hello, %N!\"    # a comment, \xe4\n"
	"BUSY,! -> NOP\n"
	"VANISH,! -> Idle\n"
	"\n"
	"\n"
	"@\"a-block.ndb\"\n"
	"\"all\",male,female,knight,paladin,sorcerer,druid,premium,promoted,pzblock,nonpvp,pvpenforced -> Queue\n"
	"%1,%2,%3,Topic<>1,Price<2,Amount>3,Type<=4,Data>=5,Level=6 -> \"x\"\n"
	"MagLevel+HP-Burning*Poison=CountMoney -> \"y\"\n"
	"Count(Type)>Amount, SpellKnown(5)=1, SpellLevel(6)<Level, QuestValue(7)<>Random(1,10) -> \"z\"\n"
	"-> Topic=1, Price=2, Amount=3, Type=4, Data=5, HP=6\n"
	"\"timers\" -> Burning(1,2), Poison(3,4), SetQuestValue(5,6), EffectMe(1), EffectOpp(2), Profession(3),\n"
	"   TeachSpell(4), Summon(5), Create(6), Delete(7)\n"
	"\"funcs\" -> CreateMoney, DeleteMoney, Teleport(1,2,3), StartPosition(4,5,6), *\n"
	"\"wrap\",Topic=4294967295,2147483648>Price -> Amount=%5+%-1*2147483648 - 1, "
	"\"esc \\\"q\\\" \\\\ \\n end \xe9\x01\"\n"
	"\"order\",1+2*3 - 4*5=Topic-1 - 2 -> NOP, \"after nop\"\n"
	"@\"c-two.ndb\"\n"
	"@\"c-two.ndb\"\n"
	"}\n";

static void TestConversion(const fs::path &Folder) {
	fs::path Npc = Folder / "npc";
	WriteFile(Npc / "a-block.ndb", BLOCK_A);
	WriteFile(Npc / "b-unused.ndb", BLOCK_B);
	WriteFile(Npc / "c-two.ndb", BLOCK_C);
	WriteFile(Npc / "alpha.npc", NPC_ALPHA);
	WriteFile(Npc / "beta.npc", NPC_BETA);
	WriteFile(Npc / "notes.txt", "not an NPC\n");
	WriteFile(Npc / ".npc", "not read: the name is 4 characters\n");

	TNpcFolder Data;
	try {
		ReadNpcFolder(Npc.c_str(), Data);
	} catch (const char *Error) {
		fprintf(stderr, "made-up folder: %s\n", Error);
		Failures++;
		return;
	}
	fs::path Out = Folder / "data";
	WriteNpcFolder(Data, Out.c_str());

	CHECK(Data.Npcs.size() == 2);
	CHECK(Data.Blocks.size() == 3);
	CheckOrder(Npc, Data);
	CHECK(ReadFile(Out / "scripts" / "npcs" / "beta.lua") == NPC_BETA_LUA);
	CHECK(ReadFile(Out / "scripts" / "lib" / "npcs" / "a-block.lua") == BLOCK_A_LUA);
	CHECK(ReadFile(Out / "scripts" / "lib" / "npcs" / "b-unused.lua").find("\t{{\"x\"}, {previous}},\n")
			!= std::string::npos);

	const TNpcFile *Alpha = nullptr;
	for (const TNpcFile &File : Data.Npcs) {
		if (File.FileName == "alpha") {
			Alpha = &File;
		}
	}
	CHECK(Alpha != nullptr);
	if (Alpha != nullptr) {
		CHECK(Alpha->Name == "Alpha \xe9");
		CHECK(Alpha->Sex == SEX_FEMALE);
		CHECK(Alpha->Outfit.OutfitID == 0 && Alpha->Outfit.ObjectType == 3058);
		CHECK(Alpha->Rules.size() == 18);
		CHECK(Alpha->OwnRules == 12);
		CHECK((Alpha->Blocks == std::vector<std::string>{"a-block", "c-two"}));
		std::string Lua = ReadFile(Out / "scripts" / "npcs" / "alpha.lua");
		static const char *const LINES[] = {
			"-- npcs/alpha.lua: the behaviour of the NPC Alpha \xc3\xa9, converted from\n",
			"--\n-- Alpha: f\xc3\xbcr den Test\n-- between the keys\n\nlocal behaviour",
			"local aBlock = require(\"lib.npcs.a-block\")\nlocal cTwo = require(\"lib.npcs.c-two\")\n"
					"local ADDRESS = behaviour.ADDRESS\n",
			"local rules = behaviour.rules {\n\t-- after the brace\n"
					"\t{{ADDRESS, \"hello$\", FORCE}, {\"Hello, %N!\"}}, -- a comment, \xc3\xa4\n",
			"\t{{BUSY, FORCE}, {nop}},\n\t{{VANISH, FORCE}, {idle}},\n\n\taBlock,\n",
			"\t{{\"all\", MALE, FEMALE, KNIGHT, PALADIN, SORCERER, DRUID, PREMIUM, PROMOTED, PZBLOCK, NONPVP, "
					"PVPENFORCED}, {queue}},\n",
			"\t{{P1, P2, P3, ne(TOPIC, 1), lt(PRICE, 2), gt(AMOUNT, 3), le(TYPE, 4), ge(DATA, 5), eq(LEVEL, 6)}, "
					"{\"x\"}},\n",
			"\t{{eq(sub(add(MAGLEVEL, HP), mul(BURNING, POISON)), COUNTMONEY)}, {\"y\"}},\n",
			"\t{{gt(count(TYPE), AMOUNT), eq(spellKnown(5), 1), lt(spellLevel(6), LEVEL), "
					"ne(questValue(7), random(1, 10))}, {\"z\"}},\n",
			"\t{{}, {setTopic(1), setPrice(2), setAmount(3), setType(4), setData(5), setHp(6)}},\n",
			"\t{{\"timers\"}, {burning(1, 2), poison(3, 4), setQuestValue(5, 6), effectMe(1), effectOpp(2), "
					"profession(3), teachSpell(4), summon(5), create(6), delete(7)}},\n",
			"\t{{\"funcs\"}, {createMoney, deleteMoney, teleport(1, 2, 3), startPosition(4, 5, 6), previous}},\n",
			"\t{{\"wrap\", eq(TOPIC, -1), gt(-2147483648, PRICE)}, {setAmount(sub(add(param(5), mul(param(-1), "
					"-2147483648)), 1)), \"esc \\\"q\\\" \\\\ \\n end \\233\\001\"}},\n",
			"\t{{\"order\", eq(sub(add(1, mul(2, 3)), mul(4, 5)), sub(sub(TOPIC, 1), 2))}, {nop, \"after nop\"}},\n",
			"\tcTwo,\n\tcTwo,\n}\n\nfunction onCreatureSay",
		};
		for (const char *Line : LINES) {
			if (Lua.find(Line) == std::string::npos) {
				fprintf(stderr, "alpha.lua lacks:\n%s\nin:\n%s", Line, Lua.c_str());
				Failures++;
			}
		}
		std::string Locals = "local ADDRESS = behaviour.ADDRESS\nlocal BUSY = behaviour.BUSY\n"
				"local VANISH = behaviour.VANISH\nlocal MALE = behaviour.MALE\n";
		CHECK(Lua.find(Locals) != std::string::npos);
		CHECK(Lua.find("local previous = behaviour.previous\n\nlocal rules") != std::string::npos);
	}

	std::string Toml = ReadFile(Out / "npcs.toml");
	CHECK(Toml.find("\n[[npc]]\nname = \"Alpha \xc3\xa9\"\nsex = \"female\"\nrace = 33\n"
			"outfit = { id = 0, object_type = 3058 }\nhome = [65535, 0, 15]\nradius = 0\ngo_strength = 65535\n"
			"script = \"npcs/alpha.lua\"\n") != std::string::npos);
	CHECK(Toml.find("\n[[npc]]\nname = \"Beta\"\nsex = \"male\"\nrace = 2\n"
			"outfit = { id = 128, colors = [1, 2, 3, 4] }\nhome = [1, 2, 3]\nradius = 5\ngo_strength = 20\n"
			"script = \"npcs/beta.lua\"\n") != std::string::npos);
	CHECK(Toml.compare(0, 2, "# ") == 0 && Toml.find("runs.\n\n[[npc]]\n") != std::string::npos);

	CheckData(Out / "npcs.toml", Data);
	CheckRules(Out / "scripts", Data);
}

// What the reader refuses: the game's errors and the conversion's own, each
// with the file and, where it has one, the line.
static void TestRefusals(const fs::path &Folder) {
	int Case = 0;
	// Writes Files (name, text pairs; the first is the .npc read) into a
	// folder of their own and checks the error ReadNpcFile gives: "<path>: "
	// and What in it.
	auto Refused = [&](const std::vector<std::pair<std::string, std::string>> &Files, const std::string &What) {
		fs::path Place = Folder / "refusals" / std::to_string(++Case);
		for (const auto &File : Files) {
			WriteFile(Place / File.first, File.second);
		}
		fs::path Path = Place / Files[0].first;
		std::string Error = "(no error)";
		try {
			if (Path.extension() == ".ndb") {
				ReadNpcBlock(Path.string());
			} else {
				ReadNpcFile(Path.string());
			}
		} catch (const char *Text) {
			Error = Text;
		}
		std::string Prefix = Path.string() + ": ";
		bool Ok = What == "(no error)" ? Error == What
				: Error.compare(0, Prefix.size(), Prefix) == 0 && Error.find(What) != std::string::npos;
		if (!Ok) {
			fprintf(stderr, "refusal %d: expected \"%s...%s\", got \"%s\"\n", Case, Prefix.c_str(), What.c_str(),
					Error.c_str());
			Failures++;
		}
	};
	auto Npc = [&](const std::string &Behaviour) {
		return Header() + "Behaviour = {\n" + Behaviour + "}\n";
	};
	auto One = [&](const std::string &Text, const std::string &What) {
		Refused({{"x.npc", Text}}, What);
	};

	// The game's errors.
	One(Npc("\"x\" -> Foo\n"), "error in script-file \"x.npc\", line 9: unknown identifier");
	One(Npc("\"x\" ->\n"), "error in script-file \"x.npc\", line 10: illegal action");
	One(Npc("\"x\" -> \"y\",\n}\n"), "illegal action");
	One(Npc("%4 -> \"x\"\n"), "line 9: illegal ordinal number");
	One(Npc("%0 -> \"x\"\n"), "illegal ordinal number");
	One(Npc("%1>2 -> \"x\"\n"), "line 9: '->' expected");
	One(Npc("-> *\n"), "line 9: no previous pattern");
	One(Npc("\"x\" \"y\" -> \"z\"\n"), "'->' expected");
	One(Npc("Topic -> \"x\"\n"), "relational operator expected");
	One(Npc("Topic, \"y\" -> \"x\"\n"), "relational operator expected");
	One(Npc("Foo=1 -> \"x\"\n"), "unknown value");
	One(Npc("-5<Topic -> \"x\"\n"), "illegal character");
	One(Npc("Topic=5-3 -> \"x\"\n"), "illegal value");
	One(Npc("Topic=[1,2,3] -> \"x\"\n"), "illegal value");
	One(Npc("\"x\" -> Topic 1\n"), "special-char expected");
	One(Npc("\"x\" -> EffectMe(1\n"), ") expected");
	One(Npc("\"x\" -> Burning(1 2)\n"), ", expected");
	One(Npc("\"x\" -> Teleport(1,2)\n"), ", expected");
	One(Npc("Count(1 > 2 -> \"x\"\n"), ") expected");
	One(Npc("\"x\" -> \"unterminated\n"), "unexpected end of file");
	One(Header() + "Behaviour = \n", "special-char expected");
	One("Name = \"X\"\nSex = neutral\n", "line 2: unknown constant");
	One("Name = \"X\"\nRadius 1\n", "line 2: special-char expected");
	One("5 = 1\n", "line 1: identifier expected");
	One("Name = \"X\"\nGoStrength = 10\nRace = 1\n", "gostrength before race in npc-script-file");
	One(Header() + "Behaviour = {\n}\nBehaviour = {\n}\n", "behaviour database specified twice for NPC");
	One("Sex = male\nRace = 1\nOutfit = (1,0-0-0-0)\nHome = [1,1,7]\nRadius = 1\nGoStrength = 1\nBehaviour = {\n}\n",
			"no name specified for NPC");
	One("Name = \"\"\nSex = male\nRace = 1\nOutfit = (1,0-0-0-0)\nHome = [1,1,7]\nRadius = 1\nGoStrength = 1\n"
			"Behaviour = {\n}\n", "no name specified for NPC");
	One("Name = \"X\"\nSex = male\nRace = 1\nOutfit = (1,0-0-0-0)\nRadius = 1\nGoStrength = 1\nBehaviour = {\n}\n",
			"no startpoint specified for NPC");
	One("Name = \"X\"\nSex = male\nRace = 1\nOutfit = (1,0-0-0-0)\nHome = [1,1,255]\nRadius = 1\nGoStrength = 1\n"
			"Behaviour = {\n}\n", "no startpoint specified for NPC");
	One(Header(), "no behaviour database specified for NPC");
	Refused({{"x.npc", Npc("@\"missing.ndb\"\n")}}, "Cannot open script-file");
	Refused({{"x.npc", Npc("@\"first.ndb\"\n")}, {"first.ndb", "\"y\" -> *\n-> *\n"}},
			"error in script-file \"first.ndb\", line 1: no previous pattern");
	Refused({{"x.npc", Npc("@\"bad.ndb\"\n")}, {"bad.ndb", "\n\"y\" -> Foo\n"}},
			"error in script-file \"bad.ndb\", line 2: unknown identifier");

	// The conversion's own.
	Refused({{"x.npc", Npc("\"x\",\n@\"cond.ndb\" -> \"y\"\n")}, {"cond.ndb", "\"z\"\n"}},
			"line 10: an include that is not at a rule boundary");
	Refused({{"x.npc", Npc("\"x\" -> \"y\",\n@\"act.ndb\"\n")}, {"act.ndb", "\"z\"\n\"w\" -> \"v\"\n"}},
			"line 10: an include that is not at a rule boundary");
	Refused({{"x.npc", Npc("@\"tail.ndb\" \"z\"\n")}, {"tail.ndb", "\"w\" -> \"v\",\n"}},
			"line 9: an include that is not at a rule boundary");
	Refused({{"x.npc", "@\"keys.ndb\"\n" + Header() + "Behaviour = {\n}\n"}, {"keys.ndb", ""}},
			"line 1: an include outside the behaviour block");
	Refused({{"x.npc", Header() + "Behaviour = {\n}\n@\"keys.ndb\"\n"}, {"keys.ndb", "# nothing\n"}},
			"line 10: an include outside the behaviour block");
	Refused({{"x.npc", Header() + "Behaviour = @\"open.ndb\"\n}\n"}, {"open.ndb", "{\n"}},
			"line 8: an include outside the behaviour block");
	Refused({{"x.npc", Npc("@\"outer.ndb\"\n")}, {"outer.ndb", "\"a\" -> \"b\"\n@\"inner.ndb\"\n"},
			{"inner.ndb", "\"c\" -> \"d\"\n"}},
			"error in script-file \"outer.ndb\", line 2: an include inside an .ndb");
	Refused({{"outer.ndb", "@\"inner.ndb\"\n"}, {"inner.ndb", "\"c\" -> \"d\"\n"}},
			"error in script-file \"outer.ndb\", line 1: an include inside an .ndb");
	Refused({{"x.npc", Npc("@\"sub/y.ndb\"\n")}, {"sub/y.ndb", ""}},
			"line 9: an include of \"sub/y.ndb\", not a plain");
	Refused({{"x.npc", Npc("@\"y.txt\"\n")}, {"y.txt", ""}}, "an include of \"y.txt\", not a plain");
	Refused({{"x.npc", Npc("@\"Y.ndb\"\n")}, {"Y.ndb", ""}}, "an include of \"Y.ndb\", not a plain");
	Refused({{"x.npc", Npc("@\"y_z.ndb\"\n")}, {"y_z.ndb", ""}}, "an include of \"y_z.ndb\", not a plain");
	One(Npc(std::string("\"a\0b\" -> \"c\"\n", 13)), "line 9: a string with a zero byte");
	One(Npc("\"x\" -> \"\"\n"), "line 9: an empty text action");
	One(Npc("\"\" -> \"x\"\n"), "line 9: an empty word");
	One(Npc("\"x\" -> \"y\"\n# a\rb\n"), "line 10: a comment with a CR inside");
	One("#[[ long\n" + Npc(""), "line 1: a comment that would start a Lua long comment");
	One(Npc("#[==[ long\n"), "line 9: a comment that would start a Lua long comment");
	One("Name = \"X\"\nFoo = 1\n", "line 2: a key \"foo\" the game did not know");
	One(Header() + "Radius = 2\nBehaviour = {\n}\n", "line 8: \"radius\" given twice");
	One(Header() + "Race = 2\nBehaviour = {\n}\n", "line 8: race after gostrength or given twice");
	One("Name = \"X\"\nRace = 1\nRace = 2\n", "line 3: race after gostrength or given twice");
	One("Name = \"X\"\nRace = 0\n", "line 2: a race outside 1 to 511");
	One("Name = \"X\"\nRace = 512\n", "a race outside 1 to 511");
	One("Name = \"X\"\nRace = -1\n", "a race outside 1 to 511");
	One("Name = \"X\"\nRadius = 65536\n", "line 2: a radius outside 0 to 65535");
	One("Name = \"X\"\nRadius = -1\n", "a radius outside 0 to 65535");
	One("Name = \"X\"\nRace = 1\nGoStrength = 65536\n", "line 3: a gostrength outside 0 to 65535");
	One("Outfit = (65536,0-0-0-0)\n", "line 1: an outfit id outside 0 to 65535");
	One("Outfit = (-1,0)\n", "an outfit id outside 0 to 65535");
	One("Outfit = (0,65536)\n", "an object type outside 0 to 65535");
	One("Outfit = (1,0-0-0)\n", "colors that are not four numbers of 0 to 255");
	One("Outfit = (1,0-0-0-0-0)\n", "colors that are not four numbers of 0 to 255");
	One("Outfit = (1,0-0-256-0)\n", "colors that are not four numbers of 0 to 255");
	One("Name = \"1234567890123456789012345678901\"\n", "line 1: a name longer than 30 bytes");
	One("Name = \"a\\nb\"\n", "line 1: a name with a line break");
	One(Header() + "Behaviour = {\n}\n", "(no error)");
	std::string Missing = "Name = \"X\"\nSex = male\nRace = 1\nOutfit = (1,0-0-0-0)\nHome = [1,1,7]\nRadius = 1\n"
			"GoStrength = 1\nBehaviour = {\n}\n";
	static const struct {
		const char *Line;
		const char *What;
	} KEYS[] = {
		{"Sex = male\n", "no sex specified for NPC"},
		{"Race = 1\n", "no race specified for NPC"},
		{"Outfit = (1,0-0-0-0)\n", "no outfit specified for NPC"},
		{"Radius = 1\n", "no radius specified for NPC"},
		{"GoStrength = 1\n", "no gostrength specified for NPC"},
	};
	for (const auto &Key : KEYS) {
		std::string Text = Missing;
		Text.erase(Text.find(Key.Line), strlen(Key.Line));
		if (std::string(Key.Line) == "Race = 1\n") {
			Text.erase(Text.find("GoStrength = 1\n"), strlen("GoStrength = 1\n"));
		}
		One(Text, Key.What);
	}
	One("Home = [65536,1,7]\n" + Header().substr(0, Header().find("Home")) + "Radius = 1\nGoStrength = 1\n"
			"Behaviour = {\n}\n", "line 1: a home outside x and y 0 to 65535 and z 0 to 15");
	One(Header().substr(0, Header().find("Home")) + "Home = [1,1,16]\nRadius = 1\nGoStrength = 1\nBehaviour = {\n}\n",
			"a home outside");
	Refused({{"y.ndb", "\"a\" -> \"b\"\n}\n"}}, "line 2: a closing brace where a rule begins");
	Refused({{"Bad.npc", Npc("")}}, "an .npc file whose name cannot name its script");
	Refused({{"bad_name.ndb", ""}}, "an .ndb file whose name cannot name its module");
	Refused({{"set-topic.ndb", ""}}, "an .ndb file whose name cannot name its module");
	Refused({{"end.ndb", ""}}, "an .ndb file whose name cannot name its module");

	// Two blocks of one local name in a folder.
	fs::path Twins = Folder / "refusals" / "twins";
	WriteFile(Twins / "ab-2.ndb", "");
	WriteFile(Twins / "ab2.ndb", "");
	std::string Error;
	try {
		TNpcFolder Data;
		ReadNpcFolder(Twins.c_str(), Data);
	} catch (const char *Text) {
		Error = Text;
	}
	CHECK(Error.find("ab2.ndb: an .ndb file whose local name another block of the folder has too")
			!= std::string::npos);

	// What is accepted: identifiers in any case, a comment "#[x", an include
	// as the only rule, a block of no rules, a '*' in a block's first rule
	// after a rule of the NPC.
	One(Npc("aDDress,! -> iDLE\n#[x\n"), "(no error)");
	Refused({{"x.npc", Npc("\"a\" -> \"b\"\n@\"star.ndb\"\n@\"empty.ndb\"\n")}, {"star.ndb", "-> *\n"},
			{"empty.ndb", "# only a comment\n"}}, "(no error)");
}

// One change of each kind to each written script (a byte of a word or text,
// a number, an operator, a rule left out, two rules swapped, an include
// moved) and to each block (the same but the include) must give other rules,
// or fail to load. Returns the number of changes made.
static int ScriptMutations(const fs::path &Scripts, const TNpcFolder &Folder) {
	TEngine Engine(Scripts);
	int Changes = 0;
	int Number = 0;
	// Loads the script Name (written already) and checks that it does not
	// record Expected.
	auto Try = [&](const std::string &Name, const std::string &Expected, const std::string &What) {
		Changes++;
		Recorded = "(nothing)";
		bool Loaded = true;
		try {
			LoadScript(Name.c_str(), nullptr);
		} catch (const char *) {
			Loaded = false;
		}
		if (Loaded && Recorded == Expected) {
			fprintf(stderr, "change not caught: %s\n", What.c_str());
			Failures++;
		}
	};
	// A changed NPC script: a script of its own name.
	auto TryScript = [&](const std::string &Text, const std::string &Expected, const std::string &What) {
		std::string Name = "mutations/m" + std::to_string(++Number) + ".lua";
		WriteFile(Scripts / Name, Text);
		Try(Name, Expected, What);
	};
	// A changed block: a module of its own name, loaded through a script
	// that records what it returns.
	auto TryBlock = [&](const std::string &Text, const std::string &Expected, const std::string &What) {
		std::string Module = "mut-" + std::to_string(++Number);
		WriteFile(Scripts / "lib" / "npcs" / (Module + ".lua"), Text);
		std::string Name = "blockcheck/" + Module + ".lua";
		WriteFile(Scripts / Name, "local behaviour = require(\"lib.npc\")\n"
				"NpcTest.record(behaviour.canonical(require(\"lib.npcs." + Module + "\")))\n");
		Try(Name, Expected, What);
	};

	auto Lines = [](const std::string &Text) {
		std::vector<std::string> Result;
		size_t Start = 0;
		while (Start < Text.size()) {
			size_t End = Text.find('\n', Start);
			Result.push_back(Text.substr(Start, End - Start + 1));
			Start = End + 1;
		}
		return Result;
	};
	auto Join = [](const std::vector<std::string> &List) {
		std::string Text;
		for (const std::string &Line : List) {
			Text += Line;
		}
		return Text;
	};

	// The changes of one script or block: Text, which gives Expected; Apply
	// loads a changed text.
	using TApply = std::function<void(const std::string &, const std::string &, const std::string &)>;
	auto Mutate = [&](const std::string &Text, const std::string &Expected, const std::string &Where,
			const TApply &Apply) {
		std::vector<std::string> List = Lines(Text);
		std::vector<size_t> RuleLines, BlockLines;
		bool InList = false;
		for (size_t i = 0; i < List.size(); i++) {
			if (List[i].compare(0, 3, "\t{{") == 0) {
				RuleLines.push_back(i);
			} else if (InList && List[i].size() > 2 && List[i][0] == '\t' && List[i][1] != '-') {
				BlockLines.push_back(i);
			}
			if (List[i] == "local rules = behaviour.rules {\n") {
				InList = true;
			} else if (List[i] == "}\n") {
				InList = false;
			}
		}
		if (RuleLines.empty()) {
			return;
		}
		auto Change = [&](const std::vector<std::string> &Changed, const std::string &What) {
			Apply(Join(Changed), Expected, Where + ": " + What);
		};
		// A byte of a word or text: the first letter inside quotes.
		for (size_t r : RuleLines) {
			const std::string &Line = List[r];
			size_t Quote = Line.find('"');
			if (Quote != std::string::npos && Quote + 1 < Line.size()
					&& isalpha(static_cast<unsigned char>(Line[Quote + 1]))) {
				std::vector<std::string> Changed = List;
				Changed[r][Quote + 1] = Line[Quote + 1] == 'x' ? 'y' : 'x';
				Change(Changed, "a byte of a string in line " + std::to_string(r + 1));
				break;
			}
		}
		// A number: the first digit after "(" or " " before any string.
		for (size_t r : RuleLines) {
			const std::string &Line = List[r];
			size_t At = std::string::npos;
			for (size_t i = 1; i < Line.size() && At == std::string::npos && Line[i] != '"'; i++) {
				if (isdigit(static_cast<unsigned char>(Line[i])) && (Line[i - 1] == '(' || Line[i - 1] == ' ')) {
					At = i;
				}
			}
			if (At != std::string::npos) {
				std::vector<std::string> Changed = List;
				Changed[r][At] = Line[At] == '9' ? '8' : static_cast<char>(Line[At] + 1);
				Change(Changed, "a number in line " + std::to_string(r + 1));
				break;
			}
		}
		// An operator before any string.
		static const char *const OPERATORS[][2] = {
			{"eq(", "ne("}, {"ne(", "eq("}, {"lt(", "gt("}, {"gt(", "lt("}, {"le(", "ge("}, {"ge(", "le("},
			{"add(", "sub("}, {"sub(", "add("}, {"mul(", "add("},
		};
		bool Done = false;
		for (size_t r : RuleLines) {
			for (const auto &Operator : OPERATORS) {
				size_t At = List[r].find(Operator[0]);
				if (!Done && At != std::string::npos && List[r].find('"') > At) {
					std::vector<std::string> Changed = List;
					Changed[r].replace(At, strlen(Operator[0]), Operator[1]);
					Change(Changed, "an operator in line " + std::to_string(r + 1));
					Done = true;
				}
			}
		}
		// A rule left out: the middle one.
		{
			std::vector<std::string> Changed = List;
			size_t r = RuleLines[RuleLines.size() / 2];
			Changed.erase(Changed.begin() + static_cast<long>(r));
			Change(Changed, "line " + std::to_string(r + 1) + " left out");
		}
		// Two rules swapped: the first two neighbours that differ.
		for (size_t i = 0; i + 1 < RuleLines.size(); i++) {
			size_t a = RuleLines[i], b = RuleLines[i + 1];
			std::string A = List[a].substr(0, List[a].find("}},") + 3);
			std::string B = List[b].substr(0, List[b].find("}},") + 3);
			if (A != B) {
				std::vector<std::string> Changed = List;
				std::swap(Changed[a], Changed[b]);
				Change(Changed, "lines " + std::to_string(a + 1) + " and " + std::to_string(b + 1) + " swapped");
				break;
			}
		}
		// An include moved above the rule before it.
		for (size_t Block : BlockLines) {
			size_t Before = Block;
			while (Before > 0 && List[Before - 1].compare(0, 3, "\t{{") != 0
					&& List[Before - 1].find('{') == std::string::npos) {
				Before--;
			}
			if (Before > 0 && List[Before - 1].compare(0, 3, "\t{{") == 0) {
				std::vector<std::string> Changed = List;
				std::swap(Changed[Before - 1], Changed[Block]);
				Change(Changed, "the include in line " + std::to_string(Block + 1) + " moved up");
				break;
			}
		}
	};

	for (const TNpcFile &Npc : Folder.Npcs) {
		Mutate(NpcScript(Npc), CanonicalRules(Npc.Rules), "npcs/" + Npc.FileName + ".lua", TryScript);
	}
	for (const TNpcBlock &Block : Folder.Blocks) {
		Mutate(BlockModule(Block), CanonicalRules(Block.Rules), "lib/npcs/" + Block.Name + ".lua", TryBlock);
	}
	return Changes;
}

// Every single change of an entry of npcs.toml (its race, radius, go
// strength, a coordinate of home, its outfit, a byte of its name, its sex)
// must give other NPCs through LoadNPCs, or an error. Returns the number of
// changes made.
static int EntryMutations(const fs::path &Folder, const std::string &Toml, const TNpcFolder &Data) {
	int Changes = 0;
	fs::path File = Folder / "mutated.toml";
	size_t At = Toml.find("\n[[npc]]\n");
	for (size_t Index = 0; At != std::string::npos; Index++) {
		size_t Start = At + 1;
		size_t End = Toml.find("\n\n", Start);
		End = End == std::string::npos ? Toml.size() : End + 1;
		std::string Entry = Toml.substr(Start, End - Start);
		std::string Changed = Entry;
		std::string What;
		auto Bump = [&](const char *Key) {
			size_t Line = Changed.find(std::string("\n") + Key + " = ");
			size_t Value = Line + strlen(Key) + 4;
			size_t Digit = Changed.find_first_of("0123456789", Value);
			Changed[Digit] = Changed[Digit] == '1' ? '2' : '1';
			What = Key;
		};
		switch (Index % 8) {
			case 0: Bump("race"); break;
			case 1: Bump("radius"); break;
			case 2: Bump("go_strength"); break;
			case 3: Bump("home"); break;
			case 4: Bump("outfit"); break;
			case 5: {
				size_t Quote = Changed.find("name = \"") + 8;
				Changed[Quote] = Changed[Quote] == 'X' ? 'Y' : 'X';
				What = "name";
				break;
			}
			case 6: {
				size_t Sex = Changed.find("sex = \"");
				Changed.replace(Sex, Changed.find('\n', Sex) - Sex,
						Changed.compare(Sex, 11, "sex = \"male") == 0 ? "sex = \"female\"" : "sex = \"male\"");
				What = "sex";
				break;
			}
			case 7: {
				size_t Outfit = Changed.find("outfit = { id = ");
				size_t Last = Changed.find_last_of("0123456789", Changed.find('}', Outfit));
				Changed[Last] = Changed[Last] == '1' ? '2' : '1';
				What = "outfit's last number";
				break;
			}
		}
		std::string Mutated = Toml.substr(0, Start) + Changed + Toml.substr(End);
		WriteFile(File, Mutated);
		Changes++;
		std::vector<TNPCData> Loaded;
		bool Caught = false;
		try {
			LoadNPCs(File.c_str(), Loaded);
			Caught = !CompareData(Loaded, Data).empty();
		} catch (const char *) {
			Caught = true;
		}
		if (!Caught) {
			fprintf(stderr, "change not caught: entry %zu, %s\n", Index + 1, What.c_str());
			Failures++;
		}
		At = Toml.find("\n[[npc]]\n", Start);
	}
	return Changes;
}

static void TestMutations(const fs::path &Folder) {
	TNpcFolder Data;
	try {
		ReadNpcFolder((Folder / "npc").c_str(), Data);
	} catch (const char *Error) {
		fprintf(stderr, "made-up folder: %s\n", Error);
		Failures++;
		return;
	}
	int Changes = ScriptMutations(Folder / "data" / "scripts", Data);
	Changes += EntryMutations(Folder, ReadFile(Folder / "data" / "npcs.toml"), Data);
	CHECK(Changes >= 15);
}

// The files below Folder (relative paths), sorted.
static std::vector<std::string> FilesBelow(const fs::path &Folder) {
	std::vector<std::string> Files;
	if (fs::is_directory(Folder)) {
		for (const auto &Entry : fs::directory_iterator(Folder)) {
			if (Entry.is_regular_file()) {
				Files.push_back(Entry.path().filename().string());
			}
		}
	}
	std::sort(Files.begin(), Files.end());
	return Files;
}

// The written files of Out must be the committed ones of Data, byte for
// byte, none missing or extra.
static void CompareWithCommitted(const fs::path &Out, const fs::path &Data) {
	int Different = 0, Missing = 0, Extra = 0, Same = 0;
	auto Report = [&](const std::string &Text) {
		if (Different + Missing + Extra <= 10) {
			fprintf(stderr, "%s\n", Text.c_str());
		}
	};
	if (ReadFile(Out / "npcs.toml") != ReadFile(Data / "npcs.toml")) {
		Different++;
		Report((Data / "npcs.toml").string() + " is not what patella-convert npcs writes");
	} else {
		Same++;
	}
	for (const char *Sub : {"scripts/npcs", "scripts/lib/npcs"}) {
		std::vector<std::string> Written = FilesBelow(Out / Sub);
		std::vector<std::string> Committed = FilesBelow(Data / Sub);
		for (const std::string &Name : Written) {
			if (!std::binary_search(Committed.begin(), Committed.end(), Name)) {
				Missing++;
				Report(std::string("not committed: ") + Sub + "/" + Name);
			} else if (ReadFile(Out / Sub / Name) != ReadFile(Data / Sub / Name)) {
				Different++;
				Report((Data / Sub / Name).string() + " is not what patella-convert npcs writes");
			} else {
				Same++;
			}
		}
		for (const std::string &Name : Committed) {
			if (!std::binary_search(Written.begin(), Written.end(), Name)) {
				Extra++;
				Report(std::string("committed but not written: ") + Sub + "/" + Name);
			}
		}
	}
	printf("%d written files equal the committed ones; %d differ, %d not committed, %d committed but not written\n",
			Same, Different, Missing, Extra);
	Failures += Different + Missing + Extra;
}

static int Original(const char *Tar, const char *Scratch, const char *DataFolder) {
	if (!fs::is_regular_file(Tar)) {
		printf("skipped: no %s\n", Tar);
		return 77;
	}
	fs::path Folder(Scratch);
	fs::remove_all(Folder);
	fs::create_directories(Folder);
	pid_t Child = fork();
	if (Child == 0) {
		execlp("tar", "tar", "-xf", Tar, "-C", Folder.c_str(), "npc", static_cast<char *>(nullptr));
		_exit(127);
	}
	int Status = 0;
	waitpid(Child, &Status, 0);
	if (!WIFEXITED(Status) || WEXITSTATUS(Status) != 0) {
		fprintf(stderr, "cannot extract npc/ of %s\n", Tar);
		return 1;
	}

	fs::path Npc = Folder / "npc";
	fs::path Out = Folder / "data";
	TNpcFolder Data;
	try {
		ReadNpcFolder(Npc.c_str(), Data);
		WriteNpcFolder(Data, Out.c_str());
	} catch (const char *Error) {
		fprintf(stderr, "%s\n", Error);
		return 1;
	}

	size_t Expanded = 0, Own = 0, BlockRules = 0, Includes = 0, Used = 0;
	std::set<std::string> UsedBlocks;
	for (const TNpcFile &File : Data.Npcs) {
		Expanded += File.Rules.size();
		Own += static_cast<size_t>(File.OwnRules);
		for (const TNpcListItem &Item : File.List.Items) {
			Includes += Item.Kind == TNpcListItem::BLOCK ? 1 : 0;
		}
		UsedBlocks.insert(File.Blocks.begin(), File.Blocks.end());
	}
	for (const TNpcBlock &Block : Data.Blocks) {
		BlockRules += Block.Rules.size();
		Used += UsedBlocks.count(Block.Name);
	}
	printf("%zu NPCs: %zu rules with the blocks' included, %zu of their own and %zu includes; %zu blocks with %zu "
			"rules, %zu of them included, %zu not\n", Data.Npcs.size(), Expanded, Own, Includes, Data.Blocks.size(),
			BlockRules, Used, Data.Blocks.size() - Used);
	CheckOrder(Npc, Data);

	CompareWithCommitted(Out, DataFolder);
	int Before = Failures;
	CheckData(Out / "npcs.toml", Data);
	CheckRules(Out / "scripts", Data);
	if (Failures == Before) {
		printf("LoadNPCs gives the reader's data for all %zu NPCs; the scripts give the reader's rules for "
				"all %zu NPCs and %zu blocks, and the handlers hand text and situation on\n", Data.Npcs.size(),
				Data.Npcs.size(), Data.Blocks.size());
	}
	int Changes = ScriptMutations(Out / "scripts", Data);
	int Entries = EntryMutations(Folder, ReadFile(Out / "npcs.toml"), Data);
	printf("%d changes of scripts and blocks and %d of entries, each caught: %s\n", Changes, Entries,
			Failures == Before ? "yes" : "no");
	return Failures == 0 ? 0 : 1;
}

int main(int argc, char **argv) {
	if (argc == 5 && strcmp(argv[1], "--original") == 0) {
		int Result = Original(argv[2], argv[3], argv[4]);
		if (Failures != 0) {
			fprintf(stderr, "%d failures\n", Failures);
		}
		return Result;
	}
	if (argc != 1) {
		fprintf(stderr, "usage: convert_test_npcs [--original TAR SCRATCH DATA_FOLDER]\n");
		return 2;
	}

	fs::path Folder = TemporaryFolder("convert-test-npcs");
	try {
		TestConversion(Folder);
		TestRefusals(Folder);
		TestMutations(Folder);
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
