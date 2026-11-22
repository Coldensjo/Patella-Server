// crskill.cc: the skills of the creatures. TSkillBase holds a creature's
// skills and the timers among them; the classes below are the kinds of skill:
// the level (TSkillLevel), the skills raised by use (TSkillProbe), the values
// with a maximum (TSkillAdd: hit points, mana, ...) and the timed conditions
// (poison, burning, light, ...). Also the experience curves.

#include <cmath>
#include <cstdlib>
#include <cstring>

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
#include "crmain.hh"
#include "operate.hh"
#include "sending.hh"
#include "utils.hh"
#include "time.hh"
#include "crskill.hh"
#include "scripting.hh"

struct TSkillLevel : public TSkill {
	TSkillLevel(int nr, TCreature *cr) : TSkill(nr, cr) {}
	virtual void Increase(int Amount);
	virtual void Decrease(int Amount);
	virtual int GetExpForLevel(int Level);
private:
	virtual bool Jump(int Range);
};

struct TSkillProbe : public TSkill {
	TSkillProbe(int nr, TCreature *cr) : TSkill(nr, cr) {}
	virtual void Increase(int Amount);
	virtual void Decrease(int Amount);
	virtual int GetExpForLevel(int Level);
	virtual int ProbeValue(int max, bool _Increase);
	virtual bool Probe(int Diff, int Prob, bool _Increase);
	virtual void ChangeSkill(int fp, int delta);
	virtual bool SetTimer(int cyc, int cou, int maxcou, int);
private:
	virtual void Event(int);
	virtual bool Jump(int Range);
	int TriesPerTest();
};

struct TSkillAdd : public TSkill {
	TSkillAdd(int nr, TCreature *cr) : TSkill(nr, cr) {}
	virtual void Advance(int Range);
};

struct TSkillPoison : public TSkill {
	TSkillPoison(int nr, TCreature *cr) : TSkill(nr, cr) {}
	virtual bool SetTimer(int cyc, int cou, int maxcou, int additionalvalue);
	virtual bool Process();
	virtual void Reset();
private:
	virtual void Event(int Range);
};

struct TSkillBurning : public TSkill {
	TSkillBurning(int nr, TCreature *cr) : TSkill(nr, cr) {}
private:
	virtual void Event(int);
};

struct TSkillEnergy : public TSkill {
	TSkillEnergy(int nr, TCreature *cr) : TSkill(nr, cr) {}
private:
	virtual void Event(int);
};

struct TSkillFed : public TSkill {
	TSkillFed(int nr, TCreature *cr) : TSkill(nr, cr) {}
private:
	virtual void Event(int);
};

struct TSkillIllusion : public TSkill {
	TSkillIllusion(int nr, TCreature *cr) : TSkill(nr, cr) {}
	virtual bool SetTimer(int cyc, int cou, int maxcou, int);
private:
	virtual void Event(int);
};

struct TSkillLight : public TSkill {
	TSkillLight(int nr, TCreature *cr) : TSkill(nr, cr) {}
	virtual bool SetTimer(int cyc, int cou, int maxcou, int);
private:
	virtual void Event(int);
};

struct TSkillHitpoints : public TSkillAdd {
	TSkillHitpoints(int nr, TCreature *cr) : TSkillAdd(nr, cr) {}
	virtual void Set(int a);
};

struct TSkillMana : public TSkillAdd {
	TSkillMana(int nr, TCreature *cr) : TSkillAdd(nr, cr) {}
	virtual void Set(int a);
};

struct TSkillSoulpoints : public TSkillAdd {
	TSkillSoulpoints(int nr, TCreature *cr) : TSkillAdd(nr, cr) {}
	virtual void Set(int a);
	virtual int TimerValue() { return (Cycle - 1) * MaxCount + Count; }
	virtual void Event(int);
};

struct TSkillGoStrength : public TSkillAdd {
	TSkillGoStrength(int nr, TCreature *cr) : TSkillAdd(nr, cr) {}
	virtual bool SetTimer(int cyc, int cou, int maxcou, int);
private:
	virtual void Event(int);
};

struct TSkillCarryStrength : public TSkillAdd {
	TSkillCarryStrength(int nr, TCreature *cr) : TSkillAdd(nr, cr) {}
	virtual void Set(int a);
};

// pow(double, int) as libstdc++ 3.2 computes it (std::__cmath_power<double>),
// which gives the skill thresholds. Today's pow(double, int) is libm's
// pow(double, double) and rounds differently, which would move them. __x and
// __y are kept in x87 extended precision and the result is returned
// unrounded, hence long double (the 80-bit x87 type on x86-64 Linux).
static long double __cmath_power(double __x_, unsigned int __n) {
	long double __x = __x_;
	long double __y = __n % 2 ? __x : 1;
	while (__n >>= 1) {
		__x = __x * __x;
		if (__n % 2) {
			__y = __y * __x;
		}
	}
	return __y;
}

