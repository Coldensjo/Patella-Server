-- npcs/allen.lua: the behaviour of the NPC Allen, converted from
-- allen.npc by patella-convert. The NPC is its [[npc]] entry in npcs.toml;
-- lib/npc.lua matches and runs the rules below, in the order of the file
-- (scripting.hh, "NPCs"). The comments are those of allen.npc.
--
-- GIMUD - Graphical Interface Multi User Dungeon
-- allen.npc: Möbelverkäufer Allen in Venore

local behaviour = require("lib.npc")
local genTFurnitureChairsS = require("lib.npcs.gen-t-furniture-chairs-s")
local ADDRESS = behaviour.ADDRESS
local BUSY = behaviour.BUSY
local VANISH = behaviour.VANISH
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
	{{"name"}, {"My name is Allen Richardson. I am the owner of this market."}},
	{{"job"}, {"I run this market and sell furniture."}},
	{{"time"}, {"It is %T. Too bad we run out of cuckoo clocks."}},
	{{"news"}, {"Sorry, no time to chat, let's trade."}},

	{{"offer"}, {"At this counter you can buy chairs. What do you need?"}},
	{{"goods"}, {previous}},
	{{"do", "you", "sell"}, {previous}},
	{{"do", "you", "have"}, {previous}},
	{{"furniture"}, {previous}},
	{{"equipment"}, {previous}},

	genTFurnitureChairsS,
}

function onCreatureSay(npc, creature, text, situation)
	behaviour.react(npc, rules, text, situation)
end

function onCreatureDisappear(npc, creature)
	behaviour.react(npc, rules, "", NPC_SITUATION_VANISH)
end
