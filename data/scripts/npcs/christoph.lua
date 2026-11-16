-- npcs/christoph.lua: the behaviour of the NPC Christoph, converted from
-- christoph.npc by patella-convert. The NPC is its [[npc]] entry in npcs.toml;
-- lib/npc.lua matches and runs the rules below, in the order of the file
-- (scripting.hh, "NPCs"). The comments are those of christoph.npc.
--
-- GIMUD - Graphical Interface Multi User Dungeon
-- christoph.npc: Datenbank für eine Stadtwache in Venore

local behaviour = require("lib.npc")
local guardsVenore = require("lib.npcs.guards-venore")

local rules = behaviour.rules {
	guardsVenore,
}

function onCreatureSay(npc, creature, text, situation)
	behaviour.react(npc, rules, text, situation)
end

function onCreatureDisappear(npc, creature)
	behaviour.react(npc, rules, "", NPC_SITUATION_VANISH)
end
