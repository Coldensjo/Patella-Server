// scripting_magic.cc: the API of the spells', runes' and commands' scripts
// (scripting.hh, "Spells, runes and commands"): the spell functions of
// magic.cc in the table magic, the commands' functions in command, the few
// reads the cases of the old switches made (game.to_int, creature.object,
// creature.skill, creature.effective_profession, object.new_type) and the
// constant tables effect, animation, damage, field, ammo, skill and
// profession. RegisterMagicAPI binds them; InitAll calls it after
// InitScripting.
//
// What every binding here does alike:
//  - actor and target are creature IDs, resolved with GetCreature on every
//    call; 0 or a creature that is gone gives nullptr. Where the C++ function
//    checks for nullptr itself (it logs and throws ERROR), the binding passes
//    nullptr on, so the result is the same ("nullptr: passed on" below).
//    Where it would dereference nullptr, the binding logs "<lua name>:
//    Creature does not exist." and throws ERROR ("nullptr: checked").
//  - obj is an object handle (Object::getObjectID); the binding passes
//    Object(obj) on, and the C++ function checks it with exists() as before.
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
// magic

// magic.compute_damage(actor, spell, damage, variation) -> int:
// ComputeDamage(cr, SpellNr, Damage, Variation): Damage varied by
// random(-Variation, Variation), then scaled for a player by level and magic
// level. nullptr: passed on (ComputeDamage gives the varied damage then).
// spell must be 0 to MAX_SPELLS - 1 (binding's check). Throws nothing else.
static int MagicComputeDamage(uint32_t Actor, int SpellNr, int Damage, int Variation) {
	if (SpellNr < 0 || SpellNr >= MAX_SPELLS) {
		throw "magic.compute_damage: spell number out of range";
	}
	return ComputeDamage(GetCreature(Actor), SpellNr, Damage, Variation);
}

// magic.heal(actor, mana, soul, amount): Heal(cr, Mana, SoulPoints, Healing).
// nullptr: passed on. Throws what Heal throws.
static void MagicHeal(uint32_t Actor, int Mana, int SoulPoints, int Healing) {
	Heal(GetCreature(Actor), Mana, SoulPoints, Healing);
}

// magic.mass_heal(actor, obj, mana, soul, amount, radius): MassHeal(cr,
// Target, Mana, SoulPoints, Healing, Radius). nullptr: passed on. radius at
// most MAX_CIRCLES - 1 (binding's check). Throws what MassHeal throws.
static void MagicMassHeal(uint32_t Actor, uint32_t Target, int Mana, int SoulPoints, int Healing, int Radius) {
	CheckRadius(Radius, MAX_CIRCLES, "magic.mass_heal: radius beyond the circles");
	MassHeal(GetCreature(Actor), Object(Target), Mana, SoulPoints, Healing, Radius);
}

// magic.heal_friend(actor, name, mana, soul, amount): HealFriend(cr, Name,
// Mana, SoulPoints, Healing), name copied. nullptr: passed on. Throws what
// HealFriend throws.
static void MagicHealFriend(uint32_t Actor, std::string Name, int Mana, int SoulPoints, int Healing) {
	TWord Word(Name);
	HealFriend(GetCreature(Actor), Word.Text, Mana, SoulPoints, Healing);
}

// magic.combat(actor, obj, mana, soul, damage, effect, animation,
// damage_type): Combat(cr, Target, Mana, SoulPoints, Damage, EffectNr,
// AnimationNr, DmgType), damage_type converted to unsigned short as the C++
// call converted its int constant. nullptr: passed on (Combat checks the
// target first, as before). Throws what Combat throws.
static void MagicCombat(uint32_t Actor, uint32_t Target, int Mana, int SoulPoints, int Damage, int EffectNr,
		int AnimationNr, int DmgType) {
	Combat(GetCreature(Actor), Object(Target), Mana, SoulPoints, Damage, EffectNr, AnimationNr,
			static_cast<unsigned short>(DmgType));
}

// magic.mass_combat(actor, obj, mana, soul, damage, effect, radius,
// damage_type, animation): MassCombat(cr, Target, Mana, SoulPoints, Damage,
// EffectNr, Radius, DmgType, Animation). nullptr: passed on. radius at most
// MAX_CIRCLES - 1 (binding's check). Throws what MassCombat throws.
static void MagicMassCombat(uint32_t Actor, uint32_t Target, int Mana, int SoulPoints, int Damage, int EffectNr,
		int Radius, int DmgType, int Animation) {
	CheckRadius(Radius, MAX_CIRCLES, "magic.mass_combat: radius beyond the circles");
	MassCombat(GetCreature(Actor), Object(Target), Mana, SoulPoints, Damage, EffectNr, Radius,
			static_cast<unsigned short>(DmgType), Animation);
}

