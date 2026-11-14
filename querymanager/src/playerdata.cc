// playerdata.cc: the wire form of a character's player data (playerdata.hh).

#include "playerdata.hh"

#include <cstring>

namespace qm {

namespace {

int32_t readInt(ReadBuffer &in) {
	return static_cast<int32_t>(in.readQuad());
}

void writeInt(WriteBuffer &out, int32_t value) {
	out.writeQuad(static_cast<uint32_t>(value));
}

void readOutfit(ReadBuffer &in, PlayerOutfit &outfit) {
	outfit = PlayerOutfit();
	outfit.id = readInt(in);
	if (outfit.id == 0) {
		outfit.objectType = in.readWord();
	} else {
		in.readBytes(outfit.colors, 4);
	}
}

void writeOutfit(WriteBuffer &out, const PlayerOutfit &outfit) {
	writeInt(out, outfit.id);
	if (outfit.id == 0) {
		out.writeWord(outfit.objectType);
	} else {
		out.writeBytes(outfit.colors, 4);
	}
}

void writeCount(WriteBuffer &out, size_t count, const char *what) {
	if (count > 0xFFFF) {
		throw ProtocolError(std::string("too many ") + what);
	}
	out.writeWord(static_cast<uint16_t>(count));
}

} // namespace

void readPlayerData(ReadBuffer &in, PlayerData &data) {
	data.name = in.readString();
	data.race = readInt(in);
	data.profession = readInt(in);
	readOutfit(in, data.originalOutfit);
	readOutfit(in, data.currentOutfit);
	data.lastLogin = readInt(in);
	data.lastLogout = readInt(in);
	data.startX = readInt(in);
	data.startY = readInt(in);
	data.startZ = readInt(in);
	data.posX = readInt(in);
	data.posY = readInt(in);
	data.posZ = readInt(in);
	data.playerkillerEnd = readInt(in);

	data.skills.resize(in.readWord());
	for (PlayerSkill &skill : data.skills) {
		skill.number = in.readByte();
		skill.actual = readInt(in);
		skill.maximum = readInt(in);
		skill.minimum = readInt(in);
		skill.deltaAct = readInt(in);
		skill.magicDeltaAct = readInt(in);
		skill.cycle = readInt(in);
		skill.maxCycle = readInt(in);
		skill.count = readInt(in);
		skill.maxCount = readInt(in);
		skill.addLevel = readInt(in);
		skill.experience = readInt(in);
		skill.factorPercent = readInt(in);
		skill.nextLevel = readInt(in);
		skill.delta = readInt(in);
	}

	data.spells.resize(in.readWord());
	for (uint8_t &spell : data.spells) {
		spell = in.readByte();
	}

	data.questValues.resize(in.readWord());
	for (PlayerQuestValue &quest : data.questValues) {
		quest.number = in.readWord();
		quest.value = readInt(in);
	}

	data.murders.resize(in.readWord());
	for (int32_t &murder : data.murders) {
		murder = readInt(in);
	}

	data.inventory = in.readString();

	data.depots.resize(in.readWord());
	for (PlayerDepot &depot : data.depots) {
		depot.number = in.readByte();
		depot.content = in.readString();
	}
}

void writePlayerData(WriteBuffer &out, const PlayerData &data) {
	out.writeString(data.name);
	writeInt(out, data.race);
	writeInt(out, data.profession);
	writeOutfit(out, data.originalOutfit);
	writeOutfit(out, data.currentOutfit);
	writeInt(out, data.lastLogin);
	writeInt(out, data.lastLogout);
	writeInt(out, data.startX);
	writeInt(out, data.startY);
	writeInt(out, data.startZ);
	writeInt(out, data.posX);
	writeInt(out, data.posY);
	writeInt(out, data.posZ);
	writeInt(out, data.playerkillerEnd);

	writeCount(out, data.skills.size(), "skills");
	for (const PlayerSkill &skill : data.skills) {
		out.writeByte(skill.number);
		writeInt(out, skill.actual);
		writeInt(out, skill.maximum);
		writeInt(out, skill.minimum);
		writeInt(out, skill.deltaAct);
		writeInt(out, skill.magicDeltaAct);
		writeInt(out, skill.cycle);
		writeInt(out, skill.maxCycle);
		writeInt(out, skill.count);
		writeInt(out, skill.maxCount);
		writeInt(out, skill.addLevel);
		writeInt(out, skill.experience);
		writeInt(out, skill.factorPercent);
		writeInt(out, skill.nextLevel);
		writeInt(out, skill.delta);
	}

	writeCount(out, data.spells.size(), "spells");
	for (uint8_t spell : data.spells) {
		out.writeByte(spell);
	}

	writeCount(out, data.questValues.size(), "quest values");
	for (const PlayerQuestValue &quest : data.questValues) {
		out.writeWord(quest.number);
		writeInt(out, quest.value);
	}

	writeCount(out, data.murders.size(), "murders");
	for (int32_t murder : data.murders) {
		writeInt(out, murder);
	}

	out.writeString(data.inventory);

	writeCount(out, data.depots.size(), "depots");
	for (const PlayerDepot &depot : data.depots) {
		out.writeByte(depot.number);
		out.writeString(depot.content);
	}
}

bool operator==(const PlayerOutfit &a, const PlayerOutfit &b) {
	if (a.id != b.id) {
		return false;
	}
	if (a.id == 0) {
		return a.objectType == b.objectType;
	}
	return std::memcmp(a.colors, b.colors, sizeof(a.colors)) == 0;
}

bool operator==(const PlayerSkill &a, const PlayerSkill &b) {
	return a.number == b.number && a.actual == b.actual && a.maximum == b.maximum && a.minimum == b.minimum
		&& a.deltaAct == b.deltaAct && a.magicDeltaAct == b.magicDeltaAct && a.cycle == b.cycle
		&& a.maxCycle == b.maxCycle && a.count == b.count && a.maxCount == b.maxCount
		&& a.addLevel == b.addLevel && a.experience == b.experience && a.factorPercent == b.factorPercent
		&& a.nextLevel == b.nextLevel && a.delta == b.delta;
}

bool operator==(const PlayerQuestValue &a, const PlayerQuestValue &b) {
	return a.number == b.number && a.value == b.value;
}

bool operator==(const PlayerDepot &a, const PlayerDepot &b) {
	return a.number == b.number && a.content == b.content;
}

bool operator==(const PlayerData &a, const PlayerData &b) {
	return a.characterId == b.characterId && a.name == b.name && a.race == b.race
		&& a.profession == b.profession && a.originalOutfit == b.originalOutfit
		&& a.currentOutfit == b.currentOutfit && a.lastLogin == b.lastLogin && a.lastLogout == b.lastLogout
		&& a.startX == b.startX && a.startY == b.startY && a.startZ == b.startZ && a.posX == b.posX
		&& a.posY == b.posY && a.posZ == b.posZ && a.playerkillerEnd == b.playerkillerEnd
		&& a.skills == b.skills && a.spells == b.spells && a.questValues == b.questValues
		&& a.murders == b.murders && a.inventory == b.inventory && a.depots == b.depots;
}

} // namespace qm
