-- lib/npcs/gen-t-runes-free-s.lua: the rule block gen-t-runes-free-s.ndb, converted by patella-convert.
-- An NPC that included it (@"gen-t-runes-free-s.ndb") has it in its rules where the include
-- was; lib/npc.lua matches and runs them (scripting.hh, "NPCs"). The comments
-- are those of gen-t-runes-free-s.ndb.
--
-- GIMUD - Graphical Interface Multi User Dungeon
-- gen-t-runes-free-s.ndb: Datenbank für generischen Runenverkauf - Free Account Runen
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
	{{"spell", "rune"}, {"I sell missile runes, explosive runes, field runes, wall runes, bomb runes, healing runes, convince creature runes and chameleon runes."}},
	{{"missile", "rune"}, {"I can offer you light magic missile runes, heavy magic missile runes and sudden death runes."}},
	{{"explosive", "rune"}, {"I can offer you fireball runes, great fireball runes and explosion runes."}},
	{{"field", "rune"}, {"I can offer you fire field runes, energy field runes, poison field runes and destroy field runes."}},
	{{"wall", "rune"}, {"I can offer you fire wall runes, energy wall runes and poison wall runes."}},
	{{"bomb", "rune"}, {"I can offer you firebomb runes."}},
	{{"healing", "rune"}, {"I can offer you antidote runes, intense healing runes and ultimate healing runes."}},

	{{"light", "magic", "missile", "rune"}, {setType(3174), setData(5), setAmount(1), setPrice(40), "Do you want to buy a light magic missile rune for %P gold?", setTopic(99)}},
	{{"poison", "field", "rune"}, {setType(3172), setData(3), setAmount(1), setPrice(65), "Do you want to buy a poison field rune for %P gold?", setTopic(99)}},
	{{"antidote", "rune"}, {setType(3153), setData(1), setAmount(1), setPrice(65), "Do you want to buy an antidote rune for %P gold?", setTopic(99)}},
	{{"fire", "field", "rune"}, {setType(3188), setData(3), setAmount(1), setPrice(85), "Do you want to buy a fire field rune for %P gold?", setTopic(99)}},
	{{"intense", "healing", "rune"}, {setType(3152), setData(1), setAmount(1), setPrice(95), "Do you want to buy an intense healing rune for %P gold?", setTopic(99)}},
	{{"fireball", "rune"}, {setType(3189), setData(2), setAmount(1), setPrice(95), "Do you want to buy a fireball rune for %P gold?", setTopic(99)}},
	{{"destroy", "field", "rune"}, {setType(3148), setData(3), setAmount(1), setPrice(45), "Do you want to buy a destroy field rune for %P gold?", setTopic(99)}},
	{{"heavy", "magic", "missile", "rune"}, {setType(3198), setData(5), setAmount(1), setPrice(125), "Do you want to buy a heavy magic missile rune for %P gold?", setTopic(99)}},
	{{"energy", "field", "rune"}, {setType(3164), setData(3), setAmount(1), setPrice(115), "Do you want to buy an energy field rune for %P gold?", setTopic(99)}},
	{{"ultimate", "healing", "rune"}, {setType(3160), setData(1), setAmount(1), setPrice(175), "Do you want to buy an ultimate healing rune for %P gold?", setTopic(99)}},
	{{"convince", "creature", "rune"}, {setType(3177), setData(1), setAmount(1), setPrice(80), "Do you want to buy a convince creature rune for %P gold?", setTopic(99)}},
	{{"great", "fireball", "rune"}, {setType(3191), setData(2), setAmount(1), setPrice(180), "Do you want to buy a great fireball rune for %P gold?", setTopic(99)}},
	{{"chameleon", "rune"}, {setType(3178), setData(1), setAmount(1), setPrice(210), "Do you want to buy a chameleon rune for %P gold?", setTopic(99)}},
	{{"fire", "bomb", "rune"}, {setType(3192), setData(2), setAmount(1), setPrice(235), "Do you want to buy a firebomb rune for %P gold?", setTopic(99)}},
	{{"poison", "wall", "rune"}, {setType(3176), setData(4), setAmount(1), setPrice(210), "Do you want to buy a poison wall rune for %P gold?", setTopic(99)}},
	{{"explosion", "rune"}, {setType(3200), setData(3), setAmount(1), setPrice(250), "Do you want to buy an explosion rune for %P gold?", setTopic(99)}},
	{{"fire", "wall", "rune"}, {setType(3190), setData(4), setAmount(1), setPrice(245), "Do you want to buy a fire wall rune for %P gold?", setTopic(99)}},
	{{"sudden", "death", "rune"}, {setType(3155), setData(1), setAmount(1), setPrice(325), "Do you want to buy a sudden death rune for %P gold?", setTopic(99)}},
	{{"energy", "wall", "rune"}, {setType(3166), setData(4), setAmount(1), setPrice(340), "Do you want to buy an energy wall rune for %P gold?", setTopic(99)}},

	{{"envenom", "rune"}, {"Sorry, but runes of this type can't be purchased here."}},
	{{"desintegrate", "rune"}, {previous}},
	{{"poison", "bomb", "rune"}, {previous}},
	{{"soulfire", "rune"}, {previous}},
	{{"energy", "bomb", "rune"}, {previous}},
	{{"magic", "wall", "rune"}, {previous}},
	{{"animate", "dead", "rune"}, {previous}},
	{{"paralyze", "rune"}, {previous}},

	{{P1, lt(1, P1), "light", "magic", "missile", "rune"}, {setType(3174), setData(5), setAmount(P1), setPrice(mul(40, P1)), "Do you want to buy %A light magic missile runes for %P gold?", setTopic(99)}},
	{{P1, lt(1, P1), "poison", "field", "rune"}, {setType(3172), setData(3), setAmount(P1), setPrice(mul(65, P1)), "Do you want to buy %A poison field runes for %P gold?", setTopic(99)}},
	{{P1, lt(1, P1), "antidote", "rune"}, {setType(3153), setData(1), setAmount(P1), setPrice(mul(65, P1)), "Do you want to buy %A antidote runes for %P gold?", setTopic(99)}},
	{{P1, lt(1, P1), "fire", "field", "rune"}, {setType(3188), setData(3), setAmount(P1), setPrice(mul(85, P1)), "Do you want to buy %A fire field runes for %P gold?", setTopic(99)}},
	{{P1, lt(1, P1), "intense", "healing", "rune"}, {setType(3152), setData(1), setAmount(P1), setPrice(mul(95, P1)), "Do you want to buy %A intense healing runes for %P gold?", setTopic(99)}},
	{{P1, lt(1, P1), "fireball", "rune"}, {setType(3189), setData(2), setAmount(P1), setPrice(mul(95, P1)), "Do you want to buy %A fireball runes for %P gold?", setTopic(99)}},
	{{P1, lt(1, P1), "destroy", "field", "rune"}, {setType(3148), setData(3), setAmount(P1), setPrice(mul(45, P1)), "Do you want to buy %A destroy field runes for %P gold?", setTopic(99)}},
	{{P1, lt(1, P1), "heavy", "magic", "missile", "rune"}, {setType(3198), setData(5), setAmount(P1), setPrice(mul(125, P1)), "Do you want to buy %A heavy magic missile runes for %P gold?", setTopic(99)}},
	{{P1, lt(1, P1), "energy", "field", "rune"}, {setType(3164), setData(3), setAmount(P1), setPrice(mul(115, P1)), "Do you want to buy %A energy field runes for %P gold?", setTopic(99)}},
	{{P1, lt(1, P1), "ultimate", "healing", "rune"}, {setType(3160), setData(1), setAmount(P1), setPrice(mul(175, P1)), "Do you want to buy %A ultimate healing runes for %P gold?", setTopic(99)}},
	{{P1, lt(1, P1), "convince", "creature", "rune"}, {setType(3177), setData(1), setAmount(P1), setPrice(mul(80, P1)), "Do you want to buy %A convince creature runes for %P gold?", setTopic(99)}},
	{{P1, lt(1, P1), "great", "fireball", "rune"}, {setType(3191), setData(2), setAmount(P1), setPrice(mul(180, P1)), "Do you want to buy %A great fireball runes for %P gold?", setTopic(99)}},
	{{P1, lt(1, P1), "chameleon", "rune"}, {setType(3178), setData(1), setAmount(P1), setPrice(mul(210, P1)), "Do you want to buy %A chameleon runes for %P gold?", setTopic(99)}},
	{{P1, lt(1, P1), "fire", "bomb", "rune"}, {setType(3192), setData(2), setAmount(P1), setPrice(mul(235, P1)), "Do you want to buy %A firebomb runes for %P gold?", setTopic(99)}},
	{{P1, lt(1, P1), "poison", "wall", "rune"}, {setType(3176), setData(4), setAmount(P1), setPrice(mul(210, P1)), "Do you want to buy %A poison wall runes for %P gold?", setTopic(99)}},
	{{P1, lt(1, P1), "explosion", "rune"}, {setType(3200), setData(3), setAmount(P1), setPrice(mul(250, P1)), "Do you want to buy %A explosion runes for %P gold?", setTopic(99)}},
	{{P1, lt(1, P1), "fire", "wall", "rune"}, {setType(3190), setData(4), setAmount(P1), setPrice(mul(245, P1)), "Do you want to buy %A fire wall runes for %P gold?", setTopic(99)}},
	{{P1, lt(1, P1), "sudden", "death", "rune"}, {setType(3155), setData(1), setAmount(P1), setPrice(mul(325, P1)), "Do you want to buy %A sudden death runes for %P gold?", setTopic(99)}},
	{{P1, lt(1, P1), "energy", "wall", "rune"}, {setType(3166), setData(4), setAmount(P1), setPrice(mul(340, P1)), "Do you want to buy %A energy wall runes for %P gold?", setTopic(99)}},

	{{eq(TOPIC, 99), "yes", ge(COUNTMONEY, PRICE)}, {"Here you are.", deleteMoney, create(TYPE)}},
	{{eq(TOPIC, 99), "yes"}, {"Sorry, you don't have enough gold."}},
	{{eq(TOPIC, 99)}, {"As you wish."}},
}
