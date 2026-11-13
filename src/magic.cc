#include <cstdint>
#include <cstdlib>

#include "script.hh"
#include "threads.hh"
#include "dbfuncs.hh"
#include "config.hh"
#include "objects.hh"
#include "map.hh"
#include "info.hh"
#include "cr.hh"
#include "containr.hh"
#include "connections.hh"
#include "operate.hh"
#include "magic.hh"
#include "utils.hh"
#include "time.hh"
#include "TXTEASymmetricKey.hh"

struct TCircle {
	int x[32];
	int y[32];
	int Count;
};

struct TSpellList {
	unsigned char Syllable[10];
	unsigned char RuneGr;
	unsigned char RuneNr;
	const char *Comment;
	unsigned short Level;
	unsigned short RuneLevel;
	unsigned short Flags;
	int Mana;
	int SoulPoints;
	int Amount;
};

char SpellSyllable[51][6] = {
	"",
	"al",
	"ad",
	"ex",
	"ut",
	"om",
	"para",
	"ana",
	"evo",
	"ori",
	"mort",
	"lux",
	"liber",
	"vita",
	"flam",
	"pox",
	"hur",
	"moe",
	"ani",
	"ina",
	"eta",
	"amo",
	"hora",
	"gran",
	"cogni",
	"res",
	"mas",
	"vis",
	"som",
	"aqua",
	"frigo",
	"tera",
	"ura",
	"sio",
	"grav",
	"ito",
	"pan",
	"vid",
	"isa",
	"iva",
	"con",
	"",
	"",
	"",
	"",
	"",
	"",
	"",
	"",
	"",
	"",
};	// magic.c:109
TCircle Circle[10];	// magic.c:124
TSpellList SpellList[256];	// magic.c:125

// 0x0807e530  magic.c:144
int Direction(int /*dx*/, int /*dy*/) { abort(); /* STUB */ }

// 0x0807e5c0  magic.c:166
void CheckSpellbook(TCreature * /*cr*/, int /*SpellNr*/) { abort(); /* STUB */ }

// 0x0807e670  magic.c:182
void CheckAccount(TCreature * /*cr*/, int /*SpellNr*/) { abort(); /* STUB */ }

// 0x0807e740  magic.c:202
void CheckLevel(TCreature * /*cr*/, int /*SpellNr*/) { abort(); /* STUB */ }

// 0x0807e820  magic.c:222
void CheckRuneLevel(TCreature * /*cr*/, int /*SpellNr*/) { abort(); /* STUB */ }

// 0x0807e900  magic.c:243
void CheckMagicItem(TCreature * /*cr*/, ObjectType /*Type*/) { abort(); /* STUB */ }

// 0x0807e9e0  magic.c:259
void CheckRing(TCreature * /*cr*/, int) { abort(); /* STUB */ }

// 0x0807ea70  magic.c:276
void CheckAffectedPlayers(TCreature * /*cr*/, int /*x*/, int /*y*/, int /*z*/) { abort(); /* STUB */ }

// 0x0807ebd0  magic.c:294
void CheckMana(TCreature * /*cr*/, int /*Mana*/, int /*SoulPoints*/, int /*Delay*/) { abort(); /* STUB */ }

// 0x0807ed50  magic.c:328
int ComputeDamage(TCreature * /*cr*/, int /*SpellNr*/, int /*Damage*/, int /*Variation*/) { abort(); /* STUB */ }

// 0x0807ee30  magic.c:345
bool IsAggressiveSpell(int /*SpellNr*/) { abort(); /* STUB */ }

// 0x0807eee0  magic.c:362
TDamageImpact::TDamageImpact(TCreature * /*_Actor*/, int /*_DamageType*/, int /*_Power*/, bool /*_AllowDefense*/) { abort(); /* STUB */ }

// 0x0807ef40  magic.c:374
void TDamageImpact::handleCreature(TCreature * /*Victim*/) { abort(); /* STUB */ }

