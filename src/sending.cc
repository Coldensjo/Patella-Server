#include <algorithm>
#include <csignal>
#include <cstdarg>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
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
#include "magic.hh"
#include "operate.hh"
#include "connections.hh"
#include "utils.hh"
#include "time.hh"
#include "TXTEASymmetricKey.hh"
#include "communication.hh"
#include "sending.hh"

// SendRights sends a byte for each banishment reason: bit N set allows action
// N (0 to 6, CheckBanishmentRight), and this bit an IP banishment besides.
const int RIGHTS_IP_BANISHMENT = 0x80;

// The channel number of the "Private Chat Channel" entry SendChannels lists for
// a player who may open his own channel (MayOpenChannel); no CHANNEL has it.
const int PRIVATE_CHAT_CHANNEL = 0xFFFF;

const int OBJECTS_PER_POINT = 10;

static const int OBJECTS_PER_CONTAINER = 36;
static int Skip = -1;

static inline void SendByte(TConnection *Connection, unsigned char Value) {
	if (Connection->NextToWrite < Connection->NextToSend + (int)sizeof(Connection->OutData)) {
		Connection->OutData[Connection->NextToWrite % (int)sizeof(Connection->OutData)] = Value;
		Connection->NextToWrite++;
	} else {
		Connection->Overflow = true;
	}
}

static inline void SendWord(TConnection *Connection, unsigned short Value) {
	if (Connection->NextToWrite < Connection->NextToSend + (int)sizeof(Connection->OutData) - 1) {
		Connection->OutData[Connection->NextToWrite % (int)sizeof(Connection->OutData)] = (unsigned char)Value;
		Connection->OutData[(Connection->NextToWrite + 1) % (int)sizeof(Connection->OutData)] = (unsigned char)(Value >> 8);
		Connection->NextToWrite += 2;
	} else {
		Connection->Overflow = true;
	}
}

static inline void SendQuad(TConnection *Connection, uint32_t Value) {
	if (Connection->NextToWrite < Connection->NextToSend + (int)sizeof(Connection->OutData) - 3) {
		Connection->OutData[Connection->NextToWrite % (int)sizeof(Connection->OutData)] = (unsigned char)Value;
		Connection->OutData[(Connection->NextToWrite + 1) % (int)sizeof(Connection->OutData)] = (unsigned char)(Value >> 8);
		Connection->OutData[(Connection->NextToWrite + 2) % (int)sizeof(Connection->OutData)] = (unsigned char)(Value >> 16);
		Connection->OutData[(Connection->NextToWrite + 3) % (int)sizeof(Connection->OutData)] = (unsigned char)(Value >> 24);
		Connection->NextToWrite += 4;
	} else {
		Connection->Overflow = true;
	}
}

static inline void SendText(TConnection *Connection, const char *Text, int Length) {
	if (Text == nullptr) {
		error("SendText: Text ist NULL.\n");
		return;
	}

	if (Length <= 0) {
		error("SendText: ung\xfcltige Textl\xe4nge %d.\n", Length);
		return;
	}

	if (Connection->NextToWrite > Connection->NextToSend - Length + (int)sizeof(Connection->OutData)) {
		Connection->Overflow = true;
		return;
	}

	int Offset = Connection->NextToWrite % (int)sizeof(Connection->OutData);
	if (Offset + Length > (int)sizeof(Connection->OutData)) {
		memcpy(&Connection->OutData[Offset], Text, (int)sizeof(Connection->OutData) - Offset);
		memcpy(Connection->OutData, Text + ((int)sizeof(Connection->OutData) - Offset), Offset + Length - (int)sizeof(Connection->OutData));
	} else {
		memcpy(&Connection->OutData[Offset], Text, Length);
	}
	Connection->NextToWrite += Length;
}

static inline void SendString(TConnection *Connection, const char *String) {
	if (String == nullptr) {
		error("SendString: String ist NULL.\n");
		return;
	}

	int Length = (int)strlen(String);
	SendWord(Connection, (unsigned short)Length);
	if (Length > 0) {
		SendText(Connection, String, Length);
	}
}

static inline void SendObjectType(TConnection *Connection, Object Obj) {
	if (Obj.getFlag(DISGUISE)) {
		SendWord(Connection, (unsigned short)Obj.getAttribute(DISGUISETARGET));
	} else {
		SendWord(Connection, (unsigned short)Obj.getObjectType().getTypeID());
	}
}

bool BeginSendData(TConnection *Connection) {
	if (Connection == nullptr) {
		return false;
	}

	if (!Connection->IsOnline() || Connection->GetState() == CONNECTION_LOGIN) {
		return false;
	}

	if (Connection->NextToCommit == Connection->NextToSend + (int)sizeof(Connection->OutData)) {
		print(2, "BeginSendData: Puffer von Verbindung %d ist voll.\n", Connection->GetSocket());
		return false;
	}

	Connection->NextToWrite = Connection->NextToCommit;
	Connection->Overflow = false;
	return true;
}

void FinishSendData(TConnection *Connection) {
	if (Connection == nullptr) {
		error("FinishSendData: Verbindung ist NULL.\n");
		return;
	}

	if (!Connection->IsOnline()) {
		error("FinishSendData: Verbindung ist nicht online.\n");
		return;
	}

	if (Connection->Overflow) {
		print(2, "FinishSendData: Puffer ist voll. Paket wird nicht versandt.\n");
		return;
	}

	Connection->NextToCommit = Connection->NextToWrite;
	if (!Connection->WillingToSend) {
		Connection->NextSendingConnection = FirstSendingConnection;
		FirstSendingConnection = Connection;
		Connection->WillingToSend = true;
	}
}

void SendAll() {
	TConnection *Connection = FirstSendingConnection;
	while (Connection != nullptr) {
		if (!Connection->WillingToSend) {
			error("SendAll: Verbindung ist nicht sendewillig.\n");
		} else {
			Connection->WillingToSend = false;
			if (Connection->IsOnline() && Connection->NextToSend != Connection->NextToCommit) {
				// The original is kill(PID, SIGUSR2) with the LinuxThreads PID of the
				// connection's communication thread.
				tgkill(getpid(), Connection->GetPID(), SIGUSR2);
			}
		}
		Connection = Connection->NextSendingConnection;
	}
	FirstSendingConnection = nullptr;
}

void SkipFlush(TConnection *Connection) {
	if (Skip == -1) {
		return;
	}

	do {
		if (Skip > 255) {
			SendByte(Connection, 0xFF);
			SendByte(Connection, 0xFF);
			Skip -= 256;
		} else {
			SendByte(Connection, (unsigned char)Skip);
			SendByte(Connection, 0xFF);
			Skip = -1;
		}
	} while (Skip >= 0);
}

