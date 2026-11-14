// mapdata.cc: the map's settings as the world defines them (mapdata.hh) and
// their loader, which reads map.toml (data/map.toml in the repository,
// converted from the original map.dat and the mem.dat it includes by
// tools/convert).

#include "tomlfile.hh"

#include <climits>
#include <cstdio>
#include <string>
#include <vector>

#include "config.hh"
#include "dbfuncs.hh"
#include "utils.hh"
#include "mapdata.hh"

int SectorXMin;
int SectorXMax;
int SectorYMin;
int SectorYMax;
int SectorZMin;
int SectorZMax;
int RefreshedCylinders;
int HashTableSize;
int OBCount;
vector<TDepotInfo> DepotInfo(0, 4, 5);
vector<TMark> Mark(0, 4, 5);
int Marks;
int NewbieStartPositionX;
int NewbieStartPositionY;
int NewbieStartPositionZ;
int VeteranStartPositionX;
int VeteranStartPositionY;
int VeteranStartPositionZ;

// The value under Key, which Table must have. What names the table in the
// error text ("the top level", "[sectors]", "a mark").
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

// The table under Key, which File must have.
static const toml::table &RequiredTable(const toml::table &File, const char *Key) {
	const toml::node &Node = Required(File, Key, "the top level");
	if (!Node.is_table()) {
		TomlError(Node.source(), "%s must be a table", Key);
	}
	return *Node.as_table();
}

