-- npcs/fenbala.lua: the behaviour of the NPC Fenbala, converted from
-- fenbala.npc by patella-convert. The NPC is its [[npc]] entry in npcs.toml;
-- lib/npc.lua matches and runs the rules below, in the order of the file
-- (scripting.hh, "NPCs"). The comments are those of fenbala.npc.
--
-- GIMUD - Graphical Interface Multi User Dungeon
-- fenbala.npc: Datenbank für die Wachfrau Fenbala

local behaviour = require("lib.npc")
local ADDRESS = behaviour.ADDRESS
local BUSY = behaviour.BUSY
local VANISH = behaviour.VANISH
local FORCE = behaviour.FORCE
local setHp = behaviour.setHp
local effectMe = behaviour.effectMe
local effectOpp = behaviour.effectOpp
local idle = behaviour.idle
local nop = behaviour.nop
local previous = behaviour.previous

local rules = behaviour.rules {
	{{ADDRESS, "hello$", "queen", FORCE}, {"HAIL TO THE QUEEN!"}},
	{{ADDRESS, "hail$", "queen", FORCE}, {"HAIL TO THE QUEEN!"}},
	{{ADDRESS, "salutations$", "queen", FORCE}, {"HAIL TO THE QUEEN!"}},
	{{ADDRESS, "hi$", FORCE}, {"MIND YOUR MANNERS COMMONER! To address the queen greet with her title!", idle}},
	{{ADDRESS, "hello$", FORCE}, {previous}},
	{{ADDRESS, FORCE}, {idle}},
	{{BUSY, "hello$", "queen", FORCE}, {"Wait for your audience!"}},
	{{BUSY, "hail$", "queen", FORCE}, {"Wait for your audience!"}},
	{{BUSY, "salutations$", "queen", FORCE}, {"Wait for your audience!"}},
	{{BUSY, "hi$", "queen", FORCE}, {"Wait for your audience!"}},
	{{BUSY, FORCE}, {nop}},
	{{VANISH, FORCE}, {"LONG LIVE THE QUEEN!"}},

	{{"bye"}, {"LONG LIVE THE QUEEN! You may leave now!", idle}},
	{{"farewell"}, {previous}},

	{{"fuck"}, {"Take this!", setHp(1), effectMe(8), effectOpp(5)}},
	{{"idiot"}, {"Take this!", setHp(1), effectMe(8), effectOpp(5)}},
	{{"asshole"}, {"Take this!", setHp(1), effectMe(8), effectOpp(5)}},
	{{"ass$"}, {"Take this!", setHp(1), effectMe(8), effectOpp(5)}},
	{{"fag$"}, {"Take this!", setHp(1), effectMe(8), effectOpp(5)}},
	{{"stupid"}, {"Take this!", setHp(1), effectMe(8), effectOpp(5)}},
	{{"tyrant"}, {"Take this!", setHp(1), effectMe(8), effectOpp(5)}},
	{{"shit"}, {"Take this!", setHp(1), effectMe(8), effectOpp(5)}},
	{{"lunatic"}, {"Take this!", setHp(1), effectMe(8), effectOpp(5)}},
}

function onCreatureSay(npc, creature, text, situation)
	behaviour.react(npc, rules, text, situation)
end

function onCreatureDisappear(npc, creature)
	behaviour.react(npc, rules, "", NPC_SITUATION_VANISH)
end
