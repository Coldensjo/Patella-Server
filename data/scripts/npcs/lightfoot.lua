-- npcs/lightfoot.lua: the behaviour of the NPC Lightfoot, converted from
-- lightfoot.npc by patella-convert. The NPC is its [[npc]] entry in npcs.toml;
-- lib/npc.lua matches and runs the rules below, in the order of the file
-- (scripting.hh, "NPCs"). The comments are those of lightfoot.npc.
--
-- GIMUD - Graphical Interface Multi User Dungeon
-- lightfoot.npc: Datenbank fuer den Rennhund Lightfoot

local behaviour = require("lib.npc")
local ADDRESS = behaviour.ADDRESS
local idle = behaviour.idle

local rules = behaviour.rules {
	{{ADDRESS}, {idle}},
}

function onCreatureSay(npc, creature, text, situation)
	behaviour.react(npc, rules, text, situation)
end

function onCreatureDisappear(npc, creature)
	behaviour.react(npc, rules, "", NPC_SITUATION_VANISH)
end
