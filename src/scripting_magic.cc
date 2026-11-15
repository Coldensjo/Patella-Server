// scripting_magic.cc: the API of the spells', runes' and commands' scripts
// (scripting.hh, "Spells, runes and commands"): the spell functions of
// magic.cc in the table Magic, the commands' functions in Command, the few
// reads the cases of the old switches made (Game.toInt, the methods
// creature:getSkillValue, player:getLevel, player:getVocation with
// vocation:getId and vocation:getBaseId, ItemType.fromOldNumber) and the
// constants (CONST_ME_, CONST_ANI_, COMBAT_, FIELD_, CONJURE_, SKILL_,
// VOCATION_). RegisterMagicAPI binds them; InitAll calls it after
// InitScripting.
//
// What every binding here does alike:
//  - actor and target are Creature objects (a Player is one), whose IDs are
//    resolved with GetCreature on every call; a creature that is gone gives
//    nullptr, and so does nil for a target that may be nil ("|nil"). Where
//    the C++ function checks for nullptr itself (it logs and throws ERROR),
//    the binding passes nullptr on, so the result is the same ("nullptr:
//    passed on" below). Where it would dereference nullptr, the binding logs
//    "<Lua name>: Creature does not exist." and throws ERROR ("nullptr:
//    checked").
//  - obj is an Item object, whose handle the binding passes on as Object(obj)
//    (the C++ function checks it with exists() as before), or a Creature
//    object, whose object the binding passes on (GetCreatureObject; a
//    creature that is gone is "nullptr: checked").
//  - A string passed on as a char * is a copy (TWord): at most 511
//    characters and the zero, as CheckForSpell's SpellStr holds a word (at
//    most 509 characters), so a function that changes its parameter
//    (StartMonsterraid lowers it, for one) never touches Lua's string. nil
//    gives nullptr only where the API list says "|nil"; elsewhere nil is a
//    Lua error of the argument check.
//  - What the C++ function throws (a RESULT) is raised as the result code
//    (Guarded). None of the bound functions throws anything else itself
//    except where its comment says so; std::bad_alloc from an allocation
//    would become a logged Lua error and ERROR (Guarded) instead of going
//    up.
//  - The binding's own checks (a radius beyond the circles, a skill or spell
//    number out of range, a light radius 0) guard what the C++ code never
//    got from its constant arguments and would read out of bounds or divide
//    by zero; they throw a const char *, which Guarded turns into a logged
//    Lua error and ERROR.

#include <algorithm>
#include <climits>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <string>

#include "objects.hh"
#include "map.hh"
#include "info.hh"
#include "cr.hh"
#include "crskill.hh"
#include "crmain.hh"
#include "operate.hh"
#include "magic.hh"
#include "magicdata.hh"
#include "utils.hh"
#include "scripting_api.hh"

// The size of a word's copy: CheckForSpell's SpellStr entries.
const int WORD_SIZE = 512;

// A copy of a string for a C++ function that takes a char *: the first
// WORD_SIZE - 1 bytes (up to a zero byte in the string) and the zero.
struct TWord {
	char Text[WORD_SIZE];

	explicit TWord(const std::string &Value) {
		size_t Length = std::min(Value.size(), static_cast<size_t>(WORD_SIZE - 1));
		memcpy(Text, Value.data(), Length);
		Text[Length] = 0;
	}
};

// TWord of a string that may be nil: get() is nullptr for nil.
struct TOptionalWord {
	char Text[WORD_SIZE];
	bool Given;

	explicit TOptionalWord(const sol::optional<std::string> &Value) : Given(Value.has_value()) {
		Text[0] = 0;
		if (Given) {
			size_t Length = std::min(Value->size(), static_cast<size_t>(WORD_SIZE - 1));
			memcpy(Text, Value->data(), Length);
			Text[Length] = 0;
		}
	}

	char *get() { return Given ? Text : nullptr; }
};

// The creature ID names; for a missing one (0 included) logs "<Name>:
// Creature does not exist." and throws ERROR. For the C++ functions that
// would dereference nullptr.
static TCreature *ExistingCreature(uint32_t ID, const char *Name) {
	TCreature *cr = GetCreature(ID);
	if (cr == nullptr) {
		error("%s: Creature does not exist.\n", Name);
		throw ERROR;
	}
	return cr;
}

// The creature a target that may be nil names: nullptr for nil or a creature
// that is gone.
static TCreature *OptionalCreature(const sol::optional<TScriptCreature> &Target) {
	return Target.has_value() ? GetCreature(Target->ID) : nullptr;
}

// The object an obj argument names: an Item's object, or a Creature's
// (GetCreatureObject; a creature that is gone logs "<Name>: Creature does
// not exist." and throws ERROR).
static Object ThingObject(const TScriptThing &Thing, const char *Name) {
	if (Thing.IsCreature) {
		return ExistingCreature(Thing.Value, Name)->GetCreatureObject();
	}
	return Object(Thing.Value);
}

// The player a Player or a Vocation names, for its methods: "nullptr:
// checked"; a creature that is no player has none of them (binding's check,
// which throws NotPlayer).
static TPlayer *ExistingPlayer(uint32_t ID, const char *Name, const char *NotPlayer) {
	TCreature *cr = ExistingCreature(ID, Name);
	if (cr->Type != PLAYER) {
		throw NotPlayer;
	}
	return static_cast<TPlayer *>(cr);
}

// Throws (a const char *) for a radius the circle loops of the shapes cannot
// take: Radius must be below Limit (MAX_CIRCLES for the loops "i <= Radius",
// MAX_CIRCLES + 1 for MassCreateField's "i < Radius"). A negative radius
// passes; the loops then do nothing, as in C++.
static void CheckRadius(int Radius, int Limit, const char *Message) {
	if (Radius >= Limit) {
		throw Message;
	}
}

// ---------------------------------------------------------------------------
// Magic

