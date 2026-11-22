// crnonpl.cc: the creatures that are not players. An NPC (TNPC) is made from
// its entry of npcs.toml and talks by its script (scripting.hh, "NPCs"),
// which the stimuli call; a monster (TMonster) chooses a target, fights,
// casts its spells (the spell library, scripting.hh, "Monsters") and moves,
// and calls its race's hooks; a monster home (monster_homes.toml) spawns
// monsters of one race and replaces the dead ones after its regeneration
// time.

#include <algorithm>
#include <climits>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>

#include "config.hh"
#include "threads.hh"
#include "dbfuncs.hh"
#include "objects.hh"
#include "map.hh"
#include "info.hh"
#include "cr.hh"
#include "containr.hh"
#include "crinc.hh"
#include "crmain.hh"
#include "crplayer.hh"
#include "connections.hh"
#include "operate.hh"
#include "magic.hh"
#include "strings.hh"
#include "writer.hh"
#include "utils.hh"
#include "time.hh"
#include "communication.hh"
#include "TXTEASymmetricKey.hh"
#include "npcdata.hh"
#include "monsterdata.hh"
#include "scripting.hh"
#include "reload.hh"
#include "crnonpl.hh"

// How a monster chooses its target (TRaceData::Strategy, the chance of each
// in percent): the nearest creature, the one with the fewest hit points, the
// one that did the monster the most damage, or any (the random tie breaker
// alone decides).
const int STRATEGY_NEAREST = 0;
const int STRATEGY_WEAKEST = 1;
const int STRATEGY_MOST_DAMAGE = 2;
const int STRATEGY_RANDOM = 3;

struct TMonsterhome {
	int Race;
	int x;
	int y;
	int z;
	int Radius;
	int MaxMonsters;
	int ActMonsters;
	int RegenerationTime;
	int Timer;
};

struct TMonster : public TNonplayer {
	TMonster(int Race, int x, int y, int z, int Home, uint32_t Father);
	virtual ~TMonster();
	virtual void DamageStimulus(uint32_t Attacker, int Damage, int);
	virtual void IdleStimulus();
	virtual void CreatureMoveStimulus(uint32_t CreatureID, int Type);
	virtual bool MovePossible(int x, int y, int z, bool Execute, bool);
	virtual bool IsPeaceful();
	virtual uint32_t GetMaster() { return Master; }
	void Convince(TCreature *newMaster);
	void SetTarget(TCreature *NewTarget);
	friend uint32_t GetMonsterTarget(TCreature *Monster);
private:
	bool CanKickBoxes();
	void KickBoxes(Object Obj);
	bool KickCreature(TCreature *Obstacle);
	int Home;
	uint32_t Master;
	uint32_t Target;
};

vector<TNonplayer *> NonplayerList(0, 10000, 1000, nullptr);
int FirstFreeNonplayer;
vector<TMonsterhome> Monsterhome(1, 5000, 1000);
int Monsterhomes;

void StartMonsterhomeTimer(int nr) {
	if (nr < 1 || nr > Monsterhomes) {
		error("StartMonsterhomeTimer: Invalid monster home number %d.\n", nr);
		return;
	}

	TMonsterhome &MH = Monsterhome(nr);
	if (MH.Timer > 0) {
		error("StartMonsterhomeTimer: Counter is already running.\n");
		return;
	}

	if (MH.ActMonsters >= MH.MaxMonsters) {
		error("StartMonsterhomeTimer: Maximum monster count already reached.\n");
		error("# Monster home with race %d at [%d,%d,%d]\n", MH.Race, MH.x, MH.y, MH.z);
		return;
	}

	int PlayerAdjustedTime = MH.RegenerationTime;
	int NumberOfPlayers = GetNumberOfPlayers();
	if (NumberOfPlayers > 800) {
		PlayerAdjustedTime = PlayerAdjustedTime * 2 / 5;
	} else if (NumberOfPlayers > 200) {
		PlayerAdjustedTime = PlayerAdjustedTime * 200 / (NumberOfPlayers / 2 + 100);
	}
	MH.Timer = random(PlayerAdjustedTime / 2, PlayerAdjustedTime);
}

// Makes the monster homes of <DATAPATH>/monster_homes.toml in the file's
// order (their race keys checked against the races LoadRaces read) and
// spawns their monsters. Throws what LoadMonsterHomes throws (a start error).
// Throws a start error for a race key of config.toml (Race, at Where, in the
// table Table) that names no race.
static void CheckConfigRace(const std::string &Race, const std::string &Where, const char *Table) {
	if (GetRaceByKey(Race.c_str()) == 0) {
		static char Error[4096 + 512];
		snprintf(Error, sizeof(Error), "%s: unknown race \"%s\" in %s", Where.c_str(), Race.c_str(), Table);
		throw static_cast<const char *>(Error);
	}
}

void LoadMonsterhomes() {
	char FileName[4096];
	print(1, "Initializing monster homes ...\n");
	snprintf(FileName, sizeof(FileName), "%s/monster_homes.toml", DATAPATH);

	std::vector<TMonsterHomeData> Homes;
	LoadMonsterHomes(FileName, GetMonsterRaces(), Homes);

	// The races config.toml's [spawn.monsters] and [loot.monsters] name must
	// exist.
	for (const TMonsterSpawnRate &Rate : MonsterSpawnRates) {
		CheckConfigRace(Rate.Race, Rate.Where, "[spawn.monsters]");
	}
	for (const TMonsterLootRate &Rate : MonsterLootRates) {
		CheckConfigRace(Rate.Race, Rate.Where, "[loot.monsters]");
	}

	Monsterhomes = 0;
	for (const TMonsterHomeData &Home : Homes) {
		Monsterhomes++;
		TMonsterhome &MH = Monsterhome(Monsterhomes);
		MH.Race = GetRaceByKey(Home.Race.c_str());
		MH.x = Home.x;
		MH.y = Home.y;
		MH.z = Home.z;
		if (!IsOnMap(MH.x, MH.y, MH.z)) {
			print(1, "WARNING: Monster home [%d,%d,%d] is outside the map.\n", MH.x, MH.y, MH.z);
		}
		MH.Radius = Home.Radius;
		// With config.toml's [spawn] multipliers (the race's own or the global
		// ones).
		MH.MaxMonsters = ApplySpawnAmount(Home.Amount, GetSpawnAmount(Home.Race.c_str()));
		MH.ActMonsters = 0;
		MH.RegenerationTime = ApplySpawnSpeed(Home.Regeneration, GetSpawnSpeed(Home.Race.c_str()));
		MH.Timer = 0;
	}

	print(1, "%d monster homes loaded.\n", Monsterhomes);

	for (int i = 1; i <= Monsterhomes; i++) {
		TMonsterhome &MH = Monsterhome(i);
		for (int j = 0; j < MH.MaxMonsters; j++) {
			int x = MH.x;
			int y = MH.y;
			int z = MH.z;
			int r;
			if (j == 0) {
				r = std::min(1, MH.Radius);
			} else {
				r = -std::min(10, MH.Radius);
			}
			if (SearchSpawnField(x, y, z, r, false)) {
				CreateMonster(MH.Race, x, y, z, i, 0, false);
				MH.ActMonsters++;
			}
		}

		if (MH.Timer > 0) {
			error("LoadMonsterhomes: Timer is already running (race %d at [%d,%d,%d]).\n", MH.Race, MH.x, MH.y, MH.z);
		} else if (MH.ActMonsters < MH.MaxMonsters) {
			StartMonsterhomeTimer(i);
		}
	}
}

