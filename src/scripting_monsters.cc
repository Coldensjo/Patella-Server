// scripting_monsters.cc: the API of the monster spell library and the
// monsters' scripts (scripting.hh, "Monsters, raids and monster homes"): the
// methods of Monster and MonsterType, creature:turnTo, the Impact constructors
// with impact:isAggressive, and the five shapes of magic.cc in the table
// Magic. RegisterMonsterAPI binds them; InitAll calls it after RegisterNpcAPI.
// Also the calls of the spell library lib/monsters.lua (InitMonsterSpells,
// PrepareMonsterSpell, CastMonsterSpells); the hooks are in scripting.cc.
//
// What every binding here does alike (as in scripting_npcs.cc):
//  - A Monster self is resolved with GetCreature on every call; for an ID that
//    is no monster's (gone, or another creature) the binding logs "<Lua
//    name>: Monster does not exist." and raises ERROR.
//  - A Creature self, a shape's actor and an impact's actor are resolved on
//    every call; one that is gone logs "<Lua name>: Creature does not exist."
//    and raises ERROR ("nullptr: checked"). A victim or a turn's target that
//    is gone is passed on as nullptr, which the C++ function checks itself
//    ("nullptr: passed on").
//  - An Impact holds numbers only (scripting_api.hh, TScriptImpact); the C++
//    impact of magic.hh is made on the stack at the shape's call, from the
//    Impact's actor (resolved then) and numbers, as IdleStimulus made it with
//    new, and dropped after the shape (all impacts have trivial destructors).
//  - The binding's own checks (a radius beyond the circles, a race or impact
//    kind out of range) throw a const char *, which Guarded turns into a
//    logged Lua error and ERROR.
//  - What the C++ function throws (a RESULT) is raised as the result code
//    (Guarded).

#include <climits>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <string>

#include "objects.hh"
#include "map.hh"
#include "info.hh"
#include "cr.hh"
#include "crinc.hh"
#include "crmain.hh"
#include "crnonpl.hh"
#include "magic.hh"
#include "magicdata.hh"
#include "utils.hh"
#include "scripting_api.hh"

// The creature ID names; for a missing one (0 included) logs "<Name>:
// Creature does not exist." and throws ERROR.
static TCreature *ExistingCreature(uint32_t ID, const char *Name) {
	TCreature *cr = GetCreature(ID);
	if (cr == nullptr) {
		error("%s: Creature does not exist.\n", Name);
		throw ERROR;
	}
	return cr;
}

// The monster the ID names; for one that is gone or no monster logs "<Name>:
// Monster does not exist." and throws ERROR.
static TCreature *ExistingMonster(uint32_t ID, const char *Name) {
	TCreature *cr = GetCreature(ID);
	if (cr == nullptr || cr->Type != MONSTER) {
		error("%s: Monster does not exist.\n", Name);
		throw ERROR;
	}
	return cr;
}

// The race number a MonsterType holds, checked to be 1 to MAX_RACES - 1
// (binding's check; MonsterType and monster:getType make only those).
static int CheckedRace(const TScriptMonsterType &Self, const char *Refused) {
	if (Self.Race < 1 || Self.Race > MAX_RACES - 1) {
		throw Refused;
	}
	return Self.Race;
}

// Throws Message (a const char *) for a radius the circle loops of the
// shapes ("i <= Radius" over Circle) cannot take: MAX_CIRCLES or more. A
// negative radius passes; the loops then do nothing, as in C++.
static void CheckRadius(int Radius, const char *Message) {
	if (Radius >= MAX_CIRCLES) {
		throw Message;
	}
}

// ---------------------------------------------------------------------------
// Monster

// monster:getMasterId() -> Master (GetMaster()), the raw ID (0 for none),
// also of a master that is gone. Throws nothing else.
static uint32_t MonsterGetMasterId(TScriptMonster Self) {
	return ExistingMonster(Self.ID, "Monster.getMasterId")->GetMaster();
}

// monster:getTargetId() -> Target (GetMonsterTarget), the raw ID (0 for
// none), also of a target that is gone. Throws nothing else.
static uint32_t MonsterGetTargetId(TScriptMonster Self) {
	return GetMonsterTarget(ExistingMonster(Self.ID, "Monster.getTargetId"));
}

