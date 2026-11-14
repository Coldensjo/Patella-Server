// objects_srv.cc: the reader of the original objects.srv (objects_srv.hh).
// This is the game's LoadObjects as it was before items.toml, moved here
// unchanged but for two things: the file name is given instead of made from
// DATAPATH, and the types and the order of their attributes are noted for the
// conversion.

#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#include "script.hh"
#include "objects.hh"
#include "strings.hh"
#include "utils.hh"

#include "objects_srv.hh"

void ReadObjectsSrv(const char *FileName, TObjectsSrvExtras &Extras) {
	TReadScriptFile Script;
	Script.open(const_cast<char *>(FileName));

	int TypeID = -1;
	while (true) {
		Script.nextToken();
		if (Script.Token == ENDOFFILE) {
			break;
		}

		if (Script.Token != IDENTIFIER) {
			Script.error("Identifier expected");
		}

		char Ident[30];
		strcpy(Ident, Script.getIdentifier());
		Script.readSymbol('=');

		if (strcmp(Ident, "typeid") == 0) {
			TypeID = Script.readNumber();
			ObjectTypes(TypeID).Name = nullptr;
			ObjectTypes(TypeID).Description = nullptr;
			memset(ObjectTypes(TypeID).Flags, 0, sizeof(ObjectTypes(TypeID).Flags));
			for (int i = 0; i < TYPEATTRIBUTES; i++) {
				ObjectTypes(TypeID).Attributes[i] = 0;
			}
			for (int i = 0; i < INSTANCEATTRIBUTES; i++) {
				ObjectTypes(TypeID).AttributeOffsets[i] = -1;
			}
			if (TypeID >= 0) {
				Extras.Types[TypeID].clear();
			}
		}

		if (TypeID < 0) {
			Script.error("Invalid type id");
		}

		if (strcmp(Ident, "name") == 0) {
			ObjectTypes(TypeID).Name = AddStaticString(Script.readString());
		}

		if (strcmp(Ident, "description") == 0) {
			ObjectTypes(TypeID).Description = AddStaticString(Script.readString());
		}

		if (strcmp(Ident, "flags") == 0) {
			Script.readSymbol('{');
			while (true) {
				Script.nextToken();
				if (Script.Token == SPECIAL && Script.getSpecial() == '}') {
					break;
				}
				if (Script.Token == SPECIAL && Script.getSpecial() == ',') {
					continue;
				}

				int Flag = GetFlagByName(Script.getIdentifier());
				if (Flag == -1) {
					Script.error("Unknown flag");
				}
				ObjectTypes(TypeID).Flags[Flag / 8] |= 1 << (Flag % 8);
			}

			for (int Offset = 0, i = 0; i < INSTANCEATTRIBUTES; i++) {
				if (ObjectType(TypeID).getFlag(InstanceAttributeFlags[i])
						|| (i == CONTENT && ObjectType(TypeID).getFlag(CHEST))) {
					ObjectTypes(TypeID).AttributeOffsets[i] = Offset;
					Offset++;
				}
			}
		}

		if (strcmp(Ident, "attributes") == 0) {
			Script.readSymbol('{');
			while (true) {
				Script.nextToken();
				if (Script.Token == SPECIAL && Script.getSpecial() == '}') {
					break;
				}
				if (Script.Token == SPECIAL && Script.getSpecial() == ',') {
					continue;
				}

				int Attribute = GetTypeAttributeByName(Script.getIdentifier());
				if (Attribute == -1) {
					Script.error("Unknown attribute");
				}
				Script.readSymbol('=');
				ObjectTypes(TypeID).Attributes[Attribute] = Script.readNumber();
				std::vector<int> &Order = Extras.Types[TypeID];
				bool Noted = false;
				for (int Written : Order) {
					Noted = Noted || Written == Attribute;
				}
				if (!Noted) {
					Order.push_back(Attribute);
				}

				if (Attribute == MEANING) {
					int Meaning = ObjectTypes(TypeID).Attributes[MEANING];
					// Quirk: 49 passes here and in GetSpecialObject, one past
					// the last element of SpecialObjects.
					if (!in(Meaning, 1, SPECIALMEANINGS)) {
						Script.error("Unknown meaning");
					}
					if (SpecialObjects[Meaning] != ObjectType(0)) {
						Script.error("Special object already defined");
					}
					SpecialObjects[Meaning] = ObjectType(TypeID);
				}
			}
		}
	}

	Script.close();
}
