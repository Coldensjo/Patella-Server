-- npcs/orcking.lua: the behaviour of the NPC The Orc King, converted from
-- orcking.npc by patella-convert. The NPC is its [[npc]] entry in npcs.toml;
-- lib/npc.lua matches and runs the rules below, in the order of the file
-- (scripting.hh, "NPCs"). The comments are those of orcking.npc.
--
-- GIMUD - Graphical Interface Multi User Dungeon
-- orcking.npc: Datenbank für den Orckönig

local behaviour = require("lib.npc")
local ADDRESS = behaviour.ADDRESS
local BUSY = behaviour.BUSY
local VANISH = behaviour.VANISH
local FORCE = behaviour.FORCE
local eq = behaviour.eq
local TOPIC = behaviour.TOPIC
local questValue = behaviour.questValue
local setTopic = behaviour.setTopic
local setAmount = behaviour.setAmount
local setQuestValue = behaviour.setQuestValue
local summon = behaviour.summon
local create = behaviour.create
local idle = behaviour.idle
local nop = behaviour.nop
local previous = behaviour.previous

local rules = behaviour.rules {
	{{ADDRESS, "hello$", eq(questValue(218), 0), FORCE}, {"Arrrrgh! A dirty paleskin! To me my children! Kill them my guards!", setQuestValue(218, 1), summon(19), summon(19), summon(19), summon(2), summon(2), summon(59), summon(59), summon(59), idle}},

	{{ADDRESS, "hi$", eq(questValue(218), 0), FORCE}, {previous}},
	{{ADDRESS, "hello$", FORCE}, {"Harrrrk! You think you are strong now? You shall never escape my wrath! I am immortal!"}},
	{{ADDRESS, "hi$", FORCE}, {previous}},

	{{ADDRESS, FORCE}, {idle}},
	{{BUSY, "hello$", FORCE}, {"Harrrk!"}},
	{{BUSY, "hi$", FORCE}, {previous}},
	{{BUSY, FORCE}, {nop}},
	{{VANISH, FORCE}, {"Yes, flee this place, but you will never escape my revenge!"}},

	{{"bye"}, {"We will meet again.", idle}},
	{{"farewell"}, {previous}},

	{{"name"}, {"I am Charkahn the Slayer! The immortal father of the orcs and master of this hive."}},
	{{"job"}, {previous}},
	{{"hive"}, {"I can sense the presence and the feelings of my underlings and minions. I embrace the rage of the horde."}},
	{{"minion"}, {"The orcish horde of this hive is under my control. I sense their emotions and their needs and provide them with the leadership they need to focus their hate and rage."}},
	{{"underling"}, {previous}},
	{{"horde"}, {previous}},
	{{"hate"}, {"Hate and rage are the true blessings of Blog, since they are powerful weapons. They give the hive strength. I provide them with direction and focus."}},
	{{"rage"}, {previous}},
	{{"direction"}, {"To conquer, to destroy and to dominate. Orcs are born to rule the world."}},
	{{"focus"}, {previous}},
	{{"blog"}, {"The Raging One blessed us with his burning hate. We are truly his children and therefore divine."}},
	{{"divine"}, {"The orcs are the bearers of Blogs rage. This makes us the ultimate fighters and the most powerful of all races."}},
	{{"orc"}, {previous}},
	{{"slime"}, {"Pah! Don't mock me, mortal! This shape is a curse which the evil djinn bestowed upon me!"}},
	{{"djinn"}, {"This cursed djinn king! I set him free from an enchanted lamp, and he cheated me!"}},
	{{"malor"}, {previous}},
	{{"cheat"}, {"Because I freed him he granted me three wishes. He was true to his word in the first two wishes."}},
	{{"wish"}, {"He built this fortress over Uldrek's grave within a single night. Also, he granted me my second wish and gave me immortality. Test it and try to kill me if you want. Har Har!"}},
	{{"third"}, {"I wished to father more healthy and fertile children as any orc has ever done. But the djinn cheated me and made me a slime! Then he laughed at me and left for his abandoned fortress in the Deathwish Mountains."}},
	{{"deathwish"}, {"His ancient fortress on Darama was deserted as the evil Djinn fled this world after his imprisonment. Now the time has come for the evil Djinns to return to their master although this will certainly awaken the good Djinn too."}},
	{{"abandoned"}, {previous}},
	{{"good", "djinn"}, {"I will not share anything more about that topic with you paleskins."}},
	{{"awaken"}, {previous}},
	{{"paleskins"}, {"You are as ugly as maggots, although not quite as as tasty."}},
	{{"lamp"}, {"For Eons he was trapped in an enchanted lamp by some ancient race. Now he's free to roam the world again. Although he cheated me I appreciate what he and his brethren will do to this world, now it's the time of the Djinn again!"}},
	{{"lamp", eq(questValue(283), 1), eq(questValue(284), 0), FORCE}, {"I can sense your evil intentions to imprison a djinn! You are longing for the lamp, which I still possess. ...", "Who do you want to trap in this cursed lamp?", setTopic(1)}},
	{{"lamp", eq(questValue(288), 1), eq(questValue(284), 0), FORCE}, {previous}},
	{{eq(TOPIC, 1), "malor", FORCE}, {"I was waiting for this day! Take the lamp and let Malor feel my wrath!", setAmount(1), create(3231), setQuestValue(284, 1)}},
	{{eq(TOPIC, 1)}, {"I don't know your enemy, paleskin! Begone!", idle}},
}

function onCreatureSay(npc, creature, text, situation)
	behaviour.react(npc, rules, text, situation)
end

function onCreatureDisappear(npc, creature)
	behaviour.react(npc, rules, "", NPC_SITUATION_VANISH)
end
