#ifndef PATELLA_HOUSES_HH
#define PATELLA_HOUSES_HH

#include "containr.hh"

struct Object;
struct ObjectType;
struct THelpDepot;
struct THouse;
struct THouseArea;
struct TPlayer;
struct TPlayerData;

THouseArea *GetHouseArea(unsigned short ID);
int CheckAccessRight(char *Rule, TPlayer *pl);
THouse *GetHouse(unsigned short ID);
bool IsOwner(unsigned short HouseID, TPlayer *pl);
bool IsSubowner(unsigned short HouseID, TPlayer *pl, int TimeStamp);
bool IsGuest(unsigned short HouseID, TPlayer *pl, int TimeStamp);
bool IsInvited(unsigned short HouseID, TPlayer *pl, int TimeStamp);
char *GetHouseName(unsigned short HouseID);
char *GetHouseOwner(unsigned short HouseID);
void ShowSubownerList(unsigned short HouseID, TPlayer *pl, char *Buffer);
void ShowGuestList(unsigned short HouseID, TPlayer *pl, char *Buffer);
void ChangeSubowners(unsigned short HouseID, TPlayer *pl, char *Buffer);
void ChangeGuests(unsigned short HouseID, TPlayer *pl, char *Buffer);
void GetExitPosition(unsigned short HouseID, int &x, int &y, int &z);
void KickGuest(unsigned short HouseID, TPlayer *Guest);
void KickGuest(unsigned short HouseID, TPlayer *Host, TPlayer *Guest);
void KickGuests(unsigned short HouseID);
bool MayOpenDoor(Object Door, TPlayer *pl);
void ShowNameDoor(Object Door, TPlayer *pl, char *Buffer);
void ChangeNameDoor(Object Door, TPlayer *pl, char *Buffer);
int DeleteCoins(Object Con, ObjectType Type, int Amount);
void DeleteMoney(Object Con, int Amount);
void CleanField(int x, int y, int z, Object Box);
void CleanHouse(THouse *house, TPlayerData *PlayerData);
void ClearHouse(THouse *house);
void FinishAuctions();
void CollectRents();
void StartAuctions();
void PrepareHouseCleanup();
void FinishHouseCleanup();
void CleanHouseField(int x, int y, int z);
void LoadHouseAreas();
void LoadHouses();
void LoadOwners();
void SaveOwners();
void InitHouses();
void ExitHouses();

extern vector<THouseArea> HouseArea;
extern int HouseAreas;
extern vector<THouse> House;
extern int Houses;
extern int MaxHouseX;
extern int MaxHouseY;
extern int PaymentExtension;
extern vector<THelpDepot> HelpDepot;
extern int HelpDepots;

#endif
