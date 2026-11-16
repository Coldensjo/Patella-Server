-- npcs/captain6.lua: the behaviour of the NPC Petros, converted from
-- captain6.npc by patella-convert. The NPC is its [[npc]] entry in npcs.toml;
-- lib/npc.lua matches and runs the rules below, in the order of the file
-- (scripting.hh, "NPCs"). The comments are those of captain6.npc.
--
-- GIMUD - Graphical Interface Multi User Dungeon
-- petros.npc: Fährmann Petros bei Darashia

local behaviour = require("lib.npc")
local ADDRESS = behaviour.ADDRESS
local BUSY = behaviour.BUSY
local VANISH = behaviour.VANISH
local PREMIUM = behaviour.PREMIUM
local PZBLOCK = behaviour.PZBLOCK
local FORCE = behaviour.FORCE
local eq = behaviour.eq
local lt = behaviour.lt
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
	{{ADDRESS, "hello$", FORCE}, {"Greetings, %N. I can take you to Venore, Port Hope or Ankrahmun if you like."}},
	{{ADDRESS, "hi$", FORCE}, {previous}},
	{{ADDRESS, FORCE}, {idle}},
	{{BUSY, "hello$", FORCE}, {"One moment please, %N.", queue}},
	{{BUSY, "hi$", FORCE}, {previous}},
	{{BUSY, FORCE}, {nop}},
	{{VANISH, FORCE}, {"Good bye!"}},

	{{"bye"}, {"Good bye!", idle}},
	{{"farewell"}, {previous}},
	{{"name"}, {"My name is Petros."}},
	{{"job"}, {"I take along people to Venore, Port Hope and Ankrahmun."}},
	{{"ghost"}, {"Oh, I don't believe in ghosts."}},

	{{"ship"}, {"My boat is ready to bring you to Venore, Port Hope or Ankrahmun."}},
	{{"boat"}, {previous}},
	{{"passage"}, {previous}},
	{{"venore"}, {setPrice(60), "Do you want to get to Venore for %P gold?", setTopic(1)}},
	{{"ankrahmun"}, {setPrice(100), "Do you want to get to Ankrahmun for %P gold?", setTopic(2)}},
	{{"port", "hope"}, {setPrice(180), "Do you seek a passage to Port Hope for %P gold?", setTopic(8)}},

	{{"venore", gt(questValue(250), 2)}, {setPrice(50), "Do you want to get to Venore for %P gold?", setTopic(1)}},
	{{"ankrahmun", gt(questValue(250), 2)}, {setPrice(90), "Do you want to get to Ankrahmun for %P gold?", setTopic(2)}},
	{{"port", "hope", gt(questValue(250), 2)}, {setPrice(170), "Do you seek a passage to Port Hope for %P gold?", setTopic(8)}},

	{{gt(TOPIC, 0), lt(TOPIC, 9), "yes", PZBLOCK, FORCE}, {"First get rid of those blood stains! You are not going to ruin my vehicle!"}},

	--Topic=1,"yes",PZBlock,! -> "First get rid of those blood stains! You are not going to ruin my vehicle!"
	--Topic=2,"yes",PZBlock,! -> *

	{{eq(TOPIC, 1), "yes", ge(COUNTMONEY, PRICE)}, {"Here we go!", deleteMoney, idle, effectOpp(11), teleport(32954, 32023, 6), effectOpp(11)}},
	{{eq(TOPIC, 2), "yes", ge(COUNTMONEY, PRICE)}, {"Here we go!", deleteMoney, idle, effectOpp(11), teleport(33091, 32883, 6), effectOpp(11)}},
	{{eq(TOPIC, 8), "yes", PREMIUM, ge(COUNTMONEY, PRICE)}, {"Set the sails!", deleteMoney, idle, effectOpp(11), teleport(32530, 32784, 6), effectOpp(11)}},

	{{gt(TOPIC, 0), "yes"}, {"You don't have enough money."}},
	{{gt(TOPIC, 0)}, {"Maybe later."}},
}

function onCreatureSay(npc, creature, text, situation)
	behaviour.react(npc, rules, text, situation)
end

function onCreatureDisappear(npc, creature)
	behaviour.react(npc, rules, "", NPC_SITUATION_VANISH)
end
