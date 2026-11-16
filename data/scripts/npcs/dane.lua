-- npcs/dane.lua: the behaviour of the NPC Dane, converted from
-- dane.npc by patella-convert. The NPC is its [[npc]] entry in npcs.toml;
-- lib/npc.lua matches and runs the rules below, in the order of the file
-- (scripting.hh, "NPCs"). The comments are those of dane.npc.
--
-- GIMUD - Graphical Interface Multi User Dungeon
-- dane.npc: Datenbank für die Wirtin Dane

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
local setData = behaviour.setData
local create = behaviour.create
local deleteMoney = behaviour.deleteMoney
local queue = behaviour.queue
local idle = behaviour.idle
local nop = behaviour.nop
local previous = behaviour.previous

local rules = behaviour.rules {
	{{ADDRESS, "hello$", FORCE}, {"Welcome to the wave cellar, %N."}},
	{{ADDRESS, "hi$", FORCE}, {previous}},
	{{ADDRESS, FORCE}, {idle}},
	{{BUSY, "hello$", FORCE}, {"Just a minute, %N.", queue}},
	{{BUSY, "hi$", FORCE}, {previous}},
	{{BUSY, FORCE}, {nop}},
	{{VANISH, FORCE}, {"Please come back from time to time."}},

	{{"bye"}, {"Please come back from time to time.", idle}},
	{{"farewell"}, {previous}},
	{{"job"}, {"I am the owner of this place of relaxation."}},
	{{"saloon"}, {previous}},
	{{"cellar"}, {"It's pretty, isn't it?"}},
	{{"name"}, {"I am Dane."}},
	{{"time"}, {"It is exactly %T."}},
	{{"news"}, {"I heard nothing interesting lately."}},

	{{"offer"}, {"I can offer you milk, water, and lemonade."}},
	{{"do", "you", "sell"}, {previous}},
	{{"do", "you", "have"}, {previous}},
	{{"food"}, {"Sorry, we just sell drinks."}},
	{{"alcohol"}, {"Alcohol makes people too aggressive. We don't need such stuff in Carlin."}},
	{{"wine"}, {previous}},
	{{"beer"}, {previous}},

	{{"lemonade"}, {setType(2875), setData(12), setAmount(1), setPrice(5), "Do you want to buy a bottle of refreshing lemonade for %P gold?", setTopic(1)}},
	{{"milk"}, {setType(2875), setData(9), setAmount(1), setPrice(4), "Do you want to buy a bottle of our revitalizing milk for %P gold?", setTopic(1)}},
	{{"water"}, {setType(2875), setData(1), setAmount(1), setPrice(2), "Do you want to buy a bottle of crystal clear water for %P gold?", setTopic(1)}},

	{{P1, lt(1, P1), "lemonade"}, {setType(2875), setData(12), setAmount(P1), setPrice(mul(5, P1)), "Do you want to buy %A bottles of refreshing lemonade for %P gold?", setTopic(1)}},
	{{P1, lt(1, P1), "milk"}, {setType(2875), setData(9), setAmount(P1), setPrice(mul(4, P1)), "Do you want to buy %A bottles of our revitalizing milk for %P gold?", setTopic(1)}},
	{{P1, lt(1, P1), "water"}, {setType(2875), setData(1), setAmount(P1), setPrice(mul(2, P1)), "Do you want to buy %A bottles of crystal clear water for %P gold?", setTopic(1)}},

	{{eq(TOPIC, 1), "yes", ge(COUNTMONEY, PRICE)}, {"Here it is.", deleteMoney, create(TYPE)}},
	{{eq(TOPIC, 1), "yes"}, {"I am sorry, but you do not have enough gold."}},
	{{eq(TOPIC, 1)}, {"Maybe later."}},
}

function onCreatureSay(npc, creature, text, situation)
	behaviour.react(npc, rules, text, situation)
end

function onCreatureDisappear(npc, creature)
	behaviour.react(npc, rules, "", NPC_SITUATION_VANISH)
end
