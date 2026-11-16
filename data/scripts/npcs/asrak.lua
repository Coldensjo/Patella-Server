-- npcs/asrak.lua: the behaviour of the NPC Asrak, converted from
-- asrak.npc by patella-convert. The NPC is its [[npc]] entry in npcs.toml;
-- lib/npc.lua matches and runs the rules below, in the order of the file
-- (scripting.hh, "NPCs"). The comments are those of asrak.npc.
--
-- GIMUD - Graphical Interface Multi User Dungeon
-- asrak.npc: Datenbank für den Minotaurenklingenmeister Asrak

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
	{{ADDRESS, "hello$", FORCE}, {"I welcome you, %N!"}},
	{{ADDRESS, "hi$", FORCE}, {previous}},
	{{ADDRESS, FORCE}, {idle}},
	{{BUSY, "hello$", FORCE}, {"Please wait in patience, young %N.", queue}},
	{{BUSY, "hi$", FORCE}, {previous}},
	{{BUSY, FORCE}, {nop}},
	{{VANISH, FORCE}, {"May your path be as straight as an arrow."}},

	{{"bye"}, {"May your path be as straight as an arrow.", idle}},
	{{"farewell"}, {previous}},
	{{"job"}, {"I am the overseer of the pits and the trainer of the gladiators."}},
	{{"shop"}, {previous}},
	{{"name"}, {"I am known as Asrak the Ironhoof."}},
	{{"time"}, {"It is %T."}},
	{{"king"}, {"I pledge no allegiance to any king, be it human or minotaurean."}},
	{{"tibianus"}, {previous}},
	{{"venore"}, {"The city pays me well and those undisciplined gladiators need my skills and guidance badly."}},
	{{"gladiator"}, {"Those wannabe fighters are weak and most of them are unable to comprehend a higher concept like the Mooh'Tah."}},
	{{"trainer"}, {previous}},
	{{"minotaur"}, {"In the ancient wars we lost much because of the rage. The one good thing is we lost our trust in the gods, too."}},
	{{"gods"}, {"By them we were imbued with the rage that almost costed our existence. By them we were used as pawns in wars that were not ours."}},
	{{"mintwallin"}, {"The city is only a shadow of what we could have accomplished without that curse of rage that the gods bestowed upon us."}},
	{{"rage"}, {"Rage is the legacy of Blog, the beast. To overcome it is our primal goal. The Mooh'Tah is our only hope of salvation and perfection."}},
	{{"guidance"}, {"Like all true minotaurean blademasters I am a warrior-philosopher of the Mooh'Tah."}},
	{{"mooh'tah"}, {"The Mooh'Tah teaches us control. It provides you with weapon, armor, and shield. It teaches you harmony and focus."}},
	{{"harmony"}, {"There is an elegant harmony in every thing done right. If you feel the harmony of an action you can sing its song."}},
	{{"sing"}, {"Each harmonic action has it own song. If you can sing it, you are in harmony with that action. This is where the minotaurean battlesongs come from."}},
	{{"song"}, {previous}},
	{{"battlesongs"}, {"Each Mooh'Tah master focuses his skills on the harmony of battle. He is one with the song that he's singing with his voice or at least his heart."}},
	{{"mooh'tah", "master"}, {"Mooh'Tah masters are the epitome of the minotaurean warrior-philosophers. Full in control, free of rage, focused in perfect harmony with their actions."}},
	{{"warrior-philosopher"}, {previous}},
	{{"general"}, {"Your human generals are like their warriors. They lack the focus to be a true warrior."}},
	{{"army"}, {"Your human army might be big, but without skills. They are only sheep to be slaughtered."}},
	{{"ferumbras"}, {"To rely on magic is like to cheat fate. All cheaters will find their just punishment one day, and so will he."}},
	{{"excalibug"}, {"If it's truly a weapon to slay gods it might be worth to be sought for."}},
	{{"news"}, {"Focus on your own life, not on that of others."}},
	{{"help"}, {"I teach worthy warriors the way of the knight."}},
	{{"monster"}, {"Inferior creatures of rage, driven by their primitive urges. Only worthy to be noticed to test one's skills."}},
	{{"dungeon"}, {"The dungeons of your desires and fears are the only ones you really should fear and those you really have to conquer."}},
	{{"thanks"}, {"I hope you learned something."}},
	{{"thank", "you"}, {previous}},

	{{"offer"}, {"I offer you the teachings of knighthood and the way of the paladin."}},
	{{"training"}, {previous}},
	{{"do", "you", "sell"}, {"I am not a merchant, but a warrior."}},
	{{"do", "you", "have"}, {previous}},
	{{"weapon"}, {"Make your will your weapon, and your enemies will perish."}},
	{{"armor"}, {"Courage is the only armor that shields you against rage and fear, the greatest dangers you will have to face."}},
	{{"shield"}, {"Your confidence shall be your shield. Nothing can penetrate that defence. No emotion will let you lose your focus."}},

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
	{{eq(TOPIC, 2), "bye"}, {"May your path be as straight as an arrow.", idle}},

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
	{{eq(TOPIC, 3), "bye"}, {"May your path be as straight as an arrow.", idle}},

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