// std::__pow_helper<double> of libstdc++ 3.2, the body of its pow(double, int).
static long double __pow_helper(double __x, int __n) {
	return __n < 0
		? 1.0L / __cmath_power(__x, -__n)
		: __cmath_power(__x, __n);
}

int GetSkillByName(char *Name) {
	if (strcmp(Name, "level") == 0) {
		return SKILL_LEVEL;
	}
	if (strcmp(Name, "magiclevel") == 0) {
		return SKILL_MAGICLEVEL;
	}
	if (strcmp(Name, "hitpoints") == 0) {
		return SKILL_HITPOINTS;
	}
	if (strcmp(Name, "mana") == 0) {
		return SKILL_MANA;
	}
	if (strcmp(Name, "gostrength") == 0) {
		return SKILL_GOSTRENGTH;
	}
	if (strcmp(Name, "carrystrength") == 0) {
		return SKILL_CARRYSTRENGTH;
	}
	if (strcmp(Name, "shielding") == 0) {
		return SKILL_SHIELDING;
	}
	if (strcmp(Name, "distancefighting") == 0) {
		return SKILL_DISTANCEFIGHTING;
	}
	if (strcmp(Name, "swordfighting") == 0) {
		return SKILL_SWORDFIGHTING;
	}
	if (strcmp(Name, "clubfighting") == 0) {
		return SKILL_CLUBFIGHTING;
	}
	if (strcmp(Name, "axefighting") == 0) {
		return SKILL_AXEFIGHTING;
	}
	if (strcmp(Name, "fistfighting") == 0) {
		return SKILL_FISTFIGHTING;
	}
	if (strcmp(Name, "magicdefense") == 0) {
		return SKILL_MAGICDEFENSE;
	}
	if (strcmp(Name, "fishing") == 0) {
		return SKILL_FISHING;
	}
	if (strcmp(Name, "eating") == 0) {
		return SKILL_EATING;
	}
	if (strcmp(Name, "shining") == 0) {
		return SKILL_SHINING;
	}
	if (strcmp(Name, "illusion") == 0) {
		return SKILL_ILLUSION;
	}
	if (strcmp(Name, "poison") == 0) {
		return SKILL_POISON;
	}
	if (strcmp(Name, "burning") == 0) {
		return SKILL_BURNING;
	}
	if (strcmp(Name, "energy") == 0) {
		return SKILL_ENERGY;
	}
	if (strcmp(Name, "drunken") == 0) {
		return SKILL_DRUNKEN;
	}
	if (strcmp(Name, "manashield") == 0) {
		return SKILL_MANASHIELD;
	}
	if (strcmp(Name, "soulpoints") == 0) {
		return SKILL_SOULPOINTS;
	}
	return -1;
}

TSkillBase::TSkillBase() {
	for (int i = 0; i < MAX_SKILLS; i++) {
		Skills[i] = nullptr;
		TimerList[i] = nullptr;
	}
	FirstFreeTimer = 0;
}

TSkillBase::~TSkillBase() {
	for (int i = 0; i < MAX_SKILLS; i++) {
		delete Skills[i];
	}
}

void TSkillBase::DelTimer(unsigned short SkNr) {
	if (SkNr >= MAX_SKILLS) {
		error("TSkillBase::DelTimer: Invalid SkNr: %d\n", SkNr);
		return;
	}

	TSkill *sk = Skills[SkNr];
	if (sk == nullptr) {
		return;
	}

	sk->DelTimer();
	for (int i = 0; i < FirstFreeTimer; i++) {
		if (TimerList[i] == sk) {
			TimerList[i] = TimerList[FirstFreeTimer - 1];
			FirstFreeTimer--;
			break;
		}
	}
}

bool TSkillBase::SetTimer(unsigned short SkNr, int cyc, int cou, int maxcou, int additionalvalue) {
	if (SkNr >= MAX_SKILLS) {
		error("TSkillBase::SetTimer: Invalid SkNr: %d\n", SkNr);
		return false;
	}

	TSkill *sk = Skills[SkNr];
	DelTimer(SkNr);
	bool result = sk->SetTimer(cyc, cou, maxcou, additionalvalue);
	if (result) {
		TimerList[FirstFreeTimer] = sk;
		FirstFreeTimer++;
	}
	return result;
}

void TSkillBase::ProcessSkills() {
	int i = 0;
	while (i < FirstFreeTimer) {
		TSkill *sk = TimerList[i];
		if (sk == nullptr) {
			i++;
		} else if (sk->Process()) {
			FirstFreeTimer--;
			TimerList[i] = TimerList[FirstFreeTimer];
			TimerList[FirstFreeTimer] = nullptr;
		} else {
			i++;
		}
	}
}

