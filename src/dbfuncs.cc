// dbfuncs.cc: a player's data (TPlayerData): loading it from the query manager
// and saving it there (loadPlayerData and savePlayerData, query.cc), as the
// record those requests carry, with the objects as item text; the files that
// keep a record the query manager did not take; and the outfit's text form
// (TOutfit). The game once kept this data in its player files,
// usr/NN/<id>.usr; their reader and writer are in tools/convert now.

#include <atomic>
#include <cctype>
#include <cerrno>
#include <climits>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <dirent.h>
#include <fcntl.h>
#include <new>
#include <string>
#include <unistd.h>

#include "objects.hh"
#include "script.hh"
#include "threads.hh"
#include "map.hh"
#include "cr.hh"
#include "dbfuncs.hh"
#include "config.hh"
#include "query.hh"
#include "utils.hh"
#include "writer.hh"

// The connections LoadPlayerData and SavePlayerData send their requests on.
// They are not the connections the other requests use: the communication
// threads hold one of their pool while they load a player (a second from
// that pool could wait for ever), the reader thread has none, the game
// thread may save while it holds the player data pool's mutex, and the
// other connections' buffers (16 KiB) are too small for a large depot. So
// the player data has a pool of its own, which every thread takes one
// connection from for one request at a time.
static const int PLAYERDATA_CONNECTIONS = 4;
static const int PLAYERDATA_BUFFER_SIZE = 4 * 1024 * 1024;
static TQueryManagerConnectionPool PlayerDataConnections(PLAYERDATA_CONNECTIONS, PLAYERDATA_BUFFER_SIZE);

// The largest item text ReadPlayerData accepts; a connection's buffer could
// not hold a longer one anyway.
static const uint32_t MAX_ITEM_TEXT = PLAYERDATA_BUFFER_SIZE;

// The times SavePlayerData keeps of the murders: those of the last 30 days.
static const int MURDER_MEMORY = 2592000;

// A query manager that takes the connection but does not answer (frozen,
// overloaded) costs every request the 30 s of its timeout, and the saves of
// all dirty slots one after the other (the writer's periodic save,
// ExitPlayerPool at shutdown) would take hours. So once a save has failed
// after SAVE_HANG_TIME seconds or more, SavePlayerData keeps the records in
// their files at once for SAVE_PAUSE seconds, without asking the query
// manager; the character's next load or the next start sends them. A query
// manager that is down refuses the connection at once and causes no pause.
static const int SAVE_HANG_TIME = 5;
static const int SAVE_PAUSE = 60;
static std::atomic<long> SavePausedUntil(0);

static bool SavesPaused() {
	return (long)time(nullptr) < SavePausedUntil.load();
}

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

static void WriteOutfit(TWriteStream &Stream, TOutfit &Outfit) {
	Stream.writeQuad((uint32_t)Outfit.OutfitID);
	if (Outfit.OutfitID == 0) {
		Stream.writeWord(Outfit.ObjectType);
	} else {
		Stream.writeBytes(Outfit.Colors, 4);
	}
}

static void ReadOutfit(TReadStream &Stream, TOutfit &Outfit) {
	Outfit.OutfitID = (int)Stream.readQuad();
	if (Outfit.OutfitID == 0) {
		Outfit.ObjectType = Stream.readWord();
	} else {
		Stream.readBytes(Outfit.Colors, 4);
	}
}

// The item text of an object stream: "{...}", as SaveObjects writes it.
static void WriteItemText(TWriteStream &Stream, unsigned char *Data, int Size) {
	TWriteScriptFile Script;
	Script.openText();
	TReadBuffer Buffer(Data, Size);
	SaveObjects(Buffer, Script);
	Script.close();
	Stream.writeString(const_cast<char *>(Script.getText()));
}