// monster:getType() -> MonsterType(Race), the monster's race. Throws nothing
// else.
static TScriptMonsterType MonsterGetType(TScriptMonster Self) {
	return TScriptMonsterType{ExistingMonster(Self.ID, "Monster.getType")->Race};
}

// ---------------------------------------------------------------------------
// MonsterType

// MonsterType(key) or MonsterType(number), the __call of the shared global
// MonsterType (the proxy of the class's method table, which is argument 1): a
// new MonsterType holding the race number of the key (GetRaceByKey) or the
// number; nil for an unknown key, a number outside 1 to MAX_RACES - 1 or a
// race without data (RaceData's Name empty, as CreateMonster checks). A
// number with a fraction or outside the range of int, or another value, is a
// Lua error. A Lua C function (no Guarded: the argument is a string or a
// number); it holds no C++ object when it raises an error.
static int MonsterTypeNew(lua_State *L) {
	if (lua_gettop(L) != 2) {
		return luaL_error(L, "MonsterType: one argument expected (key or number)");
	}
	int Race = 0;
	int Type = lua_type(L, 2);
	if (Type == LUA_TSTRING) {
		const char *Key = lua_tostring(L, 2);
		try {
			Race = GetRaceByKey(Key);
		} catch (...) {
			Race = 0;
		}
	} else if (Type == LUA_TNUMBER) {
		double Value = lua_tonumber(L, 2);
		if (!(Value >= INT_MIN && Value <= INT_MAX) || Value != floor(Value)) {
			return luaL_argerror(L, 2, "integer expected");
		}
		Race = static_cast<int>(Value);
	} else {
		return luaL_typerror(L, 2, "race key or number");
	}

	if (Race < 1 || Race > MAX_RACES - 1 || RaceData[Race].Name[0] == 0) {
		lua_pushnil(L);
	} else {
		PushScriptObject(L, SCRIPT_MONSTERTYPE, static_cast<uint32_t>(Race));
	}
	return 1;
}

// monsterType:getId() -> the race number. Throws nothing else.
static int MonsterTypeGetId(TScriptMonsterType Self) {
	return CheckedRace(Self, "MonsterType.getId: no race number");
}

// monsterType:getKey() -> RaceData[Race].Key, the race's key ("demon").
// Throws nothing else.
static std::string MonsterTypeGetKey(TScriptMonsterType Self) {
	return RaceData[CheckedRace(Self, "MonsterType.getKey: no race number")].Key;
}

// monsterType:name() -> RaceData[Race].Name, the name players see (without
// the article). Throws nothing else.
static std::string MonsterTypeName(TScriptMonsterType Self) {
	return RaceData[CheckedRace(Self, "MonsterType.name: no race number")].Name;
}

// monsterType:runHealth() -> RaceData[Race].FleeThreshold, the hit points at
// or below which a monster without a master flees. Throws nothing else.
static int MonsterTypeRunHealth(TScriptMonsterType Self) {
	return RaceData[CheckedRace(Self, "MonsterType.runHealth: no race number")].FleeThreshold;
}

// ---------------------------------------------------------------------------
// Creature

// creature:turnTo(target) -> Rotate(TCreature *Target) of cract.cc: turns
// the creature toward the target (the direction of the larger distance).
// nullptr (self): checked; target: passed on (Rotate logs "TCreature::Rotate:
// Target is NULL."). Throws what Rotate throws.
static void CreatureTurnTo(TScriptCreature Self, TScriptCreature Target) {
	TCreature *cr = ExistingCreature(Self.ID, "Creature.turnTo");
	cr->Rotate(GetCreature(Target.ID));
}

// ---------------------------------------------------------------------------
// Impact

// An Impact of Kind for the actor's ID with the numbers a, b, c, d and no
// colors.
static TScriptImpact MakeImpact(TScriptImpactKind Kind, uint32_t Actor, int a, int b = 0, int c = 0, int d = 0) {
	TScriptImpact Impact;
	memset(&Impact, 0, sizeof(Impact));
	Impact.Kind = Kind;
	Impact.Actor = Actor;
	Impact.Values[0] = a;
	Impact.Values[1] = b;
	Impact.Values[2] = c;
	Impact.Values[3] = d;
	return Impact;
}