bool TSkillBase::NewSkill(unsigned short SkillNo, TCreature *cr) {
	if (SkillNo >= MAX_SKILLS) {
		error("TSkillBase::NewSkill: Unknown skill number %d\n", SkillNo);
		return false;
	}

	if (Skills[SkillNo] != nullptr) {
		delete Skills[SkillNo];
		Skills[SkillNo] = nullptr;
	}

	switch (SkillNo) {
		case SKILL_LEVEL:
			Skills[SkillNo] = new TSkillLevel(SkillNo, cr);
			break;
		case SKILL_MAGICLEVEL:
		case SKILL_SHIELDING:
		case SKILL_DISTANCEFIGHTING:
		case SKILL_SWORDFIGHTING:
		case SKILL_CLUBFIGHTING:
		case SKILL_AXEFIGHTING:
		case SKILL_FISTFIGHTING:
		case SKILL_FISHING:
			Skills[SkillNo] = new TSkillProbe(SkillNo, cr);
			break;
		case SKILL_HITPOINTS:
			Skills[SkillNo] = new TSkillHitpoints(SkillNo, cr);
			break;
		case SKILL_MANA:
			Skills[SkillNo] = new TSkillMana(SkillNo, cr);
			break;
		case SKILL_GOSTRENGTH:
			Skills[SkillNo] = new TSkillGoStrength(SkillNo, cr);
			break;
		case SKILL_CARRYSTRENGTH:
			Skills[SkillNo] = new TSkillCarryStrength(SkillNo, cr);
			break;
		case SKILL_EATING:
			Skills[SkillNo] = new TSkillFed(SkillNo, cr);
			break;
		case SKILL_SHINING:
			Skills[SkillNo] = new TSkillLight(SkillNo, cr);
			break;
		case SKILL_ILLUSION:
			Skills[SkillNo] = new TSkillIllusion(SkillNo, cr);
			break;
		case SKILL_POISON:
			Skills[SkillNo] = new TSkillPoison(SkillNo, cr);
			break;
		case SKILL_BURNING:
			Skills[SkillNo] = new TSkillBurning(SkillNo, cr);
			break;
		case SKILL_ENERGY:
			Skills[SkillNo] = new TSkillEnergy(SkillNo, cr);
			break;
		case SKILL_SOULPOINTS:
			Skills[SkillNo] = new TSkillSoulpoints(SkillNo, cr);
			break;
		default:
			Skills[SkillNo] = new TSkill(SkillNo, cr);
			break;
	}
	return true;
}

bool TSkillBase::SetSkills(int Race) {
	if (Race < 1 || Race > MAX_RACES - 1) {
		error("TSkillBase::SetSkills: Invalid race number %d.\n", Race);
		return false;
	}

	for (int i = 1; i <= RaceData[Race].Skills; i++) {
		TSkillData &S = RaceData[Race].Skill(i);
		TSkill *sk = Skills[S.Nr];
		sk->SetAct(S.Actual);
		sk->SetMin(S.Minimum);
		sk->SetMax(S.Maximum);
		sk->SetLastLevel(0);
		sk->SetNextLevel(S.NextLevel);
		sk->SetDelta(S.NextLevel);
		sk->SetFactorPercent(S.FactorPercent);
		sk->SetAddLevel(S.AddLevel);
		sk->SetMaxCount(0);
	}
	return true;
}

TSkill::TSkill(int nr, TCreature *cr) {
	SkNr = nr;
	Reset();
	Master = cr;
}

TSkill::~TSkill() {
	if (Master != nullptr) {
		Master->DelTimer(SkNr);
	}
}

void TSkill::Reset() {
	Act = 0;
	Max = 0x7fffffff;
	Min = 0;
	DAct = 0;
	MDAct = 0;
	FactorPercent = 1000;
	LastLevel = 0;
	NextLevel = 0x7fffffff;
	Exp = 0;
	Delta = 0x7fffffff;
	Cycle = 0;
	MaxCycle = 0;
	Count = 0;
	MaxCount = 0;
	AddLevel = 0;
}

void TSkill::Load(int act, int max, int min, int dact, int mdact, int cyc, int maxcyc, int cnt, int maxcnt, int addlvl, int exp, int fp, int nextlvl, int delta) {
	Act = act;
	Max = max;
	Min = min;
	DAct = dact;
	MDAct = mdact;
	Cycle = cyc;
	MaxCycle = maxcyc;
	Count = cnt;
	MaxCount = maxcnt;
	AddLevel = addlvl;
	Exp = exp;
	FactorPercent = fp;
	NextLevel = nextlvl;
	Delta = delta;
	LastLevel = GetExpForLevel(act);
	if (Master != nullptr && Cycle != 0) {
		Master->SetTimer(SkNr, Cycle, Count, MaxCount, FactorPercent);
	}
}

void TSkill::Save(int &act, int &max, int &min, int &dact, int &mdact, int &cyc, int &maxcyc, int &cnt, int &maxcnt, int &addlvl, int &exp, int &fp, int &nextlvl, int &delta) {
	act = Act;
	max = Max;
	min = Min;
	dact = DAct;
	mdact = MDAct;
	cyc = Cycle;
	maxcyc = MaxCycle;
	cnt = Count;
	maxcnt = MaxCount;
	addlvl = AddLevel;
	exp = Exp;
	fp = FactorPercent;
	nextlvl = NextLevel;
	delta = Delta;
}

