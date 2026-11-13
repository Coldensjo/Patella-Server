#include <csignal>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cctype>
#include <ctime>
#include <unistd.h>

#include "script.hh"
#include "config.hh"
#include "threads.hh"
#include "dbfuncs.hh"
#include "objects.hh"
#include "map.hh"
#include "info.hh"
#include "cr.hh"
#include "crmain.hh"
#include "crplayer.hh"
#include "containr.hh"
#include "strings.hh"
#include "mudinc.hh"
#include "operate.hh"
#include "houses.hh"
#include "writer.hh"
#include "sending.hh"
#include "connections.hh"
#include "utils.hh"
#include "time.hh"
#include "communication.hh"
#include "TXTEASymmetricKey.hh"
#include "receiving.hh"

// Every command handler that takes a TReadBuffer is declared
// throw(const char *) in the original. C++17 has no dynamic exception
// specifications, so an exception of another type now propagates instead of
// ending in std::unexpected().

// 0x080d6770  receiving.c:38
bool CommandAllowed(TConnection *Connection, unsigned short Command) {
	if (Connection == nullptr) {
		error("CommandAllowed: Connection ist NULL.\n");
		return false;
	}

	int State = Connection->GetState();
	if (State == CONNECTION_GAME) {
		return Command != 10 && Command != 11;
	}
	if (State == CONNECTION_DEAD || State == CONNECTION_LOGOUT) {
		return Command == 0xE6 || Command == 0xE8 || Command == 0x1E || Command == 0x14;
	}

	error("CommandAllowed: Ung\xfcltiger Verbindungsstatus %d bei Kommando %d.\n", State, Command);
	return false;
}

// 0x080d6800  receiving.c:61
bool CheckSpecialCoordinates(int Command, int x, int y, int z, bool AnyAllowed) {
	if (x != 0xFFFF) {
		return IsOnMap(x, y, z);
	}

	if (y == 0) {
		return AnyAllowed;
	}

	if (y <= 10 || (y >= 64 && y <= 79)) {
		return true;
	}

	print(3, "CheckSpecialCoordinates: Ung\xfcltige Koordinaten [%d,%d,%d] bei Kommando %d.\n", x, y, z, Command);
	return false;
}

// 0x080d6890  receiving.c:79
bool CheckVisibility(int Command, TConnection *Connection, int x, int y, int z) {
	if (Connection == nullptr) {
		error("CheckVisibility: Connection ist NULL.\n");
		return false;
	}

	if (Connection->IsVisible(x, y, z)) {
		return true;
	}

	int posx, posy, posz;
	Connection->GetPosition(posx, posy, posz);
	print(3, "CheckVisibility: Feld [%d,%d,%d] ist von [%d,%d,%d] aus nicht sichtbar bei Kommando %d.\n",
			x, y, z, posx, posy, posz, Command);
	return false;
}

// 0x080d6950  receiving.c:97
bool CheckObjectType(int Command, int TypeID) {
	if (!ObjectTypeExists(TypeID)) {
		print(3, "CheckObjectType: Ung\xfcltiger Objekttyp %d bei Kommando %d.\n", TypeID, Command);
		return false;
	}
	return true;
}

// 0x080d69a0  receiving.c:108
ObjectType GetObjectType(unsigned short TypeID) {
	if (!CheckObjectType(0x78, TypeID)) {
		return ObjectType(0);
	}
	return ObjectType(TypeID);
}

// 0x080d6a20  receiving.c:122
void CGoDirection(TConnection *Connection, int dx, int dy) {
	if (Connection == nullptr) {
		error("CGoDirection: Connection ist NULL.\n");
		return;
	}

	int x, y, z;
	Connection->GetPosition(x, y, z);

	TPlayer *pl = Connection->GetPlayer();
	if (pl == nullptr) {
		return;
	}

	try {
		if (pl->ToDoClear()) {
			SendSnapback(Connection);
		}

		TToDoEntry TD;
		TD.Code = TDGo;
		TD.Go.x = x + dx;
		TD.Go.y = y + dy;
		TD.Go.z = z;
		pl->ToDoAdd(TD);
		pl->ToDoStart();
	} catch (RESULT r) {
		SendResult(Connection, r);
		pl->ToDoYield();
	}
}

// 0x080d6b20  receiving.c:151
void CGoPath(TConnection *Connection, TReadBuffer &Buffer) {
	if (Connection == nullptr) {
		error("CGoPath: Connection ist NULL.\n");
		return;
	}

	int Steps = Buffer.readByte();
	int x, y, z;
	Connection->GetPosition(x, y, z);

	TPlayer *pl = Connection->GetPlayer();
	if (pl == nullptr) {
		return;
	}

	try {
		if (pl->ToDoClear()) {
			SendSnapback(Connection);
		}

		for (int i = 0; i < Steps; i++) {
			switch (Buffer.readByte()) {
				case 1: x++; break;
				case 2: x++; y--; break;
				case 3: y--; break;
				case 4: x--; y--; break;
				case 5: x--; break;
				case 6: x--; y++; break;
				case 7: y++; break;
				case 8: x++; y++; break;
				default: continue;
			}

			TToDoEntry TD;
			TD.Code = TDGo;
			TD.Go.x = x;
			TD.Go.y = y;
			TD.Go.z = z;
			pl->ToDoAdd(TD);
		}

		pl->ToDoStart();
	} catch (RESULT r) {
		SendResult(Connection, r);
		pl->ToDoYield();
	}
}

// 0x080d6ca0  receiving.c:194
void CStop(TConnection *Connection) {
	if (Connection == nullptr) {
		error("CGoStop: Connection ist NULL.\n");
		return;
	}

	TPlayer *pl = Connection->GetPlayer();
	if (pl != nullptr) {
		pl->ToDoStop();
	}
}

// 0x080d6ce0  receiving.c:209
void CLookAtPoint(TConnection *Connection, TReadBuffer &Buffer) {
	if (Connection == nullptr) {
		error("CLookAtPoint: Connection ist NULL.\n");
		return;
	}

	int x = Buffer.readWord();
	int y = Buffer.readWord();
	int z = Buffer.readByte();
	if (!CheckSpecialCoordinates(0x8C, x, y, z, false)) {
		return;
	}

	if (x != 0xFFFF && !CheckVisibility(0x8C, Connection, x, y, z)) {
		return;
	}

	TPlayer *pl = Connection->GetPlayer();
	if (pl == nullptr) {
		return;
	}

	uint32_t CreatureID = pl->ID;
	Object Obj = GetObject(CreatureID, x, y, z, (x == 0xFFFF ? z : -1), ObjectType(0));
	if (!Obj.exists()) {
		return;
	}

	try {
		Look(CreatureID, Obj);
	} catch (RESULT r) {
		SendResult(Connection, r);
	}
}

// 0x080d6f00  receiving.c:240
void CRefreshField(TConnection *Connection, TReadBuffer &Buffer) {
	if (Connection == nullptr) {
		error("CRefreshField: Connection ist NULL.\n");
		return;
	}

	int x = Buffer.readWord();
	int y = Buffer.readWord();
	int z = Buffer.readByte();
	if (!IsOnMap(x, y, z)) {
		return;
	}

	if (!CheckVisibility(0xC9, Connection, x, y, z)) {
		return;
	}

	SendFieldData(Connection, x, y, z);
}

