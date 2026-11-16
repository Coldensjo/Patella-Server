-- npcs/sarina.lua: the behaviour of the NPC Sarina, converted from
-- sarina.npc by patella-convert. The NPC is its [[npc]] entry in npcs.toml;
-- lib/npc.lua matches and runs the rules below, in the order of the file
-- (scripting.hh, "NPCs"). The comments are those of sarina.npc.
--
-- GIMUD - Graphical Interface Multi User Dungeon
-- sarina.npc: Datenbank für die Händlerin Sarina

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
local setData = behaviour.setData
local create = behaviour.create
local deleteMoney = behaviour.deleteMoney
local queue = behaviour.queue
local idle = behaviour.idle
local nop = behaviour.nop
local previous = behaviour.previous

local rules = behaviour.rules {
	{{ADDRESS, "hello$", FORCE}, {"Oh, please come in, %N. What can I do for you?"}},
	{{ADDRESS, "hi$", FORCE}, {previous}},
	{{ADDRESS, FORCE}, {idle}},
	{{BUSY, "hello$", FORCE}, {"Sorry %N, I am talking to a customer. Please stand in line.", queue}},
	{{BUSY, "hi$", FORCE}, {previous}},
	{{BUSY, FORCE}, {nop}},
	{{VANISH, FORCE}, {"Good bye."}},

	{{"bye"}, {"Good bye.", idle}},
	{{"farewell"}, {previous}},
	{{"job"}, {"I am selling equipment of all kinds. Do you need anything?"}},
	{{"name"}, {"I am Sarina. I am selling everything the adventurer needs."}},
	{{"time"}, {"It is exactly %T. Maybe you want to buy a watch?"}},
	{{"food"}, {"Sorry, I don't sell food."}},
	{{"ghostlands"}, {"Since the druids sealed that placed with their magic, rarely anyone was there. Perhaps whatever haunted that place is long gone, who knows."}},

	{{"equipment"}, {"I sell shovels, picks, scythes, fishing rods, sixpacks of worms, bags, ropes, backpacks, plates, cups, scrolls, documents, parchments, footballs, and watches. I also sell means of illumination."}},
	{{"offer"}, {previous}},
	{{"goods"}, {previous}},
	{{"do", "you", "sell"}, {previous}},
	{{"do", "you", "have"}, {previous}},
	{{"illumination"}, {"I sell torches, candlesticks, candelabra, and oil."}},

	{{"scroll"}, {setType(2815), setAmount(1), setPrice(5), "Do you want to buy a scroll for %P gold?", setTopic(1)}},
	{{"document"}, {setType(2834), setAmount(1), setPrice(12), "Do you want to buy a document for %P gold?", setTopic(1)}},
	{{"parchment"}, {setType(2835), setAmount(1), setPrice(8), "Do you want to buy a parchment for %P gold?", setTopic(1)}},
	{{"bag"}, {setType(2860), setAmount(1), setPrice(5), "Do you want to buy a bag for %P gold?", setTopic(1)}},
	{{"backpack"}, {setType(2868), setAmount(1), setPrice(20), "Do you want to buy a backpack for %P gold?", setTopic(1)}},
	{{"cup"}, {setType(2881), setAmount(1), setPrice(3), "Do you want to buy a cup for %P gold?", setTopic(1)}},
	{{"plate"}, {setType(2905), setAmount(1), setPrice(6), "Do you want to buy a plate for %P gold?", setTopic(1)}},
	{{"watch"}, {setType(2906), setAmount(1), setPrice(20), "Do you want to buy one of my high quality watches for %P gold?", setTopic(1)}},
	{{"football"}, {setType(2990), setAmount(1), setPrice(111), "Do you want to buy a football for %P gold?", setTopic(1)}},
	{{"rope"}, {setType(3003), setAmount(1), setPrice(50), "Do you want to buy a rope for %P gold?", setTopic(1)}},
	{{"scythe"}, {setType(3453), setAmount(1), setPrice(50), "Do you want to buy a scythe for %P gold?", setTopic(1)}},
	{{"pick"}, {setType(3456), setAmount(1), setPrice(50), "Do you want to buy a pick for %P gold?", setTopic(1)}},
	{{"shovel"}, {setType(3457), setAmount(1), setPrice(50), "Do you want to buy a shovel for %P gold?", setTopic(1)}},
	{{"rod"}, {setType(3483), setAmount(1), setPrice(150), "Do you want to buy a fishing rod for %P gold?", setTopic(1)}},

	{{"torch"}, {setType(2920), setAmount(1), setPrice(2), "Do you want to buy a torch for %P gold?", setTopic(1)}},
	{{"candlestick"}, {setType(2917), setAmount(1), setPrice(3), "Do you want to buy a candlestick for %P gold?", setTopic(1)}},
	{{"candelab"}, {setType(2911), setAmount(1), setPrice(8), "Do you want to buy a candelabrum for %P gold?", setTopic(1)}},
	{{"oil"}, {setType(2874), setData(7), setAmount(1), setPrice(20), "Do you want to buy oil for %P gold?", setTopic(1)}},

	{{P1, lt(1, P1), "scroll"}, {setType(2815), setAmount(P1), setPrice(mul(5, P1)), "Do you want to buy %A scrolls for %P gold?", setTopic(1)}},
	{{P1, lt(1, P1), "document"}, {setType(2834), setAmount(P1), setPrice(mul(12, P1)), "Do you want to buy %A documents for %P gold?", setTopic(1)}},
	{{P1, lt(1, P1), "parchment"}, {setType(2835), setAmount(P1), setPrice(mul(8, P1)), "Do you want to buy %A parchments for %P gold?", setTopic(1)}},
	{{P1, lt(1, P1), "bag"}, {setType(2860), setAmount(P1), setPrice(mul(5, P1)), "Do you want to buy %A bags for %P gold?", setTopic(1)}},
	{{P1, lt(1, P1), "backpack"}, {setType(2868), setAmount(P1), setPrice(mul(20, P1)), "Do you want to buy %A backpacks for %P gold?", setTopic(1)}},
	{{P1, lt(1, P1), "cup"}, {setType(2881), setAmount(P1), setPrice(mul(3, P1)), "Do you want to buy %A cups for %P gold?", setTopic(1)}},
	{{P1, lt(1, P1), "plate"}, {setType(2905), setAmount(P1), setPrice(mul(6, P1)), "Do you want to buy %A plates for %P gold?", setTopic(1)}},
	{{P1, lt(1, P1), "watch"}, {setType(2906), setAmount(P1), setPrice(mul(20, P1)), "Do you want to buy %A of my high quality watches for %P gold?", setTopic(1)}},
	{{P1, lt(1, P1), "football"}, {setType(2990), setAmount(P1), setPrice(mul(111, P1)), "Do you want to buy %A footballs for %P gold?", setTopic(1)}},
	{{P1, lt(1, P1), "rope"}, {setType(3003), setAmount(P1), setPrice(mul(50, P1)), "Do you want to buy %A ropes for %P gold?", setTopic(1)}},
	{{P1, lt(1, P1), "scythe"}, {setType(3453), setAmount(P1), setPrice(mul(50, P1)), "Do you want to buy %A scythes for %P gold?", setTopic(1)}},
	{{P1, lt(1, P1), "pick"}, {setType(3456), setAmount(P1), setPrice(mul(50, P1)), "Do you want to buy %A picks for %P gold?", setTopic(1)}},
	{{P1, lt(1, P1), "shovel"}, {setType(3457), setAmount(P1), setPrice(mul(50, P1)), "Do you want to buy %A shovels for %P gold?", setTopic(1)}},
	{{P1, lt(1, P1), "rod"}, {setType(3483), setAmount(P1), setPrice(mul(150, P1)), "Do you want to buy %A fishing rods for %P gold?", setTopic(1)}},

	{{P1, lt(1, P1), "torch"}, {setType(2920), setAmount(P1), setPrice(mul(2, P1)), "Do you want to buy %A torches for %P gold?", setTopic(1)}},
	{{P1, lt(1, P1), "candlestick"}, {setType(2917), setAmount(P1), setPrice(mul(3, P1)), "Do you want to buy %A candlesticks for %P gold?", setTopic(1)}},
	{{P1, lt(1, P1), "candelab"}, {setType(2911), setAmount(P1), setPrice(mul(8, P1)), "Do you want to buy %A candelabra for %P gold?", setTopic(1)}},
	{{P1, lt(1, P1), "oil"}, {setType(2874), setData(7), setAmount(P1), setPrice(mul(20, P1)), "Do you want to buy %A vials of oil for %P gold?", setTopic(1)}},

	{{"worm"}, {"I sell worms only in six-packs for 5 gold each, how many sixpacks of worms do you want to buy?"}},
	{{"worms"}, {previous}},
	{{P1, "worms"}, {setType(3492), setAmount(mul(P1, 6)), setPrice(mul(5, P1)), "So you want to buy some of my sixpacks with altogether %A worms for %P gold?", setTopic(1)}},
	{{P1, "six", "pack"}, {previous}},

	{{eq(TOPIC, 1), "yes", ge(COUNTMONEY, PRICE)}, {"Here you are.", deleteMoney, create(TYPE)}},
	{{eq(TOPIC, 1), "yes"}, {"Come back, when you have enough money."}},
	{{eq(TOPIC, 1)}, {"Hmm, but next time."}},
}

function onCreatureSay(npc, creature, text, situation)
	behaviour.react(npc, rules, text, situation)
end

function onCreatureDisappear(npc, creature)
	behaviour.react(npc, rules, "", NPC_SITUATION_VANISH)
end
