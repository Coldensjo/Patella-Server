-- lib/npcs/gen-t-helm-s.lua: the rule block gen-t-helm-s.ndb, converted by patella-convert.
-- An NPC that included it (@"gen-t-helm-s.ndb") has it in its rules where the include
-- was; lib/npc.lua matches and runs them (scripting.hh, "NPCs"). The comments
-- are those of gen-t-helm-s.ndb.
--
-- GIMUD - Graphical Interface Multi User Dungeon
-- genThelmV.ndb:  Datenbank für generischen Helmverkauf

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
	{{"chain", "helmet"}, {setType(3352), setAmount(1), setPrice(52), "Do you want to buy a chain helmet for %P gold?", setTopic(20)}},
	{{"leather", "helmet"}, {setType(3355), setAmount(1), setPrice(12), "Do you want to buy a leather helmet for %P gold?", setTopic(20)}},
	{{"studded", "helmet"}, {setType(3376), setAmount(1), setPrice(63), "Do you want to buy a studded helmet for %P gold?", setTopic(21)}},
	{{"brass", "helmet"}, {setType(3354), setAmount(1), setPrice(120), "Do you want to buy a brass helmet for %P gold?", setTopic(21)}},
	{{"viking", "helmet"}, {setType(3367), setAmount(1), setPrice(265), "Do you want to buy a viking helmet for %P gold?", setTopic(21)}},

	{{eq(TOPIC, 20), "yes", ge(COUNTMONEY, PRICE)}, {"Thank you. Here it is.", deleteMoney, create(TYPE)}},
	{{eq(TOPIC, 20), "yes"}, {"Sorry, you do not have enough gold."}},
	{{eq(TOPIC, 20)}, {"Maybe you will buy it another time."}},
}
