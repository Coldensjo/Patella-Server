// magic.cc: the spells. CheckForSpell, which parses a spoken text into
// syllables and finds the spell; the checks a spell passes (spellbook, level,
// mana, ...); the impacts (TImpact) and the shapes that apply them to fields
// and creatures, which the monsters' spells use too; the spell functions, from
// attack and healing spells to the gamemasters' commands; runes, magic items
// and potions. The spell table, the syllables and the circles are in
// magicdata.cc.

#include <algorithm>
#include <climits>
#include <cstdint>
#include <cstdio>
#include <cstdlib>

#include "script.hh"
#include "threads.hh"
#include "dbfuncs.hh"
#include "config.hh"
#include "objects.hh"
#include "map.hh"
#include "info.hh"
#include "cr.hh"
#include "containr.hh"
#include "connections.hh"
#include "operate.hh"
#include "magic.hh"
#include "magicdata.hh"
#include "utils.hh"
#include "time.hh"
#include "TXTEASymmetricKey.hh"
#include "crmain.hh"
#include "crnonpl.hh"
#include "crplayer.hh"
#include "mudinc.hh"
#include "sending.hh"
#include "writer.hh"
#include <cstring>
#include <sstream>
#include "houses.hh"

// CheckSpellbook, CheckAccount, CheckLevel, CheckRuneLevel, CheckRing,
// CheckAffectedPlayers, SpendManaAndSoulPoints and every spell function from MassCombat
// to ChangeData report a failure by throwing a RESULT. They have no exception
// specification (C++17 has none), so an exception of another type passes to
// the caller too.

// The ammunition CreateArrows conjures, named after the spells that use them
// (Conjure Arrow, Poisoned Arrow, Explosive Arrow, Conjure Bolt, Power Bolt).
const int CONJURE_ARROW = 0;
const int CONJURE_POISONED_ARROW = 1;
const int CONJURE_EXPLOSIVE_ARROW = 2;
const int CONJURE_BOLT = 3;
const int CONJURE_POWER_BOLT = 4;

// The creatures a player can have summoned or convinced at once
// (SummonCreature, Convince).
const int MAX_SUMMONS = 2;

static void CreateField(int x, int y, int z, int FieldType, uint32_t Owner, bool Peaceful);

int Direction(int dx, int dy) {
	if (dx == 0) {
		if (dy == 0) {
			return COMPASS_NONE;
		}
		if (dy > 0) {
			return COMPASS_SOUTH;
		}
		return COMPASS_NORTH;
	}

	int s = (dy << 8) / dx;
	int r;
	if (s < -618) {
		r = COMPASS_NORTH;
	} else if (s < -106) {
		r = COMPASS_NORTHEAST;
	} else if (s < 106) {
		r = COMPASS_EAST;
	} else if (s < 618) {
		r = COMPASS_SOUTHEAST;
	} else {
		r = COMPASS_SOUTH;
	}

	if (dx < 0) {
		r += 4;
	}
	if (r > 8) {
		r -= 8;
	}
	return r;
}

void CheckSpellbook(TCreature *cr, int SpellNr) {
	if (cr == nullptr) {
		error("CheckSpellbook: Given creature does not exist.\n");
		throw ERROR;
	}

	if (cr->Type == PLAYER) {
		TPlayer *pl = static_cast<TPlayer *>(cr);
		if (!CheckRight(cr->ID, ALL_SPELLS) && !pl->SpellKnown(SpellNr)) {
			throw SPELLUNKNOWN;
		}
	}
}

void CheckAccount(TCreature *cr, int SpellNr) {
	if (cr == nullptr) {
		error("CheckAccount: Given creature does not exist.\n");
		throw ERROR;
	}

	if (SpellNr < 1 || SpellNr > MAX_SPELLS - 1) {
		error("CheckAccount: Invalid spell number %d.\n", SpellNr);
		throw ERROR;
	}

	if (cr->Type == PLAYER && (SpellList[SpellNr].Flags & SPELLFLAG_PREMIUM) != 0
			&& !CheckRight(cr->ID, PREMIUM_ACCOUNT)) {
		throw NOPREMIUMACCOUNT;
	}
}

void CheckLevel(TCreature *cr, int SpellNr) {
	if (cr == nullptr) {
		error("CheckLevel: Given creature does not exist.\n");
		throw ERROR;
	}

	if (cr->Type != PLAYER) {
		return;
	}

	if (CheckRight(cr->ID, ALL_SPELLS)) {
		return;
	}

	int Level = SpellList[SpellNr].Level;
	TSkill *sk = cr->Skills[SKILL_LEVEL];
	if (sk == nullptr) {
		error("CheckLevel: No skill LEVEL.\n");
		throw ERROR;
	}

	if (sk->Get() < Level) {
		throw LOWLEVEL;
	}
}

void CheckRuneLevel(TCreature *cr, int SpellNr) {
	if (cr == nullptr) {
		error("CheckRuneLevel: Given creature does not exist.\n");
		throw ERROR;
	}

	if (cr->Type != PLAYER) {
		return;
	}

	if (CheckRight(cr->ID, ALL_SPELLS)) {
		return;
	}

	int Level = SpellList[SpellNr].RuneLevel;
	TSkill *sk = cr->Skills[SKILL_MAGICLEVEL];
	if (sk == nullptr) {
		error("CheckLevel: No skill MAGLEVEL.\n");
		throw ERROR;
	}

	if (sk->Get() < Level) {
		throw LOWMAGICLEVEL;
	}
}

void CheckRing(TCreature *cr, int) {
	if (cr == nullptr) {
		error("CheckRing: Given creature does not exist.\n");
		throw ERROR;
	}

	if (cr->Type != PLAYER) {
		return;
	}

	if (CheckRight(cr->ID, ALL_SPELLS)) {
		return;
	}
}

void CheckAffectedPlayers(TCreature *cr, int x, int y, int z) {
	if (cr == nullptr) {
		error("CheckAffectedPlayers: Given creature does not exist.\n");
		throw ERROR;
	}

	if (cr->Type != PLAYER || cr->Combat.GetSecureMode() == SECURE_MODE_OFF || WorldType != NORMAL) {
		return;
	}

	Object Obj = GetFirstObject(x, y, z);
	while (Obj != NONE) {
		if (Obj.isCreature()
				&& IsCreaturePlayer(Obj.getCreatureID())
				&& Obj.getCreatureID() != cr->ID
				&& !static_cast<TPlayer *>(cr)->IsAttackJustified(Obj.getCreatureID())) {
			throw SECUREMODE;
		}
		Obj++;
	}
}

void SpendManaAndSoulPoints(TCreature *cr, int Mana, int SoulPoints, int Delay) {
	if (cr == nullptr) {
		error("SpendManaAndSoulPoints: Given creature does not exist.\n");
		throw ERROR;
	}

	if (cr->Type != PLAYER || Mana < 0) {
		return;
	}

	TSkill *mana = cr->Skills[SKILL_MANA];
	TSkill *soulpoints = cr->Skills[SKILL_SOULPOINTS];
	if (mana == nullptr) {
		error("SpendManaAndSoulPoints: No skill MANA!\n");
		throw ERROR;
	}

	if (soulpoints == nullptr) {
		error("SpendManaAndSoulPoints: No skill SOULPOINTS!\n");
		throw ERROR;
	}

	if (!CheckRight(cr->ID, UNLIMITED_MANA)) {
		if (mana->Get() < Mana) {
			throw NOTENOUGHMANA;
		}

		if (soulpoints->Get() < SoulPoints) {
			throw NOTENOUGHSOULPOINTS;
		}

		mana->Change(-Mana);
		soulpoints->Change(-SoulPoints);
	}

	if (Mana > 0) {
		cr->Skills[SKILL_MAGICLEVEL]->Increase(Mana);
	}

	cr->EarliestSpellTime = std::max<uint32_t>(cr->EarliestSpellTime, ServerMilliseconds + Delay);
}

int ComputeDamage(TCreature *cr, int SpellNr, int Damage, int Variation) {
	if (Variation != 0) {
		Damage += random(-Variation, Variation);
	}

	if (cr == nullptr || cr->Type != PLAYER) {
		return Damage;
	}

	int DamagePercent = cr->Skills[SKILL_LEVEL]->Get() * 2;
	DamagePercent += cr->Skills[SKILL_MAGICLEVEL]->Get() * 3;
	if (SpellNr != 0) {
		if ((SpellList[SpellNr].Flags & SPELLFLAG_FACTOR_MAX_100) != 0 && DamagePercent > 100) {
			DamagePercent = 100;
		}
		if ((SpellList[SpellNr].Flags & SPELLFLAG_FACTOR_MIN_100) != 0 && DamagePercent < 100) {
			DamagePercent = 100;
		}
	}

	int dam = Damage * DamagePercent / 100;
	return dam;
}

bool IsAggressiveSpell(int SpellNr) {
	if (SpellNr < 1 || SpellNr > MAX_SPELLS - 1) {
		error("IsAggressiveSpell: Invalid spell number %d.\n", SpellNr);
		return false;
	}

	return (SpellList[SpellNr].Flags & SPELLFLAG_AGGRESSIVE) != 0;
}

TDamageImpact::TDamageImpact(TCreature *_Actor, int _DamageType, int _Power, bool _AllowDefense) {
	if (_Actor == nullptr) {
		error("TDamageImpact::TDamageImpact: Actor is NULL.\n");
	}

	Actor = _Actor;
	DamageType = _DamageType;
	Power = _Power;
	AllowDefense = _AllowDefense;
}

void TDamageImpact::handleCreature(TCreature *Victim) {
	if (Victim == nullptr) {
		error("TDamageImpact::handleCreature: Victim does not exist.\n");
		return;
	}

	if (Actor == nullptr || Victim == Actor) {
		return;
	}

	if (WorldType == NON_PVP && Actor->IsPeaceful() && Victim->IsPeaceful()) {
		return;
	}

	int Help = Power;
	if (DamageType == DAMAGE_PHYSICAL && AllowDefense) {
		Help -= Victim->Combat.RollDefense();
	}

	Victim->Damage(Actor, Help, DamageType);
}

TFieldImpact::TFieldImpact(TCreature *_Actor, int _FieldType) {
	if (_Actor == nullptr) {
		error("TFieldImpact::TFieldImpact: Actor is NULL.\n");
	}

	Actor = _Actor;
	FieldType = _FieldType;
}

void TFieldImpact::handleField(int x, int y, int z) {
	if (Actor == nullptr) {
		return;
	}

	try {
		CreateField(x, y, z, FieldType, Actor->ID, (WorldType == NON_PVP && Actor->IsPeaceful()));
	} catch (RESULT) {
	}
}

THealingImpact::THealingImpact(TCreature *_Actor, int _Power) {
	if (_Actor == nullptr) {
		error("THealingImpact::THealingImpact: Actor is NULL.\n");
	}

	if (_Power < 0) {
		error("THealingImpact::THealingImpact: Power is negative (Actor: %s).\n",
				(_Actor != nullptr ? _Actor->Name : "(unknown)"));
	}

	Actor = _Actor;
	Power = _Power;
}

void THealingImpact::handleCreature(TCreature *Victim) {
	if (Victim == nullptr) {
		error("THealingImpact::handleCreature: Victim does not exist.\n");
		return;
	}

	if (Actor == nullptr || Power < 0) {
		return;
	}

	if (Victim->Skills[SKILL_HITPOINTS]->Get() > 0) {
		Victim->Skills[SKILL_HITPOINTS]->Change(Power);
		if (Victim->Skills[SKILL_GOSTRENGTH]->GetMDAct() < 0) {
			Victim->SetTimer(SKILL_GOSTRENGTH, 0, 0, 0, -1);
		}
	}
}

TSpeedImpact::TSpeedImpact(TCreature *_Actor, int _Percent, int _Duration) {
	if (_Actor == nullptr) {
		error("TSpeedImpact::TSpeedImpact: Actor is NULL.\n");
	}

	Actor = _Actor;
	Percent = _Percent;
	Duration = _Duration;
}

void TSpeedImpact::handleCreature(TCreature *Victim) {
	if (Victim == nullptr) {
		error("TSpeedImpact::handleCreature: Victim does not exist.\n");
		return;
	}

	if (Actor == nullptr) {
		return;
	}

	if (Percent < 0 && Actor == Victim) {
		return;
	}

	if (Percent < 0 && WorldType == NON_PVP && Actor->IsPeaceful() && Victim->IsPeaceful()) {
		return;
	}

	if (Percent < 0 && GetRaceNoParalyze(Victim->Race)) {
		return;
	}

	if (Percent < 0 && Victim->Type == PLAYER && CheckRight(Victim->ID, INVULNERABLE)) {
		return;
	}

	TSkill *sk = Victim->Skills[SKILL_GOSTRENGTH];
	int Modification;
	if (Percent < -100) {
		Modification = -20 - sk->GetAct();
	} else {
		Modification = Percent * sk->GetAct() / 100;
	}
	sk->SetMDAct(Modification);
	Victim->SetTimer(SKILL_GOSTRENGTH, Duration, 1, 1, -1);
}

TDrunkenImpact::TDrunkenImpact(TCreature *_Actor, int _Power, int _Duration) {
	if (_Actor == nullptr) {
		error("TDrunkenImpact::TDrunkenImpact: Actor is NULL.\n");
	}

	if (_Power > 6) {
		error("TDrunkenImpact::TDrunkenImpact: Power is too large (%d).\n", _Power);
		_Power = 6;
	}

	Actor = _Actor;
	Power = _Power;
	Duration = _Duration;
}

void TDrunkenImpact::handleCreature(TCreature *Victim) {
	if (Victim == nullptr) {
		error("TDrunkenImpact::handleCreature: Victim does not exist.\n");
		return;
	}

	if (Actor == nullptr || Actor == Victim) {
		return;
	}

	if (WorldType == NON_PVP && Actor->IsPeaceful() && Victim->IsPeaceful()) {
		return;
	}

	if (GetRaceNoParalyze(Victim->Race)) {
		return;
	}

	if (Victim->Type == PLAYER && CheckRight(Victim->ID, INVULNERABLE)) {
		return;
	}

	if (Victim->Skills[SKILL_DRUNKEN]->TimerValue() <= Power) {
		Victim->SetTimer(SKILL_DRUNKEN, Power, Duration, Duration, -1);
	}
}

TStrengthImpact::TStrengthImpact(TCreature *_Actor, int _Skills, int _Percent, int _Duration) {
	if (_Actor == nullptr) {
		error("TStrengthImpact::TStrengthImpact: Actor is NULL.\n");
	}

	Actor = _Actor;
	Skills = _Skills;
	Percent = _Percent;
	Duration = _Duration;
}

void TStrengthImpact::handleCreature(TCreature *Victim) {
	if (Victim == nullptr) {
		error("TStrengthImpact::handleCreature: Victim does not exist.\n");
		return;
	}

	if (Actor == nullptr) {
		return;
	}

	if (Percent < 0 && Actor == Victim) {
		return;
	}

	if (Percent < 0 && WorldType == NON_PVP && Actor->IsPeaceful() && Victim->IsPeaceful()) {
		return;
	}

	if (Percent < 0 && GetRaceNoParalyze(Victim->Race)) {
		return;
	}

	if (Percent < 0 && Victim->Type == PLAYER && CheckRight(Victim->ID, INVULNERABLE)) {
		return;
	}

	for (int Skill = SKILL_SHIELDING; Skill <= SKILL_FISTFIGHTING; Skill++) {
		if ((Skills & 1) == 0 && in(Skill, SKILL_SWORDFIGHTING, SKILL_FISTFIGHTING)) {
			continue;
		}
		if ((Skills & 2) == 0 && Skill == SKILL_DISTANCEFIGHTING) {
			continue;
		}
		if ((Skills & 4) == 0 && Skill == SKILL_SHIELDING) {
			continue;
		}

		TSkill *sk = Victim->Skills[Skill];
		int Modification;
		if (Percent < -100) {
			Modification = -20 - sk->GetAct();
		} else {
			Modification = Percent * sk->GetAct() / 100;
		}
		sk->SetMDAct(Modification);
		Victim->SetTimer((unsigned short)Skill, Duration, 1, 1, -1);
	}
}

TOutfitImpact::TOutfitImpact(TCreature *_Actor, TOutfit _Outfit, int _Duration) {
	if (_Actor == nullptr) {
		error("TOutfitImpact::TOutfitImpact: Actor is NULL.\n");
	}

	Outfit = _Outfit;
	Duration = _Duration;
}

void TOutfitImpact::handleCreature(TCreature *Victim) {
	if (Victim == nullptr) {
		error("TOutfitImpact::handleCreature: Victim does not exist.\n");
		return;
	}

	Victim->Outfit = Outfit;
	Victim->SetTimer(SKILL_ILLUSION, 1, Duration, Duration, -1);
}

TSummonImpact::TSummonImpact(TCreature *_Actor, int _Race, int _Maximum) {
	if (_Actor == nullptr) {
		error("TSummonImpact::TSummonImpact: Actor is NULL.\n");
	}

	if (!in(_Race, 1, MAX_RACES - 1)) {
		error("TSummonImpact::TSummonImpact: Invalid race number %d.\n", _Race);
	}

	Actor = _Actor;
	Race = _Race;
	Maximum = _Maximum;
}

void TSummonImpact::handleField(int x, int y, int z) {
	if (Actor == nullptr) {
		return;
	}

	if (in(Race, 1, MAX_RACES - 1) && Actor->SummonedCreatures < Maximum
			&& SearchSummonField(x, y, z, 2)) {
		CreateMonster(Race, x, y, z, 0, Actor->ID, true);
	}
}

