// query.cc: the game server's client of the query manager. TQueryManagerConnection, one connection with a request
// for each query the game sends and the reading of its answer; TQueryManagerConnectionPool, the connections the
// communication threads share; and the login data each connection sends first (SetQueryManagerLoginData).

#include <algorithm>
#include <cstdint>
#include <cstdlib>
#include <time.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <poll.h>
#include <netdb.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "script.hh"
#include "config.hh"
#include "threads.hh"
#include "objects.hh"
#include "map.hh"
#include "info.hh"
#include "dbfuncs.hh"
#include "query.hh"
#include "utils.hh"

int ApplicationType;
char LoginData[30];

// The word that stands for the size of a request or response that does not
// fit in a word: the size follows as a quad.
const int QUERY_LONG_SIZE_MARKER = 0xFFFF;

void SetQueryManagerLoginData(int Type, char *Data) {
	ApplicationType = Type;
	if (Data != nullptr) {
		strncpy(LoginData, Data, sizeof(LoginData));
	} else {
		LoginData[0] = 0;
	}
}

// 16384 is the smallest buffer a connection gets. It stays a number: a named
// constant given to std::max by reference compiles to different code.
TQueryManagerConnection::TQueryManagerConnection(int QueryBufferSize) : Buffer(new unsigned char[std::max(QueryBufferSize, 16384)]), ReadBuffer(Buffer, std::max(QueryBufferSize, 16384)), WriteBuffer(Buffer, std::max(QueryBufferSize, 16384)) {
	BufferSize = std::max(QueryBufferSize, 16384);
	Socket = -1;
	QueryOk = false;
	connect();
}

TQueryManagerConnection::~TQueryManagerConnection() {
	disconnect();
	delete[] Buffer;
}

void TQueryManagerConnection::connect() {
	for (int Try = 0; Try < NumberOfQueryManagers; Try++) {
		sockaddr_in QueryManagerAddress;
		memset(&QueryManagerAddress, 0, sizeof(QueryManagerAddress));
		QueryManagerAddress.sin_addr.s_addr = inet_addr(QUERY_MANAGER[Try].Host);
		if (QueryManagerAddress.sin_addr.s_addr == INADDR_NONE) {
			hostent HostEnt;
			hostent *HostEntAddr;
			char Buffer[2048];
			int ErrorCode = 0;
			int nss_status = gethostbyname_r(QUERY_MANAGER[Try].Host, &HostEnt, Buffer, sizeof(Buffer), &HostEntAddr, &ErrorCode);
			if (nss_status != 0 || HostEntAddr == nullptr) {
				print(2, "TQueryManagerConnection::connect: Kann Rechnernamen nicht aufl\xf6sen.\n");
				continue;
			}

			in_addr *IPAddress = reinterpret_cast<in_addr *>(HostEntAddr->h_addr_list[0]);
			QueryManagerAddress.sin_addr.s_addr = inet_addr(inet_ntoa(*IPAddress));
		}

		Socket = socket(AF_INET, SOCK_STREAM, 0);
		if (Socket < 0) {
			print(2, "TQueryManagerConnection::connect: Kann Socket nicht \xf6" "ffnen.\n");
			continue;
		}

		QueryManagerAddress.sin_family = AF_INET;
		QueryManagerAddress.sin_port = htons(static_cast<unsigned short>(QUERY_MANAGER[Try].Port));
		if (::connect(Socket, reinterpret_cast<sockaddr *>(&QueryManagerAddress), sizeof(QueryManagerAddress)) < 0) {
			print(2, "TQueryManagerConnection::connect: Kann Verbindung nicht herstellen.\n");
			disconnect();
			continue;
		}

		prepareQuery(QUERY_LOGIN);
		sendByte(static_cast<unsigned char>(ApplicationType));
		sendString(QUERY_MANAGER[Try].Password);
		if (ApplicationType == APPLICATION_GAME) {
			sendString(LoginData);
		}

		int Result = executeQuery(30, false);
		if (Result == STATUS_OK) {
			return;
		}

		print(2, "TQueryManagerConnection::connect: Anmeldung fehlgeschlagen (%d).\n", Result);
		disconnect();
	}

	print(2, "TQueryManagerConnection::connect: Kein Query-Manager verf\xfcgbar.\n");
	Socket = -1;
}

void TQueryManagerConnection::disconnect() {
	if (isConnected()) {
		if (close(Socket) < 0) {
			error("TQueryManagerConnection::disconnect: Fehler %d beim Schlie\xdf" "en der Socket.\n", errno);
		}
		Socket = -1;
	}
}

int TQueryManagerConnection::write(unsigned char *Buffer, int Size) {
	int Chances = 50;
	int nleft = Size;
	while (nleft > 0) {
		int nwritten = static_cast<int>(::write(Socket, Buffer, nleft));
		if (nwritten == -1 && errno == EAGAIN && Chances > 0) {
			DelayThread(0, 100000);
			Chances--;
			continue;
		}

		if (nwritten <= 0) {
			return nwritten;
		}

		nleft -= nwritten;
		Buffer += nwritten;
	}

	return Size - nleft;
}

int TQueryManagerConnection::read(unsigned char *Buffer, int Size, int Timeout) {
	int Chances = 50;
	int nleft = Size;
	pollfd ufds;
	while (nleft > 0) {
		ufds.fd = Socket;
		ufds.events = POLLIN;
		ufds.revents = 0;
		if (poll(&ufds, 1, Timeout * 1000) <= 0) {
			return -2;
		}

		int nread = static_cast<int>(::read(Socket, Buffer, nleft));
		if (nread == 0) {
			break;
		}

		if (nread < 0) {
			if (errno == EINTR) {
				continue;
			}

			if (errno != EAGAIN || nleft == Size || Chances == 0) {
				return nread;
			}

			Chances--;
			DelayThread(0, 100000);
			continue;
		}

		nleft -= nread;
		Buffer += nread;
	}

	return Size - nleft;
}

void TQueryManagerConnection::prepareQuery(int QueryType) {
	try {
		QueryOk = true;
		WriteBuffer.rewind();
		WriteBuffer.writeWord(0);
		WriteBuffer.writeByte(static_cast<unsigned char>(QueryType));
	} catch (const char *s) {
		error("TQueryManagerConnection::prepareQuery: %s.\n", s);
		QueryOk = false;
	}
}

void TQueryManagerConnection::sendFlag(bool f) {
	WriteBuffer.writeByte(f ? 1 : 0);
}

