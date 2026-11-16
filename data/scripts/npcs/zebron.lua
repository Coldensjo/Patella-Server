-- npcs/zebron.lua: the behaviour of the NPC Zebron, converted from
-- zebron.npc by patella-convert. The NPC is its [[npc]] entry in npcs.toml;
-- lib/npc.lua matches and runs the rules below, in the order of the file
-- (scripting.hh, "NPCs"). The comments are those of zebron.npc.
--
-- GIMUD - Graphical Interface Multi User Dungeon
-- zebron.npc: Datenbank für den Spieler Zebron

local behaviour = require("lib.npc")
local ADDRESS = behaviour.ADDRESS
local BUSY = behaviour.BUSY
local VANISH = behaviour.VANISH
local FORCE = behaviour.FORCE
local P1 = behaviour.P1
local eq = behaviour.eq
local lt = behaviour.lt
local gt = behaviour.gt
local ge = behaviour.ge
local TOPIC = behaviour.TOPIC
local AMOUNT = behaviour.AMOUNT
local COUNTMONEY = behaviour.COUNTMONEY
local random = behaviour.random
local mul = behaviour.mul
local setTopic = behaviour.setTopic
local setPrice = behaviour.setPrice
local setAmount = behaviour.setAmount
local createMoney = behaviour.createMoney
local deleteMoney = behaviour.deleteMoney
local queue = behaviour.queue
local idle = behaviour.idle
local nop = behaviour.nop
local previous = behaviour.previous

local rules = behaviour.rules {
	{{ADDRESS, "hello$", FORCE}, {"Greetings, high roller. So you care for a game, %N?", setTopic(1)}},
	{{ADDRESS, "hi$", FORCE}, {previous}},
	{{ADDRESS, FORCE}, {idle}},
	{{BUSY, "hello$", FORCE}, {"Take a minute to count your money, %N. I'll be here soon.", queue}},
	{{BUSY, "hi$", FORCE}, {previous}},
	{{BUSY, FORCE}, {nop}},
	{{VANISH, FORCE}, {"Hey, you can't leave. Luck is smiling on you. I can feel it!"}},

	{{"bye"}, {"Hey, you can't leave. Luck is smiling on you. I can feel it!", idle}},
	{{"farewell"}, {previous}},
	{{"job"}, {"Oh, I am just sitting around here and gamble now and then."}},
	{{"tavern"}, {"It's a fine place to be around, isn't it?"}},
	{{"name"}, {"I am known as Zebron."}},
	{{"time"}, {"It is exactly %T."}},
	{{"king"}, {"Ah, our beloved king! Bless him for the gambling licence of Venore."}},
	{{"tibianus"}, {previous}},
	{{"licence"}, {"I don't care much for that law stuff, but as far as I know those Venore merchants got a royal gambling licence for the city."}},
	{{"venore"}, {"Aaaah, Venore, Venore, what a wonderful town. Especially for someone with love for gambling like me."}},
	{{"army"}, {"Hehe, they spent a good part of their salary here in the tavern."}},
	{{"excalibug"}, {"I would not bet that anyone will ever find it."}},
	{{"thais"}, {"Thais is a bit too conservative for me."}},
	{{"tibia"}, {"What would I need more than that what I can get right here?"}},
	{{"carlin"}, {"Carlin, the beerless ... what a shame."}},
	{{"hugo"}, {"I had a cousin named hugo, why do you ask?"}},
	{{"news"}, {"Bah, always the same chitchat. Swampelves this and amazons that ... blah blah."}},
	{{"rumors"}, {previous}},
	{{"swamp"}, {previous}},
	{{"amazon"}, {previous}},

	{{"gambl"}, {"So you care for a civilized game of dice?", setTopic(1)}},
	{{"game"}, {previous}},
	{{"dice"}, {previous}},

	{{eq(TOPIC, 1), "yes"}, {"I will roll a dice. If it shows 6, you will get five times your bet. How much do you want to bet?", setAmount(random(1, 6)), setTopic(2)}},
	{{eq(TOPIC, 1), "no"}, {"Oh come on, don't be a child."}},

	{{eq(TOPIC, 2), P1, lt(0, P1), gt(100, P1), ge(COUNTMONEY, P1), eq(AMOUNT, 6)}, {setPrice(mul(P1, 5)), "Ok, here we go ... 6! You have won %P, congratulations. One more game?", createMoney, setTopic(1)}},
	{{eq(TOPIC, 2), P1, lt(0, P1), gt(100, P1), ge(COUNTMONEY, P1)}, {setPrice(P1), "Ok, here we go ... %A! You have lost. Bad luck. One more game?", deleteMoney, setTopic(1)}},
	{{eq(TOPIC, 2), P1, lt(0, P1), gt(100, P1)}, {"I am sorry, but you don't have so much money. How much do you want to bet?", setTopic(2)}},
	{{eq(TOPIC, 2), P1}, {"I am sorry, but I accept only bets between 1 and 99 gold. I don't want to ruin you after all. How much do you want to bet?", setTopic(2)}},
}

function onCreatureSay(npc, creature, text, situation)
	behaviour.react(npc, rules, text, situation)
end

function onCreatureDisappear(npc, creature)
	behaviour.react(npc, rules, "", NPC_SITUATION_VANISH)
end
