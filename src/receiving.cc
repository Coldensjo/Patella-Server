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
#include "communication.hh"
#include "TXTEASymmetricKey.hh"
#include "receiving.hh"

// 0x080d6770  receiving.c:38
bool CommandAllowed(TConnection * /*Connection*/, unsigned short /*Command*/) { abort(); /* STUB */ }

// 0x080d6800  receiving.c:61
bool CheckSpecialCoordinates(int /*Command*/, int /*x*/, int /*y*/, int /*z*/, bool /*AnyAllowed*/) { abort(); /* STUB */ }

// 0x080d6890  receiving.c:79
bool CheckVisibility(int /*Command*/, TConnection * /*Connection*/, int /*x*/, int /*y*/, int /*z*/) { abort(); /* STUB */ }

// 0x080d6950  receiving.c:97
bool CheckObjectType(int /*Command*/, int /*TypeID*/) { abort(); /* STUB */ }

// 0x080d69a0  receiving.c:108
ObjectType GetObjectType(unsigned short /*TypeID*/) { abort(); /* STUB */ }

// 0x080d6a20  receiving.c:122
void CGoDirection(TConnection * /*Connection*/, int /*dx*/, int /*dy*/) { abort(); /* STUB */ }

// 0x080d6b20  receiving.c:151
void CGoPath(TConnection * /*Connection*/, TReadBuffer & /*Buffer*/) { abort(); /* STUB */ }

// 0x080d6ca0  receiving.c:194
void CStop(TConnection * /*Connection*/) { abort(); /* STUB */ }

// 0x080d6ce0  receiving.c:209
void CLookAtPoint(TConnection * /*Connection*/, TReadBuffer & /*Buffer*/) { abort(); /* STUB */ }

// 0x080d6f00  receiving.c:240
void CRefreshField(TConnection * /*Connection*/, TReadBuffer & /*Buffer*/) { abort(); /* STUB */ }

// 0x080d7030  receiving.c:258
void CTalk(TConnection * /*Connection*/, TReadBuffer & /*Buffer*/) { abort(); /* STUB */ }

// 0x080d7710  receiving.c:386
void CGetChannels(TConnection * /*Connection*/) { abort(); /* STUB */ }

// 0x080d7730  receiving.c:398
void CJoinChannel(TConnection * /*Connection*/, TReadBuffer & /*Buffer*/) { abort(); /* STUB */ }

// 0x080d7830  receiving.c:425
void CLeaveChannel(TConnection * /*Connection*/, TReadBuffer & /*Buffer*/) { abort(); /* STUB */ }

// 0x080d78c0  receiving.c:442
void CPrivateChannel(TConnection * /*Connection*/, TReadBuffer & /*Buffer*/) { abort(); /* STUB */ }

// 0x080d7990  receiving.c:461
void CProcessRequest(TConnection * /*Connection*/, TReadBuffer & /*Buffer*/) { abort(); /* STUB */ }

// 0x080d7ac0  receiving.c:494
void CRemoveRequest(TConnection * /*Connection*/, TReadBuffer & /*Buffer*/) { abort(); /* STUB */ }

// 0x080d7c00  receiving.c:524
void CCancelRequest(TConnection * /*Connection*/) { abort(); /* STUB */ }

// 0x080d7cb0  receiving.c:545
void CRotate(TConnection * /*Connection*/, int /*Direction*/) { abort(); /* STUB */ }

// 0x080d7d60  receiving.c:567
void CBugReport(TConnection * /*Connection*/, TReadBuffer & /*Buffer*/) { abort(); /* STUB */ }

// 0x080d8140  receiving.c:630
void CErrorFileEntry(TConnection * /*Connection*/, TReadBuffer & /*Buffer*/) { abort(); /* STUB */ }

// 0x080d8540  receiving.c:707
void CMoveObject(TConnection * /*Connection*/, TReadBuffer & /*Buffer*/) { abort(); /* STUB */ }

// 0x080d88e0  receiving.c:747
void CTradeObject(TConnection * /*Connection*/, TReadBuffer & /*Buffer*/) { abort(); /* STUB */ }

// 0x080d8b40  receiving.c:781
void CInspectTrade(TConnection * /*Connection*/, TReadBuffer & /*Buffer*/) { abort(); /* STUB */ }

// 0x080d8c50  receiving.c:805
void CAcceptTrade(TConnection * /*Connection*/) { abort(); /* STUB */ }

// 0x080d8c90  receiving.c:819
void CRejectTrade(TConnection * /*Connection*/) { abort(); /* STUB */ }

