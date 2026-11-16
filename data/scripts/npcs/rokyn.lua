-- npcs/rokyn.lua: the behaviour of the NPC Rokyn, converted from
-- rokyn.npc by patella-convert. The NPC is its [[npc]] entry in npcs.toml;
-- lib/npc.lua matches and runs the rules below, in the order of the file
-- (scripting.hh, "NPCs"). The comments are those of rokyn.npc.
--
-- GIMUD - Graphical Interface Multi User Dungeon
-- rokyn.npc: Datenbank für den Bankangestellten Rokyn

local behaviour = require("lib.npc")
local genBank = require("lib.npcs.gen-bank")
local ADDRESS = behaviour.ADDRESS
local BUSY = behaviour.BUSY
local VANISH = behaviour.VANISH
local FORCE = behaviour.FORCE
local queue = behaviour.queue
local idle = behaviour.idle
local nop = behaviour.nop
local previous = behaviour.previous

local rules = behaviour.rules {
	{{ADDRESS, "hello$", FORCE}, {"Hiho, Hiho, %N! What can I do for you?"}},
	{{ADDRESS, "hi$", FORCE}, {previous}},
	{{ADDRESS, "hiho$", FORCE}, {previous}},
	{{ADDRESS, FORCE}, {idle}},
	{{BUSY, "hello$", FORCE}, {"Hey, %N gimme a minute, ok?", queue}},
	{{BUSY, "hi$", FORCE}, {previous}},
	{{BUSY, "hiho$", FORCE}, {previous}},
	{{BUSY, FORCE}, {nop}},
	{{VANISH, FORCE}, {"Yeah, bye."}},

	{{"bye"}, {"Good bye.", idle}},
	{{"farewell"}, {previous}},

	{{"job"}, {"I can change money for you."}},
	{{"name"}, {"I am Rokyn Pursesniffer, son of Fire, proud member of the Molten Rock fellowship."}},
	{{"time"}, {"It is exactly %T right now."}},

	genBank,
}

function onCreatureSay(npc, creature, text, situation)
	behaviour.react(npc, rules, text, situation)
end

function onCreatureDisappear(npc, creature)
	behaviour.react(npc, rules, "", NPC_SITUATION_VANISH)
end