void ProcessMonsterhomes() {
	for (int i = 1; i <= Monsterhomes; i++) {
		TMonsterhome &MH = Monsterhome(i);
		if (MH.Timer == 0) {
			continue;
		}

		MH.Timer--;
		if (MH.Timer > 0) {
			continue;
		}

		int x = MH.x;
		int y = MH.y;
		int z = MH.z;
		int MaxRadius = std::min(10, MH.Radius);
		TFindCreatures Search(MaxRadius + 9, MaxRadius + 7, x, y, FIND_PLAYERS);
		while (true) {
			uint32_t ID = Search.getNext();
			if (ID == 0) {
				break;
			}

			TCreature *cr = GetCreature(ID);
			if (cr == nullptr) {
				error("ProcessMonsterhomes: Creature does not exist.\n");
				break;
			}

			if ((cr->posz <= 7 && z <= 7) || (cr->posz > 7 && abs(z - cr->posz) <= 2)) {
				MaxRadius = std::min(MaxRadius, std::max(abs(cr->posx - x) - 9, abs(cr->posy - y) - 7));
			}
		}

		int r;
		if (MH.ActMonsters == 0) {
			r = std::min(1, MH.Radius);
		} else {
			r = -MaxRadius;
		}

		if (MaxRadius >= 0 && SearchSpawnField(x, y, z, r, false)) {
			CreateMonster(MH.Race, x, y, z, i, 0, false);
			MH.ActMonsters++;
			if (MH.Timer > 0) {
				error("ProcessMonsterhomes: Timer is already running (race %d at [%d,%d,%d]).\n", MH.Race, x, y, z);
				continue;
			}
		}

		if (MH.ActMonsters < MH.MaxMonsters) {
			StartMonsterhomeTimer(i);
		}
	}
}

void NotifyMonsterhomeOfDeath(int nr) {
	if (nr < 1 || nr > Monsterhomes) {
		error("NotifyMonsterhomeOfDeath: Invalid monster home number %d.\n", nr);
		return;
	}

	TMonsterhome &MH = Monsterhome(nr);
	if (MH.ActMonsters <= 0) {
		error("NotifyMonsterhomeOfDeath: Monster home has no living creatures.\n");
		return;
	}

	MH.ActMonsters--;
	if (MH.ActMonsters >= MH.MaxMonsters) {
		error("NotifyMonsterhomeOfDeath: Monster home %d had too many monsters (%d instead of %d).\n",
				nr, MH.ActMonsters + 1, MH.MaxMonsters);
	} else if (MH.Timer == 0) {
		StartMonsterhomeTimer(nr);
	}
}

bool MonsterhomeInRange(int nr, int x, int y, int z) {
	if (nr == 0) {
		return true;
	}

	if (nr < 1 || nr > Monsterhomes) {
		error("MonsterhomeInRange: Invalid monster home number %d.\n", nr);
		return false;
	}

	TMonsterhome &MH = Monsterhome(nr);
	return abs(z - MH.z) <= 2 && abs(x - MH.x) <= MH.Radius && abs(y - MH.y) <= MH.Radius;
}

// Whether a monster of home nr at [x,y,z] may stay, by config.toml's [lure]:
// always with limit false or no home (nr 0), else within radius times the
// home's radius along x and y and floors floors above or below its middle.
// With the defaults the same as MonsterhomeInRange.
bool MonsterhomeInLureRange(int nr, int x, int y, int z) {
	if (nr == 0 || !LureLimit) {
		return true;
	}

	if (nr < 1 || nr > Monsterhomes) {
		error("MonsterhomeInLureRange: Invalid monster home number %d.\n", nr);
		return false;
	}

	TMonsterhome &MH = Monsterhome(nr);
	double Range = MH.Radius * LureRadius;
	return abs(z - MH.z) <= LureFloors && abs(x - MH.x) <= Range && abs(y - MH.y) <= Range;
}

TNonplayer::TNonplayer() {
	this->State = SLEEPING;
}

TNonplayer::~TNonplayer() {
	this->DelInList();
}

void TNonplayer::SetInList() {
	this->SetInCrList();
	NonplayerList(FirstFreeNonplayer) = this;
	FirstFreeNonplayer++;
}

void TNonplayer::DelInList() {
	for (int i = 0; i < FirstFreeNonplayer; i++) {
		if (NonplayerList(i) == this) {
			NonplayerList(i) = NonplayerList(FirstFreeNonplayer - 1);
			NonplayerList(FirstFreeNonplayer - 1) = nullptr;
			FirstFreeNonplayer--;
			return;
		}
	}

	error("TNonplayer::DelInList: Creature not found.\n");
}

// The NPCs' scripts, for the reload, in the order the NPCs were made (that
// of their creature IDs): each NPC that got a creature ID, with its script's
// name and the position of its entry's script key (npcs.toml is read only at
// start).
struct TNPCScriptRecord {
	uint32_t ID;
	std::string Script;
	std::string Where;
};
static std::vector<TNPCScriptRecord> NPCScripts;

// Made from Entry, an entry of npcs.toml, in the order the old constructor
// took the keys of an .npc file; the script is loaded (and its handlers
// required) where the behaviour database was parsed, before the checks of the
// home field. Throws what LoadScript and RequireHandler throw (a start
// error, with the position of the entry's script key).
TNPC::TNPC(const TNPCData &Entry) : QueuedPlayers(0, 9, 10), QueuedAddresses(0, 9, 10) {
	print(1, "Loading NPC data of %s from %s...\n", Entry.Name.c_str(), Entry.Where.c_str());
	this->Type = NPC;
	this->Direction = DIRECTION_SOUTH;
	strcpy(this->Name, Entry.Name.c_str());
	this->Sex = Entry.Sex;
	this->Race = Entry.Race;
	this->SetSkills(this->Race);
	this->Outfit = Entry.Outfit;
	this->posx = Entry.HomeX;
	this->posy = Entry.HomeY;
	this->posz = Entry.HomeZ;
	this->startx = Entry.HomeX;
	this->starty = Entry.HomeY;
	this->startz = Entry.HomeZ;
	this->Radius = Entry.Radius;
	this->Skills[SKILL_GOSTRENGTH]->SetAct(Entry.GoStrength);
	this->Skills[SKILL_GOSTRENGTH]->SetMax(Entry.GoStrength);
	this->Topic = 0;
	this->Price = 0;
	this->Amount = 1;
	this->TypeID = 0;
	this->Data = 0;
	this->LastTalk = 0;
	this->QueueLength = 0;
	this->ScriptNumber = LoadScript(Entry.Script.c_str(), Entry.Where.c_str());
	RequireHandler(this->ScriptNumber, "onCreatureSay", Entry.Where.c_str());
	RequireHandler(this->ScriptNumber, "onCreatureDisappear", Entry.Where.c_str());

	if (!IsOnMap(this->posx, this->posy, this->posz)) {
		print(1, "WARNING: NPC '%s' is outside the map.\n", this->Name);
		return;
	}

	this->OrgOutfit = this->Outfit;
	this->SetID(0);
	NPCScripts.push_back({this->ID, Entry.Script, Entry.Where});
	this->SetInList();
	if (!this->SetOnMap()) {
		print(1, "WARNING: Cannot place NPC '%s'.\n", this->Name);
		return;
	}

	this->ToDoYield();
}

// The script's onCreatureSay(npc, creature, text, situation), where the old
// code called react(this, Text, Situation): the interlocutor at the call as
// the creature. A script failure is logged and the stimulus carries on
// (TScriptCall::runCaught); a result code the script raises is thrown.
void TNPC::CallCreatureSay(char *Text, SITUATION Situation) {
	TScriptCall Call(this->ScriptNumber, "onCreatureSay");
	Call.addNpc(this->ID);
	Call.addCreature(this->Interlocutor);
	Call.addString(Text);
	Call.addNumber(static_cast<int>(Situation));
	Call.runCaught();
}