// 0x080d8cd0  receiving.c:834
void CUseObject(TConnection * /*Connection*/, TReadBuffer & /*Buffer*/) { abort(); /* STUB */ }

// 0x080d8fa0  receiving.c:870
void CUseTwoObjects(TConnection * /*Connection*/, TReadBuffer & /*Buffer*/) { abort(); /* STUB */ }

// 0x080d93d0  receiving.c:912
void CUseOnCreature(TConnection * /*Connection*/, TReadBuffer & /*Buffer*/) { abort(); /* STUB */ }

// 0x080d9840  receiving.c:961
void CTurnObject(TConnection * /*Connection*/, TReadBuffer & /*Buffer*/) { abort(); /* STUB */ }

// 0x080d9a90  receiving.c:994
void CCloseContainer(TConnection * /*Connection*/, TReadBuffer & /*Buffer*/) { abort(); /* STUB */ }

// 0x080d9b20  receiving.c:1012
void CUpContainer(TConnection * /*Connection*/, TReadBuffer & /*Buffer*/) { abort(); /* STUB */ }

// 0x080d9c80  receiving.c:1039
void CRefreshContainer(TConnection * /*Connection*/, TReadBuffer & /*Buffer*/) { abort(); /* STUB */ }

// 0x080d9d20  receiving.c:1058
void CGetOutfit(TConnection * /*Connection*/) { abort(); /* STUB */ }

// 0x080d9da0  receiving.c:1074
void CSetOutfit(TConnection * /*Connection*/, TReadBuffer & /*Buffer*/) { abort(); /* STUB */ }

// 0x080d9fb0  receiving.c:1112
void CEditText(TConnection * /*Connection*/, TReadBuffer & /*Buffer*/) { abort(); /* STUB */ }

// 0x080da1e0  receiving.c:1147
void CEditList(TConnection * /*Connection*/, TReadBuffer & /*Buffer*/) { abort(); /* STUB */ }

// 0x080da410  receiving.c:1183
void CSetTactics(TConnection * /*Connection*/, TReadBuffer & /*Buffer*/) { abort(); /* STUB */ }

// 0x080da540  receiving.c:1216
void CAttack(TConnection * /*Connection*/, TReadBuffer & /*Buffer*/, bool /*Follow*/) { abort(); /* STUB */ }

// 0x080da630  receiving.c:1240
void CInviteToParty(TConnection * /*Connection*/, TReadBuffer & /*Buffer*/) { abort(); /* STUB */ }

// 0x080da770  receiving.c:1258
void CJoinParty(TConnection * /*Connection*/, TReadBuffer & /*Buffer*/) { abort(); /* STUB */ }

// 0x080da7e0  receiving.c:1274
void CRevokeInvitation(TConnection * /*Connection*/, TReadBuffer & /*Buffer*/) { abort(); /* STUB */ }

// 0x080da890  receiving.c:1294
void CPassLeadership(TConnection * /*Connection*/, TReadBuffer & /*Buffer*/) { abort(); /* STUB */ }

// 0x080da940  receiving.c:1314
void CLeaveParty(TConnection * /*Connection*/) { abort(); /* STUB */ }

// 0x080da9c0  receiving.c:1333
void COpenChannel(TConnection * /*Connection*/) { abort(); /* STUB */ }

// 0x080daa50  receiving.c:1352
void CInviteToChannel(TConnection * /*Connection*/, TReadBuffer & /*Buffer*/) { abort(); /* STUB */ }

// 0x080dab30  receiving.c:1377
void CExcludeFromChannel(TConnection * /*Connection*/, TReadBuffer & /*Buffer*/) { abort(); /* STUB */ }

// 0x080dabf0  receiving.c:1398
void CCancel(TConnection * /*Connection*/) { abort(); /* STUB */ }

// 0x080dac90  receiving.c:1417
void CAddBuddy(TConnection * /*Connection*/, TReadBuffer & /*Buffer*/) { abort(); /* STUB */ }

// 0x080dad20  receiving.c:1435
void CRemoveBuddy(TConnection * /*Connection*/, TReadBuffer & /*Buffer*/) { abort(); /* STUB */ }

// 0x080dad90  receiving.c:1451
void CRuleViolation(TConnection * /*Connection*/, TReadBuffer & /*Buffer*/) { abort(); /* STUB */ }

// 0x080db170  receiving.c:1528
void CQuitGame(TConnection * /*Connection*/) { abort(); /* STUB */ }

// 0x080db210  receiving.c:1551
void ReceiveData(TConnection * /*Connection*/) { abort(); /* STUB */ }

// 0x080db990  receiving.c:1667
void ReceiveData() { abort(); /* STUB */ }
