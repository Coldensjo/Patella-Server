-- npcs/windtrouser.lua: the behaviour of the NPC Dalbrect, converted from
-- windtrouser.npc by patella-convert. The NPC is its [[npc]] entry in npcs.toml;
-- lib/npc.lua matches and runs the rules below, in the order of the file
-- (scripting.hh, "NPCs"). The comments are those of windtrouser.npc.
--
-- GIMUD - Graphical Interface Multi User Dungeon
-- windtrouser.npc: Fischer Dalbrect Windtrouser nahe Mönchsinsel

local behaviour = require("lib.npc")
local ADDRESS = behaviour.ADDRESS
local BUSY = behaviour.BUSY
local VANISH = behaviour.VANISH
local PZBLOCK = behaviour.PZBLOCK
local FORCE = behaviour.FORCE
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
local questValue = behaviour.questValue
local setTopic = behaviour.setTopic
local setPrice = behaviour.setPrice
local setAmount = behaviour.setAmount
local setType = behaviour.setType
local setQuestValue = behaviour.setQuestValue
local effectOpp = behaviour.effectOpp
local delete = behaviour.delete
local deleteMoney = behaviour.deleteMoney
local queue = behaviour.queue
local idle = behaviour.idle
local nop = behaviour.nop
local teleport = behaviour.teleport
local previous = behaviour.previous

local rules = behaviour.rules {
	{{ADDRESS, "hello$", gt(questValue(220), 0), eq(questValue(62), 2), FORCE}, {"Sorry my friend, the monks don't allow me to talk to you because of your intrusion in their chambers. Unless you plea to the abbot for absolution I can do nothing for you.", idle}},
	{{ADDRESS, "hi$", gt(questValue(220), 0), eq(questValue(62), 2), FORCE}, {previous}},

	{{ADDRESS, "hello$", gt(questValue(220), 0), FORCE}, {"The monks forbid me to talk to you for you evil deeds.", idle}},
	{{ADDRESS, "hi$", gt(questValue(220), 0), FORCE}, {previous}},

	{{ADDRESS, "hello$", FORCE}, {"Be greeted, traveler."}},
	{{ADDRESS, "hi$", FORCE}, {previous}},
	{{ADDRESS, FORCE}, {idle}},

	{{BUSY, "hello$", gt(questValue(220), 0), eq(questValue(62), 2), FORCE}, {"Sorry %N, the monks don't allow me to talk to you because of your intrusion in their chambers. Unless you plea to the abbot for absolution I can do nothing for you.", idle}},
	{{BUSY, "hi$", gt(questValue(220), 0), eq(questValue(62), 2), FORCE}, {previous}},

	{{BUSY, "hello$", gt(questValue(220), 0), FORCE}, {"Sorry %N, the monks forbid me to talk to you for you evil deeds.", idle}},
	{{BUSY, "hi$", gt(questValue(220), 0), FORCE}, {previous}},

	{{BUSY, "hello$", FORCE}, {"One moment please.", queue}},
	{{BUSY, "hi$", FORCE}, {previous}},
	{{BUSY, FORCE}, {nop}},

	{{VANISH, FORCE}, {"Good bye. You are welcome."}},

	{{"bye"}, {"Good bye. You are welcome.", idle}},
	{{"farewell"}, {previous}},
	{{"name"}, {"My name is Dalbrect Windtrouser, of the once proud windtrouser family."}},
	{{"job"}, {"I am merely a humble fisher now that nothing is left of my noble legacy."}},
	{{"captain"}, {previous}},
	{{"ship"}, {"My ship is my only pride and joy."}},
	{{"ferry"}, {previous}},
	{{"legacy"}, {"Once my family was once noble and wealthy, but fate turned against us and threw us into poverty."}},
	{{"family"}, {previous}},
	{{"nobility"}, {previous}},
	{{"fate"}, {"When Carlin tried to colonize the region now known as the ghostlands, my ancestors put their fortune in that project."}},
	{{"poverty"}, {previous}},
	{{"project"}, {"Our family fortune was lost when the colonization of those cursed lands failed. Now nothing is left of our fame or our fortune. If I only had something as a reminder of those better times. <sigh>"}},
	{{"ghostlands"}, {previous}},

	{{"brooch", eq(questValue(62), 2), FORCE}, {"You have recovered my brooch! I shall forever be in your debt, my friend!"}},
	{{"brooch"}, {"What? You want me to examine a brooch?", setType(3205), setAmount(1), setTopic(3)}},
	{{eq(TOPIC, 3), "yes", lt(count(TYPE), AMOUNT)}, {"What are you talking about? I am too poor to be interested in jewelry."}},
	{{eq(TOPIC, 3), "yes", ge(count(TYPE), AMOUNT)}, {"Can it be? I recognize my family's arms! You have found a treasure indeed! I am poor and all I can offer you is my friendship, but ... please ... give that brooch to me?", setTopic(2)}},
	{{eq(TOPIC, 3)}, {"Then stop being a fool. I am poor and I have to work the whole day through!"}},

	{{eq(TOPIC, 2), "yes", ge(count(TYPE), AMOUNT)}, {"Thank you! I shall consider you my friend from now on! Just let me know if you need something!", delete(TYPE), setQuestValue(62, 2)}},
	{{eq(TOPIC, 2), "yes"}, {"I should have known better then to ask for an act of kindness in this cruel, selfish, world!"}},
	{{eq(TOPIC, 2)}, {previous}},

	{{"trip"}, {"I have only sailed to the isle of the kings once or twice. I dare not anger the monks by bringing travellers there without their permission."}},
	{{"passage"}, {previous}},
	{{"carlin"}, {"To think my family used to belong to the local nobility! And now those arrogant women are in charge!"}},
	{{"island"}, {"The only isle I visit regularly is the isle of the kings. I bring food and the occasional visitor to the monastery."}},
	{{"monastery"}, {"The monks are not exactly fond of visitors, so I rarely take somebody there without their permission."}},
	{{"white", "raven"}, {"I think that is the name both of the monastery and of the monks' order."}},
	{{"passage", eq(questValue(62), 1)}, {setPrice(20), "Since you have the abbot's permission I can sail you to the isle of the kings for %P gold. Is that ok for you?", setTopic(1)}},
	{{"passage", eq(questValue(62), 2)}, {setPrice(10), "Since you are my friend now I will sail you to the isle of the kings for %P gold. Is that okay for you?", setTopic(1)}},
	{{"passage"}, {"You do not have the abbot's permission, and I won't risk angering the monks because of some guy I do not even know."}},

	{{eq(TOPIC, 1), "yes", PZBLOCK, FORCE}, {"First get rid of those blood stains! You are not going to ruin my vehicle!"}},

	{{eq(TOPIC, 1), "yes", ge(COUNTMONEY, PRICE)}, {"Have a nice trip!", deleteMoney, idle, effectOpp(11), teleport(32190, 31957, 6), effectOpp(11)}},
	{{eq(TOPIC, 1), "yes"}, {"You don't have enough money."}},
	{{eq(TOPIC, 1)}, {"Well, I'll be here if you change your mind."}},
}

function onCreatureSay(npc, creature, text, situation)
	behaviour.react(npc, rules, text, situation)
end

function onCreatureDisappear(npc, creature)
	behaviour.react(npc, rules, "", NPC_SITUATION_VANISH)
end
