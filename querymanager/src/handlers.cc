#include "handlers.hh"

#include "log.hh"

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

	if (logLevel() >= LogLevel::Debug) {
		// The login requests carry passwords; their bytes stay out of the log.
		bool secret = type == QUERY_LOGIN || type == QUERY_LOGIN_GAME || type == QUERY_LOGIN_ACCOUNT;
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
			response.size(), hexDump(response, 64).c_str());
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

// Query 0, login. Sent by TQueryManagerConnection::connect (0x80e04b0) right
// after the TCP connect, with a 30 second timeout.
//   request:  byte ApplicationType, string Password,
//             string LoginData (only if ApplicationType is 1)
//   response: status only
// connect() treats every status other than 0 as "Anmeldung fehlgeschlagen"
// and reads nothing else. The password is the one from the QueryManager
// entry in .tibia after DisguisePassword (config.c, 0x8052c10); for the game
// server LoginData is its world name (InitAll 0x804c6d0).
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

// Query 11, loginAccount. Not in bin/game: the request of this project's
// login server (loginserver/), which answers a client's login with the
// character list. Only the login server's application type may send it.
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

// Query 53, loadWorldConfig (0x80e1950), called once by LoadWorldConfig
// (main.c, 0x804c570) on a connection of its own.
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

// Query 50, loadPlayers (0x80e6590), called by InitPlayerIndex (crplayer.c,
// 0x80a8d80) to build the name index of all characters of the world.
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

// Query 20, loginGame (0x80e1c10), called by PerformRegistration
// (communication.c, 0x80dce70) on a pool connection when a client logs in.
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
// What decides each error code is not visible in bin/game; the checks below
// follow the texts the game shows for the codes:
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
	// Set by the original when this login starts premium time that was
	// bought but not yet running. There is no payment system here.
	out.writeFlag(false);
	Detail += format(" buddies=%zu rights=%zu", buddies.size(), rights.size());
}

// Query 21, logoutGame (0x80e3010), called by ProcessLogoutOrder (writer.c,
// 0x806a270) on the writer thread's connection when a player leaves.
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

// Query 32, decrementIsOnline (0x80e46e0), called by PerformRegistration
// (0x80dce70) and HandleLogin (0x80dd5b0) when a login that loginGame
// accepted does not lead to the player entering the game (connection gone,
// no free slot, waiting list, premium-only world).
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

// Query 46, clearIsOnline (0x80e6080), called by InitWriter (writer.c,
// 0x806b830) at start and by ClearPlayers (0x806b7e0).
//   request:  nothing
//   response: word NumberOfAffectedPlayers
// The game logs an error if the number is not zero ("%d Spieler waren als
// eingeloggt markiert").
void Dispatcher::handleClearIsOnline(Session &session, ReadBuffer &, WriteBuffer &out) {
	uint32_t affected = Store.clearIsOnline(requireWorld(session));
	out.writeByte(STATUS_OK);
	out.writeWord(static_cast<uint16_t>(std::min<uint32_t>(affected, 0xFFFF)));
	Detail = format("affected=%u", affected);
}

// Query 47, createPlayerlist (0x80e6170), called by ProcessPlayerlistOrder
// (writer.c, 0x806a340); the order comes from CreatePlayerList (crplayer.c,
// 0x80a76a0).
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

// Query 48, logKilledCreatures (0x80e63b0), called by
// ProcessKillStatisticsOrder (writer.c, 0x806a560).
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

// Query 29, logCharacterDeath (0x80e4230), called by
// ProcessCharacterDeathOrder (writer.c, 0x806b150).
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

// Query 30, addBuddy (0x80e4480), called by ProcessAddBuddyOrder (writer.c,
// 0x806b1e0).
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

// Query 31, removeBuddy (0x80e45b0), called by ProcessRemoveBuddyOrder
// (writer.c, 0x806b250).
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

// Query 45, insertHouses (0x80e5c80), called by LoadHouses (houses.c,
// 0x8065790) at every start with all houses of dat/houses.dat.
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

// Query 42, getHouseOwners (0x80e57c0), called by LoadOwners (houses.c,
// 0x80663d0) and StartAuctions (0x8064b30).
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

// Query 39, insertHouseOwner (0x80e53e0), called by StartAuctions
// (0x8064b30) for a rented house the query manager did not list.
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

