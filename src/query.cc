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

int ApplicationType;	// query.c:85
char LoginData[30];	// query.c:86

// 0x080e0210  query.c:97
void SetQueryManagerLoginData(int Type, char *Data) {
	ApplicationType = Type;
	if (Data != nullptr) {
		strncpy(LoginData, Data, sizeof(LoginData));
	} else {
		LoginData[0] = 0;
	}
}

// 0x080e0310  query.c:109
TQueryManagerConnection::TQueryManagerConnection(int QueryBufferSize) : Buffer(new unsigned char[std::max(QueryBufferSize, 16384)]), ReadBuffer(Buffer, std::max(QueryBufferSize, 16384)), WriteBuffer(Buffer, std::max(QueryBufferSize, 16384)) {
	BufferSize = std::max(QueryBufferSize, 16384);
	Socket = -1;
	QueryOk = false;
	connect();
}

// 0x080e0440  query.c:123
TQueryManagerConnection::~TQueryManagerConnection() {
	disconnect();
	delete[] Buffer;
}

// 0x080e04b0  query.c:132
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

		prepareQuery(0);
		sendByte(static_cast<unsigned char>(ApplicationType));
		sendString(QUERY_MANAGER[Try].Password);
		if (ApplicationType == 1) {
			sendString(LoginData);
		}

		int Result = executeQuery(30, false);
		if (Result == 0) {
			return;
		}

		print(2, "TQueryManagerConnection::connect: Anmeldung fehlgeschlagen (%d).\n", Result);
		disconnect();
	}

	print(2, "TQueryManagerConnection::connect: Kein Query-Manager verf\xfcgbar.\n");
	Socket = -1;
}

// 0x080e0820  query.c:184
void TQueryManagerConnection::disconnect() {
	if (isConnected()) {
		if (close(Socket) < 0) {
			error("TQueryManagerConnection::disconnect: Fehler %d beim Schlie\xdf" "en der Socket.\n", errno);
		}
		Socket = -1;
	}
}

// 0x080e0870  query.c:195
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

// 0x080e0900  query.c:217
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

// 0x080e09e0  query.c:254
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

// 0x080e0a60  query.c:270
void TQueryManagerConnection::sendFlag(bool f) {
	WriteBuffer.writeByte(f ? 1 : 0);
}

// 0x080e0a80  query.c:278
void TQueryManagerConnection::sendByte(unsigned char b) {
	try {
		WriteBuffer.writeByte(b);
	} catch (const char *s) {
		error("TQueryManagerConnection::sendByte: %s.\n", s);
		QueryOk = false;
	}
}

// 0x080e0af0  query.c:291
void TQueryManagerConnection::sendWord(unsigned short w) {
	try {
		WriteBuffer.writeWord(w);
	} catch (const char *s) {
		error("TQueryManagerConnection::sendWord: %s.\n", s);
		QueryOk = false;
	}
}

// 0x080e0b60  query.c:304
void TQueryManagerConnection::sendQuad(uint32_t q) {
	try {
		WriteBuffer.writeQuad(q);
	} catch (const char *s) {
		error("TQueryManagerConnection::sendQuad: %s.\n", s);
		QueryOk = false;
	}
}

// 0x080e0bd0  query.c:316
void TQueryManagerConnection::sendString(char *s) {
	try {
		WriteBuffer.writeString(s);
	} catch (const char *s) {
		error("TQueryManagerConnection::sendString: %s.\n", s);
		QueryOk = false;
	}
}

// 0x080e0c40  query.c:329
void TQueryManagerConnection::sendBytes(unsigned char *Buffer, int Count) {
	try {
		WriteBuffer.writeBytes(Buffer, Count);
	} catch (const char *s) {
		error("TQueryManagerConnection::sendBytes: %s.\n", s);
		QueryOk = false;
	}
}

// 0x080e0cb0  query.c:342
bool TQueryManagerConnection::getFlag() {
	return getByte() != 0;
}

// 0x080e0d10  query.c:350
unsigned char TQueryManagerConnection::getByte() {
	try {
		return ReadBuffer.readByte();
	} catch (const char *s) {
		error("TQueryManagerConnection::getByte: %s.\n", s);
		return 0;
	}
}

// 0x080e0d70  query.c:363
unsigned short TQueryManagerConnection::getWord() {
	try {
		return ReadBuffer.readWord();
	} catch (const char *s) {
		error("TQueryManagerConnection::getWord: %s.\n", s);
		return 0;
	}
}

// 0x080e0dd0  query.c:376
uint32_t TQueryManagerConnection::getQuad() {
	try {
		return ReadBuffer.readQuad();
	} catch (const char *s) {
		error("TQueryManagerConnection::getQuad: %s.\n", s);
		return 0;
	}
}

// 0x080e0e30  query.c:389
void TQueryManagerConnection::getString(char *Buffer, int MaxLength) {
	try {
		ReadBuffer.readString(Buffer, MaxLength);
	} catch (const char *s) {
		error("TQueryManagerConnection::getString: %s.\n", s);
		Buffer[0] = 0;
	}
}

