-- npcs/gail.lua: the behaviour of the NPC Gail, converted from
-- gail.npc by patella-convert. The NPC is its [[npc]] entry in npcs.toml;
-- lib/npc.lua matches and runs the rules below, in the order of the file
-- (scripting.hh, "NPCs"). The comments are those of gail.npc.
--
-- GIMUD - Graphical Interface Multi User Dungeon
-- gail.npc: Datenbank für die edelsteinhändlerin gail

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
	{{ADDRESS, "hello$", FORCE}, {"Please feel welcome."}},
	{{ADDRESS, "hi$", FORCE}, {previous}},
	{{ADDRESS, "hiho$", FORCE}, {previous}},
	{{ADDRESS, FORCE}, {idle}},

	{{BUSY, "hello$", FORCE}, {"Please wait a moment. I'll be with you within a heartbeat.", queue}},
	{{BUSY, "hi$", FORCE}, {previous}},
	{{BUSY, "hiho$", FORCE}, {previous}},
	{{BUSY, FORCE}, {nop}},
	{{VANISH, FORCE}, {"Good bye."}},

	{{"bye"}, {"Good bye.", idle}},
	{{"farewell"}, {previous}},
	{{"job"}, {"I am buying and selling gems and jewellery."}},
	{{"name"}, {"My name is Gail, nice to meet you."}},
	{{"time"}, {"I am sorry but watches are not that common here. Time has little meaning in this quite boring colony."}},
	{{"king"}, {"The Thaian monarch entrusted Venore with much responsibility to manage this colony. Sadly he did not entrust the entire control in our hands which is a constant reason for all kinds of problems."}},
	{{"venore"}, {"Venore is a beautiful city and its community is built up on commerce. Therefore it serves the needs of everyone who wants to contribute his share to the welfare of the city."}},
	{{"thais"}, {"A somewhat chaotic town. I understand how its growth dictated the shape of its community but I also see the flaws in the outcome."}},
	{{"carlin"}, {"Their independence is just insane viewed from an economic angle. A complete waste of resources."}},
	{{"edron"}, {"Another area that could and should prosper under Venoran guidance. If the king sees how we handle this settlement, he will for sure be impressed enough to allow us more freedom on that promising isle."}},
	{{"jungle"}, {"As soon as we have handled the problem with the natives, wealth is awaiting us, considering the endless resources waiting there to be taken."}},

	{{"tibia"}, {"There is so much left that still needs to be discovered in this world."}},

	{{"kazordoon"}, {"Dwarven craftsmen are some of the best in their fields. Sadly they charge quite a lot for their work and it's hard to make some profit with dwarven wares."}},
	{{"dwarves"}, {previous}},
	{{"dwarfs"}, {previous}},
	{{"ab'dendriel"}, {"Elves are bad customers. We did not manage to break into their market yet."}},
	{{"elves"}, {previous}},
	{{"elfs"}, {previous}},
	{{"darama"}, {"This continent is a challenge and an opportunity at the same time."}},
	{{"darashia"}, {"People there are somewhat strange. In my opinion their philosophy isn't good for business but the minds of people can be changed if you use the right arguments."}},
	{{"ankrahmun"}, {"Well, I admit I fail to see any profit that could safely be made by trading with that city, but even unsafe profit is a good one. Prices have to be adjusted accordingly to the course."}},
	{{"ferumbras"}, {"One evil sorcerer, no matter how powerful he might be, can have only a certain influence on the market. There are other threats to our profit that are more urgent."}},
	{{"excalibug"}, {"A knight's fairy tale."}},
	{{"apes"}, {"Those animals are one of the worst things we have to face here."}},
	{{"lizard"}, {"There must be some way to leverage their hatred towards those apes."}},
	{{"dworcs"}, {"Luckily, this horrible ugly things stay in their own territory but who knows for how long? We should never show them any sign of weakness."}},

	{{"offer"}, {"I can offer you various gems, pearls or some wonderful jewels."}},
	{{"goods"}, {previous}},
	{{"do", "you", "sell"}, {previous}},
	{{"do", "you", "have"}, {previous}},
	{{"gem"}, {"You can buy and sell small diamonds, small sapphires, small rubies, small emeralds and small amethysts."}},
	{{"pearl"}, {"There are white and black pearls you can buy or sell."}},
	{{"jewel"}, {"Currently you can purchase wedding rings, golden amulets, and ruby necklaces."}},
	{{"talon"}, {"I don't trade or work with these magic gems. It's better you ask a mage about this."}},

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
	{{eq(TOPIC, 1), "yes"}, {"Come back when you have enough money."}},
	{{eq(TOPIC, 1)}, {"Hmm, but next time."}},

	{{eq(TOPIC, 2), "yes", ge(count(TYPE), AMOUNT)}, {"Ok. Here is your money.", delete(TYPE), createMoney}},
	{{eq(TOPIC, 2), "yes"}, {"Sorry, you do not have one."}},
	{{eq(TOPIC, 2), "yes", gt(AMOUNT, 1)}, {"Sorry, you do not have that many."}},
	{{eq(TOPIC, 2)}, {"Maybe next time."}},
}

function onCreatureSay(npc, creature, text, situation)
	behaviour.react(npc, rules, text, situation)
end

function onCreatureDisappear(npc, creature)
	behaviour.react(npc, rules, "", NPC_SITUATION_VANISH)
end