void TSkill::Event(int) {
}

bool TSkill::Jump(int) {
	return true;
}

int TSkill::Get() {
	int a = Act;
	if (a < Min) {
		a = Min;
	}
	return a + MDAct + DAct;
}

int TSkill::GetProgress() {
	if (NextLevel <= LastLevel) {
		return 0;
	}

	long long ExpGained = Exp - LastLevel;
	long long ExpNecessary = NextLevel - LastLevel;
	int Percent = (int)(ExpGained * 100 / ExpNecessary);
	if (!in(Percent, 0, 99)) {
		error("TSkill::GetProgress: Calculation error Exp %d, Last %d, Next %d, Percent %d.\n",
				Exp, LastLevel, NextLevel, Percent);
		error("# Player %s - Skill %d\n", Master != nullptr ? Master->Name : "(Unknown", SkNr);
		Percent = 0;
	}
	return Percent;
}

void TSkill::SetMDAct(int value) {
	MDAct = value;
	if (SkNr == SKILL_GOSTRENGTH && Master != nullptr && Master->Type == PLAYER) {
		static_cast<TPlayer *>(Master)->CheckState();
	}
}

void TSkill::Set(int a) {
	Act = a;
	Check();
}

void TSkill::Change(int a) {
	Set(GetAct() + a);
	Check();
}

void TSkill::Check() {
	if (Act > Max) {
		Act = Max;
	}
}

bool TSkill::Process() {
	if (Cycle == 0) {
		if (Master != nullptr && Master->Type == PLAYER) {
			static_cast<TPlayer *>(Master)->CheckState();
		}
		return true;
	}

	if (Count > 0) {
		Count--;
		return false;
	}

	Count = MaxCount;
	int Range = Cycle > 0 ? -1 : 1;
	Cycle += Range;
	Event(Range);
	return false;
}

bool TSkill::SetTimer(int cyc, int cou, int maxcou, int) {
	Cycle = cyc;
	Count = cou;
	MaxCount = maxcou;
	if (Master != nullptr && Master->Type == PLAYER) {
		static_cast<TPlayer *>(Master)->CheckState();
	}
	return true;
}

bool TSkill::DelTimer() {
	Cycle = 0;
	Count = 0;
	MaxCount = 0;
	if (Master != nullptr && Master->Type == PLAYER) {
		static_cast<TPlayer *>(Master)->CheckState();
	}
	return true;
}

int TSkillLevel::GetExpForLevel(int Level) {
	if (Level < 1) {
		error("TSkillLevel::GetExpForLevel: Invalid level %d.\n", Level);
		return 0;
	}

	if (Delta < 1) {
		error("TSkillLevel::GetExpForLevel: Invalid delta value %d.\n", Delta);
		return 0;
	}

	if (Level > 500) {
		error("TSkillLevel::GetExpForLevel: Level=%d; guard the formula against overflow.\n", Level);
		return -1;
	}

	return (((Level - 6) * Level + 17) * Level - 12) / 6 * Delta;
}

void TSkillLevel::Increase(int Amount) {
	if (Amount < 0) {
		error("TSkillLevel::Increase: Amount negative (%d).\n", Amount);
		return;
	}

	int Range = 0;
	Exp += Amount;
	while (Exp >= NextLevel) {
		Act++;
		NextLevel = GetExpForLevel(Act + 1);
		LastLevel = GetExpForLevel(Act);
		if (NextLevel < 0) {
			error("TSkillLevel::Increase: Skill about to overflow (%s, Skill %d).\n",
					GetMaster()->Name, GetSkNr());
			NextLevel = Exp;
			Exp -= 1000000;
			break;
		}
		Range++;
	}

	if (Range != 0) {
		Jump(Range);
	}

	TCreature *cr = GetMaster();
	if (cr == nullptr) {
		error("TSkillLevel::Increase: GetMaster returns NULL.\n");
		return;
	}

	if (cr->Type == PLAYER) {
		SendPlayerData(cr->GetConnection());
	}
}

void TSkillLevel::Decrease(int Amount) {
	if (Amount < 0) {
		error("TSkillLevel::Decrease: Amount negative (%d).\n", Amount);
		return;
	}

	if (Amount > Exp && Exp > 100000) {
		error("TSkillLevel::Decrease: Amount too large (%d).\n", Amount);
		return;
	}

	int Range = 0;
	if (Exp > Amount) {
		Exp -= Amount;
	} else {
		Exp = 0;
	}

	while (true) {
		int PreviousLevel = GetExpForLevel(Act);
		if (Exp >= PreviousLevel) {
			break;
		}
		NextLevel = PreviousLevel;
		Range--;
		Act--;
		LastLevel = GetExpForLevel(Act);
	}

	if (Range != 0) {
		Jump(Range);
	}

	TCreature *cr = GetMaster();
	if (cr == nullptr) {
		error("TSkillLevel::Decrease: GetMaster returns NULL.\n");
		return;
	}

	if (cr->Type == PLAYER) {
		SendPlayerData(cr->GetConnection());
	}
}