// Magic.computeDamage(actor, spell, damage, variation) -> int:
// ComputeDamage(cr, SpellNr, Damage, Variation): Damage varied by
// random(-Variation, Variation), then scaled for a player by level and magic
// level. nullptr: passed on (ComputeDamage gives the varied damage then).
// spell must be 0 to MAX_SPELLS - 1 (binding's check). Throws nothing else.
static int MagicComputeDamage(TScriptCreature Actor, int SpellNr, int Damage, int Variation) {
	if (SpellNr < 0 || SpellNr >= MAX_SPELLS) {
		throw "Magic.computeDamage: spell number out of range";
	}
	return ComputeDamage(GetCreature(Actor.ID), SpellNr, Damage, Variation);
}

// Magic.heal(actor, mana, soul, amount): Heal(cr, Mana, SoulPoints, Healing).
// nullptr: passed on. Throws what Heal throws.
static void MagicHeal(TScriptCreature Actor, int Mana, int SoulPoints, int Healing) {
	Heal(GetCreature(Actor.ID), Mana, SoulPoints, Healing);
}

// Magic.massHeal(actor, obj, mana, soul, amount, radius): MassHeal(cr,
// Target, Mana, SoulPoints, Healing, Radius). nullptr: passed on. radius at
// most MAX_CIRCLES - 1 (binding's check). Throws what MassHeal throws.
static void MagicMassHeal(TScriptCreature Actor, TScriptThing Target, int Mana, int SoulPoints, int Healing,
		int Radius) {
	CheckRadius(Radius, MAX_CIRCLES, "Magic.massHeal: radius beyond the circles");
	Object Obj = ThingObject(Target, "Magic.massHeal");
	MassHeal(GetCreature(Actor.ID), Obj, Mana, SoulPoints, Healing, Radius);
}

// Magic.healFriend(actor, name, mana, soul, amount): HealFriend(cr, Name,
// Mana, SoulPoints, Healing), name copied. nullptr: passed on. Throws what
// HealFriend throws.
static void MagicHealFriend(TScriptCreature Actor, std::string Name, int Mana, int SoulPoints, int Healing) {
	TWord Word(Name);
	HealFriend(GetCreature(Actor.ID), Word.Text, Mana, SoulPoints, Healing);
}

// Magic.combat(actor, obj, mana, soul, damage, effect, animation,
// damageType): Combat(cr, Target, Mana, SoulPoints, Damage, EffectNr,
// AnimationNr, DmgType), damageType converted to unsigned short as the C++
// call converted its int constant. nullptr: passed on (Combat checks the
// target first, as before). Throws what Combat throws.
static void MagicCombat(TScriptCreature Actor, TScriptThing Target, int Mana, int SoulPoints, int Damage,
		int EffectNr, int AnimationNr, int DmgType) {
	Object Obj = ThingObject(Target, "Magic.combat");
	Combat(GetCreature(Actor.ID), Obj, Mana, SoulPoints, Damage, EffectNr, AnimationNr,
			static_cast<unsigned short>(DmgType));
}

// Magic.massCombat(actor, obj, mana, soul, damage, effect, radius,
// damageType, animation): MassCombat(cr, Target, Mana, SoulPoints, Damage,
// EffectNr, Radius, DmgType, Animation). nullptr: passed on. radius at most
// MAX_CIRCLES - 1 (binding's check). Throws what MassCombat throws.
static void MagicMassCombat(TScriptCreature Actor, TScriptThing Target, int Mana, int SoulPoints, int Damage,
		int EffectNr, int Radius, int DmgType, int Animation) {
	CheckRadius(Radius, MAX_CIRCLES, "Magic.massCombat: radius beyond the circles");
	Object Obj = ThingObject(Target, "Magic.massCombat");
	MassCombat(GetCreature(Actor.ID), Obj, Mana, SoulPoints, Damage, EffectNr, Radius,
			static_cast<unsigned short>(DmgType), Animation);
}

// Magic.angleCombat(actor, mana, soul, damage, effect, radius, angle,
// damageType): AngleCombat(cr, Mana, SoulPoints, Damage, EffectNr, Radius,
// Angle, DmgType). nullptr: passed on. Throws what AngleCombat throws.
static void MagicAngleCombat(TScriptCreature Actor, int Mana, int SoulPoints, int Damage, int EffectNr,
		int Radius, int Angle, int DmgType) {
	AngleCombat(GetCreature(Actor.ID), Mana, SoulPoints, Damage, EffectNr, Radius, Angle,
			static_cast<unsigned short>(DmgType));
}

// Magic.killAllMonsters(actor, effect, radius): KillAllMonsters(cr,
// EffectNr, Radius). nullptr: passed on. radius at most MAX_CIRCLES - 1
// (binding's check). Throws what KillAllMonsters throws.
static void MagicKillAllMonsters(TScriptCreature Actor, int EffectNr, int Radius) {
	CheckRadius(Radius, MAX_CIRCLES, "Magic.killAllMonsters: radius beyond the circles");
	KillAllMonsters(GetCreature(Actor.ID), EffectNr, Radius);
}

// Magic.createField(actor, mana, soul, fieldType): CreateField(cr, Mana,
// SoulPoints, Type), on the field the creature faces. nullptr: passed on.
// Throws what CreateField throws.
static void MagicCreateField(TScriptCreature Actor, int Mana, int SoulPoints, int Type) {
	CreateField(GetCreature(Actor.ID), Mana, SoulPoints, Type);
}

// Magic.createFieldOn(actor, obj, mana, soul, fieldType): CreateField(cr,
// Obj, Mana, SoulPoints, Type). nullptr: passed on. Throws what CreateField
// throws.
static void MagicCreateFieldOn(TScriptCreature Actor, TScriptThing Target, int Mana, int SoulPoints, int Type) {
	Object Obj = ThingObject(Target, "Magic.createFieldOn");
	CreateField(GetCreature(Actor.ID), Obj, Mana, SoulPoints, Type);
}

// Magic.massCreateField(actor, obj, mana, soul, fieldType, radius):
// MassCreateField(cr, Obj, Mana, SoulPoints, Type, Radius). nullptr: passed
// on. radius at most MAX_CIRCLES (its loop is "i < Radius"; binding's check).
// Throws what MassCreateField throws.
static void MagicMassCreateField(TScriptCreature Actor, TScriptThing Target, int Mana, int SoulPoints, int Type,
		int Radius) {
	CheckRadius(Radius, MAX_CIRCLES + 1, "Magic.massCreateField: radius beyond the circles");
	Object Obj = ThingObject(Target, "Magic.massCreateField");
	MassCreateField(GetCreature(Actor.ID), Obj, Mana, SoulPoints, Type, Radius);
}

