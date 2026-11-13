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
#include "operate.hh"
#include "utils.hh"
#include "time.hh"
#include "crskill.hh"

struct TSkillLevel : public TSkill {
	// inline  crskill.c:34
	TSkillLevel(int nr, TCreature *cr) : TSkill(nr, cr) { abort(); /* STUB */ }
	virtual void Increase(int Amount);
	virtual void Decrease(int Amount);
	virtual int GetExpForLevel(int Level);
private:
	virtual bool Jump(int Range);
};

struct TSkillProbe : public TSkill {
	// inline  crskill.c:44
	TSkillProbe(int nr, TCreature *cr) : TSkill(nr, cr) { abort(); /* STUB */ }
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
};

struct TSkillAdd : public TSkill {
	// inline  crskill.c:59
	TSkillAdd(int nr, TCreature *cr) : TSkill(nr, cr) { abort(); /* STUB */ }
	virtual void Advance(int Range);
};

struct TSkillPoison : public TSkill {
	// inline  crskill.c:66
	TSkillPoison(int nr, TCreature *cr) : TSkill(nr, cr) { abort(); /* STUB */ }
	virtual bool SetTimer(int cyc, int cou, int maxcou, int additionalvalue);
	virtual bool Process();
	virtual void Reset();
private:
	virtual void Event(int Range);
};

struct TSkillBurning : public TSkill {
	// inline  crskill.c:76
	TSkillBurning(int nr, TCreature *cr) : TSkill(nr, cr) { abort(); /* STUB */ }
private:
	virtual void Event(int);
};

struct TSkillEnergy : public TSkill {
	// inline  crskill.c:83
	TSkillEnergy(int nr, TCreature *cr) : TSkill(nr, cr) { abort(); /* STUB */ }
private:
	virtual void Event(int);
};

struct TSkillFed : public TSkill {
	// inline  crskill.c:90
	TSkillFed(int nr, TCreature *cr) : TSkill(nr, cr) { abort(); /* STUB */ }
private:
	virtual void Event(int);
};

struct TSkillIllusion : public TSkill {
	// inline  crskill.c:97
	TSkillIllusion(int nr, TCreature *cr) : TSkill(nr, cr) { abort(); /* STUB */ }
	virtual bool SetTimer(int cyc, int cou, int maxcou, int);
private:
	virtual void Event(int);
};

struct TSkillLight : public TSkill {
	// inline  crskill.c:105
	TSkillLight(int nr, TCreature *cr) : TSkill(nr, cr) { abort(); /* STUB */ }
	virtual bool SetTimer(int cyc, int cou, int maxcou, int);
private:
	virtual void Event(int);
};

struct TSkillHitpoints : public TSkillAdd {
	// inline  crskill.c:113
	TSkillHitpoints(int nr, TCreature *cr) : TSkillAdd(nr, cr) { abort(); /* STUB */ }
	virtual void Set(int a);
};

struct TSkillMana : public TSkillAdd {
	// inline  crskill.c:119
	TSkillMana(int nr, TCreature *cr) : TSkillAdd(nr, cr) { abort(); /* STUB */ }
	virtual void Set(int a);
};

struct TSkillSoulpoints : public TSkillAdd {
	// inline  crskill.c:125
	TSkillSoulpoints(int nr, TCreature *cr) : TSkillAdd(nr, cr) { abort(); /* STUB */ }
	virtual void Set(int a);
	// 0x080cab50  crskill.c:127
	virtual int TimerValue() { abort(); /* STUB */ }
	virtual void Event(int);
};

struct TSkillGoStrength : public TSkillAdd {
	// inline  crskill.c:133
	TSkillGoStrength(int nr, TCreature *cr) : TSkillAdd(nr, cr) { abort(); /* STUB */ }
	virtual bool SetTimer(int cyc, int cou, int maxcou, int);
private:
	virtual void Event(int);
};

struct TSkillCarryStrength : public TSkillAdd {
	// inline  crskill.c:141
	TSkillCarryStrength(int nr, TCreature *cr) : TSkillAdd(nr, cr) { abort(); /* STUB */ }
	virtual void Set(int a);
};

// 0x080c71e0  crskill.c:154
int GetSkillByName(char * /*Name*/) { abort(); /* STUB */ }

