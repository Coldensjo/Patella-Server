// crmain.cc: the creatures as a whole. The hash list by ID, the creature list
// each round goes through (ProcessSkills, ProcessCreatures), the to-do queue
// (MoveCreatures) and the chains of creatures per map block, which
// TFindCreatures searches; TCreature's construction and destruction, ID,
// logout, damage and death; the kill statistics; the races (monsters.toml) and
// the monster raids (raids.toml) of DATAPATH. InitCr sets it all up, ExitCr
// takes it down.

#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <map>
#include <string>
#include <time.h>
#include <vector>

#include "script.hh"
#include "config.hh"
#include "threads.hh"
#include "dbfuncs.hh"
#include "objects.hh"
#include "map.hh"
#include "info.hh"
#include "cr.hh"
#include "containr.hh"
#include "crinc.hh"
#include "crskill.hh"
#include "crplayer.hh"
#include "crnonpl.hh"
#include "operate.hh"
#include "connections.hh"
#include "sending.hh"
#include "strings.hh"
#include "writer.hh"
#include "shm.hh"
#include "utils.hh"
#include "time.hh"
#include "communication.hh"
#include "TXTEASymmetricKey.hh"
#include "monsterdata.hh"
#include "scripting.hh"
#include "crmain.hh"

// The width and height in fields of a block of FirstChainCreature, which
// chains the creatures in each block (two blocks per sector side, see InitCr).
const int CHAIN_BLOCK_SIZE = 16;

// TCreature::MovePossible may throw RESULT, and LoadRaces, LoadMonsterRaids
// and InitCr may throw const char *. None of them has an exception
// specification, so an exception of another type passes to the caller as
// well.

TCreature *HashList[HASH_LIST_SIZE];
vector<TCreature *> CreatureList(0, 10000, 1000, nullptr);
int FirstFreeCreature;
TRaceData RaceData[MAX_RACES];
uint32_t NextCreatureID;
priority_queue<uint32_t, uint32_t> ToDoQueue(5000, 1000);
priority_queue<uint32_t, TAttackWave *> AttackWaveQueue(100, 100);
matrix<uint32_t> *FirstChainCreature;
int KilledCreatures[MAX_RACES];
int KilledPlayers[MAX_RACES];

TCreature *GetCreature(uint32_t id) {
	if (id == 0) {
		return nullptr;
	}

	uint32_t m = id % HASH_LIST_SIZE;
	TCreature *cr = HashList[m];
	while (cr != nullptr && cr->ID != id) {
		cr = cr->NextHashEntry;
	}

	return cr;
}

TCreature *GetCreature(Object Obj) {
	return GetCreature(Obj.getCreatureID());
}

bool IsCreaturePlayer(uint32_t CreatureID) {
	return CreatureID < FIRST_NONPLAYER_CREATURE_ID;
}

void ProcessSkills() {
	for (int i = 0; i < FirstFreeCreature; i++) {
		TCreature *cr = CreatureList(i);
		if (cr == nullptr) {
			error("ProcessSkills: Creature %d does not exist.\n", i);
			continue;
		}

		if (cr->TimerActive()) {
			cr->ProcessSkills();
		}
	}
}

void ProcessCreatures() {
	for (int i = 0; i < FirstFreeCreature; i++) {
		TCreature *cr = CreatureList(i);
		if (cr == nullptr) {
			error("ProcessCreatures: Creature %d does not exist.\n", i);
			continue;
		}

		if (cr->Skills[SKILL_EATING]->Get() > 0
				&& RoundNr % cr->Skills[SKILL_EATING]->Get() == 0
				&& !cr->IsDead
				&& !IsProtectionZone(cr->posx, cr->posy, cr->posz)) {
			cr->Skills[SKILL_HITPOINTS]->Change(1);
			cr->Skills[SKILL_MANA]->Change(4);
			if (cr->Type == PLAYER) {
				SendPlayerData(cr->GetConnection());
			}
		}

		if (cr->Type == PLAYER && cr->GetConnection() != nullptr) {
			((TPlayer *)cr)->CheckState();
		}

		if (cr->Type == PLAYER && cr->EarliestLogoutRound != 0 && cr->EarliestLogoutRound <= RoundNr) {
			((TPlayer *)cr)->ClearPlayerkillingMarks();
			cr->EarliestLogoutRound = 0;
		}

		if (!cr->IsDead && cr->Skills[SKILL_HITPOINTS]->Get() <= 0) {
			error("ProcessCreatures: Creature %s is not dead, although it has no HP left.\n", cr->Name);
			cr->Death();
		}

		if (cr->LoggingOut && cr->LogoutPossible() == LOGOUT_POSSIBLE) {
			if (cr->IsDead && cr->Skills[SKILL_HITPOINTS]->Get() > 0) {
				error("ProcessCreatures: Creature %s has HP, although it is dead.\n", cr->Name);
				cr->Skills[SKILL_HITPOINTS]->Set(0);
			}

			if (cr->Type == PLAYER) {
				EventLogout(cr->ID);
			}

			// The destructor moves the last entry of CreatureList to this
			// index, so the index is processed again.
			delete cr;
			i--;
		}
	}
}

void MoveCreatures(int Delay) {
	ServerMilliseconds += Delay;

	while (true) {
		uint32_t ExecutionTime;
		uint32_t CreatureID;
		bool Found = ToDoQueue.getMin(ExecutionTime, CreatureID);
		if (!Found || ExecutionTime > ServerMilliseconds) {
			break;
		}

		ToDoQueue.deleteMin();
		TCreature *cr = GetCreature(CreatureID);
		if (cr != nullptr) {
			cr->Execute();
		}
	}
}

void DeleteChainCreature(TCreature *cr) {
	if (cr == nullptr) {
		error("DeleteChainCreature: Given creature does not exist.\n");
		return;
	}

	int blockx = cr->posx / CHAIN_BLOCK_SIZE;
	int blocky = cr->posy / CHAIN_BLOCK_SIZE;
	uint32_t ID = (*FirstChainCreature)(blockx, blocky);
	if (ID == cr->ID) {
		(*FirstChainCreature)(blockx, blocky) = cr->NextChainCreature;
		return;
	}

	while (true) {
		if (ID == 0) {
			error("DeleteChainCreature: Creature not found.\n");
			return;
		}

		TCreature *Help = GetCreature(ID);
		if (Help == nullptr) {
			error("DeleteChainCreature: Creature does not exist.\n");
			return;
		}

		ID = Help->NextChainCreature;
		if (ID == cr->ID) {
			Help->NextChainCreature = cr->NextChainCreature;
			return;
		}
	}
}

void InsertChainCreature(TCreature *cr, int newx, int newy) {
	if (cr == nullptr) {
		// Quirk: the message names DeleteChainCreature.
		error("InsertChainCreature: Given creature does not exist.\n");
		return;
	}

	if (newx == 0) {
		newx = cr->posx;
	}
	if (newy == 0) {
		newy = cr->posy;
	}

	int blockx = newx / CHAIN_BLOCK_SIZE;
	int blocky = newy / CHAIN_BLOCK_SIZE;
	cr->NextChainCreature = (*FirstChainCreature)(blockx, blocky);
	(*FirstChainCreature)(blockx, blocky) = cr->ID;
}

