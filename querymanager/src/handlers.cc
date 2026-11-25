// handlers.cc: the dispatcher and the request handlers; each handler's comment
// gives the layout of its request and its response.

#include "handlers.hh"

#include "log.hh"

#include <poll.h>

#include <algorithm>
#include <cstdarg>
#include <cstdio>
#include <ctime>

namespace qm {

namespace {

// Thrown by a handler that cannot carry out a well-formed request.
class RequestFailed : public std::runtime_error {
public:
	explicit RequestFailed(const std::string &what) : std::runtime_error(what) {}
};

std::string format(const char *fmt, ...) __attribute__((format(printf, 1, 2)));

// Whether the client has closed its side of the connection (socket -1: no
// connection, never).
bool clientClosed(int socket) {
	if (socket < 0) {
		return false;
	}
	struct pollfd fd;
	fd.fd = socket;
	fd.events = POLLRDHUP;
	fd.revents = 0;
	return poll(&fd, 1, 0) > 0 && (fd.revents & (POLLRDHUP | POLLHUP | POLLERR)) != 0;
}

std::string format(const char *fmt, ...) {
	char buffer[512];
	va_list args;
	va_start(args, fmt);
	std::vsnprintf(buffer, sizeof(buffer), fmt, args);
	va_end(args);
	return buffer;
}

std::string hexDump(const std::vector<uint8_t> &data, size_t limit) {
	static const char Digits[] = "0123456789abcdef";
	std::string out;
	size_t count = std::min(data.size(), limit);
	for (size_t i = 0; i < count; i++) {
		out.push_back(Digits[data[i] >> 4]);
		out.push_back(Digits[data[i] & 0x0F]);
	}
	if (data.size() > limit) {
		out += "...";
	}
	return out;
}

std::vector<uint8_t> failedResponse() {
	return std::vector<uint8_t>{STATUS_FAILED};
}

void writeError(WriteBuffer &out, uint8_t code) {
	out.writeByte(STATUS_ERROR);
	out.writeByte(code);
}

// The game reads every name into a 30-byte buffer (readString with
// MaxLength 0x1e) and drops what does not fit. Cutting on this side changes
// nothing the game sees, and keeps a response inside the buffer the game
// sized for 29-byte names (loadPlayers: 360007 bytes for 10000 entries).
std::string cut(const std::string &name) {
	return name.size() > MAX_NAME_LENGTH ? name.substr(0, MAX_NAME_LENGTH) : name;
}

bool contains(const std::vector<std::string> &list, const char *value) {
	return std::find(list.begin(), list.end(), value) != list.end();
}

// The banishments of the game's own banishAccount (the rule violation
// dialog): the first lasts FIRST_BANISHMENT_DAYS, the next ones
// LATER_BANISHMENT_DAYS; one with a final warning, and every one from the
// third on, FINAL_WARNING_DAYS with a final warning; one after a final
// warning is for good.
const uint32_t FIRST_BANISHMENT_DAYS = 7;
const uint32_t LATER_BANISHMENT_DAYS = 15;
const uint32_t FINAL_WARNING_DAYS = 30;

// How long the game's own banishIPAddress blocks an address.
const uint32_t IP_BANISHMENT_DAYS = 7;

// The most days a banishment of the admin port may last (more: for good).
const uint32_t MAX_BANISHMENT_DAYS = 3650;

const uint32_t SECONDS_PER_DAY = 86400;

// The end of a banishment of Days days from now (0: for good).
uint32_t banishmentEnd(uint32_t now, uint32_t days) {
	if (days == 0 || days > MAX_BANISHMENT_DAYS) {
		return PERMANENT_BANISHMENT;
	}
	uint64_t end = static_cast<uint64_t>(now) + static_cast<uint64_t>(days) * SECONDS_PER_DAY;
	return end >= PERMANENT_BANISHMENT ? PERMANENT_BANISHMENT - 1 : static_cast<uint32_t>(end);
}

// Whether the account of the character is a gamemaster's (it may not be
// punished, as the 2006 query manager refused).
bool isGamemasterAccount(Storage &store, const CharacterRecord &character) {
	if (character.accountId == 0) {
		return false;
	}
	std::optional<AccountRecord> account = store.findAccount(character.accountId);
	return account && account->gamemaster;
}

// Whether the text is an IPv4 address in dotted form ("192.0.2.1").
bool isIpAddress(const std::string &text) {
	int parts = 0;
	size_t i = 0;
	while (i <= text.size()) {
		size_t start = i;
		int value = 0;
		while (i < text.size() && text[i] >= '0' && text[i] <= '9' && i - start < 3) {
			value = value * 10 + (text[i] - '0');
			i++;
		}
		if (i == start || value > 255) {
			return false;
		}
		parts++;
		if (i == text.size()) {
			break;
		}
		if (text[i] != '.' || parts == 4) {
			return false;
		}
		i++;
	}
	return parts == 4;
}

} // namespace

uint32_t systemClock() {
	return static_cast<uint32_t>(std::time(nullptr));
}

Dispatcher::Dispatcher(Storage &storage, Clock clock) : Store(storage), Now(std::move(clock)) {
}

std::vector<uint8_t> Dispatcher::handle(Session &session, const std::vector<uint8_t> &request) {
	std::lock_guard<std::mutex> lock(Mutex);

	if (request.empty()) {
		logMessage(LogLevel::Error, "conn %llu: empty request",
			static_cast<unsigned long long>(session.connectionId));
		return failedResponse();
	}
	uint8_t type = request[0];
	const QueryInfo *info = findQueryInfo(type);
	const char *world = session.worldName.empty() ? "-" : session.worldName.c_str();
	unsigned long long conn = static_cast<unsigned long long>(session.connectionId);

	// The login requests carry passwords and the player data requests a
	// player's data; their bytes stay out of the log.
	bool secret = type == QUERY_LOGIN || type == QUERY_LOGIN_GAME || type == QUERY_LOGIN_ACCOUNT
		|| type == QUERY_LOAD_PLAYER_DATA || type == QUERY_SAVE_PLAYER_DATA;
	if (logLevel() >= LogLevel::Debug) {
		logMessage(LogLevel::Debug, "conn %llu %s request type=%u size=%zu data=%s", conn, world,
			static_cast<unsigned>(type), request.size(),
			secret ? "(not logged)" : hexDump(request, 64).c_str());
	}

	if (info == nullptr) {
		logMessage(LogLevel::Error, "conn %llu %s UNKNOWN query type %u (%zu bytes) -> FAILED", conn,
			world, static_cast<unsigned>(type), request.size());
		return failedResponse();
	}
	if (!info->implemented) {
		logMessage(LogLevel::Error, "conn %llu %s UNIMPLEMENTED %s (type %u, %zu bytes) -> FAILED", conn,
			world, info->name, static_cast<unsigned>(type), request.size());
		return failedResponse();
	}
	if (type != QUERY_LOGIN && !session.loggedIn) {
		logMessage(LogLevel::Error, "conn %llu %s %s before login -> FAILED", conn, world, info->name);
		return failedResponse();
	}

	Detail.clear();
	WriteBuffer out;
	bool open = false;
	// A request that fails must leave the session as it was (a login whose
	// request is malformed or cannot be committed is not a login).
	Session before = session;
	try {
		ReadBuffer in(request.data() + 1, request.size() - 1);
		Store.begin();
		open = true;
		dispatch(session, type, in, out);
		if (!in.eof()) {
			// The layout this server expects is shorter than what the game
			// sent: the handler is wrong, so do not pretend it worked.
			throw ProtocolError(format("%zu unread bytes at the end of the request", in.remaining()));
		}
		Store.commit();
		open = false;
	} catch (const std::exception &e) {
		session = before;
		if (open) {
			try {
				Store.rollback();
			} catch (const std::exception &e2) {
				logMessage(LogLevel::Error, "conn %llu rollback failed: %s", conn, e2.what());
			}
		}
		logMessage(LogLevel::Error, "conn %llu %s %s -> FAILED: %s", conn,
			session.worldName.empty() ? "-" : session.worldName.c_str(), info->name, e.what());
		return failedResponse();
	}

	world = session.worldName.empty() ? "-" : session.worldName.c_str();
	const std::vector<uint8_t> &response = out.data();
	if (response.empty()) {
		logMessage(LogLevel::Error, "conn %llu %s %s produced no response -> FAILED", conn, world,
			info->name);
		return failedResponse();
	}
	if (response[0] == STATUS_OK) {
		logMessage(LogLevel::Info, "conn %llu %s %s -> OK%s%s", conn, world, info->name,
			Detail.empty() ? "" : " ", Detail.c_str());
	} else if (response[0] == STATUS_ERROR && response.size() >= 2) {
		logMessage(LogLevel::Info, "conn %llu %s %s -> ERROR %u%s%s", conn, world, info->name,
			static_cast<unsigned>(response[1]), Detail.empty() ? "" : " ", Detail.c_str());
	} else {
		logMessage(LogLevel::Info, "conn %llu %s %s -> status %u%s%s", conn, world, info->name,
			static_cast<unsigned>(response[0]), Detail.empty() ? "" : " ", Detail.c_str());
	}
	if (logLevel() >= LogLevel::Debug) {
		logMessage(LogLevel::Debug, "conn %llu %s response size=%zu data=%s", conn, world,
			response.size(), secret ? "(not logged)" : hexDump(response, 64).c_str());
	}
	return response;
}

void Dispatcher::dispatch(Session &session, uint8_t type, ReadBuffer &in, WriteBuffer &out) {
	switch (type) {
	case QUERY_LOGIN: handleLogin(session, in, out); break;
	case QUERY_LOGIN_ACCOUNT: handleLoginAccount(session, in, out); break;
	case QUERY_LOAD_WORLD_CONFIG: handleLoadWorldConfig(session, in, out); break;
	case QUERY_LOAD_PLAYERS: handleLoadPlayers(session, in, out); break;
	case QUERY_LOGIN_GAME: handleLoginGame(session, in, out); break;
	case QUERY_LOGOUT_GAME: handleLogoutGame(session, in, out); break;
	case QUERY_DECREMENT_IS_ONLINE: handleDecrementIsOnline(session, in, out); break;
	case QUERY_CLEAR_IS_ONLINE: handleClearIsOnline(session, in, out); break;
	case QUERY_CREATE_PLAYERLIST: handleCreatePlayerlist(session, in, out); break;
	case QUERY_LOG_KILLED_CREATURES: handleLogKilledCreatures(session, in, out); break;
	case QUERY_LOG_CHARACTER_DEATH: handleLogCharacterDeath(session, in, out); break;
	case QUERY_ADD_BUDDY: handleAddBuddy(session, in, out); break;
	case QUERY_REMOVE_BUDDY: handleRemoveBuddy(session, in, out); break;
	case QUERY_INSERT_HOUSES: handleInsertHouses(session, in, out); break;
	case QUERY_GET_HOUSE_OWNERS: handleGetHouseOwners(session, in, out); break;
	case QUERY_INSERT_HOUSE_OWNER: handleInsertHouseOwner(session, in, out); break;
	case QUERY_UPDATE_HOUSE_OWNER: handleUpdateHouseOwner(session, in, out); break;
	case QUERY_DELETE_HOUSE_OWNER: handleDeleteHouseOwner(session, in, out); break;
	case QUERY_GET_AUCTIONS: handleGetAuctions(session, in, out); break;
	case QUERY_START_AUCTION: handleStartAuction(session, in, out); break;
	case QUERY_FINISH_AUCTIONS: handleFinishAuctions(session, in, out); break;
	case QUERY_EXCLUDE_FROM_AUCTIONS: handleExcludeFromAuctions(session, in, out); break;
	case QUERY_TRANSFER_HOUSES: handleTransferHouses(session, in, out); break;
	case QUERY_CANCEL_HOUSE_TRANSFER: handleCancelHouseTransfer(session, in, out); break;
	case QUERY_EVICT_FREE_ACCOUNTS: handleEvictFreeAccounts(session, in, out); break;
	case QUERY_EVICT_DELETED_CHARACTERS: handleEvictDeletedCharacters(session, in, out); break;
	case QUERY_EVICT_EX_GUILDLEADERS: handleEvictExGuildleaders(session, in, out); break;
	case QUERY_LOAD_PLAYER_DATA: handleLoadPlayerData(session, in, out); break;
	case QUERY_SAVE_PLAYER_DATA: handleSavePlayerData(session, in, out); break;
	case QUERY_SET_NAMELOCK: handleSetNamelock(session, in, out); break;
	case QUERY_BANISH_ACCOUNT: handleBanishAccount(session, in, out); break;
	case QUERY_SET_NOTATION: handleSetNotation(session, in, out); break;
	case QUERY_BANISH_IP_ADDRESS: handleBanishIPAddress(session, in, out); break;
	case QUERY_BANISH_ACCOUNT_FOR: handleBanishAccountFor(session, in, out); break;
	case QUERY_LIFT_BANISHMENT: handleLiftBanishment(session, in, out); break;
	case QUERY_BANISH_IP_ADDRESS_FOR: handleBanishIPAddressFor(session, in, out); break;
	case QUERY_RENAME_CHARACTER: handleRenameCharacter(session, in, out); break;
	default:
		// The catalogue says "implemented" but there is no case for it.
		throw RequestFailed("no handler");
	}
}

uint32_t Dispatcher::requireWorld(const Session &session) const {
	if (session.worldId == 0) {
		throw RequestFailed("the application is not logged in for a world");
	}
	return session.worldId;
}

// Query 0, login. Sent by TQueryManagerConnection::connect right after the
// TCP connect, with a 30 second timeout.
//   request:  byte ApplicationType, string Password,
//             string LoginData (only if ApplicationType is 1)
//   response: status only
// connect() treats every status other than 0 as "Login failed" and reads
// nothing else. The password is the one from the QueryManager
// entry in config.toml after DisguisePassword (src/config.cc); for the game
// server LoginData is its world name (InitAll).
void Dispatcher::handleLogin(Session &session, ReadBuffer &in, WriteBuffer &out) {
	uint8_t applicationType = in.readByte();
	std::string password = in.readString();
	std::string loginData;
	if (applicationType == APPLICATION_GAME) {
		loginData = in.readString();
	}

	if (!Store.checkApplicationPassword(applicationType, password)) {
		throw RequestFailed(format("wrong password for application type %u",
			static_cast<unsigned>(applicationType)));
	}
	uint32_t worldId = 0;
	std::string worldName;
	if (applicationType == APPLICATION_GAME) {
		std::optional<WorldRecord> world = Store.findWorld(loginData);
		if (!world) {
			throw RequestFailed("unknown world \"" + loginData + "\"");
		}
		worldId = world->id;
		worldName = world->name;
	}
	session.loggedIn = true;
	session.applicationType = applicationType;
	session.worldId = worldId;
	session.worldName = worldName;
	Detail = format("application=%u", static_cast<unsigned>(applicationType));
	out.writeByte(STATUS_OK);
}

// Query 11, loginAccount. Not sent by the game server: the request of this
// project's login server (loginserver/), which answers a client's login with
// the character list. Only the login server's application type may send it.
//   request:  quad AccountID, string Password, string IPAddress
//   response, OK:
//             byte NumberOfCharacters, then per character string Name,
//             string World, 4 bytes IPAddress, word Port; then
//             word PremiumDaysLeft
//   response, ERROR: byte code, see LoginAccountError
// The OK layout is the one of loginAdmin (12), the catalogued request that
// answers with the same kind of list. IPAddress and Port of a character are
// those of its world in the worlds table, which the game listens on
// (loadWorldConfig). Characters of a world that does not exist are left
// out, and at most 255 are sent. PremiumDaysLeft counts started days; with
// 32-bit times it stays below 65535, which the client would show as
// "Gratis Premium Account".
void Dispatcher::handleLoginAccount(Session &session, ReadBuffer &in, WriteBuffer &out) {
	uint32_t accountId = in.readQuad();
	std::string password = in.readString();
	std::string address = in.readString();
	if (session.applicationType != APPLICATION_LOGIN) {
		throw RequestFailed("only the login server may ask for an account's characters");
	}
	uint32_t now = Now();

	Detail = format("account=%u", accountId);
	std::optional<AccountRecord> account = Store.findAccount(accountId);
	if (!account || account->deleted) {
		Detail += " unknown";
		return writeError(out, ACCOUNT_UNKNOWN);
	}
	if (account->password != password) {
		Detail += " wrong password";
		return writeError(out, ACCOUNT_WRONG_PASSWORD);
	}
	if (account->banishedUntil > now) {
		Detail += " banished";
		return writeError(out, ACCOUNT_BANISHED);
	}
	if (Store.isIpBanished(address, now)) {
		Detail += " ip banished";
		return writeError(out, ACCOUNT_IP_BANISHED);
	}

	struct Entry {
		const CharacterRecord *character;
		const WorldRecord *world;
	};
	std::vector<CharacterRecord> characters = Store.listAccountCharacters(accountId);
	std::vector<WorldRecord> worlds;
	std::vector<Entry> entries;
	worlds.reserve(characters.size());
	for (const CharacterRecord &character : characters) {
		const WorldRecord *world = nullptr;
		for (const WorldRecord &known : worlds) {
			if (known.id == character.worldId) {
				world = &known;
				break;
			}
		}
		if (world == nullptr) {
			std::optional<WorldRecord> found = Store.getWorld(character.worldId);
			if (!found) {
				continue;
			}
			worlds.push_back(*found);
			world = &worlds.back();
		}
		if (entries.size() < 255) {
			entries.push_back(Entry{&character, world});
		}
	}

	uint32_t premiumDays = 0;
	if (account->premiumUntil > now) {
		premiumDays = (account->premiumUntil - now - 1) / 86400 + 1;
	}

	out.writeByte(STATUS_OK);
	out.writeByte(static_cast<uint8_t>(entries.size()));
	for (const Entry &entry : entries) {
		out.writeString(cut(entry.character->name));
		out.writeString(cut(entry.world->name));
		out.writeBytes(entry.world->config.address, 4);
		out.writeWord(entry.world->config.port);
	}
	out.writeWord(static_cast<uint16_t>(premiumDays));
	Detail += format(" characters=%zu premium_days=%u", entries.size(), premiumDays);
}

// Query 53, loadWorldConfig, called once by LoadWorldConfig (src/main.cc) on
// a connection of its own.
//   request:  nothing
//   response: byte WorldType, byte RebootTime, 4 bytes IPAddress, word Port,
//             word MaxPlayers, word PremiumPlayerBuffer, word MaxNewbies,
//             word PremiumNewbieBuffer
// The game multiplies RebootTime by 60 and compares it with the minute of
// the day, so the byte is the hour of the daily reboot. IPAddress and Port
// are what the game binds its listen socket to (OpenSocket).
void Dispatcher::handleLoadWorldConfig(Session &session, ReadBuffer &, WriteBuffer &out) {
	std::optional<WorldRecord> world = Store.getWorld(requireWorld(session));
	if (!world) {
		throw RequestFailed("world record is gone");
	}
	const WorldConfig &config = world->config;
	out.writeByte(STATUS_OK);
	out.writeByte(config.type);
	out.writeByte(config.rebootHour);
	out.writeBytes(config.address, 4);
	out.writeWord(config.port);
	out.writeWord(config.maxPlayers);
	out.writeWord(config.premiumPlayerBuffer);
	out.writeWord(config.maxNewbies);
	out.writeWord(config.premiumNewbieBuffer);
	Detail = format("type=%u reboot=%u:00 address=%u.%u.%u.%u:%u players=%u/%u newbies=%u/%u",
		static_cast<unsigned>(config.type), static_cast<unsigned>(config.rebootHour),
		static_cast<unsigned>(config.address[0]), static_cast<unsigned>(config.address[1]),
		static_cast<unsigned>(config.address[2]), static_cast<unsigned>(config.address[3]),
		static_cast<unsigned>(config.port), static_cast<unsigned>(config.maxPlayers),
		static_cast<unsigned>(config.premiumPlayerBuffer), static_cast<unsigned>(config.maxNewbies),
		static_cast<unsigned>(config.premiumNewbieBuffer));
}

// Query 50, loadPlayers, called by InitPlayerIndex (src/crplayer.cc) to build
// the name index of all characters of the world.
//   request:  quad MinimumCharacterID
//   response: quad NumberOfPlayers, then per player string Name,
//             quad CharacterID
// The caller has room for 10000 entries. It asks again with the last id + 1
// while it receives exactly 10000, so the answer is ordered by id.
void Dispatcher::handleLoadPlayers(Session &session, ReadBuffer &in, WriteBuffer &out) {
	uint32_t minimumId = in.readQuad();
	std::vector<PlayerEntry> players = Store.listPlayers(requireWorld(session), minimumId,
		MAX_PLAYERS_PER_LOAD);
	out.writeByte(STATUS_OK);
	out.writeQuad(static_cast<uint32_t>(players.size()));
	for (const PlayerEntry &player : players) {
		out.writeString(cut(player.name));
		out.writeQuad(player.id);
	}
	Detail = format("minimum=%u players=%zu", minimumId, players.size());
}

// Query 20, loginGame, called by PerformRegistration (src/communication.cc)
// on a pool connection when a client logs in.
//   request:  quad AccountID, string PlayerName, string Password,
//             string IPAddress, flag PrivateWorld,
//             flag PremiumAccountRequired, flag GamemasterRequired
//   response, OK:
//             quad CharacterID, string PlayerName, byte Sex, string Guild,
//             string Rank, string Title, byte NumberOfBuddies, then per
//             buddy quad CharacterID, string Name; byte NumberOfRights, then
//             per right string Name; flag PremiumAccountActivated
//   response, ERROR: byte code, see LoginGameError
// The game always sends PremiumAccountRequired = false and passes its
// "State = private" setting as PrivateWorld. It overwrites its copy of the
// name with the returned one. Rights arrive by name; "PREMIUM_ACCOUNT" is
// the one the game tests to tell premium from free accounts. A successful
// login counts the character as online; the game takes that back with
// decrementIsOnline if it cannot let the player in after all.
//
// The game server does not show what decides each error code; the checks
// below follow the texts the game shows for the codes:
//  - INVITED and ALLOW_MULTICLIENT are right names the game ignores when
//    they arrive, so they are taken to be the rights the query manager
//    itself checks for private worlds and for a second character online.
//  - A character without an account can only be asked for with account 0.
//    The game then refuses it itself ("not assigned to an account") without
//    calling decrementIsOnline, so that login is not counted as online.
//  - Codes 7 and 9 (temporary blocks after failed logins) are never sent.
void Dispatcher::handleLoginGame(Session &session, ReadBuffer &in, WriteBuffer &out) {
	uint32_t accountId = in.readQuad();
	std::string name = in.readString();
	std::string password = in.readString();
	std::string address = in.readString();
	bool privateWorld = in.readFlag();
	bool premiumRequired = in.readFlag();
	bool gamemasterRequired = in.readFlag();
	uint32_t worldId = requireWorld(session);
	uint32_t now = Now();

	if (premiumRequired) {
		// No caller sets this and no error code is known for it.
		throw RequestFailed("PremiumAccountRequired is not supported");
	}

	std::optional<CharacterRecord> character = Store.findCharacterByName(name);
	if (!character) {
		Detail = "character unknown";
		return writeError(out, LOGIN_CHARACTER_UNKNOWN);
	}
	Detail = format("character=%u account=%u", character->id, accountId);
	if (character->deleted) {
		return writeError(out, LOGIN_CHARACTER_DELETED);
	}
	if (character->worldId != worldId) {
		return writeError(out, LOGIN_WRONG_WORLD);
	}
	if (character->accountId != accountId) {
		return writeError(out, LOGIN_WRONG_ACCOUNT);
	}

	std::vector<std::string> rights = Store.listCharacterRights(character->id);
	std::optional<AccountRecord> account;
	if (accountId != 0) {
		account = Store.findAccount(accountId);
		if (!account || account->deleted) {
			return writeError(out, LOGIN_ACCOUNT_DELETED);
		}
		if (account->password != password) {
			return writeError(out, LOGIN_WRONG_PASSWORD);
		}
		if (account->banishedUntil > now) {
			return writeError(out, LOGIN_ACCOUNT_BANISHED);
		}
		for (const std::string &right : Store.listAccountRights(accountId)) {
			if (!contains(rights, right.c_str())) {
				rights.push_back(right);
			}
		}
		if (account->premiumUntil > now && !contains(rights, "PREMIUM_ACCOUNT")) {
			rights.push_back("PREMIUM_ACCOUNT");
		}
	}
	if (character->namelocked) {
		return writeError(out, LOGIN_NAMELOCKED);
	}
	if (Store.isIpBanished(address, now)) {
		return writeError(out, LOGIN_IP_BANISHED);
	}
	if (privateWorld && !contains(rights, "INVITED")) {
		return writeError(out, LOGIN_NOT_INVITED);
	}
	if (gamemasterRequired && (!account || !account->gamemaster)) {
		return writeError(out, LOGIN_NOT_GAMEMASTER);
	}
	std::vector<PlayerEntry> buddies;
	if (account) {
		if (Store.countOnlineCharacters(accountId, character->id) > 0
				&& !contains(rights, "ALLOW_MULTICLIENT")) {
			return writeError(out, LOGIN_ALREADY_ONLINE);
		}
		buddies = Store.listBuddies(accountId);
		if (buddies.size() > MAX_BUDDIES) {
			buddies.resize(MAX_BUDDIES);
		}
		Store.incrementIsOnline(character->id);
	}
	if (rights.size() > 255) {
		throw RequestFailed("more than 255 rights");
	}

	out.writeByte(STATUS_OK);
	out.writeQuad(character->id);
	out.writeString(cut(character->name));
	out.writeByte(character->sex);
	out.writeString(cut(character->guild));
	out.writeString(cut(character->rank));
	out.writeString(cut(character->title));
	out.writeByte(static_cast<uint8_t>(buddies.size()));
	for (const PlayerEntry &buddy : buddies) {
		out.writeQuad(buddy.id);
		out.writeString(cut(buddy.name));
	}
	out.writeByte(static_cast<uint8_t>(rights.size()));
	for (const std::string &right : rights) {
		out.writeString(right);
	}
	// PremiumAccountActivated: this login starts premium time that was bought
	// but not yet running. There is no payment system here, so it is false.
	out.writeFlag(false);
	Detail += format(" buddies=%zu rights=%zu", buddies.size(), rights.size());
}

// Query 21, logoutGame, called by ProcessLogoutOrder (src/writer.cc) on the
// writer thread's connection when a player leaves.
//   request:  quad CharacterID, word Level, string Profession,
//             string Residence, quad LastLoginTime, word TutorActivities
//   response: status only
void Dispatcher::handleLogoutGame(Session &session, ReadBuffer &in, WriteBuffer &out) {
	uint32_t characterId = in.readQuad();
	uint16_t level = in.readWord();
	std::string profession = in.readString();
	std::string residence = in.readString();
	uint32_t lastLogin = in.readQuad();
	uint16_t tutorActivities = in.readWord();
	requireWorld(session);

	if (!Store.recordLogout(characterId, level, profession, residence, lastLogin, tutorActivities)) {
		throw RequestFailed(format("unknown character %u", characterId));
	}
	Store.decrementIsOnline(characterId);
	out.writeByte(STATUS_OK);
	Detail = format("character=%u level=%u tutor=%u", characterId, static_cast<unsigned>(level),
		static_cast<unsigned>(tutorActivities));
}

// Query 32, decrementIsOnline, called by PerformRegistration and HandleLogin
// when a login that loginGame accepted does not lead to the player entering
// the game (connection gone, no free slot, waiting list, premium-only
// world).
//   request:  quad CharacterID
//   response: status only
void Dispatcher::handleDecrementIsOnline(Session &session, ReadBuffer &in, WriteBuffer &out) {
	uint32_t characterId = in.readQuad();
	requireWorld(session);
	if (!Store.decrementIsOnline(characterId)) {
		throw RequestFailed(format("unknown character %u", characterId));
	}
	out.writeByte(STATUS_OK);
	Detail = format("character=%u", characterId);
}

// Query 46, clearIsOnline, called by ClearPlayers (src/writer.cc), which
// InitWriter calls at start.
//   request:  nothing
//   response: word NumberOfAffectedPlayers
// The game logs an error if the number is not zero ("%d players were marked
// as logged in").
void Dispatcher::handleClearIsOnline(Session &session, ReadBuffer &, WriteBuffer &out) {
	uint32_t affected = Store.clearIsOnline(requireWorld(session));
	out.writeByte(STATUS_OK);
	out.writeWord(static_cast<uint16_t>(std::min<uint32_t>(affected, 0xFFFF)));
	Detail = format("affected=%u", affected);
}

// Query 47, createPlayerlist, called by ProcessPlayerlistOrder
// (src/writer.cc); the order comes from CreatePlayerList (src/crplayer.cc).
//   request:  word NumberOfPlayers, then per player string Name, word Level,
//             string Profession
//   response: flag NewRecord
// NumberOfPlayers is 0xFFFF, with no entries, when the game calls
// CreatePlayerList(false): the world is going offline. NewRecord makes the
// game broadcast "New record: %d players are logged in."
void Dispatcher::handleCreatePlayerlist(Session &session, ReadBuffer &in, WriteBuffer &out) {
	uint32_t worldId = requireWorld(session);
	uint16_t count = in.readWord();
	bool newRecord = false;
	if (count == 0xFFFF) {
		Store.replaceOnlinePlayers(worldId, std::vector<OnlinePlayer>());
		Store.setWorldOnline(worldId, false);
		Detail = "world offline";
	} else {
		std::vector<OnlinePlayer> players;
		players.reserve(count);
		for (uint16_t i = 0; i < count; i++) {
			OnlinePlayer player;
			player.name = in.readString();
			player.level = in.readWord();
			player.profession = in.readString();
			players.push_back(player);
		}
		std::optional<WorldRecord> world = Store.getWorld(worldId);
		if (!world) {
			throw RequestFailed("world record is gone");
		}
		Store.replaceOnlinePlayers(worldId, players);
		Store.setWorldOnline(worldId, true);
		if (count > world->onlineRecord) {
			Store.setOnlineRecord(worldId, count, Now());
			newRecord = true;
		}
		Detail = format("players=%u%s", static_cast<unsigned>(count), newRecord ? " new record" : "");
	}
	out.writeByte(STATUS_OK);
	out.writeFlag(newRecord);
}

// Query 48, logKilledCreatures, called by ProcessKillStatisticsOrder
// (src/writer.cc).
//   request:  word NumberOfRaces, then per race string Name,
//             quad KilledPlayers, quad KilledCreatures
//   response: status only
void Dispatcher::handleLogKilledCreatures(Session &session, ReadBuffer &in, WriteBuffer &out) {
	uint32_t worldId = requireWorld(session);
	uint16_t count = in.readWord();
	std::vector<KilledCreatures> entries;
	entries.reserve(count);
	for (uint16_t i = 0; i < count; i++) {
		KilledCreatures entry;
		entry.race = in.readString();
		entry.killedPlayers = in.readQuad();
		entry.killedCreatures = in.readQuad();
		entries.push_back(entry);
	}
	Store.addKilledCreatures(worldId, entries, Now());
	out.writeByte(STATUS_OK);
	Detail = format("races=%u", static_cast<unsigned>(count));
}

// Query 29, logCharacterDeath, called by ProcessCharacterDeathOrder
// (src/writer.cc).
//   request:  quad CharacterID, word Level, quad Offender, string Remark,
//             flag Unjustified, quad Time
//   response: status only
void Dispatcher::handleLogCharacterDeath(Session &session, ReadBuffer &in, WriteBuffer &out) {
	CharacterDeath death;
	death.characterId = in.readQuad();
	death.level = in.readWord();
	death.offenderId = in.readQuad();
	death.remark = in.readString();
	death.unjustified = in.readFlag();
	death.time = in.readQuad();
	requireWorld(session);
	Store.addCharacterDeath(death);
	out.writeByte(STATUS_OK);
	Detail = format("character=%u offender=%u", death.characterId, death.offenderId);
}

// Query 30, addBuddy, called by ProcessAddBuddyOrder (src/writer.cc).
//   request:  quad AccountID, quad Buddy (a character id)
//   response: status only
void Dispatcher::handleAddBuddy(Session &session, ReadBuffer &in, WriteBuffer &out) {
	uint32_t accountId = in.readQuad();
	uint32_t buddyId = in.readQuad();
	requireWorld(session);
	Store.addBuddy(accountId, buddyId);
	out.writeByte(STATUS_OK);
	Detail = format("account=%u buddy=%u", accountId, buddyId);
}

// Query 31, removeBuddy, called by ProcessRemoveBuddyOrder (src/writer.cc).
//   request:  quad AccountID, quad Buddy
//   response: status only
void Dispatcher::handleRemoveBuddy(Session &session, ReadBuffer &in, WriteBuffer &out) {
	uint32_t accountId = in.readQuad();
	uint32_t buddyId = in.readQuad();
	requireWorld(session);
	Store.removeBuddy(accountId, buddyId);
	out.writeByte(STATUS_OK);
	Detail = format("account=%u buddy=%u", accountId, buddyId);
}

// Query 45, insertHouses, called by LoadHouses (src/houses.cc) at every
// start with all houses of houses.toml in its data folder (houses.dat before).
//   request:  word NumberOfHouses, then per house word HouseID, string Name,
//             quad Rent, string Description, word Size, word PositionX,
//             word PositionY, byte PositionZ, string Town, flag Guildhouse
//   response: status only
void Dispatcher::handleInsertHouses(Session &session, ReadBuffer &in, WriteBuffer &out) {
	uint32_t worldId = requireWorld(session);
	uint16_t count = in.readWord();
	std::vector<HouseRecord> houses;
	houses.reserve(count);
	for (uint16_t i = 0; i < count; i++) {
		HouseRecord house;
		house.id = in.readWord();
		house.name = in.readString();
		house.rent = in.readQuad();
		house.description = in.readString();
		house.size = in.readWord();
		house.x = in.readWord();
		house.y = in.readWord();
		house.z = in.readByte();
		house.town = in.readString();
		house.guildhouse = in.readFlag();
		houses.push_back(house);
	}
	Store.replaceHouses(worldId, houses);
	out.writeByte(STATUS_OK);
	Detail = format("houses=%u", static_cast<unsigned>(count));
}

// Query 42, getHouseOwners, called by LoadOwners (src/houses.cc) and
// StartAuctions.
//   request:  nothing
//   response: word NumberOfHouses, then per house word HouseID,
//             quad OwnerID, string OwnerName, quad PaidUntil
// The game fails the request if it gets more entries than it has houses.
// LoadOwners takes only the owner names from the answer (owner ids come from
// dat/owners.dat). StartAuctions compares owner and PaidUntil with its own
// state and then sends insert/update/deleteHouseOwner for the differences.
void Dispatcher::handleGetHouseOwners(Session &session, ReadBuffer &, WriteBuffer &out) {
	std::vector<HouseOwner> owners = Store.listHouseOwners(requireWorld(session));
	out.writeByte(STATUS_OK);
	out.writeWord(static_cast<uint16_t>(owners.size()));
	for (const HouseOwner &owner : owners) {
		out.writeWord(owner.houseId);
		out.writeQuad(owner.ownerId);
		out.writeString(cut(owner.ownerName));
		out.writeQuad(owner.paidUntil);
	}
	Detail = format("owners=%zu", owners.size());
}

// Query 39, insertHouseOwner, called by StartAuctions for a rented house the
// query manager did not list.
//   request:  word HouseID, quad OwnerID, quad PaidUntil
//   response: status only
void Dispatcher::handleInsertHouseOwner(Session &session, ReadBuffer &in, WriteBuffer &out) {
	uint16_t houseId = in.readWord();
	uint32_t ownerId = in.readQuad();
	uint32_t paidUntil = in.readQuad();
	if (!Store.insertHouseOwner(requireWorld(session), houseId, ownerId, paidUntil)) {
		throw RequestFailed(format("house %u already has an owner entry",
			static_cast<unsigned>(houseId)));
	}
	out.writeByte(STATUS_OK);
	Detail = format("house=%u owner=%u", static_cast<unsigned>(houseId), ownerId);
}

// Query 40, updateHouseOwner, called by StartAuctions for a listed house
// whose owner or PaidUntil differs.
//   request:  word HouseID, quad OwnerID, quad PaidUntil
//   response: status only
// StartAuctions sends this for every listed house that differs, also for
// one whose tenant it has just thrown out: such a house first gets
// deleteHouseOwner and then updateHouseOwner with OwnerID 0. The entry is
// gone by then, so an update that finds no entry has to succeed without
// doing anything, like an SQL UPDATE that matches no row. Answering it with
// a failure makes the game log "Cannot update data of rented house %d" for
// every evicted house (seen in a run of the game server).
void Dispatcher::handleUpdateHouseOwner(Session &session, ReadBuffer &in, WriteBuffer &out) {
	uint16_t houseId = in.readWord();
	uint32_t ownerId = in.readQuad();
	uint32_t paidUntil = in.readQuad();
	bool existed = Store.updateHouseOwner(requireWorld(session), houseId, ownerId, paidUntil);
	out.writeByte(STATUS_OK);
	Detail = format("house=%u owner=%u%s", static_cast<unsigned>(houseId), ownerId,
		existed ? "" : " (no entry)");
}

// Query 41, deleteHouseOwner, called by CollectRents for the houses it
// emptied because of a transfer or one of the three evict lists, and by
// StartAuctions for a listed house that has no owner any more (a tenant
// thrown out for the rent).
//   request:  word HouseID
//   response: status only
// A missing entry is not an error, as with updateHouseOwner: the house has
// no owner entry afterwards either way.
void Dispatcher::handleDeleteHouseOwner(Session &session, ReadBuffer &in, WriteBuffer &out) {
	uint16_t houseId = in.readWord();
	bool existed = Store.deleteHouseOwner(requireWorld(session), houseId);
	out.writeByte(STATUS_OK);
	Detail = format("house=%u%s", static_cast<unsigned>(houseId), existed ? "" : " (no entry)");
}

// Query 43, getAuctions, called by StartAuctions.
//   request:  nothing
//   response: word NumberOfAuctions, then that many word HouseID
// The game starts an auction for every house without an owner that is not
// in this list.
void Dispatcher::handleGetAuctions(Session &session, ReadBuffer &, WriteBuffer &out) {
	std::vector<uint16_t> houses = Store.listAuctions(requireWorld(session));
	out.writeByte(STATUS_OK);
	out.writeWord(static_cast<uint16_t>(houses.size()));
	for (uint16_t houseId : houses) {
		out.writeWord(houseId);
	}
	Detail = format("auctions=%zu", houses.size());
}

// Query 44, startAuction, called by StartAuctions.
//   request:  word HouseID
//   response: status only
void Dispatcher::handleStartAuction(Session &session, ReadBuffer &in, WriteBuffer &out) {
	uint16_t houseId = in.readWord();
	if (!Store.startAuction(requireWorld(session), houseId)) {
		throw RequestFailed(format("house %u is unknown or already up for auction",
			static_cast<unsigned>(houseId)));
	}
	out.writeByte(STATUS_OK);
	Detail = format("house=%u", static_cast<unsigned>(houseId));
}

// Query 33, finishAuctions, called by FinishAuctions (src/houses.cc) at
// start.
//   request:  nothing
//   response: word NumberOfAuctions, then per auction word HouseID,
//             quad CharacterID, string CharacterName, quad Bid
// The game gives each house to the named character and takes the bid and
// the first rent from the depot. It logs "on reset: INSERT INTO
// HouseAssignments ..." for each entry, i.e. after a reset of the game state
// the entry would have to be put back: the query manager hands out a
// finished auction once.
void Dispatcher::handleFinishAuctions(Session &session, ReadBuffer &, WriteBuffer &out) {
	std::vector<AuctionResult> results = Store.takeFinishedAuctions(requireWorld(session), Now());
	out.writeByte(STATUS_OK);
	out.writeWord(static_cast<uint16_t>(results.size()));
	for (const AuctionResult &result : results) {
		out.writeWord(result.houseId);
		out.writeQuad(result.characterId);
		out.writeString(cut(result.characterName));
		out.writeQuad(result.bid);
	}
	Detail = format("finished=%zu", results.size());
}

// Query 23, setNamelock, called by ProcessPunishmentOrder (the rule violation
// dialog) and by the admin port's namelock.
//   request:  quad GamemasterID, string PlayerName, string IPAddress,
//             string Reason, string Comment
//   response: OK: nothing. ERROR: byte 1 (no such character), 3 (it is
//             namelocked already). (2 and 4, "already approved", never:
//             this server has no approval of names.)
// The character cannot log in until it is renamed (renameCharacter lifts
// the lock).
void Dispatcher::handleSetNamelock(Session &, ReadBuffer &in, WriteBuffer &out) {
	uint32_t gamemasterId = in.readQuad();
	std::string name = in.readString();
	in.readString();
	std::string reason = in.readString();
	in.readString();
	std::optional<CharacterRecord> character = Store.findCharacterByName(name);
	if (!character || character->deleted) {
		return writeError(out, 1);
	}
	if (character->namelocked) {
		return writeError(out, 3);
	}
	character->namelocked = true;
	Store.putCharacter(*character);
	out.writeByte(STATUS_OK);
	Detail = format("character=%u gamemaster=%u reason=%s", character->id, gamemasterId, reason.c_str());
}

// Query 25, banishAccount, called by ProcessPunishmentOrder (the rule
// violation dialog).
//   request:  quad GamemasterID, string PlayerName, string IPAddress,
//             string Reason, string Comment, flag FinalWarning
//   response: OK: quad BanishmentID, byte Days (0xFF = for good), flag
//             FinalWarning. ERROR: byte 1 (no such character, or one
//             without an account), 2 (a gamemaster's account), 3 (the
//             account is banished already).
// The length follows the account's earlier banishments (FIRST_BANISHMENT_DAYS
// and the other constants above).
void Dispatcher::handleBanishAccount(Session &, ReadBuffer &in, WriteBuffer &out) {
	uint32_t gamemasterId = in.readQuad();
	std::string name = in.readString();
	in.readString();
	std::string reason = in.readString();
	std::string comment = in.readString();
	bool finalWarning = in.readFlag();
	uint32_t now = Now();
	std::optional<CharacterRecord> character = Store.findCharacterByName(name);
	if (!character || character->deleted || character->accountId == 0) {
		return writeError(out, 1);
	}
	if (isGamemasterAccount(Store, *character)) {
		return writeError(out, 2);
	}
	std::optional<AccountRecord> account = Store.findAccount(character->accountId);
	if (!account) {
		return writeError(out, 1);
	}
	if (account->banishedUntil > now) {
		return writeError(out, 3);
	}
	BanishmentHistory history = Store.banishmentHistory(account->id);
	uint32_t days;
	if (history.finalWarning) {
		days = 0;
		finalWarning = false;
	} else if (finalWarning || history.count >= 2) {
		days = FINAL_WARNING_DAYS;
		finalWarning = true;
	} else {
		days = history.count == 0 ? FIRST_BANISHMENT_DAYS : LATER_BANISHMENT_DAYS;
	}
	BanishmentRecord banishment;
	banishment.accountId = account->id;
	banishment.characterId = character->id;
	banishment.gamemasterId = gamemasterId;
	banishment.reason = reason;
	banishment.comment = comment;
	banishment.time = now;
	banishment.until = banishmentEnd(now, days);
	banishment.finalWarning = finalWarning;
	uint32_t id = Store.addBanishment(banishment);
	out.writeByte(STATUS_OK);
	out.writeQuad(id);
	out.writeByte(days == 0 ? 0xFF : static_cast<uint8_t>(days));
	out.writeFlag(finalWarning);
	Detail = format("account=%u character=%u days=%u final_warning=%d", account->id, character->id, days,
		finalWarning ? 1 : 0);
}

// Query 26, setNotation, called by ProcessPunishmentOrder (the rule violation
// dialog) and by the admin port's notation.
//   request:  quad GamemasterID, string PlayerName, string IPAddress,
//             string Reason, string Comment
//   response: OK: quad NotationID. ERROR: byte 1 (no such character), 2 (a
//             gamemaster's account).
void Dispatcher::handleSetNotation(Session &, ReadBuffer &in, WriteBuffer &out) {
	uint32_t gamemasterId = in.readQuad();
	std::string name = in.readString();
	in.readString();
	std::string reason = in.readString();
	std::string comment = in.readString();
	std::optional<CharacterRecord> character = Store.findCharacterByName(name);
	if (!character || character->deleted) {
		return writeError(out, 1);
	}
	if (isGamemasterAccount(Store, *character)) {
		return writeError(out, 2);
	}
	NotationRecord notation;
	notation.characterId = character->id;
	notation.gamemasterId = gamemasterId;
	notation.reason = reason;
	notation.comment = comment;
	notation.time = Now();
	uint32_t id = Store.addNotation(notation);
	out.writeByte(STATUS_OK);
	out.writeQuad(id);
	Detail = format("character=%u notation=%u", character->id, id);
}

// Query 28, banishIPAddress, called by ProcessPunishmentOrder (the rule
// violation dialog).
//   request:  quad GamemasterID, string PlayerName, string IPAddress,
//             string Reason, string Comment
//   response: OK: nothing. ERROR: byte 1 (no such character, or no IPv4
//             address), 2 (a gamemaster's account).
// Blocks the address for IP_BANISHMENT_DAYS.
void Dispatcher::handleBanishIPAddress(Session &, ReadBuffer &in, WriteBuffer &out) {
	in.readQuad();
	std::string name = in.readString();
	std::string address = in.readString();
	in.readString();
	in.readString();
	std::optional<CharacterRecord> character = Store.findCharacterByName(name);
	if (!character || character->deleted || !isIpAddress(address)) {
		return writeError(out, 1);
	}
	if (isGamemasterAccount(Store, *character)) {
		return writeError(out, 2);
	}
	Store.banishIpAddress(address, banishmentEnd(Now(), IP_BANISHMENT_DAYS));
	out.writeByte(STATUS_OK);
	Detail = format("character=%u address=%s days=%u", character->id, address.c_str(), IP_BANISHMENT_DAYS);
}

// Query 56, banishAccountFor, this project's own (the admin port's ban):
// banishes the account of a character for a length the caller chooses.
//   request:  quad GamemasterID, string PlayerName, string Reason,
//             string Comment, quad Days (0, or more than
//             MAX_BANISHMENT_DAYS: for good)
//   response: OK: quad BanishmentID, quad Until (4294967295: for good).
//             ERROR: byte 1 (no such character, or one without an
//             account), 2 (a gamemaster's account).
// A banishment running already ends at the later of the two ends.
void Dispatcher::handleBanishAccountFor(Session &, ReadBuffer &in, WriteBuffer &out) {
	uint32_t gamemasterId = in.readQuad();
	std::string name = in.readString();
	std::string reason = in.readString();
	std::string comment = in.readString();
	uint32_t days = in.readQuad();
	uint32_t now = Now();
	std::optional<CharacterRecord> character = Store.findCharacterByName(name);
	if (!character || character->deleted || character->accountId == 0 || !Store.findAccount(character->accountId)) {
		return writeError(out, 1);
	}
	if (isGamemasterAccount(Store, *character)) {
		return writeError(out, 2);
	}
	BanishmentRecord banishment;
	banishment.accountId = character->accountId;
	banishment.characterId = character->id;
	banishment.gamemasterId = gamemasterId;
	banishment.reason = reason;
	banishment.comment = comment;
	banishment.time = now;
	banishment.until = banishmentEnd(now, days);
	uint32_t id = Store.addBanishment(banishment);
	std::optional<AccountRecord> account = Store.findAccount(character->accountId);
	out.writeByte(STATUS_OK);
	out.writeQuad(id);
	out.writeQuad(account->banishedUntil);
	Detail = format("account=%u character=%u days=%u", character->accountId, character->id, days);
}

// Query 57, liftBanishment, this project's own (the admin port's unban):
// ends the banishment of a character's account at once.
//   request:  string PlayerName
//   response: OK: nothing. ERROR: byte 1 (no such character, or one without
//             an account), 2 (the account is not banished).
void Dispatcher::handleLiftBanishment(Session &, ReadBuffer &in, WriteBuffer &out) {
	std::string name = in.readString();
	std::optional<CharacterRecord> character = Store.findCharacterByName(name);
	if (!character || character->deleted || character->accountId == 0) {
		return writeError(out, 1);
	}
	if (!Store.liftBanishment(character->accountId, Now())) {
		return writeError(out, 2);
	}
	out.writeByte(STATUS_OK);
	Detail = format("account=%u character=%u", character->accountId, character->id);
}

// Query 58, banishIPAddressFor, this project's own (the admin port's banip):
// blocks an address for a length the caller chooses.
//   request:  quad GamemasterID, string IPAddress, string Reason,
//             string Comment, quad Days (0, or more than
//             MAX_BANISHMENT_DAYS: for good)
//   response: OK: quad Until. ERROR: byte 1 (no IPv4 address).
void Dispatcher::handleBanishIPAddressFor(Session &, ReadBuffer &in, WriteBuffer &out) {
	in.readQuad();
	std::string address = in.readString();
	in.readString();
	in.readString();
	uint32_t days = in.readQuad();
	if (!isIpAddress(address)) {
		return writeError(out, 1);
	}
	uint32_t until = banishmentEnd(Now(), days);
	Store.banishIpAddress(address, until);
	out.writeByte(STATUS_OK);
	out.writeQuad(until);
	Detail = format("address=%s days=%u", address.c_str(), days);
}

// Query 59, renameCharacter, this project's own (the admin port's rename):
// gives a character a new name (its player data's too) and lifts its
// namelock. The game checks the name's form; the character is not online.
//   request:  quad CharacterID, string NewName
//   response: OK: nothing. ERROR: byte 1 (no such character), 2 (another
//             character has the name, in any case), 3 (an empty name or
//             one longer than the game reads).
void Dispatcher::handleRenameCharacter(Session &, ReadBuffer &in, WriteBuffer &out) {
	uint32_t characterId = in.readQuad();
	std::string name = in.readString();
	std::optional<CharacterRecord> character = Store.findCharacter(characterId);
	if (!character || character->deleted) {
		return writeError(out, 1);
	}
	if (name.empty() || name.size() > MAX_NAME_LENGTH) {
		return writeError(out, 3);
	}
	if (!Store.renameCharacter(characterId, name)) {
		return writeError(out, 2);
	}
	out.writeByte(STATUS_OK);
	Detail = format("character=%u", characterId);
}

// Query 51, excludeFromAuctions, called by FinishAuctions with
// Banish = true for an auction winner who cannot pay, and by CollectRents
// with Banish = false for a tenant who is thrown out for not paying the rent.
//   request:  quad CharacterID, flag Banish
//   response: status only
// The game server does not show how long a banished character stays
// excluded; the exclusion is recorded with its time and the flag.
void Dispatcher::handleExcludeFromAuctions(Session &session, ReadBuffer &in, WriteBuffer &out) {
	uint32_t characterId = in.readQuad();
	bool banish = in.readFlag();
	Store.excludeFromAuctions(requireWorld(session), characterId, banish, Now());
	out.writeByte(STATUS_OK);
	Detail = format("character=%u banish=%d", characterId, banish ? 1 : 0);
}

// Query 35, transferHouses, called by CollectRents at start ("Bearbeite
// freiwillige Kuendigungen").
//   request:  nothing
//   response: word NumberOfTransfers, then per transfer word HouseID,
//             quad NewOwnerID, string NewOwnerName, quad Price
// NewOwnerID 0 means the owner moves out. Otherwise the game moves the house
// to the new owner and takes Price from that character's depot; if the money
// is missing it sends cancelHouseTransfer. The game removes the owner entry
// itself afterwards (deleteHouseOwner), so nothing is changed here.
void Dispatcher::handleTransferHouses(Session &session, ReadBuffer &, WriteBuffer &out) {
	std::vector<HouseTransfer> transfers = Store.listHouseTransfers(requireWorld(session), Now());
	out.writeByte(STATUS_OK);
	out.writeWord(static_cast<uint16_t>(transfers.size()));
	for (const HouseTransfer &transfer : transfers) {
		out.writeWord(transfer.houseId);
		out.writeQuad(transfer.newOwnerId);
		out.writeString(cut(transfer.newOwnerName));
		out.writeQuad(transfer.price);
	}
	Detail = format("transfers=%zu", transfers.size());
}

// Query 52, cancelHouseTransfer, called by CollectRents when the buyer of a
// house cannot pay the price.
//   request:  word HouseID
//   response: status only
// The game logs the statement this stands for: "UPDATE HouseOwners SET
// Termination=null,NewOwner=null,Accepted=0,Price=null WHERE ...". Like
// updateHouseOwner it succeeds when there is no entry to change.
void Dispatcher::handleCancelHouseTransfer(Session &session, ReadBuffer &in, WriteBuffer &out) {
	uint16_t houseId = in.readWord();
	bool existed = Store.cancelHouseTransfer(requireWorld(session), houseId);
	out.writeByte(STATUS_OK);
	Detail = format("house=%u%s", static_cast<unsigned>(houseId), existed ? "" : " (no entry)");
}

// Query 36, evictFreeAccounts, called by CollectRents at start.
//   request:  nothing
//   response: word NumberOfEvictions, then per eviction word HouseID,
//             quad OwnerID
// The game empties each listed house whose owner id still matches ("Account
// of house %d is no longer paid"). An owner whose character has no account
// here is not listed: nothing is known about that account.
void Dispatcher::handleEvictFreeAccounts(Session &session, ReadBuffer &, WriteBuffer &out) {
	std::vector<HouseEviction> evictions = Store.listHousesOfFreeAccounts(requireWorld(session), Now());
	out.writeByte(STATUS_OK);
	out.writeWord(static_cast<uint16_t>(evictions.size()));
	for (const HouseEviction &eviction : evictions) {
		out.writeWord(eviction.houseId);
		out.writeQuad(eviction.ownerId);
	}
	Detail = format("evictions=%zu", evictions.size());
}

// Query 37, evictDeletedCharacters, called by CollectRents at start.
//   request:  nothing
//   response: word NumberOfEvictions, then that many word HouseID
// The game empties each listed house ("Owner of house %d is deleted").
void Dispatcher::handleEvictDeletedCharacters(Session &session, ReadBuffer &, WriteBuffer &out) {
	std::vector<uint16_t> houses = Store.listHousesOfDeletedCharacters(requireWorld(session));
	out.writeByte(STATUS_OK);
	out.writeWord(static_cast<uint16_t>(houses.size()));
	for (uint16_t houseId : houses) {
		out.writeWord(houseId);
	}
	Detail = format("evictions=%zu", houses.size());
}

// Query 38, evictExGuildleaders, called by CollectRents at start if at least
// one guildhouse is rented.
//   request:  word NumberOfGuildhouses, then per guildhouse word HouseID,
//             quad Guildleader (the owner of the house)
//   response: word NumberOfEvictions, then that many word HouseID
// The game empties each listed house ("Tenant of guildhouse %d is no longer
// a guild leader"). A house is listed if its owner is recorded as a
// member of a guild without being its leader. An owner with no guild
// recorded is not listed: the installation has no guild data, so that says
// nothing about the character.
void Dispatcher::handleEvictExGuildleaders(Session &session, ReadBuffer &in, WriteBuffer &out) {
	requireWorld(session);
	uint16_t count = in.readWord();
	std::vector<uint16_t> evictions;
	for (uint16_t i = 0; i < count; i++) {
		uint16_t houseId = in.readWord();
		uint32_t leaderId = in.readQuad();
		std::optional<CharacterRecord> character = Store.findCharacter(leaderId);
		if (character && !character->guild.empty() && !character->guildLeader) {
			evictions.push_back(houseId);
		}
	}
	out.writeByte(STATUS_OK);
	out.writeWord(static_cast<uint16_t>(evictions.size()));
	for (uint16_t houseId : evictions) {
		out.writeWord(houseId);
	}
	Detail = format("guildhouses=%u evictions=%zu", static_cast<unsigned>(count), evictions.size());
}

// The player data requests may name a character of the session's world, or
// one the characters table does not know (the game saves what it loaded, also
// for a house owner it was not told about); not one of another world.
void Dispatcher::requirePlayerDataWorld(uint32_t characterId, uint32_t worldId) {
	std::optional<CharacterRecord> character = Store.findCharacter(characterId);
	if (character && character->worldId != worldId) {
		throw RequestFailed(format("character %u belongs to another world", characterId));
	}
}

// Query 54, loadPlayerData. This project's own: LoadPlayerData
// (src/dbfuncs.cc) sends it on one of the game's player data connections
// when it needs a character's data that is not in its player data pool: at
// login, for a house owner, for mail to an offline character.
//   request:  quad CharacterID
//   response: flag Stored, then if Stored the record of playerdata.hh
// Stored is false for a character with no player data; the game starts it as
// a new character then, as it did for a missing player file. Any failure is
// STATUS_FAILED, on which the game refuses the login: a character must never
// start as new because its data could not be read.
void Dispatcher::handleLoadPlayerData(Session &session, ReadBuffer &in, WriteBuffer &out) {
	uint32_t characterId = in.readQuad();
	uint32_t worldId = requireWorld(session);
	requirePlayerDataWorld(characterId, worldId);
	std::optional<PlayerData> data = Store.loadPlayerData(characterId);
	out.writeByte(STATUS_OK);
	out.writeFlag(data.has_value());
	if (data) {
		writePlayerData(out, *data);
	}
	Detail = format("character=%u %s", characterId, data ? "stored" : "none stored");
}

// Query 55, savePlayerData. This project's own: SavePlayerData
// (src/dbfuncs.cc) sends it on one of the game's player data connections
// when it writes a character's data back: from the periodic save of the
// writer thread, when a pool slot is reused, and when the game shuts down.
//   request:  quad CharacterID, then the record of playerdata.hh
//   response: status only
// The record replaces what was stored. If the game gets no STATUS_OK, it
// keeps the record in a file of its own and sends it again later (see
// src/dbfuncs.cc).
// A request whose game has closed the connection is refused: the game gave
// up waiting (its 30 s timeout, while this server was frozen or its queue
// long) and keeps the record in its file, and it may have saved newer data
// of the character on another connection since, which this older record
// must not replace. The game closes the connection before it sends anything
// else, and this check runs under the dispatcher's lock: if a newer save was
// stored first, this connection is already seen closed here.
void Dispatcher::handleSavePlayerData(Session &session, ReadBuffer &in, WriteBuffer &out) {
	PlayerData data;
	data.characterId = in.readQuad();
	readPlayerData(in, data);
	uint32_t worldId = requireWorld(session);
	if (data.characterId == 0) {
		throw RequestFailed("character id 0");
	}
	requirePlayerDataWorld(data.characterId, worldId);
	if (clientClosed(session.socket)) {
		throw RequestFailed(format("character %u: the game closed the connection before the record was stored",
			data.characterId));
	}
	Store.savePlayerData(data);
	out.writeByte(STATUS_OK);
	Detail = format("character=%u skills=%zu depots=%zu", data.characterId, data.skills.size(),
		data.depots.size());
}

} // namespace qm
