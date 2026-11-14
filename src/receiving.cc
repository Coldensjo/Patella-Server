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

bool CommandAllowed(TConnection *Connection, unsigned short Command) {
	if (Connection == nullptr) {
		error("CommandAllowed: Connection ist NULL.\n");
		return false;
	}

	int State = Connection->GetState();
	if (State == CONNECTION_GAME) {
		return Command != CL_LOGIN && Command != CL_JOIN_GAME;
	}
	if (State == CONNECTION_DEAD || State == CONNECTION_LOGOUT) {
		return Command == CL_BUG_REPORT || Command == CL_ERROR_FILE_ENTRY || Command == CL_PING
				|| Command == CL_QUIT_GAME;
	}

	error("CommandAllowed: Ung\xfcltiger Verbindungsstatus %d bei Kommando %d.\n", State, Command);
	return false;
}

bool CheckSpecialCoordinates(int Command, int x, int y, int z, bool AnyAllowed) {
	if (x != POSITION_NOT_ON_MAP) {
		return IsOnMap(x, y, z);
	}

	if (y == 0) {
		return AnyAllowed;
	}

	if (y <= LAST_BODY_POSITION
			|| (y >= POSITION_CONTAINER_BASE && y <= POSITION_CONTAINER_BASE + MAX_OPEN_CONTAINERS - 1)) {
		return true;
	}

	print(3, "CheckSpecialCoordinates: Ung\xfcltige Koordinaten [%d,%d,%d] bei Kommando %d.\n", x, y, z, Command);
	return false;
}

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

bool CheckObjectType(int Command, int TypeID) {
	if (!ObjectTypeExists(TypeID)) {
		print(3, "CheckObjectType: Ung\xfcltiger Objekttyp %d bei Kommando %d.\n", TypeID, Command);
		return false;
	}
	return true;
}

ObjectType GetObjectType(unsigned short TypeID) {
	if (!CheckObjectType(CL_MOVE_OBJECT, TypeID)) {
		return ObjectType(0);
	}
	return ObjectType(TypeID);
}

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
				case COMPASS_EAST: x++; break;
				case COMPASS_NORTHEAST: x++; y--; break;
				case COMPASS_NORTH: y--; break;
				case COMPASS_NORTHWEST: x--; y--; break;
				case COMPASS_WEST: x--; break;
				case COMPASS_SOUTHWEST: x--; y++; break;
				case COMPASS_SOUTH: y++; break;
				case COMPASS_SOUTHEAST: x++; y++; break;
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