// 0x0807f030  magic.c:391
TFieldImpact::TFieldImpact(TCreature * /*_Actor*/, int /*_FieldType*/) { abort(); /* STUB */ }

// 0x0807f070  magic.c:401
void TFieldImpact::handleField(int /*x*/, int /*y*/, int /*z*/) { abort(); /* STUB */ }

// 0x0807f170  magic.c:412
THealingImpact::THealingImpact(TCreature * /*_Actor*/, int /*_Power*/) { abort(); /* STUB */ }

// 0x0807f1e0  magic.c:423
void THealingImpact::handleCreature(TCreature * /*Victim*/) { abort(); /* STUB */ }

// 0x0807f2e0  magic.c:439
TSpeedImpact::TSpeedImpact(TCreature * /*_Actor*/, int /*_Percent*/, int /*_Duration*/) { abort(); /* STUB */ }

// 0x0807f330  magic.c:450
void TSpeedImpact::handleCreature(TCreature * /*Victim*/) { abort(); /* STUB */ }

// 0x0807f4f0  magic.c:473
TDrunkenImpact::TDrunkenImpact(TCreature * /*_Actor*/, int /*_Power*/, int /*_Duration*/) { abort(); /* STUB */ }

// 0x0807f560  magic.c:488
void TDrunkenImpact::handleCreature(TCreature * /*Victim*/) { abort(); /* STUB */ }

// 0x0807f6a0  magic.c:507
TStrengthImpact::TStrengthImpact(TCreature * /*_Actor*/, int /*_Skills*/, int /*_Percent*/, int /*_Duration*/) { abort(); /* STUB */ }

// 0x0807f6f0  magic.c:519
void TStrengthImpact::handleCreature(TCreature * /*Victim*/) { abort(); /* STUB */ }

// 0x0807f8f0  magic.c:547
TOutfitImpact::TOutfitImpact(TCreature * /*_Actor*/, TOutfit /*_Outfit*/, int /*_Duration*/) { abort(); /* STUB */ }

// 0x0807f950  magic.c:558
void TOutfitImpact::handleCreature(TCreature * /*Victim*/) { abort(); /* STUB */ }

// 0x0807fa50  magic.c:572
TSummonImpact::TSummonImpact(TCreature * /*_Actor*/, int /*_Race*/, int /*_Maximum*/) { abort(); /* STUB */ }

// 0x0807fad0  magic.c:584
void TSummonImpact::handleField(int /*x*/, int /*y*/, int /*z*/) { abort(); /* STUB */ }

// 0x0807fb80  magic.c:603
void ActorShapeSpell(TCreature * /*Actor*/, TImpact * /*Impact*/, int /*Effect*/) { abort(); /* STUB */ }

// 0x0807fc30  magic.c:618
void VictimShapeSpell(TCreature * /*Actor*/, TCreature * /*Victim*/, int /*Range*/, int /*Animation*/, TImpact * /*Impact*/, int /*Effect*/) { abort(); /* STUB */ }

// 0x0807fe00  magic.c:638
void OriginShapeSpell(TCreature * /*Actor*/, int /*Radius*/, TImpact * /*Impact*/, int /*Effect*/) { abort(); /* STUB */ }

// 0x08080020  magic.c:666
void DestinationShapeSpell(TCreature * /*Actor*/, TCreature * /*Victim*/, int /*Range*/, int /*Animation*/, int /*Radius*/, TImpact * /*Impact*/, int /*Effect*/) { abort(); /* STUB */ }

// 0x080800c0  magic.c:680
void CircleShapeSpell(TCreature * /*Actor*/, int /*DestX*/, int /*DestY*/, int /*DestZ*/, int /*Range*/, int /*Animation*/, int /*Radius*/, TImpact * /*Impact*/, int /*Effect*/) { abort(); /* STUB */ }

