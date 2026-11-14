// sqlite_storage.cc: the SQLite storage: the schema and the statements of
// every storage method.

#include "sqlite_storage.hh"

#include "text.hh"

#include <sqlite3.h>

#include <cstdio>

namespace qm {

namespace {

// Version of the schema below, stored in meta.schema_version. 2 added the
// player data (PlayerDataSchema); migrate() brings a version 1 database up.
const int SchemaVersion = 2;

// Table and column names follow the SQL statements the game writes to its
// house log for the operator ("on reset: ..." in src/houses.cc): tables
// Auctions and HouseOwners, columns World, HouseID, Termination, NewOwner,
// Accepted, Price. Everything else is this server's own design.
const char *const Schema =
	"CREATE TABLE meta ("
	"	key TEXT PRIMARY KEY,"
	"	value TEXT NOT NULL"
	") WITHOUT ROWID;"

	// Password each application type presents in the login request.
	"CREATE TABLE applications ("
	"	application_type INTEGER PRIMARY KEY,"
	"	password TEXT NOT NULL"
	");"

	"CREATE TABLE worlds ("
	"	world_id INTEGER PRIMARY KEY AUTOINCREMENT,"
	"	name TEXT NOT NULL,"
	"	name_key TEXT NOT NULL UNIQUE,"
	"	type INTEGER NOT NULL,"
	"	reboot_hour INTEGER NOT NULL,"
	"	address TEXT NOT NULL,"
	"	port INTEGER NOT NULL,"
	"	max_players INTEGER NOT NULL,"
	"	premium_player_buffer INTEGER NOT NULL,"
	"	max_newbies INTEGER NOT NULL,"
	"	premium_newbie_buffer INTEGER NOT NULL,"
	"	private_world INTEGER NOT NULL DEFAULT 0,"
	"	online INTEGER NOT NULL DEFAULT 0,"
	"	online_record INTEGER NOT NULL DEFAULT 0,"
	"	online_record_time INTEGER NOT NULL DEFAULT 0"
	");"

	"CREATE TABLE accounts ("
	"	account_id INTEGER PRIMARY KEY,"
	"	password TEXT NOT NULL,"
	"	premium_until INTEGER NOT NULL DEFAULT 0,"
	"	banished_until INTEGER NOT NULL DEFAULT 0,"
	"	deleted INTEGER NOT NULL DEFAULT 0,"
	"	gamemaster INTEGER NOT NULL DEFAULT 0"
	");"

	"CREATE TABLE account_rights ("
	"	account_id INTEGER NOT NULL,"
	"	right_name TEXT NOT NULL,"
	"	PRIMARY KEY (account_id, right_name)"
	") WITHOUT ROWID;"

	"CREATE TABLE buddies ("
	"	account_id INTEGER NOT NULL,"
	"	buddy_id INTEGER NOT NULL,"
	"	PRIMARY KEY (account_id, buddy_id)"
	") WITHOUT ROWID;"

	"CREATE TABLE ip_banishments ("
	"	address TEXT PRIMARY KEY,"
	"	banished_until INTEGER NOT NULL"
	") WITHOUT ROWID;"

	// account_id 0: not assigned to an account.
	"CREATE TABLE characters ("
	"	character_id INTEGER PRIMARY KEY,"
	"	world_id INTEGER NOT NULL,"
	"	account_id INTEGER NOT NULL DEFAULT 0,"
	"	name TEXT NOT NULL,"
	"	name_key TEXT NOT NULL UNIQUE,"
	"	sex INTEGER NOT NULL DEFAULT 1,"
	"	guild TEXT NOT NULL DEFAULT '',"
	"	guild_rank TEXT NOT NULL DEFAULT '',"
	"	guild_title TEXT NOT NULL DEFAULT '',"
	"	guild_leader INTEGER NOT NULL DEFAULT 0,"
	"	level INTEGER NOT NULL DEFAULT 0,"
	"	profession TEXT NOT NULL DEFAULT '',"
	"	residence TEXT NOT NULL DEFAULT '',"
	"	last_login INTEGER NOT NULL DEFAULT 0,"
	"	tutor_activities INTEGER NOT NULL DEFAULT 0,"
	"	is_online INTEGER NOT NULL DEFAULT 0,"
	"	deleted INTEGER NOT NULL DEFAULT 0,"
	"	namelocked INTEGER NOT NULL DEFAULT 0"
	");"
	"CREATE INDEX characters_world ON characters (world_id, character_id);"
	"CREATE INDEX characters_account ON characters (account_id);"

	"CREATE TABLE character_rights ("
	"	character_id INTEGER NOT NULL,"
	"	right_name TEXT NOT NULL,"
	"	PRIMARY KEY (character_id, right_name)"
	") WITHOUT ROWID;"

	// Last player list the world reported (createPlayerlist).
	"CREATE TABLE online_players ("
	"	world_id INTEGER NOT NULL,"
	"	name TEXT NOT NULL,"
	"	level INTEGER NOT NULL,"
	"	profession TEXT NOT NULL"
	");"
	"CREATE INDEX online_players_world ON online_players (world_id);"

	// One row per race and report (logKilledCreatures).
	"CREATE TABLE killed_creatures ("
	"	world_id INTEGER NOT NULL,"
	"	time INTEGER NOT NULL,"
	"	race TEXT NOT NULL,"
	"	killed_players INTEGER NOT NULL,"
	"	killed_creatures INTEGER NOT NULL"
	");"

	"CREATE TABLE character_deaths ("
	"	character_id INTEGER NOT NULL,"
	"	level INTEGER NOT NULL,"
	"	offender_id INTEGER NOT NULL,"
	"	remark TEXT NOT NULL,"
	"	unjustified INTEGER NOT NULL,"
	"	time INTEGER NOT NULL"
	");"
	"CREATE INDEX character_deaths_character ON character_deaths (character_id);"

	// Master data the game sends at every start (insertHouses).
	"CREATE TABLE houses ("
	"	world_id INTEGER NOT NULL,"
	"	house_id INTEGER NOT NULL,"
	"	name TEXT NOT NULL,"
	"	rent INTEGER NOT NULL,"
	"	description TEXT NOT NULL,"
	"	size INTEGER NOT NULL,"
	"	x INTEGER NOT NULL,"
	"	y INTEGER NOT NULL,"
	"	z INTEGER NOT NULL,"
	"	town TEXT NOT NULL,"
	"	guildhouse INTEGER NOT NULL,"
	"	PRIMARY KEY (world_id, house_id)"
	") WITHOUT ROWID;"

	// termination/new_owner/accepted/price describe a move-out or sale the
	// owner announced; NULL termination means none is pending.
	"CREATE TABLE house_owners ("
	"	world_id INTEGER NOT NULL,"
	"	house_id INTEGER NOT NULL,"
	"	owner_id INTEGER NOT NULL,"
	"	paid_until INTEGER NOT NULL,"
	"	termination INTEGER,"
	"	new_owner INTEGER,"
	"	accepted INTEGER NOT NULL DEFAULT 0,"
	"	price INTEGER,"
	"	PRIMARY KEY (world_id, house_id)"
	") WITHOUT ROWID;"