void ActorShapeSpell(TCreature *Actor, TImpact *Impact, int Effect) {
	if (Actor == nullptr) {
		error("ActorShapeSpell: Caster does not exist.\n");
		return;
	}

	if (Impact->isAggressive() && IsProtectionZone(Actor->posx, Actor->posy, Actor->posz)) {
		return;
	}

	Impact->handleCreature(Actor);
	if (Effect != 0) {
		GraphicalEffect(Actor->posx, Actor->posy, Actor->posz, Effect);
	}
}

void VictimShapeSpell(TCreature *Actor, TCreature *Victim, int Range, int Animation, TImpact *Impact, int Effect) {
	if (Actor == nullptr) {
		error("VictimShapeSpell: Caster does not exist.\n");
		return;
	}

	if (Victim == nullptr || Actor->posz != Victim->posz) {
		return;
	}

	if (std::max(std::abs(Actor->posx - Victim->posx), std::abs(Actor->posy - Victim->posy)) > Range) {
		return;
	}

	if (Impact->isAggressive() && IsProtectionZone(Victim->posx, Victim->posy, Victim->posz)) {
		return;
	}

	if (!ThrowPossible(Actor->posx, Actor->posy, Actor->posz, Victim->posx, Victim->posy, Victim->posz, 0)) {
		return;
	}

	if (Animation != 0 && (Actor->posx != Victim->posx || Actor->posy != Victim->posy)) {
		Missile(Actor->GetCreatureObject(), Victim->GetCreatureObject(), Animation);
	}

	Impact->handleCreature(Victim);
	if (Effect != 0) {
		GraphicalEffect(Victim->posx, Victim->posy, Victim->posz, Effect);
	}
}

void OriginShapeSpell(TCreature *Actor, int Radius, TImpact *Impact, int Effect) {
	if (Actor == nullptr) {
		error("OriginShapeSpell: Given creature does not exist.\n");
		return;
	}

	for (int i = 0; i <= Radius; i++) {
		for (int j = 0; j < Circle[i].Count; j++) {
			int x = Actor->posx + Circle[i].x[j];
			int y = Actor->posy + Circle[i].y[j];
			int z = Actor->posz;
			if (!ThrowPossible(Actor->posx, Actor->posy, z, x, y, z, 0)) {
				continue;
			}

			if (Impact->isAggressive() && IsProtectionZone(x, y, z)) {
				continue;
			}

			Impact->handleField(x, y, z);
			Object Obj = GetFirstObject(x, y, z);
			while (Obj != NONE) {
				Object Help = Obj;
				Obj++;
				if (Help.isCreature()) {
					TCreature *dest = GetCreature(Help);
					if (dest != nullptr) {
						Impact->handleCreature(dest);
					}
				}
			}

			if (Effect != 0) {
				GraphicalEffect(x, y, z, Effect);
			}
		}
	}
}

void DestinationShapeSpell(TCreature *Actor, TCreature *Victim, int Range, int Animation, int Radius, TImpact *Impact, int Effect) {
	if (Actor == nullptr) {
		error("DestinationShapeSpell: Caster does not exist.\n");
		return;
	}

	if (Victim == nullptr) {
		return;
	}

	CircleShapeSpell(Actor, Victim->posx, Victim->posy, Victim->posz, Range, Animation, Radius, Impact, Effect);
}

void CircleShapeSpell(TCreature *Actor, int DestX, int DestY, int DestZ, int Range, int Animation, int Radius, TImpact *Impact, int Effect) {
	if (Actor == nullptr) {
		error("CircleShapeSpell: Caster does not exist.\n");
		return;
	}

	if (Actor->posz != DestZ) {
		return;
	}

	if (std::max(std::abs(Actor->posx - DestX), std::abs(Actor->posy - DestY)) > Range) {
		return;
	}

	if (!ThrowPossible(Actor->posx, Actor->posy, Actor->posz, DestX, DestY, DestZ, 0)) {
		return;
	}

	if (Animation != 0 && (Actor->posx != DestX || Actor->posy != DestY)) {
		Missile(Actor->GetCreatureObject(), GetMapContainer(DestX, DestY, DestZ), Animation);
	}

	for (int i = 0; i <= Radius; i++) {
		for (int j = 0; j < Circle[i].Count; j++) {
			int x = DestX + Circle[i].x[j];
			int y = DestY + Circle[i].y[j];
			int z = DestZ;
			if (!ThrowPossible(DestX, DestY, DestZ, x, y, z, 0)) {
				continue;
			}

			if (Impact->isAggressive() && IsProtectionZone(x, y, z)) {
				continue;
			}

			Impact->handleField(x, y, z);
			Object Obj = GetFirstObject(x, y, z);
			while (Obj != NONE) {
				Object Help = Obj;
				Obj++;
				if (Help.isCreature()) {
					TCreature *dest = GetCreature(Help);
					if (dest != nullptr) {
						Impact->handleCreature(dest);
					}
				}
			}

			if (Effect != 0) {
				GraphicalEffect(x, y, z, Effect);
			}
		}
	}
}

void AngleShapeSpell(TCreature *Actor, int Angle, int Range, TImpact *Impact, int Effect) {
	if (Actor == nullptr) {
		error("AngleShapeSpell: Given creature does not exist.\n");
		return;
	}

	for (int i = 1; i <= Range; i++) {
		for (int j = -i * Angle / 90; j <= i * Angle / 90; j++) {
			int x;
			int y;
			int z = Actor->posz;
			switch (Actor->Direction) {
				case DIRECTION_NORTH: {
					x = Actor->posx + j;
					y = Actor->posy - i;
					break;
				}

				case DIRECTION_EAST: {
					x = Actor->posx + i;
					y = Actor->posy + j;
					break;
				}

				case DIRECTION_SOUTH: {
					x = Actor->posx + j;
					y = Actor->posy + i;
					break;
				}

				case DIRECTION_WEST: {
					x = Actor->posx - i;
					y = Actor->posy + j;
					break;
				}

				default: {
					error("AngleShapeSpell: Invalid facing direction %d.\n", Actor->Direction);
					return;
				}
			}

			if (!ThrowPossible(Actor->posx, Actor->posy, Actor->posz, x, y, z, 0)) {
				continue;
			}

			if (Impact->isAggressive() && IsProtectionZone(x, y, z)) {
				continue;
			}

			Impact->handleField(x, y, z);
			Object Obj = GetFirstObject(x, y, z);
			while (Obj != NONE) {
				Object Help = Obj;
				Obj++;
				if (Help.isCreature()) {
					TCreature *dest = GetCreature(Help);
					if (dest != nullptr) {
						Impact->handleCreature(dest);
					}
				}
			}

			if (Effect != 0) {
				GraphicalEffect(x, y, z, Effect);
			}
		}
	}
}

void MassCombat(TCreature *cr, Object Target, int Mana, int SoulPoints, int Damage, int EffectNr, int Radius, unsigned short DmgType, int Animation) {
	if (!Target.exists()) {
		error("MassCombat: Given target does not exist.\n");
		throw ERROR;
	}

	if (cr == nullptr) {
		error("MassCombat: Given creature does not exist.\n");
		throw ERROR;
	}

	int x, y, z;
	GetObjectCoordinates(Target, x, y, z);
	CheckAffectedPlayers(cr, x, y, z);
	if (!ThrowPossible(cr->posx, cr->posy, cr->posz, x, y, z, 0)) {
		throw CANNOTTHROW;
	}

	SpendManaAndSoulPoints(cr, Mana, SoulPoints, (WorldType == PVP_ENFORCED ? 1000 : 2000));
	TDamageImpact Impact(cr, DmgType, Damage, false);
	CircleShapeSpell(cr, x, y, z, INT_MAX, Animation, Radius, &Impact, EffectNr);
}

void AngleCombat(TCreature *cr, int Mana, int SoulPoints, int Damage, int EffectNr, int Radius, int Angle, unsigned short DmgType) {
	if (cr == nullptr) {
		error("AngleCombat: Given creature does not exist.\n");
		throw ERROR;
	}

	SpendManaAndSoulPoints(cr, Mana, SoulPoints, (((Radius == 1 && Angle == 0) || WorldType == PVP_ENFORCED) ? 1000 : 2000));
	TDamageImpact Impact(cr, DmgType, Damage, false);
	AngleShapeSpell(cr, Angle, Radius, &Impact, EffectNr);
}

void Combat(TCreature *cr, Object Target, int Mana, int SoulPoints, int Damage, int EffectNr, int AnimationNr, unsigned short DmgType) {
	if (!Target.exists()) {
		error("Combat: Given target does not exist.\n");
		throw ERROR;
	}

	if (cr == nullptr) {
		error("Combat: Given creature does not exist.\n");
		throw ERROR;
	}

	int x, y, z;
	GetObjectCoordinates(Target, x, y, z);
	if (!Target.isCreature()) {
		Target = GetFirstSpecObject(x, y, z, ObjectType(TYPEID_CREATURE));
	}

	if (Target == NONE) {
		throw NOCREATURE;
	}

	CheckAffectedPlayers(cr, x, y, z);
	if (!ThrowPossible(cr->posx, cr->posy, cr->posz, x, y, z, 0)) {
		throw CANNOTTHROW;
	}

	SpendManaAndSoulPoints(cr, Mana, SoulPoints, (WorldType == PVP_ENFORCED ? 1000 : 2000));
	TDamageImpact Impact(cr, DmgType, Damage, false);
	CircleShapeSpell(cr, x, y, z, INT_MAX, AnimationNr, 0, &Impact, EffectNr);
}

void KillAllMonsters(TCreature *cr, int EffectNr, int Radius) {
	if (cr == nullptr) {
		error("KillAllMonsters: Given creature does not exist.\n");
		throw ERROR;
	}

	if (cr->Type == PLAYER && !CheckRight(cr->ID, CREATE_MONSTERS)) {
		return;
	}

	for (int i = 0; i <= Radius; i++) {
		for (int j = 0; j < Circle[i].Count; j++) {
			int dx = cr->posx + Circle[i].x[j];
			int dy = cr->posy + Circle[i].y[j];
			if (!ThrowPossible(cr->posx, cr->posy, cr->posz, dx, dy, cr->posz, 0)
					|| IsProtectionZone(dx, dy, cr->posz)) {
				continue;
			}

			Object Obj = GetFirstObject(dx, dy, cr->posz);
			while (Obj != NONE) {
				Object Help = Obj;
				Obj++;
				if (Help.isCreature()) {
					TCreature *dest = GetCreature(Help);
					if (dest == nullptr) {
						error("KillAllMonsters: Invalid creature.\n");
					} else if (dest != cr && dest->Type == MONSTER) {
						print(3, "Killing %s...\n", dest->Name);
						dest->Skills[SKILL_HITPOINTS]->Set(0);
						dest->Death();
					}
				}
			}

			GraphicalEffect(dx, dy, cr->posz, EffectNr);
		}
	}
}

void CreateField(TCreature *cr, Object Obj, int Mana, int SoulPoints, int Type) {
	if (cr == nullptr) {
		error("CreateField: Invalid creature given.\n");
		throw ERROR;
	}

	if (!Obj.exists()) {
		error("CreateField: Given object does not exist.\n");
		throw ERROR;
	}

	int x, y, z;
	GetObjectCoordinates(Obj, x, y, z);
	if (!FieldPossible(x, y, z, Type)) {
		throw NOROOM;
	}

	CheckAffectedPlayers(cr, x, y, z);
	if (!ThrowPossible(cr->posx, cr->posy, cr->posz, x, y, z, 0)) {
		throw CANNOTTHROW;
	}

	SpendManaAndSoulPoints(cr, Mana, SoulPoints, (WorldType == PVP_ENFORCED ? 1000 : 2000));
	Missile(cr->GetCreatureObject(), Obj, (Type == FIELD_FIRE ? ANIMATION_FIRE : ANIMATION_ENERGY));
	CreateField(x, y, z, Type, cr->ID, (WorldType == NON_PVP && cr->IsPeaceful()));
	if (in(Type, FIELD_FIRE, FIELD_ENERGY)) {
		cr->BlockLogout(60, true);
	}
}

void CreateField(TCreature *cr, int Mana, int SoulPoints, int Type) {
	if (cr == nullptr) {
		error("CreateField: Invalid creature given.\n");
		throw ERROR;
	}

	int x = cr->posx;
	int y = cr->posy;
	int z = cr->posz;
	switch (cr->Direction) {
		case DIRECTION_NORTH: y -= 1; break;
		case DIRECTION_EAST: x += 1; break;
		case DIRECTION_SOUTH: y += 1; break;
		case DIRECTION_WEST: x -= 1; break;
	}

	if (cr->Type == PLAYER && !CheckRight(cr->ID, ATTACK_EVERYWHERE) && IsProtectionZone(x, y, z)) {
		throw PROTECTIONZONE;
	}

	CreateField(cr, GetMapContainer(x, y, z), Mana, SoulPoints, Type);
}

void MassCreateField(TCreature *cr, Object Obj, int Mana, int SoulPoints, int Type, int Radius) {
	if (cr == nullptr) {
		error("MassCreateField: Invalid creature given.\n");
		throw ERROR;
	}

	if (!Obj.exists()) {
		error("MassCreateField: Given object does not exist.\n");
		throw ERROR;
	}

	int x, y, z;
	GetObjectCoordinates(Obj, x, y, z);
	CheckAffectedPlayers(cr, x, y, z);
	if (!ThrowPossible(cr->posx, cr->posy, cr->posz, x, y, z, 0)) {
		throw CANNOTTHROW;
	}

	SpendManaAndSoulPoints(cr, Mana, SoulPoints, (WorldType == PVP_ENFORCED ? 1000 : 2000));
	if (cr->posx != x || cr->posy != y || cr->posz != z) {
		Missile(cr->GetCreatureObject(), Obj, (Type == FIELD_FIRE ? ANIMATION_FIRE : ANIMATION_ENERGY));
	}

	// Quirk: the outer bound is "<", not "<=" as in the other circle spells.
	for (int i = 0; i < Radius; i++) {
		for (int j = 0; j < Circle[i].Count; j++) {
			int dx = x + Circle[i].x[j];
			int dy = y + Circle[i].y[j];
			if (ThrowPossible(x, y, z, dx, dy, z, 0) && !IsProtectionZone(dx, dy, z)) {
				CreateField(dx, dy, z, Type, cr->ID, (WorldType == NON_PVP && cr->IsPeaceful()));
			}
		}
	}

	if (in(Type, FIELD_FIRE, FIELD_ENERGY)) {
		cr->BlockLogout(60, true);
	}
}

void CreateFieldWall(TCreature *cr, Object Obj, int Mana, int SoulPoints, int Type, int Extension) {
	if (cr == nullptr) {
		error("CreateFieldWall: Invalid creature given.\n");
		throw ERROR;
	}

	if (!Obj.exists()) {
		error("CreateFieldWall: Given object does not exist.\n");
		throw ERROR;
	}

	int x, y, z;
	GetObjectCoordinates(Obj, x, y, z);
	CheckAffectedPlayers(cr, x, y, z);
	if (!ThrowPossible(cr->posx, cr->posy, cr->posz, x, y, z, 0)) {
		throw CANNOTTHROW;
	}

	SpendManaAndSoulPoints(cr, Mana, SoulPoints, (WorldType == PVP_ENFORCED ? 1000 : 2000));
	if (cr->posx != x || cr->posy != y || cr->posz != z) {
		Missile(cr->GetCreatureObject(), Obj, (Type == FIELD_FIRE ? ANIMATION_FIRE : ANIMATION_ENERGY));
	}

	int StepX, StepY;
	int Dir = Direction(x - cr->posx, y - cr->posy);
	switch (Dir) {
		case COMPASS_NONE: {
			throw NOROOM;
		}

		case COMPASS_EAST:
		case COMPASS_WEST: {
			StepX = 0;
			StepY = 1;
			break;
		}

		case COMPASS_NORTHEAST:
		case COMPASS_SOUTHWEST: {
			StepX = 1;
			StepY = 1;
			break;
		}

		case COMPASS_NORTH:
		case COMPASS_SOUTH: {
			StepX = 1;
			StepY = 0;
			break;
		}

		case COMPASS_NORTHWEST:
		case COMPASS_SOUTHEAST: {
			StepX = -1;
			StepY = 1;
			break;
		}

		default: {
			error("CreateFieldWall: Invalid direction %d.\n", Dir);
			throw ERROR;
		}
	}

	bool Peaceful = false;
	if (WorldType == NON_PVP && cr->IsPeaceful()) {
		Peaceful = true;
	}

	CreateField(x, y, z, Type, cr->ID, Peaceful);
	for (int i = 1; i <= Extension; i++) {
		int dx = x + i * StepX;
		int dy = y + i * StepY;
		if (ThrowPossible(cr->posx, cr->posy, cr->posz, dx, dy, z, 0) && !IsProtectionZone(dx, dy, z)) {
			CreateField(dx, dy, z, Type, cr->ID, Peaceful);
		}

		dx = x - i * StepX;
		dy = y - i * StepY;
		if (ThrowPossible(cr->posx, cr->posy, cr->posz, dx, dy, z, 0) && !IsProtectionZone(dx, dy, z)) {
			CreateField(dx, dy, z, Type, cr->ID, Peaceful);
		}

		if ((Dir & 1) == 0) {
			dx = x + i * StepX;
			dy = y + (i - 1) * StepY;
			if (ThrowPossible(cr->posx, cr->posy, cr->posz, dx, dy, z, 0) && !IsProtectionZone(dx, dy, z)) {
				CreateField(dx, dy, z, Type, cr->ID, Peaceful);
			}

			dx = x - i * StepX;
			dy = y - (i - 1) * StepY;
			if (ThrowPossible(cr->posx, cr->posy, cr->posz, dx, dy, z, 0) && !IsProtectionZone(dx, dy, z)) {
				CreateField(dx, dy, z, Type, cr->ID, Peaceful);
			}
		}
	}

	if (in(Type, FIELD_FIRE, FIELD_ENERGY)) {
		cr->BlockLogout(60, true);
	}
}