// Magic.createFieldWall(actor, obj, mana, soul, fieldType, extension):
// CreateFieldWall(cr, Obj, Mana, SoulPoints, Type, Extension). nullptr:
// passed on. Throws what CreateFieldWall throws.
static void MagicCreateFieldWall(TScriptCreature Actor, TScriptThing Target, int Mana, int SoulPoints, int Type,
		int Extension) {
	Object Obj = ThingObject(Target, "Magic.createFieldWall");
	CreateFieldWall(GetCreature(Actor.ID), Obj, Mana, SoulPoints, Type, Extension);
}

// Magic.deleteField(actor, obj, mana, soul): DeleteField(cr, Obj, Mana,
// SoulPoints). nullptr: passed on. Throws what DeleteField throws.
static void MagicDeleteField(TScriptCreature Actor, TScriptThing Target, int Mana, int SoulPoints) {
	Object Obj = ThingObject(Target, "Magic.deleteField");
	DeleteField(GetCreature(Actor.ID), Obj, Mana, SoulPoints);
}

// Magic.cleanupField(actor): CleanupField(cr), the gamemaster's cleanup of
// the field in front. nullptr: passed on. Throws what CleanupField throws.
static void MagicCleanupField(TScriptCreature Actor) {
	CleanupField(GetCreature(Actor.ID));
}

// Magic.cleanupFieldOn(actor, obj, mana, soul): CleanupField(cr, Obj, Mana,
// SoulPoints). nullptr: passed on. Throws what CleanupField throws.
static void MagicCleanupFieldOn(TScriptCreature Actor, TScriptThing Target, int Mana, int SoulPoints) {
	Object Obj = ThingObject(Target, "Magic.cleanupFieldOn");
	CleanupField(GetCreature(Actor.ID), Obj, Mana, SoulPoints);
}

// Magic.magicRope(actor, mana, soul): MagicRope(cr, Mana, SoulPoints).
// nullptr: passed on. Throws what MagicRope throws.
static void MagicMagicRope(TScriptCreature Actor, int Mana, int SoulPoints) {
	MagicRope(GetCreature(Actor.ID), Mana, SoulPoints);
}

// Magic.magicClimbing(actor, mana, soul, direction): MagicClimbing(cr, Mana,
// SoulPoints, Direction), direction copied. nullptr: passed on. Throws what
// MagicClimbing throws.
static void MagicMagicClimbing(TScriptCreature Actor, int Mana, int SoulPoints, std::string Direction) {
	TWord Word(Direction);
	MagicClimbing(GetCreature(Actor.ID), Mana, SoulPoints, Word.Text);
}

// Magic.magicClimbingFree(actor, direction): MagicClimbing(cr, Direction),
// the gamemaster's levitation, direction copied. nullptr: passed on. Throws
// what MagicClimbing throws.
static void MagicMagicClimbingFree(TScriptCreature Actor, std::string Direction) {
	TWord Word(Direction);
	MagicClimbing(GetCreature(Actor.ID), Word.Text);
}

// Magic.createFood(actor, mana, soul): CreateFood(cr, Mana, SoulPoints).
// nullptr: passed on. Throws what CreateFood throws.
static void MagicCreateFood(TScriptCreature Actor, int Mana, int SoulPoints) {
	CreateFood(GetCreature(Actor.ID), Mana, SoulPoints);
}

// Magic.createArrows(actor, mana, soul, ammo, amount): CreateArrows(cr,
// Mana, SoulPoints, Ammo, Amount), ammo one of the CONJURE_ constants.
// nullptr: passed on. Throws what CreateArrows throws.
static void MagicCreateArrows(TScriptCreature Actor, int Mana, int SoulPoints, int Ammo, int Amount) {
	CreateArrows(GetCreature(Actor.ID), Mana, SoulPoints, Ammo, Amount);
}

// Magic.summonCreature(actor, mana, raceName, god): SummonCreature(cr, Mana,
// RaceName, God), raceName copied. nullptr: passed on. Throws what
// SummonCreature throws.
static void MagicSummonCreature(TScriptCreature Actor, int Mana, std::string RaceName, bool God) {
	TWord Word(RaceName);
	SummonCreature(GetCreature(Actor.ID), Mana, Word.Text, God);
}

// Magic.raiseDead(actor, obj, mana, soul): RaiseDead(cr, Obj, Mana,
// SoulPoints). nullptr: passed on. Throws what RaiseDead throws.
static void MagicRaiseDead(TScriptCreature Actor, TScriptThing Target, int Mana, int SoulPoints) {
	Object Obj = ThingObject(Target, "Magic.raiseDead");
	RaiseDead(GetCreature(Actor.ID), Obj, Mana, SoulPoints);
}

// Magic.massRaiseDead(actor, obj, mana, soul, radius): MassRaiseDead(cr,
// Target, Mana, SoulPoints, Radius). nullptr: passed on. radius at most
// MAX_CIRCLES - 1 (binding's check). Throws what MassRaiseDead throws.
static void MagicMassRaiseDead(TScriptCreature Actor, TScriptThing Target, int Mana, int SoulPoints, int Radius) {
	CheckRadius(Radius, MAX_CIRCLES, "Magic.massRaiseDead: radius beyond the circles");
	Object Obj = ThingObject(Target, "Magic.massRaiseDead");
	MassRaiseDead(GetCreature(Actor.ID), Obj, Mana, SoulPoints, Radius);
}

// Magic.magicGoStrength(actor, target|nil, mana, soul, percent, duration):
// MagicGoStrength(cr, dest, Mana, SoulPoints, percent, duration). nullptr:
// passed on, for both (MagicGoStrength checks the target first, as before).
// Throws what MagicGoStrength throws.
static void MagicMagicGoStrength(TScriptCreature Actor, sol::optional<TScriptCreature> Target, int Mana,
		int SoulPoints, int Percent, int Duration) {
	TCreature *cr = GetCreature(Actor.ID);
	TCreature *dest = OptionalCreature(Target);
	MagicGoStrength(cr, dest, Mana, SoulPoints, Percent, Duration);
}

