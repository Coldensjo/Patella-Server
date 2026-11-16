-- lib/npcs/gen-t-legs-s.lua: the rule block gen-t-legs-s.ndb, converted by patella-convert.
-- An NPC that included it (@"gen-t-legs-s.ndb") has it in its rules where the include
-- was; lib/npc.lua matches and runs them (scripting.hh, "NPCs"). The comments
-- are those of gen-t-legs-s.ndb.
--
-- GIMUD - Graphical Interface Multi User Dungeon
-- genTlegsV.ndb:  Datenbank für generischen Legsverkauf

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
	{{"leather", "boot"}, {setType(3552), setAmount(1), setPrice(2), "Do you want to buy one of my wonderful leather boots for %P gold?", setTopic(23)}},
	{{"leather", "legs"}, {setType(3559), setAmount(1), setPrice(10), "Do you want to buy leather legs for %P gold?", setTopic(23)}},
	{{"studded", "legs"}, {setType(3362), setAmount(1), setPrice(60), "Do you want to buy studded legs for %P gold?", setTopic(23)}},
	{{"chain", "legs"}, {setType(3558), setAmount(1), setPrice(80), "You want buy chain legs for %P gold?", setTopic(23)}},
	{{"brass", "legs"}, {setType(3372), setAmount(1), setPrice(195), "You want buy brass legs for %P gold?", setTopic(23)}},

	{{eq(TOPIC, 23), "yes", ge(COUNTMONEY, PRICE)}, {"Thank you. Here it is.", deleteMoney, create(TYPE)}},
	{{eq(TOPIC, 23), "yes"}, {"Sorry, you do not have enough gold."}},
	{{eq(TOPIC, 23)}, {"Maybe you will buy it another time."}},
}