// 0x080d7030  receiving.c:258
void CTalk(TConnection *Connection, TReadBuffer &Buffer) {
	if (Connection == nullptr) {
		error("CTalk: Connection ist NULL.\n");
		return;
	}

	int Mode = Buffer.readByte();
	if (Mode != TALK_SAY
			&& Mode != TALK_WHISPER
			&& Mode != TALK_YELL
			&& Mode != TALK_PRIVATE_MESSAGE
			&& Mode != TALK_CHANNEL_CALL
			&& Mode != TALK_GAMEMASTER_REQUEST
			&& Mode != TALK_GAMEMASTER_ANSWER
			&& Mode != TALK_PLAYER_ANSWER
			&& Mode != TALK_GAMEMASTER_MESSAGE
			&& Mode != TALK_ANONYMOUS_MESSAGE
			&& Mode != TALK_GAMEMASTER_CHANNELCALL
			&& Mode != TALK_ANONYMOUS_CHANNELCALL
			&& Mode != TALK_GAMEMASTER_BROADCAST
			&& Mode != TALK_ANONYMOUS_BROADCAST) {
		print(3, "CTalk: Unbekannter Sprechmodus %d.\n", Mode);
		return;
	}

	char Addressee[30];
	Addressee[0] = '\0';
	int Channel = 0;
	if (Mode == TALK_PRIVATE_MESSAGE
			|| Mode == TALK_GAMEMASTER_ANSWER
			|| Mode == TALK_GAMEMASTER_MESSAGE
			|| Mode == TALK_ANONYMOUS_MESSAGE) {
		Buffer.readString(Addressee, sizeof(Addressee));
		if (Addressee[0] == '\0') {
			print(3, "CTalk: Adressat nicht angegeben.\n");
			return;
		}
	}

	if (Mode == TALK_CHANNEL_CALL
			|| Mode == TALK_GAMEMASTER_CHANNELCALL
			|| Mode == TALK_ANONYMOUS_CHANNELCALL) {
		Channel = Buffer.readWord();
		if (Channel >= GetNumberOfChannels()) {
			print(3, "CTalk: Ung\xfcltiger Kanal %d.\n", Channel);
			return;
		}
		sprintf(Addressee, "%d", Channel);
	}

	char Text[256];
	Buffer.readString(Text, sizeof(Text));
	if (Text[0] == '\0') {
		print(3, "CTalk: Kein Text.\n");
		return;
	}

	TPlayer *pl = Connection->GetPlayer();
	if (pl == nullptr) {
		print(3, "CTalk: Der Verbindung ist kein Spieler zugeordnet.\n");
		return;
	}

	if (strchr(Text, '\n') != nullptr) {
		error("CTalk: %s verwendet Newlines (%d,%s)\n", pl->Name, Mode, Text);
		return;
	}

	if (Mode == TALK_GAMEMASTER_ANSWER && !CheckRight(pl->ID, READ_GAMEMASTER_CHANNEL)) {
		Mode = TALK_PRIVATE_MESSAGE;
	}
	if (Mode == TALK_GAMEMASTER_BROADCAST && !CheckRight(pl->ID, GAMEMASTER_BROADCAST)) {
		Mode = TALK_SAY;
	}
	if (Mode == TALK_GAMEMASTER_CHANNELCALL && !CheckRight(pl->ID, GAMEMASTER_BROADCAST)) {
		Mode = TALK_CHANNEL_CALL;
	}
	if (Mode == TALK_GAMEMASTER_MESSAGE && !CheckRight(pl->ID, GAMEMASTER_BROADCAST)) {
		Mode = TALK_PRIVATE_MESSAGE;
	}
	if (Mode == TALK_ANONYMOUS_BROADCAST && !CheckRight(pl->ID, ANONYMOUS_BROADCAST)) {
		Mode = TALK_SAY;
	}
	if (Mode == TALK_ANONYMOUS_CHANNELCALL && !CheckRight(pl->ID, ANONYMOUS_BROADCAST)) {
		Mode = TALK_CHANNEL_CALL;
	}
	if (Mode == TALK_ANONYMOUS_MESSAGE && !CheckRight(pl->ID, ANONYMOUS_BROADCAST)) {
		Mode = TALK_PRIVATE_MESSAGE;
	}

	if (Mode == TALK_YELL && pl->Skills[0]->Get() < 2) {
		SendMessage(Connection, TALK_FAILURE_MESSAGE, "You may not yell as long as you are on level 1.");
		return;
	}

	if (Mode == TALK_CHANNEL_CALL && pl->Skills[0]->Get() < 2) {
		SendMessage(Connection, TALK_FAILURE_MESSAGE, "You may not speak into channels as long as you are on level 1.");
		return;
	}

	if ((Mode == TALK_CHANNEL_CALL
				|| Mode == TALK_GAMEMASTER_CHANNELCALL
				|| Mode == TALK_ANONYMOUS_CHANNELCALL)
			&& Channel == REQUEST_QUEUE) {
		print(3, "ChannelCall f\xfcr GM-Request-Queue.\n");
		return;
	}

	if ((Mode == TALK_CHANNEL_CALL
				|| Mode == TALK_GAMEMASTER_CHANNELCALL
				|| Mode == TALK_ANONYMOUS_CHANNELCALL)
			&& !ChannelSubscribed(Channel, pl->ID)) {
		Addressee[0] = '\0';
		Mode = TALK_SAY;
	}

	if (Mode == TALK_CHANNEL_CALL && Channel == HELP_CHANNEL
			&& CheckRight(pl->ID, HIGHLIGHT_HELP_CHANNEL)) {
		pl->TutorActivities++;
		print(3, "Aktvit\xe4tspunkt f\xfcr %s.\n", pl->Name);
		Mode = TALK_HIGHLIGHT_CHANNELCALL;
	}

	if (Mode == TALK_PLAYER_ANSWER) {
		if (pl->Request == 0) {
			return;
		}

		int GamemasterID = pl->RequestProcessingGamemaster;
		if (GamemasterID == 0) {
			SendMessage(Connection, TALK_FAILURE_MESSAGE, "Please wait until your request is answered.");
			return;
		}

		TCreature *Gamemaster = GetCreature(GamemasterID);
		if (Gamemaster == nullptr) {
			SendCloseRequest(Connection);
			pl->Request = 0;
			return;
		}

		strcpy(Addressee, Gamemaster->Name);
	}

	if (Mode == TALK_GAMEMASTER_REQUEST) {
		if (pl->Request != 0) {
			SendMessage(Connection, TALK_FAILURE_MESSAGE,
					"You have already submitted a request. Please wait until it is answered.");
			return;
		}

		if (Text[0] == '\0') {
			return;
		}

		pl->Request = AddDynamicString(Text);
		pl->RequestTimestamp = RoundNr;
		pl->RequestProcessingGamemaster = 0;
		for (TConnection *Connection = GetFirstConnection();
				Connection != nullptr;
				Connection = GetNextConnection()) {
			if (Connection->IsOnline()) {
				TCreature *recv = Connection->GetPlayer();
				if (recv != nullptr && ChannelSubscribed(REQUEST_QUEUE, recv->ID)) {
					SendTalk(Connection, 0, pl->Name, Mode, Text, 0);
				}
			}
		}
		return;
	}

	try {
		pl->ToDoTalk(Mode, Addressee, Text, true);
		pl->ToDoStart();
	} catch (RESULT r) {
		SendResult(Connection, r);
		pl->ToDoYield();
	}
}

// 0x080d7710  receiving.c:386
void CGetChannels(TConnection *Connection) {
	if (Connection == nullptr) {
		error("CGetChannels: Connection ist NULL.\n");
		return;
	}

	SendChannels(Connection);
}

// 0x080d7730  receiving.c:398
void CJoinChannel(TConnection *Connection, TReadBuffer &Buffer) {
	if (Connection == nullptr) {
		error("CJoinChannel:  ist NULL.\n");
		return;
	}

	int Channel = Buffer.readWord();
	if (Channel >= GetNumberOfChannels()) {
		return;
	}

	TPlayer *pl = Connection->GetPlayer();
	if (pl == nullptr) {
		return;
	}

	try {
		bool OwnChannel = JoinChannel(Channel, pl->ID);
		if (Channel == REQUEST_QUEUE) {
			SendOpenRequestQueue(Connection);
			SendExistingRequests(Connection);
		} else if (OwnChannel) {
			SendOpenOwnChannel(Connection, Channel);
		} else {
			SendOpenChannel(Connection, Channel);
		}
	} catch (RESULT) {
		print(3, "Spieler darf Kanal nicht h\xf6ren.\n");
	}
}