void WritePlayerData(TWriteStream &Stream, TPlayerData *Slot, int MurderLimitation) {
	Stream.writeString(Slot->Name);
	Stream.writeQuad((uint32_t)Slot->Race);
	Stream.writeQuad((uint32_t)Slot->Profession);
	WriteOutfit(Stream, Slot->OriginalOutfit);
	WriteOutfit(Stream, Slot->CurrentOutfit);
	// The player file wrote the times as int.
	Stream.writeQuad((uint32_t)(int)Slot->LastLoginTime);
	Stream.writeQuad((uint32_t)(int)Slot->LastLogoutTime);
	Stream.writeQuad((uint32_t)Slot->startx);
	Stream.writeQuad((uint32_t)Slot->starty);
	Stream.writeQuad((uint32_t)Slot->startz);
	Stream.writeQuad((uint32_t)Slot->posx);
	Stream.writeQuad((uint32_t)Slot->posy);
	Stream.writeQuad((uint32_t)Slot->posz);
	Stream.writeQuad((uint32_t)Slot->PlayerkillerEnd);

	// A skill whose Minimum is INT_MIN was never set and is left out.
	int Skills = 0;
	for (int SkillNr = 0; SkillNr < MAX_SKILLS; SkillNr++) {
		if (Slot->Minimum[SkillNr] != INT_MIN) {
			Skills++;
		}
	}
	Stream.writeWord((unsigned short)Skills);
	for (int SkillNr = 0; SkillNr < MAX_SKILLS; SkillNr++) {
		if (Slot->Minimum[SkillNr] != INT_MIN) {
			Stream.writeByte((unsigned char)SkillNr);
			Stream.writeQuad((uint32_t)Slot->Actual[SkillNr]);
			Stream.writeQuad((uint32_t)Slot->Maximum[SkillNr]);
			Stream.writeQuad((uint32_t)Slot->Minimum[SkillNr]);
			Stream.writeQuad((uint32_t)Slot->DeltaAct[SkillNr]);
			Stream.writeQuad((uint32_t)Slot->MagicDeltaAct[SkillNr]);
			Stream.writeQuad((uint32_t)Slot->Cycle[SkillNr]);
			Stream.writeQuad((uint32_t)Slot->MaxCycle[SkillNr]);
			Stream.writeQuad((uint32_t)Slot->Count[SkillNr]);
			Stream.writeQuad((uint32_t)Slot->MaxCount[SkillNr]);
			Stream.writeQuad((uint32_t)Slot->AddLevel[SkillNr]);
			Stream.writeQuad((uint32_t)Slot->Experience[SkillNr]);
			Stream.writeQuad((uint32_t)Slot->FactorPercent[SkillNr]);
			Stream.writeQuad((uint32_t)Slot->NextLevel[SkillNr]);
			Stream.writeQuad((uint32_t)Slot->Delta[SkillNr]);
		}
	}

	int Spells = 0;
	for (int SpellNr = 0; SpellNr < MAX_SPELLS; SpellNr++) {
		if (Slot->SpellList[SpellNr] != 0) {
			Spells++;
		}
	}
	Stream.writeWord((unsigned short)Spells);
	for (int SpellNr = 0; SpellNr < MAX_SPELLS; SpellNr++) {
		if (Slot->SpellList[SpellNr] != 0) {
			Stream.writeByte((unsigned char)SpellNr);
		}
	}

	int QuestValues = 0;
	for (int Number = 0; Number < MAX_QUESTS; Number++) {
		if (Slot->QuestValues[Number] != 0) {
			QuestValues++;
		}
	}
	Stream.writeWord((unsigned short)QuestValues);
	for (int Number = 0; Number < MAX_QUESTS; Number++) {
		if (Slot->QuestValues[Number] != 0) {
			Stream.writeWord((unsigned short)Number);
			Stream.writeQuad((uint32_t)Slot->QuestValues[Number]);
		}
	}

	int Murders = 0;
	for (int i = 0; i < 20; i++) {
		if (Slot->MurderTimestamps[i] > MurderLimitation) {
			Murders++;
		}
	}
	Stream.writeWord((unsigned short)Murders);
	for (int i = 0; i < 20; i++) {
		if (Slot->MurderTimestamps[i] > MurderLimitation) {
			Stream.writeQuad((uint32_t)Slot->MurderTimestamps[i]);
		}
	}

	// The inventory as the player file had it: each occupied body position
	// and its objects, with the file's line breaks between them.
	TWriteScriptFile Script;
	Script.openText();
	Script.writeText("{");
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
	Script.close();
	Stream.writeString(const_cast<char *>(Script.getText()));

	int Depots = 0;
	for (int DepotNr = 0; DepotNr < MAX_DEPOTS; DepotNr++) {
		if (Slot->Depot[DepotNr] != nullptr) {
			Depots++;
		}
	}
	Stream.writeWord((unsigned short)Depots);
	for (int DepotNr = 0; DepotNr < MAX_DEPOTS; DepotNr++) {
		if (Slot->Depot[DepotNr] != nullptr) {
			Stream.writeByte((unsigned char)DepotNr);
			WriteItemText(Stream, Slot->Depot[DepotNr], Slot->DepotSize[DepotNr]);
		}
	}
}

