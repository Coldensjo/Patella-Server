// account.cc: the login server's connection to the query manager
// (account.hh).

#include "account.hh"

#include "protocol.hh"

#include <cerrno>
#include <cstring>

#include <arpa/inet.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <unistd.h>

namespace ls {

namespace {

std::string systemError(const char *what) {
	return std::string(what) + ": " + std::strerror(errno);
}

} // namespace

QueryManagerClient::QueryManagerClient(const std::string &address, uint16_t port, const std::string &password,
		int timeoutSeconds)
	: Address(address), Port(port), Password(password), TimeoutSeconds(timeoutSeconds), Fd(-1) {
}

QueryManagerClient::~QueryManagerClient() {
	disconnect();
}

void QueryManagerClient::disconnect() {
	if (Fd >= 0) {
		close(Fd);
		Fd = -1;
	}
}

bool QueryManagerClient::open(std::string &problem) {
	struct sockaddr_in addr;
	std::memset(&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_port = htons(Port);
	if (inet_pton(AF_INET, Address.c_str(), &addr.sin_addr) != 1) {
		problem = "query manager address is not an IPv4 address: " + Address;
		return false;
	}
	Fd = socket(AF_INET, SOCK_STREAM | SOCK_CLOEXEC, 0);
	if (Fd < 0) {
		problem = systemError("socket");
		return false;
	}
	struct timeval timeout;
	timeout.tv_sec = TimeoutSeconds;
	timeout.tv_usec = 0;
	setsockopt(Fd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));
	setsockopt(Fd, SOL_SOCKET, SO_SNDTIMEO, &timeout, sizeof(timeout));
	int one = 1;
	setsockopt(Fd, IPPROTO_TCP, TCP_NODELAY, &one, sizeof(one));
	if (connect(Fd, reinterpret_cast<struct sockaddr *>(&addr), sizeof(addr)) != 0) {
		problem = systemError(("connect " + Address + ":" + std::to_string(Port)).c_str());
		disconnect();
		return false;
	}

	// Query 0, login: application type and password; no login data for
	// any type but the game's.
	qm::WriteBuffer request;
	request.writeByte(qm::QUERY_LOGIN);
	request.writeByte(qm::APPLICATION_LOGIN);
	request.writeString(Password);
	std::vector<uint8_t> response;
	if (!exchange(request.data(), response, problem)) {
		return false;
	}
	if (response.size() != 1 || response[0] != qm::STATUS_OK) {
		problem = "query manager refused the login (check --qm-password and querymanager-seed --login-password)";
		disconnect();
		return false;
	}
	return true;
}

bool QueryManagerClient::exchange(const std::vector<uint8_t> &request, std::vector<uint8_t> &response,
		std::string &problem) {
	std::vector<uint8_t> frame = qm::encodeFrame(request);
	size_t sent = 0;
	while (sent < frame.size()) {
		ssize_t n = send(Fd, frame.data() + sent, frame.size() - sent, MSG_NOSIGNAL);
		if (n < 0 && errno == EINTR) {
			continue;
		}
		if (n <= 0) {
			problem = systemError("send to query manager");
			disconnect();
			return false;
		}
		sent += static_cast<size_t>(n);
	}

	std::vector<uint8_t> buffer;
	uint8_t chunk[4096];
	while (true) {
		size_t consumed = 0;
		qm::FrameState state = qm::decodeFrame(buffer.data(), buffer.size(), 1u << 20, response, consumed);
		if (state == qm::FrameState::Complete) {
			return true;
		}
		if (state == qm::FrameState::Invalid) {
			problem = "invalid frame from query manager";
			disconnect();
			return false;
		}
		ssize_t n = recv(Fd, chunk, sizeof(chunk), 0);
		if (n < 0 && errno == EINTR) {
			continue;
		}
		if (n == 0) {
			problem = "query manager closed the connection";
			disconnect();
			return false;
		}
		if (n < 0) {
			problem = errno == EAGAIN || errno == EWOULDBLOCK ? "query manager did not answer in time"
				: systemError("recv from query manager");
			disconnect();
			return false;
		}
		buffer.insert(buffer.end(), chunk, chunk + n);
	}
}

AccountAnswer QueryManagerClient::loginAccount(uint32_t accountId, const std::string &password,
		const std::string &ipAddress) {
	AccountAnswer answer;
	qm::WriteBuffer request;
	request.writeByte(qm::QUERY_LOGIN_ACCOUNT);
	request.writeQuad(accountId);
	request.writeString(password);
	request.writeString(ipAddress);

	std::lock_guard<std::mutex> lock(Mutex);
	std::vector<uint8_t> response;
	bool done = false;
	for (int attempt = 0; attempt < 2 && !done; attempt++) {
		bool fresh = Fd < 0;
		if (fresh && !open(answer.problem)) {
			return answer;
		}
		done = exchange(request.data(), response, answer.problem);
		if (!done && fresh) {
			return answer;
		}
	}
	if (!done) {
		return answer;
	}

	try {
		qm::ReadBuffer in(response);
		uint8_t status = in.readByte();
		if (status == qm::STATUS_ERROR) {
			switch (in.readByte()) {
			case qm::ACCOUNT_UNKNOWN: answer.result = AccountAnswer::Result::UnknownAccount; break;
			case qm::ACCOUNT_WRONG_PASSWORD: answer.result = AccountAnswer::Result::WrongPassword; break;
			case qm::ACCOUNT_BANISHED: answer.result = AccountAnswer::Result::Banished; break;
			case qm::ACCOUNT_IP_BANISHED: answer.result = AccountAnswer::Result::IpBanished; break;
			default: answer.problem = "unknown error code from query manager"; break;
			}
			return answer;
		}
		if (status != qm::STATUS_OK) {
			answer.problem = "query manager answered status " + std::to_string(status);
			return answer;
		}
		uint8_t count = in.readByte();
		for (uint8_t i = 0; i < count; i++) {
			Answer::Character character;
			character.name = in.readString();
			character.world = in.readString();
			in.readBytes(character.address, 4);
			character.port = in.readWord();
			answer.characters.push_back(character);
		}
		answer.premiumDays = in.readWord();
		answer.result = AccountAnswer::Result::Ok;
	} catch (const qm::ProtocolError &e) {
		answer.characters.clear();
		answer.problem = std::string("malformed answer from query manager: ") + e.what();
	}
	return answer;
}

} // namespace ls
