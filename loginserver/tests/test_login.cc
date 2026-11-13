// The login protocol: a login packet built the way the client builds it,
// the answers and their encryption. All data is made up.

#include "RSA_PrivateKey.hh"
#include "TRSAPublicKey.hh"
#include "login.hh"
#include "test.hh"
#include "vlong.hh"

#include <cstring>
#include <stdexcept>
#include <string>
#include <vector>

using namespace ls;

namespace {

const uint8_t TestKey[16] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16};

void putWord(std::vector<uint8_t> &data, uint16_t value) {
	data.push_back(static_cast<uint8_t>(value));
	data.push_back(static_cast<uint8_t>(value >> 8));
}

void putQuad(std::vector<uint8_t> &data, uint32_t value) {
	putWord(data, static_cast<uint16_t>(value));
	putWord(data, static_cast<uint16_t>(value >> 16));
}

// Communication.cpp SendLogin: the plain block raised to 65537 modulo P*Q.
std::vector<uint8_t> loginPacket(uint16_t system, uint16_t version, uint32_t accountId, const std::string &password,
		uint8_t first = 0) {
	std::vector<uint8_t> block;
	block.push_back(first);
	block.insert(block.end(), TestKey, TestKey + 16);
	putQuad(block, accountId);
	putWord(block, static_cast<uint16_t>(password.size()));
	block.insert(block.end(), password.begin(), password.end());
	while (block.size() < RSA_BLOCK_SIZE) {
		block.push_back(static_cast<uint8_t>(block.size() * 37));
	}
	block.resize(RSA_BLOCK_SIZE);

	vlong p;
	vlong q;
	vlong::convert(RSA_PRIME_P, p);
	vlong::convert(RSA_PRIME_Q, q);
	vlong plain;
	vlong::convert(block.data(), RSA_BLOCK_SIZE, plain);
	vlong cipher = vlong::modexp(plain, RSA_EXPONENT, p * q);
	uint8_t encrypted[RSA_BLOCK_SIZE];
	vlong::convert(cipher, encrypted, RSA_BLOCK_SIZE);

	std::vector<uint8_t> packet;
	packet.push_back(LOGIN_SEND_LOGIN);
	putWord(packet, system);
	putWord(packet, version);
	putQuad(packet, 0x11111111);
	putQuad(packet, 0x22222222);
	putQuad(packet, 0x33333333);
	packet.insert(packet.end(), encrypted, encrypted + RSA_BLOCK_SIZE);
	return packet;
}

void testTerminals() {
	CHECK(isTerminalSupported(2, 772));
	CHECK(isTerminalSupported(0, 770));
	CHECK(!isTerminalSupported(2, 769));
	CHECK(!isTerminalSupported(3, 772));
}

void testParse(LoginKey &key) {
	LoginRequest request;
	CHECK(parseLogin(loginPacket(2, 772, 1234567, "secret"), key, request) == ParseResult::Ok);
	CHECK(request.terminalType == 2 && request.terminalVersion == 772);
	CHECK(request.datSignature == 0x11111111 && request.picSignature == 0x33333333);
	CHECK(std::memcmp(request.key, TestKey, 16) == 0);
	CHECK(request.accountId == 1234567);
	CHECK(request.password == "secret");

	// The game keeps 29 bytes of a password.
	CHECK(parseLogin(loginPacket(2, 772, 1, std::string(40, 'x')), key, request) == ParseResult::Ok);
	CHECK(request.password == std::string(29, 'x'));

	// A block made with another key does not start with 0.
	std::vector<uint8_t> packet = loginPacket(2, 772, 1, "a");
	packet[40] ^= 0x01;
	CHECK(parseLogin(packet, key, request) == ParseResult::BadBlock);

	// A password longer than the block.
	LoginRequest corrupt;
	CHECK(parseLogin(loginPacket(2, 772, 1, std::string(120, 'y')), key, corrupt) == ParseResult::Corrupt);
	CHECK(std::memcmp(corrupt.key, TestKey, 16) == 0);

	std::vector<uint8_t> wrongType = loginPacket(2, 772, 1, "a");
	wrongType[0] = 0x0A;
	CHECK(parseLogin(wrongType, key, request) == ParseResult::NotLogin);
	wrongType.resize(LOGIN_PACKET_SIZE - 1);
	wrongType[0] = LOGIN_SEND_LOGIN;
	CHECK(parseLogin(wrongType, key, request) == ParseResult::NotLogin);
}

