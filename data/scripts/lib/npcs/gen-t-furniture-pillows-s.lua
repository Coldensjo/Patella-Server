-- lib/npcs/gen-t-furniture-pillows-s.lua: the rule block gen-t-furniture-pillows-s.ndb, converted by patella-convert.
-- An NPC that included it (@"gen-t-furniture-pillows-s.ndb") has it in its rules where the include
-- was; lib/npc.lua matches and runs them (scripting.hh, "NPCs"). The comments
-- are those of gen-t-furniture-pillows-s.ndb.
--
-- GIMUD - Graphical Interface Multi User Dungeon
-- gen-t-furniture-pillows-s.ndb: Datenbank für generischen Möbelverkauf - Kissen
-- Verwendete Topics: 81 bis 84
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
	{{"pillow"}, {"I can offer small pillows, round pillows, square pillows and heart pillows. Which one might it be?"}},
	{{"small", "pillow"}, {"What color would you prefer? Purple, green, red, blue, orange, turquoise or white?", setTopic(82)}},
	{{eq(TOPIC, 82), "purple"}, {setType(2386), setAmount(1), setPrice(20), "You want to buy a small, purple pillow for %P gold?", setTopic(81)}},
	{{eq(TOPIC, 82), "green"}, {setType(2387), setAmount(1), setPrice(20), "You want to buy a small, green pillow for %P gold?", setTopic(81)}},
	{{eq(TOPIC, 82), "red"}, {setType(2388), setAmount(1), setPrice(20), "You want to buy a small, red pillow for %P gold?", setTopic(81)}},
	{{eq(TOPIC, 82), "blue"}, {setType(2389), setAmount(1), setPrice(20), "You want to buy a small, blue pillow for %P gold?", setTopic(81)}},
	{{eq(TOPIC, 82), "orange"}, {setType(2390), setAmount(1), setPrice(20), "You want to buy a small, orange pillow for %P gold?", setTopic(81)}},
	{{eq(TOPIC, 82), "turquoise"}, {setType(2391), setAmount(1), setPrice(20), "You want to buy a small, turquoise pillow for %P gold?", setTopic(81)}},
	{{eq(TOPIC, 82), "white"}, {setType(2392), setAmount(1), setPrice(20), "You want to buy a small, white pillow for %P gold?", setTopic(81)}},
	{{"round", "pillow"}, {"What color would you prefer? Purple, red, blue or turquoise?", setTopic(83)}},
	{{eq(TOPIC, 83), "blue"}, {setType(2398), setAmount(1), setPrice(25), "You want to buy a blue, round pillow for %P gold?", setTopic(81)}},
	{{eq(TOPIC, 83), "purple"}, {setType(2400), setAmount(1), setPrice(25), "You want to buy a purple, round pillow for %P gold?", setTopic(81)}},
	{{eq(TOPIC, 83), "red"}, {setType(2399), setAmount(1), setPrice(25), "You want to buy a red, round pillow for %P gold?", setTopic(81)}},
	{{eq(TOPIC, 83), "turquoise"}, {setType(2401), setAmount(1), setPrice(25), "You want to buy a turquoise, round pillow for %P gold?", setTopic(81)}},
	{{"square", "pillow"}, {"What color would you prefer? Red, green, blue or yellow?", setTopic(84)}},
	{{eq(TOPIC, 84), "blue"}, {setType(2394), setAmount(1), setPrice(25), "You want to buy a blue, square pillow for %P gold?", setTopic(81)}},
	{{eq(TOPIC, 84), "red"}, {setType(2395), setAmount(1), setPrice(25), "You want to buy a red, square pillow for %P gold?", setTopic(81)}},
	{{eq(TOPIC, 84), "green"}, {setType(2396), setAmount(1), setPrice(25), "You want to buy a green, square pillow for %P gold?", setTopic(81)}},
	{{eq(TOPIC, 84), "yellow"}, {setType(2397), setAmount(1), setPrice(25), "You want to buy a yellow, square pillow for %P gold?", setTopic(81)}},
	{{"heart", "pillow"}, {setType(2393), setAmount(1), setPrice(30), "You want to buy a heart pillow for %P gold?", setTopic(81)}},

	{{eq(TOPIC, 81), "yes", PREMIUM, ge(COUNTMONEY, PRICE)}, {"Here you are.", deleteMoney, create(TYPE)}},
	{{eq(TOPIC, 81), "yes", ge(COUNTMONEY, PRICE)}, {"I'm sorry, but you need a premium account if you want to buy furniture."}},
	{{eq(TOPIC, 81), "yes"}, {"Come back, when you have enough money."}},
	{{eq(TOPIC, 81)}, {"Hmm, but I'm sure, it would fit nicely into your house."}},
}
