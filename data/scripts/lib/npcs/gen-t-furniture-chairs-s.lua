-- lib/npcs/gen-t-furniture-chairs-s.lua: the rule block gen-t-furniture-chairs-s.ndb, converted by patella-convert.
-- An NPC that included it (@"gen-t-furniture-chairs-s.ndb") has it in its rules where the include
-- was; lib/npc.lua matches and runs them (scripting.hh, "NPCs"). The comments
-- are those of gen-t-furniture-chairs-s.ndb.
--
-- GIMUD - Graphical Interface Multi User Dungeon
-- gen-t-furniture-chairs-s.ndb: Datenbank für generischen Möbelverkauf - Stühle
-- Verwendete Topics: 81
--"offer"           -> "I sell statues, tables, chairs, flowers, pillows, pottery, instruments, decoration, tapestries and containers."
--"furniture"       -> *
--"equipment"       -> *
--"goods"           -> *
--"do","you","sell" -> *
--"do","you","have" -> *

local behaviour = require("lib.npc")
local PREMIUM = behaviour.PREMIUM
local FORCE = behaviour.FORCE
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
local previous = behaviour.previous

return behaviour.rules {
	{{"red", "cushioned", FORCE}, {setType(2775), setAmount(1), setPrice(40), "You want to buy a red cushioned chair for %P gold?", setTopic(81)}},
	{{"red", "chair", FORCE}, {previous}},
	{{"green", "cushioned", FORCE}, {setType(2776), setAmount(1), setPrice(40), "You want to buy a green cushioned chair for %P gold?", setTopic(81)}},
	{{"green", "chair", FORCE}, {previous}},
	{{"chair"}, {"I can offer you wooden chairs, rocking chairs, red cushioned chairs, green cushioned chairs and sofa chairs."}},
	{{"wooden", "chair"}, {setType(2777), setAmount(1), setPrice(15), "You want to buy a wooden chair for %P gold?", setTopic(81)}},
	{{"rocking", "chair"}, {setType(2778), setAmount(1), setPrice(25), "You want to buy a rocking chair for %P gold?", setTopic(81)}},
	{{"cushioned", "chair"}, {"I can offer you a red cushioned chair or a green cushioned chair."}},
	{{"sofa", "chair"}, {setType(2779), setAmount(1), setPrice(55), "You want to buy a sofa chair for %P gold?", setTopic(81)}},

	{{eq(TOPIC, 81), "yes", PREMIUM, ge(COUNTMONEY, PRICE)}, {"Here you are.", deleteMoney, create(TYPE)}},
	{{eq(TOPIC, 81), "yes", ge(COUNTMONEY, PRICE)}, {"I'm sorry, but you need a premium account if you want to buy furniture."}},
	{{eq(TOPIC, 81), "yes"}, {"Come back, when you have enough money."}},
	{{eq(TOPIC, 81)}, {"Hmm, but I'm sure, it would fit nicely into your house."}},
}
