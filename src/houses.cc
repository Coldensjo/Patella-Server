#include <cstdint>
#include <cstdlib>

#include "script.hh"
#include "threads.hh"
#include "dbfuncs.hh"
#include "query.hh"
#include "config.hh"
#include "objects.hh"
#include "map.hh"
#include "info.hh"
#include "cr.hh"
#include "containr.hh"
#include "operate.hh"
#include "utils.hh"
#include "time.hh"
#include "houses.hh"

struct THouseGuest {
	char Name[60];
	// inline  houses.c:31
	operator char *() { abort(); /* STUB */ }
};

struct THouseArea {
	unsigned short ID;
	int SQMPrice;
	int DepotNr;
};

struct THelpDepot {
	uint32_t CharacterID;
	Object Box;
	int DepotNr;
};

struct THouse {
	unsigned short ID;
	char Name[50];
	char Description[500];
	int Size;
	uint32_t Rent;
	int DepotNr;
	bool NoAuction;
	bool GuildHouse;
	int ExitX;
	int ExitY;
	int ExitZ;
	int CenterX;
	int CenterY;
	int CenterZ;
	uint32_t OwnerID;
	char OwnerName[30];
	int LastTransition;
	int PaidUntil;
	vector<THouseGuest> Subowner;
	int Subowners;
	vector<THouseGuest> Guest;
	int Guests;
	int Help;
	// inline  houses.c:67
	THouse() : Subowner(0, 4, 5), Guest(0, 9, 10) { abort(); /* STUB */ }
};

vector<THouseArea> HouseArea(0, 99, 50);	// houses.c:71
int HouseAreas;	// houses.c:72
vector<THouse> House(0, 99, 100);	// houses.c:73
int Houses;	// houses.c:74
int MaxHouseX;	// houses.c:75
int MaxHouseY;	// houses.c:75
[[maybe_unused]] static TQueryManagerConnection *QueryManagerConnection;	// houses.c:76
int PaymentExtension;	// houses.c:77
vector<THelpDepot> HelpDepot(1, 10, 10);	// houses.c:78
int HelpDepots;	// houses.c:79

// 0x08060d60  houses.c:90
THouseArea *GetHouseArea(unsigned short /*ID*/) { abort(); /* STUB */ }

// 0x08060dd0  houses.c:104
int CheckAccessRight(char * /*Rule*/, TPlayer * /*pl*/) { abort(); /* STUB */ }

// 0x08060ef0  houses.c:133
THouse *GetHouse(unsigned short /*ID*/) { abort(); /* STUB */ }

// 0x08060fa0  houses.c:150
bool IsOwner(unsigned short /*HouseID*/, TPlayer * /*pl*/) { abort(); /* STUB */ }

// 0x08061010  houses.c:167
bool IsSubowner(unsigned short /*HouseID*/, TPlayer * /*pl*/, int /*TimeStamp*/) { abort(); /* STUB */ }

// 0x080610b0  houses.c:190
bool IsGuest(unsigned short /*HouseID*/, TPlayer * /*pl*/, int /*TimeStamp*/) { abort(); /* STUB */ }

// 0x08061150  houses.c:213
bool IsInvited(unsigned short /*HouseID*/, TPlayer * /*pl*/, int /*TimeStamp*/) { abort(); /* STUB */ }

// 0x08061200  houses.c:230
char *GetHouseName(unsigned short /*HouseID*/) { abort(); /* STUB */ }

// 0x08061240  houses.c:243
char *GetHouseOwner(unsigned short /*HouseID*/) { abort(); /* STUB */ }

// 0x08061280  houses.c:262
void ShowSubownerList(unsigned short /*HouseID*/, TPlayer * /*pl*/, char * /*Buffer*/) { abort(); /* STUB */ }

// 0x08061410  houses.c:291
void ShowGuestList(unsigned short /*HouseID*/, TPlayer * /*pl*/, char * /*Buffer*/) { abort(); /* STUB */ }

// 0x080615d0  houses.c:321
void ChangeSubowners(unsigned short /*HouseID*/, TPlayer * /*pl*/, char * /*Buffer*/) { abort(); /* STUB */ }

// 0x080617b0  houses.c:367
void ChangeGuests(unsigned short /*HouseID*/, TPlayer * /*pl*/, char * /*Buffer*/) { abort(); /* STUB */ }

// 0x080619d0  houses.c:418
void GetExitPosition(unsigned short /*HouseID*/, int & /*x*/, int & /*y*/, int & /*z*/) { abort(); /* STUB */ }

// 0x08061a40  houses.c:434
void KickGuest(unsigned short /*HouseID*/, TPlayer * /*Guest*/) { abort(); /* STUB */ }

// 0x08061ce0  houses.c:466
void KickGuest(unsigned short /*HouseID*/, TPlayer * /*Host*/, TPlayer * /*Guest*/) { abort(); /* STUB */ }

// 0x08061ed0  houses.c:493
void KickGuests(unsigned short /*HouseID*/) { abort(); /* STUB */ }

// 0x08062010  houses.c:521
bool MayOpenDoor(Object /*Door*/, TPlayer * /*pl*/) { abort(); /* STUB */ }

// 0x08062180  houses.c:566
void ShowNameDoor(Object /*Door*/, TPlayer * /*pl*/, char * /*Buffer*/) { abort(); /* STUB */ }

// 0x080623f0  houses.c:606
void ChangeNameDoor(Object /*Door*/, TPlayer * /*pl*/, char * /*Buffer*/) { abort(); /* STUB */ }

// 0x08062620  houses.c:647
int DeleteCoins(Object /*Con*/, ObjectType /*Type*/, int /*Amount*/) { abort(); /* STUB */ }

// 0x080627a0  houses.c:675
void DeleteMoney(Object /*Con*/, int /*Amount*/) { abort(); /* STUB */ }

// 0x08062a00  houses.c:693
void CleanField(int /*x*/, int /*y*/, int /*z*/, Object /*Box*/) { abort(); /* STUB */ }

// 0x08062ce0  houses.c:737
void CleanHouse(THouse * /*house*/, TPlayerData * /*PlayerData*/) { abort(); /* STUB */ }

// 0x08063070  houses.c:778
void ClearHouse(THouse * /*house*/) { abort(); /* STUB */ }

// 0x080630a0  houses.c:790
void FinishAuctions() { abort(); /* STUB */ }

// 0x08063760  houses.c:879
void CollectRents() { abort(); /* STUB */ }

// 0x08064b30  houses.c:1097
void StartAuctions() { abort(); /* STUB */ }

// 0x08065120  houses.c:1178
void PrepareHouseCleanup() { abort(); /* STUB */ }

// 0x08065130  houses.c:1186
void FinishHouseCleanup() { abort(); /* STUB */ }

// 0x08065210  houses.c:1204
void CleanHouseField(int /*x*/, int /*y*/, int /*z*/) { abort(); /* STUB */ }

// 0x080655b0  houses.c:1258
void LoadHouseAreas() { abort(); /* STUB */ }

// 0x08065790  houses.c:1293
void LoadHouses() { abort(); /* STUB */ }

// 0x080663d0  houses.c:1407
void LoadOwners() { abort(); /* STUB */ }

// 0x08066c10  houses.c:1522
void SaveOwners() { abort(); /* STUB */ }

// 0x08066f30  houses.c:1578
void InitHouses() { abort(); /* STUB */ }

// 0x08067000  houses.c:1599
void ExitHouses() { abort(); /* STUB */ }
