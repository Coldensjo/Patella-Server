// TCP server of the query manager.
//
// The game server opens 14 connections while it starts and keeps 11 of them
// open: the pool of ten that the login threads share (src/communication.cc)
// and the one of the writer thread (src/writer.cc). The other three (world
// config, player index, houses) are closed when their start-up work is done.
// It sends one request at a time on each connection. Every connection gets a
// thread here; the dispatcher serialises the actual work.

#ifndef QM_SERVER_HH
#define QM_SERVER_HH

#include "handlers.hh"

#include <atomic>
#include <cstdint>
#include <memory>
#include <string>
#include <thread>
#include <vector>

namespace qm {

class Server {
public:
	// `address` is a dotted IPv4 address; port 0 lets the system choose.
	Server(Dispatcher &dispatcher, const std::string &address, uint16_t port);
	~Server();

	Server(const Server &) = delete;
	Server &operator=(const Server &) = delete;

	// Binds and listens. Throws std::runtime_error on failure.
	void start();
	// The port the server listens on (after start).
	uint16_t port() const { return Port; }
	// Accepts and serves connections until stop() is called, then closes
	// all connections and returns.
	void run();
	// Makes run() return. Async-signal-safe and callable from any thread.
	void stop();

private:
	struct Connection {
		int fd = -1;
		uint64_t id = 0;
		std::string peer;
		std::thread thread;
		std::atomic<bool> done{false};
	};

	void serve(Connection *connection);
	bool sendAll(int fd, const std::vector<uint8_t> &data);
	void reap(bool all);

	Dispatcher &Handler;
	std::string Address;
	uint16_t Port;
	int ListenFd;
	int WakePipe[2];
	std::atomic<bool> Stopping;
	uint64_t NextConnectionId;
	std::vector<std::unique_ptr<Connection>> Connections;
};

} // namespace qm

#endif
