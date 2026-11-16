-- npcs/tezila.lua: the behaviour of the NPC Tezila, converted from
-- tezila.npc by patella-convert. The NPC is its [[npc]] entry in npcs.toml;
-- lib/npc.lua matches and runs the rules below, in the order of the file
-- (scripting.hh, "NPCs"). The comments are those of tezila.npc.
--
-- GIMUD - Graphical Interface Multi User Dungeon
-- tezila.npc: Datenbank für die Juwelierin Tezila

local behaviour = require("lib.npc")
local genBank = require("lib.npcs.gen-bank")
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
	{{ADDRESS, "hello$", FORCE}, {"Hiho, %N. Come in and look. But don't touch the exhibits, jawoll!"}},
	{{ADDRESS, "hi$", FORCE}, {previous}},
	{{ADDRESS, FORCE}, {idle}},
	{{BUSY, "hello$", FORCE}, {"Sorry %N, I am busy with a customer. Please have some patience.", queue}},
	{{BUSY, "hi$", FORCE}, {previous}},
	{{BUSY, FORCE}, {nop}},
	{{VANISH, FORCE}, {"Good bye."}},

	{{"bye"}, {"Good bye.", idle}},
	{{"farewell"}, {previous}},
	{{"job"}, {"I am a jeweller. Maybe you want to have a look at my wonderful offers. I also exchange money."}},
	{{"name"}, {"I am Tezila Gemcutter, daughter of Fire, from the Savage Axes."}},
	{{"time"}, {"It's %T."}},

	{{"offer"}, {"I can sell you glittering gems, precious pearls or some ... uhm ... jolly jewels."}},
	{{"goods"}, {previous}},
	{{"do", "you", "sell"}, {previous}},
	{{"do", "you", "have"}, {previous}},
	{{"gem"}, {"You can buy and sell small diamonds, sapphires, rubies, emeralds, and amethysts."}},
	{{"pearl"}, {"I have white and black pearls you can buy, but you can also sell me some."}},
	{{"jewel"}, {"You can purchase our fine dwarfish wares like wedding rings, golden amulets, and ruby necklaces."}},
	{{"talon"}, {"I am suspicious of these magic gems. Better you ask some mages about this."}},

	{{"wedding", "ring"}, {setType(3004), setAmount(1), setPrice(990), "Do you want to buy a wedding ring for %P gold?", setTopic(1)}},
	{{"golden", "amulet"}, {setType(3013), setAmount(1), setPrice(6600), "Do you want to buy a golden amulet for %P gold?", setTopic(1)}},
	{{"ruby", "necklace"}, {setType(3016), setAmount(1), setPrice(3560), "Do you want to buy a ruby necklace for %P gold?", setTopic(1)}},
	{{"white", "pearl"}, {setType(3026), setAmount(1), setPrice(320), "Do you want to buy a white pearl for %P gold?", setTopic(1)}},
	{{"black", "pearl"}, {setType(3027), setAmount(1), setPrice(560), "Do you want to buy a black pearl for %P gold?", setTopic(1)}},
	{{"small", "diamond"}, {setType(3028), setAmount(1), setPrice(600), "Do you want to buy a small diamond for %P gold?", setTopic(1)}},
	{{"small", "sapphire"}, {setType(3029), setAmount(1), setPrice(500), "Do you want to buy a small sapphire for %P gold?", setTopic(1)}},
	{{"small", "ruby"}, {setType(3030), setAmount(1), setPrice(500), "Do you want to buy a small ruby for %P gold?", setTopic(1)}},
	{{"small", "emerald"}, {setType(3032), setAmount(1), setPrice(500), "Do you want to buy a small emerald for %P gold?", setTopic(1)}},
	{{"small", "amethyst"}, {setType(3033), setAmount(1), setPrice(400), "Do you want to buy a small amethyst for %P gold?", setTopic(1)}},

	{{P1, lt(1, P1), "wedding", "ring"}, {setType(3004), setAmount(P1), setPrice(mul(990, P1)), "Do you want to buy %A wedding rings for %P gold?", setTopic(1)}},
	{{P1, lt(1, P1), "golden", "amulet"}, {setType(3013), setAmount(P1), setPrice(mul(6600, P1)), "Do you want to buy %A golden amulets for %P gold?", setTopic(1)}},
	{{P1, lt(1, P1), "ruby", "necklace"}, {setType(3016), setAmount(P1), setPrice(mul(3560, P1)), "Do you want to buy %A ruby necklaces for %P gold?", setTopic(1)}},
	{{P1, lt(1, P1), "white", "pearl"}, {setType(3026), setAmount(P1), setPrice(mul(320, P1)), "Do you want to buy %A white pearls for %P gold?", setTopic(1)}},
	{{P1, lt(1, P1), "black", "pearl"}, {setType(3027), setAmount(P1), setPrice(mul(560, P1)), "Do you want to buy %A black pearls for %P gold?", setTopic(1)}},
	{{P1, lt(1, P1), "small", "diamond"}, {setType(3028), setAmount(P1), setPrice(mul(600, P1)), "Do you want to buy %A small diamonds for %P gold?", setTopic(1)}},
	{{P1, lt(1, P1), "small", "sapphire"}, {setType(3029), setAmount(P1), setPrice(mul(500, P1)), "Do you want to buy %A small sapphires for %P gold?", setTopic(1)}},
	{{P1, lt(1, P1), "small", "rub"}, {setType(3030), setAmount(P1), setPrice(mul(500, P1)), "Do you want to buy %A small rubies for %P gold?", setTopic(1)}},
	{{P1, lt(1, P1), "small", "emerald"}, {setType(3032), setAmount(P1), setPrice(mul(500, P1)), "Do you want to buy %A small emeralds for %P gold?", setTopic(1)}},
	{{P1, lt(1, P1), "small", "amethyst"}, {setType(3033), setAmount(P1), setPrice(mul(400, P1)), "Do you want to buy %A small amethysts for %P gold?", setTopic(1)}},

	{{"sell", "white", "pearl"}, {setType(3026), setAmount(1), setPrice(160), "Do you want to sell a white pearl for %P gold?", setTopic(2)}},
	{{"sell", "black", "pearl"}, {setType(3027), setAmount(1), setPrice(280), "Do you want to sell a black pearl for %P gold?", setTopic(2)}},
	{{"sell", "small", "diamond"}, {setType(3028), setAmount(1), setPrice(300), "Do you want to sell a small diamond for %P gold?", setTopic(2)}},
	{{"sell", "small", "sapphire"}, {setType(3029), setAmount(1), setPrice(250), "Do you want to sell a small sapphire for %P gold?", setTopic(2)}},
	{{"sell", "small", "ruby"}, {setType(3030), setAmount(1), setPrice(250), "Do you want to sell a small ruby for %P gold?", setTopic(2)}},
	{{"sell", "small", "emerald"}, {setType(3032), setAmount(1), setPrice(250), "Do you want to sell a small emerald for %P gold?", setTopic(2)}},
	{{"sell", "small", "amethyst"}, {setType(3033), setAmount(1), setPrice(200), "Do you want to sell a small amethyst for %P gold?", setTopic(2)}},

	{{"sell", P1, lt(1, P1), "white", "pearl"}, {setType(3026), setAmount(P1), setPrice(mul(160, P1)), "Do you want to sell %A white pearls for %P gold?", setTopic(2)}},
	{{"sell", P1, lt(1, P1), "black", "pearl"}, {setType(3027), setAmount(P1), setPrice(mul(280, P1)), "Do you want to sell %A black pearls for %P gold?", setTopic(2)}},
	{{"sell", P1, lt(1, P1), "small", "diamond"}, {setType(3028), setAmount(P1), setPrice(mul(300, P1)), "Do you want to sell %A small diamonds for %P gold?", setTopic(2)}},
	{{"sell", P1, lt(1, P1), "small", "sapphire"}, {setType(3029), setAmount(P1), setPrice(mul(250, P1)), "Do you want to sell %A small sapphires for %P gold?", setTopic(2)}},
	{{"sell", P1, lt(1, P1), "small", "rub"}, {setType(3030), setAmount(P1), setPrice(mul(250, P1)), "Do you want to sell %A small rubies for %P gold?", setTopic(2)}},
	{{"sell", P1, lt(1, P1), "small", "emerald"}, {setType(3032), setAmount(P1), setPrice(mul(250, P1)), "Do you want to sell %A small emeralds for %P gold?", setTopic(2)}},
	{{"sell", P1, lt(1, P1), "small", "amethyst"}, {setType(3033), setAmount(P1), setPrice(mul(200, P1)), "Do you want to sell %A small amethysts for %P gold?", setTopic(2)}},

	{{eq(TOPIC, 1), "yes", ge(COUNTMONEY, PRICE)}, {"Here you are.", deleteMoney, create(TYPE)}},
	{{eq(TOPIC, 1), "yes"}, {"Come back, when you have enough money."}},
	{{eq(TOPIC, 1)}, {"Hmm, but next time."}},

	{{eq(TOPIC, 2), "yes", ge(count(TYPE), AMOUNT)}, {"Ok. Here is your money.", delete(TYPE), createMoney}},
	{{eq(TOPIC, 2), "yes"}, {"Sorry, you do not have one."}},
	{{eq(TOPIC, 2), "yes", gt(AMOUNT, 1)}, {"Sorry, you do not have that many."}},
	{{eq(TOPIC, 2)}, {"Maybe next time."}},

	genBank,
}

function onCreatureSay(npc, creature, text, situation)
	behaviour.react(npc, rules, text, situation)
end

function onCreatureDisappear(npc, creature)
	behaviour.react(npc, rules, "", NPC_SITUATION_VANISH)
end
