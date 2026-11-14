// tomlwrite.cc: what the converters share for writing TOML (tomlwrite.hh).

#include "tomlwrite.hh"

#include <cstdio>

#include "tomlfile.hh"

std::string TomlQuote(const std::string &Latin1) {
	std::string Escaped;
	for (char Character : Latin1) {
		unsigned char c = static_cast<unsigned char>(Character);
		switch (c) {
			case '"': Escaped += "\\\""; break;
			case '\\': Escaped += "\\\\"; break;
			case '\b': Escaped += "\\b"; break;
			case '\t': Escaped += "\\t"; break;
			case '\n': Escaped += "\\n"; break;
			case '\f': Escaped += "\\f"; break;
			case '\r': Escaped += "\\r"; break;
			default:
				if (c < 0x20 || c == 0x7F) {
					char Code[8];
					snprintf(Code, sizeof(Code), "\\u%04X", c);
					Escaped += Code;
				} else {
					Escaped += Character;
				}
				break;
		}
	}
	return "\"" + Latin1ToUtf8(Escaped) + "\"";
}

void WriteTextFile(const char *FileName, const std::string &Text) {
	FILE *File = std::string(FileName) == "-" ? stdout : fopen(FileName, "wb");
	if (File == nullptr) {
		throw "cannot open the output file";
	}
	bool Ok = fwrite(Text.data(), 1, Text.size(), File) == Text.size();
	if (File == stdout) {
		Ok = fflush(File) == 0 && Ok;
	} else {
		Ok = fclose(File) == 0 && Ok;
	}
	if (!Ok) {
		throw "cannot write the output file";
	}
}
