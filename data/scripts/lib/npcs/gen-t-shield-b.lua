-- lib/npcs/gen-t-shield-b.lua: the rule block gen-t-shield-b.ndb, converted by patella-convert.
-- An NPC that included it (@"gen-t-shield-b.ndb") has it in its rules where the include
-- was; lib/npc.lua matches and runs them (scripting.hh, "NPCs"). The comments
-- are those of gen-t-shield-b.ndb.
--
-- GIMUD - Graphical Interface Multi User Dungeon
-- gen-t-shield-b.ndb: Datenbank für generischen Schildkauf

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

return behaviour.rules {
	{{"sell", "wooden", "shield"}, {setType(3412), setAmount(1), setPrice(5), "Do you want to sell a wooden shield for %P gold?", setTopic(52)}},
	{{"sell", "battle", "shield"}, {setType(3413), setAmount(1), setPrice(95), "Do you want to sell a battle shield for %P gold?", setTopic(52)}},
	{{"sell", "steel", "shield"}, {setType(3409), setAmount(1), setPrice(80), "Do you want to sell a steel shield for %P gold?", setTopic(52)}},
	{{"sell", "brass", "shield"}, {setType(3411), setAmount(1), setPrice(16), "Do you want to sell a brass shield for %P gold?", setTopic(52)}},
	{{"sell", "plate", "shield"}, {setType(3410), setAmount(1), setPrice(45), "Do you want to sell a plate shield for %P gold?", setTopic(52)}},
	{{"sell", "dwarven", "shield"}, {setType(3425), setAmount(1), setPrice(100), "You want sell a dwarven shield for %P gold?", setTopic(52)}},
	{{"sell", "guardians", "shield"}, {setType(3415), setAmount(1), setPrice(180), "Do you want to sell this for %P gold?", setTopic(52)}},
	{{"sell", "dragon", "shield"}, {setType(3416), setAmount(1), setPrice(360), "Do you want to sell this for %P gold?", setTopic(52)}},

	{{"sell", P1, lt(1, P1), "wooden", "shield"}, {setType(3412), setAmount(P1), setPrice(mul(5, P1)), "Do you want to sell those wooden shields for %P gold?", setTopic(52)}},
	{{"sell", P1, lt(1, P1), "battle", "shield"}, {setType(3413), setAmount(P1), setPrice(mul(95, P1)), "Do you want to sell those battle shields for %P gold?", setTopic(52)}},
	{{"sell", P1, lt(1, P1), "steel", "shield"}, {setType(3409), setAmount(P1), setPrice(mul(80, P1)), "Do you want to sell those steel shields for %P gold?", setTopic(52)}},
	{{"sell", P1, lt(1, P1), "brass", "shield"}, {setType(3411), setAmount(P1), setPrice(mul(16, P1)), "Do you want to sell those brass shields for %P gold?", setTopic(52)}},
	{{"sell", P1, lt(1, P1), "plate", "shield"}, {setType(3410), setAmount(P1), setPrice(mul(45, P1)), "Do you want to sell those plate shields for %P gold?", setTopic(52)}},
	{{"sell", P1, lt(1, P1), "dwarven", "shield"}, {setType(3425), setAmount(P1), setPrice(mul(100, P1)), "Do you want sell those dwarven shields for %P gold?", setTopic(52)}},
	{{"sell", P1, lt(1, P1), "guardians", "shield"}, {setType(3415), setAmount(P1), setPrice(mul(180, P1)), "Do you want to sell those guardian shields for %P gold?", setTopic(52)}},
	{{"sell", P1, lt(1, P1), "dragon", "shield"}, {setType(3416), setAmount(P1), setPrice(mul(360, P1)), "Do you want to sell those dragon shields for %P gold?", setTopic(52)}},

	{{eq(TOPIC, 52), "yes", ge(COUNTMONEY, PRICE)}, {"Thank you. Here it is.", deleteMoney, create(TYPE)}},
	{{eq(TOPIC, 52), "yes"}, {"Sorry, you do not have enough gold."}},
	{{eq(TOPIC, 52)}, {"Maybe you will buy it another time."}},
}
