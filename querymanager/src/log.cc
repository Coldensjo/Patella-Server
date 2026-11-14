// log.cc: the query manager's log (log.hh).

#include "log.hh"

#include <atomic>
#include <cstdarg>
#include <cstdio>
#include <ctime>
#include <mutex>

#include <sys/time.h>

namespace qm {

namespace {

std::atomic<int> Level{static_cast<int>(LogLevel::Info)};
std::mutex Mutex;
FILE *File = nullptr;

const char *levelName(LogLevel level) {
	switch (level) {
	case LogLevel::Error: return "ERROR";
	case LogLevel::Warning: return "WARN ";
	case LogLevel::Info: return "INFO ";
	case LogLevel::Debug: return "DEBUG";
	}
	return "?";
}

} // namespace

void setLogLevel(LogLevel level) {
	Level.store(static_cast<int>(level));
}

LogLevel logLevel() {
	return static_cast<LogLevel>(Level.load());
}

bool parseLogLevel(const std::string &text, LogLevel &level) {
	if (text == "error") {
		level = LogLevel::Error;
	} else if (text == "warning") {
		level = LogLevel::Warning;
	} else if (text == "info") {
		level = LogLevel::Info;
	} else if (text == "debug") {
		level = LogLevel::Debug;
	} else {
		return false;
	}
	return true;
}

bool setLogFile(const std::string &path) {
	FILE *file = std::fopen(path.c_str(), "a");
	if (file == nullptr) {
		return false;
	}
	std::lock_guard<std::mutex> lock(Mutex);
	if (File != nullptr) {
		std::fclose(File);
	}
	File = file;
	return true;
}

void closeLog() {
	std::lock_guard<std::mutex> lock(Mutex);
	if (File != nullptr) {
		std::fclose(File);
		File = nullptr;
	}
}

void logMessage(LogLevel level, const char *format, ...) {
	if (static_cast<int>(level) > Level.load()) {
		return;
	}
	char text[2048];
	va_list args;
	va_start(args, format);
	std::vsnprintf(text, sizeof(text), format, args);
	va_end(args);

	struct timeval now;
	gettimeofday(&now, nullptr);
	struct tm parts;
	localtime_r(&now.tv_sec, &parts);
	char stamp[32];
	std::strftime(stamp, sizeof(stamp), "%Y-%m-%d %H:%M:%S", &parts);

	std::lock_guard<std::mutex> lock(Mutex);
	FILE *out = File != nullptr ? File : stderr;
	std::fprintf(out, "%s.%03d %s %s\n", stamp, static_cast<int>(now.tv_usec / 1000), levelName(level), text);
	std::fflush(out);
}

} // namespace qm