void SendMapObject(TConnection *Connection, Object Obj) {
	if (!Obj.exists()) {
		error("SendMapObject: \xdc" "bergebenes Objekt existiert nicht.\n");
		return;
	}

	if (Obj.isCreature()) {
		TCreature *cr = GetCreature(Obj);
		if (cr == nullptr) {
			error("SendMapObject: Kreatur hat kein Kreatur-Objekt\n");
			return;
		}

		int HelpBrightness, HelpColor;
		switch (Connection->KnownCreature(cr->ID, true)) {
			case KNOWNCREATURE_FREE: {
				SendByte(Connection, UNKNOWNCREATURE);
				SendByte(Connection, 0x00);
				SendQuad(Connection, Connection->NewKnownCreature(cr->ID));
				SendQuad(Connection, cr->ID);
				if (cr->Type == MONSTER) {
					char *s = strchr(cr->Name, ' ');
					if (s != nullptr) {
						s++;
					} else {
						s = cr->Name;
					}
					SendString(Connection, s);
				} else {
					SendString(Connection, cr->Name);
				}
				break;
			}

			case KNOWNCREATURE_OUTDATED: {
				SendByte(Connection, OUTDATEDCREATURE);
				SendByte(Connection, 0x00);
				SendQuad(Connection, cr->ID);
				break;
			}

			case KNOWNCREATURE_UPTODATE: {
				SendByte(Connection, TYPEID_CREATURE);
				SendByte(Connection, 0x00);
				SendQuad(Connection, cr->ID);
				SendByte(Connection, (unsigned char)cr->Direction);
				return;
			}

			default: {
				return;
			}
		}

		SendByte(Connection, (unsigned char)cr->GetHealth());
		SendByte(Connection, (unsigned char)cr->Direction);
		SendWord(Connection, (unsigned short)cr->Outfit.OutfitID);
		if (cr->Outfit.OutfitID == 0) {
			SendWord(Connection, cr->Outfit.ObjectType);
		} else {
			SendText(Connection, (const char *)cr->Outfit.Colors, 4);
		}
		GetCreatureLight(cr->ID, HelpBrightness, HelpColor);
		SendByte(Connection, (unsigned char)HelpBrightness);
		SendByte(Connection, (unsigned char)HelpColor);
		SendWord(Connection, (unsigned short)cr->GetSpeed());
		if (cr->Type == PLAYER) {
			SendByte(Connection, (unsigned char)((TPlayer *)cr)->GetPlayerkillingMark(Connection->GetPlayer()));
		} else {
			SendByte(Connection, SKULL_NONE);
		}
		if (cr->Type == PLAYER) {
			SendByte(Connection, (unsigned char)((TPlayer *)cr)->GetPartyMark(Connection->GetPlayer()));
		} else {
			SendByte(Connection, PARTY_SHIELD_NONE);
		}
	} else {
		SendObjectType(Connection, Obj);
		if (Obj.getFlag(LIQUIDCONTAINER)) {
			SendByte(Connection, GetLiquidColor(Obj.getAttribute(CONTAINERLIQUIDTYPE)));
		}
		if (Obj.getFlag(LIQUIDPOOL)) {
			SendByte(Connection, GetLiquidColor(Obj.getAttribute(POOLLIQUIDTYPE)));
		}
		if (Obj.getFlag(CUMULATIVE)) {
			SendByte(Connection, (unsigned char)Obj.getAttribute(AMOUNT));
		}
	}
}

void SendMapPoint(TConnection *Connection, int x, int y, int z) {
	Object Obj = GetFirstObject(x, y, z);
	if (Obj == NONE) {
		Skip++;
		return;
	}

	SkipFlush(Connection);

	int Count = 0;
	while (Obj != NONE) {
		SendMapObject(Connection, Obj);
		Count++;
		Obj++;
		if (Count == OBJECTS_PER_POINT) {
			break;
		}
	}

	Skip++;
}

void SendResult(TConnection *Connection, RESULT r) {
	if (Connection == nullptr) {
		return;
	}

	switch (r) {
		case NOERROR: error("SendResult: NOERROR\n"); break;
		case NOTACCESSIBLE: SendMessage(Connection, TALK_FAILURE_MESSAGE, "Sorry, not possible."); break;
		case NOTMOVABLE: SendMessage(Connection, TALK_FAILURE_MESSAGE, "You cannot move this object."); break;
		case NOTTAKABLE: SendMessage(Connection, TALK_FAILURE_MESSAGE, "You cannot take this object."); break;
		case NOROOM: SendMessage(Connection, TALK_FAILURE_MESSAGE, "There is not enough room."); break;
		case OUTOFRANGE: SendMessage(Connection, TALK_FAILURE_MESSAGE, "Destination is out of range."); break;
		case OUTOFHOME: error("SendResult: OUTOFHOME\n"); break;
		case CANNOTTHROW: SendMessage(Connection, TALK_FAILURE_MESSAGE, "You cannot throw there."); break;
		case TOOHEAVY: SendMessage(Connection, TALK_FAILURE_MESSAGE, "This object is too heavy."); break;
		case CROSSREFERENCE: SendMessage(Connection, TALK_FAILURE_MESSAGE, "This is impossible."); break;
		case CONTAINERFULL: SendMessage(Connection, TALK_FAILURE_MESSAGE, "You cannot put more objects in this container."); break;
		case WRONGPOSITION: SendMessage(Connection, TALK_FAILURE_MESSAGE, "Put this object in your hand."); break;
		case WRONGPOSITION2: SendMessage(Connection, TALK_FAILURE_MESSAGE, "Put this object in both hands."); break;
		case WRONGCLOTHES: SendMessage(Connection, TALK_FAILURE_MESSAGE, "You cannot dress this object there."); break;
		case HANDSNOTFREE: SendMessage(Connection, TALK_FAILURE_MESSAGE, "Both hands have to be free."); break;
		case HANDBLOCKED: SendMessage(Connection, TALK_FAILURE_MESSAGE, "Drop the double-handed object first."); break;
		case ONEWEAPONONLY: SendMessage(Connection, TALK_FAILURE_MESSAGE, "You may only use one weapon."); break;
		case NOMATCH: error("SendResult: NOMATCH\n"); break;
		case NOTCUMULABLE: error("SendResult: NOTCUMULABLE\n"); break;
		case TOOMANYPARTS: error("SendResult: TOOMANYPARTS\n"); break;
		case EMPTYCONTAINER: error("SendResult: EMPTYCONTAINER\n"); break;
		case SPLITOBJECT: error("SendResult: SPLITOBJECT\n"); break;
		case NOKEYMATCH: SendMessage(Connection, TALK_FAILURE_MESSAGE, "The key does not match."); break;
		case UPSTAIRS: SendMessage(Connection, TALK_FAILURE_MESSAGE, "First go upstairs."); break;
		case DOWNSTAIRS: SendMessage(Connection, TALK_FAILURE_MESSAGE, "First go downstairs."); break;
		case CREATURENOTEXISTING: SendMessage(Connection, TALK_FAILURE_MESSAGE, "A creature with this name does not exist."); break;
		case PLAYERNOTEXISTING: SendMessage(Connection, TALK_FAILURE_MESSAGE, "A player with this name does not exist."); break;
		case PLAYERNOTONLINE: SendMessage(Connection, TALK_FAILURE_MESSAGE, "A player with this name is not online."); break;
		case NAMEAMBIGUOUS: SendMessage(Connection, TALK_FAILURE_MESSAGE, "Playername is ambiguous."); break;
		case NOTUSABLE: SendMessage(Connection, TALK_FAILURE_MESSAGE, "You cannot use this object."); break;
		case FEDUP: SendMessage(Connection, TALK_FAILURE_MESSAGE, "You are full."); break;
		case SPELLUNKNOWN: SendMessage(Connection, TALK_FAILURE_MESSAGE, "You must learn this spell first."); break;
		case LOWMAGICLEVEL: SendMessage(Connection, TALK_FAILURE_MESSAGE, "Your magic level is too low."); break;
		case MAGICITEM: SendMessage(Connection, TALK_FAILURE_MESSAGE, "A magic item is necessary to cast this spell."); break;
		case NOTENOUGHMANA: SendMessage(Connection, TALK_FAILURE_MESSAGE, "You do not have enough mana."); break;
		case NOSKILL: error("SendResult: NOSKILL\n"); break;
		case TARGETLOST: SendMessage(Connection, TALK_FAILURE_MESSAGE, "Target lost."); break;
		case NOCREATURE: SendMessage(Connection, TALK_FAILURE_MESSAGE, "You can only use this rune on creatures."); break;
		case TOOLONG: error("SendResult: TOOLONG\n"); break;
		case ATTACKNOTALLOWED: SendMessage(Connection, TALK_FAILURE_MESSAGE, "You may not attack this person."); break;
		case NOWAY: SendMessage(Connection, TALK_FAILURE_MESSAGE, "There is no way."); break;
		case LOGINERROR: SendMessage(Connection, TALK_FAILURE_MESSAGE, "An error occured while logging in."); break;
		case PROTECTIONZONE: SendMessage(Connection, TALK_FAILURE_MESSAGE, "This action is not permitted in a protection zone."); break;
		case ENTERPROTECTIONZONE: {
			SendMessage(Connection, TALK_FAILURE_MESSAGE, "Characters who attacked other players may not enter a protection zone.");
			SendSnapback(Connection);
			break;
		}
		case EXHAUSTED: SendMessage(Connection, TALK_FAILURE_MESSAGE, "You are exhausted."); break;
		case NOTINVITED: {
			SendMessage(Connection, TALK_FAILURE_MESSAGE, "You are not invited.");
			SendSnapback(Connection);
			break;
		}
		case NOPREMIUMACCOUNT: SendMessage(Connection, TALK_FAILURE_MESSAGE, "You need a premium account."); break;
		case MOVENOTPOSSIBLE: {
			SendMessage(Connection, TALK_FAILURE_MESSAGE, "Sorry, not possible.");
			SendSnapback(Connection);
			break;
		}
		case ALREADYTRADING: SendMessage(Connection, TALK_FAILURE_MESSAGE, "You are already trading. Finish this trade first."); break;
		case PARTNERTRADING: SendMessage(Connection, TALK_FAILURE_MESSAGE, "This person is already trading."); break;
		case TOOMANYOBJECTS: SendMessage(Connection, TALK_FAILURE_MESSAGE, "You can only trade up to 100 objects at one time."); break;
		case TOOMANYSLAVES: SendMessage(Connection, TALK_FAILURE_MESSAGE, "You cannot control more creatures."); break;
		case NOTTURNABLE: SendMessage(Connection, TALK_FAILURE_MESSAGE, "You cannot turn this object."); break;
		case SECUREMODE: SendMessage(Connection, TALK_FAILURE_MESSAGE, "Turn secure mode off if you really want to attack unmarked players."); break;
		case NOTENOUGHSOULPOINTS: SendMessage(Connection, TALK_FAILURE_MESSAGE, "You do not have enough soulpoints."); break;
		case LOWLEVEL: SendMessage(Connection, TALK_FAILURE_MESSAGE, "Your level is too low."); break;
		default: break;
	}
}