// Magic.shielding(actor, mana, soul, duration): Shielding(cr, Mana,
// SoulPoints, Duration). nullptr: passed on. Throws what Shielding throws.
static void MagicShielding(TScriptCreature Actor, int Mana, int SoulPoints, int Duration) {
	Shielding(GetCreature(Actor.ID), Mana, SoulPoints, Duration);
}

// Magic.negatePoison(actor, mana, soul, target|nil): NegatePoison(cr, Mana,
// SoulPoints, dest). nullptr: passed on, for both. Throws what NegatePoison
// throws.
static void MagicNegatePoison(TScriptCreature Actor, int Mana, int SoulPoints,
		sol::optional<TScriptCreature> Target) {
	TCreature *cr = GetCreature(Actor.ID);
	TCreature *dest = OptionalCreature(Target);
	NegatePoison(cr, Mana, SoulPoints, dest);
}

// Magic.enlight(actor, mana, soul, radius, duration): Enlight(cr, Mana,
// SoulPoints, Radius, Duration). nullptr: passed on. radius must not be 0
// (Enlight divides by it; binding's check). Throws what Enlight throws.
static void MagicEnlight(TScriptCreature Actor, int Mana, int SoulPoints, int Radius, int Duration) {
	if (Radius == 0) {
		throw "Magic.enlight: radius 0";
	}
	Enlight(GetCreature(Actor.ID), Mana, SoulPoints, Radius, Duration);
}

// Magic.invisibility(actor, mana, soul, duration): Invisibility(cr, Mana,
// SoulPoints, Duration). nullptr: passed on. Throws what Invisibility throws.
static void MagicInvisibility(TScriptCreature Actor, int Mana, int SoulPoints, int Duration) {
	Invisibility(GetCreature(Actor.ID), Mana, SoulPoints, Duration);
}

// Magic.cancelInvisibility(actor, obj, mana, soul, radius):
// CancelInvisibility(cr, Target, Mana, SoulPoints, Radius). nullptr: passed
// on. radius at most MAX_CIRCLES - 1 (binding's check). Throws what
// CancelInvisibility throws.
static void MagicCancelInvisibility(TScriptCreature Actor, TScriptThing Target, int Mana, int SoulPoints,
		int Radius) {
	CheckRadius(Radius, MAX_CIRCLES, "Magic.cancelInvisibility: radius beyond the circles");
	Object Obj = ThingObject(Target, "Magic.cancelInvisibility");
	CancelInvisibility(GetCreature(Actor.ID), Obj, Mana, SoulPoints, Radius);
}

// Magic.creatureIllusion(actor, mana, soul, raceName, duration):
// CreatureIllusion(cr, Mana, SoulPoints, RaceName, Duration), raceName
// copied. nullptr: passed on. Throws what CreatureIllusion throws.
static void MagicCreatureIllusion(TScriptCreature Actor, int Mana, int SoulPoints, std::string RaceName,
		int Duration) {
	TWord Word(RaceName);
	CreatureIllusion(GetCreature(Actor.ID), Mana, SoulPoints, Word.Text, Duration);
}

// Magic.objectIllusion(actor, mana, soul, obj, duration): ObjectIllusion(cr,
// Mana, SoulPoints, Obj, Duration). nullptr: passed on. Throws what
// ObjectIllusion throws.
static void MagicObjectIllusion(TScriptCreature Actor, int Mana, int SoulPoints, TScriptThing Target,
		int Duration) {
	Object Obj = ThingObject(Target, "Magic.objectIllusion");
	ObjectIllusion(GetCreature(Actor.ID), Mana, SoulPoints, Obj, Duration);
}

// Magic.enchantObject(actor, mana, soul, originType, targetType):
// EnchantObject(cr, Mana, SoulPoints, Origin, Target), the object types made
// from their IDs (ObjectType(TypeID), which logs an ID that is no type and
// takes 0 then; ItemType.fromOldNumber gives IDs that are types or 0).
// nullptr: passed on. Throws what EnchantObject throws.
static void MagicEnchantObject(TScriptCreature Actor, int Mana, int SoulPoints, int Origin, int Target) {
	EnchantObject(GetCreature(Actor.ID), Mana, SoulPoints, ObjectType(Origin), ObjectType(Target));
}

// Magic.convince(actor, target|nil): Convince(master, slave). nullptr:
// checked, for both (Convince dereferences both; the actor is checked
// first). Throws what Convince throws.
static void MagicConvince(TScriptCreature Actor, sol::optional<TScriptCreature> Target) {
	TCreature *Master = ExistingCreature(Actor.ID, "Magic.convince");
	TCreature *Slave = ExistingCreature(Target.has_value() ? Target->ID : 0, "Magic.convince");
	Convince(Master, Slave);
}

// Magic.challenge(actor, mana, soul, radius): Challenge(cr, Mana, SoulPoints,
// Radius). nullptr: passed on (Challenge dereferences nothing before
// SpendManaAndSoulPoints, which logs and throws ERROR for it). radius at most
// MAX_CIRCLES - 1 (binding's check). Throws what Challenge throws.
static void MagicChallenge(TScriptCreature Actor, int Mana, int SoulPoints, int Radius) {
	CheckRadius(Radius, MAX_CIRCLES, "Magic.challenge: radius beyond the circles");
	Challenge(GetCreature(Actor.ID), Mana, SoulPoints, Radius);
}

// Magic.findPerson(actor, mana, soul, name): FindPerson(cr, Mana,
// SoulPoints, Name), name copied. nullptr: passed on. Throws what FindPerson
// throws.
static void MagicFindPerson(TScriptCreature Actor, int Mana, int SoulPoints, std::string Name) {
	TWord Word(Name);
	FindPerson(GetCreature(Actor.ID), Mana, SoulPoints, Word.Text);
}

// ---------------------------------------------------------------------------
// Command: every function checks for nullptr itself (it logs and throws
// ERROR), so each binding passes it on; the strings are copied (TWord). Each
// throws what its C++ function throws.

