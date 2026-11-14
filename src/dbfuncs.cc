#include <climits>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <new>
#include <unistd.h>

#include "objects.hh"
#include "script.hh"
#include "threads.hh"
#include "map.hh"
#include "cr.hh"
#include "dbfuncs.hh"
#include "config.hh"
#include "utils.hh"

void TOutfit::load(TReadScriptFile &Script) {
	Script.readSymbol('(');
	OutfitID = Script.readNumber();
	Script.readSymbol(',');
	if (OutfitID == 0) {
		ObjectType = (unsigned short)Script.readNumber();
	} else {
		memcpy(Colors, Script.readBytesequence(), sizeof(Colors));
	}
	Script.readSymbol(')');
}

void TOutfit::save(TWriteScriptFile &Script) {
	Script.writeText("(");
	Script.writeNumber(OutfitID);
	Script.writeText(",");
	if (OutfitID == 0) {
		Script.writeNumber(ObjectType);
	} else {
		Script.writeBytesequence(Colors, 4);
	}
	Script.writeText(")");
}

void PlayerDataPath(uint32_t CharacterID, char *Extension, char *Path) {
	// The original passes the unsigned 32-bit ID to %ld, which prints it as a signed 32-bit number.
	sprintf(Path, "%s/%02ld/%ld.%s", USERPATH, (long)(CharacterID % 100), (long)(int32_t)CharacterID, Extension);
}

bool PlayerDataExists(uint32_t CharacterID) {
	char FileName[4096];

	PlayerDataPath(CharacterID, "usr", FileName);
	return FileExists(FileName);
}

