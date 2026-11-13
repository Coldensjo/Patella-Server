// Logging for the query manager: one line per event, with a timestamp,
// to stderr or to a file. Safe to call from several threads.

#ifndef QM_LOG_HH
#define QM_LOG_HH

#include <string>

namespace qm {

enum class LogLevel {
	Error = 0,
	Warning = 1,
	Info = 2,      // one line per request
	Debug = 3,     // additionally request and response sizes and hex dumps
};

void setLogLevel(LogLevel level);
LogLevel logLevel();
// Parses "error", "warning", "info" or "debug".
bool parseLogLevel(const std::string &text, LogLevel &level);
// Appends to the file from now on; false if it cannot be opened.
bool setLogFile(const std::string &path);
void closeLog();

void logMessage(LogLevel level, const char *format, ...) __attribute__((format(printf, 2, 3)));

} // namespace qm

#endif
