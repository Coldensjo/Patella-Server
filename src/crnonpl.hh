// crnonpl.hh: the functions and data of crnonpl.cc, the creatures that are
// not players: NPCs, monsters and the monster homes that spawn monsters. The
// NPCs' class is here for the bindings of their scripts (scripting_npcs.cc).

#ifndef PATELLA_CRNONPL_HH
#define PATELLA_CRNONPL_HH

#include <cstdint>

#include "containr.hh"
#include "cr.hh"

struct TMonsterhome;
struct TNPCData;

// The states of a nonplayer (TNonplayer::State). An NPC sleeps with no
// player near, is IDLE (walks) when free, TALKING in a conversation and
// LEAVING while it says the last texts of one; a monster also is
// UNDERATTACK, ATTACKING and in PANIC.
enum STATE {
	SLEEPING = 0,
	IDLE = 1,
	UNDERATTACK = 2,
	TALKING = 3,
	LEAVING = 4,
	ATTACKING = 5,
	PANIC = 6,
};

// The situations in which an NPC's script is called (scripting.hh, "NPCs"):
// a talk to the NPC while it is free (ADDRESS), by its interlocutor
// (DEFAULT) or by another player while it is busy (BUSY), the greeting of
// the first player of its queue (ADDRESSQUEUE), and the interlocutor gone
// (VANISH, onCreatureDisappear).
enum SITUATION {
	DEFAULT = 0,
	ADDRESS = 1,
	ADDRESSQUEUE = 2,
	BUSY = 3,
	VANISH = 4,
};

struct TNonplayer : public TCreature {
	TNonplayer();
	virtual ~TNonplayer();
	void SetInList();
	void DelInList();
protected:
	STATE State;
};

// An NPC, made from its entry of npcs.toml. Its script decides what it says
// and does in a conversation (onCreatureSay, onCreatureDisappear); the
// stimuli, the walking, the queue of players waiting and the trade helpers
// are here. The bindings of the script read and set the conversation and
// call the helpers through the public members.
struct TNPC : public TNonplayer {
	TNPC(const TNPCData &Entry);
	virtual void TalkStimulus(uint32_t Speaker, char *Text);
	virtual void IdleStimulus();
	virtual void CreatureMoveStimulus(uint32_t CreatureID, int Type);
	virtual bool MovePossible(int x, int y, int z, bool, bool);
	void ChangeState(STATE NewState, bool Stimulus);
	STATE GetState() const { return this->State; }
	void GiveTo(ObjectType Type, int Amount);
	void GetFrom(ObjectType Type, int Amount);
	void GiveMoney(int Amount);
	void GetMoney(int Amount);
	void Enqueue(uint32_t Interlocutor, char *Text);
	// The conversation: the player the NPC talks to, the topic, price,
	// amount, object type and data its rules set, and the round of the last
	// talk (the interlocutor's, or the end of the NPC's answer).
	uint32_t Interlocutor;
	int Topic;
	int Price;
	int Amount;
	int TypeID;
	uint32_t Data;
	uint32_t LastTalk;
private:
	void TurnToInterlocutor();
	void CallCreatureSay(char *Text, SITUATION Situation);
	void CallCreatureDisappear();
	vector<uint32_t> QueuedPlayers;
	vector<uint32_t> QueuedAddresses;
	int QueueLength;
	// The NPC's script (LoadScript's number).
	int ScriptNumber;
};

void StartMonsterhomeTimer(int nr);
void LoadMonsterhomes();
void ProcessMonsterhomes();
void NotifyMonsterhomeOfDeath(int nr);
bool MonsterhomeInRange(int nr, int x, int y, int z);
void ChangeNPCState(TCreature *npc, int NewState, bool Stimulus);
TCreature *CreateMonster(int Race, int x, int y, int z, int Home, uint32_t Father, bool ShowEffect);
void ConvinceMonster(TCreature *Master, TCreature *Slave);
void ChallengeMonster(TCreature *Challenger, TCreature *Monster);
// The Target of a monster (Type MONSTER; not checked), its raw creature ID (0
// for none), for monster:getTargetId (scripting_monsters.cc).
uint32_t GetMonsterTarget(TCreature *Monster);
void InitNPCs();
void InitNonplayer();
void ExitNonplayer();

extern vector<TNonplayer *> NonplayerList;
extern int FirstFreeNonplayer;
extern vector<TMonsterhome> Monsterhome;
extern int Monsterhomes;

#endif
