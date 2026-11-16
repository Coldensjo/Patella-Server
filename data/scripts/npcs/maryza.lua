-- npcs/maryza.lua: the behaviour of the NPC Maryza, converted from
-- maryza.npc by patella-convert. The NPC is its [[npc]] entry in npcs.toml;
-- lib/npc.lua matches and runs the rules below, in the order of the file
-- (scripting.hh, "NPCs"). The comments are those of maryza.npc.
--
-- GIMUD - Graphical Interface Multi User Dungeon
-- maryza.npc: Datenbank für die Wirtin Maryza

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
local TYPE = behaviour.TYPE
local COUNTMONEY = behaviour.COUNTMONEY
local questValue = behaviour.questValue
local mul = behaviour.mul
local setTopic = behaviour.setTopic
local setPrice = behaviour.setPrice
local setAmount = behaviour.setAmount
local setType = behaviour.setType
local poison = behaviour.poison
local setQuestValue = behaviour.setQuestValue
local effectMe = behaviour.effectMe
local effectOpp = behaviour.effectOpp
local create = behaviour.create
local deleteMoney = behaviour.deleteMoney
local queue = behaviour.queue
local idle = behaviour.idle
local nop = behaviour.nop
local previous = behaviour.previous

local rules = behaviour.rules {
	{{ADDRESS, "hello$", "maryza", FORCE}, {"Welcome to the Jolly Axeman, %N. Have a good time!"}},
	{{ADDRESS, "hi$", "maryza", FORCE}, {previous}},
	{{ADDRESS, "hello$", FORCE}, {"Talking to me?", idle}},
	{{ADDRESS, "hi$", FORCE}, {previous}},
	{{ADDRESS, FORCE}, {idle}},
	{{BUSY, "hello$", "maryza", FORCE}, {"Shut up %N. Busy. You wait!", queue}},
	{{BUSY, "hi$", "maryza", FORCE}, {previous}},
	{{BUSY, "hello$", FORCE}, {"Talking to me, %N?"}},
	{{BUSY, "hi$", FORCE}, {previous}},
	{{BUSY, FORCE}, {nop}},
	{{VANISH, FORCE}, {"HEY! You lousy....!"}},

	{{"bye"}, {"Yeah, bye", idle}},
	{{"farewell"}, {"Yeah, farewell", idle}},
	{{"hello$", "jimbin", FORCE}, {"Yeah, bye", idle}},
	{{"hi$", "jimbin", FORCE}, {"Yeah, bye", idle}},
	{{"job"}, {"I'm the cook of the Jolly Axeman."}},
	{{"tavern"}, {previous}},
	{{"jimbin"}, {"I am so proud of him. In drinking, he's second only to our mighty general."}},
	{{"name"}, {"I am Maryza Firehand, daughter of Earth, from the Molten Rock."}},
	{{"time"}, {"To busy, ask my husband."}},
	{{"king"}, {"Don't like these upper cave guys."}},
	{{"army"}, {"We could better feed some dragons instead of these fools."}},
	{{"ferumbras"}, {"Heard that's what the humans call one of their boggiemen."}},
	{{"general"}, {"A fine drinker and strategist. Wastes his skill with these idiots of the army. What a shame."}},
	{{"excalibug"}, {"Would slice a dragon or two for steaks if i'd get it."}},
	{{"tark"}, {"He loved my dragonsteaks. Heard he died by a cave in while fighting drags in the Plains of Havoc."}},
	{{"thais"}, {"Puny town for puny guys."}},
	{{"tibia"}, {"We don't care much about outsiders anymore."}},
	{{"carlin"}, {"Don't like it, has an elfish touch, ye know?"}},
	{{"news"}, {"The boys of the Savage Axe at the bridge are running wild in these days."}},
	{{"rumors"}, {previous}},
	{{"bloody", "mary"}, {setType(3113), setAmount(1), "YOU &/$#@!", poison(15, 1), effectOpp(1), effectMe(3), create(TYPE)}},
	{{"buy"}, {"I can offer you some food if ye like."}},
	{{"do", "you", "sell"}, {previous}},
	{{"do", "you", "have"}, {previous}},
	{{"food"}, {"I sell normal and brown bread, meat, ham, cookies, rolls, and cheese made of mushrooms."}},
	{{"book"}, {setType(3234), setAmount(1), setPrice(150), "The cookbook of the famous dwarfish kitchen. You're lucky. I have a few copies on sale. Do you like one for %P gold?", setTopic(2)}},
	{{"cookbook"}, {previous}},
	{{"book", gt(questValue(279), 0), FORCE}, {"I'm sorry but I sell only one copy to each customer. Otherwise they would have been sold out a long time ago."}},
	{{"cookbook", gt(questValue(279), 0), FORCE}, {previous}},

	{{"bread"}, {setType(3600), setAmount(1), setPrice(4), "Do you wanna buy a bread for %P gold?", setTopic(1)}},
	{{"cheese"}, {setType(3607), setAmount(1), setPrice(6), "Do you wanna buy cheese for %P gold?", setTopic(1)}},
	{{"meat"}, {setType(3577), setAmount(1), setPrice(5), "Do you wanna buy meat for %P gold?", setTopic(1)}},
	{{"ham"}, {setType(3582), setAmount(1), setPrice(8), "Do you wanna buy a ham for %P gold?", setTopic(1)}},
	{{"cookie"}, {setType(3598), setAmount(1), setPrice(2), "Do you wanna buy a cookie for %P gold?", setTopic(1)}},
	{{"roll"}, {setType(3601), setAmount(1), setPrice(2), "Do you wanna buy a roll for %P gold?", setTopic(1)}},
	{{"brown", "bread"}, {setType(3602), setAmount(1), setPrice(3), "Do you wanna buy a brown bread for %P gold?", setTopic(1)}},

	{{P1, lt(1, P1), "bread"}, {setType(3600), setAmount(P1), setPrice(mul(4, P1)), "Do you wanna buy %A breads for %P gold?", setTopic(1)}},
	{{P1, lt(1, P1), "cheese"}, {setType(3607), setAmount(P1), setPrice(mul(6, P1)), "Do you wanna buy %A cheese for %P gold?", setTopic(1)}},
	{{P1, lt(1, P1), "meat"}, {setType(3577), setAmount(P1), setPrice(mul(5, P1)), "Do you wanna buy %A meat for %P gold?", setTopic(1)}},
	{{P1, lt(1, P1), "ham"}, {setType(3582), setAmount(P1), setPrice(mul(8, P1)), "Do you wanna buy %A ham for %P gold?", setTopic(1)}},
	{{P1, lt(1, P1), "cookie"}, {setType(3598), setAmount(P1), setPrice(mul(2, P1)), "Do you wanna buy %A cookies for %P gold?", setTopic(1)}},
	{{P1, lt(1, P1), "roll"}, {setType(3601), setAmount(P1), setPrice(mul(2, P1)), "Do you wanna buy %A rolls for %P gold?", setTopic(1)}},
	{{P1, lt(1, P1), "brown", "bread"}, {setType(3602), setAmount(P1), setPrice(mul(3, P1)), "Do you wanna buy %A brown breads for %P gold?", setTopic(1)}},

	{{eq(TOPIC, 1), "yes", ge(COUNTMONEY, PRICE)}, {"Here it is.", deleteMoney, create(TYPE)}},
	{{eq(TOPIC, 1), "yes"}, {"No gold, no sale, that's it."}},
	{{eq(TOPIC, 1)}, {"You &/$#@!"}},

	{{eq(TOPIC, 2), "yes", ge(COUNTMONEY, PRICE)}, {"Here you are. Happy cooking!", deleteMoney, create(TYPE), setQuestValue(279, 1)}},
	{{eq(TOPIC, 2), "yes"}, {"No gold, no sale, that's it."}},
	{{eq(TOPIC, 2)}, {"I have but a few copies, anyway."}},
}

function onCreatureSay(npc, creature, text, situation)
	behaviour.react(npc, rules, text, situation)
end

function onCreatureDisappear(npc, creature)
	behaviour.react(npc, rules, "", NPC_SITUATION_VANISH)
end
