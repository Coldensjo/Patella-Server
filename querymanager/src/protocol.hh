// Wire protocol between bin/game and the query manager.
//
// Everything in this file was read from the client side of the protocol in
// bin/game (compile unit query.c, class TQueryManagerConnection) and from
// the stream classes in utils.c. Addresses are those of the May 2006 binary.
//
// Framing, both directions (TQueryManagerConnection::executeQuery, 0x80e0f10):
//   u16 length                 little-endian, number of payload bytes
//   [u32 length]               only if the u16 is 0xFFFF; replaces it
//   payload
// A request payload is `u8 query type` followed by the request fields
// (prepareQuery, 0x80e09e0). A response payload is `u8 status` followed by
// the response fields. The game closes the socket on a zero-length response
// and on one that is longer than the buffer of that connection.
//
// Timing (executeQuery, 0x80e0f10, and read, 0x80e0900): the game waits up
// to a per-query timeout for every read of a response, between 30 seconds
// (most queries) and 1800 seconds (getKeptCharacters); loginGame and
// logoutGame have 120, loadPlayers 900. A timeout fails the query. If the
// connection turns out to be closed, the game connects and logs in again
// and repeats the request once, so this server can be restarted while the
// game runs.
//
// Scalars (TWriteBuffer/TReadBuffer, 0x804db10..0x804dc00, 0x804d790..0x804d880):
//   byte   u8
//   flag   u8, 0 = false, anything else = true
//   word   u16 little-endian
//   quad   u32 little-endian
// Strings (TWriteStream::writeString 0x804d580, TReadStream::readString 0x804d2d0):
//   u16 length, or 0xFFFF followed by a u32 length; then the bytes, with no
//   terminator. The bytes are in the game's 8-bit character set (ISO-8859-1).
//   The game reads a string into a fixed buffer and silently drops what does
//   not fit, so a name longer than 29 bytes is cut on its side.

#ifndef QM_PROTOCOL_HH
#define QM_PROTOCOL_HH

#include <cstddef>
#include <cstdint>
#include <stdexcept>
#include <string>
#include <vector>