void MoveChainCreature(TCreature *cr, int newx, int newy) {
	if (cr == nullptr) {
		// Quirk: the message names DeleteChainCreature.
		error("MoveChainCreature: Given creature does not exist.\n");
		return;
	}

	if (cr->posx / CHAIN_BLOCK_SIZE == newx / CHAIN_BLOCK_SIZE && cr->posy / CHAIN_BLOCK_SIZE == newy / CHAIN_BLOCK_SIZE) {
		return;
	}

	DeleteChainCreature(cr);
	InsertChainCreature(cr, newx, newy);
}

TFindCreatures::TFindCreatures(int radx, int rady, int x, int y, int Mask) {
	initSearch(radx, rady, x, y, Mask);
}

// If the creature does not exist, only finished is set.
TFindCreatures::TFindCreatures(int radx, int rady, uint32_t CreatureID, int Mask) {
	TCreature *cr = GetCreature(CreatureID);
	if (cr == nullptr) {
		error("TFindCreatures::TFindCreatures: Creature does not exist.\n");
		finished = true;
		return;
	}

	initSearch(radx, rady, cr->posx, cr->posy, Mask);
	SkipID = CreatureID;
}

// If the object does not exist, only finished is set.
TFindCreatures::TFindCreatures(int radx, int rady, Object Obj, int Mask) {
	if (!Obj.exists()) {
		error("TFindCreatures::TFindCreatures: Given object does not exist.\n");
		finished = true;
		return;
	}

	int x, y, z;
	GetObjectCoordinates(Obj, x, y, z);
	initSearch(radx, rady, x, y, Mask);
}

void TFindCreatures::initSearch(int radx, int rady, int x, int y, int mask) {
	startx = x - radx;
	starty = y - rady;
	endx = x + radx;
	endy = y + rady;
	blockx = startx / CHAIN_BLOCK_SIZE - 1;
	blocky = starty / CHAIN_BLOCK_SIZE;
	ActID = 0;
	SkipID = 0;
	Mask = mask;
	finished = false;
}

uint32_t TFindCreatures::getNext() {
	if (finished) {
		return 0;
	}

again:
	while (ActID == 0) {
		blockx++;
		if (blockx > endx / CHAIN_BLOCK_SIZE) {
			blockx = startx / CHAIN_BLOCK_SIZE;
			blocky++;
			if (blocky > endy / CHAIN_BLOCK_SIZE) {
				finished = true;
				return 0;
			}
		}

		if (blockx < SectorXMin * 2 || blockx > SectorXMax * 2 + 1
				|| blocky < SectorYMin * 2 || blocky > SectorYMax * 2 + 1) {
			ActID = 0;
		} else {
			ActID = (*FirstChainCreature)(blockx, blocky);
		}
	}

	TCreature *cr = GetCreature(ActID);
	if (cr == nullptr) {
		error("TFindCreatures::getNext: Creature does not exist.\n");
		ActID = 0;
		goto again;
	}

	ActID = cr->NextChainCreature;
	if (cr->ID == SkipID) {
		goto again;
	}
	if (!in(cr->posx, startx, endx)) {
		goto again;
	}
	if (!in(cr->posy, starty, endy)) {
		goto again;
	}
	if (cr->Type == PLAYER && (Mask & FIND_PLAYERS) == 0) {
		goto again;
	}
	if (cr->Type == NPC && (Mask & FIND_NPCS) == 0) {
		goto again;
	}
	if (cr->Type == MONSTER && (Mask & FIND_MONSTERS) == 0) {
		goto again;
	}

	return cr->ID;
}

void InitKillStatistics() {
	for (int i = 0; i < MAX_RACES; i++) {
		KilledCreatures[i] = 0;
		KilledPlayers[i] = 0;
	}
}

void WriteKillStatistics() {
	int NumberOfRaces = 0;
	char *Names = new char[MAX_RACES * 30];
	int *Players = new int[MAX_RACES];
	int *Creatures = new int[MAX_RACES];

	for (int i = 0; i < MAX_RACES; i++) {
		if (KilledCreatures[i] == 0 && KilledPlayers[i] == 0) {
			continue;
		}

		if (i > 0) {
			sprintf(Names + NumberOfRaces * 30, "%s %s", RaceData[i].Article, RaceData[i].Name);
			// Quirk: the result is not copied back, so the name stays in the
			// singular and without capitals.
			Capitals(Plural(Names + NumberOfRaces * 30, 0));
		} else {
			strcpy(Names + NumberOfRaces * 30, "(fire/poison/energy)");
		}

		Players[NumberOfRaces] = KilledPlayers[i];
		Creatures[NumberOfRaces] = KilledCreatures[i];
		NumberOfRaces++;
	}

	KillStatisticsOrder(NumberOfRaces, Names, Players, Creatures);
	InitKillStatistics();
}

void AddKillStatistics(int AttackerRace, int DefenderRace) {
	if (strcmp(RaceData[AttackerRace].Name, "human") == 0) {
		KilledCreatures[DefenderRace]++;
	}

	if (strcmp(RaceData[DefenderRace].Name, "human") == 0) {
		KilledPlayers[AttackerRace]++;
	}
}

void ExitKillStatistics() {
	WriteKillStatistics();
}

// Name, OrgOutfit, Outfit, Sex, Race, Type and Profession are not
// initialized here.
TCreature::TCreature() : ToDoList(0, 20, 10) {
	Combat.SetMaster(this);
	ID = 0;
	NextHashEntry = nullptr;
	NextChainCreature = 0;
	startx = 0;
	starty = 0;
	startz = 0;
	posx = 0;
	posy = 0;
	posz = 0;
	Direction = DIRECTION_NORTH;
	Radius = 0x7fffffff;
	IsDead = false;
	LoseInventory = LOSE_INVENTORY_ALL;
	LoggingOut = false;
	LogoutAllowed = false;
	EarliestLogoutRound = 0;
	EarliestProtectionZoneRound = 0;
	EarliestYellRound = 0;
	EarliestTradeChannelRound = 0;
	EarliestSpellTime = 0;
	EarliestMultiuseTime = 0;
	EarliestWalkTime = 0;
	LifeEndRound = 0;
	ActToDo = 0;
	NrToDo = 0;
	NextWakeup = 0;
	Stop = false;
	LockToDo = false;
	Connection = nullptr;

	for (int i = 0; i < MAX_SKILLS; i++) {
		NewSkill(i, this);
	}

	Murderer[0] = 0;
	FirstKnowingConnection = nullptr;
	SummonedCreatures = 0;
	FireDamageOrigin = 0;
	PoisonDamageOrigin = 0;
	EnergyDamageOrigin = 0;
}

