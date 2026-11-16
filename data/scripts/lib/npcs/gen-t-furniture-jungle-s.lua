-- lib/npcs/gen-t-furniture-jungle-s.lua: the rule block gen-t-furniture-jungle-s.ndb, converted by patella-convert.
-- An NPC that included it (@"gen-t-furniture-jungle-s.ndb") has it in its rules where the include
-- was; lib/npc.lua matches and runs them (scripting.hh, "NPCs"). The comments
-- are those of gen-t-furniture-jungle-s.ndb.
--
-- GIMUD - Graphical Interface Multi User Dungeon
-- gen-t-furniture-jungle-s.ndb: Datenbank für generischen Möbelverkauf - Bambusmöbel
-- Verwendete Topics: 81
--"offer"           -> "I sell statues, tables, chairs, flowers, pillows, pottery, instruments, decoration, tapestries and containers."
--"furniture"       -> *
--"equipment"       -> *
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
	{{"chair"}, {"I can offer you tusk chairs, ivory chairs and trunk chairs."}},
	{{"tusk", "chair"}, {setType(2780), setAmount(1), setPrice(25), "You want to buy a tusk chair for %P gold?", setTopic(81)}},
	{{"ivory", "chair"}, {setType(2781), setAmount(1), setPrice(25), "You want to buy an ivory chair for %P gold?", setTopic(81)}},
	{{"trunk", "chair"}, {setType(2809), setAmount(1), setPrice(20), "You want to buy a trunk chair for %P gold?", setTopic(81)}},

	{{"table"}, {"I can offer you stone tables, tusk tables, bamboo tables and trunk tables."}},
	{{"stone", "table"}, {setType(2786), setAmount(1), setPrice(30), "You want to buy a stone table for %P gold?", setTopic(81)}},
	{{"tusk", "table"}, {setType(2787), setAmount(1), setPrice(25), "You want to buy a tusk table for %P gold?", setTopic(81)}},
	{{"bamboo", "table"}, {setType(2788), setAmount(1), setPrice(25), "You want to buy a bamboo table for %P gold?", setTopic(81)}},
	{{"trunk", "table"}, {setType(2810), setAmount(1), setPrice(20), "You want to buy a trunk table for %P gold?", setTopic(81)}},

	{{"drawer"}, {"I can offer you bamboo drawers."}},
	{{"bamboo", "drawer"}, {setType(2795), setAmount(1), setPrice(20), "You want to buy a bamboo drawer for %P gold?", setTopic(81)}},

	{{eq(TOPIC, 81), "yes", PREMIUM, ge(COUNTMONEY, PRICE)}, {"Here you are.", deleteMoney, create(TYPE)}},
	{{eq(TOPIC, 81), "yes", ge(COUNTMONEY, PRICE)}, {"I'm sorry, but you need a premium account if you want to buy furniture."}},
	{{eq(TOPIC, 81), "yes"}, {"Come back, when you have enough money."}},
	{{eq(TOPIC, 81)}, {"Hmm, but I'm sure, it would fit nicely into your house."}},
}
