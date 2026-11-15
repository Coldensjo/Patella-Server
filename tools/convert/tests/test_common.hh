// test_common.hh: what the round-trip tests of the circles and the spells
// share: CHECK, files, running a load in a child process, comparing two dumps
// of "field\tvalue" lines field by field, and checking the scripts that
// spells.toml, commands.toml and items.toml name.

#ifndef PATELLA_CONVERT_TEST_COMMON_HH
#define PATELLA_CONVERT_TEST_COMMON_HH

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <functional>
#include <map>
#include <sstream>
#include <string>

#include <sys/wait.h>
#include <unistd.h>

namespace fs = std::filesystem;

static int Failures = 0;

#define CHECK(Condition) \
	do { \
		if (!(Condition)) { \
			fprintf(stderr, "%s:%d: CHECK failed: %s\n", __FILE__, __LINE__, #Condition); \
			Failures++; \
		} \
	} while (0)

static inline void WriteFile(const fs::path &Path, const std::string &Text) {
	fs::create_directories(Path.parent_path());
	std::ofstream Out(Path, std::ios::binary);
	Out << Text;
}

static inline std::string ReadFile(const fs::path &Path) {
	std::ifstream In(Path, std::ios::binary);
	std::stringstream Text;
	Text << In.rdbuf();
	return Text.str();
}

static inline bool IsError(const std::string &Out) {
	return Out.compare(0, 7, "error: ") == 0 || Out == "crashed";
}

// Runs Work in a child process and returns what it returned there, or
// "error: <text>" for a const char * it threw, or "crashed" for a child that
// did not end normally. Each load so starts from the tables as the game
// starts with them.
static inline std::string InChild(const std::function<std::string()> &Work) {
	fflush(stdout);
	fflush(stderr);
	int Pipe[2];
	if (pipe(Pipe) != 0) {
		perror("pipe");
		exit(1);
	}
	pid_t Child = fork();
	if (Child < 0) {
		perror("fork");
		exit(1);
	}
	if (Child == 0) {
		close(Pipe[0]);
		std::string Out;
		try {
			Out = Work();
		} catch (const char *Error) {
			Out = std::string("error: ") + Error + "\n";
		}
		size_t Written = 0;
		while (Written < Out.size()) {
			ssize_t n = write(Pipe[1], Out.data() + Written, Out.size() - Written);
			if (n <= 0) {
				_exit(1);
			}
			Written += static_cast<size_t>(n);
		}
		close(Pipe[1]);
		_exit(0);
	}
	close(Pipe[1]);
	std::string Out;
	char Buffer[65536];
	ssize_t n;
	while ((n = read(Pipe[0], Buffer, sizeof(Buffer))) > 0) {
		Out.append(Buffer, static_cast<size_t>(n));
	}
	close(Pipe[0]);
	int Status = 0;
	waitpid(Child, &Status, 0);
	if (!WIFEXITED(Status) || WEXITSTATUS(Status) != 0) {
		return "crashed";
	}
	return Out;
}

static inline std::map<std::string, std::string> Fields(const std::string &Dump) {
	std::map<std::string, std::string> Result;
	std::istringstream In(Dump);
	std::string Line;
	while (std::getline(In, Line)) {
		size_t Tab = Line.find('\t');
		if (Tab == std::string::npos) {
			Result["(line)"] += Line + " ";
		} else {
			Result[Line.substr(0, Tab)] = Line.substr(Tab + 1);
		}
	}
	return Result;
}

// The number of fields in which two dumps differ. With Report, prints the
// first 30 of them as "<What>: <field>: <OldName> gives X, <NewName> Y".
static inline int Differences(const std::string &Expected, const std::string &Actual, const char *What,
		const char *OldName, const char *NewName, bool Report) {
	std::map<std::string, std::string> A = Fields(Expected), B = Fields(Actual);
	int Count = 0;
	auto Note = [&](const std::string &Key, const std::string &X, const std::string &Y) {
		if (Report && Count < 30) {
			fprintf(stderr, "%s: %s: %s gives %s, %s %s\n", What, Key.c_str(), OldName, X.c_str(), NewName, Y.c_str());
		}
		Count++;
	};
	for (const auto &Pair : A) {
		auto Other = B.find(Pair.first);
		if (Other == B.end()) {
			Note(Pair.first, Pair.second, "(nothing)");
		} else if (Other->second != Pair.second) {
			Note(Pair.first, Pair.second, Other->second);
		}
	}
	for (const auto &Pair : B) {
		if (A.find(Pair.first) == A.end()) {
			Note(Pair.first, "(nothing)", Pair.second);
		}
	}
	return Count;
}

// Whether two dumps are the same; prints the fields that differ and counts a
// failure otherwise.
static inline bool Same(const std::string &Expected, const std::string &Actual, const char *What, const char *OldName,
		const char *NewName) {
	int Count = Differences(Expected, Actual, What, OldName, NewName, true);
	if (Count != 0) {
		fprintf(stderr, "%s: %d fields differ\n", What, Count);
		Failures++;
	}
	return Count == 0;
}

// Checks that loading Text gives an error "error: <File>:<Line>:..." that
// contains What. Load writes Text to File and loads it in a child.
static inline void ExpectError(const std::function<std::string(const std::string &)> &Load, const fs::path &File,
		const std::string &Text, int Line, const char *What) {
	std::string Error = Load(Text);
	std::string Where = "error: " + File.string() + ":" + std::to_string(Line) + ":";
	bool Ok = Error.compare(0, Where.size(), Where) == 0 && Error.find(What) != std::string::npos;
	if (!Ok) {
		fprintf(stderr, "for:\n%s\nexpected \"%s...%s\", got \"%s\"\n", Text.c_str(), Where.c_str(), What,
				Error.c_str());
		Failures++;
	}
}

// A new temporary folder for a test's files; the test removes it.
static inline fs::path TemporaryFolder(const char *Name) {
	std::string Pattern = std::string("/tmp/") + Name + "-XXXXXX";
	if (mkdtemp(Pattern.data()) == nullptr) {
		perror("mkdtemp");
		exit(1);
	}
	return fs::canonical(Pattern);
}

// Checks every script that the spells.toml, commands.toml or items.toml text
// Toml, the file TomlFile, names (each line "script = \"<name>\""): that it
// is a file in the scripts folder next to TomlFile and that its text defines
// the handler of its folder as a global function at the start of a line,
// "function onCastSpell(" for spells/ and runes/, "function onSay(" for
// commands/ and "function onUse(" for items/ (not "local function"). Returns
// the number of scripts checked.
static inline int CheckScriptFiles(const fs::path &TomlFile, const std::string &Toml) {
	static const struct {
		const char *Folder;
		const char *Handler;
	} Handlers[] = {
		{"spells/", "function onCastSpell("},
		{"runes/", "function onCastSpell("},
		{"commands/", "function onSay("},
		{"items/", "function onUse("},
	};
	const std::string Key = "\nscript = \"";
	fs::path Scripts = TomlFile.parent_path() / "scripts";
	int Count = 0;
	for (size_t At = Toml.find(Key); At != std::string::npos; At = Toml.find(Key, At + 1)) {
		size_t Start = At + Key.size();
		std::string Name = Toml.substr(Start, Toml.find('"', Start) - Start);
		Count++;
		const char *Handler = nullptr;
		for (const auto &Entry : Handlers) {
			if (Name.compare(0, strlen(Entry.Folder), Entry.Folder) == 0) {
				Handler = Entry.Handler;
			}
		}
		fs::path File = Scripts / Name;
		if (Handler == nullptr) {
			fprintf(stderr, "%s: script %s is in none of spells/, runes/, commands/ and items/\n", TomlFile.c_str(),
					Name.c_str());
			Failures++;
		} else if (!fs::is_regular_file(File)) {
			fprintf(stderr, "%s: script %s: there is no file %s\n", TomlFile.c_str(), Name.c_str(), File.c_str());
			Failures++;
		} else if (std::string Text = ReadFile(File);
				Text.compare(0, strlen(Handler), Handler) != 0
				&& Text.find(std::string("\n") + Handler) == std::string::npos) {
			fprintf(stderr, "%s: script %s does not define %s\n", TomlFile.c_str(), Name.c_str(), Handler);
			Failures++;
		}
	}
	return Count;
}

#endif