TCreature::~TCreature() noexcept(false) {
	if (IsDead) {
		try {
			if (RaceData[Race].Blood == BT_BLOOD) {
				CreatePool(GetMapContainer(GetCreatureObject()), GetSpecialObject(BLOOD_POOL), LIQUID_BLOOD);
			}

			if (RaceData[Race].Blood == BT_SLIME) {
				CreatePool(GetMapContainer(GetCreatureObject()), GetSpecialObject(BLOOD_POOL), LIQUID_SLIME);
			}
		} catch (RESULT r) {
			if (r != NOROOM && r != DESTROYED) {
				error("TCreature::~TCreature: Cannot place pool of blood (Exc %d, Pos [%d,%d,%d]).\n", r, posx, posy, posz);
			}
		}

		try {
			ObjectType CorpseType = (Sex == SEX_MALE) ? RaceData[Race].MaleCorpse : RaceData[Race].FemaleCorpse;
			if (CorpseType.getFlag(MAGICFIELD)) {
				Object Obj = GetFirstObject(posx, posy, posz);
				while (Obj != NONE) {
					Object Help = Obj;
					Obj++;
					if (Help.getFlag(MAGICFIELD)) {
						try {
							Delete(Help, -1);
						} catch (RESULT r) {
							error("TCreature::~TCreature: Exception %d while clearing a field.\n", r);
						}
					}
				}
			}

			Object Corpse = Create(GetMapContainer(posx, posy, posz), CorpseType, 0);
			Log("game", "Death of %s: LoseInventory=%d.\n", Name, LoseInventory);

			if (Type == PLAYER) {
				char s[128];
				sprintf(s, "You recognize %s", Name);
				if (Murderer[0] != 0) {
					if (Sex == SEX_MALE) {
						strcat(s, ". He ");
					} else {
						strcat(s, ". She ");
					}
					strcat(s, "was killed by ");
					strcat(s, Murderer);
				}
				Change(Corpse, TEXTSTRING, AddDynamicString(s));
			}

			if (LoseInventory != LOSE_INVENTORY_NONE) {
				for (int i = FIRST_BODY_POSITION; i <= LAST_BODY_POSITION; i++) {
					Object Obj = GetBodyObject(ID, i);
					if (Obj == NONE) {
						continue;
					}

					if (LoseInventory == LOSE_INVENTORY_ALL || Obj.getFlag(CONTAINER) || random(0, 9) == 0) {
						::Move(0, Obj, Corpse, -1, false, NONE);
					}
				}
			}

			if (Type == PLAYER && LoseInventory != LOSE_INVENTORY_ALL) {
				TPlayer *pl = (TPlayer *)this;
				pl->SaveInventory();
			}
		} catch (RESULT r) {
			error("TCreature::~TCreature: Cannot drop corpse/inventory (Exc %d, Pos [%d,%d,%d], %s).\n", r, posx, posy, posz, Name);
		}
	}

	if (CrObject != NONE && CrObject.exists()) {
		DelOnMap();
	}

	ToDoClear();

	if (Type == PLAYER && GetConnection() != nullptr) {
		GetConnection()->Logout(30, true);
	}

	DelInCrList();

	if (ID != 0) {
		DelID();
	}

	TKnownCreature *KnownCreature = FirstKnowingConnection;
	while (KnownCreature != nullptr) {
		if (KnownCreature->CreatureID != ID) {
			error("TCreature::~TCreature: Chaining error at creature %lu.\n", (unsigned long)ID);
		}
		KnownCreature->State = KNOWNCREATURE_FREE;
		KnownCreature = KnownCreature->Next;
	}
}

void TCreature::SetID(uint32_t CharacterID) {
	if (ID != 0) {
		error("TCreature::SetID: ID is already set.\n");
	}

	if (CharacterID == 0) {
		ID = NextCreatureID;
		NextCreatureID++;

		int test = 0;
		while (GetCreature(ID) != nullptr) {
			test++;
			error("TCreature::SetID: Duplicate nonplayer ID %d found.\n", ID);
			ID = NextCreatureID;
			NextCreatureID++;
			if (test > 16) {
				error("TCreature::SetID: Duplicate ID 16 times in a row. Using duplicate ID %d now\n", ID);
				break;
			}
		}
	} else {
		ID = CharacterID;
		if (GetCreature(CharacterID) != nullptr) {
			error("TCreature::SetID: Duplicate character ID %d found.\n", ID);
		}
	}

	uint32_t m = ID % HASH_LIST_SIZE;
	NextHashEntry = HashList[m];
	HashList[m] = this;
}

// The entry is found by its ID, and its predecessor in the hash chain is
// linked to its successor.
void TCreature::DelID() {
	uint32_t m = ID % HASH_LIST_SIZE;
	TCreature *cr = HashList[m];
	TCreature *PreviousEntry = nullptr;
	if (cr == nullptr) {
		error("TCreature::DelID: Hash entry not found id = %d\n", ID);
		return;
	}

	if (cr->ID == ID) {
		HashList[m] = cr->NextHashEntry;
		return;
	}

	while (cr->NextHashEntry != nullptr) {
		PreviousEntry = cr;
		cr = cr->NextHashEntry;
		if (cr->ID == ID) {
			break;
		}
	}

	if (cr->ID != ID) {
		error("TCreature::DelID: id=%d not found.\n", ID);
		return;
	}

	PreviousEntry->NextHashEntry = cr->NextHashEntry;
}

void TCreature::SetInCrList() {
	CreatureList(FirstFreeCreature) = this;
	FirstFreeCreature++;
}

void TCreature::DelInCrList() {
	for (int i = 0; i < FirstFreeCreature; i++) {
		if (CreatureList(i) == this) {
			CreatureList(i) = CreatureList(FirstFreeCreature - 1);
			CreatureList(FirstFreeCreature - 1) = nullptr;
			FirstFreeCreature--;
		}
	}
}

void TCreature::StartLogout(bool Force, bool StopFight) {
	LoggingOut = true;
	if (Force || LagDetected()) {
		LogoutAllowed = true;
	}

	if (Type == PLAYER && GetConnection() != nullptr) {
		GetConnection()->Logout(0, true);
	}

	Combat.StopAttack(StopFight ? 0 : 60);
}

int TCreature::LogoutPossible() {
	if (LogoutAllowed || GameEnding() || IsDead) {
		return LOGOUT_POSSIBLE;
	}

	if (EarliestLogoutRound > RoundNr && !LagDetected()) {
		return LOGOUT_AFTER_FIGHT;
	}

	if (IsNoLogoutField(posx, posy, posz)) {
		return LOGOUT_NO_LOGOUT_FIELD;
	}

	LogoutAllowed = true;
	return LOGOUT_POSSIBLE;
}

void TCreature::BlockLogout(int Delay, bool BlockProtectionZone) {
	if (WorldType == NON_PVP) {
		BlockProtectionZone = false;
	}

	if (Type != PLAYER || CheckRight(ID, NO_LOGOUT_BLOCK)) {
		return;
	}

	if (!BlockProtectionZone && EarliestProtectionZoneRound < RoundNr
			&& Type == PLAYER && Connection == nullptr) {
		return;
	}

	uint32_t Round = RoundNr + Delay;
	if ((BlockProtectionZone || EarliestProtectionZoneRound > RoundNr) && Round > EarliestProtectionZoneRound) {
		EarliestProtectionZoneRound = Round;
	}

	if (Round > EarliestLogoutRound) {
		EarliestLogoutRound = Round;
	}

	((TPlayer *)this)->CheckState();
}

int TCreature::GetHealth() {
	TSkill *sk = Skills[SKILL_HITPOINTS];
	int MaxHitpoints = sk->GetMax();
	if (MaxHitpoints <= 0) {
		if (!IsDead) {
			error("TCreature::GetHealth: MaxHitpoints of %s is %d, although it is not dead.\n", Name, MaxHitpoints);
		}
		return 0;
	}

	int Health = sk->Get() * 100 / MaxHitpoints;
	if (Health <= 0) {
		Health = (sk->Get() != 0) ? 1 : 0;
	}

	return Health;
}

