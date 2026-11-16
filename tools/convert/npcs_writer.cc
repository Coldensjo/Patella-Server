// npcs_writer.cc: the writers of the converted NPCs (npcs_writer.hh).

#include "npcs_writer.hh"

#include <cstdio>
#include <filesystem>
#include <set>

#include "cr.hh"
#include "tomlfile.hh"
#include "tomlwrite.hh"

namespace {

const char *const TOML_HEADER =
	"# npcs.toml: the NPCs of the game server, converted from the original npc/*.npc\n"
	"# files by patella-convert. The game reads npcs.toml in its data folder ([paths]\n"
	"# data of config.toml) at start and makes the NPCs in the order of the file.\n"
	"# The order gives each NPC its creature ID and its turn among the creatures;\n"
	"# it is the order in which the original game read its files on the reference\n"
	"# installation (readdir of npc/). An unknown key, a missing one or a value of\n"
	"# the wrong type is an error.\n"
	"#\n"
	"# Each [[npc]] is one NPC, and every key is needed:\n"
	"#   name         Its name, 1 to 30 characters.\n"
	"#   sex          \"male\" or \"female\".\n"
	"#   race         Its race number, 1 to 511; the NPC gets the race's skills.\n"
	"#   outfit       What it looks like: { id = <outfit>, colors = [<head>,\n"
	"#                <body>, <legs>, <feet>] }, id 1 to 65535 and the colors 0 to\n"
	"#                255; or { id = 0, object_type = <type> } (0 to 65535) to look\n"
	"#                like an object.\n"
	"#   home         [x, y, z], x and y 0 to 65535, z 0 to 15: where it is placed\n"
	"#                at start, and the middle of the square it walks in.\n"
	"#   radius       How far it walks from home, in fields along x and y, 0 to\n"
	"#                65535, on home's floor.\n"
	"#   go_strength  Its GoStrength skill (its speed), 0 to 65535, set as the\n"
	"#                skill's value and maximum after the race's skills.\n"
	"#   script       Its behaviour, \"npcs/<name>.lua\", a script in the scripts\n"
	"#                folder (data/scripts/ in the repository) with the global\n"
	"#                functions onCreatureSay(npc, creature, text, situation) and\n"
	"#                onCreatureDisappear(npc, creature). The converted scripts hold\n"
	"#                the NPC's rules, which lib/npc.lua matches and runs.\n";

// The names of lib/npc.lua a script makes locals of, in the order it makes
// them (scripting.hh, "NPCs").
const char *const LOCALS[] = {
	"ADDRESS", "BUSY", "VANISH", "MALE", "FEMALE", "KNIGHT", "PALADIN", "SORCERER", "DRUID", "PREMIUM",
	"PROMOTED", "PZBLOCK", "NONPVP", "PVPENFORCED", "FORCE", "P1", "P2", "P3", "eq", "ne", "lt", "gt", "le",
	"ge", "param", "TOPIC", "PRICE", "AMOUNT", "TYPE", "DATA", "LEVEL", "MAGLEVEL", "HP", "BURNING", "POISON",
	"COUNTMONEY", "count", "spellKnown", "spellLevel", "questValue", "random", "add", "sub", "mul", "setTopic",
	"setPrice", "setAmount", "setType", "setData", "setHp", "burning", "poison", "setQuestValue", "effectMe",
	"effectOpp", "profession", "teachSpell", "summon", "create", "delete", "createMoney", "deleteMoney",
	"queue", "idle", "nop", "teleport", "startPosition", "previous",
};

const char *const HANDLERS =
	"function onCreatureSay(npc, creature, text, situation)\n"
	"\tbehaviour.react(npc, rules, text, situation)\n"
	"end\n"
	"\n"
	"function onCreatureDisappear(npc, creature)\n"
	"\tbehaviour.react(npc, rules, \"\", NPC_SITUATION_VANISH)\n"
	"end\n";

void UsedNames(const TNpcElement &Element, std::set<std::string> &Names) {
	if (Element.Kind == TNpcElement::NAME || Element.Kind == TNpcElement::CALL) {
		Names.insert(Element.Text);
	}
	for (const TNpcElement &Argument : Element.Arguments) {
		UsedNames(Argument, Names);
	}
}

std::string Elements(const std::vector<TNpcElement> &List) {
	std::string Text;
	for (size_t i = 0; i < List.size(); i++) {
		if (i > 0) {
			Text += ", ";
		}
		Text += LuaElement(List[i]);
	}
	return Text;
}

// The locals of the names the rules use, one per line in LOCALS's order.
std::string Locals(const std::vector<const TNpcRule *> &Rules) {
	std::set<std::string> Names;
	for (const TNpcRule *Rule : Rules) {
		for (const TNpcElement &Element : Rule->Conditions) {
			UsedNames(Element, Names);
		}
		for (const TNpcElement &Element : Rule->Actions) {
			UsedNames(Element, Names);
		}
	}
	std::string Text;
	size_t Known = 0;
	for (const char *Name : LOCALS) {
		if (Names.count(Name) != 0) {
			Text += std::string("local ") + Name + " = behaviour." + Name + "\n";
			Known++;
		}
	}
	if (Known != Names.size()) {
		throw "a rule uses a name lib/npc.lua does not have (a fault of patella-convert)";
	}
	return Text;
}

std::string Comment(const std::string &Latin1) {
	return "--" + Latin1ToUtf8(Latin1);
}

// The header comments of a script, if any: "--", then each comment.
std::string HeaderComments(const TNpcList &List) {
	std::string Text;
	if (!List.HeaderComments.empty()) {
		Text += "--\n";
		for (const std::string &Line : List.HeaderComments) {
			Text += Comment(Line) + "\n";
		}
	}
	return Text;
}

// The lines between "behaviour.rules {" and "}".
std::string ListLines(const TNpcList &List, const std::vector<TNpcRule> &Rules) {
	std::string Text;
	for (const TNpcListItem &Item : List.Items) {
		std::string After = Item.HasComment ? " " + Comment(Item.Comment) : "";
		switch (Item.Kind) {
			case TNpcListItem::RULE: {
				for (const std::string &Line : Item.CommentsBefore) {
					Text += "\t" + Comment(Line) + "\n";
				}
				const TNpcRule &Rule = Rules[static_cast<size_t>(Item.Rule)];
				Text += "\t{{" + Elements(Rule.Conditions) + "}, {" + Elements(Rule.Actions) + "}}," + After + "\n";
				break;
			}
			case TNpcListItem::BLOCK:
				Text += "\t" + BlockLocalName(Item.Block) + "," + After + "\n";
				break;
			case TNpcListItem::COMMENT:
				Text += "\t" + Comment(Item.Comment) + "\n";
				break;
			case TNpcListItem::BLANK:
				Text += "\n";
				break;
		}
	}
	return Text;
}

std::string TomlEntry(const TNpcFile &Npc) {
	std::string Text = "[[npc]]\n";
	Text += "name = " + TomlQuote(Npc.Name) + "\n";
	Text += std::string("sex = \"") + (Npc.Sex == SEX_FEMALE ? "female" : "male") + "\"\n";
	Text += "race = " + std::to_string(Npc.Race) + "\n";
	if (Npc.Outfit.OutfitID == 0) {
		Text += "outfit = { id = 0, object_type = " + std::to_string(Npc.Outfit.ObjectType) + " }\n";
	} else {
		Text += "outfit = { id = " + std::to_string(Npc.Outfit.OutfitID) + ", colors = [";
		for (int i = 0; i < 4; i++) {
			Text += (i > 0 ? ", " : "") + std::to_string(Npc.Outfit.Colors[i]);
		}
		Text += "] }\n";
	}
	Text += "home = [" + std::to_string(Npc.HomeX) + ", " + std::to_string(Npc.HomeY) + ", "
			+ std::to_string(Npc.HomeZ) + "]\n";
	Text += "radius = " + std::to_string(Npc.Radius) + "\n";
	Text += "go_strength = " + std::to_string(Npc.GoStrength) + "\n";
	Text += "script = \"npcs/" + Npc.FileName + ".lua\"\n";
	return Text;
}

} // namespace

