-- npcs/dermot.lua: the behaviour of the NPC Dermot, converted from
-- dermot.npc by patella-convert. The NPC is its [[npc]] entry in npcs.toml;
-- lib/npc.lua matches and runs the rules below, in the order of the file
-- (scripting.hh, "NPCs"). The comments are those of dermot.npc.
--
-- GIMUD - Graphical Interface Multi User Dungeon
-- dermot.npc: Datenbank für den Ortsvorsteher Dermot auf der Insel Fibula

local behaviour = require("lib.npc")
local ADDRESS = behaviour.ADDRESS
local BUSY = behaviour.BUSY
local VANISH = behaviour.VANISH
local FORCE = behaviour.FORCE
local eq = behaviour.eq
local ge = behaviour.ge
local TOPIC = behaviour.TOPIC
local PRICE = behaviour.PRICE
local AMOUNT = behaviour.AMOUNT
local TYPE = behaviour.TYPE
local COUNTMONEY = behaviour.COUNTMONEY
local count = behaviour.count
local questValue = behaviour.questValue
local setTopic = behaviour.setTopic
local setPrice = behaviour.setPrice
local setAmount = behaviour.setAmount
local setType = behaviour.setType
local setData = behaviour.setData
local setQuestValue = behaviour.setQuestValue
local create = behaviour.create
local delete = behaviour.delete
local deleteMoney = behaviour.deleteMoney
local queue = behaviour.queue
local idle = behaviour.idle
local nop = behaviour.nop
local previous = behaviour.previous

local rules = behaviour.rules {
	{{ADDRESS, "hello$", FORCE}, {"Hello, traveller %N. How can I help you?"}},
	{{ADDRESS, "hi$", FORCE}, {previous}},
	{{ADDRESS, FORCE}, {idle}},
	{{BUSY, "hello$", FORCE}, {"Sorry, I cannot talk to two persons at the same time. You'll have to wait, %N.", queue}},
	{{BUSY, "hi$", FORCE}, {previous}},
	{{BUSY, FORCE}, {nop}},
	{{VANISH, FORCE}, {"See you again."}},

	{{"bye"}, {"See you again.", idle}},
	{{"farewell"}, {previous}},
	{{"job"}, {"I am the magistrate of this isle."}},
	{{"equipment"}, {"I am not selling equipment. You'll have to visit Timur."}},
	{{"timur"}, {"He is the salesman in this village. "}},
	{{"name"}, {"I am Dermot, the magistrate of this isle."}},
	{{"time"}, {"Time is not important on Fibula."}},
	{{"dermot"}, {"I am the magistrate of this isle."}},
	{{"magistrate"}, {"Thats me."}},
	{{"fibula"}, {"You are at Fibula. This isle is not very dangerous. Just the wolves bother outside the village."}},
	{{"wolf"}, {"There are a lot of wolves outside the townwall. They disturb our farmers."}},
	{{"farmer"}, {"The inhabitants of Fibula live on fishing, farming, and hunting."}},

	{{eq(questValue(231), 1), "present"}, {setType(3218), setAmount(1), "You have a present for me?? Really?", setTopic(2)}},
	{{"present"}, {"I don't understand what you are talking about."}},
	{{eq(TOPIC, 2), "yes", ge(count(TYPE), AMOUNT)}, {"Thank you very much!", delete(TYPE), setQuestValue(231, 2)}},
	{{eq(TOPIC, 2), "yes"}, {"What? There is no present, at least none for me! Stop this foolish jokes!", idle}},
	{{eq(TOPIC, 2)}, {"Hmm, maybe next time."}},

	{{"dungeon"}, {"Oh, my god. In the dungeon of Fibula are a lot of monsters. That's why we have sealed it with a solid door."}},
	{{"sewer"}, {previous}},
	{{"monster"}, {previous}},
	{{"entrance"}, {"The entrance is near here."}},
	{{"key"}, {setType(2968), setData(3940), setAmount(1), setPrice(2000), "Do you want to buy the dungeon key for %P gold?", setTopic(1)}},
	{{"door"}, {previous}},

	{{eq(TOPIC, 1), "yes", ge(COUNTMONEY, PRICE)}, {"Now you own the key to the dungeon.", deleteMoney, create(TYPE)}},
	{{eq(TOPIC, 1), "yes"}, {"You've not enough money to buy the key."}},
	{{eq(TOPIC, 1)}, {"Hmm, maybe next time."}},
}

function onCreatureSay(npc, creature, text, situation)
	behaviour.react(npc, rules, text, situation)
end

function onCreatureDisappear(npc, creature)
	behaviour.react(npc, rules, "", NPC_SITUATION_VANISH)
end