void TQueryManagerConnection::sendByte(unsigned char b) {
	try {
		WriteBuffer.writeByte(b);
	} catch (const char *s) {
		error("TQueryManagerConnection::sendByte: %s.\n", s);
		QueryOk = false;
	}
}

void TQueryManagerConnection::sendWord(unsigned short w) {
	try {
		WriteBuffer.writeWord(w);
	} catch (const char *s) {
		error("TQueryManagerConnection::sendWord: %s.\n", s);
		QueryOk = false;
	}
}

void TQueryManagerConnection::sendQuad(uint32_t q) {
	try {
		WriteBuffer.writeQuad(q);
	} catch (const char *s) {
		error("TQueryManagerConnection::sendQuad: %s.\n", s);
		QueryOk = false;
	}
}

void TQueryManagerConnection::sendString(char *s) {
	try {
		WriteBuffer.writeString(s);
	} catch (const char *s) {
		error("TQueryManagerConnection::sendString: %s.\n", s);
		QueryOk = false;
	}
}

bool TQueryManagerConnection::getFlag() {
	return getByte() != 0;
}

unsigned char TQueryManagerConnection::getByte() {
	try {
		return ReadBuffer.readByte();
	} catch (const char *s) {
		error("TQueryManagerConnection::getByte: %s.\n", s);
		return 0;
	}
}

unsigned short TQueryManagerConnection::getWord() {
	try {
		return ReadBuffer.readWord();
	} catch (const char *s) {
		error("TQueryManagerConnection::getWord: %s.\n", s);
		return 0;
	}
}

uint32_t TQueryManagerConnection::getQuad() {
	try {
		return ReadBuffer.readQuad();
	} catch (const char *s) {
		error("TQueryManagerConnection::getQuad: %s.\n", s);
		return 0;
	}
}

void TQueryManagerConnection::getString(char *Buffer, int MaxLength) {
	try {
		ReadBuffer.readString(Buffer, MaxLength);
	} catch (const char *s) {
		error("TQueryManagerConnection::getString: %s.\n", s);
		Buffer[0] = 0;
	}
}

int TQueryManagerConnection::executeQuery(int Timeout, bool AutoReconnect) {
	int DataSize = WriteBuffer.getDataSize();
	WriteBuffer.rewind();
	if (DataSize < QUERY_LONG_SIZE_MARKER) {
		sendWord(static_cast<unsigned short>(DataSize - 2));
	} else {
		DataSize += 4;
		if (DataSize > BufferSize) {
			error("TQueryManagerConnection::executeQuery: Puffer zu klein.\n");
			return STATUS_FAILED;
		}

		memmove(Buffer + 6, Buffer + 2, DataSize - 6);
		sendWord(QUERY_LONG_SIZE_MARKER);
		sendQuad(DataSize - 6);
	}

	bool SecondChance = true;
	if (!QueryOk) {
		error("TQueryManagerConnection::executeQuery: Fehler beim Zusammenbauen der Anfrage.\n");
		return STATUS_FAILED;
	}

again:
	if (!isConnected()) {
		if (!AutoReconnect) {
			return STATUS_FAILED;
		}

		// connect() sends its login request through the same buffer.
		if (DataSize >= 16384) {
			unsigned char *Help = new unsigned char[DataSize];
			memcpy(Help, Buffer, DataSize);
			connect();
			memcpy(Buffer, Help, DataSize);
			delete[] Help;
		} else {
			unsigned char Help[DataSize];
			memcpy(Help, Buffer, DataSize);
			connect();
			memcpy(Buffer, Help, DataSize);
		}

		if (!isConnected()) {
			return STATUS_FAILED;
		}
	}

	if (write(Buffer, DataSize) != DataSize) {
		disconnect();
		if (SecondChance) {
			SecondChance = false;
			goto again;
		}

		error("TQueryManagerConnection::executeQuery: Fehler beim Abschicken der Anfrage.\n");
		return STATUS_FAILED;
	}

	unsigned char Help[2];
	int Result = read(Help, 2, Timeout);
	if (Result != 2) {
		disconnect();
		if (Result != -2 && SecondChance) {
			SecondChance = false;
			goto again;
		}

		return STATUS_FAILED;
	}

	int Size = Help[0] + (Help[1] << 8);
	if (Size == QUERY_LONG_SIZE_MARKER) {
		unsigned char Help2[4];
		if (read(Help2, 4, Timeout) != 4) {
			disconnect();
			return STATUS_FAILED;
		}

		Size = static_cast<int>(static_cast<uint32_t>(Help2[0])
			+ (static_cast<uint32_t>(Help2[1]) << 8)
			+ (static_cast<uint32_t>(Help2[2]) << 16)
			+ (static_cast<uint32_t>(Help2[3]) << 24));
	}

	if (Size == 0 || Size > BufferSize) {
		disconnect();
		error("TQueryManagerConnection::executeQuery: Ung\xfcltige Datengr\xf6\xdf" "e %d.\n", Size);
		return STATUS_FAILED;
	}

	Result = read(Buffer, Size, Timeout);
	if (Result != Size) {
		disconnect();
		error("TQueryManagerConnection::executeQuery: Fehler beim Auslesen der Daten.\n");
		return STATUS_FAILED;
	}

	ReadBuffer.resize(Result);
	ReadBuffer.rewind();
	Result = getByte();
	if (Result == STATUS_FAILED) {
		error("TQueryManagerConnection::executeQuery: Anfrage fehlgeschlagen.\n");
	}

	return Result;
}

int TQueryManagerConnection::loadWorldConfig(int &WorldType, int &RebootTime, int *IPAddress, int &Port, int &MaxPlayers, int &PremiumPlayerBuffer, int &MaxNewbies, int &PremiumNewbieBuffer) {
	prepareQuery(QUERY_LOAD_WORLD_CONFIG);
	int Result = executeQuery(30, true);
	if (Result != STATUS_OK) {
		return -1;
	}

	WorldType = getByte();
	RebootTime = getByte() * 60;
	for (int i = 0; i < 4; i++) {
		IPAddress[i] = getByte();
	}

	Port = getWord();
	MaxPlayers = getWord();
	PremiumPlayerBuffer = getWord();
	MaxNewbies = getWord();
	PremiumNewbieBuffer = getWord();
	return 0;
}

