// conversion_writer.cc: writes the game's conversion of old object numbers as
// conversion.toml (conversion_writer.hh).

#include "conversion_writer.hh"

#include "objects.hh"

static std::string Header() {
	std::string Text;
	Text += "# conversion.toml: the old numbers of the object types of the game server,\n";
	Text += "# converted from the original conversion.lst by patella-convert. The game\n";
	Text += "# reads conversion.toml in its data folder ([paths] data of config.toml). An\n";
	Text += "# unknown key, a group or number that is not 0 to 255 written without leading\n";
	Text += "# zeros, or a type that is not 1 to 8191, is an error.\n";
	Text += "#\n";
	Text += "# Before each object type had a number of its own (the id in items.toml), a\n";
	Text += "# type was known by a group and a number within it. The game still names some\n";
	Text += "# types that way: its code those of the food, arrows and bolts the spells\n";
	Text += "# make, the enchanted staff and the amulet of loss, and spells.toml the runes\n";
	Text += "# the rune spells make. This table gives their types.\n";
	Text += "#\n";
	Text += "# Each table [group.G] holds the old numbers N of group G as N = type, the\n";
	Text += "# id of the type in items.toml. A number that is not given has no type (the\n";
	Text += "# game logs an error when it asks for one). The game sets 0/0 to 0/10 (no\n";
	Text += "# type and the body containers, which keep their numbers) and 250/0 (the\n";
	Text += "# creature, type 99) itself; they may not be given.\n";
	return Text;
}

static bool OwnEntry(int Place) {
	return Place <= LAST_BODY_POSITION || Place == 250 * 256 + 0;
}

std::string ConversionToToml() {
	for (int Place = 0; Place < 65536; Place++) {
		int Expected = Place == 250 * 256 + 0 ? TYPEID_CREATURE : Place;
		if (OwnEntry(Place) ? NewType[Place] != Expected
				: NewType[Place] != -1 && (NewType[Place] < 1 || NewType[Place] > CONVERSION_TYPES - 1)) {
			throw "a type outside 1 to 8191, or the game's own entries not as the game sets them";
		}
	}

	std::string Text = Header();
	for (int Group = 0; Group < 256; Group++) {
		bool Started = false;
		for (int Number = 0; Number < 256; Number++) {
			int Place = Group * 256 + Number;
			if (OwnEntry(Place) || NewType[Place] == -1) {
				continue;
			}
			if (!Started) {
				Text += "\n[group." + std::to_string(Group) + "]\n";
				Started = true;
			}
			Text += std::to_string(Number) + " = " + std::to_string(NewType[Place]) + "\n";
		}
	}
	return Text;
}