	// bidder_id/bid/finish_time are NULL until somebody bids.
	"CREATE TABLE auctions ("
	"	world_id INTEGER NOT NULL,"
	"	house_id INTEGER NOT NULL,"
	"	bidder_id INTEGER,"
	"	bid INTEGER,"
	"	finish_time INTEGER,"
	"	PRIMARY KEY (world_id, house_id)"
	") WITHOUT ROWID;"

	"CREATE TABLE auction_exclusions ("
	"	world_id INTEGER NOT NULL,"
	"	character_id INTEGER NOT NULL,"
	"	banish INTEGER NOT NULL,"
	"	time INTEGER NOT NULL"
	");";

// The player data (playerdata.hh, loadPlayerData and savePlayerData), which
// the game once kept in its player files. Keyed by character id alone, like
// the files, and without a reference to characters: the game saves what it
// loaded, also for a house owner the characters table does not know. The
// numbers are the game's int values (negative ones stay negative); times are
// Unix seconds. One row per character in player_data, the lists in tables of
// their own; a list entry the game does not save (a skill it never set, a
// spell not known, a quest value of 0) has no row. The item texts (inventory,
// player_depots.content) are the text the player file had for the objects.
const char *const PlayerDataSchema =
	// The outfit columns: *_outfit is the outfit id; *_object_type the
	// object the character looks like when that id is 0, the colors
	// (*_head, *_body, *_legs, *_feet) those of any other outfit.
	"CREATE TABLE player_data ("
	"	character_id INTEGER PRIMARY KEY,"
	"	name TEXT NOT NULL,"
	"	race INTEGER NOT NULL,"
	"	profession INTEGER NOT NULL,"
	"	original_outfit INTEGER NOT NULL,"
	"	original_object_type INTEGER NOT NULL DEFAULT 0,"
	"	original_head INTEGER NOT NULL DEFAULT 0,"
	"	original_body INTEGER NOT NULL DEFAULT 0,"
	"	original_legs INTEGER NOT NULL DEFAULT 0,"
	"	original_feet INTEGER NOT NULL DEFAULT 0,"
	"	current_outfit INTEGER NOT NULL,"
	"	current_object_type INTEGER NOT NULL DEFAULT 0,"
	"	current_head INTEGER NOT NULL DEFAULT 0,"
	"	current_body INTEGER NOT NULL DEFAULT 0,"
	"	current_legs INTEGER NOT NULL DEFAULT 0,"
	"	current_feet INTEGER NOT NULL DEFAULT 0,"
	"	last_login INTEGER NOT NULL,"
	"	last_logout INTEGER NOT NULL,"
	"	start_x INTEGER NOT NULL,"
	"	start_y INTEGER NOT NULL,"
	"	start_z INTEGER NOT NULL,"
	"	pos_x INTEGER NOT NULL,"
	"	pos_y INTEGER NOT NULL,"
	"	pos_z INTEGER NOT NULL,"
	"	playerkiller_end INTEGER NOT NULL,"
	"	inventory TEXT NOT NULL"
	");"

	// The skills the game has set (TPlayerData's Minimum is not INT_MIN).
	"CREATE TABLE player_skills ("
	"	character_id INTEGER NOT NULL,"
	"	skill_nr INTEGER NOT NULL,"
	"	actual INTEGER NOT NULL,"
	"	maximum INTEGER NOT NULL,"
	"	minimum INTEGER NOT NULL,"
	"	delta_act INTEGER NOT NULL,"
	"	magic_delta_act INTEGER NOT NULL,"
	"	cycle INTEGER NOT NULL,"
	"	max_cycle INTEGER NOT NULL,"
	"	count INTEGER NOT NULL,"
	"	max_count INTEGER NOT NULL,"
	"	add_level INTEGER NOT NULL,"
	"	experience INTEGER NOT NULL,"
	"	factor_percent INTEGER NOT NULL,"
	"	next_level INTEGER NOT NULL,"
	"	delta INTEGER NOT NULL,"
	"	PRIMARY KEY (character_id, skill_nr)"
	") WITHOUT ROWID;"

	"CREATE TABLE player_spells ("
	"	character_id INTEGER NOT NULL,"
	"	spell_nr INTEGER NOT NULL,"
	"	PRIMARY KEY (character_id, spell_nr)"
	") WITHOUT ROWID;"

	"CREATE TABLE player_quest_values ("
	"	character_id INTEGER NOT NULL,"
	"	quest_nr INTEGER NOT NULL,"
	"	value INTEGER NOT NULL,"
	"	PRIMARY KEY (character_id, quest_nr)"
	") WITHOUT ROWID;"

	// The times of the character's unjustified kills; position 0 is the
	// oldest. The game keeps the last 20 and saves those of the last 30 days.
	"CREATE TABLE player_murders ("
	"	character_id INTEGER NOT NULL,"
	"	position INTEGER NOT NULL,"
	"	time INTEGER NOT NULL,"
	"	PRIMARY KEY (character_id, position)"
	") WITHOUT ROWID;"

	// The depots that have contents (0 to 8, the town depots of map.toml).
	"CREATE TABLE player_depots ("
	"	character_id INTEGER NOT NULL,"
	"	depot_nr INTEGER NOT NULL,"
	"	content TEXT NOT NULL,"
	"	PRIMARY KEY (character_id, depot_nr)"
	") WITHOUT ROWID;";

class Stmt {
public:
	Stmt(sqlite3 *db, const char *sql) : Db(db), Handle(nullptr) {
		if (sqlite3_prepare_v2(db, sql, -1, &Handle, nullptr) != SQLITE_OK) {
			throw StorageError(std::string("sqlite prepare failed: ") + sqlite3_errmsg(db));
		}
	}

	~Stmt() {
		sqlite3_finalize(Handle);
	}

	Stmt(const Stmt &) = delete;
	Stmt &operator=(const Stmt &) = delete;

	Stmt &bind(int index, int64_t value) {
		check(sqlite3_bind_int64(Handle, index, value));
		return *this;
	}

	// Binds an ISO-8859-1 string as UTF-8 text.
	Stmt &bindText(int index, const std::string &latin1) {
		std::string utf8 = latin1ToUtf8(latin1);
		check(sqlite3_bind_text(Handle, index, utf8.data(), static_cast<int>(utf8.size()),
			SQLITE_TRANSIENT));
		return *this;
	}

	// Binds NULL for 0, the value otherwise.
	Stmt &bindOptional(int index, int64_t value) {
		check(value == 0 ? sqlite3_bind_null(Handle, index) : sqlite3_bind_int64(Handle, index, value));
		return *this;
	}

	// True while there is a row.
	bool step() {
		int rc = sqlite3_step(Handle);
		if (rc == SQLITE_ROW) {
			return true;
		}
		if (rc != SQLITE_DONE) {
			throw StorageError(std::string("sqlite step failed: ") + sqlite3_errmsg(Db));
		}
		return false;
	}

	// Runs a statement that returns no rows; returns the number of changes.
	int run() {
		while (step()) {
		}
		return sqlite3_changes(Db);
	}

	void reset() {
		sqlite3_reset(Handle);
		sqlite3_clear_bindings(Handle);
	}

	int64_t columnInt(int column) {
		return sqlite3_column_int64(Handle, column);
	}