// 0x080803f0  magic.c:713
void AngleShapeSpell(TCreature * /*Actor*/, int /*Angle*/, int /*Range*/, TImpact * /*Impact*/, int /*Effect*/) { abort(); /* STUB */ }

// 0x080806b0  magic.c:754
void MassCombat(TCreature * /*cr*/, Object /*Target*/, int /*Mana*/, int /*SoulPoints*/, int /*Damage*/, int /*EffectNr*/, int /*Radius*/, unsigned short /*DmgType*/, int /*Animation*/) { abort(); /* STUB */ }

// 0x08080870  magic.c:783
void AngleCombat(TCreature * /*cr*/, int /*Mana*/, int /*SoulPoints*/, int /*Damage*/, int /*EffectNr*/, int /*Radius*/, int /*Angle*/, unsigned short /*DmgType*/) { abort(); /* STUB */ }

// 0x08080990  magic.c:802
void Combat(TCreature * /*cr*/, Object /*Target*/, int /*Mana*/, int /*SoulPoints*/, int /*Damage*/, int /*EffectNr*/, int /*AnimationNr*/, unsigned short /*DmgType*/) { abort(); /* STUB */ }

// 0x08080be0  magic.c:829
void KillAllMonsters(TCreature * /*cr*/, int /*EffectNr*/, int /*Radius*/) { abort(); /* STUB */ }

// 0x08080ea0  magic.c:896
void CreateField(TCreature * /*cr*/, Object /*Obj*/, int /*Mana*/, int /*SoulPoints*/, int /*Type*/) { abort(); /* STUB */ }

// 0x080810f0  magic.c:921
void CreateField(TCreature * /*cr*/, int /*Mana*/, int /*SoulPoints*/, int /*Type*/) { abort(); /* STUB */ }

// 0x08081250  magic.c:945
void MassCreateField(TCreature * /*cr*/, Object /*Obj*/, int /*Mana*/, int /*SoulPoints*/, int /*Type*/, int /*Radius*/) { abort(); /* STUB */ }

// 0x08081570  magic.c:976
void CreateFieldWall(TCreature * /*cr*/, Object /*Obj*/, int /*Mana*/, int /*SoulPoints*/, int /*Type*/, int /*Extension*/) { abort(); /* STUB */ }

// 0x08081b70  magic.c:1030
void DeleteField(TCreature * /*cr*/, Object /*Obj*/, int /*Mana*/, int /*SoulPoints*/) { abort(); /* STUB */ }

// 0x08081d50  magic.c:1057
void CleanupField(TCreature * /*cr*/, Object /*Obj*/, int /*Mana*/, int /*SoulPoints*/) { abort(); /* STUB */ }

// 0x08081fe0  magic.c:1088
void CleanupField(TCreature * /*cr*/) { abort(); /* STUB */ }

// 0x08082150  magic.c:1116
void Teleport(TCreature * /*cr*/, char * /*Param*/) { abort(); /* STUB */ }

// 0x08082760  magic.c:1197
void TeleportToCreature(TCreature * /*cr*/, char * /*Name*/) { abort(); /* STUB */ }

// 0x08082a50  magic.c:1245
void TeleportPlayerToMe(TCreature * /*cr*/, char * /*Name*/) { abort(); /* STUB */ }

// 0x08082cd0  magic.c:1341
void MagicRope(TCreature * /*cr*/, int /*Mana*/, int /*SoulPoints*/) { abort(); /* STUB */ }

// 0x08082e50  magic.c:1360
void MagicClimbing(TCreature * /*cr*/, int /*Mana*/, int /*SoulPoints*/, char * /*Direction*/) { abort(); /* STUB */ }

// 0x08083190  magic.c:1408
void MagicClimbing(TCreature * /*cr*/, char * /*Direction*/) { abort(); /* STUB */ }

// 0x08083260  magic.c:1434
void CreateThing(TCreature * /*cr*/, char * /*Param1*/, char * /*Param2*/) { abort(); /* STUB */ }

