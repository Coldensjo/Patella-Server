-- lib/npcs/gen-t-shield-s.lua: the rule block gen-t-shield-s.ndb, converted by patella-convert.
-- An NPC that included it (@"gen-t-shield-s.ndb") has it in its rules where the include
-- was; lib/npc.lua matches and runs them (scripting.hh, "NPCs"). The comments
-- are those of gen-t-shield-s.ndb.
--
-- GIMUD - Graphical Interface Multi User Dungeon
-- genTshieldV.ndb: Datenbank für generischen Schildverkauf

local behaviour = require("lib.npc")
local eq = behaviour.eq
local ge = behaviour.ge
local TOPIC = behaviour.TOPIC
local PRICE = behaviour.PRICE
local TYPE = behaviour.TYPE
local COUNTMONEY = behaviour.COUNTMONEY
local setTopic = behaviour.setTopic
local setPrice = behaviour.setPrice
local setAmount = behaviour.setAmount
local setType = behaviour.setType
local create = behaviour.create
local deleteMoney = behaviour.deleteMoney

return behaviour.rules {
	{{"wooden", "shield"}, {setType(3412), setAmount(1), setPrice(15), "Do you want to buy a wooden shield for %P gold?", setTopic(22)}},
	{{"studded", "shield"}, {setType(3426), setAmount(1), setPrice(50), "Do you want to buy a studded shield for %P gold?", setTopic(22)}},
	{{"steel", "shield"}, {setType(3409), setAmount(1), setPrice(240), "Do you want to buy a steel shield for %P gold?", setTopic(22)}},
	{{"brass", "shield"}, {setType(3411), setAmount(1), setPrice(65), "Do you want to buy a brass shield for %P gold?", setTopic(22)}},
	{{"plate", "shield"}, {setType(3410), setAmount(1), setPrice(125), "Do you want to buy a plate shield for %P gold?", setTopic(22)}},
	{{"dwarven", "shield"}, {setType(3425), setAmount(1), setPrice(500), "Do you want to buy a dwarven shield for %P gold?", setTopic(22)}},

	{{eq(TOPIC, 22), "yes", ge(COUNTMONEY, PRICE)}, {"Thank you. Here it is.", deleteMoney, create(TYPE)}},
	{{eq(TOPIC, 22), "yes"}, {"Sorry, you do not have enough gold."}},
	{{eq(TOPIC, 22)}, {"Maybe you will buy it another time."}},
}