void SendRefresh(TConnection *Connection) {
	SendFullScreen(Connection);
	SendAmbiente(Connection);
	SendPlayerData(Connection);
	SendPlayerSkills(Connection);
}

void SendInitGame(TConnection *Connection, uint32_t id) {
	if (!BeginSendData(Connection)) {
		return;
	}

	SendByte(Connection, SV_INIT_GAME);
	SendQuad(Connection, id);
	SendWord(Connection, (unsigned short)Beat);
	SendByte(Connection, CheckRight(id, SEND_BUGREPORTS) ? 1 : 0);
	FinishSendData(Connection);
}

void SendRights(TConnection *Connection) {
	if (!BeginSendData(Connection)) {
		return;
	}

	SendByte(Connection, SV_RIGHTS);
	TPlayer *pl = Connection->GetPlayer();
	bool SendIt = false;
	for (int Right = FIRST_BANISHMENT_RIGHT; Right <= LAST_BANISHMENT_RIGHT; Right++) {
		int Actions = 0;
		if (CheckRight(pl->ID, (RIGHT)Right)) {
			int Reason = Right - FIRST_BANISHMENT_RIGHT;
			for (int Action = 0; Action <= 6; Action++) {
				if (CheckBanishmentRight(pl->ID, Reason, Action)) {
					Actions |= 1 << Action;
				}
			}

			if (Actions != 0 && CheckRight(pl->ID, IP_BANISHMENT)) {
				Actions |= RIGHTS_IP_BANISHMENT;
			}
		}

		SendByte(Connection, (unsigned char)Actions);
		if (Actions != 0) {
			SendIt = true;
		}
	}

	if (SendIt) {
		FinishSendData(Connection);
	}
}

void SendPing(TConnection *Connection) {
	if (!BeginSendData(Connection)) {
		return;
	}

	SendByte(Connection, SV_PING);
	FinishSendData(Connection);
}

void SendFullScreen(TConnection *Connection) {
	if (!BeginSendData(Connection)) {
		return;
	}

	SendByte(Connection, SV_FULL_SCREEN);

	int posx, posy, posz;
	Connection->GetPosition(posx, posy, posz);
	SendWord(Connection, (unsigned short)posx);
	SendWord(Connection, (unsigned short)posy);
	SendByte(Connection, (unsigned char)posz);

	int sx = posx - Connection->GetTerminalOffsetX();
	int sy = posy - Connection->GetTerminalOffsetY();
	Skip = -1;
	if (posz <= 7) {
		for (int z = 7; z >= 0; z--) {
			for (int x = 0; x < Connection->GetTerminalWidth(); x++) {
				for (int y = 0; y < Connection->GetTerminalHeight(); y++) {
					SendMapPoint(Connection, sx + x + (posz - z), sy + y + (posz - z), z);
				}
			}
		}
	} else {
		for (int z = posz - 2; z <= posz + 2 && z <= 15; z++) {
			for (int x = 0; x < Connection->GetTerminalWidth(); x++) {
				for (int y = 0; y < Connection->GetTerminalHeight(); y++) {
					SendMapPoint(Connection, sx + x + (posz - z), sy + y + (posz - z), z);
				}
			}
		}
	}

	SkipFlush(Connection);
	FinishSendData(Connection);
}

void SendRow(TConnection *Connection, int Direction) {
	if (!BeginSendData(Connection)) {
		return;
	}

	switch (Direction) {
		case DIRECTION_NORTH: SendByte(Connection, SV_ROW_NORTH); break;
		case DIRECTION_EAST: SendByte(Connection, SV_ROW_EAST); break;
		case DIRECTION_SOUTH: SendByte(Connection, SV_ROW_SOUTH); break;
		case DIRECTION_WEST: SendByte(Connection, SV_ROW_WEST); break;
		default: {
			error("SendRow: Ung\xfcltige Richtungsangabe %d.\n", Direction);
			return;
		}
	}

	int posx, posy, posz;
	Connection->GetPosition(posx, posy, posz);

	int sx = posx - Connection->GetTerminalOffsetX();
	int sy = posy - Connection->GetTerminalOffsetY();
	Skip = -1;

	int z, dz, end;
	if (posz <= 7) {
		z = 7;
		dz = -1;
		end = -1;
	} else {
		z = posz - 2;
		dz = 1;
		end = posz + 3;
		if (end > 16) {
			end = 16;
		}
	}

	while (z != end) {
		switch (Direction) {
			case DIRECTION_NORTH: {
				for (int x = 0; x < Connection->GetTerminalWidth(); x++) {
					SendMapPoint(Connection, sx + x + (posz - z), sy + (posz - z), z);
				}
				break;
			}

			case DIRECTION_EAST: {
				for (int y = 0; y < Connection->GetTerminalHeight(); y++) {
					SendMapPoint(Connection, sx + Connection->GetTerminalWidth() - 1 + (posz - z), sy + y + (posz - z), z);
				}
				break;
			}

			case DIRECTION_SOUTH: {
				for (int x = 0; x < Connection->GetTerminalWidth(); x++) {
					SendMapPoint(Connection, sx + x + (posz - z), sy + Connection->GetTerminalHeight() - 1 + (posz - z), z);
				}
				break;
			}

			case DIRECTION_WEST: {
				for (int y = 0; y < Connection->GetTerminalHeight(); y++) {
					SendMapPoint(Connection, sx + (posz - z), sy + y + (posz - z), z);
				}
				break;
			}
		}
		z += dz;
	}

	SkipFlush(Connection);
	FinishSendData(Connection);
}

void SendFloors(TConnection *Connection, bool Up) {
	if (!BeginSendData(Connection)) {
		return;
	}

	SendByte(Connection, Up ? SV_FLOORS_UP : SV_FLOORS_DOWN);

	int posx, posy, posz;
	Connection->GetPosition(posx, posy, posz);

	int z, end, dz;
	if (Up) {
		if (posz > 7) {
			z = posz - 2;
			end = posz - 3;
			dz = -1;
		} else if (posz == 7) {
			z = 5;
			end = -1;
			dz = -1;
		} else {
			z = -1;
			end = -1;
			dz = 0;
		}
	} else {
		if (posz > 8) {
			z = posz + 2;
			end = posz + 3;
			dz = 1;
			if (z > 16) {
				z = 16;
			}
			if (end > 16) {
				end = 16;
			}
		} else if (posz == 8) {
			z = 8;
			end = 11;
			dz = 1;
		} else {
			z = -1;
			end = -1;
			dz = 0;
		}
	}

	int sx = posx - Connection->GetTerminalOffsetX();
	int sy = posy - Connection->GetTerminalOffsetY();
	Skip = -1;
	while (z != end) {
		for (int x = 0; x < Connection->GetTerminalWidth(); x++) {
			for (int y = 0; y < Connection->GetTerminalHeight(); y++) {
				SendMapPoint(Connection, sx + x + (posz - z), sy + y + (posz - z), z);
			}
		}
		z += dz;
	}

	SkipFlush(Connection);
	FinishSendData(Connection);
}

