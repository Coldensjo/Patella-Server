-- npcs/digger.lua: the behaviour of the NPC Digger, converted from
-- digger.npc by patella-convert. The NPC is its [[npc]] entry in npcs.toml;
-- lib/npc.lua matches and runs the rules below, in the order of the file
-- (scripting.hh, "NPCs"). The comments are those of digger.npc.
--
-- GIMUD - Graphical Interface Multi User Dungeon
-- digger.npc: Datenbank für den Magieladen-Verkäufer Digger

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
local PRICE = behaviour.PRICE
local AMOUNT = behaviour.AMOUNT
local TYPE = behaviour.TYPE
local COUNTMONEY = behaviour.COUNTMONEY
local count = behaviour.count
local mul = behaviour.mul
local setTopic = behaviour.setTopic
local setPrice = behaviour.setPrice
local setAmount = behaviour.setAmount
local setType = behaviour.setType
local setData = behaviour.setData
local create = behaviour.create
local delete = behaviour.delete
local createMoney = behaviour.createMoney
local deleteMoney = behaviour.deleteMoney
local queue = behaviour.queue
local idle = behaviour.idle
local nop = behaviour.nop
local previous = behaviour.previous

local rules = behaviour.rules {
	{{ADDRESS, "hello$", FORCE}, {"Oh, %N is that you? You look inconveniently healthy."}},
	{{ADDRESS, "hi$", FORCE}, {previous}},
	{{ADDRESS, FORCE}, {idle}},
	{{BUSY, "hello$", FORCE}, {"Wait %N, your time will finally come.", queue}},
	{{BUSY, "hi$", FORCE}, {previous}},
	{{BUSY, FORCE}, {nop}},
	{{VANISH, FORCE}, {"Bye."}},

	{{"bye"}, {"Bye.", idle}},
	{{"farewell"}, {previous}},
	{{"name"}, {"They call me Digger, that fine with me."}},
	{{"frans"}, {"I think the FRANS is bugged."}},
	{{"digger"}, {"So what?"}},
	{{"job"}, {"I am selling some potions."}},
	{{"sorcerer"}, {"The way of the magicwielder is the only way to true power."}},
	{{"druid"}, {previous}},
	{{"magic"}, {"This is the magic market. Just have a look around."}},
	{{"market"}, {previous}},
	{{"vladruc"}, {"Better don't cross the master!"}},
	{{"urghain"}, {previous}},
	{{"ferumbras"}, {"An upstart of minor skills and great ambitions."}},
	{{"excalibug"}, {"Just a knights' legend."}},

	{{"offer"}, {"You may be interested in my life and mana fluids."}},
	{{"goods"}, {previous}},
	{{"do", "you", "sell"}, {previous}},
	{{"do", "you", "have"}, {previous}},
	{{"potion"}, {previous}},

	{{"life", "fluid"}, {setType(2874), setData(11), setAmount(1), setPrice(60), "Do you want to buy life fluid for %P gold?", setTopic(2)}},
	{{"mana", "fluid"}, {setType(2874), setData(10), setAmount(1), setPrice(55), "Do you want to buy mana fluid for %P gold?", setTopic(2)}},
	{{P1, lt(1, P1), "life", "fluid"}, {setType(2874), setData(11), setAmount(P1), setPrice(mul(60, P1)), "Do you want to buy %A potions of life fluid for %P gold?", setTopic(2)}},
	{{P1, lt(1, P1), "mana", "fluid"}, {setType(2874), setData(10), setAmount(P1), setPrice(mul(55, P1)), "Do you want to buy %A potions of mana fluid for %P gold?", setTopic(2)}},

	{{"deposit"}, {"I will pay you 5 gold for every empty vial. Ok?", setData(0), setTopic(3)}},
	{{"vial"}, {previous}},
	{{"flask"}, {previous}},

	{{eq(TOPIC, 2), "yes", ge(COUNTMONEY, PRICE)}, {"Here you are. There is a deposit of 5 gold on the vial.", deleteMoney, create(TYPE)}},
	{{eq(TOPIC, 2)}, {"Don't overestimate my patience."}},

	{{eq(TOPIC, 3), "yes", gt(count(2874), 0)}, {setAmount(count(2874)), setPrice(mul(AMOUNT, 5)), "Here you are ... %P gold.", delete(2874), createMoney}},
	{{eq(TOPIC, 3)}, {"Don't overestimate my patience."}},
}

function onCreatureSay(npc, creature, text, situation)
	behaviour.react(npc, rules, text, situation)
end

function onCreatureDisappear(npc, creature)
	behaviour.react(npc, rules, "", NPC_SITUATION_VANISH)
end