int TCreature::GetSpeed() {
	TSkill *sk = Skills[SKILL_GOSTRENGTH];
	if (sk == nullptr) {
		error("TCreature::GetSpeed: No skill GOSTRENGTH.\n");
		return 0;
	}

	return sk->Get() * 2 + 80;
}

void TCreature::Death() {
	IsDead = true;
	LoggingOut = true;
}

int TCreature::Damage(TCreature *atk, int Damage, int DamageType) {
	if (IsDead || Type == NPC) {
		return 0;
	}

	if (Type == PLAYER && GetConnection() != nullptr && atk != nullptr) {
		SendMarkCreature(GetConnection(), atk->ID, COLOR_BLACK);
	}

	if (Type == PLAYER && atk != nullptr && atk->Type == PLAYER
			&& DamageType != DAMAGE_POISON_CONDITION && DamageType != DAMAGE_FIRE_CONDITION
			&& DamageType != DAMAGE_ENERGY_CONDITION) {
		Damage = (Damage + 1) / 2;
	}

	TCreature *Responsible = atk;
	if (atk != nullptr && atk->GetMaster() != 0) {
		Responsible = GetCreature(atk->GetMaster());
	}

	uint32_t atkID = 0;
	if (atk != nullptr) {
		atkID = atk->ID;
	}

	if (atk != nullptr) {
		atk->BlockLogout(60, Type == PLAYER);
	}

	if (Responsible != nullptr) {
		Responsible->BlockLogout(60, Type == PLAYER);
		if (Responsible->Type == PLAYER && Type == PLAYER) {
			((TPlayer *)Responsible)->RecordAttack(ID);
		}
	}

	if (Damage <= 0 || (Type == PLAYER && CheckRight(ID, INVULNERABLE))) {
		GraphicalEffect(GetCreatureObject(), EFFECT_POFF);
		return 0;
	}

	if (Type == PLAYER) {
		for (int b = FIRST_BODY_POSITION; b <= LAST_BODY_POSITION && Damage > 0; b++) {
			Object Obj = GetBodyObject(ID, b);
			if (Obj == NONE || !Obj.getFlag(PROTECTION)) {
				continue;
			}

			if (!Obj.getFlag(CLOTHES)) {
				error("TCreature::Damage: Object %d has PROTECTION, but not CLOTHES.\n", Obj.getObjectType().getTypeID());
				continue;
			}

			if (Obj.getAttribute(BODYPOSITION) != (uint32_t)b) {
				continue;
			}

			if ((Obj.getAttribute(PROTECTIONDAMAGETYPES) & DamageType) == 0) {
				continue;
			}

			// The product is calculated as unsigned.
			Damage = (int)((uint32_t)Damage * (100 - Obj.getAttribute(DAMAGEREDUCTION)) / 100);

			if (Obj.getFlag(WEAROUT)) {
				if (Obj.getAttribute(REMAININGUSES) > 1) {
					Change(Obj, REMAININGUSES, Obj.getAttribute(REMAININGUSES) - 1);
				} else {
					try {
						Change(Obj, ObjectType(Obj.getAttribute(WEAROUTTARGET)), 0);
					} catch (RESULT r) {
						// Quirk: one argument for two conversions: the type
						// takes the place of the exception number, and the
						// second number is undefined.
						error("TCreature::Damage: Exception %d while wearing out object %d.\n", Obj.getObjectType().getTypeID());
					}
				}
			}
		}
	}

	if (Damage <= 0) {
		GraphicalEffect(GetCreatureObject(), EFFECT_POFF);
		return 0;
	}

	if (DamageType == DAMAGE_POISON_CONDITION) {
		if (RaceData[Race].NoPoison) {
			return 0;
		}

		if (Damage > Skills[SKILL_POISON]->TimerValue()) {
			PoisonDamageOrigin = atkID;
			SetTimer(SKILL_POISON, Damage, 3, 3, -1);
		}

		DamageStimulus(atkID, Damage, DamageType);
		return Damage;
	}

	if (DamageType == DAMAGE_FIRE_CONDITION) {
		if (RaceData[Race].NoBurning) {
			return 0;
		}

		FireDamageOrigin = atkID;
		SetTimer(SKILL_BURNING, Damage / 10, 8, 8, -1);
		DamageStimulus(atkID, Damage, DamageType);
		return Damage;
	}

	if (DamageType == DAMAGE_ENERGY_CONDITION) {
		if (RaceData[Race].NoEnergy) {
			return 0;
		}

		EnergyDamageOrigin = atkID;
		SetTimer(SKILL_ENERGY, Damage / 20, 10, 10, -1);
		DamageStimulus(atkID, Damage, DamageType);
		return Damage;
	}

	if (DamageType == DAMAGE_PHYSICAL) {
		Damage -= Combat.GetArmorStrength();
		if (Damage <= 0) {
			GraphicalEffect(GetCreatureObject(), EFFECT_BLOCK_HIT);
			return 0;
		}
	}

	if ((DamageType == DAMAGE_POISON && RaceData[Race].NoPoison)
			|| (DamageType == DAMAGE_FIRE && RaceData[Race].NoBurning)
			|| (DamageType == DAMAGE_ENERGY && RaceData[Race].NoEnergy)
			|| (DamageType == DAMAGE_PHYSICAL && RaceData[Race].NoHit)
			|| (DamageType == DAMAGE_LIFEDRAIN && RaceData[Race].NoLifeDrain)) {
		GraphicalEffect(GetCreatureObject(), EFFECT_BLOCK_HIT);
		return 0;
	}

	DamageStimulus(atkID, Damage, DamageType);

	if (IsInvisible() && Type != PLAYER) {
		SetTimer(SKILL_ILLUSION, 0, 0, 0, -1);
		Outfit = OrgOutfit;
		AnnounceChangedCreature(ID, CREATURE_OUTFIT);
		NotifyAllCreatures(GetCreatureObject(), OBJECT_CHANGED, NONE);
	}

	if (DamageType == DAMAGE_MANADRAIN) {
		TSkill *sk = Skills[SKILL_MANA];
		if (Damage > sk->Get()) {
			Damage = sk->Get();
		}

		if (Damage == 0) {
			return 0;
		}

		sk->Change(-Damage);
		if (Type == PLAYER && GetConnection() != nullptr) {
			SendMessage(GetConnection(), TALK_STATUS_MESSAGE, "You lose %d mana.", Damage);
		}

		GraphicalEffect(GetCreatureObject(), EFFECT_MAGIC_RED);
		TextualEffect(GetCreatureObject(), COLOR_BLUE, "%d", Damage);
		return Damage;
	}

	if (Skills[SKILL_MANASHIELD]->TimerValue() > 0 || Skills[SKILL_MANASHIELD]->Get() > 0) {
		TSkill *sk = Skills[SKILL_MANA];
		if (sk->Get() >= Damage) {
			sk->Change(-Damage);
			GraphicalEffect(GetCreatureObject(), EFFECT_MANA_HIT);
			TextualEffect(GetCreatureObject(), COLOR_BLUE, "%d", Damage);
			if (Type == PLAYER && GetConnection() != nullptr) {
				if (atk != nullptr) {
					SendMessage(GetConnection(), TALK_STATUS_MESSAGE, "You lose %d mana blocking an attack by %s.", Damage, atk->Name);
				} else {
					SendMessage(GetConnection(), TALK_STATUS_MESSAGE, "You lose %d mana.", Damage);
				}
				SendPlayerData(GetConnection());
			}
			return Damage;
		}

		Damage -= sk->Get();
		sk->Set(0);
	}

	TSkill *sk = Skills[SKILL_HITPOINTS];
	if (Damage > sk->Get()) {
		Damage = sk->Get();
	}

	sk->Change(-Damage);

	if (Responsible == atk) {
		if (atk != nullptr) {
			Combat.AddDamageToCombatList(atk->ID, Damage);
		}
	} else {
		if (Responsible != nullptr) {
			Combat.AddDamageToCombatList(Responsible->ID, Damage / 2);
		}
		if (atk != nullptr) {
			Combat.AddDamageToCombatList(atk->ID, Damage / 2);
		}
	}

	switch (DamageType) {
		case DAMAGE_PHYSICAL: {
			switch (RaceData[Race].Blood) {
				case BT_BLOOD: {
					GraphicalEffect(GetCreatureObject(), EFFECT_BLOOD_HIT);
					TextualEffect(GetCreatureObject(), COLOR_RED, "%d", Damage);
					try {
						CreatePool(GetMapContainer(GetCreatureObject()), GetSpecialObject(BLOOD_SPLASH), LIQUID_BLOOD);
					} catch (RESULT) {
					}
					break;
				}

				case BT_SLIME: {
					GraphicalEffect(GetCreatureObject(), EFFECT_POISON_HIT);
					TextualEffect(GetCreatureObject(), COLOR_GREEN, "%d", Damage);
					try {
						CreatePool(GetMapContainer(GetCreatureObject()), GetSpecialObject(BLOOD_SPLASH), LIQUID_SLIME);
					} catch (RESULT) {
					}
					break;
				}

				case BT_BONES: {
					GraphicalEffect(GetCreatureObject(), EFFECT_HIT_AREA);
					TextualEffect(GetCreatureObject(), COLOR_LIGHTGRAY, "%d", Damage);
					break;
				}

				case BT_FIRE: {
					GraphicalEffect(GetCreatureObject(), EFFECT_BLOOD_HIT);
					TextualEffect(GetCreatureObject(), COLOR_ORANGE, "%d", Damage);
					break;
				}

				case BT_ENERGY: {
					GraphicalEffect(GetCreatureObject(), EFFECT_ENERGY_HIT);
					TextualEffect(GetCreatureObject(), COLOR_LIGHTBLUE, "%d", Damage);
					break;
				}

				default: {
					error("TCreature::Damage: Invalid blood type %d for race %d.\n", RaceData[Race].Blood, Race);
					break;
				}
			}
			break;
		}

		case DAMAGE_POISON: {
			GraphicalEffect(GetCreatureObject(), EFFECT_POISON);
			TextualEffect(GetCreatureObject(), COLOR_GREEN, "%d", Damage);
			break;
		}

		case DAMAGE_FIRE: {
			GraphicalEffect(GetCreatureObject(), EFFECT_FIRE_HIT);
			TextualEffect(GetCreatureObject(), COLOR_ORANGE, "%d", Damage);
			break;
		}

		case DAMAGE_ENERGY: {
			GraphicalEffect(GetCreatureObject(), EFFECT_ENERGY_HIT);
			TextualEffect(GetCreatureObject(), COLOR_LIGHTBLUE, "%d", Damage);
			break;
		}

		case DAMAGE_LIFEDRAIN: {
			GraphicalEffect(GetCreatureObject(), EFFECT_MAGIC_RED);
			TextualEffect(GetCreatureObject(), COLOR_RED, "%d", Damage);
			break;
		}

		default: {
			// Quirk: this returns with the hitpoints already reduced,
			// so a death is not handled.
			error("TCreature::Damage: Invalid damage type %d.\n", DamageType);
			return Damage;
		}
	}

	if (Type == PLAYER && GetConnection() != nullptr) {
		if (atk != nullptr) {
			SendMessage(GetConnection(), TALK_STATUS_MESSAGE, "You lose %d hitpoint%s due to an attack by %s.",
					Damage, (Damage != 1) ? "s" : "", atk->Name);
		} else {
			SendMessage(GetConnection(), TALK_STATUS_MESSAGE, "You lose %d hitpoint%s.",
					Damage, (Damage != 1) ? "s" : "");
		}
		SendPlayerData(GetConnection());
	}

	if (sk->Get() <= 0) {
		if (Type == PLAYER) {
			for (int b = FIRST_BODY_POSITION; b <= LAST_BODY_POSITION; b++) {
				Object Obj = GetBodyObject(ID, b);
				if (Obj == NONE) {
					continue;
				}

				if (!Obj.getFlag(CLOTHES) || Obj.getAttribute(BODYPOSITION) != (uint32_t)b) {
					continue;
				}

				bool DeleteObject = false;
				if (Obj.getObjectType() == GetNewObjectType(77, 12)) {
					Log("game", "%s dies with Amulet of Loss.\n", Name);
					LoseInventory = LOSE_INVENTORY_NONE;
					DeleteObject = true;
				}

				if (DeleteObject) {
					try {
						Delete(Obj, -1);
					} catch (RESULT r) {
						error("TCreature::Damage: Exception %d while deleting object %d.\n", r, Obj.getObjectType().getTypeID());
					}
				}
			}
		}

		int OldLevel = Skills[SKILL_LEVEL]->Get();
		if (Type == PLAYER) {
			((TPlayer *)this)->DeathKiller = Responsible != nullptr ? Responsible->ID : 0;
		}
		Death();

		if (atk != nullptr && Type == PLAYER) {
			atk->BlockLogout(900, true);
		}

		if (Responsible != nullptr && Type == PLAYER) {
			Responsible->BlockLogout(900, true);
		}

		uint32_t MurdererID = 0;
		char Remark[30];
		Remark[0] = 0;

		if (atk != nullptr) {
			AddKillStatistics(atk->Race, Race);
			strcpy(Murderer, atk->Name);
			if (Responsible != nullptr && Responsible->Type == PLAYER) {
				MurdererID = Responsible->ID;
			}

			if (atk->Type != PLAYER) {
				strcpy(Remark, atk->Name);
			}
		} else {
			AddKillStatistics(0, Race);
			switch (DamageType) {
				case DAMAGE_PHYSICAL: strcpy(Remark, "a hit"); break;
				case DAMAGE_POISON: strcpy(Remark, "poison"); break;
				case DAMAGE_FIRE: strcpy(Remark, "fire"); break;
				case DAMAGE_ENERGY: strcpy(Remark, "energy"); break;
				default: {
					error("TCreature::Damage: Invalid damage type %d as cause of death.\n", DamageType);
					break;
				}
			}
		}

		if (Type == PLAYER) {
			for (int i = 1; i <= 2; i++) {
				uint32_t Offender;
				if (i == 1) {
					Offender = MurdererID;
				} else {
					Offender = Combat.GetMostDangerousAttacker();
				}

				if (i == 2 && (Offender == 0 || Offender == MurdererID || Offender >= FIRST_NONPLAYER_CREATURE_ID)) {
					continue;
				}

				int Unjustified = 0;
				if (Offender != 0 && Offender < FIRST_NONPLAYER_CREATURE_ID) {
					TPlayer *pl = GetPlayer(Offender);
					if (pl == nullptr) {
						continue;
					}

					if (!pl->IsAttackJustified(ID)) {
						Unjustified = 1;
					}
					pl->RecordMurder(ID);
				}

				CharacterDeathOrder(this, OldLevel, Offender, Remark, Offender != ID && Unjustified != 0);
				Remark[0] = 0;
			}
		}

		// The corpse is made later, when the dead monster is deleted
		// (~TCreature), so the hook gets none.
		if (Type == MONSTER) {
			MonsterDeathHook(RaceData[Race].Script, ID, Responsible != nullptr ? Responsible->ID : 0);
		}
	}

	AnnounceChangedCreature(ID, CREATURE_HEALTH);
	return Damage;
}