// A field [x, y, z]: x MinX to 65535, y 0 to 65535, z 0 to 15.
static void ReadCoordinate(const toml::node &Node, const char *What, int MinX, int &x, int &y, int &z) {
	const toml::array *Array = Node.as_array();
	if (Array == nullptr || Array->size() != 3) {
		TomlError(Node.source(), "%s must be an array [x, y, z]", What);
	}
	int *Values[3] = {&x, &y, &z};
	for (int i = 0; i < 3; i++) {
		static const char Axes[3] = {'x', 'y', 'z'};
		const toml::node &Value = *Array->get(static_cast<size_t>(i));
		int Min = i == 0 ? MinX : 0;
		int Max = i < 2 ? MAX_MAP_COORDINATE_XY : MAX_MAP_COORDINATE_Z;
		if (!Value.is_integer()) {
			TomlError(Value.source(), "%c of %s must be an integer", Axes[i], What);
		}
		int64_t Number = Value.as_integer()->get();
		if (Number < Min || Number > Max) {
			TomlError(Value.source(), "%c of %s must lie between %d and %d", Axes[i], What, Min, Max);
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

// The lower and upper end of one axis of [sectors].
static void ReadSectorRange(const toml::table &Sectors, const char *MinKey, const char *MaxKey, int Lowest,
		int Highest, int &Min, int &Max) {
	const toml::node &MinNode = Required(Sectors, MinKey, "[sectors]");
	Min = ReadInteger(MinNode, MinKey, Lowest, Highest);
	Max = ReadInteger(Required(Sectors, MaxKey, "[sectors]"), MaxKey, Lowest, Highest);
	if (Min > Max) {
		TomlError(MinNode.source(), "%s %d is greater than %s %d", MinKey, Min, MaxKey, Max);
	}
}

static void ReadSectors(const toml::table &Sectors) {
	TomlCheckKeys(Sectors, "[sectors]", {"x_min", "x_max", "y_min", "y_max", "z_min", "z_max"});
	ReadSectorRange(Sectors, "x_min", "x_max", MIN_SECTOR_XY, MAX_SECTOR_XY, SectorXMin, SectorXMax);
	ReadSectorRange(Sectors, "y_min", "y_max", MIN_SECTOR_XY, MAX_SECTOR_XY, SectorYMin, SectorYMax);
	ReadSectorRange(Sectors, "z_min", "z_max", 0, MAX_SECTOR_Z, SectorZMin, SectorZMax);
}

static void ReadMemory(const toml::table &Memory) {
	TomlCheckKeys(Memory, "[memory]", {"objects", "cache_size"});
	const toml::node &ObjectsNode = Required(Memory, "objects", "[memory]");
	int Objects = ReadInteger(ObjectsNode, "objects", 1, INT_MAX);
	if ((Objects & (Objects - 1)) != 0) {
		TomlError(ObjectsNode.source(), "objects must be a power of 2");
	}
	const toml::node &CacheNode = Required(Memory, "cache_size", "[memory]");
	int CacheSize = ReadInteger(CacheNode, "cache_size", OBJECTS_PER_BLOCK, INT_MAX);
	if (CacheSize % OBJECTS_PER_BLOCK != 0) {
		TomlError(CacheNode.source(), "cache_size must be a multiple of %d", OBJECTS_PER_BLOCK);
	}
	HashTableSize = Objects;
	OBCount = CacheSize / OBJECTS_PER_BLOCK;
}

// One [[mark]] of map.toml.
static void ReadMark(const toml::table &Table) {
	TomlCheckKeys(Table, "[[mark]]", {"name", "position"});
	TMark &Entry = Mark(Marks);
	Required(Table, "name", "a mark");
	TomlGetString(Table, "name", Entry.Name, sizeof(Entry.Name));
	ReadCoordinate(Required(Table, "position", "a mark"), "position", 0, Entry.x, Entry.y, Entry.z);
	Marks++;
}

// One [[depot]] of map.toml. Defined notes the depot numbers given so far.
static void ReadDepot(const toml::table &Table, std::vector<bool> &Defined) {
	TomlCheckKeys(Table, "[[depot]]", {"id", "town", "size"});
	const toml::node &IDNode = Required(Table, "id", "a depot");
	int ID = ReadInteger(IDNode, "id", 0, MAX_DEPOTS - 1);
	if (Defined[static_cast<size_t>(ID)]) {
		TomlError(IDNode.source(), "depot %d is defined twice", ID);
	}
	Defined[static_cast<size_t>(ID)] = true;
	std::string What = "depot " + std::to_string(ID);
	TDepotInfo &Entry = DepotInfo(ID);
	Required(Table, "town", What.c_str());
	TomlGetString(Table, "town", Entry.Town, sizeof(Entry.Town));
	Entry.Size = ReadInteger(Required(Table, "size", What.c_str()), "size", INT_MIN, INT_MAX);
}

void LoadMapData(const char *FileName) {
	toml::table File = TomlParseFile(FileName);
	TomlCheckKeys(File, "the top level",
			{"refreshed_cylinders", "newbie_start", "veteran_start", "sectors", "memory", "mark", "depot"});
	Marks = 0;

	RefreshedCylinders = ReadInteger(Required(File, "refreshed_cylinders", "the top level"), "refreshed_cylinders",
			INT_MIN, INT_MAX);
	ReadCoordinate(Required(File, "newbie_start", "the top level"), "newbie_start", 1, NewbieStartPositionX,
			NewbieStartPositionY, NewbieStartPositionZ);
	ReadCoordinate(Required(File, "veteran_start", "the top level"), "veteran_start", 1, VeteranStartPositionX,
			VeteranStartPositionY, VeteranStartPositionZ);
	ReadSectors(RequiredTable(File, "sectors"));
	ReadMemory(RequiredTable(File, "memory"));

	if (const toml::array *Entries = ReadEntries(File, "mark")) {
		for (const toml::node &Node : *Entries) {
			ReadMark(*Node.as_table());
		}
	}

	std::vector<bool> Defined(MAX_DEPOTS, false);
	if (const toml::array *Entries = ReadEntries(File, "depot")) {
		for (const toml::node &Node : *Entries) {
			ReadDepot(*Node.as_table(), Defined);
		}
	}
}

void ReadMapConfig() {
	char FileName[4096];
	snprintf(FileName, sizeof(FileName), "%s/map.toml", DATAPATH);
	LoadMapData(FileName);
}