bool TSkillLevel::Jump(int Range) {
	TCreature *cr = GetMaster();
	if (cr == nullptr) {
		error("TSkillLevel::Jump: GetMaster returns NULL!\n");
		return false;
	}

	TSkill *sk = cr->Skills[SKILL_HITPOINTS];
	sk->Advance(Range);
	sk = cr->Skills[SKILL_MANA];
	sk->Advance(Range);
	sk = cr->Skills[SKILL_CARRYSTRENGTH];
	sk->Advance(Range);
	sk = cr->Skills[SKILL_GOSTRENGTH];
	sk->Advance(Range);

	AnnounceChangedCreature(cr->ID, CREATURE_SPEED);
	cr->Combat.CheckCombatValues();

	int OldLevel = Get() - Range;
	if (Range > 0) {
		if (cr->Type == PLAYER) {
			SendMessage(cr->GetConnection(), TALK_EVENT_MESSAGE,
					"You advanced from Level %d to Level %d.", OldLevel, Get());
			EventAdvance(cr->ID, GetSkNr(), OldLevel, Get());
		}
	} else {
		if (cr->Type == PLAYER) {
			SendMessage(cr->GetConnection(), TALK_EVENT_MESSAGE,
					"You were downgraded from Level %d to Level %d.", OldLevel, Get());
		}
	}
	return true;
}

bool TSkillProbe::SetTimer(int cyc, int cou, int maxcou, int) {
	TSkill::SetTimer(cyc, cou, maxcou, -1);

	TCreature *cr = GetMaster();
	if (cr == nullptr) {
		error("TSkillProbe::SetTimer: GetMaster returns NULL!\n");
		return false;
	}

	if (cr->Type == PLAYER) {
		SendPlayerData(cr->GetConnection());
	}
	return true;
}

void TSkillProbe::Event(int) {
	if (Cycle == 0) {
		TCreature *cr = GetMaster();
		if (cr == nullptr) {
			error("TSkillProbe::Event: GetMaster returns NULL!\n");
			return;
		}

		SetMDAct(0);
		if (cr->Type == PLAYER) {
			SendPlayerSkills(cr->GetConnection());
		}
	}
}

bool TSkillProbe::Jump(int Range) {
	TCreature *cr = GetMaster();
	if (cr == nullptr) {
		error("TSkillProbe::Jump: GetMaster returns NULL!\n");
		return false;
	}

	if (cr->Type == PLAYER) {
		if (GetSkNr() == SKILL_MAGICLEVEL) {
			SendPlayerData(cr->GetConnection());
		} else {
			SendPlayerSkills(cr->GetConnection());
		}

		if (Range >= 0) {
			switch (GetSkNr()) {
				case SKILL_MAGICLEVEL:
					SendMessage(cr->GetConnection(), TALK_EVENT_MESSAGE,
							"You advanced to magic level %d.", Get());
					break;
				case SKILL_SHIELDING:
					SendMessage(cr->GetConnection(), TALK_EVENT_MESSAGE,
							"You advanced in shielding.");
					break;
				case SKILL_DISTANCEFIGHTING:
					SendMessage(cr->GetConnection(), TALK_EVENT_MESSAGE,
							"You advanced in distance fighting.");
					break;
				case SKILL_SWORDFIGHTING:
					SendMessage(cr->GetConnection(), TALK_EVENT_MESSAGE,
							"You advanced in sword fighting.");
					break;
				case SKILL_CLUBFIGHTING:
					SendMessage(cr->GetConnection(), TALK_EVENT_MESSAGE,
							"You advanced in club fighting.");
					break;
				case SKILL_AXEFIGHTING:
					SendMessage(cr->GetConnection(), TALK_EVENT_MESSAGE,
							"You advanced in axe fighting.");
					break;
				case SKILL_FISTFIGHTING:
					SendMessage(cr->GetConnection(), TALK_EVENT_MESSAGE,
							"You advanced in fist fighting.");
					break;
				case SKILL_FISHING:
					SendMessage(cr->GetConnection(), TALK_EVENT_MESSAGE,
							"You advanced in fishing.");
					break;
			}
			// Every TSkillProbe is one of the skills above (TSkillBase::
			// NewSkill), so each advance here was told to the player.
			EventAdvance(cr->ID, GetSkNr(), Get() - Range, Get());
		}
	}
	return true;
}