// 0x080835f0  magic.c:1481
void CreateMoney(TCreature * /*cr*/, char * /*Param*/) { abort(); /* STUB */ }

// 0x08083850  magic.c:1514
void CreateFood(TCreature * /*cr*/, int /*Mana*/, int /*SoulPoints*/) { abort(); /* STUB */ }

// 0x08083a80  magic.c:1544
void CreateArrows(TCreature * /*cr*/, int /*Mana*/, int /*SoulPoints*/, int /*Ammo*/, int /*Amount*/) { abort(); /* STUB */ }

// 0x08083c40  magic.c:1570
void SummonCreature(TCreature * /*cr*/, int /*Mana*/, int /*Race*/, bool /*God*/) { abort(); /* STUB */ }

// 0x08083e90  magic.c:1598
void SummonCreature(TCreature * /*cr*/, int /*Mana*/, char * /*RaceName*/, bool /*God*/) { abort(); /* STUB */ }

// 0x08083f90  magic.c:1622
void StartMonsterraid(TCreature * /*cr*/, char * /*RaidName*/) { abort(); /* STUB */ }

// 0x08084140  magic.c:1652
void RaiseDead(TCreature * /*cr*/, Object /*Obj*/, int /*Mana*/, int /*SoulPoints*/) { abort(); /* STUB */ }

// 0x08084440  magic.c:1683
void MassRaiseDead(TCreature * /*cr*/, Object /*Target*/, int /*Mana*/, int /*SoulPoints*/, int /*Radius*/) { abort(); /* STUB */ }

// 0x08084880  magic.c:1728
void Heal(TCreature * /*cr*/, int /*Mana*/, int /*SoulPoints*/, int /*Healing*/) { abort(); /* STUB */ }

// 0x080849d0  magic.c:1751
void MassHeal(TCreature * /*cr*/, Object /*Target*/, int /*Mana*/, int /*SoulPoints*/, int /*Healing*/, int /*Radius*/) { abort(); /* STUB */ }

// 0x08084d90  magic.c:1796
void HealFriend(TCreature * /*cr*/, char * /*Name*/, int /*Mana*/, int /*SoulPoints*/, int /*Healing*/) { abort(); /* STUB */ }

// 0x08084fa0  magic.c:1831
void RefreshMana(TCreature * /*cr*/, int /*Mana*/, int /*SoulPoints*/, int /*AddMana*/) { abort(); /* STUB */ }

// 0x08085070  magic.c:1854
void MagicGoStrength(TCreature * /*cr*/, TCreature * /*dest*/, int /*Mana*/, int /*SoulPoints*/, int /*percent*/, int /*duration*/) { abort(); /* STUB */ }

// 0x08085270  magic.c:1882
void Shielding(TCreature * /*cr*/, int /*Mana*/, int /*SoulPoints*/, int /*Duration*/) { abort(); /* STUB */ }

// 0x08085350  magic.c:1897
void NegatePoison(TCreature * /*cr*/, int /*Mana*/, int /*SoulPoints*/, TCreature * /*dest*/) { abort(); /* STUB */ }

// 0x08085440  magic.c:1916
void Enlight(TCreature * /*cr*/, int /*Mana*/, int /*SoulPoints*/, int /*Radius*/, int /*Duration*/) { abort(); /* STUB */ }

// 0x08085530  magic.c:1932
void Invisibility(TCreature * /*cr*/, int /*Mana*/, int /*SoulPoints*/, int /*Duration*/) { abort(); /* STUB */ }

// 0x08085620  magic.c:1949
void CancelInvisibility(TCreature * /*cr*/, Object /*Target*/, int /*Mana*/, int /*SoulPoints*/, int /*Radius*/) { abort(); /* STUB */ }

// 0x08085c30  magic.c:2005
void CreatureIllusion(TCreature * /*cr*/, int /*Mana*/, int /*SoulPoints*/, char * /*RaceName*/, int /*Duration*/) { abort(); /* STUB */ }

