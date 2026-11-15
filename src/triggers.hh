// triggers.hh: the functions and data of triggers.cc: the events that run the
// trigger rules (triggersdata.hh, the trigger library of data/scripts/lib/),
// the object functions their actions use, depot boxes, mail, opening a
// container window (UseContainer) and the trigger rules of use and multi-use
// (UseObject, UseObjects); the other built-in uses of objects are scripts
// (data/scripts/items/).

#ifndef PATELLA_TRIGGERS_HH
#define PATELLA_TRIGGERS_HH

#include <cstdint>

#include "containr.hh"

struct Object;
struct ObjectType;
struct TDelayedMail;
struct TPlayerData;

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
void UseObject(uint32_t CreatureID, Object Obj);
void UseObjects(uint32_t CreatureID, Object Obj1, Object Obj2);
void MovementEvent(Object Obj, Object Start, Object Dest);
void SeparationEvent(Object Obj, Object Start);
void CollisionEvent(Object Obj, Object Dest);
void InitTriggers();
void ExitTriggers();

extern vector<TDelayedMail> DelayedMail;
extern int DelayedMails;

#endif
