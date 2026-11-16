-- npcs/chephan.lua: the behaviour of the NPC Chephan, converted from
-- chephan.npc by patella-convert. The NPC is its [[npc]] entry in npcs.toml;
-- lib/npc.lua matches and runs the rules below, in the order of the file
-- (scripting.hh, "NPCs"). The comments are those of chephan.npc.
--
-- GIMUD - Graphical Interface Multi User Dungeon
-- chephan.npc: Datenbank für den Küchenbedarfshändler Chephan

local behaviour = require("lib.npc")
local ADDRESS = behaviour.ADDRESS
local BUSY = behaviour.BUSY
local VANISH = behaviour.VANISH
local FORCE = behaviour.FORCE
local P1 = behaviour.P1
local eq = behaviour.eq
local lt = behaviour.lt
local ge = behaviour.ge
local TOPIC = behaviour.TOPIC
local PRICE = behaviour.PRICE
local TYPE = behaviour.TYPE
local COUNTMONEY = behaviour.COUNTMONEY
local mul = behaviour.mul
local setTopic = behaviour.setTopic
local setPrice = behaviour.setPrice
local setAmount = behaviour.setAmount
local setType = behaviour.setType
local create = behaviour.create
local deleteMoney = behaviour.deleteMoney
local queue = behaviour.queue
local idle = behaviour.idle
local nop = behaviour.nop
local previous = behaviour.previous

