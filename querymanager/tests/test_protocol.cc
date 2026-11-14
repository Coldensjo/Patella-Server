// Protocol layer: scalars, strings, framing and the query catalogue.

#include "playerdata.hh"
#include "protocol.hh"
#include "test.hh"
#include "text.hh"

#include <string>
#include <vector>

using namespace qm;

static void testScalars() {
	WriteBuffer out;
	out.writeByte(0xAB);
	out.writeFlag(true);
	out.writeFlag(false);
	out.writeWord(0x1234);
	out.writeQuad(0x89ABCDEFu);
	const std::vector<uint8_t> expected = {0xAB, 0x01, 0x00, 0x34, 0x12, 0xEF, 0xCD, 0xAB, 0x89};
	CHECK(out.data() == expected);

	ReadBuffer in(out.data());
	CHECK(in.readByte() == 0xAB);
	CHECK(in.readFlag() == true);
	CHECK(in.readFlag() == false);
	CHECK(in.readWord() == 0x1234);
	CHECK(in.readQuad() == 0x89ABCDEFu);
	CHECK(in.eof());
	CHECK_THROWS(in.readByte(), ProtocolError);

	// Any non-zero byte is a true flag.
	const std::vector<uint8_t> flag = {0x7F};
	ReadBuffer flagIn(flag);
	CHECK(flagIn.readFlag() == true);
}

static void testStrings() {
	WriteBuffer out;
	out.writeString("");
	out.writeString("Abc");
	const std::vector<uint8_t> expected = {0x00, 0x00, 0x03, 0x00, 'A', 'b', 'c'};
	CHECK(out.data() == expected);

	ReadBuffer in(out.data());
	CHECK(in.readString() == "");
	CHECK(in.readString() == "Abc");
	CHECK(in.eof());

	// Bytes above 0x7F pass through unchanged.
	WriteBuffer latin;
	latin.writeString("K\xF6nig");
	ReadBuffer latinIn(latin.data());
	CHECK(latinIn.readString() == "K\xF6nig");

	// 0xFFFF and longer use the escape word and a quad length.
	std::string longText(0xFFFF, 'x');
	WriteBuffer big;
	big.writeString(longText);
	CHECK(big.size() == 2 + 4 + longText.size());
	CHECK(big.data()[0] == 0xFF && big.data()[1] == 0xFF);
	CHECK(big.data()[2] == 0xFF && big.data()[3] == 0xFF && big.data()[4] == 0x00 && big.data()[5] == 0x00);
	ReadBuffer bigIn(big.data());
	CHECK(bigIn.readString() == longText);

	// 0xFFFE is the longest string with a plain word length.
	std::string edge(0xFFFE, 'y');
	WriteBuffer edgeOut;
	edgeOut.writeString(edge);
	CHECK(edgeOut.size() == 2 + edge.size());

	// A length that runs past the payload is an error, not a short read.
	const std::vector<uint8_t> truncated = {0x05, 0x00, 'a', 'b'};
	ReadBuffer truncatedIn(truncated);
	CHECK_THROWS(truncatedIn.readString(), ProtocolError);
}

