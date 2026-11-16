-- npcs/tokel.lua: the behaviour of the NPC Tokel, converted from
-- tokel.npc by patella-convert. The NPC is its [[npc]] entry in npcs.toml;
-- lib/npc.lua matches and runs the rules below, in the order of the file
-- (scripting.hh, "NPCs"). The comments are those of tokel.npc.
--
-- GIMUD - Graphical Interface Multi User Dungeon
-- tokel.npc: Datenbank für den Bauern Tokel in Greenshore

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
	{{ADDRESS, "hello$", FORCE}, {"Hi there, %N."}},
	{{ADDRESS, "hi$", FORCE}, {previous}},
	{{ADDRESS, FORCE}, {idle}},
	{{BUSY, "hello$", FORCE}, {"Sorry %N, I am busy right now. Please wait a minute.", queue}},
	{{BUSY, "hi$", FORCE}, {previous}},
	{{BUSY, FORCE}, {nop}},
	{{VANISH, FORCE}, {"Bye."}},

	{{"bye"}, {"Bye.", idle}},
	{{"how", "are", "you"}, {"I am fine, thank you."}},
	{{"sell"}, {"I sell some food."}},
	{{"job"}, {"I am a farmer, and proud of it."}},
	{{"name"}, {"My name is Tokel."}},
	{{"time"}, {"Oh, now that you mention it: I have much left to do, please excuse me.", idle}},
	{{"help"}, {"Sorry, I have no idea how to help you."}},
	{{"monster"}, {"It's relatively peaceful here."}},
	{{"dungeon"}, {"Here are no dungeons as far as I know."}},
	{{"god"}, {"I pray to Crunor to bless our harvests."}},
	{{"king"}, {"I wish I'd be as rich as him."}},
	{{"greenshore"}, {"The soil is a bit dry and there are a lot of stones. It's very hard to work this soil."}},
	{{"magic"}, {"I know nothing but about such stuff."}},
	{{"weapon"}, {previous}},
	{{"spell"}, {previous}},
	{{"tibia"}, {"I have not seen much of it yet. I am thinking about moving to Edron soon."}},
	{{"thais"}, {"The city is too lousy and crowded for my taste."}},
	{{"edron"}, {"They say life is easy there, the soil is rich, the city save. One day I might move there."}},

	{{"offer"}, {"I can offer you bread, cheese, ham, or meat."}},
	{{"buy"}, {previous}},
	{{"food"}, {"Are you looking for food? I have bread, cheese, ham, and meat."}},

	{{"bread"}, {setType(3600), setAmount(1), setPrice(3), "Do you want to buy a bread for %P gold?", setTopic(1)}},
	{{"cheese"}, {setType(3607), setAmount(1), setPrice(5), "Do you want to buy a cheese for %P gold?", setTopic(1)}},
	{{"meat"}, {setType(3577), setAmount(1), setPrice(5), "Do you want to buy meat for %P gold?", setTopic(1)}},
	{{"ham"}, {setType(3582), setAmount(1), setPrice(8), "Do you want to buy a ham for %P gold?", setTopic(1)}},

	{{P1, lt(1, P1), "bread"}, {setType(3600), setAmount(P1), setPrice(mul(3, P1)), "Do you want to buy %A breads for %P gold?", setTopic(1)}},
	{{P1, lt(1, P1), "cheese"}, {setType(3607), setAmount(P1), setPrice(mul(5, P1)), "Do you want to buy %A cheeses for %P gold?", setTopic(1)}},
	{{P1, lt(1, P1), "meat"}, {setType(3577), setAmount(P1), setPrice(mul(5, P1)), "Do you want to buy %A meat for %P gold?", setTopic(1)}},
	{{P1, lt(1, P1), "ham"}, {setType(3582), setAmount(P1), setPrice(mul(8, P1)), "Do you want to buy %A ham for %P gold?", setTopic(1)}},

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
