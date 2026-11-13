#include <cstdint>
#include <cstdlib>

#include "script.hh"
#include "config.hh"
#include "threads.hh"
#include "dbfuncs.hh"
#include "objects.hh"
#include "map.hh"
#include "info.hh"
#include "cr.hh"
#include "containr.hh"
#include "operate.hh"
#include "connections.hh"
#include "utils.hh"
#include "time.hh"
#include "TXTEASymmetricKey.hh"
#include "communication.hh"
#include "sending.hh"

const int OBJECTS_PER_POINT = 10;

[[maybe_unused]] static const int OBJECTS_PER_CONTAINER = 36;	// sending.c:28
[[maybe_unused]] static int Skip = -1;	// sending.c:208

// inline  sending.c:111
[[maybe_unused]] static inline void SendByte(TConnection * /*Connection*/, unsigned char /*Value*/) { abort(); /* STUB */ }

// inline  sending.c:124
[[maybe_unused]] static inline void SendWord(TConnection * /*Connection*/, unsigned short /*Value*/) { abort(); /* STUB */ }

// inline  sending.c:139
[[maybe_unused]] static inline void SendQuad(TConnection * /*Connection*/, uint32_t /*Value*/) { abort(); /* STUB */ }

// inline  sending.c:183
[[maybe_unused]] static inline void SendString(TConnection * /*Connection*/, const char * /*String*/) { abort(); /* STUB */ }

// inline  sending.c:198
[[maybe_unused]] static inline void SendObjectType(TConnection * /*Connection*/, Object /*Obj*/) { abort(); /* STUB */ }

// 0x080cc0b0  sending.c:43
bool BeginSendData(TConnection * /*Connection*/) { abort(); /* STUB */ }

// 0x080cc130  sending.c:61
void FinishSendData(TConnection * /*Connection*/) { abort(); /* STUB */ }

// 0x080cc1c0  sending.c:88
void SendAll() { abort(); /* STUB */ }

// 0x080cc250  sending.c:213
void SkipFlush(TConnection * /*Connection*/) { abort(); /* STUB */ }

// 0x080cc3e0  sending.c:232
void SendMapObject(TConnection * /*Connection*/, Object /*Obj*/) { abort(); /* STUB */ }

// 0x080cd200  sending.c:302
void SendMapPoint(TConnection * /*Connection*/, int /*x*/, int /*y*/, int /*z*/) { abort(); /* STUB */ }

// 0x080cd2c0  sending.c:329
void SendResult(TConnection * /*Connection*/, RESULT /*r*/) { abort(); /* STUB */ }

// 0x080cd690  sending.c:401
void SendRefresh(TConnection * /*Connection*/) { abort(); /* STUB */ }

// 0x080cd7d0  sending.c:412
void SendInitGame(TConnection * /*Connection*/, uint32_t /*id*/) { abort(); /* STUB */ }

// 0x080cd9e0  sending.c:425
void SendRights(TConnection * /*Connection*/) { abort(); /* STUB */ }

// 0x080cdb90  sending.c:448
void SendPing(TConnection * /*Connection*/) { abort(); /* STUB */ }

// 0x080cdc10  sending.c:458
void SendFullScreen(TConnection * /*Connection*/) { abort(); /* STUB */ }

// 0x080cdf70  sending.c:491
void SendRow(TConnection * /*Connection*/, int /*Direction*/) { abort(); /* STUB */ }

// 0x080ce310  sending.c:536
void SendFloors(TConnection * /*Connection*/, bool /*Up*/) { abort(); /* STUB */ }

// 0x080ce530  sending.c:569
void SendFieldData(TConnection * /*Connection*/, int /*x*/, int /*y*/, int /*z*/) { abort(); /* STUB */ }

// 0x080ce710  sending.c:585
void SendCloseContainer(TConnection * /*Connection*/, int /*ContNr*/) { abort(); /* STUB */ }

// 0x080ce7e0  sending.c:596
void SendContainer(TConnection * /*Connection*/, int /*ContNr*/) { abort(); /* STUB */ }

// 0x080cef60  sending.c:632
void SendCreateInContainer(TConnection * /*Connection*/, int /*ContNr*/, Object /*Obj*/) { abort(); /* STUB */ }

// 0x080cf2d0  sending.c:651
void SendChangeInContainer(TConnection * /*Connection*/, int /*ContNr*/, Object /*Obj*/) { abort(); /* STUB */ }

