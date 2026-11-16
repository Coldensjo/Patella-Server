-- npcs/quero.lua: the behaviour of the NPC Quero, converted from
-- quero.npc by patella-convert. The NPC is its [[npc]] entry in npcs.toml;
-- lib/npc.lua matches and runs the rules below, in the order of the file
-- (scripting.hh, "NPCs"). The comments are those of quero.npc.
--
-- GIMUD - Graphical Interface Multi User Dungeon
-- quero.npc: Datenbank für den Barden Quero

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
	{{ADDRESS, "hello$", FORCE}, {"Welcome %N! Can I help you?"}},
	{{ADDRESS, "hi$", FORCE}, {previous}},
	{{ADDRESS, FORCE}, {idle}},
	{{BUSY, "hello$", FORCE}, {"Sorry, I am already talking to someone, %N.", queue}},
	{{BUSY, "hi$", FORCE}, {previous}},
	{{BUSY, FORCE}, {nop}},
	{{VANISH, FORCE}, {"Good bye."}},

	{{"bye"}, {"Good bye.", idle}},
	{{"farewell"}, {previous}},
	{{"job"}, {"I make instruments and sometimes I'm wandering through the lands of Tibia as a bard."}},
	{{"name"}, {"My name is Quero."}},
	{{"time"}, {"Sorry, I don't know what time it is."}},

	{{"music"}, {"I love the music of the elves."}},
	{{"elf"}, {"They live in the northeast of Tibia."}},
	{{"elves"}, {previous}},
	{{"bard"}, {"Selling instruments isn't enough to live on and I love music. That's why I wander through the lands from time to time."}},

	{{"benjamin"}, {"He's nice."}},

	{{"offer"}, {"You can buy a lyre, lute, drum, and simple fanfare."}},
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
	{{eq(TOPIC, 1), "yes"}, {"Come back, when you have enough money."}},
	{{eq(TOPIC, 1)}, {"Hmm, but next time."}},
}

function onCreatureSay(npc, creature, text, situation)
	behaviour.react(npc, rules, text, situation)
end

function onCreatureDisappear(npc, creature)
	behaviour.react(npc, rules, "", NPC_SITUATION_VANISH)
end