// 0x08085df0  magic.c:2036
void ObjectIllusion(TCreature * /*cr*/, int /*Mana*/, int /*SoulPoints*/, Object /*Obj*/, int /*Duration*/) { abort(); /* STUB */ }

// 0x08085fc0  magic.c:2061
void ChangeData(TCreature * /*cr*/, char * /*Param*/) { abort(); /* STUB */ }

// 0x080863e0  magic.c:2111
void EnchantObject(TCreature * /*cr*/, int /*Mana*/, int /*SoulPoints*/, ObjectType /*Origin*/, ObjectType /*Target*/) { abort(); /* STUB */ }

// 0x08086620  magic.c:2131
void Convince(TCreature * /*master*/, TCreature * /*slave*/) { abort(); /* STUB */ }

// 0x08086790  magic.c:2148
void Challenge(TCreature * /*cr*/, int /*Mana*/, int /*SoulPoints*/, int /*Radius*/) { abort(); /* STUB */ }

// 0x08086950  magic.c:2175
void FindPerson(TCreature * /*cr*/, int /*Mana*/, int /*SoulPoints*/, char * /*Name*/) { abort(); /* STUB */ }

// 0x08086cd0  magic.c:2231
void GetPosition(TCreature * /*cr*/) { abort(); /* STUB */ }

// 0x08086db0  magic.c:2250
void GetQuestValue(TCreature * /*cr*/, char * /*Param*/) { abort(); /* STUB */ }

// 0x08086f00  magic.c:2278
void SetQuestValue(TCreature * /*cr*/, char * /*Param1*/, char * /*Param2*/) { abort(); /* STUB */ }

// 0x080870a0  magic.c:2312
void ClearQuestValues(TCreature * /*cr*/) { abort(); /* STUB */ }

// 0x08087190  magic.c:2331
void CreateKnowledge(TCreature * /*cr*/, char * /*Param1*/, char * /*Param2*/) { abort(); /* STUB */ }

// 0x080872f0  magic.c:2362
void ChangeProfession(TCreature * /*cr*/, char * /*Param*/) { abort(); /* STUB */ }

// 0x08087550  magic.c:2408
void EditGuests(TCreature * /*cr*/) { abort(); /* STUB */ }

// 0x08087660  magic.c:2430
void EditSubowners(TCreature * /*cr*/) { abort(); /* STUB */ }

// 0x08087770  magic.c:2452
void KickGuest(TCreature * /*cr*/, char * /*Name*/) { abort(); /* STUB */ }

// 0x080878d0  magic.c:2480
void EditNameDoor(TCreature * /*cr*/) { abort(); /* STUB */ }

// 0x08087b80  magic.c:2532
void Notation(TCreature * /*cr*/, char * /*Name*/, char * /*Comment*/) { abort(); /* STUB */ }

// 0x08087c40  magic.c:2557
void NameLock(TCreature * /*cr*/, char * /*Name*/) { abort(); /* STUB */ }

// 0x08087ce0  magic.c:2578
void BanishAccount(TCreature * /*cr*/, char * /*Name*/, int, char * /*Reason*/) { abort(); /* STUB */ }

// 0x08087da0  magic.c:2603
void DeleteAccount(TCreature * /*cr*/, char * /*Name*/, char * /*Reason*/) { abort(); /* STUB */ }

// 0x08087e60  magic.c:2628
void BanishCharacter(TCreature * /*cr*/, char * /*Name*/, int, char * /*Reason*/) { abort(); /* STUB */ }

// 0x08087f20  magic.c:2653
void DeleteCharacter(TCreature * /*cr*/, char * /*Name*/, char * /*Reason*/) { abort(); /* STUB */ }

// 0x08087fe0  magic.c:2679
void IPBanishment(TCreature * /*cr*/, char * /*Name*/, char * /*Reason*/) { abort(); /* STUB */ }