int TSkillProbe::GetExpForLevel(int Level) {
	if (Level < 0 || Level < Min) {
		error("TSkillProbe::GetExpForLevel: Invalid level %d.\n", Level);
		return 0;
	}

	if (Delta <= 0) {
		error("TSkillProbe::GetExpForLevel: Invalid delta value %d.\n", Delta);
		return 0;
	}

	if (FactorPercent < 1000) {
		error("TSkillProbe::GetExpForLevel: Invalid FactorPercent value %d for %s.\n",
				FactorPercent, GetMaster() != nullptr ? GetMaster()->Name : "---");
	}

	if (FactorPercent > 1000 && FactorPercent < 1050) {
		error("TSkillProbe::GetExpForLevel: Invalid FactorPercent value %d. Continuing with 1000.\n",
				FactorPercent);
	}

	if (FactorPercent < 1050) {
		return (Level - Min) * Delta;
	}

	// Computed in x87 extended precision and rounded to double only at FP and
	// at the result of floor(), which keeps the skill thresholds. A result
	// outside of int (or NaN) converts to INT_MIN, as x87's fistp does.
	double FP = (double)((long double)FactorPercent / 1000.0L);
	return (int)(double)floorl((__pow_helper(FP, Level - Min) - 1.0L) / (FP - 1.0L) * Delta);
}

void TSkillProbe::Increase(int Amount) {
	if (Amount < 0) {
		error("TSkillProbe::Increase: Amount negative (%d).\n", Amount);
		return;
	}

	int Range = 0;
	int OldProgress = GetProgress();
	Exp += Amount;
	while (Exp >= NextLevel) {
		Act++;
		NextLevel = GetExpForLevel(Act + 1);
		LastLevel = GetExpForLevel(Act);
		if (NextLevel < 0) {
			error("TSkillProbe::Increase: Skill about to overflow (%s, Skill %d).\n",
					GetMaster()->Name, GetSkNr());
			NextLevel = Exp;
			Exp -= 1000;
			break;
		}
		Range++;
	}

	if (Range != 0) {
		Jump(Range);
	} else {
		TCreature *cr = GetMaster();
		if (cr != nullptr && cr->Type == PLAYER && GetProgress() != OldProgress) {
			if (GetSkNr() == SKILL_MAGICLEVEL) {
				SendPlayerData(cr->GetConnection());
			} else {
				SendPlayerSkills(cr->GetConnection());
			}
		}
	}
}

void TSkillProbe::Decrease(int Amount) {
	if (Amount < 0) {
		error("TSkillProbe::Decrease: Amount negative (%d).\n", Amount);
		return;
	}

	int Range = 0;
	int OldProgress = GetProgress();
	if (Exp > Amount) {
		Exp -= Amount;
	} else {
		Exp = 0;
	}

	while (Act > Min) {
		int PreviousLevel = GetExpForLevel(Act);
		if (Exp >= PreviousLevel) {
			break;
		}
		Act--;
		Range--;
		NextLevel = PreviousLevel;
		LastLevel = GetExpForLevel(Act);
	}

	if (Range != 0) {
		Jump(Range);
	} else {
		TCreature *cr = GetMaster();
		if (cr != nullptr && cr->Type == PLAYER && GetProgress() != OldProgress) {
			if (GetSkNr() == SKILL_MAGICLEVEL) {
				SendPlayerData(cr->GetConnection());
			} else {
				SendPlayerSkills(cr->GetConnection());
			}
		}
	}
}

// The tries one test of a skill counts: 1, for a player times [rates] skill.
int TSkillProbe::TriesPerTest() {
	TCreature *cr = GetMaster();
	if (cr != nullptr && cr->Type == PLAYER) {
		return MultiplyRate(1, SkillRate);
	}
	return 1;
}

bool TSkillProbe::Probe(int Diff, int Prob, bool _Increase) {
	if (_Increase) {
		Increase(TriesPerTest());
	}

	if (Diff == 0) {
		return true;
	}

	int Result = rand() % Diff;
	if (Result > Act) {
		return false;
	}
	return rand() % 100 <= Prob;
}

int TSkillProbe::ProbeValue(int max, bool _Increase) {
	if (_Increase) {
		Increase(TriesPerTest());
	}

	int NewMax = max * (Get() * 5 + 50);
	int Result = (rand() % 100 + rand() % 100) / 2 * NewMax / 10000;
	return Result;
}

void TSkillProbe::ChangeSkill(int fp, int delta) {
	// As in GetExpForLevel: extended precision, rounded to double only at
	// Progress, FP and the results of floor().
	double Progress = 0.0;
	if (NextLevel > LastLevel) {
		double ExpGained = Exp - LastLevel;
		double ExpNecessary = NextLevel - LastLevel;
		Progress = (double)((long double)ExpGained / (long double)ExpNecessary);
	}

	FactorPercent = fp;
	Delta = delta;

	double FP = (double)((long double)fp / 1000.0L);
	NextLevel = (int)(double)floorl((__pow_helper(FP, Act - Min + 1) - 1.0L) / (FP - 1.0L) * delta);
	LastLevel = (int)(double)floorl((__pow_helper(FP, Act - Min) - 1.0L) / (FP - 1.0L) * delta);
	Exp = (int)(double)floorl((NextLevel - LastLevel) * (long double)Progress) + LastLevel;

	TCreature *cr = GetMaster();
	if (cr != nullptr && cr->Type == PLAYER) {
		if (GetSkNr() == SKILL_MAGICLEVEL) {
			SendPlayerData(cr->GetConnection());
		} else {
			SendPlayerSkills(cr->GetConnection());
		}
	}
}

