-- lib/npcs/gen-t-furniture-flowers-s.lua: the rule block gen-t-furniture-flowers-s.ndb, converted by patella-convert.
-- An NPC that included it (@"gen-t-furniture-flowers-s.ndb") has it in its rules where the include
-- was; lib/npc.lua matches and runs them (scripting.hh, "NPCs"). The comments
-- are those of gen-t-furniture-flowers-s.ndb.
--
-- GIMUD - Graphical Interface Multi User Dungeon
-- gen-t-furniture-flowers-s.ndb: Datenbank für generischen Möbelverkauf - Pflanzen und Blumen
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
local previous = behaviour.previous

return behaviour.rules {
	{{"flower"}, {"I offer indoor plants, flower bowls, god flowers, honey flowers and potted flowers. What do you need?"}},
	{{"plant"}, {previous}},
	{{"god", "flower"}, {setType(2981), setAmount(1), setPrice(5), "Do you want to buy god flowers for %P gold?", setTopic(81)}},
	{{"indoor", "plant"}, {setType(2811), setAmount(1), setPrice(8), "Do you want to buy an indoor plant for %P gold?", setTopic(81)}},
	{{"flower", "bowl"}, {setType(2983), setAmount(1), setPrice(6), "Do you want to buy a flower bowl for %P gold?", setTopic(81)}},
	{{"honey", "flower"}, {setType(2984), setAmount(1), setPrice(5), "Do you want to buy a honey flower for %P gold?", setTopic(81)}},
	{{"potted", "flower"}, {setType(2985), setAmount(1), setPrice(5), "Do you want to buy a potted flower for %P gold?", setTopic(81)}},
	-- "christmas","tree" -> Type=2812, Amount=1, Price=50, "A christmas tree is a very nice decoration for your house. Unfortunately it passes off in some weeks! Do you want to buy one for %P gold?", Topic=81

	{{eq(TOPIC, 81), "yes", PREMIUM, ge(COUNTMONEY, PRICE)}, {"Here you are.", deleteMoney, create(TYPE)}},
	{{eq(TOPIC, 81), "yes", ge(COUNTMONEY, PRICE)}, {"I'm sorry, but you need a premium account if you want to buy furniture."}},
	{{eq(TOPIC, 81), "yes"}, {"Come back, when you have enough money."}},
	{{eq(TOPIC, 81)}, {"Hmm, but I'm sure, it would fit nicely into your house."}},
}