static void testFraming() {
	const std::vector<uint8_t> payload = {0x35};
	std::vector<uint8_t> frame = encodeFrame(payload);
	const std::vector<uint8_t> expected = {0x01, 0x00, 0x35};
	CHECK(frame == expected);

	std::vector<uint8_t> decoded;
	size_t consumed = 0;
	CHECK(decodeFrame(frame.data(), 0, 1024, decoded, consumed) == FrameState::Incomplete);
	CHECK(decodeFrame(frame.data(), 1, 1024, decoded, consumed) == FrameState::Incomplete);
	CHECK(decodeFrame(frame.data(), 2, 1024, decoded, consumed) == FrameState::Incomplete);
	CHECK(decodeFrame(frame.data(), 3, 1024, decoded, consumed) == FrameState::Complete);
	CHECK(decoded == payload);
	CHECK(consumed == 3);

	// Two frames in one buffer: only the first is taken.
	std::vector<uint8_t> two = frame;
	two.insert(two.end(), {0x02, 0x00, 0x2E, 0x00});
	CHECK(decodeFrame(two.data(), two.size(), 1024, decoded, consumed) == FrameState::Complete);
	CHECK(consumed == 3);
	CHECK(decodeFrame(two.data() + 3, two.size() - 3, 1024, decoded, consumed) == FrameState::Complete);
	CHECK(consumed == 4);
	CHECK(decoded.size() == 2 && decoded[0] == 0x2E);

	// A zero length and a length above the limit are invalid.
	const std::vector<uint8_t> zero = {0x00, 0x00};
	CHECK(decodeFrame(zero.data(), zero.size(), 1024, decoded, consumed) == FrameState::Invalid);
	const std::vector<uint8_t> large = {0x01, 0x04};
	CHECK(decodeFrame(large.data(), large.size(), 1024, decoded, consumed) == FrameState::Invalid);

	// Long form: 0xFFFF, then a quad length.
	std::vector<uint8_t> big(70000, 0x5A);
	std::vector<uint8_t> bigFrame = encodeFrame(big);
	CHECK(bigFrame.size() == big.size() + 6);
	CHECK(bigFrame[0] == 0xFF && bigFrame[1] == 0xFF);
	CHECK(bigFrame[2] == 0x70 && bigFrame[3] == 0x11 && bigFrame[4] == 0x01 && bigFrame[5] == 0x00);
	CHECK(decodeFrame(bigFrame.data(), 5, 1 << 20, decoded, consumed) == FrameState::Incomplete);
	CHECK(decodeFrame(bigFrame.data(), bigFrame.size() - 1, 1 << 20, decoded, consumed)
		== FrameState::Incomplete);
	CHECK(decodeFrame(bigFrame.data(), bigFrame.size(), 1 << 20, decoded, consumed) == FrameState::Complete);
	CHECK(decoded == big);
	CHECK(consumed == bigFrame.size());

	// The game writes payloads of 0xFFFD and 0xFFFE bytes in the long form
	// too (executeQuery compares the size including the two header bytes).
	std::vector<uint8_t> gameFrame = {0xFF, 0xFF, 0x03, 0x00, 0x00, 0x00, 0x14, 0x01, 0x02};
	CHECK(decodeFrame(gameFrame.data(), gameFrame.size(), 1024, decoded, consumed) == FrameState::Complete);
	CHECK(decoded.size() == 3 && decoded[0] == 0x14);

	// The longest payload with a word length.
	std::vector<uint8_t> edge(0xFFFE, 0x01);
	CHECK(encodeFrame(edge).size() == edge.size() + 2);
}

static void testCatalogue() {
	size_t count = 0;
	const QueryInfo *table = queryTable(count);
	CHECK(count == 52);
	size_t implemented = 0;
	for (size_t i = 0; i < count; i++) {
		if (i > 0) {
			CHECK(table[i - 1].type < table[i].type);
		}
		// loginAccount, loadPlayerData and savePlayerData are this project's
		// requests; the 2006 bin/game has none of them.
		if (table[i].type == QUERY_LOGIN_ACCOUNT || table[i].type == QUERY_LOAD_PLAYER_DATA
				|| table[i].type == QUERY_SAVE_PLAYER_DATA) {
			CHECK(table[i].address == 0);
		} else {
			CHECK(table[i].address >= 0x80e04b0 && table[i].address < 0x80e8c80);
		}
		CHECK(findQueryInfo(table[i].type) == &table[i]);
		if (table[i].implemented) {
			implemented++;
		}
	}
	CHECK(implemented == 29);

	CHECK(std::string(queryName(QUERY_LOGIN)) == "login");
	CHECK(std::string(queryName(11)) == "loginAccount");
	CHECK(std::string(queryName(20)) == "loginGame");
	CHECK(std::string(queryName(21)) == "logoutGame");
	CHECK(std::string(queryName(50)) == "loadPlayers");
	CHECK(std::string(queryName(53)) == "loadWorldConfig");
	CHECK(std::string(queryName(54)) == "loadPlayerData");
	CHECK(std::string(queryName(55)) == "savePlayerData");
	CHECK(std::string(queryName(215)) == "cancelPaymentNew");
	CHECK(findQueryInfo(1) == nullptr);
	CHECK(findQueryInfo(34) == nullptr);
	CHECK(std::string(queryName(34)) == "unknown");

	size_t rights = 0;
	rightNames(rights);
	CHECK(rights == 111);
	CHECK(isKnownRight("PREMIUM_ACCOUNT"));
	CHECK(isKnownRight("OPEN_NAMEDDOORS"));
	CHECK(!isKnownRight("premium_account"));
	CHECK(!isKnownRight(""));
}

