// houses_writer.cc: writes the game's house areas and houses as houses.toml
// (houses_writer.hh).

#include "houses_writer.hh"

#include <cstdint>

#include "housesdata.hh"
#include "tomlfile.hh"
#include "tomlwrite.hh"

// The fields written on one line of a house's fields.
const int FIELDS_PER_LINE = 5;

static std::string Header() {
	std::string Text;
	Text += "# houses.toml: the houses of the game server and the areas they lie in,\n";
	Text += "# converted from the original houseareas.dat and houses.dat by\n";
	Text += "# patella-convert. The game reads houses.toml in its data folder ([paths]\n";
	Text += "# data of config.toml). An unknown or missing key, a value of the wrong type\n";
	Text += "# or out of range, an area that does not exist, or house ids that do not\n";
	Text += "# ascend, is an error. Who owns a house, its subowners and guests are not\n";
	Text += "# here: they are in owners.dat, which the game writes.\n";
	Text += "#\n";
	Text += "# Each [[area]] is a house area, a part of a town:\n";
	Text += "#   id         The area's number, 0 to 65535, each at most once.\n";
	Text += "#   name       The area's name, for the reader; the game leaves it.\n";
	Text += "#   sqm_price  What each field of a house in the area adds to its rent, in\n";
	Text += "#              gold.\n";
	Text += "#   depot      The number of the town's depot (Depot in map.dat): the town\n";
	Text += "#              the query manager lists the area's houses under, and the depot\n";
	Text += "#              the owner's items go to when a house is cleared.\n";
	Text += "#\n";
	Text += "# Each [[house]] is a house, in the order of their ids:\n";
	Text += "#   id           The house's number, 1 to 65535, each higher than the one\n";
	Text += "#                before. The doors, owners.dat and the query manager know\n";
	Text += "#                the house by it.\n";
	Text += "#   name         Its name, at most 49 characters.\n";
	Text += "#   description  Its description for the query manager (the auctions), at\n";
	Text += "#                most 499 characters.\n";
	Text += "#   rent_offset  What its rent adds to sqm_price times its number of fields,\n";
	Text += "#                in gold.\n";
	Text += "#   area         The id of its [[area]]; the comment after it gives the\n";
	Text += "#                area's name.\n";
	Text += "#   guild_house  Whether it is a guild hall, true or false.\n";
	Text += "#   exit         The field [x, y, z] a player is put on who must leave the\n";
	Text += "#                house or may not log in inside it.\n";
	Text += "#   center       The field [x, y, z] the query manager shows it at; the\n";
	Text += "#                game looks for its guests and items around it, as far out\n";
	Text += "#                as the farthest field of any house from its center.\n";
	Text += "#   fields       The fields [x, y, z] that belong to it. A field that\n";
	Text += "#                already belongs to a house earlier in the file stays with\n";
	Text += "#                that house (and an error is logged).\n";
	Text += "# Coordinates: x and y 0 to 65535, z 0 to 15. An exit or a center with x 0\n";
	Text += "# counts as not set (an error is logged).\n";
	return Text;
}

// A text for a comment: Latin-1 as UTF-8, control characters, which a
// comment cannot hold, as spaces.
static std::string CommentText(const std::string &Latin1) {
	std::string Text = Latin1;
	for (char &Character : Text) {
		unsigned char c = static_cast<unsigned char>(Character);
		if (c < 0x20 || c == 0x7F) {
			Character = ' ';
		}
	}
	return Latin1ToUtf8(Text);
}

static std::string Coordinate(int x, int y, int z) {
	if (x < 0 || x > MAX_HOUSE_COORDINATE_XY || y < 0 || y > MAX_HOUSE_COORDINATE_XY || z < 0
			|| z > MAX_HOUSE_COORDINATE_Z) {
		throw "a coordinate outside x and y 0 to 65535 and z 0 to 15, which houses.toml cannot hold";
	}
	return "[" + std::to_string(x) + ", " + std::to_string(y) + ", " + std::to_string(z) + "]";
}

std::string HousesToToml(const THousesDatExtras &Extras) {
	if (static_cast<int>(Extras.AreaNames.size()) != HouseAreas
			|| static_cast<int>(Extras.Houses.size()) != Houses) {
		throw "the noted areas and houses are not the tables'";
	}

	std::string Text = Header();
	for (int i = 0; i < HouseAreas; i++) {
		const THouseArea &Area = HouseArea(i);
		for (int j = 0; j < i; j++) {
			if (HouseArea(j).ID == Area.ID) {
				throw "an area number twice, which houses.toml cannot hold";
			}
		}
		Text += "\n[[area]]\n";
		Text += "id = " + std::to_string(Area.ID) + "\n";
		Text += "name = " + TomlQuote(Extras.AreaNames[static_cast<size_t>(i)]) + "\n";
		Text += "sqm_price = " + std::to_string(Area.SQMPrice) + "\n";
		Text += "depot = " + std::to_string(Area.DepotNr) + "\n";
	}

	for (int i = 0; i < Houses; i++) {
		const THouse &Entry = House(i);
		const THouseDatHouse &Noted = Extras.Houses[static_cast<size_t>(i)];
		int AreaIndex = -1;
		for (int j = 0; j < HouseAreas && AreaIndex == -1; j++) {
			if (HouseArea(j).ID == Noted.Area) {
				AreaIndex = j;
			}
		}
		if (AreaIndex == -1) {
			throw "a house whose area is not there";
		}
		const THouseArea &Area = HouseArea(AreaIndex);
		uint32_t Rent = static_cast<uint32_t>(Area.SQMPrice) * static_cast<uint32_t>(Entry.Size)
				+ static_cast<uint32_t>(Noted.RentOffset);
		if (Entry.Rent != Rent || Entry.DepotNr != Area.DepotNr
				|| Entry.Size != static_cast<int>(Noted.Fields.size())) {
			throw "a house whose rent, depot or size does not go with its area and fields";
		}

		Text += "\n[[house]]\n";
		Text += "id = " + std::to_string(Entry.ID) + "\n";
		Text += "name = " + TomlQuote(Entry.Name) + "\n";
		Text += "description = " + TomlQuote(Entry.Description) + "\n";
		Text += "rent_offset = " + std::to_string(Noted.RentOffset) + "\n";
		Text += "area = " + std::to_string(Noted.Area) + "  # "
				+ CommentText(Extras.AreaNames[static_cast<size_t>(AreaIndex)]) + "\n";
		Text += std::string("guild_house = ") + (Entry.GuildHouse ? "true" : "false") + "\n";
		Text += "exit = " + Coordinate(Entry.ExitX, Entry.ExitY, Entry.ExitZ) + "\n";
		Text += "center = " + Coordinate(Entry.CenterX, Entry.CenterY, Entry.CenterZ) + "\n";
		if (Noted.Fields.empty()) {
			Text += "fields = []\n";
			continue;
		}
		Text += "fields = [\n";
		for (size_t j = 0; j < Noted.Fields.size(); j++) {
			const THouseDatField &Field = Noted.Fields[j];
			Text += (j % FIELDS_PER_LINE == 0 ? "\t" : " ") + Coordinate(Field.x, Field.y, Field.z) + ",";
			if (j % FIELDS_PER_LINE == FIELDS_PER_LINE - 1 || j + 1 == Noted.Fields.size()) {
				Text += "\n";
			}
		}
		Text += "]\n";
	}
	return Text;
}