bool LoadPlayerData(TPlayerData *Slot) {
	if (Slot == nullptr) {
		error("LoadPlayerData: Slot ist NULL.\n");
		return false;
	}

	if (Slot->CharacterID == 0) {
		error("LoadPlayerData: Slot enth\xe4lt keinen Charakter.\n");
		return false;
	}

	char FileName[4096];
	PlayerDataPath(Slot->CharacterID, "usr", FileName);
	if (!FileExists(FileName)) {
		Slot->Race = 1;
		Slot->Profession = PROFESSION_NONE;
		memset(&Slot->OriginalOutfit, 0, sizeof(Slot->OriginalOutfit));
		memset(&Slot->CurrentOutfit, 0, sizeof(Slot->CurrentOutfit));
		Slot->LastLoginTime = 0;
		Slot->LastLogoutTime = 0;
		Slot->startx = 0;
		Slot->starty = 0;
		Slot->startz = 0;
		Slot->posx = 0;
		Slot->posy = 0;
		Slot->posz = 0;
		Slot->PlayerkillerEnd = 0;
		for (int SkillNr = 0; SkillNr < MAX_SKILLS; SkillNr++) {
			Slot->Minimum[SkillNr] = INT_MIN;
		}
		memset(Slot->SpellList, 0, sizeof(Slot->SpellList));
		memset(Slot->QuestValues, 0, sizeof(Slot->QuestValues));
		memset(Slot->MurderTimestamps, 0, sizeof(Slot->MurderTimestamps));
		Slot->Inventory = nullptr;
		Slot->InventorySize = 0;
		for (int DepotNr = 0; DepotNr < MAX_DEPOTS; DepotNr++) {
			Slot->Depot[DepotNr] = nullptr;
			Slot->DepotSize[DepotNr] = 0;
		}
		return true;
	}

	bool FileOk = false;
	TDynamicWriteBuffer HelpBuffer(16384);
	try {
		TReadScriptFile Script;

		Script.open(FileName);

		Script.readIdentifier();
		Script.readSymbol('=');
		Script.readNumber();

		Script.readIdentifier();
		Script.readSymbol('=');
		strcpy(Slot->Name, Script.readString());

		Script.readIdentifier();
		Script.readSymbol('=');
		Slot->Race = Script.readNumber();

		Script.readIdentifier();
		Script.readSymbol('=');
		Slot->Profession = Script.readNumber();

		Script.readIdentifier();
		Script.readSymbol('=');
		Slot->OriginalOutfit.load(Script);

		Script.readIdentifier();
		Script.readSymbol('=');
		Slot->CurrentOutfit.load(Script);

		Script.readIdentifier();
		Script.readSymbol('=');
		Slot->LastLoginTime = Script.readNumber();

		Script.readIdentifier();
		Script.readSymbol('=');
		Slot->LastLogoutTime = Script.readNumber();

		Script.readIdentifier();
		Script.readSymbol('=');
		Script.readCoordinate(Slot->startx, Slot->starty, Slot->startz);

		Script.readIdentifier();
		Script.readSymbol('=');
		Script.readCoordinate(Slot->posx, Slot->posy, Slot->posz);

		Script.readIdentifier();
		Script.readSymbol('=');
		Slot->PlayerkillerEnd = Script.readNumber();

		for (int SkillNr = 0; SkillNr < MAX_SKILLS; SkillNr++) {
			Slot->Minimum[SkillNr] = INT_MIN;
		}

		while (true) {
			Script.readIdentifier();
			if (strcmp(Script.getIdentifier(), "skill") != 0) {
				break;
			}

			Script.readSymbol('=');
			Script.readSymbol('(');
			int SkillNr = Script.readNumber();
			if (!in(SkillNr, 0, MAX_SKILLS - 1)) {
				Script.error("illegal skill number");
			}
			Script.readSymbol(',');
			Slot->Actual[SkillNr] = Script.readNumber();
			Script.readSymbol(',');
			Slot->Maximum[SkillNr] = Script.readNumber();
			Script.readSymbol(',');
			Slot->Minimum[SkillNr] = Script.readNumber();
			Script.readSymbol(',');
			Slot->DeltaAct[SkillNr] = Script.readNumber();
			Script.readSymbol(',');
			Slot->MagicDeltaAct[SkillNr] = Script.readNumber();
			Script.readSymbol(',');
			Slot->Cycle[SkillNr] = Script.readNumber();
			Script.readSymbol(',');
			Slot->MaxCycle[SkillNr] = Script.readNumber();
			Script.readSymbol(',');
			Slot->Count[SkillNr] = Script.readNumber();
			Script.readSymbol(',');
			Slot->MaxCount[SkillNr] = Script.readNumber();
			Script.readSymbol(',');
			Slot->AddLevel[SkillNr] = Script.readNumber();
			Script.readSymbol(',');
			Slot->Experience[SkillNr] = Script.readNumber();
			Script.readSymbol(',');
			Slot->FactorPercent[SkillNr] = Script.readNumber();
			Script.readSymbol(',');
			Slot->NextLevel[SkillNr] = Script.readNumber();
			Script.readSymbol(',');
			Slot->Delta[SkillNr] = Script.readNumber();
			Script.readSymbol(')');
		}

		Script.readSymbol('=');
		Script.readSymbol('{');
		while (true) {
			Script.nextToken();
			if (Script.Token == SPECIAL && Script.getSpecial() == '}') {
				break;
			}
			if (Script.Token == SPECIAL && Script.getSpecial() == ',') {
				continue;
			}

			int SpellNr = Script.getNumber();
			if (!in(SpellNr, 0, MAX_SPELLS - 1)) {
				Script.error("illegal spell number");
			}
			Slot->SpellList[SpellNr] = 1;
		}

		Script.readIdentifier();
		Script.readSymbol('=');
		Script.readSymbol('{');
		while (true) {
			Script.nextToken();
			if (Script.Token == SPECIAL && Script.getSpecial() == '}') {
				break;
			}
			if (Script.Token == SPECIAL && Script.getSpecial() == ',') {
				continue;
			}

			if (Script.Token != SPECIAL || Script.getSpecial() != '(') {
				Script.error("'(' expected");
			}
			int Number = Script.readNumber();
			if (!in(Number, 0, MAX_QUESTS - 1)) {
				Script.error("illegal quest number");
			}
			Script.readSymbol(',');
			Slot->QuestValues[Number] = Script.readNumber();
			Script.readSymbol(')');
		}

		Script.readIdentifier();
		Script.readSymbol('=');
		Script.readSymbol('{');
		while (true) {
			Script.nextToken();
			if (Script.Token == SPECIAL && Script.getSpecial() == '}') {
				break;
			}
			if (Script.Token == SPECIAL && Script.getSpecial() == ',') {
				continue;
			}

			for (int i = 1; i < 20; i++) {
				Slot->MurderTimestamps[i - 1] = Slot->MurderTimestamps[i];
			}
			Slot->MurderTimestamps[19] = Script.getNumber();
		}

		Script.readIdentifier();
		Script.readSymbol('=');
		Script.readSymbol('{');
		HelpBuffer.rewind();
		while (true) {
			Script.nextToken();
			if (Script.Token == SPECIAL && Script.getSpecial() == '}') {
				break;
			}
			if (Script.Token == SPECIAL && Script.getSpecial() == ',') {
				continue;
			}

			int Position = Script.getNumber();
			HelpBuffer.writeByte((unsigned char)Position);
			Script.readIdentifier();
			Script.readSymbol('=');
			LoadObjects(Script, HelpBuffer, false);
		}
		HelpBuffer.writeByte(0xFF);
		Slot->InventorySize = HelpBuffer.getDataSize();
		Slot->Inventory = new unsigned char[Slot->InventorySize];
		memcpy(Slot->Inventory, HelpBuffer.getData(), Slot->InventorySize);

		Script.readIdentifier();
		Script.readSymbol('=');
		Script.readSymbol('{');
		for (int DepotNr = 0; DepotNr < MAX_DEPOTS; DepotNr++) {
			Slot->Depot[DepotNr] = nullptr;
		}
		while (true) {
			Script.nextToken();
			if (Script.Token == SPECIAL && Script.getSpecial() == '}') {
				break;
			}
			if (Script.Token == SPECIAL && Script.getSpecial() == ',') {
				continue;
			}

			int DepotNr = Script.getNumber();
			Script.readIdentifier();
			Script.readSymbol('=');
			HelpBuffer.rewind();
			LoadObjects(Script, HelpBuffer, false);
			Slot->DepotSize[DepotNr] = HelpBuffer.getDataSize();
			Slot->Depot[DepotNr] = new unsigned char[Slot->DepotSize[DepotNr]];
			memcpy(Slot->Depot[DepotNr], HelpBuffer.getData(), Slot->DepotSize[DepotNr]);
		}

		Script.nextToken();
		if (Script.Token != ENDOFFILE) {
			Script.error("end of file expected");
		}

		Script.close();
		FileOk = true;
	} catch (const char *s) {
		error("LoadPlayerData: Kann Gegenst\xe4nde des Spielers %ld nicht laden.\n", (long)(int32_t)Slot->CharacterID);
		error("# Fehler: %s\n", s);
	} catch (const std::bad_alloc &) {
		error("LoadPlayerData: Kein Speicher frei beim Laden von Spieler %ld.\n", (long)(int32_t)Slot->CharacterID);
	}

	if (!FileOk) {
		if (Slot->Inventory != nullptr) {
			delete[] Slot->Inventory;
			Slot->Inventory = nullptr;
		}
		Slot->InventorySize = 0;
		for (int DepotNr = 0; DepotNr < MAX_DEPOTS; DepotNr++) {
			if (Slot->Depot[DepotNr] != nullptr) {
				delete[] Slot->Depot[DepotNr];
				Slot->Depot[DepotNr] = nullptr;
			}
			Slot->DepotSize[DepotNr] = 0;
		}
	}

	// The original returns true here even when the file could not be read.
	return true;
}