local rules = behaviour.rules {
	{{ADDRESS, "hello$", FORCE}, {"Ah, looking for some cooking gear today, %N?"}},
	{{ADDRESS, "hi$", FORCE}, {previous}},
	{{ADDRESS, "hail$", FORCE}, {previous}},
	{{ADDRESS, FORCE}, {idle}},
	{{BUSY, "hello$", FORCE}, {"Please give me another minute, %N. I am talking already, but will be avaliable for you very soon.", queue}},
	{{BUSY, "hi$", FORCE}, {previous}},
	{{BUSY, "hail$", FORCE}, {previous}},
	{{BUSY, FORCE}, {nop}},
	{{VANISH, FORCE}, {"So long, %N."}},

	{{"bye"}, {"So long, %N.", idle}},
	{{"name"}, {"I am Chephan, at your service."}},
	{{"job"}, {"I sell all the cooking gear you can dream of."}},
	{{"warehouse"}, {"Here you can by so many things you will need one day or another. Just have a look."}},
	{{"time"}, {"Watches are sold in the south east part of this warehouse."}},
	{{"king"}, {"Even a king needs a fork now and then. To scratch his back or to poke servants for example."}},
	{{"tibianus"}, {previous}},
	{{"army"}, {"They brought most of their cooking gear from thais."}},
	{{"ferumbras"}, {"See this fork? Now imagine what a hero like you could do to an evil sorcerer with that fork! Care to buy one?"}},
	{{"excalibug"}, {"Just an oversized kitchenknife. Better buy the real thing."}},
	{{"thais"}, {"Thaian cooking gear is of inferior quality. Make sure to upgrade yours here as soon as you can."}},
	{{"tibia"}, {"The world is flat as this plate. You should buy one as a symbol for Tibia."}},
	{{"carlin"}, {"So many women and so little intrest in cooking, horrible."}},
	{{"news"}, {"My recipies are family secrets, sorry."}},
	{{"tax"}, {"Those taxes are killing me. And they are getting worse each year!"}},
	{{"privilege"}, {"I don't feel that privileged. In fact our beloved city is bleeding for the profit of Thais."}},
	{{"gambling"}, {"Thanks to that taxes I have not enough spare money to gamble much."}},

	{{"offer"}, {"That would be: Buckets, bottles, mugs, cups, jugs, plates, baking trays, pots, pans, forks, spoons, knifes, wooden spoons, cleavers, spatulas, and rolling pins."}},
	{{"goods"}, {previous}},
	{{"do", "you", "sell"}, {previous}},
	{{"do", "you", "have"}, {previous}},

	{{"bucket"}, {setType(2873), setAmount(1), setPrice(4), "Do you want to buy a bucket for %P gold?", setTopic(1)}},
	{{"bottle"}, {setType(2875), setAmount(1), setPrice(3), "Do you want to buy a bottle for %P gold?", setTopic(1)}},
	{{"mug"}, {setType(2880), setAmount(1), setPrice(4), "Do you want to buy a mug for %P gold?", setTopic(1)}},
	{{"cup"}, {setType(2881), setAmount(1), setPrice(2), "Do you want to buy a cup for %P gold?", setTopic(1)}},
	{{"jug"}, {setType(2882), setAmount(1), setPrice(10), "Do you want to buy a jug for %P gold?", setTopic(1)}},
	{{"plate"}, {setType(2905), setAmount(1), setPrice(6), "Do you want to buy a plate for %P gold?", setTopic(1)}},
	{{"baking", "tray"}, {setType(3464), setAmount(1), setPrice(20), "Do you want to buy a baking tray for %P gold?", setTopic(1)}},
	{{"pot"}, {setType(3465), setAmount(1), setPrice(30), "Do you want to buy a pot for %P gold?", setTopic(1)}},
	{{"pan"}, {setType(3466), setAmount(1), setPrice(20), "Do you want to buy a pan for %P gold?", setTopic(1)}},
	{{"fork"}, {setType(3467), setAmount(1), setPrice(10), "Do you want to buy a fork for %P gold?", setTopic(1)}},
	{{"spoon"}, {setType(3468), setAmount(1), setPrice(10), "Do you want to buy a spoon for %P gold?", setTopic(1)}},
	{{"knife"}, {setType(3469), setAmount(1), setPrice(10), "Do you want to buy a knife for %P gold?", setTopic(1)}},
	{{"wooden", "spoon"}, {setType(3470), setAmount(1), setPrice(5), "Do you want to buy a wooden spoon for %P gold?", setTopic(1)}},
	{{"cleaver"}, {setType(3471), setAmount(1), setPrice(15), "Do you want to buy a cleaver for %P gold?", setTopic(1)}},
	{{"spatula"}, {setType(3472), setAmount(1), setPrice(12), "Do you want to buy an oven spatula for %P gold?", setTopic(1)}},
	{{"rolling", "pin"}, {setType(3473), setAmount(1), setPrice(12), "Do you want to buy a rolling pin for %P gold?", setTopic(1)}},

	{{P1, lt(1, P1), "bucket"}, {setType(2873), setAmount(P1), setPrice(mul(4, P1)), "Do you want to buy %A buckets for %P gold?", setTopic(1)}},
	{{P1, lt(1, P1), "bottle"}, {setType(2875), setAmount(P1), setPrice(mul(3, P1)), "Do you want to buy %A bottles for %P gold?", setTopic(1)}},
	{{P1, lt(1, P1), "mug"}, {setType(2880), setAmount(P1), setPrice(mul(4, P1)), "Do you want to buy %A mugs for %P gold?", setTopic(1)}},
	{{P1, lt(1, P1), "cup"}, {setType(2881), setAmount(P1), setPrice(mul(2, P1)), "Do you want to buy %A cups for %P gold?", setTopic(1)}},
	{{P1, lt(1, P1), "jug"}, {setType(2882), setAmount(P1), setPrice(mul(10, P1)), "Do you want to buy %A jugs for %P gold?", setTopic(1)}},
	{{P1, lt(1, P1), "plate"}, {setType(2905), setAmount(P1), setPrice(mul(6, P1)), "Do you want to buy %A plates for %P gold?", setTopic(1)}},
	{{P1, lt(1, P1), "baking", "tray"}, {setType(3464), setAmount(P1), setPrice(mul(20, P1)), "Do you want to buy %A baking trays for %P gold?", setTopic(1)}},
	{{P1, lt(1, P1), "pot"}, {setType(3465), setAmount(P1), setPrice(mul(30, P1)), "Do you want to buy %A pots for %P gold?", setTopic(1)}},
	{{P1, lt(1, P1), "pan"}, {setType(3466), setAmount(P1), setPrice(mul(20, P1)), "Do you want to buy %A pans for %P gold?", setTopic(1)}},
	{{P1, lt(1, P1), "fork"}, {setType(3467), setAmount(P1), setPrice(mul(10, P1)), "Do you want to buy %A forks for %P gold?", setTopic(1)}},
	{{P1, lt(1, P1), "spoon"}, {setType(3468), setAmount(P1), setPrice(mul(10, P1)), "Do you want to buy %A spoons for %P gold?", setTopic(1)}},
	{{P1, lt(1, P1), "knife"}, {setType(3469), setAmount(P1), setPrice(mul(10, P1)), "Do you want to buy %A knives for %P gold?", setTopic(1)}},
	{{P1, lt(1, P1), "wooden", "spoon"}, {setType(3470), setAmount(P1), setPrice(mul(5, P1)), "Do you want to buy %A wooden spoons for %P gold?", setTopic(1)}},
	{{P1, lt(1, P1), "cleaver"}, {setType(3471), setAmount(P1), setPrice(mul(15, P1)), "Do you want to buy %A cleavers for %P gold?", setTopic(1)}},
	{{P1, lt(1, P1), "spatula"}, {setType(3472), setAmount(P1), setPrice(mul(12, P1)), "Do you want to buy %A oven spatulas for %P gold?", setTopic(1)}},
	{{P1, lt(1, P1), "rolling", "pin"}, {setType(3473), setAmount(P1), setPrice(mul(12, P1)), "Do you want to buy %A rolling pins for %P gold?", setTopic(1)}},

	{{eq(TOPIC, 1), "yes", ge(COUNTMONEY, PRICE)}, {"Here you are.", deleteMoney, create(TYPE)}},
	{{eq(TOPIC, 1), "yes"}, {"Please come back with more money."}},
	{{eq(TOPIC, 1)}, {"I hope next time."}},
}

function onCreatureSay(npc, creature, text, situation)
	behaviour.react(npc, rules, text, situation)
end

function onCreatureDisappear(npc, creature)
	behaviour.react(npc, rules, "", NPC_SITUATION_VANISH)
end
