-- lib/npcs/gen-t-armor-s.lua: the rule block gen-t-armor-s.ndb, converted by patella-convert.
-- An NPC that included it (@"gen-t-armor-s.ndb") has it in its rules where the include
-- was; lib/npc.lua matches and runs them (scripting.hh, "NPCs"). The comments
-- are those of gen-t-armor-s.ndb.
--
-- GIMUD - Graphical Interface Multi User Dungeon
-- genTarmorV.ndb: Datenbank für generischen Rüstungsverkauf

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
	{{"jacket"}, {setType(3561), setAmount(1), setPrice(10), "Do you want to buy a jacket for %P gold?", setTopic(21)}},
	{{"coat"}, {setType(3562), setAmount(1), setPrice(8), "Do you want to buy a coat for %P gold?", setTopic(21)}},
	{{"doublet"}, {setType(3379), setAmount(1), setPrice(16), "Do you want to buy a dublet for %P gold?", setTopic(21)}},
	{{"leather", "armor"}, {setType(3361), setAmount(1), setPrice(35), "Do you want to buy a leather armor for %P gold?", setTopic(21)}},
	{{"chain", "armor"}, {setType(3358), setAmount(1), setPrice(200), "Do you want to buy a chain armor for %P gold?", setTopic(21)}},
	{{"brass", "armor"}, {setType(3359), setAmount(1), setPrice(450), "Do you want to buy a brass armor for %P gold?", setTopic(21)}},
	{{"plate", "armor"}, {setType(3357), setAmount(1), setPrice(1200), "Do you want to buy a plate armor for %P gold?", setTopic(21)}},

	{{eq(TOPIC, 21), "yes", ge(COUNTMONEY, PRICE)}, {"Thank you. Here it is.", deleteMoney, create(TYPE)}},
	{{eq(TOPIC, 21), "yes"}, {"Sorry, you do not have enough gold."}},
	{{eq(TOPIC, 21)}, {"Maybe you will buy it another time."}},
}
