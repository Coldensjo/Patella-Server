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
#include "crinc.hh"
#include "connections.hh"
#include "operate.hh"
#include "utils.hh"
#include "time.hh"
#include "communication.hh"
#include "TXTEASymmetricKey.hh"
#include "crnonpl.hh"

struct TNPC;

enum STATE {
	SLEEPING = 0,
	IDLE = 1,
	UNDERATTACK = 2,
	TALKING = 3,
	LEAVING = 4,
	ATTACKING = 5,
	PANIC = 6,
};

enum SITUATION {
	DEFAULT = 0,
	ADDRESS = 1,
	ADDRESSQUEUE = 2,
	BUSY = 3,
	VANISH = 4,
};

struct TNode {
	int Type;
	int Data;
	TNode *Left;
	TNode *Right;
};

struct TCondition {
	// inline  crnonpl.c:56
	TCondition() { abort(); /* STUB */ }
	bool set(int Type, void *Data);
	void clear();
	int Type;
	uint32_t Text;
	TNode *Expression;
	int Property;
	int Number;
};

struct TAction {
	// inline  crnonpl.c:68
	TAction() { abort(); /* STUB */ }
	bool set(int Type, void *Data, void *Data2, void *Data3, void *Data4);
	void clear();
	int Type;
	uint32_t Text;
	int Number;
	TNode *Expression;
	TNode *Expression2;
	TNode *Expression3;
};

struct TBehaviour {
	TBehaviour();
	~TBehaviour();
	bool addCondition(int Type, void *Data);
	bool addAction(int Type, void *Data, void *Data2, void *Data3, void *Data4);
	vector<TCondition> Condition;
	vector<TAction> Action;
	int Conditions;
	int Actions;
};

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

struct TBehaviourDatabase {
	TBehaviourDatabase(TReadScriptFile &Script);
	void react(TNPC *npc, char *Text, SITUATION Situation);
private:
	vector<TBehaviour> Behaviour;
	int Behaviours;
	TNode *readTerm(TReadScriptFile &Script);
	TNode *readFactor(TReadScriptFile &Script);
	TNode *readValue(TReadScriptFile &Script);
	int evaluate(TNPC *npc, TNode *Node, int *Number);
};

struct TNonplayer : public TCreature {
	TNonplayer();
	virtual ~TNonplayer();
	void SetInList();
	void DelInList();
protected:
	STATE State;
};

struct TNPC : public TNonplayer {
	TNPC(char *FileName);
	virtual ~TNPC();
	virtual void TalkStimulus(uint32_t Speaker, char *Text);
	virtual void DamageStimulus(uint32_t, int, int);
	virtual void IdleStimulus();
	virtual void CreatureMoveStimulus(uint32_t CreatureID, int Type);
	virtual bool MovePossible(int x, int y, int z, bool, bool);
	void ChangeState(STATE NewState, bool Stimulus);
private:
	void GiveTo(ObjectType Type, int Amount);
	void GetFrom(ObjectType Type, int Amount);
	void GiveMoney(int Amount);
	void GetMoney(int Amount);
	void Enqueue(uint32_t Interlocutor, char *Text);
	void TurnToInterlocutor();
	uint32_t Interlocutor;
	int Topic;
	int Price;
	int Amount;
	int TypeID;
	uint32_t Data;
	uint32_t LastTalk;
	vector<uint32_t> QueuedPlayers;
	vector<uint32_t> QueuedAddresses;
	int QueueLength;
	TBehaviourDatabase *Behaviour;
};

struct TMonster : public TNonplayer {
	TMonster(int Race, int x, int y, int z, int Home, uint32_t Father);
	virtual ~TMonster();
	virtual void DamageStimulus(uint32_t Attacker, int Damage, int);
	virtual void IdleStimulus();
	virtual void CreatureMoveStimulus(uint32_t CreatureID, int Type);
	virtual bool MovePossible(int x, int y, int z, bool Execute, bool);
	virtual bool IsPeaceful();
	// 0x080bc810  crnonpl.c:161
	virtual uint32_t GetMaster() { abort(); /* STUB */ }
	void Convince(TCreature *newMaster);
	void SetTarget(TCreature *NewTarget);
private:
	bool CanKickBoxes();
	void KickBoxes(Object Obj);
	bool KickCreature(TCreature *Obstacle);
	int Home;
	uint32_t Master;
	uint32_t Target;
};

vector<TNonplayer *> NonplayerList(0, 10000, 1000, nullptr);	// crnonpl.c:174
int FirstFreeNonplayer;	// crnonpl.c:175
int CurrentNonplayer;	// crnonpl.c:176
vector<TMonsterhome> Monsterhome(1, 5000, 1000);	// crnonpl.c:177
int Monsterhomes;	// crnonpl.c:178
store<TNode, 256> *NodeTable;	// crnonpl.c:179