// 0x080d7830  receiving.c:425
void CLeaveChannel(TConnection *Connection, TReadBuffer &Buffer) {
	if (Connection == nullptr) {
		error("CLeaveChannel: Connection ist NULL.\n");
		return;
	}

	int Channel = Buffer.readWord();
	if (Channel >= GetNumberOfChannels()) {
		return;
	}

	TPlayer *pl = Connection->GetPlayer();
	if (pl == nullptr) {
		return;
	}

	LeaveChannel(Channel, pl->ID, true);
}

// 0x080d78c0  receiving.c:442
void CPrivateChannel(TConnection *Connection, TReadBuffer &Buffer) {
	if (Connection == nullptr) {
		error("CPrivateChannel: Connection ist NULL.\n");
		return;
	}

	char Name[30];
	Buffer.readString(Name, sizeof(Name));
	if (Name[0] == '\0') {
		return;
	}

	uint32_t OppID = GetCharacterID(Name);
	if (OppID == 0) {
		SendResult(Connection, PLAYERNOTEXISTING);
		return;
	}

	// The original does not check the player for NULL here.
	if (OppID == Connection->GetPlayer()->ID) {
		SendMessage(Connection, TALK_FAILURE_MESSAGE, "You cannot set up a private message channel with yourself.");
		return;
	}

	SendPrivateChannel(Connection, GetCharacterName(Name));
}

// 0x080d7990  receiving.c:461
void CProcessRequest(TConnection *Connection, TReadBuffer &Buffer) {
	if (Connection == nullptr) {
		error("CProcessRequest: Connection ist NULL.\n");
		return;
	}

	TPlayer *pl = Connection->GetPlayer();
	if (pl == nullptr || !ChannelAvailable(REQUEST_QUEUE, pl->ID)) {
		return;
	}

	char Name[30];
	Buffer.readString(Name, sizeof(Name));
	if (Name[0] == '\0') {
		return;
	}

	// The original sends the literal "Name" instead of the player's name to
	// SendFinishRequest in both error cases.
	TPlayer *Opp;
	if (IdentifyPlayer(Name, true, true, Opp) != 0) {
		SendMessage(Connection, TALK_FAILURE_MESSAGE, "Player is not online any more.");
		SendFinishRequest(Connection, "Name");
		return;
	}

	if (Opp->Request == 0 || Opp->RequestProcessingGamemaster != 0) {
		SendMessage(Connection, TALK_FAILURE_MESSAGE, "Request has already been processed.");
		SendFinishRequest(Connection, "Name");
		return;
	}

	Opp->RequestProcessingGamemaster = pl->ID;
	DeleteGamemasterRequest(Opp->Name);
}

// 0x080d7ac0  receiving.c:494
void CRemoveRequest(TConnection *Connection, TReadBuffer &Buffer) {
	if (Connection == nullptr) {
		error("CRemoveRequest: Connection ist NULL.\n");
		return;
	}

	TPlayer *pl = Connection->GetPlayer();
	if (pl == nullptr || !ChannelAvailable(REQUEST_QUEUE, pl->ID)) {
		return;
	}

	char Name[30];
	Buffer.readString(Name, sizeof(Name));
	if (Name[0] == '\0') {
		return;
	}

	// The original sends the literal "Name" instead of the player's name to
	// SendFinishRequest.
	TPlayer *Opp;
	if (IdentifyPlayer(Name, true, true, Opp) != 0) {
		SendMessage(Connection, TALK_FAILURE_MESSAGE, "Player is not online any more.");
		SendFinishRequest(Connection, "Name");
		return;
	}

	if (Opp->Request == 0) {
		return;
	}

	if (Opp->RequestProcessingGamemaster == 0) {
		DeleteGamemasterRequest(Opp->Name);
	} else if (Opp->RequestProcessingGamemaster != pl->ID) {
		return;
	}

	SendCloseRequest(Opp->GetConnection());
	Opp->Request = 0;
}

// 0x080d7c00  receiving.c:524
void CCancelRequest(TConnection *Connection) {
	if (Connection == nullptr) {
		error("CCancelRequest: Connection ist NULL.\n");
		return;
	}

	TPlayer *pl = Connection->GetPlayer();
	if (pl == nullptr || !ChannelAvailable(REQUEST_QUEUE, pl->ID) || pl->Request == 0) {
		return;
	}

	int GamemasterID = pl->RequestProcessingGamemaster;
	if (GamemasterID == 0) {
		DeleteGamemasterRequest(pl->Name);
	} else {
		TPlayer *Gamemaster = GetPlayer(GamemasterID);
		if (Gamemaster != nullptr) {
			SendFinishRequest(Gamemaster->GetConnection(), pl->Name);
		}
	}

	pl->Request = 0;
}

// 0x080d7cb0  receiving.c:545
void CRotate(TConnection *Connection, int Direction) {
	if (Connection == nullptr) {
		error("CRotate: Connection ist NULL.\n");
		return;
	}

	if (Direction < 0 || Direction > 3) {
		return;
	}

	TPlayer *pl = Connection->GetPlayer();
	if (pl == nullptr) {
		return;
	}

	try {
		pl->ToDoRotate(Direction);
		pl->ToDoStart();
	} catch (RESULT r) {
		SendResult(Connection, r);
		pl->ToDoYield();
	}
}

// 0x080d7d60  receiving.c:567
void CBugReport(TConnection *Connection, TReadBuffer &Buffer) {
	if (Connection == nullptr) {
		error("CBugReport: Connection ist NULL.\n");
		return;
	}

	char Text[1024];
	Buffer.readString(Text, sizeof(Text));
	Text[sizeof(Text) - 1] = '\0';
	while (Text[0] != '\0' && !isprint(Text[strlen(Text) - 1])) {
		Text[strlen(Text) - 1] = '\0';
	}

	if (Text[0] == '\0') {
		return;
	}

	time_t Time;
	time(&Time);
	int posx, posy, posz;
	Connection->GetPosition(posx, posy, posz);
	Log("bugreport", "%s - %d/%d - [%d,%d,%d] - %s",
			Connection->GetName(),
			Connection->GetTerminalType(),
			Connection->GetTerminalVersion(),
			posx, posy, posz,
			ctime(&Time));

	// A whitespace character other than ' ' at the start of a line that is
	// followed by more than 74 characters without another whitespace makes
	// this loop log empty lines forever, as in the original.
	int LineStart = 0;
	while (Text[LineStart] != '\0') {
		int i = 0;
		int LastSpace = -1;
		while (true) {
			if (Text[LineStart + i] == '\0') {
				Log("bugreport", "%s\n", &Text[LineStart]);
				LineStart += i;
				break;
			}

			if (i > 74) {
				char Line[80];
				if (LastSpace == -1) {
					strncpy(Line, &Text[LineStart], 74);
					Line[74] = '-';
					Line[75] = '\0';
					Log("bugreport", "%s\n", Line);
					LineStart += 74;
				} else {
					strncpy(Line, &Text[LineStart], LastSpace);
					Line[LastSpace] = '\0';
					Log("bugreport", "%s\n", Line);
					if (Text[LineStart + LastSpace] == ' ') {
						LineStart++;
					}
					LineStart += LastSpace;
				}
				break;
			}

			if (Text[LineStart + i] == '\n') {
				char Line[80];
				strncpy(Line, &Text[LineStart], i);
				Line[i] = '\0';
				Log("bugreport", "%s\n", Line);
				LineStart += i + 1;
				break;
			}

			if (isSpace(Text[LineStart + i])) {
				LastSpace = i;
			}
			i++;
		}
	}

	Log("bugreport", "---------------------------------------------------------------------------\n");
	SendMessage(Connection, TALK_STATUS_MESSAGE, "Comment sent.");
}

