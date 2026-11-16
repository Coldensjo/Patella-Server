-- npcs/miraia.lua: the behaviour of the NPC Miraia, converted from
-- miraia.npc by patella-convert. The NPC is its [[npc]] entry in npcs.toml;
-- lib/npc.lua matches and runs the rules below, in the order of the file
-- (scripting.hh, "NPCs"). The comments are those of miraia.npc.
--
-- GIMUD - Graphical Interface Multi User Dungeon
-- miraia.npc: Datenbank für die Wirtin Miraia

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
	{{ADDRESS, "hello$", FORCE}, {"Daraman's blessings, %N. Welcome to the Enlightened Oasis."}},
	{{ADDRESS, "hi$", FORCE}, {previous}},
	{{ADDRESS, FORCE}, {idle}},
	{{BUSY, "hello$", FORCE}, {"I will serve you in a minute, %N.", queue}},
	{{BUSY, "hi$", FORCE}, {previous}},
	{{BUSY, FORCE}, {nop}},
	{{VANISH, FORCE}, {"Daraman's blessings. Come back soon."}},

	{{"bye"}, {"Daraman's blessings. Come back soon.", idle}},
	{{"job"}, {"I am the owner of this tavern, this oasis for the thirst, home of shadow and relaxation."}},
	{{"tavern"}, {previous}},
	{{"frodo"}, {"He's my cousin and lives in Thais."}},
	{{"name"}, {"My name is Miraia."}},
	{{"time"}, {"Don't worry about time right now."}},
	{{"caliph"}, {"Sadly the caliph does not visit this humble place."}},
	{{"kazzan"}, {previous}},
	{{"ferumbras"}, {"Travellers talked to me about his evilness. Thrice damned be his name."}},
	{{"excalibug"}, {"Some foolish adventurers seek for it in the haunted ruins of Drefia."}},
	{{"thais"}, {"Thais is a place of evil and corruption."}},
	{{"tibia"}, {"Here we are far away from the temptations of the world."}},
	{{"carlin"}, {"At least they shun alcohol over there."}},
	{{"news"}, {"Oh, just listen to the tales told by the other visitors."}},
	{{"rumour"}, {previous}},
	{{"rumor"}, {previous}},

	{{"buy"}, {"I can offer you food and drinks."}},
	{{"do", "you", "sell"}, {previous}},
	{{"do", "you", "have"}, {previous}},
	{{"offer"}, {previous}},
	{{"food"}, {"Are you looking for food? I have bread, cheese, ham, and meat."}},
	{{"drink"}, {"I can offer you lemonade, milk, and water."}},

	{{"bread"}, {setType(3600), setAmount(1), setPrice(4), "Do you want to buy a bread for %P gold?", setTopic(1)}},
	{{"cheese"}, {setType(3607), setAmount(1), setPrice(6), "Do you want to buy cheese for %P gold?", setTopic(1)}},
	{{"meat"}, {setType(3577), setAmount(1), setPrice(5), "Do you want to buy meat for %P gold?", setTopic(1)}},
	{{"ham"}, {setType(3582), setAmount(1), setPrice(8), "Do you want to buy a ham for %P gold?", setTopic(1)}},

	{{P1, lt(1, P1), "bread"}, {setType(3600), setAmount(P1), setPrice(mul(4, P1)), "Do you wanna buy %A breads for %P gold?", setTopic(1)}},
	{{P1, lt(1, P1), "cheese"}, {setType(3607), setAmount(P1), setPrice(mul(6, P1)), "Do you wanna buy %A cheese for %P gold?", setTopic(1)}},
	{{P1, lt(1, P1), "meat"}, {setType(3577), setAmount(P1), setPrice(mul(5, P1)), "Do you wanna buy %A meat for %P gold?", setTopic(1)}},
	{{P1, lt(1, P1), "ham"}, {setType(3582), setAmount(P1), setPrice(mul(8, P1)), "Do you wanna buy %A ham for %P gold?", setTopic(1)}},

	{{"lemonade"}, {setType(2880), setData(12), setAmount(1), setPrice(3), "Do you want to buy a mug of lemonade for %P gold?", setTopic(1)}},
	{{"water"}, {setType(2880), setData(1), setAmount(1), setPrice(2), "Do you want to buy a mug of water for %P gold?", setTopic(1)}},
	{{"milk"}, {setType(2880), setData(9), setAmount(1), setPrice(5), "Do you want to buy a mug of camel milk for %P gold?", setTopic(1)}},

	{{P1, lt(1, P1), "lemonade"}, {setType(2880), setData(12), setAmount(P1), setPrice(mul(3, P1)), "Do you want to buy %A mugs of lemonade for %P gold?", setTopic(1)}},
	{{P1, lt(1, P1), "water"}, {setType(2880), setData(1), setAmount(P1), setPrice(mul(2, P1)), "Do you want to buy %A mugs of water for %P gold?", setTopic(1)}},
	{{P1, lt(1, P1), "milk"}, {setType(2880), setData(9), setAmount(P1), setPrice(mul(5, P1)), "Do you want to buy %A mugs of camel milk for %P gold?", setTopic(1)}},

	{{eq(TOPIC, 1), "yes", ge(COUNTMONEY, PRICE)}, {"Here it is.", deleteMoney, create(TYPE)}},
	{{eq(TOPIC, 1), "yes"}, {"I am sorry, but you do not have enough gold."}},
	{{eq(TOPIC, 1)}, {"Maybe later."}},
}

function onCreatureSay(npc, creature, text, situation)
	behaviour.react(npc, rules, text, situation)
end

function onCreatureDisappear(npc, creature)
	behaviour.react(npc, rules, "", NPC_SITUATION_VANISH)
end
