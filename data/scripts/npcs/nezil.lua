-- npcs/nezil.lua: the behaviour of the NPC Nezil, converted from
-- nezil.npc by patella-convert. The NPC is its [[npc]] entry in npcs.toml;
-- lib/npc.lua matches and runs the rules below, in the order of the file
-- (scripting.hh, "NPCs"). The comments are those of nezil.npc.
--
-- GIMUD - Graphical Interface Multi User Dungeon
-- nezil.npc: Datenbank für den Händler Nezil

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
	{{ADDRESS, "hello$", "nezil", FORCE}, {"Hiho %N, Nezil at your service."}},
	{{ADDRESS, "hi$", "nezil", FORCE}, {previous}},
	{{ADDRESS, "hiho$", "nezil", FORCE}, {previous}},
	{{ADDRESS, "hello$", FORCE}, {"Uhm, me or my sis', %N?", idle}},
	{{ADDRESS, "hi$", FORCE}, {previous}},
	{{ADDRESS, "hiho$", FORCE}, {previous}},
	{{ADDRESS, FORCE}, {idle}},
	{{BUSY, "hello$", "nezil", FORCE}, {"Hey %N, I am busy. Stand in line.", queue}},
	{{BUSY, "hi$", "nezil", FORCE}, {previous}},
	{{BUSY, "hiho$", "nezil", FORCE}, {previous}},
	{{BUSY, "hello$", FORCE}, {"Uhm, me or my sis', %N?", idle}},
	{{BUSY, "hi$", FORCE}, {previous}},
	{{BUSY, "hiho$", FORCE}, {previous}},
	{{BUSY, FORCE}, {nop}},
	{{VANISH, FORCE}, {"Good bye."}},

	{{"hello$", "bezil"}, {"Good bye.", idle}},
	{{"hi$", "bezil"}, {previous}},
	{{"hiho$", "bezil"}, {previous}},
	{{"bye"}, {previous}},
	{{"farewell"}, {previous}},
	{{"job"}, {"We sell equipment of all kinds. Is there anything you need?"}},
	{{"equipment"}, {"We sell shovels, picks, scythes, bags, ropes, backpacks, cups, scrolls, documents, parchments, and watches. We also sell lightsources."}},
	{{"goods"}, {previous}},
	{{"light"}, {"We sell torches, candlesticks, candelabra, and oil."}},
	{{"name"}, {"I am Nezil Whetstone, son of Fire, of the Savage Axes. I and my sis' Bezil are selling stuff, ye' know?"}},
	{{"bezil"}, {"She's my sis'."}},
	{{"time"}, {"I think it's about %T. If you'd bought a watch you'd know for sure."}},
	{{"food"}, {"Sorry, visit the Jolly Axeman Tavern for that."}},

	{{"goods"}, {"Let me see ... we have shovels, picks, scythes, bags, ropes, backpacks, scrolls, documents, parchments, watches, fishing rods, sixpacks of worms and some lightsources."}},
	{{"do", "you", "sell"}, {previous}},
	{{"do", "you", "have"}, {previous}},
	{{"offer"}, {previous}},

	{{"torch"}, {setType(2920), setAmount(1), setPrice(2), "Do you wanna buy a torch for %P gold?", setTopic(1)}},
	{{"candelabr"}, {setType(2911), setAmount(1), setPrice(8), "Do you wanna buy a candelabrum for %P gold?", setTopic(1)}},
	{{"candlestick"}, {setType(2917), setAmount(1), setPrice(2), "Do you want to buy a candlestick for %P gold?", setTopic(1)}},
	{{"bag"}, {setType(2862), setAmount(1), setPrice(4), "Do you wanna buy a bag for %P gold?", setTopic(1)}},
	{{"scroll"}, {setType(2815), setAmount(1), setPrice(5), "Do you wanna buy a scroll for %P gold?", setTopic(1)}},
	{{"document"}, {setType(2834), setAmount(1), setPrice(12), "Do you want to buy a document for %P gold?", setTopic(1)}},
	{{"parchment"}, {setType(2835), setAmount(1), setPrice(8), "Do you want to buy a parchment for %P gold?", setTopic(1)}},
	{{"shovel"}, {setType(3457), setAmount(1), setPrice(10), "Do you wanna buy a shovel for %P gold?", setTopic(1)}},
	{{"backpack"}, {setType(2870), setAmount(1), setPrice(10), "Do you wanna buy a backpack for %P gold?", setTopic(1)}},
	{{"scythe"}, {setType(3453), setAmount(1), setPrice(40), "Do you wanna buy a scythe for %P gold?", setTopic(1)}},
	{{"pick"}, {setType(3456), setAmount(1), setPrice(50), "Do you wanna buy a pick for %P gold?", setTopic(1)}},
	{{"watch"}, {setType(2906), setAmount(1), setPrice(20), "Do you wanna buy one of my high quality watches for %P gold?", setTopic(1)}},
	{{"rope"}, {setType(3003), setAmount(1), setPrice(50), "Do you wanna buy a rope for %P gold?", setTopic(1)}},
	{{"rod"}, {setType(3483), setAmount(1), setPrice(150), "Do you wanna buy a fishing rod for %P gold?", setTopic(1)}},
	{{"crowbar"}, {setType(3304), setAmount(1), setPrice(260), "Do you wanna buy a dwarfensteel crowbar for %P gold?", setTopic(1)}},
	{{"present"}, {setType(2856), setAmount(1), setPrice(10), "Do you wanna buy a present for %P gold?", setTopic(1)}},
	{{"bucket"}, {setType(2873), setAmount(1), setPrice(4), "Do you wanna buy a bucket for %P gold?", setTopic(1)}},
	{{"bottle"}, {setType(2875), setAmount(1), setPrice(3), "Do you wanna buy a bottle for %P gold?", setTopic(1)}},
	{{"water", "hose"}, {setType(2901), setAmount(1), setPrice(10), setData(1), "Do you wanna buy a water hose for %P gold?", setTopic(1)}},
	{{"oil"}, {setType(2874), setAmount(1), setPrice(20), setData(7), "Do you wanna buy oil for %P gold?", setTopic(2)}},

	{{P1, lt(1, P1), "torch"}, {setType(2920), setAmount(P1), setPrice(mul(2, P1)), "Do you wanna buy %A torches for %P gold?", setTopic(1)}},
	{{P1, lt(1, P1), "candelabr"}, {setType(2911), setAmount(P1), setPrice(mul(8, P1)), "Do you wanna buy %A candelabra for %P gold?", setTopic(1)}},
	{{P1, lt(1, P1), "candlestick"}, {setType(2917), setAmount(P1), setPrice(mul(2, P1)), "Do you want to buy %A candlesticks for %P gold?", setTopic(1)}},
	{{P1, lt(1, P1), "bag"}, {setType(2862), setAmount(P1), setPrice(mul(4, P1)), "Do you wanna buy %A bags for %P gold?", setTopic(1)}},
	{{P1, lt(1, P1), "scroll"}, {setType(2815), setAmount(P1), setPrice(mul(5, P1)), "Do you wanna buy %A scrolls for %P gold?", setTopic(1)}},
	{{P1, lt(1, P1), "document"}, {setType(2834), setAmount(P1), setPrice(mul(12, P1)), "Do you want to buy %A documents for %P gold?", setTopic(1)}},
	{{P1, lt(1, P1), "parchment"}, {setType(2835), setAmount(P1), setPrice(mul(8, P1)), "Do you want to buy %A parchments for %P gold?", setTopic(1)}},
	{{P1, lt(1, P1), "shovel"}, {setType(3457), setAmount(P1), setPrice(mul(10, P1)), "Do you wanna buy %A shovels for %P gold?", setTopic(1)}},
	{{P1, lt(1, P1), "backpack"}, {setType(2870), setAmount(P1), setPrice(mul(10, P1)), "Do you wanna buy %A backpacks for %P gold?", setTopic(1)}},
	{{P1, lt(1, P1), "scythe"}, {setType(3453), setAmount(P1), setPrice(mul(40, P1)), "Do you wanna buy %A scythes for %P gold?", setTopic(1)}},
	{{P1, lt(1, P1), "pick"}, {setType(3456), setAmount(P1), setPrice(mul(50, P1)), "Do you wanna buy %A picks for %P gold?", setTopic(1)}},
	{{P1, lt(1, P1), "watch"}, {setType(2906), setAmount(P1), setPrice(mul(20, P1)), "Do you wanna buy %A of my high quality watches for %P gold?", setTopic(1)}},
	{{P1, lt(1, P1), "rope"}, {setType(3003), setAmount(P1), setPrice(mul(50, P1)), "Do you wanna buy %A ropes for %P gold?", setTopic(1)}},
	{{P1, lt(1, P1), "rod"}, {setType(3483), setAmount(P1), setPrice(mul(150, P1)), "Do you wanna buy %A fishing rods for %P gold?", setTopic(1)}},
	{{P1, lt(1, P1), "crowbar"}, {setType(3304), setAmount(P1), setPrice(mul(260, P1)), "Do you wanna buy %A dwarfensteel crowbars for %P gold?", setTopic(1)}},
	{{P1, lt(1, P1), "present"}, {setType(2856), setAmount(P1), setPrice(mul(10, P1)), "Do you wanna buy %A presents for %P gold?", setTopic(1)}},
	{{P1, lt(1, P1), "bucket"}, {setType(2873), setAmount(P1), setPrice(mul(4, P1)), "Do you wanna buy %A buckets for %P gold?", setTopic(1)}},
	{{P1, lt(1, P1), "bottle"}, {setType(2875), setAmount(P1), setPrice(mul(3, P1)), "Do you wanna buy %A bottles for %P gold?", setTopic(1)}},
	{{P1, lt(1, P1), "water", "hose"}, {setType(2901), setAmount(P1), setPrice(mul(10, P1)), setData(1), "Do you wanna buy %A water hoses for %P gold?", setTopic(1)}},
	{{P1, lt(1, P1), "oil"}, {setType(2874), setAmount(P1), setPrice(mul(20, P1)), setData(7), "Do you wanna buy %A vials of oil for %P gold?", setTopic(2)}},

	{{"worm"}, {"I sell worms only in sixpacks for 5 gold each, how many sixpacks of worms do you want to buy?"}},
	{{"worms"}, {previous}},
	{{P1, "worms"}, {setType(3492), setAmount(mul(P1, 6)), setPrice(mul(5, P1)), "So you want to buy some of my sixpacks with altogether %A worms for %P gold?", setTopic(1)}},
	{{P1, "six", "pack"}, {previous}},

	{{"deposit"}, {"I will give you 5 gold for every empty vial. Ok?", setData(0), setTopic(4)}},
	{{"vial"}, {previous}},
	{{"flask"}, {previous}},

	{{eq(TOPIC, 1), "yes", ge(COUNTMONEY, PRICE)}, {"Here, catch it!", deleteMoney, create(TYPE)}},
	{{eq(TOPIC, 1), "yes"}, {"Nice joke, pauper!"}},
	{{eq(TOPIC, 1)}, {"Then not."}},

	{{eq(TOPIC, 2), "yes", ge(COUNTMONEY, PRICE)}, {"Ok, take it. There is a deposit of 5 gold on the vial.", deleteMoney, create(TYPE)}},
	{{eq(TOPIC, 2), "yes"}, {"Nice joke, pauper!"}},
	{{eq(TOPIC, 2)}, {"Then not."}},

	{{eq(TOPIC, 3), "yes", ge(count(TYPE), AMOUNT)}, {"Ok. Here's your money.", delete(TYPE), createMoney}},
	{{eq(TOPIC, 3), "yes"}, {"Sorry, you are not having one."}},
	{{eq(TOPIC, 3)}, {"Maybe next time."}},

	{{eq(TOPIC, 4), "yes", gt(count(2874), 0)}, {setAmount(count(2874)), setPrice(mul(AMOUNT, 5)), "Here you are ... %P gold.", delete(2874), createMoney}},
	{{eq(TOPIC, 4), "yes"}, {"You don't have any empty vials."}},
	{{eq(TOPIC, 4)}, {"Hmm, but please keep our town litter free."}},
}

function onCreatureSay(npc, creature, text, situation)
	behaviour.react(npc, rules, text, situation)
end

function onCreatureDisappear(npc, creature)
	behaviour.react(npc, rules, "", NPC_SITUATION_VANISH)
end