void DeleteField(TCreature *cr, Object Obj, int Mana, int SoulPoints) {
	if (cr == nullptr) {
		error("DeleteField: Invalid creature given.\n");
		throw ERROR;
	}

	if (!Obj.exists()) {
		error("DeleteField: Given object does not exist.\n");
		throw ERROR;
	}

	int x, y, z;
	GetObjectCoordinates(Obj, x, y, z);
	if (!FieldPossible(x, y, z, 0)) {
		throw NOROOM;
	}

	SpendManaAndSoulPoints(cr, Mana, SoulPoints, 1000);
	Obj = GetFirstObject(x, y, z);
	while (Obj != NONE) {
		Object Help = Obj;
		Obj++;
		if (Help.getFlag(MAGICFIELD)) {
			Delete(Help, -1);
		}
	}

	GraphicalEffect(x, y, z, EFFECT_POFF);
}

void CleanupField(TCreature *cr, Object Obj, int Mana, int SoulPoints) {
	if (cr == nullptr) {
		error("CleanupField: Invalid creature given.\n");
		throw ERROR;
	}

	if (!Obj.exists()) {
		error("CleanupField: Given object does not exist.\n");
		throw ERROR;
	}

	int x, y, z;
	GetObjectCoordinates(Obj, x, y, z);
	if (z != cr->posz || std::abs(x - cr->posx) > 1 || std::abs(y - cr->posy) > 1) {
		throw OUTOFRANGE;
	}

	SpendManaAndSoulPoints(cr, Mana, SoulPoints, 1000);
	Obj = GetFirstObject(x, y, z);
	while (Obj != NONE) {
		Object Help = Obj;
		Obj++;
		if (!Help.getFlag(UNMOVE)
				&& !Help.isCreature()
				&& (!Help.getFlag(CORPSE) || Help.getAttribute(CORPSETYPE) != 0)) {
			Delete(Help, -1);
		}
	}

	GraphicalEffect(x, y, z, EFFECT_POFF);
	cr->BlockLogout(60, true);
}

void CleanupField(TCreature *cr) {
	if (cr == nullptr) {
		error("CleanupField: Invalid creature given.\n");
		throw ERROR;
	}

	if (cr->Type != PLAYER) {
		error("CleanupField: Spell can only be cast by players.\n");
		throw ERROR;
	}

	if (!CheckRight(cr->ID, CLEANUP_FIELDS)) {
		return;
	}

	switch (cr->Direction) {
		case DIRECTION_NORTH: {
			CleanupField(cr, GetMapContainer(cr->posx, cr->posy - 1, cr->posz), 0, 0);
			break;
		}

		case DIRECTION_EAST: {
			CleanupField(cr, GetMapContainer(cr->posx + 1, cr->posy, cr->posz), 0, 0);
			break;
		}

		case DIRECTION_SOUTH: {
			CleanupField(cr, GetMapContainer(cr->posx, cr->posy + 1, cr->posz), 0, 0);
			break;
		}

		case DIRECTION_WEST: {
			CleanupField(cr, GetMapContainer(cr->posx - 1, cr->posy, cr->posz), 0, 0);
			break;
		}
	}
}

void Teleport(TCreature *cr, char *Param) {
	if (cr == nullptr) {
		error("Teleport: Invalid creature given.\n");
		throw ERROR;
	}

	if (Param == nullptr) {
		error("Teleport: Param is NULL.\n");
		throw ERROR;
	}

	if (cr->Type != PLAYER) {
		error("Teleport: Spell can only be cast by players.\n");
		throw ERROR;
	}

	TPlayer *pl = static_cast<TPlayer *>(cr);
	int x, y, z;
	Object Obj = cr->GetCreatureObject();
	GetObjectCoordinates(Obj, x, y, z);
	if (stricmp(Param, "up", -1) == 0) {
		if (!CheckRight(cr->ID, TELEPORT_VERTICAL)) {
			return;
		}

		z -= 1;
		if (!SearchFreeField(x, y, z, 1, 0xFFFF, true)) {
			throw NOROOM;
		}

		Move(0, Obj, GetMapContainer(x, y, z), -1, false, NONE);
		GraphicalEffect(x, y, z, EFFECT_TELEPORT);
	} else if (stricmp(Param, "down", -1) == 0) {
		if (!CheckRight(cr->ID, TELEPORT_VERTICAL)) {
			return;
		}

		z += 1;
		if (!SearchFreeField(x, y, z, 1, 0xFFFF, true)) {
			throw NOROOM;
		}

		Move(0, Obj, GetMapContainer(x, y, z), -1, false, NONE);
		GraphicalEffect(x, y, z, EFFECT_TELEPORT);
	} else if (stricmp(Param, "fast", -1) == 0) {
		if (!CheckRight(cr->ID, MODIFY_GOSTRENGTH)) {
			return;
		}

		cr->Skills[SKILL_GOSTRENGTH]->SetMDAct(100);
		AnnounceChangedCreature(cr->ID, CREATURE_SPEED);
		GraphicalEffect(Obj, EFFECT_MAGIC_BLUE);
	} else if (stricmp(Param, "fastest", -1) == 0) {
		if (!CheckRight(cr->ID, MODIFY_GOSTRENGTH)) {
			return;
		}

		cr->Skills[SKILL_GOSTRENGTH]->SetMDAct(200);
		AnnounceChangedCreature(cr->ID, CREATURE_SPEED);
		GraphicalEffect(Obj, EFFECT_MAGIC_BLUE);
	} else if (stricmp(Param, "slow", -1) == 0 || stricmp(Param, "normal", -1) == 0) {
		if (!CheckRight(cr->ID, MODIFY_GOSTRENGTH)) {
			return;
		}

		cr->Skills[SKILL_GOSTRENGTH]->SetMDAct(0);
		AnnounceChangedCreature(cr->ID, CREATURE_SPEED);
		GraphicalEffect(Obj, EFFECT_MAGIC_BLUE);
	} else {
		if (sscanf(Param, "%d,%d,%d", &x, &y, &z) == 3 || sscanf(Param, "[%d,%d,%d]", &x, &y, &z) == 3) {
			if (!CheckRight(cr->ID, TELEPORT_TO_COORDINATE)) {
				return;
			}

			if (x < 1 || y < 1 || z < 1 || !IsOnMap(x, y, z)) {
				if (!IsOnMap(x + cr->posx, y + cr->posy, z + cr->posz)) {
					SendMessage(pl->GetConnection(), TALK_FAILURE_MESSAGE, "Invalid coordinates.");
					return;
				}

				x += cr->posx;
				y += cr->posy;
				z += cr->posz;
			}

			if (!SearchFreeField(x, y, z, 1, 0xFFFF, true)) {
				throw NOROOM;
			}

			Move(0, Obj, GetMapContainer(x, y, z), -1, false, NONE);
			GraphicalEffect(x, y, z, EFFECT_TELEPORT);
		} else {
			if (!CheckRight(cr->ID, TELEPORT_TO_MARK)) {
				return;
			}

			if (!GetMarkPosition(Param, x, y, z)) {
				SendMessage(pl->GetConnection(), TALK_FAILURE_MESSAGE, "There is no mark of this name.");
				return;
			}

			if (!SearchFreeField(x, y, z, 1, GetHouseID(x, y, z), true)) {
				throw NOROOM;
			}

			Move(0, Obj, GetMapContainer(x, y, z), -1, false, NONE);
			GraphicalEffect(x, y, z, EFFECT_TELEPORT);
		}
	}
}

void TeleportToCreature(TCreature *cr, char *Name) {
	if (cr == nullptr) {
		error("TeleportToCreature: Invalid creature given.\n");
		throw ERROR;
	}

	if (cr->Type != PLAYER) {
		error("TeleportToCreature: Spell can only be cast by players.\n");
		throw ERROR;
	}

	TPlayer *pl = static_cast<TPlayer *>(cr);
	if (!CheckRight(cr->ID, TELEPORT_TO_CHARACTER)) {
		return;
	}

	if (Name == nullptr || Name[0] == 0) {
		SendMessage(pl->GetConnection(), TALK_FAILURE_MESSAGE, "You must enter a name.");
		return;
	}

	TPlayer *dest;
	int x, y, z;
	Object Obj;
	int Result = IdentifyPlayer(Name, false, !CheckRight(cr->ID, READ_GAMEMASTER_CHANNEL), dest);
	switch (Result) {
		case 0: {
			break;
		}

		case -1: {
			throw PLAYERNOTONLINE;
		}

		case -2: {
			throw NAMEAMBIGUOUS;
		}

		default: {
			error("TeleportToCreature: Invalid return value from IdentifyPlayer.\n");
			throw ERROR;
		}
	}

	Obj = cr->GetCreatureObject();
	if (dest == pl) {
		GraphicalEffect(Obj, EFFECT_TELEPORT);
		return;
	}

	GraphicalEffect(Obj, EFFECT_POFF);
	x = dest->posx;
	y = dest->posy;
	z = dest->posz;
	if (!SearchFreeField(x, y, z, 1, GetHouseID(x, y, z), true) && IsProtectionZone(x, y, z)) {
		throw NOROOM;
	}

	Move(0, Obj, GetMapContainer(x, y, z), -1, false, NONE);
	GraphicalEffect(x, y, z, EFFECT_TELEPORT);
	Log("banish", "%s teleports to %s.\n", cr->Name, dest->Name);
}

void TeleportPlayerToMe(TCreature *cr, char *Name) {
	if (cr == nullptr) {
		error("TeleportPlayerToMe: Invalid creature given.\n");
		throw ERROR;
	}

	if (cr->Type != PLAYER) {
		error("TeleportPlayerToMe: Spell can only be cast by players.\n");
		throw ERROR;
	}

	TPlayer *pl = static_cast<TPlayer *>(cr);
	if (!CheckRight(cr->ID, RETRIEVE)) {
		return;
	}

	if (Name == nullptr || Name[0] == 0) {
		SendMessage(pl->GetConnection(), TALK_FAILURE_MESSAGE, "You must enter a name.");
		return;
	}

	TPlayer *dest;
	int x, y, z;
	Object Obj;
	int Result = IdentifyPlayer(Name, false, !CheckRight(cr->ID, READ_GAMEMASTER_CHANNEL), dest);
	switch (Result) {
		case 0: {
			break;
		}

		case -1: {
			throw PLAYERNOTONLINE;
		}

		case -2: {
			throw NAMEAMBIGUOUS;
		}

		default: {
			error("TeleportPlayerToMe: Invalid return value from IdentifyPlayer.\n");
			throw ERROR;
		}
	}

	Obj = dest->GetCreatureObject();
	GraphicalEffect(Obj, EFFECT_POFF);
	x = cr->posx;
	y = cr->posy;
	z = cr->posz;
	if (!SearchFreeField(x, y, z, 1, GetHouseID(x, y, z), false)) {
		throw NOROOM;
	}

	Move(0, Obj, GetMapContainer(x, y, z), -1, false, NONE);
	GraphicalEffect(x, y, z, EFFECT_TELEPORT);
}

void MagicRope(TCreature *cr, int Mana, int SoulPoints) {
	if (cr == nullptr) {
		error("MagicRope: Invalid creature given.\n");
		throw ERROR;
	}

	int x, y, z;
	Object Obj = cr->GetCreatureObject();
	GetObjectCoordinates(Obj, x, y, z);
	if (!CoordinateFlag(x, y, z, ROPESPOT)) {
		throw NOTACCESSIBLE;
	}

	SpendManaAndSoulPoints(cr, Mana, SoulPoints, 1000);
	Move(0, Obj, GetMapContainer(x, y + 1, z - 1), -1, false, NONE);
	GraphicalEffect(x, y, z, EFFECT_TELEPORT);
}

void MagicClimbing(TCreature *cr, int Mana, int SoulPoints, char *Direction) {
	if (cr == nullptr) {
		error("MagicClimbing: Invalid creature given.\n");
		throw ERROR;
	}

	if (Direction == nullptr) {
		error("MagicClimbing: Invalid direction given.\n");
		throw ERROR;
	}

	int x, y, z;
	Object Obj = cr->GetCreatureObject();
	GetObjectCoordinates(Obj, x, y, z);

	int dx = x;
	int dy = y;
	int dz = z;
	switch (cr->Direction) {
		case DIRECTION_NORTH: dy -= 1; break;
		case DIRECTION_EAST: dx += 1; break;
		case DIRECTION_SOUTH: dy += 1; break;
		case DIRECTION_WEST: dx -= 1; break;
	}

	if (z > 0 && stricmp(Direction, "up", -1) == 0) {
		if (CoordinateFlag(x, y, z - 1, BANK)
				|| CoordinateFlag(x, y, z - 1, UNPASS)
				|| !cr->MovePossible(dx, dy, z - 1, true, true)) {
			throw NOTACCESSIBLE;
		}
		dz -= 1;
	}

	if (z < 15 && stricmp(Direction, "down", -1) == 0) {
		if (CoordinateFlag(dx, dy, z, BANK)
				|| CoordinateFlag(dx, dy, z, UNPASS)
				|| !cr->MovePossible(dx, dy, z + 1, true, true)) {
			throw NOTACCESSIBLE;
		}
		dz += 1;
	}

	if (z == dz) {
		throw NOTACCESSIBLE;
	}

	SpendManaAndSoulPoints(cr, Mana, SoulPoints, 1000);
	Move(0, Obj, GetMapContainer(dx, dy, dz), -1, false, NONE);
	GraphicalEffect(x, y, z, EFFECT_TELEPORT);
}

void MagicClimbing(TCreature *cr, char *Direction) {
	if (cr == nullptr) {
		error("MagicClimbing: Invalid creature given.\n");
		throw ERROR;
	}

	if (Direction == nullptr) {
		error("MagicClimbing: Invalid direction given.\n");
		throw ERROR;
	}

	if (cr->Type != PLAYER) {
		error("MagicClimbing: Spell can only be cast by players.\n");
		throw ERROR;
	}

	if (CheckRight(cr->ID, LEVITATE)) {
		MagicClimbing(cr, 0, 0, Direction);
	}
}

void CreateThing(TCreature *cr, char *Param1, char *Param2) {
	if (cr == nullptr) {
		error("CreateThing: Invalid creature given.\n");
		throw ERROR;
	}

	if (Param1 == nullptr) {
		error("CreateThing: Invalid parameter \"Param1\".\n");
		throw ERROR;
	}

	if (cr->Type != PLAYER) {
		error("CreateThing: Spell can only be cast by players.\n");
		throw ERROR;
	}

	TPlayer *pl = static_cast<TPlayer *>(cr);
	if (!CheckRight(cr->ID, CREATE_OBJECTS)) {
		return;
	}

	ObjectType Type;
	int Count = 1;
	if (atoi(Param1) == 0) {
		Type = GetObjectTypeByName(Param1, true);
	} else {
		int TypeID = atoi(Param1);
		if (TypeID > ObjectType(TYPEID_CREATURE).getTypeID() && ObjectTypeExists(TypeID)) {
			Type = TypeID;
		}

		if (Type.getFlag(UNMOVE)) {
			Type = 0;
		}
	}

	if (Type == 0) {
		SendMessage(pl->GetConnection(), TALK_FAILURE_MESSAGE, "There is no such takeable object.");
		return;
	}

	if (Param2 != nullptr) {
		Count = atoi(Param2);
	}

	if (Count < 1 || Count > 100) {
		SendMessage(pl->GetConnection(), TALK_FAILURE_MESSAGE, "You may only create 1 to 100 objects.");
		return;
	}

	if (!Type.getFlag(TAKE) && Count != 1) {
		SendMessage(pl->GetConnection(), TALK_FAILURE_MESSAGE, "You may only create one untakeable object.");
		return;
	}

	Object Con = GetMapContainer(cr->GetCreatureObject());
	if (Type.getFlag(CUMULATIVE)) {
		CreateAtCreature(cr->ID, Type, Count);
	} else {
		for (int i = 0; i < Count; i++) {
			CreateAtCreature(cr->ID, Type, 0);
		}
	}

	GraphicalEffect(Con, EFFECT_MAGIC_GREEN);
}

void CreateMoney(TCreature *cr, char *Param) {
	if (cr == nullptr) {
		error("CreateMoney: Invalid creature given.\n");
		throw ERROR;
	}

	if (Param == nullptr) {
		error("CreateMoney: Invalid parameter \"Param1\".\n");
		throw ERROR;
	}

	if (cr->Type != PLAYER) {
		error("CreateMoney: Spell can only be cast by players.\n");
		throw ERROR;
	}

	TPlayer *pl = static_cast<TPlayer *>(cr);
	if (!CheckRight(cr->ID, CREATE_MONEY)) {
		return;
	}

	int Count = atoi(Param);
	if (Count < 1 || Count > 1000000) {
		SendMessage(pl->GetConnection(), TALK_FAILURE_MESSAGE, "You may only create 1 to 1,000,000 gold.");
		return;
	}

	Object Con = GetMapContainer(cr->GetCreatureObject());
	if (Count / 10000 > 0) {
		CreateAtCreature(cr->ID, GetSpecialObject(MONEY_TENTHOUSAND), Count / 10000);
	}

	if (Count % 10000 / 100 > 0) {
		CreateAtCreature(cr->ID, GetSpecialObject(MONEY_HUNDRED), Count % 10000 / 100);
	}

	if (Count % 100 > 0) {
		CreateAtCreature(cr->ID, GetSpecialObject(MONEY_ONE), Count % 100);
	}

	GraphicalEffect(Con, EFFECT_MAGIC_GREEN);
}