// 0x080c7550  crskill.c:189
TSkillBase::TSkillBase() { abort(); /* STUB */ }

// 0x080c75b0  crskill.c:200
TSkillBase::~TSkillBase() { abort(); /* STUB */ }

// 0x080c75e0  crskill.c:211
void TSkillBase::DelTimer(unsigned short /*SkNr*/) { abort(); /* STUB */ }

// 0x080c7670  crskill.c:230
bool TSkillBase::SetTimer(unsigned short /*SkNr*/, int /*cyc*/, int /*cou*/, int /*maxcou*/, int /*additionalvalue*/) { abort(); /* STUB */ }

// 0x080c7700  crskill.c:253
void TSkillBase::ProcessSkills() { abort(); /* STUB */ }

// 0x080c7780  crskill.c:271
bool TSkillBase::NewSkill(unsigned short /*SkillNo*/, TCreature * /*cr*/) { abort(); /* STUB */ }

// 0x080c7f60  crskill.c:317
bool TSkillBase::SetSkills(int /*Race*/) { abort(); /* STUB */ }

// 0x080c80b0  crskill.c:346
TSkill::TSkill(int /*nr*/, TCreature * /*cr*/) { abort(); /* STUB */ }

// 0x080c8180  crskill.c:355
TSkill::~TSkill() { abort(); /* STUB */ }

// 0x080c8200  crskill.c:364
void TSkill::Reset() { abort(); /* STUB */ }

// 0x080c8280  crskill.c:382
void TSkill::Load(int /*act*/, int /*max*/, int /*min*/, int /*dact*/, int /*mdact*/, int /*cyc*/, int /*maxcyc*/, int /*cnt*/, int /*maxcnt*/, int /*addlvl*/, int /*exp*/, int /*fp*/, int /*nextlvl*/, int /*delta*/) { abort(); /* STUB */ }

// 0x080c8390  crskill.c:407
void TSkill::Save(int & /*act*/, int & /*max*/, int & /*min*/, int & /*dact*/, int & /*mdact*/, int & /*cyc*/, int & /*maxcyc*/, int & /*cnt*/, int & /*maxcnt*/, int & /*addlvl*/, int & /*exp*/, int & /*fp*/, int & /*nextlvl*/, int & /*delta*/) { abort(); /* STUB */ }

// 0x080c8410  crskill.c:428
void TSkill::Event(int) { abort(); /* STUB */ }

// 0x080c8420  crskill.c:435
bool TSkill::Jump(int) { abort(); /* STUB */ }

// 0x080c8430  crskill.c:442
int TSkill::Get() { abort(); /* STUB */ }

// 0x080c8450  crskill.c:453
int TSkill::GetProgress() { abort(); /* STUB */ }

// 0x080c8540  crskill.c:468
void TSkill::SetMDAct(int /*value*/) { abort(); /* STUB */ }

// 0x080c8570  crskill.c:477
void TSkill::Set(int /*a*/) { abort(); /* STUB */ }

// 0x080c8590  crskill.c:485
void TSkill::Change(int /*a*/) { abort(); /* STUB */ }

// 0x080c85d0  crskill.c:494
void TSkill::Check() { abort(); /* STUB */ }

// 0x080c85f0  crskill.c:504
bool TSkill::Process() { abort(); /* STUB */ }

// 0x080c8660  crskill.c:523
bool TSkill::SetTimer(int /*cyc*/, int /*cou*/, int /*maxcou*/, int) { abort(); /* STUB */ }

// 0x080c86a0  crskill.c:537
bool TSkill::DelTimer() { abort(); /* STUB */ }

// 0x080c86f0  crskill.c:554
int TSkillLevel::GetExpForLevel(int /*Level*/) { abort(); /* STUB */ }

// 0x080c8780  crskill.c:574
void TSkillLevel::Increase(int /*Amount*/) { abort(); /* STUB */ }

// 0x080c8880  crskill.c:607
void TSkillLevel::Decrease(int /*Amount*/) { abort(); /* STUB */ }

// 0x080c8970  crskill.c:641
bool TSkillLevel::Jump(int /*Range*/) { abort(); /* STUB */ }

