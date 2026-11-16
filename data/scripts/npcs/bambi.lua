-- npcs/bambi.lua: the behaviour of the NPC Bambi Bonecrusher, converted from
-- bambi.npc by patella-convert. The NPC is its [[npc]] entry in npcs.toml;
-- lib/npc.lua matches and runs the rules below, in the order of the file
-- (scripting.hh, "NPCs"). The comments are those of bambi.npc.
--
-- GIMUD - Graphical Interface Multi User Dungeon
-- bambi.npc: Datenbank für die Wächterin Bambi Bonecrusher (Carlin)

local behaviour = require("lib.npc")
local guardsCarlin = require("lib.npcs.guards-carlin")

local rules = behaviour.rules {
	guardsCarlin,
}

function onCreatureSay(npc, creature, text, situation)
	behaviour.react(npc, rules, text, situation)
end

function onCreatureDisappear(npc, creature)
	behaviour.react(npc, rules, "", NPC_SITUATION_VANISH)
end
