// npcdata.hh: the NPCs as npcs.toml defines them (npcdata.cc): one TNPCData
// per [[npc]] entry, in the file's order, and their loader. The game makes a
// TNPC of each (InitNPCs, crnonpl.cc); tools/convert reads the file it wrote
// back with the same loader. The loader needs no game state.

#ifndef PATELLA_NPCDATA_HH
#define PATELLA_NPCDATA_HH

#include <string>
#include <vector>

#include "dbfuncs.hh"

// The longest name (TCreature::Name holds it with its terminating zero).
const int MAX_NPC_NAME_LENGTH = 30;

// The ranges of the keys (the header of npcs.toml): the outfit number, a
// color, the object type of an outfit of id 0, the home field (x and y as
// the map's sectors give them, z its floors), the radius and the GoStrength.
// The race is 1 to MAX_RACES - 1 (crmain.hh).
const int MAX_NPC_OUTFIT = 65535;
const int MAX_NPC_OUTFIT_COLOR = 255;
const int MAX_NPC_OUTFIT_OBJECT_TYPE = 65535;
const int MAX_NPC_HOME_XY = 65535;
const int MAX_NPC_HOME_Z = 15;
const int MAX_NPC_RADIUS = 65535;
const int MAX_NPC_GOSTRENGTH = 65535;

// The longest script name of an NPC: the longest LoadScript takes.
const int MAX_NPC_SCRIPT_LENGTH = 256;

// The data of one NPC: its name (Latin-1, 1 to 30 characters), sex
// (SEX_MALE or SEX_FEMALE of cr.hh), race (1 to MAX_RACES - 1), outfit (an
// OutfitID of 1 to 65535 with four colors of 0 to 255, or OutfitID 0 with an
// ObjectType of 0 to 65535; the unused bytes of the union are 0), home field
// (x and y 0 to 65535, z 0 to 15), the radius it walks in and its GoStrength
// (0 to 65535 each), its script ("npcs/<name>.lua") and Where, the place of
// the script key, "<file>:<line>:<column>", which LoadScript names in its
// errors.
struct TNPCData {
	std::string Name;
	int Sex;
	int Race;
	TOutfit Outfit;
	int HomeX, HomeY, HomeZ;
	int Radius;
	int GoStrength;
	std::string Script;
	std::string Where;
};

// Reads npcs.toml (FileName) into NPCs (cleared first), in the file's order.
// Throws a const char * "<file>:<line>:<column>: <what>" (tomlfile.hh) for a
// missing file, a syntax error, an unknown or missing key, a value of the
// wrong type or out of range; NPCs then holds the entries before the wrong
// one. A missing name, home or script gives the game's old texts "no name
// specified for NPC", "no startpoint specified for NPC" and "no behaviour
// database specified for NPC" (an empty name counts as missing, as it did).
void LoadNPCs(const char *FileName, std::vector<TNPCData> &NPCs);

#endif
