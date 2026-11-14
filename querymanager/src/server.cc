// server.cc: the query manager's TCP server (server.hh).

#include "server.hh"

#include "log.hh"

#include <cerrno>
#include <cstring>
#include <stdexcept>

#include <arpa/inet.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <poll.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <unistd.h>

namespace qm {

namespace {

void setNonBlocking(int fd) {
	int flags = fcntl(fd, F_GETFL, 0);
	if (flags >= 0) {
		fcntl(fd, F_SETFL, flags | O_NONBLOCK);
	}
}

std::string systemError(const char *what) {
	return std::string(what) + ": " + std::strerror(errno);
}

} // namespace

Server::Server(Dispatcher &dispatcher, const std::string &address, uint16_t port)
	: Handler(dispatcher), Address(address), Port(port), ListenFd(-1), WakePipe{-1, -1},
	Stopping(false), NextConnectionId(1) {
	if (pipe(WakePipe) != 0) {
		throw std::runtime_error(systemError("pipe"));
	}
	setNonBlocking(WakePipe[0]);
	setNonBlocking(WakePipe[1]);
	fcntl(WakePipe[0], F_SETFD, FD_CLOEXEC);
	fcntl(WakePipe[1], F_SETFD, FD_CLOEXEC);
}

Server::~Server() {
	stop();
	reap(true);
	if (ListenFd >= 0) {
		close(ListenFd);
	}
	close(WakePipe[0]);
	close(WakePipe[1]);
}

void Server::start() {
	struct sockaddr_in addr;
	std::memset(&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_port = htons(Port);
	if (inet_pton(AF_INET, Address.c_str(), &addr.sin_addr) != 1) {
		throw std::runtime_error("not an IPv4 address: " + Address);
	}

	ListenFd = socket(AF_INET, SOCK_STREAM | SOCK_CLOEXEC, 0);
	if (ListenFd < 0) {
		throw std::runtime_error(systemError("socket"));
	}
	int one = 1;
	setsockopt(ListenFd, SOL_SOCKET, SO_REUSEADDR, &one, sizeof(one));
	if (bind(ListenFd, reinterpret_cast<struct sockaddr *>(&addr), sizeof(addr)) != 0) {
		throw std::runtime_error(systemError(("bind " + Address + ":" + std::to_string(Port)).c_str()));
	}
	if (listen(ListenFd, 64) != 0) {
		throw std::runtime_error(systemError("listen"));
	}
	socklen_t length = sizeof(addr);
	if (getsockname(ListenFd, reinterpret_cast<struct sockaddr *>(&addr), &length) == 0) {
		Port = ntohs(addr.sin_port);
	}
	setNonBlocking(ListenFd);
	logMessage(LogLevel::Info, "listening on %s:%u", Address.c_str(), static_cast<unsigned>(Port));
}

void Server::stop() {
	// Only async-signal-safe calls: this runs in the signal handler.
	Stopping.store(true);
	char byte = 0;
	ssize_t ignored = write(WakePipe[1], &byte, 1);
	(void)ignored;
}

void Server::run() {
	while (!Stopping.load()) {
		struct pollfd fds[2];
		fds[0].fd = ListenFd;
		fds[0].events = POLLIN;
		fds[0].revents = 0;
		fds[1].fd = WakePipe[0];
		fds[1].events = POLLIN;
		fds[1].revents = 0;
		int rc = poll(fds, 2, 1000);
		reap(false);
		if (rc < 0) {
			if (errno == EINTR) {
				continue;
			}
			logMessage(LogLevel::Error, "poll failed: %s", std::strerror(errno));
			break;
		}
		if (fds[1].revents != 0) {
			break;
		}
		if ((fds[0].revents & POLLIN) == 0) {
			continue;
		}
		while (true) {
			struct sockaddr_in peer;
			socklen_t length = sizeof(peer);
			int fd = accept4(ListenFd, reinterpret_cast<struct sockaddr *>(&peer), &length, SOCK_CLOEXEC);
			if (fd < 0) {
				if (errno != EAGAIN && errno != EWOULDBLOCK && errno != EINTR) {
					logMessage(LogLevel::Error, "accept failed: %s", std::strerror(errno));
					// Out of descriptors or memory: the connection stays in the
					// backlog and poll reports it again at once, so wait a
					// moment instead of spinning.
					poll(nullptr, 0, 100);
				}
				break;
			}
			int one = 1;
			setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, &one, sizeof(one));
			struct timeval timeout;
			timeout.tv_sec = 30;
			timeout.tv_usec = 0;
			setsockopt(fd, SOL_SOCKET, SO_SNDTIMEO, &timeout, sizeof(timeout));

			char text[INET_ADDRSTRLEN] = "?";
			inet_ntop(AF_INET, &peer.sin_addr, text, sizeof(text));

			std::unique_ptr<Connection> connection(new Connection());
			connection->fd = fd;
			connection->id = NextConnectionId++;
			connection->peer = std::string(text) + ":" + std::to_string(ntohs(peer.sin_port));
			Connection *raw = connection.get();
			connection->thread = std::thread([this, raw]() { serve(raw); });
			Connections.push_back(std::move(connection));
		}
	}

	logMessage(LogLevel::Info, "shutting down, closing %zu connection(s)", Connections.size());
	reap(true);
	if (ListenFd >= 0) {
		close(ListenFd);
		ListenFd = -1;
	}
}

void Server::reap(bool all) {
	for (size_t i = 0; i < Connections.size();) {
		Connection *connection = Connections[i].get();
		if (all || connection->done.load()) {
			if (connection->thread.joinable()) {
				connection->thread.join();
			}
			Connections.erase(Connections.begin() + static_cast<std::ptrdiff_t>(i));
		} else {
			i++;
		}
	}
}

bool Server::sendAll(int fd, const std::vector<uint8_t> &data) {
	size_t sent = 0;
	while (sent < data.size()) {
		ssize_t n = send(fd, data.data() + sent, data.size() - sent, MSG_NOSIGNAL);
		if (n < 0) {
			if (errno == EINTR) {
				continue;
			}
			return false;
		}
		sent += static_cast<size_t>(n);
	}
	return true;
}

void Server::serve(Connection *connection) {
	Session session;
	session.connectionId = connection->id;
	session.socket = connection->fd;
	unsigned long long id = static_cast<unsigned long long>(connection->id);
	logMessage(LogLevel::Info, "conn %llu: connected from %s", id, connection->peer.c_str());

	std::vector<uint8_t> buffer;
	std::vector<uint8_t> payload;
	uint8_t chunk[16384];
	unsigned long long requests = 0;
	const char *reason = "closed by peer";

	bool running = true;
	while (running) {
		struct pollfd fds[2];
		fds[0].fd = connection->fd;
		fds[0].events = POLLIN;
		fds[0].revents = 0;
		fds[1].fd = WakePipe[0];
		fds[1].events = POLLIN;
		fds[1].revents = 0;
		int rc = poll(fds, 2, -1);
		if (rc < 0) {
			if (errno == EINTR) {
				continue;
			}
			reason = "poll error";
			break;
		}
		if (fds[1].revents != 0) {
			reason = "server stopping";
			break;
		}
		if (fds[0].revents == 0) {
			continue;
		}
		ssize_t n = recv(connection->fd, chunk, sizeof(chunk), 0);
		if (n == 0) {
			break;
		}
		if (n < 0) {
			if (errno == EINTR || errno == EAGAIN || errno == EWOULDBLOCK) {
				continue;
			}
			reason = "read error";
			break;
		}
		buffer.insert(buffer.end(), chunk, chunk + n);

		while (true) {
			size_t consumed = 0;
			FrameState state = decodeFrame(buffer.data(), buffer.size(), MAX_REQUEST_SIZE, payload, consumed);
			if (state == FrameState::Incomplete) {
				break;
			}
			if (state == FrameState::Invalid) {
				logMessage(LogLevel::Error, "conn %llu: invalid frame length, closing", id);
				reason = "invalid frame";
				running = false;
				break;
			}
			buffer.erase(buffer.begin(), buffer.begin() + static_cast<std::ptrdiff_t>(consumed));
			requests++;
			std::vector<uint8_t> response = Handler.handle(session, payload);
			if (!sendAll(connection->fd, encodeFrame(response))) {
				reason = "write error";
				running = false;
				break;
			}
		}
	}

	shutdown(connection->fd, SHUT_RDWR);
	close(connection->fd);
	connection->fd = -1;
	logMessage(LogLevel::Info, "conn %llu: disconnected (%s) after %llu request(s)", id, reason, requests);
	connection->done.store(true);
}

} // namespace qm