bool TCreature::MovePossible(int x, int y, int z, bool Execute, bool Jump) {
	if (Jump) {
		if (!JumpPossible(x, y, z, false)) {
			return false;
		}
	} else {
		if (!::MovePossible(x, y, z)) {
			return false;
		}
	}

	if (!Execute && CoordinateAvoid(x, y, z)) {
		return false;
	}

	return true;
}

TRaceData::TRaceData() : Skill(1, 5, 5), Talk(1, 5, 5, 0), Item(1, 5, 5) {
	Key[0] = 0;
	Name[0] = 0;
	Article[0] = 0;
	Outfit.OutfitID = 0;
	memset(Outfit.Colors, 0, sizeof(Outfit.Colors));
	Blood = BT_BLOOD;
	ExperiencePoints = 0;
	FleeThreshold = 0;
	Attack = 0;
	Defend = 0;
	Armor = 0;
	Poison = 0;
	SummonCost = 0;
	LoseTarget = 0;
	Strategy[0] = 100;
	for (int i = 1; i < 4; i++) {
		Strategy[i] = 0;
	}
	KickBoxes = false;
	KickCreatures = false;
	SeeInvisible = false;
	Unpushable = false;
	DistanceFighting = false;
	NoSummon = false;
	NoIllusion = false;
	NoConvince = false;
	NoBurning = false;
	NoPoison = false;
	NoEnergy = false;
	NoHit = false;
	NoLifeDrain = false;
	NoParalyze = false;
	Skills = 0;
	Talks = 0;
	Items = 0;
	Spells = 0;
	Script = -1;
}

