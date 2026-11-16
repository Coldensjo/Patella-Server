-- npcs/lector.lua: the behaviour of the NPC Lector, converted from
-- lector.npc by patella-convert. The NPC is its [[npc]] entry in npcs.toml;
-- lib/npc.lua matches and runs the rules below, in the order of the file
-- (scripting.hh, "NPCs"). The comments are those of lector.npc.
--
-- GIMUD - Graphical Interface Multi User Dungeon
-- lector.npc: Datenbank für den Metzger Lector

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
	{{ADDRESS, "hello$", FORCE}, {"Welcome to my humble shop, %N."}},
	{{ADDRESS, "hi$", FORCE}, {previous}},
	{{ADDRESS, FORCE}, {idle}},
	{{BUSY, "hello$", FORCE}, {"Just a minute, %N.", queue}},
	{{BUSY, "hi$", FORCE}, {previous}},
	{{BUSY, FORCE}, {nop}},
	{{VANISH, FORCE}, {"Please come and buy again."}},

	{{"bye"}, {"Please come and buy again.", idle}},
	{{"farewell"}, {previous}},
	{{"job"}, {"I am the butcher. I am selling delicious meat."}},
	{{"butcher"}, {previous}},
	{{"name"}, {"My father named me Lector."}},
	{{"father"}, {"My father, Hannibal, was the royal cook. He died some years ago in an attack of the evil Ferumbras."}},
	{{"time"}, {"It is exactly %T."}},
	{{"graveyard"}, {"I heared, the mausoleum is haunted!"}},
	{{"mausoleum"}, {previous}},
	{{"dragon", "steak"}, {"Dragon steak was the favourite meal of Tark Trueblade."}},
	{{"tark", "trueblade"}, {"Tark Trueblade was the greatest dragonslayer of all. I heared, he died in a battle with an ancient dragon lord."}},
	{{"ghostlands"}, {"A bloody place with a bloody history. I wonder if it realy drove people mad or if it just attracted those already disbalanced in their minds."}},

	{{"buy"}, {"I can offer you ham or meat. Dragon steaks are out. <giggle>"}},
	{{"do", "you", "sell"}, {previous}},
	{{"do", "you", "have"}, {previous}},
	{{"food"}, {"Are you looking for food? I can offer you ham or meat."}},

	{{"meat"}, {setType(3577), setAmount(1), setPrice(3), "Do you want to buy meat for %P gold?", setTopic(1)}},
	{{"ham"}, {setType(3582), setAmount(1), setPrice(6), "Do you want to buy a ham for %P gold?", setTopic(1)}},

	{{P1, lt(1, P1), "meat"}, {setType(3577), setAmount(P1), setPrice(mul(3, P1)), "Do you wanna buy %A meat for %P gold?", setTopic(1)}},
	{{P1, lt(1, P1), "ham"}, {setType(3582), setAmount(P1), setPrice(mul(6, P1)), "Do you wanna buy %A ham for %P gold?", setTopic(1)}},

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
