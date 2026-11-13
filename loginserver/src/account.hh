// The login server's connection to the query manager (querymanager/).
//
// It logs in as application type APPLICATION_LOGIN with its own password
// (stored with querymanager-seed --login-password) and asks loginAccount
// for every client login; the request and its answer are described at
// handleLoginAccount in querymanager/src/handlers.cc. One connection is kept
// open and shared; requests on it are serialised. A connection that turns
// out to be closed is opened again and the request repeated once, so the
// query manager may be restarted while the login server runs.

#ifndef LS_ACCOUNT_HH
#define LS_ACCOUNT_HH

#include "login.hh"

#include <cstdint>
#include <mutex>
#include <string>
#include <vector>

namespace ls {

struct AccountAnswer {
	enum class Result {
		Ok,
		UnknownAccount,
		WrongPassword,
		Banished,
		IpBanished,
		Unavailable,       // no connection, login refused or a failed request
	};
	Result result = Result::Unavailable;
	std::vector<Answer::Character> characters;
	uint16_t premiumDays = 0;
	std::string problem;           // for the log when Unavailable
};

class QueryManagerClient {
public:
	QueryManagerClient(const std::string &address, uint16_t port, const std::string &password,
			int timeoutSeconds);
	~QueryManagerClient();

	QueryManagerClient(const QueryManagerClient &) = delete;
	QueryManagerClient &operator=(const QueryManagerClient &) = delete;

	AccountAnswer loginAccount(uint32_t accountId, const std::string &password, const std::string &ipAddress);

private:
	// Each returns false and sets `problem` on failure; the connection is
	// closed then.
	bool open(std::string &problem);
	bool exchange(const std::vector<uint8_t> &request, std::vector<uint8_t> &response, std::string &problem);
	void disconnect();

	std::string Address;
	uint16_t Port;
	std::string Password;
	int TimeoutSeconds;
	std::mutex Mutex;
	int Fd;
};

} // namespace ls

#endif
