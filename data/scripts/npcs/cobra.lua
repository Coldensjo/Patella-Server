-- npcs/cobra.lua: the behaviour of the NPC Cobra, converted from
-- cobra.npc by patella-convert. The NPC is its [[npc]] entry in npcs.toml;
-- lib/npc.lua matches and runs the rules below, in the order of the file
-- (scripting.hh, "NPCs"). The comments are those of cobra.npc.
--
-- GIMUD - Graphical Interface Multi User Dungeon
-- cobra.npc: Datenbank für die steincobra

local behaviour = require("lib.npc")
local ADDRESS = behaviour.ADDRESS
local BUSY = behaviour.BUSY
local VANISH = behaviour.VANISH
local FORCE = behaviour.FORCE
local gt = behaviour.gt
local POISON = behaviour.POISON
local questValue = behaviour.questValue
local setQuestValue = behaviour.setQuestValue
local effectOpp = behaviour.effectOpp
local queue = behaviour.queue
local idle = behaviour.idle
local nop = behaviour.nop
local teleport = behaviour.teleport
local previous = behaviour.previous

local rules = behaviour.rules {
	{{ADDRESS, "hello$", gt(questValue(270), 0), gt(POISON, 500), FORCE}, {"Venture the path of decay!", teleport(33397, 32836, 14), effectOpp(11), setQuestValue(270, 0)}},
	{{ADDRESS, "hi$", gt(questValue(270), 0), gt(POISON, 500), FORCE}, {previous}},

	{{ADDRESS, "hello$", FORCE}, {"Begone! Hissssss! You bear not the mark of the cobra!", idle}},
	{{ADDRESS, "hi$", FORCE}, {previous}},
	{{ADDRESS, FORCE}, {idle}},
	{{BUSY, "hello$", FORCE}, {"Sssssilence!", queue}},
	{{BUSY, "hi$", FORCE}, {previous}},
	{{BUSY, FORCE}, {nop}},
	{{VANISH, FORCE}, {"Hissssssssss."}},
}

function onCreatureSay(npc, creature, text, situation)
	behaviour.react(npc, rules, text, situation)
end

function onCreatureDisappear(npc, creature)
	behaviour.react(npc, rules, "", NPC_SITUATION_VANISH)
end
