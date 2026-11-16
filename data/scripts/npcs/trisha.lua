-- npcs/trisha.lua: the behaviour of the NPC Trisha, converted from
-- trisha.npc by patella-convert. The NPC is its [[npc]] entry in npcs.toml;
-- lib/npc.lua matches and runs the rules below, in the order of the file
-- (scripting.hh, "NPCs"). The comments are those of trisha.npc.
--
-- GIMUD - Graphical Interface Multi User Dungeon
-- trisha.npc: Datenbank fuer die Ritterin Trisha

local behaviour = require("lib.npc")
local ADDRESS = behaviour.ADDRESS
local BUSY = behaviour.BUSY
local VANISH = behaviour.VANISH
local MALE = behaviour.MALE
local FEMALE = behaviour.FEMALE
local KNIGHT = behaviour.KNIGHT
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
	{{ADDRESS, KNIGHT, "hello$", FORCE}, {"Welcome back, knight %N!"}},
	{{ADDRESS, KNIGHT, "hi$", FORCE}, {previous}},
	{{ADDRESS, "hello$", FORCE}, {"Salutations, %N. What can I do for you?"}},
	{{ADDRESS, "hi$", FORCE}, {previous}},
	{{ADDRESS, FORCE}, {idle}},
	{{BUSY, "hello$", FORCE}, {"Please wait a moment, %N.", queue}},
	{{BUSY, "hi$", FORCE}, {previous}},
	{{BUSY, FORCE}, {nop}},
	{{VANISH, FORCE}, {"Be careful on your journeys."}},

	{{"bye", MALE}, {"Don't hurt yourself with that weapon, little one.", idle}},
	{{"farewell", MALE}, {previous}},
	{{"bye", FEMALE}, {"Take care, sister.", idle}},
	{{"farewell", FEMALE}, {previous}},
	{{"job"}, {"I am the high knight of Carlin. I trained the the greatest heroines and even some males."}},
	{{"name"}, {"I am Trisha Ironfist."}},
	{{"time"}, {"It is time for a fight!"}},
	{{"hero"}, {"Heroes are knights and knights are heroes, of course."}},
	{{"knight"}, {"Knights are the true heroes of Tibia. Fame can only be earned by hand to hand combat. Brave women can join us, and we even accept suitable males now and then."}},
	{{"vocation"}, {"Your vocation is your profession. There are four vocations in Tibia: Knights, paladins, sorcerers, and druids."}},
	{{"spellbook"}, {"In a spellbook, all your spells are listed. There you will find the pronunciation of each spell. If you want to buy one, visit the magicians shop in the south of Carlin."}},
	{{KNIGHT, "spell"}, {"I can teach you healing spells and support spells. What kind of spell do you wish to learn? You can also tell me for which level you would like to learn a spell, if you prefer that.", setTopic(2)}},
	{{"spell"}, {"Sorry, I only sell spells to knights."}},

	{{KNIGHT, "instant", "spell"}, {"I can teach you healing spells and support spells. What kind of spell do you wish to learn?"}},
	{{KNIGHT, "healing", "spell"}, {"In this category I have 'Light Healing' and 'Antidote'."}},
	{{KNIGHT, "support", "spell"}, {"In this category I have 'Light', 'Find Person' and 'Great Light'."}},

	{{eq(TOPIC, 2), "healing", "spell"}, {"In this category I have 'Light Healing' and 'Antidote'."}},
	{{eq(TOPIC, 2), "support", "spell"}, {"In this category I have 'Light', 'Find Person' and 'Great Light'."}},
	{{eq(TOPIC, 2), "bye"}, {"Be careful on your journeys."}},

	{{eq(TOPIC, 2), "8$"}, {"For level 8 I have 'Find Person' and 'Light'.", setTopic(2)}},
	{{eq(TOPIC, 2), "9$"}, {"For level 9 I have 'Light Healing'.", setTopic(2)}},
	{{eq(TOPIC, 2), "10$"}, {"For level 10 I have 'Antidote'.", setTopic(2)}},
	{{eq(TOPIC, 2), "13$"}, {"For level 13 I have 'Great Light'.", setTopic(2)}},
	{{eq(TOPIC, 2)}, {"Sorry, I have only spells for level 8, 9, 10 and 13.", setTopic(2)}},

	{{KNIGHT, "find", "person"}, {setType(20), setPrice(80), "Do you want to buy the spell 'Find Person' for %P gold?", setTopic(3)}},
	{{KNIGHT, "light"}, {setType(10), setPrice(100), "Do you want to buy the spell 'Light' for %P gold?", setTopic(3)}},
	{{KNIGHT, "light", "healing"}, {setType(1), setPrice(170), "Do you want to buy the spell 'Light Healing' for %P gold?", setTopic(3)}},
	{{KNIGHT, "antidote"}, {setType(29), setPrice(150), "Do you want to buy the spell 'Antidote' for %P gold?", setTopic(3)}},
	{{KNIGHT, "great", "light"}, {setType(11), setPrice(500), "Do you want to buy the spell 'Great Light' for %P gold?", setTopic(3)}},

	{{eq(TOPIC, 3), "yes", eq(spellKnown(TYPE), 1)}, {"You already know how to cast this spell."}},
	{{eq(TOPIC, 3), "yes", lt(LEVEL, spellLevel(TYPE))}, {setAmount(spellLevel(TYPE)), "You have to be level %A to learn this spell."}},
	{{eq(TOPIC, 3), "yes", lt(COUNTMONEY, PRICE)}, {"Return when you have enough gold."}},
	{{eq(TOPIC, 3), "yes"}, {"Here you are. Look in your spellbook for the pronunciation of this spell.", deleteMoney, effectOpp(13), teachSpell(TYPE)}},
	{{eq(TOPIC, 3)}, {"Maybe next time."}},
}

function onCreatureSay(npc, creature, text, situation)
	behaviour.react(npc, rules, text, situation)
end

function onCreatureDisappear(npc, creature)
	behaviour.react(npc, rules, "", NPC_SITUATION_VANISH)
end
