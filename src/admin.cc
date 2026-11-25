// admin.cc: the admin port (admin.hh): its thread, which listens on
// 127.0.0.1 at [admin] port, reads one request at a time, checks its password
// and waits for the main loop to answer it, and the hand-over of the request
// between that thread and the main loop (TakeAdminRequest,
// AnswerAdminRequest). The thread never touches the game: it only passes
// the line on and the answer back.

#include <arpa/inet.h>
#include <atomic>
#include <cerrno>
#include <csignal>
#include <cstring>
#include <netinet/in.h>
#include <poll.h>
#include <pthread.h>
#include <string>
#include <sys/socket.h>
#include <sys/time.h>
#include <unistd.h>

#include "admin.hh"
#include "config.hh"
#include "threads.hh"
#include "tomlfile.hh"
#include "utils.hh"
#include "writer.hh"

// How long the thread waits for a connection before it looks whether it is to
// stop, in milliseconds.
static const int ADMIN_POLL_INTERVAL = 500;

// The connections waiting to be accepted.
static const int ADMIN_BACKLOG = 4;

// The listening socket (-1: none) and the thread.
static int AdminSocket = -1;
static Thread AdminThread = 0;
static std::atomic<bool> AdminStopping(false);

// The request between the thread and the main loop, under RequestMutex: a
// line waiting to be taken, or taken and not answered yet; the answer; and
// ReplyReady, which the main loop (or ExitAdmin) raises when the answer is
// there.
static pthread_mutex_t RequestMutex = PTHREAD_MUTEX_INITIALIZER;
static bool RequestWaiting = false;
static bool RequestTaken = false;
static std::string RequestLine;
static bool ReplyOk = false;
static std::string ReplyText;
static Semaphore ReplyReady(0);

bool TakeAdminRequest(std::string &Line) {
	pthread_mutex_lock(&RequestMutex);
	bool Taken = RequestWaiting;
	if (Taken) {
		Line = RequestLine;
		RequestWaiting = false;
		RequestTaken = true;
	}
	pthread_mutex_unlock(&RequestMutex);
	return Taken;
}

void AnswerAdminRequest(bool Ok, const std::string &Reply) {
	pthread_mutex_lock(&RequestMutex);
	bool Taken = RequestTaken;
	if (Taken) {
		ReplyOk = Ok;
		ReplyText = Reply;
		RequestTaken = false;
	}
	pthread_mutex_unlock(&RequestMutex);
	if (Taken) {
		ReplyReady.up();
	}
}

// Hands Line to the main loop and waits for its answer; when the game is
// stopping, answers at once.
static void SubmitRequest(const std::string &Line, bool &Ok, std::string &Reply) {
	pthread_mutex_lock(&RequestMutex);
	if (AdminStopping) {
		pthread_mutex_unlock(&RequestMutex);
		Ok = false;
		Reply = "The game server is shutting down.\n";
		return;
	}
	RequestLine = Line;
	RequestWaiting = true;
	pthread_mutex_unlock(&RequestMutex);

	ReplyReady.down();

	pthread_mutex_lock(&RequestMutex);
	Ok = ReplyOk;
	Reply = ReplyText;
	pthread_mutex_unlock(&RequestMutex);
}

// Reads the next line of the connection into Line (without '\n' and a '\r'
// before it), from what Pending holds and what comes; false at the end of
// the connection, a timeout, an error or a line longer than MAX_ADMIN_LINE.
static bool ReadLine(int Socket, std::string &Pending, std::string &Line) {
	while (true) {
		size_t End = Pending.find('\n');
		if (End != std::string::npos) {
			Line = Pending.substr(0, End);
			Pending.erase(0, End + 1);
			if (!Line.empty() && Line.back() == '\r') {
				Line.pop_back();
			}
			return Line.size() <= static_cast<size_t>(MAX_ADMIN_LINE);
		}
		if (Pending.size() > static_cast<size_t>(MAX_ADMIN_LINE) + 1) {
			return false;
		}
		char Buffer[1024];
		ssize_t Count = recv(Socket, Buffer, sizeof(Buffer), 0);
		if (Count <= 0) {
			return false;
		}
		Pending.append(Buffer, static_cast<size_t>(Count));
	}
}

// Sends all of Text; gives up at an error (the client went away).
static void SendAll(int Socket, const std::string &Text) {
	size_t Sent = 0;
	while (Sent < Text.size()) {
		ssize_t Count = send(Socket, Text.data() + Sent, Text.size() - Sent, MSG_NOSIGNAL);
		if (Count <= 0) {
			return;
		}
		Sent += static_cast<size_t>(Count);
	}
}

