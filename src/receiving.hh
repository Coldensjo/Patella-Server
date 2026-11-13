#ifndef PATELLA_RECEIVING_HH
#define PATELLA_RECEIVING_HH

struct ObjectType;
struct TConnection;
struct TReadBuffer;

bool CommandAllowed(TConnection *Connection, unsigned short Command);
bool CheckSpecialCoordinates(int Command, int x, int y, int z, bool AnyAllowed);
bool CheckVisibility(int Command, TConnection *Connection, int x, int y, int z);
bool CheckObjectType(int Command, int TypeID);
ObjectType GetObjectType(unsigned short TypeID);
void CGoDirection(TConnection *Connection, int dx, int dy);
void CGoPath(TConnection *Connection, TReadBuffer &Buffer);
void CStop(TConnection *Connection);
void CLookAtPoint(TConnection *Connection, TReadBuffer &Buffer);
void CRefreshField(TConnection *Connection, TReadBuffer &Buffer);
void CTalk(TConnection *Connection, TReadBuffer &Buffer);
void CGetChannels(TConnection *Connection);
void CJoinChannel(TConnection *Connection, TReadBuffer &Buffer);
void CLeaveChannel(TConnection *Connection, TReadBuffer &Buffer);
void CPrivateChannel(TConnection *Connection, TReadBuffer &Buffer);
void CProcessRequest(TConnection *Connection, TReadBuffer &Buffer);
void CRemoveRequest(TConnection *Connection, TReadBuffer &Buffer);
void CCancelRequest(TConnection *Connection);
void CRotate(TConnection *Connection, int Direction);
void CBugReport(TConnection *Connection, TReadBuffer &Buffer);
void CErrorFileEntry(TConnection *Connection, TReadBuffer &Buffer);
void CMoveObject(TConnection *Connection, TReadBuffer &Buffer);
void CTradeObject(TConnection *Connection, TReadBuffer &Buffer);
void CInspectTrade(TConnection *Connection, TReadBuffer &Buffer);
void CAcceptTrade(TConnection *Connection);
void CRejectTrade(TConnection *Connection);
void CUseObject(TConnection *Connection, TReadBuffer &Buffer);
void CUseTwoObjects(TConnection *Connection, TReadBuffer &Buffer);
void CUseOnCreature(TConnection *Connection, TReadBuffer &Buffer);
void CTurnObject(TConnection *Connection, TReadBuffer &Buffer);
void CCloseContainer(TConnection *Connection, TReadBuffer &Buffer);
void CUpContainer(TConnection *Connection, TReadBuffer &Buffer);
void CRefreshContainer(TConnection *Connection, TReadBuffer &Buffer);
void CGetOutfit(TConnection *Connection);
void CSetOutfit(TConnection *Connection, TReadBuffer &Buffer);
void CEditText(TConnection *Connection, TReadBuffer &Buffer);
void CEditList(TConnection *Connection, TReadBuffer &Buffer);
void CSetTactics(TConnection *Connection, TReadBuffer &Buffer);
void CAttack(TConnection *Connection, TReadBuffer &Buffer, bool Follow);
void CInviteToParty(TConnection *Connection, TReadBuffer &Buffer);
void CJoinParty(TConnection *Connection, TReadBuffer &Buffer);
void CRevokeInvitation(TConnection *Connection, TReadBuffer &Buffer);
void CPassLeadership(TConnection *Connection, TReadBuffer &Buffer);
void CLeaveParty(TConnection *Connection);
void COpenChannel(TConnection *Connection);
void CInviteToChannel(TConnection *Connection, TReadBuffer &Buffer);
void CExcludeFromChannel(TConnection *Connection, TReadBuffer &Buffer);
void CCancel(TConnection *Connection);
void CAddBuddy(TConnection *Connection, TReadBuffer &Buffer);
void CRemoveBuddy(TConnection *Connection, TReadBuffer &Buffer);
void CRuleViolation(TConnection *Connection, TReadBuffer &Buffer);
void CQuitGame(TConnection *Connection);
void ReceiveData(TConnection *Connection);
void ReceiveData();

#endif
