-- lib/npcs/gen-t-gems-s.lua: the rule block gen-t-gems-s.ndb, converted by patella-convert.
-- An NPC that included it (@"gen-t-gems-s.ndb") has it in its rules where the include
-- was; lib/npc.lua matches and runs them (scripting.hh, "NPCs"). The comments
-- are those of gen-t-gems-s.ndb.
--
-- GIMUD - Graphical Interface Multi User Dungeon
-- gen-t-gems-s.ndb: Datenbank für generischen Edelsteinverkauf

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
local previous = behaviour.previous

return behaviour.rules {
	{{"offer"}, {"I can offer you various gems, pearls or some wonderful jewels."}},
	{{"goods"}, {previous}},
	{{"do", "you", "sell"}, {previous}},
	{{"do", "you", "have"}, {previous}},
	{{"gem"}, {"You can buy and sell small diamonds, sapphires, rubies, emeralds, and amethysts."}},
	{{"pearl"}, {"There are white and black pearls you can buy or sell."}},
	{{"jewel"}, {"Currently you can purchase wedding rings, golden amulets, and ruby necklaces."}},

	{{"wedding", "ring"}, {setType(3004), setAmount(1), setPrice(990), "Do you want to buy a wedding ring for %P gold?", setTopic(29)}},
	{{"golden", "amulet"}, {setType(3013), setAmount(1), setPrice(6600), "Do you want to buy a golden amulet for %P gold?", setTopic(29)}},
	{{"ruby", "necklace"}, {setType(3016), setAmount(1), setPrice(3560), "Do you want to buy a ruby necklace for %P gold?", setTopic(29)}},
	{{"white", "pearl"}, {setType(3026), setAmount(1), setPrice(320), "Do you want to buy a white pearl for %P gold?", setTopic(29)}},
	{{"black", "pearl"}, {setType(3027), setAmount(1), setPrice(560), "Do you want to buy a black pearl for %P gold?", setTopic(29)}},
	{{"small", "diamond"}, {setType(3028), setAmount(1), setPrice(600), "Do you want to buy a small diamond for %P gold?", setTopic(29)}},
	{{"small", "sapphire"}, {setType(3029), setAmount(1), setPrice(500), "Do you want to buy a small sapphire for %P gold?", setTopic(29)}},
	{{"small", "ruby"}, {setType(3030), setAmount(1), setPrice(500), "Do you want to buy a small ruby for %P gold?", setTopic(29)}},
	{{"small", "emerald"}, {setType(3032), setAmount(1), setPrice(500), "Do you want to buy a small emerald for %P gold?", setTopic(29)}},
	{{"small", "amethyst"}, {setType(3033), setAmount(1), setPrice(400), "Do you want to buy a small amethyst for %P gold?", setTopic(29)}},

	{{eq(TOPIC, 29), "yes", ge(COUNTMONEY, PRICE)}, {"Here you are.", deleteMoney, create(TYPE)}},
	{{eq(TOPIC, 29), "yes"}, {"Come back, when you have enough money."}},
	{{eq(TOPIC, 29)}, {"Hmm, but next time."}},
}
