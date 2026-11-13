// querymanager: the account and database service bin/game connects to.

#include "handlers.hh"
#include "log.hh"
#include "server.hh"
#include "sqlite_storage.hh"

#include <cerrno>
#include <csignal>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>

namespace {

qm::Server *ActiveServer = nullptr;

void handleSignal(int) {
	int saved = errno;
	if (ActiveServer != nullptr) {
		ActiveServer->stop();
	}
	errno = saved;
}

void usage(const char *program) {
	std::fprintf(stderr,
		"usage: %s [options]\n"
		"  --db PATH          SQLite database file (default: querymanager.sqlite)\n"
		"  --address ADDRESS  IPv4 address to listen on (default: 127.0.0.1)\n"
		"  --port PORT        TCP port to listen on (default: 7173)\n"
		"  --log FILE         append the log to FILE instead of writing to stderr\n"
		"  --log-level LEVEL  error, warning, info (default) or debug\n"
		"The database is created empty if it does not exist; fill it with\n"
		"querymanager-seed. Stop the server with SIGINT or SIGTERM.\n",
		program);
}

} // namespace

int main(int argc, char **argv) {
	std::string database = "querymanager.sqlite";
	std::string address = "127.0.0.1";
	long port = 7173;

	for (int i = 1; i < argc; i++) {
		std::string option = argv[i];
		const char *value = i + 1 < argc ? argv[i + 1] : nullptr;
		if (option == "--help" || option == "-h") {
			usage(argv[0]);
			return 0;
		}
		if (value == nullptr) {
			usage(argv[0]);
			return 2;
		}
		if (option == "--db") {
			database = value;
		} else if (option == "--address") {
			address = value;
		} else if (option == "--port") {
			char *end = nullptr;
			port = std::strtol(value, &end, 10);
			if (*end != '\0' || port < 0 || port > 65535) {
				std::fprintf(stderr, "invalid port: %s\n", value);
				return 2;
			}
		} else if (option == "--log") {
			if (!qm::setLogFile(value)) {
				std::fprintf(stderr, "cannot open log file %s\n", value);
				return 2;
			}
		} else if (option == "--log-level") {
			qm::LogLevel level;
			if (!qm::parseLogLevel(value, level)) {
				std::fprintf(stderr, "invalid log level: %s\n", value);
				return 2;
			}
			qm::setLogLevel(level);
		} else {
			usage(argv[0]);
			return 2;
		}
		i++;
	}

	try {
		qm::SqliteStorage storage(database);
		if (storage.countRows("applications") == 0 || storage.countRows("worlds") == 0) {
			qm::logMessage(qm::LogLevel::Warning,
				"database %s has no application password or no world; every login will fail"
				" until it is seeded", database.c_str());
		}
		qm::Dispatcher dispatcher(storage);
		qm::Server server(dispatcher, address, static_cast<uint16_t>(port));
		server.start();

		ActiveServer = &server;
		struct sigaction action;
		std::memset(&action, 0, sizeof(action));
		action.sa_handler = handleSignal;
		sigemptyset(&action.sa_mask);
		sigaction(SIGINT, &action, nullptr);
		sigaction(SIGTERM, &action, nullptr);
		signal(SIGPIPE, SIG_IGN);

		qm::logMessage(qm::LogLevel::Info, "query manager ready, database %s", database.c_str());
		server.run();
		ActiveServer = nullptr;
		qm::logMessage(qm::LogLevel::Info, "query manager stopped");
	} catch (const std::exception &e) {
		qm::logMessage(qm::LogLevel::Error, "fatal: %s", e.what());
		qm::closeLog();
		return 1;
	}
	qm::closeLog();
	return 0;
}
