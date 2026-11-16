-- npcs/morun.lua: the behaviour of the NPC Morun, converted from
-- morun.npc by patella-convert. The NPC is its [[npc]] entry in npcs.toml;
-- lib/npc.lua matches and runs the rules below, in the order of the file
-- (scripting.hh, "NPCs"). The comments are those of morun.npc.
--
-- GIMUD - Graphical Interface Multi User Dungeon
-- morun.npc: Datenbank für die Stadtwache Morun in Darashia

local behaviour = require("lib.npc")
local guardsDarama = require("lib.npcs.guards-darama")

local rules = behaviour.rules {
	guardsDarama,
}

function onCreatureSay(npc, creature, text, situation)
	behaviour.react(npc, rules, text, situation)
end

function onCreatureDisappear(npc, creature)
	behaviour.react(npc, rules, "", NPC_SITUATION_VANISH)
end