// Whether the two strings are equal, in a time that does not depend on where
// they differ.
static bool SameSecret(const std::string &A, const std::string &B) {
	size_t Length = A.size() > B.size() ? A.size() : B.size();
	unsigned char Difference = A.size() == B.size() ? 0 : 1;
	for (size_t i = 0; i < Length; i++) {
		unsigned char a = i < A.size() ? static_cast<unsigned char>(A[i]) : 0;
		unsigned char b = i < B.size() ? static_cast<unsigned char>(B[i]) : 0;
		Difference |= static_cast<unsigned char>(a ^ b);
	}
	return Difference == 0 && !B.empty();
}

// Serves one connection: reads the password and the command line, answers.
static void ServeConnection(int Socket) {
	struct timeval Timeout;
	Timeout.tv_sec = ADMIN_TIMEOUT;
	Timeout.tv_usec = 0;
	setsockopt(Socket, SOL_SOCKET, SO_RCVTIMEO, &Timeout, sizeof(Timeout));
	setsockopt(Socket, SOL_SOCKET, SO_SNDTIMEO, &Timeout, sizeof(Timeout));

	std::string Pending;
	std::string Password;
	std::string Line;
	if (!ReadLine(Socket, Pending, Password) || !ReadLine(Socket, Pending, Line)) {
		SendAll(Socket, "ERROR\nThe request must be the password and the command, a line each.\n");
		return;
	}

	if (!SameSecret(Password, AdminPassword)) {
		Log("admin", "A request with a wrong password was refused.\n");
		DelayThread(1, 0);
		SendAll(Socket, "ERROR\nWrong password.\n");
		return;
	}

	std::string Latin1;
	if (!Utf8ToLatin1(Line, Latin1) || Latin1.find('\0') != std::string::npos) {
		SendAll(Socket, "ERROR\nThe command must be UTF-8 text with Latin-1 characters only.\n");
		return;
	}

	bool Ok = false;
	std::string Reply;
	SubmitRequest(Latin1, Ok, Reply);
	SendAll(Socket, std::string(Ok ? "OK\n" : "ERROR\n") + Latin1ToUtf8(Reply));
}

static int AdminThreadLoop(void *) {
	// The thread takes no signal: the beat's SIGALRM and the signals sent to
	// the server's PID go to the other threads. It stops when AdminStopping
	// is set, which it looks at between its waits.
	sigset_t SignalSet;
	sigfillset(&SignalSet);
	pthread_sigmask(SIG_SETMASK, &SignalSet, nullptr);

	while (!AdminStopping) {
		struct pollfd Poll;
		Poll.fd = AdminSocket;
		Poll.events = POLLIN;
		Poll.revents = 0;
		int Ready = poll(&Poll, 1, ADMIN_POLL_INTERVAL);
		if (Ready <= 0 || AdminStopping) {
			continue;
		}

		int Socket = accept(AdminSocket, nullptr, nullptr);
		if (Socket < 0) {
			if (!AdminStopping && errno != EINTR && errno != EAGAIN) {
				error("AdminThreadLoop: Error %d at accept.\n", errno);
			}
			continue;
		}
		ServeConnection(Socket);
		close(Socket);
	}
	return 0;
}

void InitAdmin() {
	if (!AdminEnabled) {
		return;
	}

	AdminStopping = false;
	AdminSocket = socket(AF_INET, SOCK_STREAM, 0);
	if (AdminSocket < 0) {
		throw "cannot open the admin port's socket";
	}

	int On = 1;
	setsockopt(AdminSocket, SOL_SOCKET, SO_REUSEADDR, &On, sizeof(On));

	struct sockaddr_in Address;
	memset(&Address, 0, sizeof(Address));
	Address.sin_family = AF_INET;
	Address.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
	Address.sin_port = htons(static_cast<unsigned short>(AdminPort));
	if (bind(AdminSocket, reinterpret_cast<struct sockaddr *>(&Address), sizeof(Address)) < 0
			|| listen(AdminSocket, ADMIN_BACKLOG) < 0) {
		error("InitAdmin: Error %d while opening the admin port %d.\n", errno, AdminPort);
		close(AdminSocket);
		AdminSocket = -1;
		throw "cannot open the admin port";
	}

	AdminThread = StartThread(AdminThreadLoop, nullptr, false);
	if (AdminThread == 0) {
		close(AdminSocket);
		AdminSocket = -1;
		throw "cannot start the admin thread";
	}
	print(1, "Admin port open on 127.0.0.1:%d.\n", AdminPort);
}

void ExitAdmin() {
	pthread_mutex_lock(&RequestMutex);
	AdminStopping = true;
	bool Pending = RequestWaiting || RequestTaken;
	if (Pending) {
		ReplyOk = false;
		ReplyText = "The game server is shutting down.\n";
		RequestWaiting = false;
		RequestTaken = false;
	}
	pthread_mutex_unlock(&RequestMutex);
	if (Pending) {
		ReplyReady.up();
	}

	if (AdminThread != 0) {
		JoinThread(AdminThread);
		AdminThread = 0;
	}
	if (AdminSocket != -1) {
		close(AdminSocket);
		AdminSocket = -1;
	}
}