// 0x080afff0  crnonpl.c:190
TNode *NewNode(int /*op*/, TNode * /*left*/, TNode * /*right*/) { abort(); /* STUB */ }

// 0x080b0020  crnonpl.c:202
TNode *NewNode(int /*type*/, int /*val*/) { abort(); /* STUB */ }

// 0x080b0050  crnonpl.c:215
void DeleteNode(TNode * /*Node*/) { abort(); /* STUB */ }

// 0x080b0090  crnonpl.c:226
bool TCondition::set(int /*Type*/, void * /*Data*/) { abort(); /* STUB */ }

// 0x080b02c0  crnonpl.c:268
void TCondition::clear() { abort(); /* STUB */ }

// 0x080b0330  crnonpl.c:278
bool TAction::set(int /*Type*/, void * /*Data*/, void * /*Data2*/, void * /*Data3*/, void * /*Data4*/) { abort(); /* STUB */ }

// 0x080b03d0  crnonpl.c:314
void TAction::clear() { abort(); /* STUB */ }

// 0x080b0560  crnonpl.c:323
TBehaviour::TBehaviour() : Condition(0, 5, 5), Action(0, 5, 5) { abort(); /* STUB */ }

// 0x080b0680  crnonpl.c:335
TBehaviour::~TBehaviour() { abort(); /* STUB */ }

// 0x080b0710  crnonpl.c:344
bool TBehaviour::addCondition(int /*Type*/, void * /*Data*/) { abort(); /* STUB */ }

// 0x080b0760  crnonpl.c:355
bool TBehaviour::addAction(int /*Type*/, void * /*Data*/, void * /*Data2*/, void * /*Data3*/, void * /*Data4*/) { abort(); /* STUB */ }

// 0x080b1d50  crnonpl.c:363
TBehaviourDatabase::TBehaviourDatabase(TReadScriptFile & /*Script*/) : Behaviour(0, 50, 25) { abort(); /* STUB */ }

// 0x080b32d0  crnonpl.c:501
TNode *TBehaviourDatabase::readValue(TReadScriptFile & /*Script*/) { abort(); /* STUB */ }

// 0x080b3e60  crnonpl.c:573
TNode *TBehaviourDatabase::readFactor(TReadScriptFile & /*Script*/) { abort(); /* STUB */ }

// 0x080b3ef0  crnonpl.c:588
TNode *TBehaviourDatabase::readTerm(TReadScriptFile & /*Script*/) { abort(); /* STUB */ }

// 0x080b4060  crnonpl.c:604
int TBehaviourDatabase::evaluate(TNPC * /*npc*/, TNode * /*Node*/, int * /*Number*/) { abort(); /* STUB */ }

// 0x080b4590  crnonpl.c:672
void TBehaviourDatabase::react(TNPC * /*npc*/, char * /*Text*/, SITUATION /*Situation*/) { abort(); /* STUB */ }

// 0x080b56e0  crnonpl.c:900
void StartMonsterhomeTimer(int /*nr*/) { abort(); /* STUB */ }

// 0x080b5800  crnonpl.c:929
void LoadMonsterhomes() { abort(); /* STUB */ }

// 0x080b5c50  crnonpl.c:978
void ProcessMonsterhomes() { abort(); /* STUB */ }

// 0x080b5f70  crnonpl.c:1018
void NotifyMonsterhomeOfDeath(int /*nr*/) { abort(); /* STUB */ }

// 0x080b6010  crnonpl.c:1040
bool MonsterhomeInRange(int /*nr*/, int /*x*/, int /*y*/, int /*z*/) { abort(); /* STUB */ }

// 0x080b60f0  crnonpl.c:1062
TNonplayer::TNonplayer() { abort(); /* STUB */ }

// 0x080b61d0  crnonpl.c:1070
TNonplayer::~TNonplayer() { abort(); /* STUB */ }

// 0x080b6330  crnonpl.c:1078
void TNonplayer::SetInList() { abort(); /* STUB */ }

// 0x080b6370  crnonpl.c:1088
void TNonplayer::DelInList() { abort(); /* STUB */ }

// 0x080b6af0  crnonpl.c:1104
TNPC::TNPC(char * /*FileName*/) : QueuedPlayers(0, 9, 10), QueuedAddresses(0, 9, 10) { abort(); /* STUB */ }

// 0x080b7280  crnonpl.c:1191
TNPC::~TNPC() { abort(); /* STUB */ }

// 0x080b7400  crnonpl.c:1202
void TNPC::TalkStimulus(uint32_t /*Speaker*/, char * /*Text*/) { abort(); /* STUB */ }