namespace qm {

// Status byte at the start of every response.
// executeQuery returns it to the caller; 3 is also logged by executeQuery
// itself ("Anfrage fehlgeschlagen"). Status 1 is followed by a u8 error code
// in the requests that define error codes. No caller in bin/game handles 2.
enum Status : uint8_t {
	STATUS_OK = 0,
	STATUS_ERROR = 1,
	STATUS_FAILED = 3,
};

// Application types sent in the login request. bin/game always sends 1
// together with its world name (InitAll 0x804c6d0 calls
// SetQueryManagerLoginData(1, WorldName)). Other values belong to other
// CipSoft programs and are not known from this binary; 2 is this project's
// choice for its own login server (loginserver/), which sends no login data.
enum ApplicationType : uint8_t {
	APPLICATION_GAME = 1,
	APPLICATION_LOGIN = 2,
};

// Every query type bin/game can send. The number is the immediate passed to
// the first TWriteBuffer::writeByte of the method (checked in the
// disassembly), the name is the TQueryManagerConnection method.
enum QueryType : uint8_t {
	QUERY_LOGIN = 0,
	QUERY_CHECK_ACCOUNT_PASSWORD = 10,
	// Not sent by bin/game: the request of this project's login server. The
	// number is free in the catalogue; CipSoft's own request for it is not
	// known.
	QUERY_LOGIN_ACCOUNT = 11,
	QUERY_LOGIN_ADMIN = 12,
	QUERY_LOGIN_GAME = 20,
	QUERY_LOGOUT_GAME = 21,
	QUERY_SET_NAMELOCK = 23,
	QUERY_BANISH_ACCOUNT = 25,
	QUERY_SET_NOTATION = 26,
	QUERY_REPORT_STATEMENT = 27,
	QUERY_BANISH_IP_ADDRESS = 28,
	QUERY_LOG_CHARACTER_DEATH = 29,
	QUERY_ADD_BUDDY = 30,
	QUERY_REMOVE_BUDDY = 31,
	QUERY_DECREMENT_IS_ONLINE = 32,
	QUERY_FINISH_AUCTIONS = 33,
	QUERY_TRANSFER_HOUSES = 35,
	QUERY_EVICT_FREE_ACCOUNTS = 36,
	QUERY_EVICT_DELETED_CHARACTERS = 37,
	QUERY_EVICT_EX_GUILDLEADERS = 38,
	QUERY_INSERT_HOUSE_OWNER = 39,
	QUERY_UPDATE_HOUSE_OWNER = 40,
	QUERY_DELETE_HOUSE_OWNER = 41,
	QUERY_GET_HOUSE_OWNERS = 42,
	QUERY_GET_AUCTIONS = 43,
	QUERY_START_AUCTION = 44,
	QUERY_INSERT_HOUSES = 45,
	QUERY_CLEAR_IS_ONLINE = 46,
	QUERY_CREATE_PLAYERLIST = 47,
	QUERY_LOG_KILLED_CREATURES = 48,
	QUERY_LOAD_PLAYERS = 50,
	QUERY_EXCLUDE_FROM_AUCTIONS = 51,
	QUERY_CANCEL_HOUSE_TRANSFER = 52,
	QUERY_LOAD_WORLD_CONFIG = 53,
	QUERY_GET_KEPT_CHARACTERS = 200,
	QUERY_GET_DELETED_CHARACTERS = 201,
	QUERY_DELETE_OLD_CHARACTER = 202,
	QUERY_GET_HIDDEN_CHARACTERS = 203,
	QUERY_CREATE_HIGHSCORES = 204,
	QUERY_CREATE_CENSUS = 205,
	QUERY_CREATE_KILL_STATISTICS = 206,
	QUERY_GET_PLAYERS_ONLINE = 207,
	QUERY_GET_WORLDS = 208,
	QUERY_GET_SERVER_LOAD = 209,
	QUERY_INSERT_PAYMENT_DATA_OLD = 210,
	QUERY_ADD_PAYMENT_OLD = 211,
	QUERY_CANCEL_PAYMENT_OLD = 212,
	QUERY_INSERT_PAYMENT_DATA_NEW = 213,
	QUERY_ADD_PAYMENT_NEW = 214,
	QUERY_CANCEL_PAYMENT_NEW = 215,
};

// Error codes that follow STATUS_ERROR in a loginGame response. The valid
// range is 1..15 (loginGame 0x80e1c10 rejects anything else); the meaning of
// each comes from the switch in PerformRegistration (0x80dce70). Code 5 is
// in the valid range but has no case there.
enum LoginGameError : uint8_t {
	LOGIN_CHARACTER_UNKNOWN = 1,        // "Spieler existiert nicht."
	LOGIN_CHARACTER_DELETED = 2,        // "Spieler wurde geloescht."
	LOGIN_WRONG_WORLD = 3,              // "Spieler lebt nicht auf dieser Welt."
	LOGIN_NOT_INVITED = 4,              // "Spieler ist nicht eingeladen."
	LOGIN_WRONG_PASSWORD = 6,           // "Falsches Passwort fuer Spieler %s"
	LOGIN_ACCOUNT_BLOCKED = 7,          // "Account disabled for five minutes."
	LOGIN_ACCOUNT_DELETED = 8,          // "Account von Spieler %s wurde geloescht."
	LOGIN_IP_BLOCKED = 9,               // "IP address blocked for 30 minutes."
	LOGIN_ACCOUNT_BANISHED = 10,        // "Account ist verbannt."
	LOGIN_NAMELOCKED = 11,              // "Character muss umbenannt werden."
	LOGIN_IP_BANISHED = 12,             // "IP-Adresse ist gesperrt."
	LOGIN_ALREADY_ONLINE = 13,          // "Schon andere Charaktere desselben Accounts eingeloggt."
	LOGIN_NOT_GAMEMASTER = 14,          // "Login mit Gamemaster-Client auf Nicht-Gamemaster-Account."
	LOGIN_WRONG_ACCOUNT = 15,           // "Falsche Accountnummer %lu fuer %s."
};

// Error codes that follow STATUS_ERROR in a loginAccount response. This
// project's own, like the request.
enum LoginAccountError : uint8_t {
	ACCOUNT_UNKNOWN = 1,                // no such account, or it is deleted
	ACCOUNT_WRONG_PASSWORD = 2,
	ACCOUNT_BANISHED = 3,
	ACCOUNT_IP_BANISHED = 4,
};

// Limits the game applies to responses.
enum Limits : uint32_t {
	// loadPlayers: InitPlayerIndex (0x80a8d80) has room for 10000 entries
	// per request and asks again while it receives exactly 10000.
	MAX_PLAYERS_PER_LOAD = 10000,
	// loginGame: 100 buddies are stored, more are read and discarded.
	MAX_BUDDIES = 100,
	// Fixed 30-byte buffers on the game side: 29 characters and a NUL.
	MAX_NAME_LENGTH = 29,
	// Largest frame this server accepts from a client. The biggest request
	// buffer in the game is max(0x4000, Houses * 600) for insertHouses.
	MAX_REQUEST_SIZE = 16u * 1024u * 1024u,
};

struct QueryInfo {
	uint8_t type;
	const char *name;        // TQueryManagerConnection method name
	uint32_t address;        // address of that method in bin/game; 0 if bin/game has none
	bool implemented;        // false: catalogued only, answered with STATUS_FAILED
};

// The catalogue of all query types, ordered by type.
const QueryInfo *queryTable(size_t &count);
// Returns nullptr for a type bin/game never sends.
const QueryInfo *findQueryInfo(uint8_t type);
// Name for logging; "unknown" for a type that is not in the catalogue.
const char *queryName(uint8_t type);

// The right names the game knows (string compares in loginGame, 0x80e1c10).
// A right is sent to the game by name; names it does not know are logged
// there as "Unbekanntes Recht".
const char *const *rightNames(size_t &count);
bool isKnownRight(const std::string &name);

// Thrown when a payload ends before a field or a frame is malformed.
class ProtocolError : public std::runtime_error {
public:
	explicit ProtocolError(const std::string &what) : std::runtime_error(what) {}
};

// Reads the fields of one payload.
class ReadBuffer {
public:
	ReadBuffer(const uint8_t *data, size_t size) : Data(data), Size(size), Position(0) {}
	explicit ReadBuffer(const std::vector<uint8_t> &payload)
		: Data(payload.data()), Size(payload.size()), Position(0) {}

