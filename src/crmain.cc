#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <dirent.h>
#include <time.h>

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
#include "crmain.hh"

// The width and height in fields of a block of FirstChainCreature, which
// chains the creatures in each block (two blocks per sector side, see InitCr).
const int CHAIN_BLOCK_SIZE = 16;

// TCreature::MovePossible is declared throw(RESULT) in the original, and
// LoadRace, LoadRaces, LoadMonsterRaid, LoadMonsterRaids and InitCr are
// declared throw(const char *). C++17 has no dynamic exception specifications,
// so an exception of another type now propagates instead of ending in
// std::unexpected().

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
			error("ProcessSkills: Kreatur %d existiert nicht.\n", i);
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
			error("ProcessCreatures: Kreatur %d existiert nicht.\n", i);
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
			error("ProcessCreatures: Kreatur %s ist nicht tot, obwohl sie keine HP mehr hat.\n", cr->Name);
			cr->Death();
		}

		if (cr->LoggingOut && cr->LogoutPossible() == LOGOUT_POSSIBLE) {
			if (cr->IsDead && cr->Skills[SKILL_HITPOINTS]->Get() > 0) {
				error("ProcessCreatures: Kreatur %s hat HP, obwohl sie tot ist.\n", cr->Name);
				cr->Skills[SKILL_HITPOINTS]->Set(0);
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
		error("DeleteChainCreature: \xdc" "begebene Kreatur existiert nicht.\n");
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
			error("DeleteChainCreature: Kreatur nicht gefunden.\n");
			return;
		}

		TCreature *Help = GetCreature(ID);
		if (Help == nullptr) {
			error("DeleteChainCreature: Kreatur existiert nicht.\n");
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
		// The original names DeleteChainCreature here.
		error("DeleteChainCreature: \xdc" "begebene Kreatur existiert nicht.\n");
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
		// The original names DeleteChainCreature here.
		error("DeleteChainCreature: \xdc" "begebene Kreatur existiert nicht.\n");
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

// Only finished is set if the creature does not exist, as in the original.
TFindCreatures::TFindCreatures(int radx, int rady, uint32_t CreatureID, int Mask) {
	TCreature *cr = GetCreature(CreatureID);
	if (cr == nullptr) {
		error("TFindCreatures::TFindCreatures: Kreatur existiert nicht.\n");
		finished = true;
		return;
	}

	initSearch(radx, rady, cr->posx, cr->posy, Mask);
	SkipID = CreatureID;
}

// Only finished is set if the object does not exist, as in the original.
TFindCreatures::TFindCreatures(int radx, int rady, Object Obj, int Mask) {
	if (!Obj.exists()) {
		error("TFindCreatures::TFindCreatures: \xdc" "bergebenes Objekt existiert nicht.\n");
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
		error("TFindCreatures::getNext: Kreatur existiert nicht.\n");
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
			// The result is not copied back, as in the original: the name
			// stays in the singular and without capitals.
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
// initialized, as in the original.
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
				error("TCreature::~TCreature: Kann Blutlache nicht setzen (Exc %d, Pos [%d,%d,%d]).\n", r, posx, posy, posz);
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
							error("TCreature::~TCreature: Exception %d beim L\xf6schen eines Feldes.\n", r);
						}
					}
				}
			}

			Object Corpse = Create(GetMapContainer(posx, posy, posz), CorpseType, 0);
			Log("game", "Tod von %s: LoseInventory=%d.\n", Name, LoseInventory);

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
			error("TCreature::~TCreature: Kann Leiche/Inventory nicht ablegen (Exc %d, Pos [%d,%d,%d], %s).\n", r, posx, posy, posz, Name);
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
			error("TCreature::~TCreature: Verkettungsfehler bei Kreatur %lu.\n", (unsigned long)ID);
		}
		KnownCreature->State = KNOWNCREATURE_FREE;
		KnownCreature = KnownCreature->Next;
	}
}

