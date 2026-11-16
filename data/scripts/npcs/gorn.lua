-- npcs/gorn.lua: the behaviour of the NPC Gorn, converted from
-- gorn.npc by patella-convert. The NPC is its [[npc]] entry in npcs.toml;
-- lib/npc.lua matches and runs the rules below, in the order of the file
-- (scripting.hh, "NPCs"). The comments are those of gorn.npc.
--
-- GIMUD - Graphical Interface Multi User Dungeon
-- gorn.npc: Datenbank für den Händler Gorn

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
	{{ADDRESS, "hello$", FORCE}, {"Oh, please come in, %N. What do you need?"}},
	{{ADDRESS, "hi$", FORCE}, {previous}},
	{{ADDRESS, FORCE}, {idle}},
	{{BUSY, "hello$", FORCE}, {"Sorry %N, I am already talking to a customer. Wait a moment.", queue}},
	{{BUSY, "hi$", FORCE}, {previous}},
	{{BUSY, FORCE}, {nop}},
	{{VANISH, FORCE}, {"Good bye."}},

	{{"bye"}, {"Good bye.", idle}},
	{{"farewell"}, {previous}},
	{{"job"}, {"I am selling equipment of all kinds. Do you need anything?"}},
	{{"name"}, {"I am Gorn. My goods are known all over Tibia."}},
	{{"time"}, {"It is exactly %T. Maybe you want to buy a watch?"}},

	{{"food"}, {"If you are looking for food, go to Frodo's Hut."}},
	{{"king"}, {"The king supports Tibia's economy a lot."}},
	{{"tibianus"}, {previous}},
	{{"quentin"}, {"He advices newcomers to buy at my store. I love that guy!"}},
	{{"lynda"}, {"That's a pretty one."}},
	{{"harkath"}, {"I hardly know him."}},
	{{"army"}, {"Armies are too hierarchical for my taste."}},
	{{"ferumbras"}, {"We had a clash or two in the old days."}},
	{{"general"}, {"I don't like titles."}},
	{{"sam"}, {"Strong as an ox, could armwrestle a minotaur, I bet."}},
	{{"frodo"}, {"Frodo is a jolly fellow."}},
	{{"elane"}, {"Elane is the leader of the paladin guild."}},
	{{"paladin"}, {previous}},
	{{"muriel"}, {"You can find Muriel in the sorcerer guild."}},
	{{"sorcerer"}, {previous}},
	{{"gregor"}, {"Even the strong knights need my equipment on their travels though Tibia."}},
	{{"knight"}, {previous}},
	{{"marvik"}, {"These druids are nice people, you will find them in the east of the town."}},
	{{"druid"}, {previous}},
	{{"bozo"}, {"Bah! Go away with this bozoguy."}},
	{{"baxter"}, {"Old Baxter was a rowdy, once. In our youth we shared some adventures and women."}},
	{{"oswald"}, {"This Oswald has not enough to work and too much time to spread rumours."}},
	{{"sherry"}, {"I hardly know the McRonalds."}},
	{{"donald"}, {previous}},
	{{"mcronald"}, {previous}},
	{{"lugri"}, {"Never heared that name."}},
	{{"excalibug"}, {"I would pay thousands of gold coins for this weapon."}},
	{{"news"}, {"Taxes will increase soon, so buy as much as you can right now."}},

	{{"offer"}, {"My inventory is large, just have a look at the blackboards."}},
	{{"goods"}, {previous}},
	{{"do", "you", "sell"}, {previous}},
	{{"do", "you", "have"}, {previous}},
	{{"equipment"}, {previous}},

	{{"ammunition"}, {"Galuna sells them now in her own shop. Go and ask her about that."}},
	{{"bow"}, {previous}},
	{{"crossbow"}, {previous}},
	{{"arrow"}, {previous}},
	{{"bolt"}, {previous}},
	{{"galuna"}, {"In the past she delivered me with all the bows and arrows. She has now her own shop at the paladin guild."}},
	{{"magic"}, {"Magic? Ask a sorcerer or druid about that."}},
	{{"fluid"}, {"Find the magic shop."}},
	{{"xodet"}, {"He owns the magic shop here. But be aware: The prices are enormous."}},
	{{"book"}, {"I offer different kind of books: brown, black and small books. Which book do you want?"}},

	{{"torch"}, {setType(2920), setAmount(1), setPrice(2), "Do you want to buy a torch for %P gold?", setTopic(1)}},
	{{"bag"}, {setType(2853), setAmount(1), setPrice(5), "Do you want to buy a bag for %P gold?", setTopic(1)}},
	{{"backpack"}, {setType(2854), setAmount(1), setPrice(20), "Do you want to buy a backpack for %P gold?", setTopic(1)}},
	{{"present"}, {setType(2856), setAmount(1), setPrice(10), "Do you want to buy a present for %P gold?", setTopic(1)}},
	{{"scroll"}, {setType(2815), setAmount(1), setPrice(5), "Do you want to buy a scroll for %P gold?", setTopic(1)}},
	{{"document"}, {setType(2834), setAmount(1), setPrice(12), "Do you want to buy a document for %P gold?", setTopic(1)}},
	{{"parchment"}, {setType(2835), setAmount(1), setPrice(8), "Do you want to buy a parchment for %P gold?", setTopic(1)}},
	{{"brown", "book"}, {setType(2837), setAmount(1), setPrice(15), "Do you want to buy a brown book for %P gold?", setTopic(1)}},
	{{"black", "book"}, {setType(2838), setAmount(1), setPrice(15), "Do you want to buy a black book for %P gold?", setTopic(1)}},
	{{"small", "book"}, {setType(2839), setAmount(1), setPrice(15), "Do you want to buy a small book for %P gold?", setTopic(1)}},
	{{"bucket"}, {setType(2873), setAmount(1), setPrice(4), "Do you want to buy a bucket for %P gold?", setTopic(1)}},
	{{"bottle"}, {setType(2875), setAmount(1), setPrice(3), "Do you want to buy a bottle for %P gold?", setTopic(1)}},
	{{"mug"}, {setType(2880), setAmount(1), setPrice(4), "Do you want to buy a mug for %P gold?", setTopic(1)}},
	{{"cup"}, {setType(2881), setAmount(1), setPrice(2), "Do you want to buy a cup for %P gold?", setTopic(1)}},
	{{"jug"}, {setType(2882), setAmount(1), setPrice(10), "Do you want to buy a jug for %P gold?", setTopic(1)}},
	{{"plate"}, {setType(2905), setAmount(1), setPrice(6), "Do you want to buy a plate for %P gold?", setTopic(1)}},
	{{"watch"}, {setType(2906), setAmount(1), setPrice(20), "Do you want to buy one of my high quality watches for %P gold?", setTopic(1)}},
	{{"football"}, {setType(2990), setAmount(1), setPrice(111), "Do you want to buy a football for %P gold?", setTopic(1)}},
	{{"rope"}, {setType(3003), setAmount(1), setPrice(50), "Do you want to buy a rope for %P gold?", setTopic(1)}},
	{{"machete"}, {setType(3308), setAmount(1), setPrice(40), "Do you want to buy a machete for %P gold?", setTopic(1)}},
	{{"scythe"}, {setType(3453), setAmount(1), setPrice(50), "Do you want to buy a scythe for %P gold?", setTopic(1)}},
	{{"pick"}, {setType(3456), setAmount(1), setPrice(50), "Do you want to buy a pick for %P gold?", setTopic(1)}},
	{{"shovel"}, {setType(3457), setAmount(1), setPrice(50), "Do you want to buy a shovel for %P gold?", setTopic(1)}},
	{{"rod"}, {setType(3483), setAmount(1), setPrice(150), "Do you want to buy a fishing rod for %P gold?", setTopic(1)}},

	{{P1, lt(1, P1), "torch"}, {setType(2920), setAmount(P1), setPrice(mul(2, P1)), "Do you want to buy %A torches for %P gold?", setTopic(1)}},
	{{P1, lt(1, P1), "bag"}, {setType(2853), setAmount(P1), setPrice(mul(5, P1)), "Do you want to buy %A bags for %P gold?", setTopic(1)}},
	{{P1, lt(1, P1), "backpack"}, {setType(2854), setAmount(P1), setPrice(mul(20, P1)), "Do you want to buy %A backpacks for %P gold?", setTopic(1)}},
	{{P1, lt(1, P1), "present"}, {setType(2856), setAmount(P1), setPrice(mul(10, P1)), "Do you want to buy %A presents for %P gold?", setTopic(1)}},
	{{P1, lt(1, P1), "scroll"}, {setType(2815), setAmount(P1), setPrice(mul(5, P1)), "Do you want to buy %A scrolls for %P gold?", setTopic(1)}},
	{{P1, lt(1, P1), "document"}, {setType(2834), setAmount(P1), setPrice(mul(12, P1)), "Do you want to buy %A documents for %P gold?", setTopic(1)}},
	{{P1, lt(1, P1), "parchment"}, {setType(2835), setAmount(P1), setPrice(mul(8, P1)), "Do you want to buy %A parchments for %P gold?", setTopic(1)}},
	{{P1, lt(1, P1), "brown", "book"}, {setType(2837), setAmount(P1), setPrice(mul(15, P1)), "Do you want to buy %A brown books for %P gold?", setTopic(1)}},
	{{P1, lt(1, P1), "black", "book"}, {setType(2838), setAmount(P1), setPrice(mul(15, P1)), "Do you want to buy %A black books for %P gold?", setTopic(1)}},
	{{P1, lt(1, P1), "small", "book"}, {setType(2839), setAmount(P1), setPrice(mul(15, P1)), "Do you want to buy %A small books for %P gold?", setTopic(1)}},
	{{P1, lt(1, P1), "bucket"}, {setType(2873), setAmount(P1), setPrice(mul(4, P1)), "Do you want to buy %A buckets for %P gold?", setTopic(1)}},
	{{P1, lt(1, P1), "bottle"}, {setType(2875), setAmount(P1), setPrice(mul(3, P1)), "Do you want to buy %A bottles for %P gold?", setTopic(1)}},
	{{P1, lt(1, P1), "mug"}, {setType(2880), setAmount(P1), setPrice(mul(4, P1)), "Do you want to buy %A mugs for %P gold?", setTopic(1)}},
	{{P1, lt(1, P1), "cup"}, {setType(2881), setAmount(P1), setPrice(mul(2, P1)), "Do you want to buy %A cups for %P gold?", setTopic(1)}},
	{{P1, lt(1, P1), "jug"}, {setType(2882), setAmount(P1), setPrice(mul(10, P1)), "Do you want to buy %A jugs for %P gold?", setTopic(1)}},
	{{P1, lt(1, P1), "plate"}, {setType(2905), setAmount(P1), setPrice(mul(6, P1)), "Do you want to buy %A plates for %P gold?", setTopic(1)}},
	{{P1, lt(1, P1), "watch"}, {setType(2906), setAmount(P1), setPrice(mul(20, P1)), "Do you want to buy %A of my high quality watches for %P gold?", setTopic(1)}},
	{{P1, lt(1, P1), "football"}, {setType(2990), setAmount(P1), setPrice(mul(111, P1)), "Do you want to buy %A footballs for %P gold?", setTopic(1)}},
	{{P1, lt(1, P1), "rope"}, {setType(3003), setAmount(P1), setPrice(mul(50, P1)), "Do you want to buy %A ropes for %P gold?", setTopic(1)}},
	{{P1, lt(1, P1), "machete"}, {setType(3308), setAmount(P1), setPrice(mul(40, P1)), "Do you want to buy %A machetes for %P gold?", setTopic(1)}},
	{{P1, lt(1, P1), "scythe"}, {setType(3453), setAmount(P1), setPrice(mul(50, P1)), "Do you want to buy %A scythes for %P gold?", setTopic(1)}},
	{{P1, lt(1, P1), "pick"}, {setType(3456), setAmount(P1), setPrice(mul(50, P1)), "Do you want to buy %A picks for %P gold?", setTopic(1)}},
	{{P1, lt(1, P1), "shovel"}, {setType(3457), setAmount(P1), setPrice(mul(50, P1)), "Do you want to buy %A shovels for %P gold?", setTopic(1)}},
	{{P1, lt(1, P1), "rod"}, {setType(3483), setAmount(P1), setPrice(mul(150, P1)), "Do you want to buy %A fishing rods for %P gold?", setTopic(1)}},

	{{"worm"}, {"I sell worms only in sixpacks for 5 gold each, how many sixpacks of worms do you want to buy?"}},
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
