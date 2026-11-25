// patella-admin: gives the game server commands through its admin port
// (src/admin.hh has the protocol; config.toml's [admin] opens the port).
//
//	patella-admin [--config FILE] [--port N] [--password-file FILE] [COMMAND [WORD...]]
//
// The port and the password come from the [admin] table of the game's
// config.toml (--config, else config.toml in $HOME, where the game reads it);
// --port and --password-file (the file's first line) take their place, and so
// does the environment variable PATELLA_ADMIN_PASSWORD for the password. It
// connects to 127.0.0.1 only, where the game listens.
//
// With a command it sends that one (the words joined by spaces, a word with a
// space or a tab in double quotes) and prints the reply; the exit code is 0
// for OK, 1 for ERROR, 2 when there is no answer (no connection, a bad
// option). Without one it reads commands from its input, one a line, prints
// each reply, and ends at the end of the input or at "quit" or "exit"; "help"
// lists the game's commands.

#include <arpa/inet.h>
#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iostream>
#include <netinet/in.h>
#include <string>
#include <sys/socket.h>
#include <sys/time.h>
#include <unistd.h>
#include <vector>

#include <toml++/toml.hpp>

// The exit codes.
static const int EXIT_OK = 0;
static const int EXIT_REFUSED = 1;
static const int EXIT_NO_ANSWER = 2;

// How long the tool waits for the game to take the connection and the
// request, in seconds. The answer itself has no limit: a command over every
// character can take a while.
static const int CONNECT_TIMEOUT = 10;

struct TSettings {
	int Port = 7177;
	std::string Password;
	bool Enabled = true;
	std::string ConfigFile;
};

static void Usage() {
	std::fprintf(stderr,
			"usage: patella-admin [--config FILE] [--port N] [--password-file FILE] [COMMAND [WORD...]]\n"
			"Without a command, reads commands from the input; \"help\" lists them.\n");
}

// Reads [admin] of the config file into Settings; false with a message when
// it cannot be read (a missing file is no error when Required is false).
static bool ReadConfig(const std::string &FileName, bool Required, TSettings &Settings) {
	std::ifstream Test(FileName);
	if (!Test.good()) {
		if (Required) {
			std::fprintf(stderr, "patella-admin: cannot read %s\n", FileName.c_str());
			return false;
		}
		return true;
	}
	try {
		toml::table Config = toml::parse_file(FileName);
		if (const toml::table *Admin = Config["admin"].as_table()) {
			if (auto Port = (*Admin)["port"].value<int64_t>()) {
				Settings.Port = static_cast<int>(*Port);
			}
			if (auto Password = (*Admin)["password"].value<std::string>()) {
				Settings.Password = *Password;
			}
			if (auto Enabled = (*Admin)["enabled"].value<bool>()) {
				Settings.Enabled = *Enabled;
			}
		} else {
			Settings.Enabled = false;
		}
	} catch (const toml::parse_error &e) {
		std::fprintf(stderr, "patella-admin: %s: %s\n", FileName.c_str(), std::string(e.description()).c_str());
		return false;
	}
	Settings.ConfigFile = FileName;
	return true;
}

// The command line of the words: joined by spaces, a word with a space or a
// tab (or an empty one) in double quotes. false for a word with a double
// quote or a newline, which a line cannot carry.
static bool JoinWords(const std::vector<std::string> &Words, std::string &Line) {
	Line.clear();
	for (const std::string &Word : Words) {
		if (Word.find('"') != std::string::npos || Word.find('\n') != std::string::npos
				|| Word.find('\r') != std::string::npos) {
			std::fprintf(stderr, "patella-admin: a word may not hold a double quote or a newline: %s\n", Word.c_str());
			return false;
		}
		if (!Line.empty()) {
			Line += ' ';
		}
		if (Word.empty() || Word.find(' ') != std::string::npos || Word.find('\t') != std::string::npos) {
			Line += '"' + Word + '"';
		} else {
			Line += Word;
		}
	}
	return true;
}

