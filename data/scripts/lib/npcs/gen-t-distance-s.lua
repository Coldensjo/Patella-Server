-- lib/npcs/gen-t-distance-s.lua: the rule block gen-t-distance-s.ndb, converted by patella-convert.
-- An NPC that included it (@"gen-t-distance-s.ndb") has it in its rules where the include
-- was; lib/npc.lua matches and runs them (scripting.hh, "NPCs"). The comments
-- are those of gen-t-distance-s.ndb.
--
-- GIMUD - Graphical Interface Multi User Dungeon
-- gen-t-distance-s.ndb: Datenbank für generischen Fernwaffenverkauf

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
local create = behaviour.create
local deleteMoney = behaviour.deleteMoney
local previous = behaviour.previous

return behaviour.rules {
	{{"buy"}, {"I am selling bows, crossbows, and ammunition. Do you need anything?"}},
	{{"do", "you", "sell"}, {previous}},
	{{"do", "you", "have"}, {previous}},
	{{"offer"}, {previous}},
	{{"goods"}, {previous}},
	{{"ammo"}, {"Do you need arrows for a bow, or bolts for a crossbow?"}},
	{{"ammunition"}, {previous}},

	{{"sell", "bow"}, {"I don't buy used bows."}},
	{{"sell", "crossbow"}, {"I don't buy used crossbows."}},

	{{"bow"}, {setType(3350), setAmount(1), setPrice(400), "Do you want to buy a bow for %P gold?", setTopic(34)}},
	{{"crossbow"}, {setType(3349), setAmount(1), setPrice(500), "Do you want to buy a crossbow for %P gold?", setTopic(34)}},
	{{"arrow"}, {setType(3447), setAmount(10), setPrice(20), "Do you want to buy %A arrows for %P gold?", setTopic(34)}},
	{{"bolt"}, {setType(3446), setAmount(10), setPrice(30), "Do you want to buy %A bolts for %P gold?", setTopic(34)}},
	{{P1, lt(0, P1), "arrow"}, {setType(3447), setAmount(P1), setPrice(mul(2, P1)), "Do you want to buy %A arrows for %P gold?", setTopic(34)}},
	{{P1, lt(0, P1), "bolt"}, {setType(3446), setAmount(P1), setPrice(mul(3, P1)), "Do you want to buy %A bolts for %P gold?", setTopic(34)}},

	{{eq(TOPIC, 34), "yes", ge(COUNTMONEY, PRICE)}, {"Here you are.", deleteMoney, create(TYPE)}},
	{{eq(TOPIC, 34), "yes"}, {"Come back, when you have enough money."}},
	{{eq(TOPIC, 34)}, {"Hmm, but next time."}},
}