// Command.createThing(actor, p1, p2|nil): CreateThing(cr, Param1, Param2).
static void CommandCreateThing(TScriptCreature Actor, std::string Param1, sol::optional<std::string> Param2) {
	TWord Word1(Param1);
	TOptionalWord Word2(Param2);
	CreateThing(GetCreature(Actor.ID), Word1.Text, Word2.get());
}

// Command.teleport(actor, p): Teleport(cr, Param).
static void CommandTeleport(TScriptCreature Actor, std::string Param) {
	TWord Word(Param);
	Teleport(GetCreature(Actor.ID), Word.Text);
}

// Command.createKnowledge(actor, p1, p2|nil): CreateKnowledge(cr, Param1,
// Param2).
static void CommandCreateKnowledge(TScriptCreature Actor, std::string Param1, sol::optional<std::string> Param2) {
	TWord Word1(Param1);
	TOptionalWord Word2(Param2);
	CreateKnowledge(GetCreature(Actor.ID), Word1.Text, Word2.get());
}

// Command.changeData(actor, p): ChangeData(cr, Param).
static void CommandChangeData(TScriptCreature Actor, std::string Param) {
	TWord Word(Param);
	ChangeData(GetCreature(Actor.ID), Word.Text);
}

// Command.teleportToCreature(actor, name): TeleportToCreature(cr, Name).
static void CommandTeleportToCreature(TScriptCreature Actor, std::string Name) {
	TWord Word(Name);
	TeleportToCreature(GetCreature(Actor.ID), Word.Text);
}

// Command.teleportPlayerToMe(actor, name): TeleportPlayerToMe(cr, Name).
static void CommandTeleportPlayerToMe(TScriptCreature Actor, std::string Name) {
	TWord Word(Name);
	TeleportPlayerToMe(GetCreature(Actor.ID), Word.Text);
}

// Command.showPosition(actor): GetPosition(cr), which tells the caster its
// position.
static void CommandShowPosition(TScriptCreature Actor) {
	GetPosition(GetCreature(Actor.ID));
}

// Command.createMoney(actor, p): CreateMoney(cr, Param).
static void CommandCreateMoney(TScriptCreature Actor, std::string Param) {
	TWord Word(Param);
	CreateMoney(GetCreature(Actor.ID), Word.Text);
}

// Command.changeProfession(actor, p): ChangeProfession(cr, Param).
static void CommandChangeProfession(TScriptCreature Actor, std::string Param) {
	TWord Word(Param);
	ChangeProfession(GetCreature(Actor.ID), Word.Text);
}

// Command.editGuests(actor): EditGuests(cr).
static void CommandEditGuests(TScriptCreature Actor) {
	EditGuests(GetCreature(Actor.ID));
}

// Command.editSubowners(actor): EditSubowners(cr).
static void CommandEditSubowners(TScriptCreature Actor) {
	EditSubowners(GetCreature(Actor.ID));
}

// Command.kickGuest(actor, name): KickGuest(cr, Name).
static void CommandKickGuest(TScriptCreature Actor, std::string Name) {
	TWord Word(Name);
	KickGuest(GetCreature(Actor.ID), Word.Text);
}

// Command.editNameDoor(actor): EditNameDoor(cr).
static void CommandEditNameDoor(TScriptCreature Actor) {
	EditNameDoor(GetCreature(Actor.ID));
}

// Command.getQuestValue(actor, p): GetQuestValue(cr, Param).
static void CommandGetQuestValue(TScriptCreature Actor, std::string Param) {
	TWord Word(Param);
	GetQuestValue(GetCreature(Actor.ID), Word.Text);
}

// Command.setQuestValue(actor, p1, p2): SetQuestValue(cr, Param1, Param2).
static void CommandSetQuestValue(TScriptCreature Actor, std::string Param1, std::string Param2) {
	TWord Word1(Param1);
	TWord Word2(Param2);
	SetQuestValue(GetCreature(Actor.ID), Word1.Text, Word2.Text);
}

// Command.clearQuestValues(actor): ClearQuestValues(cr).
static void CommandClearQuestValues(TScriptCreature Actor) {
	ClearQuestValues(GetCreature(Actor.ID));
}

// Command.startMonsterraid(actor, name): StartMonsterraid(cr, RaidName),
// which lowers the copy. The const char * of a raid that fails to load is
// caught inside StartMonsterraid, as before.
static void CommandStartMonsterraid(TScriptCreature Actor, std::string RaidName) {
	TWord Word(RaidName);
	StartMonsterraid(GetCreature(Actor.ID), Word.Text);
}

// Command.homeTeleport(actor, name): HomeTeleport(cr, Name).
static void CommandHomeTeleport(TScriptCreature Actor, std::string Name) {
	TWord Word(Name);
	HomeTeleport(GetCreature(Actor.ID), Word.Text);
}

// Command.banishAccount(actor, name, n, reason): BanishAccount(cr, Name, n,
// Reason).
static void CommandBanishAccount(TScriptCreature Actor, std::string Name, int Number, std::string Reason) {
	TWord NameWord(Name);
	TWord ReasonWord(Reason);
	BanishAccount(GetCreature(Actor.ID), NameWord.Text, Number, ReasonWord.Text);
}

// Command.deleteAccount(actor, name, reason): DeleteAccount(cr, Name,
// Reason).
static void CommandDeleteAccount(TScriptCreature Actor, std::string Name, std::string Reason) {
	TWord NameWord(Name);
	TWord ReasonWord(Reason);
	DeleteAccount(GetCreature(Actor.ID), NameWord.Text, ReasonWord.Text);
}

// Command.setNameRule(actor, name): SetNameRule(cr, Name).
static void CommandSetNameRule(TScriptCreature Actor, std::string Name) {
	TWord Word(Name);
	SetNameRule(GetCreature(Actor.ID), Word.Text);
}

// Command.notation(actor, name, comment): Notation(cr, Name, Comment).
static void CommandNotation(TScriptCreature Actor, std::string Name, std::string Comment) {
	TWord NameWord(Name);
	TWord CommentWord(Comment);
	Notation(GetCreature(Actor.ID), NameWord.Text, CommentWord.Text);
}