int TQueryManagerConnection::loginGame(uint32_t AccountID, char *PlayerName, char *Password, char *IPAddress, bool PrivateWorld, bool PremiumAccountRequired, bool GamemasterRequired, uint32_t &CharacterID, int &Sex, char *Guild, char *Rank, char *Title, int &NumberOfBuddies, uint32_t *BuddyIDs, char (*BuddyNames)[30], unsigned char *Rights, bool &PremiumAccountActivated) {
	prepareQuery(QUERY_LOGIN_GAME);
	sendQuad(AccountID);
	sendString(PlayerName);
	sendString(Password);
	sendString(IPAddress);
	sendFlag(PrivateWorld);
	sendFlag(PremiumAccountRequired);
	sendFlag(GamemasterRequired);
	int Result = executeQuery(120, true);
	if (Result == STATUS_OK) {
		CharacterID = getQuad();
		getString(PlayerName, 30);
		Sex = getByte();
		getString(Guild, 30);
		getString(Rank, 30);
		getString(Title, 30);
		NumberOfBuddies = getByte();
		if (NumberOfBuddies > MAX_BUDDIES) {
			error("TQueryManagerConnection::loginGame: zu viele Buddys (%d) f\xfcr %s.\n", NumberOfBuddies, PlayerName);
		}

		for (int i = 0; i < std::min(NumberOfBuddies, MAX_BUDDIES); i++) {
			BuddyIDs[i] = getQuad();
			getString(BuddyNames[i], 30);
		}

		for (int i = MAX_BUDDIES; i < NumberOfBuddies; i++) {
			char Help[30];
			getQuad();
			getString(Help, 30);
		}

		int NumberOfRights = getByte();
		for (int i = 0; i < NumberOfRights; i++) {
			char RightName[30];
			int Right = -1;
			getString(RightName, 30);
			if (strcmp(RightName, "PREMIUM_ACCOUNT") == 0) Right = PREMIUM_ACCOUNT;
			if (strcmp(RightName, "NOTATION") == 0) Right = NOTATION;
			if (strcmp(RightName, "NAMELOCK") == 0) Right = NAMELOCK;
			if (strcmp(RightName, "STATEMENT_REPORT") == 0) Right = STATEMENT_REPORT;
			if (strcmp(RightName, "BANISHMENT") == 0) Right = BANISHMENT;
			if (strcmp(RightName, "FINAL_WARNING") == 0) Right = FINAL_WARNING;
			if (strcmp(RightName, "IP_BANISHMENT") == 0) Right = IP_BANISHMENT;
			if (strcmp(RightName, "KICK") == 0) Right = KICK;
			if (strcmp(RightName, "HOME_TELEPORT") == 0) Right = HOME_TELEPORT;
			if (strcmp(RightName, "CLEAR_CHARACTER_INFO") == 0) Right = -2;
			if (strcmp(RightName, "CLEAR_GUILDS") == 0) Right = -2;
			if (strcmp(RightName, "DELETE_GUILDS") == 0) Right = -2;
			if (strcmp(RightName, "BOARD_REPORT") == 0) Right = -2;
			if (strcmp(RightName, "BOARD_MODERATION") == 0) Right = -2;
			if (strcmp(RightName, "BOARD_ANONYMOUS_EDIT") == 0) Right = -2;
			if (strcmp(RightName, "BOARD_PRECONFIRMED") == 0) Right = -2;
			if (strcmp(RightName, "KEEP_ACCOUNT") == 0) Right = -2;
			if (strcmp(RightName, "GAMEMASTER_BROADCAST") == 0) Right = GAMEMASTER_BROADCAST;
			if (strcmp(RightName, "ANONYMOUS_BROADCAST") == 0) Right = ANONYMOUS_BROADCAST;
			if (strcmp(RightName, "INVITED") == 0) Right = -2;
			if (strcmp(RightName, "NO_BANISHMENT") == 0) Right = NO_BANISHMENT;
			if (strcmp(RightName, "ALLOW_MULTICLIENT") == 0) Right = -2;
			if (strcmp(RightName, "CIPWATCH_ADMIN") == 0) Right = -2;
			if (strcmp(RightName, "CIPWATCH_USER") == 0) Right = -2;
			if (strcmp(RightName, "CREATECHAR_GAMEMASTER") == 0) Right = -2;
			if (strcmp(RightName, "CREATECHAR_GOD") == 0) Right = -2;
			if (strcmp(RightName, "CREATECHAR_TEST") == 0) Right = -2;
			if (strcmp(RightName, "VIEW_ACCOUNT") == 0) Right = -2;
			if (strcmp(RightName, "VIEW_GAMEMASTER_RECORD") == 0) Right = -2;
			if (strcmp(RightName, "VIEW_CRIMINAL_RECORD") == 0) Right = -2;
			if (strcmp(RightName, "VIEW_LOG_FILES") == 0) Right = -2;
			if (strcmp(RightName, "LOG_COMMUNICATION") == 0) Right = LOG_COMMUNICATION;
			if (strcmp(RightName, "MODIFY_BANISHMENT") == 0) Right = -2;
			if (strcmp(RightName, "READ_GAMEMASTER_CHANNEL") == 0) Right = READ_GAMEMASTER_CHANNEL;
			if (strcmp(RightName, "READ_TUTOR_CHANNEL") == 0) Right = READ_TUTOR_CHANNEL;
			if (strcmp(RightName, "HIGHLIGHT_HELP_CHANNEL") == 0) Right = HIGHLIGHT_HELP_CHANNEL;
			if (strcmp(RightName, "SEND_BUGREPORTS") == 0) Right = SEND_BUGREPORTS;
			if (strcmp(RightName, "APPOINT_CIP") == 0) Right = -2;
			if (strcmp(RightName, "APPOINT_SGM") == 0) Right = -2;
			if (strcmp(RightName, "APPOINT_JGM") == 0) Right = -2;
			if (strcmp(RightName, "APPOINT_SENATOR") == 0) Right = -2;
			if (strcmp(RightName, "SET_ACCOUNT_RIGHTS") == 0) Right = -2;
			if (strcmp(RightName, "SET_CHARACTER_RIGHTS") == 0) Right = -2;
			if (strcmp(RightName, "SET_ACCOUNTGROUP_RIGHTS") == 0) Right = -2;
			if (strcmp(RightName, "SET_CHARACTERGROUP_RIGHTS") == 0) Right = -2;
			if (strcmp(RightName, "NAME_INSULTING") == 0) Right = NAME_INSULTING;
			if (strcmp(RightName, "NAME_SENTENCE") == 0) Right = NAME_SENTENCE;
			if (strcmp(RightName, "NAME_NONSENSICAL_LETTERS") == 0) Right = NAME_NONSENSICAL_LETTERS;
			if (strcmp(RightName, "NAME_BADLY_FORMATTED") == 0) Right = NAME_BADLY_FORMATTED;
			if (strcmp(RightName, "NAME_NO_PERSON") == 0) Right = NAME_NO_PERSON;
			if (strcmp(RightName, "NAME_CELEBRITY") == 0) Right = NAME_CELEBRITY;
			if (strcmp(RightName, "NAME_COUNTRY") == 0) Right = NAME_COUNTRY;
			if (strcmp(RightName, "NAME_FAKE_IDENTITY") == 0) Right = NAME_FAKE_IDENTITY;
			if (strcmp(RightName, "NAME_FAKE_POSITION") == 0) Right = NAME_FAKE_POSITION;
			if (strcmp(RightName, "STATEMENT_INSULTING") == 0) Right = STATEMENT_INSULTING;
			if (strcmp(RightName, "STATEMENT_SPAMMING") == 0) Right = STATEMENT_SPAMMING;
			if (strcmp(RightName, "STATEMENT_ADVERT_OFFTOPIC") == 0) Right = STATEMENT_ADVERT_OFFTOPIC;
			if (strcmp(RightName, "STATEMENT_ADVERT_MONEY") == 0) Right = STATEMENT_ADVERT_MONEY;
			if (strcmp(RightName, "STATEMENT_NON_ENGLISH") == 0) Right = STATEMENT_NON_ENGLISH;
			if (strcmp(RightName, "STATEMENT_CHANNEL_OFFTOPIC") == 0) Right = STATEMENT_CHANNEL_OFFTOPIC;
			if (strcmp(RightName, "STATEMENT_VIOLATION_INCITING") == 0) Right = STATEMENT_VIOLATION_INCITING;
			if (strcmp(RightName, "CHEATING_BUG_ABUSE") == 0) Right = CHEATING_BUG_ABUSE;
			if (strcmp(RightName, "CHEATING_GAME_WEAKNESS") == 0) Right = CHEATING_GAME_WEAKNESS;
			if (strcmp(RightName, "CHEATING_MACRO_USE") == 0) Right = CHEATING_MACRO_USE;
			if (strcmp(RightName, "CHEATING_MODIFIED_CLIENT") == 0) Right = CHEATING_MODIFIED_CLIENT;
			if (strcmp(RightName, "CHEATING_HACKING") == 0) Right = CHEATING_HACKING;
			if (strcmp(RightName, "CHEATING_MULTI_CLIENT") == 0) Right = CHEATING_MULTI_CLIENT;
			if (strcmp(RightName, "CHEATING_ACCOUNT_TRADING") == 0) Right = CHEATING_ACCOUNT_TRADING;
			if (strcmp(RightName, "CHEATING_ACCOUNT_SHARING") == 0) Right = CHEATING_ACCOUNT_SHARING;
			if (strcmp(RightName, "GAMEMASTER_THREATENING") == 0) Right = GAMEMASTER_THREATENING;
			if (strcmp(RightName, "GAMEMASTER_PRETENDING") == 0) Right = GAMEMASTER_PRETENDING;
			if (strcmp(RightName, "GAMEMASTER_INFLUENCE") == 0) Right = GAMEMASTER_INFLUENCE;
			if (strcmp(RightName, "GAMEMASTER_FALSE_REPORTS") == 0) Right = GAMEMASTER_FALSE_REPORTS;
			if (strcmp(RightName, "KILLING_EXCESSIVE_UNJUSTIFIED") == 0) Right = KILLING_EXCESSIVE_UNJUSTIFIED;
			if (strcmp(RightName, "DESTRUCTIVE_BEHAVIOUR") == 0) Right = DESTRUCTIVE_BEHAVIOUR;
			if (strcmp(RightName, "SPOILING_AUCTION") == 0) Right = SPOILING_AUCTION;
			if (strcmp(RightName, "INVALID_PAYMENT") == 0) Right = INVALID_PAYMENT;
			if (strcmp(RightName, "TELEPORT_TO_CHARACTER") == 0) Right = TELEPORT_TO_CHARACTER;
			if (strcmp(RightName, "TELEPORT_TO_MARK") == 0) Right = TELEPORT_TO_MARK;
			if (strcmp(RightName, "TELEPORT_VERTICAL") == 0) Right = TELEPORT_VERTICAL;
			if (strcmp(RightName, "TELEPORT_TO_COORDINATE") == 0) Right = TELEPORT_TO_COORDINATE;
			if (strcmp(RightName, "LEVITATE") == 0) Right = LEVITATE;
			if (strcmp(RightName, "SPECIAL_MOVEUSE") == 0) Right = SPECIAL_MOVEUSE;
			if (strcmp(RightName, "MODIFY_GOSTRENGTH") == 0) Right = MODIFY_GOSTRENGTH;
			if (strcmp(RightName, "SHOW_COORDINATE") == 0) Right = SHOW_COORDINATE;
			if (strcmp(RightName, "RETRIEVE") == 0) Right = RETRIEVE;
			if (strcmp(RightName, "INVULNERABLE") == 0) Right = INVULNERABLE;
			if (strcmp(RightName, "UNLIMITED_MANA") == 0) Right = UNLIMITED_MANA;
			if (strcmp(RightName, "KEEP_INVENTORY") == 0) Right = KEEP_INVENTORY;
			if (strcmp(RightName, "ALL_SPELLS") == 0) Right = ALL_SPELLS;
			if (strcmp(RightName, "UNLIMITED_CAPACITY") == 0) Right = UNLIMITED_CAPACITY;
			if (strcmp(RightName, "ZERO_CAPACITY") == 0) Right = ZERO_CAPACITY;
			if (strcmp(RightName, "ATTACK_EVERYWHERE") == 0) Right = ATTACK_EVERYWHERE;
			if (strcmp(RightName, "NO_ATTACK") == 0) Right = NO_ATTACK;
			if (strcmp(RightName, "NO_RUNES") == 0) Right = NO_RUNES;
			if (strcmp(RightName, "NO_LOGOUT_BLOCK") == 0) Right = NO_LOGOUT_BLOCK;
			if (strcmp(RightName, "GAMEMASTER_OUTFIT") == 0) Right = GAMEMASTER_OUTFIT;
			if (strcmp(RightName, "ILLUMINATE") == 0) Right = ILLUMINATE;
			if (strcmp(RightName, "CHANGE_PROFESSION") == 0) Right = CHANGE_PROFESSION;
			if (strcmp(RightName, "IGNORED_BY_MONSTERS") == 0) Right = IGNORED_BY_MONSTERS;
			if (strcmp(RightName, "SHOW_KEYHOLE_NUMBERS") == 0) Right = SHOW_KEYHOLE_NUMBERS;
			if (strcmp(RightName, "CREATE_OBJECTS") == 0) Right = CREATE_OBJECTS;
			if (strcmp(RightName, "CREATE_MONEY") == 0) Right = CREATE_MONEY;
			if (strcmp(RightName, "CREATE_MONSTERS") == 0) Right = CREATE_MONSTERS;
			if (strcmp(RightName, "CHANGE_SKILLS") == 0) Right = CHANGE_SKILLS;
			if (strcmp(RightName, "CLEANUP_FIELDS") == 0) Right = CLEANUP_FIELDS;
			if (strcmp(RightName, "NO_STATISTICS") == 0) Right = NO_STATISTICS;
			if (strcmp(RightName, "KEEP_CHARACTER") == 0) Right = -2;
			if (strcmp(RightName, "EXTRA_CHARACTER") == 0) Right = -2;
			if (strcmp(RightName, "ENTER_HOUSES") == 0) Right = ENTER_HOUSES;
			if (strcmp(RightName, "OPEN_NAMEDDOORS") == 0) Right = OPEN_NAMEDOORS;
			// -2 marks a right the query manager knows and the game server has no use for.
			if (Right == -2) {
				continue;
			}

			if (Right == -1) {
				error("TQueryManagerConnection::loginGame: Unbekanntes Recht %s.\n", RightName);
			} else {
				Rights[Right / 8] |= static_cast<unsigned char>(1 << (Right % 8));
			}
		}

		PremiumAccountActivated = getFlag();
		return 0;
	}

	if (Result == STATUS_ERROR) {
		int FailureCode = getByte();
		if (in(FailureCode, LOGIN_CHARACTER_UNKNOWN, LOGIN_WRONG_ACCOUNT)) {
			return FailureCode;
		}

		error("TQueryManagerConnection::loginGame: Ung\xfcltiger Fehlercode %d.\n", FailureCode);
	}

	return -1;
}

