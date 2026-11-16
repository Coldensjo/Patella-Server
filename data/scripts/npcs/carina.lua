-- npcs/carina.lua: the behaviour of the NPC Carina, converted from
-- carina.npc by patella-convert. The NPC is its [[npc]] entry in npcs.toml;
-- lib/npc.lua matches and runs the rules below, in the order of the file
-- (scripting.hh, "NPCs"). The comments are those of carina.npc.
--
-- GIMUD - Graphical Interface Multi User Dungeon
-- carina.npc: Datenbank für die Juwelierin Carina

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
local queue = behaviour.queue
local idle = behaviour.idle
local nop = behaviour.nop
local previous = behaviour.previous

local rules = behaviour.rules {
	{{ADDRESS, "hello$", FORCE}, {"Welcome, %N. I am looking forward to trade with you."}},
	{{ADDRESS, "hi$", FORCE}, {previous}},
	{{ADDRESS, FORCE}, {idle}},
	{{BUSY, "hello$", FORCE}, {"Please, %N, give me another minute with our other customer first.", queue}},
	{{BUSY, "hi$", FORCE}, {previous}},
	{{BUSY, FORCE}, {nop}},
	{{VANISH, FORCE}, {"Good bye."}},

	{{"bye"}, {"Good bye.", idle}},
	{{"farewell"}, {previous}},
	{{"job"}, {"I sell some of the most beautiful jewels of the lands."}},
	{{"name"}, {"I am Carina Carlson."}},
	{{"time"}, {"It's %T."}},
	{{"offer"}, {"I am selling jewels, just have a look."}},
	{{"goods"}, {previous}},
	{{"do", "you", "sell"}, {previous}},
	{{"do", "you", "have"}, {previous}},
	{{"jewel"}, {"We offer wedding rings, golden amulets, and ruby necklaces."}},

	{{"wedding", "ring"}, {setType(3004), setAmount(1), setPrice(990), "Do you want to buy a wedding ring for %P gold?", setTopic(1)}},
	{{"golden", "amulet"}, {setType(3013), setAmount(1), setPrice(6600), "Do you want to buy a golden amulet for %P gold?", setTopic(1)}},
	{{"ruby", "necklace"}, {setType(3016), setAmount(1), setPrice(3560), "Do you want to buy a ruby necklace for %P gold?", setTopic(1)}},

	{{P1, lt(1, P1), "wedding", "ring"}, {setType(3004), setAmount(P1), setPrice(mul(990, P1)), "Do you want to buy %A wedding rings for %P gold?", setTopic(1)}},
	{{P1, lt(1, P1), "golden", "amulet"}, {setType(3013), setAmount(P1), setPrice(mul(6600, P1)), "Do you want to buy %A golden amulets for %P gold?", setTopic(1)}},
	{{P1, lt(1, P1), "ruby", "necklace"}, {setType(3016), setAmount(P1), setPrice(mul(3560, P1)), "Do you want to buy %A ruby necklaces for %P gold?", setTopic(1)}},

	{{eq(TOPIC, 1), "yes", ge(COUNTMONEY, PRICE)}, {"Here you are.", deleteMoney, create(TYPE)}},
	{{eq(TOPIC, 1), "yes"}, {"Sorry, make sure to come back, as soon as you have enough money."}},
	{{eq(TOPIC, 1)}, {"Perhaps next time."}},
}

function onCreatureSay(npc, creature, text, situation)
	behaviour.react(npc, rules, text, situation)
end

function onCreatureDisappear(npc, creature)
	behaviour.react(npc, rules, "", NPC_SITUATION_VANISH)
end