void TCreature::SetID(uint32_t CharacterID) {
	if (ID != 0) {
		error("TCreature::SetID: ID ist schon gesetzt.\n");
	}

	if (CharacterID == 0) {
		ID = NextCreatureID;
		NextCreatureID++;

		int test = 0;
		while (GetCreature(ID) != nullptr) {
			test++;
			error("TCreature::SetID: Doppelte Nonplayer-ID %d gefunden.\n", ID);
			ID = NextCreatureID;
			NextCreatureID++;
			if (test > 16) {
				error("TCreature::SetID: 16x hintereinander doppelte ID. Verwende nun doppelte ID %d\n", ID);
				break;
			}
		}
	} else {
		ID = CharacterID;
		if (GetCreature(CharacterID) != nullptr) {
			error("TCreature::SetID: Doppelte Character-ID %d gefunden.\n", ID);
		}
	}

	uint32_t m = ID % HASH_LIST_SIZE;
	NextHashEntry = HashList[m];
	HashList[m] = this;
}

// The entry is found by its ID, and the successor of the found entry is
// linked in, as in the original.
void TCreature::DelID() {
	uint32_t m = ID % HASH_LIST_SIZE;
	TCreature *cr = HashList[m];
	TCreature *PreviousEntry = nullptr;
	if (cr == nullptr) {
		error("TCreature::DelID: Hasheintrag nicht gefunden id = %d\n", ID);
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
		error("TCreature::DelID: id=%d nicht gefunden.\n", ID);
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
			error("TCreature::GetHealth: MaxHitpoints von %s ist %d, obwohl sie nicht tot ist.\n", Name, MaxHitpoints);
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
		error("TCreature::GetSpeed: Kein Skill GOSTRENGTH vorhanden.\n");
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
				error("TCreature::Damage: Objekt %d hat PROTECTION, aber nicht CLOTHES.\n", Obj.getObjectType().getTypeID());
				continue;
			}

			if (Obj.getAttribute(BODYPOSITION) != (uint32_t)b) {
				continue;
			}

			if ((Obj.getAttribute(PROTECTIONDAMAGETYPES) & DamageType) == 0) {
				continue;
			}

			// The original calculates this as unsigned.
			Damage = (int)((uint32_t)Damage * (100 - Obj.getAttribute(DAMAGEREDUCTION)) / 100);

			if (Obj.getFlag(WEAROUT)) {
				if (Obj.getAttribute(REMAININGUSES) > 1) {
					Change(Obj, REMAININGUSES, Obj.getAttribute(REMAININGUSES) - 1);
				} else {
					try {
						Change(Obj, ObjectType(Obj.getAttribute(WEAROUTTARGET)), 0);
					} catch (RESULT r) {
						// The original passes one argument for two
						// conversions: the type takes the place of the
						// exception number, the second number is undefined.
						error("TCreature::Damage: Exception %d beim Abnutzen von Objekt %d.\n", Obj.getObjectType().getTypeID());
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
					error("TCreature::Damage: Ung\xfcltiger Bluttyp %d f\xfcr Rasse %d.\n", RaceData[Race].Blood, Race);
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
			// The original returns here: the hitpoints are already
			// reduced, but a death is not handled.
			error("TCreature::Damage: Ung\xfcltiger Schadenstyp %d.\n", DamageType);
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
					Log("game", "%s stirbt mit Amulett of Loss.\n", Name);
					LoseInventory = LOSE_INVENTORY_NONE;
					DeleteObject = true;
				}

				if (DeleteObject) {
					try {
						Delete(Obj, -1);
					} catch (RESULT r) {
						error("TCreature::Damage: Exception %d beim L\xf6schen von Objekt %d.\n", r, Obj.getObjectType().getTypeID());
					}
				}
			}
		}

		int OldLevel = Skills[SKILL_LEVEL]->Get();
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
					error("TCreature::Damage: Ung\xfcltiger Schadenstyp %d als Todesursache.\n", DamageType);
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

TRaceData::TRaceData() : Skill(1, 5, 5), Talk(1, 5, 5, 0), Item(1, 5, 5), Spell(1, 5, 5) {
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
}

char *GetRaceName(int Race) {
	if (Race < 1 || Race >= MAX_RACES) {
		error("GetRaceName: Ung\xfcltige Rassen-Nummer %d.\n", Race);
		return nullptr;
	}

	return RaceData[Race].Name;
}

TOutfit GetRaceOutfit(int Race) {
	if (Race < 1 || Race >= MAX_RACES) {
		error("GetRaceOutfit: Ung\xfcltige Rassen-Nummer %d.\n", Race);
		return RaceData[1].Outfit;
	}

	return RaceData[Race].Outfit;
}

bool GetRaceNoSummon(int Race) {
	if (Race < 1 || Race >= MAX_RACES) {
		error("GetRaceNoSummon: Ung\xfcltige Rassen-Nummer %d.\n", Race);
		return true;
	}

	return RaceData[Race].NoSummon;
}

bool GetRaceNoConvince(int Race) {
	if (Race < 1 || Race >= MAX_RACES) {
		error("GetRaceNoConvince: Ung\xfcltige Rassen-Nummer %d.\n", Race);
		return true;
	}

	return RaceData[Race].NoConvince;
}

bool GetRaceNoIllusion(int Race) {
	if (Race < 1 || Race >= MAX_RACES) {
		error("GetRaceNoIllusion: Ung\xfcltige Rassen-Nummer %d.\n", Race);
		return true;
	}

	return RaceData[Race].NoIllusion;
}

bool GetRaceNoParalyze(int Race) {
	if (Race < 1 || Race >= MAX_RACES) {
		error("GetRaceNoParalyze: Ung\xfcltige Rassen-Nummer %d.\n", Race);
		return true;
	}

	return RaceData[Race].NoParalyze;
}

int GetRaceSummonCost(int Race) {
	if (Race < 1 || Race >= MAX_RACES) {
		error("GetRaceSummonCost: Ung\xfcltige Rassen-Nummer %d.\n", Race);
		return 0;
	}

	return RaceData[Race].SummonCost;
}

int GetRacePoison(int Race) {
	if (Race < 1 || Race >= MAX_RACES) {
		error("GetRacePoison: Ung\xfcltige Rassen-Nummer %d.\n", Race);
		return 0;
	}

	return RaceData[Race].Poison;
}

bool GetRaceUnpushable(int Race) {
	if (Race < 1 || Race >= MAX_RACES) {
		error("GetRaceUnpushable: Ung\xfcltige Rassen-Nummer %d.\n", Race);
		return true;
	}

	return RaceData[Race].Unpushable;
}

// The properties are tested one after the other without else, and neither an
// unknown property nor an unknown flag is an error, as in the original.
void LoadRace(char *FileName) {
	print(1, "Lade Rassendaten aus Datei %s...\n", FileName);

	TReadScriptFile Script;
	Script.open(FileName);

	if (strcmp(Script.readIdentifier(), "racenumber") != 0) {
		Script.error("race number expected");
	}

	Script.readSymbol('=');
	int Race = Script.readNumber();
	if (Race < 1 || Race >= MAX_RACES) {
		Script.error("illegal race number");
	}

	TRaceData &R = RaceData[Race];
	if (R.Name[0] != 0) {
		Script.error("race already defined");
	}

	memset(R.Outfit.Colors, 0, sizeof(R.Outfit.Colors));
	R.Outfit.OutfitID = Race;

	while (true) {
		Script.nextToken();
		if (Script.Token == ENDOFFILE) {
			break;
		}

		if (Script.Token != IDENTIFIER) {
			Script.error("identifier expected");
		}

		char Ident[30];
		strcpy(Ident, Script.getIdentifier());
		Script.readSymbol('=');

		if (strcmp(Ident, "name") == 0) {
			strcpy(R.Name, Script.readString());
		}

		if (strcmp(Ident, "article") == 0) {
			strcpy(R.Article, Script.readString());
		}

		if (strcmp(Ident, "outfit") == 0) {
			R.Outfit.load(Script);
		}

		if (strcmp(Ident, "corpse") == 0) {
			R.MaleCorpse = Script.readNumber();
			R.FemaleCorpse = R.MaleCorpse;
		}

		if (strcmp(Ident, "corpses") == 0) {
			R.MaleCorpse = Script.readNumber();
			Script.readSymbol(',');
			R.FemaleCorpse = Script.readNumber();
		}

		if (strcmp(Ident, "blood") == 0) {
			char *Value = Script.readIdentifier();
			if (strcmp(Value, "blood") == 0) {
				R.Blood = BT_BLOOD;
			} else if (strcmp(Value, "slime") == 0) {
				R.Blood = BT_SLIME;
			} else if (strcmp(Value, "bones") == 0) {
				R.Blood = BT_BONES;
			} else if (strcmp(Value, "fire") == 0) {
				R.Blood = BT_FIRE;
			} else if (strcmp(Value, "energy") == 0) {
				R.Blood = BT_ENERGY;
			} else {
				Script.error("unknown blood type");
			}
		}

		if (strcmp(Ident, "experience") == 0) {
			R.ExperiencePoints = Script.readNumber();
		}

		if (strcmp(Ident, "summoncost") == 0) {
			R.SummonCost = Script.readNumber();
		}

		if (strcmp(Ident, "fleethreshold") == 0) {
			R.FleeThreshold = Script.readNumber();
		}

		if (strcmp(Ident, "attack") == 0) {
			R.Attack = Script.readNumber();
		}

		if (strcmp(Ident, "defend") == 0) {
			R.Defend = Script.readNumber();
		}

		if (strcmp(Ident, "armor") == 0) {
			R.Armor = Script.readNumber();
		}

		if (strcmp(Ident, "poison") == 0) {
			R.Poison = Script.readNumber();
		}

		if (strcmp(Ident, "losetarget") == 0) {
			R.LoseTarget = Script.readNumber();
		}

		if (strcmp(Ident, "strategy") == 0) {
			Script.readSymbol('(');
			for (int i = 0; i < 4; i++) {
				if (i != 0) {
					Script.readSymbol(',');
				}
				R.Strategy[i] = Script.readNumber();
			}
			Script.readSymbol(')');
		}

		if (strcmp(Ident, "flags") == 0) {
			Script.readSymbol('{');
			do {
				char *Flag = Script.readIdentifier();
				if (strcmp(Flag, "kickboxes") == 0) {
					R.KickBoxes = true;
				}
				if (strcmp(Flag, "kickcreatures") == 0) {
					R.KickCreatures = true;
				}
				if (strcmp(Flag, "seeinvisible") == 0) {
					R.SeeInvisible = true;
				}
				if (strcmp(Flag, "unpushable") == 0) {
					R.Unpushable = true;
				}
				if (strcmp(Flag, "distancefighting") == 0) {
					R.DistanceFighting = true;
				}
				if (strcmp(Flag, "nosummon") == 0) {
					R.NoSummon = true;
				}
				if (strcmp(Flag, "noconvince") == 0) {
					R.NoConvince = true;
				}
				if (strcmp(Flag, "noillusion") == 0) {
					R.NoIllusion = true;
				}
				if (strcmp(Flag, "noburning") == 0) {
					R.NoBurning = true;
				}
				if (strcmp(Flag, "nopoison") == 0) {
					R.NoPoison = true;
				}
				if (strcmp(Flag, "noenergy") == 0) {
					R.NoEnergy = true;
				}
				if (strcmp(Flag, "nohit") == 0) {
					R.NoHit = true;
				}
				if (strcmp(Flag, "nolifedrain") == 0) {
					R.NoLifeDrain = true;
				}
				if (strcmp(Flag, "noparalyze") == 0) {
					R.NoParalyze = true;
				}
			} while (Script.readSpecial() != '}');
		}

		if (strcmp(Ident, "skills") == 0) {
			Script.readSymbol('{');
			do {
				Script.readSymbol('(');
				int SkillNr = GetSkillByName(Script.readIdentifier());
				if (SkillNr == -1) {
					Script.error("unknown skill name");
				}

				R.Skills++;
				TSkillData &S = R.Skill(R.Skills);
				S.Nr = SkillNr;
				Script.readSymbol(',');
				S.Actual = Script.readNumber();
				Script.readSymbol(',');
				S.Minimum = Script.readNumber();
				Script.readSymbol(',');
				S.Maximum = Script.readNumber();
				Script.readSymbol(',');
				S.NextLevel = Script.readNumber();
				Script.readSymbol(',');
				S.FactorPercent = Script.readNumber();
				Script.readSymbol(',');
				S.AddLevel = Script.readNumber();
				Script.readSymbol(')');
			} while (Script.readSpecial() != '}');
		}

		if (strcmp(Ident, "talk") == 0) {
			Script.readSymbol('{');
			do {
				R.Talks++;
				R.Talk(R.Talks) = AddDynamicString(Script.readString());
			} while (Script.readSpecial() != '}');
		}

		if (strcmp(Ident, "inventory") == 0) {
			Script.readSymbol('{');
			do {
				Script.readSymbol('(');
				R.Items++;
				TItemData &I = R.Item(R.Items);
				I.Type = Script.readNumber();
				Script.readSymbol(',');
				I.Maximum = Script.readNumber();
				Script.readSymbol(',');
				I.Probability = Script.readNumber();
				Script.readSymbol(')');
			} while (Script.readSpecial() != '}');
		}

		if (strcmp(Ident, "spells") == 0) {
			Script.readSymbol('{');
			do {
				R.Spells++;
				TSpellData &S = R.Spell(R.Spells);

				char *Shape = Script.readIdentifier();
				if (strcmp(Shape, "actor") == 0) {
					S.Shape = SHAPE_ACTOR;
					Script.readSymbol('(');
					S.ShapeParam1 = Script.readNumber();
					Script.readSymbol(')');
				} else if (strcmp(Shape, "victim") == 0) {
					S.Shape = SHAPE_VICTIM;
					Script.readSymbol('(');
					S.ShapeParam1 = Script.readNumber();
					Script.readSymbol(',');
					S.ShapeParam2 = Script.readNumber();
					Script.readSymbol(',');
					S.ShapeParam3 = Script.readNumber();
					Script.readSymbol(')');
				} else if (strcmp(Shape, "origin") == 0) {
					S.Shape = SHAPE_ORIGIN;
					Script.readSymbol('(');
					S.ShapeParam1 = Script.readNumber();
					Script.readSymbol(',');
					S.ShapeParam2 = Script.readNumber();
					Script.readSymbol(')');
				} else if (strcmp(Shape, "destination") == 0) {
					S.Shape = SHAPE_DESTINATION;
					Script.readSymbol('(');
					S.ShapeParam1 = Script.readNumber();
					Script.readSymbol(',');
					S.ShapeParam2 = Script.readNumber();
					Script.readSymbol(',');
					S.ShapeParam3 = Script.readNumber();
					Script.readSymbol(',');
					S.ShapeParam4 = Script.readNumber();
					Script.readSymbol(')');
				} else if (strcmp(Shape, "angle") == 0) {
					S.Shape = SHAPE_ANGLE;
					Script.readSymbol('(');
					S.ShapeParam1 = Script.readNumber();
					Script.readSymbol(',');
					S.ShapeParam2 = Script.readNumber();
					Script.readSymbol(',');
					S.ShapeParam3 = Script.readNumber();
					Script.readSymbol(')');
				} else {
					Script.error("unknown spell shape");
				}

				Script.readSymbol('I');

				char *Impact = Script.readIdentifier();
				if (strcmp(Impact, "damage") == 0) {
					S.Impact = IMPACT_DAMAGE;
					Script.readSymbol('(');
					S.ImpactParam1 = Script.readNumber();
					Script.readSymbol(',');
					S.ImpactParam2 = Script.readNumber();
					Script.readSymbol(',');
					S.ImpactParam3 = Script.readNumber();
					Script.readSymbol(')');
				} else if (strcmp(Impact, "field") == 0) {
					S.Impact = IMPACT_FIELD;
					Script.readSymbol('(');
					S.ImpactParam1 = Script.readNumber();
					Script.readSymbol(')');
				} else if (strcmp(Impact, "healing") == 0) {
					S.Impact = IMPACT_HEALING;
					Script.readSymbol('(');
					S.ImpactParam1 = Script.readNumber();
					Script.readSymbol(',');
					S.ImpactParam2 = Script.readNumber();
					Script.readSymbol(')');
				} else if (strcmp(Impact, "speed") == 0) {
					S.Impact = IMPACT_SPEED;
					Script.readSymbol('(');
					S.ImpactParam1 = Script.readNumber();
					Script.readSymbol(',');
					S.ImpactParam2 = Script.readNumber();
					Script.readSymbol(',');
					S.ImpactParam3 = Script.readNumber();
					Script.readSymbol(')');
				} else if (strcmp(Impact, "drunken") == 0) {
					S.Impact = IMPACT_DRUNKEN;
					Script.readSymbol('(');
					S.ImpactParam1 = Script.readNumber();
					Script.readSymbol(',');
					S.ImpactParam2 = Script.readNumber();
					Script.readSymbol(',');
					S.ImpactParam3 = Script.readNumber();
					Script.readSymbol(')');
				} else if (strcmp(Impact, "strength") == 0) {
					S.Impact = IMPACT_STRENGTH;
					Script.readSymbol('(');
					S.ImpactParam1 = Script.readNumber();
					Script.readSymbol(',');
					S.ImpactParam2 = Script.readNumber();
					Script.readSymbol(',');
					S.ImpactParam3 = Script.readNumber();
					Script.readSymbol(',');
					S.ImpactParam4 = Script.readNumber();
					Script.readSymbol(')');
				} else if (strcmp(Impact, "outfit") == 0) {
					S.Impact = IMPACT_OUTFIT;
					Script.readSymbol('(');
					// For an outfit of an object type only two of the four
					// bytes are read, so ImpactParam3 is undefined, as in
					// the original.
					TOutfit HelpOutfit;
					HelpOutfit.load(Script);
					S.ImpactParam1 = HelpOutfit.OutfitID;
					S.ImpactParam2 = HelpOutfit.Colors[0] + HelpOutfit.Colors[1] * 256;
					S.ImpactParam3 = HelpOutfit.Colors[2] + HelpOutfit.Colors[3] * 256;
					Script.readSymbol(',');
					S.ImpactParam4 = Script.readNumber();
					Script.readSymbol(')');
				} else if (strcmp(Impact, "summon") == 0) {
					S.Impact = IMPACT_SUMMON;
					Script.readSymbol('(');
					S.ImpactParam1 = Script.readNumber();
					Script.readSymbol(',');
					S.ImpactParam2 = Script.readNumber();
					Script.readSymbol(')');
				} else {
					Script.error("unknown spell impact");
				}

				Script.readSymbol(':');
				S.Delay = Script.readNumber();
				if (S.Delay == 0) {
					Script.error("zero spell delay");
				}
			} while (Script.readSpecial() != '}');
		}
	}

	Script.close();
}

void LoadRaces() {
	DIR *WorkDir = opendir(MONSTERPATH);
	if (WorkDir == nullptr) {
		error("LoadRaces: Unterverzeichnis %s nicht gefunden\n", MONSTERPATH);
		throw "Cannot load races";
	}

	dirent *DirEntry;
	while ((DirEntry = readdir(WorkDir)) != nullptr) {
		char *s = DirEntry->d_name;
		if (strlen(s) > 4 && strncmp(s + strlen(s) - 4, ".mon", 4) == 0) {
			char FileName[4096];
			sprintf(FileName, "%s/%s", MONSTERPATH, s);
			LoadRace(FileName);
		}
	}

	closedir(WorkDir);
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
}

TAttackWave::~TAttackWave() {
	if (Message != 0) {
		DeleteDynamicString(Message);
	}
}

// The properties of a wave are tested one after the other without else, and
// an unknown property is not an error, as in the original. Type is only
// written with the type of the raid; it is not preset.
void LoadMonsterRaid(char *FileName, int Start, bool *Type, int *Date, int *Interval, int *Duration) {
	if (FileName == nullptr) {
		error("LoadMonsterRaid: Dateiname ist NULL.\n");
		throw "cannot load monster raid";
	}

	if (Start < 0) {
		if (Type == nullptr || Date == nullptr || Interval == nullptr || Duration == nullptr) {
			error("LoadMonsterRaid: Zeiger f\xfcr R\xfc" "ckgabewerte sind NULL.\n");
			throw "cannot load monster raid";
		}

		*Date = 0;
		*Interval = 0;
		*Duration = 0;
	} else {
		print(1, "Plane Raid %s ein f\xfcr Runde %d.\n", FileName, Start);
	}

	TReadScriptFile Script;
	Script.open(FileName);

	Script.nextToken();
	if (strcmp(Script.getIdentifier(), "description") == 0) {
		Script.readSymbol('=');
		Script.readString();
		Script.nextToken();
	}

	if (strcmp(Script.getIdentifier(), "type") != 0) {
		Script.error("type expected");
	}

	Script.readSymbol('=');
	Script.readIdentifier();
	if (Start < 0) {
		*Type = strcmp(Script.getIdentifier(), "bigraid") == 0;
	}

	Script.readIdentifier();
	if (strcmp(Script.getIdentifier(), "date") == 0) {
		Script.readSymbol('=');
		Script.readNumber();
		if (Start < 0) {
			*Date = Script.getNumber();
		}
	} else if (strcmp(Script.getIdentifier(), "interval") == 0) {
		Script.readSymbol('=');
		Script.readNumber();
		if (Start < 0) {
			*Interval = Script.getNumber();
		}
	} else {
		Script.error("date or interval expected");
	}

	if (strcmp(Script.readIdentifier(), "delay") != 0) {
		Script.error("delay expected");
	}

	while (Script.Token == IDENTIFIER) {
		Script.readSymbol('=');
		int Delay = Script.readNumber();
		TAttackWave *AttackWave = new TAttackWave;

		while (true) {
			Script.nextToken();
			if (Script.Token == ENDOFFILE) {
				break;
			}

			if (Script.Token != IDENTIFIER) {
				Script.error("identifier expected");
			}

			if (strcmp(Script.getIdentifier(), "delay") == 0) {
				break;
			}

			char Ident[30];
			strcpy(Ident, Script.getIdentifier());
			Script.readSymbol('=');

			if (strcmp(Ident, "location") == 0) {
				Script.readString();
			}

			if (strcmp(Ident, "position") == 0) {
				Script.readCoordinate(AttackWave->x, AttackWave->y, AttackWave->z);
			}

			if (strcmp(Ident, "spread") == 0) {
				AttackWave->Spread = Script.readNumber();
			}

			if (strcmp(Ident, "race") == 0) {
				AttackWave->Race = Script.readNumber();
				if (!in(AttackWave->Race, 1, MAX_RACES - 1)) {
					Script.error("illegal race number");
				}
			}

			if (strcmp(Ident, "count") == 0) {
				Script.readSymbol('(');
				AttackWave->MinCount = Script.readNumber();
				Script.readSymbol(',');
				AttackWave->MaxCount = Script.readNumber();
				Script.readSymbol(')');
				if (AttackWave->MinCount > AttackWave->MaxCount) {
					Script.error("mincount greater than maxcount");
				}

				if (AttackWave->MinCount < 0 || AttackWave->MaxCount <= 0) {
					Script.error("illegal number of monsters");
				}
			}

			if (strcmp(Ident, "radius") == 0) {
				AttackWave->Radius = Script.readNumber();
			}

			if (strcmp(Ident, "lifetime") == 0) {
				AttackWave->Lifetime = Script.readNumber();
			}

			if (strcmp(Ident, "message") == 0) {
				AttackWave->Message = AddDynamicString(Script.readString());
			}

			if (strcmp(Ident, "inventory") == 0) {
				Script.readSymbol('{');
				do {
					Script.readSymbol('(');
					AttackWave->ExtraItems++;
					TItemData &I = AttackWave->ExtraItem(AttackWave->ExtraItems);
					I.Type = Script.readNumber();
					Script.readSymbol(',');
					I.Maximum = Script.readNumber();
					Script.readSymbol(',');
					I.Probability = Script.readNumber();
					Script.readSymbol(')');
				} while (Script.readSpecial() != '}');
			}
		}

		if (AttackWave->x == 0) {
			Script.error("position expected");
		}

		if (AttackWave->Race == -1) {
			Script.error("race expected");
		}

		if (AttackWave->MaxCount == 0) {
			Script.error("count expected");
		}

		if (Start < 0) {
			int End;
			if (AttackWave->Lifetime == 0) {
				End = Delay + 3600;
			} else {
				End = Delay + AttackWave->Lifetime;
			}

			if (End > *Duration) {
				*Duration = End;
			}

			delete AttackWave;
		} else {
			AttackWaveQueue.insert(Start + Delay, AttackWave);
		}
	}

	Script.close();
}

void LoadMonsterRaids() {
	time_t Now = static_cast<int32_t>(time(nullptr));
	int Hour, Minute;
	GetRealTime(Hour, Minute);
	int Remaining = ((RebootTime - (Hour * 60 + Minute) + 1440) % 1440) * 60;

	DIR *WorkDir = opendir(MONSTERPATH);
	if (WorkDir == nullptr) {
		error("LoadMonsterRaids: Unterverzeichnis %s nicht gefunden.\n", MONSTERPATH);
		throw "Cannot load monster raids";
	}

	dirent *DirEntry;
	int TieBreaker = -1;
	char BigRaidName[4096];
	BigRaidName[0] = 0;
	int BigRaidDuration = 0;

	while ((DirEntry = readdir(WorkDir)) != nullptr) {
		char *s = DirEntry->d_name;
		if (strlen(s) <= 4 || strncmp(s + strlen(s) - 4, ".evt", 4) != 0) {
			continue;
		}

		char FileName[4096];
		sprintf(FileName, "%s/%s", MONSTERPATH, s);

		bool BigRaid;
		int Date, Interval, Duration;
		LoadMonsterRaid(FileName, -1, &BigRaid, &Date, &Interval, &Duration);
		print(1, "Raid %s: Date %d, Interval %d, Duration %d\n", FileName, Date, Interval, Duration);

		if (Date > 0 && in(Date, (int)Now, (int)(Now + Remaining))) {
			LoadMonsterRaid(FileName, (int)(RoundNr + Date - Now), nullptr, nullptr, nullptr, nullptr);
			if (BigRaid) {
				BigRaidName[0] = 0;
				TieBreaker = 100;
			}
		}

		if (Date == 0 && Duration <= Remaining && random(0, Interval - 1) < Remaining) {
			if (BigRaid) {
				int NewTieBreaker = random(0, 99);
				if (NewTieBreaker > TieBreaker) {
					TieBreaker = NewTieBreaker;
					strcpy(BigRaidName, FileName);
					BigRaidDuration = Duration;
				}
			} else {
				LoadMonsterRaid(FileName, RoundNr + random(0, Remaining - Duration), nullptr, nullptr, nullptr, nullptr);
			}
		}
	}

	closedir(WorkDir);

	if (BigRaidName[0] != 0) {
		LoadMonsterRaid(BigRaidName, RoundNr + random(0, Remaining - BigRaidDuration), nullptr, nullptr, nullptr, nullptr);
	}
}

void ProcessMonsterRaids() {
	uint32_t ExecutionRound;
	TAttackWave *AttackWave;

	while (AttackWaveQueue.getMin(ExecutionRound, AttackWave) && ExecutionRound <= RoundNr) {
		AttackWaveQueue.deleteMin();
		print(2, "Angriff von Monstern der Rasse %d.\n", AttackWave->Race);
		if (AttackWave->Message != 0) {
			// The format ends with a single '%', as in the original.
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
					error("ProcessMonsterRaids: Exception %d bei Rasse %d beim Erstellen des Rucksacks.\n", r, cr->Race);
					continue;
				}
			}

			int Amount = random(1, I.Maximum);
			print(2, "Verteile %d Objekte vom Typ %d.\n", Amount, I.Type.getTypeID());
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
						error("ProcessMonsterRaids: Exception %d bei Rasse %d, ggf. CarryStrength erh\xf6hen.\n", r, cr->Race);
						break;
					}
				} else {
					Obj = CreateAtCreature(cr->ID, I.Type, Amount);
				}

				if (Obj.getContainer().isMapContainer()) {
					error("ProcessMonsterRaids: Objekt f\xe4llt auf die Karte. CarryStrength f\xfcr Rasse %d erh\xf6hen.\n", cr->Race);
					Delete(Obj, -1);
				}
			}

			if (GetFirstContainerObject(Bag) == NONE) {
				Delete(Bag, -1);
			}
		}

		delete AttackWave;
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
