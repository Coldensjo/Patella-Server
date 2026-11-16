-- npcs/costello.lua: the behaviour of the NPC Costello, converted from
-- costello.npc by patella-convert. The NPC is its [[npc]] entry in npcs.toml;
-- lib/npc.lua matches and runs the rules below, in the order of the file
-- (scripting.hh, "NPCs"). The comments are those of costello.npc.
--
-- GIMUD - Graphical Interface Multi User Dungeon
-- costello.npc: Datenbank für den Abt Costello

local behaviour = require("lib.npc")
local ADDRESS = behaviour.ADDRESS
local BUSY = behaviour.BUSY
local VANISH = behaviour.VANISH
local FORCE = behaviour.FORCE
local eq = behaviour.eq
local lt = behaviour.lt
local gt = behaviour.gt
local ge = behaviour.ge
local TOPIC = behaviour.TOPIC
local PRICE = behaviour.PRICE
local AMOUNT = behaviour.AMOUNT
local TYPE = behaviour.TYPE
local LEVEL = behaviour.LEVEL
local HP = behaviour.HP
local BURNING = behaviour.BURNING
local POISON = behaviour.POISON
local COUNTMONEY = behaviour.COUNTMONEY
local count = behaviour.count
local questValue = behaviour.questValue
local setTopic = behaviour.setTopic
local setPrice = behaviour.setPrice
local setAmount = behaviour.setAmount
local setType = behaviour.setType
local setHp = behaviour.setHp
local burning = behaviour.burning
local poison = behaviour.poison
local setQuestValue = behaviour.setQuestValue
local effectOpp = behaviour.effectOpp
local create = behaviour.create
local delete = behaviour.delete
local deleteMoney = behaviour.deleteMoney
local queue = behaviour.queue
local idle = behaviour.idle
local nop = behaviour.nop
local previous = behaviour.previous