void SendFieldData(TConnection *Connection, int x, int y, int z) {
	if (!BeginSendData(Connection)) {
		return;
	}

	SendByte(Connection, SV_FIELD_DATA);
	SendWord(Connection, (unsigned short)x);
	SendWord(Connection, (unsigned short)y);
	SendByte(Connection, (unsigned char)z);
	Skip = -1;
	SendMapPoint(Connection, x, y, z);
	SkipFlush(Connection);
	FinishSendData(Connection);
}

void SendCloseContainer(TConnection *Connection, int ContNr) {
	if (!BeginSendData(Connection)) {
		return;
	}

	SendByte(Connection, SV_CLOSE_CONTAINER);
	SendByte(Connection, (unsigned char)ContNr);
	FinishSendData(Connection);
}

void SendContainer(TConnection *Connection, int ContNr) {
	if (!BeginSendData(Connection)) {
		return;
	}

	SendByte(Connection, SV_CONTAINER);
	SendByte(Connection, (unsigned char)ContNr);

	TPlayer *pl = Connection->GetPlayer();
	if (pl == nullptr) {
		error("SendContainer: Zu dieser Verbindung geh\xf6rt kein Spieler.\n");
		return;
	}

	Object Obj = pl->GetOpenContainer(ContNr);
	if (!Obj.exists()) {
		error("SendContainer: Container %d existiert nicht.\n", ContNr);
		return;
	}

	SendObjectType(Connection, Obj);
	SendString(Connection, Obj.getObjectType().getName(-1));
	SendByte(Connection, (unsigned char)Obj.getAttribute(CAPACITY));
	SendByte(Connection, (!Obj.getContainer().isMapContainer() && !Obj.getContainer().isBodyContainer()) ? 1 : 0);
	SendByte(Connection, (unsigned char)std::min(CountObjectsInContainer(Obj), OBJECTS_PER_CONTAINER));

	int Count = 0;
	Obj = GetFirstContainerObject(Obj);
	while (Obj != NONE && Count < OBJECTS_PER_CONTAINER) {
		SendObjectType(Connection, Obj);
		if (Obj.getFlag(LIQUIDCONTAINER)) {
			SendByte(Connection, GetLiquidColor(Obj.getAttribute(CONTAINERLIQUIDTYPE)));
		}
		if (Obj.getFlag(LIQUIDPOOL)) {
			SendByte(Connection, GetLiquidColor(Obj.getAttribute(POOLLIQUIDTYPE)));
		}
		if (Obj.getFlag(CUMULATIVE)) {
			SendByte(Connection, (unsigned char)Obj.getAttribute(AMOUNT));
		}
		Obj++;
		Count++;
	}

	FinishSendData(Connection);
}

void SendCreateInContainer(TConnection *Connection, int ContNr, Object Obj) {
	if (!BeginSendData(Connection)) {
		return;
	}

	if (!Obj.exists()) {
		error("SendCreateInContainer: \xdc" "bergebenes Objekt existiert nicht.\n");
		return;
	}

	SendByte(Connection, SV_CREATE_IN_CONTAINER);
	SendByte(Connection, (unsigned char)ContNr);
	SendObjectType(Connection, Obj);
	if (Obj.getFlag(LIQUIDCONTAINER)) {
		SendByte(Connection, GetLiquidColor(Obj.getAttribute(CONTAINERLIQUIDTYPE)));
	}
	if (Obj.getFlag(LIQUIDPOOL)) {
		SendByte(Connection, GetLiquidColor(Obj.getAttribute(POOLLIQUIDTYPE)));
	}
	if (Obj.getFlag(CUMULATIVE)) {
		SendByte(Connection, (unsigned char)Obj.getAttribute(AMOUNT));
	}
	FinishSendData(Connection);
}

void SendChangeInContainer(TConnection *Connection, int ContNr, Object Obj) {
	if (!BeginSendData(Connection)) {
		return;
	}

	if (!Obj.exists()) {
		error("SendChangeInContainer: \xdc" "bergebenes Objekt existiert nicht.\n");
		return;
	}

	int RNum = GetObjectRNum(Obj);
	if (RNum >= OBJECTS_PER_CONTAINER) {
		return;
	}

	SendByte(Connection, SV_CHANGE_IN_CONTAINER);
	SendByte(Connection, (unsigned char)ContNr);
	SendByte(Connection, (unsigned char)RNum);
	SendObjectType(Connection, Obj);
	if (Obj.getFlag(LIQUIDCONTAINER)) {
		SendByte(Connection, GetLiquidColor(Obj.getAttribute(CONTAINERLIQUIDTYPE)));
	}
	if (Obj.getFlag(LIQUIDPOOL)) {
		SendByte(Connection, GetLiquidColor(Obj.getAttribute(POOLLIQUIDTYPE)));
	}
	if (Obj.getFlag(CUMULATIVE)) {
		SendByte(Connection, (unsigned char)Obj.getAttribute(AMOUNT));
	}
	FinishSendData(Connection);
}

void SendDeleteInContainer(TConnection *Connection, int ContNr, Object Obj) {
	if (!BeginSendData(Connection)) {
		return;
	}

	if (!Obj.exists()) {
		error("SendDeleteInContainer: \xdc" "bergebenes Objekt existiert nicht.\n");
		return;
	}

	int RNum = GetObjectRNum(Obj);
	if (RNum >= OBJECTS_PER_CONTAINER) {
		return;
	}

	SendByte(Connection, SV_DELETE_IN_CONTAINER);
	SendByte(Connection, (unsigned char)ContNr);
	SendByte(Connection, (unsigned char)RNum);
	FinishSendData(Connection);
}

void SendBodyInventory(TConnection *Connection, uint32_t CreatureID) {
	for (int Position = FIRST_BODY_POSITION; Position <= LAST_BODY_POSITION; Position++) {
		Object Obj = GetBodyObject(CreatureID, Position);
		if (Obj != NONE) {
			SendSetInventory(Connection, Position, Obj);
		}
	}
}

void SendSetInventory(TConnection *Connection, int Position, Object Obj) {
	if (!BeginSendData(Connection)) {
		return;
	}

	SendByte(Connection, SV_SET_INVENTORY);
	SendByte(Connection, (unsigned char)Position);
	if (!Obj.exists()) {
		error("SendSetInventory: Objekt existiert nicht.\n");
		return;
	}

	SendObjectType(Connection, Obj);
	if (Obj.getFlag(LIQUIDCONTAINER)) {
		SendByte(Connection, GetLiquidColor(Obj.getAttribute(CONTAINERLIQUIDTYPE)));
	}
	if (Obj.getFlag(LIQUIDPOOL)) {
		SendByte(Connection, GetLiquidColor(Obj.getAttribute(POOLLIQUIDTYPE)));
	}
	if (Obj.getFlag(CUMULATIVE)) {
		SendByte(Connection, (unsigned char)Obj.getAttribute(AMOUNT));
	}
	FinishSendData(Connection);
}

void SendDeleteInventory(TConnection *Connection, int Position) {
	if (!BeginSendData(Connection)) {
		return;
	}

	SendByte(Connection, SV_DELETE_INVENTORY);
	SendByte(Connection, (unsigned char)Position);
	FinishSendData(Connection);
}

