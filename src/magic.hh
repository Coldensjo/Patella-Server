// magic.hh: the spells (magic.cc): the impacts (TImpact and its kinds) that a
// shaped spell applies and the spell functions. The spell table and the
// circles are in magicdata.hh.

#ifndef PATELLA_MAGIC_HH
#define PATELLA_MAGIC_HH

#include <cstdint>
#include <cstdlib>

#include "dbfuncs.hh"
#include "magicdata.hh"

struct Object;
struct ObjectType;
struct TCreature;

// The ammunition CreateArrows conjures, named after the spells that use them
// (Conjure Arrow, Poisoned Arrow, Explosive Arrow, Conjure Bolt, Power Bolt).
const int CONJURE_ARROW = 0;
const int CONJURE_POISONED_ARROW = 1;
const int CONJURE_EXPLOSIVE_ARROW = 2;
const int CONJURE_BOLT = 3;
const int CONJURE_POWER_BOLT = 4;

struct TImpact {
	virtual void handleField(int, int, int) {}
	virtual void handleCreature(TCreature *) {}
	virtual bool isAggressive() { return true; }
};

struct TDamageImpact : public TImpact {
	TDamageImpact(TCreature *_Actor, int _DamageType, int _Power, bool _AllowDefense);
	virtual void handleCreature(TCreature *Victim);
private:
	TCreature *Actor;
	int DamageType;
	int Power;
	bool AllowDefense;
};

struct TFieldImpact : public TImpact {
	TFieldImpact(TCreature *_Actor, int _FieldType);
	virtual void handleField(int x, int y, int z);
private:
	TCreature *Actor;
	int FieldType;
};

struct THealingImpact : public TImpact {
	THealingImpact(TCreature *_Actor, int _Power);
	virtual void handleCreature(TCreature *Victim);
	virtual bool isAggressive() { return false; }
private:
	TCreature *Actor;
	int Power;
};

struct TSpeedImpact : public TImpact {
	TSpeedImpact(TCreature *_Actor, int _Percent, int _Duration);
	virtual void handleCreature(TCreature *Victim);
private:
	TCreature *Actor;
	int Percent;
	int Duration;
};

struct TDrunkenImpact : public TImpact {
	TDrunkenImpact(TCreature *_Actor, int _Power, int _Duration);
	virtual void handleCreature(TCreature *Victim);
private:
	TCreature *Actor;
	int Power;
	int Duration;
};

struct TStrengthImpact : public TImpact {
	TStrengthImpact(TCreature *_Actor, int _Skills, int _Percent, int _Duration);
	virtual void handleCreature(TCreature *Victim);
private:
	TCreature *Actor;
	int Skills;
	int Percent;
	int Duration;
};

struct TOutfitImpact : public TImpact {
	TOutfitImpact(TCreature *_Actor, TOutfit _Outfit, int _Duration);
	virtual void handleCreature(TCreature *Victim);
private:
	TOutfit Outfit;
	int Duration;
};

struct TSummonImpact : public TImpact {
	TSummonImpact(TCreature *_Actor, int _Race, int _Maximum);
	virtual void handleField(int x, int y, int z);
private:
	TCreature *Actor;
	int Race;
	int Maximum;
};

