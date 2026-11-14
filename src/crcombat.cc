// crcombat.cc: TCombat (cr.hh), a creature's fight: its target and chasing,
// close and distance attacks, the attack and defense rolls, its weapons and
// ammunition, and the list of damage that shares out the experience.

#include <algorithm>
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
#include "crinc.hh"
#include "crmain.hh"
#include "magic.hh"
#include "sending.hh"
#include "utils.hh"
#include "time.hh"
#include "crcombat.hh"

// CheckTargetAndChase, Attack, SetAttackDest, CloseAttack and RangeAttack
// report a failure by throwing a RESULT. They have no exception specification
// (C++17 has none), so an exception of another type passes to the caller too.

// The size of TCombat::CombatList: the attackers whose damage a creature
// remembers for the experience they get.
const int MAX_COMBAT_ENTRIES = 20;

// A target farther away than this is lost: the attack or following stops.
const int TARGET_LOST_DISTANCE = 8;

// The values of the AMMOSPECIALEFFECT and THROWSPECIALEFFECT attributes that
// RangeAttack knows (items.toml: the poison arrow and the burst arrow).
const int SPECIALEFFECT_POISON = 1;
const int SPECIALEFFECT_BURST = 2;

// Quirk: Following is left uninitialized.
TCombat::TCombat() {
	Master = nullptr;
	AttackDest = 0;
	EarliestAttackTime = 0;
	EarliestDefendTime = 0;
	LastDefendTime = 0;
	LatestAttackTime = 0;
	for (int i = 0; i < MAX_COMBAT_ENTRIES; i++) {
		CombatList[i].ID = 0;
		CombatList[i].Damage = 0;
		CombatList[i].TimeStamp = 0;
	}
	ActCombatEntry = 0;
	CombatDamage = 0;
	LearningPoints = 0;
	Missile = NONE;
	Wand = NONE;
	Close = NONE;
	Shield = NONE;
	Throw = NONE;
	Ammo = NONE;
	Fist = false;
	AttackMode = ATTACK_MODE_BALANCED;
	ChaseMode = CHASE_MODE_STAND;
	SecureMode = SECURE_MODE_ON;
}

void TCombat::CheckTargetAndChase() {
	if (AttackDest == 0) {
		return;
	}

	TCreature *dfd = GetCreature(AttackDest);
	if (dfd == nullptr) {
		StopAttack(0);
		throw TARGETLOST;
	}

	if (Master == nullptr) {
		error("TCombat::CanAttack: No Master set!\n");
		throw ERROR;
	}

	if (Master->Type == PLAYER && dfd->Type != PLAYER && dfd->IsInvisible()) {
		StopAttack(0);
		throw TARGETLOST;
	}

	if (!Following && Master->Type == PLAYER && dfd->Type == PLAYER
			&& SecureMode == SECURE_MODE_ON && WorldType == NORMAL
			&& !((TPlayer *)Master)->IsAttackJustified(dfd->ID)) {
		StopAttack(0);
		throw SECUREMODE;
	}

	if (!Following && WorldType == NON_PVP
			&& Master->IsPeaceful() && dfd->IsPeaceful()
			&& (Master->Type != PLAYER || !CheckRight(Master->ID, ATTACK_EVERYWHERE))) {
		StopAttack(0);
		throw ATTACKNOTALLOWED;
	}

	int Distance = ObjectDistance(Master->GetCreatureObject(), dfd->GetCreatureObject());
	if (Distance > TARGET_LOST_DISTANCE) {
		StopAttack(0);
		throw TARGETLOST;
	}

	int Mode = Following ? CHASE_MODE_FOLLOW : ChaseMode;
	switch (Mode) {
		case CHASE_MODE_FOLLOW:
			if (Distance > 1) {
				Master->ToDoGo(dfd->posx, dfd->posy, dfd->posz, false, 3);
			}
			break;

		case CHASE_MODE_KEEP_DISTANCE:
			if (Distance <= 3) {
				int x, y, z;
				if (SearchFlightField(Master->ID, AttackDest, x, y, z)) {
					Master->ToDoGo(x, y, z, true, 0x7fffffff);
				}
			} else if (Distance > 4) {
				Master->ToDoGo(dfd->posx, dfd->posy, dfd->posz, false, Distance - 4);
			}
			break;
	}
}