void SendTradeObjects(TConnection *Connection, Object Obj) {
	if (!Obj.exists()) {
		return;
	}

	SendObjectType(Connection, Obj);
	if (Obj.getFlag(LIQUIDCONTAINER)) {
		SendByte(Connection, GetLiquidColor(Obj.getAttribute(CONTAINERLIQUIDTYPE)));
	}
	if (Obj.getFlag(LIQUIDPOOL)) {
		SendByte(Connection, GetLiquidColor(Obj.getAttribute(POOLLIQUIDTYPE)));
	}
	if (Obj.getFlag(CUMULATIVE)) {
		SendByte(Connection, (unsigned char)Obj.getAttribute(AMOUNT));
	}

	if (Obj.getFlag(CONTAINER)) {
		Obj = GetFirstContainerObject(Obj);
		while (Obj != NONE) {
			SendTradeObjects(Connection, Obj);
			Obj++;
		}
	}
}

void SendTradeOffer(TConnection *Connection, char *Name, bool OwnOffer, Object Obj) {
	if (!BeginSendData(Connection)) {
		return;
	}

	SendByte(Connection, OwnOffer ? SV_TRADE_OFFER_OWN : SV_TRADE_OFFER_PARTNER);
	if (Name == nullptr) {
		error("SendTradeOffer: Name ist NULL.\n");
		return;
	}

	SendString(Connection, Name);
	if (!Obj.exists()) {
		error("SendTradeOffer: Objekt existiert nicht.\n");
		return;
	}

	SendByte(Connection, (unsigned char)CountObjects(Obj));
	SendTradeObjects(Connection, Obj);
	FinishSendData(Connection);
}

void SendCloseTrade(TConnection *Connection) {
	if (!BeginSendData(Connection)) {
		return;
	}

	SendByte(Connection, SV_CLOSE_TRADE);
	FinishSendData(Connection);
}

void SendAddField(TConnection *Connection, int x, int y, int z, Object Obj) {
	if (!BeginSendData(Connection)) {
		return;
	}

	SendByte(Connection, SV_ADD_FIELD);
	SendWord(Connection, (unsigned short)x);
	SendWord(Connection, (unsigned short)y);
	SendByte(Connection, (unsigned char)z);
	if (!Obj.exists()) {
		error("SendAddField: \xdc" "bergebenes Objekt existiert nicht.\n");
		return;
	}

	SendMapObject(Connection, Obj);
	FinishSendData(Connection);
}

void SendDeleteField(TConnection *Connection, int x, int y, int z, Object Obj) {
	if (!BeginSendData(Connection)) {
		return;
	}

	SendByte(Connection, SV_DELETE_FIELD);
	SendWord(Connection, (unsigned short)x);
	SendWord(Connection, (unsigned short)y);
	SendByte(Connection, (unsigned char)z);
	if (!Obj.exists()) {
		// The original names SendAddField in this message.
		error("SendAddField: \xdc" "bergebenes Objekt existiert nicht.\n");
		return;
	}

	int RNum = GetObjectRNum(Obj);
	if (RNum >= OBJECTS_PER_POINT) {
		return;
	}

	SendByte(Connection, (unsigned char)RNum);
	FinishSendData(Connection);
}

void SendChangeField(TConnection *Connection, int x, int y, int z, Object Obj) {
	if (!BeginSendData(Connection)) {
		return;
	}

	SendByte(Connection, SV_CHANGE_FIELD);
	SendWord(Connection, (unsigned short)x);
	SendWord(Connection, (unsigned short)y);
	SendByte(Connection, (unsigned char)z);
	if (!Obj.exists()) {
		error("SendChangeField: \xdc" "bergebenes Objekt existiert nicht.\n");
		return;
	}

	int RNum = GetObjectRNum(Obj);
	if (RNum >= OBJECTS_PER_POINT) {
		return;
	}

	SendByte(Connection, (unsigned char)RNum);
	SendMapObject(Connection, Obj);
	FinishSendData(Connection);
}

void SendMoveCreature(TConnection *Connection, uint32_t CreatureID, int x, int y, int z) {
	if (Connection == nullptr) {
		return;
	}

	TCreature *cr = GetCreature(CreatureID);
	if (cr == nullptr) {
		error("SendMoveCreature: Kreatur existiert nicht.\n");
		return;
	}

	int RNum = GetObjectRNum(cr->GetCreatureObject());
	bool StartVisible = Connection->IsVisible(cr->posx, cr->posy, cr->posz) && RNum < OBJECTS_PER_POINT;
	bool DestVisible = Connection->IsVisible(x, y, z);

	if (StartVisible && DestVisible) {
		if (!BeginSendData(Connection)) {
			return;
		}

		SendByte(Connection, SV_MOVE_CREATURE);
		SendWord(Connection, (unsigned short)cr->posx);
		SendWord(Connection, (unsigned short)cr->posy);
		SendByte(Connection, (unsigned char)cr->posz);
		SendByte(Connection, (unsigned char)RNum);
		SendWord(Connection, (unsigned short)x);
		SendWord(Connection, (unsigned short)y);
		SendByte(Connection, (unsigned char)z);
		FinishSendData(Connection);
	}

	if (StartVisible && !DestVisible) {
		SendDeleteField(Connection, cr->posx, cr->posy, cr->posz, cr->GetCreatureObject());
	}

	if (!StartVisible && DestVisible) {
		SendAddField(Connection, x, y, z, cr->GetCreatureObject());
	}
}

void SendGraphicalEffect(TConnection *Connection, int x, int y, int z, int Type) {
	if (!BeginSendData(Connection)) {
		return;
	}

	SendByte(Connection, SV_GRAPHICAL_EFFECT);
	SendWord(Connection, (unsigned short)x);
	SendWord(Connection, (unsigned short)y);
	SendByte(Connection, (unsigned char)z);
	SendByte(Connection, (unsigned char)Type);
	FinishSendData(Connection);
}

void SendTextualEffect(TConnection *Connection, int x, int y, int z, int Color, char *Text) {
	if (Text == nullptr) {
		error("SendTextualEffect: Text ist NULL.\n");
		return;
	}

	if (Text[0] == 0) {
		error("SendTextualEffect: Text ist leer.\n");
		return;
	}

	if (!BeginSendData(Connection)) {
		return;
	}

	SendByte(Connection, SV_TEXTUAL_EFFECT);
	SendWord(Connection, (unsigned short)x);
	SendWord(Connection, (unsigned short)y);
	SendByte(Connection, (unsigned char)z);
	SendByte(Connection, (unsigned char)Color);
	SendString(Connection, Text);
	FinishSendData(Connection);
}

void SendMissileEffect(TConnection *Connection, int sx, int sy, int sz, int zx, int zy, int zz, int Type) {
	if (!BeginSendData(Connection)) {
		return;
	}

	SendByte(Connection, SV_MISSILE_EFFECT);
	SendWord(Connection, (unsigned short)sx);
	SendWord(Connection, (unsigned short)sy);
	SendByte(Connection, (unsigned char)sz);
	SendWord(Connection, (unsigned short)zx);
	SendWord(Connection, (unsigned short)zy);
	SendByte(Connection, (unsigned char)zz);
	SendByte(Connection, (unsigned char)Type);
	FinishSendData(Connection);
}

void SendMarkCreature(TConnection *Connection, uint32_t CreatureID, int Color) {
	if (!BeginSendData(Connection)) {
		return;
	}

	SendByte(Connection, SV_MARK_CREATURE);
	SendQuad(Connection, CreatureID);
	SendByte(Connection, (unsigned char)Color);
	FinishSendData(Connection);
}

