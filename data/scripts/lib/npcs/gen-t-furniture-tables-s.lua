-- lib/npcs/gen-t-furniture-tables-s.lua: the rule block gen-t-furniture-tables-s.ndb, converted by patella-convert.
-- An NPC that included it (@"gen-t-furniture-tables-s.ndb") has it in its rules where the include
-- was; lib/npc.lua matches and runs them (scripting.hh, "NPCs"). The comments
-- are those of gen-t-furniture-tables-s.ndb.
--
-- GIMUD - Graphical Interface Multi User Dungeon
-- gen-t-furniture-tables-s.ndb: Datenbank für generischen Möbelverkauf - Tische
-- Verwendete Topics: 81
--"offer"           -> "I sell statues, tables, chairs, flowers, pillows, pottery, instruments, decoration, tapestries and containers."
--"furniture"       -> *
--"goods"           -> *
--"do","you","sell" -> *
--"do","you","have" -> *

local behaviour = require("lib.npc")
local PREMIUM = behaviour.PREMIUM
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
	{{"table"}, {"Do you want to buy a small table, a round table, a square table or a big table?"}},
	{{"small", "table"}, {setType(2782), setAmount(1), setPrice(20), "Do you want to buy a small table for %P gold?", setTopic(81)}},
	{{"round", "table"}, {setType(2783), setAmount(1), setPrice(25), "Do you want to buy a round table for %P gold?", setTopic(81)}},
	{{"square", "table"}, {setType(2784), setAmount(1), setPrice(25), "Do you want to buy a square table for %P gold?", setTopic(81)}},
	{{"big", "table"}, {setType(2785), setAmount(1), setPrice(30), "Do you want to buy a big table for %P gold?", setTopic(81)}},

	{{eq(TOPIC, 81), "yes", PREMIUM, ge(COUNTMONEY, PRICE)}, {"Here you are.", deleteMoney, create(TYPE)}},
	{{eq(TOPIC, 81), "yes", ge(COUNTMONEY, PRICE)}, {"I'm sorry, but you need a premium account if you want to buy furniture."}},
	{{eq(TOPIC, 81), "yes"}, {"Come back, when you have enough money."}},
	{{eq(TOPIC, 81)}, {"Hmm, but I'm sure, it would fit nicely into your house."}},
}
