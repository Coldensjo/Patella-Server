#ifndef PATELLA_CR_HH
#define PATELLA_CR_HH

#include <cstdint>
#include <cstdlib>

#include "map.hh"
#include "dbfuncs.hh"
#include "containr.hh"
#include "info.hh"
#include "time.hh"

struct TConnection;
struct TCreature;
struct TKnownCreature;

enum CreatureType {
	PLAYER = 0,
	MONSTER = 1,
	NPC = 2,
};

enum ToDoType {
	TDWait = 0,
	TDGo = 1,
	TDRotate = 2,
	TDMove = 3,
	TDTrade = 4,
	TDUse = 5,
	TDTurn = 6,
	TDAttack = 7,
	TDTalk = 8,
	TDChangeState = 9,
};

struct TCombatEntry {
	uint32_t ID;
	uint32_t Damage;
	int TimeStamp;
};

struct TToDoEntry {
	ToDoType Code;
	union {
		struct {
			uint32_t Time;
		} Wait;
		struct {
			int x;
			int y;
			int z;
		} Go;
		struct {
			int Direction;
		} Rotate;
		struct {
			uint32_t Obj;
			int x;
			int y;
			int z;
			int Count;
		} Move;
		struct {
			uint32_t Obj;
			uint32_t Partner;
		} Trade;
		struct {
			uint32_t Obj1;
			uint32_t Obj2;
			int Dummy;
		} Use;
		struct {
			uint32_t Obj;
		} Turn;
		struct {
			uint32_t Text;
			int Mode;
			uint32_t Addressee;
			bool CheckSpamming;
		} Talk;
		struct {
			int NewState;
		} ChangeState;
	};
};

struct TFindCreatures {
	TFindCreatures(int radx, int rady, int x, int y, int Mask);
	TFindCreatures(int radx, int rady, uint32_t CreatureID, int Mask);
	TFindCreatures(int radx, int rady, Object Obj, int Mask);
	uint32_t getNext();
private:
	void initSearch(int radx, int rady, int x, int y, int mask);
	int startx;
	int starty;
	int endx;
	int endy;
	int blockx;
	int blocky;
	uint32_t ActID;
	uint32_t SkipID;
	int Mask;
	bool finished;
};

struct TCombat {
	TCombat();
	// inline  cr.h:160
	void SetMaster(TCreature *cr) { Master = cr; }
	// inline  cr.h:163
	bool AttackReady() { return ServerMilliseconds >= EarliestAttackTime; }
	// inline  cr.h:164
	bool DefendReady() { return ServerMilliseconds >= EarliestDefendTime; }
	// inline  cr.h:165
	int GetAttackDelay() { return (int)(EarliestAttackTime - ServerMilliseconds); }
	void DelayAttack(int Milliseconds);
	void CanToDoAttack();
	int GetDistance();
	void Attack();
	void SetAttackDest(uint32_t dfd, bool Follow);
	// inline  cr.h:175
	uint32_t GetAttackDest() { return AttackDest; }
	void SetAttackMode(unsigned char b);
	unsigned char GetAttackMode();
	void SetChaseMode(unsigned char b);
	// inline  cr.h:179
	unsigned char GetChaseMode() { return ChaseMode; }
	void SetSecureMode(unsigned char b);
	// inline  cr.h:181
	unsigned char GetSecureMode() { return SecureMode; }
	// inline  cr.h:182
	bool IsFollowing() { return Following; }
	void StopAttack(int Delay);
	void CheckCombatValues();
	unsigned short GetArmorStrength();
	void AddDamageToCombatList(uint32_t atk, unsigned short Damage);
	void DistributeExperiencePoints(uint32_t Exp);
	uint32_t GetDamageByCreature(uint32_t CreatureID);
	uint32_t GetMostDangerousAttacker();
	short GetAttackDamage();
	short GetDefendDamage();
private:
	TCreature *Master;
	uint32_t EarliestAttackTime;
	uint32_t EarliestDefendTime;
	uint32_t LastDefendTime;
	uint32_t LatestAttackTime;
	int TypeToSkill(int type);
	void GetWeapon();
	void GetAmmo();
	unsigned char AttackMode;
	unsigned char ChaseMode;
	unsigned char SecureMode;
	uint32_t AttackDest;
	bool Following;
	Object Shield;
	Object Close;
	Object Missile;
	Object Throw;
	Object Wand;
	Object Ammo;
	bool Fist;
	void CloseAttack(TCreature *dfd);
	void RangeAttack(TCreature *dfd);
	void GetAttackValue(short &Value, unsigned short &Skill);
	void GetDefendValue(short &Value, unsigned short &Skill);
	uint32_t CombatDamage;
	int ActCombatEntry;
	TCombatEntry CombatList[20];
	int LearningPoints;
	void ActivateLearning();
};

