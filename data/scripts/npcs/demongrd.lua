-- npcs/demongrd.lua: the behaviour of the NPC Demonguard, converted from
-- demongrd.npc by patella-convert. The NPC is its [[npc]] entry in npcs.toml;
-- lib/npc.lua matches and runs the rules below, in the order of the file
-- (scripting.hh, "NPCs"). The comments are those of demongrd.npc.
--
-- GIMUD - Graphical Interface Multi User Dungeon
-- demongrd.npc: Datenbank für den Demonguard

local behaviour = require("lib.npc")
local burning = behaviour.burning
local effectMe = behaviour.effectMe
local effectOpp = behaviour.effectOpp
local idle = behaviour.idle

local rules = behaviour.rules {
	{{}, {"Die, intruder!", burning(25, 8), effectOpp(16), effectMe(14), idle}},
}

function onCreatureSay(npc, creature, text, situation)
	behaviour.react(npc, rules, text, situation)
end

function onCreatureDisappear(npc, creature)
	behaviour.react(npc, rules, "", NPC_SITUATION_VANISH)
end