// The script's onCreatureDisappear(npc, creature), where the old code called
// react(this, "", VANISH). A script failure is logged and the stimulus
// carries on (TScriptCall::runCaught); a result code the script raises is
// thrown.
void TNPC::CallCreatureDisappear() {
	TScriptCall Call(this->ScriptNumber, "onCreatureDisappear");
	Call.addNpc(this->ID);
	Call.addCreature(this->Interlocutor);
	Call.runCaught();
}

void TNPC::TalkStimulus(uint32_t Speaker, char *Text) {
	if (Speaker == this->ID) {
		return;
	}

	if (Text == nullptr) {
		error("TNPC::TalkStimulus: Given text does not exist.\n");
		return;
	}

	if (this->State == TALKING || this->QueueLength != 0) {
		if (Speaker == this->Interlocutor) {
			this->LastTalk = RoundNr;
			this->CallCreatureSay(Text, DEFAULT);
		} else {
			uint32_t Help = this->Interlocutor;
			this->Interlocutor = Speaker;
			this->CallCreatureSay(Text, BUSY);
			this->Interlocutor = Help;
		}
	} else {
		this->ToDoClear();
		this->State = TALKING;
		this->Interlocutor = Speaker;
		this->LastTalk = RoundNr;
		this->CallCreatureSay(Text, ADDRESS);
		if (this->State == TALKING) {
			this->TurnToInterlocutor();
		}
	}
}

void TNPC::IdleStimulus() {
	if (this->State == TALKING) {
		if (RoundNr <= this->LastTalk + 30) {
			this->ToDoWait(2000);
			this->ToDoStart();
			return;
		}
		this->CallCreatureDisappear();
		this->State = IDLE;
	}

	if (this->State != IDLE) {
		return;
	}

Again:
	if (this->QueueLength > 0) {
		this->State = TALKING;
		this->ToDoClear();
		this->Interlocutor = this->QueuedPlayers(0);
		if (GetCreature(this->Interlocutor) == nullptr) {
			error("TNPC::IdleStimulus: Interlocutor does not exist.\n");
			this->State = IDLE;
		} else {
			this->CallCreatureSay(GetDynamicString(this->QueuedAddresses(0)), ADDRESSQUEUE);
		}

		DeleteDynamicString(this->QueuedAddresses(0));
		for (int i = 1; i < this->QueueLength; i++) {
			this->QueuedPlayers(i - 1) = this->QueuedPlayers(i);
			this->QueuedAddresses(i - 1) = this->QueuedAddresses(i);
		}
		this->QueueLength--;

		if (this->State == TALKING) {
			this->TurnToInterlocutor();
			this->LastTalk = RoundNr;
			return;
		}
		goto Again;
	}

	if (this->LockToDo) {
		return;
	}

	TFindCreatures Search(10, 10, this->ID, FIND_PLAYERS);
	if (Search.getNext() == 0) {
		this->State = SLEEPING;
		return;
	}

	int Try = 0;
	int destX;
	int destY;
	do {
		Try++;
		destX = this->posx;
		destY = this->posy;
		if (Try > 10) {
			this->ToDoWait(2000);
			this->ToDoStart();
			return;
		}

		int dir = rand() % 4;
		switch (dir) {
			case 0: destX--; break;
			case 1: destX++; break;
			case 2: destY--; break;
			case 3: destY++; break;
		}
	} while (!this->MovePossible(destX, destY, this->posz, true, false));

	try {
		this->ToDoGo(destX, destY, this->posz, true, INT_MAX);
		this->ToDoWait(2000);
		this->ToDoStart();
	} catch (RESULT r) {
		error("TNPC::IdleStimulus: Exception %d!\n", r);
	}
}

void TNPC::CreatureMoveStimulus(uint32_t CreatureID, int Type) {
	for (int i = 0; i < this->QueueLength; i++) {
		if (CreatureID == this->QueuedPlayers(i) || CreatureID == this->ID) {
			TCreature *cr = GetCreature(this->QueuedPlayers(i));
			// Quirk: cr is not checked for NULL.
			if (Type == OBJECT_DELETED
					|| abs(cr->posx - this->posx) >= 5
					|| abs(cr->posy - this->posy) >= 4
					|| cr->posz != this->posz) {
				DeleteDynamicString(this->QueuedAddresses(i));
				for (int j = i + 1; j < this->QueueLength; j++) {
					this->QueuedPlayers(j - 1) = this->QueuedPlayers(j);
					this->QueuedAddresses(j - 1) = this->QueuedAddresses(j);
				}
				this->QueueLength--;
				i--;
			}
		}
	}

	if (this->State == TALKING || this->State == LEAVING) {
		if (CreatureID == this->Interlocutor || CreatureID == this->ID) {
			TCreature *cr = GetCreature(this->Interlocutor);
			if (cr == nullptr) {
				error("TNPC::CreatureMoveStimulus: Interlocutor does not exist.\n");
				this->State = IDLE;
				this->ToDoYield();
				return;
			}

			if (Type != OBJECT_DELETED) {
				this->TurnToInterlocutor();
			}

			if (this->State != LEAVING) {
				if (Type == OBJECT_DELETED
						|| abs(cr->posx - this->posx) >= 5
						|| abs(cr->posy - this->posy) >= 4
						|| cr->posz != this->posz) {
					this->CallCreatureDisappear();
					this->State = IDLE;
					this->ToDoYield();
				}
			}
		}
	}

	if (CreatureID == this->ID) {
		return;
	}

	if (this->State == SLEEPING && Type != OBJECT_DELETED) {
		this->State = IDLE;
		this->ToDoYield();
	}
}

bool TNPC::MovePossible(int x, int y, int z, bool, bool) {
	return ::MovePossible(x, y, z)
		&& !CoordinateAvoid(x, y, z)
		&& abs(x - this->startx) <= this->Radius
		&& abs(y - this->starty) <= this->Radius
		&& z == this->startz
		&& !IsHouse(x, y, z);
}

void TNPC::GiveTo(ObjectType Type, int Amount) {
	if (Amount == 0) {
		return;
	}

	if (Type == ObjectType(0) || Type.getName(1) == nullptr) {
		error("TNPC::GiveTo: %s wants to create objects of type %d.\n", this->Name, Type.getTypeID());
		return;
	}

	Log("npc", "%s -> %ld: %d %s\n", this->Name, (long)this->Interlocutor, Amount, Type.getName(1));
	try {
		if (Type.getFlag(CUMULATIVE)) {
			// 100 is MAX_AMOUNT, written as a number: std::min takes its
			// arguments by reference, and with the name the compiler makes other
			// code for GiveTo.
			while (Amount > 0) {
				CreateAtCreature(this->Interlocutor, Type, std::min(Amount, 100));
				Amount -= std::min(Amount, 100);
			}
		} else {
			for (int i = 0; i < Amount; i++) {
				CreateAtCreature(this->Interlocutor, Type, this->Data);
			}
		}
	} catch (RESULT r) {
		error("TNPC::GiveTo: NPC %s, Exception %d, object %d.\n", this->Name, r, Type.getTypeID());
	}
}

void TNPC::GetFrom(ObjectType Type, int Amount) {
	if (Amount == 0) {
		return;
	}

	Log("npc", "%s <- %ld: %d %s\n", this->Name, (long)this->Interlocutor, Amount, Type.getName(1));
	try {
		DeleteAtCreature(this->Interlocutor, Type, Amount, this->Data);
	} catch (RESULT r) {
		error("TNPC::GetFrom: NPC %s, Exception %d, object %d.\n", this->Name, r, Type.getTypeID());
	}
}