struct TSkill {
	TSkill(int nr, TCreature *cr);
	virtual ~TSkill();
	// inline  cr.h:236
	TCreature *GetMaster() { return Master; }
	void Load(int act, int max, int min, int dact, int mdact, int cyc, int maxcyc, int cnt, int maxcnt, int addlvl, int exp, int fp, int nextlvl, int delta);
	void Save(int &act, int &max, int &min, int &dact, int &mdact, int &cyc, int &maxcyc, int &cnt, int &maxcnt, int &addlvl, int &exp, int &fp, int &nextlvl, int &delta);
	// inline  cr.h:241
	void SetAct(int value) { Act = value; }
	// inline  cr.h:242
	void SetMax(int value) { Max = value; }
	// inline  cr.h:243
	void SetMin(int value) { Min = value; }
	// inline  cr.h:244
	void SetDAct(int value) { DAct = value; }
	void SetMDAct(int value);
	// inline  cr.h:246
	void SetAddLevel(int value) { AddLevel = value; }
	// inline  cr.h:247
	void SetExp(int value) { Exp = value; }
	// inline  cr.h:248
	void SetFactorPercent(int value) { FactorPercent = value; }
	// inline  cr.h:249
	void SetLastLevel(int value) { LastLevel = value; }
	// inline  cr.h:250
	void SetNextLevel(int value) { NextLevel = value; }
	// inline  cr.h:251
	void SetDelta(int value) { Delta = value; }
	// inline  cr.h:252
	void SetMaxCount(int value) { MaxCount = value; }
	void SetCount(int);
	// inline  cr.h:256
	int GetMax() { return Max; }
	int GetMin();
	// inline  cr.h:258
	int GetAct() { return Act; }
	// inline  cr.h:259
	int GetDAct() { return DAct; }
	// inline  cr.h:260
	int GetMDAct() { return MDAct; }
	// inline  cr.h:261
	int GetExp() { return Exp; }
	// inline  cr.h:262
	unsigned short GetSkNr() { return SkNr; }
	int Get();
	int GetProgress();
	virtual void Set(int a);
	void Change(int a);
	void Check();
	// 0x080caa50  cr.h:273
	virtual void Increase(int) {}
	// 0x080caa60  cr.h:274
	virtual void Decrease(int) {}
	// 0x080caa70  cr.h:275
	virtual int GetExpForLevel(int) { return 0; }
	// 0x080caa80  cr.h:276
	virtual void Advance(int) {}
	// 0x080caa90  cr.h:277
	virtual void ChangeSkill(int, int) {}
	// 0x080caaa0  cr.h:280
	virtual int ProbeValue(int, bool) { return 0; }
	// 0x080caab0  cr.h:281
	virtual bool Probe(int, int, bool) { return false; }
	virtual bool Process();
	virtual bool SetTimer(int cyc, int cou, int maxcou, int);
	virtual bool DelTimer();
	// 0x080caac0  cr.h:287
	virtual int TimerValue() { return Cycle; }
protected:
	virtual bool Jump(int);
	virtual void Event(int);
	virtual void Reset();
	int DAct;
	int MDAct;
private:
	unsigned short SkNr;
protected:
	TCreature *Master;
	int Act;
	int Max;
	int Min;
	int FactorPercent;
	int LastLevel;
	int NextLevel;
	int Delta;
	int Exp;
	int Cycle;
	int MaxCycle;
	int Count;
	int MaxCount;
	int AddLevel;
};

struct TSkillBase {
	TSkillBase();
	~TSkillBase();
	TSkill *Skills[25];
	bool SetTimer(unsigned short SkNr, int cyc, int cou, int maxcou, int additionalvalue);
	void DelTimer(unsigned short SkNr);
	void ProcessSkills();
	// inline  cr.h:327
	bool TimerActive() { return FirstFreeTimer != 0; }
	bool NewSkill(unsigned short SkillNo, TCreature *cr);
	bool SetSkills(int Race);
private:
	TSkill *TimerList[25];
	unsigned short FirstFreeTimer;
};

