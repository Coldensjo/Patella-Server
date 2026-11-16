-- npcs/pemaret.lua: the behaviour of the NPC Pemaret, converted from
-- pemaret.npc by patella-convert. The NPC is its [[npc]] entry in npcs.toml;
-- lib/npc.lua matches and runs the rules below, in the order of the file
-- (scripting.hh, "NPCs"). The comments are those of pemaret.npc.
--
-- GIMUD - Graphical Interface Multi User Dungeon
-- pemaret.npc: Fischer Pemaret auf Cormaya

local behaviour = require("lib.npc")
local ADDRESS = behaviour.ADDRESS
local BUSY = behaviour.BUSY
local VANISH = behaviour.VANISH
local MALE = behaviour.MALE
local FEMALE = behaviour.FEMALE
local PZBLOCK = behaviour.PZBLOCK
local FORCE = behaviour.FORCE
local P1 = behaviour.P1
local eq = behaviour.eq
local lt = behaviour.lt
local gt = behaviour.gt
local ge = behaviour.ge
local TOPIC = behaviour.TOPIC
local PRICE = behaviour.PRICE
local TYPE = behaviour.TYPE
local COUNTMONEY = behaviour.COUNTMONEY
local questValue = behaviour.questValue
local mul = behaviour.mul
local setTopic = behaviour.setTopic
local setPrice = behaviour.setPrice
local setAmount = behaviour.setAmount
local setType = behaviour.setType
local effectOpp = behaviour.effectOpp
local create = behaviour.create
local deleteMoney = behaviour.deleteMoney
local queue = behaviour.queue
local idle = behaviour.idle
local nop = behaviour.nop
local teleport = behaviour.teleport
local previous = behaviour.previous

local rules = behaviour.rules {
	{{ADDRESS, "hello$", MALE, FORCE}, {"Greetings, young man. Looking for a passage or some fish, %N?"}},
	{{ADDRESS, "hi$", MALE, FORCE}, {previous}},
	{{ADDRESS, "hello$", FEMALE, FORCE}, {"Greetings, young lady. Looking for a passage or some fish, %N?"}},
	{{ADDRESS, "hi$", FEMALE, FORCE}, {previous}},
	{{ADDRESS, FORCE}, {idle}},
	{{BUSY, "hello$", FORCE}, {"One moment please, %N.", queue}},
	{{BUSY, "hi$", FORCE}, {previous}},
	{{BUSY, FORCE}, {nop}},
	{{VANISH, FORCE}, {"Good bye. You are welcome."}},

	{{"bye"}, {"Good bye. You are welcome.", idle}},
	{{"farewell"}, {previous}},
	{{"name"}, {"My name is Pemaret, the fisherman."}},
	{{"job"}, {"I'm a fisherman and I take along people to Edron. You can also buy some fresh fish."}},
	{{"tibia"}, {"I love to sail on the seas of Tibia."}},
	{{"sea"}, {previous}},
	{{"cormaya"}, {"It's a lovely and peaceful isle. Did you already visit the nice sandy beach?"}},
	{{"isle"}, {previous}},
	{{"beach"}, {"There is a nice sandy beach in the west of Cormaya."}},

	{{"ship"}, {"My boat is ready to bring you to Edron."}},
	{{"boat"}, {previous}},
	{{"passage"}, {previous}},

	{{"edron"}, {setPrice(20), "Do you want to get to Edron for %P gold?", setTopic(1)}},
	{{"edron", gt(questValue(250), 2)}, {setPrice(10), "Do you want to get to Edron for %P gold?", setTopic(1)}},

	{{"fish"}, {setType(3578), setAmount(1), setPrice(5), "Do you want to buy a fresh fish for %P gold?", setTopic(2)}},
	{{P1, lt(1, P1), "fish"}, {setType(3578), setAmount(P1), setPrice(mul(5, P1)), "Do you want to buy %A fresh fishes for %P gold?", setTopic(2)}},

	{{"eremo"}, {"Oh, you know the good old sage Eremo. I can bring you to his little island. Do you want me to do that?", setTopic(3)}},
	{{"sage"}, {previous}},

	{{eq(TOPIC, 1), "yes", PZBLOCK, FORCE}, {"First get rid of those blood stains! You are not going to ruin my vehicle!"}},
	{{eq(TOPIC, 3), "yes", PZBLOCK, FORCE}, {previous}},

	{{eq(TOPIC, 1), "yes", ge(COUNTMONEY, PRICE)}, {"Here we go!", deleteMoney, idle, effectOpp(11), teleport(33175, 31764, 6), effectOpp(11)}},
	{{eq(TOPIC, 1), "yes"}, {"You don't have enough money."}},
	{{eq(TOPIC, 1)}, {"Maybe later."}},

	{{eq(TOPIC, 2), "yes", ge(COUNTMONEY, PRICE)}, {"Here it is.", deleteMoney, create(TYPE)}},
	{{eq(TOPIC, 2), "yes"}, {"I am sorry, but you do not have enough gold."}},
	{{eq(TOPIC, 2)}, {"Maybe later."}},

	{{eq(TOPIC, 3), "yes"}, {"Here we go!", idle, effectOpp(11), teleport(33315, 31882, 7), effectOpp(11)}},
	{{eq(TOPIC, 3)}, {"Maybe later."}},
}

function onCreatureSay(npc, creature, text, situation)
	behaviour.react(npc, rules, text, situation)
end

function onCreatureDisappear(npc, creature)
	behaviour.react(npc, rules, "", NPC_SITUATION_VANISH)
end