// Command.nameLock(actor, name): NameLock(cr, Name).
static void CommandNameLock(TScriptCreature Actor, std::string Name) {
	TWord Word(Name);
	NameLock(GetCreature(Actor.ID), Word.Text);
}

// Command.kickPlayer(actor, name): KickPlayer(cr, Name).
static void CommandKickPlayer(TScriptCreature Actor, std::string Name) {
	TWord Word(Name);
	KickPlayer(GetCreature(Actor.ID), Word.Text);
}

// Command.deleteCharacter(actor, name, reason): DeleteCharacter(cr, Name,
// Reason).
static void CommandDeleteCharacter(TScriptCreature Actor, std::string Name, std::string Reason) {
	TWord NameWord(Name);
	TWord ReasonWord(Reason);
	DeleteCharacter(GetCreature(Actor.ID), NameWord.Text, ReasonWord.Text);
}

// Command.ipBanishment(actor, name, reason): IPBanishment(cr, Name, Reason).
static void CommandIPBanishment(TScriptCreature Actor, std::string Name, std::string Reason) {
	TWord NameWord(Name);
	TWord ReasonWord(Reason);
	IPBanishment(GetCreature(Actor.ID), NameWord.Text, ReasonWord.Text);
}

// Command.banishCharacter(actor, name, n, reason): BanishCharacter(cr, Name,
// n, Reason).
static void CommandBanishCharacter(TScriptCreature Actor, std::string Name, int Number, std::string Reason) {
	TWord NameWord(Name);
	TWord ReasonWord(Reason);
	BanishCharacter(GetCreature(Actor.ID), NameWord.Text, Number, ReasonWord.Text);
}

// ---------------------------------------------------------------------------
// Game, the classes' methods, ItemType

// Game.toInt(text) -> int: (int)std::clamp(strtol(Text, nullptr, 10),
// (long)INT32_MIN, (long)INT32_MAX) of the copy, as AccountRightSpell read
// the numbers of the words. Throws nothing.
static int GameToInt(std::string Text) {
	TWord Word(Text);
	return static_cast<int>(std::clamp(strtol(Word.Text, nullptr, 10), static_cast<long>(INT32_MIN),
			static_cast<long>(INT32_MAX)));
}

// creature:getSkillValue(skill) -> Skills[skill]->Get(), the skill's value
// with its temporary changes. nullptr: checked. skill must be 0 to
// MAX_SKILLS - 1 and exist for the creature (binding's check).
static int CreatureGetSkillValue(TScriptCreature Self, int SkillNr) {
	TCreature *cr = ExistingCreature(Self.ID, "Creature.getSkillValue");
	if (SkillNr < 0 || SkillNr >= MAX_SKILLS || cr->Skills[SkillNr] == nullptr) {
		throw "Creature.getSkillValue: no such skill";
	}
	return cr->Skills[SkillNr]->Get();
}

// player:getLevel() -> Skills[SKILL_LEVEL]->Get(). nullptr: checked.
static int PlayerGetLevel(TScriptPlayer Self) {
	TCreature *cr = ExistingCreature(Self.ID, "Player.getLevel");
	if (cr->Skills[SKILL_LEVEL] == nullptr) {
		throw "Player.getLevel: no such skill";
	}
	return cr->Skills[SKILL_LEVEL]->Get();
}

// player:getVocation() -> the player's Vocation (an object holding the
// player's ID; its methods resolve it again). nullptr: checked; a creature
// that is no player has none (binding's check).
static TScriptVocation PlayerGetVocation(TScriptPlayer Self) {
	ExistingPlayer(Self.ID, "Player.getVocation", "Player.getVocation: the creature is not a player");
	return TScriptVocation{Self.ID};
}

// vocation:getId() -> GetRealProfession(), the profession with its promotion
// (VOCATION_ constants; VOCATION_PROMOTION + VOCATION_KNIGHT for an elite
// knight). nullptr: checked; no player: binding's check.
static int VocationGetId(TScriptVocation Self) {
	return ExistingPlayer(Self.PlayerID, "Vocation.getId", "Vocation.getId: the creature is not a player")
			->GetRealProfession();
}

// vocation:getBaseId() -> GetEffectiveProfession(), the profession without
// its promotion. nullptr: checked; no player: binding's check.
static int VocationGetBaseId(TScriptVocation Self) {
	return ExistingPlayer(Self.PlayerID, "Vocation.getBaseId",
			"Vocation.getBaseId: the creature is not a player")->GetEffectiveProfession();
}

// ItemType.fromOldNumber(group, number) -> GetNewObjectType(Group,
// Number).getTypeID(), the type's ID (0 for a group and number without a
// type, which GetNewObjectType logs). group and number must be 0 to 255
// (binding's check).
static int ItemTypeFromOldNumber(int Group, int Number) {
	if (Group < 0 || Group > 255 || Number < 0 || Number > 255) {
		throw "ItemType.fromOldNumber: group and number must be 0 to 255";
	}
	return GetNewObjectType(static_cast<unsigned char>(Group), static_cast<unsigned char>(Number)).getTypeID();
}

// A constant of the scripts: its name and its value.
struct TScriptConstant {
	const char *Name;
	int Value;
};

