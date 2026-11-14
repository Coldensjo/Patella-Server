// housesdata.cc: the houses as the world defines them (housesdata.hh): the
// house areas and the houses and their loader, which reads houses.toml
// (data/houses.toml in the repository, converted from the original
// houseareas.dat and houses.dat by tools/convert).

#include "tomlfile.hh"

#include <climits>
#include <cstdio>
#include <cstdlib>
#include <string>

#include "utils.hh"
#include "housesdata.hh"

vector<THouseArea> HouseArea(0, 99, 50);
int HouseAreas;
vector<THouse> House(0, 99, 100);
int Houses;
int MaxHouseX;
int MaxHouseY;

THouseArea *GetHouseArea(unsigned short ID) {
	for (int i = 0; i < HouseAreas; i++) {
		if (HouseArea(i).ID == ID) {
			return &HouseArea(i);
		}
	}

	error("GetHouseArea: Gebiet mit ID %d nicht gefunden.\n", ID);
	return nullptr;
}

// The value under Key, which Table must have. What names the entry in the
// error text ("[[house]] 10001").
static const toml::node &Required(const toml::table &Table, const char *Key, const char *What) {
	const toml::node *Node = Table.get(Key);
	if (Node == nullptr) {
		TomlError(Table.source(), "%s needs %s", What, Key);
	}
	return *Node;
}

static int ReadInteger(const toml::node &Node, const char *What, int Min, int Max) {
	if (!Node.is_integer()) {
		TomlError(Node.source(), "%s must be an integer", What);
	}
	int64_t Number = Node.as_integer()->get();
	if (Number < Min || Number > Max) {
		TomlError(Node.source(), "%s must lie between %d and %d", What, Min, Max);
	}
	return static_cast<int>(Number);
}

// A field [x, y, z]: x and y 0 to 65535, z 0 to 15.
static void ReadCoordinate(const toml::node &Node, const char *What, int &x, int &y, int &z) {
	const toml::array *Array = Node.as_array();
	if (Array == nullptr || Array->size() != 3) {
		TomlError(Node.source(), "%s must be an array [x, y, z]", What);
	}
	int *Values[3] = {&x, &y, &z};
	for (int i = 0; i < 3; i++) {
		static const char Axes[3] = {'x', 'y', 'z'};
		const toml::node &Value = *Array->get(static_cast<size_t>(i));
		int Max = i < 2 ? MAX_HOUSE_COORDINATE_XY : MAX_HOUSE_COORDINATE_Z;
		if (!Value.is_integer()) {
			TomlError(Value.source(), "%c of %s must be an integer", Axes[i], What);
		}
		int64_t Number = Value.as_integer()->get();
		if (Number < 0 || Number > Max) {
			TomlError(Value.source(), "%c of %s must lie between 0 and %d", Axes[i], What, Max);
		}
		*Values[i] = static_cast<int>(Number);
	}
}

// The entries of the array of tables under Key, or none.
static const toml::array *ReadEntries(const toml::table &File, const char *Key) {
	const toml::array *Entries = TomlGetArray(File, Key);
	if (Entries != nullptr) {
		for (const toml::node &Node : *Entries) {
			if (!Node.is_table()) {
				TomlError(Node.source(), "an entry of %s must be a table", Key);
			}
		}
	}
	return Entries;
}

// One [[area]] of houses.toml.
static void ReadArea(const toml::table &Area) {
	TomlCheckKeys(Area, "[[area]]", {"id", "name", "sqm_price", "depot"});
	const toml::node &IDNode = Required(Area, "id", "an area");
	int ID = ReadInteger(IDNode, "id", 0, USHRT_MAX);
	std::string What = "[[area]] " + std::to_string(ID);
	for (int i = 0; i < HouseAreas; i++) {
		if (HouseArea(i).ID == ID) {
			TomlError(IDNode.source(), "area %d is defined twice", ID);
		}
	}

	// The name is for the reader (and the comments of the houses); the game
	// leaves it.
	const toml::node &Name = Required(Area, "name", What.c_str());
	std::string Latin1;
	if (!Name.is_string() || !Utf8ToLatin1(Name.as_string()->get(), Latin1)) {
		TomlError(Name.source(), "name must be a Latin-1 string");
	}

	THouseArea &Entry = HouseArea(HouseAreas);
	Entry.ID = static_cast<unsigned short>(ID);
	Entry.SQMPrice = ReadInteger(Required(Area, "sqm_price", What.c_str()), "sqm_price", INT_MIN, INT_MAX);
	Entry.DepotNr = ReadInteger(Required(Area, "depot", What.c_str()), "depot", INT_MIN, INT_MAX);
	HouseAreas++;
}