void CLookAtPoint(TConnection *Connection, TReadBuffer &Buffer) {
	if (Connection == nullptr) {
		error("CLookAtPoint: Connection ist NULL.\n");
		return;
	}

	int x = Buffer.readWord();
	int y = Buffer.readWord();
	int z = Buffer.readByte();
	if (!CheckSpecialCoordinates(CL_LOOK_AT_POINT, x, y, z, false)) {
		return;
	}

	if (x != POSITION_NOT_ON_MAP && !CheckVisibility(CL_LOOK_AT_POINT, Connection, x, y, z)) {
		return;
	}

	TPlayer *pl = Connection->GetPlayer();
	if (pl == nullptr) {
		return;
	}

	uint32_t CreatureID = pl->ID;
	Object Obj = GetObject(CreatureID, x, y, z, (x == POSITION_NOT_ON_MAP ? z : -1), ObjectType(0));
	if (!Obj.exists()) {
		return;
	}

	try {
		Look(CreatureID, Obj);
	} catch (RESULT r) {
		SendResult(Connection, r);
	}
}

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

	if (!CheckVisibility(CL_REFRESH_FIELD, Connection, x, y, z)) {
		return;
	}

	SendFieldData(Connection, x, y, z);
}

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

	if (Mode == TALK_YELL && pl->Skills[SKILL_LEVEL]->Get() < 2) {
		SendMessage(Connection, TALK_FAILURE_MESSAGE, "You may not yell as long as you are on level 1.");
		return;
	}

	if (Mode == TALK_CHANNEL_CALL && pl->Skills[SKILL_LEVEL]->Get() < 2) {
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

void CGetChannels(TConnection *Connection) {
	if (Connection == nullptr) {
		error("CGetChannels: Connection ist NULL.\n");
		return;
	}

	SendChannels(Connection);
}

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

void CRotate(TConnection *Connection, int Direction) {
	if (Connection == nullptr) {
		error("CRotate: Connection ist NULL.\n");
		return;
	}

	if (Direction < DIRECTION_NORTH || Direction > DIRECTION_WEST) {
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
	if (sx != POSITION_NOT_ON_MAP) {
		RNum = 1;
	}

	if (!CheckSpecialCoordinates(CL_MOVE_OBJECT, sx, sy, sz, false)) {
		return;
	}

	if (sx != POSITION_NOT_ON_MAP && !CheckVisibility(CL_MOVE_OBJECT, Connection, sx, sy, sz)) {
		return;
	}

	if (!CheckSpecialCoordinates(CL_MOVE_OBJECT, zx, zy, zz, true)) {
		return;
	}

	if (zx != POSITION_NOT_ON_MAP && !CheckVisibility(CL_MOVE_OBJECT, Connection, zx, zy, zz)) {
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
	if (x != POSITION_NOT_ON_MAP) {
		RNum = 1;
	}

	if (!CheckSpecialCoordinates(CL_TRADE_OBJECT, x, y, z, false)) {
		return;
	}

	if (x != POSITION_NOT_ON_MAP && !CheckVisibility(CL_TRADE_OBJECT, Connection, x, y, z)) {
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

	if (!CheckSpecialCoordinates(CL_USE_OBJECT, x, y, z, false)) {
		return;
	}

	if (x != POSITION_NOT_ON_MAP && !CheckVisibility(CL_USE_OBJECT, Connection, x, y, z)) {
		return;
	}

	if (Type == 0) {
		return;
	}

	if (Type.getFlag(MULTIUSE)) {
		return;
	}

	if (Type.getFlag(CONTAINER) && Dummy > MAX_OPEN_CONTAINERS - 1) {
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

	if (!CheckSpecialCoordinates(CL_USE_TWO_OBJECTS, x1, y1, z1, false)) {
		return;
	}

	if (x1 != POSITION_NOT_ON_MAP && !CheckVisibility(CL_USE_TWO_OBJECTS, Connection, x1, y1, z1)) {
		return;
	}

	if (Type1 == 0) {
		return;
	}

	if (!Type1.getFlag(MULTIUSE)) {
		return;
	}

	if (!CheckSpecialCoordinates(CL_USE_TWO_OBJECTS, x2, y2, z2, false)) {
		return;
	}

	if (x2 != POSITION_NOT_ON_MAP && !CheckVisibility(CL_USE_TWO_OBJECTS, Connection, x2, y2, z2)) {
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

	if (!CheckSpecialCoordinates(CL_USE_ON_CREATURE, x, y, z, false)) {
		return;
	}

	if (x != POSITION_NOT_ON_MAP && !CheckVisibility(CL_USE_ON_CREATURE, Connection, x, y, z)) {
		return;
	}

	if (Type == 0) {
		return;
	}

	TCreature *Dest = GetCreature(CreatureID);
	if (Dest == nullptr) {
		return;
	}

	if (!CheckVisibility(CL_USE_ON_CREATURE, Connection, Dest->posx, Dest->posy, Dest->posz)) {
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

	if (!CheckSpecialCoordinates(CL_TURN_OBJECT, x, y, z, false)) {
		return;
	}

	if (x != POSITION_NOT_ON_MAP && !CheckVisibility(CL_TURN_OBJECT, Connection, x, y, z)) {
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

void CCloseContainer(TConnection *Connection, TReadBuffer &Buffer) {
	if (Connection == nullptr) {
		error("CCloseContainer: Connection ist NULL.\n");
		return;
	}

	int Window = Buffer.readByte();
	if (Window > MAX_OPEN_CONTAINERS - 1) {
		return;
	}

	TPlayer *pl = Connection->GetPlayer();
	if (pl == nullptr) {
		return;
	}

	pl->SetOpenContainer(Window, NONE);
	SendCloseContainer(Connection, Window);
}

void CUpContainer(TConnection *Connection, TReadBuffer &Buffer) {
	if (Connection == nullptr) {
		error("CUpContainer: Connection ist NULL.\n");
		return;
	}

	int Window = Buffer.readByte();
	if (Window > MAX_OPEN_CONTAINERS - 1) {
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

void CRefreshContainer(TConnection *Connection, TReadBuffer &Buffer) {
	if (Connection == nullptr) {
		error("CRefreshContainer: Connection ist NULL.\n");
		return;
	}

	int Window = Buffer.readByte();
	if (Window > MAX_OPEN_CONTAINERS - 1) {
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

	if (pl->Sex == SEX_MALE) {
		if (!in(OutfitID, FIRST_MALE_OUTFIT, LAST_MALE_OUTFIT)
				|| (!CheckRight(pl->ID, PREMIUM_ACCOUNT) && OutfitID > LAST_FREE_MALE_OUTFIT)) {
			print(3, "CSetOutfit: Ung\xfcltiges Outfit %d f\xfcr M\xe4nner.\n", OutfitID);
			return;
		}
	}

	if (pl->Sex == SEX_FEMALE) {
		if (!in(OutfitID, FIRST_FEMALE_OUTFIT, LAST_FEMALE_OUTFIT)
				|| (!CheckRight(pl->ID, PREMIUM_ACCOUNT) && OutfitID > LAST_FREE_FEMALE_OUTFIT)) {
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
		AnnounceChangedCreature(pl->ID, CREATURE_OUTFIT);
	}
	pl->OrgOutfit = NewOutfit;
}

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
			case EDIT_LIST_HOUSE_GUESTS: {
				ChangeGuests((unsigned short)ID, pl, Text);
				break;
			}

			case EDIT_LIST_HOUSE_SUBOWNERS: {
				ChangeSubowners((unsigned short)ID, pl, Text);
				break;
			}

			case EDIT_LIST_NAME_DOOR: {
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

void CSetTactics(TConnection *Connection, TReadBuffer &Buffer) {
	if (Connection == nullptr) {
		error("CSetTactics: Connection ist NULL.\n");
		return;
	}

	int AttackMode = Buffer.readByte();
	int ChaseMode = Buffer.readByte();
	int SecureMode = Buffer.readByte();

	if (!in(AttackMode, ATTACK_MODE_OFFENSIVE, ATTACK_MODE_DEFENSIVE)) {
		print(3, "CSetTactics: Ung\xfcltiger Angriffsmodus %d.\n", AttackMode);
		return;
	}

	if (!in(ChaseMode, CHASE_MODE_STAND, CHASE_MODE_FOLLOW)) {
		print(3, "CSetTactics: Ung\xfcltiger Verfolgungsmodus %d.\n", ChaseMode);
		return;
	}

	if (!in(SecureMode, SECURE_MODE_OFF, SECURE_MODE_ON)) {
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

	if (!CheckVisibility(CL_INVITE_TO_PARTY, Connection, guest->posx, guest->posy, guest->posz)) {
		return;
	}

	InviteToParty(pl->ID, Guest);
}

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

	if (Action == BANISHMENT_ACTION_STATEMENT_REPORT && StatementID == 0) {
		SendMessage(Connection, TALK_FAILURE_MESSAGE, "Please provide a statement for this rule violation.");
		return;
	}

	TPlayer *Criminal = nullptr;
	char *IPAddress = nullptr;
	if (IdentifyPlayer(Name, false, !CheckRight(Gamemaster->ID, READ_GAMEMASTER_CHANNEL), Criminal) == 0) {
		uint32_t CriminalID = Criminal->ID;
		if (CheckRight(CriminalID, NO_BANISHMENT)) {
			if (Action == BANISHMENT_ACTION_NAMELOCK || Action == BANISHMENT_ACTION_NAMELOCK_BANISHMENT
					|| Action == BANISHMENT_ACTION_NAMELOCK_BANISHMENT_FINAL_WARNING) {
				SendMessage(Connection, TALK_FAILURE_MESSAGE, "This name has already been approved.");
			} else if (Action == BANISHMENT_ACTION_STATEMENT_REPORT) {
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
	if ((in(Reason + FIRST_BANISHMENT_RIGHT, FIRST_REPORT_RIGHT, LAST_REPORT_RIGHT)
				|| Reason == DESTRUCTIVE_BEHAVIOUR - FIRST_BANISHMENT_RIGHT)
			&& StatementID != 0) {
		int Result = GetCommunicationContext((Action == BANISHMENT_ACTION_STATEMENT_REPORT ? 0 : Gamemaster->ID),
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

void CQuitGame(TConnection *Connection) {
	TPlayer *pl = Connection->GetPlayer();
	if (pl != nullptr) {
		switch (pl->LogoutPossible()) {
			case LOGOUT_POSSIBLE: {
				break;
			}

			case LOGOUT_AFTER_FIGHT: {
				SendMessage(Connection, TALK_FAILURE_MESSAGE, "You may not logout during or immediately after a fight!");
				return;
			}

			case LOGOUT_NO_LOGOUT_FIELD: {
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
		if (Command != CL_JOIN_GAME) {
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
			case CL_QUIT_GAME: CQuitGame(Connection); break;
			case CL_PING: break;
			case CL_GO_PATH: CGoPath(Connection, Buffer); break;
			case CL_GO_NORTH: CGoDirection(Connection, 0, -1); break;
			case CL_GO_EAST: CGoDirection(Connection, 1, 0); break;
			case CL_GO_SOUTH: CGoDirection(Connection, 0, 1); break;
			case CL_GO_WEST: CGoDirection(Connection, -1, 0); break;
			case CL_STOP: CStop(Connection); break;
			case CL_GO_NORTHEAST: CGoDirection(Connection, 1, -1); break;
			case CL_GO_SOUTHEAST: CGoDirection(Connection, 1, 1); break;
			case CL_GO_SOUTHWEST: CGoDirection(Connection, -1, 1); break;
			case CL_GO_NORTHWEST: CGoDirection(Connection, -1, -1); break;
			case CL_ROTATE_NORTH: CRotate(Connection, DIRECTION_NORTH); break;
			case CL_ROTATE_EAST: CRotate(Connection, DIRECTION_EAST); break;
			case CL_ROTATE_SOUTH: CRotate(Connection, DIRECTION_SOUTH); break;
			case CL_ROTATE_WEST: CRotate(Connection, DIRECTION_WEST); break;
			case CL_MOVE_OBJECT: CMoveObject(Connection, Buffer); break;
			case CL_TRADE_OBJECT: CTradeObject(Connection, Buffer); break;
			case CL_INSPECT_TRADE: CInspectTrade(Connection, Buffer); break;
			case CL_ACCEPT_TRADE: CAcceptTrade(Connection); break;
			case CL_REJECT_TRADE: CRejectTrade(Connection); break;
			case CL_USE_OBJECT: CUseObject(Connection, Buffer); break;
			case CL_USE_TWO_OBJECTS: CUseTwoObjects(Connection, Buffer); break;
			case CL_USE_ON_CREATURE: CUseOnCreature(Connection, Buffer); break;
			case CL_TURN_OBJECT: CTurnObject(Connection, Buffer); break;
			case CL_CLOSE_CONTAINER: CCloseContainer(Connection, Buffer); break;
			case CL_UP_CONTAINER: CUpContainer(Connection, Buffer); break;
			case CL_EDIT_TEXT: CEditText(Connection, Buffer); break;
			case CL_EDIT_LIST: CEditList(Connection, Buffer); break;
			case CL_LOOK_AT_POINT: CLookAtPoint(Connection, Buffer); break;
			case CL_TALK: CTalk(Connection, Buffer); break;
			case CL_GET_CHANNELS: CGetChannels(Connection); break;
			case CL_JOIN_CHANNEL: CJoinChannel(Connection, Buffer); break;
			case CL_LEAVE_CHANNEL: CLeaveChannel(Connection, Buffer); break;
			case CL_PRIVATE_CHANNEL: CPrivateChannel(Connection, Buffer); break;
			case CL_PROCESS_REQUEST: CProcessRequest(Connection, Buffer); break;
			case CL_REMOVE_REQUEST: CRemoveRequest(Connection, Buffer); break;
			case CL_CANCEL_REQUEST: CCancelRequest(Connection); break;
			case CL_SET_TACTICS: CSetTactics(Connection, Buffer); break;
			case CL_ATTACK: CAttack(Connection, Buffer, false); break;
			case CL_FOLLOW: CAttack(Connection, Buffer, true); break;
			case CL_INVITE_TO_PARTY: CInviteToParty(Connection, Buffer); break;
			case CL_JOIN_PARTY: CJoinParty(Connection, Buffer); break;
			case CL_REVOKE_INVITATION: CRevokeInvitation(Connection, Buffer); break;
			case CL_PASS_LEADERSHIP: CPassLeadership(Connection, Buffer); break;
			case CL_LEAVE_PARTY: CLeaveParty(Connection); break;
			case CL_OPEN_CHANNEL: COpenChannel(Connection); break;
			case CL_INVITE_TO_CHANNEL: CInviteToChannel(Connection, Buffer); break;
			case CL_EXCLUDE_FROM_CHANNEL: CExcludeFromChannel(Connection, Buffer); break;
			case CL_CANCEL: CCancel(Connection); break;
			case CL_REFRESH_FIELD: CRefreshField(Connection, Buffer); break;
			case CL_REFRESH_CONTAINER: CRefreshContainer(Connection, Buffer); break;
			case CL_GET_OUTFIT: CGetOutfit(Connection); break;
			case CL_SET_OUTFIT: CSetOutfit(Connection, Buffer); break;
			case CL_ADD_BUDDY: CAddBuddy(Connection, Buffer); break;
			case CL_REMOVE_BUDDY: CRemoveBuddy(Connection, Buffer); break;
			case CL_BUG_REPORT: CBugReport(Connection, Buffer); break;
			case CL_RULE_VIOLATION: CRuleViolation(Connection, Buffer); break;
			case CL_ERROR_FILE_ENTRY: CErrorFileEntry(Connection, Buffer); break;
			default: {
				print(3, "Unbekanntes Kommando %d.\n", Command);
				break;
			}
		}
	} catch (const char *s) {
		print(3, "Fehler beim Auslesen der Daten f\xfcr Kommando %d (%s).\n", Command, s);
	}
}

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
