-- npcs/tesha.lua: the behaviour of the NPC Tesha, converted from
-- tesha.npc by patella-convert. The NPC is its [[npc]] entry in npcs.toml;
-- lib/npc.lua matches and runs the rules below, in the order of the file
-- (scripting.hh, "NPCs"). The comments are those of tesha.npc.
--
-- GIMUD - Graphical Interface Multi User Dungeon
-- tesha.npc: Datenbank für die pyramidenhändlerin Tesha

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
	{{ADDRESS, "hello$", FORCE}, {"Be mourned pilgrim in flesh."}},
	{{ADDRESS, "hi$", FORCE}, {"Be mourned pilgrim in flesh."}},
	{{ADDRESS, "be", "mourned", "pilgrim", "flesh", FORCE}, {"Be mourned pilgrim in flesh."}},
	{{ADDRESS, FORCE}, {idle}},

	{{BUSY, "hello$", FORCE}, {"Mourned %N, know that I am busy.", queue}},
	{{BUSY, "hi$", FORCE}, {previous}},
	{{BUSY, "be", "mourned", "pilgrim", "flesh", FORCE}, {previous}},
	{{BUSY, FORCE}, {nop}},
	{{VANISH, FORCE}, {"May enlightenment be your path."}},

	{{"bye"}, {"May enlightenment be your path.", idle}},
	{{"farewell"}, {previous}},
	{{"job"}, {"I sell and buy gems and jewelry."}},
	{{"name"}, {"I am the mourned Tesha."}},
	{{"time"}, {"Time is yet another burden that lies heavy on our mortal bodies."}},
	{{"temple"}, {"The temple can offer us guidance and solace in our mortal existence."}},
	{{"pharaoh"}, {"He is the benevolent father of this nation. Blessed be our saviour."}},
	{{"oldpharaoh"}, {"This poor man could not comprehend his son's wisdom. Perhaps he has spelled his own eternal doom."}},
	{{"scarab"}, {"The priests say they are sacred beings, although ... <whispers>I find them scary!"}},
	{{"chosen"}, {"I can only hope my humble work for our community and for the temple will make me worthy one day to be elevated to the rank of a chosen one. One to whom the path of ascension is opened up through undeath."}},

	{{"tibia"}, {"The world is so huge, and I have seen so little. Perhaps if I am chosen one day I will travel and see it all."}},
	{{"carlin"}, {"Those citites are so far away. So far that the enlightened preachings of our divine pharaoh cannot reach those poor misguided souls."}},
	{{"thais"}, {previous}},
	{{"edron"}, {previous}},
	{{"venore"}, {previous}},
	{{"kazordoon"}, {"Dwarves have a strong Akh. This makes them arrogant and deaf to the true creed."}},
	{{"dwarves"}, {previous}},
	{{"dwarfes"}, {previous}},
	{{"ab'dendriel"}, {"Most elves lack the sincerity to strive for ascension. At least that's what the priests are telling us."}},
	{{"elves"}, {previous}},
	{{"elfes"}, {previous}},
	{{"darama"}, {"In the desert the lines of life and death are clearly drawn. Because of this it is easier for us, its children, to focus on them. In the jungle those lines are fuzzy and blurred, and people easily fall victim to temptation."}},
	{{"darashia"}, {"Those poor souls there might still be saved if only they listened."}},
	{{"daraman"}, {"He was a great man. If he had left his mortal existence behind he might have become one of the greatest prophets of the true faith, second only to the pharaoh himself."}},
	{{"Ankrahmun"}, {"This city is both a refuge and centre of learning for the believers of the true faith taught by his divine majesty the pharaoh."}},

	{{"pharaoh"}, {"The pharaoh has such amazing patience with us puny mortals. He is truly a caring father of this nation."}},
	{{"mortality"}, {"Mortality can be overcome. It is a sickness, but it can be cured through undeath."}},
	{{"false", "gods"}, {"These greedy beings are trying to devour us all. May the pharaoh thwart their evil plans and free us from their reign of terror!"}},
	{{"ascension"}, {"Oh, I am not asking for much, you know. I mean, I really don't have to be a god or something. All I wish for is a bit of the wisdom that comes with ascension."}},
	{{"Akh'rah", "Uthun"}, {"I don't really understand this concept, but from what I know it is the three components that make up every being."}},
	{{"Akh"}, {"Our body. The only physical part of the Akh'rah Uthun."}},

	{{"undead"}, {"Undeath is the reward for a life of faith and service."}},
	{{"undeath"}, {previous}},
	{{"Rah"}, {"The Rah is our essence. The spiritual bond that keep the other parts of the Akh'rah Uthun together."}},
	{{"uthun"}, {"The Uthun is all we learned in life."}},
	{{"mourn"}, {"The dead mourn our tempted existence, and we mourn ourselves."}},
	{{"arena"}, {"Look for it in the eastern part of the city."}},
	{{"palace"}, {"Isn't the palace magnificent to behold? It is so impressive!"}},

	{{"offer"}, {"I can offer you various gems, pearls or some wonderful jewels. I also change money."}},
	{{"goods"}, {previous}},
	{{"do", "you", "sell"}, {previous}},
	{{"do", "you", "have"}, {previous}},

	{{"gem"}, {"You can buy and sell small diamonds, small sapphires, small rubies, small emeralds, and small amethysts."}},
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
