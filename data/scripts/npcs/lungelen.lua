-- npcs/lungelen.lua: the behaviour of the NPC Lungelen, converted from
-- lungelen.npc by patella-convert. The NPC is its [[npc]] entry in npcs.toml;
-- lib/npc.lua matches and runs the rules below, in the order of the file
-- (scripting.hh, "NPCs"). The comments are those of lungelen.npc.
--
-- GIMUD - Graphical Interface Multi User Dungeon
-- lungelen.npc: Datenbank für die Erzmagierin Lungelen

local behaviour = require("lib.npc")
local ADDRESS = behaviour.ADDRESS
local BUSY = behaviour.BUSY
local VANISH = behaviour.VANISH
local FORCE = behaviour.FORCE
local idle = behaviour.idle
local nop = behaviour.nop
local previous = behaviour.previous

local rules = behaviour.rules {
	{{ADDRESS, "hello$", FORCE}, {"Please don't disturb me, I am very busy in my recent researches. Have a nice day!", idle}},
	{{ADDRESS, "hi$", FORCE}, {previous}},
	{{ADDRESS, FORCE}, {idle}},
	{{BUSY, "hello$", FORCE}, {nop}},
	{{BUSY, "hi$", FORCE}, {nop}},
	{{BUSY, FORCE}, {nop}},
	{{VANISH, FORCE}, {nop}},
}

function onCreatureSay(npc, creature, text, situation)
	behaviour.react(npc, rules, text, situation)
end

function onCreatureDisappear(npc, creature)
	behaviour.react(npc, rules, "", NPC_SITUATION_VANISH)
end
