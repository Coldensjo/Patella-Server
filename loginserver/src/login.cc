#include "login.hh"

#include "RSA_PrivateKey.hh"
#include "protocol.hh"

#include <algorithm>
#include <cstdlib>
#include <cstring>
#include <stdexcept>

namespace ls {

bool isTerminalSupported(uint16_t terminalType, uint16_t terminalVersion) {
	return terminalType < TERMINAL_TYPES && terminalVersion >= TERMINAL_VERSION[terminalType];
}

LoginKey::LoginKey() {
	// Some of the texts TRSAPrivateKey throws belong to a string that is
	// gone by the time they are caught (an original bug kept in src/), so
	// they are not read here.
	try {
		Key.init(RSA_PRIME_P, RSA_PRIME_Q);
	} catch (const char *) {
		throw std::runtime_error("cannot set up the RSA key");
	}
}

void LoginKey::decrypt(uint8_t block[RSA_BLOCK_SIZE]) {
	std::lock_guard<std::mutex> lock(Mutex);
	try {
		Key.decrypt(block);
	} catch (const char *) {
		throw std::runtime_error("RSA decryption failed");
	}
}

ParseResult parseLogin(const std::vector<uint8_t> &payload, LoginKey &key, LoginRequest &request) {
	if (payload.size() < LOGIN_PACKET_SIZE || payload[0] != LOGIN_SEND_LOGIN) {
		return ParseResult::NotLogin;
	}
	qm::ReadBuffer in(payload.data() + 1, payload.size() - 1);
	request.terminalType = in.readWord();
	request.terminalVersion = in.readWord();
	request.datSignature = in.readQuad();
	request.sprSignature = in.readQuad();
	request.picSignature = in.readQuad();
	uint8_t block[RSA_BLOCK_SIZE];
	in.readBytes(block, RSA_BLOCK_SIZE);

	key.decrypt(block);
	if (block[0] != 0) {
		return ParseResult::BadBlock;
	}
	std::memcpy(request.key, block + 1, XTEA_KEY_LENGTH);
	try {
		qm::ReadBuffer secret(block + 1 + XTEA_KEY_LENGTH, RSA_BLOCK_SIZE - 1 - XTEA_KEY_LENGTH);
		request.accountId = secret.readQuad();
		request.password = secret.readString();
	} catch (const qm::ProtocolError &) {
		return ParseResult::Corrupt;
	}
	if (request.password.size() > MAX_PASSWORD_LENGTH) {
		request.password.resize(MAX_PASSWORD_LENGTH);
	}
	return ParseResult::Ok;
}

void Answer::writeString(const std::string &text) {
	// The client reads a u16 length (Utils.cpp TReadBuffer::GetString).
	size_t length = text.size() > 0xFFFF ? 0xFFFF : text.size();
	Data.push_back(static_cast<uint8_t>(length));
	Data.push_back(static_cast<uint8_t>(length >> 8));
	Data.insert(Data.end(), text.begin(), text.begin() + static_cast<std::ptrdiff_t>(length));
}

void Answer::error(const std::string &text) {
	Data.push_back(LOGIN_RECEIVE_ERROR);
	writeString(text);
}

void Answer::messageOfTheDay(uint32_t number, const std::string &text) {
	Data.push_back(LOGIN_RECEIVE_MESSAGE_OF_THE_DAY);
	writeString(std::to_string(number) + "\n" + text);
}

namespace {

size_t entrySize(const Answer::Character &character) {
	return 2 + std::min<size_t>(character.name.size(), MAX_NAME_LENGTH)
		+ 2 + std::min<size_t>(character.world.size(), MAX_NAME_LENGTH) + 4 + 2;
}

} // namespace

size_t Answer::messageOfTheDaySize(uint32_t number, const std::string &text) {
	return 1 + 2 + std::to_string(number).size() + 1 + text.size();
}

size_t Answer::characterListSize(const std::vector<Character> &characters) {
	size_t size = 1 + 1 + 2;
	for (size_t i = 0; i < characters.size() && i < 255; i++) {
		size += entrySize(characters[i]);
	}
	return size;
}

size_t Answer::characterList(const std::vector<Character> &characters, uint16_t premiumDays, size_t room) {
	size_t size = 1 + 1 + 2;
	size_t count = 0;
	while (count < characters.size() && count < 255 && size + entrySize(characters[count]) <= room) {
		size += entrySize(characters[count]);
		count++;
	}
	Data.push_back(LOGIN_RECEIVE_CHARACTER_LIST);
	Data.push_back(static_cast<uint8_t>(count));
	for (size_t i = 0; i < count; i++) {
		const Character &character = characters[i];
		writeString(character.name.substr(0, MAX_NAME_LENGTH));
		writeString(character.world.substr(0, MAX_NAME_LENGTH));
		Data.insert(Data.end(), character.address, character.address + 4);
		Data.push_back(static_cast<uint8_t>(character.port));
		Data.push_back(static_cast<uint8_t>(character.port >> 8));
	}
	Data.push_back(static_cast<uint8_t>(premiumDays));
	Data.push_back(static_cast<uint8_t>(premiumDays >> 8));
	return count;
}

std::vector<uint8_t> encryptFrame(const std::vector<uint8_t> &payload, const uint8_t key[XTEA_KEY_LENGTH],
		unsigned int &seed) {
	if (payload.size() > MAX_ANSWER_SIZE) {
		throw std::runtime_error("answer too long for the client's receive buffer");
	}
	size_t size = payload.size() + 2;
	while (size % XTEA_BLOCK_LENGTH != 0) {
		size++;
	}
	std::vector<uint8_t> frame(2 + size);
	frame[0] = static_cast<uint8_t>(size);
	frame[1] = static_cast<uint8_t>(size >> 8);
	frame[2] = static_cast<uint8_t>(payload.size());
	frame[3] = static_cast<uint8_t>(payload.size() >> 8);
	std::memcpy(frame.data() + 4, payload.data(), payload.size());
	for (size_t i = 4 + payload.size(); i < frame.size(); i++) {
		frame[i] = static_cast<uint8_t>(rand_r(&seed));
	}
	TXTEASymmetricKey cipher;
	cipher.init(key);
	for (size_t offset = 2; offset < frame.size(); offset += XTEA_BLOCK_LENGTH) {
		cipher.encrypt(frame.data() + offset);
	}
	return frame;
}

std::vector<uint8_t> decryptFrame(const std::vector<uint8_t> &frame, const uint8_t key[XTEA_KEY_LENGTH]) {
	if (frame.size() < 2) {
		throw std::runtime_error("frame without size");
	}
	size_t size = frame[0] | (static_cast<size_t>(frame[1]) << 8);
	if (size != frame.size() - 2 || size % XTEA_BLOCK_LENGTH != 0 || size < XTEA_BLOCK_LENGTH) {
		throw std::runtime_error("frame size does not fit");
	}
	std::vector<uint8_t> plain(frame.begin() + 2, frame.end());
	TXTEASymmetricKey cipher;
	cipher.init(key);
	for (size_t offset = 0; offset < plain.size(); offset += XTEA_BLOCK_LENGTH) {
		cipher.decrypt(plain.data() + offset);
	}
	size_t length = plain[0] | (static_cast<size_t>(plain[1]) << 8);
	if (length + 2 > plain.size()) {
		throw std::runtime_error("payload runs past the frame");
	}
	return std::vector<uint8_t>(plain.begin() + 2, plain.begin() + 2 + static_cast<std::ptrdiff_t>(length));
}

} // namespace ls