	uint8_t readByte();
	bool readFlag();
	uint16_t readWord();
	uint32_t readQuad();
	std::string readString();
	void readBytes(uint8_t *buffer, size_t count);

	bool eof() const { return Position >= Size; }
	size_t remaining() const { return Size - Position; }

private:
	void need(size_t count) const;

	const uint8_t *Data;
	size_t Size;
	size_t Position;
};

// Builds one payload.
class WriteBuffer {
public:
	void writeByte(uint8_t b);
	void writeFlag(bool f);
	void writeWord(uint16_t w);
	void writeQuad(uint32_t q);
	void writeString(const std::string &s);
	void writeBytes(const uint8_t *buffer, size_t count);

	const std::vector<uint8_t> &data() const { return Data; }
	size_t size() const { return Data.size(); }
	void clear() { Data.clear(); }

private:
	std::vector<uint8_t> Data;
};

// Prepends the length header to a payload.
std::vector<uint8_t> encodeFrame(const std::vector<uint8_t> &payload);

// Result of looking for one complete frame at the start of a byte stream.
enum class FrameState {
	Incomplete,   // more bytes are needed
	Complete,     // payload and consumed are set
	Invalid,      // zero-length or oversized frame; the connection must be closed
};

// Extracts the first frame from `stream` (size bytes). On Complete, `payload`
// holds the payload and `consumed` the number of bytes the frame occupied.
FrameState decodeFrame(const uint8_t *stream, size_t size, uint32_t maxPayload,
		std::vector<uint8_t> &payload, size_t &consumed);

} // namespace qm

#endif