int TCombat::GetDistance() {
	int range = 0;
	if (Close != NONE || Fist) {
		range = 1;
	} else if (Throw != NONE) {
		range = 2;
	} else if (Missile != NONE || Wand != NONE) {
		range = 3;
	}
	return range;
}

void TCombat::Attack() {
	if (AttackDest == 0 || Following) {
		return;
	}

	TCreature *dfd = GetCreature(AttackDest);
	if (dfd == nullptr) {
		StopAttack(0);
		throw TARGETLOST;
	}

	if (Master == nullptr) {
		error("TCombat::CanAttack: No Master set!\n");
		throw ERROR;
	}

	if (LatestAttackTime != 0 && LatestAttackTime < RoundNr) {
		StopAttack(0);
		return;
	}

	if (Master->Type == PLAYER && dfd->Type != PLAYER && dfd->IsInvisible()) {
		StopAttack(0);
		throw TARGETLOST;
	}

	if (Master->Type == PLAYER && dfd->Type == PLAYER
			&& SecureMode == SECURE_MODE_ON && WorldType == NORMAL
			&& !((TPlayer *)Master)->IsAttackJustified(dfd->ID)) {
		StopAttack(0);
		throw SECUREMODE;
	}

	int distance = ObjectDistance(Master->GetCreatureObject(), dfd->GetCreatureObject());
	int range = GetDistance();
	if (distance > TARGET_LOST_DISTANCE) {
		StopAttack(0);
		throw TARGETLOST;
	}

	if (Master->Type == PLAYER && dfd->Type == PLAYER
			&& ((TPlayer *)Master)->GetRealProfession() == PROFESSION_NONE
			&& !CheckRight(Master->ID, ATTACK_EVERYWHERE)) {
		StopAttack(0);
		throw ATTACKNOTALLOWED;
	}

	if (Master->Type == PLAYER && CheckRight(Master->ID, NO_ATTACK)) {
		StopAttack(0);
		throw ATTACKNOTALLOWED;
	}

	if (IsProtectionZone(Master->posx, Master->posy, Master->posz)
			|| IsProtectionZone(dfd->posx, dfd->posy, dfd->posz)) {
		StopAttack(0);
		throw PROTECTIONZONE;
	}

	int distx = abs(Master->posx - dfd->posx);
	int disty = abs(Master->posy - dfd->posy);

	Master->BlockLogout(60, dfd->Type == PLAYER);
	dfd->BlockLogout(60, false);
	if (Master->Type == PLAYER && dfd->Type == PLAYER) {
		((TPlayer *)Master)->RecordAttack(dfd->ID);
	}

	DelayAttack(200);

	switch (range) {
		case 1:
			if (distance > 1) {
				throw TARGETOUTOFRANGE;
			}
			CloseAttack(dfd);
			break;

		case 2:
		case 3:
			if (distx > 7 || disty > 5) {
				throw TARGETOUTOFRANGE;
			}
			RangeAttack(dfd);
			break;

		default:
			throw ERROR;
	}

	DelayAttack(2000);

	if (dfd->IsDead) {
		StopAttack(0);
	}
}