// A string of any length (TWriteStream::writeString's form).
static std::string ReadText(TReadStream &Stream) {
	uint32_t Length = Stream.readWord();
	if (Length == 0xFFFF) {
		Length = Stream.readQuad();
	}

	if (Length > MAX_ITEM_TEXT) {
		throw "item text too long";
	}

	std::string Text(Length, '\0');
	if (Length > 0) {
		Stream.readBytes(reinterpret_cast<unsigned char *>(&Text[0]), (int)Length);
	}
	return Text;
}

// Turns the inventory's item text into the slot's object stream: each body
// position as a byte, followed by its objects, and 0xFF at the end.
static void ReadInventory(const std::string &Text, TPlayerData *Slot, TDynamicWriteBuffer &HelpBuffer) {
	char Name[64];
	snprintf(Name, sizeof(Name), "inventory of player %ld", (long)(int32_t)Slot->CharacterID);
	TReadScriptFile Script;
	Script.openText(Name, Text.data(), (int)Text.size());
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

	Script.nextToken();
	if (Script.Token != ENDOFFILE) {
		Script.error("end of text expected");
	}
	Script.close();

	Slot->InventorySize = HelpBuffer.getDataSize();
	Slot->Inventory = new unsigned char[Slot->InventorySize];
	memcpy(Slot->Inventory, HelpBuffer.getData(), Slot->InventorySize);
}

static void ReadDepot(const std::string &Text, int DepotNr, TPlayerData *Slot, TDynamicWriteBuffer &HelpBuffer) {
	char Name[64];
	snprintf(Name, sizeof(Name), "depot %d of player %ld", DepotNr, (long)(int32_t)Slot->CharacterID);
	TReadScriptFile Script;
	Script.openText(Name, Text.data(), (int)Text.size());
	HelpBuffer.rewind();
	LoadObjects(Script, HelpBuffer, false);

	Script.nextToken();
	if (Script.Token != ENDOFFILE) {
		Script.error("end of text expected");
	}
	Script.close();

	Slot->DepotSize[DepotNr] = HelpBuffer.getDataSize();
	Slot->Depot[DepotNr] = new unsigned char[Slot->DepotSize[DepotNr]];
	memcpy(Slot->Depot[DepotNr], HelpBuffer.getData(), Slot->DepotSize[DepotNr]);
}