// The constants, TFS's names with 7.72's values. The effects and
// projectiles without a constant of operate.hh (which names only the ones the
// code uses) have the client's numbers: effects 1 to 25, projectiles 0 to 15.
static const TScriptConstant Constants[] = {
	{"CONST_ME_NONE", 0},
	{"CONST_ME_DRAWBLOOD", EFFECT_BLOOD_HIT},
	{"CONST_ME_LOSEENERGY", EFFECT_MANA_HIT},
	{"CONST_ME_POFF", EFFECT_POFF},
	{"CONST_ME_BLOCKHIT", EFFECT_BLOCK_HIT},
	{"CONST_ME_EXPLOSIONAREA", EFFECT_EXPLOSION_AREA},
	{"CONST_ME_EXPLOSIONHIT", EFFECT_EXPLOSION},
	{"CONST_ME_FIREAREA", EFFECT_FIRE_AREA},
	{"CONST_ME_YELLOW_RINGS", 8},
	{"CONST_ME_GREEN_RINGS", EFFECT_POISON},
	{"CONST_ME_HITAREA", EFFECT_HIT_AREA},
	{"CONST_ME_TELEPORT", EFFECT_TELEPORT},
	{"CONST_ME_ENERGYHIT", EFFECT_ENERGY_HIT},
	{"CONST_ME_MAGIC_BLUE", EFFECT_MAGIC_BLUE},
	{"CONST_ME_MAGIC_RED", EFFECT_MAGIC_RED},
	{"CONST_ME_MAGIC_GREEN", EFFECT_MAGIC_GREEN},
	{"CONST_ME_HITBYFIRE", EFFECT_FIRE_HIT},
	{"CONST_ME_HITBYPOISON", EFFECT_POISON_HIT},
	{"CONST_ME_MORTAREA", EFFECT_DEATH},
	{"CONST_ME_SOUND_GREEN", 19},
	{"CONST_ME_SOUND_RED", 20},
	{"CONST_ME_POISONAREA", 21},
	{"CONST_ME_SOUND_YELLOW", 22},
	{"CONST_ME_SOUND_PURPLE", 23},
	{"CONST_ME_SOUND_BLUE", 24},
	{"CONST_ME_SOUND_WHITE", 25},

	{"CONST_ANI_NONE", ANIMATION_NONE},
	{"CONST_ANI_SPEAR", 1},
	{"CONST_ANI_BOLT", 2},
	{"CONST_ANI_ARROW", 3},
	{"CONST_ANI_FIRE", ANIMATION_FIRE},
	{"CONST_ANI_ENERGY", ANIMATION_ENERGY},
	{"CONST_ANI_POISONARROW", 6},
	{"CONST_ANI_BURSTARROW", 7},
	{"CONST_ANI_THROWINGSTAR", 8},
	{"CONST_ANI_THROWINGKNIFE", 9},
	{"CONST_ANI_SMALLSTONE", 10},
	{"CONST_ANI_DEATH", ANIMATION_DEATH},
	{"CONST_ANI_LARGEROCK", 12},
	{"CONST_ANI_SNOWBALL", 13},
	{"CONST_ANI_POWERBOLT", 14},
	{"CONST_ANI_POISON", 15},

	// The damage types of cr.hh; 7.72 calls the earth damage of later
	// versions poison, so it has both names.
	{"COMBAT_PHYSICALDAMAGE", DAMAGE_PHYSICAL},
	{"COMBAT_POISONDAMAGE", DAMAGE_POISON},
	{"COMBAT_EARTHDAMAGE", DAMAGE_POISON},
	{"COMBAT_FIREDAMAGE", DAMAGE_FIRE},
	{"COMBAT_ENERGYDAMAGE", DAMAGE_ENERGY},
	{"COMBAT_POISONCONDITION", DAMAGE_POISON_CONDITION},
	{"COMBAT_FIRECONDITION", DAMAGE_FIRE_CONDITION},
	{"COMBAT_ENERGYCONDITION", DAMAGE_ENERGY_CONDITION},
	{"COMBAT_LIFEDRAIN", DAMAGE_LIFEDRAIN},
	{"COMBAT_MANADRAIN", DAMAGE_MANADRAIN},

	// The fields CreateField makes (info.hh).
	{"FIELD_FIRE", FIELD_FIRE},
	{"FIELD_POISON", FIELD_POISON},
	{"FIELD_ENERGY", FIELD_ENERGY},
	{"FIELD_MAGICWALL", FIELD_MAGICWALL},
	{"FIELD_RUSHWOOD", FIELD_RUSHWOOD},

	// The ammunition CreateArrows conjures (magic.hh).
	{"CONJURE_ARROW", CONJURE_ARROW},
	{"CONJURE_POISONED_ARROW", CONJURE_POISONED_ARROW},
	{"CONJURE_EXPLOSIVE_ARROW", CONJURE_EXPLOSIVE_ARROW},
	{"CONJURE_BOLT", CONJURE_BOLT},
	{"CONJURE_POWER_BOLT", CONJURE_POWER_BOLT},

	// The skills of crskill.hh: TFS's names for the ones TFS has.
	{"SKILL_LEVEL", SKILL_LEVEL},
	{"SKILL_MAGLEVEL", SKILL_MAGICLEVEL},
	{"SKILL_HITPOINTS", SKILL_HITPOINTS},
	{"SKILL_MANA", SKILL_MANA},
	{"SKILL_GOSTRENGTH", SKILL_GOSTRENGTH},
	{"SKILL_CARRYSTRENGTH", SKILL_CARRYSTRENGTH},
	{"SKILL_SHIELD", SKILL_SHIELDING},
	{"SKILL_DISTANCE", SKILL_DISTANCEFIGHTING},
	{"SKILL_SWORD", SKILL_SWORDFIGHTING},
	{"SKILL_CLUB", SKILL_CLUBFIGHTING},
	{"SKILL_AXE", SKILL_AXEFIGHTING},
	{"SKILL_FIST", SKILL_FISTFIGHTING},
	{"SKILL_MAGICDEFENSE", SKILL_MAGICDEFENSE},
	{"SKILL_FISHING", SKILL_FISHING},
	{"SKILL_EATING", SKILL_EATING},
	{"SKILL_SHINING", SKILL_SHINING},
	{"SKILL_ILLUSION", SKILL_ILLUSION},
	{"SKILL_POISON", SKILL_POISON},
	{"SKILL_BURNING", SKILL_BURNING},
	{"SKILL_ENERGY", SKILL_ENERGY},
	{"SKILL_DRUNKEN", SKILL_DRUNKEN},
	{"SKILL_MANASHIELD", SKILL_MANASHIELD},
	{"SKILL_SOULPOINTS", SKILL_SOULPOINTS},

	// The professions of cr.hh (vocation:getId, vocation:getBaseId).
	{"VOCATION_NONE", PROFESSION_NONE},
	{"VOCATION_KNIGHT", PROFESSION_KNIGHT},
	{"VOCATION_PALADIN", PROFESSION_PALADIN},
	{"VOCATION_SORCERER", PROFESSION_SORCERER},
	{"VOCATION_DRUID", PROFESSION_DRUID},
	{"VOCATION_PROMOTION", PROFESSION_PROMOTION},
};