int Direction(int dx, int dy);
void CheckSpellbook(TCreature *cr, int SpellNr);
void CheckAccount(TCreature *cr, int SpellNr);
void CheckLevel(TCreature *cr, int SpellNr);
void CheckRuneLevel(TCreature *cr, int SpellNr);
void CheckRing(TCreature *cr, int);
void CheckAffectedPlayers(TCreature *cr, int x, int y, int z);
void SpendManaAndSoulPoints(TCreature *cr, int Mana, int SoulPoints, int Delay);
// Whether the player CharacterID has the right of the command SpellNr's check
// Check (CommandRight, commands.toml); a check without a right is logged and
// false.
bool CheckCommandRight(uint32_t CharacterID, int SpellNr, int Check = COMMAND_RIGHT);
int ComputeDamage(TCreature *cr, int SpellNr, int Damage, int Variation);
bool IsAggressiveSpell(int SpellNr);
void ActorShapeSpell(TCreature *Actor, TImpact *Impact, int Effect);
void VictimShapeSpell(TCreature *Actor, TCreature *Victim, int Range, int Animation, TImpact *Impact, int Effect);
void OriginShapeSpell(TCreature *Actor, int Radius, TImpact *Impact, int Effect);
void DestinationShapeSpell(TCreature *Actor, TCreature *Victim, int Range, int Animation, int Radius, TImpact *Impact, int Effect);
void CircleShapeSpell(TCreature *Actor, int DestX, int DestY, int DestZ, int Range, int Animation, int Radius, TImpact *Impact, int Effect);
void AngleShapeSpell(TCreature *Actor, int Angle, int Range, TImpact *Impact, int Effect);
void MassCombat(TCreature *cr, Object Target, int Mana, int SoulPoints, int Damage, int EffectNr, int Radius, unsigned short DmgType, int Animation);
void AngleCombat(TCreature *cr, int Mana, int SoulPoints, int Damage, int EffectNr, int Radius, int Angle, unsigned short DmgType);
void Combat(TCreature *cr, Object Target, int Mana, int SoulPoints, int Damage, int EffectNr, int AnimationNr, unsigned short DmgType);
void CreateField(TCreature *cr, Object Obj, int Mana, int SoulPoints, int Type);
void CreateField(TCreature *cr, int Mana, int SoulPoints, int Type);
void MassCreateField(TCreature *cr, Object Obj, int Mana, int SoulPoints, int Type, int Radius);
void CreateFieldWall(TCreature *cr, Object Obj, int Mana, int SoulPoints, int Type, int Extension);
void DeleteField(TCreature *cr, Object Obj, int Mana, int SoulPoints);
void CleanupField(TCreature *cr, Object Obj, int Mana, int SoulPoints);
void MagicRope(TCreature *cr, int Mana, int SoulPoints);
void MagicClimbing(TCreature *cr, int Mana, int SoulPoints, char *Direction);
void CreateFood(TCreature *cr, int Mana, int SoulPoints);
void CreateArrows(TCreature *cr, int Mana, int SoulPoints, int Ammo, int Amount);
void SummonCreature(TCreature *cr, int Mana, int Race, bool God);
void SummonCreature(TCreature *cr, int Mana, char *RaceName, bool God);
void RaiseDead(TCreature *cr, Object Obj, int Mana, int SoulPoints);
void MassRaiseDead(TCreature *cr, Object Target, int Mana, int SoulPoints, int Radius);
void Heal(TCreature *cr, int Mana, int SoulPoints, int Healing);
void MassHeal(TCreature *cr, Object Target, int Mana, int SoulPoints, int Healing, int Radius);
void HealFriend(TCreature *cr, char *Name, int Mana, int SoulPoints, int Healing);
void RefreshMana(TCreature *cr, int Mana, int SoulPoints, int AddMana);
void MagicGoStrength(TCreature *cr, TCreature *dest, int Mana, int SoulPoints, int percent, int duration);
void Shielding(TCreature *cr, int Mana, int SoulPoints, int Duration);
void NegatePoison(TCreature *cr, int Mana, int SoulPoints, TCreature *dest);
void Enlight(TCreature *cr, int Mana, int SoulPoints, int Radius, int Duration);
void Invisibility(TCreature *cr, int Mana, int SoulPoints, int Duration);
void CancelInvisibility(TCreature *cr, Object Target, int Mana, int SoulPoints, int Radius);
void CreatureIllusion(TCreature *cr, int Mana, int SoulPoints, char *RaceName, int Duration);
void ObjectIllusion(TCreature *cr, int Mana, int SoulPoints, Object Obj, int Duration);
void EnchantObject(TCreature *cr, int Mana, int SoulPoints, ObjectType Origin, ObjectType Target);
void Convince(TCreature *master, TCreature *slave);
void Challenge(TCreature *cr, int Mana, int SoulPoints, int Radius);
void FindPerson(TCreature *cr, int Mana, int SoulPoints, char *Name);
void SpellFailed(uint32_t CreatureID);
// The words of the spoken spell are SpellStr[1] to SpellStr[LastWord], the
// last entry CheckForSpell set (the empty one that ends them); the spell's
// script gets them (scripting.hh).
void CharacterRightSpell(uint32_t CreatureID, int SpellNr, char (*SpellStr)[512], int LastWord);
void AccountRightSpell(uint32_t CreatureID, int SpellNr, char (*SpellStr)[512], int LastWord);
void CastSpell(uint32_t CreatureID, int SpellNr, char (*SpellStr)[512], int LastWord);
void RuneSpell(uint32_t CreatureID, int SpellNr);
int TypeOfSpell(char *Text);
int FindSpell(unsigned char *Syllable);
int FindSpell(Object Obj);
void GetSpellString(int SpellNr, char *Text);
void GetMagicItemDescription(Object Obj, char *SpellString, int &MagicLevel);
void GetSpellbook(uint32_t CharacterID, char *Buffer);
int GetSpellLevel(int SpellNr);
int CheckForSpell(uint32_t CreatureID, char *Text);
void DeleteRune(Object Obj);
void UseMagicItem(uint32_t CreatureID, Object Obj, Object Dest);
void InitMagic();

#endif
