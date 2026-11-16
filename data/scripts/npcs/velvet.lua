-- npcs/velvet.lua: the behaviour of the NPC Velvet, converted from
-- velvet.npc by patella-convert. The NPC is its [[npc]] entry in npcs.toml;
-- lib/npc.lua matches and runs the rules below, in the order of the file
-- (scripting.hh, "NPCs"). The comments are those of velvet.npc.
--
-- GIMUD - Graphical Interface Multi User Dungeon
-- velvet.npc: Möbelverkäuferin Velvet in Venore

local behaviour = require("lib.npc")
local genTFurniturePillowsS = require("lib.npcs.gen-t-furniture-pillows-s")
local genTFurnitureTapestriesS = require("lib.npcs.gen-t-furniture-tapestries-s")
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
	{{ADDRESS, "hello$", FORCE}, {"Welcome to our shop, %N!"}},
	{{ADDRESS, "hi$", FORCE}, {previous}},
	{{ADDRESS, FORCE}, {idle}},
	{{BUSY, "hello$", FORCE}, {"Just a moment please, %N.", queue}},
	{{BUSY, "hi$", FORCE}, {previous}},
	{{BUSY, FORCE}, {nop}},
	{{VANISH, FORCE}, {"Good bye."}},

	{{"bye"}, {"Good bye.", idle}},
	{{"farewell"}, {previous}},
	{{"name"}, {"I am Velvet. How can I help you?"}},
	{{"job"}, {"I'm working here in this shop. Are you interested in any of our goods?"}},
	{{"time", MALE}, {"It's %T, sire."}},
	{{"time", FEMALE}, {"It's %T, my lady."}},

	{{"offer"}, {"I sell pillows and tapestries."}},
	{{"goods"}, {previous}},
	{{"do", "you", "sell"}, {previous}},
	{{"do", "you", "have"}, {previous}},
	{{"furniture"}, {previous}},
	{{"equipment"}, {previous}},

	genTFurniturePillowsS,
	genTFurnitureTapestriesS,
}

function onCreatureSay(npc, creature, text, situation)
	behaviour.react(npc, rules, text, situation)
end

function onCreatureDisappear(npc, creature)
	behaviour.react(npc, rules, "", NPC_SITUATION_VANISH)
end
