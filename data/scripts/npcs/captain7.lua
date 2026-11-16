-- npcs/captain7.lua: the behaviour of the NPC Captain Sinbeard, converted from
-- captain7.npc by patella-convert. The NPC is its [[npc]] entry in npcs.toml;
-- lib/npc.lua matches and runs the rules below, in the order of the file
-- (scripting.hh, "NPCs"). The comments are those of captain7.npc.
--
-- GIMUD - Graphical Interface Multi User Dungeon
-- sinbeard.npc: Kapitän Sinbeard in Ankrahmun

local behaviour = require("lib.npc")
local ADDRESS = behaviour.ADDRESS
local BUSY = behaviour.BUSY
local VANISH = behaviour.VANISH
local MALE = behaviour.MALE
local FEMALE = behaviour.FEMALE
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
	{{ADDRESS, "hello$", MALE, FORCE}, {"Welcome on board, Sir %N."}},
	{{ADDRESS, "hi$", MALE, FORCE}, {previous}},
	{{ADDRESS, "hello$", FEMALE, FORCE}, {"Welcome on board, Madam %N."}},
	{{ADDRESS, "hi$", FEMALE, FORCE}, {previous}},
	{{ADDRESS, FORCE}, {idle}},
	{{BUSY, "hello$", FORCE}, {"One moment please %N. You're next in line.", queue}},
	{{BUSY, "hi$", FORCE}, {previous}},
	{{BUSY, FORCE}, {nop}},
	{{VANISH, FORCE}, {"Good bye. Recommend us, if you were satisfied with our service."}},

	{{"bye"}, {"Good bye. Recommend us, if you were satisfied with our service.", idle}},
	{{"farewell"}, {previous}},
	{{"name"}, {"I am known all over the world as Captain Sinbeard."}},
	{{"job"}, {"I am the captain of this sailing-ship."}},
	{{"captain"}, {previous}},
	{{"ship"}, {"My ship is the fastest in the whole world."}},
	{{"line"}, {previous}},
	{{"company"}, {previous}},
	{{"tibia"}, {previous}},
	{{"good"}, {"We can transport everything you want."}},
	{{"passenger"}, {"We would like to welcome you on board."}},
	{{"trip"}, {"Where do you want to go? To Darashia, Venore, Port Hope or Edron?"}},
	{{"route"}, {previous}},
	{{"passage"}, {previous}},
	{{"town"}, {previous}},
	{{"destination"}, {previous}},
	{{"sail"}, {previous}},
	{{"go"}, {previous}},
	{{"thais"}, {"I'm sorry but my ship does not currently service that port."}},
	{{"carlin"}, {previous}},
	{{"ab'dendriel"}, {previous}},

	{{"darashia"}, {setPrice(100), "Do you seek a passage to Darashia for %P gold?", setTopic(2)}},
	{{"edron"}, {setPrice(160), "Do you seek a passage to Edron for %P gold?", setTopic(4)}},
	{{"venore"}, {setPrice(150), "Do you seek a passage to Venore for %P gold?", setTopic(5)}},
	{{"port", "hope"}, {setPrice(80), "Do you seek a passage to Port Hope for %P gold?", setTopic(8)}},

	{{"darashia", gt(questValue(250), 2)}, {setPrice(90), "Do you seek a passage to Darashia for %P gold?", setTopic(2)}},
	{{"edron", gt(questValue(250), 2)}, {setPrice(150), "Do you seek a passage to Edron for %P gold?", setTopic(4)}},
	{{"venore", gt(questValue(250), 2)}, {setPrice(140), "Do you seek a passage to Venore for %P gold?", setTopic(5)}},
	{{"port", "hope", gt(questValue(250), 2)}, {setPrice(70), "Do you seek a passage to Port Hope for %P gold?", setTopic(8)}},

	{{gt(TOPIC, 0), lt(TOPIC, 9), "yes", PZBLOCK, FORCE}, {"First get rid of those blood stains! You are not going to ruin my vehicle!"}},

	--Topic=2,"yes",PZBlock,! -> "First get rid of those blood stains! You are not going to ruin my vehicle!"
	--Topic=4,"yes",PZBlock,! -> *
	--Topic=5,"yes",PZBlock,! -> *

	{{eq(TOPIC, 2), "yes", PREMIUM, ge(COUNTMONEY, PRICE)}, {"Set the sails!", deleteMoney, idle, effectOpp(11), teleport(33290, 32481, 6), effectOpp(11)}},
	{{eq(TOPIC, 4), "yes", PREMIUM, ge(COUNTMONEY, PRICE)}, {"Set the sails!", deleteMoney, idle, effectOpp(11), teleport(33175, 31764, 6), effectOpp(11)}},
	{{eq(TOPIC, 5), "yes", PREMIUM, ge(COUNTMONEY, PRICE)}, {"Set the sails!", deleteMoney, idle, effectOpp(11), teleport(32954, 32023, 6), effectOpp(11)}},
	{{eq(TOPIC, 8), "yes", PREMIUM, ge(COUNTMONEY, PRICE)}, {"Set the sails!", deleteMoney, idle, effectOpp(11), teleport(32530, 32784, 6), effectOpp(11)}},

	{{gt(TOPIC, 0), "yes", ge(COUNTMONEY, PRICE)}, {"I'm sorry, but you need a premium account in order to travel onboard our ships."}},
	{{gt(TOPIC, 0), "yes"}, {"You don't have enough money."}},
	{{gt(TOPIC, 0)}, {"We would like to serve you some time."}},
}

function onCreatureSay(npc, creature, text, situation)
	behaviour.react(npc, rules, text, situation)
end

function onCreatureDisappear(npc, creature)
	behaviour.react(npc, rules, "", NPC_SITUATION_VANISH)
end