void TSkillAdd::Advance(int Range) {
	Max += AddLevel * Range;
	Act += AddLevel * Range;
	Check();
}

void TSkillPoison::Reset() {
	TSkill::Reset();
	FactorPercent = 50;
}

bool TSkillPoison::SetTimer(int cyc, int cou, int maxcou, int additionalvalue) {
	bool retval = TSkill::SetTimer(cyc, cou, maxcou, additionalvalue);
	if (additionalvalue == -1) {
		additionalvalue = 50;
	}
	if (additionalvalue < 10) {
		additionalvalue = 10;
	}
	if (additionalvalue > 1000) {
		additionalvalue = 1000;
	}
	FactorPercent = additionalvalue;
	return retval;
}

bool TSkillPoison::Process() {
	if (Cycle == 0) {
		if (Master != nullptr && Master->Type == PLAYER) {
			static_cast<TPlayer *>(Master)->CheckState();
		}
		return true;
	}

	if (Count > 0) {
		Count--;
		return false;
	}

	Count = MaxCount;
	int Range = Cycle * FactorPercent / 1000;
	if (Range == 0) {
		Range = Cycle > 0 ? 1 : -1;
	}
	Cycle -= Range;
	Event(Range);
	return false;
}

void TSkillPoison::Event(int Range) {
	TCreature *cr = GetMaster();
	if (cr == nullptr) {
		error("TSkillPoison::Event: GetMaster returns NULL!\n");
		return;
	}

	if (Range < 0) {
		Range = -Range;
	}

	cr->Damage(GetCreature(cr->PoisonDamageOrigin), Range, DAMAGE_POISON);

	Object Obj = GetFirstObject(cr->posx, cr->posy, cr->posz);
	while (Obj != NONE) {
		if (Obj.getFlag(AVOID) && Obj.getAttribute(AVOIDDAMAGETYPES) == DAMAGE_POISON) {
			Cycle++;
			break;
		}
		Obj++;
	}
}

void TSkillBurning::Event(int) {
	TCreature *cr = GetMaster();
	if (cr == nullptr) {
		error("TSkillBurning::Event: GetMaster returns NULL!\n");
		return;
	}

	cr->Damage(GetCreature(cr->FireDamageOrigin), 10, DAMAGE_FIRE);

	Object Obj = GetFirstObject(cr->posx, cr->posy, cr->posz);
	while (Obj != NONE) {
		if (Obj.getFlag(AVOID) && Obj.getAttribute(AVOIDDAMAGETYPES) == DAMAGE_FIRE) {
			Cycle++;
			break;
		}
		Obj++;
	}
}

void TSkillEnergy::Event(int) {
	TCreature *cr = GetMaster();
	if (cr == nullptr) {
		error("TSkillEnergy::Event: GetMaster returns NULL!\n");
		return;
	}

	cr->Damage(GetCreature(cr->EnergyDamageOrigin), 25, DAMAGE_ENERGY);

	Object Obj = GetFirstObject(cr->posx, cr->posy, cr->posz);
	while (Obj != NONE) {
		if (Obj.getFlag(AVOID) && Obj.getAttribute(AVOIDDAMAGETYPES) == DAMAGE_ENERGY) {
			Cycle++;
			break;
		}
		Obj++;
	}
}

void TSkillFed::Event(int) {
	TCreature *cr = GetMaster();
	if (cr == nullptr) {
		error("TSkillFed::Event: GetMaster returns NULL!\n");
		return;
	}

	if (!cr->IsDead && !IsProtectionZone(cr->posx, cr->posy, cr->posz)) {
		int SecsPerHP = 12;
		int SecsPerMana = 6;
		int Profession = PROFESSION_NONE;
		if (cr->Type == PLAYER) {
			Profession = static_cast<TPlayer *>(cr)->GetActiveProfession();
		}

		switch (Profession) {
			case PROFESSION_NONE:
			case PROFESSION_KNIGHT:
				SecsPerHP = 6;
				SecsPerMana = 6;
				break;
			case PROFESSION_PALADIN:
				SecsPerHP = 8;
				SecsPerMana = 4;
				break;
			case PROFESSION_SORCERER:
			case PROFESSION_DRUID:
				SecsPerHP = 12;
				SecsPerMana = 3;
				break;
			case PROFESSION_ELITE_KNIGHT:
				SecsPerHP = 4;
				SecsPerMana = 6;
				break;
			case PROFESSION_ROYAL_PALADIN:
				SecsPerHP = 6;
				SecsPerMana = 3;
				break;
			case PROFESSION_MASTER_SORCERER:
			case PROFESSION_ELDER_DRUID:
				SecsPerHP = 12;
				SecsPerMana = 2;
				break;
			default:
				error("TSkillFed::Event: Unknown profession %d.\n", Profession);
				break;
		}

		if (TimerValue() % SecsPerHP == 0) {
			cr->Skills[SKILL_HITPOINTS]->Change(1);
		}

		if (TimerValue() % SecsPerMana == 0) {
			cr->Skills[SKILL_MANA]->Change(2);
		}
	}
}

