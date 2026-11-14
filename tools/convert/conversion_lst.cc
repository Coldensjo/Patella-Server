// conversion_lst.cc: the reader of the original conversion.lst
// (conversion_lst.hh). This is the game's LoadConversionList as it was before
// conversion.toml, moved here unchanged but for these things: the file name
// is given instead of made from DATAPATH; the log line for a file that cannot
// be opened gets the file name its text asks for; an entry that is not three
// numbers is refused, where the old reader went on forever (the stream
// failed, so the next group never came and the end was never reached) or,
// for a last entry cut short, took the type of the entry before; and a group
// and number whose place Group * 256 + Number lies outside NewType are
// refused, where the old reader wrote past it.
//
// Quirks of the format the reader keeps: the file is whitespace-separated
// numbers, three to an entry (group, number, type), with no comments; an
// entry given twice counts the last time; a number above 255 runs into the
// next group (1/256 is 2/0); the game's own entries (0/0 to 0/10, 250/0) are
// set after the file and so win over it.

#include <cstdint>
#include <cstdio>
#include <fstream>

#include "objects.hh"
#include "utils.hh"

#include "conversion_lst.hh"

void ReadConversionLst(const char *FileName) {
	std::ifstream IN;
	int Group, Number, Type;

	for (int i = 0; i < 65536; i++) {
		NewType[i] = -1;
	}

	IN.open(FileName, std::ios::in);
	if (IN.fail()) {
		error("LoadConversionList: Cannot open file %s.\n", FileName);
		throw "Cannot open conversion.lst";
	}

	while (true) {
		IN >> Group;
		if (IN.eof()) {
			break;
		}
		IN >> Number >> Type;
		if (IN.fail()) {
			throw "an entry that is not three numbers, where the old reader went on forever or took the type before";
		}

		if (Type < 1 || Type > CONVERSION_TYPES - 1) {
			error("LoadConversionList: Invalid type %d.\n", Type);
			throw "Error while loading conversion.lst";
		}

		int64_t Place = static_cast<int64_t>(Group) * 256 + Number;
		if (Place < 0 || Place > 65535) {
			throw "a group and number outside the table, where the old reader wrote past NewType";
		}
		NewType[Group * 256 + Number] = Type;
	}

	IN.close();

	// No type (0) and the body containers keep their numbers, in group 0.
	for (int i = 0; i <= LAST_BODY_POSITION; i++) {
		NewType[i] = i;
	}

	NewType[250 * 256 + 0] = TYPEID_CREATURE;
}
