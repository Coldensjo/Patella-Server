-- npcs/captain5.lua: the behaviour of the NPC Captain Fearless, converted from
-- captain5.npc by patella-convert. The NPC is its [[npc]] entry in npcs.toml;
-- lib/npc.lua matches and runs the rules below, in the order of the file
-- (scripting.hh, "NPCs"). The comments are those of captain5.npc.
--
-- GIMUD - Graphical Interface Multi User Dungeon
-- captain5.npc: Kapitän Fearless in Venore

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
local random = behaviour.random
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
	{{BUSY, "hello$", FORCE}, {"One moment please %N. You're next in line", queue}},
	{{BUSY, "hi$", FORCE}, {previous}},
	{{BUSY, FORCE}, {nop}},
	{{VANISH, FORCE}, {"Good bye. Recommend us, if you were satisfied with our service."}},

	{{"bye"}, {"Good bye. Recommend us, if you were satisfied with our service.", idle}},
	{{"farewell"}, {previous}},
	{{"name"}, {"My name is Captain Fearless from the Royal Tibia Line."}},
	{{"job"}, {"I am the captain of this sailing-ship."}},
	{{"captain"}, {previous}},
	{{"ship"}, {"The Royal Tibia Line connects all seaside towns of Tibia."}},
	{{"line"}, {previous}},
	{{"company"}, {previous}},
	{{"route"}, {previous}},
	{{"tibia"}, {previous}},
	{{"good"}, {"We can transport everything you want."}},
	{{"passenger"}, {"We would like to welcome you on board."}},
	{{"trip"}, {"Where do you want to go? To Thais, Carlin, Ab'Dendriel, Port Hope, Edron, Darashia or Ankrahmun?"}},
	{{"passage"}, {previous}},
	{{"town"}, {previous}},
	{{"destination"}, {previous}},
	{{"sail"}, {previous}},
	{{"go"}, {previous}},
	{{"ice"}, {"I'm sorry, but we don't serve the routes to the Ice Islands."}},
	{{"senja"}, {previous}},
	{{"folda"}, {previous}},
	{{"vega"}, {previous}},
	{{"ghost"}, {"There's a legend of a ghostship cruising between Venore and Darashia. Many captains are afraid to sail this route. Hah, but not me!"}},

	{{"thais"}, {setPrice(170), "Do you seek a passage to Thais for %P gold?", setTopic(1)}},
	{{"carlin"}, {setPrice(130), "Do you seek a passage to Carlin for %P gold?", setTopic(2)}},
	{{"ab'dendriel"}, {setPrice(90), "Do you seek a passage to Ab'Dendriel for %P gold?", setTopic(3)}},
	{{"venore"}, {"This is Venore. Where do you want to go?"}},
	{{"darashia"}, {setPrice(60), "Do you seek a passage to Darashia for %P gold?", setTopic(5)}},
	{{"edron"}, {setPrice(40), "Do you seek a passage to Edron for %P gold?", setTopic(4)}},
	{{"ankrahmun"}, {setPrice(150), "Do you seek a passage to Ankrahmun for %P gold?", setTopic(7)}},
	{{"port", "hope"}, {setPrice(160), "Do you seek a passage to Port Hope for %P gold?", setTopic(8)}},

	{{"thais", gt(questValue(250), 2)}, {setPrice(160), "Do you seek a passage to Thais for %P gold?", setTopic(1)}},
	{{"carlin", gt(questValue(250), 2)}, {setPrice(120), "Do you seek a passage to Carlin for %P gold?", setTopic(2)}},
	{{"ab'dendriel", gt(questValue(250), 2)}, {setPrice(80), "Do you seek a passage to Ab'Dendriel for %P gold?", setTopic(3)}},
	{{"darashia", gt(questValue(250), 2)}, {setPrice(50), "Do you seek a passage to Darashia for %P gold?", setTopic(5)}},
	{{"edron", gt(questValue(250), 2)}, {setPrice(30), "Do you seek a passage to Edron for %P gold?", setTopic(4)}},
	{{"ankrahmun", gt(questValue(250), 2)}, {setPrice(140), "Do you seek a passage to Ankrahmun for %P gold?", setTopic(7)}},
	{{"port", "hope", gt(questValue(250), 2)}, {setPrice(150), "Do you seek a passage to Port Hope for %P gold?", setTopic(8)}},

	{{gt(TOPIC, 0), lt(TOPIC, 9), "yes", PZBLOCK, FORCE}, {"First get rid of those blood stains! You are not going to ruin my vehicle!"}},

	--Topic=1,"yes",PZBlock,! -> "First get rid of those blood stains! You are not going to ruin my vehicle!"
	--Topic=2,"yes",PZBlock,! -> *
	--Topic=3,"yes",PZBlock,! -> *
	--Topic=4,"yes",PZBlock,! -> *
	--Topic=5,"yes",PZBlock,! -> *
	--Topic=7,"yes",PZBlock,! -> *

	{{eq(TOPIC, 1), "yes", PREMIUM, ge(COUNTMONEY, PRICE)}, {"Set the sails!", deleteMoney, idle, effectOpp(11), teleport(32312, 32211, 6), effectOpp(11)}},
	{{eq(TOPIC, 2), "yes", PREMIUM, ge(COUNTMONEY, PRICE)}, {"Set the sails!", deleteMoney, idle, effectOpp(11), teleport(32387, 31821, 6), effectOpp(11)}},
	{{eq(TOPIC, 3), "yes", PREMIUM, ge(COUNTMONEY, PRICE)}, {"Set the sails!", deleteMoney, idle, effectOpp(11), teleport(32733, 31668, 6), effectOpp(11)}},
	{{eq(TOPIC, 4), "yes", PREMIUM, ge(COUNTMONEY, PRICE)}, {"Set the sails!", deleteMoney, idle, effectOpp(11), teleport(33175, 31764, 6), effectOpp(11)}},

	{{eq(TOPIC, 5), "yes", PREMIUM, ge(COUNTMONEY, PRICE)}, {"I warn you! This route is haunted by a ghostship. Do you really want to go there?", setTopic(6)}},
	{{eq(TOPIC, 6), "yes", PREMIUM, ge(COUNTMONEY, PRICE), eq(random(1, 10), 1)}, {"Set the sails!", deleteMoney, idle, effectOpp(11), teleport(33330, 32172, 5), effectOpp(11)}},
	{{eq(TOPIC, 6), "yes", PREMIUM, ge(COUNTMONEY, PRICE)}, {"Set the sails!", deleteMoney, idle, effectOpp(11), teleport(33290, 32481, 6), effectOpp(11)}},

	{{eq(TOPIC, 7), "yes", PREMIUM, ge(COUNTMONEY, PRICE)}, {"Set the sails!", deleteMoney, idle, effectOpp(11), teleport(33091, 32883, 6), effectOpp(11)}},
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