struct TCreature : public TSkillBase {
	TCreature();
	// Exceptions other than RESULT leave the original destructor; a C++17
	// destructor would end the process there unless it says otherwise.
	virtual ~TCreature() noexcept(false);
	int GetHealth();
	int GetSpeed();
	int Damage(TCreature *atk, int Damage, int DamageType);
	virtual void Death();
	TCombat Combat;
	// inline  cr.h:351
	bool IsInvisible() { return Outfit.OutfitID == 0 && Outfit.ObjectType == 0; }
	virtual bool MovePossible(int x, int y, int z, bool Execute, bool Jump);
	// 0x080a53a0  cr.h:353
	virtual bool IsPeaceful() { return true; }
	// 0x080a53b0  cr.h:354
	virtual uint32_t GetMaster() { return 0; }
	void StartLogout(bool Force, bool StopFight);
	int LogoutPossible();
	void BlockLogout(int Delay, bool BlockProtectionZone);
	// inline  cr.h:359
	bool IsLoggingOut() { return LoggingOut; }
	// inline  cr.h:361
	TConnection *GetConnection() { return Connection; }
	// inline  cr.h:362
	void SetConnection(TConnection *_Connection) { Connection = _Connection; }
	void NotifyCreate();
	void NotifyDelete();
	void NotifyGo();
	void NotifyTurn(Object DestCon);
	void NotifyChangeInventory();
	int CalculateDelay();
	void Execute();
	void ToDoAdd(TToDoEntry &TD);
	void ToDoStart();
	bool ToDoClear();
	void ToDoStop();
	void ToDoYield();
	void ToDoWait(int Delay);
	void ToDoTalk(int Mode, char *Addressee, char *Text, bool CheckSpamming);
	void ToDoGo(int DestX, int DestY, int DestZ, bool Dest, int MaxSteps);
	void ToDoRotate(int Direction);
	void ToDoMove(int sx, int sy, int sz, ObjectType Type, unsigned char RNum, int zx, int zy, int zz, unsigned char Count);
	void ToDoMove(Object Obj, int zx, int zy, int zz, unsigned char Count);
	void ToDoTrade(int x, int y, int z, ObjectType Type, unsigned char RNum, uint32_t TradePartner);
	void ToDoUse(unsigned char Count, int x1, int y1, int z1, ObjectType Type1, unsigned char RNum1, unsigned char Dummy, int x2, int y2, int z2, ObjectType Type2, unsigned char RNum2);
	void ToDoUse(unsigned char Count, Object Obj1, Object Obj2);
	void ToDoTurn(int x, int y, int z, ObjectType Type, unsigned char RNum);
	void ToDoAttack();
	void ToDoChangeState(int NewState);
	// 0x080a53c0  cr.h:393
	virtual void TalkStimulus(uint32_t, char *) {}
	// 0x080a53d0  cr.h:394
	virtual void DamageStimulus(uint32_t, int, int) {}
	// 0x080a53e0  cr.h:395
	virtual void IdleStimulus() {}
	virtual void CreatureMoveStimulus(uint32_t CreatureID, int Type);
	// 0x080a53f0  cr.h:397
	virtual void AttackStimulus(uint32_t) {}
	// inline  cr.h:399
	Object GetCreatureObject() { return CrObject; }
	// inline  cr.h:400
	void SetCreatureObject(Object Obj) { CrObject = Obj; }
	uint32_t ID;
	TCreature *NextHashEntry;
	uint32_t NextChainCreature;
	char Name[31];
	char Murderer[31];
	TOutfit OrgOutfit;
	TOutfit Outfit;
	int startx;
	int starty;
	int startz;
	int posx;
	int posy;
	int posz;
	int Sex;
	int Race;
	int Direction;
	int Radius;
	CreatureType Type;
	bool IsDead;
	int LoseInventory;
	bool LoggingOut;
	bool LogoutAllowed;
	uint32_t EarliestLogoutRound;
	uint32_t EarliestProtectionZoneRound;
	uint32_t EarliestYellRound;
	uint32_t EarliestTradeChannelRound;
	uint32_t EarliestSpellTime;
	uint32_t EarliestMultiuseTime;
	uint32_t EarliestWalkTime;
	uint32_t LifeEndRound;
	TKnownCreature *FirstKnowingConnection;
	int SummonedCreatures;
	uint32_t FireDamageOrigin;
	uint32_t PoisonDamageOrigin;
	uint32_t EnergyDamageOrigin;
protected:
	void Go(int x, int y, int z);
	void Move(Object Obj, int zx, int zy, int zz, unsigned char Count);
	void Use(Object Obj1, Object Obj2, unsigned char Dummy);
	void Turn(Object Obj);
	void Trade(Object Obj, uint32_t PartnerID);
	void Rotate(unsigned char Dir);
	void Rotate(TCreature *Target);
	void Attack();
	void SetInCrList();
	void DelInCrList();
private:
	Object CrObject;
	vector<TToDoEntry> ToDoList;
	int ActToDo;
	int NrToDo;
	uint32_t NextWakeup;
	bool Stop;
protected:
	bool SetOnMap();
	bool DelOnMap();
	void SetID(uint32_t CharacterID);
	void DelID();
	bool LockToDo;
	unsigned char Profession;
	TConnection *Connection;
};

