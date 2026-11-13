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
#include "utils.hh"
#include "time.hh"

// 0x0806df00  info.c:29
const char *GetName(Object /*Obj*/) { abort(); /* STUB */ }

// 0x0806e100  info.c:57
const char *GetInfo(Object /*Obj*/) { abort(); /* STUB */ }

// 0x0806e150  info.c:71
int GetWeight(Object /*Obj*/, int /*Count*/) { abort(); /* STUB */ }

// 0x0806e2e0  info.c:96
int GetRowWeight(Object /*Obj*/) { abort(); /* STUB */ }

// 0x0806e3f0  info.c:110
int GetCompleteWeight(Object /*Obj*/) { abort(); /* STUB */ }

// 0x0806e490  info.c:121
uint32_t GetObjectCreatureID(Object /*Obj*/) { abort(); /* STUB */ }

// 0x0806e520  info.c:138
int GetObjectBodyPosition(Object /*Obj*/) { abort(); /* STUB */ }

// 0x0806e5d0  info.c:151
int GetObjectRNum(Object /*Obj*/) { abort(); /* STUB */ }

// 0x0806e6b0  info.c:178
bool ObjectInRange(uint32_t /*CreatureID*/, Object /*Obj*/, int /*Range*/) { abort(); /* STUB */ }

// 0x0806e790  info.c:199
bool ObjectAccessible(uint32_t /*CreatureID*/, Object /*Obj*/, int /*Range*/) { abort(); /* STUB */ }

// 0x0806ea50  info.c:227
const char *GetLiquidName(uint32_t /*nr*/) { abort(); /* STUB */ }

// 0x0806eae0  info.c:252
unsigned char GetLiquidColor(uint32_t /*nr*/) { abort(); /* STUB */ }

// 0x0806eb50  info.c:277
int ObjectDistance(Object /*Obj1*/, Object /*Obj2*/) { abort(); /* STUB */ }

// 0x0806ec30  info.c:300
Object GetBodyContainer(uint32_t /*CreatureID*/, int /*Position*/) { abort(); /* STUB */ }

// 0x0806ed50  info.c:327
Object GetBodyObject(uint32_t /*CreatureID*/, int /*Position*/) { abort(); /* STUB */ }

// 0x0806edd0  info.c:343
Object GetTopObject(int /*x*/, int /*y*/, int /*z*/, bool /*Move*/) { abort(); /* STUB */ }

// 0x0806ef40  info.c:357
Object GetContainer(uint32_t /*CreatureID*/, int /*x*/, int /*y*/, int /*z*/) { abort(); /* STUB */ }

// 0x0806efa0  info.c:368
Object GetObject(uint32_t /*CreatureID*/, int /*x*/, int /*y*/, int /*z*/, int /*RNum*/, ObjectType /*Type*/) { abort(); /* STUB */ }

// 0x0806f2f0  info.c:415
Object GetRowObject(Object /*Obj*/, ObjectType /*Type*/, uint32_t /*Value*/, bool /*Recurse*/) { abort(); /* STUB */ }

// 0x0806f4a0  info.c:435
Object GetInventoryObject(uint32_t /*CreatureID*/, ObjectType /*Type*/, uint32_t /*Value*/) { abort(); /* STUB */ }

// 0x0806f740  info.c:464
int CountObjectsInContainer(Object /*Con*/) { abort(); /* STUB */ }

// 0x0806f7d0  info.c:483
int CountObjects(Object /*Obj*/) { abort(); /* STUB */ }

// 0x0806f8a0  info.c:501
int CountObjects(Object /*Obj*/, ObjectType /*Type*/, uint32_t /*Value*/) { abort(); /* STUB */ }

// 0x0806fa80  info.c:519
int CountInventoryObjects(uint32_t /*CreatureID*/, ObjectType /*Type*/, uint32_t /*Value*/) { abort(); /* STUB */ }

// 0x0806fb40  info.c:536
int CountMoney(Object /*Obj*/) { abort(); /* STUB */ }

// 0x0806fce0  info.c:553
int CountInventoryMoney(uint32_t /*CreatureID*/) { abort(); /* STUB */ }

// 0x0806fd70  info.c:573
void CalculateChange(int /*Amount*/, int & /*Gold*/, int & /*Platinum*/, int & /*Crystal*/) { abort(); /* STUB */ }

// 0x0806ff60  info.c:618
int GetHeight(int /*x*/, int /*y*/, int /*z*/) { abort(); /* STUB */ }

// 0x08070030  info.c:629
bool JumpPossible(int /*x*/, int /*y*/, int /*z*/, bool /*AvoidPlayers*/) { abort(); /* STUB */ }

// 0x08070190  info.c:649
bool FieldPossible(int /*x*/, int /*y*/, int /*z*/, int /*Type*/) { abort(); /* STUB */ }

// 0x08070320  info.c:668
bool SearchFreeField(int & /*x*/, int & /*y*/, int & /*z*/, int /*Distance*/, unsigned short /*HouseID*/, bool /*Jump*/) { abort(); /* STUB */ }

// 0x080705d0  info.c:696
bool SearchLoginField(int & /*x*/, int & /*y*/, int & /*z*/, int /*Distance*/, bool /*Player*/) { abort(); /* STUB */ }

// 0x08070950  info.c:742
bool SearchSpawnField(int & /*x*/, int & /*y*/, int & /*z*/, int /*Distance*/, bool /*Player*/) { abort(); /* STUB */ }

// 0x08071090  info.c:808
bool SearchFlightField(uint32_t /*FugitiveID*/, uint32_t /*PursuerID*/, int & /*x*/, int & /*y*/, int & /*z*/) { abort(); /* STUB */ }

// 0x08071310  info.c:869
bool SearchSummonField(int & /*x*/, int & /*y*/, int & /*z*/, int /*Distance*/) { abort(); /* STUB */ }

// 0x08071520  info.c:894
bool ThrowPossible(int /*sx*/, int /*sy*/, int /*sz*/, int /*zx*/, int /*zy*/, int /*zz*/, int /*Power*/) { abort(); /* STUB */ }

// 0x08071830  info.c:941
void GetCreatureLight(uint32_t /*CreatureID*/, int & /*Brightness*/, int & /*Color*/) { abort(); /* STUB */ }

// 0x08071ad0  info.c:979
int GetInventoryWeight(uint32_t /*CreatureID*/) { abort(); /* STUB */ }

// 0x08071b30  info.c:997
bool CheckRight(uint32_t /*CharacterID*/, RIGHT /*Right*/) { abort(); /* STUB */ }

// 0x08071bb0  info.c:1014
bool CheckBanishmentRight(uint32_t /*CharacterID*/, int /*Reason*/, int /*Action*/) { abort(); /* STUB */ }

// 0x08071f70  info.c:1052
const char *GetBanishmentReason(int /*Reason*/) { abort(); /* STUB */ }

// 0x080720c0  info.c:1100
void InitInfo() { abort(); /* STUB */ }

// 0x080720d0  info.c:1108
void ExitInfo() { abort(); /* STUB */ }