// magic.angle_combat(actor, mana, soul, damage, effect, radius, angle,
// damage_type): AngleCombat(cr, Mana, SoulPoints, Damage, EffectNr, Radius,
// Angle, DmgType). nullptr: passed on. Throws what AngleCombat throws.
static void MagicAngleCombat(uint32_t Actor, int Mana, int SoulPoints, int Damage, int EffectNr, int Radius,
		int Angle, int DmgType) {
	AngleCombat(GetCreature(Actor), Mana, SoulPoints, Damage, EffectNr, Radius, Angle,
			static_cast<unsigned short>(DmgType));
}

// magic.kill_all_monsters(actor, effect, radius): KillAllMonsters(cr,
// EffectNr, Radius). nullptr: passed on. radius at most MAX_CIRCLES - 1
// (binding's check). Throws what KillAllMonsters throws.
static void MagicKillAllMonsters(uint32_t Actor, int EffectNr, int Radius) {
	CheckRadius(Radius, MAX_CIRCLES, "magic.kill_all_monsters: radius beyond the circles");
	KillAllMonsters(GetCreature(Actor), EffectNr, Radius);
}

// magic.create_field(actor, mana, soul, field_type): CreateField(cr, Mana,
// SoulPoints, Type), on the field the creature faces. nullptr: passed on.
// Throws what CreateField throws.
static void MagicCreateField(uint32_t Actor, int Mana, int SoulPoints, int Type) {
	CreateField(GetCreature(Actor), Mana, SoulPoints, Type);
}

// magic.create_field_on(actor, obj, mana, soul, field_type): CreateField(cr,
// Obj, Mana, SoulPoints, Type). nullptr: passed on. Throws what CreateField
// throws.
static void MagicCreateFieldOn(uint32_t Actor, uint32_t Obj, int Mana, int SoulPoints, int Type) {
	CreateField(GetCreature(Actor), Object(Obj), Mana, SoulPoints, Type);
}

// magic.mass_create_field(actor, obj, mana, soul, field_type, radius):
// MassCreateField(cr, Obj, Mana, SoulPoints, Type, Radius). nullptr: passed
// on. radius at most MAX_CIRCLES (its loop is "i < Radius"; binding's check).
// Throws what MassCreateField throws.
static void MagicMassCreateField(uint32_t Actor, uint32_t Obj, int Mana, int SoulPoints, int Type, int Radius) {
	CheckRadius(Radius, MAX_CIRCLES + 1, "magic.mass_create_field: radius beyond the circles");
	MassCreateField(GetCreature(Actor), Object(Obj), Mana, SoulPoints, Type, Radius);
}

// magic.create_field_wall(actor, obj, mana, soul, field_type, extension):
// CreateFieldWall(cr, Obj, Mana, SoulPoints, Type, Extension). nullptr:
// passed on. Throws what CreateFieldWall throws.
static void MagicCreateFieldWall(uint32_t Actor, uint32_t Obj, int Mana, int SoulPoints, int Type, int Extension) {
	CreateFieldWall(GetCreature(Actor), Object(Obj), Mana, SoulPoints, Type, Extension);
}

// magic.delete_field(actor, obj, mana, soul): DeleteField(cr, Obj, Mana,
// SoulPoints). nullptr: passed on. Throws what DeleteField throws.
static void MagicDeleteField(uint32_t Actor, uint32_t Obj, int Mana, int SoulPoints) {
	DeleteField(GetCreature(Actor), Object(Obj), Mana, SoulPoints);
}

// magic.cleanup_field(actor): CleanupField(cr), the gamemaster's cleanup of
// the field in front. nullptr: passed on. Throws what CleanupField throws.
static void MagicCleanupField(uint32_t Actor) {
	CleanupField(GetCreature(Actor));
}

// magic.cleanup_field_on(actor, obj, mana, soul): CleanupField(cr, Obj, Mana,
// SoulPoints). nullptr: passed on. Throws what CleanupField throws.
static void MagicCleanupFieldOn(uint32_t Actor, uint32_t Obj, int Mana, int SoulPoints) {
	CleanupField(GetCreature(Actor), Object(Obj), Mana, SoulPoints);
}

// magic.magic_rope(actor, mana, soul): MagicRope(cr, Mana, SoulPoints).
// nullptr: passed on. Throws what MagicRope throws.
static void MagicMagicRope(uint32_t Actor, int Mana, int SoulPoints) {
	MagicRope(GetCreature(Actor), Mana, SoulPoints);
}

// magic.magic_climbing(actor, mana, soul, direction): MagicClimbing(cr, Mana,
// SoulPoints, Direction), direction copied. nullptr: passed on. Throws what
// MagicClimbing throws.
static void MagicMagicClimbing(uint32_t Actor, int Mana, int SoulPoints, std::string Direction) {
	TWord Word(Direction);
	MagicClimbing(GetCreature(Actor), Mana, SoulPoints, Word.Text);
}

// magic.magic_climbing_free(actor, direction): MagicClimbing(cr, Direction),
// the gamemaster's levitation, direction copied. nullptr: passed on. Throws
// what MagicClimbing throws.
static void MagicMagicClimbingFree(uint32_t Actor, std::string Direction) {
	TWord Word(Direction);
	MagicClimbing(GetCreature(Actor), Word.Text);
}

