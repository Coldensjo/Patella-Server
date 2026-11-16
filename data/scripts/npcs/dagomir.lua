-- npcs/dagomir.lua: the behaviour of the NPC Dagomir, converted from
-- dagomir.npc by patella-convert. The NPC is its [[npc]] entry in npcs.toml;
-- lib/npc.lua matches and runs the rules below, in the order of the file
-- (scripting.hh, "NPCs"). The comments are those of dagomir.npc.
--
-- GIMUD - Graphical Interface Multi User Dungeon
-- dagomir.npc: Datenbank für den "Bankier" Dagomir

local behaviour = require("lib.npc")
local ADDRESS = behaviour.ADDRESS
local FORCE = behaviour.FORCE
local idle = behaviour.idle
local previous = behaviour.previous

local rules = behaviour.rules {
	{{ADDRESS, "hello$", FORCE}, {"Wha... what?? HOW DARE YOU!!?? LEAVE ME ALONE ON MY TOILET AT ONCE!", idle}},
	{{ADDRESS, "hi$", FORCE}, {previous}},
	{{ADDRESS, FORCE}, {idle}},
}

function onCreatureSay(npc, creature, text, situation)
	behaviour.react(npc, rules, text, situation)
end

function onCreatureDisappear(npc, creature)
	behaviour.react(npc, rules, "", NPC_SITUATION_VANISH)
end