void ReadPlayerData(TReadStream &Stream, TPlayerData *Slot) {
	for (int DepotNr = 0; DepotNr < MAX_DEPOTS; DepotNr++) {
		Slot->Depot[DepotNr] = nullptr;
		Slot->DepotSize[DepotNr] = 0;
	}
	Slot->Inventory = nullptr;
	Slot->InventorySize = 0;

	try {
		Stream.readString(Slot->Name, sizeof(Slot->Name));
		Slot->Race = (int)Stream.readQuad();
		Slot->Profession = (int)Stream.readQuad();
		ReadOutfit(Stream, Slot->OriginalOutfit);
		ReadOutfit(Stream, Slot->CurrentOutfit);
		Slot->LastLoginTime = (int)Stream.readQuad();
		Slot->LastLogoutTime = (int)Stream.readQuad();
		Slot->startx = (int)Stream.readQuad();
		Slot->starty = (int)Stream.readQuad();
		Slot->startz = (int)Stream.readQuad();
		Slot->posx = (int)Stream.readQuad();
		Slot->posy = (int)Stream.readQuad();
		Slot->posz = (int)Stream.readQuad();
		Slot->PlayerkillerEnd = (int)Stream.readQuad();

		for (int SkillNr = 0; SkillNr < MAX_SKILLS; SkillNr++) {
			Slot->Minimum[SkillNr] = INT_MIN;
		}
		int Skills = Stream.readWord();
		for (int i = 0; i < Skills; i++) {
			int SkillNr = Stream.readByte();
			if (!in(SkillNr, 0, MAX_SKILLS - 1)) {
				throw "illegal skill number";
			}
			Slot->Actual[SkillNr] = (int)Stream.readQuad();
			Slot->Maximum[SkillNr] = (int)Stream.readQuad();
			Slot->Minimum[SkillNr] = (int)Stream.readQuad();
			Slot->DeltaAct[SkillNr] = (int)Stream.readQuad();
			Slot->MagicDeltaAct[SkillNr] = (int)Stream.readQuad();
			Slot->Cycle[SkillNr] = (int)Stream.readQuad();
			Slot->MaxCycle[SkillNr] = (int)Stream.readQuad();
			Slot->Count[SkillNr] = (int)Stream.readQuad();
			Slot->MaxCount[SkillNr] = (int)Stream.readQuad();
			Slot->AddLevel[SkillNr] = (int)Stream.readQuad();
			Slot->Experience[SkillNr] = (int)Stream.readQuad();
			Slot->FactorPercent[SkillNr] = (int)Stream.readQuad();
			Slot->NextLevel[SkillNr] = (int)Stream.readQuad();
			Slot->Delta[SkillNr] = (int)Stream.readQuad();
		}

		memset(Slot->SpellList, 0, sizeof(Slot->SpellList));
		int Spells = Stream.readWord();
		for (int i = 0; i < Spells; i++) {
			Slot->SpellList[Stream.readByte()] = 1;
		}

		memset(Slot->QuestValues, 0, sizeof(Slot->QuestValues));
		int QuestValues = Stream.readWord();
		for (int i = 0; i < QuestValues; i++) {
			int Number = Stream.readWord();
			if (!in(Number, 0, MAX_QUESTS - 1)) {
				throw "illegal quest number";
			}
			Slot->QuestValues[Number] = (int)Stream.readQuad();
		}

		// The murders come oldest first; the slot keeps the last 20.
		memset(Slot->MurderTimestamps, 0, sizeof(Slot->MurderTimestamps));
		int Murders = Stream.readWord();
		for (int i = 0; i < Murders; i++) {
			for (int j = 1; j < 20; j++) {
				Slot->MurderTimestamps[j - 1] = Slot->MurderTimestamps[j];
			}
			Slot->MurderTimestamps[19] = (int)Stream.readQuad();
		}

		TDynamicWriteBuffer HelpBuffer(16384);
		ReadInventory(ReadText(Stream), Slot, HelpBuffer);

		int Depots = Stream.readWord();
		for (int i = 0; i < Depots; i++) {
			int DepotNr = Stream.readByte();
			if (!in(DepotNr, 0, MAX_DEPOTS - 1)) {
				throw "illegal depot number";
			}
			if (Slot->Depot[DepotNr] != nullptr) {
				throw "depot given twice";
			}
			ReadDepot(ReadText(Stream), DepotNr, Slot, HelpBuffer);
		}
	} catch (...) {
		delete[] Slot->Inventory;
		Slot->Inventory = nullptr;
		Slot->InventorySize = 0;
		for (int DepotNr = 0; DepotNr < MAX_DEPOTS; DepotNr++) {
			delete[] Slot->Depot[DepotNr];
			Slot->Depot[DepotNr] = nullptr;
			Slot->DepotSize[DepotNr] = 0;
		}
		throw;
	}
}

void SetNewPlayerData(TPlayerData *Slot) {
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
}

// The files that keep a record (the savePlayerData request after the
// character ID) the query manager did not take: SAVEPATH/playerdata-<id>.rec,
// written to a temporary file first and renamed, so that the file is either
// complete or not there. While such a file exists it is newer than what the
// query manager has: LoadPlayerData sends it first, InitPlayerData sends them
// all at start, and the next successful save makes it obsolete.
static void PendingPlayerDataPath(uint32_t CharacterID, char *Path, size_t Size) {
	snprintf(Path, Size, "%s/playerdata-%lu.rec", SAVEPATH, (unsigned long)CharacterID);
}