// magic.create_food(actor, mana, soul): CreateFood(cr, Mana, SoulPoints).
// nullptr: passed on. Throws what CreateFood throws.
static void MagicCreateFood(uint32_t Actor, int Mana, int SoulPoints) {
	CreateFood(GetCreature(Actor), Mana, SoulPoints);
}

// magic.create_arrows(actor, mana, soul, ammo, amount): CreateArrows(cr,
// Mana, SoulPoints, Ammo, Amount), ammo one of the table ammo. nullptr:
// passed on. Throws what CreateArrows throws.
static void MagicCreateArrows(uint32_t Actor, int Mana, int SoulPoints, int Ammo, int Amount) {
	CreateArrows(GetCreature(Actor), Mana, SoulPoints, Ammo, Amount);
}

// magic.summon_creature(actor, mana, race_name, god): SummonCreature(cr,
// Mana, RaceName, God), race_name copied. nullptr: passed on. Throws what
// SummonCreature throws.
static void MagicSummonCreature(uint32_t Actor, int Mana, std::string RaceName, bool God) {
	TWord Word(RaceName);
	SummonCreature(GetCreature(Actor), Mana, Word.Text, God);
}

// magic.raise_dead(actor, obj, mana, soul): RaiseDead(cr, Obj, Mana,
// SoulPoints). nullptr: passed on. Throws what RaiseDead throws.
static void MagicRaiseDead(uint32_t Actor, uint32_t Obj, int Mana, int SoulPoints) {
	RaiseDead(GetCreature(Actor), Object(Obj), Mana, SoulPoints);
}

// magic.mass_raise_dead(actor, obj, mana, soul, radius): MassRaiseDead(cr,
// Target, Mana, SoulPoints, Radius). nullptr: passed on. radius at most
// MAX_CIRCLES - 1 (binding's check). Throws what MassRaiseDead throws.
static void MagicMassRaiseDead(uint32_t Actor, uint32_t Target, int Mana, int SoulPoints, int Radius) {
	CheckRadius(Radius, MAX_CIRCLES, "magic.mass_raise_dead: radius beyond the circles");
	MassRaiseDead(GetCreature(Actor), Object(Target), Mana, SoulPoints, Radius);
}

// magic.magic_go_strength(actor, target, mana, soul, percent, duration):
// MagicGoStrength(cr, dest, Mana, SoulPoints, percent, duration). nullptr:
// passed on, for both (MagicGoStrength checks the target first, as before).
// Throws what MagicGoStrength throws.
static void MagicMagicGoStrength(uint32_t Actor, uint32_t Target, int Mana, int SoulPoints, int Percent,
		int Duration) {
	TCreature *cr = GetCreature(Actor);
	TCreature *dest = GetCreature(Target);
	MagicGoStrength(cr, dest, Mana, SoulPoints, Percent, Duration);
}

// magic.shielding(actor, mana, soul, duration): Shielding(cr, Mana,
// SoulPoints, Duration). nullptr: passed on. Throws what Shielding throws.
static void MagicShielding(uint32_t Actor, int Mana, int SoulPoints, int Duration) {
	Shielding(GetCreature(Actor), Mana, SoulPoints, Duration);
}

// magic.negate_poison(actor, mana, soul, target): NegatePoison(cr, Mana,
// SoulPoints, dest). nullptr: passed on, for both. Throws what NegatePoison
// throws.
static void MagicNegatePoison(uint32_t Actor, int Mana, int SoulPoints, uint32_t Target) {
	TCreature *cr = GetCreature(Actor);
	TCreature *dest = GetCreature(Target);
	NegatePoison(cr, Mana, SoulPoints, dest);
}

// magic.enlight(actor, mana, soul, radius, duration): Enlight(cr, Mana,
// SoulPoints, Radius, Duration). nullptr: passed on. radius must not be 0
// (Enlight divides by it; binding's check). Throws what Enlight throws.
static void MagicEnlight(uint32_t Actor, int Mana, int SoulPoints, int Radius, int Duration) {
	if (Radius == 0) {
		throw "magic.enlight: radius 0";
	}
	Enlight(GetCreature(Actor), Mana, SoulPoints, Radius, Duration);
}

// magic.invisibility(actor, mana, soul, duration): Invisibility(cr, Mana,
// SoulPoints, Duration). nullptr: passed on. Throws what Invisibility throws.
static void MagicInvisibility(uint32_t Actor, int Mana, int SoulPoints, int Duration) {
	Invisibility(GetCreature(Actor), Mana, SoulPoints, Duration);
}

// magic.cancel_invisibility(actor, obj, mana, soul, radius):
// CancelInvisibility(cr, Target, Mana, SoulPoints, Radius). nullptr: passed
// on. radius at most MAX_CIRCLES - 1 (binding's check). Throws what
// CancelInvisibility throws.
static void MagicCancelInvisibility(uint32_t Actor, uint32_t Target, int Mana, int SoulPoints, int Radius) {
	CheckRadius(Radius, MAX_CIRCLES, "magic.cancel_invisibility: radius beyond the circles");
	CancelInvisibility(GetCreature(Actor), Object(Target), Mana, SoulPoints, Radius);
}

