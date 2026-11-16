-- npcs/bashira.lua: the behaviour of the NPC Bashira, converted from
-- bashira.npc by patella-convert. The NPC is its [[npc]] entry in npcs.toml;
-- lib/npc.lua matches and runs the rules below, in the order of the file
-- (scripting.hh, "NPCs"). The comments are those of bashira.npc.
--
-- GIMUD - Graphical Interface Multi User Dungeon
-- bashira.npc: Datenbank fuer die Haendlerin Bashira (Elfenstadt)

local behaviour = require("lib.npc")
local ADDRESS = behaviour.ADDRESS
local BUSY = behaviour.BUSY
local VANISH = behaviour.VANISH
local FORCE = behaviour.FORCE
local P1 = behaviour.P1
local eq = behaviour.eq
local lt = behaviour.lt
local gt = behaviour.gt
local ge = behaviour.ge
local TOPIC = behaviour.TOPIC
local PRICE = behaviour.PRICE
local AMOUNT = behaviour.AMOUNT
local TYPE = behaviour.TYPE
local COUNTMONEY = behaviour.COUNTMONEY
local count = behaviour.count
local mul = behaviour.mul
local setTopic = behaviour.setTopic
local setPrice = behaviour.setPrice
local setAmount = behaviour.setAmount
local setType = behaviour.setType
local setData = behaviour.setData
local create = behaviour.create
local delete = behaviour.delete
local createMoney = behaviour.createMoney
local deleteMoney = behaviour.deleteMoney
local queue = behaviour.queue
local idle = behaviour.idle
local nop = behaviour.nop
local previous = behaviour.previous