char *GetRaceName(int Race) {
	if (Race < 1 || Race >= MAX_RACES) {
		error("GetRaceName: Invalid race number %d.\n", Race);
		return nullptr;
	}

	return RaceData[Race].Name;
}

TOutfit GetRaceOutfit(int Race) {
	if (Race < 1 || Race >= MAX_RACES) {
		error("GetRaceOutfit: Invalid race number %d.\n", Race);
		return RaceData[1].Outfit;
	}

	return RaceData[Race].Outfit;
}

bool GetRaceNoSummon(int Race) {
	if (Race < 1 || Race >= MAX_RACES) {
		error("GetRaceNoSummon: Invalid race number %d.\n", Race);
		return true;
	}

	return RaceData[Race].NoSummon;
}

bool GetRaceNoConvince(int Race) {
	if (Race < 1 || Race >= MAX_RACES) {
		error("GetRaceNoConvince: Invalid race number %d.\n", Race);
		return true;
	}

	return RaceData[Race].NoConvince;
}

bool GetRaceNoIllusion(int Race) {
	if (Race < 1 || Race >= MAX_RACES) {
		error("GetRaceNoIllusion: Invalid race number %d.\n", Race);
		return true;
	}

	return RaceData[Race].NoIllusion;
}

bool GetRaceNoParalyze(int Race) {
	if (Race < 1 || Race >= MAX_RACES) {
		error("GetRaceNoParalyze: Invalid race number %d.\n", Race);
		return true;
	}

	return RaceData[Race].NoParalyze;
}

int GetRaceSummonCost(int Race) {
	if (Race < 1 || Race >= MAX_RACES) {
		error("GetRaceSummonCost: Invalid race number %d.\n", Race);
		return 0;
	}

	return RaceData[Race].SummonCost;
}

int GetRacePoison(int Race) {
	if (Race < 1 || Race >= MAX_RACES) {
		error("GetRacePoison: Invalid race number %d.\n", Race);
		return 0;
	}

	return RaceData[Race].Poison;
}

bool GetRaceUnpushable(int Race) {
	if (Race < 1 || Race >= MAX_RACES) {
		error("GetRaceUnpushable: Invalid race number %d.\n", Race);
		return true;
	}

	return RaceData[Race].Unpushable;
}

// The races of monsters.toml as LoadRaces read them, and their numbers by key.
static std::vector<TMonsterData> MonsterRaces;
static std::map<std::string, int> RaceNumbers;

int GetRaceByKey(const char *Key) {
	if (Key == nullptr) {
		return 0;
	}

	auto Found = RaceNumbers.find(Key);
	return Found != RaceNumbers.end() ? Found->second : 0;
}

const std::vector<TMonsterData> &GetMonsterRaces() {
	return MonsterRaces;
}

// The text of a start error LoadRace throws, which lives on after the throw.
static char RaceErrorString[4096 + 512];