// Impact.damage(actor, damageType, power, allowDefense) -> the Impact of
// TDamageImpact(actor, DamageType, Power, AllowDefense) (COMBAT_ constants).
// Throws nothing.
static TScriptImpact ImpactDamage(TScriptCreature Actor, int DamageType, int Power, bool AllowDefense) {
	return MakeImpact(SCRIPT_IMPACT_DAMAGE, Actor.ID, DamageType, Power, AllowDefense ? 1 : 0);
}

// Impact.field(actor, fieldType) -> the Impact of TFieldImpact(actor,
// FieldType) (FIELD_ constants). Throws nothing.
static TScriptImpact ImpactField(TScriptCreature Actor, int FieldType) {
	return MakeImpact(SCRIPT_IMPACT_FIELD, Actor.ID, FieldType);
}

// Impact.healing(actor, power) -> the Impact of THealingImpact(actor, Power).
// Throws nothing.
static TScriptImpact ImpactHealing(TScriptCreature Actor, int Power) {
	return MakeImpact(SCRIPT_IMPACT_HEALING, Actor.ID, Power);
}

// Impact.speed(actor, percent, duration) -> the Impact of TSpeedImpact(actor,
// Percent, Duration). Throws nothing.
static TScriptImpact ImpactSpeed(TScriptCreature Actor, int Percent, int Duration) {
	return MakeImpact(SCRIPT_IMPACT_SPEED, Actor.ID, Percent, Duration);
}

// Impact.drunken(actor, power, duration) -> the Impact of
// TDrunkenImpact(actor, Power, Duration). A power above 6 is logged and made 6
// here, as TDrunkenImpact's constructor does, so the line is logged where
// IdleStimulus made the impact (also when its shape is not cast) and the
// constructor at the shape's call logs nothing. Throws nothing.
static TScriptImpact ImpactDrunken(TScriptCreature Actor, int Power, int Duration) {
	if (Power > 6) {
		error("TDrunkenImpact::TDrunkenImpact: Power is too large (%d).\n", Power);
		Power = 6;
	}
	return MakeImpact(SCRIPT_IMPACT_DRUNKEN, Actor.ID, Power, Duration);
}

// Impact.strength(actor, skills, percent, duration) -> the Impact of
// TStrengthImpact(actor, Skills, Percent, Duration). Throws nothing.
static TScriptImpact ImpactStrength(TScriptCreature Actor, int Skills, int Percent, int Duration) {
	return MakeImpact(SCRIPT_IMPACT_STRENGTH, Actor.ID, Skills, Percent, Duration);
}

// Impact.outfit(actor, outfitId, colors, duration) -> the Impact of
// TOutfitImpact(actor, Outfit, Duration), the outfit made at the shape's call
// as IdleStimulus made it: OutfitID = outfitId, Colors[i] = (unsigned
// char)colors[i + 1] (colors an array of four numbers: the four color bytes,
// or for an object's outfit, outfitId 0, the object type's low and high byte
// and two 0). Throws nothing.
static TScriptImpact ImpactOutfit(TScriptCreature Actor, int OutfitID, TScriptColors Colors, int Duration) {
	TScriptImpact Impact = MakeImpact(SCRIPT_IMPACT_OUTFIT, Actor.ID, OutfitID, Duration);
	for (int i = 0; i < 4; i++) {
		Impact.Colors[i] = static_cast<unsigned char>(Colors.Values[i]);
	}
	return Impact;
}

// Impact.summon(actor, race, maximum) -> the Impact of TSummonImpact(actor,
// Race, Maximum) (race a race number). Throws nothing.
static TScriptImpact ImpactSummon(TScriptCreature Actor, int Race, int Maximum) {
	return MakeImpact(SCRIPT_IMPACT_SUMMON, Actor.ID, Race, Maximum);
}

// impact:isAggressive() -> isAggressive() of the impact's class (magic.hh):
// false for THealingImpact, which overrides it, true for the other kinds,
// which keep TImpact's. Decided by the kind, without making the C++ impact
// (whose constructor may log). Throws nothing.
static bool ImpactIsAggressive(TScriptImpact Self) {
	return Self.Kind != SCRIPT_IMPACT_HEALING;
}

