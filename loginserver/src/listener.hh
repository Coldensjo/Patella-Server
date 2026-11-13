// TCP side of the login server: accepts clients, one thread per
// connection. A connection carries one login: the client sends its login
// packet, the server answers with one frame and closes.

#ifndef LS_LISTENER_HH
#define LS_LISTENER_HH

#include "account.hh"
#include "login.hh"

#include <atomic>
#include <cstdint>
#include <memory>
#include <string>
#include <thread>
#include <vector>

namespace ls {

// A client that connects must send the length of its login packet within
// this time, so that idle connections do not hold the slots for long.
const int HEADER_TIMEOUT_MS = 3000;
// After the answer the server waits this long for the client to close
// first, so that the answer is not cut off by a reset.
const int DRAIN_TIMEOUT_MS = 300;

struct ListenerOptions {
	std::string address = "0.0.0.0";
	uint16_t port = 7171;
	// Time a client has to deliver its login packet, in seconds. The first
	// two bytes must come within HEADER_TIMEOUT_MS of that already.
	int timeoutSeconds = 10;
	// Connections beyond these, in all and from one address, are closed at
	// once.
	size_t maxConnections = 64;
	size_t maxPerAddress = 4;
	bool motd = false;
	uint32_t motdNumber = 0;
	std::string motdText;          // ISO-8859-1
};

class Listener {
public:
	Listener(const ListenerOptions &options, LoginKey &key, QueryManagerClient &accounts);
	~Listener();

	Listener(const Listener &) = delete;
	Listener &operator=(const Listener &) = delete;

	// Binds and listens. Throws std::runtime_error on failure.
	void start();
	uint16_t port() const { return Options.port; }
	// Serves until stop() is called, then waits for the open connections.
	void run();
	// Makes run() return. Async-signal-safe and callable from any thread.
	void stop();

private:
	struct Connection {
		int fd = -1;
		std::string peer;          // address:port
		std::string address;       // address only, for the query manager
		std::thread thread;
		std::atomic<bool> done{false};
	};

	void serve(Connection *connection);
	// Reads exactly `count` bytes before the connection's deadline.
	bool readExactly(int fd, uint8_t *buffer, size_t count, int64_t deadline, const char *&reason);
	// Builds the answer to a parsed login and returns the text for the log.
	std::string answerLogin(const LoginRequest &request, const std::string &address, Answer &answer);
	void reap(bool all);

	ListenerOptions Options;
	LoginKey &Key;
	QueryManagerClient &Accounts;
	int ListenFd;
	int WakePipe[2];
	std::atomic<bool> Stopping;
	std::vector<std::unique_ptr<Connection>> Connections;
};

} // namespace ls

#endif
