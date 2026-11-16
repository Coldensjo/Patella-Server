-- npcs/lea.lua: the behaviour of the NPC Lea, converted from
-- lea.npc by patella-convert. The NPC is its [[npc]] entry in npcs.toml;
-- lib/npc.lua matches and runs the rules below, in the order of the file
-- (scripting.hh, "NPCs"). The comments are those of lea.npc.
--
-- GIMUD - Graphical Interface Multi User Dungeon
-- lea.npc: Datenbank für die Magierin Lea

local behaviour = require("lib.npc")
local ADDRESS = behaviour.ADDRESS
local BUSY = behaviour.BUSY
local VANISH = behaviour.VANISH
local SORCERER = behaviour.SORCERER
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
local questValue = behaviour.questValue
local setTopic = behaviour.setTopic
local setPrice = behaviour.setPrice
local setAmount = behaviour.setAmount
local setType = behaviour.setType
local setQuestValue = behaviour.setQuestValue
local effectOpp = behaviour.effectOpp
local teachSpell = behaviour.teachSpell
local create = behaviour.create
local deleteMoney = behaviour.deleteMoney
local queue = behaviour.queue
local idle = behaviour.idle
local nop = behaviour.nop
local previous = behaviour.previous

local rules = behaviour.rules {
	{{ADDRESS, SORCERER, "hello$", FORCE}, {"Welcome back, %N!"}},
	{{ADDRESS, SORCERER, "hi$", FORCE}, {"Welcome back, %N!"}},
	{{ADDRESS, "hello$", FORCE}, {"Greetings, %N."}},
	{{ADDRESS, "hi$", FORCE}, {previous}},
	{{ADDRESS, FORCE}, {idle}},
	{{BUSY, "hello$", FORCE}, {"Be patient %N, please.", queue}},
	{{BUSY, "hi$", FORCE}, {previous}},
	{{BUSY, FORCE}, {nop}},
	{{VANISH, FORCE}, {"Take care on your journeys."}},

	{{"bye"}, {"Take care on your journeys.", idle}},
	{{"farewell"}, {previous}},
	{{"job"}, {"I am the archsorcerer of Carlin. I keep the secrets of our order."}},
	{{"name"}, {"My name is Lea."}},
	{{"time"}, {"Time is a force we sorcerers will master one day."}},
	{{"wisdom"}, {"You need great wisdom to cast spells of power."}},
	{{"male"}, {"Some tricks of sorcery are easy enough to be mastered even by males, but they'd better stick to cardtricks."}},
	{{"sorcerer"}, {"Any sorcerer dedicates his whole life to the study of the arcane arts."}},
	{{"power"}, {"We sorcerers wield arcane powers beyond comprehension of men."}},
	{{"arcane"}, {previous}},
	{{"vocation"}, {"Your vocation is your profession. There are four vocations in this world: Sorcerers, paladins, knights, and druids."}},
	{{"spellbook"}, {"A spellbook lists all your spells. There you can find the pronunciation of each spell. You can buy one at the magicians' shop."}},
	{{"spell", SORCERER}, {"I can teach you rune spells and instant spells. What kind of spell do you wish to learn? You can also tell me for which level you would like to learn a spell, if you prefer that.", setTopic(2)}},
	{{"spell"}, {"Sorry, I only sell spells to Sorcerers."}},

	{{SORCERER, "wand", lt(questValue(333), 1)}, {"Oh, you did not purchase your first magical wand yet? Please take this little present from me as your magic teacher!", setQuestValue(333, 1), setType(3074), setAmount(1), create(TYPE)}},

	{{eq(TOPIC, 2), "rune", "spell"}, {"I sell attack rune spells and support rune spells. Which of these interests you most?"}},
	{{eq(TOPIC, 2), "instant", "spell"}, {"I sell attack spells, healing spells, support spells and summon spells. Which of these interests you most?"}},
	{{eq(TOPIC, 2), "level"}, {"For which level would you like to learn a spell?", setTopic(2)}},
	{{eq(TOPIC, 2), "bye"}, {"Take care on your journeys.", idle}},

	{{SORCERER, "level"}, {"For which level would you like to learn a spell?", setTopic(2)}},
	{{SORCERER, "rune", "spell"}, {"I sell attack rune spells and support rune spells. Which of these interests you most?"}},
	{{SORCERER, "instant", "spell"}, {"I sell attack spells, healing spells, support spells and summon spells. Which of these interests you most?"}},

	{{SORCERER, "attack", "rune", "spell"}, {"I sell missile rune spells, explosive rune spells, field rune spells, wall rune spells and bomb rune spells."}},
	{{SORCERER, "support", "rune", "spell"}, {"In this category I have 'Destroy Field'."}},

	{{SORCERER, "missile", "rune", "spell"}, {"In this category I have 'Light Magic Missile', 'Heavy Magic Missile' and 'Sudden Death'."}},
	{{SORCERER, "explosive", "rune", "spell"}, {"In this category I have 'Fireball', 'Great Fireball' and 'Explosion'."}},
	{{SORCERER, "field", "rune", "spell"}, {"In this category I have 'Poison Field', 'Fire Field' and 'Energy Field'."}},
	{{SORCERER, "wall", "rune", "spell"}, {"In this category I have 'Fire Wall', 'Poison Wall' and 'Energy Wall'."}},
	{{SORCERER, "bomb", "rune", "spell"}, {"In this category I have 'Firebomb'."}},

	{{SORCERER, "attack", "spell"}, {"In this category I have 'Fire Wave', 'Energy Wave', 'Energy Beam' and 'Great Energy Beam'."}},
	{{SORCERER, "healing", "spell"}, {"In this category I have 'Light Healing', 'Intense Healing', 'Ultimate Healing' and 'Antidote'."}},
	{{SORCERER, "support", "spell"}, {"In this category I have 'Find Person', 'Light', 'Great Light', 'Magic Shield', 'Creature Illusion' and 'Invisible'."}},
	{{SORCERER, "summon", "spell"}, {"In this category I have 'Summon Creature'."}},

	{{SORCERER, "find", "person"}, {setType(20), setPrice(80), "Do you want to buy the spell 'Find Person' for %P gold?", setTopic(3)}},
	{{SORCERER, "light"}, {setType(10), setPrice(100), "Do you want to buy the spell 'Light' for %P gold?", setTopic(3)}},
	{{SORCERER, "light", "healing"}, {setType(1), setPrice(170), "Do you want to buy the spell 'Light Healing' for %P gold?", setTopic(3)}},
	{{SORCERER, "light", "missile"}, {setType(7), setPrice(500), "Do you want to buy the spell 'Light Magic Missile' for %P gold?", setTopic(3)}},
	{{SORCERER, "antidote"}, {setType(29), setPrice(150), "Do you want to buy the spell 'Antidote' for %P gold?", setTopic(3)}},
	{{SORCERER, "intense", "healing"}, {setType(2), setPrice(350), "Do you want to buy the spell 'Intense Healing' for %P gold?", setTopic(3)}},
	{{SORCERER, "poison", "field"}, {setType(26), setPrice(300), "Do you want to buy the spell 'Poison Field' for %P gold?", setTopic(3)}},
	{{SORCERER, "great", "light"}, {setType(11), setPrice(500), "Do you want to buy the spell 'Great Light' for %P gold?", setTopic(3)}},
	{{SORCERER, "fire", "field"}, {setType(25), setPrice(500), "Do you want to buy the spell 'Fire Field' for %P gold?", setTopic(3)}},
	{{SORCERER, "heavy", "missile"}, {setType(8), setPrice(1500), "Do you want to buy the spell 'Heavy Magic Missile' for %P gold?", setTopic(3)}},
	{{SORCERER, "magic", "shield"}, {setType(44), setPrice(450), "Do you want to buy the spell 'Magic Shield' for %P gold?", setTopic(3)}},
	{{SORCERER, "fireball"}, {setType(15), setPrice(800), "Do you want to buy the spell 'Fireball' for %P gold?", setTopic(3)}},
	{{SORCERER, "energy", "field"}, {setType(27), setPrice(700), "Do you want to buy the spell 'Energy Field' for %P gold?", setTopic(3)}},
	{{SORCERER, "destroy", "field"}, {setType(30), setPrice(700), "Do you want to buy the spell 'Destroy Field' for %P gold?", setTopic(3)}},
	{{SORCERER, "fire", "wave"}, {setType(19), setPrice(850), "Do you want to buy the spell 'Fire Wave' for %P gold?", setTopic(3)}},
	{{SORCERER, "ultimate", "healing"}, {setType(3), setPrice(1000), "Do you want to buy the spell 'Ultimate Healing' for %P gold?", setTopic(3)}},
	{{SORCERER, "great", "fireball"}, {setType(16), setPrice(1200), "Do you want to buy the spell 'Great Fireball' for %P gold?", setTopic(3)}},
	{{SORCERER, "fire", "bomb"}, {setType(17), setPrice(1500), "Do you want to buy the spell 'Fire Bomb' for %P gold?", setTopic(3)}},
	{{SORCERER, "energy", "beam"}, {setType(22), setPrice(1000), "Do you want to buy the spell 'Energy Beam' for %P gold?", setTopic(3)}},
	{{SORCERER, "creature", "illusion"}, {setType(38), setPrice(1000), "Do you want to buy the spell 'Creature Illusion' for %P gold?", setTopic(3)}},
	{{SORCERER, "poison", "wall"}, {setType(32), setPrice(1600), "Do you want to buy the spell 'Poison Wall' for %P gold?", setTopic(3)}},
	{{SORCERER, "explosion"}, {setType(18), setPrice(1800), "Do you want to buy the spell 'Explosion' for %P gold?", setTopic(3)}},
	{{SORCERER, "fire", "wall"}, {setType(28), setPrice(2000), "Do you want to buy the spell 'Fire Wall' for %P gold?", setTopic(3)}},
	{{SORCERER, "great", "energy", "beam"}, {setType(23), setPrice(1800), "Do you want to buy the spell 'Great Energy Beam' for %P gold?", setTopic(3)}},
	{{SORCERER, "invisible"}, {setType(45), setPrice(2000), "Do you want to buy the spell 'Invisible' for %P gold?", setTopic(3)}},
	{{SORCERER, "summon", "creature"}, {setType(9), setPrice(2000), "Do you want to buy the spell 'Summon Creature' for %P gold?", setTopic(3)}},
	{{SORCERER, "energy", "wall"}, {setType(33), setPrice(2500), "Do you want to buy the spell 'Energy Wall' for %P gold?", setTopic(3)}},
	{{SORCERER, "energy", "wave"}, {setType(13), setPrice(2500), "Do you want to buy the spell 'Energy Wave' for %P gold?", setTopic(3)}},
	{{SORCERER, "sudden", "death"}, {setType(21), setPrice(3000), "Do you want to buy the spell 'Sudden Death' for %P gold?", setTopic(3)}},

	{{eq(TOPIC, 2), "8$"}, {"For level 8 I have 'Find Person' and 'Light'.", setTopic(2)}},
	{{eq(TOPIC, 2), "9$"}, {"For level 9 I have 'Light Healing'.", setTopic(2)}},
	{{eq(TOPIC, 2), "10$"}, {"For level 10 I have 'Antidote'.", setTopic(2)}},
	{{eq(TOPIC, 2), "11$"}, {"For level 11 I have 'Intense Healing'.", setTopic(2)}},
	{{eq(TOPIC, 2), "13$"}, {"For level 13 I have 'Great Light'.", setTopic(2)}},
	{{eq(TOPIC, 2), "14$"}, {"For level 14 I have 'Poison Field' and 'Magic Shield'.", setTopic(2)}},
	{{eq(TOPIC, 2), "15$"}, {"For level 15 I have 'Fire Field' and 'Light Magic Missile'.", setTopic(2)}},
	{{eq(TOPIC, 2), "17$"}, {"For level 17 I have 'Fireball' and 'Destroy Field'.", setTopic(2)}},
	{{eq(TOPIC, 2), "18$"}, {"For level 18 I have 'Energy Field' and 'Fire Wave'.", setTopic(2)}},
	{{eq(TOPIC, 2), "20$"}, {"For level 20 I have 'Ultimate Healing'.", setTopic(2)}},
	{{eq(TOPIC, 2), "23$"}, {"For level 23 I have 'Great Fireball', 'Energy Beam' and 'Creature Illusion'.", setTopic(2)}},
	{{eq(TOPIC, 2), "25$"}, {"For level 25 I have 'Summon Creature' and 'Heavy Magic Missile'.", setTopic(2)}},
	{{eq(TOPIC, 2), "27$"}, {"For level 27 I have 'Firebomb'.", setTopic(2)}},
	{{eq(TOPIC, 2), "29$"}, {"For level 29 I have 'Poison Wall' and 'Great Energy Beam'.", setTopic(2)}},
	{{eq(TOPIC, 2), "31$"}, {"For level 31 I have 'Explosion'.", setTopic(2)}},
	{{eq(TOPIC, 2), "33$"}, {"For level 33 I have 'Fire Wall'.", setTopic(2)}},
	{{eq(TOPIC, 2), "35$"}, {"For level 35 I have 'Invisible'.", setTopic(2)}},
	{{eq(TOPIC, 2), "38$"}, {"For level 38 I have 'Energy Wave'.", setTopic(2)}},
	{{eq(TOPIC, 2), "41$"}, {"For level 41 I have 'Energy Wall'.", setTopic(2)}},
	{{eq(TOPIC, 2), "45$"}, {"For level 45 I have 'Sudden Death'.", setTopic(2)}},

	{{eq(TOPIC, 2)}, {"Hmm, I have no spells for this level, but for many levels from 8 to 45.", setTopic(2)}},

	{{eq(TOPIC, 3), "yes", eq(spellKnown(TYPE), 1)}, {"You already know how to cast this spell."}},
	{{eq(TOPIC, 3), "yes", lt(LEVEL, spellLevel(TYPE))}, {setAmount(spellLevel(TYPE)), "You have to be level %A to learn this spell."}},
	{{eq(TOPIC, 3), "yes", lt(COUNTMONEY, PRICE)}, {"You need more money."}},
	{{eq(TOPIC, 3), "yes"}, {"Here you are. Look in your spellbook for the pronunciation of this spell.", deleteMoney, effectOpp(13), teachSpell(TYPE)}},
	{{eq(TOPIC, 3)}, {"Then not."}},
}

function onCreatureSay(npc, creature, text, situation)
	behaviour.react(npc, rules, text, situation)
end

function onCreatureDisappear(npc, creature)
	behaviour.react(npc, rules, "", NPC_SITUATION_VANISH)
end
