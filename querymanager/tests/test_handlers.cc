// Request handlers against an in-memory database. All data is made up.

#include "handlers.hh"
#include "log.hh"
#include "sqlite_storage.hh"
#include "test.hh"

#include <sqlite3.h>
#include <sys/socket.h>
#include <unistd.h>

#include <cstdio>
#include <string>
#include <vector>

using namespace qm;

namespace {

const uint32_t StartTime = 1000000;

struct Fixture {
	SqliteStorage storage{":memory:"};
	uint32_t now = StartTime;
	Dispatcher dispatcher{storage, [this]() { return now; }};
	Session session;
	uint32_t worldId = 0;
	uint32_t otherWorldId = 0;

	Fixture() {
		storage.setApplicationPassword(APPLICATION_GAME, "secret");
		WorldConfig config;
		config.type = 1;
		config.rebootHour = 6;
		config.address[0] = 10;
		config.address[1] = 20;
		config.address[2] = 30;
		config.address[3] = 40;
		config.port = 7171;
		config.maxPlayers = 900;
		config.premiumPlayerBuffer = 150;
		config.maxNewbies = 300;
		config.premiumNewbieBuffer = 25;
		worldId = storage.putWorld("Testia", config, false);
		otherWorldId = storage.putWorld("Elsewhere", config, false);
	}

	std::vector<uint8_t> call(const WriteBuffer &request) {
		return dispatcher.handle(session, request.data());
	}

	void login() {
		WriteBuffer request;
		request.writeByte(QUERY_LOGIN);
		request.writeByte(APPLICATION_GAME);
		request.writeString("secret");
		request.writeString("Testia");
		std::vector<uint8_t> response = call(request);
		CHECK(response.size() == 1 && response[0] == STATUS_OK);
	}

	CharacterRecord addCharacter(uint32_t id, const std::string &name, uint32_t accountId = 0) {
		CharacterRecord character;
		character.id = id;
		character.worldId = worldId;
		character.accountId = accountId;
		character.name = name;
		storage.putCharacter(character);
		return character;
	}

	AccountRecord addAccount(uint32_t id, const std::string &password, uint32_t premiumUntil = 0) {
		AccountRecord account;
		account.id = id;
		account.password = password;
		account.premiumUntil = premiumUntil;
		storage.putAccount(account);
		return account;
	}