static bool WritePendingPlayerData(uint32_t CharacterID, unsigned char *Data, int Size) {
	char FileName[4096];
	char TempName[4096 + 8];
	PendingPlayerDataPath(CharacterID, FileName, sizeof(FileName));
	snprintf(TempName, sizeof(TempName), "%s.tmp", FileName);

	int File = open(TempName, O_WRONLY | O_CREAT | O_TRUNC, 0600);
	if (File < 0) {
		error("WritePendingPlayerData: Cannot create %s (error %d).\n", TempName, errno);
		return false;
	}

	int Written = 0;
	while (Written < Size) {
		ssize_t Result = write(File, Data + Written, (size_t)(Size - Written));
		if (Result < 0 && errno == EINTR) {
			continue;
		}
		if (Result <= 0) {
			break;
		}
		Written += (int)Result;
	}

	bool Ok = Written == Size && fsync(File) == 0;
	if (close(File) != 0) {
		Ok = false;
	}
	if (Ok && rename(TempName, FileName) != 0) {
		Ok = false;
	}
	if (!Ok) {
		error("WritePendingPlayerData: Cannot write %s (error %d).\n", FileName, errno);
		unlink(TempName);
		return false;
	}

	return true;
}

// True if no file is left.
static bool RemovePendingPlayerData(uint32_t CharacterID) {
	char FileName[4096];
	PendingPlayerDataPath(CharacterID, FileName, sizeof(FileName));
	if (unlink(FileName) != 0 && errno != ENOENT) {
		error("RemovePendingPlayerData: Cannot remove %s (error %d).\n", FileName, errno);
		return false;
	}

	return true;
}

// Sends the kept record of the character, if there is one, and removes the
// file once the query manager has it. True if no file is left.
static bool SendPendingPlayerData(uint32_t CharacterID) {
	char FileName[4096];
	PendingPlayerDataPath(CharacterID, FileName, sizeof(FileName));
	FILE *File = fopen(FileName, "rb");
	if (File == nullptr) {
		if (errno == ENOENT) {
			return true;
		}
		error("SendPendingPlayerData: Cannot open %s (error %d).\n", FileName, errno);
		return false;
	}

	std::string Data;
	char Buffer[65536];
	size_t Read;
	while ((Read = fread(Buffer, 1, sizeof(Buffer), File)) > 0) {
		Data.append(Buffer, Read);
	}
	bool ReadOk = ferror(File) == 0;
	fclose(File);
	if (!ReadOk || Data.empty()) {
		error("SendPendingPlayerData: Cannot read %s.\n", FileName);
		return false;
	}

	int Result;
	{
		TQueryManagerPoolConnection Connection(&PlayerDataConnections);
		if (Connection.operator->() == nullptr) {
			return false;
		}
		Result = Connection->savePlayerData(CharacterID, reinterpret_cast<unsigned char *>(&Data[0]), (int)Data.size());
	}
	if (Result != 0) {
		error("SendPendingPlayerData: The query manager did not take %s.\n", FileName);
		return false;
	}

	if (unlink(FileName) != 0) {
		error("SendPendingPlayerData: Cannot remove %s (error %d).\n", FileName, errno);
		return false;
	}

	print(2, "Kept data of player %lu sent to the query manager.\n", (unsigned long)CharacterID);
	return true;
}

void InitPlayerData() {
	PlayerDataConnections.init();

	DIR *SaveDir = opendir(SAVEPATH);
	if (SaveDir == nullptr) {
		error("InitPlayerData: Cannot open the save folder %s.\n", SAVEPATH);
		return;
	}

	int Sent = 0;
	int Left = 0;
	while (dirent *Entry = readdir(SaveDir)) {
		const char *Name = Entry->d_name;
		if (strncmp(Name, "playerdata-", 11) != 0 || !isdigit((unsigned char)Name[11])) {
			continue;
		}

		char *End;
		unsigned long CharacterID = strtoul(Name + 11, &End, 10);
		if (strcmp(End, ".rec") != 0 || CharacterID == 0 || CharacterID > 0xFFFFFFFFul) {
			continue;
		}

		if (SendPendingPlayerData((uint32_t)CharacterID)) {
			Sent++;
		} else {
			Left++;
		}
	}
	closedir(SaveDir);

	if (Sent > 0 || Left > 0) {
		print(1, "%d kept player data records sent to the query manager, %d left.\n", Sent, Left);
	}
}

