-- npcs/gundralph.lua: the behaviour of the NPC Gundralph, converted from
-- gundralph.npc by patella-convert. The NPC is its [[npc]] entry in npcs.toml;
-- lib/npc.lua matches and runs the rules below, in the order of the file
-- (scripting.hh, "NPCs"). The comments are those of gundralph.npc.
--
-- GIMUD - Graphical Interface Multi User Dungeon
-- gundralph.npc: Datenbank für den Zauberlehrer Gundralph

local behaviour = require("lib.npc")
local ADDRESS = behaviour.ADDRESS
local BUSY = behaviour.BUSY
local VANISH = behaviour.VANISH
local SORCERER = behaviour.SORCERER
local DRUID = behaviour.DRUID
local FORCE = behaviour.FORCE
local eq = behaviour.eq
local lt = behaviour.lt
local TOPIC = behaviour.TOPIC
local PRICE = behaviour.PRICE
local TYPE = behaviour.TYPE
local LEVEL = behaviour.LEVEL
local COUNTMONEY = behaviour.COUNTMONEY
local spellKnown = behaviour.spellKnown
local spellLevel = behaviour.spellLevel
local setTopic = behaviour.setTopic
local setPrice = behaviour.setPrice
local setAmount = behaviour.setAmount
local setType = behaviour.setType
local effectOpp = behaviour.effectOpp
local teachSpell = behaviour.teachSpell
local deleteMoney = behaviour.deleteMoney
local queue = behaviour.queue
local idle = behaviour.idle
local nop = behaviour.nop
local previous = behaviour.previous

local rules = behaviour.rules {
	{{ADDRESS, "hello$", FORCE}, {"Oh, be welcome %N."}},
	{{ADDRESS, "hi$", FORCE}, {previous}},
	{{ADDRESS, FORCE}, {idle}},
	{{BUSY, "hello$", FORCE}, {"Wait %N, please.", queue}},
	{{BUSY, "hi$", FORCE}, {previous}},
	{{BUSY, FORCE}, {nop}},
	{{VANISH, FORCE}, {"Goodbye."}},

	{{"bye"}, {"Goodbye", idle}},
	{{"job"}, {"I am a teacher for some powerful Spells."}},
	{{"name"}, {"They call me Gundralph."}},
	{{"time"}, {"Let me see, it's %T."}},
	{{"king"}, {"Unfortunately, I never met King Tibianus III in person."}},
	{{"tibianus"}, {previous}},
	{{"army"}, {"They live in the castle to the west."}},
	{{"ferumbras"}, {"How low can a sorceror sink."}},
	{{"excalibug"}, {"An awesome weapon if it exists."}},
	{{"thais"}, {"I see Thais as a lost course."}},
	{{"tibia"}, {"The world is so big and we have only so little time to travel."}},
	{{"carlin"}, {"Carlin is a fine place for druids."}},
	{{"edron"}, {"Sciences are thriving on this isle."}},
	{{"news"}, {"I have heard nothing of intrest lately, sorry."}},
	{{"rumors"}, {previous}},

	{{"spellbook"}, {"Please ask the stationer in the west tower for that."}},
	{{"spell"}, {"I have 'Ultimate Light', 'Soulfire', 'Magic Wall', 'Cancel Invisibility', and 'Undead Legion'. Are you interested?"}},

	{{"ultimate", "light", SORCERER}, {setType(75), setPrice(1600), "Do you want to learn the spell 'Ultimate Light' for %P gold?", setTopic(1)}},
	{{"ultimate", "light", DRUID}, {previous}},
	{{"ultimate", "light"}, {"I'm sorry, but this spell is only for druids and sorcerers."}},
	{{"soulfire", SORCERER}, {setType(50), setPrice(1800), "Do you want to learn the spell 'Soulfire' for %P gold?", setTopic(1)}},
	{{"soulfire", DRUID}, {previous}},
	{{"soulfire"}, {"I'm sorry, but this spell is only for druids and sorcerers."}},
	{{"magic", "wall", SORCERER}, {setType(86), setPrice(2100), "Do you want to learn the spell 'Magic Wall' for %P gold?", setTopic(1)}},
	{{"magic", "wall"}, {"I'm sorry, but this spell is only for sorcerers."}},
	{{"undead", "legion", DRUID}, {setType(85), setPrice(2000), "Do you want to learn the spell 'Undead Legion' for %P gold?", setTopic(1)}},
	{{"undead", "legion"}, {"I'm sorry, but this spell is only for druids."}},
	{{"cancel", "invisibility", SORCERER}, {setType(90), setPrice(1600), "Do you want to learn the spell 'Cancel Invisibility' for %P gold?", setTopic(1)}},
	{{"cancel", "invisibility"}, {"I'm sorry, but this spell is only for sorcerers."}},

	{{eq(TOPIC, 1), "yes", eq(spellKnown(TYPE), 1)}, {"Hmm, you already know this spell."}},
	{{eq(TOPIC, 1), "yes", lt(LEVEL, spellLevel(TYPE))}, {setAmount(spellLevel(TYPE)), "Hmm, you need to advance to level %A to learn this spell."}},
	{{eq(TOPIC, 1), "yes", lt(COUNTMONEY, PRICE)}, {"Hmm, you don't have enough money to pay my service."}},
	{{eq(TOPIC, 1), "yes"}, {"Voila, from now on you can cast this spell. Use your knowledge wisely.", deleteMoney, effectOpp(13), teachSpell(TYPE)}},
	{{eq(TOPIC, 1)}, {"Hmm, but next time."}},
}

function onCreatureSay(npc, creature, text, situation)
	behaviour.react(npc, rules, text, situation)
end

function onCreatureDisappear(npc, creature)
	behaviour.react(npc, rules, "", NPC_SITUATION_VANISH)
end
