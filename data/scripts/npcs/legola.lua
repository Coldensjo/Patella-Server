-- npcs/legola.lua: the behaviour of the NPC Legola, converted from
-- legola.npc by patella-convert. The NPC is its [[npc]] entry in npcs.toml;
-- lib/npc.lua matches and runs the rules below, in the order of the file
-- (scripting.hh, "NPCs"). The comments are those of legola.npc.
--
-- GIMUD - Graphical Interface Multi User Dungeon
-- legola.npc: Datenbank für die Paladinin Legola

local behaviour = require("lib.npc")
local ADDRESS = behaviour.ADDRESS
local BUSY = behaviour.BUSY
local VANISH = behaviour.VANISH
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
	{{ADDRESS, PALADIN, "hello$", FORCE}, {"Hello, %N! Nice to see you."}},
	{{ADDRESS, PALADIN, "hi$", FORCE}, {previous}},
	{{ADDRESS, "hello$", FORCE}, {"Welcome to the Paladins, %N! What is your business here?"}},
	{{ADDRESS, "hi$", FORCE}, {previous}},
	{{ADDRESS, FORCE}, {idle}},
	{{BUSY, "hello$", FORCE}, {"Please wait a minute, %N.", queue}},
	{{BUSY, "hi$", FORCE}, {previous}},
	{{BUSY, FORCE}, {nop}},
	{{VANISH, FORCE}, {"Bye."}},

	{{"bye"}, {"Good bye.", idle}},
	{{"farewell"}, {previous}},
	{{"job"}, {"I am the local leader of the paladins' guild. I am trainer and teacher to our members."}},
	{{"name"}, {"My name is Legola. I am the head of the local paladins' guild."}},
	{{"time"}, {"It is %T."}},
	{{"member"}, {"Paladins profit from their chosen vocation. It has many advantages to be a paladin."}},
	{{"profit"}, {"The guild will help paladins to improve their skills. Besides we offer spells for our members."}},
	{{"advantage"}, {previous}},
	{{"vocation"}, {"Your vocation is your profession. There are four vocations in Tibia: Paladins, knights, sorcerers, and druids."}},
	{{"paladin"}, {"Paladins are great warriors and able magicians. Besides we are deadly missile fighters. Many people in Tibia want to join us."}},
	{{"skill"}, {previous}},
	{{"warrior"}, {"Of course, we aren't as strong as knights, but no druid or sorcerer will ever defeat a paladin with a sword."}},
	{{"magician"}, {"Paladins learn to use most runes and can cast some usefull spells."}},
	{{"missile"}, {"Paladins are missile fighters, unequaled in Tibia!"}},
	{{"woman"}, {"All guild leaders in Carlin are chosen for their wisdom, and so all are women."}},
	{{"spellbook"}, {"In a spellbook, your spells are listed. There you will find the pronunciation of each spell. If you want to buy one, visit the magicians' shop in the south of Carlin."}},
	{{"ghostlands"}, {"Many tried to break that curse, but the evil there is so deep and overwhelmig there seems to be no hope."}},

	{{"spell", PALADIN}, {"I can teach you rune spells and instant spells. What kind of spell do you wish to learn? You can also tell me for which level you would like to learn a spell, if you prefer that.", setTopic(2)}},
	{{"spell"}, {"Sorry, I only sell spells to paladins."}},

	{{eq(TOPIC, 2), "rune", "spell"}, {"I sell 'Light Magic Missile', 'Heavy Magic Missile', 'Fireball' and 'Destroy Field'."}},
	{{eq(TOPIC, 2), "instant", "spell"}, {"I sell healing spells, supply spells and support spells. Which of these interests you most?"}},
	{{eq(TOPIC, 2), "level"}, {"For which level would you like to learn a spell?", setTopic(2)}},
	{{eq(TOPIC, 2), "bye"}, {"Good bye.", idle}},

	{{eq(TOPIC, 2), "8$"}, {"For level 8 I have 'Find Person' and 'Light'.", setTopic(2)}},
	{{eq(TOPIC, 2), "9$"}, {"For level 9 I have 'Light Healing'.", setTopic(2)}},
	{{eq(TOPIC, 2), "10$"}, {"For level 10 I have 'Antidote'.", setTopic(2)}},
	{{eq(TOPIC, 2), "11$"}, {"For level 11 I have 'Intense Healing'.", setTopic(2)}},
	{{eq(TOPIC, 2), "13$"}, {"For level 13 I have 'Great Light' and 'Conjure Arrow'.", setTopic(2)}},
	{{eq(TOPIC, 2), "14$"}, {"For level 14 I have 'Food' and 'Magic Shield'.", setTopic(2)}},
	{{eq(TOPIC, 2), "15$"}, {"For level 15 I have 'Light Magic Missile'.", setTopic(2)}},
	{{eq(TOPIC, 2), "16$"}, {"For level 16 I have 'Poisoned Arrow'.", setTopic(2)}},
	{{eq(TOPIC, 2), "17$"}, {"For level 17 I have 'Fireball' and 'Destroy Field'.", setTopic(2)}},
	{{eq(TOPIC, 2), "20$"}, {"For level 20 I have 'Ultimate Healing'.", setTopic(2)}},
	{{eq(TOPIC, 2), "25$"}, {"For level 25 I have 'Explosive Arrow' and 'Heavy Magic Missile'.", setTopic(2)}},
	{{eq(TOPIC, 2), "35$"}, {"For level 35 I have 'Invisible'.", setTopic(2)}},

	{{eq(TOPIC, 2)}, {"Sorry, I have only spells for level 8 to 11 and 13 to 17 as well as for level 20, 25 and 35.", setTopic(2)}},

	{{PALADIN, "rune", "spell"}, {"I sell 'Light Magic Missile', 'Heavy Magic Missile', 'Fireball' and 'Destroy Field'."}},
	{{PALADIN, "instant", "spell"}, {"I sell healing spells, supply spells and support spells. Which of these interests you most?"}},
	{{PALADIN, "level"}, {"For which level would you like to learn a spell?", setTopic(2)}},

	{{PALADIN, "healing", "spell"}, {"In this category I have 'Light Healing', 'Intense Healing', 'Ultimate Healing' and 'Antidote'."}},
	{{PALADIN, "supply", "spell"}, {"In this category I have 'Food', 'Conjure Arrow', 'Poisoned Arrow' and 'Explosive Arrow'."}},
	{{PALADIN, "support", "spell"}, {"In this category I have 'Find Person', 'Light', 'Great Light', 'Magic Shield' and 'Invisible'."}},

	{{PALADIN, "find", "person"}, {setType(20), setPrice(80), "Do you want to buy the spell 'Find Person' for %P gold?", setTopic(3)}},
	{{PALADIN, "light"}, {setType(10), setPrice(100), "Do you want to buy the spell 'Light' for %P gold?", setTopic(3)}},
	{{PALADIN, "food"}, {setType(42), setPrice(300), "Do you want to buy the spell 'Food' for %P gold?", setTopic(3)}},
	{{PALADIN, "light", "healing"}, {setType(1), setPrice(170), "Do you want to buy the spell 'Light Healing' for %P gold?", setTopic(3)}},
	{{PALADIN, "light", "missile"}, {setType(7), setPrice(500), "Do you want to buy the spell 'Light Magic Missile' for %P gold?", setTopic(3)}},
	{{PALADIN, "antidote"}, {setType(29), setPrice(150), "Do you want to buy the spell 'Antidote' for %P gold?", setTopic(3)}},
	{{PALADIN, "intense", "healing"}, {setType(2), setPrice(350), "Do you want to buy the spell 'Intense Healing' for %P gold?", setTopic(3)}},
	{{PALADIN, "conjure", "arrow"}, {setType(51), setPrice(450), "Do you want to buy the spell 'Conjure Arrow' for %P gold?", setTopic(3)}},
	{{PALADIN, "great", "light"}, {setType(11), setPrice(500), "Do you want to buy the spell 'Great Light' for %P gold?", setTopic(3)}},
	{{PALADIN, "heavy", "missile"}, {setType(8), setPrice(1500), "Do you want to buy the spell 'Heavy Magic Missile' for %P gold?", setTopic(3)}},
	{{PALADIN, "magic", "shield"}, {setType(44), setPrice(450), "Do you want to buy the spell 'Magic Shield' for %P gold?", setTopic(3)}},
	{{PALADIN, "fireball"}, {setType(15), setPrice(800), "Do you want to buy the spell 'Fireball' for %P gold?", setTopic(3)}},
	{{PALADIN, "poison", "arrow"}, {setType(48), setPrice(700), "Do you want to buy the spell 'Poisoned Arrow' for %P gold?", setTopic(3)}},
	{{PALADIN, "destroy", "field"}, {setType(30), setPrice(700), "Do you want to buy the spell 'Destroy Field' for %P gold?", setTopic(3)}},
	{{PALADIN, "ultimate", "healing"}, {setType(3), setPrice(1000), "Do you want to buy the spell 'Ultimate Healing' for %P gold?", setTopic(3)}},
	{{PALADIN, "explosive", "arrow"}, {setType(49), setPrice(1000), "Do you want to buy the spell 'Explosive Arrow' for %P gold?", setTopic(3)}},
	{{PALADIN, "invisible"}, {setType(45), setPrice(2000), "Do you want to buy the spell 'Invisible' for %P gold?", setTopic(3)}},

	{{eq(TOPIC, 3), "yes", eq(spellKnown(TYPE), 1)}, {"You already know how to cast this spell."}},
	{{eq(TOPIC, 3), "yes", lt(LEVEL, spellLevel(TYPE))}, {setAmount(spellLevel(TYPE)), "You must be level %A to learn this spell."}},
	{{eq(TOPIC, 3), "yes", lt(COUNTMONEY, PRICE)}, {"Oh. You do not have enough money."}},
	{{eq(TOPIC, 3), "yes"}, {"Here you are. Look in your spellbook for the pronunciation of this spell.", deleteMoney, effectOpp(13), teachSpell(TYPE)}},
	{{eq(TOPIC, 3)}, {"Ok. Then not."}},
}

function onCreatureSay(npc, creature, text, situation)
	behaviour.react(npc, rules, text, situation)
end

function onCreatureDisappear(npc, creature)
	behaviour.react(npc, rules, "", NPC_SITUATION_VANISH)
end