static void testText() {
	CHECK(latin1ToUtf8("abc") == "abc");
	CHECK(latin1ToUtf8("K\xF6nig") == "K\xC3\xB6nig");
	CHECK(utf8ToLatin1("K\xC3\xB6nig") == "K\xF6nig");
	CHECK(utf8ToLatin1(latin1ToUtf8("\x80\xFF\xA0")) == "\x80\xFF\xA0");
	// Outside ISO-8859-1 (U+20AC) and malformed input become '?'.
	CHECK(utf8ToLatin1("a\xE2\x82\xACz") == "a?z");
	CHECK(utf8ToLatin1("a\xFFz") == "a?z");
	CHECK(foldLatin1("AbC \xC4\xD6\xDC\xDF\xD7") == "abc \xE4\xF6\xFC\xDF\xD7");
}

// The record of loadPlayerData and savePlayerData, byte by byte.
static void testPlayerDataRecord() {
	PlayerData data;
	data.name = "Ab";
	data.race = 1;
	data.profession = -2;
	data.originalOutfit.id = 0;
	data.originalOutfit.objectType = 0x0BF2;
	data.currentOutfit.id = 130;
	data.currentOutfit.colors[0] = 1;
	data.currentOutfit.colors[1] = 2;
	data.currentOutfit.colors[2] = 3;
	data.currentOutfit.colors[3] = 4;
	data.lastLogin = 0x01020304;
	data.posZ = 7;
	PlayerSkill skill;
	skill.number = 5;
	skill.actual = 9;
	skill.delta = -1;
	data.skills = {skill};
	data.spells = {255};
	PlayerQuestValue quest;
	quest.number = 0x01F3;
	quest.value = 6;
	data.questValues = {quest};
	data.murders = {0x10};
	data.inventory = "{}";
	PlayerDepot depot;
	depot.number = 8;
	depot.content = "{1}";
	data.depots = {depot};

	WriteBuffer out;
	writePlayerData(out, data);
	std::vector<uint8_t> expected = {
		0x02, 0x00, 'A', 'b',                       // Name
		0x01, 0x00, 0x00, 0x00,                     // Race
		0xFE, 0xFF, 0xFF, 0xFF,                     // Profession -2
		0x00, 0x00, 0x00, 0x00, 0xF2, 0x0B,         // OriginalOutfit: id 0, ObjectType
		0x82, 0x00, 0x00, 0x00, 1, 2, 3, 4,         // CurrentOutfit: id 130, Colors
		0x04, 0x03, 0x02, 0x01,                     // LastLogin
		0x00, 0x00, 0x00, 0x00,                     // LastLogout
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,         // StartX, StartY, StartZ
		0, 0, 0, 0, 0, 0, 0, 0, 7, 0, 0, 0,         // PosX, PosY, PosZ
		0x00, 0x00, 0x00, 0x00,                     // PlayerkillerEnd
		0x01, 0x00, 0x05,                           // one skill, number 5
		9, 0, 0, 0,                                 // Actual
	};
	for (int i = 0; i < 12; i++) {                  // Maximum .. NextLevel
		expected.insert(expected.end(), {0, 0, 0, 0});
	}
	const std::vector<uint8_t> tail = {
		0xFF, 0xFF, 0xFF, 0xFF,                     // Delta -1
		0x01, 0x00, 0xFF,                           // one spell, 255
		0x01, 0x00, 0xF3, 0x01, 6, 0, 0, 0,         // one quest value
		0x01, 0x00, 0x10, 0x00, 0x00, 0x00,         // one murder
		0x02, 0x00, '{', '}',                       // Inventory
		0x01, 0x00, 0x08, 0x03, 0x00, '{', '1', '}', // one depot
	};
	expected.insert(expected.end(), tail.begin(), tail.end());
	CHECK(out.data() == expected);

	ReadBuffer in(out.data());
	PlayerData back;
	readPlayerData(in, back);
	CHECK(in.eof());
	CHECK(back == data);

	// Each shorter payload is refused.
	for (size_t size = 0; size < expected.size(); size++) {
		ReadBuffer part(expected.data(), size);
		PlayerData partial;
		CHECK_THROWS(readPlayerData(part, partial), ProtocolError);
	}
}

static void all() {
	testScalars();
	testStrings();
	testFraming();
	testCatalogue();
	testPlayerDataRecord();
	testText();
}

int main() {
	return test::run(all);
}
