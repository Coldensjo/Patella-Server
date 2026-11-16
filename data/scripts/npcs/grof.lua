-- npcs/grof.lua: the behaviour of the NPC Grof, the guard, converted from
-- grof.npc by patella-convert. The NPC is its [[npc]] entry in npcs.toml;
-- lib/npc.lua matches and runs the rules below, in the order of the file
-- (scripting.hh, "NPCs"). The comments are those of grof.npc.
--
-- GIMUD - Graphical Interface Multi User Dungeon
-- grof.npc: Datenbank für die Stadtwache am Nordtor

local behaviour = require("lib.npc")
local guardsThais = require("lib.npcs.guards-thais")

local rules = behaviour.rules {
	guardsThais,
}

function onCreatureSay(npc, creature, text, situation)
	behaviour.react(npc, rules, text, situation)
end

function onCreatureDisappear(npc, creature)
	behaviour.react(npc, rules, "", NPC_SITUATION_VANISH)
end
