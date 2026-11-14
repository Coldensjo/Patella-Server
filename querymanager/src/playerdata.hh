// Player data: what the game keeps of a character between logins (TPlayerData
// in src/dbfuncs.hh, which the game once kept in the player file
// usr/NN/<id>.usr), as the requests loadPlayerData and savePlayerData carry it
// and the storage keeps it.
//
// The record on the wire (after the character ID; WritePlayerData and
// ReadPlayerData in src/dbfuncs.cc are the game's side):
//   string Name                  the name the game saved with the data
//   quad Race, quad Profession
//   outfit OriginalOutfit, outfit CurrentOutfit:
//                                quad OutfitID, then word ObjectType if
//                                OutfitID is 0, else 4 bytes Colors (head,
//                                body, legs, feet)
//   quad LastLogin, quad LastLogout
//   quad StartX, quad StartY, quad StartZ   the start position (temple)
//   quad PosX, quad PosY, quad PosZ         the position at logout
//   quad PlayerkillerEnd
//   word NumberOfSkills, then per skill byte SkillNr and 14 quads: Actual,
//                                Maximum, Minimum, DeltaAct, MagicDeltaAct,
//                                Cycle, MaxCycle, Count, MaxCount, AddLevel,
//                                Experience, FactorPercent, NextLevel, Delta
//   word NumberOfSpells, then per spell byte SpellNr
//   word NumberOfQuestValues, then per value word Number, quad Value
//   word NumberOfMurders, then per murder quad Time, oldest first
//   string Inventory             item text, see below
//   word NumberOfDepots, then per depot byte DepotNr, string Content
// The quads of numbers and times are the game's int values as 32 bits, so
// negative ones arrive as large quads; here they are int32_t again.
//
// Item text is the text the player file had for the objects: the inventory as
// "{Position Content={...}, ...}" (the value of the file's Inventory key, with
// its line breaks), a depot as "{...}" (the Content of its entry in the file's
// Depots). The query manager stores it as it is; only the game reads it.

#ifndef QM_PLAYERDATA_HH
#define QM_PLAYERDATA_HH

#include "protocol.hh"

#include <cstdint>
#include <string>
#include <vector>

namespace qm {

struct PlayerOutfit {
	int32_t id = 0;
	uint16_t objectType = 0;         // when id is 0
	uint8_t colors[4] = {0, 0, 0, 0}; // head, body, legs, feet; when id is not 0
};

struct PlayerSkill {
	uint8_t number = 0;
	int32_t actual = 0;
	int32_t maximum = 0;
	int32_t minimum = 0;
	int32_t deltaAct = 0;
	int32_t magicDeltaAct = 0;
	int32_t cycle = 0;
	int32_t maxCycle = 0;
	int32_t count = 0;
	int32_t maxCount = 0;
	int32_t addLevel = 0;
	int32_t experience = 0;
	int32_t factorPercent = 0;
	int32_t nextLevel = 0;
	int32_t delta = 0;
};

struct PlayerQuestValue {
	uint16_t number = 0;
	int32_t value = 0;
};

struct PlayerDepot {
	uint8_t number = 0;
	std::string content;             // item text
};

struct PlayerData {
	uint32_t characterId = 0;
	std::string name;
	int32_t race = 0;
	int32_t profession = 0;
	PlayerOutfit originalOutfit;
	PlayerOutfit currentOutfit;
	int32_t lastLogin = 0;
	int32_t lastLogout = 0;
	int32_t startX = 0;
	int32_t startY = 0;
	int32_t startZ = 0;
	int32_t posX = 0;
	int32_t posY = 0;
	int32_t posZ = 0;
	int32_t playerkillerEnd = 0;
	std::vector<PlayerSkill> skills;
	std::vector<uint8_t> spells;
	std::vector<PlayerQuestValue> questValues;
	std::vector<int32_t> murders;    // oldest first
	std::string inventory;           // item text
	std::vector<PlayerDepot> depots;
};

// Reads the record that follows the character ID into data (characterId
// stays as it is). Throws ProtocolError if the payload ends early.
void readPlayerData(ReadBuffer &in, PlayerData &data);
// Writes the record, without the character ID. Throws ProtocolError for a
// list longer than its count word allows.
void writePlayerData(WriteBuffer &out, const PlayerData &data);

bool operator==(const PlayerOutfit &a, const PlayerOutfit &b);
bool operator==(const PlayerSkill &a, const PlayerSkill &b);
bool operator==(const PlayerQuestValue &a, const PlayerQuestValue &b);
bool operator==(const PlayerDepot &a, const PlayerDepot &b);
bool operator==(const PlayerData &a, const PlayerData &b);

} // namespace qm

#endif
