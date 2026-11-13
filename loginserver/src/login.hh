// The login protocol between a Tibia 7.72 client and the login server.
//
// The client side is Patella-Plus (C:\Tibia\Patella-Plus\src); the names of
// the packet types are those of its Protocol.h, the field layouts those of
// the functions named at each item. Encryption is the game server's own:
// the RSA key and the XTEA cipher of src/ (TRSAPrivateKey, TXTEASymmetricKey,
// RSA_PrivateKey.hh), so the client uses one RsaModulus for both servers.
//
// Framing (Network.cpp SendCommand and PrepareNextPacket):
//   client to server, the login packet only, unencrypted:
//     u16 length, payload
//   server to client, always encrypted:
//     u16 total, XTEA(u16 length, payload, padding to a multiple of 8)
//   XTEA runs over the bytes after `total` in 8-byte blocks of two
//   little-endian words (TXTEA.cpp, TXTEASymmetricKey.cc), with the key the
//   client put into the RSA block.
//
// The login packet (Communication.cpp SendLogin):
//   u8  0x01 (LOGIN_SEND_LOGIN)
//   u16 operating system (CLIENT_OS_WINDOWS, 2) = the game's TerminalType
//   u16 version (CLIENT_VERSION, 772)            = the game's TerminalVersion
//   u32 signatures of Tibia.dat, Tibia.spr and Tibia.pic
//   128 bytes RSA: raw, e = 65537, no padding, big-endian, holding
//       u8 0, 16 bytes XTEA key, u32 account number, string password,
//       random filler
// Strings are u16 length and the bytes, ISO-8859-1 (Utils.cpp PutString).
//
// The answers (Control.cpp ReceivePackets in connection state 4,
// Communication.cpp Receive*): one frame may carry several packets.
//   0x0A LOGIN_RECEIVE_ERROR   string text; the client shows it under
//                              "Sorry" and disconnects (ReceiveLoginError)
//   0x0B LOGIN_RECEIVE_MESSAGE string text, shown under "For Your
//                              Information" (ReceiveLoginMessage)
//   0x14 LOGIN_RECEIVE_MESSAGE_OF_THE_DAY
//                              string "N\ntext": N a decimal number; the
//                              client shows the text only when N differs
//                              from the last one it saw (Preferences.cpp
//                              SetMessageOfTheDay)
//   0x28 LOGIN_RECEIVE_SERVER_OVERLOADED
//                              no fields; the client tries another server
//   0x64 LOGIN_RECEIVE_CHARACTER_LIST
//                              u8 count, per character string name (at most
//                              29 bytes), string world (at most 29 bytes),
//                              4 bytes IPv4 address in network order
//                              (address[0] is the first number), u16 port;
//                              then u16 premium days (0 free account,
//                              0xFFFF "Gratis Premium Account")
//                              (ReceiveCharacterList, Status.cpp
//                              SetCharacter, Dialogs.cpp)

#ifndef LS_LOGIN_HH
#define LS_LOGIN_HH

#include "TRSAPrivateKey.hh"
#include "TXTEASymmetricKey.hh"

#include <cstddef>
#include <cstdint>
#include <mutex>
#include <string>
#include <vector>

