-- npcs/hofech.lua: the behaviour of the NPC Hofech, converted from
-- hofech.npc by patella-convert. The NPC is its [[npc]] entry in npcs.toml;
-- lib/npc.lua matches and runs the rules below, in the order of the file
-- (scripting.hh, "NPCs"). The comments are those of hofech.npc.
--
-- GIMUD - Graphical Interface Multi User Dungeon
-- hofech.npc: Datenbank für den Möbelhändler Hofech in Darashia

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
	{{ADDRESS, "hello$", FORCE}, {"Welcome %N! Have a look at my wares, my friend."}},
	{{ADDRESS, "hi$", FORCE}, {previous}},
	{{ADDRESS, FORCE}, {idle}},
	{{BUSY, "hello$", FORCE}, {"%N, just a moment, please.", queue}},
	{{BUSY, "hi$", FORCE}, {previous}},
	{{BUSY, FORCE}, {nop}},
	{{VANISH, FORCE}, {"Daraman's blessings."}},

	{{"bye"}, {"Daraman's blessings.", idle}},
	{{"name"}, {"I am Hofech Ibn Kalith."}},
	{{"job"}, {"I am selling furniture and equipment to grace the homes."}},
	{{"time"}, {"It's %T my friend."}},
	{{"caliph"}, {"The caliph has the finest home and interior decoration in Darashia."}},
	{{"kazzan"}, {previous}},
	{{"daraman"}, {"The prophet is alive! Both in the heavens and in our our hearts."}},
	{{"ferumbras"}, {"I don't know about such a person, my friend."}},
	{{"excalibug"}, {"I think if that sword really existed, it would make a splendid decoration for any wall."}},
	{{"thais"}, {"The wood we are using to make our exquisite furniture is partly supplied by Thais."}},
	{{"tibia"}, {"One day I will take my flying carpet to see the whole world."}},
	{{"carlin"}, {"I was not there yet. But I plan to travel there one day."}},

	{{"offer"}, {"I sell statues, tables, chairs, flowers, pillows, pottery, instruments, decoration, tapestries and containers."}},
	{{"goods"}, {previous}},
	{{"do", "you", "sell"}, {previous}},
	{{"do", "you", "have"}, {previous}},
	{{"furniture"}, {previous}},
	{{"equipment"}, {previous}},
	{{"special"}, {"My offers are extraordinarily cheap, given the fine quality of our stock. Just look at that table!"}},
	{{"carpet"}, {"No no, I don't sell any carpets."}},

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