void TNPC::GiveMoney(int Amount) {
	int Crystal = Amount / 10000;
	int Platinum = (Amount % 10000) / 100;
	int Gold = Amount % 100;
	this->GiveTo(GetSpecialObject(MONEY_TENTHOUSAND), Crystal);
	this->GiveTo(GetSpecialObject(MONEY_HUNDRED), Platinum);
	this->GiveTo(GetSpecialObject(MONEY_ONE), Gold);
}

void TNPC::GetMoney(int Amount) {
	int Crystal = CountInventoryObjects(this->Interlocutor, GetSpecialObject(MONEY_TENTHOUSAND), 0);
	int Platinum = CountInventoryObjects(this->Interlocutor, GetSpecialObject(MONEY_HUNDRED), 0);
	int Gold = CountInventoryObjects(this->Interlocutor, GetSpecialObject(MONEY_ONE), 0);
	CalculateChange(Amount, Gold, Platinum, Crystal);
	if (Gold > 0) {
		this->GetFrom(GetSpecialObject(MONEY_ONE), Gold);
	}
	if (Platinum > 0) {
		this->GetFrom(GetSpecialObject(MONEY_HUNDRED), Platinum);
	}
	if (Crystal > 0) {
		this->GetFrom(GetSpecialObject(MONEY_TENTHOUSAND), Crystal);
	}
	if (Gold < 0) {
		this->GiveTo(GetSpecialObject(MONEY_ONE), -Gold);
	}
	if (Platinum < 0) {
		this->GiveTo(GetSpecialObject(MONEY_HUNDRED), -Platinum);
	}
}

void TNPC::Enqueue(uint32_t Interlocutor, char *Text) {
	if (Interlocutor == 0) {
		error("TNPC::Enqueue: Interlocutor is Null.\n");
		return;
	}

	if (Text == nullptr) {
		error("TNPC::Enqueue: Text is NULL.\n");
		return;
	}

	for (int i = 0; i < this->QueueLength; i++) {
		if (this->QueuedPlayers(i) == Interlocutor) {
			return;
		}
	}

	this->QueuedPlayers(this->QueueLength) = Interlocutor;
	this->QueuedAddresses(this->QueueLength) = AddDynamicString(Text);
	this->QueueLength++;
}

void TNPC::TurnToInterlocutor() {
	TCreature *Opp = GetCreature(this->Interlocutor);
	if (Opp == nullptr) {
		error("TNPC::TurnToInterlocutor: Interlocutor does not exist.\n");
		return;
	}

	int dx = Opp->posx - this->posx;
	int dy = Opp->posy - this->posy;
	int Dir;
	if (abs(dx) < abs(dy)) {
		if (dy < 0) {
			Dir = DIRECTION_NORTH;
		} else {
			Dir = DIRECTION_SOUTH;
		}
	} else {
		if (dx < 0) {
			Dir = DIRECTION_WEST;
		} else {
			Dir = DIRECTION_EAST;
		}
	}
	this->Rotate((unsigned char)Dir);
}

void TNPC::ChangeState(STATE NewState, bool Stimulus) {
	this->State = NewState;
	if (Stimulus) {
		this->ToDoYield();
	}
}

void ChangeNPCState(TCreature *npc, int NewState, bool Stimulus) {
	if (npc == nullptr) {
		error("ChangeNPCState: npc is NULL.\n");
		return;
	}

	if (npc->Type != NPC) {
		error("ChangeNPCState: npc is not an NPC.\n");
		return;
	}

	((TNPC *)npc)->ChangeState((STATE)NewState, Stimulus);
}

TMonster::TMonster(int Race, int x, int y, int z, int Home, uint32_t Father) {
	this->Type = MONSTER;
	this->State = IDLE;
	this->Race = Race;
	this->startx = x;
	this->posx = x;
	this->starty = y;
	this->posy = y;
	this->startz = z;
	this->posz = z;
	this->Home = Home;
	this->Master = Father;
	this->Target = 0;

	while (this->Master != 0) {
		TCreature *master = GetCreature(this->Master);
		if (master == nullptr) {
			// Quirk: the constructor ends here and leaves the monster off the
			// lists and off the map.
			error("TMonster::TMonster: Master does not exist.\n");
			return;
		}

		if (master->Type == MONSTER && ((TMonster *)master)->Master != 0) {
			error("TMonster::TMonster: Children must not create children of their own.\n");
			this->Master = ((TMonster *)master)->Master;
			continue;
		}

		master->SummonedCreatures++;
		this->LifeEndRound = master->LifeEndRound;
		break;
	}

	if (RaceData[Race].Article[0] == 0) {
		strcpy(this->Name, RaceData[Race].Name);
	} else {
		snprintf(this->Name, 30, "%s %s", RaceData[Race].Article, RaceData[Race].Name);
	}
	this->SetSkills(Race);
	this->Outfit = RaceData[Race].Outfit;
	this->OrgOutfit = RaceData[Race].Outfit;
	this->SetID(0);
	this->SetInList();
	if (!this->SetOnMap()) {
		error("TMonster::TMonster: Cannot place monster on the map.\n");
		return;
	}

	if (this->Master == 0 && RaceData[Race].Items > 0) {
		try {
			Object Bag = Create(GetBodyContainer(this->ID, BODY_CONTAINER), GetSpecialObject(INVENTORY_CONTAINER), 0);
			// With config.toml's [loot] chance (the race's own or the global one).
			double Chance = GetLootChance(RaceData[Race].Key);
			for (int i = 1; i <= RaceData[Race].Items; i++) {
				TItemData &I = RaceData[Race].Item(i);
				if (random(0, 999) > ApplyLootChance(I.Probability, Chance)) {
					continue;
				}

				int Amount = random(1, I.Maximum);
				int Repeat = 1;
				if (!I.Type.getFlag(CUMULATIVE)) {
					Repeat = Amount;
					Amount = 0;
				}

				for (int j = 0; j < Repeat; j++) {
					Object Obj;
					if (I.Type.getFlag(WEAPON)
							|| I.Type.getFlag(SHIELD)
							|| I.Type.getFlag(BOW)
							|| I.Type.getFlag(THROW)
							|| I.Type.getFlag(WAND)
							|| I.Type.getFlag(WEAROUT)
							|| I.Type.getFlag(EXPIRE)
							|| I.Type.getFlag(EXPIRESTOP)) {
						try {
							Obj = Create(Bag, I.Type, 0);
						} catch (RESULT r) {
							error("TMonster::TMonster: Exception %d for race %d, maybe increase CarryStrength.\n", r, Race);
							break;
						}
					} else {
						Obj = CreateAtCreature(this->ID, I.Type, Amount);
					}

					if (Obj.getContainer().isMapContainer()) {
						error("TMonster::TMonster: Object falls onto the map. Increase CarryStrength for race %d.\n", Race);
						Delete(Obj, -1);
					}
				}
			}

			if (GetFirstContainerObject(Bag) == NONE) {
				Delete(Bag, -1);
			}
		} catch (RESULT r) {
			error("TMonster::TMonster: Exception %d for race %d.\n", r, Race);
		}
	}

	this->Combat.CheckCombatValues();
	this->Combat.SetAttackMode(ATTACK_MODE_BALANCED);
	this->ToDoWait(0);
	this->ToDoStart();
}

TMonster::~TMonster() {
	if (!this->IsDead) {
		GraphicalEffect(this->posx, this->posy, this->posz, EFFECT_POFF);
	} else if (this->Master == 0) {
		// [rates] experience multiplies what a slain monster gives.
		this->Combat.DistributeExperiencePoints(
				(uint32_t)MultiplyRate(RaceData[this->Race].ExperiencePoints, ExperienceRate));
	}

	if (this->Home != 0) {
		NotifyMonsterhomeOfDeath(this->Home);
	}

	if (this->Master != 0) {
		TCreature *master = GetCreature(this->Master);
		if (master != nullptr && master->SummonedCreatures > 0) {
			master->SummonedCreatures--;
		}
	}
}

