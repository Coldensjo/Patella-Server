-- lib/npcs/gen-t-furniture-tapestries-s.lua: the rule block gen-t-furniture-tapestries-s.ndb, converted by patella-convert.
-- An NPC that included it (@"gen-t-furniture-tapestries-s.ndb") has it in its rules where the include
-- was; lib/npc.lua matches and runs them (scripting.hh, "NPCs"). The comments
-- are those of gen-t-furniture-tapestries-s.ndb.
--
-- GIMUD - Graphical Interface Multi User Dungeon
-- gen-t-furniture-tapestries-s.ndb: Datenbank für generischen Möbelverkauf - Vorhänge
-- Verwendete Topics: 81, 85
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
	{{"curtain"}, {"Well, actually it's better to call them tapestries."}},
	{{"tapestr"}, {"Please tell me what color you would prefer: purple, green, yellow, orange, red, blue or white?", setTopic(85)}},
	{{eq(TOPIC, 85), "purple"}, {setType(2644), setAmount(1), setPrice(25), "You want to buy a purple tapestry for %P gold?", setTopic(81)}},
	{{eq(TOPIC, 85), "green"}, {setType(2647), setAmount(1), setPrice(25), "You want to buy a green tapestry for %P gold?", setTopic(81)}},
	{{eq(TOPIC, 85), "yellow"}, {setType(2650), setAmount(1), setPrice(25), "You want to buy a yellow tapestry for %P gold?", setTopic(81)}},
	{{eq(TOPIC, 85), "orange"}, {setType(2653), setAmount(1), setPrice(25), "You want to buy a orange tapestry for %P gold?", setTopic(81)}},
	{{eq(TOPIC, 85), "red"}, {setType(2656), setAmount(1), setPrice(25), "You want to buy a red tapestry for %P gold?", setTopic(81)}},
	{{eq(TOPIC, 85), "orange"}, {setType(2653), setAmount(1), setPrice(25), "You want to buy a orange tapestry for %P gold?", setTopic(81)}},
	{{eq(TOPIC, 85), "red"}, {setType(2656), setAmount(1), setPrice(25), "You want to buy a red tapestry for %P gold?", setTopic(81)}},
	{{eq(TOPIC, 85), "blue"}, {setType(2659), setAmount(1), setPrice(25), "You want to buy a blue tapestry for %P gold?", setTopic(81)}},
	{{eq(TOPIC, 85), "white"}, {setType(2667), setAmount(1), setPrice(25), "You want to buy a white tapestry for %P gold?", setTopic(81)}},

	{{eq(TOPIC, 81), "yes", PREMIUM, ge(COUNTMONEY, PRICE)}, {"Here you are.", deleteMoney, create(TYPE)}},
	{{eq(TOPIC, 81), "yes", ge(COUNTMONEY, PRICE)}, {"I'm sorry, but you need a premium account if you want to buy furniture."}},
	{{eq(TOPIC, 81), "yes"}, {"Come back, when you have enough money."}},
	{{eq(TOPIC, 81)}, {"Hmm, but I'm sure, it would fit nicely into your house."}},
}
