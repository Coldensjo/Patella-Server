// Minimal test support: CHECK macros that count failures and keep going.

#ifndef QM_TEST_HH
#define QM_TEST_HH

#include <cstdio>
#include <exception>

namespace test {

inline int &failures() {
	static int count = 0;
	return count;
}

inline int &checks() {
	static int count = 0;
	return count;
}

inline void report(bool ok, const char *expression, const char *file, int line) {
	checks()++;
	if (!ok) {
		failures()++;
		std::fprintf(stderr, "%s:%d: CHECK failed: %s\n", file, line, expression);
	}
}

// Runs the test function and turns the outcome into a process exit code.
inline int run(void (*function)()) {
	try {
		function();
	} catch (const std::exception &e) {
		failures()++;
		std::fprintf(stderr, "uncaught exception: %s\n", e.what());
	}
	std::fprintf(stderr, "%d checks, %d failures\n", checks(), failures());
	return failures() == 0 ? 0 : 1;
}

} // namespace test

#define CHECK(expression) test::report(static_cast<bool>(expression), #expression, __FILE__, __LINE__)

#define CHECK_THROWS(statement, exception) \
	do { \
		bool thrown = false; \
		try { \
			statement; \
		} catch (const exception &) { \
			thrown = true; \
		} \
		test::report(thrown, #statement " throws " #exception, __FILE__, __LINE__); \
	} while (false)

#endif