void TCombat::SetAttackDest(uint32_t dfd, bool Follow) {
	if (AttackDest == dfd && Following == Follow) {
		return;
	}

	if (dfd == 0 || dfd == Master->ID) {
		StopAttack(0);
		return;
	}

	TCreature *cr = GetCreature(dfd);
	if (cr == nullptr) {
		StopAttack(0);
		return;
	}

	if (!Follow) {
		if (Master->Type == PLAYER && cr->Type == PLAYER
				&& SecureMode == SECURE_MODE_ON && WorldType == NORMAL
				&& !((TPlayer *)Master)->IsAttackJustified(cr->ID)) {
			StopAttack(0);
			throw SECUREMODE;
		}

		if (Master->Type == PLAYER
				&& !CheckRight(Master->ID, ATTACK_EVERYWHERE)
				&& (IsProtectionZone(cr->posx, cr->posy, cr->posz)
					|| IsProtectionZone(Master->posx, Master->posy, Master->posz))) {
			StopAttack(0);
			throw PROTECTIONZONE;
		}

		if (Master->Type == PLAYER && CheckRight(Master->ID, NO_ATTACK)) {
			StopAttack(0);
			throw ATTACKNOTALLOWED;
		}

		if (Master->Type == PLAYER && cr->Type == PLAYER
				&& ((TPlayer *)Master)->GetRealProfession() == PROFESSION_NONE
				&& !CheckRight(Master->ID, ATTACK_EVERYWHERE)) {
			StopAttack(0);
			throw ATTACKNOTALLOWED;
		}

		if (WorldType == NON_PVP
				&& Master->IsPeaceful() && cr->IsPeaceful()
				&& (Master->Type != PLAYER || !CheckRight(Master->ID, ATTACK_EVERYWHERE))) {
			StopAttack(0);
			throw ATTACKNOTALLOWED;
		}

		if (cr->Type == NPC) {
			StopAttack(0);
			throw ATTACKNOTALLOWED;
		}
	}

	if (Master->Type == PLAYER && cr->Type != PLAYER && cr->IsInvisible()) {
		StopAttack(0);
		throw TARGETLOST;
	}

	if (ObjectDistance(Master->GetCreatureObject(), cr->GetCreatureObject()) > TARGET_LOST_DISTANCE) {
		StopAttack(0);
		throw TARGETLOST;
	}

	AttackDest = dfd;
	Following = Follow;
	if (!Follow) {
		cr->AttackStimulus(Master->ID);
		Master->BlockLogout(60, cr->Type == PLAYER);
		if (Master->Type == PLAYER && cr->Type == PLAYER) {
			((TPlayer *)Master)->RecordAttack(cr->ID);
		}
		LatestAttackTime = 0;
	}
}

void TCombat::SetAttackMode(unsigned char b) {
	if (b < ATTACK_MODE_OFFENSIVE || b > ATTACK_MODE_DEFENSIVE) {
		error("TCombat::SetAttackMode: Invalid attack mode %d.\n", b);
		return;
	}

	if (AttackMode != b) {
		DelayAttack(2000);
		AttackMode = b;
	}
}

void TCombat::SetChaseMode(unsigned char b) {
	if (b > CHASE_MODE_FOLLOW) {
		error("TCombat::SetChaseMode: Invalid chase mode %d.\n", b);
		return;
	}

	ChaseMode = b;
}

void TCombat::SetSecureMode(unsigned char b) {
	if (b > SECURE_MODE_ON) {
		error("TCombat::SetSecureMode: Invalid secure mode %d.\n", b);
		return;
	}

	SecureMode = b;
}

void TCombat::StopAttack(int Delay) {
	if (Delay == 0) {
		AttackDest = 0;
		if (Master->Type == PLAYER) {
			SendClearTarget(Master->GetConnection());
		}
	} else {
		LatestAttackTime = RoundNr + Delay;
	}
}

void TCombat::DelayAttack(int Milliseconds) {
	EarliestAttackTime = std::max<uint32_t>(EarliestAttackTime, ServerMilliseconds + Milliseconds);
}

void TCombat::CheckCombatValues() {
	Object OldMissile = Missile;
	Object OldWand = Wand;
	Object OldClose = Close;
	Object OldShield = Shield;
	Object OldThrow = Throw;
	Object OldAmmo = Ammo;
	bool OldFist = Fist;

	FindWeapons();
	FindAmmo();

	if (OldMissile != Missile || OldWand != Wand || OldClose != Close
			|| OldShield != Shield || OldThrow != Throw || OldAmmo != Ammo
			|| OldFist != Fist) {
		DelayAttack(2000);
	}
}

unsigned short TCombat::GetArmorStrength() {
	unsigned short Value = 0;
	for (int i = FIRST_BODY_POSITION; i <= LAST_BODY_POSITION; i++) {
		Object Obj = GetBodyObject(Master->ID, i);
		if (Obj.exists() && Obj.getFlag(CLOTHES)
				&& (int)Obj.getAttribute(BODYPOSITION) == i
				&& Obj.getFlag(ARMOR)) {
			Value += Obj.getAttribute(ARMORVALUE);
		}
	}

	Value += RaceData[Master->Race].Armor;
	if (Value > 1) {
		Value = Value / 2 + rand() % (Value / 2);
	}
	return Value;
}

void TCombat::AddDamageToCombatList(uint32_t atk, unsigned short Damage) {
	CombatDamage += Damage;
	for (int i = 0; i < MAX_COMBAT_ENTRIES; i++) {
		if (CombatList[i].ID == atk) {
			CombatList[i].Damage += Damage;
			CombatList[i].TimeStamp = RoundNr;
			return;
		}
	}

	CombatList[ActCombatEntry].ID = atk;
	CombatList[ActCombatEntry].Damage = Damage;
	CombatList[ActCombatEntry].TimeStamp = RoundNr;
	ActCombatEntry++;
	if (ActCombatEntry == MAX_COMBAT_ENTRIES) {
		ActCombatEntry = 0;
	}
}

