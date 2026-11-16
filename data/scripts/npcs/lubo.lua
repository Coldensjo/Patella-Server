-- npcs/lubo.lua: the behaviour of the NPC Lubo, converted from
-- lubo.npc by patella-convert. The NPC is its [[npc]] entry in npcs.toml;
-- lib/npc.lua matches and runs the rules below, in the order of the file
-- (scripting.hh, "NPCs"). The comments are those of lubo.npc.
--
-- GIMUD - Graphical Interface Multi User Dungeon
-- lubo.npc: Datenbank fuer Lubo, den Haendler im Abenteurer-Laden

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
local questValue = behaviour.questValue
local mul = behaviour.mul
local setTopic = behaviour.setTopic
local setPrice = behaviour.setPrice
local setAmount = behaviour.setAmount
local setType = behaviour.setType
local setData = behaviour.setData
local setQuestValue = behaviour.setQuestValue
local create = behaviour.create
local deleteMoney = behaviour.deleteMoney
local queue = behaviour.queue
local idle = behaviour.idle
local nop = behaviour.nop
local previous = behaviour.previous

local rules = behaviour.rules {
	{{ADDRESS, "hello$", FORCE}, {"Welcome to my adventurer shop, %N! What do you need?"}},
	{{ADDRESS, "hi$", FORCE}, {previous}},
	{{ADDRESS, FORCE}, {idle}},
	{{BUSY, "hello$", FORCE}, {"Sorry %N, I am already talking to a customer. Please wait for your turn.", queue}},
	{{BUSY, "hi$", FORCE}, {previous}},
	{{BUSY, FORCE}, {nop}},
	{{VANISH, FORCE}, {"Good bye."}},

	{{"bye"}, {"Good bye.", idle}},
	{{"farewell"}, {previous}},
	{{"job"}, {"I am selling equipment for adventurers. Do you need anything?"}},
	{{"name"}, {"I am Lubo, the owner of this shop."}},
	{{"time"}, {"It is exactly %T."}},
	{{"mountain"}, {"It is said that once there lived a great magician on the top of this mountain."}},
	{{"magician"}, {"I don't remember his name, but it's said that his banner was the black eye."}},
	{{"food"}, {"I sell the best apples in Tibia."}},
	{{"map"}, {"Oh! I'm sorry, I sold the last one just five minutes ago."}},
	{{"magic"}, {"There's a lot of magic flowing in the mountain to the north."}},
	{{"weapon"}, {"If you want to buy weapons, you'll have to go to a town or city."}},
	{{"dog"}, {"This is Ruffy my dog, please don't do him any harm."}},
	{{"pet"}, {"There are some strange stories about a magicians pet names. Ask Hoggle about it."}},
	{{"finger"}, {"Oh, you sure mean this old story about the mage Dago, who lost two fingers when he conjured a dragon."}},

	{{"inn"}, {"Frodo runs a nice inn in the near town Thais."}},
	{{"crunor", "cottage"}, {"Ah yes, I remember my grandfather talking about that name. This house used to be an inn a long time ago. My family bought it from some of these flower guys."}},
	{{"flower", "guy"}, {"Oh, I mean druids of course. They sold the cottage to my family after some of them died in an accident or something like that."}},
	{{"accident"}, {"As far as I can remember the story, a pet escaped its stable behind the inn. It got somehow involved with powerfull magic at a ritual and was transformed in some way."}},
	{{"stable", eq(questValue(211), 3)}, {"My grandpa told me, in the old days there were some behind this cottage. Nothing big though, just small ones, for chicken or rabbits.", setQuestValue(211, 4)}},
	{{"stable", lt(questValue(211), 3)}, {"Sorry speak louder I can't hear you."}},

	{{"equipment"}, {"I sell torches, fishing rods, sixpacks of worms, ropes, water hoses, backpacks, apples, and maps."}},
	{{"goods"}, {previous}},
	{{"do", "you", "sell"}, {previous}},
	{{"do", "you", "have"}, {previous}},
	{{"offer"}, {previous}},

	{{"torch"}, {setType(2920), setAmount(1), setPrice(3), "Do you want to buy a torch for %P gold?", setTopic(1)}},
	{{"rope"}, {setType(3003), setAmount(1), setPrice(60), "Do you want to buy a rope for %P gold?", setTopic(1)}},
	{{"water", "hose"}, {setType(2901), setAmount(1), setPrice(10), setData(1), "Do you want to buy a water hose for %P gold?", setTopic(1)}},
	{{"backpack"}, {setType(2854), setAmount(1), setPrice(25), "Do you want to buy a backpack for %P gold?", setTopic(1)}},
	{{"fishing", "rod"}, {setType(3483), setAmount(1), setPrice(175), "Do you want to buy a fishing rod for %P gold?", setTopic(1)}},
	{{"crowbar"}, {setType(3304), setAmount(1), setPrice(260), "Do you want to buy a crowbar for %P gold? I know its rather expensive, but I must protect people from thieves.", setTopic(1)}},
	{{"apple"}, {setType(3585), setAmount(1), setPrice(3), "Do you want to buy an apple for %P gold?", setTopic(1)}},
	{{P1, "torch"}, {setType(2920), setAmount(P1), setPrice(mul(3, P1)), "Do you want to buy %A torches for %P gold?", setTopic(1)}},
	{{P1, "apple"}, {setType(3585), setAmount(P1), setPrice(mul(3, P1)), "Do you want to buy %A apples for %P gold?", setTopic(1)}},

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