void ExitPlayerData() {
	PlayerDataConnections.exit();
}

bool LoadPlayerData(TPlayerData *Slot) {
	if (Slot == nullptr) {
		error("LoadPlayerData: Slot is NULL.\n");
		return false;
	}

	if (Slot->CharacterID == 0) {
		error("LoadPlayerData: Slot holds no character.\n");
		return false;
	}

	if (!SendPendingPlayerData(Slot->CharacterID)) {
		error("LoadPlayerData: Data of player %ld kept under %s has not reached the query manager; not loaded.\n",
				(long)(int32_t)Slot->CharacterID, SAVEPATH);
		return false;
	}

	int Result;
	{
		TQueryManagerPoolConnection Connection(&PlayerDataConnections);
		if (Connection.operator->() == nullptr) {
			error("LoadPlayerData: No connection to the query manager.\n");
			return false;
		}
		Result = Connection->loadPlayerData(Slot);
	}

	if (Result == 1) {
		Log("game", "Player %ld logs in for the first time.\n", (long)Slot->CharacterID);
		SetNewPlayerData(Slot);
		return true;
	}

	if (Result != 0) {
		error("LoadPlayerData: Cannot load the data of player %ld.\n", (long)(int32_t)Slot->CharacterID);
		return false;
	}

	return true;
}

bool SavePlayerData(TPlayerData *Slot) {
	if (Slot == nullptr) {
		error("SavePlayerData: Slot is NULL.\n");
		return false;
	}

	if (Slot->CharacterID == 0) {
		error("SavePlayerData: Slot holds no character.\n");
		return false;
	}

	TDynamicWriteBuffer Record(16384);
	try {
		// Only the murders of the last 30 days are kept.
		WritePlayerData(Record, Slot, (int)time(nullptr) - MURDER_MEMORY);
	} catch (const char *s) {
		error("SavePlayerData: Cannot write the data of player %ld.\n", (long)(int32_t)Slot->CharacterID);
		error("# Error: %s\n", s);
		return false;
	}

	int Result = -1;
	time_t Start = time(nullptr);
	if (!SavesPaused()) {
		{
			TQueryManagerPoolConnection Connection(&PlayerDataConnections);
			// The pause may have begun while this thread waited for the
			// connection.
			if (Connection.operator->() != nullptr && !SavesPaused()) {
				Result = Connection->savePlayerData(Slot->CharacterID, Record.getData(), Record.getDataSize());
			}
		}

		if (Result != 0 && time(nullptr) - Start >= SAVE_HANG_TIME && !SavesPaused()) {
			SavePausedUntil = (long)time(nullptr) + SAVE_PAUSE;
			error("SavePlayerData: The query manager did not answer for %d seconds; player data is kept in files for the next %d seconds.\n",
					(int)(time(nullptr) - Start), SAVE_PAUSE);
		}
	}

	if (Result == 0) {
		// What a file kept is older than what was just saved, and the next
		// load or start would send it. If it cannot be removed, it gets this
		// record instead; if that fails too, the slot stays dirty, so that it
		// is not freed and its next save tries again.
		if (!RemovePendingPlayerData(Slot->CharacterID)
				&& !WritePendingPlayerData(Slot->CharacterID, Record.getData(), Record.getDataSize())) {
			error("SavePlayerData: The query manager has the data of player %ld, but an older record of it is kept under %s; it stays in memory.\n",
					(long)(int32_t)Slot->CharacterID, SAVEPATH);
			return false;
		}
		return true;
	}

	char FileName[4096];
	PendingPlayerDataPath(Slot->CharacterID, FileName, sizeof(FileName));
	if (WritePendingPlayerData(Slot->CharacterID, Record.getData(), Record.getDataSize())) {
		error("SavePlayerData: The query manager did not save the data of player %ld; kept in %s.\n",
				(long)(int32_t)Slot->CharacterID, FileName);
		return true;
	}

	error("SavePlayerData: The data of player %ld is neither with the query manager nor in %s; it stays in memory.\n",
			(long)(int32_t)Slot->CharacterID, FileName);
	return false;
}
