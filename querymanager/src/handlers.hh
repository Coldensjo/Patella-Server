// Request handlers: one per implemented query type.
//
// The dispatcher takes a request payload, runs the handler inside a storage
// transaction and returns the response payload. It never throws; whatever
// goes wrong becomes STATUS_FAILED, which the game logs and treats as an
// error. A query type that is catalogued but not implemented is answered
// with STATUS_FAILED and logged as "UNIMPLEMENTED <name>".

#ifndef QM_HANDLERS_HH
#define QM_HANDLERS_HH

#include "protocol.hh"
#include "storage.hh"

#include <cstdint>
#include <functional>
#include <mutex>
#include <string>
#include <vector>

namespace qm {

// State of one client connection.
struct Session {
	uint64_t connectionId = 0;
	bool loggedIn = false;
	uint8_t applicationType = 0;
	uint32_t worldId = 0;          // 0: the application is not bound to a world
	std::string worldName;
};

// Returns the current Unix time; replaceable in tests.
using Clock = std::function<uint32_t()>;
uint32_t systemClock();

class Dispatcher {
public:
	explicit Dispatcher(Storage &storage, Clock clock = systemClock);

	// Handles one request and returns the response payload (status byte
	// first). Requests of all connections are serialised here.
	std::vector<uint8_t> handle(Session &session, const std::vector<uint8_t> &request);

private:
	void dispatch(Session &session, uint8_t type, ReadBuffer &in, WriteBuffer &out);
	uint32_t requireWorld(const Session &session) const;

	void handleLogin(Session &session, ReadBuffer &in, WriteBuffer &out);
	void handleLoadWorldConfig(Session &session, ReadBuffer &in, WriteBuffer &out);
	void handleLoadPlayers(Session &session, ReadBuffer &in, WriteBuffer &out);
	void handleLoginGame(Session &session, ReadBuffer &in, WriteBuffer &out);
	void handleLogoutGame(Session &session, ReadBuffer &in, WriteBuffer &out);
	void handleDecrementIsOnline(Session &session, ReadBuffer &in, WriteBuffer &out);
	void handleClearIsOnline(Session &session, ReadBuffer &in, WriteBuffer &out);
	void handleCreatePlayerlist(Session &session, ReadBuffer &in, WriteBuffer &out);
	void handleLogKilledCreatures(Session &session, ReadBuffer &in, WriteBuffer &out);
	void handleLogCharacterDeath(Session &session, ReadBuffer &in, WriteBuffer &out);
	void handleAddBuddy(Session &session, ReadBuffer &in, WriteBuffer &out);
	void handleRemoveBuddy(Session &session, ReadBuffer &in, WriteBuffer &out);
	void handleInsertHouses(Session &session, ReadBuffer &in, WriteBuffer &out);
	void handleGetHouseOwners(Session &session, ReadBuffer &in, WriteBuffer &out);
	void handleInsertHouseOwner(Session &session, ReadBuffer &in, WriteBuffer &out);
	void handleUpdateHouseOwner(Session &session, ReadBuffer &in, WriteBuffer &out);
	void handleDeleteHouseOwner(Session &session, ReadBuffer &in, WriteBuffer &out);
	void handleGetAuctions(Session &session, ReadBuffer &in, WriteBuffer &out);
	void handleStartAuction(Session &session, ReadBuffer &in, WriteBuffer &out);
	void handleFinishAuctions(Session &session, ReadBuffer &in, WriteBuffer &out);
	void handleExcludeFromAuctions(Session &session, ReadBuffer &in, WriteBuffer &out);
	void handleTransferHouses(Session &session, ReadBuffer &in, WriteBuffer &out);
	void handleCancelHouseTransfer(Session &session, ReadBuffer &in, WriteBuffer &out);
	void handleEvictFreeAccounts(Session &session, ReadBuffer &in, WriteBuffer &out);
	void handleEvictDeletedCharacters(Session &session, ReadBuffer &in, WriteBuffer &out);
	void handleEvictExGuildleaders(Session &session, ReadBuffer &in, WriteBuffer &out);

	Storage &Store;
	Clock Now;
	std::mutex Mutex;
	std::string Detail;            // per-request text for the log line
};

} // namespace qm

#endif