// Sends one request and prints the reply (the status line left out, ERROR's
// reply to the error output). Gives the exit code.
static int SendRequest(const TSettings &Settings, const std::string &Line) {
	int Socket = socket(AF_INET, SOCK_STREAM, 0);
	if (Socket < 0) {
		std::fprintf(stderr, "patella-admin: cannot open a socket (%s)\n", std::strerror(errno));
		return EXIT_NO_ANSWER;
	}

	struct timeval Timeout;
	Timeout.tv_sec = CONNECT_TIMEOUT;
	Timeout.tv_usec = 0;
	setsockopt(Socket, SOL_SOCKET, SO_SNDTIMEO, &Timeout, sizeof(Timeout));

	struct sockaddr_in Address;
	std::memset(&Address, 0, sizeof(Address));
	Address.sin_family = AF_INET;
	Address.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
	Address.sin_port = htons(static_cast<unsigned short>(Settings.Port));
	if (connect(Socket, reinterpret_cast<struct sockaddr *>(&Address), sizeof(Address)) < 0) {
		std::fprintf(stderr, "patella-admin: cannot connect to 127.0.0.1:%d (%s); is the game running with [admin] "
				"enabled = true?\n", Settings.Port, std::strerror(errno));
		close(Socket);
		return EXIT_NO_ANSWER;
	}

	std::string Request = Settings.Password + "\n" + Line + "\n";
	size_t Sent = 0;
	while (Sent < Request.size()) {
		ssize_t Count = send(Socket, Request.data() + Sent, Request.size() - Sent, MSG_NOSIGNAL);
		if (Count <= 0) {
			std::fprintf(stderr, "patella-admin: cannot send the request (%s)\n", std::strerror(errno));
			close(Socket);
			return EXIT_NO_ANSWER;
		}
		Sent += static_cast<size_t>(Count);
	}

	std::string Answer;
	char Buffer[4096];
	while (true) {
		ssize_t Count = recv(Socket, Buffer, sizeof(Buffer), 0);
		if (Count < 0 && errno == EINTR) {
			continue;
		}
		if (Count <= 0) {
			break;
		}
		Answer.append(Buffer, static_cast<size_t>(Count));
	}
	close(Socket);

	size_t End = Answer.find('\n');
	if (End == std::string::npos) {
		std::fprintf(stderr, "patella-admin: the game sent no answer\n");
		return EXIT_NO_ANSWER;
	}
	std::string Status = Answer.substr(0, End);
	std::string Text = Answer.substr(End + 1);
	if (Status == "OK") {
		std::fwrite(Text.data(), 1, Text.size(), stdout);
		std::fflush(stdout);
		return EXIT_OK;
	}
	std::fwrite(Text.data(), 1, Text.size(), stderr);
	std::fflush(stderr);
	return Status == "ERROR" ? EXIT_REFUSED : EXIT_NO_ANSWER;
}

int main(int argc, char **argv) {
	TSettings Settings;
	std::string ConfigFile;
	std::string PasswordFile;
	int Port = 0;
	std::vector<std::string> Words;

	for (int i = 1; i < argc; i++) {
		std::string Argument = argv[i];
		if (!Words.empty()) {
			Words.push_back(Argument);
		} else if ((Argument == "--config" || Argument == "--port" || Argument == "--password-file") && i + 1 < argc) {
			std::string Value = argv[++i];
			if (Argument == "--config") {
				ConfigFile = Value;
			} else if (Argument == "--password-file") {
				PasswordFile = Value;
			} else {
				char *End = nullptr;
				long Number = std::strtol(Value.c_str(), &End, 10);
				if (End == Value.c_str() || *End != 0 || Number < 1 || Number > 65535) {
					std::fprintf(stderr, "patella-admin: --port must be 1 to 65535\n");
					return EXIT_NO_ANSWER;
				}
				Port = static_cast<int>(Number);
			}
		} else if (Argument == "--help" || Argument == "-h") {
			Usage();
			return EXIT_OK;
		} else if (Argument.size() > 1 && Argument[0] == '-' && Argument[1] == '-') {
			Usage();
			return EXIT_NO_ANSWER;
		} else {
			Words.push_back(Argument);
		}
	}

	if (!ConfigFile.empty()) {
		if (!ReadConfig(ConfigFile, true, Settings)) {
			return EXIT_NO_ANSWER;
		}
	} else if (const char *Home = std::getenv("HOME")) {
		if (!ReadConfig(std::string(Home) + "/config.toml", false, Settings)) {
			return EXIT_NO_ANSWER;
		}
	}
	if (Port != 0) {
		Settings.Port = Port;
	}
	if (const char *Password = std::getenv("PATELLA_ADMIN_PASSWORD")) {
		Settings.Password = Password;
	}
	if (!PasswordFile.empty()) {
		std::ifstream File(PasswordFile);
		if (!File.good() || !std::getline(File, Settings.Password)) {
			std::fprintf(stderr, "patella-admin: cannot read the password from %s\n", PasswordFile.c_str());
			return EXIT_NO_ANSWER;
		}
		if (!Settings.Password.empty() && Settings.Password.back() == '\r') {
			Settings.Password.pop_back();
		}
	}
	if (Settings.Password.empty()) {
		std::fprintf(stderr, "patella-admin: no password: set [admin] password in config.toml (--config), or give "
				"--password-file or PATELLA_ADMIN_PASSWORD\n");
		return EXIT_NO_ANSWER;
	}
	if (!Settings.Enabled && !Settings.ConfigFile.empty()) {
		std::fprintf(stderr, "patella-admin: note: [admin] enabled is not true in %s\n", Settings.ConfigFile.c_str());
	}

	if (!Words.empty()) {
		std::string Line;
		if (!JoinWords(Words, Line)) {
			return EXIT_NO_ANSWER;
		}
		return SendRequest(Settings, Line);
	}

	bool Interactive = isatty(STDIN_FILENO);
	int Result = EXIT_OK;
	std::string Line;
	while (true) {
		if (Interactive) {
			std::fputs("patella> ", stdout);
			std::fflush(stdout);
		}
		if (!std::getline(std::cin, Line)) {
			break;
		}
		if (!Line.empty() && Line.back() == '\r') {
			Line.pop_back();
		}
		size_t First = Line.find_first_not_of(" \t");
		if (First == std::string::npos) {
			continue;
		}
		if (Line.compare(First, std::string::npos, "quit") == 0 || Line.compare(First, std::string::npos, "exit") == 0) {
			break;
		}
		Result = SendRequest(Settings, Line);
		if (Result == EXIT_NO_ANSWER && !Interactive) {
			return Result;
		}
	}
	if (Interactive) {
		std::fputs("\n", stdout);
	}
	return Interactive ? EXIT_OK : Result;
}