// magic.creature_illusion(actor, mana, soul, race_name, duration):
// CreatureIllusion(cr, Mana, SoulPoints, RaceName, Duration), race_name
// copied. nullptr: passed on. Throws what CreatureIllusion throws.
static void MagicCreatureIllusion(uint32_t Actor, int Mana, int SoulPoints, std::string RaceName, int Duration) {
	TWord Word(RaceName);
	CreatureIllusion(GetCreature(Actor), Mana, SoulPoints, Word.Text, Duration);
}

// magic.object_illusion(actor, mana, soul, obj, duration): ObjectIllusion(cr,
// Mana, SoulPoints, Obj, Duration). nullptr: passed on. Throws what
// ObjectIllusion throws.
static void MagicObjectIllusion(uint32_t Actor, int Mana, int SoulPoints, uint32_t Obj, int Duration) {
	ObjectIllusion(GetCreature(Actor), Mana, SoulPoints, Object(Obj), Duration);
}

// magic.enchant_object(actor, mana, soul, origin_type, target_type):
// EnchantObject(cr, Mana, SoulPoints, Origin, Target), the object types made
// from their IDs (ObjectType(TypeID), which logs an ID that is no type and
// takes 0 then; object.new_type gives IDs that are types or 0). nullptr:
// passed on. Throws what EnchantObject throws.
static void MagicEnchantObject(uint32_t Actor, int Mana, int SoulPoints, int Origin, int Target) {
	EnchantObject(GetCreature(Actor), Mana, SoulPoints, ObjectType(Origin), ObjectType(Target));
}

// magic.convince(actor, target): Convince(master, slave). nullptr: checked,
// for both (Convince dereferences both; the actor is checked first). Throws
// what Convince throws.
static void MagicConvince(uint32_t Actor, uint32_t Target) {
	TCreature *Master = ExistingCreature(Actor, "magic.convince");
	TCreature *Slave = ExistingCreature(Target, "magic.convince");
	Convince(Master, Slave);
}

// magic.challenge(actor, mana, soul, radius): Challenge(cr, Mana, SoulPoints,
// Radius). nullptr: passed on (Challenge dereferences nothing before
// SpendManaAndSoulPoints, which logs and throws ERROR for it). radius at most
// MAX_CIRCLES - 1 (binding's check). Throws what Challenge throws.
static void MagicChallenge(uint32_t Actor, int Mana, int SoulPoints, int Radius) {
	CheckRadius(Radius, MAX_CIRCLES, "magic.challenge: radius beyond the circles");
	Challenge(GetCreature(Actor), Mana, SoulPoints, Radius);
}

// magic.find_person(actor, mana, soul, name): FindPerson(cr, Mana,
// SoulPoints, Name), name copied. nullptr: passed on. Throws what FindPerson
// throws.
static void MagicFindPerson(uint32_t Actor, int Mana, int SoulPoints, std::string Name) {
	TWord Word(Name);
	FindPerson(GetCreature(Actor), Mana, SoulPoints, Word.Text);
}

// ---------------------------------------------------------------------------
// command: every function checks for nullptr itself (it logs and throws
// ERROR), so each binding passes it on; the strings are copied (TWord). Each
// throws what its C++ function throws.

// command.create_thing(actor, p1, p2|nil): CreateThing(cr, Param1, Param2).
static void CommandCreateThing(uint32_t Actor, std::string Param1, sol::optional<std::string> Param2) {
	TWord Word1(Param1);
	TOptionalWord Word2(Param2);
	CreateThing(GetCreature(Actor), Word1.Text, Word2.get());
}

// command.teleport(actor, p): Teleport(cr, Param).
static void CommandTeleport(uint32_t Actor, std::string Param) {
	TWord Word(Param);
	Teleport(GetCreature(Actor), Word.Text);
}

// command.create_knowledge(actor, p1, p2|nil): CreateKnowledge(cr, Param1,
// Param2).
static void CommandCreateKnowledge(uint32_t Actor, std::string Param1, sol::optional<std::string> Param2) {
	TWord Word1(Param1);
	TOptionalWord Word2(Param2);
	CreateKnowledge(GetCreature(Actor), Word1.Text, Word2.get());
}

// command.change_data(actor, p): ChangeData(cr, Param).
static void CommandChangeData(uint32_t Actor, std::string Param) {
	TWord Word(Param);
	ChangeData(GetCreature(Actor), Word.Text);
}

// command.teleport_to_creature(actor, name): TeleportToCreature(cr, Name).
static void CommandTeleportToCreature(uint32_t Actor, std::string Name) {
	TWord Word(Name);
	TeleportToCreature(GetCreature(Actor), Word.Text);
}

