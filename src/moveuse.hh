// moveuse.hh: the functions and data of moveuse.cc: the events that run the
// move and use rules (moveusedata.hh), the object functions their actions use,
// depot boxes, mail, and the built-in uses of objects (containers, chests,
// liquids, food, texts, doors, weapons).

#ifndef PATELLA_MOVEUSE_HH
#define PATELLA_MOVEUSE_HH

#include <cstdint>

#include "containr.hh"

struct Object;
struct ObjectType;
struct TDelayedMail;
struct TPlayerData;

Object GetEventObject(int Number, Object User, Object Obj1, Object Obj2, Object Temp);
bool Compare(int Value1, int Operator, int Value2);
Object CreateObject(Object Con, ObjectType Type, uint32_t Value);
void ChangeObject(Object Obj, ObjectType NewType, uint32_t Value);
void MoveOneObject(Object Obj, Object Con);
void MoveAllObjects(Object Obj, Object Dest, Object Exclude, bool MoveUnmovable);
void DeleteAllObjects(Object Obj, Object Exclude, bool DeleteUnmovable);
void ClearField(Object Obj, Object Exclude);
void LoadDepotBox(uint32_t CreatureID, int Nr, Object Con);
void SaveDepotBox(uint32_t CreatureID, int Nr, Object Con);
void SendMail(Object Obj);
void SendMails(TPlayerData *PlayerData);
void TextEffect(char *Text, int x, int y, int z, int Radius);
void UseContainer(uint32_t CreatureID, Object Con, unsigned char Window);
void UseChest(uint32_t CreatureID, Object Chest);
void UseLiquidContainer(uint32_t CreatureID, Object Obj, Object Dest);
void UseFood(uint32_t CreatureID, Object Obj);
void UseTextObject(uint32_t CreatureID, Object Obj);
void UseAnnouncer(uint32_t CreatureID, Object Obj);
void UseKeyDoor(uint32_t, Object Key, Object Door);
void UseNameDoor(uint32_t CreatureID, Object Door);
void UseLevelDoor(uint32_t CreatureID, Object Door);
void UseQuestDoor(uint32_t CreatureID, Object Door);
void UseWeapon(uint32_t CreatureID, Object Weapon, Object Target);
void UseChangeObject(uint32_t CreatureID, Object Obj);
void UseObject(uint32_t CreatureID, Object Obj);
void UseObjects(uint32_t CreatureID, Object Obj1, Object Obj2);
void MovementEvent(Object Obj, Object Start, Object Dest);
void SeparationEvent(Object Obj, Object Start);
void CollisionEvent(Object Obj, Object Dest);
void InitMoveUse();
void ExitMoveUse();

extern vector<TDelayedMail> DelayedMail;
extern int DelayedMails;

#endif
