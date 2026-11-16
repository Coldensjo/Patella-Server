-- npcs/busty.lua: the behaviour of the NPC Busty Bonecrusher, converted from
-- busty.npc by patella-convert. The NPC is its [[npc]] entry in npcs.toml;
-- lib/npc.lua matches and runs the rules below, in the order of the file
-- (scripting.hh, "NPCs"). The comments are those of busty.npc.
--
-- GIMUD - Graphical Interface Multi User Dungeon
-- busty.npc: Datenbank für die Wächterin Busty Bonecrusher

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