// command.teleport_player_to_me(actor, name): TeleportPlayerToMe(cr, Name).
static void CommandTeleportPlayerToMe(uint32_t Actor, std::string Name) {
	TWord Word(Name);
	TeleportPlayerToMe(GetCreature(Actor), Word.Text);
}

// command.get_position(actor): GetPosition(cr).
static void CommandGetPosition(uint32_t Actor) {
	GetPosition(GetCreature(Actor));
}

// command.create_money(actor, p): CreateMoney(cr, Param).
static void CommandCreateMoney(uint32_t Actor, std::string Param) {
	TWord Word(Param);
	CreateMoney(GetCreature(Actor), Word.Text);
}

// command.change_profession(actor, p): ChangeProfession(cr, Param).
static void CommandChangeProfession(uint32_t Actor, std::string Param) {
	TWord Word(Param);
	ChangeProfession(GetCreature(Actor), Word.Text);
}

// command.edit_guests(actor): EditGuests(cr).
static void CommandEditGuests(uint32_t Actor) {
	EditGuests(GetCreature(Actor));
}

// command.edit_subowners(actor): EditSubowners(cr).
static void CommandEditSubowners(uint32_t Actor) {
	EditSubowners(GetCreature(Actor));
}

// command.kick_guest(actor, name): KickGuest(cr, Name).
static void CommandKickGuest(uint32_t Actor, std::string Name) {
	TWord Word(Name);
	KickGuest(GetCreature(Actor), Word.Text);
}

// command.edit_name_door(actor): EditNameDoor(cr).
static void CommandEditNameDoor(uint32_t Actor) {
	EditNameDoor(GetCreature(Actor));
}

// command.get_quest_value(actor, p): GetQuestValue(cr, Param).
static void CommandGetQuestValue(uint32_t Actor, std::string Param) {
	TWord Word(Param);
	GetQuestValue(GetCreature(Actor), Word.Text);
}

// command.set_quest_value(actor, p1, p2): SetQuestValue(cr, Param1, Param2).
static void CommandSetQuestValue(uint32_t Actor, std::string Param1, std::string Param2) {
	TWord Word1(Param1);
	TWord Word2(Param2);
	SetQuestValue(GetCreature(Actor), Word1.Text, Word2.Text);
}

// command.clear_quest_values(actor): ClearQuestValues(cr).
static void CommandClearQuestValues(uint32_t Actor) {
	ClearQuestValues(GetCreature(Actor));
}

// command.start_monsterraid(actor, name): StartMonsterraid(cr, RaidName),
// which lowers the copy. The const char * of a raid that fails to load is
// caught inside StartMonsterraid, as before.
static void CommandStartMonsterraid(uint32_t Actor, std::string RaidName) {
	TWord Word(RaidName);
	StartMonsterraid(GetCreature(Actor), Word.Text);
}

// command.home_teleport(actor, name): HomeTeleport(cr, Name).
static void CommandHomeTeleport(uint32_t Actor, std::string Name) {
	TWord Word(Name);
	HomeTeleport(GetCreature(Actor), Word.Text);
}

// command.banish_account(actor, name, n, reason): BanishAccount(cr, Name, n,
// Reason).
static void CommandBanishAccount(uint32_t Actor, std::string Name, int Number, std::string Reason) {
	TWord NameWord(Name);
	TWord ReasonWord(Reason);
	BanishAccount(GetCreature(Actor), NameWord.Text, Number, ReasonWord.Text);
}

// command.delete_account(actor, name, reason): DeleteAccount(cr, Name,
// Reason).
static void CommandDeleteAccount(uint32_t Actor, std::string Name, std::string Reason) {
	TWord NameWord(Name);
	TWord ReasonWord(Reason);
	DeleteAccount(GetCreature(Actor), NameWord.Text, ReasonWord.Text);
}

// command.set_name_rule(actor, name): SetNameRule(cr, Name).
static void CommandSetNameRule(uint32_t Actor, std::string Name) {
	TWord Word(Name);
	SetNameRule(GetCreature(Actor), Word.Text);
}

// command.notation(actor, name, comment): Notation(cr, Name, Comment).
static void CommandNotation(uint32_t Actor, std::string Name, std::string Comment) {
	TWord NameWord(Name);
	TWord CommentWord(Comment);
	Notation(GetCreature(Actor), NameWord.Text, CommentWord.Text);
}

// command.name_lock(actor, name): NameLock(cr, Name).
static void CommandNameLock(uint32_t Actor, std::string Name) {
	TWord Word(Name);
	NameLock(GetCreature(Actor), Word.Text);
}

// command.kick_player(actor, name): KickPlayer(cr, Name).
static void CommandKickPlayer(uint32_t Actor, std::string Name) {
	TWord Word(Name);
	KickPlayer(GetCreature(Actor), Word.Text);
}

// command.delete_character(actor, name, reason): DeleteCharacter(cr, Name,
// Reason).
static void CommandDeleteCharacter(uint32_t Actor, std::string Name, std::string Reason) {
	TWord NameWord(Name);
	TWord ReasonWord(Reason);
	DeleteCharacter(GetCreature(Actor), NameWord.Text, ReasonWord.Text);
}