namespace ls {

enum PacketType : uint8_t {
	LOGIN_SEND_LOGIN = 0x01,
	LOGIN_RECEIVE_ERROR = 0x0A,
	LOGIN_RECEIVE_MESSAGE = 0x0B,
	LOGIN_RECEIVE_MESSAGE_OF_THE_DAY = 0x14,
	LOGIN_RECEIVE_SERVER_OVERLOADED = 0x28,
	LOGIN_RECEIVE_CHARACTER_LIST = 0x64,
};

enum : size_t {
	RSA_BLOCK_SIZE = 128,
	// u8 type, u16 system, u16 version, 3 x u32 signature, RSA block.
	LOGIN_PACKET_SIZE = 1 + 2 + 2 + 12 + RSA_BLOCK_SIZE,
	// Largest login packet this server reads; the client sends exactly
	// LOGIN_PACKET_SIZE bytes.
	MAX_LOGIN_PACKET_SIZE = 1024,
	// The game reads the password into a 30-byte buffer (HandleLogin,
	// readString(PlayerPassword, 30)) and drops what does not fit; the
	// accounts' passwords are compared with that.
	MAX_PASSWORD_LENGTH = 29,
	// Names and worlds land in 30-byte buffers on the client.
	MAX_NAME_LENGTH = 29,
	// Largest frame the client takes: its receive buffer ReceiveData has
	// 0x4000 bytes (Network.h), and OnReceive (Network.cpp) drops a frame
	// whose u16 size + 2 is larger.
	MAX_FRAME_SIZE = 0x4000,
	// Largest answer payload in such a frame: u16 size, then u16 length,
	// the payload and padding to a multiple of 8 (the XTEA block).
	MAX_ANSWER_SIZE = (MAX_FRAME_SIZE - 2) / 8 * 8 - 2,
	// A character list entry at its largest: name and world of 29 bytes
	// with their u16 lengths, address, port.
	MAX_CHARACTER_ENTRY_SIZE = 2 + MAX_NAME_LENGTH + 2 + MAX_NAME_LENGTH + 4 + 2,
	// The longest message of the day leaves room for a list of this many
	// entries at their largest. 255 of them do not fit into a frame even
	// alone (see Answer::characterList).
	MOTD_RESERVED_CHARACTERS = 200,
	// Longest message of the day: the answer less the packet's type, length
	// and "N" plus newline (N up to 10 digits), and less the list's type, count,
	// premium days and reserved entries.
	MAX_MOTD_LENGTH = MAX_ANSWER_SIZE - (1 + 2 + 10 + 1) - (1 + 1 + 2)
		- MOTD_RESERVED_CHARACTERS * MAX_CHARACTER_ENTRY_SIZE,
};

// The game's version check (communication.c HandleLogin, TERMINALTYPES and
// TERMINALVERSION): terminal types 0 to 2, each from version 770 on.
const int TERMINAL_TYPES = 3;
const uint16_t TERMINAL_VERSION[TERMINAL_TYPES] = {770, 770, 770};
bool isTerminalSupported(uint16_t terminalType, uint16_t terminalVersion);

// The game server's RSA key. The decryption is serialised, as in the game
// (RSAMutex): vlong shares its values between copies without locking.
class LoginKey {
public:
	LoginKey();
	// Decrypts the block in place. Throws std::runtime_error.
	void decrypt(uint8_t block[RSA_BLOCK_SIZE]);

private:
	TRSAPrivateKey Key;
	std::mutex Mutex;
};

struct LoginRequest {
	uint16_t terminalType = 0;
	uint16_t terminalVersion = 0;
	uint32_t datSignature = 0;
	uint32_t sprSignature = 0;
	uint32_t picSignature = 0;
	uint8_t key[XTEA_KEY_LENGTH] = {0};
	uint32_t accountId = 0;
	std::string password;            // cut to MAX_PASSWORD_LENGTH
};

enum class ParseResult {
	Ok,
	NotLogin,     // too short, or not packet type 0x01
	BadBlock,     // the RSA block does not start with 0: another key
	Corrupt,      // the block's fields do not fit into it
};

// Reads the payload of the login packet and decrypts its RSA block. On
// Corrupt the key is already set, so an answer can still be sent.
ParseResult parseLogin(const std::vector<uint8_t> &payload, LoginKey &key, LoginRequest &request);

// Builders of answer packets (payloads without framing).
class Answer {
public:
	void error(const std::string &text);
	void messageOfTheDay(uint32_t number, const std::string &text);

	struct Character {
		std::string name;
		std::string world;
		uint8_t address[4] = {0, 0, 0, 0};
		uint16_t port = 0;
	};
	// Writes the list with as many characters as fit into `room` bytes, at
	// most 255, and returns how many that are. `room` must hold at least the
	// list without entries (4 bytes).
	size_t characterList(const std::vector<Character> &characters, uint16_t premiumDays, size_t room);

	// Sizes of the packets above, in bytes. characterListSize counts all
	// characters, up to 255.
	static size_t messageOfTheDaySize(uint32_t number, const std::string &text);
	static size_t characterListSize(const std::vector<Character> &characters);

	const std::vector<uint8_t> &data() const { return Data; }

private:
	void writeString(const std::string &text);

	std::vector<uint8_t> Data;
};

// Frames and encrypts a payload for the client. `seed` is the state of the
// padding's random numbers (rand_r, as the game's WriteToSocket). Throws
// std::runtime_error for a payload longer than MAX_ANSWER_SIZE.
std::vector<uint8_t> encryptFrame(const std::vector<uint8_t> &payload, const uint8_t key[XTEA_KEY_LENGTH],
		unsigned int &seed);

// The client's side of encryptFrame, for the tests: decrypts one frame and
// returns its payload. Throws std::runtime_error on a malformed frame.
std::vector<uint8_t> decryptFrame(const std::vector<uint8_t> &frame, const uint8_t key[XTEA_KEY_LENGTH]);

} // namespace ls

#endif