void TMonster::IdleStimulus() {
	if (this->LockToDo) {
		return;
	}

	if (this->LoggingOut) {
		return;
	}

	if (this->LifeEndRound != 0 && RoundNr >= this->LifeEndRound) {
		print(3, "Lifetime of %s expired.\n", this->Name);
		this->StartLogout(true, true);
		this->State = SLEEPING;
		return;
	}

	MonsterThinkHook(RaceData[this->Race].Script, this->ID);

	if (this->Master != 0) {
		TCreature *master = GetCreature(this->Master);
		if (master == nullptr) {
			if (this->Master >= FIRST_NONPLAYER_CREATURE_ID) {
				print(3, "Creature %s loses its mother monster.\n", this->Name);
				this->Skills[SKILL_HITPOINTS]->Set(0);
				this->Death();
			} else {
				print(3, "Creature %s loses its player master.\n", this->Name);
				this->StartLogout(true, true);
			}
			this->State = SLEEPING;
			return;
		}

		if (this->Master < FIRST_NONPLAYER_CREATURE_ID && master->SummonedCreatures == 0) {
			print(3, "Player master has just logged in again.\n");
			this->StartLogout(true, true);
			this->State = SLEEPING;
			return;
		}

		if ((master != nullptr
					&& (abs(this->posz - master->posz) > 1
						|| abs(this->posx - master->posx) > 30
						|| abs(this->posy - master->posy) > 30))
				|| (this->posz != master->posz && this->Master >= FIRST_NONPLAYER_CREATURE_ID)) {
			if (this->Master < FIRST_NONPLAYER_CREATURE_ID) {
				print(3, "Player master is too far away.\n");
				this->StartLogout(true, true);
			} else {
				this->Skills[SKILL_HITPOINTS]->Set(0);
				this->Death();
				print(3, "Mother monster is too far away.\n");
			}
			this->State = SLEEPING;
			return;
		}

		uint32_t OldTarget = this->Target;
		if (master != nullptr && !master->Combat.IsFollowing()) {
			this->Target = master->Combat.GetAttackDest();
		} else {
			this->Target = 0;
		}
		if (this->Target == 0 || this->Target == this->ID) {
			this->Target = this->Master;
		}
		if (this->Target != 0 && this->Target != OldTarget) {
			MonsterTargetHook(RaceData[this->Race].Script, this->ID, this->Target);
		}
	} else if (!MonsterhomeInLureRange(this->Home, this->posx, this->posy, this->posz)) {
		print(3, "%s at [%d,%d,%d] too far from [%d,%d,%d].\n",
				this->Name, this->posx, this->posy, this->posz,
				Monsterhome(this->Home).x, Monsterhome(this->Home).y, Monsterhome(this->Home).z);
		this->StartLogout(true, true);
		this->State = SLEEPING;
		return;
	}

	if (this->Target != 0) {
		TCreature *target = GetCreature(this->Target);
		if (target == nullptr
				|| abs(this->posx - target->posx) > 10
				|| abs(this->posy - target->posy) > 10
				|| this->posz != target->posz) {
			this->Target = 0;
		}

		if (target != nullptr) {
			if (IsProtectionZone(target->posx, target->posy, target->posz)) {
				this->Target = 0;
			}
			if (IsHouse(target->posx, target->posy, target->posz)) {
				this->Target = 0;
			}
			if (!RaceData[this->Race].SeeInvisible && target->IsInvisible()) {
				this->Target = 0;
			}
		}

		if (this->Master == 0 && random(0, 99) < RaceData[this->Race].LoseTarget) {
			this->Target = 0;
		}
	}

	if (this->State != PANIC && this->State != UNDERATTACK) {
		this->State = IDLE;
	}

	if (RaceData[this->Race].Talks > 0 && rand() % 50 == 0) {
		int Nr = rand() % RaceData[this->Race].Talks + 1;
		char *Text = GetDynamicString(RaceData[this->Race].Talk(Nr));
		try {
			if (Text[0] == '#') {
				Talk(this->ID, TALK_ANIMAL_LOUD, nullptr, Text + 3, false);
			} else {
				Talk(this->ID, TALK_ANIMAL_LOW, nullptr, Text, false);
			}
		} catch (RESULT r) {
			error("TMonster::IdleStimulus: Exception %d at Talk.\n", r);
		}
	}

	if (this->Target == 0) {
		bool Sleep = true;
		if (this->Master == 0) {
			int Goodness = INT_MIN;
			int TieBreaker = 0;
			int Strategy = 0;
			int StrategySelection = random(0, 99);
			for (int i = 0; i < 3; i++) {
				if (StrategySelection < RaceData[this->Race].Strategy[i]) {
					break;
				}
				Strategy++;
				StrategySelection -= RaceData[this->Race].Strategy[i];
			}

			TFindCreatures Search(12, 12, this->ID, FIND_PLAYERS | FIND_MONSTERS);
			while (true) {
				uint32_t ID = Search.getNext();
				if (ID == 0) {
					break;
				}

				TCreature *cr = GetCreature(ID);
				if (cr == nullptr) {
					error("TMonster::IdleStimulus: Creature does not exist.\n");
					break;
				}

				if (cr->Type == MONSTER) {
					uint32_t master = ((TMonster *)cr)->Master;
					if (master == 0) {
						continue;
					}
					TCreature *help = GetCreature(master);
					if (help == nullptr || help->Type != PLAYER) {
						continue;
					}
				}

				if ((cr->posz <= 7 && this->posz <= 7) || (cr->posz > 7 && abs(this->posz - cr->posz) <= 2)) {
					Sleep = false;
				}

				if (this->posz != cr->posz) {
					continue;
				}

				if (cr->Type == PLAYER && CheckRight(cr->ID, IGNORED_BY_MONSTERS)) {
					continue;
				}

				if (!RaceData[this->Race].SeeInvisible && cr->IsInvisible()) {
					continue;
				}

				if (IsProtectionZone(cr->posx, cr->posy, cr->posz)) {
					continue;
				}

				if (IsHouse(cr->posx, cr->posy, cr->posz)) {
					continue;
				}

				int dx = abs(this->posx - cr->posx);
				int dy = abs(this->posy - cr->posy);
				if (dx > 10 || dy > 10) {
					continue;
				}

				int NewGoodness;
				switch (Strategy) {
					case STRATEGY_NEAREST:
						NewGoodness = -(dx + dy);
						break;
					case STRATEGY_WEAKEST:
						NewGoodness = -cr->Skills[SKILL_HITPOINTS]->Get();
						break;
					case STRATEGY_MOST_DAMAGE:
						NewGoodness = (int)this->Combat.GetDamageByCreature(cr->ID);
						break;
					case STRATEGY_RANDOM:
						NewGoodness = 0;
						break;
					default:
						NewGoodness = 0;
						error("TMonster::IdleStimulus: Unknown strategy %d.\n", Strategy);
						break;
				}

				int NewTieBreaker = random(0, 99);
				if (NewGoodness > Goodness || (NewGoodness == Goodness && NewTieBreaker > TieBreaker)) {
					this->Target = ID;
					Goodness = NewGoodness;
					TieBreaker = NewTieBreaker;
				}
			}

			// Target was 0 before the choice.
			if (this->Target != 0) {
				MonsterTargetHook(RaceData[this->Race].Script, this->ID, this->Target);
			}
		}

		if (this->Target == 0 && Sleep && this->State != UNDERATTACK && this->State != PANIC) {
			if (this->Master == 0) {
				this->State = SLEEPING;
				return;
			}
			this->ToDoWait(1000);
			this->ToDoStart();
			return;
		}

		if (this->Target == 0 && this->State == PANIC) {
			this->State = IDLE;
		}

		if (this->State == UNDERATTACK) {
			this->State = IDLE;
		}
	}

	if (RaceData[this->Race].Spells > 0) {
		CastMonsterSpells(this->ID, this->Race, this->Target);
	}

	if (this->Target != 0) {
		TCreature *target = GetCreature(this->Target);
		if (target == nullptr) {
			error("TMonster::IdleStimulus: Creature does not exist.\n");
			this->Target = 0;
			goto RandomMove;
		}

		try {
			if (this->Master == 0 && this->Skills[SKILL_HITPOINTS]->Get() <= RaceData[this->Race].FleeThreshold) {
				int x;
				int y;
				int z;
				if (!SearchFlightField(this->ID, this->Target, x, y, z)) {
					goto RandomMove;
				}
				this->ToDoGo(x, y, z, true, INT_MAX);
			} else if (this->Target == this->Master) {
				int dx = target->posx - this->posx;
				int dy = target->posy - this->posy;
				if (abs(dx) + abs(dy) <= 1) {
					goto RandomMove;
				}

				if (abs(dx) + abs(dy) == 2) {
					this->ToDoWait(1000);
				} else {
					if (abs(dx) + abs(dy) == 3) {
						this->ToDoWait(1000);
					}
					this->ToDoGo(target->posx, target->posy, target->posz, false, 3);
				}
				this->ToDoStart();
				return;
			} else {
				if (this->Skills[SKILL_FISTFIGHTING]->Get() > 0 && this->State != PANIC) {
					this->State = ATTACKING;
				}

				if (this->State == ATTACKING || this->State == PANIC) {
					this->Combat.SetAttackDest(this->Target, false);
					this->Combat.SetChaseMode(CHASE_MODE_STAND);
				}

				if (RaceData[this->Race].DistanceFighting
						&& this->Target != this->Master
						&& ThrowPossible(this->posx, this->posy, this->posz,
							target->posx, target->posy, target->posz, 0)) {
					int Distance = ObjectDistance(this->GetCreatureObject(), target->GetCreatureObject());
					if (Distance <= 3) {
						int x;
						int y;
						int z;
						if (SearchFlightField(this->ID, this->Target, x, y, z)) {
							this->ToDoGo(x, y, z, true, INT_MAX);
						} else {
							this->ToDoWait(1000);
						}
					} else if (Distance == 4) {
						int destX = this->posx;
						int destY = this->posy;
						int dir = rand() % 5;
						switch (dir) {
							case 0: destX--; break;
							case 1: destX++; break;
							case 2: destY--; break;
							case 3: destY++; break;
						}
						if (dir <= 3
								&& this->MovePossible(destX, destY, this->posz, true, false)
								&& std::max(abs(destX - target->posx), abs(destY - target->posy)) == 4) {
							this->ToDoGo(destX, destY, this->posz, true, INT_MAX);
						}
						this->ToDoWait(1000);
					} else {
						this->ToDoGo(target->posx, target->posy, target->posz, false, Distance - 4);
					}
				} else {
					this->Combat.SetChaseMode(CHASE_MODE_FOLLOW);
					if (abs(this->posx - target->posx) <= 1 && abs(this->posy - target->posy) <= 1) {
						int destX = this->posx;
						int destY = this->posy;
						int dir = rand() % 5;
						switch (dir) {
							case 0: destX--; break;
							case 1: destX++; break;
							case 2: destY--; break;
							case 3: destY++; break;
						}
						if (dir <= 3
								&& this->MovePossible(destX, destY, this->posz, true, false)
								&& abs(destX - target->posx) <= 1
								&& abs(destY - target->posy) <= 1) {
							this->ToDoGo(destX, destY, this->posz, true, INT_MAX);
						}
						if (this->State == PANIC) {
							this->State = ATTACKING;
						}
					} else if (this->State != ATTACKING && this->State != PANIC) {
						this->ToDoGo(target->posx, target->posy, target->posz, false, 3);
					}
				}

				if (this->State == ATTACKING || this->State == PANIC) {
					this->Rotate(GetCreature(this->Target));
					if (this->Master == 0 || this->Combat.GetAttackDest() != this->Master) {
						this->ToDoAttack();
					} else {
						error("TMonster::IdleStimulus: %s attacks master %lu (St=%d, T: %lu).\n",
								this->Name, (unsigned long)this->Combat.GetAttackDest(), this->State,
								(unsigned long)this->Target);
					}
				} else {
					this->ToDoWait(1000);
				}
			}
		} catch (RESULT r) {
			this->Target = 0;
			this->ToDoClear();
			if (r != EXHAUSTED) {
				goto RandomMove;
			}
			this->ToDoWait(1000);
			this->ToDoStart();
			return;
		}

		this->ToDoStart();
		return;
	}

RandomMove:
	{
		int Try = 0;
		int destX = this->posx;
		int destY = this->posy;
	Again:
		Try++;
		if (Try > 10) {
			this->ToDoWait(1000);
			this->ToDoStart();
			return;
		}

		int dir = rand() % 4;
		switch (dir) {
			case 0: destX--; break;
			case 1: destX++; break;
			case 2: destY--; break;
			case 3: destY++; break;
		}

		try {
			if (!this->MovePossible(destX, destY, this->posz, true, false)) {
				destX = this->posx;
				destY = this->posy;
				goto Again;
			}
		} catch (RESULT r) {
			destX = this->posx;
			destY = this->posy;
			goto Again;
		}

		try {
			this->ToDoGo(destX, destY, this->posz, true, INT_MAX);
			this->ToDoWait(1000);
			this->ToDoStart();
		} catch (RESULT r) {
			error("TMonster::IdleStimulus: Exception %d.\n", r);
		}
	}
}

