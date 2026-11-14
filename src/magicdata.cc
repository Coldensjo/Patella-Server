// magicdata.cc: the data of the spells (magicdata.hh): the syllables of their
// words; the spell table, loaded from spells.toml (data/spells.toml in the
// repository, written by tools/convert from the table the game had in its
// code); and the circles, loaded from circles.toml (data/circles.toml,
// converted from the original circles.dat by tools/convert).

#include "tomlfile.hh"

#include <climits>
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>

#include "config.hh"
#include "magicdata.hh"
#include "strings.hh"

char SpellSyllable[MAX_SYLLABLES][6] = {
	"",
	"al",
	"ad",
	"ex",
	"ut",
	"om",
	"para",
	"ana",
	"evo",
	"ori",
	"mort",
	"lux",
	"liber",
	"vita",
	"flam",
	"pox",
	"hur",
	"moe",
	"ani",
	"ina",
	"eta",
	"amo",
	"hora",
	"gran",
	"cogni",
	"res",
	"mas",
	"vis",
	"som",
	"aqua",
	"frigo",
	"tera",
	"ura",
	"sio",
	"grav",
	"ito",
	"pan",
	"vid",
	"isa",
	"iva",
	"con",
	"",
	"",
	"",
	"",
	"",
	"",
	"",
	"",
	"",
	"",
};
TSpellList SpellList[MAX_SPELLS];
TCircle Circle[MAX_CIRCLES];

// The flags of a spell as spells.toml names them, by their bit in
// TSpellList's Flags.
static const struct {
	const char *Key;
	int Flag;
} SpellFlagKeys[] = {
	{"aggressive", SPELLFLAG_AGGRESSIVE},
	{"premium", SPELLFLAG_PREMIUM},
	{"factor_max_100", SPELLFLAG_FACTOR_MAX_100},
	{"factor_min_100", SPELLFLAG_FACTOR_MIN_100},
};

// The number of a syllable's text, or 0 for none.
static int GetSyllableByText(const std::string &Text) {
	for (int i = 1; i < MAX_SYLLABLES; i++) {
		if (SpellSyllable[i][0] != 0 && Text == SpellSyllable[i]) {
			return i;
		}
	}
	return 0;
}

// Fills Syllable from the words of a spell as the game writes them
// (GetSpellString): the class syllable and the second syllable as one word,
// each further syllable as a word of its own, separated by single spaces.
static void ReadWords(const toml::table &Entry, unsigned char *Syllable) {
	const toml::node *Node = Entry.get("words");
	if (!Node->is_string()) {
		TomlError(Node->source(), "words must be a string");
	}

	const std::string &Words = Node->as_string()->get();
	std::vector<std::string> Parts;
	size_t Start = 0;
	while (true) {
		size_t End = Words.find(' ', Start);
		Parts.push_back(Words.substr(Start, End == std::string::npos ? std::string::npos : End - Start));
		if (End == std::string::npos) {
			break;
		}
		Start = End + 1;
	}
	for (const std::string &Part : Parts) {
		if (Part.empty()) {
			TomlError(Node->source(), "words must be syllables separated by single spaces");
		}
	}

	// The class syllable (al, ad, ex, ut or om) is the first two letters.
	std::string Class = Parts[0].substr(0, 2);
	int ClassSyllable = GetSyllableByText(Class);
	if (ClassSyllable < SYLLABLE_AL || ClassSyllable > SYLLABLE_OM) {
		TomlError(Node->source(), "words must start with al, ad, ex, ut or om");
	}
	Parts[0] = Parts[0].substr(2);
	if (Parts[0].empty()) {
		if (Parts.size() > 1) {
			TomlError(Node->source(), "words must be syllables separated by single spaces");
		}
		Parts.clear();
	}
	if (1 + Parts.size() > static_cast<size_t>(MAX_SPELL_SYLLABLES)) {
		TomlError(Node->source(), "words have more than %d syllables", MAX_SPELL_SYLLABLES);
	}

	Syllable[0] = static_cast<unsigned char>(ClassSyllable);
	for (size_t i = 0; i < Parts.size(); i++) {
		int Number = GetSyllableByText(Parts[i]);
		if (Number == 0) {
			TomlError(Node->source(), "unknown syllable \"%s\" in words", Parts[i].c_str());
		}
		Syllable[i + 1] = static_cast<unsigned char>(Number);
	}
}

// Reads an integer of Min..Max under Key into Value, which stays 0 when there
// is none.
template<typename T>
static void ReadNumber(const toml::table &Table, const char *Key, T &Value, int Min, int Max) {
	int Number = 0;
	TomlGetNumber(Table, Key, Number, Min, Max);
	Value = static_cast<T>(Number);
}