// Query 40, updateHouseOwner (0x80e5550), called by StartAuctions
// (0x8064b30) for a listed house whose owner or PaidUntil differs.
//   request:  word HouseID, quad OwnerID, quad PaidUntil
//   response: status only
// StartAuctions sends this for every listed house that differs, also for
// one whose tenant it has just thrown out: such a house first gets
// deleteHouseOwner and then updateHouseOwner with OwnerID 0. The entry is
// gone by then, so an update that finds no entry has to succeed without
// doing anything, like an SQL UPDATE that matches no row. Answering it with
// a failure makes the game log "Kann Daten des vermieteten Hauses %d nicht
// aktualisieren" for every evicted house (seen when running bin/game).
void Dispatcher::handleUpdateHouseOwner(Session &session, ReadBuffer &in, WriteBuffer &out) {
	uint16_t houseId = in.readWord();
	uint32_t ownerId = in.readQuad();
	uint32_t paidUntil = in.readQuad();
	bool existed = Store.updateHouseOwner(requireWorld(session), houseId, ownerId, paidUntil);
	out.writeByte(STATUS_OK);
	Detail = format("house=%u owner=%u%s", static_cast<unsigned>(houseId), ownerId,
		existed ? "" : " (no entry)");
}

// Query 41, deleteHouseOwner (0x80e56c0), called by CollectRents (0x8063760)
// for the houses it emptied because of a transfer or one of the three evict
// lists, and by StartAuctions (0x8064b30) for a listed house that has no
// owner any more (a tenant thrown out for the rent).
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

// Query 43, getAuctions (0x80e5a10), called by StartAuctions (0x8064b30).
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

// Query 44, startAuction (0x80e5b80), called by StartAuctions (0x8064b30).
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

// Query 33, finishAuctions (0x80e47e0), called by FinishAuctions (houses.c,
// 0x80630a0) at start.
//   request:  nothing
//   response: word NumberOfAuctions, then per auction word HouseID,
//             quad CharacterID, string CharacterName, quad Bid
// The game gives each house to the named character and takes the bid and
// the first rent from the depot. It logs "bei Reset: INSERT INTO
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

// Query 51, excludeFromAuctions (0x80e4a30), called by FinishAuctions
// (0x80630a0) with Banish = true for an auction winner who cannot pay, and
// by CollectRents (0x8063760) with Banish = false for a tenant who is thrown
// out for not paying the rent.
//   request:  quad CharacterID, flag Banish
//   response: status only
// How long a banished character stays excluded is not visible in bin/game;
// the exclusion is recorded with its time and the flag.
void Dispatcher::handleExcludeFromAuctions(Session &session, ReadBuffer &in, WriteBuffer &out) {
	uint32_t characterId = in.readQuad();
	bool banish = in.readFlag();
	Store.excludeFromAuctions(requireWorld(session), characterId, banish, Now());
	out.writeByte(STATUS_OK);
	Detail = format("character=%u banish=%d", characterId, banish ? 1 : 0);
}

// Query 35, transferHouses (0x80e4b40), called by CollectRents (0x8063760)
// at start ("Bearbeite freiwillige Kuendigungen").
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

// Query 52, cancelHouseTransfer (0x80e4d90), called by CollectRents
// (0x8063760) when the buyer of a house cannot pay the price.
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

// Query 36, evictFreeAccounts (0x80e4e90), called by CollectRents
// (0x8063760) at start.
//   request:  nothing
//   response: word NumberOfEvictions, then per eviction word HouseID,
//             quad OwnerID
// The game empties each listed house whose owner id still matches ("Account
// von Haus %d ist nicht mehr bezahlt"). An owner whose character has no
// account here is not listed: nothing is known about that account.
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

// Query 37, evictDeletedCharacters (0x80e5030), called by CollectRents
// (0x8063760) at start.
//   request:  nothing
//   response: word NumberOfEvictions, then that many word HouseID
// The game empties each listed house ("Besitzer von Haus %d ist geloescht").
void Dispatcher::handleEvictDeletedCharacters(Session &session, ReadBuffer &, WriteBuffer &out) {
	std::vector<uint16_t> houses = Store.listHousesOfDeletedCharacters(requireWorld(session));
	out.writeByte(STATUS_OK);
	out.writeWord(static_cast<uint16_t>(houses.size()));
	for (uint16_t houseId : houses) {
		out.writeWord(houseId);
	}
	Detail = format("evictions=%zu", houses.size());
}

// Query 38, evictExGuildleaders (0x80e51a0), called by CollectRents
// (0x8063760) at start if at least one guildhouse is rented.
//   request:  word NumberOfGuildhouses, then per guildhouse word HouseID,
//             quad Guildleader (the owner of the house)
//   response: word NumberOfEvictions, then that many word HouseID
// The game empties each listed house ("Mieter von Gildenhaus %d ist kein
// Gildenfuehrer mehr"). A house is listed if its owner is recorded as a
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

} // namespace qm