void SendEditText(TConnection *Connection, Object Obj) {
	if (!BeginSendData(Connection)) {
		return;
	}

	SendByte(Connection, SV_EDIT_TEXT);
	if (!Obj.exists()) {
		error("SendEditText: \xdc" "bergebenes Objekt existiert nicht.\n");
		return;
	}

	SendQuad(Connection, Obj.getObjectID());
	SendObjectType(Connection, Obj);

	unsigned short MaxLen;
	char *Text;
	char *Editor;
	char Buffer[4096] = "";
	if (Obj.getFlag(WRITE) || Obj.getFlag(WRITEONCE)) {
		MaxLen = (unsigned short)(Obj.getAttribute(Obj.getFlag(WRITE) ? MAXLENGTH : MAXLENGTHONCE) - 1);
		Text = GetDynamicString(Obj.getAttribute(TEXTSTRING));
		Editor = GetDynamicString(Obj.getAttribute(EDITOR));
	} else if (Obj.getFlag(INFORMATION) && Obj.getAttribute(INFORMATIONTYPE) == INFORMATION_SPELLBOOK) {
		TPlayer *pl = Connection->GetPlayer();
		if (pl == nullptr) {
			error("SendEditText: Zu dieser Verbindung geh\xf6rt kein Spieler.\n");
			return;
		}

		GetSpellbook(pl->ID, Buffer);
		Text = Buffer;
		Editor = nullptr;
		MaxLen = (unsigned short)strlen(Text);
	} else {
		Text = GetDynamicString(Obj.getAttribute(TEXTSTRING));
		Editor = GetDynamicString(Obj.getAttribute(EDITOR));
		MaxLen = 0;
		if (Text != nullptr) {
			MaxLen = (unsigned short)strlen(Text);
		}
	}

	SendWord(Connection, MaxLen);
	if (Text != nullptr && MaxLen != 0) {
		SendString(Connection, Text);
		if (Editor != nullptr) {
			SendString(Connection, Editor);
		} else {
			SendWord(Connection, 0);
		}
	} else {
		SendWord(Connection, 0);
		SendWord(Connection, 0);
	}

	FinishSendData(Connection);
}

void SendEditList(TConnection *Connection, unsigned char Type, uint32_t ID, char *Text) {
	if (!BeginSendData(Connection)) {
		return;
	}

	SendByte(Connection, SV_EDIT_LIST);
	SendByte(Connection, Type);
	SendQuad(Connection, ID);
	if (strlen(Text) > 4000) {
		error("SendEditList: Text ist zu lang.\n");
		return;
	}

	SendString(Connection, Text);
	FinishSendData(Connection);
}

void SendAmbiente(TConnection *Connection) {
	if (!BeginSendData(Connection)) {
		return;
	}

	SendByte(Connection, SV_AMBIENTE);

	int Brightness, Color;
	GetAmbiente(Brightness, Color);
	SendByte(Connection, (unsigned char)Brightness);
	SendByte(Connection, (unsigned char)Color);
	FinishSendData(Connection);
}

void SendCreatureHealth(TConnection *Connection, uint32_t CreatureID) {
	if (!BeginSendData(Connection)) {
		return;
	}

	SendByte(Connection, SV_CREATURE_HEALTH);
	SendQuad(Connection, CreatureID);

	TCreature *cr = GetCreature(CreatureID);
	if (cr == nullptr) {
		error("SendCreatureHealth: Kreatur %ld existiert nicht.\n", (long)(int32_t)CreatureID);
		return;
	}

	SendByte(Connection, (unsigned char)cr->GetHealth());
	FinishSendData(Connection);
}

void SendCreatureLight(TConnection *Connection, uint32_t CreatureID) {
	if (!BeginSendData(Connection)) {
		return;
	}

	SendByte(Connection, SV_CREATURE_LIGHT);
	SendQuad(Connection, CreatureID);

	TCreature *cr = GetCreature(CreatureID);
	if (cr == nullptr) {
		error("SendCreatureLight: Kreatur %ld existiert nicht.\n", (long)(int32_t)CreatureID);
		return;
	}

	int HelpBrightness, HelpColor;
	GetCreatureLight(cr->ID, HelpBrightness, HelpColor);
	SendByte(Connection, (unsigned char)HelpBrightness);
	SendByte(Connection, (unsigned char)HelpColor);
	FinishSendData(Connection);
}

void SendCreatureOutfit(TConnection *Connection, uint32_t CreatureID) {
	if (!BeginSendData(Connection)) {
		return;
	}

	SendByte(Connection, SV_CREATURE_OUTFIT);
	SendQuad(Connection, CreatureID);

	TCreature *cr = GetCreature(CreatureID);
	if (cr == nullptr) {
		error("SendCreatureOutfit: Kreatur %ld existiert nicht.\n", (long)(int32_t)CreatureID);
		return;
	}

	SendWord(Connection, (unsigned short)cr->Outfit.OutfitID);
	if (cr->Outfit.OutfitID == 0) {
		SendWord(Connection, cr->Outfit.ObjectType);
	} else {
		SendText(Connection, (const char *)cr->Outfit.Colors, 4);
	}
	FinishSendData(Connection);
}

void SendCreatureSpeed(TConnection *Connection, uint32_t CreatureID) {
	if (!BeginSendData(Connection)) {
		return;
	}

	SendByte(Connection, SV_CREATURE_SPEED);
	SendQuad(Connection, CreatureID);

	TCreature *cr = GetCreature(CreatureID);
	if (cr == nullptr) {
		error("SendCreatureSpeed: Kreatur %ld existiert nicht.\n", (long)(int32_t)CreatureID);
		return;
	}

	SendWord(Connection, (unsigned short)cr->GetSpeed());
	FinishSendData(Connection);
}

void SendCreatureSkull(TConnection *Connection, uint32_t CreatureID) {
	if (!BeginSendData(Connection)) {
		return;
	}

	SendByte(Connection, SV_CREATURE_SKULL);
	SendQuad(Connection, CreatureID);

	TPlayer *pl = GetPlayer(CreatureID);
	if (pl == nullptr) {
		error("SendCreatureSkull: Kreatur %ld existiert nicht.\n", (long)(int32_t)CreatureID);
		return;
	}

	SendByte(Connection, (unsigned char)pl->GetPlayerkillingMark(Connection->GetPlayer()));
	FinishSendData(Connection);
}

void SendCreatureParty(TConnection *Connection, uint32_t CreatureID) {
	if (!BeginSendData(Connection)) {
		return;
	}

	SendByte(Connection, SV_CREATURE_PARTY);
	SendQuad(Connection, CreatureID);

	TPlayer *pl = GetPlayer(CreatureID);
	if (pl == nullptr) {
		error("SendCreatureParty: Kreatur %ld existiert nicht.\n", (long)(int32_t)CreatureID);
		return;
	}

	SendByte(Connection, (unsigned char)pl->GetPartyMark(Connection->GetPlayer()));
	FinishSendData(Connection);
}

void SendClearTarget(TConnection *Connection) {
	if (!BeginSendData(Connection)) {
		return;
	}

	SendByte(Connection, SV_CLEAR_TARGET);
	FinishSendData(Connection);
}

void SendPlayerData(TConnection *Connection) {
	if (!BeginSendData(Connection)) {
		return;
	}

	SendByte(Connection, SV_PLAYER_DATA);

	TPlayer *pl = Connection->GetPlayer();
	if (pl == nullptr) {
		error("SendStatus: Zu dieser Verbindung geh\xf6rt kein Spieler.\n");
		return;
	}

	TSkill *sk = pl->Skills[SKILL_HITPOINTS];
	if (sk != nullptr) {
		SendWord(Connection, (unsigned short)sk->Get());
	} else {
		SendWord(Connection, 0);
	}
	if (sk != nullptr) {
		SendWord(Connection, (unsigned short)sk->GetMax());
	} else {
		SendWord(Connection, 0);
	}

	int Capacity = 0;
	sk = pl->Skills[SKILL_CARRYSTRENGTH];
	if (sk != nullptr) {
		Capacity = sk->Get() * 100 - GetInventoryWeight(pl->ID);
	}
	if (Capacity < 0) {
		Capacity = 0;
	}
	if (CheckRight(pl->ID, ZERO_CAPACITY)) {
		Capacity = 0;
	}
	SendWord(Connection, (unsigned short)(Capacity / 100));

	sk = pl->Skills[SKILL_LEVEL];
	if (sk != nullptr) {
		SendQuad(Connection, (uint32_t)sk->GetExp());
	} else {
		SendQuad(Connection, 0);
	}
	if (sk != nullptr) {
		SendWord(Connection, (unsigned short)sk->Get());
	} else {
		SendWord(Connection, 0);
	}
	if (sk != nullptr) {
		SendByte(Connection, (unsigned char)sk->GetProgress());
	} else {
		SendByte(Connection, 0);
	}

	sk = pl->Skills[SKILL_MANA];
	if (sk != nullptr) {
		SendWord(Connection, (unsigned short)sk->Get());
	} else {
		SendWord(Connection, 0);
	}
	if (sk != nullptr) {
		SendWord(Connection, (unsigned short)sk->GetMax());
	} else {
		SendWord(Connection, 0);
	}

	sk = pl->Skills[SKILL_MAGICLEVEL];
	if (sk != nullptr) {
		SendByte(Connection, (unsigned char)sk->Get());
	} else {
		SendByte(Connection, 0);
	}
	if (sk != nullptr) {
		SendByte(Connection, (unsigned char)sk->GetProgress());
	} else {
		SendByte(Connection, 0);
	}

	sk = pl->Skills[SKILL_SOULPOINTS];
	if (sk != nullptr) {
		SendByte(Connection, (unsigned char)sk->Get());
	} else {
		SendByte(Connection, 0);
	}

	FinishSendData(Connection);
}

