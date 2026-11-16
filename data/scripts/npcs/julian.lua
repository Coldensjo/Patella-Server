-- npcs/julian.lua: the behaviour of the NPC Julian, converted from
-- julian.npc by patella-convert. The NPC is its [[npc]] entry in npcs.toml;
-- lib/npc.lua matches and runs the rules below, in the order of the file
-- (scripting.hh, "NPCs"). The comments are those of julian.npc.
--
-- GIMUD - Graphical Interface Multi User Dungeon
-- julian.npc: Datenbank für den Musikhändler Julian

local behaviour = require("lib.npc")
local genTFurnitureInstrumentsS = require("lib.npcs.gen-t-furniture-instruments-s")
local ADDRESS = behaviour.ADDRESS
local BUSY = behaviour.BUSY
local VANISH = behaviour.VANISH
local MALE = behaviour.MALE
local FEMALE = behaviour.FEMALE
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
	{{ADDRESS, "hello$", FORCE}, {"Be greeted, %N! May I help you?"}},
	{{ADDRESS, "hi$", FORCE}, {previous}},
	{{ADDRESS, FORCE}, {idle}},
	{{BUSY, "hello$", FORCE}, {"Sorry, I am busy right now, %N.", queue}},
	{{BUSY, "hi$", FORCE}, {previous}},
	{{BUSY, FORCE}, {nop}},
	{{VANISH, FORCE}, {"Good bye."}},

	{{"bye"}, {"Good bye.", idle}},
	{{"farewell"}, {previous}},
	{{"job"}, {"I make instruments and sometimes I'm wandering through the lands of Tibia as a bard."}},
	{{"name", MALE}, {"My name is Julian, sire."}},
	{{"name", FEMALE}, {"My name is Julian, my lady."}},
	{{"time"}, {"Sorry, I don't know what time it is."}},
	{{"music"}, {"Music is the food of love."}},
	{{"bard"}, {"Bards from all over the world come here to buy their instruments."}},

	{{"offer"}, {"Here you can buy lyres, lutes, drums, and simple fanfares. I also have a piano and a harp."}},
	{{"goods"}, {previous}},
	{{"do", "you", "sell"}, {previous}},
	{{"do", "you", "have"}, {previous}},
	{{"instrument"}, {previous}},

	{{"lyre"}, {setType(2949), setAmount(1), setPrice(120), "Do you want to buy a lyre for %P gold?", setTopic(1)}},
	{{"lute"}, {setType(2950), setAmount(1), setPrice(195), "Do you want to buy a lute for %P gold?", setTopic(1)}},
	{{"drum"}, {setType(2952), setAmount(1), setPrice(140), "Do you want to buy a drum for %P gold?", setTopic(1)}},
	{{"simple", "fanfare"}, {setType(2954), setAmount(1), setPrice(150), "Do you want to buy a simple fanfare for %P gold?", setTopic(1)}},

	{{P1, lt(1, P1), "lyre"}, {setType(2949), setAmount(P1), setPrice(mul(120, P1)), "Do you want to buy %A lyres for %P gold?", setTopic(1)}},
	{{P1, lt(1, P1), "lute"}, {setType(2950), setAmount(P1), setPrice(mul(195, P1)), "Do you want to buy %A lutes for %P gold?", setTopic(1)}},
	{{P1, lt(1, P1), "drum"}, {setType(2952), setAmount(P1), setPrice(mul(140, P1)), "Do you want to buy %A drums for %P gold?", setTopic(1)}},
	{{P1, lt(1, P1), "simple", "fanfare"}, {setType(2954), setAmount(P1), setPrice(mul(150, P1)), "Do you want to buy %A simple fanfares for %P gold?", setTopic(1)}},

	{{eq(TOPIC, 1), "yes", ge(COUNTMONEY, PRICE)}, {"Here you are.", deleteMoney, create(TYPE)}},
	{{eq(TOPIC, 1), "yes"}, {"You need some more money."}},
	{{eq(TOPIC, 1)}, {"Oh well, next time perhaps."}},

	genTFurnitureInstrumentsS,
}

function onCreatureSay(npc, creature, text, situation)
	behaviour.react(npc, rules, text, situation)
end

function onCreatureDisappear(npc, creature)
	behaviour.react(npc, rules, "", NPC_SITUATION_VANISH)
end
