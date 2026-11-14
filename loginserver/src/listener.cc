// listener.cc: the login server's TCP side: accepting clients, reading their
// login packet and sending the answer (listener.hh).

#include "listener.hh"

#include "log.hh"

#include <algorithm>
#include <cerrno>
#include <chrono>
#include <cstring>
#include <ctime>
#include <stdexcept>

#include <arpa/inet.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <poll.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <unistd.h>

namespace ls {

namespace {

// The game's texts where it has one for the same case (src/communication.cc
// HandleLogin), else this server's.
const char *const TextVersionTooOld =
	"Your terminal version is too old.\nPlease get a new version at\nhttp://www.tibia.com.";
const char *const TextCorruptData = "Login failed due to corrupt data.";
const char *const TextWrongLogin = "Accountnumber or password is not correct.";
const char *const TextBanished = "Your account is banished.";
const char *const TextIpBanished = "Your IP address is banished.";
const char *const TextNoCharacters =
	"This account does not contain any character yet.\nCreate a new character first.";
const char *const TextUnavailable =
	"The account database is not available.\nPlease try again later.";

void setNonBlocking(int fd) {
	int flags = fcntl(fd, F_GETFL, 0);
	if (flags >= 0) {
		fcntl(fd, F_SETFL, flags | O_NONBLOCK);
	}
}

std::string systemError(const char *what) {
	return std::string(what) + ": " + std::strerror(errno);
}

int64_t nowMilliseconds() {
	return std::chrono::duration_cast<std::chrono::milliseconds>(
		std::chrono::steady_clock::now().time_since_epoch()).count();
}

bool sendAll(int fd, const std::vector<uint8_t> &data) {
	size_t sent = 0;
	while (sent < data.size()) {
		ssize_t n = send(fd, data.data() + sent, data.size() - sent, MSG_NOSIGNAL);
		if (n < 0 && errno == EINTR) {
			continue;
		}
		if (n <= 0) {
			return false;
		}
		sent += static_cast<size_t>(n);
	}
	return true;
}

} // namespace

Listener::Listener(const ListenerOptions &options, LoginKey &key, QueryManagerClient &accounts)
	: Options(options), Key(key), Accounts(accounts), ListenFd(-1), WakePipe{-1, -1}, Stopping(false) {
	if (pipe(WakePipe) != 0) {
		throw std::runtime_error(systemError("pipe"));
	}
	setNonBlocking(WakePipe[0]);
	setNonBlocking(WakePipe[1]);
	fcntl(WakePipe[0], F_SETFD, FD_CLOEXEC);
	fcntl(WakePipe[1], F_SETFD, FD_CLOEXEC);
}

Listener::~Listener() {
	stop();
	reap(true);
	if (ListenFd >= 0) {
		close(ListenFd);
	}
	close(WakePipe[0]);
	close(WakePipe[1]);
}

void Listener::start() {
	struct sockaddr_in addr;
	std::memset(&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_port = htons(Options.port);
	if (inet_pton(AF_INET, Options.address.c_str(), &addr.sin_addr) != 1) {
		throw std::runtime_error("not an IPv4 address: " + Options.address);
	}
	ListenFd = socket(AF_INET, SOCK_STREAM | SOCK_CLOEXEC, 0);
	if (ListenFd < 0) {
		throw std::runtime_error(systemError("socket"));
	}
	int one = 1;
	setsockopt(ListenFd, SOL_SOCKET, SO_REUSEADDR, &one, sizeof(one));
	if (bind(ListenFd, reinterpret_cast<struct sockaddr *>(&addr), sizeof(addr)) != 0) {
		throw std::runtime_error(systemError(("bind " + Options.address + ":"
			+ std::to_string(Options.port)).c_str()));
	}
	if (listen(ListenFd, 64) != 0) {
		throw std::runtime_error(systemError("listen"));
	}
	socklen_t length = sizeof(addr);
	if (getsockname(ListenFd, reinterpret_cast<struct sockaddr *>(&addr), &length) == 0) {
		Options.port = ntohs(addr.sin_port);
	}
	setNonBlocking(ListenFd);
	qm::logMessage(qm::LogLevel::Info, "listening on %s:%u", Options.address.c_str(),
		static_cast<unsigned>(Options.port));
}

void Listener::stop() {
	// Only async-signal-safe calls: this runs in the signal handler.
	Stopping.store(true);
	char byte = 0;
	ssize_t ignored = write(WakePipe[1], &byte, 1);
	(void)ignored;
}

void Listener::run() {
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
			qm::logMessage(qm::LogLevel::Error, "poll failed: %s", std::strerror(errno));
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
					qm::logMessage(qm::LogLevel::Error, "accept failed: %s", std::strerror(errno));
					poll(nullptr, 0, 100);
				}
				break;
			}
			char text[INET_ADDRSTRLEN] = "?";
			inet_ntop(AF_INET, &peer.sin_addr, text, sizeof(text));
			std::string peerName = std::string(text) + ":" + std::to_string(ntohs(peer.sin_port));
			if (Connections.size() >= Options.maxConnections) {
				qm::logMessage(qm::LogLevel::Warning, "connection from %s closed: %zu connections open",
					peerName.c_str(), Connections.size());
				close(fd);
				continue;
			}
			size_t fromAddress = 0;
			for (const std::unique_ptr<Connection> &open : Connections) {
				if (open->address == text) {
					fromAddress++;
				}
			}
			if (fromAddress >= Options.maxPerAddress) {
				qm::logMessage(qm::LogLevel::Warning, "connection from %s closed: %zu connections open from %s",
					peerName.c_str(), fromAddress, text);
				close(fd);
				continue;
			}
			int one = 1;
			setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, &one, sizeof(one));
			struct timeval timeout;
			timeout.tv_sec = Options.timeoutSeconds;
			timeout.tv_usec = 0;
			setsockopt(fd, SOL_SOCKET, SO_SNDTIMEO, &timeout, sizeof(timeout));

			// Out of memory or threads: this connection is dropped, the
			// server goes on. The vector has room before the thread starts,
			// so a started thread always gets its entry.
			try {
				Connections.reserve(Connections.size() + 1);
				std::unique_ptr<Connection> connection(new Connection());
				connection->fd = fd;
				connection->peer = peerName;
				connection->address = text;
				Connection *raw = connection.get();
				connection->thread = std::thread([this, raw]() { serve(raw); });
				Connections.push_back(std::move(connection));
			} catch (const std::exception &e) {
				qm::logMessage(qm::LogLevel::Error, "connection from %s closed: %s", peerName.c_str(), e.what());
				close(fd);
			}
		}
	}

	qm::logMessage(qm::LogLevel::Info, "shutting down, %zu connection(s) open", Connections.size());
	reap(true);
	if (ListenFd >= 0) {
		close(ListenFd);
		ListenFd = -1;
	}
}

