-- npcs/iwar.lua: the behaviour of the NPC Iwar, converted from
-- iwar.npc by patella-convert. The NPC is its [[npc]] entry in npcs.toml;
-- lib/npc.lua matches and runs the rules below, in the order of the file
-- (scripting.hh, "NPCs"). The comments are those of iwar.npc.
--
-- GIMUD - Graphical Interface Multi User Dungeon
-- iwar.npc: Möbelverkäufer Iwar in Kazordoon

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
	{{ADDRESS, "hello$", FORCE}, {"Hiho %N! Welcome to Kazordoon Furniture Store."}},
	{{ADDRESS, "hi$", FORCE}, {previous}},
	{{ADDRESS, "hiho$", FORCE}, {previous}},
	{{ADDRESS, FORCE}, {idle}},
	{{BUSY, "hello$", FORCE}, {"One moment, %N, me busy.", queue}},
	{{BUSY, "hi$", FORCE}, {previous}},
	{{BUSY, "hiho$", FORCE}, {previous}},
	{{BUSY, FORCE}, {nop}},
	{{VANISH, FORCE}, {"Guut bye."}},

	{{"bye"}, {"Guut bye.", idle}},
	{{"farewell"}, {previous}},
	{{"name"}, {"Me is Iwar Woodpecker, son of Earth, from the Savage Axes. Me run this store."}},
	{{"job"}, {"You moving to new home? Me specialist for equipping it."}},
	{{"time"}, {"Time is %T. You needing clock for your house?"}},
	{{"news"}, {"You meaning my specials, eh?"}},

	{{"offer"}, {"Me selling statues, tables, chairs, flowers, pillows, pottery, instruments, decoration, tapestries and containers."}},
	{{"goods"}, {previous}},
	{{"do", "you", "sell"}, {previous}},
	{{"do", "you", "have"}, {previous}},
	{{"furniture"}, {previous}},
	{{"equipment"}, {previous}},
	{{"special"}, {"My offers permanently extraordinary cheap."}},

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