std::string LuaString(const std::string &Latin1) {
	std::string Text = "\"";
	for (char Character : Latin1) {
		unsigned char c = static_cast<unsigned char>(Character);
		if (c == '"' || c == '\\') {
			Text += '\\';
			Text += Character;
		} else if (c == '\n') {
			Text += "\\n";
		} else if (c >= 32 && c <= 126) {
			Text += Character;
		} else {
			char Code[8];
			snprintf(Code, sizeof(Code), "\\%03u", static_cast<unsigned>(c));
			Text += Code;
		}
	}
	return Text + "\"";
}

std::string LuaElement(const TNpcElement &Element) {
	switch (Element.Kind) {
		case TNpcElement::STRING:
			return LuaString(Element.Text);
		case TNpcElement::NUMBER:
			return std::to_string(Element.Number);
		case TNpcElement::NAME:
			return Element.Text;
		case TNpcElement::CALL:
			return Element.Text + "(" + Elements(Element.Arguments) + ")";
	}
	return "";
}

std::string CanonicalRules(const std::vector<TNpcRule> &Rules) {
	std::string Text;
	for (const TNpcRule &Rule : Rules) {
		Text += Elements(Rule.Conditions) + " -> " + Elements(Rule.Actions) + "\n";
	}
	return Text;
}

