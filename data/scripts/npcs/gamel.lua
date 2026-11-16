-- npcs/gamel.lua: the behaviour of the NPC Gamel, converted from
-- gamel.npc by patella-convert. The NPC is its [[npc]] entry in npcs.toml;
-- lib/npc.lua matches and runs the rules below, in the order of the file
-- (scripting.hh, "NPCs"). The comments are those of gamel.npc.
--
-- GIMUD - Graphical Interface Multi User Dungeon
-- Gamel.npc: Datenbank für den Gauner Gamel

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
local AMOUNT = behaviour.AMOUNT
local TYPE = behaviour.TYPE
local COUNTMONEY = behaviour.COUNTMONEY
local count = behaviour.count
local mul = behaviour.mul
local setTopic = behaviour.setTopic
local setPrice = behaviour.setPrice
local setAmount = behaviour.setAmount
local setType = behaviour.setType
local poison = behaviour.poison
local effectMe = behaviour.effectMe
local effectOpp = behaviour.effectOpp
local create = behaviour.create
local deleteMoney = behaviour.deleteMoney
local idle = behaviour.idle
local nop = behaviour.nop
local previous = behaviour.previous

local rules = behaviour.rules {
	{{ADDRESS, "hello$", FORCE}, {"Pssst! Be silent. Do you wish to buy something?"}},
	{{ADDRESS, "hi$", FORCE}, {previous}},
	{{ADDRESS, FORCE}, {idle}},
	{{BUSY, "hello$", FORCE}, {"Not right now."}},
	{{BUSY, "hi$", FORCE}, {previous}},
	{{BUSY, FORCE}, {nop}},
	{{VANISH, FORCE}, {"Bye. Tell others about... my little shop here."}},

	{{"bye"}, {"Bye. Tell others about... my little shop here.", idle}},
	{{"farewell"}, {previous}},
	{{"job"}, {"I am selling some... things."}},
	{{"name"}, {"Names don't matter."}},
	{{"gamel"}, {"Oh, you know my name. Please don't tell it to the others."}},

	{{"rebellion"}, {"Uhm... who sent you?", setTopic(3)}},
	{{"berfasmur"}, {"Never heard that name!"}},

	{{"offer"}, {"I sell maces, staffs, daggers and brass helmets."}},
	{{"goods"}, {previous}},
	{{"do", "you", "sell"}, {previous}},
	{{"do", "you", "have"}, {previous}},
	{{"things"}, {previous}},
	{{"help"}, {previous}},

	{{"staff"}, {setType(3289), setAmount(1), setPrice(40), "Do you want to buy it for %P gold?", setTopic(1)}},
	{{"dagger"}, {setType(3267), setAmount(1), setPrice(5), "Do you want to buy it for %P gold?", setTopic(1)}},
	{{"mace"}, {setType(3286), setAmount(1), setPrice(90), "Do you want to buy it for %P gold?", setTopic(1)}},
	{{"brass", "helmet"}, {setType(3354), setAmount(1), setPrice(120), "Do you want to buy it for %P gold?", setTopic(1)}},
	{{"throwing", "knife"}, {setType(3298), setAmount(1), setPrice(25), "Do you want to buy it for %P gold?", setTopic(1)}},

	{{P1, lt(1, P1), "staff"}, {setType(3289), setAmount(P1), setPrice(mul(40, P1)), "Do you want to buy %A staffs for %P gold?", setTopic(1)}},
	{{P1, lt(1, P1), "dagger"}, {setType(3267), setAmount(P1), setPrice(mul(5, P1)), "Do you want to buy %A daggers for %P gold?", setTopic(1)}},
	{{P1, lt(1, P1), "mace"}, {setType(3286), setAmount(P1), setPrice(mul(90, P1)), "Do you want to buy %A maces for %P gold?", setTopic(1)}},
	{{P1, lt(1, P1), "brass", "helmet"}, {setType(3354), setAmount(P1), setPrice(mul(120, P1)), "Do you want to buy %A brass helmets for %P gold?", setTopic(1)}},
	{{P1, lt(1, P1), "throwing", "kni"}, {setType(3298), setAmount(P1), setPrice(mul(25, P1)), "Do you want to buy %A throwing knives for %P gold?", setTopic(1)}},

	{{eq(TOPIC, 1), "yes", ge(COUNTMONEY, PRICE)}, {"And here it is.", deleteMoney, create(TYPE)}},
	{{eq(TOPIC, 1), "yes"}, {"Hey, you do not have enough gold."}},
	{{eq(TOPIC, 1)}, {"Maybe we will make the deal another time."}},

	{{"magic", "crystal"}, {setType(3061), setAmount(1), "Did you bring me a magic crystal?", setTopic(4)}},
	{{eq(TOPIC, 3), "berfasmur"}, {"So, you are a new recruit in the ranks of the rebellion! To proof your worthyness, go and get us a magic crystal."}},
	{{eq(TOPIC, 4), "yes", ge(count(TYPE), AMOUNT)}, {"Brilliant! Bring it to the priest Lugri so that he can cast a deathcurse on the king. The password is 'death to noodles'."}},
	{{eq(TOPIC, 4), "yes", lt(count(TYPE), AMOUNT)}, {"Idiot! You don't have the crystal!", poison(2, 10), effectOpp(9), effectMe(15)}},
}

function onCreatureSay(npc, creature, text, situation)
	behaviour.react(npc, rules, text, situation)
end

function onCreatureDisappear(npc, creature)
	behaviour.react(npc, rules, "", NPC_SITUATION_VANISH)
end
