-- npcs/blossom.lua: the behaviour of the NPC Blossom Bonecrusher, converted from
-- blossom.npc by patella-convert. The NPC is its [[npc]] entry in npcs.toml;
-- lib/npc.lua matches and runs the rules below, in the order of the file
-- (scripting.hh, "NPCs"). The comments are those of blossom.npc.
--
-- GIMUD - Graphical Interface Multi User Dungeon
-- blossom.npc: Datenbank für die Wächterin Blossom Bonecrusher

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
