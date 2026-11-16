-- npcs/pino.lua: the behaviour of the NPC Pino, converted from
-- pino.npc by patella-convert. The NPC is its [[npc]] entry in npcs.toml;
-- lib/npc.lua matches and runs the rules below, in the order of the file
-- (scripting.hh, "NPCs"). The comments are those of pino.npc.
--
-- GIMUD - Graphical Interface Multi User Dungeon
-- pino: Datenbank für den Teppichpiloten Pino in Edron

local behaviour = require("lib.npc")
local ADDRESS = behaviour.ADDRESS
local BUSY = behaviour.BUSY
local VANISH = behaviour.VANISH
local PZBLOCK = behaviour.PZBLOCK
local FORCE = behaviour.FORCE
local eq = behaviour.eq
local gt = behaviour.gt
local ge = behaviour.ge
local TOPIC = behaviour.TOPIC
local PRICE = behaviour.PRICE
local COUNTMONEY = behaviour.COUNTMONEY
local questValue = behaviour.questValue
local setTopic = behaviour.setTopic
local setPrice = behaviour.setPrice
local effectOpp = behaviour.effectOpp
local deleteMoney = behaviour.deleteMoney
local queue = behaviour.queue
local idle = behaviour.idle
local nop = behaviour.nop
local teleport = behaviour.teleport
local previous = behaviour.previous

local rules = behaviour.rules {
	{{ADDRESS, "hello$", FORCE}, {"Greetings, traveller %N."}},
	{{ADDRESS, "hi$", FORCE}, {previous}},
	{{ADDRESS, FORCE}, {idle}},
	{{BUSY, "hello$", FORCE}, {"Please wait, %N.", queue}},
	{{BUSY, "hi$", FORCE}, {previous}},
	{{BUSY, FORCE}, {nop}},
	{{VANISH, FORCE}, {"Good bye!"}},

	{{"bye"}, {"Good bye!", idle}},
	{{"name"}, {"Pino at your service."}},
	{{"job"}, {"I am a carpetpilot. I can fly you to the Femor Hills or Darashia."}},
	{{"service"}, {previous}},
	{{"time"}, {"It's %T right now."}},
	{{"tibia"}, {"What a wonderful world. Especially if you look down on it."}},

	{{"passage"}, {"I can fly you to Darashia on Darama or to the Femor Hills if you like. Where do you want to go?"}},
	{{"transport"}, {previous}},
	{{"ride"}, {previous}},
	{{"trip"}, {previous}},

	{{"darashia"}, {setPrice(40), "Do you want to get a ride to Darashia on Darama for %P gold?", setTopic(1)}},
	{{"darama"}, {previous}},
	{{"hill"}, {setPrice(60), "Do you want to get a ride to the Femor Hills for %P gold?", setTopic(2)}},
	{{"femor"}, {previous}},
	{{"femur"}, {"You are probably talking about the FEMOR hills."}},

	{{"darashia", gt(questValue(250), 2)}, {setPrice(30), "Do you want to get a ride to Darashia on Darama for %P gold?", setTopic(1)}},
	{{"darama", gt(questValue(250), 2)}, {previous}},
	{{"hill", gt(questValue(250), 2)}, {setPrice(50), "Do you want to get a ride to the Femor Hills for %P gold?", setTopic(2)}},
	{{"femor", gt(questValue(250), 2)}, {previous}},

	{{eq(TOPIC, 1), "yes", PZBLOCK, FORCE}, {"First get rid of those blood stains! You are not going to ruin my vehicle!"}},
	{{eq(TOPIC, 2), "yes", PZBLOCK, FORCE}, {previous}},

	{{eq(TOPIC, 1), "yes", ge(COUNTMONEY, PRICE)}, {"Hold on!", deleteMoney, idle, effectOpp(11), teleport(33269, 32441, 6), effectOpp(11)}},
	{{eq(TOPIC, 1), "yes"}, {"You don't have enough money."}},
	{{eq(TOPIC, 1)}, {"You shouldn't miss the experience."}},

	{{eq(TOPIC, 2), "yes", ge(COUNTMONEY, PRICE)}, {"Hold on!", deleteMoney, idle, effectOpp(11), teleport(32535, 31837, 4), effectOpp(11)}},
	{{eq(TOPIC, 2), "yes"}, {"You don't have enough money."}},
	{{eq(TOPIC, 2)}, {"You shouldn't miss the experience."}},
}

function onCreatureSay(npc, creature, text, situation)
	behaviour.react(npc, rules, text, situation)
end

function onCreatureDisappear(npc, creature)
	behaviour.react(npc, rules, "", NPC_SITUATION_VANISH)
end