void TCombat::DistributeExperiencePoints(uint32_t Exp) {
	print(3, "%s died. Distributing %ld EXP...\n", Master->Name, (long)(int32_t)Exp);
	if (CombatDamage == 0) {
		return;
	}

	for (int i = 0; i < MAX_COMBAT_ENTRIES; i++) {
		TCreature *cr = GetCreature(CombatList[i].ID);
		if (cr == nullptr || cr->IsDead) {
			continue;
		}

		int Amount = Exp * CombatList[i].Damage / CombatDamage;
		if (Master->Type == PLAYER && cr->Type == PLAYER) {
			if (((TPlayer *)Master)->GetPartyLeader(true) != 0
					&& ((TPlayer *)Master)->GetPartyLeader(true) == ((TPlayer *)cr)->GetPartyLeader(true)) {
				continue;
			}

			int LevelVictim = Master->Skills[SKILL_LEVEL]->Get();
			int LevelMurderer = cr->Skills[SKILL_LEVEL]->Get();
			if (LevelMurderer >= LevelVictim * 11 / 10) {
				continue;
			}

			Amount = (LevelVictim * 11 / 10 - LevelMurderer) * Amount / LevelVictim;
			print(3, "%s receives %d EXP.\n", cr->Name, Amount);
		}

		if (Amount != 0) {
			if (cr->Type == PLAYER && Amount >= cr->Skills[SKILL_LEVEL]->Get()) {
				int Interval = ((TPlayer *)cr)->GetActivePromotion() ? 15 : 120;
				int Cycles = 240 / Interval;
				int Rest = cr->Skills[SKILL_SOULPOINTS]->TimerValue() % Interval;
				if (Rest == 0) {
					Rest = Interval;
				}
				cr->SetTimer(SKILL_SOULPOINTS, Cycles, Rest, Interval, -1);
			}

			cr->Skills[SKILL_LEVEL]->Increase(Amount);
			TextualEffect(cr->GetCreatureObject(), COLOR_WHITE, "%d", Amount);
		}
	}
}

uint32_t TCombat::GetDamageByCreature(uint32_t CreatureID) {
	for (int i = 0; i < MAX_COMBAT_ENTRIES; i++) {
		if (CombatList[i].ID == CreatureID) {
			return CombatList[i].Damage;
		}
	}
	return 0;
}

uint32_t TCombat::GetMostDangerousAttacker() {
	uint32_t Attacker = 0;
	uint32_t MaximumDamage = 0;
	int Limitation = RoundNr - 60;
	for (int i = 0; i < MAX_COMBAT_ENTRIES; i++) {
		if (CombatList[i].TimeStamp > Limitation && CombatList[i].Damage > MaximumDamage) {
			Attacker = CombatList[i].ID;
			MaximumDamage = CombatList[i].Damage;
		}
	}
	return Attacker;
}

int TCombat::TypeToSkill(int type) {
	switch (type) {
		case WEAPON_NONE: return SKILL_FISTFIGHTING;
		case WEAPON_SWORD: return SKILL_SWORDFIGHTING;
		case WEAPON_CLUB: return SKILL_CLUBFIGHTING;
		case WEAPON_AXE: return SKILL_AXEFIGHTING;
		case WEAPON_SHIELD: return SKILL_SHIELDING;
		case WEAPON_BOW: return SKILL_DISTANCEFIGHTING;
		case WEAPON_THROW: return SKILL_DISTANCEFIGHTING;
		default: return SKILL_FISTFIGHTING;
	}
}

