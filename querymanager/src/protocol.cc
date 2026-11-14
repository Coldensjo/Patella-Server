// protocol.cc: the query catalogue, the right names, and the readers, writers
// and framing of the wire protocol (protocol.hh).

#include "protocol.hh"

#include <cstring>

namespace qm {

// Query catalogue. Layouts of the implemented queries are documented at
// their handlers in handlers.cc. The queries that are only catalogued have
// no caller in the game server (they are used by other CipSoft programs that
// share TQueryManagerConnection) or implement policy this server does not
// have; their layouts are recorded here, as the 2006 methods write and read
// them, so that they can be added later.
//
//  10 checkAccountPassword
//       request:  quad AccountID, string Password, string IPAddress
//       response: OK: nothing. ERROR: byte code 1..4.
//  11 loginAccount: not sent by the game server, see handlers.cc.
//  54 loadPlayerData, 55 savePlayerData: this project's own, see
//       handlers.cc.
//  12 loginAdmin
//       request:  quad AccountID, flag PrivateWorld
//       response: OK: byte NumberOfCharacters, then per character
//                 string Name, string World, 4 bytes IPAddress, word Port;
//                 then word PremiumDaysLeft. ERROR: byte code 1.
//  23 setNamelock
//       request:  quad GamemasterID, string PlayerName, string IPAddress,
//                 string Reason, string Comment
//       response: OK: nothing. ERROR: byte code 1..4.
//  25 banishAccount
//       request:  quad GamemasterID, string PlayerName, string IPAddress,
//                 string Reason, string Comment, flag FinalWarning
//       response: OK: quad BanishmentID, byte Days (0xFF = unlimited),
//                 flag FinalWarning. ERROR: byte code 1..3.
//  26 setNotation
//       request:  quad GamemasterID, string PlayerName, string IPAddress,
//                 string Reason, string Comment
//       response: OK: quad BanishmentID. ERROR: byte code 1..2.
//  27 reportStatement
//       request:  quad ReporterID, string PlayerName, string Reason,
//                 string Comment, quad BanishmentID, quad StatementID,
//                 word Count, then per statement with a StatementID
//                 other than 0: quad StatementID, quad TimeStamp,
//                 quad CharacterID, string Channel, string Text.
//                 Count is NumberOfStatements minus the LEADING entries
//                 with StatementID 0 only (reportStatement's loop stops at
//                 the first other one), but every entry with StatementID 0
//                 is left out, so Count can be larger than the number of
//                 entries that follow. Read entries to the end of the
//                 request, not by Count.
//       response: OK: nothing. ERROR: byte code 1..2.
//  28 banishIPAddress
//       request:  quad GamemasterID, string PlayerName, string IPAddress,
//                 string Reason, string Comment
//       response: OK: nothing. ERROR: byte code 1..2.
// 200 getKeptCharacters, 201 getDeletedCharacters, 203 getHiddenCharacters
//       request:  quad MinimumCharacterID
//       response: quad NumberOfPlayers, then that many quad CharacterID
// 202 deleteOldCharacter
//       request:  quad CharacterID
//       response: nothing
// 204 createHighscores
//       request:  quad NumberOfPlayers, then per player quad CharacterID,
//                 quad ExpPoints, word ExpLevel, word Fist, word Club,
//                 word Axe, word Sword, word Distance, word Shielding,
//                 word Magic, word Fishing
//       response: nothing
// 205 createCensus, 206 createKillStatistics
//       request:  nothing
//       response: nothing
// 207 getPlayersOnline
//       request:  nothing
//       response: byte NumberOfWorlds, then per world string Name,
//                 word Players
// 208 getWorlds
//       request:  nothing
//       response: byte NumberOfWorlds, then that many string Name
// 209 getServerLoad
//       request:  string World, byte Period
//       response: 600 words (0xFFFF = no value)
// 210 insertPaymentDataOld, 213 insertPaymentDataNew
//       request:  quad PurchaseNr, quad ReferenceNr, strings FirstName,
//                 LastName, Company, Street, Zip, City, Country, State,
//                 Phone, Fax, EMail, PaymentMethod, quad ProductID,
//                 string Registrant, then quad AccountID (old) or
//                 string PaymentKey (new)
//       response: OK: quad PaymentID. ERROR: byte code 1.
// 211 addPaymentOld
//       request:  quad AccountID, string Description, quad PaymentID,
//                 word Days
//       response: OK: byte ActionTaken. ERROR: byte code 1..2.
// 212 cancelPaymentOld
//       request:  quad PurchaseNr, quad ReferenceNr, quad AccountID
//       response: OK: flag IllegalUse, string EMailAddress.
//                 ERROR: byte code 1..2.
// 214 addPaymentNew
//       request:  string PaymentKey, quad PaymentID
//       response: OK: byte ActionTaken, and if ActionTaken is 5 a
//                 string EMailReceiver. ERROR: byte code 1..3.
// 215 cancelPaymentNew
//       request:  quad PurchaseNr, quad ReferenceNr, string PaymentKey
//       response: OK: flag IllegalUse, flag Present, string EMailAddress.
//                 ERROR: byte code 1.
static const QueryInfo QueryTable[] = {
	{QUERY_LOGIN, "login", 0x80e04b0, true},
	{QUERY_CHECK_ACCOUNT_PASSWORD, "checkAccountPassword", 0x80e1440, false},
	{QUERY_LOGIN_ACCOUNT, "loginAccount", 0, true},
	{QUERY_LOGIN_ADMIN, "loginAdmin", 0x80e1610, false},
	{QUERY_LOGIN_GAME, "loginGame", 0x80e1c10, true},
	{QUERY_LOGOUT_GAME, "logoutGame", 0x80e3010, true},
	{QUERY_SET_NAMELOCK, "setNamelock", 0x80e34f0, false},
	{QUERY_BANISH_ACCOUNT, "banishAccount", 0x80e3760, false},
	{QUERY_SET_NOTATION, "setNotation", 0x80e3240, false},
	{QUERY_REPORT_STATEMENT, "reportStatement", 0x80e3b00, false},
	{QUERY_BANISH_IP_ADDRESS, "banishIPAddress", 0x80e3fc0, false},
	{QUERY_LOG_CHARACTER_DEATH, "logCharacterDeath", 0x80e4230, true},
	{QUERY_ADD_BUDDY, "addBuddy", 0x80e4480, true},
	{QUERY_REMOVE_BUDDY, "removeBuddy", 0x80e45b0, true},
	{QUERY_DECREMENT_IS_ONLINE, "decrementIsOnline", 0x80e46e0, true},
	{QUERY_FINISH_AUCTIONS, "finishAuctions", 0x80e47e0, true},
	{QUERY_TRANSFER_HOUSES, "transferHouses", 0x80e4b40, true},
	{QUERY_EVICT_FREE_ACCOUNTS, "evictFreeAccounts", 0x80e4e90, true},
	{QUERY_EVICT_DELETED_CHARACTERS, "evictDeletedCharacters", 0x80e5030, true},
	{QUERY_EVICT_EX_GUILDLEADERS, "evictExGuildleaders", 0x80e51a0, true},
	{QUERY_INSERT_HOUSE_OWNER, "insertHouseOwner", 0x80e53e0, true},
	{QUERY_UPDATE_HOUSE_OWNER, "updateHouseOwner", 0x80e5550, true},
	{QUERY_DELETE_HOUSE_OWNER, "deleteHouseOwner", 0x80e56c0, true},
	{QUERY_GET_HOUSE_OWNERS, "getHouseOwners", 0x80e57c0, true},
	{QUERY_GET_AUCTIONS, "getAuctions", 0x80e5a10, true},
	{QUERY_START_AUCTION, "startAuction", 0x80e5b80, true},
	{QUERY_INSERT_HOUSES, "insertHouses", 0x80e5c80, true},
	{QUERY_CLEAR_IS_ONLINE, "clearIsOnline", 0x80e6080, true},
	{QUERY_CREATE_PLAYERLIST, "createPlayerlist", 0x80e6170, true},
	{QUERY_LOG_KILLED_CREATURES, "logKilledCreatures", 0x80e63b0, true},
	{QUERY_LOAD_PLAYERS, "loadPlayers", 0x80e6590, true},
	{QUERY_EXCLUDE_FROM_AUCTIONS, "excludeFromAuctions", 0x80e4a30, true},
	{QUERY_CANCEL_HOUSE_TRANSFER, "cancelHouseTransfer", 0x80e4d90, true},
	{QUERY_LOAD_WORLD_CONFIG, "loadWorldConfig", 0x80e1950, true},
	{QUERY_LOAD_PLAYER_DATA, "loadPlayerData", 0, true},
	{QUERY_SAVE_PLAYER_DATA, "savePlayerData", 0, true},
	{QUERY_GET_KEPT_CHARACTERS, "getKeptCharacters", 0x80e6770, false},
	{QUERY_GET_DELETED_CHARACTERS, "getDeletedCharacters", 0x80e6900, false},
	{QUERY_DELETE_OLD_CHARACTER, "deleteOldCharacter", 0x80e6a90, false},
	{QUERY_GET_HIDDEN_CHARACTERS, "getHiddenCharacters", 0x80e6b90, false},
	{QUERY_CREATE_HIGHSCORES, "createHighscores", 0x80e6d20, false},
	{QUERY_CREATE_CENSUS, "createCensus", 0x80e7150, false},
	{QUERY_CREATE_KILL_STATISTICS, "createKillStatistics", 0x80e7200, false},
	{QUERY_GET_PLAYERS_ONLINE, "getPlayersOnline", 0x80e72b0, false},
	{QUERY_GET_WORLDS, "getWorlds", 0x80e7450, false},
	{QUERY_GET_SERVER_LOAD, "getServerLoad", 0x80e75b0, false},
	{QUERY_INSERT_PAYMENT_DATA_OLD, "insertPaymentDataOld", 0x80e7740, false},
	{QUERY_ADD_PAYMENT_OLD, "addPaymentOld", 0x80e7d00, false},
	{QUERY_CANCEL_PAYMENT_OLD, "cancelPaymentOld", 0x80e7f70, false},
	{QUERY_INSERT_PAYMENT_DATA_NEW, "insertPaymentDataNew", 0x80e81e0, false},
	{QUERY_ADD_PAYMENT_NEW, "addPaymentNew", 0x80e87a0, false},
	{QUERY_CANCEL_PAYMENT_NEW, "cancelPaymentNew", 0x80e89e0, false},
};

const QueryInfo *queryTable(size_t &count) {
	count = sizeof(QueryTable) / sizeof(QueryTable[0]);
	return QueryTable;
}

const QueryInfo *findQueryInfo(uint8_t type) {
	for (const QueryInfo &info : QueryTable) {
		if (info.type == type) {
			return &info;
		}
	}
	return nullptr;
}

const char *queryName(uint8_t type) {
	const QueryInfo *info = findQueryInfo(type);
	return info != nullptr ? info->name : "unknown";
}

// In the order of the compares in loginGame.
static const char *const RightNames[] = {
	"PREMIUM_ACCOUNT", "NOTATION", "NAMELOCK", "STATEMENT_REPORT",
	"BANISHMENT", "FINAL_WARNING", "IP_BANISHMENT", "KICK", "HOME_TELEPORT",
	"CLEAR_CHARACTER_INFO", "CLEAR_GUILDS", "DELETE_GUILDS", "BOARD_REPORT",
	"BOARD_MODERATION", "BOARD_ANONYMOUS_EDIT", "BOARD_PRECONFIRMED",
	"KEEP_ACCOUNT", "GAMEMASTER_BROADCAST", "ANONYMOUS_BROADCAST", "INVITED",
	"NO_BANISHMENT", "ALLOW_MULTICLIENT", "CIPWATCH_ADMIN", "CIPWATCH_USER",
	"CREATECHAR_GAMEMASTER", "CREATECHAR_GOD", "CREATECHAR_TEST",
	"VIEW_ACCOUNT", "VIEW_GAMEMASTER_RECORD", "VIEW_CRIMINAL_RECORD",
	"VIEW_LOG_FILES", "LOG_COMMUNICATION", "MODIFY_BANISHMENT",
	"READ_GAMEMASTER_CHANNEL", "READ_TUTOR_CHANNEL", "HIGHLIGHT_HELP_CHANNEL",
	"SEND_BUGREPORTS", "APPOINT_CIP", "APPOINT_SGM", "APPOINT_JGM",
	"APPOINT_SENATOR", "SET_ACCOUNT_RIGHTS", "SET_CHARACTER_RIGHTS",
	"SET_ACCOUNTGROUP_RIGHTS", "SET_CHARACTERGROUP_RIGHTS", "NAME_INSULTING",
	"NAME_SENTENCE", "NAME_NONSENSICAL_LETTERS", "NAME_BADLY_FORMATTED",
	"NAME_NO_PERSON", "NAME_CELEBRITY", "NAME_COUNTRY", "NAME_FAKE_IDENTITY",
	"NAME_FAKE_POSITION", "STATEMENT_INSULTING", "STATEMENT_SPAMMING",
	"STATEMENT_ADVERT_OFFTOPIC", "STATEMENT_ADVERT_MONEY",
	"STATEMENT_NON_ENGLISH", "STATEMENT_CHANNEL_OFFTOPIC",
	"STATEMENT_VIOLATION_INCITING", "CHEATING_BUG_ABUSE",
	"CHEATING_GAME_WEAKNESS", "CHEATING_MACRO_USE",
	"CHEATING_MODIFIED_CLIENT", "CHEATING_HACKING", "CHEATING_MULTI_CLIENT",
	"CHEATING_ACCOUNT_TRADING", "CHEATING_ACCOUNT_SHARING",
	"GAMEMASTER_THREATENING", "GAMEMASTER_PRETENDING", "GAMEMASTER_INFLUENCE",
	"GAMEMASTER_FALSE_REPORTS", "KILLING_EXCESSIVE_UNJUSTIFIED",
	"DESTRUCTIVE_BEHAVIOUR", "SPOILING_AUCTION", "INVALID_PAYMENT",
	"TELEPORT_TO_CHARACTER", "TELEPORT_TO_MARK", "TELEPORT_VERTICAL",
	"TELEPORT_TO_COORDINATE", "LEVITATE", "SPECIAL_MOVEUSE",
	"MODIFY_GOSTRENGTH", "SHOW_COORDINATE", "RETRIEVE", "INVULNERABLE",
	"UNLIMITED_MANA", "KEEP_INVENTORY", "ALL_SPELLS", "UNLIMITED_CAPACITY",
	"ZERO_CAPACITY", "ATTACK_EVERYWHERE", "NO_ATTACK", "NO_RUNES",
	"NO_LOGOUT_BLOCK", "GAMEMASTER_OUTFIT", "ILLUMINATE", "CHANGE_PROFESSION",
	"IGNORED_BY_MONSTERS", "SHOW_KEYHOLE_NUMBERS", "CREATE_OBJECTS",
	"CREATE_MONEY", "CREATE_MONSTERS", "CHANGE_SKILLS", "CLEANUP_FIELDS",
	"NO_STATISTICS", "KEEP_CHARACTER", "EXTRA_CHARACTER", "ENTER_HOUSES",
	"OPEN_NAMEDDOORS",
};

const char *const *rightNames(size_t &count) {
	count = sizeof(RightNames) / sizeof(RightNames[0]);
	return RightNames;
}

bool isKnownRight(const std::string &name) {
	for (const char *right : RightNames) {
		if (name == right) {
			return true;
		}
	}
	return false;
}

void ReadBuffer::need(size_t count) const {
	if (count > Size - Position) {
		throw ProtocolError("request ends before the expected field");
	}
}

uint8_t ReadBuffer::readByte() {
	need(1);
	return Data[Position++];
}

bool ReadBuffer::readFlag() {
	return readByte() != 0;
}

uint16_t ReadBuffer::readWord() {
	need(2);
	uint16_t w = static_cast<uint16_t>(Data[Position] | (Data[Position + 1] << 8));
	Position += 2;
	return w;
}

uint32_t ReadBuffer::readQuad() {
	need(4);
	uint32_t q = static_cast<uint32_t>(Data[Position])
		| (static_cast<uint32_t>(Data[Position + 1]) << 8)
		| (static_cast<uint32_t>(Data[Position + 2]) << 16)
		| (static_cast<uint32_t>(Data[Position + 3]) << 24);
	Position += 4;
	return q;
}

std::string ReadBuffer::readString() {
	size_t length = readWord();
	if (length == 0xFFFF) {
		length = readQuad();
	}
	need(length);
	std::string s(reinterpret_cast<const char *>(Data + Position), length);
	Position += length;
	return s;
}

void ReadBuffer::readBytes(uint8_t *buffer, size_t count) {
	need(count);
	std::memcpy(buffer, Data + Position, count);
	Position += count;
}

void WriteBuffer::writeByte(uint8_t b) {
	Data.push_back(b);
}

void WriteBuffer::writeFlag(bool f) {
	Data.push_back(f ? 1 : 0);
}

void WriteBuffer::writeWord(uint16_t w) {
	Data.push_back(static_cast<uint8_t>(w));
	Data.push_back(static_cast<uint8_t>(w >> 8));
}

void WriteBuffer::writeQuad(uint32_t q) {
	Data.push_back(static_cast<uint8_t>(q));
	Data.push_back(static_cast<uint8_t>(q >> 8));
	Data.push_back(static_cast<uint8_t>(q >> 16));
	Data.push_back(static_cast<uint8_t>(q >> 24));
}

void WriteBuffer::writeString(const std::string &s) {
	// TWriteStream::writeString: lengths below 0xFFFF fit the word.
	if (s.size() < 0xFFFF) {
		writeWord(static_cast<uint16_t>(s.size()));
	} else {
		writeWord(0xFFFF);
		writeQuad(static_cast<uint32_t>(s.size()));
	}
	Data.insert(Data.end(), s.begin(), s.end());
}

void WriteBuffer::writeBytes(const uint8_t *buffer, size_t count) {
	Data.insert(Data.end(), buffer, buffer + count);
}

std::vector<uint8_t> encodeFrame(const std::vector<uint8_t> &payload) {
	std::vector<uint8_t> frame;
	frame.reserve(payload.size() + 6);
	// executeQuery: a payload shorter than 0xFFFF - 2 bytes uses the word.
	// Staying below 0xFFFF is all the reader needs.
	if (payload.size() < 0xFFFF) {
		frame.push_back(static_cast<uint8_t>(payload.size()));
		frame.push_back(static_cast<uint8_t>(payload.size() >> 8));
	} else {
		uint32_t size = static_cast<uint32_t>(payload.size());
		frame.push_back(0xFF);
		frame.push_back(0xFF);
		frame.push_back(static_cast<uint8_t>(size));
		frame.push_back(static_cast<uint8_t>(size >> 8));
		frame.push_back(static_cast<uint8_t>(size >> 16));
		frame.push_back(static_cast<uint8_t>(size >> 24));
	}
	frame.insert(frame.end(), payload.begin(), payload.end());
	return frame;
}

FrameState decodeFrame(const uint8_t *stream, size_t size, uint32_t maxPayload,
		std::vector<uint8_t> &payload, size_t &consumed) {
	if (size < 2) {
		return FrameState::Incomplete;
	}
	size_t header = 2;
	uint32_t length = static_cast<uint32_t>(stream[0] | (stream[1] << 8));
	if (length == 0xFFFF) {
		if (size < 6) {
			return FrameState::Incomplete;
		}
		header = 6;
		length = static_cast<uint32_t>(stream[2])
			| (static_cast<uint32_t>(stream[3]) << 8)
			| (static_cast<uint32_t>(stream[4]) << 16)
			| (static_cast<uint32_t>(stream[5]) << 24);
	}
	if (length == 0 || length > maxPayload) {
		return FrameState::Invalid;
	}
	if (size - header < length) {
		return FrameState::Incomplete;
	}
	payload.assign(stream + header, stream + header + length);
	consumed = header + length;
	return FrameState::Complete;
}

} // namespace qm
