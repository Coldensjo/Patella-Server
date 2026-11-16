-- npcs/shiantis.lua: the behaviour of the NPC Shiantis, converted from
-- shiantis.npc by patella-convert. The NPC is its [[npc]] entry in npcs.toml;
-- lib/npc.lua matches and runs the rules below, in the order of the file
-- (scripting.hh, "NPCs"). The comments are those of shiantis.npc.
--
-- GIMUD - Graphical Interface Multi User Dungeon
-- shiantis.npc: Datenbank für die Händlerin Shiantis in Venore

local behaviour = require("lib.npc")
local genTFurnitureDecorationS = require("lib.npcs.gen-t-furniture-decoration-s")
local ADDRESS = behaviour.ADDRESS
local BUSY = behaviour.BUSY
local VANISH = behaviour.VANISH
local PREMIUM = behaviour.PREMIUM
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
	{{ADDRESS, "hello$", FORCE}, {"Welcome %N. What is your need today?"}},
	{{ADDRESS, "hi$", FORCE}, {previous}},
	{{ADDRESS, FORCE}, {idle}},
	{{BUSY, "hello$", FORCE}, {"Excuse me, %N, I am already talking to another customer. Wait just a moment.", queue}},
	{{BUSY, "hi$", FORCE}, {previous}},
	{{BUSY, FORCE}, {nop}},
	{{VANISH, FORCE}, {"Good bye."}},

	{{"bye"}, {"Good bye.", idle}},
	{{"farewell"}, {previous}},
	{{"job"}, {"I am selling several kinds of equipment and decoration. What is your need?"}},
	{{"name"}, {"I am Shiantis."}},
	{{"time"}, {"I won't tell you for free, but maybe you want to buy a watch?"}},
	{{"king"}, {"I would love to see the royal taxes lowered."}},
	{{"tibianus"}, {previous}},
	{{"army"}, {"I think it's needed for protection. We pay enough taxes for this."}},
	{{"ferumbras"}, {"Ferumbras dolls were not the saleshit we expected. Sold all stock to a strange guy who bought a bunch of needles, too."}},
	{{"excalibug"}, {"Sorry, we run out of stock. I expect another load of wodden excalibug simulacra to arrive next week."}},
	{{"news"}, {"I heard the merchants will petition the king to lower the taxes."}},
	{{"tax"}, {previous}},

	{{"offer"}, {"As you can see, our inventory is large, just have a look."}},
	{{"goods"}, {"At this booth we sell containers, decoration, illumination, paperware, footballs, and watches."}},
	{{"do", "you", "sell"}, {previous}},
	{{"do", "you", "have"}, {previous}},
	{{"equipment"}, {previous}},
	{{"containers"}, {"In that department we offer bags, backpacks, and present boxes."}},
	{{"illumination"}, {"In that department we offer torches, candlesticks, candelabra, oil and coal basins."}},
	{{"paperware"}, {"In that department we offer scrolls, documents, parchments, and books."}},
	{{"book"}, {"I offer different kind of books: brown, black and small books. Which book do you want?"}},
	{{"magic"}, {"You will have to visit that spooky magic market for that stuff."}},
	{{"fluid"}, {previous}},

	{{"torch"}, {setType(2920), setAmount(1), setPrice(2), "Do you wanna buy a torch for %P gold?", setTopic(1)}},
	{{"candelabr"}, {setType(2911), setAmount(1), setPrice(8), "Do you wanna buy a candelabrum for %P gold?", setTopic(1)}},
	{{"candlestick"}, {setType(2917), setAmount(1), setPrice(2), "Do you want to buy a candlestick for %P gold?", setTopic(1)}},
	{{"oil"}, {setType(2874), setData(7), setAmount(1), setPrice(20), "Do you wanna buy oil for %P gold?", setTopic(2)}},
	{{"coal", "basin"}, {setType(2806), setAmount(1), setPrice(25), "Do you want to buy a coal basin for %P gold?", setTopic(3)}},
	{{"bag"}, {setType(2859), setAmount(1), setPrice(5), "Do you want to buy a bag for %P gold?", setTopic(1)}},
	{{"backpack"}, {setType(2867), setAmount(1), setPrice(20), "Do you want to buy a backpack for %P gold?", setTopic(1)}},
	{{"present"}, {setType(2856), setAmount(1), setPrice(10), "Do you want to buy a present for %P gold?", setTopic(1)}},
	{{"scroll"}, {setType(2815), setAmount(1), setPrice(5), "Do you want to buy a scroll for %P gold?", setTopic(1)}},
	{{"document"}, {setType(2834), setAmount(1), setPrice(12), "Do you want to buy a document for %P gold?", setTopic(1)}},
	{{"parchment"}, {setType(2835), setAmount(1), setPrice(8), "Do you want to buy a parchment for %P gold?", setTopic(1)}},
	{{"brown", "book"}, {setType(2837), setAmount(1), setPrice(15), "Do you want to buy a brown book for %P gold?", setTopic(1)}},
	{{"black", "book"}, {setType(2838), setAmount(1), setPrice(15), "Do you want to buy a black book for %P gold?", setTopic(1)}},
	{{"small", "book"}, {setType(2839), setAmount(1), setPrice(15), "Do you want to buy a small book for %P gold?", setTopic(1)}},
	{{"watch"}, {setType(2906), setAmount(1), setPrice(20), "Do you want to buy one of my high quality watches for %P gold?", setTopic(1)}},
	{{"football"}, {setType(2990), setAmount(1), setPrice(111), "Do you want to buy a football for %P gold?", setTopic(1)}},

	{{P1, lt(1, P1), "torch"}, {setType(2920), setAmount(P1), setPrice(mul(2, P1)), "Do you wanna buy %A torches for %P gold?", setTopic(1)}},
	{{P1, lt(1, P1), "candelabr"}, {setType(2911), setAmount(P1), setPrice(mul(8, P1)), "Do you wanna buy %A candelabra for %P gold?", setTopic(1)}},
	{{P1, lt(1, P1), "candlestick"}, {setType(2917), setAmount(P1), setPrice(mul(2, P1)), "Do you want to buy %A candlesticks for %P gold?", setTopic(1)}},
	{{P1, lt(1, P1), "oil"}, {setType(2874), setData(7), setAmount(P1), setPrice(mul(20, P1)), "Do you wanna buy %A vials of oil for %P gold?", setTopic(2)}},
	{{P1, lt(1, P1), "coal", "basin"}, {setType(3510), setAmount(P1), setPrice(mul(25, P1)), "Do you want to buy %A coal basins for %P gold?", setTopic(3)}},
	{{P1, lt(1, P1), "bag"}, {setType(2859), setAmount(P1), setPrice(mul(5, P1)), "Do you want to buy %A bags for %P gold?", setTopic(1)}},
	{{P1, lt(1, P1), "backpack"}, {setType(2867), setAmount(P1), setPrice(mul(20, P1)), "Do you want to buy %A backpacks for %P gold?", setTopic(1)}},
	{{P1, lt(1, P1), "present"}, {setType(2856), setAmount(P1), setPrice(mul(10, P1)), "Do you want to buy %A presents for %P gold?", setTopic(1)}},
	{{P1, lt(1, P1), "scroll"}, {setType(2815), setAmount(P1), setPrice(mul(5, P1)), "Do you want to buy %A scrolls for %P gold?", setTopic(1)}},
	{{P1, lt(1, P1), "document"}, {setType(2834), setAmount(P1), setPrice(mul(12, P1)), "Do you want to buy %A documents for %P gold?", setTopic(1)}},
	{{P1, lt(1, P1), "parchment"}, {setType(2835), setAmount(P1), setPrice(mul(8, P1)), "Do you want to buy %A parchments for %P gold?", setTopic(1)}},
	{{P1, lt(1, P1), "brown", "book"}, {setType(2837), setAmount(P1), setPrice(mul(15, P1)), "Do you want to buy %A brown books for %P gold?", setTopic(1)}},
	{{P1, lt(1, P1), "black", "book"}, {setType(2838), setAmount(P1), setPrice(mul(15, P1)), "Do you want to buy %A black books for %P gold?", setTopic(1)}},
	{{P1, lt(1, P1), "small", "book"}, {setType(2839), setAmount(P1), setPrice(mul(15, P1)), "Do you want to buy %A small books for %P gold?", setTopic(1)}},
	{{P1, lt(1, P1), "watch"}, {setType(2906), setAmount(P1), setPrice(mul(20, P1)), "Do you want to buy %A of my high quality watches for %P gold?", setTopic(1)}},
	{{P1, lt(1, P1), "football"}, {setType(2990), setAmount(P1), setPrice(mul(111, P1)), "Do you want to buy %A footballs for %P gold?", setTopic(1)}},

	{{eq(TOPIC, 1), "yes", ge(COUNTMONEY, PRICE)}, {"Here you are.", deleteMoney, create(TYPE)}},
	{{eq(TOPIC, 1), "yes"}, {"Come back, when you have enough money."}},
	{{eq(TOPIC, 1)}, {"Hmm, but next time."}},

	{{eq(TOPIC, 2), "yes", ge(COUNTMONEY, PRICE)}, {"Ok, take it. There is a deposit of 5 gold on the vial.", deleteMoney, create(TYPE)}},
	{{eq(TOPIC, 2), "yes"}, {"That's not funny!"}},
	{{eq(TOPIC, 2)}, {"Then not."}},

	{{eq(TOPIC, 3), "yes", PREMIUM, ge(COUNTMONEY, PRICE)}, {"Here you are.", deleteMoney, create(TYPE)}},
	{{eq(TOPIC, 3), "yes", ge(COUNTMONEY, PRICE)}, {"I'm sorry, but you need a premium account if you want to buy furniture."}},
	{{eq(TOPIC, 3), "yes"}, {"Come back, when you have enough money."}},
	{{eq(TOPIC, 3)}, {"Hmm, but I'm sure, it would fit nicely into your house."}},

	{{"deposit"}, {"I will give you 5 gold for every empty vial. Ok?", setData(0), setTopic(4)}},
	{{"vial"}, {previous}},
	{{"flask"}, {previous}},

	{{eq(TOPIC, 4), "yes", gt(count(2874), 0)}, {setAmount(count(2874)), setPrice(mul(AMOUNT, 5)), "Here you are ... %P gold.", delete(2874), createMoney}},
	{{eq(TOPIC, 4), "yes"}, {"You don't have any empty vials."}},
	{{eq(TOPIC, 4)}, {"Hmm, but please keep our town litter free."}},

	genTFurnitureDecorationS,
}

function onCreatureSay(npc, creature, text, situation)
	behaviour.react(npc, rules, text, situation)
end

function onCreatureDisappear(npc, creature)
	behaviour.react(npc, rules, "", NPC_SITUATION_VANISH)
end