void CreateFood(TCreature *cr, int Mana, int SoulPoints) {
	if (cr == nullptr) {
		error("CreateFood: Invalid creature given.\n");
		throw ERROR;
	}

	SpendManaAndSoulPoints(cr, Mana, SoulPoints, 1000);
	int max = rand() % 2 + 1;
	for (int i = 0; i < max; i++) {
		ObjectType Type;
		switch (rand() % 7) {
			case 0: Type = GetNewObjectType(134, 0); break;
			case 1: Type = GetNewObjectType(130, 2); break;
			case 2: Type = GetNewObjectType(136, 0); break;
			case 3: Type = GetNewObjectType(130, 13); break;
			case 4: Type = GetNewObjectType(131, 1); break;
			case 5: Type = GetNewObjectType(131, 9); break;
			case 6: Type = GetNewObjectType(134, 1); break;
		}

		CreateAtCreature(cr->ID, Type, 1);
	}

	GraphicalEffect(cr->GetCreatureObject(), EFFECT_MAGIC_GREEN);
}

void CreateArrows(TCreature *cr, int Mana, int SoulPoints, int Ammo, int Amount) {
	if (cr == nullptr) {
		error("CreateArrows: Invalid creature given.\n");
		throw ERROR;
	}

	SpendManaAndSoulPoints(cr, Mana, SoulPoints, 1000);
	ObjectType Type;
	switch (Ammo) {
		case CONJURE_ARROW: Type = GetNewObjectType(94, 1); break;
		case CONJURE_POISONED_ARROW: Type = GetNewObjectType(94, 2); break;
		case CONJURE_EXPLOSIVE_ARROW: Type = GetNewObjectType(94, 3); break;
		case CONJURE_BOLT: Type = GetNewObjectType(94, 0); break;
		case CONJURE_POWER_BOLT: Type = GetNewObjectType(94, 4); break;
		default: {
			error("CreateArrows: Invalid arrow type %d.\n", Ammo);
			throw ERROR;
		}
	}

	CreateAtCreature(cr->ID, Type, Amount);
	GraphicalEffect(cr->GetCreatureObject(), EFFECT_MAGIC_BLUE);
}

void SummonCreature(TCreature *cr, int Mana, int Race, bool God) {
	if (cr == nullptr) {
		error("SummonCreature: Invalid creature given.\n");
		throw ERROR;
	}

	if (Race < 1 || Race > MAX_RACES - 1) {
		error("SummonCreature: Invalid race number %d given.\n", Race);
		throw ERROR;
	}

	if (God) {
		if (!CheckRight(cr->ID, CREATE_MONSTERS)) {
			return;
		}
	} else {
		if (cr->Type == PLAYER && !CheckRight(cr->ID, CREATE_MONSTERS) && GetRaceNoSummon(Race)) {
			throw NOTACCESSIBLE;
		}

		if (cr->Type == PLAYER && cr->SummonedCreatures >= MAX_SUMMONS) {
			throw TOOMANYSLAVES;
		}
	}

	int posx = cr->posx;
	int posy = cr->posy;
	int posz = cr->posz;
	if (!SearchSummonField(posx, posy, posz, 2)) {
		throw NOROOM;
	}

	if (!God) {
		SpendManaAndSoulPoints(cr, GetRaceSummonCost(Race) + Mana, 0, (WorldType == PVP_ENFORCED ? 1000 : 2000));
	}

	CreateMonster(Race, posx, posy, posz, 0, (God ? 0 : cr->ID), true);
	GraphicalEffect(cr->GetCreatureObject(), EFFECT_MAGIC_BLUE);
}

void SummonCreature(TCreature *cr, int Mana, char *RaceName, bool God) {
	if (cr == nullptr) {
		error("SummonCreature: Invalid creature given.\n");
		throw ERROR;
	}

	if (RaceName == nullptr) {
		error("SummonCreature: Invalid race name given.\n");
		throw ERROR;
	}

	int Race = 0;
	for (int i = 1; i < MAX_RACES; i++) {
		if (stricmp(RaceName, GetRaceName(i), -1) == 0) {
			Race = i;
			break;
		}
	}

	if (Race == 0) {
		throw CREATURENOTEXISTING;
	}

	SummonCreature(cr, Mana, Race, God);
}

void StartMonsterraid(TCreature *cr, char *RaidName) {
	if (cr == nullptr) {
		error("StartMonsterraid: Invalid creature given.\n");
		throw ERROR;
	}

	if (RaidName == nullptr) {
		error("StartMonsterraid: Invalid raid name given.\n");
		throw ERROR;
	}

	if (!CheckRight(cr->ID, CREATE_MONSTERS)) {
		return;
	}

	strLower(RaidName);
	char FileName[4096];
	snprintf(FileName, sizeof(FileName), "%s/%s.evt", MONSTERPATH, RaidName);
	if (!FileExists(FileName)) {
		GraphicalEffect(cr->GetCreatureObject(), EFFECT_POFF);
		return;
	}

	try {
		LoadMonsterRaid(FileName, (int)RoundNr,nullptr, nullptr, nullptr, nullptr);
		GraphicalEffect(cr->GetCreatureObject(), EFFECT_MAGIC_BLUE);
	} catch (const char *) {
		GraphicalEffect(cr->GetCreatureObject(), EFFECT_POFF);
	}
}

void RaiseDead(TCreature *cr, Object Obj, int Mana, int SoulPoints) {
	if (cr == nullptr) {
		error("RaiseDead: Invalid creature given.\n");
		throw ERROR;
	}

	if (!Obj.exists()) {
		error("RaiseDead: Given object does not exist.\n");
		throw ERROR;
	}

	int x, y, z;
	GetObjectCoordinates(Obj, x, y, z);
	if (z != cr->posz || std::abs(x - cr->posx) > 1 || std::abs(y - cr->posy) > 1) {
		throw OUTOFRANGE;
	}

	Obj = GetFirstObject(x, y, z);
	while (Obj != NONE) {
		if (Obj.getFlag(CORPSE) && Obj.getAttribute(CORPSETYPE) == 1) {
			break;
		}
		Obj++;
	}

	if (Obj == NONE) {
		throw NOTUSABLE;
	}

	if (!SearchFreeField(x, y, z, 1, 0, false) || IsProtectionZone(x, y, z)) {
		throw NOROOM;
	}

	SpendManaAndSoulPoints(cr, Mana, SoulPoints, (WorldType == PVP_ENFORCED ? 1000 : 2000));
	Delete(Obj, -1);
	CreateMonster(RACE_SKELETON, x, y, z, 0, cr->ID, true);
}

void MassRaiseDead(TCreature *cr, Object Target, int Mana, int SoulPoints, int Radius) {
	if (cr == nullptr) {
		error("MassRaiseDead: Invalid creature given.\n");
		throw ERROR;
	}

	if (!Target.exists()) {
		error("MassRaiseDead: Given object does not exist.\n");
		throw ERROR;
	}

	int x, y, z;
	GetObjectCoordinates(Target, x, y, z);
	if (!ThrowPossible(cr->posx, cr->posy, cr->posz, x, y, z, 0)) {
		throw CANNOTTHROW;
	}

	SpendManaAndSoulPoints(cr, Mana, SoulPoints, (WorldType == PVP_ENFORCED ? 1000 : 2000));
	if (cr->posx != x || cr->posy != y || cr->posz != z) {
		Missile(cr->GetCreatureObject(), GetMapContainer(x, y, z), ANIMATION_ENERGY);
	}

	for (int i = 0; i <= Radius; i++) {
		for (int j = 0; j < Circle[i].Count; j++) {
			int dx = x + Circle[i].x[j];
			int dy = y + Circle[i].y[j];
			if (!ThrowPossible(x, y, z, dx, dy, z, 0)) {
				continue;
			}

			GraphicalEffect(dx, dy, z, EFFECT_MAGIC_BLUE);
			Object Obj = GetFirstObject(dx, dy, z);
			while (Obj != NONE) {
				Object Help = Obj;
				Obj++;
				if (Help.getFlag(CORPSE) && Help.getAttribute(CORPSETYPE) == 1) {
					int sx = dx;
					int sy = dy;
					int sz = z;
					if (SearchFreeField(sx, sy, sz, 1, 0, false) && !IsProtectionZone(sx, sy, sz)) {
						Delete(Help, -1);
						CreateMonster(RACE_SKELETON, sx, sy, sz, 0, cr->ID, false);
					}
				}
			}
		}
	}
}

void Heal(TCreature *cr, int Mana, int SoulPoints, int Healing) {
	if (cr == nullptr) {
		error("Heal: Invalid creature given.\n");
		throw ERROR;
	}

	if (WorldType == NON_PVP && !cr->IsPeaceful()) {
		throw NOTACCESSIBLE;
	}

	SpendManaAndSoulPoints(cr, Mana, SoulPoints, 1000);
	TSkill *sk = cr->Skills[SKILL_HITPOINTS];
	if (sk == nullptr) {
		error("Heal: Skill HITPOINTS does not exist.\n");
		throw ERROR;
	}

	if (sk->Get() > 0) {
		sk->Change(Healing);
		if (cr->Skills[SKILL_GOSTRENGTH]->GetMDAct() < 0) {
			cr->SetTimer(SKILL_GOSTRENGTH, 0, 0, 0, -1);
		}
		GraphicalEffect(cr->GetCreatureObject(), EFFECT_MAGIC_BLUE);
	}
}

void MassHeal(TCreature *cr, Object Target, int Mana, int SoulPoints, int Healing, int Radius) {
	if (cr == nullptr) {
		error("MassHeal: Invalid creature given.\n");
		throw ERROR;
	}

	if (!Target.exists()) {
		error("MassHeal: Given target does not exist.\n");
		throw ERROR;
	}

	int x, y, z;
	GetObjectCoordinates(Target, x, y, z);
	if (!ThrowPossible(cr->posx, cr->posy, cr->posz, x, y, z, 0)) {
		throw CANNOTTHROW;
	}

	SpendManaAndSoulPoints(cr, Mana, SoulPoints, 1000);
	if (cr->posx != x || cr->posy != y || cr->posz != z) {
		Missile(cr->GetCreatureObject(), GetMapContainer(x, y, z), ANIMATION_ENERGY);
	}

	for (int i = 0; i <= Radius; i++) {
		for (int j = 0; j < Circle[i].Count; j++) {
			int dx = x + Circle[i].x[j];
			int dy = y + Circle[i].y[j];
			if (!ThrowPossible(x, y, z, dx, dy, z, 0)) {
				continue;
			}

			GraphicalEffect(dx, dy, z, EFFECT_MAGIC_BLUE);
			Object Obj = GetFirstObject(dx, dy, z);
			while (Obj != NONE) {
				Object Help = Obj;
				Obj++;
				if (!Help.isCreature()) {
					continue;
				}

				TCreature *dest = GetCreature(Help);
				if (dest == nullptr) {
					error("MassHeal: Invalid creature.\n");
					continue;
				}

				if (WorldType == NON_PVP && !dest->IsPeaceful()) {
					continue;
				}

				TSkill *sk = dest->Skills[SKILL_HITPOINTS];
				if (sk == nullptr) {
					error("MassHeal: Skill HITPOINTS does not exist.\n");
					throw ERROR;
				}

				if (sk->Get() > 0) {
					sk->Change(Healing);
					if (dest->Skills[SKILL_GOSTRENGTH]->GetMDAct() < 0) {
						dest->SetTimer(SKILL_GOSTRENGTH, 0, 0, 0, -1);
					}
				}
			}
		}
	}
}

void HealFriend(TCreature *cr, char *Name, int Mana, int SoulPoints, int Healing) {
	if (cr == nullptr) {
		error("HealFriend: Invalid creature given.\n");
		throw ERROR;
	}

	if (Name == nullptr) {
		error("HealFriend: Invalid name given.\n");
		throw ERROR;
	}

	TPlayer *pl;
	int Result = IdentifyPlayer(Name, false, true, pl);
	if (Result == -1) {
		throw PLAYERNOTONLINE;
	}

	if (Result == -2) {
		throw NAMEAMBIGUOUS;
	}

	if (cr->posz != pl->posz || std::abs(cr->posx - pl->posx) > 7 || std::abs(cr->posy - pl->posy) > 5) {
		throw OUTOFRANGE;
	}

	TSkill *sk = pl->Skills[SKILL_HITPOINTS];
	if (sk == nullptr) {
		error("HealFriend: Skill HITPOINTS does not exist.\n");
		throw ERROR;
	}

	if (sk->Get() <= 0) {
		throw PLAYERNOTONLINE;
	}

	SpendManaAndSoulPoints(cr, Mana, SoulPoints, 1000);
	sk->Change(Healing);
	if (pl->Skills[SKILL_GOSTRENGTH]->GetMDAct() < 0) {
		pl->SetTimer(SKILL_GOSTRENGTH, 0, 0, 0, -1);
	}

	GraphicalEffect(cr->GetCreatureObject(), EFFECT_MAGIC_BLUE);
	GraphicalEffect(pl->GetCreatureObject(), EFFECT_MAGIC_GREEN);
}

void RefreshMana(TCreature *cr, int Mana, int SoulPoints, int AddMana) {
	if (cr == nullptr) {
		error("RefreshMana: Invalid creature given.\n");
		throw ERROR;
	}

	SpendManaAndSoulPoints(cr, Mana, SoulPoints, 1000);
	TSkill *sk = cr->Skills[SKILL_MANA];
	if (sk == nullptr) {
		error("RefreshMana: Skill MANA does not exist.\n");
		throw ERROR;
	}

	sk->Change(AddMana);
	GraphicalEffect(cr->GetCreatureObject(), EFFECT_MAGIC_BLUE);
}

void MagicGoStrength(TCreature *cr, TCreature *dest, int Mana, int SoulPoints, int percent, int duration) {
	if (dest == nullptr) {
		error("MagicGoStrength: Given target creature does not exist.\n");
		throw ERROR;
	}

	if (cr == nullptr) {
		error("MagicGoStrength: Given creature does not exist.\n");
		throw ERROR;
	}

	if (percent < 0) {
		if (GetRaceNoParalyze(dest->Race)) {
			throw NOTACCESSIBLE;
		}

		if (WorldType == NON_PVP && cr->IsPeaceful() && dest->IsPeaceful()) {
			throw ATTACKNOTALLOWED;
		}
	}

	SpendManaAndSoulPoints(cr, Mana, SoulPoints, 1000);
	TSkill *sk = dest->Skills[SKILL_GOSTRENGTH];
	int mdact;
	if (percent < -100) {
		mdact = -20 - sk->GetAct();
	} else {
		mdact = sk->GetAct() * percent / 100;
	}
	sk->SetMDAct(mdact);
	dest->SetTimer(SKILL_GOSTRENGTH, duration, 10, 10, -1);
	if (percent < 0) {
		cr->BlockLogout(60, dest->Type == PLAYER);
	}

	GraphicalEffect(cr->GetCreatureObject(), EFFECT_MAGIC_GREEN);
	GraphicalEffect(dest->GetCreatureObject(), EFFECT_MAGIC_RED);
}

void Shielding(TCreature *cr, int Mana, int SoulPoints, int Duration) {
	if (cr == nullptr) {
		error("Shielding: Invalid creature given.\n");
		throw ERROR;
	}

	SpendManaAndSoulPoints(cr, Mana, SoulPoints, 1000);
	cr->SetTimer(SKILL_MANASHIELD, 1, Duration, Duration, -1);
	GraphicalEffect(cr->GetCreatureObject(), EFFECT_MAGIC_BLUE);
}

void NegatePoison(TCreature *cr, int Mana, int SoulPoints, TCreature *dest) {
	if (cr == nullptr) {
		error("NegatePoison: Invalid creature given.\n");
		throw ERROR;
	}

	if (dest == nullptr) {
		error("NegatePoison: Target creature does not exist.\n");
		throw ERROR;
	}

	SpendManaAndSoulPoints(cr, Mana, SoulPoints, 1000);
	dest->SetTimer(SKILL_POISON, 0, 0, 0, -1);
	GraphicalEffect(dest->GetCreatureObject(), EFFECT_MAGIC_BLUE);
}

void Enlight(TCreature *cr, int Mana, int SoulPoints, int Radius, int Duration) {
	if (cr == nullptr) {
		error("Enlight: Invalid creature given.\n");
		throw ERROR;
	}

	SpendManaAndSoulPoints(cr, Mana, SoulPoints, 1000);
	if (cr->Skills[SKILL_SHINING]->TimerValue() <= Radius) {
		cr->SetTimer(SKILL_SHINING, Radius, Duration / Radius, Duration / Radius, -1);
	}

	GraphicalEffect(cr->GetCreatureObject(), EFFECT_MAGIC_BLUE);
}

