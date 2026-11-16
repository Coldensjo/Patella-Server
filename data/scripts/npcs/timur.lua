-- npcs/timur.lua: the behaviour of the NPC Timur, converted from
-- timur.npc by patella-convert. The NPC is its [[npc]] entry in npcs.toml;
-- lib/npc.lua matches and runs the rules below, in the order of the file
-- (scripting.hh, "NPCs"). The comments are those of timur.npc.
--
-- GIMUD - Graphical Interface Multi User Dungeon
-- timur.npc: Datenbank für den Händler Timur auf der Insel Fibula

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
local create = behaviour.create
local delete = behaviour.delete
local createMoney = behaviour.createMoney
local deleteMoney = behaviour.deleteMoney
local queue = behaviour.queue
local idle = behaviour.idle
local nop = behaviour.nop
local previous = behaviour.previous

local rules = behaviour.rules {
	{{ADDRESS, "hello$", "timur", FORCE}, {"Hello, %N. Come in and buy."}},
	{{ADDRESS, "hi$", "timur", FORCE}, {previous}},
	{{ADDRESS, "hello$", FORCE}, {"Welcome to my little shop, adventurer! First read my blackboards."}},
	{{ADDRESS, "hi$", FORCE}, {previous}},
	{{ADDRESS, FORCE}, {idle}},
	{{BUSY, "hello$", FORCE}, {"Sorry, I am already talking to a customer, %N. Wait, please.", queue}},
	{{BUSY, "hi$", FORCE}, {previous}},
	{{BUSY, FORCE}, {nop}},
	{{VANISH, FORCE}, {"Bye, bye."}},

	{{"bye"}, {"Bye, bye.", idle}},
	{{"farewell"}, {previous}},
	{{"job"}, {"I am selling equipment. Do you want something?"}},
	{{"name"}, {"I am Timur. Sorry, I have not much equipment for sale. The business is running low."}},
	{{"timur"}, {previous}},
	{{"time"}, {"I am sorry, I have no watch."}},
	{{"fibula"}, {"It's a very nice isle. But we don't have enough weapons to defeat the many wolves."}},
	{{"wolf"}, {"They are everywhere around the village."}},
	{{"wolves"}, {previous}},
	{{"helmet"}, {"I can sell you a viking helmet in a very good quality."}},
	{{"food"}, {"If you are looking for food, buy a rod and go fishing."}},
	{{"weapon"}, {"At the moment I have no weapons to offer. Weapons are very rare on this isle, so I have to buy a few."}},
	{{"bow"}, {"We have too few bows on this isle for our hunters."}},
	{{"crossbow"}, {"We have too few crossbows on this isle for our hunters."}},
	{{"fluid"}, {"The magic shops have a monopole on fluids now... argl!"}},
	{{"magic"}, {previous}},

	{{"equipment"}, {"I sell torches, scrolls, documents, parchments, ropes, fishing rods, sixpacks of worms, arrows, bolts, and a nice helmet."}},
	{{"goods"}, {previous}},
	{{"do", "you", "sell"}, {previous}},
	{{"do", "you", "have"}, {previous}},
	{{"offer"}, {previous}},
	{{"ammo"}, {"I have arrows and bolts in this shop."}},
	{{"ammunition"}, {previous}},

	{{"torch"}, {setType(2920), setAmount(1), setPrice(3), "Do you want to buy a torch for %P gold?", setTopic(1)}},
	{{"scroll"}, {setType(2815), setAmount(1), setPrice(10), "Do you want to buy a scroll for %P gold?", setTopic(1)}},
	{{"document"}, {setType(2834), setAmount(1), setPrice(12), "Do you want to buy a document for %P gold?", setTopic(1)}},
	{{"parchment"}, {setType(2835), setAmount(1), setPrice(8), "Do you want to buy a parchment for %P gold?", setTopic(1)}},
	{{"rope"}, {setType(3003), setAmount(1), setPrice(65), "Do you want to buy a rope for %P gold?", setTopic(1)}},
	{{"rod"}, {setType(3483), setAmount(1), setPrice(170), "Do you want to buy a fishing rod for %P gold?", setTopic(1)}},
	{{"arrow"}, {setType(3447), setAmount(1), setPrice(3), "Do you want to buy an arrow for %P gold?", setTopic(1)}},
	{{"bolt"}, {setType(3446), setAmount(1), setPrice(4), "Do you want to buy a bolt for %P gold?", setTopic(1)}},
	{{"viking", "helmet"}, {setType(3367), setAmount(1), setPrice(265), "Do you want to buy a viking helmet for %P gold?", setTopic(1)}},

	{{P1, lt(1, P1), "torch"}, {setType(2920), setAmount(P1), setPrice(mul(3, P1)), "Do you want to buy %A torches for %P gold?", setTopic(1)}},
	{{P1, lt(1, P1), "scroll"}, {setType(2815), setAmount(P1), setPrice(mul(10, P1)), "Do you want to buy %A scrolls for %P gold?", setTopic(1)}},
	{{P1, lt(1, P1), "document"}, {setType(2834), setAmount(P1), setPrice(mul(12, P1)), "Do you want to buy %A documents for %P gold?", setTopic(1)}},
	{{P1, lt(1, P1), "parchment"}, {setType(2835), setAmount(P1), setPrice(mul(8, P1)), "Do you want to buy %A parchments for %P gold?", setTopic(1)}},
	{{P1, lt(1, P1), "rope"}, {setType(3003), setAmount(P1), setPrice(mul(65, P1)), "Do you want to buy %A ropes for %P gold?", setTopic(1)}},
	{{P1, lt(1, P1), "rod"}, {setType(3483), setAmount(P1), setPrice(mul(170, P1)), "Do you want to buy %A fishing rods for %P gold?", setTopic(1)}},
	{{P1, lt(1, P1), "arrow"}, {setType(3447), setAmount(P1), setPrice(mul(3, P1)), "Do you want to buy %A arrows for %P gold?", setTopic(1)}},
	{{P1, lt(1, P1), "bolt"}, {setType(3446), setAmount(P1), setPrice(mul(4, P1)), "Do you want to buy %A bolts for %P gold?", setTopic(1)}},
	{{P1, lt(1, P1), "viking", "helmet"}, {setType(3367), setAmount(P1), setPrice(mul(265, P1)), "Do you want to buy %A viking helmets for %P gold?", setTopic(1)}},

	{{"worm"}, {"I sell worms only in sixpacks for 5 gold each, how many sixpacks of worms do you want to buy?"}},
	{{"worms"}, {previous}},
	{{P1, "worms"}, {setType(3492), setAmount(mul(P1, 6)), setPrice(mul(5, P1)), "So you want to buy some of my sixpacks with altogether %A worms for %P gold?", setTopic(1)}},
	{{P1, "six", "pack"}, {previous}},

	{{"sell", "bow"}, {setType(3350), setAmount(1), setPrice(130), "Do you want to sell a bow for %P gold?", setTopic(2)}},
	{{"sell", "crossbow"}, {setType(3349), setAmount(1), setPrice(160), "Do you want to sell a crossbow for %P gold?", setTopic(2)}},
	{{"sell", "viking", "helmet"}, {setType(3367), setAmount(1), setPrice(66), "Do you want to sell a viking helmet for %P gold?", setTopic(2)}},
	{{"sell", "mace"}, {setType(3286), setAmount(1), setPrice(30), "Do you want to sell a mace for %P gold?", setTopic(2)}},
	{{"sell", "hatchet"}, {setType(3276), setAmount(1), setPrice(25), "Do you want to sell a hatchet for %P gold?", setTopic(2)}},

	{{"sell", P1, lt(1, P1), "bow"}, {setType(3350), setAmount(P1), setPrice(mul(130, P1)), "Do you want to sell %A bows for %P gold?", setTopic(2)}},
	{{"sell", P1, lt(1, P1), "crossbow"}, {setType(3349), setAmount(P1), setPrice(mul(160, P1)), "Do you want to sell %A crossbows for %P gold?", setTopic(2)}},
	{{"sell", P1, lt(1, P1), "viking", "helmet"}, {setType(3367), setAmount(P1), setPrice(mul(66, P1)), "Do you want to sell %A viking helmets for %P gold?", setTopic(2)}},
	{{"sell", P1, lt(1, P1), "mace"}, {setType(3286), setAmount(P1), setPrice(mul(30, P1)), "Do you want to sell %A maces for %P gold?", setTopic(2)}},
	{{"sell", P1, lt(1, P1), "hatchet"}, {setType(3276), setAmount(P1), setPrice(mul(25, P1)), "Do you want to sell %A hatchets for %P gold?", setTopic(2)}},

	{{eq(TOPIC, 1), "yes", ge(COUNTMONEY, PRICE)}, {"Thank you. Nice to do business with you.", deleteMoney, create(TYPE)}},
	{{eq(TOPIC, 1), "yes"}, {"You've not enough money."}},
	{{eq(TOPIC, 1)}, {"Hmm, maybe next time."}},

	{{eq(TOPIC, 2), "yes", ge(count(TYPE), AMOUNT)}, {"Ok. Here is your money.", delete(TYPE), createMoney}},
	{{eq(TOPIC, 2), "yes"}, {"Hey, you do not have one."}},
	{{eq(TOPIC, 2), "yes", gt(AMOUNT, 1)}, {"Sorry, you do not have so many."}},
	{{eq(TOPIC, 2)}, {"Maybe next time."}},
}

function onCreatureSay(npc, creature, text, situation)
	behaviour.react(npc, rules, text, situation)
end

function onCreatureDisappear(npc, creature)
	behaviour.react(npc, rules, "", NPC_SITUATION_VANISH)
end
