-- npcs/billy.lua: the behaviour of the NPC Billy, converted from
-- billy.npc by patella-convert. The NPC is its [[npc]] entry in npcs.toml;
-- lib/npc.lua matches and runs the rules below, in the order of the file
-- (scripting.hh, "NPCs"). The comments are those of billy.npc.
--
-- GIMUD - Graphical Interface Multi User Dungeon
-- billy.npc: Datenbank fuer den Farmer Billy

local behaviour = require("lib.npc")
local ADDRESS = behaviour.ADDRESS
local BUSY = behaviour.BUSY
local VANISH = behaviour.VANISH
local PREMIUM = behaviour.PREMIUM
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
local setData = behaviour.setData
local create = behaviour.create
local delete = behaviour.delete
local createMoney = behaviour.createMoney
local deleteMoney = behaviour.deleteMoney
local queue = behaviour.queue
local idle = behaviour.idle
local nop = behaviour.nop
local previous = behaviour.previous

local rules = behaviour.rules {
	{{ADDRESS, "hello$", PREMIUM, FORCE}, {"Howdy %N."}},
	{{ADDRESS, "hi$", PREMIUM, FORCE}, {previous}},
	{{ADDRESS, "hello$", FORCE}, {"You did not pay your tax. Get lost!", idle}},
	{{ADDRESS, "hi$", FORCE}, {previous}},

	{{ADDRESS, FORCE}, {idle}},
	{{BUSY, "hello$", PREMIUM, FORCE}, {"Can't you see i am talking? Wait!", queue}},
	{{BUSY, "hi$", PREMIUM, FORCE}, {previous}},
	{{BUSY, "hello$", FORCE}, {"You did not pay your tax. Get lost!"}},
	{{BUSY, "hi$", FORCE}, {previous}},
	{{BUSY, FORCE}, {nop}},
	{{VANISH, FORCE}, {"YOU RUDE $\167&$"}},

	{{"bye"}, {"Bye.", idle}},
	{{"farewell"}, {"Farewell.", idle}},
	{{"how", "are", "you"}, {"I think, I'm fine."}},
	{{"job"}, {"I am farmer and a cook."}},
	{{"cook"}, {"I am the best cook around. You can sell me most types of food."}},
	{{"willie"}, {"Don't listen to that old wannabe, I'm the best cook around."}},
	{{"recipe"}, {"I would love to try a pancake. But I lack a decent pan. If you get me one, I will reward you."}},
	{{"name"}, {"Billy."}},
	{{"time"}, {"I came here to have some peace and leisure so leave me alone with 'time'."}},
	{{"help"}, {"Can't help you, sorry. I'm a cook, not a priest."}},
	{{"monster"}, {"Don't be afraid, in the town you should be save."}},
	{{"dungeon"}, {"You'll find a lot of dungeons if you look around."}},
	{{"sewer"}, {"The local sewers are invested by rats, fresh rats give a good stew, you can sell them to me."}},
	{{"god"}, {"I am the god of cooking, indeed!"}},
	{{"king"}, {"The king and his tax collectors are far away. You'll meet them soon enough."}},
	{{"obi"}, {"I like him, we usualy have a drink or two once a week and share storys about Willie."}},
	{{"seymour"}, {"I don't like his headmaster behaviour. Then again, he IS a headmaster after all."}},
	{{"dallheim"}, {"One of the kings best men, here to protect us."}},
	{{"cipfried"}, {"He never leaves this temple and only has time to care about those new arivals."}},
	{{"amber"}, {"Shes pretty indeed! I wonder if she likes bearded men."}},
	{{"weapon"}, {"Ask one of the shopkeepers. They make a fortune here with all those wannabe heroes."}},
	{{"magic"}, {"I can spell but know no spell."}},
	{{"spell"}, {previous}},
	{{"tibia"}, {"There is so much to be explored! Better hurry to get to the continent!"}},

	{{"offer"}, {"I can offer you bread, cheese, ham, or meat."}},
	{{"food"}, {"Are you looking for food? I have bread, cheese, ham, and meat."}},

	{{"bread"}, {setType(3600), setAmount(1), setPrice(3), "Do you want to buy a bread for %P gold?", setTopic(1)}},
	{{"cheese"}, {setType(3607), setAmount(1), setPrice(5), "Do you want to buy a cheese for %P gold?", setTopic(1)}},
	{{"meat"}, {setType(3577), setAmount(1), setPrice(5), "Do you want to buy meat for %P gold?", setTopic(1)}},
	{{"ham"}, {setType(3582), setAmount(1), setPrice(8), "Do you want to buy a ham for %P gold?", setTopic(1)}},

	{{P1, lt(1, P1), "bread"}, {setType(3600), setAmount(P1), setPrice(mul(3, P1)), "Do you want to buy %A breads for %P gold?", setTopic(1)}},
	{{P1, lt(1, P1), "cheese"}, {setType(3607), setAmount(P1), setPrice(mul(5, P1)), "Do you want to buy %A cheese for %P gold?", setTopic(1)}},
	{{P1, lt(1, P1), "meat"}, {setType(3577), setAmount(P1), setPrice(mul(5, P1)), "Do you want to buy %A meat for %P gold?", setTopic(1)}},
	{{P1, lt(1, P1), "ham"}, {setType(3582), setAmount(P1), setPrice(mul(8, P1)), "Do you want to buy %A hams for %P gold?", setTopic(1)}},

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

	{{"rat"}, {"So you bring me a fresh rat for my famous stew?", setType(3994), setAmount(1), setPrice(2), setTopic(2)}},
	{{"sell", "rat"}, {"So you bring me a fresh rat for my famous stew?", setType(3994), setAmount(1), setPrice(2), setTopic(2)}},
	{{P1, lt(1, P1), "rat"}, {setType(3994), setAmount(P1), setPrice(mul(2, P1)), "So you bring me %A fresh rats for my famous stew?", setTopic(2)}},

	{{"sell"}, {"I sell various kinds of food."}},
	{{"buy"}, {"I buy food of most kind. Since I am a great cook I need much of it."}},

	{{eq(TOPIC, 1), "yes", ge(COUNTMONEY, PRICE)}, {"Here it is.", deleteMoney, create(TYPE)}},
	{{eq(TOPIC, 1), "yes"}, {"I am sorry, but you do not have enough gold."}},
	{{eq(TOPIC, 1)}, {"Maybe later."}},

	{{eq(TOPIC, 2), "yes", ge(count(TYPE), AMOUNT)}, {"Here you are.", delete(TYPE), createMoney}},
	{{eq(TOPIC, 2), "yes"}, {"You don't have one."}},
	{{eq(TOPIC, 2), "yes", gt(AMOUNT, 1)}, {"Sorry, you do not have so many."}},
	{{eq(TOPIC, 2), "no"}, {"Then not."}},

	{{"pan"}, {setType(3466), setAmount(1), "Have you found a pan for me?", setTopic(3)}},
	{{eq(TOPIC, 3), "yes", ge(count(TYPE), AMOUNT)}, {"A pan! At last! Take this in case you eat something my cousin has cooked.", delete(TYPE), setAmount(1), setData(1), create(3153)}},
	{{eq(TOPIC, 3), "yes"}, {"Hey! You don't have it!"}},
	{{eq(TOPIC, 3), "no"}, {"$&*@!"}},
	{{eq(TOPIC, 3)}, {previous}},
}

function onCreatureSay(npc, creature, text, situation)
	behaviour.react(npc, rules, text, situation)
end

function onCreatureDisappear(npc, creature)
	behaviour.react(npc, rules, "", NPC_SITUATION_VANISH)
end