int TQueryManagerConnection::logoutGame(uint32_t CharacterID, int Level, char *Profession, char *Residence, time_t LastLoginTime, int TutorActivities) {
	prepareQuery(QUERY_LOGOUT_GAME);
	sendQuad(CharacterID);
	sendWord(static_cast<unsigned short>(Level));
	sendString(Profession);
	sendString(Residence);
	sendQuad(static_cast<uint32_t>(LastLoginTime));
	sendWord(static_cast<unsigned short>(TutorActivities));
	if (executeQuery(120, true) != STATUS_OK) {
		return -1;
	}

	return 0;
}

int TQueryManagerConnection::setNotation(uint32_t GamemasterID, char *PlayerName, char *IPAddress, char *Reason, char *Comment, uint32_t &BanishmentID) {
	prepareQuery(QUERY_SET_NOTATION);
	sendQuad(GamemasterID);
	sendString(PlayerName);
	sendString(IPAddress);
	sendString(Reason);
	sendString(Comment);
	int Result = executeQuery(30, true);
	if (Result == STATUS_OK) {
		BanishmentID = getQuad();
		return 0;
	}

	if (Result == STATUS_ERROR) {
		int FailureCode = getByte();
		if (in(FailureCode, 1, 2)) {
			return FailureCode;
		}

		error("TQueryManagerConnection::setNotation: Ung\xfcltiger Fehlercode %d.\n", FailureCode);
	} else {
		error("TQueryManagerConnection::setNotation: Anfrage fehlgeschlagen.\n");
	}

	return -1;
}