// 0x080d8140  receiving.c:630
void CErrorFileEntry(TConnection *Connection, TReadBuffer &Buffer) {
	if (Connection == nullptr) {
		error("CErrorFileEntry: Connection ist NULL.\n");
		return;
	}

	char Title[65], Date[27], Stack[2049], Comment[513];
	Buffer.readString(Title, sizeof(Title));
	Buffer.readString(Date, sizeof(Date));
	Buffer.readString(Stack, sizeof(Stack));
	Buffer.readString(Comment, sizeof(Comment));
	if (Stack[0] == '\0' && Comment[0] == '\0') {
		return;
	}

	time_t Time;
	time(&Time);
	int posx, posy, posz;
	Connection->GetPosition(posx, posy, posz);
	Log("client-error", "%s - %d/%d - [%d,%d,%d] - %s",
			Connection->GetName(),
			Connection->GetTerminalType(),
			Connection->GetTerminalVersion(),
			posx, posy, posz,
			ctime(&Time));
	Log("client-error", "%s\n", Title);
	Log("client-error", "%s\n", Date);
	Log("client-error", "\n");

	char *pStack = Stack;
	while (true) {
		char *p = strchr(pStack, '\n');
		if (p == nullptr) {
			break;
		}
		*p = '\0';
		if (pStack[0] != '\0') {
			Log("client-error", "%s\n", pStack);
		}
		pStack = p + 1;
	}

	if (pStack[0] != '\0') {
		Log("client-error", "%s\n", pStack);
	}

	if (Stack[0] != '\0' && Comment[0] != '\0') {
		Log("client-error", "\n");
	}

	// The same endless loop as in CBugReport is possible here.
	int LineStart = 0;
	while (Comment[LineStart] != '\0') {
		int i = 0;
		int LastSpace = -1;
		while (true) {
			if (Comment[LineStart + i] == '\0') {
				Log("client-error", "%s\n", &Comment[LineStart]);
				LineStart += i;
				break;
			}

			if (i > 74) {
				char Line[80];
				if (LastSpace == -1) {
					strncpy(Line, &Comment[LineStart], 74);
					Line[74] = '-';
					Line[75] = '\0';
					Log("client-error", "%s\n", Line);
					LineStart += 74;
				} else {
					strncpy(Line, &Comment[LineStart], LastSpace);
					Line[LastSpace] = '\0';
					Log("client-error", "%s\n", Line);
					if (Comment[LineStart + LastSpace] == ' ') {
						LineStart++;
					}
					LineStart += LastSpace;
				}
				break;
			}

			if (Comment[LineStart + i] == '\n') {
				char Line[80];
				strncpy(Line, &Comment[LineStart], i);
				Line[i] = '\0';
				Log("client-error", "%s\n", Line);
				LineStart += i + 1;
				break;
			}

			if (isSpace(Comment[LineStart + i])) {
				LastSpace = i;
			}
			i++;
		}
	}

	Log("client-error", "---------------------------------------------------------------------------\n");
}

// 0x080d8540  receiving.c:707
void CMoveObject(TConnection *Connection, TReadBuffer &Buffer) {
	if (Connection == nullptr) {
		error("CMoveObject: Connection ist NULL.\n");
		return;
	}

	int sx = Buffer.readWord();
	int sy = Buffer.readWord();
	int sz = Buffer.readByte();
	ObjectType Type = GetObjectType(Buffer.readWord());
	unsigned char RNum = Buffer.readByte();
	int zx = Buffer.readWord();
	int zy = Buffer.readWord();
	int zz = Buffer.readByte();
	unsigned char Count = Buffer.readByte();
	if (sx != 0xFFFF) {
		RNum = 1;
	}

	if (!CheckSpecialCoordinates(0x78, sx, sy, sz, false)) {
		return;
	}

	if (sx != 0xFFFF && !CheckVisibility(0x78, Connection, sx, sy, sz)) {
		return;
	}

	if (!CheckSpecialCoordinates(0x78, zx, zy, zz, true)) {
		return;
	}

	if (zx != 0xFFFF && !CheckVisibility(0x78, Connection, zx, zy, zz)) {
		return;
	}

	if (Type == 0) {
		return;
	}

	if (Type.getFlag(CUMULATIVE) && Count == 0) {
		return;
	}

	TPlayer *pl = Connection->GetPlayer();
	if (pl == nullptr) {
		return;
	}

	try {
		pl->ToDoMove(sx, sy, sz, Type, RNum, zx, zy, zz, Count);
		pl->ToDoStart();
	} catch (RESULT r) {
		if (r != NOERROR) {
			SendResult(Connection, r);
			pl->ToDoYield();
		}
	}
}

// 0x080d88e0  receiving.c:747
void CTradeObject(TConnection *Connection, TReadBuffer &Buffer) {
	if (Connection == nullptr) {
		error("CTradeObject: Connection ist NULL.\n");
		return;
	}

	int x = Buffer.readWord();
	int y = Buffer.readWord();
	int z = Buffer.readByte();
	ObjectType Type = GetObjectType(Buffer.readWord());
	unsigned char RNum = Buffer.readByte();
	uint32_t TradePartner = Buffer.readQuad();
	if (x != 0xFFFF) {
		RNum = 1;
	}

	if (!CheckSpecialCoordinates(0x7D, x, y, z, false)) {
		return;
	}

	if (x != 0xFFFF && !CheckVisibility(0x7D, Connection, x, y, z)) {
		return;
	}

	if (Type == 0) {
		return;
	}

	TPlayer *pl = Connection->GetPlayer();
	if (pl == nullptr) {
		return;
	}

	try {
		pl->ToDoTrade(x, y, z, Type, RNum, TradePartner);
		pl->ToDoStart();
	} catch (RESULT r) {
		if (r != NOERROR) {
			SendResult(Connection, r);
			pl->ToDoYield();
		}
	}
}

// 0x080d8b40  receiving.c:781
void CInspectTrade(TConnection *Connection, TReadBuffer &Buffer) {
	if (Connection == nullptr) {
		error("CInspectTrade: Connection ist NULL.\n");
		return;
	}

	int Side = Buffer.readByte();
	int Position = Buffer.readByte();
	TPlayer *pl = Connection->GetPlayer();
	if (pl == nullptr) {
		return;
	}

	Object Obj = pl->InspectTrade(Side == 0, Position);
	if (!Obj.exists()) {
		return;
	}

	try {
		Look(pl->ID, Obj);
	} catch (RESULT r) {
		SendResult(Connection, r);
	}
}

// 0x080d8c50  receiving.c:805
void CAcceptTrade(TConnection *Connection) {
	if (Connection == nullptr) {
		error("CAcceptTrade: Connection ist NULL.\n");
		return;
	}

	TPlayer *pl = Connection->GetPlayer();
	if (pl != nullptr) {
		pl->AcceptTrade();
	}
}

// 0x080d8c90  receiving.c:819
void CRejectTrade(TConnection *Connection) {
	if (Connection == nullptr) {
		error("CRejectTrade: Connection ist NULL.\n");
		return;
	}

	TPlayer *pl = Connection->GetPlayer();
	if (pl != nullptr) {
		pl->RejectTrade();
	}
}