void TCombat::FindWeapons() {
	Missile = NONE;
	Wand = NONE;
	Close = NONE;
	Shield = NONE;
	Throw = NONE;
	Ammo = NONE;
	Fist = true;

	if (!Master->GetCreatureObject().exists()) {
		error("TCombat::FindWeapons: Creature object does not exist.\n");
		return;
	}

	for (int Hand = BODY_RIGHTHAND; Hand <= BODY_LEFTHAND; Hand++) {
		Object Obj = GetBodyObject(Master->ID, Hand);
		if (Obj == NONE) {
			continue;
		}

		if (Obj.getFlag(RESTRICTLEVEL)
				&& Master->Skills[SKILL_LEVEL]->Get() < (int)Obj.getAttribute(MINIMUMLEVEL)) {
			continue;
		}

		if (Obj.getFlag(RESTRICTPROFESSION) && Master->Type == PLAYER
				&& (Obj.getAttribute(PROFESSIONS) & (1 << ((TPlayer *)Master)->GetEffectiveProfession())) == 0) {
			continue;
		}

		if (Obj.getFlag(SHIELD)) {
			Shield = Obj;
		}

		if (Obj.getFlag(BOW)) {
			Missile = Obj;
			Fist = false;
		}

		if (Obj.getFlag(WAND)) {
			Wand = Obj;
			Fist = false;
		}

		if (Obj.getFlag(THROW)) {
			Throw = Obj;
			Fist = false;
		}

		if (Obj.getFlag(WEAPON)) {
			Close = Obj;
			Fist = false;
		}
	}
}

void TCombat::FindAmmo() {
	if (Missile != NONE) {
		Ammo = GetBodyObject(Master->ID, BODY_AMMO);
		if (Ammo != NONE && (!Ammo.getFlag(AMMO)
				|| Ammo.getAttribute(AMMOTYPE) != Missile.getAttribute(BOWAMMOTYPE))) {
			Ammo = NONE;
		}
	} else if (Throw != NONE) {
		Ammo = Throw;
	} else if (Wand != NONE) {
		Ammo = Wand;
	}
}

void TCombat::CloseAttack(TCreature *dfd) {
	int Attack = RollAttack();
	int Defend = dfd->Combat.RollDefense();
	int Damage = Attack - Defend;
	if (Damage < 0) {
		Damage = 0;
	}

	Damage = dfd->Damage(Master, Damage, DAMAGE_PHYSICAL);
	if (Damage > 0) {
		ActivateLearning();
	}

	if (GetRacePoison(Master->Race) != 0
			&& (Damage > 0 || (Attack > Defend && rand() % 5 == 0))) {
		int Poison = random(GetRacePoison(Master->Race) / 2, GetRacePoison(Master->Race));
		if (Poison > 0) {
			dfd->Damage(Master, Poison, DAMAGE_POISON_CONDITION);
			if (dfd->Type == PLAYER) {
				SendMessage(dfd->GetConnection(), TALK_STATUS_MESSAGE, "You are poisoned.");
			}
		}
	}

	if (Close != NONE && Close.getFlag(WEAROUT)) {
		if (Close.getAttribute(REMAININGUSES) > 1) {
			Change(Close, REMAININGUSES, Close.getAttribute(REMAININGUSES) - 1);
		} else {
			Change(Close, ObjectType((int)Close.getAttribute(WEAROUTTARGET)), 0);
			CheckCombatValues();
		}
	}
}