int TQueryManagerConnection::setNamelock(uint32_t GamemasterID, char *PlayerName, char *IPAddress, char *Reason, char *Comment) {
	prepareQuery(QUERY_SET_NAMELOCK);
	sendQuad(GamemasterID);
	sendString(PlayerName);
	sendString(IPAddress);
	sendString(Reason);
	sendString(Comment);
	int Result = executeQuery(30, true);
	if (Result == STATUS_OK) {
		return 0;
	}

	if (Result == STATUS_ERROR) {
		int FailureCode = getByte();
		if (in(FailureCode, 1, 4)) {
			return FailureCode;
		}

		error("TQueryManagerConnection::setNamelock: Ung\xfcltiger Fehlercode %d.\n", FailureCode);
	} else {
		error("TQueryManagerConnection::setNamelock: Anfrage fehlgeschlagen.\n");
	}

	return -1;
}

int TQueryManagerConnection::banishAccount(uint32_t GamemasterID, char *PlayerName, char *IPAddress, char *Reason, char *Comment, bool &FinalWarning, int &Days, uint32_t &BanishmentID) {
	prepareQuery(QUERY_BANISH_ACCOUNT);
	sendQuad(GamemasterID);
	sendString(PlayerName);
	sendString(IPAddress);
	sendString(Reason);
	sendString(Comment);
	sendByte(FinalWarning);
	int Result = executeQuery(30, true);
	if (Result == STATUS_OK) {
		BanishmentID = getQuad();
		Days = getByte();
		if (Days == 0xFF) {
			Days = -1;
		}

		FinalWarning = getByte() != 0;
		return 0;
	}

	if (Result == STATUS_ERROR) {
		int FailureCode = getByte();
		if (in(FailureCode, 1, 3)) {
			return FailureCode;
		}

		error("TQueryManagerConnection::banishAccount: Ung\xfcltiger Fehlercode %d.\n", FailureCode);
	} else {
		error("TQueryManagerConnection::banishAccount: Anfrage fehlgeschlagen.\n");
	}

	return -1;
}

int TQueryManagerConnection::reportStatement(uint32_t ReporterID, char *PlayerName, char *Reason, char *Comment, uint32_t BanishmentID, uint32_t StatementID, int NumberOfStatements, uint32_t *StatementIDs, uint32_t *TimeStamps, uint32_t *CharacterIDs, char (*Channels)[30], char (*Texts)[256]) {
	prepareQuery(QUERY_REPORT_STATEMENT);
	sendQuad(ReporterID);
	sendString(PlayerName);
	sendString(Reason);
	sendString(Comment);
	sendQuad(BanishmentID);
	sendQuad(StatementID);

	// Quirk: only the unused entries at the front are left out of the count,
	// while every unused entry is left out of the list below.
	int RealNumberOfStatements = NumberOfStatements;
	for (int i = 0; i < NumberOfStatements && StatementIDs[i] == 0; i++) {
		RealNumberOfStatements--;
	}

	sendWord(static_cast<unsigned short>(RealNumberOfStatements));
	for (int i = 0; i < NumberOfStatements; i++) {
		if (StatementIDs[i] != 0) {
			sendQuad(StatementIDs[i]);
			sendQuad(TimeStamps[i]);
			sendQuad(CharacterIDs[i]);
			sendString(Channels[i]);
			sendString(Texts[i]);
		}
	}

	int Result = executeQuery(180, true);
	if (Result == STATUS_OK) {
		return 0;
	}

	if (Result == STATUS_ERROR) {
		int FailureCode = getByte();
		if (in(FailureCode, 1, 2)) {
			return FailureCode;
		}

		error("TQueryManagerConnection::reportStatement: Ung\xfcltiger Fehlercode %d.\n", FailureCode);
	} else {
		error("TQueryManagerConnection::reportStatement: Anfrage fehlgeschlagen.\n");
	}

	return -1;
}