// Fills the race of Entry (an entry of monsters.toml, checked by its loader):
// its skills and talk in the entry's order, each text of talk a dynamic string
// (AddDynamicString); Spells gets only the number of its spells (the spell
// library prepares them, LoadRaces). Then its script is loaded. Throws a
// const char * "<file>:<line>:<column>: unknown skill name ..." for a skill
// GetSkillByName does not know, and what LoadScript throws.
static void LoadRace(const TMonsterData &Entry) {
	print(1, "Loading race data of %s from %s...\n", Entry.Key.c_str(), Entry.Where.c_str());

	int Race = Entry.Number;
	TRaceData &R = RaceData[Race];
	// The loader checked the lengths: a key of at most MAX_MONSTER_KEY_LENGTH,
	// a name of at most 29 and an article of at most 2 characters.
	strcpy(R.Key, Entry.Key.c_str());
	strcpy(R.Name, Entry.Name.c_str());
	strcpy(R.Article, Entry.Article.c_str());
	R.Outfit = Entry.Outfit;
	R.MaleCorpse = Entry.Corpse;
	R.FemaleCorpse = Entry.FemaleCorpse;

	if (Entry.Blood == "blood") {
		R.Blood = BT_BLOOD;
	} else if (Entry.Blood == "slime") {
		R.Blood = BT_SLIME;
	} else if (Entry.Blood == "bones") {
		R.Blood = BT_BONES;
	} else if (Entry.Blood == "fire") {
		R.Blood = BT_FIRE;
	} else {
		R.Blood = BT_ENERGY;
	}

	R.ExperiencePoints = Entry.Experience;
	R.SummonCost = Entry.SummonCost;
	R.FleeThreshold = Entry.FleeThreshold;
	R.Attack = Entry.Attack;
	R.Defend = Entry.Defend;
	R.Armor = Entry.Armor;
	R.Poison = Entry.Poison;
	R.LoseTarget = Entry.LoseTarget;
	for (int i = 0; i < 4; i++) {
		R.Strategy[i] = Entry.Strategy[i];
	}

	for (const std::string &Flag : Entry.Flags) {
		if (Flag == "kick_boxes") {
			R.KickBoxes = true;
		} else if (Flag == "kick_creatures") {
			R.KickCreatures = true;
		} else if (Flag == "see_invisible") {
			R.SeeInvisible = true;
		} else if (Flag == "unpushable") {
			R.Unpushable = true;
		} else if (Flag == "distance_fighting") {
			R.DistanceFighting = true;
		} else if (Flag == "no_summon") {
			R.NoSummon = true;
		} else if (Flag == "no_convince") {
			R.NoConvince = true;
		} else if (Flag == "no_illusion") {
			R.NoIllusion = true;
		} else if (Flag == "no_burning") {
			R.NoBurning = true;
		} else if (Flag == "no_poison") {
			R.NoPoison = true;
		} else if (Flag == "no_energy") {
			R.NoEnergy = true;
		} else if (Flag == "no_hit") {
			R.NoHit = true;
		} else if (Flag == "no_life_drain") {
			R.NoLifeDrain = true;
		} else if (Flag == "no_paralyze") {
			R.NoParalyze = true;
		}
	}

	for (const TMonsterSkillData &Skill : Entry.Skills) {
		char SkillName[64];
		int SkillNr = -1;
		if (Skill.Skill.size() < sizeof(SkillName)) {
			strcpy(SkillName, Skill.Skill.c_str());
			SkillNr = GetSkillByName(SkillName);
		}
		if (SkillNr == -1) {
			snprintf(RaceErrorString, sizeof(RaceErrorString), "%s: unknown skill name \"%s\"",
					Skill.Where.c_str(), Skill.Skill.c_str());
			throw static_cast<const char *>(RaceErrorString);
		}

		R.Skills++;
		TSkillData &S = R.Skill(R.Skills);
		S.Nr = SkillNr;
		S.Actual = Skill.Actual;
		S.Minimum = Skill.Minimum;
		S.Maximum = Skill.Maximum;
		S.NextLevel = Skill.NextLevel;
		S.FactorPercent = Skill.FactorPercent;
		S.AddLevel = Skill.AddLevel;
	}

	for (const std::string &Text : Entry.Talk) {
		R.Talks++;
		R.Talk(R.Talks) = AddDynamicString(const_cast<char *>(Text.c_str()));
	}

	for (const TMonsterItemData &Item : Entry.Inventory) {
		R.Items++;
		TItemData &I = R.Item(R.Items);
		I.Type = Item.Type;
		I.Maximum = Item.Maximum;
		I.Probability = Item.Chance;
	}

	R.Spells = static_cast<int>(Entry.Spells.size());

	if (!Entry.Script.empty()) {
		R.Script = LoadScript(Entry.Script.c_str(), Entry.Where.c_str());
	}
}

void LoadRaces() {
	char FileName[4096];
	snprintf(FileName, sizeof(FileName), "%s/monsters.toml", DATAPATH);
	LoadMonsterRaces(FileName, MonsterRaces);

	RaceNumbers.clear();
	for (const TMonsterData &Entry : MonsterRaces) {
		RaceNumbers[Entry.Key] = Entry.Number;
		LoadRace(Entry);
	}

	InitMonsterSpells();
	for (const TMonsterData &Entry : MonsterRaces) {
		for (size_t i = 0; i < Entry.Spells.size(); i++) {
			PrepareMonsterSpell(Entry.Number, Entry.Spells[i], Entry.SpellWhere[i].c_str());
		}
	}
}

TAttackWave::TAttackWave() : ExtraItem(1, 5, 5) {
	x = 0;
	y = 0;
	z = 0;
	Spread = 0;
	Race = -1;
	MinCount = 0;
	MaxCount = 0;
	Radius = 0x7fffffff;
	Lifetime = 0;
	Message = 0;
	ExtraItems = 0;
	Raid = -1;
	Wave = 0;
}

TAttackWave::~TAttackWave() {
	if (Message != 0) {
		DeleteDynamicString(Message);
	}
}

// A raid of raids.toml, as LoadMonsterRaids read it, with the number of its
// script (LoadScript), -1 for none.
struct TRaid {
	TRaidData Data;
	int Script;
};

// The raids of raids.toml in the file's order (TAttackWave::Raid indexes it).
static std::vector<TRaid> Raids;

// A new wave of the raid (Wave counts from 0 in Raids[Raid].Data.Waves), its
// message a new dynamic string (AddDynamicString; 0 for none), which the
// destructor deletes.
static TAttackWave *MakeAttackWave(int Raid, int Wave) {
	const TRaidWaveData &W = Raids[Raid].Data.Waves[Wave];
	TAttackWave *AttackWave = new TAttackWave;
	AttackWave->x = W.x;
	AttackWave->y = W.y;
	AttackWave->z = W.z;
	AttackWave->Spread = W.Spread;
	AttackWave->Race = GetRaceByKey(W.Race.c_str());
	AttackWave->MinCount = W.MinCount;
	AttackWave->MaxCount = W.MaxCount;
	AttackWave->Radius = W.Radius;
	AttackWave->Lifetime = W.Lifetime;
	AttackWave->Message = AddDynamicString(const_cast<char *>(W.Message.c_str()));
	for (const TMonsterItemData &Item : W.Inventory) {
		AttackWave->ExtraItems++;
		TItemData &I = AttackWave->ExtraItem(AttackWave->ExtraItems);
		I.Type = Item.Type;
		I.Maximum = Item.Maximum;
		I.Probability = Item.Chance;
	}
	AttackWave->Raid = Raid;
	AttackWave->Wave = Wave + 1;
	return AttackWave;
}

// Queues the waves of the raid to start in round Start, in their order, and
// calls its onRaid.
static void QueueRaid(int Raid, int Start) {
	const TRaidData &R = Raids[Raid].Data;
	print(1, "Scheduling raid %s for round %d.\n", R.Key.c_str(), Start);
	for (int i = 0; i < static_cast<int>(R.Waves.size()); i++) {
		TAttackWave *AttackWave = MakeAttackWave(Raid, i);
		AttackWaveQueue.insert(Start + R.Waves[i].Delay, AttackWave);
	}
	RaidHook(Raids[Raid].Script, R.Key.c_str());
}

bool QueueMonsterRaid(const char *Key, int Start) {
	if (Key == nullptr) {
		return false;
	}

	for (int i = 0; i < static_cast<int>(Raids.size()); i++) {
		if (Raids[i].Data.Key == Key) {
			QueueRaid(i, Start);
			return true;
		}
	}
	return false;
}