// command.ip_banishment(actor, name, reason): IPBanishment(cr, Name, Reason).
static void CommandIPBanishment(uint32_t Actor, std::string Name, std::string Reason) {
	TWord NameWord(Name);
	TWord ReasonWord(Reason);
	IPBanishment(GetCreature(Actor), NameWord.Text, ReasonWord.Text);
}

// command.banish_character(actor, name, n, reason): BanishCharacter(cr, Name,
// n, Reason).
static void CommandBanishCharacter(uint32_t Actor, std::string Name, int Number, std::string Reason) {
	TWord NameWord(Name);
	TWord ReasonWord(Reason);
	BanishCharacter(GetCreature(Actor), NameWord.Text, Number, ReasonWord.Text);
}

// ---------------------------------------------------------------------------
// game, creature, object

// game.to_int(text) -> int: (int)std::clamp(strtol(Text, nullptr, 10),
// (long)INT32_MIN, (long)INT32_MAX) of the copy, as AccountRightSpell read
// the numbers of the words. Throws nothing.
static int GameToInt(std::string Text) {
	TWord Word(Text);
	return static_cast<int>(std::clamp(strtol(Word.Text, nullptr, 10), static_cast<long>(INT32_MIN),
			static_cast<long>(INT32_MAX)));
}

// creature.object(id) -> the handle of the creature's object
// (GetCreatureObject().getObjectID()). nullptr: checked.
static uint32_t CreatureObject(uint32_t ID) {
	return ExistingCreature(ID, "creature.object")->GetCreatureObject().getObjectID();
}

// creature.skill(id, skill) -> Skills[skill]->Get(), the skill's value.
// nullptr: checked. skill must be 0 to MAX_SKILLS - 1 and exist for the
// creature (binding's check).
static int CreatureSkill(uint32_t ID, int SkillNr) {
	TCreature *cr = ExistingCreature(ID, "creature.skill");
	if (SkillNr < 0 || SkillNr >= MAX_SKILLS || cr->Skills[SkillNr] == nullptr) {
		throw "creature.skill: no such skill";
	}
	return cr->Skills[SkillNr]->Get();
}

// creature.effective_profession(id) -> GetEffectiveProfession(), the
// profession (PROFESSION_*), a function of TPlayer. nullptr: checked. A
// creature that is no player has none (binding's check; the rune case asked
// it of the player who used the rune).
static int CreatureEffectiveProfession(uint32_t ID) {
	TCreature *cr = ExistingCreature(ID, "creature.effective_profession");
	if (cr->Type != PLAYER) {
		throw "creature.effective_profession: the creature is not a player";
	}
	return static_cast<TPlayer *>(cr)->GetEffectiveProfession();
}

// object.new_type(group, number) -> GetNewObjectType(Group,
// Number).getTypeID(), the type's ID (0 for a group and number without a
// type, which GetNewObjectType logs). group and number must be 0 to 255
// (binding's check).
static int ObjectNewType(int Group, int Number) {
	if (Group < 0 || Group > 255 || Number < 0 || Number > 255) {
		throw "object.new_type: group and number must be 0 to 255";
	}
	return GetNewObjectType(static_cast<unsigned char>(Group), static_cast<unsigned char>(Number)).getTypeID();
}

// Sets the constant Prefix##Name of a table as Name.
#define CONSTANT(Table, Prefix, Name) Table.set(#Name, Prefix##Name)