// 0x080cf6b0  sending.c:673
void SendDeleteInContainer(TConnection * /*Connection*/, int /*ContNr*/, Object /*Obj*/) { abort(); /* STUB */ }

// 0x080cf7f0  sending.c:691
void SendBodyInventory(TConnection * /*Connection*/, uint32_t /*CreatureID*/) { abort(); /* STUB */ }

// 0x080cf860  sending.c:702
void SendSetInventory(TConnection * /*Connection*/, int /*Position*/, Object /*Obj*/) { abort(); /* STUB */ }

// 0x080cfbd0  sending.c:721
void SendDeleteInventory(TConnection * /*Connection*/, int /*Position*/) { abort(); /* STUB */ }

// 0x080cfca0  sending.c:733
void SendTradeObjects(TConnection * /*Connection*/, Object /*Obj*/) { abort(); /* STUB */ }

// 0x080cfff0  sending.c:752
void SendTradeOffer(TConnection * /*Connection*/, char * /*Name*/, bool /*OwnOffer*/, Object /*Obj*/) { abort(); /* STUB */ }

// 0x080d01b0  sending.c:773
void SendCloseTrade(TConnection * /*Connection*/) { abort(); /* STUB */ }

// 0x080d0230  sending.c:783
void SendAddField(TConnection * /*Connection*/, int /*x*/, int /*y*/, int /*z*/, Object /*Obj*/) { abort(); /* STUB */ }

// 0x080d03f0  sending.c:801
void SendDeleteField(TConnection * /*Connection*/, int /*x*/, int /*y*/, int /*z*/, Object /*Obj*/) { abort(); /* STUB */ }

// 0x080d0600  sending.c:821
void SendChangeField(TConnection * /*Connection*/, int /*x*/, int /*y*/, int /*z*/, Object /*Obj*/) { abort(); /* STUB */ }

// 0x080d0820  sending.c:842
void SendMoveCreature(TConnection * /*Connection*/, uint32_t /*CreatureID*/, int /*x*/, int /*y*/, int /*z*/) { abort(); /* STUB */ }

// 0x080d0c60  sending.c:875
void SendGraphicalEffect(TConnection * /*Connection*/, int /*x*/, int /*y*/, int /*z*/, int /*Type*/) { abort(); /* STUB */ }

// 0x080d0e50  sending.c:889
void SendTextualEffect(TConnection * /*Connection*/, int /*x*/, int /*y*/, int /*z*/, int /*Color*/, char * /*Text*/) { abort(); /* STUB */ }

// 0x080d1130  sending.c:912
void SendMissileEffect(TConnection * /*Connection*/, int /*sx*/, int /*sy*/, int /*sz*/, int /*zx*/, int /*zy*/, int /*zz*/, int /*Type*/) { abort(); /* STUB */ }

// 0x080d1450  sending.c:929
void SendMarkCreature(TConnection * /*Connection*/, uint32_t /*CreatureID*/, int /*Color*/) { abort(); /* STUB */ }

// 0x080d15c0  sending.c:941
void SendEditText(TConnection * /*Connection*/, Object /*Obj*/) { abort(); /* STUB */ }

// 0x080d1d80  sending.c:994
void SendEditList(TConnection * /*Connection*/, unsigned char /*Type*/, uint32_t /*ID*/, char * /*Text*/) { abort(); /* STUB */ }

// 0x080d1fd0  sending.c:1011
void SendAmbiente(TConnection * /*Connection*/) { abort(); /* STUB */ }

// 0x080d2100  sending.c:1025
void SendCreatureHealth(TConnection * /*Connection*/, uint32_t /*CreatureID*/) { abort(); /* STUB */ }

// 0x080d22a0  sending.c:1042
void SendCreatureLight(TConnection * /*Connection*/, uint32_t /*CreatureID*/) { abort(); /* STUB */ }

// 0x080d24b0  sending.c:1062
void SendCreatureOutfit(TConnection * /*Connection*/, uint32_t /*CreatureID*/) { abort(); /* STUB */ }

// 0x080d2720  sending.c:1081
void SendCreatureSpeed(TConnection * /*Connection*/, uint32_t /*CreatureID*/) { abort(); /* STUB */ }

// 0x080d28f0  sending.c:1098
void SendCreatureSkull(TConnection * /*Connection*/, uint32_t /*CreatureID*/) { abort(); /* STUB */ }