int TQueryManagerConnection::banishIPAddress(uint32_t GamemasterID, char *PlayerName, char *IPAddress, char *Reason, char *Comment) {
	prepareQuery(QUERY_BANISH_IP_ADDRESS);
	sendQuad(GamemasterID);
	sendString(PlayerName);
	sendString(IPAddress);
	sendString(Reason);
	sendString(Comment);
	int Result = executeQuery(30, true);
	if (Result == STATUS_OK) {
		return 0;
	}

	if (Result == STATUS_ERROR) {
		int FailureCode = getByte();
		if (in(FailureCode, 1, 2)) {
			return FailureCode;
		}

		error("TQueryManagerConnection::banishIPAddress: Ung\xfcltiger Fehlercode %d.\n", FailureCode);
	} else {
		error("TQueryManagerConnection::banishIPAddress: Anfrage fehlgeschlagen.\n");
	}

	return -1;
}

int TQueryManagerConnection::logCharacterDeath(uint32_t CharacterID, int Level, uint32_t Offender, char *Remark, bool Unjustified, time_t Time) {
	prepareQuery(QUERY_LOG_CHARACTER_DEATH);
	sendQuad(CharacterID);
	sendWord(static_cast<unsigned short>(Level));
	sendQuad(Offender);
	sendString(Remark);
	sendByte(Unjustified);
	sendQuad(static_cast<uint32_t>(Time));
	if (executeQuery(90, true) != STATUS_OK) {
		error("TQueryManagerConnection::logCharacterDeath: Anfrage fehlgeschlagen.\n");
		return -1;
	}

	return 0;
}

int TQueryManagerConnection::addBuddy(uint32_t AccountID, uint32_t Buddy) {
	prepareQuery(QUERY_ADD_BUDDY);
	sendQuad(AccountID);
	sendQuad(Buddy);
	if (executeQuery(90, true) != STATUS_OK) {
		error("TQueryManagerConnection::addBuddy: Anfrage fehlgeschlagen.\n");
		return -1;
	}

	return 0;
}

int TQueryManagerConnection::removeBuddy(uint32_t AccountID, uint32_t Buddy) {
	prepareQuery(QUERY_REMOVE_BUDDY);
	sendQuad(AccountID);
	sendQuad(Buddy);
	if (executeQuery(90, true) != STATUS_OK) {
		error("TQueryManagerConnection::removeBuddy: Anfrage fehlgeschlagen.\n");
		return -1;
	}

	return 0;
}

int TQueryManagerConnection::decrementIsOnline(uint32_t CharacterID) {
	prepareQuery(QUERY_DECREMENT_IS_ONLINE);
	sendQuad(CharacterID);
	if (executeQuery(30, true) != STATUS_OK) {
		error("TQueryManagerConnection::decrementIsOnline: Anfrage fehlgeschlagen.\n");
		return -1;
	}

	return 0;
}

int TQueryManagerConnection::finishAuctions(int &NumberOfAuctions, unsigned short *HouseIDs, uint32_t *CharacterIDs, char (*CharacterNames)[30], int *Bids) {
	prepareQuery(QUERY_FINISH_AUCTIONS);
	if (executeQuery(60, true) != STATUS_OK) {
		error("TQueryManagerConnection::finishAuctions: Anfrage fehlgeschlagen.\n");
		return -1;
	}

	int MaxNumberOfAuctions = NumberOfAuctions;
	NumberOfAuctions = getWord();
	if (NumberOfAuctions > MaxNumberOfAuctions) {
		error("TQueryManagerConnection::finishAuctions: zu viele Auktionen (%d>%d).\n", NumberOfAuctions, MaxNumberOfAuctions);
		return -1;
	}

	for (int i = 0; i < NumberOfAuctions; i++) {
		HouseIDs[i] = getWord();
		CharacterIDs[i] = getQuad();
		getString(CharacterNames[i], 30);
		Bids[i] = getQuad();
	}

	return 0;
}

int TQueryManagerConnection::excludeFromAuctions(uint32_t CharacterID, bool Banish) {
	prepareQuery(QUERY_EXCLUDE_FROM_AUCTIONS);
	sendQuad(CharacterID);
	sendFlag(Banish);
	if (executeQuery(30, true) != STATUS_OK) {
		error("TQueryManagerConnection::excludeFromAuctions: Anfrage fehlgeschlagen.\n");
		return -1;
	}

	return 0;
}

int TQueryManagerConnection::transferHouses(int &NumberOfTransfers, unsigned short *HouseIDs, uint32_t *NewOwnerIDs, char (*NewOwnerNames)[30], int *Prices) {
	prepareQuery(QUERY_TRANSFER_HOUSES);
	if (executeQuery(60, true) != STATUS_OK) {
		error("TQueryManagerConnection::transferHouses: Anfrage fehlgeschlagen.\n");
		return -1;
	}

	int MaxNumberOfTransfers = NumberOfTransfers;
	NumberOfTransfers = getWord();
	if (NumberOfTransfers > MaxNumberOfTransfers) {
		error("TQueryManagerConnection::transferHouses: zu viele Transfers (%d>%d).\n", NumberOfTransfers, MaxNumberOfTransfers);
		return -1;
	}

	for (int i = 0; i < NumberOfTransfers; i++) {
		HouseIDs[i] = getWord();
		NewOwnerIDs[i] = getQuad();
		getString(NewOwnerNames[i], 30);
		Prices[i] = getQuad();
	}

	return 0;
}

int TQueryManagerConnection::cancelHouseTransfer(unsigned short HouseID) {
	prepareQuery(QUERY_CANCEL_HOUSE_TRANSFER);
	sendWord(HouseID);
	if (executeQuery(30, true) != STATUS_OK) {
		error("TQueryManagerConnection::cancelHouseTransfer: Anfrage fehlgeschlagen.\n");
		return -1;
	}

	return 0;
}