void SendPlayerSkills(TConnection *Connection) {
	if (!BeginSendData(Connection)) {
		return;
	}

	SendByte(Connection, SV_PLAYER_SKILLS);

	TPlayer *pl = Connection->GetPlayer();
	if (pl == nullptr) {
		error("SendInventory: Zu dieser Verbindung geh\xf6rt kein Spieler.\n");
		return;
	}

	TSkill *sk = pl->Skills[SKILL_FISTFIGHTING];
	if (sk != nullptr) {
		SendByte(Connection, (unsigned char)sk->Get());
	} else {
		SendByte(Connection, 0);
	}
	if (sk != nullptr) {
		SendByte(Connection, (unsigned char)sk->GetProgress());
	} else {
		SendByte(Connection, 0);
	}

	sk = pl->Skills[SKILL_CLUBFIGHTING];
	if (sk != nullptr) {
		SendByte(Connection, (unsigned char)sk->Get());
	} else {
		SendByte(Connection, 0);
	}
	if (sk != nullptr) {
		SendByte(Connection, (unsigned char)sk->GetProgress());
	} else {
		SendByte(Connection, 0);
	}

	sk = pl->Skills[SKILL_SWORDFIGHTING];
	if (sk != nullptr) {
		SendByte(Connection, (unsigned char)sk->Get());
	} else {
		SendByte(Connection, 0);
	}
	if (sk != nullptr) {
		SendByte(Connection, (unsigned char)sk->GetProgress());
	} else {
		SendByte(Connection, 0);
	}

	sk = pl->Skills[SKILL_AXEFIGHTING];
	if (sk != nullptr) {
		SendByte(Connection, (unsigned char)sk->Get());
	} else {
		SendByte(Connection, 0);
	}
	if (sk != nullptr) {
		SendByte(Connection, (unsigned char)sk->GetProgress());
	} else {
		SendByte(Connection, 0);
	}

	sk = pl->Skills[SKILL_DISTANCEFIGHTING];
	if (sk != nullptr) {
		SendByte(Connection, (unsigned char)sk->Get());
	} else {
		SendByte(Connection, 0);
	}
	if (sk != nullptr) {
		SendByte(Connection, (unsigned char)sk->GetProgress());
	} else {
		SendByte(Connection, 0);
	}

	sk = pl->Skills[SKILL_SHIELDING];
	if (sk != nullptr) {
		SendByte(Connection, (unsigned char)sk->Get());
	} else {
		SendByte(Connection, 0);
	}
	if (sk != nullptr) {
		SendByte(Connection, (unsigned char)sk->GetProgress());
	} else {
		SendByte(Connection, 0);
	}

	sk = pl->Skills[SKILL_FISHING];
	if (sk != nullptr) {
		SendByte(Connection, (unsigned char)sk->Get());
	} else {
		SendByte(Connection, 0);
	}
	if (sk != nullptr) {
		SendByte(Connection, (unsigned char)sk->GetProgress());
	} else {
		SendByte(Connection, 0);
	}

	FinishSendData(Connection);
}

void SendPlayerState(TConnection *Connection, unsigned char State) {
	if (!BeginSendData(Connection)) {
		return;
	}

	SendByte(Connection, SV_PLAYER_STATE);
	SendByte(Connection, State);
	FinishSendData(Connection);
}

void SendMessage(TConnection *Connection, int Mode, char *Text, ...) {
	if (Mode != TALK_ADMIN_MESSAGE && Mode != TALK_EVENT_MESSAGE && Mode != TALK_LOGIN_MESSAGE
			&& Mode != TALK_STATUS_MESSAGE && Mode != TALK_INFO_MESSAGE && Mode != TALK_FAILURE_MESSAGE) {
		error("SendMessage: Ung\xfcltiger Modus %d.\n", Mode);
		return;
	}

	if (!BeginSendData(Connection)) {
		return;
	}

	char Buffer[4096];
	va_list args;
	va_start(args, Text);
	vsnprintf(Buffer, sizeof(Buffer), Text, args);
	va_end(args);

	SendByte(Connection, SV_MESSAGE);
	SendByte(Connection, (unsigned char)Mode);
	SendString(Connection, Buffer);
	FinishSendData(Connection);
}

void SendSnapback(TConnection *Connection) {
	if (!BeginSendData(Connection)) {
		return;
	}

	SendByte(Connection, SV_SNAPBACK);
	TPlayer *pl = Connection->GetPlayer();
	if (pl == nullptr) {
		return;
	}

	SendByte(Connection, (unsigned char)pl->Direction);
	FinishSendData(Connection);
}

void SendTalk(TConnection *Connection, uint32_t StatementID, char *Sender, int Mode, char *Text, int Data) {
	if (Mode != TALK_PRIVATE_MESSAGE && Mode != TALK_GAMEMASTER_ANSWER && Mode != TALK_PLAYER_ANSWER
			&& Mode != TALK_GAMEMASTER_REQUEST && Mode != TALK_GAMEMASTER_MESSAGE && Mode != TALK_GAMEMASTER_BROADCAST) {
		error("SendTalk (-): Ung\xfcltiger Modus %d.\n", Mode);
		return;
	}

	if (Sender == nullptr) {
		error("SendTalk (-): Sender ist NULL.\n");
		return;
	}

	if (Text == nullptr) {
		error("SendTalk (-): Text ist NULL.\n");
		return;
	}

	if (!BeginSendData(Connection)) {
		return;
	}

	SendByte(Connection, SV_TALK);
	SendQuad(Connection, StatementID);
	SendString(Connection, Sender);
	SendByte(Connection, (unsigned char)Mode);
	if (Mode == TALK_GAMEMASTER_REQUEST) {
		SendQuad(Connection, (uint32_t)Data);
	}
	SendString(Connection, Text);
	FinishSendData(Connection);
}

void SendTalk(TConnection *Connection, uint32_t StatementID, char *Sender, int Mode, int Channel, char *Text) {
	if (Mode != TALK_CHANNEL_CALL && Mode != TALK_GAMEMASTER_CHANNELCALL
			&& Mode != TALK_HIGHLIGHT_CHANNELCALL && Mode != TALK_ANONYMOUS_CHANNELCALL) {
		error("SendTalk (C): Ung\xfcltiger Modus %d.\n", Mode);
		return;
	}

	if (Sender == nullptr) {
		error("SendTalk (C): Sender ist NULL.\n");
		return;
	}

	if (Text == nullptr) {
		error("SendTalk (C): Text ist NULL.\n");
		return;
	}

	if (!BeginSendData(Connection)) {
		return;
	}

	SendByte(Connection, SV_TALK);
	SendQuad(Connection, StatementID);
	if (Mode == TALK_ANONYMOUS_CHANNELCALL) {
		SendString(Connection, "");
	} else {
		SendString(Connection, Sender);
	}
	SendByte(Connection, (unsigned char)Mode);
	SendWord(Connection, (unsigned short)Channel);
	SendString(Connection, Text);
	FinishSendData(Connection);
}