void TMonster::CreatureMoveStimulus(uint32_t CreatureID, int Type) {
	if (CreatureID == this->ID) {
		if (this->State == SLEEPING && Type != OBJECT_DELETED) {
			this->State = IDLE;
			this->ToDoYield();
		}
		return;
	}

	if (this->State == SLEEPING && Type != OBJECT_DELETED) {
		TCreature *cr = GetCreature(CreatureID);
		if (cr == nullptr) {
			error("TMonster::CreatureMoveStimulus: Creature %d does not exist.\n", CreatureID);
			return;
		}

		if (cr->Type == NPC) {
			return;
		}

		if (cr->Type == MONSTER) {
			if (((TMonster *)cr)->Master == 0) {
				return;
			}
			TCreature *master = GetCreature(((TMonster *)cr)->Master);
			if (master == nullptr || master->Type != PLAYER) {
				return;
			}
		}

		this->State = IDLE;
		this->ToDoYield();
	}

	TCreature::CreatureMoveStimulus(CreatureID, Type);
}

void TMonster::DamageStimulus(uint32_t Attacker, int Damage, int) {
	MonsterHealthChangeHook(RaceData[this->Race].Script, this->ID, Attacker, Damage);

	if (Attacker == 0 || Damage == 0) {
		return;
	}

	if (this->State == SLEEPING) {
		this->State = (this->Target == 0 ? PANIC : UNDERATTACK);
		this->ToDoYield();
		return;
	}

	if (this->State == IDLE) {
		this->State = UNDERATTACK;
	}

	if (this->Target == 0) {
		this->State = PANIC;
	}
}