// One [[spell]] of spells.toml. Defined notes the spell numbers filled so
// far.
static void ReadSpell(const toml::node &Node, bool *Defined) {
	if (!Node.is_table()) {
		TomlError(Node.source(), "an entry of spell must be a table");
	}

	const toml::table &Entry = *Node.as_table();
	TomlCheckKeys(Entry, "[[spell]]", {"number", "name", "words", "level", "mana", "soul_points", "flags", "rune"});
	for (const char *Key : {"number", "name", "words"}) {
		if (!Entry.contains(Key)) {
			TomlError(Node.source(), "an entry of spell needs %s", Key);
		}
	}

	int SpellNr = 0;
	TomlGetNumber(Entry, "number", SpellNr, 1, MAX_SPELLS - 1);
	if (Defined[SpellNr]) {
		TomlError(Entry.get("number")->source(), "spell %d is defined twice", SpellNr);
	}
	Defined[SpellNr] = true;

	TSpellList &Spell = SpellList[SpellNr];
	memset(&Spell, 0, sizeof(TSpellList));

	char Name[MAX_SPELL_NAME_LENGTH + 1];
	TomlGetString(Entry, "name", Name, sizeof(Name));
	Spell.Comment = AddStaticString(Name);

	ReadWords(Entry, Spell.Syllable);
	for (int Other = 1; Other < MAX_SPELLS; Other++) {
		if (Other != SpellNr && Defined[Other]
				&& memcmp(SpellList[Other].Syllable, Spell.Syllable, sizeof(Spell.Syllable)) == 0) {
			TomlError(Entry.get("words")->source(), "these words are already the ones of spell %d", Other);
		}
	}

	ReadNumber(Entry, "level", Spell.Level, 0, USHRT_MAX);
	ReadNumber(Entry, "mana", Spell.Mana, 0, INT_MAX);
	ReadNumber(Entry, "soul_points", Spell.SoulPoints, 0, INT_MAX);

	if (const toml::array *Flags = TomlGetArray(Entry, "flags")) {
		for (const toml::node &FlagNode : *Flags) {
			if (!FlagNode.is_string()) {
				TomlError(FlagNode.source(), "a flag must be a string");
			}
			const std::string &Key = FlagNode.as_string()->get();
			int Flag = 0;
			for (const auto &Known : SpellFlagKeys) {
				if (Key == Known.Key) {
					Flag = Known.Flag;
				}
			}
			if (Flag == 0) {
				TomlError(FlagNode.source(), "unknown flag \"%s\"", Key.c_str());
			}
			Spell.Flags = static_cast<unsigned short>(Spell.Flags | Flag);
		}
	}

	if (const toml::table *Rune = TomlGetTable(Entry, "rune")) {
		TomlCheckKeys(*Rune, "rune", {"group", "number", "magic_level", "charges"});
		for (const char *Key : {"group", "number"}) {
			if (!Rune->contains(Key)) {
				TomlError(Rune->source(), "rune needs %s", Key);
			}
		}
		ReadNumber(*Rune, "group", Spell.RuneGr, 1, UCHAR_MAX);
		ReadNumber(*Rune, "number", Spell.RuneNr, 0, UCHAR_MAX);
		ReadNumber(*Rune, "magic_level", Spell.RuneLevel, 0, USHRT_MAX);
		ReadNumber(*Rune, "charges", Spell.Amount, 0, INT_MAX);
	}
}

void LoadSpells(const char *FileName) {
	for (int i = 1; i < MAX_SPELLS; i++) {
		memset(&SpellList[i], 0, sizeof(TSpellList));
	}

	toml::table Spells = TomlParseFile(FileName);
	TomlCheckKeys(Spells, "the top level", {"spell"});

	bool Defined[MAX_SPELLS] = {};
	if (const toml::array *Entries = TomlGetArray(Spells, "spell")) {
		for (const toml::node &Node : *Entries) {
			ReadSpell(Node, Defined);
		}
	}
}

void InitSpells() {
	char FileName[4096];
	sprintf(FileName, "%s/spells.toml", DATAPATH);
	LoadSpells(FileName);
}

// The most rows circles.toml may have (and fields in a row): the size
// circles.dat allowed.
const int MAX_CIRCLE_ROWS = 21;

void LoadCircles(const char *FileName) {
	for (int i = 0; i < MAX_CIRCLES; i++) {
		memset(&Circle[i], 0, sizeof(TCircle));
	}

	toml::table File = TomlParseFile(FileName);
	TomlCheckKeys(File, "the top level", {"rows"});
	const toml::array *Rows = TomlGetArray(File, "rows");
	if (Rows == nullptr) {
		TomlError(File.source(), "rows is missing");
	}

	// A square of fields with the center in the middle: as many rows as
	// fields in a row, an odd number.
	int Size = static_cast<int>(Rows->size());
	if (Size % 2 == 0 || Size > MAX_CIRCLE_ROWS) {
		TomlError(Rows->source(), "rows must hold an odd number of rows, at most %d", MAX_CIRCLE_ROWS);
	}

	int Center = Size / 2;
	for (int y = 0; y < Size; y++) {
		const toml::node &RowNode = *Rows->get(static_cast<size_t>(y));
		if (!RowNode.is_string()) {
			TomlError(RowNode.source(), "a row must be a string");
		}

		// The fields, one character each, separated by single spaces.
		const std::string &Row = RowNode.as_string()->get();
		bool Square = static_cast<int>(Row.size()) == 2 * Size - 1;
		for (int i = 1; Square && i < 2 * Size - 1; i += 2) {
			Square = Row[static_cast<size_t>(i)] == ' ';
		}
		if (!Square) {
			TomlError(RowNode.source(), "row %d must be %d fields separated by single spaces, one for each row", y + 1,
					Size);
		}

		for (int x = 0; x < Size; x++) {
			char Field = Row[static_cast<size_t>(2 * x)];
			if (Field == '.') {
				continue;
			}
			if (Field < '0' || Field > '9') {
				TomlError(RowNode.source(), "row %d, field %d is neither a radius 0 to 9 nor \".\"", y + 1, x + 1);
			}

			int Radius = Field - '0';
			if (Circle[Radius].Count == MAX_CIRCLE_FIELDS) {
				TomlError(RowNode.source(), "row %d, field %d: circle %d has more than %d fields", y + 1, x + 1,
						Radius, MAX_CIRCLE_FIELDS);
			}
			Circle[Radius].x[Circle[Radius].Count] = x - Center;
			Circle[Radius].y[Circle[Radius].Count] = y - Center;
			Circle[Radius].Count++;
		}
	}
}

void InitCircles() {
	char FileName[4096];
	sprintf(FileName, "%s/circles.toml", DATAPATH);
	LoadCircles(FileName);
}
