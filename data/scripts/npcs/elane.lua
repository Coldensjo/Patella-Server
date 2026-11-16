-- npcs/elane.lua: the behaviour of the NPC Elane, converted from
-- elane.npc by patella-convert. The NPC is its [[npc]] entry in npcs.toml;
-- lib/npc.lua matches and runs the rules below, in the order of the file
-- (scripting.hh, "NPCs"). The comments are those of elane.npc.
--
-- GIMUD - Graphical Interface Multi User Dungeon
-- elane.npc: Datenbank für die Paladinin Elane

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
	{{ADDRESS, PALADIN, "hello$", FORCE}, {"Hi, %N! What can I do for you?"}},
	{{ADDRESS, PALADIN, "hi$", FORCE}, {previous}},
	{{ADDRESS, "hello$", FORCE}, {"Welcome to the paladins, %N! Can I help you?"}},
	{{ADDRESS, "hi$", FORCE}, {previous}},
	{{ADDRESS, FORCE}, {idle}},
	{{BUSY, "hello$", FORCE}, {"Please wait a minute %N.", queue}},
	{{BUSY, "hi$", FORCE}, {previous}},
	{{BUSY, FORCE}, {nop}},
	{{VANISH, FORCE}, {"Bye."}},

	{{"bye"}, {"Bye.", idle}},
	{{"farewell"}, {previous}},
	{{"job"}, {"I am the leader of the Paladins. I help our members."}},
	{{"name"}, {"My name is Elane. I am the famous leader of the Paladins."}},
	{{"time"}, {"Oops. I have forgotten my watch."}},
	{{"king"}, {"King Tibianus is a wise ruler."}},
	{{"tibianus"}, {previous}},
	{{"quentin"}, {"A humble monk and a wise man."}},
	{{"lynda"}, {"Hm, a litte too nice for my taste."}},
	{{"harkath"}, {"A fine warrior and a skilled general."}},
	{{"army"}, {"Some paladins serve in the kings army."}},
	{{"ferumbras"}, {"Someday I will slay that bastard!"}},
	{{"general"}, {"Harkath Bloodblade is the royal general."}},
	{{"sam"}, {"Strong man. But a little shy."}},
	{{"gorn"}, {"He sells a lot of useful equipment."}},
	{{"frodo"}, {"The alcohol he sells shrouds the mind and the eye."}},
	{{"galuna"}, {"One of the most important members of our guild. She makes all the bows and arrows we need."}},
	{{"bozo"}, {"How spineless do you have to be to become a jester?"}},
	{{"baxter"}, {"He has some potential."}},
	{{"oswald"}, {"If there wouldn't be higher powers to protect him..."}},
	{{"sherry"}, {"The McRonalds are simple farmers."}},
	{{"donald"}, {previous}},
	{{"mcronald"}, {previous}},
	{{"elane"}, {"Yes?"}},
	{{"muriel"}, {"Just another arrogant sorcerer."}},
	{{"gregor"}, {"He and his guildfellows lack the grace of a true warrior."}},
	{{"marvik"}, {"A skilled healer, that's for sure."}},
	{{"lugri"}, {"A follower of evil that will get what he deserves one day."}},
	{{"excalibug"}, {"A weapon of myth. I don't believe that this weapon exists."}},
	{{"news"}, {"I am a paladin, not a storyteller."}},

	{{"member"}, {"Every paladin profits from his vocation. It has many advantages to be a paladin."}},
	{{"profit"}, {"We will help you to improve your skills. Besides I offer spells for paladins."}},
	{{"advantage"}, {"We will help you to improve your skills. Besides I offer spells for paladins."}},
	{{"vocation"}, {"Your vocation is your profession. There are four vocations in Tibia: Paladins, knights, sorcerers, and druids."}},
	{{"paladin"}, {"Paladins are great warriors and magicians. Besides that we are excellent missile fighters. Many people in Tibia want to join us."}},
	{{"skill"}, {"Paladins are great warriors and magicians. Besides that we are excellent missile fighters. Many people in Tibia want to join us."}},
	{{"warrior"}, {"Of course, we aren't as strong as knights, but no druid or sorcerer will ever defeat a paladin with a sword."}},
	{{"magician"}, {"There are many magic spells and runes paladins can use."}},
	{{"missile"}, {"Paladins are the best missile fighters in Tibia!"}},
	{{"spellbook"}, {"In a spellbook your spells are listed. There you will find the pronunciation of each spell. If you want to buy one, visit Xodet in his magic shop."}},
	{{"spell", PALADIN}, {"I can teach you rune spells and instant spells. What kind of spell do you wish to learn? You can also tell me for which level you would like to learn a spell, if you prefer that.", setTopic(2)}},
	{{"spell"}, {"Sorry, I only sell spells to paladins."}},

	{{eq(TOPIC, 2), "rune", "spell"}, {"I sell 'Light Magic Missile', 'Heavy Magic Missile', 'Fireball' and 'Destroy Field'."}},
	{{eq(TOPIC, 2), "instant", "spell"}, {"I sell healing spells, supply spells and support spells. Which of these interests you most?"}},
	{{eq(TOPIC, 2), "level"}, {"For which level would you like to learn a spell?", setTopic(2)}},
	{{eq(TOPIC, 2), "bye"}, {"Bye.", idle}},

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