bool TMonster::MovePossible(int x, int y, int z, bool Execute, bool) {
	if (this->posz != z) {
		error("TMonster::MovePossible: Check across floors ([%d,%d,%d] -> [%d,%d,%d]).\n",
				this->posx, this->posy, this->posz, x, y, z);
		return false;
	}

	// Out of attack and panic a monster may only step into its home's area,
	// unless config.toml's [lure] is changed and it stands outside the area
	// (lured away): then it may walk anywhere.
	if (this->State != ATTACKING && this->State != PANIC && !MonsterhomeInRange(this->Home, x, y, z)
			&& (LureIsOriginal() || MonsterhomeInRange(this->Home, this->posx, this->posy, this->posz))) {
		return false;
	}

	if (this->State != ATTACKING && this->State != PANIC
			&& std::max(abs(x - this->startx), abs(y - this->starty)) > this->Radius) {
		return false;
	}

	if (this->Skills[SKILL_GOSTRENGTH]->GetAct() < 0) {
		error("TMonster::MovePossible: Monster %s at [%d,%d,%d] may not move.\n",
				this->Name, this->posx, this->posy, this->posz);
		return false;
	}

	if (IsProtectionZone(x, y, z)) {
		return false;
	}

	if (IsHouse(x, y, z)) {
		return false;
	}

	if (Execute && this->Master != 0 && this->State != ATTACKING && this->State != PANIC) {
		TCreature *master = GetCreature(this->Master);
		if (master != nullptr
				&& master->posz == this->posz
				&& abs(master->posx - this->posx) + abs(master->posy - this->posy) > 1
				&& abs(master->posx - x) + abs(master->posy - y) <= 1) {
			return false;
		}
	}

	int Try = 0;
Again:
	Try++;
	if (Try > 100) {
		error("TMonster::MovePossible: Endless loop suspected for %s at [%d,%d,%d].\n", this->Name, x, y, z);
		throw EXHAUSTED;
	}

	Object Obj = GetFirstObject(x, y, z);
	if (Obj == NONE || !Obj.getFlag(BANK)) {
		return false;
	}

	while (Obj != NONE) {
		if (Obj.isCreature()) {
			if (this->State != ATTACKING && this->State != PANIC) {
				return false;
			}

			if (this->Target == 0) {
				return false;
			}

			if (!RaceData[this->Race].KickCreatures) {
				return false;
			}

			TCreature *Obstacle = GetCreature(Obj);
			if (Obstacle == nullptr) {
				error("TMonster::MovePossible: Cannot identify obstacle creature.\n");
				return false;
			}

			if (RaceData[Obstacle->Race].Unpushable) {
				return false;
			}

			if (Obstacle->Type == NPC) {
				return false;
			}

			if (Obstacle->Type == PLAYER) {
				if (this->Master != 0) {
					return false;
				}
				if (CheckRight(Obstacle->ID, IGNORED_BY_MONSTERS)) {
					return false;
				}
			}

			if (!RaceData[this->Race].SeeInvisible && Obstacle->IsInvisible()) {
				return false;
			}

			if (Obstacle->ID == this->Master) {
				return false;
			}

			if (Obstacle->ID == this->Target) {
				return false;
			}

			if (Execute) {
				if (Obstacle->Type == PLAYER) {
					this->Target = 0;
					throw EXHAUSTED;
				}
				if (!this->KickCreature(Obstacle)) {
					throw EXHAUSTED;
				}
				goto Again;
			}
		} else {
			if (Obj.getFlag(UNPASS)) {
				if (Obj.getFlag(UNMOVE)) {
					return false;
				}
				if (!this->CanKickBoxes()) {
					return false;
				}
				if (Execute) {
					this->KickBoxes(Obj);
					goto Again;
				}
			}

			if (Obj.getFlag(AVOID)
					&& !(this->State == PANIC && Obj.getAttribute(AVOIDDAMAGETYPES) != 0)
					&& !(RaceData[this->Race].NoBurning && Obj.getAttribute(AVOIDDAMAGETYPES) == DAMAGE_FIRE)
					&& !(RaceData[this->Race].NoPoison && Obj.getAttribute(AVOIDDAMAGETYPES) == DAMAGE_POISON)
					&& !(RaceData[this->Race].NoEnergy && Obj.getAttribute(AVOIDDAMAGETYPES) == DAMAGE_ENERGY)) {
				if (Obj.getFlag(UNMOVE)) {
					return false;
				}
				if (!this->CanKickBoxes()) {
					return false;
				}
				if (Execute) {
					this->KickBoxes(Obj);
					goto Again;
				}
			}
		}

		Obj++;
	}

	return true;
}

bool TMonster::IsPeaceful() {
	return this->Master != 0 && this->Master < FIRST_NONPLAYER_CREATURE_ID;
}

bool TMonster::CanKickBoxes() {
	if (RaceData[this->Race].KickBoxes) {
		return true;
	}

	if (this->Master == 0) {
		return false;
	}

	TCreature *master = GetCreature(this->Master);
	if (master == nullptr || master->Type != MONSTER) {
		return false;
	}

	return ((TMonster *)master)->CanKickBoxes();
}

void TMonster::KickBoxes(Object Obj) {
	if (!Obj.exists()) {
		error("TMonster::KickBoxes: Given object does not exist.\n");
		return;
	}

	int x;
	int y;
	int z;
	GetObjectCoordinates(Obj, x, y, z);
	try {
		if (y - 1 != this->posy && ::MovePossible(x, y - 1, z)) {
			::Move(this->ID, Obj, GetMapContainer(x, y - 1, z), -1, false, NONE);
		} else if (y + 1 != this->posy && ::MovePossible(x, y + 1, z)) {
			::Move(this->ID, Obj, GetMapContainer(x, y + 1, z), -1, false, NONE);
		} else if (x - 1 != this->posx && ::MovePossible(x - 1, y, z)) {
			::Move(this->ID, Obj, GetMapContainer(x - 1, y, z), -1, false, NONE);
		} else if (x + 1 != this->posx && ::MovePossible(x + 1, y, z)) {
			::Move(this->ID, Obj, GetMapContainer(x + 1, y, z), -1, false, NONE);
		} else {
			GraphicalEffect(Obj, EFFECT_BLOCK_HIT);
			Delete(Obj, -1);
		}
	} catch (RESULT r) {
		error("TMonster::KickBoxes: Exception %d, object %d.\n", r, Obj.getObjectType().getTypeID());
		error("# own position: [%d,%d,%d] - object position: [%d,%d,%d]\n",
				this->posx, this->posy, this->posz, x, y, z);
	}
}

