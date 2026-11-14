// loginserver: the login server a Tibia 7.72 client connects to first. It
// checks the account number and password through the query manager and
// answers with the character list, each character with the address and
// port of its world's game server. See login.hh for the protocol.

#include "account.hh"
#include "listener.hh"
#include "log.hh"
#include "login.hh"
#include "text.hh"

#include <cerrno>
#include <csignal>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>

namespace {

ls::Listener *ActiveListener = nullptr;

void handleSignal(int) {
	int saved = errno;
	if (ActiveListener != nullptr) {
		ActiveListener->stop();
	}
	errno = saved;
}

void usage(const char *program) {
	std::fprintf(stderr,
		"usage: %s --qm-password PASSWORD [options]\n"
		"  --address ADDRESS      IPv4 address to listen on (default: 0.0.0.0)\n"
		"  --port PORT            TCP port to listen on (default: 7171)\n"
		"  --qm-address ADDRESS   query manager address (default: 127.0.0.1)\n"
		"  --qm-port PORT         query manager port (default: 7173)\n"
		"  --qm-password PASSWORD the login server's application password, as stored\n"
		"                         with querymanager-seed --login-password\n"
		"  --motd TEXT            message of the day, shown once per number\n"
		"  --motd-number N        its number, 1 to 2147483647 (default: derived from TEXT,\n"
		"                         so that a new text is shown again)\n"
		"  --timeout SECONDS      time a client has for its login packet (default: 10;\n"
		"                         the first 2 bytes must come within 3 seconds)\n"
		"  --max-connections N    open client connections in all (default: 64)\n"
		"  --max-per-address N    open client connections from one address (default: 4)\n"
		"  --log FILE             append the log to FILE instead of writing to stderr\n"
		"  --log-level LEVEL      error, warning, info (default) or debug\n"
		"The RSA key is the game server's (src/RSA_PrivateKey.hh, or the key the\n"
		"build generated in generated/RSA_PrivateKey.hh). Stop the server with\n"
		"SIGINT or SIGTERM.\n",
		program);
}

bool parseNumber(const char *text, long minimum, long maximum, long &value) {
	char *end = nullptr;
	errno = 0;
	value = std::strtol(text, &end, 10);
	return end != text && *end == '\0' && errno == 0 && value >= minimum && value <= maximum;
}

// FNV-1a over the text, folded into 1..2^31-1: the client compares the
// number with atoi's result.
uint32_t motdNumberOf(const std::string &text) {
	uint32_t hash = 2166136261u;
	for (unsigned char c : text) {
		hash = (hash ^ c) * 16777619u;
	}
	hash &= 0x7FFFFFFF;
	return hash == 0 ? 1 : hash;
}

} // namespace

int main(int argc, char **argv) {
	ls::ListenerOptions options;
	std::string qmAddress = "127.0.0.1";
	long qmPort = 7173;
	std::string qmPassword;
	bool haveMotdNumber = false;

	for (int i = 1; i < argc; i++) {
		std::string option = argv[i];
		if (option == "--help" || option == "-h") {
			usage(argv[0]);
			return 0;
		}
		if (i + 1 >= argc) {
			usage(argv[0]);
			return 2;
		}
		const char *value = argv[++i];
		long number = 0;
		bool ok = true;
		if (option == "--address") {
			options.address = value;
		} else if (option == "--port") {
			ok = parseNumber(value, 0, 65535, number);
			options.port = static_cast<uint16_t>(number);
		} else if (option == "--qm-address") {
			qmAddress = value;
		} else if (option == "--qm-port") {
			ok = parseNumber(value, 1, 65535, qmPort);
		} else if (option == "--qm-password") {
			qmPassword = value;
		} else if (option == "--motd") {
			// The terminal gives UTF-8; the client shows ISO-8859-1.
			options.motd = true;
			options.motdText = qm::utf8ToLatin1(value);
		} else if (option == "--motd-number") {
			ok = parseNumber(value, 1, 0x7FFFFFFF, number);
			options.motdNumber = static_cast<uint32_t>(number);
			haveMotdNumber = true;
		} else if (option == "--max-connections") {
			ok = parseNumber(value, 1, 10000, number);
			options.maxConnections = static_cast<size_t>(number);
		} else if (option == "--max-per-address") {
			ok = parseNumber(value, 1, 10000, number);
			options.maxPerAddress = static_cast<size_t>(number);
		} else if (option == "--timeout") {
			ok = parseNumber(value, 1, 3600, number);
			options.timeoutSeconds = static_cast<int>(number);
		} else if (option == "--log") {
			if (!qm::setLogFile(value)) {
				std::fprintf(stderr, "cannot open log file %s\n", value);
				return 2;
			}
		} else if (option == "--log-level") {
			qm::LogLevel level;
			ok = qm::parseLogLevel(value, level);
			if (ok) {
				qm::setLogLevel(level);
			}
		} else {
			usage(argv[0]);
			return 2;
		}
		if (!ok) {
			std::fprintf(stderr, "invalid value for %s: %s\n", option.c_str(), value);
			return 2;
		}
	}
	if (qmPassword.empty()) {
		std::fprintf(stderr, "--qm-password is required\n");
		usage(argv[0]);
		return 2;
	}
	if (options.motd && options.motdText.size() > ls::MAX_MOTD_LENGTH) {
		std::fprintf(stderr, "--motd has %zu bytes; at most %zu fit next to a character list\n",
			options.motdText.size(), static_cast<size_t>(ls::MAX_MOTD_LENGTH));
		return 2;
	}
	if (options.motd && !haveMotdNumber) {
		options.motdNumber = motdNumberOf(options.motdText);
	}

	try {
		ls::LoginKey key;
		ls::QueryManagerClient accounts(qmAddress, static_cast<uint16_t>(qmPort), qmPassword, 10);
		ls::Listener listener(options, key, accounts);
		listener.start();

		ActiveListener = &listener;
		struct sigaction action;
		std::memset(&action, 0, sizeof(action));
		action.sa_handler = handleSignal;
		sigemptyset(&action.sa_mask);
		sigaction(SIGINT, &action, nullptr);
		sigaction(SIGTERM, &action, nullptr);
		signal(SIGPIPE, SIG_IGN);

		qm::logMessage(qm::LogLevel::Info, "login server ready, query manager %s:%ld%s", qmAddress.c_str(),
			qmPort, options.motd ? (", message of the day " + std::to_string(options.motdNumber)).c_str() : "");
		listener.run();
		ActiveListener = nullptr;
		qm::logMessage(qm::LogLevel::Info, "login server stopped");
	} catch (const std::exception &e) {
		qm::logMessage(qm::LogLevel::Error, "fatal: %s", e.what());
		qm::closeLog();
		return 1;
	}
	qm::closeLog();
	return 0;
}