local rules = behaviour.rules {
	{{ADDRESS, "hello$", FORCE}, {"Ashari %N."}},
	{{ADDRESS, "hi$", FORCE}, {previous}},
	{{ADDRESS, "ashari$", FORCE}, {previous}},
	{{ADDRESS, FORCE}, {idle}},
	{{BUSY, "hello$", FORCE}, {"Wait one moment, %N.", queue}},
	{{BUSY, "hi$", FORCE}, {previous}},
	{{BUSY, "ashari$", FORCE}, {previous}},
	{{BUSY, FORCE}, {nop}},
	{{VANISH, FORCE}, {"Asha Thrazi."}},

	{{"bye"}, {"Asha Thrazi.", idle}},
	{{"farewell"}, {previous}},
	{{"asha", "thrazi"}, {previous}},
	{{"job"}, {"I sell various equipment and buy some stuff."}},
	{{"equipment"}, {"I sell shovels, picks, scythes, bags, ropes, backpacks, plates, cups, scrolls, parchments, documents, watches, various sources of light, fishing rods and sixpacks of worms."}},
	{{"goods"}, {previous}},
	{{"light"}, {"I sell torches, candelabra, and oil."}},
	{{"name"}, {"I am Bashira Darkmark."}},

	{{"carlin"}, {"Carlin has some capable fighters, allthough they lack the grace of an elf."}},
	{{"thais"}, {"The people of thais boast about their mighty kingdom, but eventually their short lives will doom everything they buld."}},
	{{"venore"}, {"Their merchants have no patience and all to fast they loose their masks of friedlyness."}},
	{{"roderick"}, {"His presence here is a waste of space and talking to or even about him a waste of time."}},
	{{"olrik"}, {"He is quite amusing for a human."}},

	{{"elves"}, {"That's our race."}},
	{{"dwarfs"}, {"They have some talent in mining and smithing."}},
	{{"humans"}, {"They have nothing to give us."}},
	{{"troll"}, {"They are lazy and clumsy. We should use dwarfs instead."}},

	{{"cenath"}, {"Their magic is almost as impressive as their egos."}},
	{{"kuridai"}, {"Without us and our tools nothing would work in this town."}},
	{{"deraisim"}, {"Useless leafeaters."}},
	{{"abdaisim"}, {"They left; perhaps we should do that, too."}},
	{{"teshial"}, {"A stupid Cenath-myth."}},
	{{"ferumbras"}, {"He may scare the treedwellers or the big-mouthes above, but not the Kuridai."}},
	{{"crunor"}, {"One god of many. They are all alike and of no use."}},

	{{"time"}, {"Buy a watch."}},
	{{"food"}, {"I am not dealing with food."}},

	{{"torch"}, {setType(2920), setAmount(1), setPrice(2), "Do you want to buy a torch for %P gold?", setTopic(1)}},
	{{"candelab"}, {setType(2911), setAmount(1), setPrice(8), "Do you want to buy a candelabrum for %P gold?", setTopic(1)}},
	{{"bag"}, {setType(2857), setAmount(1), setPrice(4), "Do you want to buy a bag for %P gold?", setTopic(1)}},
	{{"scroll"}, {setType(2815), setAmount(1), setPrice(5), "Do you want to buy a scroll for %P gold?", setTopic(1)}},
	{{"document"}, {setType(2834), setAmount(1), setPrice(12), "Do you want to buy a document for %P gold?", setTopic(1)}},
	{{"parchment"}, {setType(2835), setAmount(1), setPrice(8), "Do you want to buy a parchment for %P gold?", setTopic(1)}},
	{{"shovel"}, {setType(3457), setAmount(1), setPrice(50), "Do you want to buy a shovel for %P gold?", setTopic(1)}},
	{{"backpack"}, {setType(2865), setAmount(1), setPrice(20), "Do you want to buy a backpack for %P gold?", setTopic(1)}},
	{{"scythe"}, {setType(3453), setAmount(1), setPrice(50), "Do you want to buy a scythe for %P gold?", setTopic(1)}},
	{{"pick"}, {setType(3456), setAmount(1), setPrice(50), "Do you want to buy a pick for %P gold?", setTopic(1)}},
	{{"watch"}, {setType(2906), setAmount(1), setPrice(20), "Do you want to buy one of my high quality watches for %P gold?", setTopic(1)}},
	{{"rope"}, {setType(3003), setAmount(1), setPrice(50), "Do you want to buy a rope for %P gold?", setTopic(1)}},
	{{"rod"}, {setType(3483), setAmount(1), setPrice(150), "Do you want to buy a fishing rod for %P gold?", setTopic(1)}},
	{{"present"}, {setType(2856), setAmount(1), setPrice(10), "Do you want to buy a present for %P gold?", setTopic(1)}},
	{{"oil"}, {setType(2874), setData(7), setAmount(1), setPrice(20), "Do you want to buy oil for %P gold?", setTopic(2)}},
	{{"waterskin"}, {setType(2901), setData(1), setAmount(1), setPrice(10), "Do you want to buy a waterskin for %P gold?", setTopic(1)}},
	{{"cup"}, {setType(2881), setAmount(1), setPrice(2), "Do you want to buy a cup for %P gold?", setTopic(1)}},
	{{"plate"}, {setType(2905), setAmount(1), setPrice(6), "Do you want to buy a plate for %P gold?", setTopic(1)}},
	{{"bucket"}, {setType(2873), setData(0), setAmount(1), setPrice(4), "Do you want to buy a bucket for %P gold?", setTopic(1)}},
	{{"bottle"}, {setType(2875), setData(0), setAmount(1), setPrice(3), "Do you want to buy a bottle for %P gold?", setTopic(1)}},

	{{P1, lt(1, P1), "torch"}, {setType(2920), setAmount(P1), setPrice(mul(2, P1)), "Do you want to buy %A torches for %P gold?", setTopic(1)}},
	{{P1, lt(1, P1), "candelab"}, {setType(2911), setAmount(P1), setPrice(mul(8, P1)), "Do you want to buy %A candelabra for %P gold?", setTopic(1)}},
	{{P1, lt(1, P1), "bag"}, {setType(2857), setAmount(P1), setPrice(mul(4, P1)), "Do you want to buy %A bags for %P gold?", setTopic(1)}},
	{{P1, lt(1, P1), "scroll"}, {setType(2815), setAmount(P1), setPrice(mul(5, P1)), "Do you want to buy %A scrolls for %P gold?", setTopic(1)}},
	{{P1, lt(1, P1), "document"}, {setType(2834), setAmount(P1), setPrice(mul(12, P1)), "Do you want to buy %A documents for %P gold?", setTopic(1)}},
	{{P1, lt(1, P1), "parchment"}, {setType(2835), setAmount(P1), setPrice(mul(8, P1)), "Do you want to buy %A parchments for %P gold?", setTopic(1)}},
	{{P1, lt(1, P1), "shovel"}, {setType(3457), setAmount(P1), setPrice(mul(50, P1)), "Do you want to buy %A shovels for %P gold?", setTopic(1)}},
	{{P1, lt(1, P1), "backpack"}, {setType(2865), setAmount(P1), setPrice(mul(20, P1)), "Do you want to buy %A backpacks for %P gold?", setTopic(1)}},
	{{P1, lt(1, P1), "scythe"}, {setType(3453), setAmount(P1), setPrice(mul(50, P1)), "Do you want to buy %A scythes for %P gold?", setTopic(1)}},
	{{P1, lt(1, P1), "pick"}, {setType(3456), setAmount(P1), setPrice(mul(50, P1)), "Do you want to buy %A picks for %P gold?", setTopic(1)}},
	{{P1, lt(1, P1), "watch"}, {setType(2906), setAmount(P1), setPrice(mul(20, P1)), "Do you want to buy %A of my high quality watches for %P gold?", setTopic(1)}},
	{{P1, lt(1, P1), "rope"}, {setType(3003), setAmount(P1), setPrice(mul(50, P1)), "Do you want to buy %A ropes for %P gold?", setTopic(1)}},
	{{P1, lt(1, P1), "rod"}, {setType(3483), setAmount(P1), setPrice(mul(150, P1)), "Do you want to buy %A fishing rods for %P gold?", setTopic(1)}},
	{{P1, lt(1, P1), "present"}, {setType(2856), setAmount(P1), setPrice(mul(10, P1)), "Do you want to buy %A presents for %P gold?", setTopic(1)}},
	{{P1, lt(1, P1), "oil"}, {setType(2874), setData(7), setAmount(P1), setPrice(mul(20, P1)), "Do you want to buy %A vials of oil for %P gold?", setTopic(2)}},
	{{P1, lt(1, P1), "waterskin"}, {setType(2901), setData(1), setAmount(P1), setPrice(mul(10, P1)), "Do you want to buy %A water skins for %P gold?", setTopic(1)}},
	{{P1, lt(1, P1), "cup"}, {setType(2881), setAmount(P1), setPrice(mul(2, P1)), "Do you want to buy %A cups for %P gold?", setTopic(1)}},
	{{P1, lt(1, P1), "plate"}, {setType(2905), setAmount(P1), setPrice(mul(6, P1)), "Do you want to buy %A plates for %P gold?", setTopic(1)}},
	{{P1, lt(1, P1), "bucket"}, {setType(2873), setData(0), setAmount(P1), setPrice(mul(4, P1)), "Do you want to buy %A buckets for %P gold?", setTopic(1)}},
	{{P1, lt(1, P1), "bottle"}, {setType(2875), setData(0), setAmount(P1), setPrice(mul(3, P1)), "Do you want to buy %A bottles for %P gold?", setTopic(1)}},

	{{"worm"}, {"I sell worms only in sixpacks for 5 gold each, how many sixpacks of worms do you want to buy?"}},
	{{"worms"}, {previous}},
	{{P1, "worms"}, {setType(3492), setAmount(mul(P1, 6)), setPrice(mul(5, P1)), "So you want to buy some of my sixpacks with altogether %A worms for %P gold?", setTopic(1)}},
	{{P1, "six", "pack"}, {previous}},

	{{"sell", "rope"}, {setType(3003), setAmount(1), setPrice(8), "Do you want to sell a rope for %P gold?", setTopic(3)}},
	{{"sell", P1, lt(1, P1), "rope"}, {setType(3003), setAmount(P1), setPrice(mul(8, P1)), "Do you want to sell %A ropes for %P gold?", setTopic(3)}},

	{{eq(TOPIC, 1), "yes", ge(COUNTMONEY, PRICE)}, {"Here it is.", deleteMoney, create(TYPE)}},
	{{eq(TOPIC, 1), "yes"}, {"You don't have so much money."}},
	{{eq(TOPIC, 1)}, {"Then not."}},

	{{eq(TOPIC, 2), "yes", ge(COUNTMONEY, PRICE)}, {"Here it is. There is a deposit of 5 gold on the vial.", deleteMoney, create(TYPE)}},
	{{eq(TOPIC, 2), "yes"}, {"You don't have so much money."}},
	{{eq(TOPIC, 2)}, {"Then not."}},

	{{eq(TOPIC, 3), "yes", ge(count(TYPE), AMOUNT)}, {"Ok. Here's your money.", delete(TYPE), createMoney}},
	{{eq(TOPIC, 3), "yes"}, {"Sorry, you don't have one."}},
	{{eq(TOPIC, 3)}, {"Maybe next time."}},

	{{"deposit"}, {"I will pay you 5 gold for every empty vial. Ok?", setData(0), setTopic(4)}},
	{{"vial"}, {previous}},
	{{"flask"}, {previous}},

	{{eq(TOPIC, 4), "yes", gt(count(2874), 0)}, {setAmount(count(2874)), setPrice(mul(AMOUNT, 5)), "Here you are ... %P gold.", delete(2874), createMoney}},
	{{eq(TOPIC, 4), "yes"}, {"You don't have any empty vials."}},
	{{eq(TOPIC, 4)}, {"Hmm, but please keep Tibia litter free."}},

	{{"buy"}, {"I have shovels, picks, scythes, bags, ropes, backpacks, plates, scrolls, watches, some lightsources, and other stuff."}},
	{{"do", "you", "sell"}, {previous}},
	{{"do", "you", "have"}, {previous}},
	{{"offer"}, {previous}},
	{{"stuff"}, {"Water hoses, pitchforks, presents, buckets, bottles, and the like."}},
}

function onCreatureSay(npc, creature, text, situation)
	behaviour.react(npc, rules, text, situation)
end

function onCreatureDisappear(npc, creature)
	behaviour.react(npc, rules, "", NPC_SITUATION_VANISH)
end