// Each raid in the file's order: its duration (the latest end of a wave, a
// wave without lifetime lasting 3600 seconds; each wave is made and deleted
// for it, so its message takes a dynamic string number, which stays used
// until CleanupDynamicStrings); then a dated raid is
// queued if its date comes before the next reboot, and a raid with an
// interval if its random draw falls before the reboot. Of the big raids drawn
// only the one with the highest tie breaker is queued, after all others, and
// none if a dated big raid was queued.
void LoadMonsterRaids() {
	time_t Now = static_cast<int32_t>(time(nullptr));
	int Hour, Minute;
	GetRealTime(Hour, Minute);
	int Remaining = ((RebootTime - (Hour * 60 + Minute) + 1440) % 1440) * 60;

	char FileName[4096];
	snprintf(FileName, sizeof(FileName), "%s/raids.toml", DATAPATH);
	std::vector<TRaidData> Data;
	LoadRaids(FileName, MonsterRaces, Data);

	Raids.clear();
	for (TRaidData &Entry : Data) {
		TRaid Raid;
		Raid.Data = std::move(Entry);
		Raid.Script = -1;
		if (!Raid.Data.Script.empty()) {
			Raid.Script = LoadScript(Raid.Data.Script.c_str(), Raid.Data.Where.c_str());
		}
		Raids.push_back(std::move(Raid));
	}

	int TieBreaker = -1;
	int BigRaid = -1;
	int BigRaidDuration = 0;

	for (int i = 0; i < static_cast<int>(Raids.size()); i++) {
		const TRaidData &R = Raids[i].Data;
		int Date = R.Date;
		int Interval = R.Interval;
		int Duration = 0;
		for (int j = 0; j < static_cast<int>(R.Waves.size()); j++) {
			TAttackWave *AttackWave = MakeAttackWave(i, j);
			int End;
			if (AttackWave->Lifetime == 0) {
				End = R.Waves[j].Delay + 3600;
			} else {
				End = R.Waves[j].Delay + AttackWave->Lifetime;
			}

			if (End > Duration) {
				Duration = End;
			}

			delete AttackWave;
		}
		print(1, "Raid %s: Date %d, Interval %d, Duration %d\n", R.Key.c_str(), Date, Interval, Duration);

		if (Date > 0 && in(Date, (int)Now, (int)(Now + Remaining))) {
			QueueRaid(i, (int)(RoundNr + Date - Now));
			if (R.Big) {
				BigRaid = -1;
				TieBreaker = 100;
			}
		}

		if (Date == 0 && Duration <= Remaining && random(0, Interval - 1) < Remaining) {
			if (R.Big) {
				int NewTieBreaker = random(0, 99);
				if (NewTieBreaker > TieBreaker) {
					TieBreaker = NewTieBreaker;
					BigRaid = i;
					BigRaidDuration = Duration;
				}
			} else {
				QueueRaid(i, RoundNr + random(0, Remaining - Duration));
			}
		}
	}

	if (BigRaid != -1) {
		QueueRaid(BigRaid, RoundNr + random(0, Remaining - BigRaidDuration));
	}
}

void ProcessMonsterRaids() {
	uint32_t ExecutionRound;
	TAttackWave *AttackWave;

	while (AttackWaveQueue.getMin(ExecutionRound, AttackWave) && ExecutionRound <= RoundNr) {
		AttackWaveQueue.deleteMin();
		print(2, "Attack by monsters of race %d.\n", AttackWave->Race);
		if (AttackWave->Message != 0) {
			// Quirk: the format ends with a single '%'.
			BroadcastMessage(TALK_EVENT_MESSAGE, "%s%", GetDynamicString(AttackWave->Message));
		}

		int Count = random(AttackWave->MinCount, AttackWave->MaxCount);
		TCreature *Monster[Count];
		int Monsters = 0;
		for (int i = 0; i < Count; i++) {
			int posx = AttackWave->x + random(-AttackWave->Spread, AttackWave->Spread);
			int posy = AttackWave->y + random(-AttackWave->Spread, AttackWave->Spread);
			int posz = AttackWave->z;
			if (!SearchFreeField(posx, posy, posz, 1, 0, false) || IsProtectionZone(posx, posy, posz)) {
				continue;
			}

			Monster[Monsters] = CreateMonster(AttackWave->Race, posx, posy, posz, 0, 0, true);
			if (Monster[Monsters] == nullptr) {
				continue;
			}

			if (AttackWave->Lifetime != 0) {
				Monster[Monsters]->LifeEndRound = RoundNr + AttackWave->Lifetime;
			}
			Monster[Monsters]->Radius = AttackWave->Radius;
			Monsters++;
		}

		for (int i = 1; i <= AttackWave->ExtraItems && Monsters > 0; i++) {
			TItemData &I = AttackWave->ExtraItem(i);
			if (random(0, 999) > I.Probability) {
				continue;
			}

			TCreature *cr = Monster[random(0, Monsters - 1)];
			Object Bag = GetBodyObject(cr->ID, BODY_CONTAINER);
			if (Bag == NONE) {
				try {
					Bag = Create(GetBodyContainer(cr->ID, BODY_CONTAINER), GetSpecialObject(INVENTORY_CONTAINER), 0);
				} catch (RESULT r) {
					error("ProcessMonsterRaids: Exception %d for race %d while creating the backpack.\n", r, cr->Race);
					continue;
				}
			}

			int Amount = random(1, I.Maximum);
			print(2, "Distributing %d objects of type %d.\n", Amount, I.Type.getTypeID());
			int Repeat = 1;
			if (!I.Type.getFlag(CUMULATIVE)) {
				Repeat = Amount;
				Amount = 0;
			}

			for (int j = 0; j < Repeat; j++) {
				Object Obj;
				if (I.Type.getFlag(WEAPON) || I.Type.getFlag(SHIELD) || I.Type.getFlag(BOW)
						|| I.Type.getFlag(THROW) || I.Type.getFlag(WAND) || I.Type.getFlag(WEAROUT)
						|| I.Type.getFlag(EXPIRE) || I.Type.getFlag(EXPIRESTOP)) {
					try {
						Obj = Create(Bag, I.Type, 0);
					} catch (RESULT r) {
						error("ProcessMonsterRaids: Exception %d for race %d, maybe increase CarryStrength.\n", r, cr->Race);
						break;
					}
				} else {
					Obj = CreateAtCreature(cr->ID, I.Type, Amount);
				}

				if (Obj.getContainer().isMapContainer()) {
					error("ProcessMonsterRaids: Object falls onto the map. Increase CarryStrength for race %d.\n", cr->Race);
					Delete(Obj, -1);
				}
			}

			if (GetFirstContainerObject(Bag) == NONE) {
				Delete(Bag, -1);
			}
		}

		int Raid = AttackWave->Raid;
		int Wave = AttackWave->Wave;
		delete AttackWave;
		RaidWaveHook(Raids[Raid].Script, Raids[Raid].Data.Key.c_str(), Wave);
	}
}

void InitCr() {
	FirstChainCreature = new matrix<uint32_t>(SectorXMin * 2, SectorXMax * 2 + 1, SectorYMin * 2, SectorYMax * 2 + 1, 0);
	NextCreatureID = FIRST_NONPLAYER_CREATURE_ID;
	LoadRaces();
	LoadMonsterRaids();
	FirstFreeCreature = 0;
	InitPlayer();
	InitNonplayer();
	InitKillStatistics();
}

void ExitCr() {
	ExitKillStatistics();
	ExitPlayer();
	ExitNonplayer();
	delete FirstChainCreature;
}
