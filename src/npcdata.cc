// npcdata.cc: the loader of npcs.toml (npcdata.hh), the NPCs of the game
// (data/npcs.toml in the repository, converted from the original npc/*.npc
// files by tools/convert; the file's header describes its keys). It needs no
// game state, so tools/convert links it too.

#include "tomlfile.hh"

#include <cstdint>
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>

#include "cr.hh"
#include "crmain.hh"
#include "npcdata.hh"

static_assert(MAX_NPC_NAME_LENGTH < static_cast<int>(sizeof(TCreature::Name)),
		"an NPC's name must fit into TCreature::Name");

// An integer of Min..Max; What names it in the error text.
static int ReadInteger(const toml::node &Node, const char *What, int Min, int Max) {
	if (!Node.is_integer()) {
		TomlError(Node.source(), "%s must be an integer", What);
	}
	int64_t Number = Node.as_integer()->get();
	if (Number < Min || Number > Max) {
		TomlError(Node.source(), "%s must lie between %d and %d", What, Min, Max);
	}
	return static_cast<int>(Number);
}

// The value under Key, which the entry Entry (at Where) must have.
static const toml::node &Required(const toml::table &Entry, const toml::source_region &Where, const char *Key) {
	const toml::node *Node = Entry.get(Key);
	if (Node == nullptr) {
		TomlError(Where, "[[npc]] needs %s", Key);
	}
	return *Node;
}

// The outfit, an inline table: { id = <1 to 65535>, colors = [<head>, <body>,
// <legs>, <feet>] } with each color 0 to 255, or { id = 0, object_type = <0 to
// 65535> }. The bytes of the union the outfit does not use stay 0, as for
// TOutfit() and TOutfit::load.
static TOutfit ReadOutfit(const toml::node &Node) {
	if (!Node.is_table()) {
		TomlError(Node.source(),
				"outfit must be a table { id = ..., colors = [...] } or { id = 0, object_type = ... }");
	}
	const toml::table &Table = *Node.as_table();
	TomlCheckKeys(Table, "outfit", {"id", "colors", "object_type"});

	TOutfit Outfit;
	memset(&Outfit, 0, sizeof(Outfit));
	const toml::node *Id = Table.get("id");
	if (Id == nullptr) {
		TomlError(Node.source(), "outfit needs id");
	}
	Outfit.OutfitID = ReadInteger(*Id, "id of outfit", 0, MAX_NPC_OUTFIT);

	const toml::node *Colors = Table.get("colors");
	const toml::node *ObjectType = Table.get("object_type");
	if (Outfit.OutfitID == 0) {
		if (Colors != nullptr) {
			TomlError(Colors->source(), "an outfit of id 0 has no colors (it gives object_type)");
		}
		if (ObjectType == nullptr) {
			TomlError(Node.source(), "an outfit of id 0 needs object_type");
		}
		Outfit.ObjectType = static_cast<unsigned short>(
				ReadInteger(*ObjectType, "object_type of outfit", 0, MAX_NPC_OUTFIT_OBJECT_TYPE));
	} else {
		if (ObjectType != nullptr) {
			TomlError(ObjectType->source(), "only an outfit of id 0 has object_type");
		}
		if (Colors == nullptr) {
			TomlError(Node.source(), "an outfit of id 1 or more needs colors");
		}
		const toml::array *Array = Colors->as_array();
		if (Array == nullptr || Array->size() != 4) {
			TomlError(Colors->source(), "colors of outfit must be an array [<head>, <body>, <legs>, <feet>]");
		}
		for (int i = 0; i < 4; i++) {
			Outfit.Colors[i] = static_cast<unsigned char>(
					ReadInteger(*Array->get(static_cast<size_t>(i)), "a color of outfit", 0, MAX_NPC_OUTFIT_COLOR));
		}
	}
	return Outfit;
}

// The home field [x, y, z]: x and y 0 to 65535, z 0 to 15.
static void ReadHome(const toml::node &Node, int &x, int &y, int &z) {
	const toml::array *Array = Node.as_array();
	if (Array == nullptr || Array->size() != 3) {
		TomlError(Node.source(), "home must be an array [x, y, z]");
	}
	x = ReadInteger(*Array->get(0), "x of home", 0, MAX_NPC_HOME_XY);
	y = ReadInteger(*Array->get(1), "y of home", 0, MAX_NPC_HOME_XY);
	z = ReadInteger(*Array->get(2), "z of home", 0, MAX_NPC_HOME_Z);
}

