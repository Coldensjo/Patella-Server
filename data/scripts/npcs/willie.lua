-- npcs/willie.lua: the behaviour of the NPC Willie, converted from
-- willie.npc by patella-convert. The NPC is its [[npc]] entry in npcs.toml;
-- lib/npc.lua matches and runs the rules below, in the order of the file
-- (scripting.hh, "NPCs"). The comments are those of willie.npc.
--
-- GIMUD - Graphical Interface Multi User Dungeon
-- willie.npc: Datenbank fuer den Farmer Willie

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
	{{ADDRESS, "hello$", FORCE}, {"Hiho %N."}},
	{{ADDRESS, "hi$", FORCE}, {previous}},
	{{ADDRESS, FORCE}, {idle}},
	{{BUSY, "hello$", FORCE}, {"Nah, I am talking. Wait here.", queue}},
	{{BUSY, "hi$", FORCE}, {previous}},
	{{BUSY, FORCE}, {nop}},
	{{VANISH, FORCE}, {"YOU RUDE $!@##&"}},

	{{"bye"}, {"Yeah, bye.", idle}},
	{{"farewell"}, {"Yeah, farewell.", idle}},
	{{"how", "are", "you"}, {"Fine enough."}},
	{{"job"}, {"I am a farmer and a cook."}},
	{{"cook"}, {"I try out old and new recipes. You can sell me all food you have."}},
	{{"recipe"}, {"I would love to try a banana-pie. But I lack the bananas. If you get me one, I will reward you."}},
	{{"name"}, {"Willie."}},
	{{"time"}, {"Am I a clock or what?"}},
	{{"help"}, {"Help yourself, I have not stolen my time."}},
	{{"monster"}, {"Are you afraid of monsters ... you baby?"}},
	{{"dungeon"}, {"I have no time for your dungeon nonsense."}},
	{{"sewer"}, {"What about them? Do you live there?"}},
	{{"god"}, {"I am a farmer, not a preacher."}},
	{{"king"}, {"I'm glad that we don't see many officials here."}},
	{{"obi"}, {"This little $&#@& has only #@$*# in his mind. One day I will put a #@$@ in his *@&&#@!"}},
	{{"seymour"}, {"This joke of a man thinks he is sooo important."}},
	{{"dallheim"}, {"Uhm, fine guy I think."}},
	{{"cipfried"}, {"Our little monkey."}},
	{{"amber"}, {"Quite a babe."}},
	{{"weapon"}, {"I'm not in the weapon business, but if you don't stop to harass me, I will put my hayfork in your &$&#$ and *$!&&*# it."}},
	{{"magic"}, {"I am magician in the kitchen."}},
	{{"spell"}, {"I know how to spell and i know how to spit, you little @!#&&. Wanna see?."}},
	{{"tibia"}, {"If I were you, I would stay here."}},

	{{"bread"}, {setType(3600), setAmount(1), setPrice(3), "Do you want to buy a bread for %P gold?", setTopic(1)}},
	{{"cheese"}, {setType(3607), setAmount(1), setPrice(5), "Do you want to buy a cheese for %P gold?", setTopic(1)}},
	{{"meat"}, {setType(3577), setAmount(1), setPrice(5), "Do you want to buy meat for %P gold?", setTopic(1)}},
	{{"ham"}, {setType(3582), setAmount(1), setPrice(8), "Do you want to buy a ham for %P gold?", setTopic(1)}},

	{{P1, lt(1, P1), "bread"}, {setType(3600), setAmount(P1), setPrice(mul(3, P1)), "Do you want to buy %A breads for %P gold?", setTopic(1)}},
	{{P1, lt(1, P1), "cheese"}, {setType(3607), setAmount(P1), setPrice(mul(5, P1)), "Do you want to buy %A cheese for %P gold?", setTopic(1)}},
	{{P1, lt(1, P1), "meat"}, {setType(3577), setAmount(P1), setPrice(mul(5, P1)), "Do you want to buy %A meat for %P gold?", setTopic(1)}},
	{{P1, lt(1, P1), "ham"}, {setType(3582), setAmount(P1), setPrice(mul(8, P1)), "Do you want to buy %A hams for %P gold?", setTopic(1)}},

	{{"offer"}, {"I can offer you bread, cheese, ham, or meat."}},
	{{"food"}, {"Are you looking for food? I have bread, cheese, ham, and meat."}},

	{{eq(TOPIC, 1), "yes", ge(COUNTMONEY, PRICE)}, {"Here it is.", deleteMoney, create(TYPE)}},
	{{eq(TOPIC, 1), "yes"}, {"I am sorry, but you do not have enough gold."}},
	{{eq(TOPIC, 1)}, {"Maybe later."}},

	{{"sell", "bread"}, {setType(3600), setAmount(1), setPrice(1), "So, you want to sell a bread? Hmm, I give you %P gold, ok?", setTopic(2)}},
	{{"sell", "cheese"}, {setType(3607), setAmount(1), setPrice(2), "So, you want to sell a cheese? Hmm, I give you %P gold, ok?", setTopic(2)}},
	{{"sell", "meat"}, {setType(3577), setAmount(1), setPrice(2), "So, you want to sell meat? Hmm, I give you %P gold, ok?", setTopic(2)}},
	{{"sell", "ham"}, {setType(3582), setAmount(1), setPrice(4), "So, you want to sell a ham? Hmm, I give you %P gold, ok?", setTopic(2)}},
	{{"sell", "salmon"}, {setType(3579), setAmount(1), setPrice(2), "So, you want to sell a salmon? Hmm, I give you %P gold, ok?", setTopic(2)}},
	{{"sell", "fish"}, {"Go away with this stinking &*#@@!"}},
	{{"sell", "cherry"}, {setType(3590), setAmount(1), setPrice(1), "So, you want to sell a cherry? Hmm, I give you %P gold, ok?", setTopic(2)}},

	{{"sell", P1, lt(1, P1), "bread"}, {setType(3600), setAmount(P1), setPrice(mul(1, P1)), "So, you want to sell %A breads? Hmm, I give you %P gold, ok?", setTopic(2)}},
	{{"sell", P1, lt(1, P1), "cheese"}, {setType(3607), setAmount(P1), setPrice(mul(2, P1)), "So, you want to sell %A cheese? Hmm, I give you %P gold, ok?", setTopic(2)}},
	{{"sell", P1, lt(1, P1), "meat"}, {setType(3577), setAmount(P1), setPrice(mul(2, P1)), "So, you want to sell %A meat? Hmm, I give you %P gold, ok?", setTopic(2)}},
	{{"sell", P1, lt(1, P1), "ham"}, {setType(3582), setAmount(P1), setPrice(mul(4, P1)), "So, you want to sell %A hams? Hmm, I give you %P gold, ok?", setTopic(2)}},
	{{"sell", P1, lt(1, P1), "salmon"}, {setType(3579), setAmount(P1), setPrice(mul(2, P1)), "So, you want to sell %A salmon? Hmm, I give you %P gold, ok?", setTopic(2)}},
	{{"sell", P1, lt(1, P1), "fish"}, {"Go away with this stinking &*#@@!"}},
	{{"sell", P1, lt(1, P1), "cherr"}, {setType(3590), setAmount(P1), setPrice(mul(1, P1)), "So, you want to sell %A cherries? Hmm, I give you %P gold, ok?", setTopic(2)}},

	{{"sell"}, {"I sell food of many kinds."}},
	{{"buy"}, {"I buy food of any kind. Since I am a great cook I need much of it."}},

	{{eq(TOPIC, 2), "yes", ge(count(TYPE), AMOUNT)}, {"Here you are.", delete(TYPE), createMoney}},
	{{eq(TOPIC, 2), "yes"}, {"You don't have one."}},
	{{eq(TOPIC, 2), "no"}, {"Then not."}},

	{{"banana"}, {setType(3587), setAmount(1), "Have you found a banana for me?", setTopic(3)}},
	{{eq(TOPIC, 3), "yes", ge(count(TYPE), AMOUNT)}, {"A banana! Great. Take this shield, so the &#@&* monsters don't beat the &@*&@ out of you.", delete(TYPE), create(3426)}},
	{{eq(TOPIC, 3), "yes"}, {"Hm, you don't have it."}},
	{{eq(TOPIC, 3), "yes", gt(AMOUNT, 1)}, {"Sorry, you do not have so many."}},
	{{eq(TOPIC, 3), "no"}, {"Too bad."}},
	{{eq(TOPIC, 3)}, {"Too bad."}},
}

function onCreatureSay(npc, creature, text, situation)
	behaviour.react(npc, rules, text, situation)
end

function onCreatureDisappear(npc, creature)
	behaviour.react(npc, rules, "", NPC_SITUATION_VANISH)
end