// Makes the C++ impact of Impact on the stack, its actor resolved now (gone:
// logs "<Name>: Creature does not exist." and throws ERROR), and calls Shape
// with it. The constructors log their checks here, at the shape's call
// (Impact.drunken logs and cuts a power above 6 when the Impact is made, so
// the constructor here logs nothing for it); THealingImpact's "Power is
// negative" is logged only for a healing whose shape is cast (no monster's
// healing can be negative). An impact kind out of range
// throws Refused (binding's check). Throws what Shape throws.
template<typename F>
static void WithImpact(const TScriptImpact &Impact, const char *Name, const char *Refused, F &&Shape) {
	TCreature *Actor = ExistingCreature(Impact.Actor, Name);
	const int *Value = Impact.Values;
	switch (Impact.Kind) {
		case SCRIPT_IMPACT_DAMAGE: {
			TDamageImpact Made(Actor, Value[0], Value[1], Value[2] != 0);
			Shape(&Made);
			break;
		}
		case SCRIPT_IMPACT_FIELD: {
			TFieldImpact Made(Actor, Value[0]);
			Shape(&Made);
			break;
		}
		case SCRIPT_IMPACT_HEALING: {
			THealingImpact Made(Actor, Value[0]);
			Shape(&Made);
			break;
		}
		case SCRIPT_IMPACT_SPEED: {
			TSpeedImpact Made(Actor, Value[0], Value[1]);
			Shape(&Made);
			break;
		}
		case SCRIPT_IMPACT_DRUNKEN: {
			TDrunkenImpact Made(Actor, Value[0], Value[1]);
			Shape(&Made);
			break;
		}
		case SCRIPT_IMPACT_STRENGTH: {
			TStrengthImpact Made(Actor, Value[0], Value[1], Value[2]);
			Shape(&Made);
			break;
		}
		case SCRIPT_IMPACT_OUTFIT: {
			TOutfit Outfit;
			Outfit.OutfitID = Value[0];
			for (int i = 0; i < 4; i++) {
				Outfit.Colors[i] = Impact.Colors[i];
			}
			TOutfitImpact Made(Actor, Outfit, Value[1]);
			Shape(&Made);
			break;
		}
		case SCRIPT_IMPACT_SUMMON: {
			TSummonImpact Made(Actor, Value[0], Value[1]);
			Shape(&Made);
			break;
		}
		default:
			throw Refused;
	}
}

// ---------------------------------------------------------------------------
// Magic, the shapes

// Magic.actorShapeSpell(actor, impact, effect) -> ActorShapeSpell(Actor,
// Impact, Effect) of magic.cc: the impact on the actor (CONST_ME_ effect, 0
// none). nullptr (actor, impact's actor): checked. Throws what
// ActorShapeSpell throws.
static void MagicActorShapeSpell(TScriptCreature Actor, TScriptImpact Impact, int Effect) {
	const char *Name = "Magic.actorShapeSpell";
	TCreature *cr = ExistingCreature(Actor.ID, Name);
	WithImpact(Impact, Name, "Magic.actorShapeSpell: unknown impact", [&](TImpact *Made) {
		ActorShapeSpell(cr, Made, Effect);
	});
}

// Magic.victimShapeSpell(actor, victim, range, animation, impact, effect) ->
// VictimShapeSpell(Actor, Victim, Range, Animation, Impact, Effect): the
// impact on the victim within range (CONST_ANI_ animation). nullptr (actor,
// impact's actor): checked; victim: passed on (nothing happens). Throws what
// VictimShapeSpell throws.
static void MagicVictimShapeSpell(TScriptCreature Actor, TScriptCreature Victim, int Range, int Animation,
		TScriptImpact Impact, int Effect) {
	const char *Name = "Magic.victimShapeSpell";
	TCreature *cr = ExistingCreature(Actor.ID, Name);
	TCreature *Target = GetCreature(Victim.ID);
	WithImpact(Impact, Name, "Magic.victimShapeSpell: unknown impact", [&](TImpact *Made) {
		VictimShapeSpell(cr, Target, Range, Animation, Made, Effect);
	});
}