void Invisibility(TCreature *cr, int Mana, int SoulPoints, int Duration) {
	if (cr == nullptr) {
		error("Invisibility: Invalid creature given.\n");
		throw ERROR;
	}

	SpendManaAndSoulPoints(cr, Mana, SoulPoints, 1000);
	cr->Outfit.OutfitID = 0;
	cr->Outfit.ObjectType = 0;
	cr->SetTimer(SKILL_ILLUSION, 1, Duration, Duration, -1);
	GraphicalEffect(cr->GetCreatureObject(), EFFECT_MAGIC_BLUE);
}

void CancelInvisibility(TCreature *cr, Object Target, int Mana, int SoulPoints, int Radius) {
	if (cr == nullptr) {
		error("CancelInvisibility: Invalid creature given.\n");
		throw ERROR;
	}

	if (!Target.exists()) {
		error("CancelInvisibility: Given target does not exist.\n");
		throw ERROR;
	}

	int x, y, z;
	GetObjectCoordinates(Target, x, y, z);
	if (!ThrowPossible(cr->posx, cr->posy, cr->posz, x, y, z, 0)) {
		throw CANNOTTHROW;
	}

	SpendManaAndSoulPoints(cr, Mana, SoulPoints, 1000);
	if (cr->posx != x || cr->posy != y || cr->posz != z) {
		Missile(cr->GetCreatureObject(), GetMapContainer(x, y, z), ANIMATION_ENERGY);
	}

	// The circles start at radius 1; the center field is not affected.
	for (int i = 1; i <= Radius; i++) {
		for (int j = 0; j < Circle[i].Count; j++) {
			int dx = x + Circle[i].x[j];
			int dy = y + Circle[i].y[j];
			if (!ThrowPossible(x, y, z, dx, dy, z, 0) || IsProtectionZone(dx, dy, z)) {
				continue;
			}

			bool EffectShown = false;
			Object Obj = GetFirstObject(dx, dy, z);
			while (Obj != NONE) {
				Object Help = Obj;
				Obj++;
				if (!Help.isCreature()) {
					continue;
				}

				TCreature *dest = GetCreature(Help);
				if (dest == nullptr) {
					error("CancelInvisibility: Invalid creature.\n");
					continue;
				}

				if (!dest->IsInvisible()) {
					continue;
				}

				if (WorldType == NON_PVP && dest->IsPeaceful()) {
					continue;
				}

				dest->SetTimer(SKILL_ILLUSION, 0, 0, 0, -1);
				if (dest->Skills[SKILL_ILLUSION]->Get() == 0) {
					dest->Outfit = dest->OrgOutfit;
					AnnounceChangedCreature(dest->ID, CREATURE_OUTFIT);
					NotifyAllCreatures(dest->GetCreatureObject(), OBJECT_CHANGED, NONE);
				} else {
					GraphicalEffect(dx, dy, z, EFFECT_BLOCK_HIT);
					EffectShown = true;
					if (WorldType == PVP_ENFORCED) {
						for (int b = FIRST_BODY_POSITION; b <= LAST_BODY_POSITION; b++) {
							Object Item = GetBodyObject(dest->ID, b);
							if (Item != NONE
									&& Item.getFlag(SKILLBOOST)
									&& (int)Item.getAttribute(BODYPOSITION) == b
									&& Item.getAttribute(SKILLNUMBER) == SKILL_ILLUSION) {
								if (random(1, 5) == 1) {
									Delete(Item, -1);
								}
								break;
							}
						}
					}
				}
			}

			if (!EffectShown) {
				GraphicalEffect(dx, dy, z, EFFECT_MAGIC_BLUE);
			}
		}
	}
}

void CreatureIllusion(TCreature *cr, int Mana, int SoulPoints, char *RaceName, int Duration) {
	if (cr == nullptr) {
		error("CreatureIllusion: Invalid creature given.\n");
		throw ERROR;
	}

	if (RaceName == nullptr) {
		error("CreatureIllusion: Invalid race name given.\n");
		throw ERROR;
	}

	int Race = 0;
	for (int i = 1; i < MAX_RACES; i++) {
		if (stricmp(RaceName, GetRaceName(i), -1) == 0) {
			Race = i;
			break;
		}
	}

	if (Race == 0) {
		throw CREATURENOTEXISTING;
	}

	if (GetRaceNoIllusion(Race)) {
		throw NOTACCESSIBLE;
	}

	SpendManaAndSoulPoints(cr, Mana, SoulPoints, 1000);
	if (cr->Skills[SKILL_ILLUSION]->Get() == 0) {
		cr->Outfit = GetRaceOutfit(Race);
		cr->SetTimer(SKILL_ILLUSION, 1, Duration, Duration, -1);
	}

	GraphicalEffect(cr->GetCreatureObject(), EFFECT_MAGIC_BLUE);
}

void ObjectIllusion(TCreature *cr, int Mana, int SoulPoints, Object Obj, int Duration) {
	if (cr == nullptr) {
		error("ObjectIllusion: Invalid creature given.\n");
		throw ERROR;
	}

	if (!Obj.exists()) {
		error("ObjectIllusion: Given object does not exist.\n");
		throw ERROR;
	}

	if (Obj.isCreature() || Obj.getFlag(UNMOVE)) {
		throw NOTMOVABLE;
	}

	SpendManaAndSoulPoints(cr, Mana, SoulPoints, 1000);
	if (cr->Skills[SKILL_ILLUSION]->Get() == 0) {
		cr->Outfit.OutfitID = 0;
		if (Obj.getFlag(DISGUISE)) {
			cr->Outfit.ObjectType = (unsigned short)Obj.getAttribute(DISGUISETARGET);
		} else {
			cr->Outfit.ObjectType = (unsigned short)Obj.getObjectType().getTypeID();
		}
		cr->SetTimer(SKILL_ILLUSION, 1, Duration, Duration, -1);
	}

	GraphicalEffect(cr->GetCreatureObject(), EFFECT_MAGIC_BLUE);
}

void ChangeData(TCreature *cr, char *Param) {
	if (cr == nullptr) {
		error("ChangeData: Invalid creature given.\n");
		throw ERROR;
	}

	if (Param == nullptr) {
		error("ChangeData: Invalid parameter given.\n");
		throw ERROR;
	}

	if (cr->Type != PLAYER) {
		error("ChangeData: Spell can only be cast by players.\n");
		throw ERROR;
	}

	TPlayer *pl = static_cast<TPlayer *>(cr);
	if (!CheckRight(cr->ID, CREATE_OBJECTS)) {
		return;
	}

	Object Obj = GetBodyObject(cr->ID, BODY_RIGHTHAND);
	Object Obj2 = GetBodyObject(cr->ID, BODY_LEFTHAND);
	if (Obj != NONE && Obj2 != NONE) {
		SendMessage(pl->GetConnection(), TALK_FAILURE_MESSAGE, "First drop one object.");
		return;
	}

	if (Obj2 != NONE) {
		Obj = Obj2;
	}

	if (Obj == NONE) {
		Obj = GetFirstObject(cr->posx, cr->posy, cr->posz);
		while (Obj != NONE) {
			if (Obj.getFlag(KEYDOOR)) {
				break;
			}
			Obj++;
		}
	}

	if (Obj == NONE) {
		return;
	}

	uint32_t newValue = (uint32_t)atol(Param);
	INSTANCEATTRIBUTE Attribute;
	if (Obj.getFlag(LIQUIDCONTAINER)) {
		Attribute = CONTAINERLIQUIDTYPE;
	} else if (Obj.getFlag(KEY) && newValue != 0) {
		Attribute = KEYNUMBER;
	} else if (Obj.getFlag(KEYDOOR) && newValue != 0) {
		Attribute = KEYHOLENUMBER;
	} else if (Obj.getFlag(CUMULATIVE) && newValue != 0 && newValue <= MAX_AMOUNT) {
		Attribute = AMOUNT;
	} else if (Obj.getFlag(RUNE) && newValue != 0 && newValue <= 99) {
		Attribute = CHARGES;
	} else {
		SendMessage(pl->GetConnection(), TALK_FAILURE_MESSAGE, "You can't change %s in this way.", GetName(Obj));
		return;
	}

	Change(Obj, Attribute, newValue);
	GraphicalEffect(cr->GetCreatureObject(), EFFECT_MAGIC_GREEN);
}

void EnchantObject(TCreature *cr, int Mana, int SoulPoints, ObjectType Origin, ObjectType Target) {
	if (cr == nullptr) {
		error("EnchantObject: Invalid creature given.\n");
		throw ERROR;
	}

	Object Obj = GetBodyObject(cr->ID, BODY_RIGHTHAND);
	if (Obj == NONE || Obj.getObjectType() != Origin) {
		Obj = GetBodyObject(cr->ID, BODY_LEFTHAND);
		if (Obj == NONE || Obj.getObjectType() != Origin) {
			throw MAGICITEM;
		}
	}

	SpendManaAndSoulPoints(cr, Mana, SoulPoints, 1000);
	Change(Obj, Target, 0);
	GraphicalEffect(cr->GetCreatureObject(), EFFECT_MAGIC_GREEN);
}

void Convince(TCreature *master, TCreature *slave) {
	if (slave->Type != MONSTER) {
		throw ATTACKNOTALLOWED;
	}

	if (master->Type == PLAYER && master->SummonedCreatures > MAX_SUMMONS - 1) {
		throw TOOMANYSLAVES;
	}

	if (WorldType == NON_PVP && master->IsPeaceful() && slave->IsPeaceful()) {
		throw ATTACKNOTALLOWED;
	}

	if (GetRaceNoConvince(slave->Race)) {
		throw NOTACCESSIBLE;
	}

	int Mana = GetRaceSummonCost(slave->Race);
	SpendManaAndSoulPoints(master, Mana, 0, (WorldType == PVP_ENFORCED) ? 1000 : 2000);
	ConvinceMonster(master, slave);
	GraphicalEffect(master->GetCreatureObject(), EFFECT_MAGIC_BLUE);
	GraphicalEffect(slave->GetCreatureObject(), EFFECT_HIT_AREA);
}

void Challenge(TCreature *cr, int Mana, int SoulPoints, int Radius) {
	SpendManaAndSoulPoints(cr, Mana, SoulPoints, 1000);
	for (int i = 0; i <= Radius; i++) {
		for (int j = 0; j < Circle[i].Count; j++) {
			int dx = cr->posx + Circle[i].x[j];
			int dy = cr->posy + Circle[i].y[j];
			if (!ThrowPossible(cr->posx, cr->posy, cr->posz, dx, dy, cr->posz, 0)) {
				continue;
			}

			GraphicalEffect(dx, dy, cr->posz, EFFECT_MAGIC_BLUE);
			Object Obj = GetFirstSpecObject(dx, dy, cr->posz, ObjectType(TYPEID_CREATURE));
			if (Obj != NONE) {
				TCreature *dest = GetCreature(Obj);
				if (dest == nullptr) {
					error("Challenge: Invalid creature.\n");
				} else if (dest->Type == MONSTER) {
					ChallengeMonster(cr, dest);
				}
			}
		}
	}
}

void FindPerson(TCreature *cr, int Mana, int SoulPoints, char *Name) {
	if (cr == nullptr) {
		error("FindPerson: Invalid creature given.\n");
		throw ERROR;
	}

	if (Name == nullptr) {
		error("FindPerson: Invalid name given.\n");
		throw ERROR;
	}

	if (cr->Type != PLAYER) {
		error("FindPerson: Spell can only be cast by players.\n");
		throw ERROR;
	}

	TPlayer *Opp;
	int Result = IdentifyPlayer(Name, false, !CheckRight(cr->ID, READ_GAMEMASTER_CHANNEL), Opp);
	if (Result == -1) {
		throw PLAYERNOTONLINE;
	}

	if (Result == -2) {
		throw NAMEAMBIGUOUS;
	}

	if (Opp == nullptr) {
		error("FindPerson: Opp is NULL.\n");
		throw ERROR;
	}

	SpendManaAndSoulPoints(cr, Mana, SoulPoints, 1000);
	int Distance = std::max<int>(abs(Opp->posx - cr->posx), abs(Opp->posy - cr->posy));
	if (Distance <= 4) {
		if (Opp->posz < cr->posz) {
			SendMessage(cr->GetConnection(), TALK_INFO_MESSAGE, "%s is above you.", Opp->Name);
		} else if (Opp->posz > cr->posz) {
			SendMessage(cr->GetConnection(), TALK_INFO_MESSAGE, "%s is below you.", Opp->Name);
		} else {
			SendMessage(cr->GetConnection(), TALK_INFO_MESSAGE, "%s is standing next to you.", Opp->Name);
		}
	} else {
		char *Dir;
		switch (Direction(Opp->posx - cr->posx, Opp->posy - cr->posy)) {
			case COMPASS_EAST:
				Dir = "east";
				break;
			case COMPASS_NORTHEAST:
				Dir = "north-east";
				break;
			case COMPASS_NORTH:
				Dir = "north";
				break;
			case COMPASS_NORTHWEST:
				Dir = "north-west";
				break;
			case COMPASS_WEST:
				Dir = "west";
				break;
			case COMPASS_SOUTHWEST:
				Dir = "south-west";
				break;
			case COMPASS_SOUTH:
				Dir = "south";
				break;
			case COMPASS_SOUTHEAST:
				Dir = "south-east";
				break;
			default:
				error("FindPerson: Direction is Null.\n");
				throw ERROR;
		}

		if (Distance < 100) {
			if (Opp->posz < cr->posz) {
				SendMessage(cr->GetConnection(), TALK_INFO_MESSAGE, "%s is on a higher level to the %s.", Opp->Name, Dir);
			} else if (Opp->posz > cr->posz) {
				SendMessage(cr->GetConnection(), TALK_INFO_MESSAGE, "%s is on a lower level to the %s.", Opp->Name, Dir);
			} else {
				SendMessage(cr->GetConnection(), TALK_INFO_MESSAGE, "%s is to the %s.", Opp->Name, Dir);
			}
		} else {
			SendMessage(cr->GetConnection(), TALK_INFO_MESSAGE, "%s is %sfar to the %s.", Opp->Name, (Distance < 251) ? "" : "very ", Dir);
		}
	}

	GraphicalEffect(cr->GetCreatureObject(), EFFECT_MAGIC_BLUE);
}

void GetPosition(TCreature *cr) {
	if (cr == nullptr) {
		error("GetPosition: Invalid creature given.\n");
		throw ERROR;
	}

	if (cr->Type != PLAYER) {
		error("GetPosition: Spell can only be cast by players.\n");
		throw ERROR;
	}

	TPlayer *pl = (TPlayer *)cr;
	if (CheckRight(pl->ID, SHOW_COORDINATE)) {
		SendMessage(pl->GetConnection(), TALK_EVENT_MESSAGE, "Your position is [%d,%d,%d].", pl->posx, pl->posy, pl->posz);
	}
}

void GetQuestValue(TCreature *cr, char *Param) {
	if (cr == nullptr) {
		error("GetQuestValue: Invalid creature given.\n");
		throw ERROR;
	}

	if (Param == nullptr) {
		error("GetQuestValue: Param is NULL.\n");
		return;
	}

	if (cr->Type != PLAYER) {
		error("GetQuestValue: Spell can only be cast by players.\n");
		throw ERROR;
	}

	TPlayer *pl = (TPlayer *)cr;
	if (!CheckRight(pl->ID, CHANGE_SKILLS)) {
		return;
	}

	// A number outside the int range saturates at its bound, as strtol's
	// result does with a 32-bit long.
	int Number = (int)std::clamp(strtol(Param, nullptr, 10), (long)INT32_MIN, (long)INT32_MAX);
	if (in(Number, 0, MAX_QUESTS - 1)) {
		SendMessage(pl->GetConnection(), TALK_INFO_MESSAGE, "Quest value %d is %d.", Number, pl->GetQuestValue(Number));
	} else {
		SendMessage(pl->GetConnection(), TALK_FAILURE_MESSAGE, "Invalid quest number.");
	}
}

void SetQuestValue(TCreature *cr, char *Param1, char *Param2) {
	if (cr == nullptr) {
		error("SetQuestValue: Invalid creature given.\n");
		throw ERROR;
	}

	if (Param1 == nullptr) {
		error("SetQuestValue: Param1 is NULL.\n");
		return;
	}

	if (Param2 == nullptr) {
		error("SetQuestValue: Param2 is NULL.\n");
		return;
	}

	if (cr->Type != PLAYER) {
		error("SetQuestValue: Spell can only be cast by players.\n");
		throw ERROR;
	}

	TPlayer *pl = (TPlayer *)cr;
	if (!CheckRight(pl->ID, CHANGE_SKILLS)) {
		return;
	}

	int Number = (int)std::clamp(strtol(Param1, nullptr, 10), (long)INT32_MIN, (long)INT32_MAX);
	int Value = (int)std::clamp(strtol(Param2, nullptr, 10), (long)INT32_MIN, (long)INT32_MAX);
	if (!in(Number, 0, MAX_QUESTS - 1)) {
		SendMessage(pl->GetConnection(), TALK_FAILURE_MESSAGE, "Invalid quest number.");
		return;
	}

	pl->SetQuestValue(Number, Value);
	SendMessage(pl->GetConnection(), TALK_INFO_MESSAGE, "Quest value %d set to %d.", Number, Value);
}

void ClearQuestValues(TCreature *cr) {
	if (cr == nullptr) {
		error("ClearQuestValues: Invalid creature given.\n");
		throw ERROR;
	}

	if (cr->Type != PLAYER) {
		error("ClearQuestValues: Spell can only be cast by players.\n");
		throw ERROR;
	}

	TPlayer *pl = (TPlayer *)cr;
	if (!CheckRight(pl->ID, CHANGE_SKILLS)) {
		return;
	}

	for (int i = 0; i < MAX_QUESTS; i++) {
		pl->SetQuestValue(i, 0);
	}

	SendMessage(pl->GetConnection(), TALK_INFO_MESSAGE, "All quest values deleted.");
}

