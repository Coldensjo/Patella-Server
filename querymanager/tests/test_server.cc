// The server over real sockets: framing on the wire, several connections,
// shutdown. All data is made up.

#include "handlers.hh"
#include "log.hh"
#include "server.hh"
#include "sqlite_storage.hh"
#include "test.hh"

#include <atomic>
#include <chrono>
#include <cstring>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <unistd.h>

using namespace qm;

namespace {

class Client {
public:
	explicit Client(uint16_t port) : Fd(socket(AF_INET, SOCK_STREAM, 0)) {
		if (Fd < 0) {
			throw std::runtime_error("socket failed");
		}
		struct timeval timeout;
		timeout.tv_sec = 10;
		timeout.tv_usec = 0;
		setsockopt(Fd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));
		struct sockaddr_in addr;
		std::memset(&addr, 0, sizeof(addr));
		addr.sin_family = AF_INET;
		addr.sin_port = htons(port);
		addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
		if (connect(Fd, reinterpret_cast<struct sockaddr *>(&addr), sizeof(addr)) != 0) {
			close(Fd);
			throw std::runtime_error("connect failed");
		}
	}

	~Client() {
		close(Fd);
	}

	Client(const Client &) = delete;
	Client &operator=(const Client &) = delete;

	void sendRaw(const std::vector<uint8_t> &data) {
		size_t sent = 0;
		while (sent < data.size()) {
			ssize_t n = send(Fd, data.data() + sent, data.size() - sent, MSG_NOSIGNAL);
			if (n <= 0) {
				throw std::runtime_error("send failed");
			}
			sent += static_cast<size_t>(n);
		}
	}

	// Reads one frame; `header` receives the bytes of its length header.
	std::vector<uint8_t> readFrame(std::vector<uint8_t> *header = nullptr) {
		std::vector<uint8_t> payload;
		while (true) {
			size_t consumed = 0;
			FrameState state = decodeFrame(Buffer.data(), Buffer.size(), 64u << 20, payload, consumed);
			if (state == FrameState::Complete) {
				if (header != nullptr) {
					header->assign(Buffer.begin(),
						Buffer.begin() + static_cast<std::ptrdiff_t>(consumed - payload.size()));
				}
				Buffer.erase(Buffer.begin(), Buffer.begin() + static_cast<std::ptrdiff_t>(consumed));
				return payload;
			}
			if (state == FrameState::Invalid) {
				throw std::runtime_error("invalid frame from server");
			}
			uint8_t chunk[65536];
			ssize_t n = recv(Fd, chunk, sizeof(chunk), 0);
			if (n <= 0) {
				throw std::runtime_error("connection closed while reading a frame");
			}
			Buffer.insert(Buffer.end(), chunk, chunk + n);
		}
	}

	std::vector<uint8_t> exchange(const WriteBuffer &request) {
		sendRaw(encodeFrame(request.data()));
		return readFrame();
	}

	bool login(const std::string &password = "secret", const std::string &world = "Testia") {
		WriteBuffer request;
		request.writeByte(QUERY_LOGIN);
		request.writeByte(APPLICATION_GAME);
		request.writeString(password);
		request.writeString(world);
		std::vector<uint8_t> response = exchange(request);
		return response.size() == 1 && response[0] == STATUS_OK;
	}

	// True if the server closed the connection.
	bool closedByPeer() {
		uint8_t byte;
		return recv(Fd, &byte, 1, 0) == 0;
	}

private:
	int Fd;
	std::vector<uint8_t> Buffer;
};

WriteBuffer request(uint8_t type) {
	WriteBuffer buffer;
	buffer.writeByte(type);
	return buffer;
}

// Runs the server in a thread and stops it when the test leaves early.
struct Runner {
	explicit Runner(Server &server) : Target(server), Thread([&server]() { server.run(); }) {}

	~Runner() {
		join();
	}

	void join() {
		Target.stop();
		if (Thread.joinable()) {
			Thread.join();
		}
	}

	Server &Target;
	std::thread Thread;
};