// 0x080b75a0  crnonpl.c:1232
void TNPC::IdleStimulus() { abort(); /* STUB */ }

// 0x080b79f0  crnonpl.c:1306
void TNPC::CreatureMoveStimulus(uint32_t /*CreatureID*/, int /*Type*/) { abort(); /* STUB */ }

// 0x080b7d80  crnonpl.c:1349
void TNPC::DamageStimulus(uint32_t, int, int) { abort(); /* STUB */ }

// 0x080b7d90  crnonpl.c:1356
bool TNPC::MovePossible(int /*x*/, int /*y*/, int /*z*/, bool, bool) { abort(); /* STUB */ }

// 0x080b7ec0  crnonpl.c:1369
void TNPC::GiveTo(ObjectType /*Type*/, int /*Amount*/) { abort(); /* STUB */ }

// 0x080b80d0  crnonpl.c:1391
void TNPC::GetFrom(ObjectType /*Type*/, int /*Amount*/) { abort(); /* STUB */ }

// 0x080b81d0  crnonpl.c:1405
void TNPC::GiveMoney(int /*Amount*/) { abort(); /* STUB */ }

// 0x080b82b0  crnonpl.c:1419
void TNPC::GetMoney(int /*Amount*/) { abort(); /* STUB */ }

// 0x080b8470  crnonpl.c:1437
void TNPC::Enqueue(uint32_t /*Interlocutor*/, char * /*Text*/) { abort(); /* STUB */ }

// 0x080b8540  crnonpl.c:1457
void TNPC::TurnToInterlocutor() { abort(); /* STUB */ }

// 0x080b85e0  crnonpl.c:1475
void TNPC::ChangeState(STATE /*NewState*/, bool /*Stimulus*/) { abort(); /* STUB */ }

// 0x080b8600  crnonpl.c:1484
void ChangeNPCState(TCreature * /*npc*/, int /*NewState*/, bool /*Stimulus*/) { abort(); /* STUB */ }

// 0x080b8c70  crnonpl.c:1506
TMonster::TMonster(int /*Race*/, int /*x*/, int /*y*/, int /*z*/, int /*Home*/, uint32_t /*Father*/) { abort(); /* STUB */ }

// 0x080b9370  crnonpl.c:1582
TMonster::~TMonster() { abort(); /* STUB */ }

// 0x080b9530  crnonpl.c:1600
void TMonster::IdleStimulus() { abort(); /* STUB */ }

// 0x080baef0  crnonpl.c:1935
void TMonster::CreatureMoveStimulus(uint32_t /*CreatureID*/, int /*Type*/) { abort(); /* STUB */ }

// 0x080baff0  crnonpl.c:1965
void TMonster::DamageStimulus(uint32_t /*Attacker*/, int /*Damage*/, int) { abort(); /* STUB */ }

// 0x080bb070  crnonpl.c:1982
bool TMonster::MovePossible(int /*x*/, int /*y*/, int /*z*/, bool /*Execute*/, bool) { abort(); /* STUB */ }

// 0x080bb870  crnonpl.c:2057
bool TMonster::IsPeaceful() { abort(); /* STUB */ }

// 0x080bb890  crnonpl.c:2067
bool TMonster::CanKickBoxes() { abort(); /* STUB */ }

// 0x080bb900  crnonpl.c:2079
void TMonster::KickBoxes(Object /*Obj*/) { abort(); /* STUB */ }

// 0x080bbd20  crnonpl.c:2104
bool TMonster::KickCreature(TCreature * /*Obstacle*/) { abort(); /* STUB */ }

// 0x080bc230  crnonpl.c:2146
void TMonster::Convince(TCreature * /*newMaster*/) { abort(); /* STUB */ }

// 0x080bc2c0  crnonpl.c:2164
void TMonster::SetTarget(TCreature * /*NewTarget*/) { abort(); /* STUB */ }

// 0x080bc330  crnonpl.c:2183
TCreature *CreateMonster(int /*Race*/, int /*x*/, int /*y*/, int /*z*/, int /*Home*/, uint32_t /*Father*/, bool /*ShowEffect*/) { abort(); /* STUB */ }

// 0x080bc450  crnonpl.c:2203
void ConvinceMonster(TCreature * /*Master*/, TCreature * /*Slave*/) { abort(); /* STUB */ }

// 0x080bc4a0  crnonpl.c:2224
void ChallengeMonster(TCreature * /*Challenger*/, TCreature * /*Monster*/) { abort(); /* STUB */ }

// 0x080bc530  crnonpl.c:2251
void InitNPCs() { abort(); /* STUB */ }

// 0x080bc660  crnonpl.c:2278
void InitNonplayer() { abort(); /* STUB */ }

// 0x080bc720  crnonpl.c:2292
void ExitNonplayer() { abort(); /* STUB */ }
