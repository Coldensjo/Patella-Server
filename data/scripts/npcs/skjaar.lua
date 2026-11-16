-- npcs/skjaar.lua: the behaviour of the NPC Skjaar, converted from
-- skjaar.npc by patella-convert. The NPC is its [[npc]] entry in npcs.toml;
-- lib/npc.lua matches and runs the rules below, in the order of the file
-- (scripting.hh, "NPCs"). The comments are those of skjaar.npc.
--
-- GIMUD - Graphical Interface Multi User Dungeon
-- skjaar.npc: Datenbank für Skjaar, Wächter der Krypta im Berg und Meistermagier

local behaviour = require("lib.npc")
local ADDRESS = behaviour.ADDRESS
local BUSY = behaviour.BUSY
local VANISH = behaviour.VANISH
local KNIGHT = behaviour.KNIGHT
local PALADIN = behaviour.PALADIN
local SORCERER = behaviour.SORCERER
local DRUID = behaviour.DRUID
local FORCE = behaviour.FORCE
local eq = behaviour.eq
local lt = behaviour.lt
local ge = behaviour.ge
local TOPIC = behaviour.TOPIC
local PRICE = behaviour.PRICE
local TYPE = behaviour.TYPE
local LEVEL = behaviour.LEVEL
local COUNTMONEY = behaviour.COUNTMONEY
local setTopic = behaviour.setTopic
local setPrice = behaviour.setPrice
local setAmount = behaviour.setAmount
local setType = behaviour.setType
local setData = behaviour.setData
local setHp = behaviour.setHp
local effectOpp = behaviour.effectOpp
local create = behaviour.create
local deleteMoney = behaviour.deleteMoney
local idle = behaviour.idle
local nop = behaviour.nop
local previous = behaviour.previous

local rules = behaviour.rules {
	{{BUSY, "hello$", lt(LEVEL, 15), FORCE}, {"I don't talk to little children!!", idle}},
	{{BUSY, "hi$", lt(LEVEL, 15), FORCE}, {previous}},
	{{ADDRESS, "hello$", DRUID, FORCE}, {"Hail, friend of nature! How may I help you?"}},
	{{ADDRESS, "hi$", DRUID, FORCE}, {previous}},
	{{ADDRESS, "hello$", KNIGHT, FORCE}, {"Another creature who believes thinks physical strength is more important than wisdom! Why are you disturbing me?"}},
	{{ADDRESS, "hi$", KNIGHT, FORCE}, {previous}},
	{{ADDRESS, "hello$", SORCERER, FORCE}, {"It's good to see somebody who has chosen the path of wisdom. What do you want?"}},
	{{ADDRESS, "hi$", SORCERER, FORCE}, {previous}},
	{{ADDRESS, "hello$", PALADIN, FORCE}, {"Neither strong enough to be a knight nor wise enough to be a real mage. You like it easy, don't you? Why are you disturbing me?"}},
	{{ADDRESS, "hi$", PALADIN, FORCE}, {previous}},
	{{ADDRESS, FORCE}, {idle}},
	{{BUSY, "hello$", FORCE}, {"Silence, unworthy creature!"}},
	{{BUSY, "hi$", FORCE}, {previous}},
	{{BUSY, FORCE}, {nop}},
	{{VANISH, FORCE}, {"Run away, unworthy %N!"}},

	{{"bye"}, {"Farewell, %N!", idle}},
	{{"farewell"}, {previous}},
	{{"job"}, {"Once I was the master of all mages, but now I only protect this crypt."}},
	{{"name"}, {"I am Skjaar the Mage, master of all spells."}},
	{{"door"}, {"This door seals a crypt."}},
	{{"crypt"}, {"Here lies my master. Only his closest followers may enter."}},
	{{"help"}, {"I'm not here to help anybody. I only protect my master's crypt."}},
	{{"mountain"}, {"Hundreds of years my master's castle stood on the top of this mountain. Now there is a volcano."}},
	{{"volcano"}, {"I can still feel the magical energy in the volcano."}},
	{{"castle"}, {"The castle was destroyed when my master tried to summon a nameless creature. All that is left is this volcano."}},
	{{"time"}, {"To those who have lived for a thousand years time holds no more terror."}},
	{{"master"}, {"If you are one of his followers, you need not ask about him, for you will know. And if you aren't, you are not worthy anyway!"}},
	{{"key"}, {"I will give the key to the crypt only to the closest followers of my master. Would you like me to test you?", setTopic(1)}},

	{{"idiot"}, {"Take this for your words!", setHp(1), effectOpp(14), idle}},
	{{"fuck"}, {"Take this for your words!", setHp(1), effectOpp(14), idle}},
	{{"asshole"}, {"Take this for your words!", setHp(1), effectOpp(14), idle}},

	{{eq(TOPIC, 1), "yes"}, {setPrice(1000), "Before we start I must ask you for a small donation of 1000 gold coins. Are you willing to pay 1000 gold coins for the test?", setTopic(2)}},
	{{eq(TOPIC, 1), "no"}, {"Then leave, unworthy worm!", idle}},
	{{eq(TOPIC, 1)}, {"You're not worthy if you cannot make up your mind. Leave!", idle}},

	{{eq(TOPIC, 2), "yes", ge(COUNTMONEY, PRICE)}, {"All right then. Here comes the first question. What was the name of Dago's favourite pet?", deleteMoney, setTopic(3)}},
	{{eq(TOPIC, 2), "yes", lt(COUNTMONEY, PRICE)}, {"You don't even have the money to make a donation? Then go!", idle}},
	{{eq(TOPIC, 2), "no"}, {"You're not worthy then. Now leave!", idle}},
	{{eq(TOPIC, 2)}, {"You're not worthy if you cannot make up your mind. Leave!", idle}},

	{{eq(TOPIC, 3), "redips", FORCE}, {"Perhaps you knew him after all. Tell me - how many fingers did he have when he died?", setTopic(4)}},
	{{eq(TOPIC, 3), FORCE}, {"You are wrong. Get lost!", idle}},

	{{eq(TOPIC, 4), "7", FORCE}, {"Also true. But can you also tell me the colour of the deamons in which master specialized?", setTopic(5)}},
	{{eq(TOPIC, 4), "seven", FORCE}, {"Also true. But can you also tell me the colour of the deamons in which my master specialized?", setTopic(5)}},
	{{eq(TOPIC, 4), FORCE}, {"You are wrong. Get lost!", idle}},

	{{eq(TOPIC, 5), "black", FORCE}, {"It seems you are worthy after all. Do you want the key to the crypt?", setTopic(6)}},
	{{eq(TOPIC, 5), FORCE}, {"You are wrong. Get lost!", idle}},

	{{eq(TOPIC, 6), "yes"}, {setType(2970), setData(3142), setAmount(1), "Here you are.", create(TYPE)}},
	{{eq(TOPIC, 6)}, {"It is always a wise decision to leave the dead alone."}},
}

function onCreatureSay(npc, creature, text, situation)
	behaviour.react(npc, rules, text, situation)
end

function onCreatureDisappear(npc, creature)
	behaviour.react(npc, rules, "", NPC_SITUATION_VANISH)
end
