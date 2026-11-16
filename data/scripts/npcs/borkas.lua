-- npcs/borkas.lua: the behaviour of the NPC Borkas, converted from
-- borkas.npc by patella-convert. The NPC is its [[npc]] entry in npcs.toml;
-- lib/npc.lua matches and runs the rules below, in the order of the file
-- (scripting.hh, "NPCs"). The comments are those of borkas.npc.
--
-- GIMUD - Graphical Interface Multi User Dungeon
-- borkas.npc: Möbelverkäufer Borkas in Venore

local behaviour = require("lib.npc")
local genTFurnitureContainersS = require("lib.npcs.gen-t-furniture-containers-s")
local ADDRESS = behaviour.ADDRESS
local BUSY = behaviour.BUSY
local VANISH = behaviour.VANISH
local FORCE = behaviour.FORCE
local queue = behaviour.queue
local idle = behaviour.idle
local nop = behaviour.nop
local previous = behaviour.previous

local rules = behaviour.rules {
	{{ADDRESS, "hello$", FORCE}, {"Hey %N, what'cha want?"}},
	{{ADDRESS, "hi$", FORCE}, {previous}},
	{{ADDRESS, "greetings$", FORCE}, {previous}},
	{{ADDRESS, FORCE}, {idle}},
	{{BUSY, "hello$", FORCE}, {"Hey, %N. Would ya mind not interruptin'? Thanks.", queue}},
	{{BUSY, "hi$", FORCE}, {previous}},
	{{BUSY, "greetings$", FORCE}, {previous}},
	{{BUSY, FORCE}, {nop}},
	{{VANISH, FORCE}, {"Yeah, sod off..."}},

	{{"bye"}, {"Thanks and see ya.", idle}},
	{{"farewell"}, {previous}},
	{{"job"}, {"I'm into sellin' furniture. My grandfather was in that business, then my father, and so am I."}},
	{{"shop"}, {previous}},
	{{"name"}, {"I'm Borkas Flersson, but let's not waste precious tradin' time with smalltalk."}},
	{{"time"}, {"Time is %T now."}},
	{{"thanks"}, {"No prob."}},
	{{"thank", "you"}, {previous}},
	{{"allen"}, {"Hes my boss but he likes to be one of us and sells some of his wares personally."}},
	{{"richardson"}, {previous}},

	{{"offer"}, {"I'm selling containers here."}},
	{{"goods"}, {previous}},
	{{"do", "you", "sell"}, {previous}},
	{{"do", "you", "have"}, {previous}},
	{{"furniture"}, {previous}},
	{{"equipment"}, {previous}},

	genTFurnitureContainersS,
}

function onCreatureSay(npc, creature, text, situation)
	behaviour.react(npc, rules, text, situation)
end

function onCreatureDisappear(npc, creature)
	behaviour.react(npc, rules, "", NPC_SITUATION_VANISH)
end