// One [[house]] of houses.toml, the values in the order houses.dat gave them.
static void ReadHouse(const toml::table &Table, THouseFieldFunction *SetField) {
	TomlCheckKeys(Table, "[[house]]",
			{"id", "name", "description", "rent_offset", "area", "guild_house", "exit", "center", "fields"});
	THouse &Entry = House(Houses);
	const toml::node &IDNode = Required(Table, "id", "a house");
	int ID = ReadInteger(IDNode, "id", 1, USHRT_MAX);
	if (Houses > 0 && ID <= House(Houses - 1).ID) {
		TomlError(IDNode.source(), "house %d comes after house %d: the ids must ascend", ID, House(Houses - 1).ID);
	}
	Entry.ID = static_cast<unsigned short>(ID);
	std::string What = "[[house]] " + std::to_string(ID);

	Required(Table, "name", What.c_str());
	TomlGetString(Table, "name", Entry.Name, sizeof(Entry.Name));
	Required(Table, "description", What.c_str());
	TomlGetString(Table, "description", Entry.Description, sizeof(Entry.Description));
	int RentOffset = ReadInteger(Required(Table, "rent_offset", What.c_str()), "rent_offset", INT_MIN, INT_MAX);

	const toml::node &AreaNode = Required(Table, "area", What.c_str());
	THouseArea *Area = GetHouseArea(static_cast<unsigned short>(ReadInteger(AreaNode, "area", 0, USHRT_MAX)));
	if (Area == nullptr) {
		TomlError(AreaNode.source(), "area %d does not exist", static_cast<int>(AreaNode.as_integer()->get()));
	}
	Entry.DepotNr = Area->DepotNr;

	const toml::node &GuildHouse = Required(Table, "guild_house", What.c_str());
	if (!GuildHouse.is_boolean()) {
		TomlError(GuildHouse.source(), "guild_house must be true or false");
	}
	Entry.GuildHouse = GuildHouse.as_boolean()->get();
	Entry.NoAuction = false;

	ReadCoordinate(Required(Table, "exit", What.c_str()), "exit", Entry.ExitX, Entry.ExitY, Entry.ExitZ);
	if (Entry.ExitX == 0) {
		error("LoadHouses: Ausgang f\xfcr Haus %d nicht gesetzt.\n", Entry.ID);
	}

	ReadCoordinate(Required(Table, "center", What.c_str()), "center", Entry.CenterX, Entry.CenterY, Entry.CenterZ);
	if (Entry.CenterX == 0) {
		// Quirk: the text says exit (Ausgang) for the center too.
		error("LoadHouses: Ausgang f\xfcr Haus %d nicht gesetzt.\n", Entry.ID);
	}

	const toml::node &FieldsNode = Required(Table, "fields", What.c_str());
	const toml::array *Fields = FieldsNode.as_array();
	if (Fields == nullptr) {
		TomlError(FieldsNode.source(), "fields must be an array");
	}
	Entry.Size = 0;
	for (const toml::node &Field : *Fields) {
		int x, y, z;
		ReadCoordinate(Field, "a field", x, y, z);
		SetField(x, y, z, Entry.ID);
		Entry.Size++;

		if (abs(x - Entry.CenterX) > MaxHouseX) {
			MaxHouseX = abs(x - Entry.CenterX);
		}

		if (abs(y - Entry.CenterY) > MaxHouseY) {
			MaxHouseY = abs(y - Entry.CenterY);
		}
	}

	Entry.Rent = Area->SQMPrice * Entry.Size + RentOffset;
	Entry.OwnerID = 0;
	Entry.OwnerName[0] = 0;
	Entry.LastTransition = 0;
	Entry.PaidUntil = 0;
	Entry.Subowners = 0;
	Entry.Guests = 0;
	Houses++;
}

void ReadHouseData(const toml::table &File, THouseFieldFunction *SetField) {
	HouseAreas = 0;
	Houses = 0;
	MaxHouseX = 0;
	MaxHouseY = 0;

	TomlCheckKeys(File, "the top level", {"area", "house"});
	const toml::array *Areas = ReadEntries(File, "area");
	const toml::array *HouseEntries = ReadEntries(File, "house");
	if (Areas != nullptr) {
		for (const toml::node &Node : *Areas) {
			ReadArea(*Node.as_table());
		}
	}
	if (HouseEntries != nullptr) {
		for (const toml::node &Node : *HouseEntries) {
			ReadHouse(*Node.as_table(), SetField);
		}
	}
}

void LoadHouseData(const char *FileName, THouseFieldFunction *SetField) {
	toml::table File = TomlParseFile(FileName);
	ReadHouseData(File, SetField);
}