// Whether Name is the script name of an NPC: "npcs/<name>.lua" and a script
// name as scripting.hh defines it (only a-z, 0-9, '_', '-', '/' and '.'; no
// part empty, "." or ".."; a file name that is more than ".lua"; at most
// MAX_NPC_SCRIPT_LENGTH characters).
static bool ValidNpcScript(const std::string &Name) {
	const std::string Prefix = "npcs/";
	const std::string Ending = ".lua";
	if (Name.size() > static_cast<size_t>(MAX_NPC_SCRIPT_LENGTH) || Name.size() <= Prefix.size() + Ending.size()
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

// One [[npc]]: every key of the header needed, no others.
static TNPCData ReadNPC(const toml::node &Node) {
	if (!Node.is_table()) {
		TomlError(Node.source(), "an entry of npc must be a table");
	}
	const toml::table &Entry = *Node.as_table();
	const toml::source_region &Where = Node.source();
	TomlCheckKeys(Entry, "[[npc]]", {"name", "sex", "race", "outfit", "home", "radius", "go_strength", "script"});

	TNPCData NPC;
	char Name[MAX_NPC_NAME_LENGTH + 1] = "";
	if (!TomlGetString(Entry, "name", Name, sizeof(Name)) || Name[0] == 0) {
		TomlError(Where, "no name specified for NPC");
	}
	NPC.Name = Name;

	const toml::node &Sex = Required(Entry, Where, "sex");
	if (!Sex.is_string()) {
		TomlError(Sex.source(), "sex must be a string");
	}
	const std::string &SexName = Sex.as_string()->get();
	if (SexName == "male") {
		NPC.Sex = SEX_MALE;
	} else if (SexName == "female") {
		NPC.Sex = SEX_FEMALE;
	} else {
		TomlError(Sex.source(), "sex must be \"male\" or \"female\"");
	}

	NPC.Race = ReadInteger(Required(Entry, Where, "race"), "race", 1, MAX_RACES - 1);
	NPC.Outfit = ReadOutfit(Required(Entry, Where, "outfit"));

	const toml::node *Home = Entry.get("home");
	if (Home == nullptr) {
		TomlError(Where, "no startpoint specified for NPC");
	}
	ReadHome(*Home, NPC.HomeX, NPC.HomeY, NPC.HomeZ);

	NPC.Radius = ReadInteger(Required(Entry, Where, "radius"), "radius", 0, MAX_NPC_RADIUS);
	NPC.GoStrength = ReadInteger(Required(Entry, Where, "go_strength"), "go_strength", 0, MAX_NPC_GOSTRENGTH);

	auto ScriptFound = Entry.find("script");
	if (ScriptFound == Entry.end()) {
		TomlError(Where, "no behaviour database specified for NPC");
	}
	// The errors and Where name the key's place, as for the scripts of
	// spells.toml and items.toml.
	const toml::source_region &ScriptWhere = ScriptFound->first.source();
	if (!ScriptFound->second.is_string()) {
		TomlError(ScriptWhere, "script must be a string");
	}
	NPC.Script = ScriptFound->second.as_string()->get();
	if (!ValidNpcScript(NPC.Script)) {
		TomlError(ScriptWhere, "script \"%s\" must be a script name \"npcs/<name>.lua\" (a-z, 0-9, _, -, / and ., "
				"no part . or .., at most %d characters)", NPC.Script.c_str(), MAX_NPC_SCRIPT_LENGTH);
	}
	char Position[4096 + 64];
	snprintf(Position, sizeof(Position), "%s:%u:%u",
			ScriptWhere.path != nullptr ? ScriptWhere.path->c_str() : "(unknown file)", ScriptWhere.begin.line,
			ScriptWhere.begin.column);
	NPC.Where = Position;
	return NPC;
}

void LoadNPCs(const char *FileName, std::vector<TNPCData> &NPCs) {
	NPCs.clear();
	toml::table File = TomlParseFile(FileName);
	TomlCheckKeys(File, "the top level", {"npc"});
	if (const toml::array *Entries = TomlGetArray(File, "npc")) {
		for (const toml::node &Node : *Entries) {
			NPCs.push_back(ReadNPC(Node));
		}
	}
}
