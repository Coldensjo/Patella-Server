-- npcs/adrenius.lua: the behaviour of the NPC Adrenius, converted from
-- adrenius.npc by patella-convert. The NPC is its [[npc]] entry in npcs.toml;
-- lib/npc.lua matches and runs the rules below, in the order of the file
-- (scripting.hh, "NPCs"). The comments are those of adrenius.npc.
--
-- GIMUD - Graphical Interface Multi User Dungeon
-- adrenius.npc: Datenbank für den Wüstenpriester Adrenius (Desert)

local behaviour = require("lib.npc")
local ADDRESS = behaviour.ADDRESS
local BUSY = behaviour.BUSY
local VANISH = behaviour.VANISH
local FORCE = behaviour.FORCE
local eq = behaviour.eq
local ge = behaviour.ge
local TOPIC = behaviour.TOPIC
local PRICE = behaviour.PRICE
local COUNTMONEY = behaviour.COUNTMONEY
local setTopic = behaviour.setTopic
local setPrice = behaviour.setPrice
local setData = behaviour.setData
local burning = behaviour.burning
local effectMe = behaviour.effectMe
local create = behaviour.create
local deleteMoney = behaviour.deleteMoney
local idle = behaviour.idle
local nop = behaviour.nop
local previous = behaviour.previous

local rules = behaviour.rules {
	{{ADDRESS, "hello$", FORCE}, {"Hello, %N! What can I do for you?"}},
	{{ADDRESS, "hi$", FORCE}, {previous}},
	{{ADDRESS, FORCE}, {idle}},
	{{BUSY, "hello$", FORCE}, {"Can't you see, I am talking to someone else!"}},
	{{BUSY, "hi$", FORCE}, {previous}},
	{{BUSY, FORCE}, {nop}},
	{{VANISH, FORCE}, {"Leave me, I am used to it anyways..."}},

	{{"bye"}, {"Good bye.", idle}},
	{{"name"}, {"My name is Adrenius."}},
	{{"job"}, {"I'm a priest of Fafnar."}},
	{{"fafnar"}, {"Fafnar is the stronger one of the two suns above our world."}},
	{{"thais"}, {"Yyyyess. Yes, it's the capital city of Tibia I think."}},
	{{"carlin"}, {"Carlin? Don't you mean Thais?"}},
	{{"king"}, {"Who needs a king? I don't."}},
	{{"weapon"}, {"Who needs weapons? I never had and i never will have weapons - what for?"}},
	{{"help"}, {"Help? Help? Nothing more? Don't we all demand some help?"}},
	{{"time"}, {"Time? What is time? A word? A thing? An object?"}},
	{{"sword"}, {"Swords? Don't you have something else to do?"}},
	{{"desert"}, {"Sand, sand and again sand. Sand all over. Yes, I'd say: it's truly a desert!"}},
	{{"excalibug"}, {"What's that? You start annoying me."}},
	{{"fight"}, {"Leave me alone. I don't want to fight."}},
	{{"god"}, {"Fafnar is the greatest among the gods."}},
	{{"way"}, {"Way? Which way? I forgot where most ways go to... excuse me."}},
	{{"door"}, {"Who needs doors? Free your mind!"}},
	{{"secret"}, {"Secrets ... What do you mean?"}},
	{{"treasure"}, {"Treasures? What is a treasure for you?"}},
	{{"book"}, {"Read books, it increases your intelligence and, furthermore, it's a great source of inspiration!"}},
	{{"gharonk"}, {"Hmmmm... I don't know much about it."}},
	{{"offer"}, {"I can offer you religion and mysticism."}},
	{{"library"}, {"I heard of the library, but I never was very interested in it."}},
	{{"netlios"}, {"This fool! His book is nothing but a hoax! At least I believe that. Or did you find an answer for my questions?", setTopic(1)}},

	{{eq(TOPIC, 1), "yes"}, {setPrice(500), "By the way, I would like a donation for my temple. Are %P gold ok?", setTopic(2)}},
	{{eq(TOPIC, 1), "no"}, {"Oh. So once again I am proved right."}},
	{{eq(TOPIC, 1)}, {"You can't even say 'yes' or 'no'. You are not worth talking to me!", idle}},

	{{eq(TOPIC, 2), "yes", ge(COUNTMONEY, PRICE)}, {deleteMoney, "Thank you very much. Now, name me the first person in alphabetical order, his age, his fate, and how long he was on his journeys!", setTopic(4)}},
	{{eq(TOPIC, 2), "yes"}, {"You want to fool me? May Fafnar burn your soul!", effectMe(14), burning(50, 10), idle}},
	{{eq(TOPIC, 2)}, {"Then I don't want to talk to you.", idle}},

	{{eq(TOPIC, 4), "anaso", "41", "mother-bear", "117"}, {"Hmmm, maybe. What can you tell me about the second 'adventurer'?", setTopic(5)}},
	{{eq(TOPIC, 4)}, {"No, sorry, that doesn't sound correct to me. Maybe you should reconsider your words one more time..."}},

	{{eq(TOPIC, 5), "elaeus", "39", "dragon", "100"}, {"Yes, that might be true. What did you find out about the third man?", setTopic(6)}},
	{{eq(TOPIC, 5)}, {"No, no, no! Think about it, that simply can't be true!"}},

	{{eq(TOPIC, 6), "gadinius", "42", "fire", "83"}, {"Correct again! Hmmmm... I doubt you know anything about the fourth person!", setTopic(7)}},
	{{eq(TOPIC, 6)}, {"Hmmmm... well, no. That is not true, it does not fit to the data provided by the books."}},

	{{eq(TOPIC, 7), "heso", "40", "troll", "66"}, {"Yes! Really, how did you figure that out? I bet, you don't know anything about the last adventurer!", setTopic(8)}},
	{{eq(TOPIC, 7)}, {"No, sorry. Incorrect..."}},

	{{eq(TOPIC, 8), "hestus", "38", "poison", "134"}, {"That's right! Why didn't I see it? It's obvious, Netlios was right, and his stories are great! Wait, I'll give you something!", setData(4023), create(2969)}},
	{{eq(TOPIC, 8)}, {"Well, and again it was shown: I am right and Netlios is wrong!"}},
}

function onCreatureSay(npc, creature, text, situation)
	behaviour.react(npc, rules, text, situation)
end

function onCreatureDisappear(npc, creature)
	behaviour.react(npc, rules, "", NPC_SITUATION_VANISH)
end