// 0x080e0ea0  query.c:402
void TQueryManagerConnection::getBytes(unsigned char *Buffer, int Count) {
	try {
		ReadBuffer.readBytes(Buffer, Count);
	} catch (const char *s) {
		error("TQueryManagerConnection::getBytes: %s.\n", s);
	}
}

// 0x080e0f10  query.c:416
int TQueryManagerConnection::executeQuery(int Timeout, bool AutoReconnect) {
	int DataSize = WriteBuffer.getDataSize();
	WriteBuffer.rewind();
	if (DataSize < 0xFFFF) {
		sendWord(static_cast<unsigned short>(DataSize - 2));
	} else {
		DataSize += 4;
		if (DataSize > BufferSize) {
			error("TQueryManagerConnection::executeQuery: Puffer zu klein.\n");
			return 3;
		}

		memmove(Buffer + 6, Buffer + 2, DataSize - 6);
		sendWord(0xFFFF);
		sendQuad(DataSize - 6);
	}

	bool SecondChance = true;
	if (!QueryOk) {
		error("TQueryManagerConnection::executeQuery: Fehler beim Zusammenbauen der Anfrage.\n");
		return 3;
	}

again:
	if (!isConnected()) {
		if (!AutoReconnect) {
			return 3;
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
			return 3;
		}
	}

	if (write(Buffer, DataSize) != DataSize) {
		disconnect();
		if (SecondChance) {
			SecondChance = false;
			goto again;
		}

		error("TQueryManagerConnection::executeQuery: Fehler beim Abschicken der Anfrage.\n");
		return 3;
	}

	unsigned char Help[2];
	int Result = read(Help, 2, Timeout);
	if (Result != 2) {
		disconnect();
		if (Result != -2 && SecondChance) {
			SecondChance = false;
			goto again;
		}

		return 3;
	}

	int Size = Help[0] + (Help[1] << 8);
	if (Size == 0xFFFF) {
		unsigned char Help2[4];
		if (read(Help2, 4, Timeout) != 4) {
			disconnect();
			return 3;
		}

		Size = static_cast<int>(static_cast<uint32_t>(Help2[0])
			+ (static_cast<uint32_t>(Help2[1]) << 8)
			+ (static_cast<uint32_t>(Help2[2]) << 16)
			+ (static_cast<uint32_t>(Help2[3]) << 24));
	}

	if (Size == 0 || Size > BufferSize) {
		disconnect();
		error("TQueryManagerConnection::executeQuery: Ung\xfcltige Datengr\xf6\xdf" "e %d.\n", Size);
		return 3;
	}

	Result = read(Buffer, Size, Timeout);
	if (Result != Size) {
		disconnect();
		error("TQueryManagerConnection::executeQuery: Fehler beim Auslesen der Daten.\n");
		return 3;
	}

	ReadBuffer.resize(Result);
	ReadBuffer.rewind();
	Result = getByte();
	if (Result == 3) {
		error("TQueryManagerConnection::executeQuery: Anfrage fehlgeschlagen.\n");
	}

	return Result;
}

// 0x080e1440  query.c:505
int TQueryManagerConnection::checkAccountPassword(uint32_t AccountID, char *Password, char *IPAddress) {
	prepareQuery(10);
	sendQuad(AccountID);
	sendString(Password);
	sendString(IPAddress);
	int Result = executeQuery(30, true);
	if (Result == 0) {
		return 0;
	}

	if (Result == 1) {
		int FailureCode = getByte();
		if (in(FailureCode, 1, 4)) {
			return FailureCode;
		}

		error("TQueryManagerConnection::checkAccountPassword: Ung\xfcltiger Fehlercode %d.\n", FailureCode);
	}

	return -1;
}

// 0x080e1610  query.c:531
int TQueryManagerConnection::loginAdmin(uint32_t AccountID, bool PrivateWorld, int &NumberOfCharacters, char (*Characters)[30], char (*Worlds)[30], unsigned char (*IPAddresses)[4], unsigned short *Ports, unsigned short &PremiumDaysLeft) {
	prepareQuery(12);
	sendQuad(AccountID);
	sendFlag(PrivateWorld);
	int Result = executeQuery(30, true);
	if (Result == 0) {
		NumberOfCharacters = getByte();
		for (int i = 0; i < NumberOfCharacters; i++) {
			getString(Characters[i], 30);
			getString(Worlds[i], 30);
			getBytes(IPAddresses[i], 4);
			Ports[i] = getWord();
		}

		PremiumDaysLeft = getWord();
		return 0;
	}

	if (Result == 1) {
		int FailureCode = getByte();
		if (FailureCode == 1) {
			return FailureCode;
		}

		error("TQueryManagerConnection::loginAdmin: Ung\xfcltiger Fehlercode %d.\n", FailureCode);
	}

	return -1;
}