void RegisterMagicAPI() {
	sol::table Magic = ScriptAPITable("Magic");
	Magic.set_function("computeDamage", Guarded(&MagicComputeDamage));
	Magic.set_function("heal", Guarded(&MagicHeal));
	Magic.set_function("massHeal", Guarded(&MagicMassHeal));
	Magic.set_function("healFriend", Guarded(&MagicHealFriend));
	Magic.set_function("combat", Guarded(&MagicCombat));
	Magic.set_function("massCombat", Guarded(&MagicMassCombat));
	Magic.set_function("angleCombat", Guarded(&MagicAngleCombat));
	Magic.set_function("killAllMonsters", Guarded(&MagicKillAllMonsters));
	Magic.set_function("createField", Guarded(&MagicCreateField));
	Magic.set_function("createFieldOn", Guarded(&MagicCreateFieldOn));
	Magic.set_function("massCreateField", Guarded(&MagicMassCreateField));
	Magic.set_function("createFieldWall", Guarded(&MagicCreateFieldWall));
	Magic.set_function("deleteField", Guarded(&MagicDeleteField));
	Magic.set_function("cleanupField", Guarded(&MagicCleanupField));
	Magic.set_function("cleanupFieldOn", Guarded(&MagicCleanupFieldOn));
	Magic.set_function("magicRope", Guarded(&MagicMagicRope));
	Magic.set_function("magicClimbing", Guarded(&MagicMagicClimbing));
	Magic.set_function("magicClimbingFree", Guarded(&MagicMagicClimbingFree));
	Magic.set_function("createFood", Guarded(&MagicCreateFood));
	Magic.set_function("createArrows", Guarded(&MagicCreateArrows));
	Magic.set_function("summonCreature", Guarded(&MagicSummonCreature));
	Magic.set_function("raiseDead", Guarded(&MagicRaiseDead));
	Magic.set_function("massRaiseDead", Guarded(&MagicMassRaiseDead));
	Magic.set_function("magicGoStrength", Guarded(&MagicMagicGoStrength));
	Magic.set_function("shielding", Guarded(&MagicShielding));
	Magic.set_function("negatePoison", Guarded(&MagicNegatePoison));
	Magic.set_function("enlight", Guarded(&MagicEnlight));
	Magic.set_function("invisibility", Guarded(&MagicInvisibility));
	Magic.set_function("cancelInvisibility", Guarded(&MagicCancelInvisibility));
	Magic.set_function("creatureIllusion", Guarded(&MagicCreatureIllusion));
	Magic.set_function("objectIllusion", Guarded(&MagicObjectIllusion));
	Magic.set_function("enchantObject", Guarded(&MagicEnchantObject));
	Magic.set_function("convince", Guarded(&MagicConvince));
	Magic.set_function("challenge", Guarded(&MagicChallenge));
	Magic.set_function("findPerson", Guarded(&MagicFindPerson));

	sol::table Command = ScriptAPITable("Command");
	Command.set_function("createThing", Guarded(&CommandCreateThing));
	Command.set_function("teleport", Guarded(&CommandTeleport));
	Command.set_function("createKnowledge", Guarded(&CommandCreateKnowledge));
	Command.set_function("changeData", Guarded(&CommandChangeData));
	Command.set_function("teleportToCreature", Guarded(&CommandTeleportToCreature));
	Command.set_function("teleportPlayerToMe", Guarded(&CommandTeleportPlayerToMe));
	Command.set_function("showPosition", Guarded(&CommandShowPosition));
	Command.set_function("createMoney", Guarded(&CommandCreateMoney));
	Command.set_function("changeProfession", Guarded(&CommandChangeProfession));
	Command.set_function("editGuests", Guarded(&CommandEditGuests));
	Command.set_function("editSubowners", Guarded(&CommandEditSubowners));
	Command.set_function("kickGuest", Guarded(&CommandKickGuest));
	Command.set_function("editNameDoor", Guarded(&CommandEditNameDoor));
	Command.set_function("getQuestValue", Guarded(&CommandGetQuestValue));
	Command.set_function("setQuestValue", Guarded(&CommandSetQuestValue));
	Command.set_function("clearQuestValues", Guarded(&CommandClearQuestValues));
	Command.set_function("startMonsterraid", Guarded(&CommandStartMonsterraid));
	Command.set_function("homeTeleport", Guarded(&CommandHomeTeleport));
	Command.set_function("banishAccount", Guarded(&CommandBanishAccount));
	Command.set_function("deleteAccount", Guarded(&CommandDeleteAccount));
	Command.set_function("setNameRule", Guarded(&CommandSetNameRule));
	Command.set_function("notation", Guarded(&CommandNotation));
	Command.set_function("nameLock", Guarded(&CommandNameLock));
	Command.set_function("kickPlayer", Guarded(&CommandKickPlayer));
	Command.set_function("deleteCharacter", Guarded(&CommandDeleteCharacter));
	Command.set_function("ipBanishment", Guarded(&CommandIPBanishment));
	Command.set_function("banishCharacter", Guarded(&CommandBanishCharacter));

	sol::table Game = ScriptAPITable("Game");
	Game.set_function("toInt", Guarded(&GameToInt));

	sol::table Creature = ScriptAPITable("Creature");
	Creature.set_function("getSkillValue", Guarded(&CreatureGetSkillValue));

	sol::table Player = ScriptAPITable("Player");
	Player.set_function("getLevel", Guarded(&PlayerGetLevel));
	Player.set_function("getVocation", Guarded(&PlayerGetVocation));

	sol::table Vocation = ScriptAPITable("Vocation");
	Vocation.set_function("getId", Guarded(&VocationGetId));
	Vocation.set_function("getBaseId", Guarded(&VocationGetBaseId));

	sol::table ItemType = ScriptAPITable("ItemType");
	ItemType.set_function("fromOldNumber", Guarded(&ItemTypeFromOldNumber));

	for (const TScriptConstant &Constant : Constants) {
		SetScriptConstant(Constant.Name, Constant.Value);
	}
}