// 0x080d8cd0  receiving.c:834
void CUseObject(TConnection *Connection, TReadBuffer &Buffer) {
	if (Connection == nullptr) {
		error("CUseObject: Connection ist NULL.\n");
		return;
	}

	int x = Buffer.readWord();
	int y = Buffer.readWord();
	int z = Buffer.readByte();
	ObjectType Type = GetObjectType(Buffer.readWord());
	unsigned char RNum = Buffer.readByte();
	unsigned char Dummy = Buffer.readByte();

	if (!CheckSpecialCoordinates(0x82, x, y, z, false)) {
		return;
	}

	if (x != 0xFFFF && !CheckVisibility(0x82, Connection, x, y, z)) {
		return;
	}

	if (Type == 0) {
		return;
	}

	if (Type.getFlag(MULTIUSE)) {
		return;
	}

	if (Type.getFlag(CONTAINER) && Dummy > 15) {
		return;
	}

	TPlayer *pl = Connection->GetPlayer();
	if (pl == nullptr) {
		return;
	}

	try {
		pl->ToDoWait(100);
		pl->ToDoUse(1, x, y, z, Type, RNum, Dummy, 0, 0, 0, ObjectType(0), 0);
		pl->ToDoStart();
	} catch (RESULT r) {
		pl->ToDoClear();
		SendResult(Connection, r);
		pl->ToDoYield();
	}
}

// 0x080d8fa0  receiving.c:870
void CUseTwoObjects(TConnection *Connection, TReadBuffer &Buffer) {
	if (Connection == nullptr) {
		error("CUseTwoObjects: Connection ist NULL.\n");
		return;
	}

	int x1 = Buffer.readWord();
	int y1 = Buffer.readWord();
	int z1 = Buffer.readByte();
	ObjectType Type1 = GetObjectType(Buffer.readWord());
	unsigned char RNum1 = Buffer.readByte();
	int x2 = Buffer.readWord();
	int y2 = Buffer.readWord();
	int z2 = Buffer.readByte();
	ObjectType Type2 = GetObjectType(Buffer.readWord());
	unsigned char RNum2 = Buffer.readByte();

	if (!CheckSpecialCoordinates(0x83, x1, y1, z1, false)) {
		return;
	}

	if (x1 != 0xFFFF && !CheckVisibility(0x83, Connection, x1, y1, z1)) {
		return;
	}

	if (Type1 == 0) {
		return;
	}

	if (!Type1.getFlag(MULTIUSE)) {
		return;
	}

	if (!CheckSpecialCoordinates(0x83, x2, y2, z2, false)) {
		return;
	}

	if (x2 != 0xFFFF && !CheckVisibility(0x83, Connection, x2, y2, z2)) {
		return;
	}

	if (Type2 == 0) {
		return;
	}

	TPlayer *pl = Connection->GetPlayer();
	if (pl == nullptr) {
		return;
	}

	try {
		pl->ToDoWait(100);
		pl->ToDoUse(2, x1, y1, z1, Type1, RNum1, 0, x2, y2, z2, Type2, RNum2);
		pl->ToDoStart();
	} catch (RESULT r) {
		pl->ToDoClear();
		SendResult(Connection, r);
		pl->ToDoYield();
	}
}

// 0x080d93d0  receiving.c:912
void CUseOnCreature(TConnection *Connection, TReadBuffer &Buffer) {
	if (Connection == nullptr) {
		error("CUseOnCreature: Connection ist NULL.\n");
		return;
	}

	int x = Buffer.readWord();
	int y = Buffer.readWord();
	int z = Buffer.readByte();
	ObjectType Type = GetObjectType(Buffer.readWord());
	unsigned char RNum = Buffer.readByte();
	uint32_t CreatureID = Buffer.readQuad();

	if (!CheckSpecialCoordinates(0x84, x, y, z, false)) {
		return;
	}

	if (x != 0xFFFF && !CheckVisibility(0x84, Connection, x, y, z)) {
		return;
	}

	if (Type == 0) {
		return;
	}

	TCreature *Dest = GetCreature(CreatureID);
	if (Dest == nullptr) {
		return;
	}

	if (!CheckVisibility(0x84, Connection, Dest->posx, Dest->posy, Dest->posz)) {
		return;
	}

	if (Dest->Type == PLAYER) {
		return;
	}

	TPlayer *pl = Connection->GetPlayer();
	if (pl == nullptr) {
		return;
	}

	if (abs(pl->posx - Dest->posx) >= Connection->GetTerminalOffsetX()
			|| abs(pl->posy - Dest->posy) >= Connection->GetTerminalOffsetY()) {
		SendResult(Connection, OUTOFRANGE);
		return;
	}

	Object Obj1 = GetObject(pl->ID, x, y, z, RNum, Type);
	Object Obj2 = Dest->GetCreatureObject();
	if (!Obj1.exists() || !Obj2.exists()) {
		SendResult(Connection, NOTACCESSIBLE);
		return;
	}

	try {
		pl->ToDoWait(100);
		pl->ToDoUse(2, Obj1, Obj2);
		pl->ToDoStart();
	} catch (RESULT r) {
		pl->ToDoClear();
		SendResult(Connection, r);
		pl->ToDoYield();
	}
}

// 0x080d9840  receiving.c:961
void CTurnObject(TConnection *Connection, TReadBuffer &Buffer) {
	if (Connection == nullptr) {
		error("CTurnObject: Connection ist NULL.\n");
		return;
	}

	int x = Buffer.readWord();
	int y = Buffer.readWord();
	int z = Buffer.readByte();
	ObjectType Type = GetObjectType(Buffer.readWord());
	unsigned char RNum = Buffer.readByte();

	if (!CheckSpecialCoordinates(0x85, x, y, z, false)) {
		return;
	}

	if (x != 0xFFFF && !CheckVisibility(0x85, Connection, x, y, z)) {
		return;
	}

	if (Type == 0) {
		return;
	}

	TPlayer *pl = Connection->GetPlayer();
	if (pl == nullptr) {
		return;
	}

	try {
		pl->ToDoWait(100);
		pl->ToDoTurn(x, y, z, Type, RNum);
		pl->ToDoStart();
	} catch (RESULT r) {
		pl->ToDoClear();
		SendResult(Connection, r);
		pl->ToDoYield();
	}
}

// 0x080d9a90  receiving.c:994
void CCloseContainer(TConnection *Connection, TReadBuffer &Buffer) {
	if (Connection == nullptr) {
		error("CCloseContainer: Connection ist NULL.\n");
		return;
	}

	int Window = Buffer.readByte();
	if (Window > 15) {
		return;
	}

	TPlayer *pl = Connection->GetPlayer();
	if (pl == nullptr) {
		return;
	}

	pl->SetOpenContainer(Window, NONE);
	SendCloseContainer(Connection, Window);
}

// 0x080d9b20  receiving.c:1012
void CUpContainer(TConnection *Connection, TReadBuffer &Buffer) {
	if (Connection == nullptr) {
		error("CUpContainer: Connection ist NULL.\n");
		return;
	}

	int Window = Buffer.readByte();
	if (Window > 15) {
		return;
	}

	TPlayer *pl = Connection->GetPlayer();
	if (pl == nullptr) {
		return;
	}

	Object Con = pl->GetOpenContainer(Window);
	if (Con == NONE) {
		return;
	}

	Con = Con.getContainer();
	if (Con.isMapContainer() || Con.isBodyContainer()) {
		pl->SetOpenContainer(Window, NONE);
		SendCloseContainer(Connection, Window);
	} else {
		pl->SetOpenContainer(Window, Con);
		SendContainer(Connection, Window);
	}
}

// 0x080d9c80  receiving.c:1039
void CRefreshContainer(TConnection *Connection, TReadBuffer &Buffer) {
	if (Connection == nullptr) {
		error("CRefreshContainer: Connection ist NULL.\n");
		return;
	}

	int Window = Buffer.readByte();
	if (Window > 15) {
		return;
	}

	TPlayer *pl = Connection->GetPlayer();
	if (pl == nullptr) {
		return;
	}

	Object Con = pl->GetOpenContainer(Window);
	if (Con == NONE) {
		return;
	}

	SendContainer(Connection, Window);
}