// 0x080e1950  query.c:561
int TQueryManagerConnection::loadWorldConfig(int &WorldType, int &RebootTime, int *IPAddress, int &Port, int &MaxPlayers, int &PremiumPlayerBuffer, int &MaxNewbies, int &PremiumNewbieBuffer) {
	prepareQuery(53);
	int Result = executeQuery(30, true);
	if (Result != 0) {
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

// 0x080e1c10  query.c:588
int TQueryManagerConnection::loginGame(uint32_t AccountID, char *PlayerName, char *Password, char *IPAddress, bool PrivateWorld, bool PremiumAccountRequired, bool GamemasterRequired, uint32_t &CharacterID, int &Sex, char *Guild, char *Rank, char *Title, int &NumberOfBuddies, uint32_t *BuddyIDs, char (*BuddyNames)[30], unsigned char *Rights, bool &PremiumAccountActivated) {
	prepareQuery(20);
	sendQuad(AccountID);
	sendString(PlayerName);
	sendString(Password);
	sendString(IPAddress);
	sendFlag(PrivateWorld);
	sendFlag(PremiumAccountRequired);
	sendFlag(GamemasterRequired);
	int Result = executeQuery(120, true);
	if (Result == 0) {
		CharacterID = getQuad();
		getString(PlayerName, 30);
		Sex = getByte();
		getString(Guild, 30);
		getString(Rank, 30);
		getString(Title, 30);
		NumberOfBuddies = getByte();
		if (NumberOfBuddies > 100) {
			error("TQueryManagerConnection::loginGame: zu viele Buddys (%d) f\xfcr %s.\n", NumberOfBuddies, PlayerName);
		}

		for (int i = 0; i < std::min(NumberOfBuddies, 100); i++) {
			BuddyIDs[i] = getQuad();
			getString(BuddyNames[i], 30);
		}

		for (int i = 100; i < NumberOfBuddies; i++) {
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

	if (Result == 1) {
		int FailureCode = getByte();
		if (in(FailureCode, 1, 15)) {
			return FailureCode;
		}

		error("TQueryManagerConnection::loginGame: Ung\xfcltiger Fehlercode %d.\n", FailureCode);
	}

	return -1;
}

// 0x080e3010  query.c:760
int TQueryManagerConnection::logoutGame(uint32_t CharacterID, int Level, char *Profession, char *Residence, time_t LastLoginTime, int TutorActivities) {
	prepareQuery(21);
	sendQuad(CharacterID);
	sendWord(static_cast<unsigned short>(Level));
	sendString(Profession);
	sendString(Residence);
	sendQuad(static_cast<uint32_t>(LastLoginTime));
	sendWord(static_cast<unsigned short>(TutorActivities));
	if (executeQuery(120, true) != 0) {
		return -1;
	}

	return 0;
}

// 0x080e3240  query.c:777
int TQueryManagerConnection::setNotation(uint32_t GamemasterID, char *PlayerName, char *IPAddress, char *Reason, char *Comment, uint32_t &BanishmentID) {
	prepareQuery(26);
	sendQuad(GamemasterID);
	sendString(PlayerName);
	sendString(IPAddress);
	sendString(Reason);
	sendString(Comment);
	int Result = executeQuery(30, true);
	if (Result == 0) {
		BanishmentID = getQuad();
		return 0;
	}

	if (Result == 1) {
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

// 0x080e34f0  query.c:806
int TQueryManagerConnection::setNamelock(uint32_t GamemasterID, char *PlayerName, char *IPAddress, char *Reason, char *Comment) {
	prepareQuery(23);
	sendQuad(GamemasterID);
	sendString(PlayerName);
	sendString(IPAddress);
	sendString(Reason);
	sendString(Comment);
	int Result = executeQuery(30, true);
	if (Result == 0) {
		return 0;
	}

	if (Result == 1) {
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

// 0x080e3760  query.c:834
int TQueryManagerConnection::banishAccount(uint32_t GamemasterID, char *PlayerName, char *IPAddress, char *Reason, char *Comment, bool &FinalWarning, int &Days, uint32_t &BanishmentID) {
	prepareQuery(25);
	sendQuad(GamemasterID);
	sendString(PlayerName);
	sendString(IPAddress);
	sendString(Reason);
	sendString(Comment);
	sendByte(FinalWarning);
	int Result = executeQuery(30, true);
	if (Result == 0) {
		BanishmentID = getQuad();
		Days = getByte();
		if (Days == 0xFF) {
			Days = -1;
		}

		FinalWarning = getByte() != 0;
		return 0;
	}

	if (Result == 1) {
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

// 0x080e3b00  query.c:867
int TQueryManagerConnection::reportStatement(uint32_t ReporterID, char *PlayerName, char *Reason, char *Comment, uint32_t BanishmentID, uint32_t StatementID, int NumberOfStatements, uint32_t *StatementIDs, uint32_t *TimeStamps, uint32_t *CharacterIDs, char (*Channels)[30], char (*Texts)[256]) {
	prepareQuery(27);
	sendQuad(ReporterID);
	sendString(PlayerName);
	sendString(Reason);
	sendString(Comment);
	sendQuad(BanishmentID);
	sendQuad(StatementID);

	// Only the unused entries at the front are left out of the count.
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
	if (Result == 0) {
		return 0;
	}

	if (Result == 1) {
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

// 0x080e3fc0  query.c:908
int TQueryManagerConnection::banishIPAddress(uint32_t GamemasterID, char *PlayerName, char *IPAddress, char *Reason, char *Comment) {
	prepareQuery(28);
	sendQuad(GamemasterID);
	sendString(PlayerName);
	sendString(IPAddress);
	sendString(Reason);
	sendString(Comment);
	int Result = executeQuery(30, true);
	if (Result == 0) {
		return 0;
	}

	if (Result == 1) {
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

// 0x080e4230  query.c:936
int TQueryManagerConnection::logCharacterDeath(uint32_t CharacterID, int Level, uint32_t Offender, char *Remark, bool Unjustified, time_t Time) {
	prepareQuery(29);
	sendQuad(CharacterID);
	sendWord(static_cast<unsigned short>(Level));
	sendQuad(Offender);
	sendString(Remark);
	sendByte(Unjustified);
	sendQuad(static_cast<uint32_t>(Time));
	if (executeQuery(90, true) != 0) {
		error("TQueryManagerConnection::logCharacterDeath: Anfrage fehlgeschlagen.\n");
		return -1;
	}

	return 0;
}

// 0x080e4480  query.c:956
int TQueryManagerConnection::addBuddy(uint32_t AccountID, uint32_t Buddy) {
	prepareQuery(30);
	sendQuad(AccountID);
	sendQuad(Buddy);
	if (executeQuery(90, true) != 0) {
		error("TQueryManagerConnection::addBuddy: Anfrage fehlgeschlagen.\n");
		return -1;
	}

	return 0;
}

// 0x080e45b0  query.c:972
int TQueryManagerConnection::removeBuddy(uint32_t AccountID, uint32_t Buddy) {
	prepareQuery(31);
	sendQuad(AccountID);
	sendQuad(Buddy);
	if (executeQuery(90, true) != 0) {
		error("TQueryManagerConnection::removeBuddy: Anfrage fehlgeschlagen.\n");
		return -1;
	}

	return 0;
}

// 0x080e46e0  query.c:988
int TQueryManagerConnection::decrementIsOnline(uint32_t CharacterID) {
	prepareQuery(32);
	sendQuad(CharacterID);
	if (executeQuery(30, true) != 0) {
		error("TQueryManagerConnection::decrementIsOnline: Anfrage fehlgeschlagen.\n");
		return -1;
	}

	return 0;
}

// 0x080e47e0  query.c:1004
int TQueryManagerConnection::finishAuctions(int &NumberOfAuctions, unsigned short *HouseIDs, uint32_t *CharacterIDs, char (*CharacterNames)[30], int *Bids) {
	prepareQuery(33);
	if (executeQuery(60, true) != 0) {
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

// 0x080e4a30  query.c:1030
int TQueryManagerConnection::excludeFromAuctions(uint32_t CharacterID, bool Banish) {
	prepareQuery(51);
	sendQuad(CharacterID);
	sendFlag(Banish);
	if (executeQuery(30, true) != 0) {
		error("TQueryManagerConnection::excludeFromAuctions: Anfrage fehlgeschlagen.\n");
		return -1;
	}

	return 0;
}

// 0x080e4b40  query.c:1047
int TQueryManagerConnection::transferHouses(int &NumberOfTransfers, unsigned short *HouseIDs, uint32_t *NewOwnerIDs, char (*NewOwnerNames)[30], int *Prices) {
	prepareQuery(35);
	if (executeQuery(60, true) != 0) {
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

// 0x080e4d90  query.c:1073
int TQueryManagerConnection::cancelHouseTransfer(unsigned short HouseID) {
	prepareQuery(52);
	sendWord(HouseID);
	if (executeQuery(30, true) != 0) {
		error("TQueryManagerConnection::cancelHouseTransfer: Anfrage fehlgeschlagen.\n");
		return -1;
	}

	return 0;
}

// 0x080e4e90  query.c:1089
int TQueryManagerConnection::evictFreeAccounts(int &NumberOfEvictions, unsigned short *HouseIDs, uint32_t *OwnerIDs) {
	prepareQuery(36);
	if (executeQuery(60, true) != 0) {
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

// 0x080e5030  query.c:1114
int TQueryManagerConnection::evictDeletedCharacters(int &NumberOfEvictions, unsigned short *HouseIDs) {
	prepareQuery(37);
	if (executeQuery(60, true) != 0) {
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

// 0x080e51a0  query.c:1135
int TQueryManagerConnection::evictExGuildleaders(int NumberOfGuildhouses, int &NumberOfEvictions, unsigned short *HouseIDs, uint32_t *Guildleaders) {
	prepareQuery(38);
	sendWord(static_cast<unsigned short>(NumberOfGuildhouses));
	for (int i = 0; i < NumberOfGuildhouses; i++) {
		sendWord(HouseIDs[i]);
		sendQuad(Guildleaders[i]);
	}

	if (executeQuery(60, true) != 0) {
		error("TQueryManagerConnection::evictExGuildleaders: Anfrage fehlgeschlagen.\n");
		return -1;
	}

	NumberOfEvictions = getWord();
	for (int i = 0; i < NumberOfEvictions; i++) {
		HouseIDs[i] = getWord();
	}

	return 0;
}

// 0x080e53e0  query.c:1157
int TQueryManagerConnection::insertHouseOwner(unsigned short HouseID, uint32_t OwnerID, int PaidUntil) {
	prepareQuery(39);
	sendWord(HouseID);
	sendQuad(OwnerID);
	sendQuad(PaidUntil);
	if (executeQuery(30, true) != 0) {
		error("TQueryManagerConnection::insertHouseOwner: Anfrage fehlgeschlagen.\n");
		return -1;
	}

	return 0;
}

// 0x080e5550  query.c:1174
int TQueryManagerConnection::updateHouseOwner(unsigned short HouseID, uint32_t OwnerID, int PaidUntil) {
	prepareQuery(40);
	sendWord(HouseID);
	sendQuad(OwnerID);
	sendQuad(PaidUntil);
	if (executeQuery(30, true) != 0) {
		error("TQueryManagerConnection::updateHouseOwner: Anfrage fehlgeschlagen.\n");
		return -1;
	}

	return 0;
}

// 0x080e56c0  query.c:1191
int TQueryManagerConnection::deleteHouseOwner(unsigned short HouseID) {
	prepareQuery(41);
	sendWord(HouseID);
	if (executeQuery(30, true) != 0) {
		error("TQueryManagerConnection::deleteHouseOwner: Anfrage fehlgeschlagen.\n");
		return -1;
	}

	return 0;
}

// 0x080e57c0  query.c:1206
int TQueryManagerConnection::getHouseOwners(int &NumberOfHouses, unsigned short *HouseIDs, uint32_t *OwnerIDs, char (*OwnerNames)[30], int *PaidUntils) {
	prepareQuery(42);
	if (executeQuery(60, true) != 0) {
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

// 0x080e5a10  query.c:1232
int TQueryManagerConnection::getAuctions(int &NumberOfAuctions, unsigned short *HouseIDs) {
	prepareQuery(43);
	if (executeQuery(120, true) != 0) {
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

// 0x080e5b80  query.c:1254
int TQueryManagerConnection::startAuction(unsigned short HouseID) {
	prepareQuery(44);
	sendWord(HouseID);
	if (executeQuery(30, true) != 0) {
		error("TQueryManagerConnection::startAuction: Anfrage fehlgeschlagen.\n");
		return -1;
	}

	return 0;
}

// 0x080e5c80  query.c:1269
int TQueryManagerConnection::insertHouses(int NumberOfHouses, unsigned short *HouseIDs, char **Names, int *Rents, char **Descriptions, int *Sizes, int *PositionsX, int *PositionsY, int *PositionsZ, char (*Towns)[30], bool *Guildhouses) {
	prepareQuery(45);
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

	if (executeQuery(60, true) != 0) {
		error("TQueryManagerConnection::insertHouses: Anfrage fehlgeschlagen.\n");
		return -1;
	}

	return 0;
}

// 0x080e6080  query.c:1297
int TQueryManagerConnection::clearIsOnline(int &NumberOfAffectedPlayers) {
	prepareQuery(46);
	if (executeQuery(120, true) != 0) {
		error("TQueryManagerConnection::clearIsOnline: Anfrage fehlgeschlagen.\n");
		return -1;
	}

	NumberOfAffectedPlayers = getWord();
	return 0;
}

// 0x080e6170  query.c:1313
int TQueryManagerConnection::createPlayerlist(int NumberOfPlayers, char **Names, int *Levels, char (*Professions)[30], bool &NewRecord) {
	prepareQuery(47);
	sendWord(NumberOfPlayers >= 0 ? static_cast<unsigned short>(NumberOfPlayers) : 0xFFFF);
	for (int i = 0; i < NumberOfPlayers; i++) {
		sendString(Names[i]);
		sendWord(static_cast<unsigned short>(Levels[i]));
		sendString(Professions[i]);
	}

	if (executeQuery(240, true) != 0) {
		error("TQueryManagerConnection::createPlayerlist: Anfrage fehlgeschlagen.\n");
		return -1;
	}

	NewRecord = getByte() != 0;
	return 0;
}

// 0x080e63b0  query.c:1334
int TQueryManagerConnection::logKilledCreatures(int NumberOfRaces, char **Names, int *KilledPlayers, int *KilledCreatures) {
	prepareQuery(48);
	sendWord(static_cast<unsigned short>(NumberOfRaces));
	for (int i = 0; i < NumberOfRaces; i++) {
		sendString(Names[i]);
		sendQuad(KilledPlayers[i]);
		sendQuad(KilledCreatures[i]);
	}

	if (executeQuery(240, true) != 0) {
		error("TQueryManagerConnection::logKilledCreatures: Anfrage fehlgeschlagen.\n");
		return -1;
	}

	return 0;
}

// 0x080e6590  query.c:1355
int TQueryManagerConnection::loadPlayers(uint32_t MinimumCharacterID, int &NumberOfPlayers, char (*Names)[30], uint32_t *CharacterIDs) {
	prepareQuery(50);
	sendQuad(MinimumCharacterID);
	if (executeQuery(900, true) != 0) {
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

// 0x080e6770  query.c:1376
int TQueryManagerConnection::getKeptCharacters(uint32_t MinimumCharacterID, int &NumberOfPlayers, uint32_t *CharacterIDs) {
	prepareQuery(200);
	sendQuad(MinimumCharacterID);
	if (executeQuery(1800, true) != 0) {
		error("TQueryManagerConnection::getKeptCharacters: Anfrage fehlgeschlagen.\n");
		return -1;
	}

	NumberOfPlayers = getQuad();
	for (int i = 0; i < NumberOfPlayers; i++) {
		CharacterIDs[i] = getQuad();
	}

	return 0;
}

// 0x080e6900  query.c:1394
int TQueryManagerConnection::getDeletedCharacters(uint32_t MinimumCharacterID, int &NumberOfPlayers, uint32_t *CharacterIDs) {
	prepareQuery(201);
	sendQuad(MinimumCharacterID);
	if (executeQuery(900, true) != 0) {
		// The original logs this failure under the name getKeptCharacters.
		error("TQueryManagerConnection::getKeptCharacters: Anfrage fehlgeschlagen.\n");
		return -1;
	}

	NumberOfPlayers = getQuad();
	for (int i = 0; i < NumberOfPlayers; i++) {
		CharacterIDs[i] = getQuad();
	}

	return 0;
}

// 0x080e6a90  query.c:1411
int TQueryManagerConnection::deleteOldCharacter(uint32_t CharacterID) {
	prepareQuery(202);
	sendQuad(CharacterID);
	if (executeQuery(30, true) != 0) {
		error("TQueryManagerConnection::deleteOldCharacter: Anfrage fehlgeschlagen.\n");
		return -1;
	}

	return 0;
}

// 0x080e6b90  query.c:1427
int TQueryManagerConnection::getHiddenCharacters(uint32_t MinimumCharacterID, int &NumberOfPlayers, uint32_t *CharacterIDs) {
	prepareQuery(203);
	sendQuad(MinimumCharacterID);
	if (executeQuery(120, true) != 0) {
		error("TQueryManagerConnection::getHiddenCharacters: Anfrage fehlgeschlagen.\n");
		return -1;
	}

	NumberOfPlayers = getQuad();
	for (int i = 0; i < NumberOfPlayers; i++) {
		CharacterIDs[i] = getQuad();
	}

	return 0;
}

// 0x080e6d20  query.c:1444
int TQueryManagerConnection::createHighscores(int NumberOfPlayers, uint32_t *CharacterIDs, int *ExpPoints, int *ExpLevel, int *Fist, int *Club, int *Axe, int *Sword, int *Distance, int *Shielding, int *Magic, int *Fishing) {
	prepareQuery(204);
	sendQuad(NumberOfPlayers);
	for (int i = 0; i < NumberOfPlayers; i++) {
		sendQuad(CharacterIDs[i]);
		sendQuad(ExpPoints[i]);
		sendWord(static_cast<unsigned short>(ExpLevel[i]));
		sendWord(static_cast<unsigned short>(Fist[i]));
		sendWord(static_cast<unsigned short>(Club[i]));
		sendWord(static_cast<unsigned short>(Axe[i]));
		sendWord(static_cast<unsigned short>(Sword[i]));
		sendWord(static_cast<unsigned short>(Distance[i]));
		sendWord(static_cast<unsigned short>(Shielding[i]));
		sendWord(static_cast<unsigned short>(Magic[i]));
		sendWord(static_cast<unsigned short>(Fishing[i]));
	}

	if (executeQuery(120, true) != 0) {
		error("TQueryManagerConnection::createHighscores: Anfrage fehlgeschlagen.\n");
		return -1;
	}

	return 0;
}

// 0x080e7150  query.c:1472
int TQueryManagerConnection::createCensus() {
	prepareQuery(205);
	if (executeQuery(600, true) != 0) {
		error("TQueryManagerConnection::createCensus: Anfrage fehlgeschlagen.\n");
		return -1;
	}

	return 0;
}

// 0x080e7200  query.c:1486
int TQueryManagerConnection::createKillStatistics() {
	prepareQuery(206);
	if (executeQuery(300, true) != 0) {
		error("TQueryManagerConnection::createKillStatistics: Anfrage fehlgeschlagen.\n");
		return -1;
	}

	return 0;
}

// 0x080e72b0  query.c:1500
int TQueryManagerConnection::getPlayersOnline(int &NumberOfWorlds, char (*Names)[30], unsigned short *Players) {
	prepareQuery(207);
	if (executeQuery(120, true) != 0) {
		error("TQueryManagerConnection::getPlayersOnline: Anfrage fehlgeschlagen.\n");
		return -1;
	}

	NumberOfWorlds = getByte();
	for (int i = 0; i < NumberOfWorlds; i++) {
		getString(Names[i], 30);
		Players[i] = getWord();
	}

	return 0;
}

// 0x080e7450  query.c:1519
int TQueryManagerConnection::getWorlds(int &NumberOfWorlds, char (*Names)[30]) {
	prepareQuery(208);
	if (executeQuery(120, true) != 0) {
		error("TQueryManagerConnection::getWorlds: Anfrage fehlgeschlagen.\n");
		return -1;
	}

	NumberOfWorlds = getByte();
	for (int i = 0; i < NumberOfWorlds; i++) {
		getString(Names[i], 30);
	}

	return 0;
}

// 0x080e75b0  query.c:1535
int TQueryManagerConnection::getServerLoad(char *World, int Period, int *Data) {
	prepareQuery(209);
	sendString(World);
	sendByte(static_cast<unsigned char>(Period));
	if (executeQuery(360, true) != 0) {
		error("TQueryManagerConnection::getServerLoad: Anfrage fehlgeschlagen.\n");
		return -1;
	}

	for (int i = 0; i < 600; i++) {
		unsigned short Value = getWord();
		Data[i] = (Value == 0xFFFF ? -1 : Value);
	}

	return 0;
}

// 0x080e7740  query.c:1556
int TQueryManagerConnection::insertPaymentDataOld(uint32_t PurchaseNr, uint32_t ReferenceNr, char *FirstName, char *LastName, char *Company, char *Street, char *Zip, char *City, char *Country, char *State, char *Phone, char *Fax, char *EMail, char *PaymentMethod, uint32_t ProductID, char *Registrant, uint32_t AccountID, uint32_t &PaymentID) {
	prepareQuery(210);
	sendQuad(PurchaseNr);
	sendQuad(ReferenceNr);
	sendString(FirstName);
	sendString(LastName);
	sendString(Company);
	sendString(Street);
	sendString(Zip);
	sendString(City);
	sendString(Country);
	sendString(State);
	sendString(Phone);
	sendString(Fax);
	sendString(EMail);
	sendString(PaymentMethod);
	sendQuad(ProductID);
	sendString(Registrant);
	sendQuad(AccountID);
	int Result = executeQuery(360, true);
	if (Result == 0) {
		PaymentID = getQuad();
		return 0;
	}

	if (Result == 1) {
		int FailureCode = getByte();
		if (FailureCode == 1) {
			return FailureCode;
		}

		error("TQueryManagerConnection::insertPaymentDataOld: Ung\xfcltiger Fehlercode %d.\n", FailureCode);
	} else {
		error("TQueryManagerConnection::insertPaymentDataOld: Anfrage fehlgeschlagen.\n");
	}

	return -1;
}

// 0x080e7d00  query.c:1599
int TQueryManagerConnection::addPaymentOld(uint32_t AccountID, char *Description, uint32_t PaymentID, int Days, int &ActionTaken) {
	prepareQuery(211);
	sendQuad(AccountID);
	sendString(Description);
	sendQuad(PaymentID);
	sendWord(static_cast<unsigned short>(Days));
	int Result = executeQuery(360, true);
	if (Result == 0) {
		ActionTaken = getByte();
		return 0;
	}

	if (Result == 1) {
		int FailureCode = getByte();
		if (in(FailureCode, 1, 2)) {
			return FailureCode;
		}

		error("TQueryManagerConnection::addPaymentOld: Ung\xfcltiger Fehlercode %d.\n", FailureCode);
	} else {
		error("TQueryManagerConnection::addPaymentOld: Anfrage fehlgeschlagen.\n");
	}

	return -1;
}

// 0x080e7f70  query.c:1629
int TQueryManagerConnection::cancelPaymentOld(uint32_t PurchaseNr, uint32_t ReferenceNr, uint32_t AccountID, bool &IllegalUse, char *EMailAddress) {
	prepareQuery(212);
	sendQuad(PurchaseNr);
	sendQuad(ReferenceNr);
	sendQuad(AccountID);
	int Result = executeQuery(360, true);
	if (Result == 0) {
		IllegalUse = getByte() != 0;
		getString(EMailAddress, 50);
		return 0;
	}

	if (Result == 1) {
		int FailureCode = getByte();
		if (in(FailureCode, 1, 2)) {
			return FailureCode;
		}

		// The original passes no argument for the %d and prints what the call of
		// executeQuery left in that stack slot: the timeout.
		error("TQueryManagerConnection::cancelPaymentOld: Ung\xfcltiger Fehlercode %d.\n", 360);
	} else {
		error("TQueryManagerConnection::cancelPaymentOld: Anfrage fehlgeschlagen.\n");
	}

	return -1;
}

// 0x080e81e0  query.c:1658
int TQueryManagerConnection::insertPaymentDataNew(uint32_t PurchaseNr, uint32_t ReferenceNr, char *FirstName, char *LastName, char *Company, char *Street, char *Zip, char *City, char *Country, char *State, char *Phone, char *Fax, char *EMail, char *PaymentMethod, uint32_t ProductID, char *Registrant, char *PaymentKey, uint32_t &PaymentID) {
	prepareQuery(213);
	sendQuad(PurchaseNr);
	sendQuad(ReferenceNr);
	sendString(FirstName);
	sendString(LastName);
	sendString(Company);
	sendString(Street);
	sendString(Zip);
	sendString(City);
	sendString(Country);
	sendString(State);
	sendString(Phone);
	sendString(Fax);
	sendString(EMail);
	sendString(PaymentMethod);
	sendQuad(ProductID);
	sendString(Registrant);
	sendString(PaymentKey);
	int Result = executeQuery(360, true);
	if (Result == 0) {
		PaymentID = getQuad();
		return 0;
	}

	if (Result == 1) {
		int FailureCode = getByte();
		if (FailureCode == 1) {
			return FailureCode;
		}

		error("TQueryManagerConnection::insertPaymentDataNew: Ung\xfcltiger Fehlercode %d.\n", FailureCode);
	} else {
		error("TQueryManagerConnection::insertPaymentDataNew: Anfrage fehlgeschlagen.\n");
	}

	return -1;
}

// 0x080e87a0  query.c:1702
int TQueryManagerConnection::addPaymentNew(char *PaymentKey, uint32_t PaymentID, int &ActionTaken, char *EMailReceiver) {
	prepareQuery(214);
	sendString(PaymentKey);
	sendQuad(PaymentID);
	int Result = executeQuery(360, true);
	if (Result == 0) {
		ActionTaken = getByte();
		if (ActionTaken == 5) {
			getString(EMailReceiver, 100);
		}

		return 0;
	}

	if (Result == 1) {
		int FailureCode = getByte();
		if (in(FailureCode, 1, 3)) {
			return FailureCode;
		}

		error("TQueryManagerConnection::addPaymentNew: Ung\xfcltiger Fehlercode %d.\n", FailureCode);
	} else {
		error("TQueryManagerConnection::addPaymentNew: Anfrage fehlgeschlagen.\n");
	}

	return -1;
}

// 0x080e89e0  query.c:1731
int TQueryManagerConnection::cancelPaymentNew(uint32_t PurchaseNr, uint32_t ReferenceNr, char *PaymentKey, bool &IllegalUse, bool &Present, char *EMailAddress) {
	prepareQuery(215);
	sendQuad(PurchaseNr);
	sendQuad(ReferenceNr);
	sendString(PaymentKey);
	int Result = executeQuery(360, true);
	if (Result == 0) {
		IllegalUse = getFlag();
		Present = getFlag();
		getString(EMailAddress, 50);
		return 0;
	}

	if (Result == 1) {
		int FailureCode = getByte();
		if (FailureCode == 1) {
			return FailureCode;
		}

		// The original passes no argument for the %d and prints what the call of
		// executeQuery left in that stack slot: the timeout.
		error("TQueryManagerConnection::cancelPaymentNew: Ung\xfcltiger Fehlercode %d.\n", 360);
	} else {
		error("TQueryManagerConnection::cancelPaymentNew: Anfrage fehlgeschlagen.\n");
	}

	return -1;
}

// 0x080e8d20  query.c:1761
TQueryManagerConnectionPool::TQueryManagerConnectionPool(int Connections) : FreeQueryManagerConnections(Connections), QueryManagerConnectionMutex(1) {
	if (Connections <= 0) {
		error("TQueryManagerConnectionPool::TQueryManagerConnectionPool: Ung\xfcltige Verbindungsanzahl %d.\n", Connections);
		Connections = 1;
	}

	NumberOfConnections = Connections;
	QueryManagerConnection = nullptr;
	QueryManagerConnectionFree = nullptr;
}

// 0x080e8dc0  query.c:1778
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

// 0x080e8f10  query.c:1796
void TQueryManagerConnectionPool::exit() {
	for (int i = 0; i < NumberOfConnections; i++) {
		FreeQueryManagerConnections.down();
	}

	delete[] QueryManagerConnection;
	delete[] QueryManagerConnectionFree;
}

// 0x080e8f90  query.c:1808
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

// 0x080e9000  query.c:1825
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

// 0x080e90f0  query.c:1843
TQueryManagerPoolConnection::TQueryManagerPoolConnection(TQueryManagerConnectionPool *Pool) {
	if (Pool == nullptr) {
		error("TQueryManagerPoolConnection::TQueryManagerPoolConnection: Pool ist NULL.\n");
		QueryManagerConnection = nullptr;
		return;
	}

	QueryManagerConnectionPool = Pool;
	QueryManagerConnection = Pool->getConnection();
}

// 0x080e9220  query.c:1857
TQueryManagerPoolConnection::~TQueryManagerPoolConnection() {
	if (QueryManagerConnection != nullptr) {
		QueryManagerConnectionPool->releaseConnection(QueryManagerConnection);
	}
}
