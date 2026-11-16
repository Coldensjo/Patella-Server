-- npcs/maria.lua: the behaviour of the NPC Maria, converted from
-- maria.npc by patella-convert. The NPC is its [[npc]] entry in npcs.toml;
-- lib/npc.lua matches and runs the rules below, in the order of the file
-- (scripting.hh, "NPCs"). The comments are those of maria.npc.
--
-- GIMUD - Graphical Interface Multi User Dungeon
-- maria.npc: Datenbank für die Wirtin Maria

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
	{{ADDRESS, "hello$", FORCE}, {"The Hard Rock Tavern greets you, %N."}},
	{{ADDRESS, "hi$", FORCE}, {previous}},
	{{ADDRESS, FORCE}, {idle}},
	{{BUSY, "hello$", FORCE}, {"You're served soon, %N.", queue}},
	{{BUSY, "hi$", FORCE}, {previous}},
	{{BUSY, FORCE}, {nop}},
	{{VANISH, FORCE}, {"Visit us again."}},

	{{"bye"}, {"Good bye, %N. Tell your friends about us and visit us again.", idle}},
	{{"job"}, {"I am running this upper part of the Hard Rock Tavern."}},
	{{"tavern"}, {previous}},

	{{"strange", "fellow"}, {"I don't know him. He seems very nervous and hes always fumbling around with this suspicious hat."}},
	{{"david"}, {"I am sorry but I don't know him personaly. I heared he entertained people here long before I moved to Venore."}},
	{{"brassacres"}, {previous}},

	{{"upper", "part"}, {"Yes, that's here. Below is the Pit Tavern for those fighters that use the pits."}},
	{{"pits"}, {"Well, they do a lot of fighting down there."}},
	{{"name"}, {"I am Maria."}},
	{{"maria"}, {"Yes, I am Maria, Maria Corona."}},
	{{"time"}, {"Don't be that hasty."}},
	{{"king"}, {"In Venore, everyone is a king ... until he runs out of luck or money."}},
	{{"tibianus"}, {previous}},
	{{"army"}, {"Good fighters need good entertainment. That's what they get here."}},
	{{"ferumbras"}, {"I think he's more a Thaian problem."}},
	{{"excalibug"}, {"I'd rather have a stainless steel cooking pan than such a knife."}},
	{{"thais"}, {"It's a shame that this lousy city is the heart of the kingdom."}},
	{{"tibia"}, {"In the long run it's money that rules everything in Tibia."}},
	{{"carlin"}, {"As far as the merchants say it's economically unimportant."}},
	{{"amazon"}, {"I can only hope those wild women don't scare away more customers than come here in order to fight against them."}},
	{{"news"}, {"Bah, only the usual swampelves stories."}},
	{{"rumors"}, {previous}},
	{{"swampelves"}, {"Well there's a hidden city called Shadowthorn of those warlike elves in the swamps. They are not amused of civilisation at their doorsteps and have been plotting against Venore for years."}},

	{{"buy"}, {"Food and drinks as much as you can pay for."}},
	{{"do", "you", "sell"}, {previous}},
	{{"do", "you", "have"}, {previous}},
	{{"food"}, {"We offer cookies, bread, cheese, ham, and meat, as well as eggs and tomatoes."}},
	{{"drink"}, {"Do you want beer, wine, lemonade, or water?"}},

	{{"bread"}, {setType(3600), setAmount(1), setPrice(4), "Do you want to buy a bread for %P gold?", setTopic(1)}},
	{{"cheese"}, {setType(3607), setAmount(1), setPrice(6), "Do you want to buy cheese for %P gold?", setTopic(1)}},
	{{"meat"}, {setType(3577), setAmount(1), setPrice(5), "Do you want to buy meat for %P gold?", setTopic(1)}},
	{{"ham"}, {setType(3582), setAmount(1), setPrice(8), "Do you want to buy a ham for %P gold?", setTopic(1)}},
	{{"cookie"}, {setType(3598), setAmount(1), setPrice(5), "Do you want to buy a cookie for %P gold?", setTopic(1)}},
	{{"egg"}, {setType(3606), setAmount(1), setPrice(2), "Do you want to buy an egg for %P gold?", setTopic(1)}},
	{{"tomato"}, {setType(3596), setAmount(1), setPrice(5), "Do you want to buy a tomato for %P gold?", setTopic(1)}},

	{{P1, lt(1, P1), "bread"}, {setType(3600), setAmount(P1), setPrice(mul(4, P1)), "Do you wanna buy %A breads for %P gold?", setTopic(1)}},
	{{P1, lt(1, P1), "cheese"}, {setType(3607), setAmount(P1), setPrice(mul(6, P1)), "Do you wanna buy %A cheese for %P gold?", setTopic(1)}},
	{{P1, lt(1, P1), "meat"}, {setType(3577), setAmount(P1), setPrice(mul(5, P1)), "Do you wanna buy %A meat for %P gold?", setTopic(1)}},
	{{P1, lt(1, P1), "ham"}, {setType(3582), setAmount(P1), setPrice(mul(8, P1)), "Do you wanna buy %A ham for %P gold?", setTopic(1)}},
	{{P1, lt(1, P1), "cookie"}, {setType(3598), setAmount(P1), setPrice(mul(5, P1)), "Do you wanna buy %A cookies for %P gold?", setTopic(1)}},
	{{P1, lt(1, P1), "egg"}, {setType(3606), setAmount(P1), setPrice(mul(2, P1)), "Do you want to buy %A eggs for %P gold?", setTopic(1)}},
	{{P1, lt(1, P1), "tomato"}, {setType(3596), setAmount(P1), setPrice(mul(5, P1)), "Do you want to buy %A tomatoes for %P gold?", setTopic(1)}},

	{{"lemonade"}, {setType(2880), setData(12), setAmount(1), setPrice(2), "Do you want to buy a mug of lemonade for %P gold?", setTopic(1)}},
	{{"beer"}, {setType(2880), setData(3), setAmount(1), setPrice(2), "Do you want to buy a mug of beer for %P gold?", setTopic(1)}},
	{{"wine"}, {setType(2880), setData(2), setAmount(1), setPrice(3), "Do you want to buy a mug of wine for %P gold?", setTopic(1)}},
	{{"water"}, {setType(2880), setData(1), setAmount(1), setPrice(1), "Do you want to buy a mug of water for %P gold?", setTopic(1)}},

	{{eq(TOPIC, 1), "yes", ge(COUNTMONEY, PRICE)}, {"And here is what you ordered.", deleteMoney, create(TYPE)}},
	{{eq(TOPIC, 1), "yes"}, {"You penniless beggar! Get out of here!", idle}},
	{{eq(TOPIC, 1)}, {"Hrmpf!"}},
}

function onCreatureSay(npc, creature, text, situation)
	behaviour.react(npc, rules, text, situation)
end

function onCreatureDisappear(npc, creature)
	behaviour.react(npc, rules, "", NPC_SITUATION_VANISH)
end