void SavePlayerData(TPlayerData *Slot) {
	if (Slot == nullptr) {
		error("SavePlayerData: Slot ist NULL.\n");
		return;
	}

	if (Slot->CharacterID == 0) {
		error("SavePlayerData: Slot enth\xe4lt keinen Charakter.\n");
		return;
	}

	char FileName[4096];
	PlayerDataPath(Slot->CharacterID, "usr", FileName);

	try {
		TWriteScriptFile Script;

		Script.open(FileName);

		Script.writeText("ID              = ");
		Script.writeNumber((int)Slot->CharacterID);
		Script.writeLn();

		Script.writeText("Name            = ");
		Script.writeString(Slot->Name);
		Script.writeLn();

		Script.writeText("Race            = ");
		Script.writeNumber(Slot->Race);
		Script.writeLn();

		Script.writeText("Profession      = ");
		Script.writeNumber(Slot->Profession);
		Script.writeLn();

		Script.writeText("OriginalOutfit  = ");
		Slot->OriginalOutfit.save(Script);
		Script.writeLn();

		Script.writeText("CurrentOutfit   = ");
		Slot->CurrentOutfit.save(Script);
		Script.writeLn();

		Script.writeText("LastLogin       = ");
		Script.writeNumber((int)Slot->LastLoginTime);
		Script.writeLn();

		Script.writeText("LastLogout      = ");
		Script.writeNumber((int)Slot->LastLogoutTime);
		Script.writeLn();

		Script.writeText("StartPosition   = ");
		Script.writeCoordinate(Slot->startx, Slot->starty, Slot->startz);
		Script.writeLn();

		Script.writeText("CurrentPosition = ");
		Script.writeCoordinate(Slot->posx, Slot->posy, Slot->posz);
		Script.writeLn();

		Script.writeText("PlayerkillerEnd = ");
		Script.writeNumber(Slot->PlayerkillerEnd);
		Script.writeLn();

		Script.writeLn();
		for (int SkillNr = 0; SkillNr < MAX_SKILLS; SkillNr++) {
			if (Slot->Minimum[SkillNr] != INT_MIN) {
				Script.writeText("Skill = (");
				Script.writeNumber(SkillNr);
				Script.writeText(",");
				Script.writeNumber(Slot->Actual[SkillNr]);
				Script.writeText(",");
				Script.writeNumber(Slot->Maximum[SkillNr]);
				Script.writeText(",");
				Script.writeNumber(Slot->Minimum[SkillNr]);
				Script.writeText(",");
				Script.writeNumber(Slot->DeltaAct[SkillNr]);
				Script.writeText(",");
				Script.writeNumber(Slot->MagicDeltaAct[SkillNr]);
				Script.writeText(",");
				Script.writeNumber(Slot->Cycle[SkillNr]);
				Script.writeText(",");
				Script.writeNumber(Slot->MaxCycle[SkillNr]);
				Script.writeText(",");
				Script.writeNumber(Slot->Count[SkillNr]);
				Script.writeText(",");
				Script.writeNumber(Slot->MaxCount[SkillNr]);
				Script.writeText(",");
				Script.writeNumber(Slot->AddLevel[SkillNr]);
				Script.writeText(",");
				Script.writeNumber(Slot->Experience[SkillNr]);
				Script.writeText(",");
				Script.writeNumber(Slot->FactorPercent[SkillNr]);
				Script.writeText(",");
				Script.writeNumber(Slot->NextLevel[SkillNr]);
				Script.writeText(",");
				Script.writeNumber(Slot->Delta[SkillNr]);
				Script.writeText(")");
				Script.writeLn();
			}
		}

		Script.writeLn();
		Script.writeText("Spells      = {");
		bool FirstSpell = true;
		for (int SpellNr = 0; SpellNr < MAX_SPELLS; SpellNr++) {
			if (Slot->SpellList[SpellNr] != 0) {
				if (!FirstSpell) {
					Script.writeText(",");
				}
				Script.writeNumber(SpellNr);
				FirstSpell = false;
			}
		}
		Script.writeText("}");
		Script.writeLn();

		Script.writeText("QuestValues = {");
		bool FirstQuestValue = true;
		for (int Number = 0; Number < MAX_QUESTS; Number++) {
			if (Slot->QuestValues[Number] != 0) {
				if (!FirstQuestValue) {
					Script.writeText(",");
				}
				Script.writeText("(");
				Script.writeNumber(Number);
				Script.writeText(",");
				Script.writeNumber(Slot->QuestValues[Number]);
				Script.writeText(")");
				FirstQuestValue = false;
			}
		}
		Script.writeText("}");
		Script.writeLn();

		Script.writeText("Murders     = {");
		bool FirstMurder = true;
		int Limitation = (int)time(nullptr) - 2592000;
		for (int i = 0; i < 20; i++) {
			if (Slot->MurderTimestamps[i] > Limitation) {
				if (!FirstMurder) {
					Script.writeText(",");
				}
				Script.writeNumber(Slot->MurderTimestamps[i]);
				FirstMurder = false;
			}
		}
		Script.writeText("}");
		Script.writeLn();

		Script.writeLn();
		Script.writeText("Inventory   = {");
		if (Slot->Inventory != nullptr) {
			TReadBuffer Buffer(Slot->Inventory, Slot->InventorySize);
			bool FirstPosition = true;
			while (true) {
				unsigned char Position = Buffer.readByte();
				if (Position == 0xFF) {
					break;
				}

				if (FirstPosition) {
					FirstPosition = false;
				} else {
					Script.writeText(",");
					Script.writeLn();
					Script.writeText("               ");
				}
				Script.writeNumber(Position);
				Script.writeText(" Content=");
				SaveObjects(Buffer, Script);
			}
		}
		Script.writeText("}");
		Script.writeLn();

		Script.writeLn();
		Script.writeText("Depots      = {");
		bool FirstDepot = true;
		for (int DepotNr = 0; DepotNr < MAX_DEPOTS; DepotNr++) {
			if (Slot->Depot[DepotNr] != nullptr) {
				if (FirstDepot) {
					FirstDepot = false;
				} else {
					Script.writeText(", ");
					Script.writeLn();
					Script.writeText("               ");
				}
				Script.writeNumber(DepotNr);
				Script.writeText(" Content=");
				TReadBuffer Buffer(Slot->Depot[DepotNr], Slot->DepotSize[DepotNr]);
				SaveObjects(Buffer, Script);
			}
		}
		Script.writeText("}");
		Script.writeLn();

		Script.close();
	} catch (const char *s) {
		error("SavePlayerData: Kann Gegenst\xe4nde des Spielers %ld nicht schreiben.\n", (long)(int32_t)Slot->CharacterID);
		error("# Fehler: %s\n", s);
		unlink(FileName);
	}
}
