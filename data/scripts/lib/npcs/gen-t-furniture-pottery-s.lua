-- lib/npcs/gen-t-furniture-pottery-s.lua: the rule block gen-t-furniture-pottery-s.ndb, converted by patella-convert.
-- An NPC that included it (@"gen-t-furniture-pottery-s.ndb") has it in its rules where the include
-- was; lib/npc.lua matches and runs them (scripting.hh, "NPCs"). The comments
-- are those of gen-t-furniture-pottery-s.ndb.
--
-- GIMUD - Graphical Interface Multi User Dungeon
-- gen-t-furniture-pottery-s.ndb: Datenbank für generischen Möbelverkauf - Töpfe
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
	{{"pottery"}, {"I offer vases, coal basins, amphora and large amphora. What do you need?"}},
	{{"vase"}, {setType(2876), setAmount(1), setPrice(3), "Do you want to buy a vase for %P gold?", setTopic(81)}},
	{{"large", "amphora"}, {setType(2805), setAmount(1), setPrice(50), "Do you want to buy a large amphora for %P gold?", setTopic(81)}},
	{{"amphora"}, {setType(2893), setAmount(1), setPrice(4), "Do you want to buy an amphora for %P gold?", setTopic(81)}},
	{{"coal", "basin"}, {setType(2806), setAmount(1), setPrice(25), "Do you want to buy a coal basin for %P gold?", setTopic(81)}},

	{{eq(TOPIC, 81), "yes", PREMIUM, ge(COUNTMONEY, PRICE)}, {"Here you are.", deleteMoney, create(TYPE)}},
	{{eq(TOPIC, 81), "yes", ge(COUNTMONEY, PRICE)}, {"I'm sorry, but you need a premium account if you want to buy furniture."}},
	{{eq(TOPIC, 81), "yes"}, {"Come back, when you have enough money."}},
	{{eq(TOPIC, 81)}, {"Hmm, but I'm sure, it would fit nicely into your house."}},
}