void Listener::reap(bool all) {
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

bool Listener::readExactly(int fd, uint8_t *buffer, size_t count, int64_t deadline, const char *&reason) {
	size_t received = 0;
	while (received < count) {
		int64_t left = deadline - nowMilliseconds();
		if (left <= 0) {
			reason = "timed out";
			return false;
		}
		struct pollfd fds[2];
		fds[0].fd = fd;
		fds[0].events = POLLIN;
		fds[0].revents = 0;
		fds[1].fd = WakePipe[0];
		fds[1].events = POLLIN;
		fds[1].revents = 0;
		int rc = poll(fds, 2, static_cast<int>(left));
		if (rc < 0) {
			if (errno == EINTR) {
				continue;
			}
			reason = "poll error";
			return false;
		}
		if (fds[1].revents != 0) {
			reason = "server stopping";
			return false;
		}
		if (fds[0].revents == 0) {
			continue;
		}
		ssize_t n = recv(fd, buffer + received, count - received, 0);
		if (n == 0) {
			reason = "closed by client";
			return false;
		}
		if (n < 0) {
			if (errno == EINTR || errno == EAGAIN || errno == EWOULDBLOCK) {
				continue;
			}
			reason = "read error";
			return false;
		}
		received += static_cast<size_t>(n);
	}
	return true;
}

std::string Listener::answerLogin(const LoginRequest &request, const std::string &address, Answer &answer) {
	if (!isTerminalSupported(request.terminalType, request.terminalVersion)) {
		answer.error(TextVersionTooOld);
		return "terminal " + std::to_string(request.terminalType) + " version "
			+ std::to_string(request.terminalVersion) + " too old";
	}

	AccountAnswer account = Accounts.loginAccount(request.accountId, request.password, address);
	switch (account.result) {
	case AccountAnswer::Result::Ok:
		break;
	case AccountAnswer::Result::UnknownAccount:
		answer.error(TextWrongLogin);
		return "unknown account";
	case AccountAnswer::Result::WrongPassword:
		answer.error(TextWrongLogin);
		return "wrong password";
	case AccountAnswer::Result::Banished:
		answer.error(TextBanished);
		return "account banished";
	case AccountAnswer::Result::IpBanished:
		answer.error(TextIpBanished);
		return "IP address banished";
	case AccountAnswer::Result::Unavailable:
		answer.error(TextUnavailable);
		return "query manager unavailable: " + account.problem;
	}

	if (account.characters.empty()) {
		answer.error(TextNoCharacters);
		return "no characters";
	}
	// The whole answer must fit into one frame of the client's receive
	// buffer. The character list comes first: the message of the day is
	// left out when both do not fit (the client keeps showing the last one
	// it got), and only a list that does not fit even alone is cut.
	std::string notes;
	if (Options.motd) {
		if (Answer::messageOfTheDaySize(Options.motdNumber, Options.motdText)
				+ Answer::characterListSize(account.characters) <= MAX_ANSWER_SIZE) {
			answer.messageOfTheDay(Options.motdNumber, Options.motdText);
		} else {
			notes += ", message of the day left out";
		}
	}
	size_t sent = answer.characterList(account.characters, account.premiumDays,
		MAX_ANSWER_SIZE - answer.data().size());
	if (sent < account.characters.size()) {
		notes += ", list cut to " + std::to_string(sent);
	}
	return "ok, " + std::to_string(account.characters.size()) + " character(s), "
		+ std::to_string(account.premiumDays) + " premium day(s)" + notes;
}

void Listener::serve(Connection *connection) {
	int fd = connection->fd;
	const char *peer = connection->peer.c_str();
	int64_t start = nowMilliseconds();
	int64_t deadline = start + static_cast<int64_t>(Options.timeoutSeconds) * 1000;
	int64_t headerDeadline = std::min<int64_t>(deadline, start + HEADER_TIMEOUT_MS);
	const char *reason = nullptr;
	std::string outcome;
	uint32_t accountId = 0;
	bool answered = false;

	uint8_t header[2];
	std::vector<uint8_t> payload;
	if (readExactly(fd, header, 2, headerDeadline, reason)) {
		size_t size = header[0] | (static_cast<size_t>(header[1]) << 8);
		if (size < LOGIN_PACKET_SIZE || size > MAX_LOGIN_PACKET_SIZE) {
			outcome = "not a login packet (" + std::to_string(size) + " bytes)";
		} else {
			payload.resize(size);
			if (!readExactly(fd, payload.data(), size, deadline, reason)) {
				payload.clear();
			}
		}
	}

	if (!payload.empty()) {
		LoginRequest request;
		ParseResult result = ParseResult::NotLogin;
		try {
			result = parseLogin(payload, Key, request);
		} catch (const std::exception &e) {
			outcome = e.what();
		}
		Answer answer;
		switch (result) {
		case ParseResult::NotLogin:
			if (outcome.empty()) {
				outcome = "not a login packet (type " + std::to_string(payload[0]) + ")";
			}
			break;
		case ParseResult::BadBlock:
			outcome = "RSA block does not decrypt: the client uses another RsaModulus";
			break;
		case ParseResult::Corrupt:
			answer.error(TextCorruptData);
			outcome = "corrupt login data";
			break;
		case ParseResult::Ok:
			accountId = request.accountId;
			outcome = answerLogin(request, connection->address, answer);
			break;
		}
		if (!answer.data().empty()) {
			unsigned int seed = static_cast<unsigned int>(std::time(nullptr))
				^ static_cast<unsigned int>(reinterpret_cast<uintptr_t>(connection));
			try {
				answered = sendAll(fd, encryptFrame(answer.data(), request.key, seed));
			} catch (const std::exception &e) {
				outcome += std::string(" (") + e.what() + ")";
			}
			if (!answered) {
				outcome += " (answer not delivered)";
			}
		}
	}

	if (outcome.empty()) {
		outcome = reason != nullptr ? reason : "no data";
	}
	if (answered) {
		qm::logMessage(qm::LogLevel::Info, "login from %s account %u: %s", peer, accountId, outcome.c_str());
		// Let the client read the answer and close first, so that the
		// answer is not cut off by a reset.
		shutdown(fd, SHUT_WR);
		uint8_t rest[256];
		const char *ignored = nullptr;
		readExactly(fd, rest, sizeof(rest), nowMilliseconds() + DRAIN_TIMEOUT_MS, ignored);
	} else {
		qm::logMessage(qm::LogLevel::Info, "login from %s: %s", peer, outcome.c_str());
	}
	close(fd);
	connection->fd = -1;
	connection->done.store(true);
}

} // namespace ls