	// Returns the text of a column as ISO-8859-1.
	std::string columnText(int column) {
		const unsigned char *text = sqlite3_column_text(Handle, column);
		int size = sqlite3_column_bytes(Handle, column);
		if (text == nullptr || size <= 0) {
			return std::string();
		}
		return utf8ToLatin1(std::string(reinterpret_cast<const char *>(text), static_cast<size_t>(size)));
	}

private:
	void check(int rc) {
		if (rc != SQLITE_OK) {
			throw StorageError(std::string("sqlite bind failed: ") + sqlite3_errmsg(Db));
		}
	}

	sqlite3 *Db;
	sqlite3_stmt *Handle;
};

std::string formatAddress(const uint8_t address[4]) {
	char buffer[16];
	std::snprintf(buffer, sizeof(buffer), "%u.%u.%u.%u", address[0], address[1], address[2], address[3]);
	return buffer;
}

void parseAddress(const std::string &text, uint8_t address[4]) {
	unsigned a = 0, b = 0, c = 0, d = 0;
	if (std::sscanf(text.c_str(), "%u.%u.%u.%u", &a, &b, &c, &d) != 4
			|| a > 255 || b > 255 || c > 255 || d > 255) {
		throw StorageError("world address is not a dotted IPv4 address: " + text);
	}
	address[0] = static_cast<uint8_t>(a);
	address[1] = static_cast<uint8_t>(b);
	address[2] = static_cast<uint8_t>(c);
	address[3] = static_cast<uint8_t>(d);
}

const char *const WorldColumns =
	"world_id, name, type, reboot_hour, address, port, max_players, premium_player_buffer,"
	" max_newbies, premium_newbie_buffer, private_world, online, online_record, online_record_time";

WorldRecord readWorld(Stmt &stmt) {
	WorldRecord world;
	world.id = static_cast<uint32_t>(stmt.columnInt(0));
	world.name = stmt.columnText(1);
	world.config.type = static_cast<uint8_t>(stmt.columnInt(2));
	world.config.rebootHour = static_cast<uint8_t>(stmt.columnInt(3));
	parseAddress(stmt.columnText(4), world.config.address);
	world.config.port = static_cast<uint16_t>(stmt.columnInt(5));
	world.config.maxPlayers = static_cast<uint16_t>(stmt.columnInt(6));
	world.config.premiumPlayerBuffer = static_cast<uint16_t>(stmt.columnInt(7));
	world.config.maxNewbies = static_cast<uint16_t>(stmt.columnInt(8));
	world.config.premiumNewbieBuffer = static_cast<uint16_t>(stmt.columnInt(9));
	world.privateWorld = stmt.columnInt(10) != 0;
	world.online = stmt.columnInt(11) != 0;
	world.onlineRecord = static_cast<uint32_t>(stmt.columnInt(12));
	world.onlineRecordTime = static_cast<uint32_t>(stmt.columnInt(13));
	return world;
}

const char *const CharacterColumns =
	"character_id, world_id, account_id, name, sex, guild, guild_rank, guild_title, guild_leader,"
	" level, profession, residence, last_login, tutor_activities, is_online, deleted, namelocked";

CharacterRecord readCharacter(Stmt &stmt) {
	CharacterRecord character;
	character.id = static_cast<uint32_t>(stmt.columnInt(0));
	character.worldId = static_cast<uint32_t>(stmt.columnInt(1));
	character.accountId = static_cast<uint32_t>(stmt.columnInt(2));
	character.name = stmt.columnText(3);
	character.sex = static_cast<uint8_t>(stmt.columnInt(4));
	character.guild = stmt.columnText(5);
	character.rank = stmt.columnText(6);
	character.title = stmt.columnText(7);
	character.guildLeader = stmt.columnInt(8) != 0;
	character.level = static_cast<uint16_t>(stmt.columnInt(9));
	character.profession = stmt.columnText(10);
	character.residence = stmt.columnText(11);
	character.lastLogin = static_cast<uint32_t>(stmt.columnInt(12));
	character.tutorActivities = static_cast<uint32_t>(stmt.columnInt(13));
	character.isOnline = static_cast<uint32_t>(stmt.columnInt(14));
	character.deleted = stmt.columnInt(15) != 0;
	character.namelocked = stmt.columnInt(16) != 0;
	return character;
}

} // namespace

SqliteStorage::SqliteStorage(const std::string &path) : Db(nullptr), InTransaction(false) {
	if (sqlite3_open_v2(path.c_str(), &Db, SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE | SQLITE_OPEN_FULLMUTEX,
			nullptr) != SQLITE_OK) {
		std::string message = Db != nullptr ? sqlite3_errmsg(Db) : "out of memory";
		sqlite3_close(Db);
		Db = nullptr;
		throw StorageError("cannot open database " + path + ": " + message);
	}
	try {
		sqlite3_busy_timeout(Db, 5000);
		exec("PRAGMA journal_mode = WAL");
		exec("PRAGMA synchronous = NORMAL");
		createSchema();
	} catch (...) {
		sqlite3_close(Db);
		Db = nullptr;
		throw;
	}
}

SqliteStorage::~SqliteStorage() {
	if (Db != nullptr) {
		if (InTransaction) {
			sqlite3_exec(Db, "ROLLBACK", nullptr, nullptr, nullptr);
		}
		sqlite3_close(Db);
	}
}

void SqliteStorage::exec(const char *sql) {
	char *message = nullptr;
	if (sqlite3_exec(Db, sql, nullptr, nullptr, &message) != SQLITE_OK) {
		std::string text = message != nullptr ? message : "unknown error";
		sqlite3_free(message);
		throw StorageError("sqlite exec failed: " + text);
	}
}

void SqliteStorage::createSchema() {
	bool hasMeta = false;
	{
		Stmt stmt(Db, "SELECT 1 FROM sqlite_master WHERE type = 'table' AND name = 'meta'");
		hasMeta = stmt.step();
	}
	if (!hasMeta) {
		exec("BEGIN IMMEDIATE");
		try {
			exec(Schema);
			exec(PlayerDataSchema);
			Stmt stmt(Db, "INSERT INTO meta (key, value) VALUES ('schema_version', ?)");
			stmt.bind(1, SchemaVersion).run();
		} catch (...) {
			sqlite3_exec(Db, "ROLLBACK", nullptr, nullptr, nullptr);
			throw;
		}
		exec("COMMIT");
		return;
	}
	int64_t version = 0;
	{
		Stmt stmt(Db, "SELECT value FROM meta WHERE key = 'schema_version'");
		if (stmt.step()) {
			version = stmt.columnInt(0);
		}
	}
	if (version == SchemaVersion) {
		return;
	}
	if (version < 1 || version > SchemaVersion) {
		throw StorageError("database has a different schema version than this program");
	}
	migrate(version);
}

// Brings a database of an older schema version up to SchemaVersion in one
// transaction: either every step is applied or none.
void SqliteStorage::migrate(int64_t version) {
	exec("BEGIN IMMEDIATE");
	try {
		if (version < 2) {
			exec(PlayerDataSchema);
		}
		Stmt stmt(Db, "UPDATE meta SET value = ? WHERE key = 'schema_version'");
		stmt.bind(1, SchemaVersion).run();
	} catch (...) {
		sqlite3_exec(Db, "ROLLBACK", nullptr, nullptr, nullptr);
		throw;
	}
	exec("COMMIT");
}

void SqliteStorage::begin() {
	if (InTransaction) {
		throw StorageError("transaction already open");
	}
	exec("BEGIN IMMEDIATE");
	InTransaction = true;
}

void SqliteStorage::commit() {
	if (!InTransaction) {
		throw StorageError("no transaction open");
	}
	try {
		exec("COMMIT");
	} catch (...) {
		// A COMMIT can fail with the transaction still open (SQLITE_BUSY).
		// End it, or every later BEGIN would fail.
		InTransaction = false;
		if (sqlite3_get_autocommit(Db) == 0) {
			sqlite3_exec(Db, "ROLLBACK", nullptr, nullptr, nullptr);
		}
		throw;
	}
	InTransaction = false;
}

void SqliteStorage::rollback() {
	if (InTransaction) {
		InTransaction = false;
		exec("ROLLBACK");
	}
}

uint64_t SqliteStorage::countRows(const std::string &table) {
	for (char c : table) {
		bool valid = (c >= 'a' && c <= 'z') || c == '_';
		if (!valid) {
			throw StorageError("invalid table name");
		}
	}
	Stmt stmt(Db, ("SELECT COUNT(*) FROM " + table).c_str());
	stmt.step();
	return static_cast<uint64_t>(stmt.columnInt(0));
}

// --- applications and worlds

void SqliteStorage::setApplicationPassword(uint8_t applicationType, const std::string &password) {
	Stmt stmt(Db, "INSERT INTO applications (application_type, password) VALUES (?, ?)"
		" ON CONFLICT (application_type) DO UPDATE SET password = excluded.password");
	stmt.bind(1, applicationType).bindText(2, password).run();
}

bool SqliteStorage::checkApplicationPassword(uint8_t applicationType, const std::string &password) {
	Stmt stmt(Db, "SELECT password FROM applications WHERE application_type = ?");
	stmt.bind(1, applicationType);
	if (!stmt.step()) {
		return false;
	}
	return stmt.columnText(0) == password;
}

uint32_t SqliteStorage::putWorld(const std::string &name, const WorldConfig &config, bool privateWorld) {
	{
		Stmt stmt(Db, "INSERT INTO worlds (name, name_key, type, reboot_hour, address, port, max_players,"
			" premium_player_buffer, max_newbies, premium_newbie_buffer, private_world)"
			" VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)"
			" ON CONFLICT (name_key) DO UPDATE SET name = excluded.name, type = excluded.type,"
			" reboot_hour = excluded.reboot_hour, address = excluded.address, port = excluded.port,"
			" max_players = excluded.max_players,"
			" premium_player_buffer = excluded.premium_player_buffer,"
			" max_newbies = excluded.max_newbies,"
			" premium_newbie_buffer = excluded.premium_newbie_buffer,"
			" private_world = excluded.private_world");
		stmt.bindText(1, name).bindText(2, foldLatin1(name)).bind(3, config.type)
			.bind(4, config.rebootHour).bindText(5, formatAddress(config.address))
			.bind(6, config.port).bind(7, config.maxPlayers).bind(8, config.premiumPlayerBuffer)
			.bind(9, config.maxNewbies).bind(10, config.premiumNewbieBuffer)
			.bind(11, privateWorld ? 1 : 0).run();
	}
	Stmt stmt(Db, "SELECT world_id FROM worlds WHERE name_key = ?");
	stmt.bindText(1, foldLatin1(name));
	if (!stmt.step()) {
		throw StorageError("world vanished after insert");
	}
	return static_cast<uint32_t>(stmt.columnInt(0));
}

std::optional<WorldRecord> SqliteStorage::findWorld(const std::string &name) {
	Stmt stmt(Db, (std::string("SELECT ") + WorldColumns + " FROM worlds WHERE name_key = ?").c_str());
	stmt.bindText(1, foldLatin1(name));
	if (!stmt.step()) {
		return std::nullopt;
	}
	return readWorld(stmt);
}

std::optional<WorldRecord> SqliteStorage::getWorld(uint32_t worldId) {
	Stmt stmt(Db, (std::string("SELECT ") + WorldColumns + " FROM worlds WHERE world_id = ?").c_str());
	stmt.bind(1, worldId);
	if (!stmt.step()) {
		return std::nullopt;
	}
	return readWorld(stmt);
}

void SqliteStorage::setWorldOnline(uint32_t worldId, bool online) {
	Stmt stmt(Db, "UPDATE worlds SET online = ? WHERE world_id = ?");
	stmt.bind(1, online ? 1 : 0).bind(2, worldId).run();
}

void SqliteStorage::setOnlineRecord(uint32_t worldId, uint32_t players, uint32_t time) {
	Stmt stmt(Db, "UPDATE worlds SET online_record = ?, online_record_time = ? WHERE world_id = ?");
	stmt.bind(1, players).bind(2, time).bind(3, worldId).run();
}

// --- accounts

void SqliteStorage::putAccount(const AccountRecord &account) {
	Stmt stmt(Db, "INSERT INTO accounts (account_id, password, premium_until, banished_until, deleted,"
		" gamemaster) VALUES (?, ?, ?, ?, ?, ?)"
		" ON CONFLICT (account_id) DO UPDATE SET password = excluded.password,"
		" premium_until = excluded.premium_until, banished_until = excluded.banished_until,"
		" deleted = excluded.deleted, gamemaster = excluded.gamemaster");
	stmt.bind(1, account.id).bindText(2, account.password).bind(3, account.premiumUntil)
		.bind(4, account.banishedUntil).bind(5, account.deleted ? 1 : 0)
		.bind(6, account.gamemaster ? 1 : 0).run();
}

std::optional<AccountRecord> SqliteStorage::findAccount(uint32_t accountId) {
	Stmt stmt(Db, "SELECT account_id, password, premium_until, banished_until, deleted, gamemaster"
		" FROM accounts WHERE account_id = ?");
	stmt.bind(1, accountId);
	if (!stmt.step()) {
		return std::nullopt;
	}
	AccountRecord account;
	account.id = static_cast<uint32_t>(stmt.columnInt(0));
	account.password = stmt.columnText(1);
	account.premiumUntil = static_cast<uint32_t>(stmt.columnInt(2));
	account.banishedUntil = static_cast<uint32_t>(stmt.columnInt(3));
	account.deleted = stmt.columnInt(4) != 0;
	account.gamemaster = stmt.columnInt(5) != 0;
	return account;
}

void SqliteStorage::addAccountRight(uint32_t accountId, const std::string &right) {
	Stmt stmt(Db, "INSERT OR IGNORE INTO account_rights (account_id, right_name) VALUES (?, ?)");
	stmt.bind(1, accountId).bindText(2, right).run();
}

std::vector<std::string> SqliteStorage::listAccountRights(uint32_t accountId) {
	std::vector<std::string> rights;
	Stmt stmt(Db, "SELECT right_name FROM account_rights WHERE account_id = ? ORDER BY right_name");
	stmt.bind(1, accountId);
	while (stmt.step()) {
		rights.push_back(stmt.columnText(0));
	}
	return rights;
}

uint32_t SqliteStorage::countOnlineCharacters(uint32_t accountId, uint32_t exceptCharacterId) {
	Stmt stmt(Db, "SELECT COUNT(*) FROM characters WHERE account_id = ? AND is_online > 0"
		" AND character_id <> ?");
	stmt.bind(1, accountId).bind(2, exceptCharacterId);
	stmt.step();
	return static_cast<uint32_t>(stmt.columnInt(0));
}

bool SqliteStorage::addBuddy(uint32_t accountId, uint32_t buddyId) {
	Stmt stmt(Db, "INSERT OR IGNORE INTO buddies (account_id, buddy_id) VALUES (?, ?)");
	return stmt.bind(1, accountId).bind(2, buddyId).run() > 0;
}

bool SqliteStorage::removeBuddy(uint32_t accountId, uint32_t buddyId) {
	Stmt stmt(Db, "DELETE FROM buddies WHERE account_id = ? AND buddy_id = ?");
	return stmt.bind(1, accountId).bind(2, buddyId).run() > 0;
}

std::vector<PlayerEntry> SqliteStorage::listBuddies(uint32_t accountId) {
	std::vector<PlayerEntry> buddies;
	Stmt stmt(Db, "SELECT b.buddy_id, c.name FROM buddies b"
		" JOIN characters c ON c.character_id = b.buddy_id"
		" WHERE b.account_id = ? AND c.deleted = 0 ORDER BY b.buddy_id");
	stmt.bind(1, accountId);
	while (stmt.step()) {
		PlayerEntry entry;
		entry.id = static_cast<uint32_t>(stmt.columnInt(0));
		entry.name = stmt.columnText(1);
		buddies.push_back(entry);
	}
	return buddies;
}

// --- IP banishments

void SqliteStorage::banishIpAddress(const std::string &address, uint32_t until) {
	Stmt stmt(Db, "INSERT INTO ip_banishments (address, banished_until) VALUES (?, ?)"
		" ON CONFLICT (address) DO UPDATE SET banished_until = excluded.banished_until");
	stmt.bindText(1, address).bind(2, until).run();
}

bool SqliteStorage::isIpBanished(const std::string &address, uint32_t now) {
	Stmt stmt(Db, "SELECT 1 FROM ip_banishments WHERE address = ? AND banished_until > ?");
	stmt.bindText(1, address).bind(2, now);
	return stmt.step();
}

// --- characters

void SqliteStorage::putCharacter(const CharacterRecord &character) {
	// The conflict target is the id only: a second character with the name
	// of an existing one violates the name_key constraint and throws.
	Stmt stmt(Db, "INSERT INTO characters (character_id, world_id, account_id, name, name_key, sex,"
		" guild, guild_rank, guild_title, guild_leader, level, profession, residence, last_login,"
		" tutor_activities, is_online, deleted, namelocked)"
		" VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)"
		" ON CONFLICT (character_id) DO UPDATE SET world_id = excluded.world_id,"
		" account_id = excluded.account_id, name = excluded.name, name_key = excluded.name_key,"
		" sex = excluded.sex, guild = excluded.guild, guild_rank = excluded.guild_rank,"
		" guild_title = excluded.guild_title, guild_leader = excluded.guild_leader,"
		" level = excluded.level, profession = excluded.profession, residence = excluded.residence,"
		" last_login = excluded.last_login, tutor_activities = excluded.tutor_activities,"
		" is_online = excluded.is_online, deleted = excluded.deleted,"
		" namelocked = excluded.namelocked");
	stmt.bind(1, character.id).bind(2, character.worldId).bind(3, character.accountId)
		.bindText(4, character.name).bindText(5, foldLatin1(character.name)).bind(6, character.sex)
		.bindText(7, character.guild).bindText(8, character.rank).bindText(9, character.title)
		.bind(10, character.guildLeader ? 1 : 0).bind(11, character.level)
		.bindText(12, character.profession).bindText(13, character.residence)
		.bind(14, character.lastLogin).bind(15, character.tutorActivities)
		.bind(16, character.isOnline).bind(17, character.deleted ? 1 : 0)
		.bind(18, character.namelocked ? 1 : 0).run();
}

std::optional<CharacterRecord> SqliteStorage::findCharacter(uint32_t characterId) {
	Stmt stmt(Db, (std::string("SELECT ") + CharacterColumns
		+ " FROM characters WHERE character_id = ?").c_str());
	stmt.bind(1, characterId);
	if (!stmt.step()) {
		return std::nullopt;
	}
	return readCharacter(stmt);
}

std::optional<CharacterRecord> SqliteStorage::findCharacterByName(const std::string &name) {
	Stmt stmt(Db, (std::string("SELECT ") + CharacterColumns
		+ " FROM characters WHERE name_key = ?").c_str());
	stmt.bindText(1, foldLatin1(name));
	if (!stmt.step()) {
		return std::nullopt;
	}
	return readCharacter(stmt);
}

uint32_t SqliteStorage::nextCharacterId() {
	Stmt stmt(Db, "SELECT COALESCE(MAX(character_id), 0) FROM characters");
	stmt.step();
	int64_t highest = stmt.columnInt(0);
	if (highest >= 0xFFFFFFFFLL) {
		throw StorageError("no character id left");
	}
	return static_cast<uint32_t>(highest + 1);
}

void SqliteStorage::addCharacterRight(uint32_t characterId, const std::string &right) {
	Stmt stmt(Db, "INSERT OR IGNORE INTO character_rights (character_id, right_name) VALUES (?, ?)");
	stmt.bind(1, characterId).bindText(2, right).run();
}

std::vector<std::string> SqliteStorage::listCharacterRights(uint32_t characterId) {
	std::vector<std::string> rights;
	Stmt stmt(Db, "SELECT right_name FROM character_rights WHERE character_id = ? ORDER BY right_name");
	stmt.bind(1, characterId);
	while (stmt.step()) {
		rights.push_back(stmt.columnText(0));
	}
	return rights;
}

std::vector<PlayerEntry> SqliteStorage::listPlayers(uint32_t worldId, uint32_t minimumId, uint32_t limit) {
	std::vector<PlayerEntry> players;
	Stmt stmt(Db, "SELECT character_id, name FROM characters"
		" WHERE world_id = ? AND character_id >= ? AND deleted = 0"
		" ORDER BY character_id LIMIT ?");
	stmt.bind(1, worldId).bind(2, minimumId).bind(3, limit);
	while (stmt.step()) {
		PlayerEntry entry;
		entry.id = static_cast<uint32_t>(stmt.columnInt(0));
		entry.name = stmt.columnText(1);
		players.push_back(entry);
	}
	return players;
}

std::vector<CharacterRecord> SqliteStorage::listAccountCharacters(uint32_t accountId) {
	std::vector<CharacterRecord> characters;
	Stmt stmt(Db, (std::string("SELECT ") + CharacterColumns
		+ " FROM characters WHERE account_id = ? AND deleted = 0 ORDER BY name_key").c_str());
	stmt.bind(1, accountId);
	while (stmt.step()) {
		characters.push_back(readCharacter(stmt));
	}
	return characters;
}

bool SqliteStorage::incrementIsOnline(uint32_t characterId) {
	Stmt stmt(Db, "UPDATE characters SET is_online = is_online + 1 WHERE character_id = ?");
	return stmt.bind(1, characterId).run() > 0;
}

bool SqliteStorage::decrementIsOnline(uint32_t characterId) {
	Stmt stmt(Db, "UPDATE characters SET is_online = MAX(is_online - 1, 0) WHERE character_id = ?");
	return stmt.bind(1, characterId).run() > 0;
}

uint32_t SqliteStorage::clearIsOnline(uint32_t worldId) {
	Stmt stmt(Db, "UPDATE characters SET is_online = 0 WHERE world_id = ? AND is_online > 0");
	return static_cast<uint32_t>(stmt.bind(1, worldId).run());
}

bool SqliteStorage::recordLogout(uint32_t characterId, uint16_t level, const std::string &profession,
		const std::string &residence, uint32_t lastLogin, uint16_t tutorActivities) {
	Stmt stmt(Db, "UPDATE characters SET level = ?, profession = ?, residence = ?, last_login = ?,"
		" tutor_activities = tutor_activities + ? WHERE character_id = ?");
	return stmt.bind(1, level).bindText(2, profession).bindText(3, residence).bind(4, lastLogin)
		.bind(5, tutorActivities).bind(6, characterId).run() > 0;
}

// --- statistics

void SqliteStorage::replaceOnlinePlayers(uint32_t worldId, const std::vector<OnlinePlayer> &players) {
	{
		Stmt stmt(Db, "DELETE FROM online_players WHERE world_id = ?");
		stmt.bind(1, worldId).run();
	}
	Stmt stmt(Db, "INSERT INTO online_players (world_id, name, level, profession) VALUES (?, ?, ?, ?)");
	for (const OnlinePlayer &player : players) {
		stmt.reset();
		stmt.bind(1, worldId).bindText(2, player.name).bind(3, player.level)
			.bindText(4, player.profession).run();
	}
}

void SqliteStorage::addKilledCreatures(uint32_t worldId, const std::vector<KilledCreatures> &entries,
		uint32_t time) {
	Stmt stmt(Db, "INSERT INTO killed_creatures (world_id, time, race, killed_players, killed_creatures)"
		" VALUES (?, ?, ?, ?, ?)");
	for (const KilledCreatures &entry : entries) {
		stmt.reset();
		stmt.bind(1, worldId).bind(2, time).bindText(3, entry.race).bind(4, entry.killedPlayers)
			.bind(5, entry.killedCreatures).run();
	}
}

void SqliteStorage::addCharacterDeath(const CharacterDeath &death) {
	Stmt stmt(Db, "INSERT INTO character_deaths (character_id, level, offender_id, remark, unjustified,"
		" time) VALUES (?, ?, ?, ?, ?, ?)");
	stmt.bind(1, death.characterId).bind(2, death.level).bind(3, death.offenderId)
		.bindText(4, death.remark).bind(5, death.unjustified ? 1 : 0).bind(6, death.time).run();
}

// --- houses

void SqliteStorage::replaceHouses(uint32_t worldId, const std::vector<HouseRecord> &houses) {
	{
		Stmt stmt(Db, "DELETE FROM houses WHERE world_id = ?");
		stmt.bind(1, worldId).run();
	}
	Stmt stmt(Db, "INSERT INTO houses (world_id, house_id, name, rent, description, size, x, y, z, town,"
		" guildhouse) VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)");
	for (const HouseRecord &house : houses) {
		stmt.reset();
		stmt.bind(1, worldId).bind(2, house.id).bindText(3, house.name).bind(4, house.rent)
			.bindText(5, house.description).bind(6, house.size).bind(7, house.x).bind(8, house.y)
			.bind(9, house.z).bindText(10, house.town).bind(11, house.guildhouse ? 1 : 0).run();
	}
}

std::vector<HouseOwner> SqliteStorage::listHouseOwners(uint32_t worldId) {
	std::vector<HouseOwner> owners;
	Stmt stmt(Db, "SELECT o.house_id, o.owner_id, COALESCE(c.name, ''), o.paid_until"
		" FROM house_owners o"
		" JOIN houses h ON h.world_id = o.world_id AND h.house_id = o.house_id"
		" LEFT JOIN characters c ON c.character_id = o.owner_id"
		" WHERE o.world_id = ? ORDER BY o.house_id");
	stmt.bind(1, worldId);
	while (stmt.step()) {
		HouseOwner owner;
		owner.houseId = static_cast<uint16_t>(stmt.columnInt(0));
		owner.ownerId = static_cast<uint32_t>(stmt.columnInt(1));
		owner.ownerName = stmt.columnText(2);
		owner.paidUntil = static_cast<uint32_t>(stmt.columnInt(3));
		owners.push_back(owner);
	}
	return owners;
}

bool SqliteStorage::insertHouseOwner(uint32_t worldId, uint16_t houseId, uint32_t ownerId,
		uint32_t paidUntil) {
	Stmt stmt(Db, "INSERT OR IGNORE INTO house_owners (world_id, house_id, owner_id, paid_until)"
		" VALUES (?, ?, ?, ?)");
	return stmt.bind(1, worldId).bind(2, houseId).bind(3, ownerId).bind(4, paidUntil).run() > 0;
}

bool SqliteStorage::updateHouseOwner(uint32_t worldId, uint16_t houseId, uint32_t ownerId,
		uint32_t paidUntil) {
	Stmt stmt(Db, "UPDATE house_owners SET owner_id = ?, paid_until = ?"
		" WHERE world_id = ? AND house_id = ?");
	return stmt.bind(1, ownerId).bind(2, paidUntil).bind(3, worldId).bind(4, houseId).run() > 0;
}

bool SqliteStorage::deleteHouseOwner(uint32_t worldId, uint16_t houseId) {
	Stmt stmt(Db, "DELETE FROM house_owners WHERE world_id = ? AND house_id = ?");
	return stmt.bind(1, worldId).bind(2, houseId).run() > 0;
}

bool SqliteStorage::setHouseTransfer(uint32_t worldId, uint16_t houseId, uint32_t termination,
		uint32_t newOwnerId, bool accepted, uint32_t price) {
	Stmt stmt(Db, "UPDATE house_owners SET termination = ?, new_owner = ?, accepted = ?, price = ?"
		" WHERE world_id = ? AND house_id = ?");
	return stmt.bind(1, termination).bindOptional(2, newOwnerId).bind(3, accepted ? 1 : 0)
		.bind(4, price).bind(5, worldId).bind(6, houseId).run() > 0;
}

std::vector<HouseTransfer> SqliteStorage::listHouseTransfers(uint32_t worldId, uint32_t now) {
	std::vector<HouseTransfer> transfers;
	Stmt stmt(Db, "SELECT o.house_id, COALESCE(o.new_owner, 0), COALESCE(c.name, ''), COALESCE(o.price, 0)"
		" FROM house_owners o"
		" JOIN houses h ON h.world_id = o.world_id AND h.house_id = o.house_id"
		" LEFT JOIN characters c ON c.character_id = o.new_owner"
		" WHERE o.world_id = ? AND o.termination IS NOT NULL AND o.termination <= ?"
		" AND (o.new_owner IS NULL OR o.accepted <> 0)"
		" ORDER BY o.house_id");
	stmt.bind(1, worldId).bind(2, now);
	while (stmt.step()) {
		HouseTransfer transfer;
		transfer.houseId = static_cast<uint16_t>(stmt.columnInt(0));
		transfer.newOwnerId = static_cast<uint32_t>(stmt.columnInt(1));
		transfer.newOwnerName = stmt.columnText(2);
		transfer.price = static_cast<uint32_t>(stmt.columnInt(3));
		transfers.push_back(transfer);
	}
	return transfers;
}

bool SqliteStorage::cancelHouseTransfer(uint32_t worldId, uint16_t houseId) {
	// The statement the game logs for this case:
	// UPDATE HouseOwners SET Termination=null,NewOwner=null,Accepted=0,Price=null
	Stmt stmt(Db, "UPDATE house_owners SET termination = NULL, new_owner = NULL, accepted = 0,"
		" price = NULL WHERE world_id = ? AND house_id = ?");
	return stmt.bind(1, worldId).bind(2, houseId).run() > 0;
}

std::vector<HouseEviction> SqliteStorage::listHousesOfFreeAccounts(uint32_t worldId, uint32_t now) {
	std::vector<HouseEviction> evictions;
	Stmt stmt(Db, "SELECT o.house_id, o.owner_id FROM house_owners o"
		" JOIN houses h ON h.world_id = o.world_id AND h.house_id = o.house_id"
		" JOIN characters c ON c.character_id = o.owner_id"
		" JOIN accounts a ON a.account_id = c.account_id"
		" WHERE o.world_id = ? AND a.premium_until <= ? ORDER BY o.house_id");
	stmt.bind(1, worldId).bind(2, now);
	while (stmt.step()) {
		HouseEviction eviction;
		eviction.houseId = static_cast<uint16_t>(stmt.columnInt(0));
		eviction.ownerId = static_cast<uint32_t>(stmt.columnInt(1));
		evictions.push_back(eviction);
	}
	return evictions;
}

std::vector<uint16_t> SqliteStorage::listHousesOfDeletedCharacters(uint32_t worldId) {
	std::vector<uint16_t> houses;
	Stmt stmt(Db, "SELECT o.house_id FROM house_owners o"
		" JOIN houses h ON h.world_id = o.world_id AND h.house_id = o.house_id"
		" JOIN characters c ON c.character_id = o.owner_id"
		" WHERE o.world_id = ? AND c.deleted <> 0 ORDER BY o.house_id");
	stmt.bind(1, worldId);
	while (stmt.step()) {
		houses.push_back(static_cast<uint16_t>(stmt.columnInt(0)));
	}
	return houses;
}

// --- auctions

std::vector<uint16_t> SqliteStorage::listAuctions(uint32_t worldId) {
	std::vector<uint16_t> houses;
	Stmt stmt(Db, "SELECT a.house_id FROM auctions a"
		" JOIN houses h ON h.world_id = a.world_id AND h.house_id = a.house_id"
		" WHERE a.world_id = ? ORDER BY a.house_id");
	stmt.bind(1, worldId);
	while (stmt.step()) {
		houses.push_back(static_cast<uint16_t>(stmt.columnInt(0)));
	}
	return houses;
}

bool SqliteStorage::startAuction(uint32_t worldId, uint16_t houseId) {
	Stmt stmt(Db, "INSERT OR IGNORE INTO auctions (world_id, house_id)"
		" SELECT world_id, house_id FROM houses WHERE world_id = ? AND house_id = ?");
	return stmt.bind(1, worldId).bind(2, houseId).run() > 0;
}

bool SqliteStorage::setAuctionBid(uint32_t worldId, uint16_t houseId, uint32_t bidderId, uint32_t bid,
		uint32_t finishTime) {
	Stmt stmt(Db, "UPDATE auctions SET bidder_id = ?, bid = ?, finish_time = ?"
		" WHERE world_id = ? AND house_id = ?");
	return stmt.bindOptional(1, bidderId).bind(2, bid).bind(3, finishTime).bind(4, worldId)
		.bind(5, houseId).run() > 0;
}

std::vector<AuctionResult> SqliteStorage::takeFinishedAuctions(uint32_t worldId, uint32_t now) {
	std::vector<AuctionResult> results;
	{
		Stmt stmt(Db, "SELECT a.house_id, a.bidder_id, COALESCE(c.name, ''), a.bid FROM auctions a"
			" JOIN houses h ON h.world_id = a.world_id AND h.house_id = a.house_id"
			" LEFT JOIN characters c ON c.character_id = a.bidder_id"
			" WHERE a.world_id = ? AND a.bidder_id IS NOT NULL AND a.finish_time IS NOT NULL"
			" AND a.finish_time <= ? ORDER BY a.house_id");
		stmt.bind(1, worldId).bind(2, now);
		while (stmt.step()) {
			AuctionResult result;
			result.houseId = static_cast<uint16_t>(stmt.columnInt(0));
			result.characterId = static_cast<uint32_t>(stmt.columnInt(1));
			result.characterName = stmt.columnText(2);
			result.bid = static_cast<uint32_t>(stmt.columnInt(3));
			results.push_back(result);
		}
	}
	Stmt stmt(Db, "DELETE FROM auctions WHERE world_id = ? AND house_id = ?");
	for (const AuctionResult &result : results) {
		stmt.reset();
		stmt.bind(1, worldId).bind(2, result.houseId).run();
	}
	return results;
}

void SqliteStorage::excludeFromAuctions(uint32_t worldId, uint32_t characterId, bool banish, uint32_t now) {
	{
		Stmt stmt(Db, "UPDATE auctions SET bidder_id = NULL, bid = NULL, finish_time = NULL"
			" WHERE world_id = ? AND bidder_id = ?");
		stmt.bind(1, worldId).bind(2, characterId).run();
	}
	Stmt stmt(Db, "INSERT INTO auction_exclusions (world_id, character_id, banish, time)"
		" VALUES (?, ?, ?, ?)");
	stmt.bind(1, worldId).bind(2, characterId).bind(3, banish ? 1 : 0).bind(4, now).run();
}

// --- player data

namespace {

const char *const PlayerDataColumns =
	"name, race, profession,"
	" original_outfit, original_object_type, original_head, original_body, original_legs, original_feet,"
	" current_outfit, current_object_type, current_head, current_body, current_legs, current_feet,"
	" last_login, last_logout, start_x, start_y, start_z, pos_x, pos_y, pos_z, playerkiller_end, inventory";

// The outfit from its six columns, the first at `first`.
void readOutfitColumns(Stmt &stmt, int first, PlayerOutfit &outfit) {
	outfit = PlayerOutfit();
	outfit.id = static_cast<int32_t>(stmt.columnInt(first));
	if (outfit.id == 0) {
		outfit.objectType = static_cast<uint16_t>(stmt.columnInt(first + 1));
	} else {
		for (int i = 0; i < 4; i++) {
			outfit.colors[i] = static_cast<uint8_t>(stmt.columnInt(first + 2 + i));
		}
	}
}

void bindOutfitColumns(Stmt &stmt, int first, const PlayerOutfit &outfit) {
	stmt.bind(first, outfit.id);
	stmt.bind(first + 1, outfit.id == 0 ? outfit.objectType : 0);
	for (int i = 0; i < 4; i++) {
		stmt.bind(first + 2 + i, outfit.id != 0 ? outfit.colors[i] : 0);
	}
}

int32_t columnInt32(Stmt &stmt, int column) {
	return static_cast<int32_t>(stmt.columnInt(column));
}

} // namespace

std::optional<PlayerData> SqliteStorage::loadPlayerData(uint32_t characterId) {
	PlayerData data;
	data.characterId = characterId;
	{
		Stmt stmt(Db, (std::string("SELECT ") + PlayerDataColumns
			+ " FROM player_data WHERE character_id = ?").c_str());
		stmt.bind(1, characterId);
		if (!stmt.step()) {
			return std::nullopt;
		}
		data.name = stmt.columnText(0);
		data.race = columnInt32(stmt, 1);
		data.profession = columnInt32(stmt, 2);
		readOutfitColumns(stmt, 3, data.originalOutfit);
		readOutfitColumns(stmt, 9, data.currentOutfit);
		data.lastLogin = columnInt32(stmt, 15);
		data.lastLogout = columnInt32(stmt, 16);
		data.startX = columnInt32(stmt, 17);
		data.startY = columnInt32(stmt, 18);
		data.startZ = columnInt32(stmt, 19);
		data.posX = columnInt32(stmt, 20);
		data.posY = columnInt32(stmt, 21);
		data.posZ = columnInt32(stmt, 22);
		data.playerkillerEnd = columnInt32(stmt, 23);
		data.inventory = stmt.columnText(24);
	}
	{
		Stmt stmt(Db, "SELECT skill_nr, actual, maximum, minimum, delta_act, magic_delta_act, cycle, max_cycle,"
			" count, max_count, add_level, experience, factor_percent, next_level, delta"
			" FROM player_skills WHERE character_id = ? ORDER BY skill_nr");
		stmt.bind(1, characterId);
		while (stmt.step()) {
			PlayerSkill skill;
			skill.number = static_cast<uint8_t>(stmt.columnInt(0));
			skill.actual = columnInt32(stmt, 1);
			skill.maximum = columnInt32(stmt, 2);
			skill.minimum = columnInt32(stmt, 3);
			skill.deltaAct = columnInt32(stmt, 4);
			skill.magicDeltaAct = columnInt32(stmt, 5);
			skill.cycle = columnInt32(stmt, 6);
			skill.maxCycle = columnInt32(stmt, 7);
			skill.count = columnInt32(stmt, 8);
			skill.maxCount = columnInt32(stmt, 9);
			skill.addLevel = columnInt32(stmt, 10);
			skill.experience = columnInt32(stmt, 11);
			skill.factorPercent = columnInt32(stmt, 12);
			skill.nextLevel = columnInt32(stmt, 13);
			skill.delta = columnInt32(stmt, 14);
			data.skills.push_back(skill);
		}
	}
	{
		Stmt stmt(Db, "SELECT spell_nr FROM player_spells WHERE character_id = ? ORDER BY spell_nr");
		stmt.bind(1, characterId);
		while (stmt.step()) {
			data.spells.push_back(static_cast<uint8_t>(stmt.columnInt(0)));
		}
	}
	{
		Stmt stmt(Db, "SELECT quest_nr, value FROM player_quest_values WHERE character_id = ?"
			" ORDER BY quest_nr");
		stmt.bind(1, characterId);
		while (stmt.step()) {
			PlayerQuestValue quest;
			quest.number = static_cast<uint16_t>(stmt.columnInt(0));
			quest.value = columnInt32(stmt, 1);
			data.questValues.push_back(quest);
		}
	}
	{
		Stmt stmt(Db, "SELECT time FROM player_murders WHERE character_id = ? ORDER BY position");
		stmt.bind(1, characterId);
		while (stmt.step()) {
			data.murders.push_back(columnInt32(stmt, 0));
		}
	}
	{
		Stmt stmt(Db, "SELECT depot_nr, content FROM player_depots WHERE character_id = ? ORDER BY depot_nr");
		stmt.bind(1, characterId);
		while (stmt.step()) {
			PlayerDepot depot;
			depot.number = static_cast<uint8_t>(stmt.columnInt(0));
			depot.content = stmt.columnText(1);
			data.depots.push_back(depot);
		}
	}
	return data;
}

void SqliteStorage::savePlayerData(const PlayerData &data) {
	uint32_t id = data.characterId;
	for (const char *table : {"player_skills", "player_spells", "player_quest_values", "player_murders",
			"player_depots"}) {
		Stmt stmt(Db, (std::string("DELETE FROM ") + table + " WHERE character_id = ?").c_str());
		stmt.bind(1, id).run();
	}
	{
		Stmt stmt(Db, (std::string("INSERT OR REPLACE INTO player_data (character_id, ") + PlayerDataColumns
			+ ") VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)").c_str());
		stmt.bind(1, id).bindText(2, data.name).bind(3, data.race).bind(4, data.profession);
		bindOutfitColumns(stmt, 5, data.originalOutfit);
		bindOutfitColumns(stmt, 11, data.currentOutfit);
		stmt.bind(17, data.lastLogin).bind(18, data.lastLogout).bind(19, data.startX).bind(20, data.startY)
			.bind(21, data.startZ).bind(22, data.posX).bind(23, data.posY).bind(24, data.posZ)
			.bind(25, data.playerkillerEnd).bindText(26, data.inventory).run();
	}
	if (!data.skills.empty()) {
		Stmt stmt(Db, "INSERT INTO player_skills (character_id, skill_nr, actual, maximum, minimum, delta_act,"
			" magic_delta_act, cycle, max_cycle, count, max_count, add_level, experience, factor_percent,"
			" next_level, delta) VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)");
		for (const PlayerSkill &skill : data.skills) {
			stmt.bind(1, id).bind(2, skill.number).bind(3, skill.actual).bind(4, skill.maximum)
				.bind(5, skill.minimum).bind(6, skill.deltaAct).bind(7, skill.magicDeltaAct).bind(8, skill.cycle)
				.bind(9, skill.maxCycle).bind(10, skill.count).bind(11, skill.maxCount).bind(12, skill.addLevel)
				.bind(13, skill.experience).bind(14, skill.factorPercent).bind(15, skill.nextLevel)
				.bind(16, skill.delta).run();
			stmt.reset();
		}
	}
	if (!data.spells.empty()) {
		Stmt stmt(Db, "INSERT INTO player_spells (character_id, spell_nr) VALUES (?, ?)");
		for (uint8_t spell : data.spells) {
			stmt.bind(1, id).bind(2, spell).run();
			stmt.reset();
		}
	}
	if (!data.questValues.empty()) {
		Stmt stmt(Db, "INSERT INTO player_quest_values (character_id, quest_nr, value) VALUES (?, ?, ?)");
		for (const PlayerQuestValue &quest : data.questValues) {
			stmt.bind(1, id).bind(2, quest.number).bind(3, quest.value).run();
			stmt.reset();
		}
	}
	if (!data.murders.empty()) {
		Stmt stmt(Db, "INSERT INTO player_murders (character_id, position, time) VALUES (?, ?, ?)");
		for (size_t i = 0; i < data.murders.size(); i++) {
			stmt.bind(1, id).bind(2, static_cast<int64_t>(i)).bind(3, data.murders[i]).run();
			stmt.reset();
		}
	}
	if (!data.depots.empty()) {
		Stmt stmt(Db, "INSERT INTO player_depots (character_id, depot_nr, content) VALUES (?, ?, ?)");
		for (const PlayerDepot &depot : data.depots) {
			stmt.bind(1, id).bind(2, depot.number).bindText(3, depot.content).run();
			stmt.reset();
		}
	}
}

bool SqliteStorage::hasPlayerData(uint32_t characterId) {
	Stmt stmt(Db, "SELECT 1 FROM player_data WHERE character_id = ?");
	stmt.bind(1, characterId);
	return stmt.step();
}

} // namespace qm