void TCombat::RangeAttack(TCreature *dfd) {
	int x, y, z;
	int Attack, Damage, DiffLvl;
	Object Drop;
	int DamageType, AnimType, Fragility, SpecialEffect, EffectStrength;
	bool Hit;

	if (Wand != NONE) {
		Ammo = Wand;
		DiffLvl = 0;
		if (std::max(abs(Master->posx - dfd->posx), abs(Master->posy - dfd->posy)) > (int)Wand.getAttribute(WANDRANGE)) {
			throw TARGETOUTOFRANGE;
		}

		DamageType = Wand.getAttribute(WANDDAMAGETYPE);
		AnimType = Wand.getAttribute(WANDMISSILE);
		Fragility = 0;
		SpecialEffect = 0;
		EffectStrength = 0;
	} else if (Missile != NONE) {
		DiffLvl = 90;
		Ammo = GetBodyObject(Master->ID, BODY_AMMO);
		if (Ammo != NONE && (!Ammo.getFlag(AMMO)
				|| Ammo.getAttribute(AMMOTYPE) != Missile.getAttribute(BOWAMMOTYPE))) {
			Ammo = NONE;
		}

		if (!Ammo.exists()) {
			throw OUTOFAMMO;
		}

		if (std::max(abs(Master->posx - dfd->posx), abs(Master->posy - dfd->posy)) > (int)Missile.getAttribute(BOWRANGE)) {
			throw TARGETOUTOFRANGE;
		}

		DamageType = DAMAGE_PHYSICAL;
		AnimType = Ammo.getAttribute(AMMOMISSILE);
		Fragility = 100;
		SpecialEffect = Ammo.getAttribute(AMMOSPECIALEFFECT);
		EffectStrength = Ammo.getAttribute(AMMOEFFECTSTRENGTH);
	} else if (Throw != NONE) {
		DiffLvl = 75;
		Ammo = Throw;
		if (std::max(abs(Master->posx - dfd->posx), abs(Master->posy - dfd->posy)) > (int)Throw.getAttribute(THROWRANGE)) {
			throw TARGETOUTOFRANGE;
		}

		DamageType = DAMAGE_PHYSICAL;
		AnimType = Ammo.getAttribute(THROWMISSILE);
		Fragility = Throw.getAttribute(THROWFRAGILITY);
		SpecialEffect = Throw.getAttribute(THROWSPECIALEFFECT);
		EffectStrength = Throw.getAttribute(THROWEFFECTSTRENGTH);
	} else {
		throw ERROR;
	}

	GetObjectCoordinates(dfd->GetCreatureObject(), x, y, z);
	if (!ThrowPossible(Master->posx, Master->posy, Master->posz, x, y, z, 0)) {
		throw TARGETHIDDEN;
	}

	if (Wand != NONE) {
		try {
			SpendManaAndSoulPoints(Master, Wand.getAttribute(WANDMANACONSUMPTION), 0, 0);
		} catch (RESULT r) {
			if (r == NOTENOUGHMANA) {
				throw OUTOFAMMO;
			}
			throw;
		}
		Hit = true;
	} else {
		int Diff = std::max(abs(Master->posx - dfd->posx), abs(Master->posy - dfd->posy));
		if (Diff <= 1) {
			Diff = 5;
		}

		Hit = Master->Skills[SKILL_DISTANCEFIGHTING]->Probe(Diff * 15, DiffLvl, LearningPoints > 0);
		if (LearningPoints > 0) {
			LearningPoints--;
		}
	}

	if (Wand != NONE) {
		Attack = Wand.getAttribute(WANDATTACKSTRENGTH);
		Attack += random(-(int)Wand.getAttribute(WANDATTACKVARIATION), Wand.getAttribute(WANDATTACKVARIATION));
		Damage = dfd->Damage(Master, Attack, DamageType);
		if (Damage > 0) {
			ActivateLearning();
		}
		Drop = GetMapContainer(dfd->GetCreatureObject());
	} else if (Hit) {
		Drop = GetMapContainer(dfd->GetCreatureObject());
		GetObjectCoordinates(Drop, x, y, z);
		Attack = RollAttack();
		// Quirk: this tests the attacker's own shield, not the defender's, and
		// discards the defend value; only the side effects of the defender's
		// RollDefense count (its defense timer, skill learning and shield
		// wear).
		if (Shield != NONE) {
			dfd->Combat.RollDefense();
		}

		Damage = dfd->Damage(Master, Attack, DamageType);
		if (Damage > 0) {
			ActivateLearning();
		}
	} else {
		GetObjectCoordinates(dfd->GetCreatureObject(), x, y, z);
		if (abs(Master->posx - dfd->posx) > 1 || abs(Master->posy - dfd->posy) > 1) {
			x += rand() % 3 - 1;
			y += rand() % 3 - 1;
		}

		if (!CoordinateLayable(x, y, z)
				|| !ThrowPossible(Master->posx, Master->posy, Master->posz, x, y, z, 0)) {
			x = dfd->posx;
			y = dfd->posy;
			z = dfd->posz;
		}

		Drop = GetMapContainer(x, y, z);
	}

	::Missile(Master->GetCreatureObject(), Drop, AnimType);

	if (SpecialEffect == SPECIALEFFECT_POISON) {
		if (Hit) {
			dfd->Damage(Master, EffectStrength, DAMAGE_POISON_CONDITION);
		}
	} else if (SpecialEffect == SPECIALEFFECT_BURST) {
		TDamageImpact Impact(Master, DAMAGE_PHYSICAL, ComputeDamage(Master, 0, EffectStrength, EffectStrength), false);
		GetObjectCoordinates(Drop, x, y, z);
		CircleShapeSpell(Master, x, y, z, 0x7fffffff, ANIMATION_NONE, 2, &Impact, EFFECT_FIRE_AREA);
	}

	if (Wand == NONE) {
		try {
			if (random(0, 99) < Fragility) {
				Delete(Ammo, 1);
			} else {
				Move(0, Ammo, Drop, 1, false, NONE);
			}
		} catch (RESULT r) {
			if (r != DESTROYED) {
				error("TCombat::RangeAttack: Could not move/delete ammo (Exception %d, [%d,%d,%d].\n", r, x, y, z);
			}
		}

		if (!Hit) {
			GraphicalEffect(x, y, z, EFFECT_POFF);
		}
	}
}

