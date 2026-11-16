-- npcs/noodles.lua: the behaviour of the NPC Noodles, converted from
-- noodles.npc by patella-convert. The NPC is its [[npc]] entry in npcs.toml;
-- lib/npc.lua matches and runs the rules below, in the order of the file
-- (scripting.hh, "NPCs"). The comments are those of noodles.npc.
--
-- GIMUD - Graphical Interface Multi User Dungeon
-- noodles.npc: Datenbank für den Pudel Noodles

local behaviour = require("lib.npc")
local ADDRESS = behaviour.ADDRESS
local BUSY = behaviour.BUSY
local VANISH = behaviour.VANISH
local FORCE = behaviour.FORCE
local eq = behaviour.eq
local ne = behaviour.ne
local ge = behaviour.ge
local TOPIC = behaviour.TOPIC
local AMOUNT = behaviour.AMOUNT
local TYPE = behaviour.TYPE
local count = behaviour.count
local questValue = behaviour.questValue
local setTopic = behaviour.setTopic
local setAmount = behaviour.setAmount
local setType = behaviour.setType
local setQuestValue = behaviour.setQuestValue
local delete = behaviour.delete
local idle = behaviour.idle
local nop = behaviour.nop
local previous = behaviour.previous

local rules = behaviour.rules {
	{{ADDRESS, "hello$", FORCE}, {setType(3577), setAmount(1), "<sniff> Woof! <sniff>"}},
	{{ADDRESS, "hi$", FORCE}, {previous}},
	{{ADDRESS, FORCE}, {"Grrrr!", idle}},
	{{BUSY, "hi$", FORCE}, {"Grrr! Woof!"}},
	{{BUSY, "hello$", FORCE}, {previous}},
	{{BUSY, FORCE}, {nop}},
	{{VANISH, FORCE}, {"Woof?? <sniff> <sniff>"}},

	{{"bye"}, {"Woof! <wiggle>", idle}},
	{{"farewell"}, {previous}},
	{{"how", "are", "you"}, {"Wooooof! <wiggle> <wiggle> <wiggle>"}},
	{{"king"}, {previous}},
	{{"tibianus"}, {previous}},
	{{"cat$"}, {"GRRRRRRR! WOOOOOOF! WOOOOOF! WOOOOOF!"}},
	{{"queen"}, {previous}},
	{{"eloise"}, {previous}},

	{{"pork", ge(count(TYPE), AMOUNT)}, {"Woof! Woof! <wiggle> <drool> <sniff> <gnaw>", delete(TYPE)}},

	{{"sniff", "banana", ne(questValue(233), 7)}, {"Woof!", idle}},
	{{"sniff", "banana", eq(questValue(233), 7), eq(questValue(251), 1)}, {setType(3104), setAmount(1), "<sniff> <sniff>", setTopic(2)}},
	{{"sniff", "fur", ne(questValue(233), 7)}, {"Woof!", idle}},
	{{"sniff", "fur", eq(questValue(233), 7), eq(questValue(251), 1)}, {setType(3105), setAmount(1), "<sniff> <sniff>", setTopic(2)}},
	{{"sniff", "cheese", ne(questValue(233), 7)}, {"Woof!", idle}},
	{{"sniff", "cheese", eq(questValue(233), 7), eq(questValue(251), 1)}, {setType(3120), setAmount(1), "<sniff> <sniff>", setTopic(3)}},

	{{"sniff", "banana", eq(questValue(233), 7), eq(questValue(251), 2)}, {setType(3104), setAmount(1), "<sniff> <sniff>", setTopic(2)}},
	{{"sniff", "fur", eq(questValue(233), 7), eq(questValue(251), 2)}, {setType(3105), setAmount(1), "<sniff> <sniff>", setTopic(3)}},
	{{"sniff", "cheese", eq(questValue(233), 7), eq(questValue(251), 2)}, {setType(3120), setAmount(1), "<sniff> <sniff>", setTopic(2)}},

	{{"sniff", "banana", eq(questValue(233), 7), eq(questValue(251), 3)}, {setType(3104), setAmount(1), "<sniff> <sniff>", setTopic(3)}},
	{{"sniff", "fur", eq(questValue(233), 7), eq(questValue(251), 3)}, {setType(3105), setAmount(1), "<sniff> <sniff>", setTopic(2)}},
	{{"sniff", "cheese", eq(questValue(233), 7), eq(questValue(251), 3)}, {setType(3120), setAmount(1), "<sniff> <sniff>", setTopic(2)}},

	{{eq(TOPIC, 2), "like", "that", ge(count(TYPE), AMOUNT)}, {"Woof!"}},
	{{eq(TOPIC, 3), "like", "that", ge(count(TYPE), AMOUNT)}, {"Meeep! Grrrrr! <spits>", setQuestValue(233, 8), idle}},

	{{"ferumbras"}, {"Meeep! Meeep!", idle}},
	{{"th"}, {"<sniff>"}},
	{{"ar"}, {"Woof!"}},
	{{"bo"}, {"<wiggle>"}},
	{{"an"}, {"Grrrr!"}},
	{{"go"}, {"Woof! Woof!"}},
}

function onCreatureSay(npc, creature, text, situation)
	behaviour.react(npc, rules, text, situation)
end

function onCreatureDisappear(npc, creature)
	behaviour.react(npc, rules, "", NPC_SITUATION_VANISH)
end