// 0x080d9d20  receiving.c:1058
void CGetOutfit(TConnection *Connection) {
	if (Connection == nullptr) {
		error("CGetOutfit: Connection ist NULL.\n");
		return;
	}

	TPlayer *pl = Connection->GetPlayer();
	if (pl == nullptr) {
		return;
	}

	if (CheckRight(pl->ID, GAMEMASTER_OUTFIT)) {
		SendMessage(Connection, TALK_FAILURE_MESSAGE, "You may not change your outfit.");
		return;
	}

	SendOutfit(Connection);
}

// 0x080d9da0  receiving.c:1074
void CSetOutfit(TConnection *Connection, TReadBuffer &Buffer) {
	if (Connection == nullptr) {
		error("CSetOutfit: Connection ist NULL.\n");
		return;
	}

	TPlayer *pl = Connection->GetPlayer();
	if (pl == nullptr) {
		return;
	}

	int OutfitID = Buffer.readWord();
	unsigned char Colors[4];
	Buffer.readBytes(Colors, 4);

	if (pl->Sex == 1) {
		if (!in(OutfitID, 128, 134)
				|| (!CheckRight(pl->ID, PREMIUM_ACCOUNT) && OutfitID > 131)) {
			print(3, "CSetOutfit: Ung\xfcltiges Outfit %d f\xfcr M\xe4nner.\n", OutfitID);
			return;
		}
	}

	if (pl->Sex == 2) {
		if (!in(OutfitID, 136, 142)
				|| (!CheckRight(pl->ID, PREMIUM_ACCOUNT) && OutfitID > 139)) {
			print(3, "CSetOutfit: Ung\xfcltiges Outfit %d f\xfcr Frauen.\n", OutfitID);
			return;
		}
	}

	if (Colors[0] > 132 || Colors[1] > 132 || Colors[2] > 132 || Colors[3] > 132) {
		print(3, "CSetOutfit: Ung\xfcltige Farbe f\xfcr Outfit.\n");
		return;
	}

	TOutfit NewOutfit;
	NewOutfit.OutfitID = OutfitID;
	memcpy(NewOutfit.Colors, Colors, sizeof(NewOutfit.Colors));
	if (memcmp(&pl->Outfit, &pl->OrgOutfit, sizeof(TOutfit)) == 0) {
		pl->Outfit = NewOutfit;
		AnnounceChangedCreature(pl->ID, 3);
	}
	pl->OrgOutfit = NewOutfit;
}

// 0x080d9fb0  receiving.c:1112
void CEditText(TConnection *Connection, TReadBuffer &Buffer) {
	if (Connection == nullptr) {
		error("CEditText: Connection ist NULL.\n");
		return;
	}

	Object Obj(Buffer.readQuad());
	if (!Obj.exists()
			|| (!Obj.getFlag(WRITE)
				&& (!Obj.getFlag(WRITEONCE) || Obj.getAttribute(TEXTSTRING) != 0))) {
		SendResult(Connection, NOTACCESSIBLE);
		return;
	}

	char Text[4096];
	Buffer.readString(Text, sizeof(Text));
	if (strlen(Text) >= Obj.getAttribute(Obj.getFlag(WRITE) ? MAXLENGTH : MAXLENGTHONCE)) {
		SendResult(Connection, NOROOM);
		return;
	}

	TPlayer *pl = Connection->GetPlayer();
	if (pl == nullptr) {
		return;
	}

	try {
		EditText(pl->ID, Obj, Text);
	} catch (RESULT r) {
		SendResult(Connection, r);
	}
}

// 0x080da1e0  receiving.c:1147
void CEditList(TConnection *Connection, TReadBuffer &Buffer) {
	if (Connection == nullptr) {
		error("CEditList: Connection ist NULL.\n");
		return;
	}

	unsigned char Type = Buffer.readByte();
	uint32_t ID = Buffer.readQuad();
	char Text[4096];
	Buffer.readString(Text, sizeof(Text));

	Object Door;
	TPlayer *pl = Connection->GetPlayer();
	if (pl == nullptr) {
		return;
	}

	try {
		switch (Type) {
			case 1: {
				ChangeGuests((unsigned short)ID, pl, Text);
				break;
			}

			case 2: {
				ChangeSubowners((unsigned short)ID, pl, Text);
				break;
			}

			case 3: {
				Door = Object(ID);
				if (!Door.exists() || !Door.getFlag(NAMEDOOR) || !Door.getFlag(TEXT)) {
					SendResult(Connection, NOTACCESSIBLE);
					return;
				}
				ChangeNameDoor(Door, pl, Text);
				break;
			}

			default: {
				error("CEditList: Unbekannter Typ %d.\n", Type);
				break;
			}
		}
	} catch (RESULT r) {
		SendResult(Connection, r);
	}
}

// 0x080da410  receiving.c:1183
void CSetTactics(TConnection *Connection, TReadBuffer &Buffer) {
	if (Connection == nullptr) {
		error("CSetTactics: Connection ist NULL.\n");
		return;
	}

	int AttackMode = Buffer.readByte();
	int ChaseMode = Buffer.readByte();
	int SecureMode = Buffer.readByte();

	if (!in(AttackMode, 1, 3)) {
		print(3, "CSetTactics: Ung\xfcltiger Angriffsmodus %d.\n", AttackMode);
		return;
	}

	if (!in(ChaseMode, 0, 1)) {
		print(3, "CSetTactics: Ung\xfcltiger Verfolgungsmodus %d.\n", ChaseMode);
		return;
	}

	if (!in(SecureMode, 0, 1)) {
		print(3, "CSetTactics: Ung\xfcltiger Sicherheitsmodus %d.\n", SecureMode);
		return;
	}

	TPlayer *pl = Connection->GetPlayer();
	if (pl == nullptr) {
		return;
	}

	pl->Combat.SetAttackMode((unsigned char)AttackMode);
	pl->Combat.SetChaseMode((unsigned char)ChaseMode);
	pl->Combat.SetSecureMode((unsigned char)SecureMode);
}

// 0x080da540  receiving.c:1216
void CAttack(TConnection *Connection, TReadBuffer &Buffer, bool Follow) {
	if (Connection == nullptr) {
		error("CAttack: Connection ist NULL.\n");
		return;
	}

	TPlayer *pl = Connection->GetPlayer();
	if (pl == nullptr) {
		return;
	}

	uint32_t CreatureID = Buffer.readQuad();
	try {
		pl->Combat.SetAttackDest(CreatureID, Follow);
		pl->ToDoAttack();
		pl->ToDoStart();
	} catch (RESULT r) {
		if (r != NOERROR) {
			SendResult(Connection, r);
			pl->ToDoYield();
		}
	}
}

// 0x080da630  receiving.c:1240
void CInviteToParty(TConnection *Connection, TReadBuffer &Buffer) {
	if (Connection == nullptr) {
		error("CInviteToParty: Connection ist NULL.\n");
		return;
	}

	TPlayer *pl = Connection->GetPlayer();
	if (pl == nullptr) {
		return;
	}

	uint32_t Guest = Buffer.readQuad();
	TCreature *guest = GetCreature(Guest);
	if (guest == nullptr) {
		return;
	}

	if (!CheckVisibility(0xA3, Connection, guest->posx, guest->posy, guest->posz)) {
		return;
	}

	InviteToParty(pl->ID, Guest);
}

// 0x080da770  receiving.c:1258
void CJoinParty(TConnection *Connection, TReadBuffer &Buffer) {
	if (Connection == nullptr) {
		error("CJoinParty: Connection ist NULL.\n");
		return;
	}

	TPlayer *pl = Connection->GetPlayer();
	if (pl == nullptr) {
		return;
	}

	uint32_t Host = Buffer.readQuad();
	JoinParty(pl->ID, Host);
}

