-- npcs/feizuhl.lua: the behaviour of the NPC Feizuhl, converted from
-- feizuhl.npc by patella-convert. The NPC is its [[npc]] entry in npcs.toml;
-- lib/npc.lua matches and runs the rules below, in the order of the file
-- (scripting.hh, "NPCs"). The comments are those of feizuhl.npc.
--
-- GIMUD - Graphical Interface Multi User Dungeon
-- feizuhl.npc: Möbelverkäufer Feizuhl in Ankrahmun

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
	{{ADDRESS, "hello$", FORCE}, {"Be mourned pilgrim in flesh."}},
	{{ADDRESS, "hi$", FORCE}, {previous}},
	{{ADDRESS, "greetings$", FORCE}, {previous}},
	{{ADDRESS, FORCE}, {idle}},
	{{BUSY, "hello$", FORCE}, {"Please wait a moment, mourned %N.", queue}},
	{{BUSY, "hi$", FORCE}, {previous}},
	{{BUSY, "greetings$", FORCE}, {previous}},
	{{BUSY, FORCE}, {nop}},
	{{VANISH, FORCE}, {"Good bye."}},

	{{"bye"}, {"Good bye.", idle}},
	{{"farewell"}, {previous}},
	{{"job"}, {"I sell furniture both to the mourned and the enlightened."}},
	{{"shop"}, {previous}},
	{{"name"}, {"I'm the mourned Feizuhl, pilgrim."}},
	{{"time"}, {"It is %T right now."}},
	{{"thanks"}, {"It was a pleasure, pilgrim."}},
	{{"thank", "you"}, {previous}},

	{{"darama"}, {"This is the continent of my birth, my death and also of my ascension, if I learn enough in my mortal days."}},
	{{"darashia"}, {"If they would only see the light and follow the way of ascension. Thrice mourned be they."}},
	{{"daraman"}, {"The false prophet lead his people into damnation. Mourned shall he be."}},
	{{"ankrahmun"}, {"Our city is old. Older even then our beloved pharaoh."}},
	{{"city"}, {previous}},

	{{"pharaoh"}, {"Our pharaoh holds the key to our ascension. Praised be our pharaoh."}},
	{{"arkhothep"}, {previous}},
	{{"mortality"}, {"Only if we leave mortality behind will we attain ascension."}},

	{{"ascension"}, {"The ascension to salvation and perhaps even to divine status."}},
	{{"Akh'rah", "Uthun"}, {"The Akh'rah Uthun is the union of the three parts that were never meant to be bound together."}},
	{{"Akh"}, {"The Akh is our vulnerable, ageing flesh."}},

	{{"undead"}, {"Undeath is an important step towards ascension."}},
	{{"undeath"}, {previous}},
	{{"Rah"}, {"The Rah is our spiritual essence."}},
	{{"uthun"}, {"The Uthun is our memory. Call it personality if you like."}},
	{{"mourn"}, {"We are mortals and thus to be mourned, for while we are trapped in this frail form we are excluded from enlightenment and ascension."}},

	{{"arena"}, {"Look for the arena in the heart of our city."}},
	{{"palace"}, {"The palace is located in the centre of our city, south of the arena."}},
	{{"temple"}, {"Look for the temple in the south-eastern section of Ankrahmun."}},

	{{"offer"}, {"I sell statues, tables, chairs, flowers, pillows, pottery, instruments, decoration, tapestries and containers."}},
	{{"goods"}, {previous}},
	{{"do", "you", "sell"}, {previous}},
	{{"do", "you", "have"}, {previous}},
	{{"furniture"}, {previous}},
	{{"equipment"}, {previous}},

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