// Magic.originShapeSpell(actor, radius, impact, effect) ->
// OriginShapeSpell(Actor, Radius, Impact, Effect): the impact on the circles
// around the actor up to radius, at most MAX_CIRCLES - 1 (binding's check).
// nullptr (actor, impact's actor): checked. Throws what OriginShapeSpell
// throws.
static void MagicOriginShapeSpell(TScriptCreature Actor, int Radius, TScriptImpact Impact, int Effect) {
	const char *Name = "Magic.originShapeSpell";
	TCreature *cr = ExistingCreature(Actor.ID, Name);
	CheckRadius(Radius, "Magic.originShapeSpell: radius beyond the circles");
	WithImpact(Impact, Name, "Magic.originShapeSpell: unknown impact", [&](TImpact *Made) {
		OriginShapeSpell(cr, Radius, Made, Effect);
	});
}

// Magic.destinationShapeSpell(actor, victim, range, animation, radius,
// impact, effect) -> DestinationShapeSpell(Actor, Victim, Range, Animation,
// Radius, Impact, Effect): the impact on the circles around the victim within
// range, up to radius, at most MAX_CIRCLES - 1 (binding's check). nullptr
// (actor, impact's actor): checked; victim: passed on (nothing happens).
// Throws what DestinationShapeSpell throws.
static void MagicDestinationShapeSpell(TScriptCreature Actor, TScriptCreature Victim, int Range, int Animation,
		int Radius, TScriptImpact Impact, int Effect) {
	const char *Name = "Magic.destinationShapeSpell";
	TCreature *cr = ExistingCreature(Actor.ID, Name);
	TCreature *Target = GetCreature(Victim.ID);
	CheckRadius(Radius, "Magic.destinationShapeSpell: radius beyond the circles");
	WithImpact(Impact, Name, "Magic.destinationShapeSpell: unknown impact", [&](TImpact *Made) {
		DestinationShapeSpell(cr, Target, Range, Animation, Radius, Made, Effect);
	});
}

// Magic.angleShapeSpell(actor, angle, range, impact, effect) ->
// AngleShapeSpell(Actor, Angle, Range, Impact, Effect): the impact on the
// fields within angle and range in the actor's direction. nullptr (actor,
// impact's actor): checked. Throws what AngleShapeSpell throws.
static void MagicAngleShapeSpell(TScriptCreature Actor, int Angle, int Range, TScriptImpact Impact, int Effect) {
	const char *Name = "Magic.angleShapeSpell";
	TCreature *cr = ExistingCreature(Actor.ID, Name);
	WithImpact(Impact, Name, "Magic.angleShapeSpell: unknown impact", [&](TImpact *Made) {
		AngleShapeSpell(cr, Angle, Range, Made, Effect);
	});
}

// ---------------------------------------------------------------------------
// The spell library

// The script number of lib/monsters.lua (InitMonsterSpells), -1 before.
static int MonsterSpellLibrary = -1;

// The text of the last start error PrepareMonsterSpell threw.
static std::string MonsterSpellError;

// Logs "<Function>: the scripting engine is not active." and throws ERROR
// when it is not.
static void CheckScriptingActive(const char *Function) {
	if (!ScriptingActive()) {
		error("%s: the scripting engine is not active.\n", Function);
		throw ERROR;
	}
}

void InitMonsterSpells() {
	CheckScriptingActive("InitMonsterSpells");
	MonsterSpellLibrary = LoadScript("lib/monsters.lua", nullptr);
	RequireHandler(MonsterSpellLibrary, "prepareSpell", nullptr);
	RequireHandler(MonsterSpellLibrary, "castSpells", nullptr);
}

// Throws "<Where>: <What>" as a const char * (a start error, as the TOML
// loaders' errors), each '%' doubled (InitAll logs it with error(), which
// hands the text to Log as its format string).
[[noreturn]] static void ThrowMonsterSpellError(const char *Where, const std::string &What) {
	std::string Text = std::string(Where != nullptr ? Where : "monsters.toml") + ": " + What;
	MonsterSpellError.clear();
	for (char c : Text) {
		MonsterSpellError += c;
		if (c == '%') {
			MonsterSpellError += '%';
		}
	}
	throw MonsterSpellError.c_str();
}