// 0x080da7e0  receiving.c:1274
void CRevokeInvitation(TConnection *Connection, TReadBuffer &Buffer) {
	if (Connection == nullptr) {
		error("CRevokeInvitation: Connection ist NULL.\n");
		return;
	}

	TPlayer *pl = Connection->GetPlayer();
	if (pl == nullptr) {
		return;
	}

	uint32_t Guest = Buffer.readQuad();
	if (pl->GetPartyLeader(false) == 0) {
		print(3, "Spieler ist kein Mitglied einer Jagdgruppe.\n");
		return;
	}

	RevokeInvitation(pl->ID, Guest);
}

// 0x080da890  receiving.c:1294
void CPassLeadership(TConnection *Connection, TReadBuffer &Buffer) {
	if (Connection == nullptr) {
		error("CPassLeadership: Connection ist NULL.\n");
		return;
	}

	TPlayer *pl = Connection->GetPlayer();
	if (pl == nullptr) {
		return;
	}

	uint32_t NewLeader = Buffer.readQuad();
	if (pl->GetPartyLeader(false) == 0) {
		print(3, "Spieler ist kein Mitglied einer Jagdgruppe.\n");
		return;
	}

	PassLeadership(pl->ID, NewLeader);
}

// 0x080da940  receiving.c:1314
void CLeaveParty(TConnection *Connection) {
	if (Connection == nullptr) {
		error("CLeaveParty: Connection ist NULL.\n");
		return;
	}

	TPlayer *pl = Connection->GetPlayer();
	if (pl == nullptr) {
		return;
	}

	if (pl->GetPartyLeader(false) == 0) {
		print(3, "Spieler ist kein Mitglied einer Jagdgruppe.\n");
		return;
	}

	LeaveParty(pl->ID, false);
}

// 0x080da9c0  receiving.c:1333
void COpenChannel(TConnection *Connection) {
	if (Connection == nullptr) {
		error("COpenChannel: Connection ist NULL.\n");
		return;
	}

	TPlayer *pl = Connection->GetPlayer();
	if (pl == nullptr) {
		return;
	}

	try {
		OpenChannel(pl->ID);
	} catch (RESULT r) {
		SendResult(Connection, r);
	}
}

// 0x080daa50  receiving.c:1352
void CInviteToChannel(TConnection *Connection, TReadBuffer &Buffer) {
	if (Connection == nullptr) {
		error("CInviteToChannel: Connection ist NULL.\n");
		return;
	}

	TPlayer *pl = Connection->GetPlayer();
	if (pl == nullptr) {
		return;
	}

	char PlayerName[30];
	Buffer.readString(PlayerName, sizeof(PlayerName));
	if (PlayerName[0] == '\0') {
		print(2, "CInviteToChannel: Kein Name angegeben.\n");
		return;
	}

	try {
		InviteToChannel(pl->ID, PlayerName);
	} catch (RESULT r) {
		SendResult(Connection, r);
	}
}

// 0x080dab30  receiving.c:1377
void CExcludeFromChannel(TConnection *Connection, TReadBuffer &Buffer) {
	if (Connection == nullptr) {
		error("CExcludeFromChannel: Connection ist NULL.\n");
		return;
	}

	TPlayer *pl = Connection->GetPlayer();
	if (pl == nullptr) {
		return;
	}

	char PlayerName[30];
	Buffer.readString(PlayerName, sizeof(PlayerName));
	try {
		ExcludeFromChannel(pl->ID, PlayerName);
	} catch (RESULT r) {
		SendResult(Connection, r);
	}
}

// 0x080dabf0  receiving.c:1398
void CCancel(TConnection *Connection) {
	if (Connection == nullptr) {
		error("CCancel: Connection ist NULL.\n");
		return;
	}

	TPlayer *pl = Connection->GetPlayer();
	if (pl == nullptr) {
		return;
	}

	try {
		pl->Combat.StopAttack(0);
		if (pl->ToDoClear()) {
			SendSnapback(Connection);
		}
	} catch (RESULT) {
	}

	pl->ToDoYield();
}

// 0x080dac90  receiving.c:1417
void CAddBuddy(TConnection *Connection, TReadBuffer &Buffer) {
	if (Connection == nullptr) {
		error("CAddBuddy: Connection ist NULL.\n");
		return;
	}

	char Name[30];
	Buffer.readString(Name, sizeof(Name));
	if (Name[0] == '\0') {
		return;
	}

	TPlayer *pl = Connection->GetPlayer();
	if (pl != nullptr) {
		pl->AddBuddy(Name);
	}
}

// 0x080dad20  receiving.c:1435
void CRemoveBuddy(TConnection *Connection, TReadBuffer &Buffer) {
	if (Connection == nullptr) {
		error("CRemoveBuddy: Connection ist NULL.\n");
		return;
	}

	uint32_t CharacterID = Buffer.readQuad();
	TPlayer *pl = Connection->GetPlayer();
	if (pl != nullptr) {
		pl->RemoveBuddy(CharacterID);
	}
}

// 0x080dad90  receiving.c:1451
void CRuleViolation(TConnection *Connection, TReadBuffer &Buffer) {
	if (Connection == nullptr) {
		error("CRuleViolation: Connection ist NULL.\n");
		return;
	}

	TPlayer *Gamemaster = Connection->GetPlayer();
	if (Gamemaster == nullptr) {
		return;
	}

	char Name[30], Comment[200];
	Buffer.readString(Name, sizeof(Name));
	Trim(Name);
	int Reason = Buffer.readByte();
	int Action = Buffer.readByte();
	Buffer.readString(Comment, sizeof(Comment));
	uint32_t StatementID = Buffer.readQuad();
	bool IPBanishment = (Buffer.readByte() != 0);

	if (!CheckBanishmentRight(Gamemaster->ID, Reason, Action)
			|| (IPBanishment && !CheckRight(Gamemaster->ID, IP_BANISHMENT))) {
		SendMessage(Connection, TALK_FAILURE_MESSAGE, "You have no authorization for this action.");
		return;
	}

	if (Comment[0] == '\0') {
		SendMessage(Connection, TALK_FAILURE_MESSAGE, "Please provide a comment for this report.");
		return;
	}

	if (Action == 6 && StatementID == 0) {
		SendMessage(Connection, TALK_FAILURE_MESSAGE, "Please provide a statement for this rule violation.");
		return;
	}

	TPlayer *Criminal = nullptr;
	char *IPAddress = nullptr;
	if (IdentifyPlayer(Name, false, !CheckRight(Gamemaster->ID, READ_GAMEMASTER_CHANNEL), Criminal) == 0) {
		uint32_t CriminalID = Criminal->ID;
		if (CheckRight(CriminalID, NO_BANISHMENT)) {
			if (Action == 1 || Action == 3 || Action == 5) {
				SendMessage(Connection, TALK_FAILURE_MESSAGE, "This name has already been approved.");
			} else if (Action == 6) {
				SendMessage(Connection, TALK_FAILURE_MESSAGE, "Statement has already been reported.");
			} else {
				SendMessage(Connection, TALK_FAILURE_MESSAGE, "You may not report a god or gamemaster.");
			}
			return;
		}

		strcpy(Name, Criminal->Name);
		IPAddress = Criminal->IPAddress;
	} else {
		if (GetCharacterID(Name) == 0) {
			SendResult(Connection, PLAYERNOTEXISTING);
			return;
		}

		strcpy(Name, GetCharacterName(Name));
		if (IPBanishment) {
			SendMessage(Connection, TALK_FAILURE_MESSAGE, "Player is not online. No IP address was banished.");
			IPBanishment = false;
		}
	}

	int NumberOfStatements = 0;
	vector<TReportedStatement> *ReportedStatements = nullptr;
	if ((in(Reason + FIRST_BANISHMENT_RIGHT, FIRST_REPORT_RIGHT, LAST_REPORT_RIGHT) || Reason == 29)
			&& StatementID != 0) {
		int Result = GetCommunicationContext((Action == 6 ? 0 : Gamemaster->ID),
				StatementID, NumberOfStatements, ReportedStatements);
		if (Result == 1) {
			SendMessage(Connection, TALK_FAILURE_MESSAGE, "Statement is unknown. Perhaps it is too old?");
			return;
		}
		if (Result == 2) {
			SendMessage(Connection, TALK_FAILURE_MESSAGE, "Statement has already been reported.");
			return;
		}
	}

	PunishmentOrder(Gamemaster, Name, IPAddress, Reason, Action, Comment,
			NumberOfStatements, ReportedStatements, StatementID, IPBanishment);
}

