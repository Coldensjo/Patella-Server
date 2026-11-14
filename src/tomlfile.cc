// tomlfile.cc: the helpers the game's TOML readers share (tomlfile.hh).

#include "tomlfile.hh"

#include <cstdarg>
#include <cstdio>
#include <cstring>

// The text of the errors thrown, which lives on after the throw.
static char ErrorString[4096 + 512];

static const char *SourcePath(const toml::source_region &Where) {
	return Where.path != nullptr ? Where.path->c_str() : "(unknown file)";
}

toml::table TomlParseFile(const char *FileName) {
	try {
		return toml::parse_file(FileName);
	} catch (const toml::parse_error &e) {
		const toml::source_region &Where = e.source();
		if (Where.path == nullptr || Where.begin.line == 0) {
			snprintf(ErrorString, sizeof(ErrorString), "%s: %s", FileName, std::string(e.description()).c_str());
		} else {
			snprintf(ErrorString, sizeof(ErrorString), "%s:%u:%u: %s", SourcePath(Where), Where.begin.line,
					Where.begin.column, std::string(e.description()).c_str());
		}
		throw static_cast<const char *>(ErrorString);
	}
}

void TomlError(const toml::source_region &Where, const char *Format, ...) {
	char Text[1024];
	va_list Args;
	va_start(Args, Format);
	vsnprintf(Text, sizeof(Text), Format, Args);
	va_end(Args);
	snprintf(ErrorString, sizeof(ErrorString), "%s:%u:%u: %s", SourcePath(Where), Where.begin.line,
			Where.begin.column, Text);
	throw static_cast<const char *>(ErrorString);
}

void TomlCheckKeys(const toml::table &Table, const char *What, std::initializer_list<const char *> Keys) {
	for (const auto &Entry : Table) {
		bool Known = false;
		for (const char *Key : Keys) {
			if (Entry.first.str() == Key) {
				Known = true;
				break;
			}
		}
		if (!Known) {
			TomlError(Entry.first.source(), "unknown key \"%s\" in %s", std::string(Entry.first.str()).c_str(), What);
		}
	}
}

const toml::table *TomlGetTable(const toml::table &Table, const char *Key) {
	const toml::node *Node = Table.get(Key);
	if (Node == nullptr) {
		return nullptr;
	}
	if (!Node->is_table()) {
		TomlError(Node->source(), "%s must be a table", Key);
	}
	return Node->as_table();
}

const toml::array *TomlGetArray(const toml::table &Table, const char *Key) {
	const toml::node *Node = Table.get(Key);
	if (Node == nullptr) {
		return nullptr;
	}
	if (!Node->is_array()) {
		TomlError(Node->source(), "%s must be an array", Key);
	}
	return Node->as_array();
}

void TomlGetNumber(const toml::table &Table, const char *Key, int &Value, int Min, int Max) {
	const toml::node *Node = Table.get(Key);
	if (Node == nullptr) {
		return;
	}
	if (!Node->is_integer()) {
		TomlError(Node->source(), "%s must be an integer", Key);
	}
	int64_t Number = Node->as_integer()->get();
	if (Number < Min || Number > Max) {
		TomlError(Node->source(), "%s must lie between %d and %d", Key, Min, Max);
	}
	Value = static_cast<int>(Number);
}

bool TomlGetString(const toml::table &Table, const char *Key, char *Buffer, int Size) {
	const toml::node *Node = Table.get(Key);
	if (Node == nullptr) {
		return false;
	}
	if (!Node->is_string()) {
		TomlError(Node->source(), "%s must be a string", Key);
	}
	std::string Latin1;
	if (!Utf8ToLatin1(Node->as_string()->get(), Latin1)) {
		TomlError(Node->source(), "%s has a character outside Latin-1", Key);
	}
	if (Latin1.find('\0') != std::string::npos) {
		TomlError(Node->source(), "%s has a zero character", Key);
	}
	if (static_cast<int>(Latin1.size()) >= Size) {
		TomlError(Node->source(), "%s is longer than %d characters", Key, Size - 1);
	}
	memcpy(Buffer, Latin1.c_str(), Latin1.size() + 1);
	return true;
}

bool Utf8ToLatin1(const std::string &Text, std::string &Latin1) {
	// toml++ hands over valid UTF-8 only, so the sequences need no checking
	// beyond their first byte.
	Latin1.clear();
	for (size_t i = 0; i < Text.size(); i++) {
		unsigned char c = static_cast<unsigned char>(Text[i]);
		if (c < 0x80) {
			Latin1 += static_cast<char>(c);
		} else if ((c == 0xC2 || c == 0xC3) && i + 1 < Text.size()) {
			unsigned char d = static_cast<unsigned char>(Text[i + 1]);
			Latin1 += static_cast<char>(((c & 0x1F) << 6) | (d & 0x3F));
			i++;
		} else {
			return false;
		}
	}
	return true;
}

std::string Latin1ToUtf8(const std::string &Latin1) {
	std::string Text;
	for (char Character : Latin1) {
		unsigned char c = static_cast<unsigned char>(Character);
		if (c < 0x80) {
			Text += static_cast<char>(c);
		} else {
			Text += static_cast<char>(0xC0 | (c >> 6));
			Text += static_cast<char>(0x80 | (c & 0x3F));
		}
	}
	return Text;
}