void CreateKnowledge(TCreature *cr, char *Param1, char *Param2) {
	if (cr == nullptr) {
		error("CreateKnowledge: Invalid creature given.\n");
		throw ERROR;
	}

	if (Param1 == nullptr) {
		error("CreateKnowledge: Invalid parameter 1 given.\n");
		throw ERROR;
	}

	if (cr->Type != PLAYER) {
		error("CreateKnowledge: Spell can only be cast by players.\n");
		throw ERROR;
	}

	TPlayer *pl = (TPlayer *)cr;
	if (!CheckRight(pl->ID, CHANGE_SKILLS)) {
		return;
	}

	int32_t i = (int)std::clamp(strtol(Param1, nullptr, 10), (long)INT32_MIN, (long)INT32_MAX);
	int32_t k = (Param2 != nullptr) ? (int)std::clamp(strtol(Param2, nullptr, 10), (long)INT32_MIN, (long)INT32_MAX) : 0;
	if (i > 0 && Param2 == nullptr) {
		pl->Skills[SKILL_LEVEL]->Increase(i);
	} else {
		if (i < 0 || i > MAX_SKILLS - 1) {
			return;
		}

		if (k <= 0) {
			return;
		}

		pl->Skills[i]->Increase(k);
	}

	GraphicalEffect(pl->GetCreatureObject(), EFFECT_MAGIC_GREEN);
}

void ChangeProfession(TCreature *cr, char *Param) {
	if (cr == nullptr) {
		error("ChangeProfession: Invalid creature given.\n");
		throw ERROR;
	}

	if (Param == nullptr) {
		error("ChangeProfession: Invalid parameter given.\n");
		throw ERROR;
	}

	if (cr->Type != PLAYER) {
		error("ChangeProfession: Spell can only be cast by players.\n");
		throw ERROR;
	}

	TPlayer *pl = (TPlayer *)cr;
	if (!CheckRight(pl->ID, CHANGE_PROFESSION)) {
		return;
	}

	if (stricmp(Param, "male", -1) == 0) {
		pl->Sex = SEX_MALE;
	} else if (stricmp(Param, "female", -1) == 0) {
		pl->Sex = SEX_FEMALE;
	} else if (stricmp(Param, "none", -1) == 0) {
		pl->ClearProfession();
	} else if (stricmp(Param, "knight", -1) == 0) {
		pl->ClearProfession();
		pl->SetProfession(PROFESSION_KNIGHT);
	} else if (stricmp(Param, "paladin", -1) == 0) {
		pl->ClearProfession();
		pl->SetProfession(PROFESSION_PALADIN);
	} else if (stricmp(Param, "sorcerer", -1) == 0) {
		pl->ClearProfession();
		pl->SetProfession(PROFESSION_SORCERER);
	} else if (stricmp(Param, "druid", -1) == 0) {
		pl->ClearProfession();
		pl->SetProfession(PROFESSION_DRUID);
	} else if (stricmp(Param, "promote", -1) == 0) {
		if (pl->GetRealProfession() == PROFESSION_NONE || pl->GetRealProfession() >= PROFESSION_PROMOTION) {
			return;
		}

		pl->SetProfession(PROFESSION_PROMOTION);
	} else {
		return;
	}

	GraphicalEffect(pl->GetCreatureObject(), EFFECT_MAGIC_BLUE);
}

void EditGuests(TCreature *cr) {
	if (cr == nullptr) {
		error("EditGuests: Invalid creature given.\n");
		throw ERROR;
	}

	if (cr->Type != PLAYER) {
		error("EditGuests: Spell can only be cast by players.\n");
		throw ERROR;
	}

	TPlayer *pl = (TPlayer *)cr;
	unsigned short HouseID = GetHouseID(pl->posx, pl->posy, pl->posz);
	if (HouseID == 0) {
		throw NOTACCESSIBLE;
	}

	char Buffer[4096];
	ShowGuestList(HouseID, pl, Buffer);
	SendEditList(pl->GetConnection(), EDIT_LIST_HOUSE_GUESTS, HouseID, Buffer);
}

void EditSubowners(TCreature *cr) {
	if (cr == nullptr) {
		error("EditSubowners: Invalid creature given.\n");
		throw ERROR;
	}

	if (cr->Type != PLAYER) {
		error("EditSubowners: Spell can only be cast by players.\n");
		throw ERROR;
	}

	TPlayer *pl = (TPlayer *)cr;
	unsigned short HouseID = GetHouseID(pl->posx, pl->posy, pl->posz);
	if (HouseID == 0) {
		throw NOTACCESSIBLE;
	}

	char Buffer[4096];
	ShowSubownerList(HouseID, pl, Buffer);
	SendEditList(pl->GetConnection(), EDIT_LIST_HOUSE_SUBOWNERS, HouseID, Buffer);
}

void KickGuest(TCreature *cr, char *Name) {
	if (cr == nullptr) {
		error("KickGuest(magic): Invalid creature given.\n");
		throw ERROR;
	}

	if (Name == nullptr) {
		error("KickGuest(magic): Invalid guest given.\n");
		throw ERROR;
	}

	if (cr->Type != PLAYER) {
		error("KickGuest(magic): Spell can only be cast by players.\n");
		throw ERROR;
	}

	TPlayer *pl = (TPlayer *)cr;
	TPlayer *Guest;
	int Result = IdentifyPlayer(Name, false, !CheckRight(pl->ID, READ_GAMEMASTER_CHANNEL), Guest);
	if (Result == -1) {
		throw PLAYERNOTONLINE;
	}

	if (Result == -2) {
		throw NAMEAMBIGUOUS;
	}

	unsigned short HouseID = GetHouseID(Guest->posx, Guest->posy, Guest->posz);
	if (HouseID == 0) {
		throw NOTACCESSIBLE;
	}

	KickGuest(HouseID, pl, Guest);
}

void EditNameDoor(TCreature *cr) {
	if (cr == nullptr) {
		error("EditNameDoor: Invalid creature given.\n");
		throw ERROR;
	}

	if (cr->Type != PLAYER) {
		error("EditNameDoor: Spell can only be cast by players.\n");
		throw ERROR;
	}

	TPlayer *pl = (TPlayer *)cr;
	int x = pl->posx;
	int y = pl->posy;
	int z = pl->posz;
	Object Door = GetFirstObject(x, y, z);
	while (Door != NONE) {
		if (Door.getFlag(NAMEDOOR)) {
			break;
		}
		Door++;
	}

	if (Door == NONE) {
		switch (pl->Direction) {
			case DIRECTION_NORTH:
				y--;
				break;
			case DIRECTION_EAST:
				x++;
				break;
			case DIRECTION_SOUTH:
				y++;
				break;
			case DIRECTION_WEST:
				x--;
				break;
		}

		Door = GetFirstObject(x, y, z);
		while (Door != NONE) {
			if (Door.getFlag(NAMEDOOR)) {
				break;
			}
			Door++;
		}
	}

	if (Door == NONE) {
		print(3, "No NameDoor found.\n");
		throw NOTACCESSIBLE;
	}

	char Buffer[4096];
	ShowNameDoor(Door, pl, Buffer);
	SendEditList(pl->GetConnection(), EDIT_LIST_NAME_DOOR, Door.getObjectID(), Buffer);
}

void Notation(TCreature *cr, char *Name, char *Comment) {
	if (cr == nullptr) {
		error("Notation: Invalid creature given.\n");
		throw ERROR;
	}

	if (Name == nullptr) {
		error("Notation: Invalid name given.\n");
		throw ERROR;
	}

	if (Comment == nullptr) {
		error("Notation: Invalid remark given.\n");
		throw ERROR;
	}

	if (cr->Type != PLAYER) {
		error("Notation: Spell can only be cast by players.\n");
		throw ERROR;
	}

	TPlayer *pl = (TPlayer *)cr;
	SendMessage(pl->GetConnection(), TALK_FAILURE_MESSAGE, "Spell is not available any more. Press Ctrl+Y for the dialog.");
}

void NameLock(TCreature *cr, char *Name) {
	if (cr == nullptr) {
		error("NameLock: Invalid creature given.\n");
		throw ERROR;
	}

	if (Name == nullptr) {
		error("NameLock: Invalid name given.\n");
		throw ERROR;
	}

	if (cr->Type != PLAYER) {
		error("NameLock: Spell can only be cast by players.\n");
		throw ERROR;
	}

	TPlayer *pl = (TPlayer *)cr;
	SendMessage(pl->GetConnection(), TALK_FAILURE_MESSAGE, "Spell is not available any more. Press Ctrl+Y for the dialog.");
}

void BanishAccount(TCreature *cr, char *Name, int, char *Reason) {
	if (cr == nullptr) {
		error("BanishAccount: Invalid creature given.\n");
		throw ERROR;
	}

	if (Name == nullptr) {
		error("BanishAccount: Invalid name given.\n");
		throw ERROR;
	}

	if (Reason == nullptr) {
		error("BanishAccount: Invalid reason given.\n");
		throw ERROR;
	}

	if (cr->Type != PLAYER) {
		error("BanishAccount: Spell can only be cast by players.\n");
		throw ERROR;
	}

	TPlayer *pl = (TPlayer *)cr;
	SendMessage(pl->GetConnection(), TALK_FAILURE_MESSAGE, "Spell is not available any more. Press Ctrl+Y for the dialog.");
}

void DeleteAccount(TCreature *cr, char *Name, char *Reason) {
	if (cr == nullptr) {
		error("DeleteAccount: Invalid creature given.\n");
		throw ERROR;
	}

	if (Name == nullptr) {
		error("DeleteAccount: Invalid name given.\n");
		throw ERROR;
	}

	if (Reason == nullptr) {
		error("DeleteAccount: Invalid reason given.\n");
		throw ERROR;
	}

	if (cr->Type != PLAYER) {
		error("DeleteAccount: Spell can only be cast by players.\n");
		throw ERROR;
	}

	TPlayer *pl = (TPlayer *)cr;
	SendMessage(pl->GetConnection(), TALK_FAILURE_MESSAGE, "Spell is not available any more. Press Ctrl+Y for the dialog.");
}

void BanishCharacter(TCreature *cr, char *Name, int, char *Reason) {
	if (cr == nullptr) {
		error("BanishCharacter: Invalid creature given.\n");
		throw ERROR;
	}

	if (Name == nullptr) {
		error("BanishCharacter: Invalid name given.\n");
		throw ERROR;
	}

	if (Reason == nullptr) {
		error("BanishCharacter: Invalid reason given.\n");
		throw ERROR;
	}

	if (cr->Type != PLAYER) {
		error("BanishCharacter: Spell can only be cast by players.\n");
		throw ERROR;
	}

	TPlayer *pl = (TPlayer *)cr;
	SendMessage(pl->GetConnection(), TALK_FAILURE_MESSAGE, "Spell is not available any more. Press Ctrl+Y for the dialog.");
}

void DeleteCharacter(TCreature *cr, char *Name, char *Reason) {
	if (cr == nullptr) {
		error("DeleteCharacter: Invalid creature given.\n");
		throw ERROR;
	}

	if (Name == nullptr) {
		error("DeleteCharacter: Invalid name given.\n");
		throw ERROR;
	}

	if (Reason == nullptr) {
		error("DeleteCharacter: Invalid reason given.\n");
		throw ERROR;
	}

	if (cr->Type != PLAYER) {
		error("DeleteCharacter: Spell can only be cast by players.\n");
		throw ERROR;
	}

	TPlayer *pl = (TPlayer *)cr;
	SendMessage(pl->GetConnection(), TALK_FAILURE_MESSAGE, "Spell is not available any more. Press Ctrl+Y for the dialog.");
}

void IPBanishment(TCreature *cr, char *Name, char *Reason) {
	if (cr == nullptr) {
		error("IPBanishment: Invalid creature given.\n");
		throw ERROR;
	}

	if (Name == nullptr) {
		error("IPBanishment: Invalid name given.\n");
		throw ERROR;
	}

	if (Reason == nullptr) {
		error("IPBanishment: Invalid reason given.\n");
		throw ERROR;
	}

	if (cr->Type != PLAYER) {
		error("IPBanishment: Spell can only be cast by players.\n");
		throw ERROR;
	}

	TPlayer *pl = (TPlayer *)cr;
	SendMessage(pl->GetConnection(), TALK_FAILURE_MESSAGE, "Spell is not available any more. Press Ctrl+Y for the dialog.");
}

void SetNameRule(TCreature *cr, char *Name) {
	if (cr == nullptr) {
		error("SetNameRule: Invalid creature given.\n");
		throw ERROR;
	}

	if (Name == nullptr) {
		error("SetNameRule: Invalid name given.\n");
		throw ERROR;
	}

	if (cr->Type != PLAYER) {
		error("SetNameRule: Spell can only be cast by players.\n");
		throw ERROR;
	}

	TPlayer *pl = (TPlayer *)cr;
	SendMessage(pl->GetConnection(), TALK_FAILURE_MESSAGE, "Spell is not available any more.");
}

void KickPlayer(TCreature *cr, char *Name) {
	if (cr == nullptr) {
		error("KickPlayer: Invalid creature given.\n");
		throw ERROR;
	}

	if (Name == nullptr) {
		error("KickPlayer: Invalid name given.\n");
		throw ERROR;
	}

	if (cr->Type != PLAYER) {
		error("KickPlayer: Spell can only be cast by players.\n");
		throw ERROR;
	}

	TPlayer *pl = (TPlayer *)cr;
	if (!CheckRight(pl->ID, KICK)) {
		return;
	}

	TPlayer *Opp;
	int Result = IdentifyPlayer(Name, false, !CheckRight(pl->ID, READ_GAMEMASTER_CHANNEL), Opp);
	if (Result == -1) {
		throw PLAYERNOTONLINE;
	}

	if (Result == -2) {
		throw NAMEAMBIGUOUS;
	}

	GraphicalEffect(Opp->GetCreatureObject(), EFFECT_MAGIC_GREEN);
	Opp->StartLogout(true, true);
	SendMessage(pl->GetConnection(), TALK_INFO_MESSAGE, "Player %s kicked out of the game.", Opp->Name);
	Log("banish", "%s kicks %s.\n", pl->Name, Opp->Name);
}

void HomeTeleport(TCreature *cr, char *Name) {
	if (cr == nullptr) {
		error("HomeTeleport: Invalid creature given.\n");
		throw ERROR;
	}

	if (Name == nullptr || Name[0] == 0) {
		error("HomeTeleport: Name does not exist.\n");
		throw ERROR;
	}

	if (cr->Type != PLAYER) {
		error("HomeTeleport: Spell can only be cast by players.\n");
		throw ERROR;
	}

	TPlayer *pl = (TPlayer *)cr;
	if (!CheckRight(pl->ID, HOME_TELEPORT)) {
		return;
	}

	TPlayer *Opp;
	int Result = IdentifyPlayer(Name, false, !CheckRight(pl->ID, READ_GAMEMASTER_CHANNEL), Opp);
	if (Result == -1) {
		throw PLAYERNOTONLINE;
	}

	if (Result == -2) {
		throw NAMEAMBIGUOUS;
	}

	Object Obj = Opp->GetCreatureObject();
	GraphicalEffect(Obj, EFFECT_POFF);
	Move(0, Obj, GetMapContainer(Opp->startx, Opp->starty, Opp->startz), -1, false, NONE);
	GraphicalEffect(Obj, EFFECT_MAGIC_BLUE);
	SendMessage(pl->GetConnection(), TALK_INFO_MESSAGE, "Player %s has been moved to the temple.", Opp->Name);
	Log("banish", "%s teleports %s to the temple.\n", pl->Name, Opp->Name);
}

void SpellFailed(uint32_t CreatureID) {
	TCreature *cr = GetCreature(CreatureID);
	if (cr == nullptr) {
		error("SpellFailed: Creature does not exist.\n");
		return;
	}

	GraphicalEffect(cr->GetCreatureObject(), EFFECT_POFF);
}

