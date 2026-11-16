-- npcs/arnold.lua: the behaviour of the NPC Arnold, converted from
-- arnold.npc by patella-convert. The NPC is its [[npc]] entry in npcs.toml;
-- lib/npc.lua matches and runs the rules below, in the order of the file
-- (scripting.hh, "NPCs"). The comments are those of arnold.npc.
--
-- GIMUD - Graphical Interface Multi User Dungeon
-- arnold.npc: Datenbank für eine Stadtwache in Venore

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