// 0x080db170  receiving.c:1528
void CQuitGame(TConnection *Connection) {
	TPlayer *pl = Connection->GetPlayer();
	if (pl != nullptr) {
		switch (pl->LogoutPossible()) {
			case 0: {
				break;
			}

			case 1: {
				SendMessage(Connection, TALK_FAILURE_MESSAGE, "You may not logout during or immediately after a fight!");
				return;
			}

			case 2: {
				SendMessage(Connection, TALK_FAILURE_MESSAGE, "You may not logout here!");
				return;
			}

			default: {
				error("CQuitGame: Ung\xfcltiger R\xfc" "ckgabewert von LogoutPossible.\n");
				break;
			}
		}

		pl->ToDoClear();
	}

	Connection->Logout(0, true);
}

// 0x080db210  receiving.c:1551
void ReceiveData(TConnection *Connection) {
	if (Connection == nullptr) {
		error("ReceiveData: Connection ist NULL.\n");
		return;
	}

	int Length = Connection->InDataSize;
	if (Length <= 0) {
		error("ReceiveData: Keine Daten vorhanden.\n");
		return;
	}

	if (Length > (int)sizeof(Connection->InData) - 2) {
		error("ReceiveData: Zu gro\xdf" "e Paketl\xe4nge %d.\n", Length);
		return;
	}

	TReadBuffer Buffer(Connection->InData + 2, Length);
	int Command;
	try {
		Command = Buffer.readByte();
	} catch (const char *s) {
		error("ReceiveData: Fehler beim Auslesen des Kommandos (%s).\n", s);
		return;
	}

	if (Connection->GetState() == CONNECTION_LOGIN) {
		if (Command != 0x0B) {
			error("ReceiveData: Falsches Login-Kommando %d.\n", Command);
			return;
		}

		if (!Connection->JoinGame(Buffer)) {
			Log("game", "Einloggen gescheitert.\n");
			SendResult(Connection, LOGINERROR);
			TPlayer *pl = Connection->GetPlayer();
			if (pl != nullptr) {
				DecrementIsOnlineOrder(pl->ID);
			} else {
				error("ReceiveData: Einloggen gescheitert und keine Kreatur bekannt.\n");
			}
			Connection->Disconnect();
		}
		return;
	}

	if (!CommandAllowed(Connection, Command)) {
		return;
	}

	Connection->ResetTimer(Command);

	try {
		switch (Command) {
			case 0x14: CQuitGame(Connection); break;
			case 0x1E: break;
			case 0x64: CGoPath(Connection, Buffer); break;
			case 0x65: CGoDirection(Connection, 0, -1); break;
			case 0x66: CGoDirection(Connection, 1, 0); break;
			case 0x67: CGoDirection(Connection, 0, 1); break;
			case 0x68: CGoDirection(Connection, -1, 0); break;
			case 0x69: CStop(Connection); break;
			case 0x6A: CGoDirection(Connection, 1, -1); break;
			case 0x6B: CGoDirection(Connection, 1, 1); break;
			case 0x6C: CGoDirection(Connection, -1, 1); break;
			case 0x6D: CGoDirection(Connection, -1, -1); break;
			case 0x6F: CRotate(Connection, 0); break;
			case 0x70: CRotate(Connection, 1); break;
			case 0x71: CRotate(Connection, 2); break;
			case 0x72: CRotate(Connection, 3); break;
			case 0x78: CMoveObject(Connection, Buffer); break;
			case 0x7D: CTradeObject(Connection, Buffer); break;
			case 0x7E: CInspectTrade(Connection, Buffer); break;
			case 0x7F: CAcceptTrade(Connection); break;
			case 0x80: CRejectTrade(Connection); break;
			case 0x82: CUseObject(Connection, Buffer); break;
			case 0x83: CUseTwoObjects(Connection, Buffer); break;
			case 0x84: CUseOnCreature(Connection, Buffer); break;
			case 0x85: CTurnObject(Connection, Buffer); break;
			case 0x87: CCloseContainer(Connection, Buffer); break;
			case 0x88: CUpContainer(Connection, Buffer); break;
			case 0x89: CEditText(Connection, Buffer); break;
			case 0x8A: CEditList(Connection, Buffer); break;
			case 0x8C: CLookAtPoint(Connection, Buffer); break;
			case 0x96: CTalk(Connection, Buffer); break;
			case 0x97: CGetChannels(Connection); break;
			case 0x98: CJoinChannel(Connection, Buffer); break;
			case 0x99: CLeaveChannel(Connection, Buffer); break;
			case 0x9A: CPrivateChannel(Connection, Buffer); break;
			case 0x9B: CProcessRequest(Connection, Buffer); break;
			case 0x9C: CRemoveRequest(Connection, Buffer); break;
			case 0x9D: CCancelRequest(Connection); break;
			case 0xA0: CSetTactics(Connection, Buffer); break;
			case 0xA1: CAttack(Connection, Buffer, false); break;
			case 0xA2: CAttack(Connection, Buffer, true); break;
			case 0xA3: CInviteToParty(Connection, Buffer); break;
			case 0xA4: CJoinParty(Connection, Buffer); break;
			case 0xA5: CRevokeInvitation(Connection, Buffer); break;
			case 0xA6: CPassLeadership(Connection, Buffer); break;
			case 0xA7: CLeaveParty(Connection); break;
			case 0xAA: COpenChannel(Connection); break;
			case 0xAB: CInviteToChannel(Connection, Buffer); break;
			case 0xAC: CExcludeFromChannel(Connection, Buffer); break;
			case 0xBE: CCancel(Connection); break;
			case 0xC9: CRefreshField(Connection, Buffer); break;
			case 0xCA: CRefreshContainer(Connection, Buffer); break;
			case 0xD2: CGetOutfit(Connection); break;
			case 0xD3: CSetOutfit(Connection, Buffer); break;
			case 0xDC: CAddBuddy(Connection, Buffer); break;
			case 0xDD: CRemoveBuddy(Connection, Buffer); break;
			case 0xE6: CBugReport(Connection, Buffer); break;
			case 0xE7: CRuleViolation(Connection, Buffer); break;
			case 0xE8: CErrorFileEntry(Connection, Buffer); break;
			default: {
				print(3, "Unbekanntes Kommando %d.\n", Command);
				break;
			}
		}
	} catch (const char *s) {
		print(3, "Fehler beim Auslesen der Daten f\xfcr Kommando %d (%s).\n", Command, s);
	}
}

// 0x080db990  receiving.c:1667
void ReceiveData() {
	TConnection *Connection = GetFirstConnection();
	while (Connection != nullptr) {
		if (Connection->IsOnline() && Connection->WaitingForACK) {
			ReceiveData(Connection);
			Connection->WaitingForACK = false;
			if (Connection->IsOnline()) {
				tgkill(getpid(), Connection->GetPID(), SIGUSR1);
			}
		}
		Connection = GetNextConnection();
	}
}