// 0x080880a0  magic.c:2704
void SetNameRule(TCreature * /*cr*/, char * /*Name*/) { abort(); /* STUB */ }

// 0x08088140  magic.c:2725
void KickPlayer(TCreature * /*cr*/, char * /*Name*/) { abort(); /* STUB */ }

// 0x080882f0  magic.c:2757
void HomeTeleport(TCreature * /*cr*/, char * /*Name*/) { abort(); /* STUB */ }

// 0x08088520  magic.c:2796
void SpellFailed(uint32_t /*CreatureID*/) { abort(); /* STUB */ }

// 0x08088570  magic.c:2809
void CharacterRightSpell(uint32_t /*CreatureID*/, int /*SpellNr*/, char (* /*SpellStr*/)[512]) { abort(); /* STUB */ }

// 0x080887f0  magic.c:2850
void AccountRightSpell(uint32_t /*CreatureID*/, int /*SpellNr*/, char (* /*SpellStr*/)[512]) { abort(); /* STUB */ }

// 0x08088a00  magic.c:2876
void CastSpell(uint32_t /*CreatureID*/, int /*SpellNr*/, char (* /*SpellStr*/)[512]) { abort(); /* STUB */ }

// 0x08089500  magic.c:2970
void RuneSpell(uint32_t /*CreatureID*/, int /*SpellNr*/) { abort(); /* STUB */ }

// 0x08089850  magic.c:3019
int TypeOfSpell(char * /*Text*/) { abort(); /* STUB */ }

// 0x080898a0  magic.c:3030
int FindSpell(unsigned char * /*Syllable*/) { abort(); /* STUB */ }

// 0x08089940  magic.c:3055
int FindSpell(Object /*Obj*/) { abort(); /* STUB */ }

// 0x08089a20  magic.c:3079
void GetSpellString(int /*SpellNr*/, char * /*Text*/) { abort(); /* STUB */ }

// 0x08089ad0  magic.c:3098
void GetMagicItemDescription(Object /*Obj*/, char * /*SpellString*/, int & /*MagicLevel*/) { abort(); /* STUB */ }

// 0x08089bb0  magic.c:3128
void GetSpellbook(uint32_t /*CharacterID*/, char * /*Buffer*/) { abort(); /* STUB */ }

// 0x08089e40  magic.c:3179
int GetSpellLevel(int /*SpellNr*/) { abort(); /* STUB */ }

// 0x08089e80  magic.c:3197
int CheckForSpell(uint32_t /*CreatureID*/, char * /*Text*/) { abort(); /* STUB */ }

// 0x0808a5f0  magic.c:3259
void DeleteRune(Object /*Obj*/) { abort(); /* STUB */ }

// 0x0808a6c0  magic.c:3273
void UseMagicItem(uint32_t /*CreatureID*/, Object /*Obj*/, Object /*Dest*/) { abort(); /* STUB */ }

// 0x0808b330  magic.c:3382
void DrinkPotion(uint32_t /*CreatureID*/, Object /*Obj*/) { abort(); /* STUB */ }

// 0x0808b4d0  magic.c:3418
void InitCircles() { abort(); /* STUB */ }

// 0x0808b930  magic.c:3455
TSpellList *CreateSpell(int /*SpellNr*/, ...) { abort(); /* STUB */ }

// 0x0808ba20  magic.c:3480
void InitSpells() { abort(); /* STUB */ }

// 0x0808db10  magic.c:4157
void InitMagic() { abort(); /* STUB */ }

// 0x0808db50  magic.c:4168
void ExitMagic() { abort(); /* STUB */ }

// 0x0808db60  magic.c:869
[[maybe_unused]] static void CreateField(int /*x*/, int /*y*/, int /*z*/, int /*FieldType*/, uint32_t /*Owner*/, bool /*Peaceful*/) { abort(); /* STUB */ }