void TSkillIllusion::Event(int) {
	if (Cycle == 0) {
		TCreature *cr = GetMaster();
		if (cr == nullptr) {
			error("TSkillIllusion::Event: GetMaster returns NULL!\n");
			return;
		}

		if (Get() <= 0) {
			cr->Outfit = cr->OrgOutfit;
			AnnounceChangedCreature(cr->ID, CREATURE_OUTFIT);
			NotifyAllCreatures(cr->GetCreatureObject(), OBJECT_CHANGED, NONE);
		}
	}
}

bool TSkillIllusion::SetTimer(int cyc, int cou, int maxcou, int) {
	TSkill::SetTimer(cyc, cou, maxcou, -1);

	TCreature *cr = GetMaster();
	if (cr == nullptr) {
		error("TSkillIllusion::SetTimer: GetMaster returns NULL!\n");
		return false;
	}

	if (GetCreature(cr->ID) != nullptr) {
		AnnounceChangedCreature(cr->ID, CREATURE_OUTFIT);
	}
	return true;
}

void TSkillLight::Event(int) {
	TCreature *cr = GetMaster();
	if (cr == nullptr) {
		error("TSkillLight::Event: GetMaster returns NULL!\n");
		return;
	}

	AnnounceChangedCreature(cr->ID, CREATURE_LIGHT);
}

bool TSkillLight::SetTimer(int cyc, int cou, int maxcou, int) {
	TSkill::SetTimer(cyc, cou, maxcou, -1);

	TCreature *cr = GetMaster();
	if (cr == nullptr) {
		error("TSkillLight::SetTimer: GetMaster returns NULL!\n");
		return false;
	}

	if (GetCreature(cr->ID) != nullptr) {
		AnnounceChangedCreature(cr->ID, CREATURE_LIGHT);
	}
	return true;
}

void TSkillHitpoints::Set(int a) {
	TCreature *cr = GetMaster();
	if (cr != nullptr && cr->IsDead && a > 0) {
		error("TSkillHitpoints::Set: Attempt to raise HP of a dead creature.\n");
		return;
	}

	TSkill::Set(a);
	if (cr != nullptr) {
		if (cr->Type == PLAYER) {
			SendPlayerData(cr->GetConnection());
		}
		AnnounceChangedCreature(cr->ID, CREATURE_HEALTH);
	}
}

void TSkillMana::Set(int a) {
	TSkill::Set(a);

	TCreature *cr = GetMaster();
	if (cr != nullptr && cr->Type == PLAYER) {
		SendPlayerData(cr->GetConnection());
	}
}

void TSkillSoulpoints::Set(int a) {
	TSkill::Set(a);

	TCreature *cr = GetMaster();
	if (cr != nullptr && cr->Type == PLAYER) {
		SendPlayerData(cr->GetConnection());
	}
}

void TSkillSoulpoints::Event(int) {
	TCreature *cr = GetMaster();
	if (cr == nullptr) {
		error("TSkillSoulpoints::Event: GetMaster returns NULL!\n");
		return;
	}

	if (!cr->IsDead) {
		cr->Skills[SKILL_SOULPOINTS]->Change(1);
	}
}

bool TSkillGoStrength::SetTimer(int cyc, int cou, int maxcou, int) {
	TSkill::SetTimer(cyc, cou, maxcou, -1);
	if (cyc == 0) {
		SetMDAct(0);
	}

	TCreature *cr = GetMaster();
	if (cr == nullptr) {
		error("TSkillGoStrength::SetTimer: GetMaster returns NULL!\n");
		return false;
	}

	AnnounceChangedCreature(cr->ID, CREATURE_SPEED);
	return true;
}

void TSkillGoStrength::Event(int) {
	if (Cycle == 0) {
		TCreature *cr = GetMaster();
		if (cr == nullptr) {
			error("TSkillGoStrength::Event: GetMaster returns NULL!\n");
			return;
		}

		SetMDAct(0);
		AnnounceChangedCreature(cr->ID, CREATURE_SPEED);
	}
}

void TSkillCarryStrength::Set(int a) {
	TSkill::Set(a);

	TCreature *cr = GetMaster();
	if (cr != nullptr && cr->Type == PLAYER) {
		SendPlayerData(cr->GetConnection());
	}
}