bool TMonster::KickCreature(TCreature *Obstacle) {
	if (Obstacle == nullptr) {
		error("TMonster::KickCreature: Given creature does not exist.\n");
		return false;
	}

	if (Obstacle->Type != MONSTER) {
		error("TMonster::KickCreature: Creature to be pushed is not a monster.\n");
		return false;
	}

	print(3, "%s pushes %s.\n", this->Name, Obstacle->Name);
	unsigned short HelpX = 0;
	unsigned short HelpY = 0;
	try {
		int x = Obstacle->posx;
		int y = Obstacle->posy;
		int z = Obstacle->posz;
		if (y - 1 != this->posy
				&& Obstacle->MovePossible(x, y - 1, z, true, false)
				&& !CoordinateAvoid(x, y - 1, z)) {
			HelpX = (unsigned short)x;
			HelpY = (unsigned short)(y - 1);
			::Move(this->ID, Obstacle->GetCreatureObject(), GetMapContainer(x, y - 1, z), -1, false, NONE);
			return true;
		}

		if (y + 1 != this->posy
				&& Obstacle->MovePossible(x, y + 1, z, true, false)
				&& !CoordinateAvoid(x, y + 1, z)) {
			HelpX = (unsigned short)x;
			HelpY = (unsigned short)(y + 1);
			::Move(this->ID, Obstacle->GetCreatureObject(), GetMapContainer(x, y + 1, z), -1, false, NONE);
			return true;
		}

		if (x - 1 != this->posx
				&& Obstacle->MovePossible(x - 1, y, z, true, false)
				&& !CoordinateAvoid(x - 1, y, z)) {
			HelpX = (unsigned short)(x - 1);
			HelpY = (unsigned short)y;
			::Move(this->ID, Obstacle->GetCreatureObject(), GetMapContainer(x - 1, y, z), -1, false, NONE);
			return true;
		}

		if (x + 1 != this->posx
				&& Obstacle->MovePossible(x + 1, y, z, true, false)
				&& !CoordinateAvoid(x + 1, y, z)) {
			HelpX = (unsigned short)(x + 1);
			HelpY = (unsigned short)y;
			::Move(this->ID, Obstacle->GetCreatureObject(), GetMapContainer(x + 1, y, z), -1, false, NONE);
			return true;
		}

		print(3, "No room to push => kill.\n");
		GraphicalEffect(Obstacle->GetCreatureObject(), EFFECT_BLOCK_HIT);
		Obstacle->Combat.AddDamageToCombatList(this->ID, (unsigned short)Obstacle->Skills[SKILL_HITPOINTS]->Get());
		Obstacle->Skills[SKILL_HITPOINTS]->Set(0);
		Obstacle->Death();
	} catch (RESULT r) {
		error("TMonster::KickCreature: Exception %d, creature %s.\n", r, Obstacle->Name);
		error("# own position: [%d,%d,%d] - obstacle position: [%d,%d,%d]\n",
				this->posx, this->posy, this->posz, Obstacle->posx, Obstacle->posy, Obstacle->posz);
		error("# Objects on target field [%d,%d,%d]:\n", HelpX, HelpY, Obstacle->posz);
		Object Obj = GetFirstObject(HelpX, HelpY, Obstacle->posz);
		while (Obj != NONE) {
			error("# %d\n", Obj.getObjectType().getTypeID());
			Obj++;
		}
	}

	return false;
}

void TMonster::Convince(TCreature *newMaster) {
	if (this->Home != 0) {
		NotifyMonsterhomeOfDeath(this->Home);
	}
	this->Home = 0;

	if (this->Master != 0) {
		TCreature *oldMaster = GetCreature(this->Master);
		if (oldMaster != nullptr) {
			oldMaster->SummonedCreatures--;
		}
	}

	this->Master = newMaster->ID;
	newMaster->SummonedCreatures++;
	this->ToDoClear();
	this->ToDoWait(100);
	this->ToDoStart();
}

void TMonster::SetTarget(TCreature *NewTarget) {
	if (NewTarget == nullptr) {
		error("TMonster::SetTarget: NewTarget is NULL.\n");
		return;
	}

	if (this->Master != 0) {
		return;
	}

	uint32_t OldTarget = this->Target;
	this->Rotate(NewTarget);
	this->Target = NewTarget->ID;
	this->ToDoYield();
	if (this->Target != OldTarget) {
		MonsterTargetHook(RaceData[this->Race].Script, this->ID, this->Target);
	}
}

uint32_t GetMonsterTarget(TCreature *Monster) {
	return static_cast<TMonster *>(Monster)->Target;
}

TCreature *CreateMonster(int Race, int x, int y, int z, int Home, uint32_t Father, bool ShowEffect) {
	if (Race < 1 || Race > MAX_RACES - 1) {
		error("CreateMonster: Invalid race number %d.\n", Race);
		return nullptr;
	}

	if (RaceData[Race].Name[0] == 0) {
		error("CreateMonster: Data for race %d not defined.\n", Race);
		return nullptr;
	}

	SearchFreeField(x, y, z, 2, 0, false);
	TMonster *cr = new TMonster(Race, x, y, z, Home, Father);
	if (ShowEffect) {
		GraphicalEffect(x, y, z, EFFECT_TELEPORT);
	}
	return cr;
}

void ConvinceMonster(TCreature *Master, TCreature *Slave) {
	if (Master == nullptr) {
		error("ConvinceMonster: Master does not exist.\n");
		return;
	}

	if (Slave == nullptr) {
		error("ConvinceMonster: Slave does not exist.\n");
		return;
	}

	if (Slave->Type != MONSTER) {
		error("ConvinceMonster: Slave is not a monster.\n");
		return;
	}

	((TMonster *)Slave)->Convince(Master);
}

void ChallengeMonster(TCreature *Challenger, TCreature *Monster) {
	if (Challenger == nullptr) {
		error("ChallengeMonster: Challenger does not exist.\n");
		return;
	}

	if (Monster == nullptr) {
		error("ChallengeMonster: Monster does not exist.\n");
		return;
	}

	if (Monster->Type != MONSTER) {
		error("ChallengeMonster: Monster is not a monster.\n");
		return;
	}

	((TMonster *)Monster)->SetTarget(Challenger);
}

// Makes the NPCs of <DATAPATH>/npcs.toml in the file's order (which gives
// their creature IDs). Throws what LoadNPCs and the constructor throw.
void InitNPCs() {
	char FileName[4096];
	snprintf(FileName, sizeof(FileName), "%s/npcs.toml", DATAPATH);
	std::vector<TNPCData> NPCs;
	LoadNPCs(FileName, NPCs);
	for (const TNPCData &Data : NPCs) {
		new TNPC(Data);
	}
}

// The NPC of the creature ID, nullptr when it is gone.
static TNPC *LivingNPC(uint32_t ID) {
	TCreature *cr = GetCreature(ID);
	return cr != nullptr && cr->Type == NPC ? static_cast<TNPC *>(cr) : nullptr;
}

// The NPC scripts part (reload.hh): each living NPC's script, loaded into the
// current engine in the order the NPCs were made, both handlers required as
// at start; finish gives each NPC its new script number. Nothing is read or
// installed (npcs.toml is read only at start; the NPCs keep their data and
// their conversation). The start loads the scripts in TNPC's constructor.
class TNPCScriptsPart : public TReloadPart {
public:
	const char *name() const override {
		return "NPC scripts";
	}

	void read() override {
	}

	void install() override {
	}

	void loadScripts() override {
		StagedScripts.assign(NPCScripts.size(), -1);
		Loaded = 0;
		for (size_t i = 0; i < NPCScripts.size(); i++) {
			const TNPCScriptRecord &Record = NPCScripts[i];
			if (LivingNPC(Record.ID) == nullptr) {
				continue;
			}
			int Script = LoadScript(Record.Script.c_str(), Record.Where.c_str());
			RequireHandler(Script, "onCreatureSay", Record.Where.c_str());
			RequireHandler(Script, "onCreatureDisappear", Record.Where.c_str());
			StagedScripts[i] = Script;
			Loaded++;
		}
	}

	void finish() override {
		for (size_t i = 0; i < NPCScripts.size() && i < StagedScripts.size(); i++) {
			TNPC *npc = LivingNPC(NPCScripts[i].ID);
			if (StagedScripts[i] != -1 && npc != nullptr) {
				npc->ScriptNumber = StagedScripts[i];
			}
		}
		StagedScripts.clear();
	}

	void uninstall() override {
	}

	void discard() override {
		StagedScripts.clear();
	}

	std::string summary() const override {
		char Text[100];
		snprintf(Text, sizeof(Text), "NPC scripts: %d NPCs", Loaded);
		return Text;
	}

private:
	std::vector<int> StagedScripts;
	int Loaded = 0;
};

static TNPCScriptsPart NPCScriptsPart;

TReloadPart &NpcScriptsReloadPart() {
	return NPCScriptsPart;
}

void InitNonplayer() {
	print(1, "Initializing Nonplayers ...\n");
	FirstFreeNonplayer = 0;
	InitNPCs();
	LoadMonsterhomes();
}

void ExitNonplayer() {
	// Quirk: a NULL entry at position 0 loops forever.
	while (FirstFreeNonplayer > 0) {
		TNonplayer *np = NonplayerList(0);
		if (np != nullptr) {
			delete np;
		}
	}
	NPCScripts.clear();
}
