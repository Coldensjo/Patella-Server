-- npcs/padreia.lua: the behaviour of the NPC Padreia, converted from
-- padreia.npc by patella-convert. The NPC is its [[npc]] entry in npcs.toml;
-- lib/npc.lua matches and runs the rules below, in the order of the file
-- (scripting.hh, "NPCs"). The comments are those of padreia.npc.
--
-- GIMUD - Graphical Interface Multi User Dungeon
-- padreia.npc: Datenbank fuer die Druidin Padreia

local behaviour = require("lib.npc")
local ADDRESS = behaviour.ADDRESS
local BUSY = behaviour.BUSY
local VANISH = behaviour.VANISH
local DRUID = behaviour.DRUID
local FORCE = behaviour.FORCE
local eq = behaviour.eq
local lt = behaviour.lt
local ge = behaviour.ge
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
	{{ADDRESS, "hello$", DRUID, FORCE}, {"Crunor's blessings. I am glad to see you again, %N!"}},
	{{ADDRESS, "hi$", DRUID, FORCE}, {"Crunor's blessings. I am glad to see you again, %N!"}},
	{{ADDRESS, "hello$", FORCE}, {"Welcome to our humble guild, wanderer. May I be of any assistance to you?"}},
	{{ADDRESS, "hi$", FORCE}, {"Welcome to our humble guild, wanderer. May I be of any assistance to you?"}},
	{{ADDRESS, FORCE}, {idle}},
	{{BUSY, "hello$", FORCE}, {"Please wait, %N.", queue}},
	{{BUSY, "hi$", FORCE}, {"Please wait, %N.", queue}},
	{{BUSY, FORCE}, {nop}},
	{{VANISH, DRUID, FORCE}, {"Farewell, %N. May Crunor be with you, my child."}},
	{{VANISH, FORCE}, {"Farewell. May Crunor be with you."}},

	{{"bye", DRUID}, {"Farewell, %N.", idle}},
	{{"farewell", DRUID}, {previous}},
	{{"bye"}, {"Farewell.", idle}},
	{{"farewell"}, {previous}},
	{{"job"}, {"I am the grand druid of Carlin. I am responsible for the guild, the fields, and our citizens health."}},
	{{"grand", "druid"}, {previous}},
	{{"name"}, {"I am Padreia, Grand Druid of our fine city."}},
	{{"time"}, {"Time is just a crystal pillar. The center of creation and life."}},
	{{"member"}, {"Our members wield magic powers of protection and healing."}},
	{{"magic"}, {"Every member of the Druids is able to learn the numerous spells of our craft."}},
	{{"power"}, {previous}},
	{{"druid"}, {"We are druids, preservers of life. Our magic is about defense, healing, and nature."}},
	{{"sorcerer"}, {"Sorcerers are destrucitve. Their power lies in destruction and pain."}},
	{{"vocation"}, {"Your vocation is your profession. There are four vocations in this world: Druids, paladins, knights, and sorcerers."}},
	{{"spellbook"}, {"In a spellbook, your spells are listed. There you will find the pronunciation of each spell. If you want to buy one, visit Rachel."}},
	{{"spell", DRUID}, {"I can teach you rune spells and instant spells. What kind of spell do you wish to learn? You can also tell me for which level you would like to learn a spell, if you prefer that.", setTopic(2)}},
	{{"spell"}, {"Sorry, I only sell spells to druids."}},

	{{"crunor", "caress"}, {"Don't ask. They were only an unimportant footnote of history."}},
	{{"footnote", eq(questValue(211), 2)}, {"They thought they have to bring Crunor to the people, if people did not find to Crunor of their own. To achieve that they founded the inn Crunor's Cottage, south of Mt. Sternum.", setQuestValue(211, 3)}},
	{{"footnote", lt(questValue(211), 2)}, {"I have to attend other business, ask later please."}},

	{{eq(TOPIC, 2), "rune", "spell"}, {"I sell attack rune spells, healing rune spells, support rune spells and summon rune spells. Which of these interests you most?"}},
	{{eq(TOPIC, 2), "instant", "spell"}, {"I sell healing spells, supply spells, support spells and summon spells. Which of these interests you most?"}},
	{{eq(TOPIC, 2), "level"}, {"For which level would you like to learn a spell?", setTopic(2)}},
	{{eq(TOPIC, 2), "bye"}, {"Farewell.", idle}},

	{{DRUID, "rod", lt(questValue(333), 1)}, {"Oh, you did not purchase your first magical rod yet? Please take this little present from me as your magic teacher!", setQuestValue(333, 1), setType(3066), setAmount(1), create(TYPE)}},

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

	{{DRUID, "find", "person"}, {setType(20), setPrice(80), "Do you want to buy the spell 'Find Person' for %P gold?", setTopic(3)}},
	{{DRUID, "light"}, {setType(10), setPrice(100), "Do you want to buy the spell 'Light' for %P gold?", setTopic(3)}},
	{{DRUID, "food"}, {setType(42), setPrice(300), "Do you want to buy the spell 'Food' for %P gold?", setTopic(3)}},
	{{DRUID, "light", "healing"}, {setType(1), setPrice(170), "Do you want to buy the spell 'Light Healing' for %P gold?", setTopic(3)}},
	{{DRUID, "light", "missile"}, {setType(7), setPrice(500), "Do you want to buy the spell 'Light Magic Missile' for %P gold?", setTopic(3)}},
	{{DRUID, "antidote"}, {setType(29), setPrice(150), "Do you want to buy the spell 'Antidote' for %P gold?", setTopic(3)}},
	{{DRUID, "intense", "healing"}, {setType(2), setPrice(350), "Do you want to buy the spell 'Intense Healing' for %P gold?", setTopic(3)}},
	{{DRUID, "poison", "field"}, {setType(26), setPrice(300), "Do you want to buy the spell 'Poison Field' for %P gold?", setTopic(3)}},
	{{DRUID, "great", "light"}, {setType(11), setPrice(500), "Do you want to buy the spell 'Great Light' for %P gold?", setTopic(3)}},
	{{DRUID, "fire", "field"}, {setType(25), setPrice(500), "Do you want to buy the spell 'Fire Field' for %P gold?", setTopic(3)}},
	{{DRUID, "heavy", "missile"}, {setType(8), setPrice(1500), "Do you want to buy the spell 'Heavy Magic Missile' for %P gold?", setTopic(3)}},
	{{DRUID, "magic", "shield"}, {setType(44), setPrice(450), "Do you want to buy the spell 'Magic Shield' for %P gold?", setTopic(3)}},
	{{DRUID, "intense", "healing", "rune"}, {setType(4), setPrice(600), "Do you want to buy the spell 'Intense Healing Rune' for %P gold?", setTopic(3)}},
	{{DRUID, "antidote", "rune"}, {setType(31), setPrice(600), "Do you want to buy the spell 'Antidote Rune' for %P gold?", setTopic(3)}},
	{{DRUID, "fireball"}, {setType(15), setPrice(800), "Do you want to buy the spell 'Fireball' for %P gold?", setTopic(3)}},
	{{DRUID, "energy", "field"}, {setType(27), setPrice(700), "Do you want to buy the spell 'Energy Field' for %P gold?", setTopic(3)}},
	{{DRUID, "destroy", "field"}, {setType(30), setPrice(700), "Do you want to buy the spell 'Destroy Field' for %P gold?", setTopic(3)}},
	{{DRUID, "ultimate", "healing"}, {setType(3), setPrice(1000), "Do you want to buy the spell 'Ultimate Healing' for %P gold?", setTopic(3)}},
	{{DRUID, "great", "fireball"}, {setType(16), setPrice(1200), "Do you want to buy the spell 'Great Fireball' for %P gold?", setTopic(3)}},
	{{DRUID, "fire", "bomb"}, {setType(17), setPrice(1500), "Do you want to buy the spell 'Fire Bomb' for %P gold?", setTopic(3)}},
	{{DRUID, "creature", "illusion"}, {setType(38), setPrice(1000), "Do you want to buy the spell 'Creature Illusion' for %P gold?", setTopic(3)}},
	{{DRUID, "convince", "creature"}, {setType(12), setPrice(800), "Do you want to buy the spell 'Convince Creature' for %P gold?", setTopic(3)}},
	{{DRUID, "ultimate", "healing", "rune"}, {setType(5), setPrice(1500), "Do you want to buy the spell 'Ultimate Healing Rune' for %P gold?", setTopic(3)}},
	{{DRUID, "chameleon"}, {setType(14), setPrice(1300), "Do you want to buy the spell 'Chameleon' for %P gold?", setTopic(3)}},
	{{DRUID, "poison", "wall"}, {setType(32), setPrice(1600), "Do you want to buy the spell 'Poison Wall' for %P gold?", setTopic(3)}},
	{{DRUID, "explosion"}, {setType(18), setPrice(1800), "Do you want to buy the spell 'Explosion' for %P gold?", setTopic(3)}},
	{{DRUID, "fire", "wall"}, {setType(28), setPrice(2000), "Do you want to buy the spell 'Fire Wall' for %P gold?", setTopic(3)}},
	{{DRUID, "Invisible"}, {setType(45), setPrice(2000), "Do you want to buy the spell 'Invisible' for %P gold?", setTopic(3)}},
	{{DRUID, "summon", "creature"}, {setType(9), setPrice(2000), "Do you want to buy the spell 'Summon Creature' for %P gold?", setTopic(3)}},
	{{DRUID, "energy", "wall"}, {setType(33), setPrice(2500), "Do you want to buy the spell 'Energy Wall' for %P gold?", setTopic(3)}},

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

	{{eq(TOPIC, 3), "yes", eq(spellKnown(TYPE), 1)}, {"You already know how to cast this spell."}},
	{{eq(TOPIC, 3), "yes", lt(LEVEL, spellLevel(TYPE))}, {setAmount(spellLevel(TYPE)), "You have to be level %A to learn this spell."}},
	{{eq(TOPIC, 3), "yes", lt(COUNTMONEY, PRICE)}, {"Sorry, you do not have enough gold."}},
	{{eq(TOPIC, 3), "yes"}, {"Here you are. Look in your spellbook for the pronunciation of this spell.", deleteMoney, effectOpp(13), teachSpell(TYPE)}},
	{{eq(TOPIC, 3)}, {"Maybe next time."}},

	{{"cough", "syrup"}, {setType(4828), setPrice(50), "Do you want to buy a bottle of cough syrup for %P gold?", setTopic(10)}},
	{{eq(TOPIC, 10), "yes", ge(COUNTMONEY, PRICE)}, {"Thank you. Here it is.", deleteMoney, create(TYPE)}},
	{{eq(TOPIC, 10), "yes"}, {"Sorry, you do not have enough gold."}},
	{{eq(TOPIC, 10)}, {"Maybe you will need it another time."}},
}

function onCreatureSay(npc, creature, text, situation)
	behaviour.react(npc, rules, text, situation)
end

function onCreatureDisappear(npc, creature)
	behaviour.react(npc, rules, "", NPC_SITUATION_VANISH)
end