void SendTalk(TConnection *Connection, uint32_t StatementID, char *Sender, int Mode, int x, int y, int z, char *Text) {
	if (Mode != TALK_SAY && Mode != TALK_WHISPER && Mode != TALK_YELL
			&& Mode != TALK_ANIMAL_LOW && Mode != TALK_ANIMAL_LOUD) {
		error("SendTalk: Ung\xfcltiger Modus %d.\n", Mode);
		return;
	}

	if (Sender == nullptr) {
		error("SendTalk (K): Sender ist NULL.\n");
		return;
	}

	if (Text == nullptr) {
		error("SendTalk (K): Text ist NULL.\n");
		return;
	}

	if (!BeginSendData(Connection)) {
		return;
	}

	SendByte(Connection, SV_TALK);
	SendQuad(Connection, StatementID);
	SendString(Connection, Sender);
	SendByte(Connection, (unsigned char)Mode);
	SendWord(Connection, (unsigned short)x);
	SendWord(Connection, (unsigned short)y);
	SendByte(Connection, (unsigned char)z);
	SendString(Connection, Text);
	FinishSendData(Connection);
}

void SendChannels(TConnection *Connection) {
	if (!BeginSendData(Connection)) {
		return;
	}

	SendByte(Connection, SV_CHANNELS);
	TPlayer *pl = Connection->GetPlayer();
	if (pl == nullptr) {
		return;
	}

	bool OwnChannel = MayOpenChannel(pl->ID);
	int Channels = GetNumberOfChannels();
	bool Available[Channels];
	int Count = 0;
	for (int i = 0; i < Channels; i++) {
		Available[i] = ChannelAvailable(i, pl->ID);
		if (Available[i]) {
			Count++;
		}
	}

	if (OwnChannel) {
		Count++;
	}

	SendByte(Connection, (unsigned char)Count);
	if (OwnChannel) {
		SendWord(Connection, PRIVATE_CHAT_CHANNEL);
		SendString(Connection, "Private Chat Channel");
	}

	for (int i = 0; i < Channels; i++) {
		if (Available[i]) {
			SendWord(Connection, (unsigned short)i);
			SendString(Connection, GetChannelName(i, pl->ID));
		}
	}

	FinishSendData(Connection);
}

void SendOpenChannel(TConnection *Connection, int Channel) {
	if (!BeginSendData(Connection)) {
		return;
	}

	if (Channel == REQUEST_QUEUE) {
		error("SendOpenChannel: Kanal ist GM-Request-Queue.\n");
		return;
	}

	SendByte(Connection, SV_OPEN_CHANNEL);
	TPlayer *pl = Connection->GetPlayer();
	if (pl == nullptr) {
		return;
	}

	if (Channel < 0 || Channel >= GetNumberOfChannels()) {
		error("SendOpenChannel: Ung\xfcltiger Kanal %d.\n", Channel);
		return;
	}

	SendWord(Connection, (unsigned short)Channel);
	SendString(Connection, GetChannelName(Channel, pl->ID));
	FinishSendData(Connection);
}

void SendPrivateChannel(TConnection *Connection, char *Name) {
	if (!BeginSendData(Connection)) {
		return;
	}

	SendByte(Connection, SV_PRIVATE_CHANNEL);
	if (Name == nullptr) {
		error("SendPrivateChannel: Name ist NULL.\n");
		return;
	}

	SendString(Connection, Name);
	FinishSendData(Connection);
}

void SendOpenRequestQueue(TConnection *Connection) {
	if (!BeginSendData(Connection)) {
		return;
	}

	SendByte(Connection, SV_OPEN_REQUEST_QUEUE);
	SendWord(Connection, REQUEST_QUEUE);
	FinishSendData(Connection);
}

void SendDeleteRequest(TConnection *Connection, char *Name) {
	if (!BeginSendData(Connection)) {
		return;
	}

	SendByte(Connection, SV_DELETE_REQUEST);
	if (Name == nullptr) {
		error("SendDeleteRequest: Name ist NULL.\n");
		return;
	}

	SendString(Connection, Name);
	FinishSendData(Connection);
}

void SendFinishRequest(TConnection *Connection, char *Name) {
	if (!BeginSendData(Connection)) {
		return;
	}

	SendByte(Connection, SV_FINISH_REQUEST);
	if (Name == nullptr) {
		error("SendFinishRequest: Name ist NULL.\n");
		return;
	}

	SendString(Connection, Name);
	FinishSendData(Connection);
}

void SendCloseRequest(TConnection *Connection) {
	if (!BeginSendData(Connection)) {
		return;
	}

	SendByte(Connection, SV_CLOSE_REQUEST);
	FinishSendData(Connection);
}

void SendOpenOwnChannel(TConnection *Connection, int Channel) {
	if (!BeginSendData(Connection)) {
		return;
	}

	SendByte(Connection, SV_OPEN_OWN_CHANNEL);
	TPlayer *pl = Connection->GetPlayer();
	if (pl == nullptr) {
		return;
	}

	if (Channel < 0 || Channel >= GetNumberOfChannels()) {
		error("SendOpenOwnChannel: Ung\xfcltiger Kanal %d.\n", Channel);
		return;
	}

	SendWord(Connection, (unsigned short)Channel);
	SendString(Connection, GetChannelName(Channel, pl->ID));
	FinishSendData(Connection);
}

void SendCloseChannel(TConnection *Connection, int Channel) {
	if (!BeginSendData(Connection)) {
		return;
	}

	SendByte(Connection, SV_CLOSE_CHANNEL);
	SendWord(Connection, (unsigned short)Channel);
	FinishSendData(Connection);
}

void SendOutfit(TConnection *Connection) {
	if (!BeginSendData(Connection)) {
		return;
	}

	SendByte(Connection, SV_OUTFIT);
	TPlayer *pl = Connection->GetPlayer();
	if (pl == nullptr) {
		return;
	}

	SendWord(Connection, (unsigned short)pl->OrgOutfit.OutfitID);
	SendText(Connection, (const char *)pl->OrgOutfit.Colors, 4);
	if (pl->Sex == SEX_MALE) {
		SendWord(Connection, FIRST_MALE_OUTFIT);
		SendWord(Connection, CheckRight(pl->ID, PREMIUM_ACCOUNT) ? LAST_MALE_OUTFIT : LAST_FREE_MALE_OUTFIT);
	} else {
		SendWord(Connection, FIRST_FEMALE_OUTFIT);
		SendWord(Connection, CheckRight(pl->ID, PREMIUM_ACCOUNT) ? LAST_FEMALE_OUTFIT : LAST_FREE_FEMALE_OUTFIT);
	}
	FinishSendData(Connection);
}

void SendBuddyData(TConnection *Connection, uint32_t CharacterID, char *Name, bool Online) {
	if (!BeginSendData(Connection)) {
		return;
	}

	SendByte(Connection, SV_BUDDY_DATA);
	SendQuad(Connection, CharacterID);
	if (Name == nullptr) {
		return;
	}

	SendString(Connection, Name);
	SendByte(Connection, Online ? 1 : 0);
	FinishSendData(Connection);
}

void SendBuddyStatus(TConnection *Connection, uint32_t CharacterID, bool Online) {
	if (!BeginSendData(Connection)) {
		return;
	}

	SendByte(Connection, Online ? SV_BUDDY_ONLINE : SV_BUDDY_OFFLINE);
	SendQuad(Connection, CharacterID);
	FinishSendData(Connection);
}

void BroadcastMessage(int Mode, char *Text, ...) {
	va_list args;
	char Message[1024];
	va_start(args, Text);
	vsnprintf(Message, sizeof(Message), Text, args);
	va_end(args);

	TConnection *Connection = GetFirstConnection();
	while (Connection != nullptr) {
		if (Connection->IsOnline()) {
			// As in the original, the finished message is the format string here.
			SendMessage(Connection, Mode, Message);
		}
		Connection = GetNextConnection();
	}
}

void DeleteGamemasterRequest(char *Name) {
	if (Name == nullptr) {
		error("DeleteGamemasterRequest: Name ist NULL.\n");
		return;
	}

	TConnection *Connection = GetFirstConnection();
	while (Connection != nullptr) {
		if (Connection->IsOnline()) {
			TPlayer *pl = Connection->GetPlayer();
			if (pl != nullptr && ChannelSubscribed(REQUEST_QUEUE, pl->ID)) {
				SendDeleteRequest(Connection, Name);
			}
		}
		Connection = GetNextConnection();
	}
}