void testAnswers() {
	Answer error;
	error.error("No.");
	const std::vector<uint8_t> errorBytes = {0x0A, 3, 0, 'N', 'o', '.'};
	CHECK(error.data() == errorBytes);

	Answer list;
	list.messageOfTheDay(42, "Hi");
	Answer::Character character;
	character.name = "Alpha";
	character.world = "Zanera";
	character.address[0] = 127;
	character.address[3] = 1;
	character.port = 7172;
	CHECK(Answer::messageOfTheDaySize(42, "Hi") == 8);
	CHECK(Answer::characterListSize({character}) == 25);
	CHECK(list.characterList({character}, 3, 25) == 1);
	const std::vector<uint8_t> listBytes = {
		0x14, 5, 0, '4', '2', '\n', 'H', 'i',
		0x64, 1,
		5, 0, 'A', 'l', 'p', 'h', 'a',
		6, 0, 'Z', 'a', 'n', 'e', 'r', 'a',
		127, 0, 0, 1,
		0x04, 0x1C,
		3, 0,
	};
	CHECK(list.data() == listBytes);

	// No room for the entry: an empty list.
	Answer none;
	CHECK(none.characterList({character}, 3, 24) == 0);
	CHECK(none.data().size() == 4 && none.data()[1] == 0);

	// Names longer than the client's buffers are cut; at most 255 entries.
	Answer many;
	character.name = std::string(35, 'n');
	CHECK(many.characterList(std::vector<Answer::Character>(300, character), 0, 1u << 20) == 255);
	CHECK(many.data()[1] == 255);
	CHECK(many.data()[2] == 29 && many.data()[3] == 0);
	CHECK(Answer::characterListSize(std::vector<Answer::Character>(300, character)) == many.data().size());

	// Entries at their largest: 240 of them fill a frame.
	character.world = std::string(40, 'w');
	std::vector<Answer::Character> largest(255, character);
	CHECK(Answer::characterListSize(largest) == 4 + 255 * MAX_CHARACTER_ENTRY_SIZE);
	Answer full;
	CHECK(full.characterList(largest, 0, MAX_ANSWER_SIZE) == 240);
	CHECK(full.data().size() <= MAX_ANSWER_SIZE);

	// The longest message of the day and 200 of them fit together.
	CHECK(MAX_MOTD_LENGTH == 2756);
	Answer both;
	both.messageOfTheDay(0x7FFFFFFF, std::string(MAX_MOTD_LENGTH, 'm'));
	CHECK(both.characterList(largest, 0, MAX_ANSWER_SIZE - both.data().size()) == 200);
}

void testFrames() {
	unsigned int seed = 1;
	for (size_t size : {1u, 5u, 6u, 7u, 14u, 100u}) {
		std::vector<uint8_t> payload(size);
		for (size_t i = 0; i < size; i++) {
			payload[i] = static_cast<uint8_t>(i * 7 + 1);
		}
		std::vector<uint8_t> frame = encryptFrame(payload, TestKey, seed);
		CHECK((frame.size() - 2) % 8 == 0);
		CHECK(frame.size() - 2 >= size + 2 && frame.size() - 2 < size + 10);
		CHECK((frame[0] | (frame[1] << 8)) == static_cast<int>(frame.size() - 2));
		CHECK(decryptFrame(frame, TestKey) == payload);
	}

	// A known block, worked out with an independent XTEA (32 cycles, two
	// little-endian words): the inner length 6 and six zero bytes under the
	// zero key.
	const uint8_t zero[16] = {0};
	std::vector<uint8_t> frame = encryptFrame(std::vector<uint8_t>(6, 0), zero, seed);
	const std::vector<uint8_t> expected = {8, 0, 0x40, 0x6D, 0x94, 0x5E, 0x31, 0xD8, 0x64, 0xBE};
	CHECK(frame == expected);

	// The client's receive buffer takes frames of up to 0x4000 bytes.
	CHECK(MAX_ANSWER_SIZE == 16374);
	CHECK(encryptFrame(std::vector<uint8_t>(MAX_ANSWER_SIZE, 1), TestKey, seed).size() <= MAX_FRAME_SIZE);
	CHECK_THROWS(encryptFrame(std::vector<uint8_t>(MAX_ANSWER_SIZE + 1, 1), TestKey, seed), std::runtime_error);
}

void all() {
	LoginKey key;
	testTerminals();
	testParse(key);
	testAnswers();
	testFrames();
}

} // namespace

int main() {
	return test::run(all);
}
