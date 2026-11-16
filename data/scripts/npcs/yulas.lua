-- npcs/yulas.lua: the behaviour of the NPC Yulas, converted from
-- yulas.npc by patella-convert. The NPC is its [[npc]] entry in npcs.toml;
-- lib/npc.lua matches and runs the rules below, in the order of the file
-- (scripting.hh, "NPCs"). The comments are those of yulas.npc.
--
-- GIMUD - Graphical Interface Multi User Dungeon
-- yulas.npc: Möbelverkäufer Yulas in Venore

local behaviour = require("lib.npc")
local genTFurnitureTablesS = require("lib.npcs.gen-t-furniture-tables-s")
local ADDRESS = behaviour.ADDRESS
local BUSY = behaviour.BUSY
local VANISH = behaviour.VANISH
local MALE = behaviour.MALE
local FEMALE = behaviour.FEMALE
local FORCE = behaviour.FORCE
local queue = behaviour.queue
local idle = behaviour.idle
local nop = behaviour.nop
local previous = behaviour.previous

local rules = behaviour.rules {
	{{ADDRESS, "hello$", FORCE}, {"Welcome to the Plank and Treasurechest Market, %N!"}},
	{{ADDRESS, "hi$", FORCE}, {previous}},
	{{ADDRESS, FORCE}, {idle}},
	{{BUSY, "hello$", FORCE}, {"Just a moment please, %N.", queue}},
	{{BUSY, "hi$", FORCE}, {previous}},
	{{BUSY, FORCE}, {nop}},
	{{VANISH, FORCE}, {"Good bye."}},

	{{"bye"}, {"Good bye.", idle}},
	{{"farewell"}, {previous}},
	{{"name"}, {"I am Yulas. I will be your salesperson today."}},
	{{"job"}, {"We are into home improvement."}},
	{{"time", MALE}, {"It's %T, sire."}},
	{{"time", FEMALE}, {"It's %T, my lady."}},
	{{"news"}, {"Sorry, we are not allowed to chat."}},
	{{"allen"}, {"To think just because he is around here to watch what we do, he want to be considered one of us..."}},
	{{"richardson"}, {previous}},

	{{"offer"}, {"We sell furniture and equipment. At this counter you can buy tables."}},
	{{"goods"}, {previous}},
	{{"do", "you", "sell"}, {previous}},
	{{"do", "you", "have"}, {previous}},
	{{"furniture"}, {previous}},
	{{"equipment"}, {previous}},

	genTFurnitureTablesS,
}

function onCreatureSay(npc, creature, text, situation)
	behaviour.react(npc, rules, text, situation)
end

function onCreatureDisappear(npc, creature)
	behaviour.react(npc, rules, "", NPC_SITUATION_VANISH)
end
