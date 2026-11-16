-- npcs/rose.lua: the behaviour of the NPC Rose, converted from
-- rose.npc by patella-convert. The NPC is its [[npc]] entry in npcs.toml;
-- lib/npc.lua matches and runs the rules below, in the order of the file
-- (scripting.hh, "NPCs"). The comments are those of rose.npc.
--
-- GIMUD - Graphical Interface Multi User Dungeon
-- rose.npc: Blumenverkäuferin Rose in Venore

local behaviour = require("lib.npc")
local genTFurnitureFlowersS = require("lib.npcs.gen-t-furniture-flowers-s")
local ADDRESS = behaviour.ADDRESS
local BUSY = behaviour.BUSY
local VANISH = behaviour.VANISH
local FORCE = behaviour.FORCE
local queue = behaviour.queue
local idle = behaviour.idle
local nop = behaviour.nop
local previous = behaviour.previous

local rules = behaviour.rules {
	{{ADDRESS, "hello$", FORCE}, {"Welcome to Crunor's Finest Warehouse, %N."}},
	{{ADDRESS, "hi$", FORCE}, {previous}},
	{{ADDRESS, FORCE}, {idle}},
	{{BUSY, "hello$", FORCE}, {"One moment please, %N.", queue}},
	{{BUSY, "hi$", FORCE}, {previous}},
	{{BUSY, FORCE}, {nop}},
	{{VANISH, FORCE}, {"Good bye."}},

	{{"bye"}, {"Good bye.", idle}},
	{{"farewell"}, {previous}},
	{{"name"}, {"My name is Rose, nice to meet you, %N."}},
	{{"rose"}, {"That's me. I am not for sale. <giggles>"}},
	{{"job"}, {"Here you may buy some of the most beautiful flowers."}},
	{{"time"}, {"Sorry, I have no watch on me."}},
	{{"news"}, {"You mean my specials, don't you?"}},

	{{"offer"}, {"I am selling beautiful flowers here."}},
	{{"goods"}, {previous}},
	{{"do", "you", "sell"}, {previous}},
	{{"do", "you", "have"}, {previous}},
	{{"furniture"}, {previous}},
	{{"equipment"}, {previous}},

	genTFurnitureFlowersS,
}

function onCreatureSay(npc, creature, text, situation)
	behaviour.react(npc, rules, text, situation)
end

function onCreatureDisappear(npc, creature)
	behaviour.react(npc, rules, "", NPC_SITUATION_VANISH)
end