// 0x080d2aa0  sending.c:1115
void SendCreatureParty(TConnection * /*Connection*/, uint32_t /*CreatureID*/) { abort(); /* STUB */ }

// 0x080d2c50  sending.c:1132
void SendClearTarget(TConnection * /*Connection*/) { abort(); /* STUB */ }

// 0x080d2cd0  sending.c:1142
void SendPlayerData(TConnection * /*Connection*/) { abort(); /* STUB */ }

// 0x080d3630  sending.c:1179
void SendPlayerSkills(TConnection * /*Connection*/) { abort(); /* STUB */ }

// 0x080d3f30  sending.c:1217
void SendPlayerState(TConnection * /*Connection*/, unsigned char /*State*/) { abort(); /* STUB */ }

// 0x080d4000  sending.c:1228
void SendMessage(TConnection * /*Connection*/, int /*Mode*/, char * /*Text*/, ...) { abort(); /* STUB */ }

// 0x080d41b0  sending.c:1251
void SendSnapback(TConnection * /*Connection*/) { abort(); /* STUB */ }

// 0x080d4290  sending.c:1264
void SendTalk(TConnection * /*Connection*/, uint32_t /*StatementID*/, char * /*Sender*/, int /*Mode*/, char * /*Text*/, int /*Data*/) { abort(); /* STUB */ }

// 0x080d46d0  sending.c:1292
void SendTalk(TConnection * /*Connection*/, uint32_t /*StatementID*/, char * /*Sender*/, int /*Mode*/, int /*Channel*/, char * /*Text*/) { abort(); /* STUB */ }

// 0x080d4b70  sending.c:1320
void SendTalk(TConnection * /*Connection*/, uint32_t /*StatementID*/, char * /*Sender*/, int /*Mode*/, int /*x*/, int /*y*/, int /*z*/, char * /*Text*/) { abort(); /* STUB */ }

// 0x080d5000  sending.c:1350
void SendChannels(TConnection * /*Connection*/) { abort(); /* STUB */ }

// 0x080d5410  sending.c:1382
void SendOpenChannel(TConnection * /*Connection*/, int /*Channel*/) { abort(); /* STUB */ }

// 0x080d5640  sending.c:1404
void SendPrivateChannel(TConnection * /*Connection*/, char * /*Name*/) { abort(); /* STUB */ }

// 0x080d5790  sending.c:1419
void SendOpenRequestQueue(TConnection * /*Connection*/) { abort(); /* STUB */ }

// 0x080d5870  sending.c:1430
void SendDeleteRequest(TConnection * /*Connection*/, char * /*Name*/) { abort(); /* STUB */ }

// 0x080d59c0  sending.c:1445
void SendFinishRequest(TConnection * /*Connection*/, char * /*Name*/) { abort(); /* STUB */ }

// 0x080d5b10  sending.c:1460
void SendCloseRequest(TConnection * /*Connection*/) { abort(); /* STUB */ }

// 0x080d5b90  sending.c:1470
void SendOpenOwnChannel(TConnection * /*Connection*/, int /*Channel*/) { abort(); /* STUB */ }

// 0x080d5da0  sending.c:1488
void SendCloseChannel(TConnection * /*Connection*/, int /*Channel*/) { abort(); /* STUB */ }

// 0x080d5ea0  sending.c:1499
void SendOutfit(TConnection * /*Connection*/) { abort(); /* STUB */ }

// 0x080d6170  sending.c:1520
void SendBuddyData(TConnection * /*Connection*/, uint32_t /*CharacterID*/, char * /*Name*/, bool /*Online*/) { abort(); /* STUB */ }

// 0x080d63b0  sending.c:1534
void SendBuddyStatus(TConnection * /*Connection*/, uint32_t /*CharacterID*/, bool /*Online*/) { abort(); /* STUB */ }

// 0x080d6500  sending.c:1545
void BroadcastMessage(int /*Mode*/, char * /*Text*/, ...) { abort(); /* STUB */ }

// 0x080d6580  sending.c:1559
void DeleteGamemasterRequest(char * /*Name*/) { abort(); /* STUB */ }

// 0x080d6630  sending.c:158
[[maybe_unused]] static inline void SendText(TConnection * /*Connection*/, const char * /*Text*/, int /*Length*/) { abort(); /* STUB */ }