int TQueryManagerConnection::evictFreeAccounts(int &NumberOfEvictions, unsigned short *HouseIDs, uint32_t *OwnerIDs) {
	prepareQuery(QUERY_EVICT_FREE_ACCOUNTS);
	if (executeQuery(60, true) != STATUS_OK) {
		error("TQueryManagerConnection::evictFreeAccounts: Anfrage fehlgeschlagen.\n");
		return -1;
	}

	int MaxNumberOfEvictions = NumberOfEvictions;
	NumberOfEvictions = getWord();
	if (NumberOfEvictions > MaxNumberOfEvictions) {
		error("TQueryManagerConnection::evictFreeAccounts: zu viele R\xe4umungen (%d>%d).\n", NumberOfEvictions, MaxNumberOfEvictions);
		return -1;
	}

	for (int i = 0; i < NumberOfEvictions; i++) {
		HouseIDs[i] = getWord();
		OwnerIDs[i] = getQuad();
	}

	return 0;
}

int TQueryManagerConnection::evictDeletedCharacters(int &NumberOfEvictions, unsigned short *HouseIDs) {
	prepareQuery(QUERY_EVICT_DELETED_CHARACTERS);
	if (executeQuery(60, true) != STATUS_OK) {
		error("TQueryManagerConnection::evictDeletedCharacters: Anfrage fehlgeschlagen.\n");
		return -1;
	}

	int MaxNumberOfEvictions = NumberOfEvictions;
	NumberOfEvictions = getWord();
	if (NumberOfEvictions > MaxNumberOfEvictions) {
		error("TQueryManagerConnection::evictDeletedCharacters: zu viele R\xe4umungen (%d>%d).\n", NumberOfEvictions, MaxNumberOfEvictions);
		return -1;
	}

	for (int i = 0; i < NumberOfEvictions; i++) {
		HouseIDs[i] = getWord();
	}

	return 0;
}

int TQueryManagerConnection::evictExGuildleaders(int NumberOfGuildhouses, int &NumberOfEvictions, unsigned short *HouseIDs, uint32_t *Guildleaders) {
	prepareQuery(QUERY_EVICT_EX_GUILDLEADERS);
	sendWord(static_cast<unsigned short>(NumberOfGuildhouses));
	for (int i = 0; i < NumberOfGuildhouses; i++) {
		sendWord(HouseIDs[i]);
		sendQuad(Guildleaders[i]);
	}

	if (executeQuery(60, true) != STATUS_OK) {
		error("TQueryManagerConnection::evictExGuildleaders: Anfrage fehlgeschlagen.\n");
		return -1;
	}

	NumberOfEvictions = getWord();
	for (int i = 0; i < NumberOfEvictions; i++) {
		HouseIDs[i] = getWord();
	}

	return 0;
}

int TQueryManagerConnection::insertHouseOwner(unsigned short HouseID, uint32_t OwnerID, int PaidUntil) {
	prepareQuery(QUERY_INSERT_HOUSE_OWNER);
	sendWord(HouseID);
	sendQuad(OwnerID);
	sendQuad(PaidUntil);
	if (executeQuery(30, true) != STATUS_OK) {
		error("TQueryManagerConnection::insertHouseOwner: Anfrage fehlgeschlagen.\n");
		return -1;
	}

	return 0;
}

int TQueryManagerConnection::updateHouseOwner(unsigned short HouseID, uint32_t OwnerID, int PaidUntil) {
	prepareQuery(QUERY_UPDATE_HOUSE_OWNER);
	sendWord(HouseID);
	sendQuad(OwnerID);
	sendQuad(PaidUntil);
	if (executeQuery(30, true) != STATUS_OK) {
		error("TQueryManagerConnection::updateHouseOwner: Anfrage fehlgeschlagen.\n");
		return -1;
	}

	return 0;
}

int TQueryManagerConnection::deleteHouseOwner(unsigned short HouseID) {
	prepareQuery(QUERY_DELETE_HOUSE_OWNER);
	sendWord(HouseID);
	if (executeQuery(30, true) != STATUS_OK) {
		error("TQueryManagerConnection::deleteHouseOwner: Anfrage fehlgeschlagen.\n");
		return -1;
	}

	return 0;
}

int TQueryManagerConnection::getHouseOwners(int &NumberOfHouses, unsigned short *HouseIDs, uint32_t *OwnerIDs, char (*OwnerNames)[30], int *PaidUntils) {
	prepareQuery(QUERY_GET_HOUSE_OWNERS);
	if (executeQuery(60, true) != STATUS_OK) {
		error("TQueryManagerConnection::getHouseOwners: Anfrage fehlgeschlagen.\n");
		return -1;
	}

	int MaxNumberOfHouses = NumberOfHouses;
	NumberOfHouses = getWord();
	if (NumberOfHouses > MaxNumberOfHouses) {
		error("TQueryManagerConnection::getHouseOwners: zu viele H\xe4user (%d>%d).\n", NumberOfHouses, MaxNumberOfHouses);
		return -1;
	}

	for (int i = 0; i < NumberOfHouses; i++) {
		HouseIDs[i] = getWord();
		OwnerIDs[i] = getQuad();
		getString(OwnerNames[i], 30);
		PaidUntils[i] = getQuad();
	}

	return 0;
}

int TQueryManagerConnection::getAuctions(int &NumberOfAuctions, unsigned short *HouseIDs) {
	prepareQuery(QUERY_GET_AUCTIONS);
	if (executeQuery(120, true) != STATUS_OK) {
		error("TQueryManagerConnection::getAuctions: Anfrage fehlgeschlagen.\n");
		return -1;
	}

	int MaxNumberOfAuctions = NumberOfAuctions;
	NumberOfAuctions = getWord();
	if (NumberOfAuctions > MaxNumberOfAuctions) {
		error("TQueryManagerConnection::getAuctions: zu viele Auktionen (%d>%d).\n", NumberOfAuctions, MaxNumberOfAuctions);
		return -1;
	}

	for (int i = 0; i < NumberOfAuctions; i++) {
		HouseIDs[i] = getWord();
	}

	return 0;
}

int TQueryManagerConnection::startAuction(unsigned short HouseID) {
	prepareQuery(QUERY_START_AUCTION);
	sendWord(HouseID);
	if (executeQuery(30, true) != STATUS_OK) {
		error("TQueryManagerConnection::startAuction: Anfrage fehlgeschlagen.\n");
		return -1;
	}

	return 0;
}

