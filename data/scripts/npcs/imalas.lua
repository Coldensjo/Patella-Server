-- npcs/imalas.lua: the behaviour of the NPC Imalas, converted from
-- imalas.npc by patella-convert. The NPC is its [[npc]] entry in npcs.toml;
-- lib/npc.lua matches and runs the rules below, in the order of the file
-- (scripting.hh, "NPCs"). The comments are those of imalas.npc.
--
-- GIMUD - Graphical Interface Multi User Dungeon
-- imalas.npc: Datenbank für den Nahrungsmittelhändler Imalas

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
	{{ADDRESS, "hello$", FORCE}, {"Hello %N! What do you need?"}},
	{{ADDRESS, "hi$", FORCE}, {previous}},
	{{ADDRESS, FORCE}, {idle}},
	{{BUSY, "hello$", FORCE}, {"Sorry %N, I am already talking to a customer. Please wait.", queue}},
	{{BUSY, "hi$", FORCE}, {previous}},
	{{BUSY, FORCE}, {nop}},
	{{VANISH, FORCE}, {"Good bye."}},

	{{"bye"}, {"Good bye.", idle}},
	{{"farewell"}, {previous}},
	{{"job"}, {"I'm a shopkeeper. You can buy food here."}},
	{{"name"}, {"My name is Imalas."}},
	{{"time"}, {"Sorry, I have no watch."}},
	{{"ghostlands"}, {"Sorry I know nothing more then it has to be a horrible place and that scares me enough."}},

	{{"offer"}, {"Just have a look at my blackboard."}},
	{{"goods"}, {previous}},
	{{"do", "you", "sell"}, {previous}},
	{{"do", "you", "have"}, {previous}},

	{{"banana"}, {setType(3587), setAmount(1), setPrice(2), "Do you want to buy a banana for %P gold?", setTopic(1)}},
	{{"cherry"}, {setType(3590), setAmount(1), setPrice(1), "Do you want to buy a cherry for %P gold?", setTopic(1)}},
	{{"grapes"}, {setType(3592), setAmount(1), setPrice(3), "Do you want to buy grapes for %P gold?", setTopic(1)}},
	{{"melon"}, {setType(3593), setAmount(1), setPrice(8), "Do you want to buy a melon for %P gold?", setTopic(1)}},
	{{"pumpkin"}, {setType(3594), setAmount(1), setPrice(10), "Do you want to buy a pumpkin for %P gold?", setTopic(1)}},
	{{"carrot"}, {setType(3595), setAmount(1), setPrice(2), "Do you want to buy a carrot for %P gold?", setTopic(1)}},
	{{"cookie"}, {setType(3598), setAmount(1), setPrice(2), "Do you want to buy a cookie for %P gold?", setTopic(1)}},
	{{"roll"}, {setType(3601), setAmount(1), setPrice(2), "Do you want to buy a roll for %P gold?", setTopic(1)}},
	{{"brown", "bread"}, {setType(3602), setAmount(1), setPrice(3), "Do you want to buy a brown bread for %P gold?", setTopic(1)}},
	{{"egg"}, {setType(3606), setAmount(1), setPrice(2), "Do you want to buy an egg for %P gold?", setTopic(1)}},
	{{"cheese"}, {setType(3607), setAmount(1), setPrice(5), "Do you want to buy cheese for %P gold?", setTopic(1)}},

	{{P1, lt(1, P1), "banana"}, {setType(3587), setAmount(P1), setPrice(mul(2, P1)), "Do you want to buy %A bananas for %P gold?", setTopic(1)}},
	{{P1, lt(1, P1), "cherr"}, {setType(3590), setAmount(P1), setPrice(mul(1, P1)), "Do you want to buy %A cherries for %P gold?", setTopic(1)}},
	{{P1, lt(1, P1), "grapes"}, {setType(3592), setAmount(P1), setPrice(mul(3, P1)), "Do you want to buy %A grapes for %P gold?", setTopic(1)}},
	{{P1, lt(1, P1), "melon"}, {setType(3593), setAmount(P1), setPrice(mul(8, P1)), "Do you want to buy %A melons for %P gold?", setTopic(1)}},
	{{P1, lt(1, P1), "pumpkin"}, {setType(3594), setAmount(P1), setPrice(mul(10, P1)), "Do you want to buy %A pumpkins for %P gold?", setTopic(1)}},
	{{P1, lt(1, P1), "carrot"}, {setType(3595), setAmount(P1), setPrice(mul(2, P1)), "Do you want to buy %A carrots for %P gold?", setTopic(1)}},
	{{P1, lt(1, P1), "cookie"}, {setType(3598), setAmount(P1), setPrice(mul(2, P1)), "Do you want to buy %A cookies for %P gold?", setTopic(1)}},
	{{P1, lt(1, P1), "roll"}, {setType(3601), setAmount(P1), setPrice(mul(2, P1)), "Do you want to buy %A rolls for %P gold?", setTopic(1)}},
	{{P1, lt(1, P1), "brown", "bread"}, {setType(3602), setAmount(P1), setPrice(mul(3, P1)), "Do you want to buy %A brown breads for %P gold?", setTopic(1)}},
	{{P1, lt(1, P1), "egg"}, {setType(3606), setAmount(P1), setPrice(mul(2, P1)), "Do you want to buy %A eggs for %P gold?", setTopic(1)}},
	{{P1, lt(1, P1), "cheese"}, {setType(3607), setAmount(P1), setPrice(mul(5, P1)), "Do you want to buy %A cheese for %P gold?", setTopic(1)}},

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
