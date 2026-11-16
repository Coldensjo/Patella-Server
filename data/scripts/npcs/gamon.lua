-- npcs/gamon.lua: the behaviour of the NPC Gamon, converted from
-- gamon.npc by patella-convert. The NPC is its [[npc]] entry in npcs.toml;
-- lib/npc.lua matches and runs the rules below, in the order of the file
-- (scripting.hh, "NPCs"). The comments are those of gamon.npc.
--
-- GIMUD - Graphical Interface Multi User Dungeon
-- gamon.npc: Möbelverkäufer Gamon in Thais

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
local MALE = behaviour.MALE
local FEMALE = behaviour.FEMALE
local FORCE = behaviour.FORCE
local idle = behaviour.idle
local nop = behaviour.nop
local previous = behaviour.previous

local rules = behaviour.rules {
	{{ADDRESS, "hello$", FEMALE, FORCE}, {"Well, hello there, Lady, %N welcome to Gamon's humble furniture shop!"}},
	{{ADDRESS, "hi$", FEMALE, FORCE}, {previous}},
	{{ADDRESS, "hello$", MALE, FORCE}, {"Nice to meet you, Mister %N! Looking for furniture? You've come to the right place!"}},
	{{ADDRESS, "hi$", MALE, FORCE}, {previous}},
	{{ADDRESS, FORCE}, {idle}},
	{{BUSY, "hello$", FORCE}, {"One moment, please. I got a customer here."}},
	{{BUSY, "hi$", FORCE}, {previous}},
	{{BUSY, FORCE}, {nop}},
	{{VANISH, MALE, FORCE}, {"Now where's he gone? I could have sworn..."}},
	{{VANISH, FORCE}, {"Lady? Lady?? Hm. How strange..."}},

	{{"bye"}, {"You'll come back. They all do.", idle}},
	{{"farewell"}, {previous}},
	{{"job"}, {"I am Thais's foremost furniture salesman."}},
	{{"news"}, {"News? Of course there's news! There's a new furniture man in town, and he's here to stay!"}},
	{{"how", "are", "you"}, {"Excellent! Never felt better in my life!"}},
	{{"name"}, {"My friends call me Gamon. My fans call me the incredible Gammy!"}},
	{{"time"}, {"Any time's a good time to buy some furniture."}},
	{{"Thais"}, {"Thais is obsessed with its past. Everybody here is so proud of their history. Bah! Thais might have a long history, but it has no idea when it comes to interior decoration."}},
	{{"Venore"}, {"The place where it all happens. That town really rocks! Thais could learn a lot about interior decoration from Venore!"}},
	{{"power"}, {"There are a few rumours about a rebellion, but that is all they are."}},
	{{"rebellion"}, {"Well - a few paranoid souls think that Venore wants to gain independence from Thais. Nothing more than rumours."}},

	{{"news"}, {"You mean my specials, don't you?"}},

	{{"quality"}, {"Our furniture is produced by the finest carpenters on the continent using the rare wood of the Venorean marsh willow!"}},
	{{"marsh", "willow"}, {"You can't get any better wood in this world. And it has got a nice smell to it, too. There is nothing nicer than a marsh willow campfire."}},
	{{"king"}, {"His Royal Highness will start to appreciate the superior quality of our stock soon enough!"}},
	{{"sam"}, {"I heard rumours he has some special offers for customers who know to ask for the correct things."}},
	{{"benjamin"}, {"He's incredibly slow. Just your average postman, I guess."}},
	{{"gorn"}, {"He sells stuff of inferior quality. Nothing compared to venores high quality goods."}},
	{{"quentin"}, {"That old monk has probably never left that overcrowded town here."}},
	{{"quest"}, {"A quest?! Who needs quests when there is interior decorating!"}},
	{{"bozo"}, {"Bozo! Damn that clown! He keeps making fake orders. It isn't funny to deliver a wardrobe to an address that doesn't even exist, you know!"}},
	{{"tibia"}, {"Tibia is a wonderful place full of business opportunities."}},
	{{"castle"}, {"I've said it a thousand times! That place needs a complete refurbishing!"}},
	{{"muriel"}, {"The sorcerers guild could realy need someone with taste to redecorate it."}},
	{{"elane"}, {"She's pretty, but I am the kind of man who enjoys a long and healthy life."}},
	{{"marvik"}, {"Druids are obsessed with trees and the bane of any carpenter."}},
	{{"gregor"}, {"Those knights know how to party. And after such partys theres allways need for new furniture."}},
	{{"guild"}, {"Now those people really talk business. There is a new era dawning."}},
	{{"merchants"}, {previous}},
	{{"Topsy"}, {"Ah, those twins. Strange people they are (*sigh*). Oh, they are great to work with, of course. Excellent quality, competetive prices! But well... (whispers) they give me the creeps!"}},
	{{"Turvy"}, {previous}},
	{{"twins"}, {previous}},
	{{"creeps"}, {"Yes! I can never figure out which is which. And they are always watching me!"}},
	{{"watching"}, {"Look! They are doing it again! And they are always smiling!"}},
	{{"smiling"}, {"Yes! Smiling! How a professional sales artist such as myself is supposed to work in such an atmosphere is beyond me!"}},
	{{"artist"}, {"Yes! Selling is a form of art! The elaborate combination of rhetoric and acting which serves to create a sublime longing for the infinite, embodied by second class furniture."}},
	{{"rug"}, {"Oh, silly me! Rugs are out of stock at the moment! But we expect a new shipment anytime. Just watch out for the next update!... Of our inventory I mean."}},

	{{"offer"}, {"I sell statues, tables, chairs, flowers, pillows, pottery, instruments, decoration, tapestries and containers."}},
	{{"goods"}, {previous}},
	{{"do", "you", "sell"}, {previous}},
	{{"do", "you", "have"}, {previous}},
	{{"furniture"}, {previous}},
	{{"equipment"}, {previous}},
	{{"special"}, {"My offers are permanently extraordinarily cheap."}},

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
