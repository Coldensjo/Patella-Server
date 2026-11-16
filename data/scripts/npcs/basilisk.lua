-- npcs/basilisk.lua: the behaviour of the NPC Basilisk, converted from
-- basilisk.npc by patella-convert. The NPC is its [[npc]] entry in npcs.toml;
-- lib/npc.lua matches and runs the rules below, in the order of the file
-- (scripting.hh, "NPCs"). The comments are those of basilisk.npc.
--
-- GIMUD - Graphical Interface Multi User Dungeon
-- basilisk.npc: Datenbank für den Basilisken

local behaviour = require("lib.npc")
local ADDRESS = behaviour.ADDRESS
local BUSY = behaviour.BUSY
local VANISH = behaviour.VANISH
local FORCE = behaviour.FORCE
local effectMe = behaviour.effectMe
local idle = behaviour.idle
local nop = behaviour.nop
local previous = behaviour.previous

local rules = behaviour.rules {
	{{ADDRESS, "a", FORCE}, {effectMe(9), idle}},
	{{ADDRESS, "b", FORCE}, {previous}},
	{{ADDRESS, "c", FORCE}, {previous}},
	{{ADDRESS, "d", FORCE}, {previous}},
	{{ADDRESS, "e", FORCE}, {previous}},

	{{ADDRESS, "f", FORCE}, {effectMe(13), idle}},
	{{ADDRESS, "g", FORCE}, {previous}},
	{{ADDRESS, "h", FORCE}, {previous}},

	{{ADDRESS, "i", FORCE}, {effectMe(14), idle}},
	{{ADDRESS, "j", FORCE}, {previous}},
	{{ADDRESS, "k", FORCE}, {previous}},
	{{ADDRESS, "l", FORCE}, {previous}},
	{{ADDRESS, "m", FORCE}, {previous}},

	{{ADDRESS, "n", FORCE}, {effectMe(15), idle}},
	{{ADDRESS, "o", FORCE}, {previous}},
	{{ADDRESS, "p", FORCE}, {previous}},
	{{ADDRESS, "r", FORCE}, {previous}},
	{{ADDRESS, "s", FORCE}, {previous}},

	{{ADDRESS, "t", FORCE}, {effectMe(17), idle}},
	{{ADDRESS, "u", FORCE}, {previous}},
	{{ADDRESS, "v", FORCE}, {previous}},
	{{ADDRESS, "w", FORCE}, {previous}},
	{{ADDRESS, "y", FORCE}, {previous}},

	{{ADDRESS, FORCE}, {idle}},

	{{BUSY, FORCE}, {nop}},

	{{VANISH, FORCE}, {nop}},
}

function onCreatureSay(npc, creature, text, situation)
	behaviour.react(npc, rules, text, situation)
end

function onCreatureDisappear(npc, creature)
	behaviour.react(npc, rules, "", NPC_SITUATION_VANISH)
end