struct TPlayer : public TCreature {
	TPlayer(TConnection *Connection, uint32_t CharacterID);
	virtual ~TPlayer();
	void TakeOver(TConnection *Connection);
	virtual void Death();
	virtual void AttackStimulus(uint32_t);
	virtual void IdleStimulus();
	virtual void DamageStimulus(uint32_t, int, int);
	virtual bool MovePossible(int x, int y, int z, bool Execute, bool Jump);
	void ClearProfession();
	void SetProfession(unsigned char prof);
	unsigned char GetRealProfession();
	unsigned char GetEffectiveProfession();
	unsigned char GetActiveProfession();
	bool GetActivePromotion();
	void SetOpenContainer(int Window, Object Con);
	Object GetOpenContainer(int Window);
	void CloseAllContainers();
	void AcceptTrade();
	void RejectTrade();
	Object InspectTrade(bool OwnOffer, int Position);
	void CheckState();
	void ClearConnection();
	void LoadData();
	void SaveData();
	void LoadInventory(bool SetStandardInventory);
	void SaveInventory();
	void StartCoordinates();
	void SetInList();
	void DelInList();
	void CheckOutfit();
	void AddBuddy(char *Name);
	void RemoveBuddy(uint32_t CharacterID);
	void SendBuddies();
	bool SpellKnown(int SpellNumber);
	void LearnSpell(int SpellNumber);
	int GetQuestValue(int Number);
	void SetQuestValue(int Number, int Value);
	bool IsAttacker(uint32_t Victim, bool CheckFormer);
	bool IsAggressor(bool CheckFormer);
	// inline  cr.h:512
	bool IsPlayerkiller() { return PlayerData->PlayerkillerEnd != 0; }
	bool IsAttackJustified(uint32_t Victim);
	void RecordAttack(uint32_t Victim);
	void RecordMurder(uint32_t Victim);
	int CheckPlayerkilling();
	void ClearAttacker(uint32_t Victim);
	void ClearPlayerkillingMarks();
	int GetPlayerkillingMark(TPlayer *Observer);
	uint32_t GetPartyLeader(bool CheckFormer);
	void JoinParty(uint32_t Leader);
	void LeaveParty();
	int GetPartyMark(TPlayer *Observer);
	int RecordTalk();
	int RecordMessage(uint32_t Addressee);
	int CheckForMuting();
private:
	void Regenerate();
public:
	uint32_t AccountID;
	char Guild[31];
	char Rank[31];
	char Title[31];
	char IPAddress[16];
	unsigned char Rights[12];
	Object Depot;
	int DepotNr;
	int DepotSpace;
	RESULT ConstructError;
	TPlayerData *PlayerData;
	Object TradeObject;
	uint32_t TradePartner;
	bool TradeAccepted;
	int OldState;
	uint32_t Request;
	int RequestTimestamp;
	uint32_t RequestProcessingGamemaster;
	int TutorActivities;
private:
	unsigned char SpellList[256];
	int QuestValues[500];
	Object OpenContainer[16];
	vector<uint32_t> AttackedPlayers;
	int NumberOfAttackedPlayers;
	bool Aggressor;
	vector<uint32_t> FormerAttackedPlayers;
	int NumberOfFormerAttackedPlayers;
	bool FormerAggressor;
	uint32_t FormerLogoutRound;
	uint32_t PartyLeader;
	uint32_t PartyLeavingRound;
	uint32_t TalkBufferFullTime;
	uint32_t MutingEndRound;
	int NumberOfMutings;
	uint32_t Addressees[20];
	uint32_t AddresseesTimes[20];
};

#endif
