-- npcs/stutch.lua: the behaviour of the NPC Stutch, converted from
-- stutch.npc by patella-convert. The NPC is its [[npc]] entry in npcs.toml;
-- lib/npc.lua matches and runs the rules below, in the order of the file
-- (scripting.hh, "NPCs"). The comments are those of stutch.npc.
--
-- GIMUD - Graphical Interface Multi User Dungeon
-- stutch.npc: Datenbank für den Wachmann Stutch

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
	{{ADDRESS, "hello$", "king", FORCE}, {"HAIL TO THE KING!"}},
	{{ADDRESS, "hail$", "king", FORCE}, {"HAIL TO THE KING!"}},
	{{ADDRESS, "salutations$", "king", FORCE}, {"HAIL TO THE KING!"}},
	{{ADDRESS, "hi$", FORCE}, {"MIND YOUR MANNERS COMMONER! To address the king greet with his title!", idle}},
	{{ADDRESS, "hello$", FORCE}, {previous}},
	{{ADDRESS, FORCE}, {idle}},
	{{BUSY, "hello$", "king", FORCE}, {"Wait for your audience!"}},
	{{BUSY, "hail$", "king", FORCE}, {"Wait for your audience!"}},
	{{BUSY, "salutations$", "king", FORCE}, {"Wait for your audience!"}},
	{{BUSY, "hi$", "king", FORCE}, {"Wait for your audience!"}},
	{{BUSY, FORCE}, {nop}},
	{{VANISH, FORCE}, {"LONG LIVE THE KING!"}},

	{{"bye"}, {"LONG LIVE THE KING! You may leave now!", idle}},
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
