-- lib/npcs/gen-t-furniture-statues-s.lua: the rule block gen-t-furniture-statues-s.ndb, converted by patella-convert.
-- An NPC that included it (@"gen-t-furniture-statues-s.ndb") has it in its rules where the include
-- was; lib/npc.lua matches and runs them (scripting.hh, "NPCs"). The comments
-- are those of gen-t-furniture-statues-s.ndb.
--
-- GIMUD - Graphical Interface Multi User Dungeon
-- gen-t-furniture-statues-s.ndb: Datenbank für generischen Möbelverkauf - Statuen
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
	{{"statue"}, {"What statue would you like? A knights statue, a minotaur statue or a goblin statue?"}},
	{{"knight", "statue"}, {setType(2802), setAmount(1), setPrice(50), "Do you want to buy this wonderful statue for %P gold?", setTopic(81)}},
	{{"minotaur", "statue"}, {setType(2803), setAmount(1), setPrice(50), "Do you want to buy this frigtening statue for %P gold?", setTopic(81)}},
	{{"goblin", "statue"}, {setType(2804), setAmount(1), setPrice(50), "Do you want to buy this disgusting statue for %P gold?", setTopic(81)}},

	{{eq(TOPIC, 81), "yes", PREMIUM, ge(COUNTMONEY, PRICE)}, {"Here you are.", deleteMoney, create(TYPE)}},
	{{eq(TOPIC, 81), "yes", ge(COUNTMONEY, PRICE)}, {"I'm sorry, but you need a premium account if you want to buy furniture."}},
	{{eq(TOPIC, 81), "yes"}, {"Come back, when you have enough money."}},
	{{eq(TOPIC, 81)}, {"Hmm, but I'm sure, it would fit nicely into your house."}},
}
