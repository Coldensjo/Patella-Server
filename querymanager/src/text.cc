// text.cc: the conversions between ISO-8859-1 and UTF-8, and the lower-case
// key of a name (text.hh).

#include "text.hh"

#include <cstdint>

namespace qm {

std::string latin1ToUtf8(const std::string &latin1) {
	std::string out;
	out.reserve(latin1.size());
	for (char ch : latin1) {
		uint8_t c = static_cast<uint8_t>(ch);
		if (c < 0x80) {
			out.push_back(static_cast<char>(c));
		} else {
			out.push_back(static_cast<char>(0xC0 | (c >> 6)));
			out.push_back(static_cast<char>(0x80 | (c & 0x3F)));
		}
	}
	return out;
}

std::string utf8ToLatin1(const std::string &utf8) {
	std::string out;
	out.reserve(utf8.size());
	size_t i = 0;
	while (i < utf8.size()) {
		uint8_t c = static_cast<uint8_t>(utf8[i]);
		if (c < 0x80) {
			out.push_back(static_cast<char>(c));
			i += 1;
		} else if ((c & 0xE0) == 0xC0 && i + 1 < utf8.size()
				&& (static_cast<uint8_t>(utf8[i + 1]) & 0xC0) == 0x80) {
			uint32_t cp = (static_cast<uint32_t>(c & 0x1F) << 6)
				| (static_cast<uint8_t>(utf8[i + 1]) & 0x3F);
			out.push_back(cp >= 0x80 && cp <= 0xFF ? static_cast<char>(cp) : '?');
			i += 2;
		} else {
			// Longer sequence or stray byte: skip the lead byte and its
			// continuation bytes, emit one replacement.
			i += 1;
			while (i < utf8.size() && (static_cast<uint8_t>(utf8[i]) & 0xC0) == 0x80) {
				i += 1;
			}
			out.push_back('?');
		}
	}
	return out;
}

std::string foldLatin1(const std::string &latin1) {
	std::string out = latin1;
	for (char &ch : out) {
		uint8_t c = static_cast<uint8_t>(ch);
		if (c >= 'A' && c <= 'Z') {
			c = static_cast<uint8_t>(c + 0x20);
		} else if (c >= 0xC0 && c <= 0xDE && c != 0xD7) {
			// Upper-case accented letters; 0xD7 is the multiplication sign.
			c = static_cast<uint8_t>(c + 0x20);
		}
		ch = static_cast<char>(c);
	}
	return out;
}

} // namespace qm