void TCombat::GetAttackValue(short &Value, unsigned short &Skill) {
	unsigned short Type;
	Value = 0;
	if (Close != NONE) {
		Type = Close.getAttribute(WEAPONTYPE);
		Value = Close.getAttribute(WEAPONATTACKVALUE);
	} else if (Throw != NONE) {
		Type = WEAPON_THROW;
		Value = Throw.getAttribute(THROWATTACKVALUE);
	} else if (Wand != NONE) {
		Type = WEAPON_NONE;
		Value = 0;
	} else if (Missile != NONE) {
		Type = WEAPON_BOW;
		Value = Ammo.getAttribute(AMMOATTACKVALUE);
	} else {
		Type = WEAPON_NONE;
		Value = RaceData[Master->Race].Attack;
	}

	Skill = TypeToSkill(Type);
}

void TCombat::GetDefendValue(short &Value, unsigned short &Skill) {
	unsigned short Type;
	Value = 0;
	if (Shield != NONE) {
		Type = WEAPON_SHIELD;
		Value = Shield.getAttribute(SHIELDDEFENDVALUE);
	} else if (Close != NONE) {
		Type = Close.getAttribute(WEAPONTYPE);
		Value = Close.getAttribute(WEAPONDEFENDVALUE);
	} else if (Throw != NONE) {
		Type = WEAPON_THROW;
		Value = Throw.getAttribute(THROWDEFENDVALUE);
	} else if (Missile != NONE) {
		Type = WEAPON_BOW;
		Value = 0;
	} else {
		Type = WEAPON_NONE;
		Value = RaceData[Master->Race].Defend;
	}

	Skill = TypeToSkill(Type);
}

short TCombat::RollAttack() {
	TSkill *sk;
	short Value;
	unsigned short Type;

	GetAttackValue(Value, Type);
	sk = Master->Skills[Type];
	if (AttackMode == ATTACK_MODE_OFFENSIVE) {
		Value += Value * 2 / 10;
	} else if (AttackMode == ATTACK_MODE_DEFENSIVE) {
		Value -= Value * 4 / 10;
	}

	Value = sk->ProbeValue(Value, LearningPoints > 0);
	if (LearningPoints > 0) {
		LearningPoints--;
	}
	return Value;
}

short TCombat::RollDefense() {
	TSkill *sk;
	short Value;
	unsigned short Type;
	unsigned char Tac;

	if (!DefendReady()) {
		return 0;
	}

	EarliestDefendTime = LastDefendTime + 2000;
	LastDefendTime = ServerMilliseconds;

	GetDefendValue(Value, Type);
	sk = Master->Skills[Type];

	Tac = AttackMode;
	if ((Following || AttackDest == 0) && EarliestAttackTime <= ServerMilliseconds) {
		Tac = ATTACK_MODE_DEFENSIVE;
	}

	if (Tac == ATTACK_MODE_OFFENSIVE) {
		Value -= Value * 4 / 10;
	} else if (Tac == ATTACK_MODE_DEFENSIVE) {

		Value += Value * 8 / 10;
	}

	if (Type != 0) {
		Value = sk->ProbeValue(Value, Shield != NONE && LearningPoints > 0);
		if (Shield != NONE && LearningPoints > 0) {
			LearningPoints--;
		}

		if (Shield != NONE && Shield.getFlag(WEAROUT)) {
			if (Shield.getAttribute(REMAININGUSES) > 1) {
				Change(Shield, REMAININGUSES, Shield.getAttribute(REMAININGUSES) - 1);
			} else {
				Change(Shield, ObjectType((int)Shield.getAttribute(WEAROUTTARGET)), 0);
				if (!Shield.exists() || !Shield.getFlag(SHIELD)) {
					CheckCombatValues();
				}
			}
		}
	}

	return Value;
}

void TCombat::ActivateLearning() {
	LearningPoints = 30;
}
