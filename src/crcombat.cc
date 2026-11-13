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
#include "utils.hh"
#include "time.hh"
#include "crcombat.hh"

// 0x080c3ff0  crcombat.c:33
TCombat::TCombat() { abort(); /* STUB */ }

// 0x080c40c0  crcombat.c:62
void TCombat::CanToDoAttack() { abort(); /* STUB */ }

// 0x080c4420  crcombat.c:116
int TCombat::GetDistance() { abort(); /* STUB */ }

// 0x080c44a0  crcombat.c:129
void TCombat::Attack() { abort(); /* STUB */ }

// 0x080c4980  crcombat.c:204
void TCombat::SetAttackDest(uint32_t /*dfd*/, bool /*Follow*/) { abort(); /* STUB */ }

// 0x080c4e70  crcombat.c:266
void TCombat::SetAttackMode(unsigned char /*b*/) { abort(); /* STUB */ }

// 0x080c4ee0  crcombat.c:281
void TCombat::SetChaseMode(unsigned char /*b*/) { abort(); /* STUB */ }

// 0x080c4f10  crcombat.c:294
void TCombat::SetSecureMode(unsigned char /*b*/) { abort(); /* STUB */ }

// 0x080c4f40  crcombat.c:306
void TCombat::StopAttack(int /*Delay*/) { abort(); /* STUB */ }

// 0x080c4f80  crcombat.c:317
void TCombat::DelayAttack(int /*Milliseconds*/) { abort(); /* STUB */ }

// 0x080c4fc0  crcombat.c:326
void TCombat::CheckCombatValues() { abort(); /* STUB */ }

// 0x080c50d0  crcombat.c:346
unsigned short TCombat::GetArmorStrength() { abort(); /* STUB */ }

// 0x080c5220  crcombat.c:364
void TCombat::AddDamageToCombatList(uint32_t /*atk*/, unsigned short /*Damage*/) { abort(); /* STUB */ }

// 0x080c5290  crcombat.c:383
void TCombat::DistributeExperiencePoints(uint32_t /*Exp*/) { abort(); /* STUB */ }

// 0x080c54d0  crcombat.c:416
uint32_t TCombat::GetDamageByCreature(uint32_t /*CreatureID*/) { abort(); /* STUB */ }

// 0x080c5500  crcombat.c:426
uint32_t TCombat::GetMostDangerousAttacker() { abort(); /* STUB */ }

// 0x080c5540  crcombat.c:442
int TCombat::TypeToSkill(int /*type*/) { abort(); /* STUB */ }

// 0x080c5590  crcombat.c:459
void TCombat::GetWeapon() { abort(); /* STUB */ }

// 0x080c5840  crcombat.c:490
void TCombat::GetAmmo() { abort(); /* STUB */ }

// 0x080c5980  crcombat.c:505
void TCombat::CloseAttack(TCreature * /*dfd*/) { abort(); /* STUB */ }

// 0x080c5ca0  crcombat.c:540
void TCombat::RangeAttack(TCreature * /*dfd*/) { abort(); /* STUB */ }

// 0x080c6b20  crcombat.c:654
void TCombat::GetAttackValue(short & /*Value*/, unsigned short & /*Skill*/) { abort(); /* STUB */ }

// 0x080c6c90  crcombat.c:681
void TCombat::GetDefendValue(short & /*Value*/, unsigned short & /*Skill*/) { abort(); /* STUB */ }

// 0x080c6e00  crcombat.c:708
short TCombat::GetAttackDamage() { abort(); /* STUB */ }

// 0x080c6ee0  crcombat.c:730
short TCombat::GetDefendDamage() { abort(); /* STUB */ }

// 0x080c7180  crcombat.c:767
void TCombat::ActivateLearning() { abort(); /* STUB */ }