void CharacterRightSpell(uint32_t CreatureID, int SpellNr, char (*SpellStr)[512]) {
	TCreature *cr = GetCreature(CreatureID);
	if (cr == nullptr) {
		error("CharacterRightSpell: Creature does not exist.\n");
		throw ERROR;
	}

	if (cr->Type != PLAYER) {
		return;
	}

	switch (SpellNr) {
		case SPELL_GET_ITEM_COUNT:
			CreateThing(cr, SpellStr[2], SpellStr[3]);
			break;
		case SPELL_GET_ITEM:
			CreateThing(cr, SpellStr[2], nullptr);
			break;
		case SPELL_MOVE:
			Teleport(cr, SpellStr[2]);
			break;
		case SPELL_GET_EXPERIENCE:
			CreateKnowledge(cr, SpellStr[3], nullptr);
			break;
		case SPELL_CHANGE_DATA:
			ChangeData(cr, SpellStr[2]);
			break;
		case SPELL_GET_SKILL_EXPERIENCE:
			CreateKnowledge(cr, SpellStr[3], SpellStr[4]);
			break;
		case SPELL_TELEPORT_TO_FRIEND:
			TeleportToCreature(cr, SpellStr[3]);
			break;
		case SPELL_RETRIEVE_FRIEND:
			TeleportPlayerToMe(cr, SpellStr[3]);
			break;
		case SPELL_SUMMON_WILD_CREATURE:
			SummonCreature(cr, 0, SpellStr[3], true);
			break;
		case SPELL_GET_POSITION:
			GetPosition(cr);
			break;
		case SPELL_CREATE_GOLD:
			CreateMoney(cr, SpellStr[3]);
			break;
		case SPELL_CHANGE_PROFESSION_OR_SEX:
			ChangeProfession(cr, SpellStr[3]);
			break;
		case SPELL_INVITE_GUESTS:
			EditGuests(cr);
			break;
		case SPELL_INVITE_SUBOWNERS:
			EditSubowners(cr);
			break;
		case SPELL_KICK_GUEST:
			KickGuest(cr, SpellStr[3]);
			break;
		case SPELL_EDIT_DOOR:
			EditNameDoor(cr);
			break;
		case SPELL_GET_QUEST_VALUE:
			GetQuestValue(cr, SpellStr[3]);
			break;
		case SPELL_SET_QUEST_VALUE:
			SetQuestValue(cr, SpellStr[3], SpellStr[4]);
			break;
		case SPELL_DESINTEGRATE_SPELL:
			CleanupField(cr);
			break;
		case SPELL_LEVITATE_GAMEMASTER:
			MagicClimbing(cr, SpellStr[3]);
			break;
		case SPELL_CLEAR_QUEST_VALUES:
			ClearQuestValues(cr);
			break;
		case SPELL_KILL_ALL_CREATURES:
			KillAllMonsters(cr, EFFECT_DEATH, 2);
			break;
		case SPELL_START_MONSTERRAID:
			StartMonsterraid(cr, SpellStr[4]);
			break;
	}
}

void AccountRightSpell(uint32_t CreatureID, int SpellNr, char (*SpellStr)[512]) {
	TCreature *cr = GetCreature(CreatureID);
	if (cr == nullptr) {
		error("AccountRightSpell: Creature does not exist.\n");
		throw ERROR;
	}

	if (cr->Type != PLAYER) {
		return;
	}

	switch (SpellNr) {
		case SPELL_BANISH_ACCOUNT:
			BanishAccount(cr, SpellStr[3], (int)std::clamp(strtol(SpellStr[4], nullptr, 10), (long)INT32_MIN, (long)INT32_MAX), SpellStr[5]);
			break;
		case SPELL_TEMPLE_TELEPORT:
			HomeTeleport(cr, SpellStr[2]);
			break;
		case SPELL_DELETE_ACCOUNT:
			DeleteAccount(cr, SpellStr[4], SpellStr[5]);
			break;
		case SPELL_SET_NAMERULE:
			SetNameRule(cr, SpellStr[2]);
			break;
		case SPELL_ENTRY_IN_CRIMINAL_RECORD:
			Notation(cr, SpellStr[2], SpellStr[3]);
			break;
		case SPELL_NAMELOCK:
			NameLock(cr, SpellStr[3]);
			break;
		case SPELL_KICK_PLAYER:
			KickPlayer(cr, SpellStr[2]);
			break;
		case SPELL_DELETE_CHARACTER:
			DeleteCharacter(cr, SpellStr[4], SpellStr[5]);
			break;
		case SPELL_BANISH_IP_ADDRESS:
			IPBanishment(cr, SpellStr[3], SpellStr[4]);
			break;
		case SPELL_BANISH_CHARACTER:
			BanishCharacter(cr, SpellStr[3], (int)std::clamp(strtol(SpellStr[4], nullptr, 10), (long)INT32_MIN, (long)INT32_MAX), SpellStr[5]);
			break;
	}
}

void CastSpell(uint32_t CreatureID, int SpellNr, char (*SpellStr)[512]) {
	TCreature *cr = GetCreature(CreatureID);
	if (cr == nullptr) {
		error("CastSpell: Creature does not exist.\n", CreatureID);
		throw ERROR;
	}

	CheckSpellbook(cr, SpellNr);
	CheckAccount(cr, SpellNr);
	CheckLevel(cr, SpellNr);
	CheckRing(cr, SpellNr);
	if (cr->EarliestSpellTime > ServerMilliseconds) {
		throw EXHAUSTED;
	}

	if (IsAggressiveSpell(SpellNr)
			&& cr->Type == PLAYER
			&& !CheckRight(cr->ID, ATTACK_EVERYWHERE)
			&& IsProtectionZone(cr->posx, cr->posy, cr->posz)) {
		throw PROTECTIONZONE;
	}

	int Damage;
	switch (SpellNr) {
		case SPELL_LIGHT_HEALING:
			Heal(cr, SpellList[SpellNr].Mana, SpellList[SpellNr].SoulPoints, ComputeDamage(cr, SpellNr, 20, 10));
			break;
		case SPELL_INTENSE_HEALING:
			Heal(cr, SpellList[SpellNr].Mana, SpellList[SpellNr].SoulPoints, ComputeDamage(cr, SpellNr, 40, 20));
			break;
		case SPELL_ULTIMATE_HEALING:
			Heal(cr, SpellList[SpellNr].Mana, SpellList[SpellNr].SoulPoints, ComputeDamage(cr, SpellNr, 250, 50));
			break;
		case SPELL_HASTE:
			MagicGoStrength(cr, cr, SpellList[SpellNr].Mana, SpellList[SpellNr].SoulPoints, 30, 3);
			break;
		case SPELL_SUMMON_CREATURE:
			SummonCreature(cr, SpellList[SpellNr].Mana, SpellStr[3], false);
			break;
		case SPELL_LIGHT:
			Enlight(cr, SpellList[SpellNr].Mana, SpellList[SpellNr].SoulPoints, 6, 500);
			break;
		case SPELL_GREAT_LIGHT:
			Enlight(cr, SpellList[SpellNr].Mana, SpellList[SpellNr].SoulPoints, 8, 1000);
			break;
		case SPELL_ENERGY_WAVE:
			Damage = ComputeDamage(cr, SpellNr, 150, 50);
			AngleCombat(cr, SpellList[SpellNr].Mana, SpellList[SpellNr].SoulPoints, Damage, EFFECT_TELEPORT, 5, 30, DAMAGE_ENERGY);
			break;
		case SPELL_FIRE_WAVE:
			Damage = ComputeDamage(cr, SpellNr, 30, 10);
			AngleCombat(cr, SpellList[SpellNr].Mana, SpellList[SpellNr].SoulPoints, Damage, EFFECT_FIRE_AREA, 4, 45, DAMAGE_FIRE);
			break;
		case SPELL_FIND_PERSON:
			FindPerson(cr, SpellList[SpellNr].Mana, SpellList[SpellNr].SoulPoints, SpellStr[2]);
			break;
		case SPELL_ENERGY_BEAM:
			Damage = ComputeDamage(cr, SpellNr, 60, 20);
			AngleCombat(cr, SpellList[SpellNr].Mana, SpellList[SpellNr].SoulPoints, Damage, EFFECT_EXPLOSION, 5, 0, DAMAGE_ENERGY);
			break;
		case SPELL_GREAT_ENERGY_BEAM:
			Damage = ComputeDamage(cr, SpellNr, 120, 80);
			AngleCombat(cr, SpellList[SpellNr].Mana, SpellList[SpellNr].SoulPoints, Damage, EFFECT_EXPLOSION, 8, 0, DAMAGE_ENERGY);
			break;
		case SPELL_ULTIMATE_EXPLOSION:
			Damage = ComputeDamage(cr, SpellNr, 250, 50);
			MassCombat(cr, cr->GetCreatureObject(), SpellList[SpellNr].Mana, SpellList[SpellNr].SoulPoints, Damage, EFFECT_EXPLOSION_AREA, 6, DAMAGE_PHYSICAL, ANIMATION_FIRE);
			break;
		case SPELL_ANTIDOTE:
			NegatePoison(cr, SpellList[SpellNr].Mana, SpellList[SpellNr].SoulPoints, cr);
			break;
		case SPELL_CREATURE_ILLUSION:
			CreatureIllusion(cr, SpellList[SpellNr].Mana, SpellList[SpellNr].SoulPoints, SpellStr[4], 200);
			break;
		case SPELL_STRONG_HASTE:
			MagicGoStrength(cr, cr, SpellList[SpellNr].Mana, SpellList[SpellNr].SoulPoints, 70, 2);
			break;
		case SPELL_FOOD:
			CreateFood(cr, SpellList[SpellNr].Mana, SpellList[SpellNr].SoulPoints);
			break;
		case SPELL_MAGIC_SHIELD:
			Shielding(cr, SpellList[SpellNr].Mana, SpellList[SpellNr].SoulPoints, 200);
			break;
		case SPELL_INVISIBLE:
			Invisibility(cr, SpellList[SpellNr].Mana, SpellList[SpellNr].SoulPoints, 200);
			break;
		case SPELL_POISONED_ARROW:
			CreateArrows(cr, SpellList[SpellNr].Mana, SpellList[SpellNr].SoulPoints, CONJURE_POISONED_ARROW, 5);
			break;
		case SPELL_EXPLOSIVE_ARROW:
			CreateArrows(cr, SpellList[SpellNr].Mana, SpellList[SpellNr].SoulPoints, CONJURE_EXPLOSIVE_ARROW, 3);
			break;
		case SPELL_CONJURE_ARROW:
			CreateArrows(cr, SpellList[SpellNr].Mana, SpellList[SpellNr].SoulPoints, CONJURE_ARROW, 10);
			break;
		case SPELL_POISON_STORM:
			Damage = ComputeDamage(cr, SpellNr, 200, 50);
			MassCombat(cr, cr->GetCreatureObject(), SpellList[SpellNr].Mana, SpellList[SpellNr].SoulPoints, Damage, EFFECT_POISON, 8, DAMAGE_POISON_CONDITION, ANIMATION_NONE);
			break;
		case SPELL_ULTIMATE_LIGHT:
			Enlight(cr, SpellList[SpellNr].Mana, SpellList[SpellNr].SoulPoints, 9, 2000);
			break;
		case SPELL_MAGIC_ROPE:
			MagicRope(cr, SpellList[SpellNr].Mana, SpellList[SpellNr].SoulPoints);
			break;
		case SPELL_CONJURE_BOLT:
			CreateArrows(cr, SpellList[SpellNr].Mana, SpellList[SpellNr].SoulPoints, CONJURE_BOLT, 5);
			break;
		case SPELL_BERSERK:
			Damage = ComputeDamage(cr, SpellNr, 80, 20);
			Damage = cr->Skills[SKILL_LEVEL]->Get() * Damage / 25;
			MassCombat(cr, cr->GetCreatureObject(), cr->Skills[SKILL_LEVEL]->Get() * 4, 0, Damage, EFFECT_HIT_AREA, 2, DAMAGE_PHYSICAL, ANIMATION_NONE);
			break;
		case SPELL_LEVITATE:
			MagicClimbing(cr, SpellList[SpellNr].Mana, SpellList[SpellNr].SoulPoints, SpellStr[3]);
			break;
		case SPELL_MASS_HEALING:
			Damage = ComputeDamage(cr, SpellNr, 200, 40);
			MassHeal(cr, cr->GetCreatureObject(), SpellList[SpellNr].Mana, SpellList[SpellNr].SoulPoints, Damage, 4);
			break;
		case SPELL_HEAL_FRIEND:
			HealFriend(cr, SpellStr[3], SpellList[SpellNr].Mana, SpellList[SpellNr].SoulPoints, ComputeDamage(cr, SpellNr, 120, 40));
			break;
		case SPELL_UNDEAD_LEGION:
			MassRaiseDead(cr, cr->GetCreatureObject(), SpellList[SpellNr].Mana, SpellList[SpellNr].SoulPoints, 4);
			break;
		case SPELL_FORCE_STRIKE:
			Damage = ComputeDamage(cr, SpellNr, 45, 10);
			AngleCombat(cr, SpellList[SpellNr].Mana, SpellList[SpellNr].SoulPoints, Damage, EFFECT_DEATH, 1, 0, DAMAGE_PHYSICAL);
			break;
		case SPELL_ENERGY_STRIKE:
			Damage = ComputeDamage(cr, SpellNr, 45, 10);
			AngleCombat(cr, SpellList[SpellNr].Mana, SpellList[SpellNr].SoulPoints, Damage, EFFECT_TELEPORT, 1, 0, DAMAGE_ENERGY);
			break;
		case SPELL_FLAME_STRIKE:
			Damage = ComputeDamage(cr, SpellNr, 45, 10);
			AngleCombat(cr, SpellList[SpellNr].Mana, SpellList[SpellNr].SoulPoints, Damage, EFFECT_FIRE_AREA, 1, 0, DAMAGE_FIRE);
			break;
		case SPELL_CANCEL_INVISIBILITY:
			CancelInvisibility(cr, cr->GetCreatureObject(), SpellList[SpellNr].Mana, SpellList[SpellNr].SoulPoints, 4);
			break;
		case SPELL_ENCHANT_STAFF:
			EnchantObject(cr, SpellList[SpellNr].Mana, SpellList[SpellNr].SoulPoints, GetNewObjectType(90, 25), GetNewObjectType(90, 57));
			break;
		case SPELL_CHALLENGE:
			Challenge(cr, SpellList[SpellNr].Mana, SpellList[SpellNr].SoulPoints, 2);
			break;
		case SPELL_WILD_GROWTH:
			CreateField(cr, SpellList[SpellNr].Mana, SpellList[SpellNr].SoulPoints, FIELD_RUSHWOOD);
			break;
		case SPELL_POWER_BOLT:
			CreateArrows(cr, SpellList[SpellNr].Mana, SpellList[SpellNr].SoulPoints, CONJURE_POWER_BOLT, 1);
			break;
	}

	if (IsAggressiveSpell(SpellNr)) {
		cr->BlockLogout(60, false);
	}
}

void RuneSpell(uint32_t CreatureID, int SpellNr) {
	TCreature *cr = GetCreature(CreatureID);
	if (cr == nullptr) {
		error("RuneSpell: Creature does not exist.\n");
		throw ERROR;
	}

	if (SpellList[SpellNr].RuneGr == 0) {
		error("RuneSpell: Spell %d is a rune spell but has no rune.\n", SpellNr);
		throw ERROR;
	}

	Object Obj1 = GetBodyObject(CreatureID, BODY_RIGHTHAND);
	Object Obj2 = GetBodyObject(CreatureID, BODY_LEFTHAND);
	CheckSpellbook(cr, SpellNr);
	CheckAccount(cr, SpellNr);
	CheckLevel(cr, SpellNr);
	CheckRing(cr, SpellNr);
	if (cr->EarliestSpellTime > ServerMilliseconds) {
		throw EXHAUSTED;
	}

	bool executed = false;
	if (Obj1.exists() && Obj1.getObjectType() == GetSpecialObject(RUNE_BLANK)) {
		SpendManaAndSoulPoints(cr, SpellList[SpellNr].Mana, SpellList[SpellNr].SoulPoints, 1000);
		Change(Obj1, GetNewObjectType(SpellList[SpellNr].RuneGr, SpellList[SpellNr].RuneNr), SpellList[SpellNr].Amount);
		executed = true;
	}

	if (Obj2.exists() && Obj2.getObjectType() == GetSpecialObject(RUNE_BLANK)) {
		try {
			SpendManaAndSoulPoints(cr, SpellList[SpellNr].Mana, SpellList[SpellNr].SoulPoints, 1000);
			Change(Obj2, GetNewObjectType(SpellList[SpellNr].RuneGr, SpellList[SpellNr].RuneNr), SpellList[SpellNr].Amount);
			executed = true;
		} catch (RESULT r) {
			if (!executed) {
				throw;
			}
		}
	}

	if (!executed) {
		throw MAGICITEM;
	}

	GraphicalEffect(cr->GetCreatureObject(), EFFECT_MAGIC_RED);
}

int TypeOfSpell(char *Text) {
	if (Text != nullptr) {
		for (int i = SYLLABLE_AL; i <= SYLLABLE_OM; i++) {
			if (stricmp(Text, SpellSyllable[i], 2) == 0) {
				return i;
			}
		}
	}

	return 0;
}

int FindSpell(unsigned char *Syllable) {
	int Spell = 0;
	int MinParams = 100;
	for (int i = 1; i < MAX_SPELLS; i++) {
		int Params = 0;
		for (int j = 0; j < MAX_SPELL_SYLLABLES; j++) {
			if (SpellList[i].Syllable[j] == SYLLABLE_PARA) {
				if (Syllable[j] == 0) {
					break;
				}
			} else if (SpellList[i].Syllable[j] != Syllable[j]) {
				break;
			}

			if (SpellList[i].Syllable[j] == SYLLABLE_PARA) {
				Params++;
			}

			if (SpellList[i].Syllable[j] == 0) {
				if (Params < MinParams) {
					Spell = i;
					MinParams = Params;
				}
				break;
			}
		}
	}

	return Spell;
}

int FindSpell(Object Obj) {
	if (!Obj.exists()) {
		error("FindSpell: Given object does not exist.\n");
		return 0;
	}

	if (!Obj.getFlag(RUNE)) {
		error("FindSpell: Given object is not magic.\n");
		return 0;
	}

	for (int i = 1; i < MAX_SPELLS; i++) {
		if (Obj.getObjectType() == GetNewObjectType(SpellList[i].RuneGr, SpellList[i].RuneNr)) {
			return i;
		}
	}

	return 0;
}