void PrepareMonsterSpell(int Race, const TScriptValue &Spell, const char *Where) {
	CheckScriptingActive("PrepareMonsterSpell");
	if (MonsterSpellLibrary < 0) {
		ThrowMonsterSpellError(Where, "the spell library lib/monsters.lua is not loaded");
	}
	TScriptCall Call(MonsterSpellLibrary, "prepareSpell");
	Call.addNumber(Race);
	Call.addValue(Spell);
	try {
		Call.run();
	} catch (RESULT r) {
		const char *Message = Call.failure();
		if (Message != nullptr) {
			ThrowMonsterSpellError(Where, Message);
		}
		ThrowMonsterSpellError(Where, "the spell library raised result " + std::to_string(static_cast<int>(r)));
	}
}

void CastMonsterSpells(uint32_t MonsterID, int Race, uint32_t TargetID) {
	CheckScriptingActive("CastMonsterSpells");
	if (MonsterSpellLibrary < 0) {
		error("CastMonsterSpells: the spell library is not loaded.\n");
		throw ERROR;
	}
	// As the loop read it: GetCreature(Target) for a Target that is not 0.
	uint32_t Target = (TargetID != 0 && GetCreature(TargetID) != nullptr) ? TargetID : 0;
	TScriptCall Call(MonsterSpellLibrary, "castSpells");
	Call.addMonster(MonsterID);
	Call.addNumber(Race);
	Call.addCreature(Target);
	Call.runCaught();
}

void RegisterMonsterAPI() {
	sol::table Monster = ScriptAPITable("Monster");
	Monster.set_function("getMasterId", Guarded(&MonsterGetMasterId));
	Monster.set_function("getTargetId", Guarded(&MonsterGetTargetId));
	Monster.set_function("getType", Guarded(&MonsterGetType));

	sol::table MonsterType = ScriptAPITable("MonsterType");
	MonsterType.set_function("getId", Guarded(&MonsterTypeGetId));
	MonsterType.set_function("getKey", Guarded(&MonsterTypeGetKey));
	MonsterType.set_function("name", Guarded(&MonsterTypeName));
	MonsterType.set_function("runHealth", Guarded(&MonsterTypeRunHealth));
	// MonsterType(key or number): the __call of the shared global's proxy.
	lua_State *L = ScriptState().lua_state();
	lua_pushstring(L, "MonsterType");
	lua_rawget(L, LUA_GLOBALSINDEX);
	lua_getmetatable(L, -1);
	lua_pushcfunction(L, MonsterTypeNew);
	lua_setfield(L, -2, "__call");
	lua_pop(L, 2);

	sol::table Creature = ScriptAPITable("Creature");
	Creature.set_function("turnTo", Guarded(&CreatureTurnTo));

	sol::table Impact = ScriptAPITable("Impact");
	Impact.set_function("damage", Guarded(&ImpactDamage));
	Impact.set_function("field", Guarded(&ImpactField));
	Impact.set_function("healing", Guarded(&ImpactHealing));
	Impact.set_function("speed", Guarded(&ImpactSpeed));
	Impact.set_function("drunken", Guarded(&ImpactDrunken));
	Impact.set_function("strength", Guarded(&ImpactStrength));
	Impact.set_function("outfit", Guarded(&ImpactOutfit));
	Impact.set_function("summon", Guarded(&ImpactSummon));
	Impact.set_function("isAggressive", Guarded(&ImpactIsAggressive));

	sol::table Magic = ScriptAPITable("Magic");
	Magic.set_function("actorShapeSpell", Guarded(&MagicActorShapeSpell));
	Magic.set_function("victimShapeSpell", Guarded(&MagicVictimShapeSpell));
	Magic.set_function("originShapeSpell", Guarded(&MagicOriginShapeSpell));
	Magic.set_function("destinationShapeSpell", Guarded(&MagicDestinationShapeSpell));
	Magic.set_function("angleShapeSpell", Guarded(&MagicAngleShapeSpell));
}
