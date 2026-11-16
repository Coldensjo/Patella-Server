-- npcs/bruno.lua: the behaviour of the NPC Bruno, converted from
-- bruno.npc by patella-convert. The NPC is its [[npc]] entry in npcs.toml;
-- lib/npc.lua matches and runs the rules below, in the order of the file
-- (scripting.hh, "NPCs"). The comments are those of bruno.npc.
--
-- GIMUD - Graphical Interface Multi User Dungeon
-- bruno.npc: Der Fischverkäufer Bruno (Fields)

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
	{{ADDRESS, "hello$", FORCE}, {"Ahoi, %N. You want to buy some fresh fish?"}},
	{{ADDRESS, "hi$", FORCE}, {previous}},
	{{ADDRESS, FORCE}, {idle}},
	{{BUSY, "hello$", FORCE}, {"One moment please %N.", queue}},
	{{BUSY, "hi$", FORCE}, {previous}},
	{{BUSY, FORCE}, {nop}},
	{{VANISH, FORCE}, {"Good bye and come again!"}},

	{{"bye"}, {"Good bye and come again!", idle}},
	{{"farewell"}, {previous}},
	{{"name"}, {"My name is Bruno."}},
	{{"job"}, {"My job is to catch fish and to sell them here."}},
	{{"graubart"}, {"I like this old salt. I learned much from him. Whatever. You like some fish? *grin*"}},
	{{"marlene"}, {"Ah yes, my lovely wife. God forgive her, but she can't stop talking. So my work is a great rest for my poor ears. *laughs loudly*"}},
	{{"aneus"}, {"Hmm, I don't know him very well. But he has a very nice story to tell."}},

	{{"do", "you", "sell"}, {"Well, I sell freshly caught fish. You like some? Of course, you can buy more than one at once. *grin*"}},
	{{"offer"}, {previous}},

	{{"fish"}, {setType(3578), setAmount(1), setPrice(5), "Do you want to buy a fresh fish for %P gold?", setTopic(1)}},
	{{P1, lt(1, P1), "fish"}, {setType(3578), setAmount(P1), setPrice(mul(5, P1)), "Do you want to buy %A fresh fishes for %P gold?", setTopic(1)}},

	{{eq(TOPIC, 1), "yes", ge(COUNTMONEY, PRICE)}, {"Here you are.", deleteMoney, create(TYPE)}},
	{{eq(TOPIC, 1), "yes"}, {"Come back when you have enough money."}},
	{{eq(TOPIC, 1)}, {"*grumble* Maybe next time."}},
}

function onCreatureSay(npc, creature, text, situation)
	behaviour.react(npc, rules, text, situation)
end

function onCreatureDisappear(npc, creature)
	behaviour.react(npc, rules, "", NPC_SITUATION_VANISH)
end
