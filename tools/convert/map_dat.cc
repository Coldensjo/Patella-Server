// map_dat.cc: the reader of the original map.dat (map_dat.hh). This is the
// game's ReadMapConfig as it was before map.toml, moved here unchanged but for
// these things: the file name is given instead of made from DATAPATH; an
// identifier longer than 29 characters ("identifier too long") and a mark
// name longer than 19 ("mark name too long") are refused where the old reader
// wrote past its buffers; a depot number outside 0 to 8 is refused ("depot
// number outside 0 to 8": map.toml cannot hold it, and LoadDepot refuses such
// a depot, so no player could ever open it); and the numbers of the depots
// set are noted for the conversion.
//
// Quirks of the format the reader keeps: the names before "=" are compared in
// lower case; an entry given twice counts the last time (a depot too), but a
// mark given twice is two marks; what is left out keeps its default (the
// sectors 1000 to 1015 on each axis and 0 to 15, one refreshed cylinder,
// Objects 1048576, CacheSize 655360), but for the start positions, which must
// be there; the depot numbers need not be consecutive.

#include <cstdio>
#include <cstring>
#include <set>

#include "script.hh"
#include "utils.hh"
#include "dbfuncs.hh"
#include "mapdata.hh"

#include "map_dat.hh"

void ReadMapDat(const char *FileName, TMapDatExtras &Extras) {
	SectorXMin = 1000;
	SectorXMax = 1015;
	SectorYMin = 1000;
	SectorYMax = 1015;
	SectorZMin = 0;
	SectorZMax = 15;
	RefreshedCylinders = 1;
	HashTableSize = 1048576;
	OBCount = 655360;
	Marks = 0;
	NewbieStartPositionX = 0;
	NewbieStartPositionY = 0;
	NewbieStartPositionZ = 0;
	VeteranStartPositionX = 0;
	VeteranStartPositionY = 0;
	VeteranStartPositionZ = 0;
	std::set<int> Depots;

	TReadScriptFile Script;
	Script.open(const_cast<char *>(FileName));
	while (true) {
		Script.nextToken();
		if (Script.Token == ENDOFFILE) {
			Script.close();
			break;
		}

		if (Script.Token != IDENTIFIER) {
			Script.error("Identifier expected");
		}

		char Ident[30];
		if (strlen(Script.getIdentifier()) >= sizeof(Ident)) {
			Script.error("identifier too long");
		}
		strcpy(Ident, Script.getIdentifier());
		Script.readSymbol('=');

		if (strcmp(Ident, "sectorxmin") == 0) {
			SectorXMin = Script.readNumber();
		}

		if (strcmp(Ident, "sectorxmax") == 0) {
			SectorXMax = Script.readNumber();
		}

		if (strcmp(Ident, "sectorymin") == 0) {
			SectorYMin = Script.readNumber();
		}

		if (strcmp(Ident, "sectorymax") == 0) {
			SectorYMax = Script.readNumber();
		}

		if (strcmp(Ident, "sectorzmin") == 0) {
			SectorZMin = Script.readNumber();
		}

		if (strcmp(Ident, "sectorzmax") == 0) {
			SectorZMax = Script.readNumber();
		}

		if (strcmp(Ident, "refreshedcylinders") == 0) {
			RefreshedCylinders = Script.readNumber();
		}

		if (strcmp(Ident, "objects") == 0) {
			HashTableSize = Script.readNumber();
		}

		if (strcmp(Ident, "cachesize") == 0) {
			OBCount = Script.readNumber();
		}

		if (strcmp(Ident, "depot") == 0) {
			Script.readSymbol('(');
			int DepotNumber = Script.readNumber();
			if (DepotNumber < 0 || DepotNumber > MAX_DEPOTS - 1) {
				Script.error("depot number outside 0 to 8");
			}
			Depots.insert(DepotNumber);
			Script.readSymbol(',');
			Script.nextToken();
			if (Script.Token != STRING) {
				Script.error("town name expected");
			}

			if (strlen(Script.getString()) > 19) {
				Script.error("town name too long");
			}

			strcpy(DepotInfo(DepotNumber).Town, Script.getString());
			Script.readSymbol(',');
			DepotInfo(DepotNumber).Size = Script.readNumber();
			Script.readSymbol(')');
		}

		if (strcmp(Ident, "mark") == 0) {
			Script.readSymbol('(');
			if (strlen(Script.readString()) >= sizeof(Mark(Marks).Name)) {
				Script.error("mark name too long");
			}
			strcpy(Mark(Marks).Name, Script.getString());
			Script.readSymbol(',');
			Script.readCoordinate(Mark(Marks).x, Mark(Marks).y, Mark(Marks).z);
			Script.readSymbol(')');
			Marks++;
		}

		if (strcmp(Ident, "newbiestart") == 0) {
			Script.readCoordinate(NewbieStartPositionX, NewbieStartPositionY, NewbieStartPositionZ);
		}

		if (strcmp(Ident, "veteranstart") == 0) {
			Script.readCoordinate(VeteranStartPositionX, VeteranStartPositionY, VeteranStartPositionZ);
		}
	}

	Extras.Depots.assign(Depots.begin(), Depots.end());

	if (SectorXMin <= 0) {
		throw "illegal value for SectorXMin";
	}

	if (SectorXMax >= 2047) {
		throw "illegal value for SectorXMax";
	}

	if (SectorYMin <= 0) {
		throw "illegal value for SectorYMin";
	}

	if (SectorYMax >= 2047) {
		throw "illegal value for SectorYMax";
	}

	if (SectorZMin < 0) {
		throw "illegal value for SectorZMin";
	}

	if (SectorZMax > 15) {
		throw "illegal value for SectorZMax";
	}

	if (SectorXMin > SectorXMax) {
		throw "SectorXMin is greater than SectorXMax";
	}

	if (SectorYMin > SectorYMax) {
		throw "SectorYMin is greater than SectorYMax";
	}

	if (SectorZMin > SectorZMax) {
		throw "SectorZMin is greater than SectorZMax";
	}

	if (OBCount % OBJECTS_PER_BLOCK != 0) {
		throw "CacheSize must be a multiple of 32768";
	}

	if (OBCount <= 0) {
		throw "illegal value for CacheSize";
	}

	OBCount /= OBJECTS_PER_BLOCK;
	if (HashTableSize <= 0) {
		throw "illegal value for Objects";
	}

	if ((HashTableSize & (HashTableSize - 1)) != 0) {
		throw "Objects must be a power of 2";
	}

	if (NewbieStartPositionX == 0) {
		throw "no start position for newbies specified";
	}

	if (VeteranStartPositionX == 0) {
		throw "no start position for veterans specified";
	}
}