void RegisterMagicAPI() {
	sol::table Magic = ScriptAPITable("magic");
	Magic.set_function("compute_damage", Guarded(&MagicComputeDamage));
	Magic.set_function("heal", Guarded(&MagicHeal));
	Magic.set_function("mass_heal", Guarded(&MagicMassHeal));
	Magic.set_function("heal_friend", Guarded(&MagicHealFriend));
	Magic.set_function("combat", Guarded(&MagicCombat));
	Magic.set_function("mass_combat", Guarded(&MagicMassCombat));
	Magic.set_function("angle_combat", Guarded(&MagicAngleCombat));
	Magic.set_function("kill_all_monsters", Guarded(&MagicKillAllMonsters));
	Magic.set_function("create_field", Guarded(&MagicCreateField));
	Magic.set_function("create_field_on", Guarded(&MagicCreateFieldOn));
	Magic.set_function("mass_create_field", Guarded(&MagicMassCreateField));
	Magic.set_function("create_field_wall", Guarded(&MagicCreateFieldWall));
	Magic.set_function("delete_field", Guarded(&MagicDeleteField));
	Magic.set_function("cleanup_field", Guarded(&MagicCleanupField));
	Magic.set_function("cleanup_field_on", Guarded(&MagicCleanupFieldOn));
	Magic.set_function("magic_rope", Guarded(&MagicMagicRope));
	Magic.set_function("magic_climbing", Guarded(&MagicMagicClimbing));
	Magic.set_function("magic_climbing_free", Guarded(&MagicMagicClimbingFree));
	Magic.set_function("create_food", Guarded(&MagicCreateFood));
	Magic.set_function("create_arrows", Guarded(&MagicCreateArrows));
	Magic.set_function("summon_creature", Guarded(&MagicSummonCreature));
	Magic.set_function("raise_dead", Guarded(&MagicRaiseDead));
	Magic.set_function("mass_raise_dead", Guarded(&MagicMassRaiseDead));
	Magic.set_function("magic_go_strength", Guarded(&MagicMagicGoStrength));
	Magic.set_function("shielding", Guarded(&MagicShielding));
	Magic.set_function("negate_poison", Guarded(&MagicNegatePoison));
	Magic.set_function("enlight", Guarded(&MagicEnlight));
	Magic.set_function("invisibility", Guarded(&MagicInvisibility));
	Magic.set_function("cancel_invisibility", Guarded(&MagicCancelInvisibility));
	Magic.set_function("creature_illusion", Guarded(&MagicCreatureIllusion));
	Magic.set_function("object_illusion", Guarded(&MagicObjectIllusion));
	Magic.set_function("enchant_object", Guarded(&MagicEnchantObject));
	Magic.set_function("convince", Guarded(&MagicConvince));
	Magic.set_function("challenge", Guarded(&MagicChallenge));
	Magic.set_function("find_person", Guarded(&MagicFindPerson));

	sol::table Command = ScriptAPITable("command");
	Command.set_function("create_thing", Guarded(&CommandCreateThing));
	Command.set_function("teleport", Guarded(&CommandTeleport));
	Command.set_function("create_knowledge", Guarded(&CommandCreateKnowledge));
	Command.set_function("change_data", Guarded(&CommandChangeData));
	Command.set_function("teleport_to_creature", Guarded(&CommandTeleportToCreature));
	Command.set_function("teleport_player_to_me", Guarded(&CommandTeleportPlayerToMe));
	Command.set_function("get_position", Guarded(&CommandGetPosition));
	Command.set_function("create_money", Guarded(&CommandCreateMoney));
	Command.set_function("change_profession", Guarded(&CommandChangeProfession));
	Command.set_function("edit_guests", Guarded(&CommandEditGuests));
	Command.set_function("edit_subowners", Guarded(&CommandEditSubowners));
	Command.set_function("kick_guest", Guarded(&CommandKickGuest));
	Command.set_function("edit_name_door", Guarded(&CommandEditNameDoor));
	Command.set_function("get_quest_value", Guarded(&CommandGetQuestValue));
	Command.set_function("set_quest_value", Guarded(&CommandSetQuestValue));
	Command.set_function("clear_quest_values", Guarded(&CommandClearQuestValues));
	Command.set_function("start_monsterraid", Guarded(&CommandStartMonsterraid));
	Command.set_function("home_teleport", Guarded(&CommandHomeTeleport));
	Command.set_function("banish_account", Guarded(&CommandBanishAccount));
	Command.set_function("delete_account", Guarded(&CommandDeleteAccount));
	Command.set_function("set_name_rule", Guarded(&CommandSetNameRule));
	Command.set_function("notation", Guarded(&CommandNotation));
	Command.set_function("name_lock", Guarded(&CommandNameLock));
	Command.set_function("kick_player", Guarded(&CommandKickPlayer));
	Command.set_function("delete_character", Guarded(&CommandDeleteCharacter));
	Command.set_function("ip_banishment", Guarded(&CommandIPBanishment));
	Command.set_function("banish_character", Guarded(&CommandBanishCharacter));

	sol::table Game = ScriptAPITable("game");
	Game.set_function("to_int", Guarded(&GameToInt));

	sol::table Creature = ScriptAPITable("creature");
	Creature.set_function("object", Guarded(&CreatureObject));
	Creature.set_function("skill", Guarded(&CreatureSkill));
	Creature.set_function("effective_profession", Guarded(&CreatureEffectiveProfession));

	sol::table ObjectAPI = ScriptAPITable("object");
	ObjectAPI.set_function("new_type", Guarded(&ObjectNewType));

	// The constants, all names of operate.hh's EFFECT_ and ANIMATION_, cr.hh's
	// DAMAGE_ and PROFESSION_, info.hh's FIELD_, crskill.hh's SKILL_ and
	// magic.hh's CONJURE_ (as ammo), without the prefix.
	sol::table Effect = ScriptAPITable("effect");
	CONSTANT(Effect, EFFECT_, BLOOD_HIT);
	CONSTANT(Effect, EFFECT_, MANA_HIT);
	CONSTANT(Effect, EFFECT_, POFF);
	CONSTANT(Effect, EFFECT_, BLOCK_HIT);
	CONSTANT(Effect, EFFECT_, EXPLOSION_AREA);
	CONSTANT(Effect, EFFECT_, EXPLOSION);
	CONSTANT(Effect, EFFECT_, FIRE_AREA);
	CONSTANT(Effect, EFFECT_, POISON);
	CONSTANT(Effect, EFFECT_, HIT_AREA);
	CONSTANT(Effect, EFFECT_, TELEPORT);
	CONSTANT(Effect, EFFECT_, ENERGY_HIT);
	CONSTANT(Effect, EFFECT_, MAGIC_BLUE);
	CONSTANT(Effect, EFFECT_, MAGIC_RED);
	CONSTANT(Effect, EFFECT_, MAGIC_GREEN);
	CONSTANT(Effect, EFFECT_, FIRE_HIT);
	CONSTANT(Effect, EFFECT_, POISON_HIT);
	CONSTANT(Effect, EFFECT_, DEATH);

	sol::table Animation = ScriptAPITable("animation");
	CONSTANT(Animation, ANIMATION_, NONE);
	CONSTANT(Animation, ANIMATION_, FIRE);
	CONSTANT(Animation, ANIMATION_, ENERGY);
	CONSTANT(Animation, ANIMATION_, DEATH);

	sol::table Damage = ScriptAPITable("damage");
	CONSTANT(Damage, DAMAGE_, PHYSICAL);
	CONSTANT(Damage, DAMAGE_, POISON);
	CONSTANT(Damage, DAMAGE_, FIRE);
	CONSTANT(Damage, DAMAGE_, ENERGY);
	CONSTANT(Damage, DAMAGE_, POISON_CONDITION);
	CONSTANT(Damage, DAMAGE_, FIRE_CONDITION);
	CONSTANT(Damage, DAMAGE_, ENERGY_CONDITION);
	CONSTANT(Damage, DAMAGE_, LIFEDRAIN);
	CONSTANT(Damage, DAMAGE_, MANADRAIN);

	sol::table Field = ScriptAPITable("field");
	CONSTANT(Field, FIELD_, FIRE);
	CONSTANT(Field, FIELD_, POISON);
	CONSTANT(Field, FIELD_, ENERGY);
	CONSTANT(Field, FIELD_, MAGICWALL);
	CONSTANT(Field, FIELD_, RUSHWOOD);

	sol::table Ammo = ScriptAPITable("ammo");
	CONSTANT(Ammo, CONJURE_, ARROW);
	CONSTANT(Ammo, CONJURE_, POISONED_ARROW);
	CONSTANT(Ammo, CONJURE_, EXPLOSIVE_ARROW);
	CONSTANT(Ammo, CONJURE_, BOLT);
	CONSTANT(Ammo, CONJURE_, POWER_BOLT);

	sol::table Skill = ScriptAPITable("skill");
	CONSTANT(Skill, SKILL_, LEVEL);
	CONSTANT(Skill, SKILL_, MAGICLEVEL);
	CONSTANT(Skill, SKILL_, HITPOINTS);
	CONSTANT(Skill, SKILL_, MANA);
	CONSTANT(Skill, SKILL_, GOSTRENGTH);
	CONSTANT(Skill, SKILL_, CARRYSTRENGTH);
	CONSTANT(Skill, SKILL_, SHIELDING);
	CONSTANT(Skill, SKILL_, DISTANCEFIGHTING);
	CONSTANT(Skill, SKILL_, SWORDFIGHTING);
	CONSTANT(Skill, SKILL_, CLUBFIGHTING);
	CONSTANT(Skill, SKILL_, AXEFIGHTING);
	CONSTANT(Skill, SKILL_, FISTFIGHTING);
	CONSTANT(Skill, SKILL_, MAGICDEFENSE);
	CONSTANT(Skill, SKILL_, FISHING);
	CONSTANT(Skill, SKILL_, EATING);
	CONSTANT(Skill, SKILL_, SHINING);
	CONSTANT(Skill, SKILL_, ILLUSION);
	CONSTANT(Skill, SKILL_, POISON);
	CONSTANT(Skill, SKILL_, BURNING);
	CONSTANT(Skill, SKILL_, ENERGY);
	CONSTANT(Skill, SKILL_, DRUNKEN);
	CONSTANT(Skill, SKILL_, MANASHIELD);
	CONSTANT(Skill, SKILL_, SOULPOINTS);

	sol::table Profession = ScriptAPITable("profession");
	CONSTANT(Profession, PROFESSION_, NONE);
	CONSTANT(Profession, PROFESSION_, KNIGHT);
	CONSTANT(Profession, PROFESSION_, PALADIN);
	CONSTANT(Profession, PROFESSION_, SORCERER);
	CONSTANT(Profession, PROFESSION_, DRUID);
	CONSTANT(Profession, PROFESSION_, PROMOTION);
	CONSTANT(Profession, PROFESSION_, ELITE_KNIGHT);
	CONSTANT(Profession, PROFESSION_, ROYAL_PALADIN);
	CONSTANT(Profession, PROFESSION_, MASTER_SORCERER);
	CONSTANT(Profession, PROFESSION_, ELDER_DRUID);
}

#undef CONSTANT