local rules = behaviour.rules {
	{{ADDRESS, "hello$", gt(questValue(220), 0), FORCE}, {"WHAT? You have to be that trespasser my brothers told me about! Entering the restricted area is a horrible crime!", setTopic(3)}},
	{{ADDRESS, "hi$", gt(questValue(220), 0), FORCE}, {previous}},

	{{ADDRESS, "hello$", FORCE}, {"Welcome, %N! Feel free to tell me what has brought you here."}},
	{{ADDRESS, "hi$", FORCE}, {previous}},
	{{ADDRESS, FORCE}, {idle}},

	{{BUSY, "hello$", gt(questValue(220), 0), FORCE}, {"WHAT? You have to be that trespasser my brothers told me about! Entering the restricted area is a horrible crime!", idle}},
	{{BUSY, "hi$", gt(questValue(220), 0), FORCE}, {previous}},

	{{BUSY, "hello$", FORCE}, {"Please wait a minute, %N.", queue}},
	{{BUSY, "hi$", FORCE}, {previous}},
	{{BUSY, "heal$", gt(questValue(220), 0)}, {"I won't waste my healing powers on you, spawn of evil!", idle}},
	{{BUSY, "heal$", gt(BURNING, 0)}, {"You are burning, %N. I will help you.", burning(0, 0), effectOpp(15)}},
	{{BUSY, "heal$", gt(POISON, 0)}, {"You are poisoned, %N. I will help you.", poison(0, 0), effectOpp(14)}},
	{{BUSY, "heal$", lt(HP, 40)}, {"You are looking really bad, %N. Let me heal your wounds.", setHp(40), effectOpp(13)}},
	{{BUSY, "heal$"}, {"You aren't looking that bad, %N. Sorry, I can't help you."}},
	{{BUSY, FORCE}, {nop}},
	{{VANISH, FORCE}, {"Remember: If you are heavily wounded or poisoned, I will heal you."}},

	{{eq(TOPIC, 3), "crime", lt(LEVEL, 20), FORCE}, {"The only way to redeem such an offense is the sacrifice of 500 gold pieces! Are you willing to pay that sum?", setPrice(500), setTopic(4)}},
	{{eq(TOPIC, 3), "absolution", lt(LEVEL, 20), FORCE}, {previous}},
	{{eq(TOPIC, 3), "crime", lt(LEVEL, 40), FORCE}, {"The only way to redeem such an offense is the sacrifice of 1.000 gold pieces! Are you willing to pay that sum?", setPrice(1000), setTopic(4)}},
	{{eq(TOPIC, 3), "absolution", lt(LEVEL, 40), FORCE}, {previous}},
	{{eq(TOPIC, 3), "crime", lt(LEVEL, 60), FORCE}, {"The only way to redeem such an offense is the sacrifice of 5.000 gold pieces! Are you willing to pay that sum?", setPrice(5000), setTopic(4)}},
	{{eq(TOPIC, 3), "absolution", lt(LEVEL, 60), FORCE}, {previous}},
	{{eq(TOPIC, 3), "crime", gt(LEVEL, 59), FORCE}, {"The only way to redeem such an offense is the sacrifice of 10.000 gold pieces! Are you willing to pay that sum?", setPrice(10000), setTopic(4)}},
	{{eq(TOPIC, 3), "absolution", gt(LEVEL, 59), FORCE}, {previous}},
	{{eq(TOPIC, 3), FORCE}, {"Be gone!", idle}},

	{{eq(TOPIC, 4), "yes", lt(COUNTMONEY, PRICE), FORCE}, {"Begone! You do not have enough money!", idle}},
	{{eq(TOPIC, 4), "yes", FORCE}, {"So receive your absolution! And never do such a thing again!", deleteMoney, effectOpp(13), setQuestValue(220, 0)}},
	{{eq(TOPIC, 4), FORCE}, {"Then be gone!", idle}},

	{{"bye"}, {"Good bye, %N!", idle}},
	{{"farewell"}, {previous}},
	{{"job"}, {"I am the abbot of the white raven monastery on the isle of the kings."}},
	{{"name"}, {"My name is Costello."}},
	{{"tibia"}, {"That is the name of our world and its major continent."}},
	{{"god"}, {"They created Tibia and all life on it."}},
	{{"life"}, {"On Tibia there are many forms of life. Plants, the citizens, and monsters."}},
	{{"plant"}, {"Just walk around, you will see grass, trees, and bushes."}},
	{{"white", "raven"}, {"The legends tell us of a white raven which lead the ship of the first monk of our order here. He discovered this isle and the caves beneath it."}},
	{{"caves"}, {"Anselm, the first of our order, discovered them while looking for a suitable burial place for his king."}},
	{{"anselm"}, {"He was a humble and pious man, and he was chosen by the royal family of thais to find a resting place for their dead."}},
	{{"isle"}, {"We founded our monastery to guard the royal tombs and to gather wisdom and knowledge."}},
	{{"order"}, {previous}},
	{{"wisdom"}, {"You are allowed to enter the library upstairs. Stay there and don't go upstairs, because that area is reserved for members of our order."}},
	{{"knowledge"}, {previous}},
	{{"tibianus"}, {"One day every Tibianus ends up here."}},
	{{"king"}, {"The bygone leaders of the Thaian empire rest beneath this monastery in tombs and crypts."}},
	{{"tomb", lt(questValue(63), 1)}, {"The tombs and crypts of the Thaian lineage are well protected deep beneath our abbey, although ... but surely this will not interest you."}},
	{{"crypts", lt(questValue(63), 1)}, {previous}},
	{{"although", lt(questValue(63), 1)}, {"In my dreams the dead are talking to me about torment and disturbance. But I might be imagining things."}},
	{{"interest", lt(questValue(63), 1)}, {previous}},
	{{"imagining", lt(questValue(63), 1)}, {"Brother Fugio, the only one of our order who is allowed to enter the crypts, assures me everything is all right."}},
	{{"torment", lt(questValue(63), 1)}, {previous}},
	{{"disturbance", lt(questValue(63), 1)}, {previous}},

	{{"fugio", lt(questValue(63), 1)}, {"To be honest, I fear the omen in my dreams may be true. Perhaps Fugio is unable to see the danger down there. Perhaps ... you are willing to investigate this matter?", setTopic(1)}},
	{{"alright", lt(questValue(63), 1)}, {previous}},
	{{eq(TOPIC, 1), "yes"}, {"Thank you very much! From now on you may open the warded doors to the catacombs", setQuestValue(63, 1), setQuestValue(64, 1)}},

	-- 63=forschen 64=questtür wg. eingang von unten

	{{eq(TOPIC, 1), "no"}, {"Please forgive an old man, I shouldn't have asked a stranger anyways."}},
	{{eq(TOPIC, 1)}, {previous}},

	{{"diary", eq(questValue(219), 0)}, {"Do you want me to inspect a diary?", setType(3212), setAmount(1), setTopic(2)}},
	{{"diary", gt(questValue(219), 0)}, {"Thank you again for handing me that diary."}},

	{{eq(TOPIC, 2), "yes", ge(count(TYPE), AMOUNT)}, {"By the gods! This is brother Fugio's handwriting and what I read is horrible indeed! You have done our order a great favour by giving this diary to me! Take this blessed Ankh. May it protect you in even your darkest hours.", delete(TYPE), create(3214), setQuestValue(219, 2)}},
	{{eq(TOPIC, 2), "no"}, {"Uhm, as you wish."}},
	{{eq(TOPIC, 2)}, {previous}},

	{{"passage", eq(questValue(63), 1), eq(questValue(64), 1)}, {"Oh of course, I will order Jack and the fisher Windtrouser to give you transportation if needed.", setQuestValue(62, 1)}},
	{{"passage"}, {"You should not be here at all and I won't allow anyone to transport you from or to this isle."}},

	{{"ferumbras"}, {"Don't mention this servant of evil here."}},
	{{"excalibug"}, {"Sadly we have only little knowledge on this topic."}},
	{{"news"}, {"Sorry, we rarely hear anything new here."}},
	{{"monster"}, {"There are really too many of them in Tibia. But who are we to question the wisdom of the gods?"}},

	{{"heal$", gt(questValue(220), 0)}, {"I won't waste my healing powers on you, spawn of evil!", idle}},
	{{"heal$", gt(BURNING, 0)}, {"You are burning. I will help you.", burning(0, 0), effectOpp(15)}},
	{{"heal$", gt(POISON, 0)}, {"You are poisoned. I will help you.", poison(0, 0), effectOpp(14)}},
	{{"heal$", lt(HP, 40)}, {"You are looking really bad. Let me heal your wounds.", setHp(40), effectOpp(13)}},
	{{"heal$"}, {"You aren't looking that bad. Sorry, I can't help you."}},
}

function onCreatureSay(npc, creature, text, situation)
	behaviour.react(npc, rules, text, situation)
end

function onCreatureDisappear(npc, creature)
	behaviour.react(npc, rules, "", NPC_SITUATION_VANISH)
end
