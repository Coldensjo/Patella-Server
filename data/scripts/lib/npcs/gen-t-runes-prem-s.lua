-- lib/npcs/gen-t-runes-prem-s.lua: the rule block gen-t-runes-prem-s.ndb, converted by patella-convert.
-- An NPC that included it (@"gen-t-runes-prem-s.ndb") has it in its rules where the include
-- was; lib/npc.lua matches and runs them (scripting.hh, "NPCs"). The comments
-- are those of gen-t-runes-prem-s.ndb.
--
-- GIMUD - Graphical Interface Multi User Dungeon
-- gen-t-runes-prem-s.ndb: Datenbank für generischen Runenverkauf - Premium Account Runen
-- Verwendete Topics: 99

local behaviour = require("lib.npc")
local P1 = behaviour.P1
local eq = behaviour.eq
local lt = behaviour.lt
local ge = behaviour.ge
local TOPIC = behaviour.TOPIC
local PRICE = behaviour.PRICE
local TYPE = behaviour.TYPE
local COUNTMONEY = behaviour.COUNTMONEY
local mul = behaviour.mul
local setTopic = behaviour.setTopic
local setPrice = behaviour.setPrice
local setAmount = behaviour.setAmount
local setType = behaviour.setType
local setData = behaviour.setData
local create = behaviour.create
local deleteMoney = behaviour.deleteMoney
local previous = behaviour.previous

return behaviour.rules {
	{{"spell", "rune"}, {"I sell envenom runes, desintegrate runes, poison bomb runes, energy bomb runes, soulfire runes, magic wall runes, animate dead runes and paralyze runes."}},

	{{"envenom", "rune"}, {setType(3179), setData(3), setAmount(1), setPrice(130), "Do you want to buy an envenom rune for %P gold?", setTopic(99)}},
	{{"desintegrate", "rune"}, {setType(3197), setData(3), setAmount(1), setPrice(80), "Do you want to buy a desintegrate rune for %P gold?", setTopic(99)}},
	{{"poison", "bomb", "rune"}, {setType(3173), setData(2), setAmount(1), setPrice(170), "Do you want to buy a poison bomb rune for %P gold?", setTopic(99)}},
	{{"soulfire", "rune"}, {setType(3195), setData(2), setAmount(1), setPrice(210), "Do you want to buy a soulfire rune for %P gold?", setTopic(99)}},
	{{"energy", "bomb", "rune"}, {setType(3149), setData(2), setAmount(1), setPrice(325), "Do you want to buy an energybomb rune for %P gold?", setTopic(99)}},
	{{"magic", "wall", "rune"}, {setType(3180), setData(3), setAmount(1), setPrice(350), "Do you want to buy a magic wall rune for %P gold?", setTopic(99)}},
	{{"animate", "dead", "rune"}, {setType(3203), setData(1), setAmount(1), setPrice(375), "Do you want to buy an animate dead rune for %P gold?", setTopic(99)}},
	{{"paralyze", "rune"}, {setType(3165), setData(1), setAmount(1), setPrice(700), "Do you want to buy a paralyze rune for %P gold?", setTopic(99)}},

	{{P1, lt(1, P1), "envenom", "rune"}, {setType(3179), setData(3), setAmount(P1), setPrice(mul(130, P1)), "Do you want to buy %A envenom runes for %P gold?", setTopic(99)}},
	{{P1, lt(1, P1), "desintegrate", "rune"}, {setType(3197), setData(3), setAmount(P1), setPrice(mul(80, P1)), "Do you want to buy %A desintegrate runes for %P gold?", setTopic(99)}},
	{{P1, lt(1, P1), "poison", "bomb", "rune"}, {setType(3173), setData(2), setAmount(P1), setPrice(mul(170, P1)), "Do you want to buy %A poison bomb runes for %P gold?", setTopic(99)}},
	{{P1, lt(1, P1), "soulfire", "rune"}, {setType(3195), setData(2), setAmount(P1), setPrice(mul(210, P1)), "Do you want to buy %A soulfire runes for %P gold?", setTopic(99)}},
	{{P1, lt(1, P1), "energy", "bomb", "rune"}, {setType(3149), setData(2), setAmount(P1), setPrice(mul(325, P1)), "Do you want to buy %A energybomb runes for %P gold?", setTopic(99)}},
	{{P1, lt(1, P1), "magic", "wall", "rune"}, {setType(3180), setData(3), setAmount(P1), setPrice(mul(350, P1)), "Do you want to buy %A magic wall runes for %P gold?", setTopic(99)}},
	{{P1, lt(1, P1), "animate", "dead", "rune"}, {setType(3203), setData(1), setAmount(P1), setPrice(mul(375, P1)), "Do you want to buy %A animate dead runes for %P gold?", setTopic(99)}},
	{{P1, lt(1, P1), "paralyze", "rune"}, {setType(3165), setData(1), setAmount(P1), setPrice(mul(700, P1)), "Do you want to buy %A paralyze runes for %P gold?", setTopic(99)}},

	{{"light", "magic", "missile", "rune"}, {"Sorry, but runes of this type can't be purchased here."}},
	{{"poison", "field", "rune"}, {previous}},
	{{"antidote", "rune"}, {previous}},
	{{"fire", "field", "rune"}, {previous}},
	{{"intense", "healing", "rune"}, {previous}},
	{{"fireball", "rune"}, {previous}},
	{{"destroy", "field", "rune"}, {previous}},
	{{"heavy", "magic", "missile", "rune"}, {previous}},
	{{"energy", "field", "rune"}, {previous}},
	{{"ultimate", "healing", "rune"}, {previous}},
	{{"convince", "creature", "rune"}, {previous}},
	{{"great", "fireball", "rune"}, {previous}},
	{{"chameleon", "rune"}, {previous}},
	{{"fire", "bomb", "rune"}, {previous}},
	{{"poison", "wall", "rune"}, {previous}},
	{{"explosion", "rune"}, {previous}},
	{{"fire", "wall", "rune"}, {previous}},
	{{"sudden", "death", "rune"}, {previous}},
	{{"energy", "wall", "rune"}, {previous}},

	{{eq(TOPIC, 99), "yes", ge(COUNTMONEY, PRICE)}, {"Here you are.", deleteMoney, create(TYPE)}},
	{{eq(TOPIC, 99), "yes"}, {"Sorry, you don't have enough gold."}},
	{{eq(TOPIC, 99)}, {"As you wish."}},
}