void all() {
	setLogFile("/dev/null");

	SqliteStorage storage(":memory:");
	storage.setApplicationPassword(APPLICATION_GAME, "secret");
	WorldConfig config;
	config.port = 7171;
	config.maxPlayers = 900;
	uint32_t worldId = storage.putWorld("Testia", config, false);
	storage.begin();
	for (uint32_t i = 0; i < 5000; i++) {
		CharacterRecord character;
		character.id = 100 + i;
		character.worldId = worldId;
		character.name = "Made Up Character " + std::to_string(i);
		storage.putCharacter(character);
	}
	storage.commit();

	Dispatcher dispatcher(storage);
	Server server(dispatcher, "127.0.0.1", 0);
	server.start();
	CHECK(server.port() != 0);
	Runner runner(server);

	{
		// A plain exchange, checked byte for byte.
		Client client(server.port());
		const std::vector<uint8_t> loginFrame = {
			0x12, 0x00,                                  // 18 payload bytes
			0x00,                                        // query type 0
			0x01,                                        // application type
			0x06, 0x00, 's', 'e', 'c', 'r', 'e', 't',
			0x06, 0x00, 'T', 'e', 's', 't', 'i', 'a',
		};
		client.sendRaw(loginFrame);
		std::vector<uint8_t> header;
		std::vector<uint8_t> response = client.readFrame(&header);
		const std::vector<uint8_t> shortHeader = {0x01, 0x00};
		CHECK(header == shortHeader);
		CHECK(response.size() == 1 && response[0] == STATUS_OK);

		response = client.exchange(request(QUERY_LOAD_WORLD_CONFIG));
		CHECK(response.size() == 17);
		CHECK(response[0] == STATUS_OK);
		CHECK(response[7] == 0x03 && response[8] == 0x1C);

		// A request that arrives one byte at a time.
		std::vector<uint8_t> frame = encodeFrame(request(QUERY_CLEAR_IS_ONLINE).data());
		for (uint8_t byte : frame) {
			client.sendRaw(std::vector<uint8_t>{byte});
			std::this_thread::sleep_for(std::chrono::milliseconds(2));
		}
		const std::vector<uint8_t> cleared = {STATUS_OK, 0x00, 0x00};
		CHECK(client.readFrame() == cleared);

		// Two requests in one write are answered in order.
		std::vector<uint8_t> both = encodeFrame(request(QUERY_GET_AUCTIONS).data());
		std::vector<uint8_t> second = encodeFrame(request(QUERY_LOAD_WORLD_CONFIG).data());
		both.insert(both.end(), second.begin(), second.end());
		client.sendRaw(both);
		CHECK(client.readFrame().size() == 3);
		CHECK(client.readFrame().size() == 17);

		// A response of more than 0xFFFE bytes uses the long length form.
		WriteBuffer players = request(QUERY_LOAD_PLAYERS);
		players.writeQuad(0);
		client.sendRaw(encodeFrame(players.data()));
		response = client.readFrame(&header);
		CHECK(header.size() == 6 && header[0] == 0xFF && header[1] == 0xFF);
		CHECK(response.size() > 0xFFFF);
		ReadBuffer in(response);
		CHECK(in.readByte() == STATUS_OK);
		CHECK(in.readQuad() == 5000);
		CHECK(in.readString() == "Made Up Character 0");
		CHECK(in.readQuad() == 100);

		// So does a request of that size.
		WriteBuffer houses = request(QUERY_INSERT_HOUSES);
		houses.writeWord(700);
		for (uint16_t i = 0; i < 700; i++) {
			houses.writeWord(static_cast<uint16_t>(1000 + i));
			houses.writeString("House " + std::to_string(i));
			houses.writeQuad(1000);
			houses.writeString(std::string(100, 'd'));
			houses.writeWord(50);
			houses.writeWord(32000);
			houses.writeWord(32000);
			houses.writeByte(7);
			houses.writeString("Testtown");
			houses.writeFlag(false);
		}
		CHECK(houses.size() > 0xFFFF);
		response = client.exchange(houses);
		CHECK(response.size() == 1 && response[0] == STATUS_OK);
		CHECK(storage.countRows("houses") == 700);

		// A second connection has its own login state.
		Client stranger(server.port());
		response = stranger.exchange(request(QUERY_LOAD_WORLD_CONFIG));
		CHECK(response.size() == 1 && response[0] == STATUS_FAILED);
		CHECK(!stranger.login("wrong"));
		CHECK(stranger.login());
		response = client.exchange(request(QUERY_LOAD_WORLD_CONFIG));
		CHECK(response.size() == 17);
	}

	{
		// As many connections as the game holds, used at the same time.
		const int Connections = 14;
		const int Requests = 150;
		std::atomic<int> good{0};
		std::vector<std::thread> threads;
		for (int i = 0; i < Connections; i++) {
			threads.emplace_back([&server, &good, i]() {
				try {
					Client client(server.port());
					if (!client.login()) {
						return;
					}
					for (int j = 0; j < Requests; j++) {
						std::vector<uint8_t> response;
						if ((i + j) % 2 == 0) {
							response = client.exchange(request(QUERY_LOAD_WORLD_CONFIG));
							if (response.size() != 17) {
								return;
							}
						} else {
							WriteBuffer start = request(QUERY_START_AUCTION);
							start.writeWord(static_cast<uint16_t>(1000 + (i * Requests + j) % 700));
							response = client.exchange(start);
							if (response.size() != 1) {
								return;
							}
						}
					}
					good++;
				} catch (const std::exception &) {
				}
			});
		}
		for (std::thread &thread : threads) {
			thread.join();
		}
		CHECK(good.load() == Connections);
		// Every house was put up for auction at most once.
		CHECK(storage.countRows("auctions") <= 700);
		CHECK(storage.countRows("auctions") > 0);
	}

	{
		// A frame with length zero is a protocol violation: the server
		// closes the connection.
		Client client(server.port());
		CHECK(client.login());
		client.sendRaw(std::vector<uint8_t>{0x00, 0x00});
		CHECK(client.closedByPeer());
	}

	{
		// Stopping closes open connections and ends run().
		Client client(server.port());
		CHECK(client.login());
		runner.join();
		CHECK(client.closedByPeer());
	}
}

} // namespace

int main() {
	return test::run(all);
}
