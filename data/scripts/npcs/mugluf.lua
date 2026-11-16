-- npcs/mugluf.lua: the behaviour of the NPC Mugluf, converted from
-- mugluf.npc by patella-convert. The NPC is its [[npc]] entry in npcs.toml;
-- lib/npc.lua matches and runs the rules below, in the order of the file
-- (scripting.hh, "NPCs"). The comments are those of mugluf.npc.
--
-- GIMUD - Graphical Interface Multi User Dungeon
-- mugluf.npc: Datenbank für den Fleischhändler Mugluf

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
	{{ADDRESS, "hello$", FORCE}, {"Greetings %N, seeker of delicacies."}},
	{{ADDRESS, "hi$", FORCE}, {previous}},
	{{ADDRESS, FORCE}, {idle}},
	{{BUSY, "hello$", FORCE}, {"I will talk to you as soon as I finished this deal, %N.", queue}},
	{{BUSY, "hi$", FORCE}, {previous}},
	{{BUSY, FORCE}, {nop}},
	{{VANISH, FORCE}, {"May Daraman's wisdom enlighten your soul."}},

	{{"bye"}, {"May Daraman's wisdom enlighten your soul.", idle}},
	{{"name"}, {"I am known as Mugluf the younger."}},
	{{"job"}, {"I sell meat, ham and salmon."}},
	{{"time"}, {"Time is nothing but another illusion."}},
	{{"caliph"}, {"Caliph Kazzan is our worldly leader."}},
	{{"kazzan"}, {previous}},
	{{"ferumbras"}, {"Who might that be?"}},
	{{"noodles"}, {"That must be an important Thaian noble. Regulary Darashian delicacies are sent for him to Thais."}},
	{{"excalibug"}, {"O seeker of artifacts, I have no need for other weapons then a fork, a spoon, and a knife."}},
	{{"thais"}, {"I think we have some kind of trade agreements with them."}},
	{{"tibia"}, {"The world is an illusion, don't get trapped in it."}},
	{{"carlin"}, {"This town must be far away, we rarely even hear about it here."}},
	{{"news"}, {"People say there is a dark cloud gathering in the west."}},
	{{"rumour"}, {previous}},
	{{"desert"}, {"It's a challenge to body and soul. Praised be Daraman to have brought us here to grow on this constant test."}},

	{{"offer"}, {"I can offer you meat, ham, and salmon."}},
	{{"do", "you", "sell"}, {previous}},
	{{"do", "you", "have"}, {previous}},
	{{"food"}, {"Are you looking for food? I have meat, ham, and salmon."}},

	{{"meat"}, {setType(3577), setAmount(1), setPrice(7), "Do you want to buy meat for %P gold?", setTopic(1)}},
	{{"ham"}, {setType(3582), setAmount(1), setPrice(10), "Do you want to buy a ham for %P gold?", setTopic(1)}},
	{{"salmon"}, {setType(3579), setAmount(1), setPrice(6), "Do you want to buy a salmon for %P gold?", setTopic(1)}},

	{{P1, lt(1, P1), "meat"}, {setType(3577), setAmount(P1), setPrice(mul(7, P1)), "Do you want to buy %A meat for %P gold?", setTopic(1)}},
	{{P1, lt(1, P1), "ham"}, {setType(3582), setAmount(P1), setPrice(mul(10, P1)), "Do you want to buy %A ham for %P gold?", setTopic(1)}},
	{{P1, lt(1, P1), "salmon"}, {setType(3579), setAmount(P1), setPrice(mul(6, P1)), "Do you want to buy %A salmon for %P gold?", setTopic(1)}},

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