// 0x080c8aa0  crskill.c:685
bool TSkillProbe::SetTimer(int /*cyc*/, int /*cou*/, int /*maxcou*/, int) { abort(); /* STUB */ }

// 0x080c8b20  crskill.c:700
void TSkillProbe::Event(int) { abort(); /* STUB */ }

// 0x080c8ba0  crskill.c:716
bool TSkillProbe::Jump(int /*Range*/) { abort(); /* STUB */ }

// 0x080c8d00  crskill.c:743
int TSkillProbe::GetExpForLevel(int /*Level*/) { abort(); /* STUB */ }

// 0x080c8e70  crskill.c:766
void TSkillProbe::Increase(int /*Amount*/) { abort(); /* STUB */ }

// 0x080c9180  crskill.c:798
void TSkillProbe::Decrease(int /*Amount*/) { abort(); /* STUB */ }

// 0x080c9460  crskill.c:829
bool TSkillProbe::Probe(int /*Diff*/, int /*Prob*/, bool /*_Increase*/) { abort(); /* STUB */ }

// 0x080c94f0  crskill.c:843
int TSkillProbe::ProbeValue(int /*max*/, bool /*_Increase*/) { abort(); /* STUB */ }

// 0x080c95d0  crskill.c:858
void TSkillProbe::ChangeSkill(int /*fp*/, int /*delta*/) { abort(); /* STUB */ }

// 0x080c97e0  crskill.c:890
void TSkillAdd::Advance(int /*Range*/) { abort(); /* STUB */ }

// 0x080c9810  crskill.c:905
void TSkillPoison::Reset() { abort(); /* STUB */ }

// 0x080c9890  crskill.c:916
bool TSkillPoison::SetTimer(int /*cyc*/, int /*cou*/, int /*maxcou*/, int /*additionalvalue*/) { abort(); /* STUB */ }

// 0x080c9910  crskill.c:930
bool TSkillPoison::Process() { abort(); /* STUB */ }

// 0x080c99b0  crskill.c:949
void TSkillPoison::Event(int /*Range*/) { abort(); /* STUB */ }

// 0x080c9ad0  crskill.c:974
void TSkillBurning::Event(int) { abort(); /* STUB */ }

// 0x080c9bf0  crskill.c:998
void TSkillEnergy::Event(int) { abort(); /* STUB */ }

// 0x080c9d10  crskill.c:1022
void TSkillFed::Event(int) { abort(); /* STUB */ }

// 0x080c9e90  crskill.c:1060
void TSkillIllusion::Event(int) { abort(); /* STUB */ }

// 0x080c9f40  crskill.c:1078
bool TSkillIllusion::SetTimer(int /*cyc*/, int /*cou*/, int /*maxcou*/, int) { abort(); /* STUB */ }

// 0x080c9fe0  crskill.c:1099
void TSkillLight::Event(int) { abort(); /* STUB */ }

// 0x080ca010  crskill.c:1112
bool TSkillLight::SetTimer(int /*cyc*/, int /*cou*/, int /*maxcou*/, int) { abort(); /* STUB */ }

// 0x080ca0b0  crskill.c:1132
void TSkillHitpoints::Set(int /*a*/) { abort(); /* STUB */ }

// 0x080ca140  crskill.c:1153
void TSkillMana::Set(int /*a*/) { abort(); /* STUB */ }

// 0x080ca180  crskill.c:1167
void TSkillSoulpoints::Set(int /*a*/) { abort(); /* STUB */ }

// 0x080ca1c0  crskill.c:1177
void TSkillSoulpoints::Event(int) { abort(); /* STUB */ }

// 0x080ca220  crskill.c:1195
bool TSkillGoStrength::SetTimer(int /*cyc*/, int /*cou*/, int /*maxcou*/, int) { abort(); /* STUB */ }

// 0x080ca2d0  crskill.c:1211
void TSkillGoStrength::Event(int) { abort(); /* STUB */ }

// 0x080ca350  crskill.c:1231
void TSkillCarryStrength::Set(int /*a*/) { abort(); /* STUB */ }

// 0x080ca390  crskill.c:1246
void InitCrskill() { abort(); /* STUB */ }

// 0x080ca3a0  crskill.c:1253
void ExitCrskill() { abort(); /* STUB */ }
