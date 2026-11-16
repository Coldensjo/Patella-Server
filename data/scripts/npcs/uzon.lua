-- npcs/uzon.lua: the behaviour of the NPC Uzon, converted from
-- uzon.npc by patella-convert. The NPC is its [[npc]] entry in npcs.toml;
-- lib/npc.lua matches and runs the rules below, in the order of the file
-- (scripting.hh, "NPCs"). The comments are those of uzon.npc.
--
-- GIMUD - Graphical Interface Multi User Dungeon
-- uzon: Datenbank für den Teppichpiloten Uzon auf den Femor Hills

local behaviour = require("lib.npc")
local ADDRESS = behaviour.ADDRESS
local BUSY = behaviour.BUSY
local VANISH = behaviour.VANISH
local PREMIUM = behaviour.PREMIUM
local PZBLOCK = behaviour.PZBLOCK
local FORCE = behaviour.FORCE
local eq = behaviour.eq
local gt = behaviour.gt
local ge = behaviour.ge
local TOPIC = behaviour.TOPIC
local PRICE = behaviour.PRICE
local COUNTMONEY = behaviour.COUNTMONEY
local questValue = behaviour.questValue
local setTopic = behaviour.setTopic
local setPrice = behaviour.setPrice
local setQuestValue = behaviour.setQuestValue
local effectOpp = behaviour.effectOpp
local deleteMoney = behaviour.deleteMoney
local queue = behaviour.queue
local idle = behaviour.idle
local nop = behaviour.nop
local teleport = behaviour.teleport
local previous = behaviour.previous

local rules = behaviour.rules {
	{{ADDRESS, "hello$", FORCE}, {"Daraman's blessings, traveller %N."}},
	{{ADDRESS, "hi$", FORCE}, {previous}},
	{{ADDRESS, FORCE}, {idle}},
	{{BUSY, "hello$", FORCE}, {"Hastiness is not the way of the people of Darama, %N. Give me the time I need here.", queue}},
	{{BUSY, "hi$", FORCE}, {previous}},
	{{BUSY, FORCE}, {nop}},
	{{VANISH, FORCE}, {"Daraman's blessings."}},

	{{"bye"}, {"Daraman's blessings", idle}},
	{{"name"}, {"I am known as Uzon Ibn Kalith."}},
	{{"job"}, {"I am a licensed Darashian carpetpilot. I can bring you to Darashia or Edron."}},
	{{"time"}, {"It's %T right now. The next flight is scheduled soon."}},
	{{"caliph"}, {"The caliph welcomes travellers to his land."}},
	{{"kazzan"}, {previous}},
	{{"daraman"}, {"Oh, there is so much to tell about Daraman. You better travel to Darama to learn about his teachings."}},
	{{"ferumbras"}, {"I would never transport this one."}},
	{{"drefia"}, {"So you heared about haunted Drefia? Many adventures travel there to test their skills against the undead: vampires, mummies, and ghosts."}},
	{{"excalibug"}, {"Some people claim it is hidden somewhere under the endless sands of the devourer desert in Darama."}},
	{{"thais"}, {"Thais is noisy and overcroweded. That's why I like Darashia more."}},
	{{"tibia"}, {"I have seen almost every place on the continent."}},
	{{"continent"}, {"I could retell the tales of my travels for hours. Sadly another flight is scheduled soon."}},
	{{"carlin"}, {"Just another Thais but with women to lead them."}},
	{{"flying", "carpet"}, {"You can buy flying carpets only in Darashia."}},
	{{"fly"}, {"I transport travellers to the continent of Darama for a small fee. So many want to see the wonders of the desert and learn the secrets of Darama."}},
	{{"news"}, {"I heard too many news to recall them all."}},
	{{"rumors"}, {previous}},

	{{"passage"}, {"I can fly you to Darashia on Darama or Edron if you like. Where do you want to go?"}},
	{{"transport"}, {previous}},
	{{"ride"}, {previous}},
	{{"trip"}, {previous}},

	{{"darashia"}, {setPrice(60), "Do you want to get a ride to Darashia on Darama for %P gold?", setTopic(1)}},
	{{"darama"}, {previous}},
	{{"edron"}, {setPrice(60), "Do you want to get a ride to Edron for %P gold?", setTopic(2)}},

	{{"darashia", gt(questValue(250), 2)}, {setPrice(50), "Do you want to get a ride to Darashia on Darama for %P gold?", setTopic(1)}},
	{{"darama", gt(questValue(250), 2)}, {previous}},
	{{"edron", gt(questValue(250), 2)}, {setPrice(50), "Do you want to get a ride to Edron for %P gold?", setTopic(2)}},

	{{eq(TOPIC, 1), "yes", PZBLOCK, FORCE}, {"First get rid of those blood stains! You are not going to ruin my vehicle!"}},
	{{eq(TOPIC, 2), "yes", PZBLOCK, FORCE}, {previous}},

	{{eq(TOPIC, 1), "yes", PREMIUM, ge(COUNTMONEY, PRICE)}, {"Hold on!", deleteMoney, idle, effectOpp(11), teleport(33269, 32441, 6), effectOpp(11)}},
	{{eq(TOPIC, 1), "yes", ge(COUNTMONEY, PRICE)}, {"I'm sorry, but you need a premium account in order to enter Darama."}},
	{{eq(TOPIC, 1), "yes"}, {"You don't have enough money."}},
	{{eq(TOPIC, 1)}, {"You shouldn't miss the experience."}},

	-- für postquest
	{{eq(TOPIC, 2), "yes", PREMIUM, eq(questValue(227), 2), ge(COUNTMONEY, PRICE)}, {"Hold on!", deleteMoney, idle, effectOpp(11), teleport(33193, 31784, 3), effectOpp(11), setQuestValue(227, 3)}},

	{{eq(TOPIC, 2), "yes", PREMIUM, ge(COUNTMONEY, PRICE)}, {"Hold on!", deleteMoney, idle, effectOpp(11), teleport(33193, 31784, 3), effectOpp(11)}},
	{{eq(TOPIC, 2), "yes", ge(COUNTMONEY, PRICE)}, {"I'm sorry, but you need a premium account in order to enter Edron."}},
	{{eq(TOPIC, 2), "yes"}, {"You don't have enough money."}},
	{{eq(TOPIC, 2)}, {"You shouldn't miss the experience."}},
}

function onCreatureSay(npc, creature, text, situation)
	behaviour.react(npc, rules, text, situation)
end

function onCreatureDisappear(npc, creature)
	behaviour.react(npc, rules, "", NPC_SITUATION_VANISH)
end
