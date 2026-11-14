// items_writer.cc: writes the game's object type table as items.toml
// (items_writer.hh).

#include "items_writer.hh"

#include <cstdint>
#include <string>
#include <vector>

#include "objects.hh"
#include "tomlwrite.hh"

// Words as comment lines of at most 80 characters, each line starting with
// Indent, the words separated by Separator.
static std::string CommentList(const std::vector<std::string> &Words, const std::string &Indent,
		const std::string &Separator) {
	std::string Text;
	std::string Line = Indent;
	for (size_t i = 0; i < Words.size(); i++) {
		std::string Word = Words[i] + (i + 1 < Words.size() ? Separator : "");
		std::string Trimmed = Word;
		while (!Trimmed.empty() && Trimmed.back() == ' ') {
			Trimmed.pop_back();
		}
		if (Line.size() > Indent.size() && Line.size() + Trimmed.size() > 80) {
			while (Line.back() == ' ') {
				Line.pop_back();
			}
			Text += Line + "\n";
			Line = Indent;
		}
		Line += Word;
	}
	while (!Line.empty() && Line.back() == ' ') {
		Line.pop_back();
	}
	return Text + Line + "\n";
}

static std::string Header() {
	std::string Text;
	Text += "# items.toml: the object types of the game server (items, grounds, walls,\n";
	Text += "# doors, corpses, magic fields, ...), converted from the original objects.srv\n";
	Text += "# by patella-convert. The game reads items.toml in its data folder ([paths]\n";
	Text += "# data of config.toml). An unknown key, flag or attribute, or a value of the\n";
	Text += "# wrong type, is an error.\n";
	Text += "#\n";
	Text += "# Each [[item]] is one type:\n";
	Text += "#   id           The type number, 0 to 65535, each at most once. 0 is the map\n";
	Text += "#                container, 1 to 10 are the body containers, 99 is the\n";
	Text += "#                creature.\n";
	Text += "#   name         The name the game shows. Left out, the type has none.\n";
	Text += "#   description  What the game adds when the object is looked at. Left out,\n";
	Text += "#                the type has none.\n";
	Text += "#   flags        What the type is and does, by the names below. Left out,\n";
	Text += "#                none.\n";
	Text += "#   attributes   The type's values by the names below, each a 32-bit number\n";
	Text += "#                (written signed). The game reads an attribute only while the\n";
	Text += "#                type has the flag it belongs to. One left out is 0. Those\n";
	Text += "#                listed at the end are given by name instead. meaning makes\n";
	Text += "#                the type the special object of that name (each for at most\n";
	Text += "#                one type), which the game asks for by it (money, depot\n";
	Text += "#                chests, blood, magic fields, ...).\n";
	Text += "#\n";
	Text += "# The flags:\n";
	std::vector<std::string> Words;
	for (int i = 0; i < FLAGS; i++) {
		Words.push_back(FlagKeys[i]);
	}
	Text += CommentList(Words, "#   ", ", ");
	Text += "#\n";
	Text += "# The attributes, each with the flag it belongs to:\n";
	Words.clear();
	for (int i = 0; i < TYPEATTRIBUTES; i++) {
		Words.push_back(std::string(TypeAttributeKeys[i]) + " (" + FlagKeys[TypeAttributeFlags[i]] + ")");
	}
	Text += CommentList(Words, "#   ", ", ");
	Text += "#\n";
	Text += "# The attributes given by name, each with its names. One whose names are\n";
	Text += "# in [] is an array of them, each a bit of the value ([] for none).\n";
	for (int Attribute = 0; Attribute < TYPEATTRIBUTES; Attribute++) {
		const TNamedAttribute *Named = GetNamedAttribute(Attribute);
		if (Named == nullptr) {
			continue;
		}
		Words.clear();
		for (int i = 0; i < Named->Count; i++) {
			if (Named->Keys[i] != nullptr) {
				Words.push_back(Named->Keys[i]);
			}
		}
		bool Bits = Named->Form == NAMED_BITS;
		Words.front() = std::string(TypeAttributeKeys[Attribute]) + (Bits ? ": [" : ": ") + Words.front();
		if (Bits) {
			Words.back() += "]";
		}
		// The names of one attribute, its further lines indented.
		Text += "#   " + CommentList(Words, "#     ", ", ").substr(6);
	}
	return Text;
}

