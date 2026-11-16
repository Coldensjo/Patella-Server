-- npcs/eddy.lua: the behaviour of the NPC Eddy, converted from
-- eddy.npc by patella-convert. The NPC is its [[npc]] entry in npcs.toml;
-- lib/npc.lua matches and runs the rules below, in the order of the file
-- (scripting.hh, "NPCs"). The comments are those of eddy.npc.
--
-- GIMUD - Graphical Interface Multi User Dungeon
-- eddy.npc: Möbelverkäufer Eddy auf Fibula

local behaviour = require("lib.npc")
local genTFurnitureStatuesS = require("lib.npcs.gen-t-furniture-statues-s")
local genTFurnitureTablesS = require("lib.npcs.gen-t-furniture-tables-s")
local genTFurnitureChairsS = require("lib.npcs.gen-t-furniture-chairs-s")
local genTFurnitureFlowersS = require("lib.npcs.gen-t-furniture-flowers-s")
local genTFurniturePillowsS = require("lib.npcs.gen-t-furniture-pillows-s")
local genTFurniturePotteryS = require("lib.npcs.gen-t-furniture-pottery-s")
local genTFurnitureInstrumentsS = require("lib.npcs.gen-t-furniture-instruments-s")
local genTFurnitureDecorationS = require("lib.npcs.gen-t-furniture-decoration-s")
local genTFurnitureTapestriesS = require("lib.npcs.gen-t-furniture-tapestries-s")
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
	{{ADDRESS, "hello$", FORCE}, {"Welcome %N!"}},
	{{ADDRESS, "hi$", FORCE}, {previous}},
	{{ADDRESS, FORCE}, {idle}},
	{{BUSY, "hello$", FORCE}, {"One moment please %N.", queue}},
	{{BUSY, "hi$", FORCE}, {previous}},
	{{BUSY, FORCE}, {nop}},
	{{VANISH, FORCE}, {"Good bye."}},

	{{"bye"}, {"Good bye.", idle}},
	{{"farewell"}, {previous}},
	{{"name"}, {"My name is Eddy. I sell furniture."}},
	{{"job"}, {"Have you moved to a new home? I'm the specialist for equipping it."}},
	{{"time"}, {"It is %T. Do you need a clock for your house?"}},
	{{"news"}, {"You mean my specials, don't you?"}},

	{{"offer"}, {"I sell statues, tables, chairs, flowers, pillows, pottery, instruments, decoration, tapestries and containers."}},
	{{"goods"}, {previous}},
	{{"do", "you", "sell"}, {previous}},
	{{"do", "you", "have"}, {previous}},
	{{"furniture"}, {previous}},
	{{"equipment"}, {previous}},
	{{"special"}, {"My offers are permanently extraordinary cheap."}},

	genTFurnitureStatuesS,
	genTFurnitureTablesS,
	genTFurnitureChairsS,
	genTFurnitureFlowersS,
	genTFurniturePillowsS,
	genTFurniturePotteryS,
	genTFurnitureInstrumentsS,
	genTFurnitureDecorationS,
	genTFurnitureTapestriesS,
	genTFurnitureContainersS,
}

function onCreatureSay(npc, creature, text, situation)
	behaviour.react(npc, rules, text, situation)
end

function onCreatureDisappear(npc, creature)
	behaviour.react(npc, rules, "", NPC_SITUATION_VANISH)
end