	void addHouses(const std::vector<uint16_t> &ids) {
		std::vector<HouseRecord> houses;
		for (uint16_t id : ids) {
			HouseRecord house;
			house.id = id;
			house.name = "House " + std::to_string(id);
			houses.push_back(house);
		}
		storage.replaceHouses(worldId, houses);
	}
};

WriteBuffer request(uint8_t type) {
	WriteBuffer buffer;
	buffer.writeByte(type);
	return buffer;
}

bool isFailed(const std::vector<uint8_t> &response) {
	return response.size() == 1 && response[0] == STATUS_FAILED;
}

bool isOk(const std::vector<uint8_t> &response) {
	return response.size() == 1 && response[0] == STATUS_OK;
}

bool isError(const std::vector<uint8_t> &response, uint8_t code) {
	return response.size() == 2 && response[0] == STATUS_ERROR && response[1] == code;
}

WriteBuffer loginGameRequest(uint32_t accountId, const std::string &name, const std::string &password,
		const std::string &address = "192.0.2.1", bool privateWorld = false, bool premiumRequired = false,
		bool gamemasterRequired = false) {
	WriteBuffer buffer = request(QUERY_LOGIN_GAME);
	buffer.writeQuad(accountId);
	buffer.writeString(name);
	buffer.writeString(password);
	buffer.writeString(address);
	buffer.writeFlag(privateWorld);
	buffer.writeFlag(premiumRequired);
	buffer.writeFlag(gamemasterRequired);
	return buffer;
}

void testLogin() {
	Fixture f;

	// Nothing works before the login.
	CHECK(isFailed(f.call(request(QUERY_LOAD_WORLD_CONFIG))));

	WriteBuffer wrongPassword = request(QUERY_LOGIN);
	wrongPassword.writeByte(APPLICATION_GAME);
	wrongPassword.writeString("Secret");
	wrongPassword.writeString("Testia");
	CHECK(isFailed(f.call(wrongPassword)));
	CHECK(!f.session.loggedIn);

	WriteBuffer unknownWorld = request(QUERY_LOGIN);
	unknownWorld.writeByte(APPLICATION_GAME);
	unknownWorld.writeString("secret");
	unknownWorld.writeString("Nowhere");
	CHECK(isFailed(f.call(unknownWorld)));

	// An application type without a stored password cannot log in.
	WriteBuffer otherApplication = request(QUERY_LOGIN);
	otherApplication.writeByte(2);
	otherApplication.writeString("secret");
	CHECK(isFailed(f.call(otherApplication)));

	// The world name is matched without regard to case.
	WriteBuffer good = request(QUERY_LOGIN);
	good.writeByte(APPLICATION_GAME);
	good.writeString("secret");
	good.writeString("TESTIA");
	CHECK(isOk(f.call(good)));
	CHECK(f.session.loggedIn);
	CHECK(f.session.worldId == f.worldId);
	CHECK(f.session.worldName == "Testia");
}

WriteBuffer loginAccountRequest(uint32_t accountId, const std::string &password,
		const std::string &address = "192.0.2.1") {
	WriteBuffer buffer = request(QUERY_LOGIN_ACCOUNT);
	buffer.writeQuad(accountId);
	buffer.writeString(password);
	buffer.writeString(address);
	return buffer;
}

void loginAsLoginServer(Fixture &f) {
	WriteBuffer login = request(QUERY_LOGIN);
	login.writeByte(APPLICATION_LOGIN);
	login.writeString("login-secret");
	CHECK(isOk(f.call(login)));
	CHECK(f.session.loggedIn && f.session.worldId == 0);
}

void testLoginAccount() {
	Fixture f;
	f.storage.setApplicationPassword(APPLICATION_LOGIN, "login-secret");
	WorldConfig config;
	config.address[0] = 127;
	config.address[1] = 0;
	config.address[2] = 0;
	config.address[3] = 1;
	config.port = 7172;
	uint32_t thirdWorldId = f.storage.putWorld("Zanera", config, false);

	f.addAccount(111, "pw", StartTime + 3 * 86400);
	f.addCharacter(1, "Bravo", 111);
	f.addCharacter(2, "alpha", 111);
	CharacterRecord deleted = f.addCharacter(3, "Charlie", 111);
	deleted.deleted = true;
	f.storage.putCharacter(deleted);
	CharacterRecord elsewhere = f.addCharacter(4, "Delta", 111);
	elsewhere.worldId = thirdWorldId;
	f.storage.putCharacter(elsewhere);
	CharacterRecord lost = f.addCharacter(5, "Echo", 111);
	lost.worldId = 999;
	f.storage.putCharacter(lost);
	f.addCharacter(6, "Foxtrot", 222);
	f.addAccount(222, "other");
	f.addAccount(333, "empty");

	// The game's application may not ask, and neither may anybody before a
	// login.
	CHECK(isFailed(f.call(loginAccountRequest(111, "pw"))));
	f.login();
	CHECK(isFailed(f.call(loginAccountRequest(111, "pw"))));

	Fixture g;
	g.storage.setApplicationPassword(APPLICATION_LOGIN, "login-secret");
	WriteBuffer wrongPassword = request(QUERY_LOGIN);
	wrongPassword.writeByte(APPLICATION_LOGIN);
	wrongPassword.writeString("secret");
	CHECK(isFailed(g.call(wrongPassword)));
	CHECK(!g.session.loggedIn);

	f.session = Session();
	loginAsLoginServer(f);

	// Characters ordered by name without regard to case; the deleted one and
	// the one of an unknown world are left out. Each carries its world's
	// address and port.
	const uint8_t testia[4] = {10, 20, 30, 40};
	const uint8_t zanera[4] = {127, 0, 0, 1};
	WriteBuffer expected;
	expected.writeByte(STATUS_OK);
	expected.writeByte(3);
	expected.writeString("alpha");
	expected.writeString("Testia");
	expected.writeBytes(testia, 4);
	expected.writeWord(7171);
	expected.writeString("Bravo");
	expected.writeString("Testia");
	expected.writeBytes(testia, 4);
	expected.writeWord(7171);
	expected.writeString("Delta");
	expected.writeString("Zanera");
	expected.writeBytes(zanera, 4);
	expected.writeWord(7172);
	expected.writeWord(3);
	CHECK(f.call(loginAccountRequest(111, "pw")) == expected.data());

	// Premium days count started days.
	f.now = StartTime + 1;
	std::vector<uint8_t> response = f.call(loginAccountRequest(111, "pw"));
	CHECK(response.size() >= 2 && response[response.size() - 2] == 3 && response.back() == 0);
	f.now = StartTime + 2 * 86400 + 1;
	response = f.call(loginAccountRequest(111, "pw"));
	CHECK(response.size() >= 2 && response[response.size() - 2] == 1 && response.back() == 0);
	f.now = StartTime + 3 * 86400;
	response = f.call(loginAccountRequest(111, "pw"));
	CHECK(response.size() >= 2 && response[response.size() - 2] == 0 && response.back() == 0);
	f.now = StartTime;
	// The latest time there is: 49699 days (0xC223), below the 65535 that
	// the client shows as a gratis premium account.
	f.addAccount(111, "pw", 0xFFFFFFFFu);
	response = f.call(loginAccountRequest(111, "pw"));
	CHECK(response.size() >= 2 && response[response.size() - 2] == 0x23 && response.back() == 0xC2);

	// An account without characters is not an error here; the login server
	// decides what the client sees.
	const std::vector<uint8_t> empty = {STATUS_OK, 0, 0, 0};
	CHECK(f.call(loginAccountRequest(333, "empty")) == empty);

	CHECK(isError(f.call(loginAccountRequest(444, "pw")), ACCOUNT_UNKNOWN));
	CHECK(isError(f.call(loginAccountRequest(0, "")), ACCOUNT_UNKNOWN));
	CHECK(isError(f.call(loginAccountRequest(111, "PW")), ACCOUNT_WRONG_PASSWORD));
	CHECK(isError(f.call(loginAccountRequest(111, "")), ACCOUNT_WRONG_PASSWORD));

	AccountRecord account = *f.storage.findAccount(222);
	account.banishedUntil = StartTime + 1;
	f.storage.putAccount(account);
	// The password is checked first, so a banishment shows only to the owner.
	CHECK(isError(f.call(loginAccountRequest(222, "x")), ACCOUNT_WRONG_PASSWORD));
	CHECK(isError(f.call(loginAccountRequest(222, "other")), ACCOUNT_BANISHED));
	f.now = StartTime + 1;
	CHECK(f.call(loginAccountRequest(222, "other"))[0] == STATUS_OK);
	f.now = StartTime;

	account.banishedUntil = 0;
	account.deleted = true;
	f.storage.putAccount(account);
	CHECK(isError(f.call(loginAccountRequest(222, "other")), ACCOUNT_UNKNOWN));

	f.storage.banishIpAddress("198.51.100.7", StartTime + 60);
	CHECK(isError(f.call(loginAccountRequest(111, "pw", "198.51.100.7")), ACCOUNT_IP_BANISHED));
	CHECK(f.call(loginAccountRequest(111, "pw", "198.51.100.8"))[0] == STATUS_OK);

	// A request that is too short fails and changes nothing.
	WriteBuffer tooShort = request(QUERY_LOGIN_ACCOUNT);
	tooShort.writeQuad(111);
	CHECK(isFailed(f.call(tooShort)));
	CHECK(f.call(loginAccountRequest(111, "pw"))[0] == STATUS_OK);
}

void testWorldConfig() {
	Fixture f;
	f.login();
	std::vector<uint8_t> response = f.call(request(QUERY_LOAD_WORLD_CONFIG));
	const std::vector<uint8_t> expected = {
		STATUS_OK,
		1,                    // WorldType
		6,                    // RebootTime
		10, 20, 30, 40,       // IPAddress
		0x03, 0x1C,           // Port 7171
		0x84, 0x03,           // MaxPlayers 900
		0x96, 0x00,           // PremiumPlayerBuffer 150
		0x2C, 0x01,           // MaxNewbies 300
		0x19, 0x00,           // PremiumNewbieBuffer 25
	};
	CHECK(response == expected);
}

void testLoadPlayers() {
	Fixture f;
	f.login();
	f.addCharacter(30, "Testchar Three");
	f.addCharacter(10, "Testchar One");
	f.addCharacter(20, "Testchar Two");
	CharacterRecord deleted = f.addCharacter(25, "Testchar Gone");
	deleted.deleted = true;
	f.storage.putCharacter(deleted);
	CharacterRecord foreign = f.addCharacter(15, "Testchar Abroad");
	foreign.worldId = f.otherWorldId;
	f.storage.putCharacter(foreign);

	WriteBuffer all = request(QUERY_LOAD_PLAYERS);
	all.writeQuad(0);
	std::vector<uint8_t> response = f.call(all);
	ReadBuffer in(response);
	CHECK(in.readByte() == STATUS_OK);
	CHECK(in.readQuad() == 3);
	CHECK(in.readString() == "Testchar One");
	CHECK(in.readQuad() == 10);
	CHECK(in.readString() == "Testchar Two");
	CHECK(in.readQuad() == 20);
	CHECK(in.readString() == "Testchar Three");
	CHECK(in.readQuad() == 30);
	CHECK(in.eof());

	WriteBuffer from = request(QUERY_LOAD_PLAYERS);
	from.writeQuad(20);
	response = f.call(from);
	ReadBuffer fromIn(response);
	CHECK(fromIn.readByte() == STATUS_OK);
	CHECK(fromIn.readQuad() == 2);
	CHECK(fromIn.readString() == "Testchar Two");

	// The game asks in pages of 10000.
	f.storage.begin();
	for (uint32_t i = 0; i < 10005; i++) {
		f.addCharacter(1000 + i, "Bulk " + std::to_string(i));
	}
	f.storage.commit();
	WriteBuffer page = request(QUERY_LOAD_PLAYERS);
	page.writeQuad(1000);
	response = f.call(page);
	ReadBuffer pageIn(response);
	CHECK(pageIn.readByte() == STATUS_OK);
	CHECK(pageIn.readQuad() == 10000);
	uint32_t last = 0;
	for (uint32_t i = 0; i < 10000; i++) {
		pageIn.readString();
		last = pageIn.readQuad();
	}
	CHECK(pageIn.eof());
	CHECK(last == 10999);
	WriteBuffer rest = request(QUERY_LOAD_PLAYERS);
	rest.writeQuad(last + 1);
	response = f.call(rest);
	ReadBuffer restIn(response);
	CHECK(restIn.readByte() == STATUS_OK);
	CHECK(restIn.readQuad() == 5);
}

void testLoginGameSuccess() {
	Fixture f;
	f.login();
	f.addAccount(111111, "pw one", StartTime + 86400);
	CharacterRecord character = f.addCharacter(10, "Testchar One", 111111);
	character.sex = 2;
	character.guild = "Test Guild";
	character.rank = "Member";
	character.title = "the Tested";
	f.storage.putCharacter(character);
	f.addCharacter(20, "Testchar Two");
	f.addCharacter(30, "Testchar Three");
	f.storage.addBuddy(111111, 30);
	f.storage.addBuddy(111111, 20);
	f.storage.addBuddy(111111, 999);          // no such character: not listed
	f.storage.addCharacterRight(10, "READ_TUTOR_CHANNEL");
	f.storage.addAccountRight(111111, "SEND_BUGREPORTS");

	// The name is looked up without regard to case and returned as stored.
	std::vector<uint8_t> response = f.call(loginGameRequest(111111, "testchar ONE", "pw one"));
	ReadBuffer in(response);
	CHECK(in.readByte() == STATUS_OK);
	CHECK(in.readQuad() == 10);
	CHECK(in.readString() == "Testchar One");
	CHECK(in.readByte() == 2);
	CHECK(in.readString() == "Test Guild");
	CHECK(in.readString() == "Member");
	CHECK(in.readString() == "the Tested");
	CHECK(in.readByte() == 2);
	CHECK(in.readQuad() == 20);
	CHECK(in.readString() == "Testchar Two");
	CHECK(in.readQuad() == 30);
	CHECK(in.readString() == "Testchar Three");
	CHECK(in.readByte() == 3);
	CHECK(in.readString() == "READ_TUTOR_CHANNEL");
	CHECK(in.readString() == "SEND_BUGREPORTS");
	CHECK(in.readString() == "PREMIUM_ACCOUNT");
	CHECK(in.readFlag() == false);
	CHECK(in.eof());
	CHECK(f.storage.findCharacter(10)->isOnline == 1);

	// Once the premium time is over the right is not sent any more.
	f.now = StartTime + 86400;
	response = f.call(loginGameRequest(111111, "Testchar One", "pw one"));
	ReadBuffer later(response);
	CHECK(later.readByte() == STATUS_OK);
	later.readQuad();
	later.readString();
	later.readByte();
	later.readString();
	later.readString();
	later.readString();
	CHECK(later.readByte() == 2);
	later.readQuad();
	later.readString();
	later.readQuad();
	later.readString();
	CHECK(later.readByte() == 2);
	// The same character again is not "another character online"; the
	// counter goes up.
	CHECK(f.storage.findCharacter(10)->isOnline == 2);
}

void testLoginGameErrors() {
	Fixture f;
	f.login();
	f.addAccount(111111, "pw one");
	f.addCharacter(10, "Testchar One", 111111);

	CHECK(isError(f.call(loginGameRequest(111111, "Nobody", "pw one")), LOGIN_CHARACTER_UNKNOWN));

	CharacterRecord deleted = f.addCharacter(11, "Testchar Gone", 111111);
	deleted.deleted = true;
	f.storage.putCharacter(deleted);
	CHECK(isError(f.call(loginGameRequest(111111, "Testchar Gone", "pw one")), LOGIN_CHARACTER_DELETED));

	CharacterRecord foreign = f.addCharacter(12, "Testchar Abroad", 111111);
	foreign.worldId = f.otherWorldId;
	f.storage.putCharacter(foreign);
	CHECK(isError(f.call(loginGameRequest(111111, "Testchar Abroad", "pw one")), LOGIN_WRONG_WORLD));

	CHECK(isError(f.call(loginGameRequest(222222, "Testchar One", "pw one")), LOGIN_WRONG_ACCOUNT));
	CHECK(isError(f.call(loginGameRequest(0, "Testchar One", "pw one")), LOGIN_WRONG_ACCOUNT));
	CHECK(isError(f.call(loginGameRequest(111111, "Testchar One", "PW ONE")), LOGIN_WRONG_PASSWORD));
	CHECK(isError(f.call(loginGameRequest(111111, "Testchar One", "")), LOGIN_WRONG_PASSWORD));

	// An account that is marked deleted, and one that is gone altogether.
	AccountRecord gone = f.addAccount(333333, "pw three");
	gone.deleted = true;
	f.storage.putAccount(gone);
	f.addCharacter(13, "Testchar Orphan", 333333);
	CHECK(isError(f.call(loginGameRequest(333333, "Testchar Orphan", "pw three")), LOGIN_ACCOUNT_DELETED));
	f.addCharacter(14, "Testchar Stray", 444444);
	CHECK(isError(f.call(loginGameRequest(444444, "Testchar Stray", "x")), LOGIN_ACCOUNT_DELETED));

	AccountRecord banished = f.addAccount(555555, "pw five");
	banished.banishedUntil = StartTime + 10;
	f.storage.putAccount(banished);
	f.addCharacter(15, "Testchar Banned", 555555);
	CHECK(isError(f.call(loginGameRequest(555555, "Testchar Banned", "pw five")), LOGIN_ACCOUNT_BANISHED));
	// A wrong password is reported before the banishment.
	CHECK(isError(f.call(loginGameRequest(555555, "Testchar Banned", "nope")), LOGIN_WRONG_PASSWORD));
	f.now = StartTime + 10;
	CHECK(f.call(loginGameRequest(555555, "Testchar Banned", "pw five"))[0] == STATUS_OK);
	f.now = StartTime;

	CharacterRecord locked = f.addCharacter(16, "Testchar Locked", 111111);
	locked.namelocked = true;
	f.storage.putCharacter(locked);
	CHECK(isError(f.call(loginGameRequest(111111, "Testchar Locked", "pw one")), LOGIN_NAMELOCKED));

	f.storage.banishIpAddress("198.51.100.7", StartTime + 5);
	CHECK(isError(f.call(loginGameRequest(111111, "Testchar One", "pw one", "198.51.100.7")),
		LOGIN_IP_BANISHED));

	// Private world: only with the INVITED right.
	CHECK(isError(f.call(loginGameRequest(111111, "Testchar One", "pw one", "192.0.2.1", true)),
		LOGIN_NOT_INVITED));
	f.addAccount(666666, "pw six");
	f.addCharacter(17, "Testchar Guest", 666666);
	f.storage.addAccountRight(666666, "INVITED");
	CHECK(f.call(loginGameRequest(666666, "Testchar Guest", "pw six", "192.0.2.1", true))[0] == STATUS_OK);

	// Gamemaster client: only on a gamemaster account.
	CHECK(isError(f.call(loginGameRequest(111111, "Testchar One", "pw one", "192.0.2.1", false, false, true)),
		LOGIN_NOT_GAMEMASTER));
	AccountRecord gamemaster = f.addAccount(777777, "pw seven");
	gamemaster.gamemaster = true;
	f.storage.putAccount(gamemaster);
	f.addCharacter(18, "Testchar Master", 777777);
	CHECK(f.call(loginGameRequest(777777, "Testchar Master", "pw seven", "192.0.2.1", false, false, true))[0]
		== STATUS_OK);

	// No error code is known for PremiumAccountRequired, so it fails.
	CHECK(isFailed(f.call(loginGameRequest(111111, "Testchar One", "pw one", "192.0.2.1", false, true))));

	// None of the refused logins counted as online.
	CHECK(f.storage.findCharacter(10)->isOnline == 0);

	// A second character of the same account.
	f.addCharacter(19, "Testchar Twin", 111111);
	CHECK(f.call(loginGameRequest(111111, "Testchar One", "pw one"))[0] == STATUS_OK);
	CHECK(isError(f.call(loginGameRequest(111111, "Testchar Twin", "pw one")), LOGIN_ALREADY_ONLINE));
	CHECK(f.storage.findCharacter(19)->isOnline == 0);
	f.storage.addAccountRight(111111, "ALLOW_MULTICLIENT");
	CHECK(f.call(loginGameRequest(111111, "Testchar Twin", "pw one"))[0] == STATUS_OK);
	CHECK(f.storage.findCharacter(19)->isOnline == 1);
}

void testLoginGameWithoutAccount() {
	Fixture f;
	f.login();
	f.addCharacter(10, "Testchar One");

	// A character without an account answers to account 0 only. The game
	// refuses such a login itself and sends no decrementIsOnline, so it
	// must not count as online.
	std::vector<uint8_t> response = f.call(loginGameRequest(0, "Testchar One", "anything"));
	ReadBuffer in(response);
	CHECK(in.readByte() == STATUS_OK);
	CHECK(in.readQuad() == 10);
	CHECK(in.readString() == "Testchar One");
	CHECK(in.readByte() == 1);
	CHECK(in.readString() == "");
	CHECK(in.readString() == "");
	CHECK(in.readString() == "");
	CHECK(in.readByte() == 0);
	CHECK(in.readByte() == 0);
	CHECK(in.readFlag() == false);
	CHECK(in.eof());
	CHECK(f.storage.findCharacter(10)->isOnline == 0);

	CHECK(isError(f.call(loginGameRequest(111111, "Testchar One", "anything")), LOGIN_WRONG_ACCOUNT));
}

void testLogout() {
	Fixture f;
	f.login();
	f.addAccount(111111, "pw one");
	f.addCharacter(10, "Testchar One", 111111);
	CHECK(f.call(loginGameRequest(111111, "Testchar One", "pw one"))[0] == STATUS_OK);
	CHECK(f.storage.findCharacter(10)->isOnline == 1);

	WriteBuffer logout = request(QUERY_LOGOUT_GAME);
	logout.writeQuad(10);
	logout.writeWord(37);
	logout.writeString("Elder Druid");
	logout.writeString("Testtown");
	logout.writeQuad(StartTime - 500);
	logout.writeWord(3);
	CHECK(isOk(f.call(logout)));
	CharacterRecord character = *f.storage.findCharacter(10);
	CHECK(character.isOnline == 0);
	CHECK(character.level == 37);
	CHECK(character.profession == "Elder Druid");
	CHECK(character.residence == "Testtown");
	CHECK(character.lastLogin == StartTime - 500);
	CHECK(character.tutorActivities == 3);

	// Tutor activities add up; the counter does not go below zero.
	CHECK(isOk(f.call(logout)));
	character = *f.storage.findCharacter(10);
	CHECK(character.isOnline == 0);
	CHECK(character.tutorActivities == 6);

	WriteBuffer unknown = request(QUERY_LOGOUT_GAME);
	unknown.writeQuad(99);
	unknown.writeWord(1);
	unknown.writeString("None");
	unknown.writeString("");
	unknown.writeQuad(0);
	unknown.writeWord(0);
	CHECK(isFailed(f.call(unknown)));
}

void testIsOnline() {
	Fixture f;
	f.login();
	f.addAccount(111111, "pw one");
	f.addAccount(222222, "pw two");
	f.addCharacter(10, "Testchar One", 111111);
	f.addCharacter(20, "Testchar Two", 222222);
	CHECK(f.call(loginGameRequest(111111, "Testchar One", "pw one"))[0] == STATUS_OK);
	CHECK(f.call(loginGameRequest(222222, "Testchar Two", "pw two"))[0] == STATUS_OK);

	WriteBuffer decrement = request(QUERY_DECREMENT_IS_ONLINE);
	decrement.writeQuad(10);
	CHECK(isOk(f.call(decrement)));
	CHECK(f.storage.findCharacter(10)->isOnline == 0);
	CHECK(f.storage.findCharacter(20)->isOnline == 1);

	WriteBuffer unknown = request(QUERY_DECREMENT_IS_ONLINE);
	unknown.writeQuad(99);
	CHECK(isFailed(f.call(unknown)));

	// A character of another world keeps its counter.
	CharacterRecord foreign = f.addCharacter(30, "Testchar Abroad");
	foreign.worldId = f.otherWorldId;
	foreign.isOnline = 1;
	f.storage.putCharacter(foreign);

	std::vector<uint8_t> response = f.call(request(QUERY_CLEAR_IS_ONLINE));
	const std::vector<uint8_t> one = {STATUS_OK, 0x01, 0x00};
	CHECK(response == one);
	CHECK(f.storage.findCharacter(20)->isOnline == 0);
	CHECK(f.storage.findCharacter(30)->isOnline == 1);
	response = f.call(request(QUERY_CLEAR_IS_ONLINE));
	const std::vector<uint8_t> none = {STATUS_OK, 0x00, 0x00};
	CHECK(response == none);
}

void testPlayerlist() {
	Fixture f;
	f.login();

	WriteBuffer empty = request(QUERY_CREATE_PLAYERLIST);
	empty.writeWord(0);
	const std::vector<uint8_t> noRecord = {STATUS_OK, 0x00};
	const std::vector<uint8_t> newRecord = {STATUS_OK, 0x01};
	CHECK(f.call(empty) == noRecord);
	CHECK(f.storage.getWorld(f.worldId)->online);

	WriteBuffer two = request(QUERY_CREATE_PLAYERLIST);
	two.writeWord(2);
	two.writeString("Testchar One");
	two.writeWord(8);
	two.writeString("Knight");
	two.writeString("Testchar Two");
	two.writeWord(120);
	two.writeString("Master Sorcerer");
	CHECK(f.call(two) == newRecord);
	CHECK(f.storage.countRows("online_players") == 2);
	WorldRecord world = *f.storage.getWorld(f.worldId);
	CHECK(world.onlineRecord == 2);
	CHECK(world.onlineRecordTime == StartTime);

	// The same number again is not a record; the list is replaced.
	CHECK(f.call(two) == noRecord);
	CHECK(f.storage.countRows("online_players") == 2);

	// 0xFFFF: the world goes offline.
	WriteBuffer offline = request(QUERY_CREATE_PLAYERLIST);
	offline.writeWord(0xFFFF);
	CHECK(f.call(offline) == noRecord);
	CHECK(f.storage.countRows("online_players") == 0);
	world = *f.storage.getWorld(f.worldId);
	CHECK(!world.online);
	CHECK(world.onlineRecord == 2);

	// A list that is shorter than its count is refused and changes nothing.
	WriteBuffer broken = request(QUERY_CREATE_PLAYERLIST);
	broken.writeWord(2);
	broken.writeString("Testchar One");
	broken.writeWord(8);
	broken.writeString("Knight");
	CHECK(isFailed(f.call(broken)));
	CHECK(f.storage.countRows("online_players") == 0);
}

void testStatistics() {
	Fixture f;
	f.login();

	WriteBuffer kills = request(QUERY_LOG_KILLED_CREATURES);
	kills.writeWord(2);
	kills.writeString("rat");
	kills.writeQuad(0);
	kills.writeQuad(17);
	kills.writeString("dragon");
	kills.writeQuad(4);
	kills.writeQuad(1);
	CHECK(isOk(f.call(kills)));
	CHECK(f.storage.countRows("killed_creatures") == 2);

	WriteBuffer none = request(QUERY_LOG_KILLED_CREATURES);
	none.writeWord(0);
	CHECK(isOk(f.call(none)));
	CHECK(f.storage.countRows("killed_creatures") == 2);

	WriteBuffer death = request(QUERY_LOG_CHARACTER_DEATH);
	death.writeQuad(10);
	death.writeWord(22);
	death.writeQuad(20);
	death.writeString("a dragon");
	death.writeFlag(true);
	death.writeQuad(StartTime - 3);
	CHECK(isOk(f.call(death)));
	CHECK(f.storage.countRows("character_deaths") == 1);
}

void testBuddies() {
	Fixture f;
	f.login();
	f.addAccount(111111, "pw one");
	f.addCharacter(10, "Testchar One", 111111);
	f.addCharacter(20, "Testchar Two");

	WriteBuffer add = request(QUERY_ADD_BUDDY);
	add.writeQuad(111111);
	add.writeQuad(20);
	CHECK(isOk(f.call(add)));
	CHECK(isOk(f.call(add)));               // adding twice is harmless
	std::vector<PlayerEntry> buddies = f.storage.listBuddies(111111);
	CHECK(buddies.size() == 1 && buddies[0].id == 20 && buddies[0].name == "Testchar Two");

	WriteBuffer remove = request(QUERY_REMOVE_BUDDY);
	remove.writeQuad(111111);
	remove.writeQuad(20);
	CHECK(isOk(f.call(remove)));
	CHECK(isOk(f.call(remove)));
	CHECK(f.storage.listBuddies(111111).empty());
}

void testHouses() {
	Fixture f;
	f.login();
	f.addCharacter(10, "Testchar One");
	f.addCharacter(20, "Testchar Two");

	WriteBuffer insert = request(QUERY_INSERT_HOUSES);
	insert.writeWord(2);
	insert.writeWord(101);
	insert.writeString("Test Lane 1");
	insert.writeQuad(1500);
	insert.writeString("It has two beds.");
	insert.writeWord(40);
	insert.writeWord(32100);
	insert.writeWord(32200);
	insert.writeByte(7);
	insert.writeString("Testtown");
	insert.writeFlag(false);
	insert.writeWord(102);
	insert.writeString("Test Hall");
	insert.writeQuad(9000);
	insert.writeString("");
	insert.writeWord(300);
	insert.writeWord(32110);
	insert.writeWord(32210);
	insert.writeByte(6);
	insert.writeString("Testtown");
	insert.writeFlag(true);
	CHECK(isOk(f.call(insert)));
	CHECK(f.storage.countRows("houses") == 2);
	// The list is replaced, not appended to.
	CHECK(isOk(f.call(insert)));
	CHECK(f.storage.countRows("houses") == 2);

	const std::vector<uint8_t> empty = {STATUS_OK, 0x00, 0x00};
	CHECK(f.call(request(QUERY_GET_HOUSE_OWNERS)) == empty);

	WriteBuffer owner = request(QUERY_INSERT_HOUSE_OWNER);
	owner.writeWord(102);
	owner.writeQuad(20);
	owner.writeQuad(StartTime + 1000);
	CHECK(isOk(f.call(owner)));
	CHECK(isFailed(f.call(owner)));          // already has an owner
	WriteBuffer owner2 = request(QUERY_INSERT_HOUSE_OWNER);
	owner2.writeWord(101);
	owner2.writeQuad(777);                   // a character that is not known
	owner2.writeQuad(StartTime + 2000);
	CHECK(isOk(f.call(owner2)));

	std::vector<uint8_t> response = f.call(request(QUERY_GET_HOUSE_OWNERS));
	ReadBuffer in(response);
	CHECK(in.readByte() == STATUS_OK);
	CHECK(in.readWord() == 2);
	CHECK(in.readWord() == 101);
	CHECK(in.readQuad() == 777);
	CHECK(in.readString() == "");
	CHECK(in.readQuad() == StartTime + 2000);
	CHECK(in.readWord() == 102);
	CHECK(in.readQuad() == 20);
	CHECK(in.readString() == "Testchar Two");
	CHECK(in.readQuad() == StartTime + 1000);
	CHECK(in.eof());

	WriteBuffer update = request(QUERY_UPDATE_HOUSE_OWNER);
	update.writeWord(101);
	update.writeQuad(10);
	update.writeQuad(StartTime + 3000);
	CHECK(isOk(f.call(update)));
	std::vector<HouseOwner> owners = f.storage.listHouseOwners(f.worldId);
	CHECK(owners.size() == 2 && owners[0].ownerId == 10 && owners[0].ownerName == "Testchar One"
		&& owners[0].paidUntil == StartTime + 3000);

	WriteBuffer remove = request(QUERY_DELETE_HOUSE_OWNER);
	remove.writeWord(101);
	CHECK(isOk(f.call(remove)));
	CHECK(isOk(f.call(remove)));             // no entry any more: still fine
	CHECK(f.storage.listHouseOwners(f.worldId).size() == 1);

	// The game updates a house right after deleting its entry when it has
	// thrown the tenant out (owner 0). That succeeds and creates nothing.
	WriteBuffer updateGone = request(QUERY_UPDATE_HOUSE_OWNER);
	updateGone.writeWord(101);
	updateGone.writeQuad(0);
	updateGone.writeQuad(StartTime + 3000);
	CHECK(isOk(f.call(updateGone)));
	CHECK(f.storage.listHouseOwners(f.worldId).size() == 1);
	CHECK(f.storage.countRows("house_owners") == 1);

	// A failing request leaves no trace: the second house repeats the id of
	// the first, so the whole new list is refused and the old one stays.
	WriteBuffer duplicate = request(QUERY_INSERT_HOUSES);
	duplicate.writeWord(2);
	for (int i = 0; i < 2; i++) {
		duplicate.writeWord(300);
		duplicate.writeString("Twice");
		duplicate.writeQuad(1);
		duplicate.writeString("");
		duplicate.writeWord(1);
		duplicate.writeWord(1);
		duplicate.writeWord(1);
		duplicate.writeByte(7);
		duplicate.writeString("Testtown");
		duplicate.writeFlag(false);
	}
	CHECK(isFailed(f.call(duplicate)));
	CHECK(f.storage.countRows("houses") == 2);
	CHECK(f.storage.listHouseOwners(f.worldId).size() == 1);

	// An owner entry of a house that is not in the list is not reported.
	f.addHouses({101});
	CHECK(f.storage.listHouseOwners(f.worldId).empty());
}

void testAuctions() {
	Fixture f;
	f.login();
	f.addHouses({101, 102, 103});
	f.addCharacter(10, "Testchar One");

	const std::vector<uint8_t> empty = {STATUS_OK, 0x00, 0x00};
	CHECK(f.call(request(QUERY_GET_AUCTIONS)) == empty);
	CHECK(f.call(request(QUERY_FINISH_AUCTIONS)) == empty);

	WriteBuffer start = request(QUERY_START_AUCTION);
	start.writeWord(102);
	CHECK(isOk(f.call(start)));
	CHECK(isFailed(f.call(start)));          // already up for auction
	WriteBuffer start2 = request(QUERY_START_AUCTION);
	start2.writeWord(101);
	CHECK(isOk(f.call(start2)));
	WriteBuffer unknown = request(QUERY_START_AUCTION);
	unknown.writeWord(999);
	CHECK(isFailed(f.call(unknown)));

	const std::vector<uint8_t> two = {STATUS_OK, 0x02, 0x00, 101, 0x00, 102, 0x00};
	CHECK(f.call(request(QUERY_GET_AUCTIONS)) == two);

	// No bid: nothing to finish, however much time passes.
	f.now = StartTime + 100000;
	CHECK(f.call(request(QUERY_FINISH_AUCTIONS)) == empty);
	f.now = StartTime;

	// A bid whose auction ends later is not finished yet.
	CHECK(f.storage.setAuctionBid(f.worldId, 102, 10, 25000, StartTime + 60));
	CHECK(!f.storage.setAuctionBid(f.worldId, 103, 10, 1, StartTime));
	CHECK(f.call(request(QUERY_FINISH_AUCTIONS)) == empty);

	f.now = StartTime + 60;
	std::vector<uint8_t> response = f.call(request(QUERY_FINISH_AUCTIONS));
	ReadBuffer in(response);
	CHECK(in.readByte() == STATUS_OK);
	CHECK(in.readWord() == 1);
	CHECK(in.readWord() == 102);
	CHECK(in.readQuad() == 10);
	CHECK(in.readString() == "Testchar One");
	CHECK(in.readQuad() == 25000);
	CHECK(in.eof());

	// It is handed out once and is no auction any more.
	CHECK(f.call(request(QUERY_FINISH_AUCTIONS)) == empty);
	const std::vector<uint8_t> one = {STATUS_OK, 0x01, 0x00, 101, 0x00};
	CHECK(f.call(request(QUERY_GET_AUCTIONS)) == one);

	// Excluding the bidder withdraws the bid; the auction goes on.
	CHECK(f.storage.setAuctionBid(f.worldId, 101, 10, 500, StartTime + 70));
	WriteBuffer exclude = request(QUERY_EXCLUDE_FROM_AUCTIONS);
	exclude.writeQuad(10);
	exclude.writeFlag(true);
	CHECK(isOk(f.call(exclude)));
	CHECK(f.storage.countRows("auction_exclusions") == 1);
	f.now = StartTime + 1000;
	CHECK(f.call(request(QUERY_FINISH_AUCTIONS)) == empty);
	CHECK(f.call(request(QUERY_GET_AUCTIONS)) == one);
}

void testTransfers() {
	Fixture f;
	f.login();
	f.addHouses({101, 102, 103});
	f.addCharacter(10, "Testchar One");
	f.addCharacter(20, "Testchar Two");
	f.storage.insertHouseOwner(f.worldId, 101, 10, StartTime + 5000);
	f.storage.insertHouseOwner(f.worldId, 102, 10, StartTime + 5000);
	f.storage.insertHouseOwner(f.worldId, 103, 10, StartTime + 5000);

	const std::vector<uint8_t> empty = {STATUS_OK, 0x00, 0x00};
	CHECK(f.call(request(QUERY_TRANSFER_HOUSES)) == empty);

	// 101: moving out later. 102: sold, accepted. 103: offered, not accepted.
	CHECK(f.storage.setHouseTransfer(f.worldId, 101, StartTime + 100, 0, false, 0));
	CHECK(f.storage.setHouseTransfer(f.worldId, 102, StartTime, 20, true, 40000));
	CHECK(f.storage.setHouseTransfer(f.worldId, 103, StartTime, 20, false, 1));
	CHECK(!f.storage.setHouseTransfer(f.worldId, 999, StartTime, 0, false, 0));

	std::vector<uint8_t> response = f.call(request(QUERY_TRANSFER_HOUSES));
	ReadBuffer in(response);
	CHECK(in.readByte() == STATUS_OK);
	CHECK(in.readWord() == 1);
	CHECK(in.readWord() == 102);
	CHECK(in.readQuad() == 20);
	CHECK(in.readString() == "Testchar Two");
	CHECK(in.readQuad() == 40000);
	CHECK(in.eof());

	// Reading does not consume: the game removes the owner entry itself.
	CHECK(f.call(request(QUERY_TRANSFER_HOUSES)) == response);

	// The buyer cannot pay: the game cancels the transfer.
	WriteBuffer cancel = request(QUERY_CANCEL_HOUSE_TRANSFER);
	cancel.writeWord(102);
	CHECK(isOk(f.call(cancel)));
	CHECK(f.call(request(QUERY_TRANSFER_HOUSES)) == empty);
	// Cancelling where there is no entry changes nothing and succeeds.
	WriteBuffer cancelMissing = request(QUERY_CANCEL_HOUSE_TRANSFER);
	cancelMissing.writeWord(999);
	CHECK(isOk(f.call(cancelMissing)));
	CHECK(f.storage.countRows("house_owners") == 3);

	// The move-out becomes due; paying the rent in between does not drop it.
	WriteBuffer update = request(QUERY_UPDATE_HOUSE_OWNER);
	update.writeWord(101);
	update.writeQuad(10);
	update.writeQuad(StartTime + 9000);
	CHECK(isOk(f.call(update)));
	f.now = StartTime + 100;
	response = f.call(request(QUERY_TRANSFER_HOUSES));
	ReadBuffer due(response);
	CHECK(due.readByte() == STATUS_OK);
	CHECK(due.readWord() == 1);
	CHECK(due.readWord() == 101);
	CHECK(due.readQuad() == 0);
	CHECK(due.readString() == "");
	CHECK(due.readQuad() == 0);
	CHECK(due.eof());
}

void testEvictions() {
	Fixture f;
	f.login();
	f.addHouses({101, 102, 103, 104});
	f.addAccount(111111, "pw one", StartTime + 100);     // premium
	f.addAccount(222222, "pw two", StartTime);            // premium just ran out
	f.addCharacter(10, "Testchar One", 111111);
	f.addCharacter(20, "Testchar Two", 222222);
	f.addCharacter(30, "Testchar Three");                 // no account
	CharacterRecord deleted = f.addCharacter(40, "Testchar Gone", 111111);
	deleted.deleted = true;
	f.storage.putCharacter(deleted);
	f.storage.insertHouseOwner(f.worldId, 101, 10, 0);
	f.storage.insertHouseOwner(f.worldId, 102, 20, 0);
	f.storage.insertHouseOwner(f.worldId, 103, 30, 0);
	f.storage.insertHouseOwner(f.worldId, 104, 40, 0);

	// Only the owner with a known, free account is listed.
	const std::vector<uint8_t> freeAccounts = {STATUS_OK, 0x01, 0x00, 102, 0x00, 20, 0x00, 0x00, 0x00};
	CHECK(f.call(request(QUERY_EVICT_FREE_ACCOUNTS)) == freeAccounts);

	const std::vector<uint8_t> deletedOwners = {STATUS_OK, 0x01, 0x00, 104, 0x00};
	CHECK(f.call(request(QUERY_EVICT_DELETED_CHARACTERS)) == deletedOwners);

	// Guild leaders: 10 leads a guild, 20 is a plain member, 30 has no guild
	// recorded, 50 is not known at all.
	CharacterRecord leader = *f.storage.findCharacter(10);
	leader.guild = "Test Guild";
	leader.guildLeader = true;
	f.storage.putCharacter(leader);
	CharacterRecord member = *f.storage.findCharacter(20);
	member.guild = "Test Guild";
	f.storage.putCharacter(member);

	WriteBuffer guildhouses = request(QUERY_EVICT_EX_GUILDLEADERS);
	guildhouses.writeWord(4);
	guildhouses.writeWord(101);
	guildhouses.writeQuad(10);
	guildhouses.writeWord(102);
	guildhouses.writeQuad(20);
	guildhouses.writeWord(103);
	guildhouses.writeQuad(30);
	guildhouses.writeWord(104);
	guildhouses.writeQuad(50);
	const std::vector<uint8_t> exLeaders = {STATUS_OK, 0x01, 0x00, 102, 0x00};
	CHECK(f.call(guildhouses) == exLeaders);

	WriteBuffer noGuildhouses = request(QUERY_EVICT_EX_GUILDLEADERS);
	noGuildhouses.writeWord(0);
	const std::vector<uint8_t> empty = {STATUS_OK, 0x00, 0x00};
	CHECK(f.call(noGuildhouses) == empty);
}

void testRejectedRequests() {
	Fixture f;
	f.login();

	// Catalogued but not implemented: every one of them fails.
	size_t count = 0;
	const QueryInfo *table = queryTable(count);
	for (size_t i = 0; i < count; i++) {
		if (!table[i].implemented) {
			CHECK(isFailed(f.call(request(table[i].type))));
		}
	}
	// A type the game never sends.
	CHECK(isFailed(f.call(request(1))));
	CHECK(isFailed(f.call(request(255))));
	// An empty payload cannot come out of the framing, but must not crash.
	CHECK(isFailed(f.dispatcher.handle(f.session, std::vector<uint8_t>())));

	// A request that is longer than its layout: the layout is wrong.
	WriteBuffer tooLong = request(QUERY_LOAD_WORLD_CONFIG);
	tooLong.writeByte(0);
	CHECK(isFailed(f.call(tooLong)));
	// And one that is shorter.
	WriteBuffer tooShort = request(QUERY_LOAD_PLAYERS);
	tooShort.writeWord(0);
	CHECK(isFailed(f.call(tooShort)));

	// The connection still works afterwards.
	CHECK(f.call(request(QUERY_LOAD_WORLD_CONFIG))[0] == STATUS_OK);
}

// Made-up player data with every kind of field, negative numbers and item
// text with ISO-8859-1 letters and line breaks.
PlayerData samplePlayerData(uint32_t characterId) {
	PlayerData data;
	data.characterId = characterId;
	data.name = "M\xE4" "de Up";
	data.race = 1;
	data.profession = 12;
	data.originalOutfit.id = 130;
	data.originalOutfit.colors[0] = 78;
	data.originalOutfit.colors[1] = 69;
	data.originalOutfit.colors[2] = 58;
	data.originalOutfit.colors[3] = 76;
	data.currentOutfit.id = 0;
	data.currentOutfit.objectType = 3058;
	data.lastLogin = 1146000000;
	data.lastLogout = 1146003600;
	data.startX = 32369;
	data.startY = 32241;
	data.startZ = 7;
	data.posX = 32370;
	data.posY = 32242;
	data.posZ = 6;
	data.playerkillerEnd = -1;
	PlayerSkill level;
	level.number = 0;
	level.actual = 8;
	level.maximum = 8;
	level.minimum = 1;
	level.experience = 4200;
	level.delta = -3;
	PlayerSkill fist;
	fist.number = 7;
	fist.actual = 10;
	fist.nextLevel = 50;
	fist.factorPercent = 150;
	data.skills = {level, fist};
	data.spells = {1, 22, 255};
	PlayerQuestValue quest;
	quest.number = 499;
	quest.value = -7;
	data.questValues = {quest};
	data.murders = {1146000100, 1146000200};
	data.inventory = "{1 Content={3355},\n               3 Content={1987 Content={3031 Amount=12, "
		"2820 String=\"Gr\xFC\xDF \\\"dich\\\"\"}}}";
	PlayerDepot depot;
	depot.number = 2;
	depot.content = "{2590 Content={2594}}";
	data.depots = {depot};
	return data;
}

WriteBuffer savePlayerDataRequest(const PlayerData &data) {
	WriteBuffer buffer = request(QUERY_SAVE_PLAYER_DATA);
	buffer.writeQuad(data.characterId);
	writePlayerData(buffer, data);
	return buffer;
}

WriteBuffer loadPlayerDataRequest(uint32_t characterId) {
	WriteBuffer buffer = request(QUERY_LOAD_PLAYER_DATA);
	buffer.writeQuad(characterId);
	return buffer;
}

void testPlayerData() {
	Fixture f;
	f.login();
	f.addCharacter(500, "Made Up");

	// Nothing stored: OK with the flag clear.
	std::vector<uint8_t> response = f.call(loadPlayerDataRequest(500));
	CHECK(response == std::vector<uint8_t>({STATUS_OK, 0}));

	PlayerData data = samplePlayerData(500);
	CHECK(isOk(f.call(savePlayerDataRequest(data))));
	CHECK(f.storage.hasPlayerData(500));
	response = f.call(loadPlayerDataRequest(500));
	CHECK(response.size() > 2 && response[0] == STATUS_OK && response[1] == 1);
	if (response.size() > 2) {
		ReadBuffer in(response.data() + 2, response.size() - 2);
		PlayerData loaded;
		loaded.characterId = 500;
		readPlayerData(in, loaded);
		CHECK(in.eof());
		CHECK(loaded == data);
	}
	// The response carries exactly the bytes of the saved record.
	WriteBuffer record;
	writePlayerData(record, data);
	CHECK(std::vector<uint8_t>(response.begin() + 2, response.end()) == record.data());

	// A save replaces everything, the lists too.
	PlayerData smaller = data;
	smaller.skills.resize(1);
	smaller.spells.clear();
	smaller.questValues.clear();
	smaller.murders = {5};
	smaller.depots.clear();
	smaller.inventory = "{}";
	smaller.currentOutfit = data.originalOutfit;
	CHECK(isOk(f.call(savePlayerDataRequest(smaller))));
	CHECK(f.storage.loadPlayerData(500) == smaller);

	// A character the characters table does not know may have player data
	// (the game saves what it loaded for any house owner).
	PlayerData unknown = samplePlayerData(777);
	CHECK(isOk(f.call(savePlayerDataRequest(unknown))));
	CHECK(f.storage.loadPlayerData(777) == unknown);

	// Not one of another world, and not character 0.
	CharacterRecord stranger;
	stranger.id = 600;
	stranger.worldId = f.otherWorldId;
	stranger.name = "Made Up Stranger";
	f.storage.putCharacter(stranger);
	CHECK(isFailed(f.call(savePlayerDataRequest(samplePlayerData(600)))));
	CHECK(isFailed(f.call(loadPlayerDataRequest(600))));
	CHECK(!f.storage.hasPlayerData(600));
	CHECK(isFailed(f.call(savePlayerDataRequest(samplePlayerData(0)))));

	// A record that ends early is refused and changes nothing.
	WriteBuffer truncated = request(QUERY_SAVE_PLAYER_DATA);
	truncated.writeQuad(500);
	truncated.writeString("Made Up");
	CHECK(isFailed(f.call(truncated)));
	CHECK(f.storage.loadPlayerData(500) == smaller);

	// A save whose game has closed the connection (it gave up waiting and
	// keeps the record in a file) is refused and changes nothing; with the
	// connection open it is stored.
	int sockets[2];
	CHECK(socketpair(AF_UNIX, SOCK_STREAM, 0, sockets) == 0);
	f.session.socket = sockets[0];
	CHECK(isOk(f.call(savePlayerDataRequest(data))));
	CHECK(f.storage.loadPlayerData(500) == data);
	close(sockets[1]);
	CHECK(isFailed(f.call(savePlayerDataRequest(smaller))));
	CHECK(f.storage.loadPlayerData(500) == data);
	close(sockets[0]);
	f.session.socket = -1;

	// Only a game server (a session bound to a world) may ask.
	Fixture g;
	g.storage.setApplicationPassword(APPLICATION_LOGIN, "login");
	WriteBuffer login = request(QUERY_LOGIN);
	login.writeByte(APPLICATION_LOGIN);
	login.writeString("login");
	CHECK(isOk(g.call(login)));
	CHECK(isFailed(g.call(loadPlayerDataRequest(500))));
}

// A database of schema version 1 (before the player data) is brought up to
// date when it is opened, and keeps what it had.
void testSchemaMigration() {
	char path[] = "/tmp/qm_test_migration_XXXXXX";
	int fd = mkstemp(path);
	CHECK(fd >= 0);
	if (fd < 0) {
		return;
	}
	close(fd);
	{
		SqliteStorage storage(path);
		WorldConfig config;
		uint32_t worldId = storage.putWorld("Testia", config, false);
		CharacterRecord character;
		character.id = 9;
		character.worldId = worldId;
		character.name = "Made Up Old";
		storage.putCharacter(character);
	}
	// Turn it into a version 1 database.
	sqlite3 *db = nullptr;
	CHECK(sqlite3_open(path, &db) == SQLITE_OK);
	CHECK(sqlite3_exec(db, "DROP TABLE player_data; DROP TABLE player_skills; DROP TABLE player_spells;"
		" DROP TABLE player_quest_values; DROP TABLE player_murders; DROP TABLE player_depots;"
		" UPDATE meta SET value = '1' WHERE key = 'schema_version'", nullptr, nullptr, nullptr) == SQLITE_OK);
	sqlite3_close(db);
	{
		SqliteStorage storage(path);
		CHECK(storage.findCharacter(9).has_value());
		CHECK(storage.countRows("player_data") == 0);
		PlayerData data = samplePlayerData(9);
		storage.savePlayerData(data);
		CHECK(storage.loadPlayerData(9) == data);
	}
	db = nullptr;
	CHECK(sqlite3_open(path, &db) == SQLITE_OK);
	sqlite3_stmt *stmt = nullptr;
	CHECK(sqlite3_prepare_v2(db, "SELECT value FROM meta WHERE key = 'schema_version'", -1, &stmt, nullptr)
		== SQLITE_OK);
	CHECK(sqlite3_step(stmt) == SQLITE_ROW && sqlite3_column_int(stmt, 0) == 2);
	sqlite3_finalize(stmt);
	// A version this program does not know is refused.
	CHECK(sqlite3_exec(db, "UPDATE meta SET value = '3' WHERE key = 'schema_version'", nullptr, nullptr,
		nullptr) == SQLITE_OK);
	sqlite3_close(db);
	CHECK_THROWS(SqliteStorage storage(path), StorageError);
	for (const char *suffix : {"", "-wal", "-shm"}) {
		std::remove((std::string(path) + suffix).c_str());
	}
}

void testStorageBasics() {
	SqliteStorage storage(":memory:");
	WorldConfig config;
	uint32_t worldId = storage.putWorld("Testia", config, true);
	// Writing the world again updates it and keeps the id.
	config.maxPlayers = 5;
	CHECK(storage.putWorld("testia", config, false) == worldId);
	WorldRecord world = *storage.findWorld("TESTIA");
	CHECK(world.id == worldId && world.config.maxPlayers == 5 && !world.privateWorld);
	CHECK(!storage.findWorld("Nowhere"));
	CHECK(!storage.getWorld(worldId + 1));

	// Names keep their bytes, and upper and lower case match for the
	// accented letters of ISO-8859-1 as well.
	CharacterRecord character;
	character.id = 7;
	character.worldId = worldId;
	character.name = "T\xE9st\xF6r";
	storage.putCharacter(character);
	CHECK(storage.findCharacterByName("T\xC9ST\xD6R")->name == "T\xE9st\xF6r");
	CHECK(storage.findCharacter(7)->name == "T\xE9st\xF6r");

	// A second character may not take the name of the first.
	CharacterRecord twin;
	twin.id = 8;
	twin.worldId = worldId;
	twin.name = "t\xE9st\xF6r";
	CHECK_THROWS(storage.putCharacter(twin), StorageError);
	CHECK(storage.findCharacter(7).has_value());
	CHECK(!storage.findCharacter(8));

	// Transactions.
	storage.begin();
	storage.setApplicationPassword(3, "x");
	storage.rollback();
	CHECK(!storage.checkApplicationPassword(3, "x"));
	storage.begin();
	storage.setApplicationPassword(3, "x");
	storage.commit();
	CHECK(storage.checkApplicationPassword(3, "x"));
	CHECK(!storage.checkApplicationPassword(3, "y"));
	CHECK_THROWS(storage.commit(), StorageError);
}

void all() {
	// The rejected requests are logged as errors on purpose; keep them out
	// of the test output.
	setLogFile("/dev/null");
	testLogin();
	testLoginAccount();
	testWorldConfig();
	testLoadPlayers();
	testLoginGameSuccess();
	testLoginGameErrors();
	testLoginGameWithoutAccount();
	testLogout();
	testIsOnline();
	testPlayerlist();
	testStatistics();
	testBuddies();
	testHouses();
	testAuctions();
	testTransfers();
	testEvictions();
	testRejectedRequests();
	testPlayerData();
	testSchemaMigration();
	testStorageBasics();
}

} // namespace

int main() {
	return test::run(all);
}