void GetSpellString(int SpellNr, char *Text) {
	Text[0] = 0;
	if (SpellNr < 1 || SpellNr > MAX_SPELLS - 1) {
		error("GetSpellString: Invalid spell number %d.\n", SpellNr);
		return;
	}

	for (int i = 0; i < MAX_SPELL_SYLLABLES; i++) {
		if (SpellSyllable[SpellList[SpellNr].Syllable[i]][0] == 0) {
			break;
		}

		strcat(Text, SpellSyllable[SpellList[SpellNr].Syllable[i]]);
		if (i > 0) {
			strcat(Text, " ");
		}
	}

	if (Text[0] != 0) {
		Text[strlen(Text) - 1] = 0;
	}
}

void GetMagicItemDescription(Object Obj, char *SpellString, int &MagicLevel) {
	MagicLevel = 0;
	SpellString[0] = 0;
	if (!Obj.exists()) {
		error("GetMagicItemDescription: Given object does not exist.\n");
		return;
	}

	if (!Obj.getFlag(RUNE)) {
		error("GetMagicItemDescription: Given object is not magic.\n");
		return;
	}

	int SpellNr = FindSpell(Obj);
	if (SpellNr == 0) {
		error("GetMagicItemDescription: Object %d has no spell.\n", Obj.getObjectType().getTypeID());
		return;
	}

	GetSpellString(SpellNr, SpellString);
	MagicLevel = SpellList[SpellNr].RuneLevel;
}

void GetSpellbook(uint32_t CharacterID, char *Buffer) {
	TPlayer *pl = GetPlayer(CharacterID);
	if (pl == nullptr) {
		error("GetSpellbook: Player does not exist.\n");
		return;
	}

	if (Buffer == nullptr) {
		error("GetSpellbook: Given buffer does not exist.\n");
		return;
	}

	Buffer[0] = 0;
	for (int Level = 1; Level <= 60; Level++) {
		bool first = true;
		char Help[100];
		for (int i = 1; i < MAX_SPELLS; i++) {
			if (SpellList[i].Level != Level || !pl->SpellKnown(i)) {
				continue;
			}

			if (first) {
				sprintf(Help, "Spells for Level %d\n", Level);
				strcat(Buffer, Help);
				first = false;
			}

			GetSpellString(i, Help);
			if (strlen(Help) < 2) {
				error("GetSpellbook: Spell %d has no spell words.\n", i);
				continue;
			}

			strcat(Buffer, "  ");
			strcat(Buffer, Help);
			if (SpellList[i].Comment[0] != 0) {
				strcat(Buffer, " - ");
				strcat(Buffer, SpellList[i].Comment);
				if (i == SPELL_BERSERK) {
					strcpy(Help, " : 4*Level");
				} else if (i == SPELL_SUMMON_CREATURE || i == SPELL_CONVINCE_CREATURE) {
					strcpy(Help, " : var");
				} else {
					sprintf(Help, " : %d", SpellList[i].Mana);
				}
				strcat(Buffer, Help);
			}
			strcat(Buffer, "\n");
		}

		if (!first) {
			strcat(Buffer, "\n");
		}
	}
}

int GetSpellLevel(int SpellNr) {
	if (SpellNr < 1 || SpellNr > MAX_SPELLS - 1) {
		error("GetSpellLevel: Invalid spell number %d.\n", SpellNr);
		return 1;
	}

	return SpellList[SpellNr].Level;
}

int CheckForSpell(uint32_t CreatureID, char *Text) {
	int SpellType = TypeOfSpell(Text);
	if (SpellType == 0) {
		return 0;
	}

	std::istringstream IS(Text);
	unsigned char Syllable[MAX_SPELL_SYLLABLES + 1];
	char SpellStr[MAX_SPELL_SYLLABLES + 1][512];
	Syllable[0] = (unsigned char)SpellType;
	strcpy(SpellStr[0], SpellSyllable[SpellType]);
	IS.get();
	IS.get();
	int syl = 1;
	Syllable[syl] = 0;
	SpellStr[syl][0] = 0;
	while (syl < MAX_SPELL_SYLLABLES && !IS.eof()) {
		int Next;
		while (isSpace(Next = IS.peek())) {
			IS.get();
		}

		if (Next == '"') {
			IS.get();
			IS.get(SpellStr[syl], 510, '"');
			IS.get();
		} else {
			IS.get(SpellStr[syl], 100, ' ');
			if (strlen(SpellStr[syl]) == 99) {
				return 0;
			}
		}

		if (SpellStr[syl][0] == 0) {
			break;
		}

		// The word is not empty, so the empty entries of SpellSyllable never
		// match it.
		for (int i = 1; i <= MAX_SYLLABLES - 1; i++) {
			if (stricmp(SpellStr[syl], SpellSyllable[i], -1) == 0) {
				Syllable[syl] = (unsigned char)i;
			}
		}

		if (Syllable[syl] == 0) {
			Syllable[syl] = SYLLABLE_PARA;
		}

		syl++;
		Syllable[syl] = 0;
		SpellStr[syl][0] = 0;
	}

	int SpellNr = FindSpell(Syllable);
	if (SpellNr == 0) {
		return 0;
	}

	try {
		switch (SpellType) {
			case SYLLABLE_AL:
				CharacterRightSpell(CreatureID, SpellNr, SpellStr);
				break;
			case SYLLABLE_AD:
				RuneSpell(CreatureID, SpellNr);
				break;
			case SYLLABLE_EX:
			case SYLLABLE_UT:
				CastSpell(CreatureID, SpellNr, SpellStr);
				break;
			case SYLLABLE_OM:
				AccountRightSpell(CreatureID, SpellNr, SpellStr);
				break;
			default:
				error("CheckForSpell: Spell class %d does not exist.\n", SpellType);
				break;
		}
	} catch (RESULT r) {
		if (r != ERROR) {
			SpellFailed(CreatureID);
		}

		TPlayer *pl = GetPlayer(CreatureID);
		if (pl != nullptr) {
			SendResult(pl->GetConnection(), r);
		}

		return -SpellType;
	}

	return SpellType;
}

void DeleteRune(Object Obj) {
	if (!Obj.exists()) {
		error("DeleteRune: Given object does not exist.\n");
		throw ERROR;
	}

	if (Obj.getAttribute(CHARGES) <= 1) {
		Delete(Obj, -1);
	} else {
		Change(Obj, CHARGES, Obj.getAttribute(CHARGES) - 1);
	}
}

void UseMagicItem(uint32_t CreatureID, Object Obj, Object Dest) {
	TPlayer *pl = GetPlayer(CreatureID);
	if (pl == nullptr) {
		error("UseMagicItem: Creature does not exist.\n");
		throw ERROR;
	}

	if (!Obj.exists()) {
		error("UseMagicItem: Given object does not exist.\n");
		throw ERROR;
	}

	if (!Dest.exists()) {
		error("UseMagicItem: Given target does not exist (object %d).\n", Obj.getObjectType().getTypeID());
		throw ERROR;
	}

	if (CheckRight(pl->ID, NO_RUNES)) {
		throw NOTUSABLE;
	}

	int SpellNr = FindSpell(Obj);
	if (SpellNr == 0) {
		error("UseMagicItem: No spell exists for object %d.\n", Obj.getObjectType().getTypeID());
		throw ERROR;
	}

	TCreature *dest = nullptr;
	if (Dest.isCreature()) {
		dest = GetCreature(Dest);
	}

	{
		Object Help = GetFirstContainerObject(Dest.getContainer());
		while (Help != NONE) {
			if (Help.isCreature()) {
				if (IsAggressiveSpell(SpellNr) && (dest == nullptr || Help.getCreatureID() != pl->ID)) {
					Dest = Help;
					dest = GetCreature(Dest);
				}

				if (!IsAggressiveSpell(SpellNr) && (dest == nullptr || Help.getCreatureID() == pl->ID)) {
					Dest = Help;
					dest = GetCreature(Dest);
				}
			}
			Help++;
		}
	}

	CheckRuneLevel(pl, SpellNr);
	int dx, dy, dz;
	GetObjectCoordinates(Dest, dx, dy, dz);
	if (pl->EarliestSpellTime > ServerMilliseconds) {
		throw EXHAUSTED;
	}

	if (IsAggressiveSpell(SpellNr)
			&& !CheckRight(pl->ID, ATTACK_EVERYWHERE)
			&& (IsProtectionZone(pl->posx, pl->posy, pl->posz) || IsProtectionZone(dx, dy, dz))) {
		throw PROTECTIONZONE;
	}

	if (IsAggressiveSpell(SpellNr) && !Dest.getContainer().isMapContainer()) {
		throw NOROOM;
	}

	int Damage;
	try {
		switch (SpellNr) {
			case SPELL_INTENSE_HEALING_RUNE:
				if (dest == nullptr) {
					throw NOCREATURE;
				}
				Damage = ComputeDamage(pl, SpellNr, 70, 30);
				Heal(dest, -1, 0, Damage);
				break;
			case SPELL_ULTIMATE_HEALING_RUNE:
				if (dest == nullptr) {
					throw NOCREATURE;
				}
				Damage = ComputeDamage(pl, SpellNr, 250, 0);
				Heal(dest, -1, 0, Damage);
				break;
			case SPELL_LIGHT_MAGIC_MISSILE:
				Damage = ComputeDamage(pl, SpellNr, 15, 5);
				Combat(pl, Dest, 0, 0, Damage, EFFECT_EXPLOSION, ANIMATION_FIRE, DAMAGE_ENERGY);
				break;
			case SPELL_HEAVY_MAGIC_MISSILE:
				Damage = ComputeDamage(pl, SpellNr, 30, 10);
				Combat(pl, Dest, 0, 0, Damage, EFFECT_EXPLOSION, ANIMATION_FIRE, DAMAGE_ENERGY);
				break;
			case SPELL_CONVINCE_CREATURE:
				if (dest == nullptr) {
					throw NOCREATURE;
				}
				Convince(pl, dest);
				break;
			case SPELL_CHAMELEON:
				ObjectIllusion(pl, 0, 0, Dest, 200);
				break;
			case SPELL_FIREBALL:
				Damage = ComputeDamage(pl, SpellNr, 20, 5);
				MassCombat(pl, Dest, 0, 0, Damage, EFFECT_FIRE_AREA, 3, DAMAGE_FIRE, ANIMATION_FIRE);
				break;
			case SPELL_GREAT_FIREBALL:
				Damage = ComputeDamage(pl, SpellNr, 50, 15);
				MassCombat(pl, Dest, 0, 0, Damage, EFFECT_FIRE_AREA, 4, DAMAGE_FIRE, ANIMATION_FIRE);
				break;
			case SPELL_FIREBOMB:
				MassCreateField(pl, Dest, 0, 0, FIELD_FIRE, 3);
				break;
			case SPELL_EXPLOSION:
				Damage = ComputeDamage(pl, SpellNr, 60, 40);
				MassCombat(pl, Dest, 0, 0, Damage, EFFECT_EXPLOSION_AREA, 1, DAMAGE_PHYSICAL, ANIMATION_FIRE);
				break;
			case SPELL_SUDDEN_DEATH:
				Damage = ComputeDamage(pl, SpellNr, 150, 20);
				Combat(pl, Dest, 0, 0, Damage, EFFECT_DEATH, ANIMATION_DEATH, DAMAGE_PHYSICAL);
				break;
			case SPELL_FIRE_FIELD:
				CreateField(pl, Dest, 0, 0, FIELD_FIRE);
				break;
			case SPELL_POISON_FIELD:
				CreateField(pl, Dest, 0, 0, FIELD_POISON);
				break;
			case SPELL_ENERGY_FIELD:
				CreateField(pl, Dest, 0, 0, FIELD_ENERGY);
				break;
			case SPELL_FIRE_WALL:
				CreateFieldWall(pl, Dest, 0, 0, FIELD_FIRE, 2);
				break;
			case SPELL_DESTROY_FIELD:
				DeleteField(pl, Dest, 0, 0);
				break;
			case SPELL_ANTIDOTE_RUNE:
				if (dest == nullptr) {
					throw NOCREATURE;
				}
				NegatePoison(pl, 0, 0, dest);
				break;
			case SPELL_POISON_WALL:
				CreateFieldWall(pl, Dest, 0, 0, FIELD_POISON, 2);
				break;
			case SPELL_ENERGY_WALL:
				CreateFieldWall(pl, Dest, 0, 0, FIELD_ENERGY, 3);
				break;
			case SPELL_SOULFIRE:
				Damage = ComputeDamage(pl, SpellNr, 120, 20);
				Combat(pl, Dest, 0, 0, Damage, EFFECT_FIRE_HIT, ANIMATION_FIRE, DAMAGE_FIRE_CONDITION);
				break;
			case SPELL_PARALYZE:
				if (dest == nullptr) {
					throw NOCREATURE;
				}
				if (pl->GetEffectiveProfession() != PROFESSION_DRUID) {
					throw NOTUSABLE;
				}
				MagicGoStrength(pl, dest, SpellList[SpellNr].Mana, SpellList[SpellNr].SoulPoints, -101, 1);
				break;
			case SPELL_ENERGYBOMB:
				MassCreateField(pl, Dest, 0, 0, FIELD_ENERGY, 3);
				break;
			case SPELL_ENVENOM:
				Damage = ComputeDamage(pl, SpellNr, 70, 20);
				Combat(pl, Dest, 0, 0, Damage, EFFECT_POISON_HIT, ANIMATION_ENERGY, DAMAGE_POISON_CONDITION);
				break;
			case SPELL_DESINTEGRATE:
				CleanupField(pl, Dest, 0, 0);
				break;
			case SPELL_ANIMATE_DEAD:
				RaiseDead(pl, Dest, 0, 0);
				break;
			case SPELL_MAGIC_WALL:
				CreateField(pl, Dest, 0, 0, FIELD_MAGICWALL);
				break;
			case SPELL_POISONBOMB:
				MassCreateField(pl, Dest, 0, 0, FIELD_POISON, 3);
				break;
			default:
				error("UseMagicItem: Spell %d not implemented yet.\n", SpellNr);
				throw ERROR;
		}

		if (Obj.exists()) {
			DeleteRune(Obj);
		}

		if (IsAggressiveSpell(SpellNr)) {
			pl->BlockLogout(60, false);
		}
	} catch (RESULT r) {
		if (r != ERROR) {
			SpellFailed(CreatureID);
		}

		SendResult(pl->GetConnection(), r);
	}
}

void DrinkPotion(uint32_t CreatureID, Object Obj) {
	TPlayer *pl = GetPlayer(CreatureID);
	if (pl == nullptr) {
		error("DrinkPotion: Creature does not exist.\n");
		throw ERROR;
	}

	if (!Obj.exists()) {
		error("DrinkPotion: Given object does not exist.\n");
		throw ERROR;
	}

	if (!Obj.getFlag(LIQUIDCONTAINER)) {
		error("DrinkPotion: Given object is not a liquid container.\n");
		throw ERROR;
	}

	int Refresh;
	switch (Obj.getAttribute(CONTAINERLIQUIDTYPE)) {
		case LIQUID_MANAFLUID:
			Refresh = ComputeDamage(nullptr, 0, 100, 50);
			RefreshMana(pl, 0, 0, Refresh);
			break;
		case LIQUID_LIFEFLUID:
			Refresh = ComputeDamage(nullptr, 0, 50, 25);
			Heal(pl, 0, 0, Refresh);
			break;
		default:
			error("DrinkPotion: Object contains no potion.\n");
			throw ERROR;
	}

	Change(Obj, CONTAINERLIQUIDTYPE, LIQUID_NONE);
}

void InitMagic() {
	InitCircles();
	InitSpells();
	InitLog("banish");
}

static void CreateField(int x, int y, int z, int FieldType, uint32_t Owner, bool Peaceful) {
	ObjectType ObjectType;
	switch (FieldType) {
		case FIELD_FIRE:
			ObjectType = GetSpecialObject(Peaceful ? MAGICFIELD_FIRE_HARMLESS : MAGICFIELD_FIRE_DANGEROUS);
			break;
		case FIELD_POISON:
			ObjectType = GetSpecialObject(Peaceful ? MAGICFIELD_POISON_HARMLESS : MAGICFIELD_POISON_DANGEROUS);
			break;
		case FIELD_ENERGY:
			ObjectType = GetSpecialObject(Peaceful ? MAGICFIELD_ENERGY_HARMLESS : MAGICFIELD_ENERGY_DANGEROUS);
			break;
		case FIELD_MAGICWALL:
			ObjectType = GetSpecialObject(MAGICFIELD_MAGICWALL);
			break;
		case FIELD_RUSHWOOD:
			ObjectType = GetSpecialObject(MAGICFIELD_RUSHWOOD);
			break;
		default:
			error("CreateField: Invalid field type %d.\n", FieldType);
			throw ERROR;
	}

	if (!FieldPossible(x, y, z, FieldType)) {
		return;
	}

	Object Obj = GetFirstObject(x, y, z);
	while (Obj != NONE) {
		Object Help = Obj;
		Obj++;
		if (Help.getFlag(MAGICFIELD)) {
			Delete(Help, -1);
		}
	}

	try {
		Create(GetMapContainer(x, y, z), ObjectType, Owner);
	} catch (RESULT r) {
		if (r != DESTROYED) {
			throw;
		}
	}
}

// EnchantObject through HomeTeleport, CharacterRightSpell, AccountRightSpell,
// CastSpell, RuneSpell, DeleteRune, UseMagicItem, DrinkPotion and the static
// CreateField report a failure by throwing a RESULT, and InitMagic by throwing
// a const char * (from InitCircles and InitSpells). They have no exception
// specification (C++17 has none), so an exception of another type passes to
// the caller too.
