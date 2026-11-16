-- lib/npcs/gen-t-furniture-containers-s.lua: the rule block gen-t-furniture-containers-s.ndb, converted by patella-convert.
-- An NPC that included it (@"gen-t-furniture-containers-s.ndb") has it in its rules where the include
-- was; lib/npc.lua matches and runs them (scripting.hh, "NPCs"). The comments
-- are those of gen-t-furniture-containers-s.ndb.
--
-- GIMUD - Graphical Interface Multi User Dungeon
-- gen-t-furniture-containers-s.ndb: Datenbank für generischen Möbelverkauf - Container
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
	{{"containers"}, {"I offer drawers, dressers, lockers, crates, chests, boxes, barrels, trunks and troughs."}},
	{{"drawer"}, {setType(2789), setAmount(1), setPrice(18), "You want to buy drawers for %P gold?", setTopic(81)}},
	{{"dresser"}, {setType(2790), setAmount(1), setPrice(25), "You want to buy a dresser for %P gold?", setTopic(81)}},
	{{"locker"}, {setType(2791), setAmount(1), setPrice(30), "You want to buy a locker for %P gold?", setTopic(81)}},
	{{"crate"}, {setType(2471), setAmount(1), setPrice(10), "Do you want to buy a crate for %P gold?", setTopic(81)}},
	{{"chest"}, {setType(2472), setAmount(1), setPrice(10), "Do you want to buy a chest for %P gold?", setTopic(81)}},
	{{"box"}, {setType(2469), setAmount(1), setPrice(10), "Do you want to buy a box for %P gold?", setTopic(81)}},
	{{"barrel"}, {setType(2793), setAmount(1), setPrice(12), "Do you want to buy a barrel for %P gold?", setTopic(81)}},
	{{"trough"}, {setType(2792), setAmount(1), setPrice(7), "Do you want to buy a trough for %P gold?", setTopic(81)}},
	{{"trunk"}, {setType(2794), setAmount(1), setPrice(10), "Do you want to buy a trunk for %P gold?", setTopic(81)}},

	{{eq(TOPIC, 81), "yes", PREMIUM, ge(COUNTMONEY, PRICE)}, {"Here you are.", deleteMoney, create(TYPE)}},
	{{eq(TOPIC, 81), "yes", ge(COUNTMONEY, PRICE)}, {"I'm sorry, but you need a premium account if you want to buy furniture."}},
	{{eq(TOPIC, 81), "yes"}, {"Come back, when you have enough money."}},
	{{eq(TOPIC, 81)}, {"Hmm, but I'm sure, it would fit nicely into your house."}},
}
