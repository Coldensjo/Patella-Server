-- npcs/muzir.lua: the behaviour of the NPC Muzir, converted from
-- muzir.npc by patella-convert. The NPC is its [[npc]] entry in npcs.toml;
-- lib/npc.lua matches and runs the rules below, in the order of the file
-- (scripting.hh, "NPCs"). The comments are those of muzir.npc.
--
-- GIMUD - Graphical Interface Multi User Dungeon
-- muzir.npc: Datenbank für den Wesir Muzir

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
	{{ADDRESS, "hello$", FORCE}, {"Welcome %N! Daraman's blessings."}},
	{{ADDRESS, "hi$", FORCE}, {previous}},
	{{ADDRESS, FORCE}, {idle}},
	{{BUSY, "hello$", FORCE}, {"Sorry %N, I am busy. Please wait for your turn.", queue}},
	{{BUSY, "hi$", FORCE}, {previous}},
	{{BUSY, FORCE}, {nop}},
	{{VANISH, FORCE}, {"Daraman's blessings."}},

	{{"bye"}, {"Daraman's blessings.", idle}},
	{{"job"}, {"I am honoured to be the grandwezir of the caliph."}},
	{{"name"}, {"I am Muzir."}}, --## Ibn ??? ###
	{{"time"}, {"It is exactly %T."}},
	{{"caliph"}, {"I am caretaker for the fortune of our beloved and wise caliph."}},
	{{"kazzan"}, {previous}},
	{{"daraman"}, {"I take it upon me to involve myself with worldly issues for the prosperity of our community. I hope the taint of wealth does not harm my soul too much."}},
	{{"wezir"}, {"I am responsible for the wealth of our beloved and wise caliph. I can also change money for you."}},
	{{"wealth"}, {previous}},

	genBank,
}

function onCreatureSay(npc, creature, text, situation)
	behaviour.react(npc, rules, text, situation)
end

function onCreatureDisappear(npc, creature)
	behaviour.react(npc, rules, "", NPC_SITUATION_VANISH)
end
