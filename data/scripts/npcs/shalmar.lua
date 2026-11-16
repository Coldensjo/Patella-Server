-- npcs/shalmar.lua: the behaviour of the NPC Shalmar, converted from
-- shalmar.npc by patella-convert. The NPC is its [[npc]] entry in npcs.toml;
-- lib/npc.lua matches and runs the rules below, in the order of the file
-- (scripting.hh, "NPCs"). The comments are those of shalmar.npc.
--
-- GIMUD - Graphical Interface Multi User Dungeon
-- shalmar.npc: Datenbank fuer den Magier Shalmar

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
	{{ADDRESS, "hello$", FORCE}, {"Daraman's blessings, %N."}},
	{{ADDRESS, "hi$", FORCE}, {previous}},
	{{ADDRESS, FORCE}, {idle}},
	{{BUSY, "hello$", FORCE}, {"Wait, %N!", queue}},
	{{BUSY, "hi$", FORCE}, {previous}},
	{{BUSY, FORCE}, {nop}},
	{{VANISH, FORCE}, {"Daraman's blessings."}},

	{{"bye"}, {"Daraman's blessings.", idle}},
	{{"name"}, {"I am Shalmar Ibn Djinbar, the caliph's magician and astrologer."}},
	{{"job"}, {"I teach magic spells to the worthy."}},
	{{"time"}, {"It's %T right now."}},
	{{"caliph"}, {"The caliph has the strong soul needed to guide his people."}},
	{{"kazzan"}, {previous}},
	{{"ferumbras"}, {"His weakness is evident by the rotting of his soul."}},
	{{"excalibug"}, {"A strong mind and a pure soul has no need for such items."}},
	{{"thais"}, {"It's a city of souls who failed to see the need of ascension."}},
	{{"tibia"}, {"The world is filled with wonderous places and items."}},
	{{"carlin"}, {"I heared it's a city of druids."}},
	{{"ascension"}, {"Talk to Kasmir about that issue. It's not my place to pose as a teacher since I am a student, too."}},
	{{"news"}, {"News are distractions. Nothing of importance happens outside your own soul."}},
	{{"rumour"}, {previous}},
	{{"rumor"}, {previous}},
	{{"sorcerer"}, {"The way of the magician is not that different from the way to ascension."}},
	{{"druid"}, {previous}},

	{{"offer"}, {"I'm teaching spells to sorcerers and druids. I also used to sell magic goods, but my assistant Asima in the next room does that now for me."}},
	{{"goods"}, {previous}},
	{{"do", "you", "sell"}, {previous}},
	{{"do", "you", "have"}, {previous}},

	{{"rune"}, {"I don't sell this anymore, it sort of kept on confusing me to do that much work. Please talk to my assistant Asima in the next room to purchase magic goods."}},
	{{"life", "fluid"}, {previous}},
	{{"mana", "fluid"}, {previous}},
	{{"blank", "rune"}, {previous}},
	{{"spellbook"}, {previous}},

	{{SORCERER, "wand", lt(questValue(333), 1)}, {"Oh, you did not purchase your first magical wand yet? Please take this little present from me as your magic teacher!", setQuestValue(333, 1), setType(3074), setAmount(1), create(TYPE)}},
	{{DRUID, "rod", lt(questValue(333), 1)}, {"Oh, you did not purchase your first magical rod yet? Please take this little present from me as your magic teacher!", setQuestValue(333, 1), setType(3066), setAmount(1), create(TYPE)}},

	{{"spell", SORCERER}, {"I can teach you rune spells and instant spells. What kind of spell do you wish to learn? You can also tell me for which level you would like to learn a spell, if you prefer that.", setTopic(3)}},
	{{"spell", DRUID}, {"I can teach you rune spells and instant spells. What kind of spell do you wish to learn? You can also tell me for which level you would like to learn a spell, if you prefer that.", setTopic(2)}},
	{{"spell"}, {"Sorry, I only sell spells to Sorcerers and Druids."}},

	{{eq(TOPIC, 2), "rune", "spell"}, {"I sell attack rune spells, healing rune spells, support rune spells and summon rune spells. Which of these interests you most?"}},
	{{eq(TOPIC, 2), "instant", "spell"}, {"I sell healing spells, supply spells, support spells and summon spells. Which of these interests you most?"}},
	{{eq(TOPIC, 2), "level"}, {"For which level would you like to learn a spell?", setTopic(2)}},
	{{eq(TOPIC, 2), "bye"}, {"Daraman's blessings.", idle}},

	{{DRUID, "level"}, {"For which level would you like to learn a spell?", setTopic(2)}},
	{{DRUID, "rune", "spell"}, {"I sell attack rune spells, healing rune spells, support rune spells and summon rune spells. Which of these interests you most?"}},
	{{DRUID, "instant", "spell"}, {"I sell healing spells, supply spells, support spells and summon spells. Which of these interests you most?"}},

	{{DRUID, "attack", "rune", "spell"}, {"I sell missile rune spells, explosive rune spells, field rune spells, wall rune spells and bomb rune spells."}},
	{{DRUID, "healing", "rune", "spell"}, {"In this category I have 'Intense Healing Rune', 'Antidote Rune' and 'Ultimate Healing Rune'."}},
	{{DRUID, "support", "rune", "spell"}, {"In this category I have 'Destroy Field' and 'Chameleon'."}},
	{{DRUID, "summon", "rune", "spell"}, {"In this category I have 'Convince Creature'."}},

	{{DRUID, "missile", "rune", "spell"}, {"In this category I have 'Light Magic Missile' and 'Heavy Magic Missile'."}},
	{{DRUID, "explosive", "rune", "spell"}, {"In this category I have 'Fireball', 'Great Fireball' and 'Explosion'."}},
	{{DRUID, "field", "rune", "spell"}, {"In this category I have 'Poison Field', 'Fire Field' and 'Energy Field'."}},
	{{DRUID, "wall", "rune", "spell"}, {"In this category I have 'Fire Wall', 'Poison Wall' and 'Energy Wall'."}},
	{{DRUID, "bomb", "rune", "spell"}, {"In this category I have 'Firebomb'."}},

	{{DRUID, "healing", "spell"}, {"In this category I have 'Light Healing', 'Intense Healing', 'Ultimate Healing' and 'Antidote'."}},
	{{DRUID, "supply", "spell"}, {"In this category I have 'Food'."}},
	{{DRUID, "support", "spell"}, {"In this category I have 'Find Person', 'Light', 'Great Light', 'Magic Shield', 'Creature Illusion' and 'Invisible'."}},
	{{DRUID, "summon", "spell"}, {"In this category I have 'Summon Creature'."}},

	{{DRUID, "find", "person"}, {setType(20), setPrice(80), "Do you want to buy the spell 'Find Person' for %P gold?", setTopic(4)}},
	{{DRUID, "light"}, {setType(10), setPrice(100), "Do you want to buy the spell 'Light' for %P gold?", setTopic(4)}},
	{{DRUID, "food"}, {setType(42), setPrice(300), "Do you want to buy the spell 'Food' for %P gold?", setTopic(4)}},
	{{DRUID, "light", "healing"}, {setType(1), setPrice(170), "Do you want to buy the spell 'Light Healing' for %P gold?", setTopic(4)}},
	{{DRUID, "light", "missile"}, {setType(7), setPrice(500), "Do you want to buy the spell 'Light Magic Missile' for %P gold?", setTopic(4)}},
	{{DRUID, "antidote"}, {setType(29), setPrice(150), "Do you want to buy the spell 'Antidote' for %P gold?", setTopic(4)}},
	{{DRUID, "intense", "healing"}, {setType(2), setPrice(350), "Do you want to buy the spell 'Intense Healing' for %P gold?", setTopic(4)}},
	{{DRUID, "poison", "field"}, {setType(26), setPrice(300), "Do you want to buy the spell 'Poison Field' for %P gold?", setTopic(4)}},
	{{DRUID, "great", "light"}, {setType(11), setPrice(500), "Do you want to buy the spell 'Great Light' for %P gold?", setTopic(4)}},
	{{DRUID, "fire", "field"}, {setType(25), setPrice(500), "Do you want to buy the spell 'Fire Field' for %P gold?", setTopic(4)}},
	{{DRUID, "heavy", "missile"}, {setType(8), setPrice(1500), "Do you want to buy the spell 'Heavy Magic Missile' for %P gold?", setTopic(4)}},
	{{DRUID, "magic", "shield"}, {setType(44), setPrice(450), "Do you want to buy the spell 'Magic Shield' for %P gold?", setTopic(4)}},
	{{DRUID, "intense", "healing", "rune"}, {setType(4), setPrice(600), "Do you want to buy the spell 'Intense Healing Rune' for %P gold?", setTopic(4)}},
	{{DRUID, "antidote", "rune"}, {setType(31), setPrice(600), "Do you want to buy the spell 'Antidote Rune' for %P gold?", setTopic(4)}},
	{{DRUID, "fireball"}, {setType(15), setPrice(800), "Do you want to buy the spell 'Fireball' for %P gold?", setTopic(4)}},
	{{DRUID, "energy", "field"}, {setType(27), setPrice(700), "Do you want to buy the spell 'Energy Field' for %P gold?", setTopic(4)}},
	{{DRUID, "destroy", "field"}, {setType(30), setPrice(700), "Do you want to buy the spell 'Destroy Field' for %P gold?", setTopic(4)}},
	{{DRUID, "ultimate", "healing"}, {setType(3), setPrice(1000), "Do you want to buy the spell 'Ultimate Healing' for %P gold?", setTopic(4)}},
	{{DRUID, "great", "fireball"}, {setType(16), setPrice(1200), "Do you want to buy the spell 'Great Fireball' for %P gold?", setTopic(4)}},
	{{DRUID, "fire", "bomb"}, {setType(17), setPrice(1500), "Do you want to buy the spell 'Fire Bomb' for %P gold?", setTopic(4)}},
	{{DRUID, "creature", "illusion"}, {setType(38), setPrice(1000), "Do you want to buy the spell 'Creature Illusion' for %P gold?", setTopic(4)}},
	{{DRUID, "convince", "creature"}, {setType(12), setPrice(800), "Do you want to buy the spell 'Convince Creature' for %P gold?", setTopic(4)}},
	{{DRUID, "ultimate", "healing", "rune"}, {setType(5), setPrice(1500), "Do you want to buy the spell 'Ultimate Healing Rune' for %P gold?", setTopic(4)}},
	{{DRUID, "chameleon"}, {setType(14), setPrice(1300), "Do you want to buy the spell 'Chameleon' for %P gold?", setTopic(4)}},
	{{DRUID, "poison", "wall"}, {setType(32), setPrice(1600), "Do you want to buy the spell 'Poison Wall' for %P gold?", setTopic(4)}},
	{{DRUID, "explosion"}, {setType(18), setPrice(1800), "Do you want to buy the spell 'Explosion' for %P gold?", setTopic(4)}},
	{{DRUID, "fire", "wall"}, {setType(28), setPrice(2000), "Do you want to buy the spell 'Fire Wall' for %P gold?", setTopic(4)}},
	{{DRUID, "Invisible"}, {setType(45), setPrice(2000), "Do you want to buy the spell 'Invisible' for %P gold?", setTopic(4)}},
	{{DRUID, "summon", "creature"}, {setType(9), setPrice(2000), "Do you want to buy the spell 'Summon Creature' for %P gold?", setTopic(4)}},
	{{DRUID, "energy", "wall"}, {setType(33), setPrice(2500), "Do you want to buy the spell 'Energy Wall' for %P gold?", setTopic(4)}},

	{{eq(TOPIC, 2), "8$"}, {"For level 8 I have 'Find Person' and 'Light'.", setTopic(2)}},
	{{eq(TOPIC, 2), "9$"}, {"For level 9 I have 'Light Healing'.", setTopic(2)}},
	{{eq(TOPIC, 2), "10$"}, {"For level 10 I have 'Antidote'.", setTopic(2)}},
	{{eq(TOPIC, 2), "11$"}, {"For level 11 I have 'Intense Healing'.", setTopic(2)}},
	{{eq(TOPIC, 2), "13$"}, {"For level 13 I have 'Great Light'.", setTopic(2)}},
	{{eq(TOPIC, 2), "14$"}, {"For level 14 I have 'Food', 'Poison Field' and 'Magic Shield'.", setTopic(2)}},
	{{eq(TOPIC, 2), "15$"}, {"For level 15 I have 'Fire Field', 'Intense Healing Rune', 'Antidote Rune' and 'Light Magic Missile'.", setTopic(2)}},
	{{eq(TOPIC, 2), "16$"}, {"For level 16 I have 'Convince Creature'.", setTopic(2)}},
	{{eq(TOPIC, 2), "17$"}, {"For level 17 I have 'Fireball' and 'Destroy Field'.", setTopic(2)}},
	{{eq(TOPIC, 2), "18$"}, {"For level 18 I have 'Energy Field'.", setTopic(2)}},
	{{eq(TOPIC, 2), "20$"}, {"For level 20 I have 'Ultimate Healing'.", setTopic(2)}},
	{{eq(TOPIC, 2), "23$"}, {"For level 23 I have 'Great Fireball' and 'Creature Illusion'.", setTopic(2)}},
	{{eq(TOPIC, 2), "24$"}, {"For level 24 I have 'Ultimate Healing Rune'.", setTopic(2)}},
	{{eq(TOPIC, 2), "25$"}, {"For level 25 I have 'Summon Creature' and 'Heavy Magic Missile'.", setTopic(2)}},
	{{eq(TOPIC, 2), "27$"}, {"For level 27 I have 'Firebomb' and 'Chameleon'.", setTopic(2)}},
	{{eq(TOPIC, 2), "29$"}, {"For level 29 I have 'Poison Wall'.", setTopic(2)}},
	{{eq(TOPIC, 2), "31$"}, {"For level 31 I have 'Explosion'.", setTopic(2)}},
	{{eq(TOPIC, 2), "33$"}, {"For level 33 I have 'Fire Wall'.", setTopic(2)}},
	{{eq(TOPIC, 2), "35$"}, {"For level 35 I have 'Invisible'.", setTopic(2)}},
	{{eq(TOPIC, 2), "41$"}, {"For level 41 I have 'Energy Wall'.", setTopic(2)}},

	{{eq(TOPIC, 2)}, {"Sorry, I have only spells for level 8 to 11, 13 to 18, 20, 23 to 25 as well as for the levels 27, 29, 31, 33, 35 and 41.", setTopic(2)}},

	{{eq(TOPIC, 3), "rune", "spell"}, {"I sell attack rune spells and support rune spells. Which of these interests you most?"}},
	{{eq(TOPIC, 3), "instant", "spell"}, {"I sell attack spells, healing spells, support spells and summon spells. Which of these interests you most?"}},
	{{eq(TOPIC, 3), "level"}, {"For which level would you like to learn a spell?", setTopic(3)}},
	{{eq(TOPIC, 3), "bye"}, {"Daraman's blessings.", idle}},

	{{SORCERER, "level"}, {"For which level would you like to learn a spell?", setTopic(3)}},
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

	{{SORCERER, "find", "person"}, {setType(20), setPrice(80), "Do you want to buy the spell 'Find Person' for %P gold?", setTopic(4)}},
	{{SORCERER, "light"}, {setType(10), setPrice(100), "Do you want to buy the spell 'Light' for %P gold?", setTopic(4)}},
	{{SORCERER, "light", "healing"}, {setType(1), setPrice(170), "Do you want to buy the spell 'Light Healing' for %P gold?", setTopic(4)}},
	{{SORCERER, "light", "missile"}, {setType(7), setPrice(500), "Do you want to buy the spell 'Light Magic Missile' for %P gold?", setTopic(4)}},
	{{SORCERER, "antidote"}, {setType(29), setPrice(150), "Do you want to buy the spell 'Antidote' for %P gold?", setTopic(4)}},
	{{SORCERER, "intense", "healing"}, {setType(2), setPrice(350), "Do you want to buy the spell 'Intense Healing' for %P gold?", setTopic(4)}},
	{{SORCERER, "poison", "field"}, {setType(26), setPrice(300), "Do you want to buy the spell 'Poison Field' for %P gold?", setTopic(4)}},
	{{SORCERER, "great", "light"}, {setType(11), setPrice(500), "Do you want to buy the spell 'Great Light' for %P gold?", setTopic(4)}},
	{{SORCERER, "fire", "field"}, {setType(25), setPrice(500), "Do you want to buy the spell 'Fire Field' for %P gold?", setTopic(4)}},
	{{SORCERER, "heavy", "missile"}, {setType(8), setPrice(1500), "Do you want to buy the spell 'Heavy Magic Missile' for %P gold?", setTopic(4)}},
	{{SORCERER, "magic", "shield"}, {setType(44), setPrice(450), "Do you want to buy the spell 'Magic Shield' for %P gold?", setTopic(4)}},
	{{SORCERER, "fireball"}, {setType(15), setPrice(800), "Do you want to buy the spell 'Fireball' for %P gold?", setTopic(4)}},
	{{SORCERER, "energy", "field"}, {setType(27), setPrice(700), "Do you want to buy the spell 'Energy Field' for %P gold?", setTopic(4)}},
	{{SORCERER, "destroy", "field"}, {setType(30), setPrice(700), "Do you want to buy the spell 'Destroy Field' for %P gold?", setTopic(4)}},
	{{SORCERER, "fire", "wave"}, {setType(19), setPrice(850), "Do you want to buy the spell 'Fire Wave' for %P gold?", setTopic(4)}},
	{{SORCERER, "ultimate", "healing"}, {setType(3), setPrice(1000), "Do you want to buy the spell 'Ultimate Healing' for %P gold?", setTopic(4)}},
	{{SORCERER, "great", "fireball"}, {setType(16), setPrice(1200), "Do you want to buy the spell 'Great Fireball' for %P gold?", setTopic(4)}},
	{{SORCERER, "fire", "bomb"}, {setType(17), setPrice(1500), "Do you want to buy the spell 'Fire Bomb' for %P gold?", setTopic(4)}},
	{{SORCERER, "energy", "beam"}, {setType(22), setPrice(1000), "Do you want to buy the spell 'Energy Beam' for %P gold?", setTopic(4)}},
	{{SORCERER, "creature", "illusion"}, {setType(38), setPrice(1000), "Do you want to buy the spell 'Creature Illusion' for %P gold?", setTopic(4)}},
	{{SORCERER, "poison", "wall"}, {setType(32), setPrice(1600), "Do you want to buy the spell 'Poison Wall' for %P gold?", setTopic(4)}},
	{{SORCERER, "explosion"}, {setType(18), setPrice(1800), "Do you want to buy the spell 'Explosion' for %P gold?", setTopic(4)}},
	{{SORCERER, "fire", "wall"}, {setType(28), setPrice(2000), "Do you want to buy the spell 'Fire Wall' for %P gold?", setTopic(4)}},
	{{SORCERER, "great", "energy", "beam"}, {setType(23), setPrice(1800), "Do you want to buy the spell 'Great Energy Beam' for %P gold?", setTopic(4)}},
	{{SORCERER, "invisible"}, {setType(45), setPrice(2000), "Do you want to buy the spell 'Invisible' for %P gold?", setTopic(4)}},
	{{SORCERER, "summon", "creature"}, {setType(9), setPrice(2000), "Do you want to buy the spell 'Summon Creature' for %P gold?", setTopic(4)}},
	{{SORCERER, "energy", "wall"}, {setType(33), setPrice(2500), "Do you want to buy the spell 'Energy Wall' for %P gold?", setTopic(4)}},
	{{SORCERER, "energy", "wave"}, {setType(13), setPrice(2500), "Do you want to buy the spell 'Energy Wave' for %P gold?", setTopic(4)}},
	{{SORCERER, "sudden", "death"}, {setType(21), setPrice(3000), "Do you want to buy the spell 'Sudden Death' for %P gold?", setTopic(4)}},

	{{eq(TOPIC, 3), "8$"}, {"For level 8 I have 'Find Person' and 'Light'.", setTopic(3)}},
	{{eq(TOPIC, 3), "9$"}, {"For level 9 I have 'Light Healing'.", setTopic(3)}},
	{{eq(TOPIC, 3), "10$"}, {"For level 10 I have 'Antidote'.", setTopic(3)}},
	{{eq(TOPIC, 3), "11$"}, {"For level 11 I have 'Intense Healing'.", setTopic(3)}},
	{{eq(TOPIC, 3), "13$"}, {"For level 13 I have 'Great Light'.", setTopic(3)}},
	{{eq(TOPIC, 3), "14$"}, {"For level 14 I have 'Poison Field' and 'Magic Shield'.", setTopic(3)}},
	{{eq(TOPIC, 3), "15$"}, {"For level 15 I have 'Fire Field' and 'Light Magic Missile'.", setTopic(3)}},
	{{eq(TOPIC, 3), "17$"}, {"For level 17 I have 'Fireball' and 'Destroy Field'.", setTopic(3)}},
	{{eq(TOPIC, 3), "18$"}, {"For level 18 I have 'Energy Field' and 'Fire Wave'.", setTopic(3)}},
	{{eq(TOPIC, 3), "20$"}, {"For level 20 I have 'Ultimate Healing'.", setTopic(3)}},
	{{eq(TOPIC, 3), "23$"}, {"For level 23 I have 'Great Fireball', 'Energy Beam' and 'Creature Illusion'.", setTopic(3)}},
	{{eq(TOPIC, 3), "25$"}, {"For level 25 I have 'Summon Creature' and 'Heavy Magic Missile'.", setTopic(3)}},
	{{eq(TOPIC, 3), "27$"}, {"For level 27 I have 'Firebomb'.", setTopic(3)}},
	{{eq(TOPIC, 3), "29$"}, {"For level 29 I have 'Poison Wall' and 'Great Energy Beam'.", setTopic(3)}},
	{{eq(TOPIC, 3), "31$"}, {"For level 31 I have 'Explosion'.", setTopic(3)}},
	{{eq(TOPIC, 3), "33$"}, {"For level 33 I have 'Fire Wall'.", setTopic(3)}},
	{{eq(TOPIC, 3), "35$"}, {"For level 35 I have 'Invisible'.", setTopic(3)}},
	{{eq(TOPIC, 3), "38$"}, {"For level 38 I have 'Energy Wave'.", setTopic(3)}},
	{{eq(TOPIC, 3), "41$"}, {"For level 41 I have 'Energy Wall'.", setTopic(3)}},
	{{eq(TOPIC, 3), "45$"}, {"For level 45 I have 'Sudden Death'.", setTopic(3)}},

	{{eq(TOPIC, 3)}, {"Hmm, I have no spells for this level, but for many levels from 8 to 45.", setTopic(3)}},

	{{eq(TOPIC, 4), "yes", eq(spellKnown(TYPE), 1)}, {"You already know how to cast this spell."}},
	{{eq(TOPIC, 4), "yes", lt(LEVEL, spellLevel(TYPE))}, {setAmount(spellLevel(TYPE)), "You have to be level %A to learn this spell."}},
	{{eq(TOPIC, 4), "yes", lt(COUNTMONEY, PRICE)}, {"You need more money."}},
	{{eq(TOPIC, 4), "yes"}, {"Here you are. Look in your spellbook for the pronunciation of this spell.", deleteMoney, effectOpp(13), teachSpell(TYPE)}},
	{{eq(TOPIC, 4)}, {"Maybe next time."}},
}

function onCreatureSay(npc, creature, text, situation)
	behaviour.react(npc, rules, text, situation)
end

function onCreatureDisappear(npc, creature)
	behaviour.react(npc, rules, "", NPC_SITUATION_VANISH)
end
