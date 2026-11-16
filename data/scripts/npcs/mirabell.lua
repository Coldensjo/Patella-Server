-- npcs/mirabell.lua: the behaviour of the NPC Mirabell, converted from
-- mirabell.npc by patella-convert. The NPC is its [[npc]] entry in npcs.toml;
-- lib/npc.lua matches and runs the rules below, in the order of the file
-- (scripting.hh, "NPCs"). The comments are those of mirabell.npc.
--
-- GIMUD - Graphical Interface Multi User Dungeon
-- mirabell.npc: Datenbank für die Wirtin Mirabell

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
	{{ADDRESS, "hello$", FORCE}, {"Welcome to the Horn of Plenty, %N."}},
	{{ADDRESS, "hi$", FORCE}, {previous}},
	{{ADDRESS, FORCE}, {idle}},
	{{BUSY, "hello$", FORCE}, {"Please give me a minute, %N.", queue}},
	{{BUSY, "hi$", FORCE}, {previous}},
	{{BUSY, FORCE}, {nop}},
	{{VANISH, FORCE}, {"Come back soon, traveller."}},

	{{"bye"}, {"Come back soon, traveller.", idle}},
	{{"job"}, {"I am the owner of this tavern, which is known far beyond Edron."}},
	{{"tavern"}, {previous}},
	{{"frodo"}, {"He's my cousin and lives in Thais."}},
	{{"name"}, {"My name is Mirabell."}},
	{{"time"}, {"It is %T right now."}},
	{{"king"}, {"King Tibianus III should visit our beautiful isle more often."}},
	{{"tibianus"}, {previous}},
	{{"army"}, {"Sadly most of them are too disciplined to visit my tavern."}},
	{{"ferumbras"}, {"I heard horrible things about him."}},
	{{"excalibug"}, {"I heard the Knights of the True Blood are looking for it on this isle."}},
	{{"thais"}, {"Thais will loose influence on Edron more and more."}},
	{{"tibia"}, {"I think Edron is the best place in Tibia."}},
	{{"carlin"}, {"They should return to the Thaian realm."}},
	{{"edron"}, {"I think it is the best place in Tibia."}},
	{{"news"}, {"Oh, there are so many. Just ask other travellers like you."}},
	{{"rumors"}, {previous}},

	{{"buy"}, {"I can offer you food and drinks."}},
	{{"do", "you", "sell"}, {previous}},
	{{"do", "you", "have"}, {previous}},
	{{"food"}, {"Are you looking for food? I have bread, cheese, ham, and meat."}},
	{{"drink"}, {"I can offer you beer, wine, lemonade, and water."}},

	{{"bread"}, {setType(3600), setAmount(1), setPrice(4), "Do you want to buy a bread for %P gold?", setTopic(1)}},
	{{"cheese"}, {setType(3607), setAmount(1), setPrice(6), "Do you want to buy cheese for %P gold?", setTopic(1)}},
	{{"meat"}, {setType(3577), setAmount(1), setPrice(5), "Do you want to buy meat for %P gold?", setTopic(1)}},
	{{"ham"}, {setType(3582), setAmount(1), setPrice(8), "Do you want to buy a ham for %P gold?", setTopic(1)}},

	{{P1, lt(1, P1), "bread"}, {setType(3600), setAmount(P1), setPrice(mul(4, P1)), "Do you want to buy %A breads for %P gold?", setTopic(1)}},
	{{P1, lt(1, P1), "cheese"}, {setType(3607), setAmount(P1), setPrice(mul(6, P1)), "Do you want to buy %A cheese for %P gold?", setTopic(1)}},
	{{P1, lt(1, P1), "meat"}, {setType(3577), setAmount(P1), setPrice(mul(5, P1)), "Do you want to buy %A meat for %P gold?", setTopic(1)}},
	{{P1, lt(1, P1), "ham"}, {setType(3582), setAmount(P1), setPrice(mul(8, P1)), "Do you want to buy %A ham for %P gold?", setTopic(1)}},

	{{"lemonade"}, {setType(2880), setData(12), setAmount(1), setPrice(2), "Do you want to buy a mug of lemonade for %P gold?", setTopic(1)}},
	{{"beer"}, {setType(2880), setData(3), setAmount(1), setPrice(2), "Do you want to buy a mug of beer for %P gold?", setTopic(1)}},
	{{"wine"}, {setType(2880), setData(2), setAmount(1), setPrice(3), "Do you want to buy a mug of wine for %P gold?", setTopic(1)}},
	{{"water"}, {setType(2880), setData(1), setAmount(1), setPrice(1), "Do you want to buy a mug of water for %P gold?", setTopic(1)}},

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