std::string NpcsToToml(const TNpcFolder &Folder) {
	std::string Text = TOML_HEADER;
	for (const TNpcFile &Npc : Folder.Npcs) {
		Text += "\n" + TomlEntry(Npc);
	}
	return Text;
}

std::string NpcScript(const TNpcFile &Npc) {
	const std::string &File = Npc.FileName;
	std::string Text = "-- npcs/" + File + ".lua: the behaviour of the NPC " + Latin1ToUtf8(Npc.Name)
			+ ", converted from\n";
	Text += "-- " + File + ".npc by patella-convert. The NPC is its [[npc]] entry in npcs.toml;\n";
	Text += "-- lib/npc.lua matches and runs the rules below, in the order of the file\n";
	Text += "-- (scripting.hh, \"NPCs\"). The comments are those of " + File + ".npc.\n";
	Text += HeaderComments(Npc.List);
	Text += "\n";
	Text += "local behaviour = require(\"lib.npc\")\n";
	for (const std::string &Block : Npc.Blocks) {
		Text += "local " + BlockLocalName(Block) + " = require(\"lib.npcs." + Block + "\")\n";
	}
	std::vector<const TNpcRule *> Own;
	for (const TNpcListItem &Item : Npc.List.Items) {
		if (Item.Kind == TNpcListItem::RULE) {
			Own.push_back(&Npc.Rules[static_cast<size_t>(Item.Rule)]);
		}
	}
	Text += Locals(Own);
	Text += "\n";
	Text += "local rules = behaviour.rules {\n";
	Text += ListLines(Npc.List, Npc.Rules);
	Text += "}\n";
	for (const std::string &Line : Npc.List.TrailerComments) {
		Text += Comment(Line) + "\n";
	}
	Text += "\n";
	Text += HANDLERS;
	return Text;
}

std::string BlockModule(const TNpcBlock &Block) {
	const std::string &File = Block.Name;
	std::string Text = "-- lib/npcs/" + File + ".lua: the rule block " + File + ".ndb, converted by patella-convert.\n";
	Text += "-- An NPC that included it (@\"" + File + ".ndb\") has it in its rules where the include\n";
	Text += "-- was; lib/npc.lua matches and runs them (scripting.hh, \"NPCs\"). The comments\n";
	Text += "-- are those of " + File + ".ndb.\n";
	Text += HeaderComments(Block.List);
	Text += "\n";
	Text += "local behaviour = require(\"lib.npc\")\n";
	std::vector<const TNpcRule *> All;
	for (const TNpcRule &Rule : Block.Rules) {
		All.push_back(&Rule);
	}
	Text += Locals(All);
	Text += "\n";
	Text += "return behaviour.rules {\n";
	Text += ListLines(Block.List, Block.Rules);
	Text += "}\n";
	return Text;
}

void WriteNpcFolder(const TNpcFolder &Folder, const char *DataFolder) {
	namespace fs = std::filesystem;
	fs::path Data(DataFolder);
	fs::path Scripts = Data / "scripts" / "npcs";
	fs::path Blocks = Data / "scripts" / "lib" / "npcs";
	std::error_code Error;
	fs::create_directories(Scripts, Error);
	fs::create_directories(Blocks, Error);
	WriteTextFile((Data / "npcs.toml").c_str(), NpcsToToml(Folder));
	for (const TNpcFile &Npc : Folder.Npcs) {
		WriteTextFile((Scripts / (Npc.FileName + ".lua")).c_str(), NpcScript(Npc));
	}
	for (const TNpcBlock &Block : Folder.Blocks) {
		WriteTextFile((Blocks / (Block.Name + ".lua")).c_str(), BlockModule(Block));
	}
}
