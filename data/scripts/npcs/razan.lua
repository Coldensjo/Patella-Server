-- npcs/razan.lua: the behaviour of the NPC Razan, converted from
-- razan.npc by patella-convert. The NPC is its [[npc]] entry in npcs.toml;
-- lib/npc.lua matches and runs the rules below, in the order of the file
-- (scripting.hh, "NPCs"). The comments are those of razan.npc.
--
-- GIMUD - Graphical Interface Multi User Dungeon
-- razan.npc: Datenbank fuer den Waffenmeister Razan

local behaviour = require("lib.npc")
local ADDRESS = behaviour.ADDRESS
local BUSY = behaviour.BUSY
local VANISH = behaviour.VANISH
local KNIGHT = behaviour.KNIGHT
local PALADIN = behaviour.PALADIN
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
	{{ADDRESS, "hello$", FORCE}, {"Greetings %N. What leads you to me?"}},
	{{ADDRESS, "hi$", FORCE}, {previous}},
	{{ADDRESS, FORCE}, {idle}},
	{{BUSY, "hello$", FORCE}, {"Be patient, %N.", queue}},
	{{BUSY, "hi$", FORCE}, {previous}},
	{{BUSY, FORCE}, {nop}},
	{{VANISH, FORCE}, {"Daraman's blessings."}},

	{{"bye"}, {"Daraman's blessings.", idle}},
	{{"job"}, {"I am the weaponmaster of Caliph Kazzan the great."}},
	{{"name"}, {"Razan ... Razan Ibn Rublai."}},
	{{"time"}, {"You are talking of what you are wasting right now?"}},
	{{"caliph"}, {"We owe caliph Kazzan our loyality and gratitude, thrice praised be his name."}},
	{{"kazzan"}, {previous}},
	{{"shalmar"}, {"He is competent. That's fine enough for a mage."}},
	{{"djinn"}, {"Some people in Darashia rely to much on the services of these creatures. I wonder if they keep the path in mind."}},
	{{"path"}, {"The path of enlightenment, leading to ascension as thaught to us by Daraman."}},
	{{"Daraman"}, {"Better talk to Kasmir about that."}},
	{{"enlightenment"}, {previous}},
	{{"ascension"}, {previous}},
	{{"ferumbras"}, {"Maybe a worthy oponent, but probably only another of these spellcasting cowards."}},
	{{"army"}, {"This information is confidential."}},
	{{"guards"}, {previous}},
	{{"kasmir"}, {"You will find him in the Muhayin, the sacred tower of meditation."}},
	{{"excalibug"}, {"The skill should make a fighter strong, not the weapon."}},
	{{"news"}, {"I don't care for rumours but for facts."}},
	{{"weaponmaster"}, {"I mastered the arts of close combat and distance fight alike. I teach both, paladins and knights in their ways."}},
	{{"tibia"}, {"The world is a dangerous place for body and for soul."}},
	{{"knight"}, {"The way of the warrior is not that different from the way to ascension."}},
	{{"paladin"}, {previous}},
	{{"spellbook"}, {"In a spellbook, your spells are listed. There you will find the pronunciation of each spell. Rely more on your skills, though."}},
	{{KNIGHT, "spell"}, {"I can teach you healing spells and support spells. What kind of spell do you wish to learn? You can also tell me for which level you would like to learn a spell, if you prefer that.", setTopic(2)}},
	{{PALADIN, "spell"}, {"I can teach you rune spells and instant spells. What kind of spell do you wish to learn? You can also tell me for which level you would like to learn a spell, if you prefer that.", setTopic(3)}},
	{{"spell"}, {"Sorry, I only teach spells to knights and paladins."}},

	{{KNIGHT, "instant", "spell"}, {"I can teach you healing spells and support spells. What kind of spell do you wish to learn?"}},
	{{KNIGHT, "healing", "spell"}, {"In this category I have 'Light Healing' and 'Antidote'."}},
	{{KNIGHT, "support", "spell"}, {"In this category I have 'Light', 'Find Person' and 'Great Light'."}},
	{{KNIGHT, "level"}, {"For which level would you like to learn a spell?", setTopic(2)}},
	{{PALADIN, "rune", "spell"}, {"I sell 'Light Magic Missile', 'Heavy Magic Missile', 'Fireball' and 'Destroy Field'."}},
	{{PALADIN, "instant", "spell"}, {"I sell healing spells, supply spells and support spells. Which of these interests you most?"}},
	{{PALADIN, "level"}, {"For which level would you like to learn a spell?", setTopic(3)}},

	{{eq(TOPIC, 2), "healing", "spell"}, {"In this category I have 'Light Healing' and 'Antidote'."}},
	{{eq(TOPIC, 2), "support", "spell"}, {"In this category I have 'Light', 'Find Person' and 'Great Light'."}},
	{{eq(TOPIC, 3), "bye"}, {"Daraman's blessings.", idle}},

	{{eq(TOPIC, 2), "8$"}, {"For level 8 I have 'Find Person' and 'Light'.", setTopic(2)}},
	{{eq(TOPIC, 2), "9$"}, {"For level 9 I have 'Light Healing'.", setTopic(2)}},
	{{eq(TOPIC, 2), "10$"}, {"For level 10 I have 'Antidote'.", setTopic(2)}},
	{{eq(TOPIC, 2), "13$"}, {"For level 13 I have 'Great Light'.", setTopic(2)}},
	{{eq(TOPIC, 2)}, {"Sorry, I have only spells for level 8, 9, 10 and 13.", setTopic(2)}},

	{{KNIGHT, "find", "person"}, {setType(20), setPrice(80), "Do you want to buy the spell 'Find Person' for %P gold?", setTopic(4)}},
	{{KNIGHT, "light"}, {setType(10), setPrice(100), "Do you want to buy the spell 'Light' for %P gold?", setTopic(4)}},
	{{KNIGHT, "light", "healing"}, {setType(1), setPrice(170), "Do you want to buy the spell 'Light Healing' for %P gold?", setTopic(4)}},
	{{KNIGHT, "antidote"}, {setType(29), setPrice(150), "Do you want to buy the spell 'Antidote' for %P gold?", setTopic(4)}},
	{{KNIGHT, "great", "light"}, {setType(11), setPrice(500), "Do you want to buy the spell 'Great Light' for %P gold?", setTopic(4)}},

	{{eq(TOPIC, 3), "rune", "spell"}, {"I sell 'Light Magic Missile', 'Heavy Magic Missile', 'Fireball' and 'Destroy Field'."}},
	{{eq(TOPIC, 3), "instant", "spell"}, {"I sell healing spells, supply spells and support spells. Which of these interests you most?"}},
	{{eq(TOPIC, 3), "level"}, {"For which level would you like to learn a spell?", setTopic(3)}},
	{{eq(TOPIC, 3), "bye"}, {"Daraman's blessings.", idle}},

	{{eq(TOPIC, 3), "8$"}, {"For level 8 I have 'Find Person' and 'Light'.", setTopic(3)}},
	{{eq(TOPIC, 3), "9$"}, {"For level 9 I have 'Light Healing'.", setTopic(3)}},
	{{eq(TOPIC, 3), "10$"}, {"For level 10 I have 'Antidote'.", setTopic(3)}},
	{{eq(TOPIC, 3), "11$"}, {"For level 11 I have 'Intense Healing'.", setTopic(3)}},
	{{eq(TOPIC, 3), "13$"}, {"For level 13 I have 'Great Light' and 'Conjure Arrow'.", setTopic(3)}},
	{{eq(TOPIC, 3), "14$"}, {"For level 14 I have 'Food' and 'Magic Shield'.", setTopic(3)}},
	{{eq(TOPIC, 3), "15$"}, {"For level 15 I have 'Light Magic Missile'.", setTopic(3)}},
	{{eq(TOPIC, 3), "16$"}, {"For level 16 I have 'Poisoned Arrow'.", setTopic(3)}},
	{{eq(TOPIC, 3), "17$"}, {"For level 17 I have 'Fireball' and 'Destroy Field'.", setTopic(3)}},
	{{eq(TOPIC, 3), "20$"}, {"For level 20 I have 'Ultimate Healing'.", setTopic(3)}},
	{{eq(TOPIC, 3), "25$"}, {"For level 25 I have 'Explosive Arrow' and 'Heavy Magic Missile'.", setTopic(3)}},
	{{eq(TOPIC, 3), "35$"}, {"For level 35 I have 'Invisible'.", setTopic(3)}},
	{{eq(TOPIC, 3)}, {"Sorry, I have only spells for level 8 to 11 and 13 to 17 as well as for level 20, 25 and 35.", setTopic(3)}},

	{{PALADIN, "healing", "spell"}, {"In this category I have 'Light Healing', 'Intense Healing', 'Ultimate Healing' and 'Antidote'."}},
	{{PALADIN, "supply", "spell"}, {"In this category I have 'Food', 'Conjure Arrow', 'Poisoned Arrow' and 'Explosive Arrow'."}},
	{{PALADIN, "support", "spell"}, {"In this category I have 'Find Person', 'Light', 'Great Light', 'Magic Shield' and 'Invisible'."}},

	{{PALADIN, "find", "person"}, {setType(20), setPrice(80), "Do you want to buy the spell 'Find Person' for %P gold?", setTopic(4)}},
	{{PALADIN, "light"}, {setType(10), setPrice(100), "Do you want to buy the spell 'Light' for %P gold?", setTopic(4)}},
	{{PALADIN, "food"}, {setType(42), setPrice(300), "Do you want to buy the spell 'Food' for %P gold?", setTopic(4)}},
	{{PALADIN, "light", "healing"}, {setType(1), setPrice(170), "Do you want to buy the spell 'Light Healing' for %P gold?", setTopic(4)}},
	{{PALADIN, "light", "missile"}, {setType(7), setPrice(500), "Do you want to buy the spell 'Light Magic Missile' for %P gold?", setTopic(4)}},
	{{PALADIN, "antidote"}, {setType(29), setPrice(150), "Do you want to buy the spell 'Antidote' for %P gold?", setTopic(4)}},
	{{PALADIN, "intense", "healing"}, {setType(2), setPrice(350), "Do you want to buy the spell 'Intense Healing' for %P gold?", setTopic(4)}},
	{{PALADIN, "conjure", "arrow"}, {setType(51), setPrice(450), "Do you want to buy the spell 'Conjure Arrow' for %P gold?", setTopic(4)}},
	{{PALADIN, "great", "light"}, {setType(11), setPrice(500), "Do you want to buy the spell 'Great Light' for %P gold?", setTopic(4)}},
	{{PALADIN, "heavy", "missile"}, {setType(8), setPrice(1500), "Do you want to buy the spell 'Heavy Magic Missile' for %P gold?", setTopic(4)}},
	{{PALADIN, "magic", "shield"}, {setType(44), setPrice(450), "Do you want to buy the spell 'Magic Shield' for %P gold?", setTopic(4)}},
	{{PALADIN, "fireball"}, {setType(15), setPrice(800), "Do you want to buy the spell 'Fireball' for %P gold?", setTopic(4)}},
	{{PALADIN, "poison", "arrow"}, {setType(48), setPrice(700), "Do you want to buy the spell 'Poisoned Arrow' for %P gold?", setTopic(4)}},
	{{PALADIN, "destroy", "field"}, {setType(30), setPrice(700), "Do you want to buy the spell 'Destroy Field' for %P gold?", setTopic(4)}},
	{{PALADIN, "ultimate", "healing"}, {setType(3), setPrice(1000), "Do you want to buy the spell 'Ultimate Healing' for %P gold?", setTopic(4)}},
	{{PALADIN, "explosive", "arrow"}, {setType(49), setPrice(1000), "Do you want to buy the spell 'Explosive Arrow' for %P gold?", setTopic(4)}},
	{{PALADIN, "invisible"}, {setType(45), setPrice(2000), "Do you want to buy the spell 'Invisible' for %P gold?", setTopic(4)}},

	{{eq(TOPIC, 4), "yes", eq(spellKnown(TYPE), 1)}, {"You already know how to cast this spell."}},
	{{eq(TOPIC, 4), "yes", lt(LEVEL, spellLevel(TYPE))}, {setAmount(spellLevel(TYPE)), "You have to be level %A to learn this spell."}},
	{{eq(TOPIC, 4), "yes", lt(COUNTMONEY, PRICE)}, {"Return when you have enough gold."}},
	{{eq(TOPIC, 4), "yes"}, {"Here you are. Look in your spellbook for the pronunciation of this spell.", deleteMoney, effectOpp(13), teachSpell(TYPE)}},
	{{eq(TOPIC, 4)}, {"Maybe next time."}},
}

function onCreatureSay(npc, creature, text, situation)
	behaviour.react(npc, rules, text, situation)
end

function onCreatureDisappear(npc, creature)
	behaviour.react(npc, rules, "", NPC_SITUATION_VANISH)
end
