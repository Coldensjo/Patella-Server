-- npcs/luna.lua: the behaviour of the NPC Luna, converted from
-- luna.npc by patella-convert. The NPC is its [[npc]] entry in npcs.toml;
-- lib/npc.lua matches and runs the rules below, in the order of the file
-- (scripting.hh, "NPCs"). The comments are those of luna.npc.
--
-- GIMUD - Graphical Interface Multi User Dungeon
-- luna.npc: Datenbank für die Kräuterhändlerin Luna

local behaviour = require("lib.npc")
local ADDRESS = behaviour.ADDRESS
local BUSY = behaviour.BUSY
local VANISH = behaviour.VANISH
local FORCE = behaviour.FORCE
local P1 = behaviour.P1
local eq = behaviour.eq
local lt = behaviour.lt
local ge = behaviour.ge
local TOPIC = behaviour.TOPIC
local PRICE = behaviour.PRICE
local TYPE = behaviour.TYPE
local COUNTMONEY = behaviour.COUNTMONEY
local mul = behaviour.mul
local setTopic = behaviour.setTopic
local setPrice = behaviour.setPrice
local setAmount = behaviour.setAmount
local setType = behaviour.setType
local create = behaviour.create
local deleteMoney = behaviour.deleteMoney
local idle = behaviour.idle
local nop = behaviour.nop
local previous = behaviour.previous

local rules = behaviour.rules {
	{{ADDRESS, "hello$", FORCE}, {"Greetings, traveller."}},
	{{ADDRESS, "hi$", FORCE}, {previous}},
	{{ADDRESS, FORCE}, {idle}},
	{{BUSY, "hello$", FORCE}, {"Sorry, I'm too busy now."}},
	{{BUSY, "hi$", FORCE}, {previous}},
	{{BUSY, FORCE}, {nop}},
	{{VANISH, FORCE}, {"Goodbye, traveller."}},

	{{"bye"}, {"Goodbye, traveller.", idle}},
	{{"name"}, {"I'm called Luna."}},
	{{"job"}, {"I sell various herbs, mushrooms, and flowers."}},
	{{"time"}, {"Sorry, I don't know."}},
	{{"king"}, {"I don't know much about the king, sorry."}},
	{{"tibianus"}, {previous}},
	{{"army"}, {"I sometimes heal soldiers with my herbal mixtures."}},
	{{"heal"}, {previous}},
	{{"ferumbras"}, {"Mentioning his name makes me shiver."}},
	{{"excalibug"}, {"I am not an expert for weapons."}},
	{{"thais"}, {"I prefer the wilderness to cities."}},
	{{"tibia"}, {previous}},
	{{"carlin"}, {previous}},
	{{"edron"}, {previous}},
	{{"news"}, {"I fear I know nothing new that is of any importance."}},
	{{"rumors"}, {previous}},

	{{"offer"}, {"I'm selling various herbs, mushrooms, and flowers."}},
	{{"goods"}, {previous}},
	{{"do", "you", "sell"}, {previous}},
	{{"do", "you", "have"}, {previous}},
	{{"herbs"}, {"I have stone herbs, star herbs, and ferns. What do you want?"}},
	{{"mushroom"}, {"I have white, red, and brown mushrooms. Which one do you want?"}},
	{{"flowers"}, {"I have red roses and tulips. What do you want?"}},

	{{"white", "mushroom"}, {setType(3723), setAmount(1), setPrice(6), "Do you want to buy one of the white mushrooms for %P gold?", setTopic(1)}},
	{{"red", "mushroom"}, {setType(3724), setAmount(1), setPrice(12), "Do you want to buy one of the red mushrooms for %P gold?", setTopic(1)}},
	{{"brown", "mushroom"}, {setType(3725), setAmount(1), setPrice(10), "Do you want to buy one of the brown mushrooms for %P gold?", setTopic(1)}},

	{{P1, lt(1, P1), "white", "mushroom"}, {setType(3723), setAmount(P1), setPrice(mul(6, P1)), "Do you want to buy %A of the white mushrooms for %P gold?", setTopic(1)}},
	{{P1, lt(1, P1), "red", "mushroom"}, {setType(3724), setAmount(P1), setPrice(mul(12, P1)), "Do you want to buy %A of the red mushrooms for %P gold?", setTopic(1)}},
	{{P1, lt(1, P1), "brown", "mushroom"}, {setType(3725), setAmount(P1), setPrice(mul(10, P1)), "Do you want to buy %A of the brown mushrooms for %P gold?", setTopic(1)}},

	{{"rose"}, {setType(3658), setAmount(1), setPrice(11), "Do you want to buy a red rose for %P gold?", setTopic(1)}},
	{{"tulip"}, {setType(3668), setAmount(1), setPrice(9), "Do you want to buy a tulip for %P gold?", setTopic(1)}},
	{{"stone", "herb"}, {setType(3735), setAmount(1), setPrice(28), "Do you want to buy a stone herb for %P gold?", setTopic(1)}},
	{{"star", "herb"}, {setType(3736), setAmount(1), setPrice(21), "Do you want to buy a star herb for %P gold?", setTopic(1)}},
	{{"fern"}, {setType(3737), setAmount(1), setPrice(24), "Do you want to buy a fern for %P gold?", setTopic(1)}},

	{{eq(TOPIC, 1), "yes", ge(COUNTMONEY, PRICE)}, {"Here you are.", deleteMoney, create(TYPE)}},
	{{eq(TOPIC, 1), "yes"}, {"Come back, when you have enough money."}},
	{{eq(TOPIC, 1)}, {"Hmm, but next time."}},
}

function onCreatureSay(npc, creature, text, situation)
	behaviour.react(npc, rules, text, situation)
end

function onCreatureDisappear(npc, creature)
	behaviour.react(npc, rules, "", NPC_SITUATION_VANISH)
end
