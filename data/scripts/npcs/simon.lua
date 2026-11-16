-- npcs/simon.lua: the behaviour of the NPC Simon the Beggar, converted from
-- simon.npc by patella-convert. The NPC is its [[npc]] entry in npcs.toml;
-- lib/npc.lua matches and runs the rules below, in the order of the file
-- (scripting.hh, "NPCs"). The comments are those of simon.npc.
--
-- GIMUD - Graphical Interface Multi User Dungeon
-- simon.npc: Datenbank für den Bettler Simon auf der Insel Fibula

local behaviour = require("lib.npc")
local ADDRESS = behaviour.ADDRESS
local BUSY = behaviour.BUSY
local VANISH = behaviour.VANISH
local FORCE = behaviour.FORCE
local eq = behaviour.eq
local ge = behaviour.ge
local TOPIC = behaviour.TOPIC
local PRICE = behaviour.PRICE
local TYPE = behaviour.TYPE
local COUNTMONEY = behaviour.COUNTMONEY
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
	{{ADDRESS, "hello$", FORCE}, {"Hello %N. I am a poor man. Please help me."}},
	{{ADDRESS, "hi$", FORCE}, {previous}},
	{{ADDRESS, FORCE}, {idle}},
	{{BUSY, "hello$", FORCE}, {"Please wait, %N.", queue}},
	{{BUSY, "hi$", FORCE}, {previous}},
	{{BUSY, FORCE}, {nop}},
	{{VANISH, FORCE}, {"Have a nice day."}},

	{{"bye"}, {"Have a nice day.", idle}},
	{{"farewell"}, {previous}},
	{{"job"}, {"I have no job. I am a beggar."}},
	{{"beggar"}, {"I have no gold and no job, so I am a beggar."}},
	{{"gold"}, {"I need gold. I love gold. I need help."}},
	{{"name"}, {"My name is Simon. I am a very poor man."}},
	{{"simon"}, {"I am Simon. The poorest human all over the continent."}},

	{{"help"}, {setPrice(100), "I need gold. Can you spend me %P gold pieces?", setTopic(1)}},
	{{eq(TOPIC, 1), "yes", ge(COUNTMONEY, PRICE)}, {deleteMoney, setPrice(500), "Thank you very much. Can you spend me %P more gold pieces? I will give you a nice hint.", setTopic(2)}},
	{{eq(TOPIC, 1), "yes"}, {"You've not enough money for me."}},
	{{eq(TOPIC, 1)}, {"Hmm, maybe next time."}},
	{{eq(TOPIC, 2), "yes", ge(COUNTMONEY, PRICE)}, {deleteMoney, setPrice(200), "That's great! I have stolen something from Dermot. You can buy it for %P gold. Do you want to buy it?", setTopic(3)}},
	{{eq(TOPIC, 2), "yes"}, {"Sorry, that's not enough."}},
	{{eq(TOPIC, 2)}, {"It was your decision."}},
	{{eq(TOPIC, 3), "yes", ge(COUNTMONEY, PRICE)}, {deleteMoney, "Now you own the hot key.", setData(3940), create(2968)}},
	{{eq(TOPIC, 3), "yes"}, {"Pah! I said 200 gold. You don't have so much."}},
	{{eq(TOPIC, 3)}, {"Ok. No problem. I'll find another buyer."}},

	{{"dermot"}, {"The magistrate of the village. I heard he is selling something for the Fibula Dungeon."}},
	{{"village"}, {"To the north is the village Fibula. A very small village."}},
	{{"key"}, {"Key? There are a lot of keys. Please change the topic."}},
	{{"dungeon"}, {"I heard a lot about the Fibula Dungeon. But I never was there."}},
	{{"fibula"}, {"I hate Fibula. Too many wolves are here."}},
	{{"timur"}, {"I hate Timur. He is too expensive. But sometimes I find maces and hatchets. Timur is buying these items."}},
	{{"wolf"}, {"Please kill them ... ALL."}},
	{{"flute"}, {"Har, har. The stupid Dermot lost his flute. I know that some minotaurs have it in their treasure room."}},
	{{"minotaurs"}, {"Very rich monsters. But they are too strong for me. However, there are even stronger monsters."}},
	{{"minos"}, {previous}},
	{{"treasure"}, {"I know there are two rooms. And I know you can pass only the first door. The second door can't be opened."}},
	{{"giant", "spider"}, {"I know that terrible monster. It killed the fishers on the isle to the north."}},
	{{"monster"}, {"The strongest monster I know is the giant spider."}},
	{{"jetty"}, {"I hate this jetty. I have never seen a ship here."}},
	{{"ship"}, {"There is a large sea-monster outside. I think there is no gritty captain to sail in this quarter."}},
	{{"tibia"}, {"Hehe, do you have a shovel? I can sell you a shovel if you want to return to Tibia."}},

	{{"shovel"}, {setType(3457), setAmount(1), setPrice(50), "Do you want to buy a shovel for %P gold?", setTopic(4)}},

	{{eq(TOPIC, 4), "yes", ge(COUNTMONEY, PRICE)}, {"Here you are.", deleteMoney, create(TYPE)}},
	{{eq(TOPIC, 4), "yes"}, {"Come back, when you have enough money."}},
	{{eq(TOPIC, 4)}, {"Hmm, but next time."}},
}

function onCreatureSay(npc, creature, text, situation)
	behaviour.react(npc, rules, text, situation)
end

function onCreatureDisappear(npc, creature)
	behaviour.react(npc, rules, "", NPC_SITUATION_VANISH)
end