int TQueryManagerConnection::insertHouses(int NumberOfHouses, unsigned short *HouseIDs, char **Names, int *Rents, char **Descriptions, int *Sizes, int *PositionsX, int *PositionsY, int *PositionsZ, char (*Towns)[30], bool *Guildhouses) {
	prepareQuery(QUERY_INSERT_HOUSES);
	sendWord(static_cast<unsigned short>(NumberOfHouses));
	for (int i = 0; i < NumberOfHouses; i++) {
		sendWord(HouseIDs[i]);
		sendString(Names[i]);
		sendQuad(Rents[i]);
		sendString(Descriptions[i]);
		sendWord(static_cast<unsigned short>(Sizes[i]));
		sendWord(static_cast<unsigned short>(PositionsX[i]));
		sendWord(static_cast<unsigned short>(PositionsY[i]));
		sendByte(static_cast<unsigned char>(PositionsZ[i]));
		sendString(Towns[i]);
		sendByte(Guildhouses[i]);
	}

	if (executeQuery(60, true) != STATUS_OK) {
		error("TQueryManagerConnection::insertHouses: Anfrage fehlgeschlagen.\n");
		return -1;
	}

	return 0;
}

int TQueryManagerConnection::clearIsOnline(int &NumberOfAffectedPlayers) {
	prepareQuery(QUERY_CLEAR_IS_ONLINE);
	if (executeQuery(120, true) != STATUS_OK) {
		error("TQueryManagerConnection::clearIsOnline: Anfrage fehlgeschlagen.\n");
		return -1;
	}

	NumberOfAffectedPlayers = getWord();
	return 0;
}

int TQueryManagerConnection::createPlayerlist(int NumberOfPlayers, char **Names, int *Levels, char (*Professions)[30], bool &NewRecord) {
	prepareQuery(QUERY_CREATE_PLAYERLIST);
	sendWord(NumberOfPlayers >= 0 ? static_cast<unsigned short>(NumberOfPlayers) : 0xFFFF);
	for (int i = 0; i < NumberOfPlayers; i++) {
		sendString(Names[i]);
		sendWord(static_cast<unsigned short>(Levels[i]));
		sendString(Professions[i]);
	}

	if (executeQuery(240, true) != STATUS_OK) {
		error("TQueryManagerConnection::createPlayerlist: Anfrage fehlgeschlagen.\n");
		return -1;
	}

	NewRecord = getByte() != 0;
	return 0;
}

int TQueryManagerConnection::logKilledCreatures(int NumberOfRaces, char **Names, int *KilledPlayers, int *KilledCreatures) {
	prepareQuery(QUERY_LOG_KILLED_CREATURES);
	sendWord(static_cast<unsigned short>(NumberOfRaces));
	for (int i = 0; i < NumberOfRaces; i++) {
		sendString(Names[i]);
		sendQuad(KilledPlayers[i]);
		sendQuad(KilledCreatures[i]);
	}

	if (executeQuery(240, true) != STATUS_OK) {
		error("TQueryManagerConnection::logKilledCreatures: Anfrage fehlgeschlagen.\n");
		return -1;
	}

	return 0;
}

int TQueryManagerConnection::loadPlayers(uint32_t MinimumCharacterID, int &NumberOfPlayers, char (*Names)[30], uint32_t *CharacterIDs) {
	prepareQuery(QUERY_LOAD_PLAYERS);
	sendQuad(MinimumCharacterID);
	if (executeQuery(900, true) != STATUS_OK) {
		error("TQueryManagerConnection::loadPlayers: Anfrage fehlgeschlagen.\n");
		return -1;
	}

	NumberOfPlayers = getQuad();
	for (int i = 0; i < NumberOfPlayers; i++) {
		getString(Names[i], 30);
		CharacterIDs[i] = getQuad();
	}

	return 0;
}

TQueryManagerConnectionPool::TQueryManagerConnectionPool(int Connections) : FreeQueryManagerConnections(Connections), QueryManagerConnectionMutex(1) {
	if (Connections <= 0) {
		error("TQueryManagerConnectionPool::TQueryManagerConnectionPool: Ung\xfcltige Verbindungsanzahl %d.\n", Connections);
		Connections = 1;
	}

	NumberOfConnections = Connections;
	QueryManagerConnection = nullptr;
	QueryManagerConnectionFree = nullptr;
}

void TQueryManagerConnectionPool::init() {
	QueryManagerConnection = new TQueryManagerConnection[NumberOfConnections];
	QueryManagerConnectionFree = new bool[NumberOfConnections];
	for (int i = 0; i < NumberOfConnections; i++) {
		if (!QueryManagerConnection[i].isConnected()) {
			error("TQueryManagerConnectionPool::init: Kann nicht zum Query-Manager verbinden.\n");
			throw "cannot connect to query manager";
		}

		QueryManagerConnectionFree[i] = true;
	}
}

void TQueryManagerConnectionPool::exit() {
	for (int i = 0; i < NumberOfConnections; i++) {
		FreeQueryManagerConnections.down();
	}

	delete[] QueryManagerConnection;
	delete[] QueryManagerConnectionFree;
}

TQueryManagerConnection *TQueryManagerConnectionPool::getConnection() {
	FreeQueryManagerConnections.down();
	QueryManagerConnectionMutex.down();
	for (int i = 0; i < NumberOfConnections; i++) {
		if (QueryManagerConnectionFree[i]) {
			QueryManagerConnectionFree[i] = false;
			QueryManagerConnectionMutex.up();
			return &QueryManagerConnection[i];
		}
	}

	error("TQueryManagerConnectionPool::getConnection: Keine freie Verbindung gefunden.\n");
	QueryManagerConnectionMutex.up();
	return nullptr;
}

void TQueryManagerConnectionPool::releaseConnection(TQueryManagerConnection *Connection) {
	for (int i = 0; i < NumberOfConnections; i++) {
		if (&QueryManagerConnection[i] == Connection) {
			QueryManagerConnectionFree[i] = true;
			FreeQueryManagerConnections.up();
			return;
		}
	}

	error("TQueryManagerConnectionPool::releaseConnection: Verbindung nicht gefunden.\n");
}

TQueryManagerPoolConnection::TQueryManagerPoolConnection(TQueryManagerConnectionPool *Pool) {
	if (Pool == nullptr) {
		error("TQueryManagerPoolConnection::TQueryManagerPoolConnection: Pool ist NULL.\n");
		QueryManagerConnection = nullptr;
		return;
	}

	QueryManagerConnectionPool = Pool;
	QueryManagerConnection = Pool->getConnection();
}

TQueryManagerPoolConnection::~TQueryManagerPoolConnection() {
	if (QueryManagerConnection != nullptr) {
		QueryManagerConnectionPool->releaseConnection(QueryManagerConnection);
	}
}