// The name of a named attribute's value in items.toml: a string, or an array
// of them. Throws for a value or bit without a name.
static std::string NamedValue(const TNamedAttribute &Named, uint32_t Value) {
	static std::string Refusal;
	Refusal = std::string("a value of ") + TypeAttributeKeys[Named.Attribute]
			+ " without a name, which items.toml cannot hold";
	if (Named.Form == NAMED_VALUE) {
		const char *Key = GetValueKey(Named.Keys, Named.Count, static_cast<int32_t>(Value));
		if (Key == nullptr) {
			throw Refusal.c_str();
		}
		return TomlQuote(Key);
	}

	std::vector<std::string> Names;
	for (int Bit = 0; Bit < 32; Bit++) {
		if ((Value & (UINT32_C(1) << Bit)) == 0) {
			continue;
		}
		const char *Key = GetValueKey(Named.Keys, Named.Count, Bit);
		if (Key == nullptr) {
			throw Refusal.c_str();
		}
		Names.push_back(TomlQuote(Key));
	}
	if (Named.Form == NAMED_BIT) {
		if (Names.size() != 1) {
			throw Refusal.c_str();
		}
		return Names.front();
	}
	std::string Text;
	for (const std::string &Name : Names) {
		Text += (Text.empty() ? "" : ", ") + Name;
	}
	return "[" + Text + "]";
}

std::string ItemsToToml(const TObjectsSrvExtras &Extras) {
	// A special object whose type was defined again without its meaning keeps
	// it in SpecialObjects; items.toml has no way to say that.
	for (int Meaning = 1; Meaning < SPECIALMEANINGS; Meaning++) {
		int TypeID = SpecialObjects[Meaning].getTypeID();
		if (TypeID == 0) {
			continue;
		}
		auto Entry = Extras.Types.find(TypeID);
		bool Given = false;
		if (Entry != Extras.Types.end()) {
			for (int Attribute : Entry->second) {
				Given = Given || Attribute == MEANING;
			}
		}
		if (!Given || ObjectTypes(TypeID).Attributes[MEANING] != static_cast<uint32_t>(Meaning)) {
			throw "a special object whose type was defined again without its meaning, which items.toml cannot hold";
		}
	}

	std::string Text = Header();
	for (const auto &Entry : Extras.Types) {
		int TypeID = Entry.first;
		if (TypeID > 65535) {
			throw "a type above 65535, which items.toml cannot hold";
		}

		const TObjectType &Type = ObjectTypes(TypeID);
		Text += "\n[[item]]\n";
		Text += "id = " + std::to_string(TypeID) + "\n";
		if (Type.Name != nullptr) {
			Text += "name = " + TomlQuote(Type.Name) + "\n";
		}
		if (Type.Description != nullptr) {
			Text += "description = " + TomlQuote(Type.Description) + "\n";
		}

		std::string Flags;
		for (int Flag = 0; Flag < FLAGS; Flag++) {
			if ((Type.Flags[Flag / 8] & (1 << (Flag % 8))) != 0) {
				Flags += std::string(Flags.empty() ? "" : ", ") + "\"" + FlagKeys[Flag] + "\"";
			}
		}
		if (!Flags.empty()) {
			Text += "flags = [" + Flags + "]\n";
		}

		std::string Attributes;
		for (int Attribute : Entry.second) {
			int32_t Value = static_cast<int32_t>(Type.Attributes[Attribute]);
			if (Attribute == MEANING && Value == SPECIALMEANINGS) {
				throw "a meaning of 49, which items.toml refuses (it lies past SpecialObjects)";
			}
			const TNamedAttribute *Named = GetNamedAttribute(Attribute);
			Attributes += std::string(Attributes.empty() ? "" : ", ") + TypeAttributeKeys[Attribute] + " = "
					+ (Named != nullptr ? NamedValue(*Named, Type.Attributes[Attribute]) : std::to_string(Value));
		}
		if (!Attributes.empty()) {
			Text += "attributes = { " + Attributes + " }\n";
		}
	}
	return Text;
}
