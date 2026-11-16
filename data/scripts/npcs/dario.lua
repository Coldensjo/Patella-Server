-- npcs/dario.lua: the behaviour of the NPC Dario, converted from
-- dario.npc by patella-convert. The NPC is its [[npc]] entry in npcs.toml;
-- lib/npc.lua matches and runs the rules below, in the order of the file
-- (scripting.hh, "NPCs"). The comments are those of dario.npc.
--
-- GIMUD - Graphical Interface Multi User Dungeon
-- dario.npc: Datenbank fuer den elfen dario in der arena (ankrahmun)

local behaviour = require("lib.npc")
local ADDRESS = behaviour.ADDRESS
local BUSY = behaviour.BUSY
local VANISH = behaviour.VANISH
local PALADIN = behaviour.PALADIN
local FORCE = behaviour.FORCE
local P1 = behaviour.P1
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
local mul = behaviour.mul
local setTopic = behaviour.setTopic
local setPrice = behaviour.setPrice
local setAmount = behaviour.setAmount
local setType = behaviour.setType
local effectOpp = behaviour.effectOpp
local teachSpell = behaviour.teachSpell
local create = behaviour.create
local deleteMoney = behaviour.deleteMoney
local queue = behaviour.queue
local idle = behaviour.idle
local nop = behaviour.nop
local previous = behaviour.previous

local rules = behaviour.rules {
	{{ADDRESS, "hello$", FORCE}, {"Ashari %N."}},
	{{ADDRESS, "hi$", FORCE}, {previous}},
	{{ADDRESS, "ashari$", FORCE}, {previous}},
	{{ADDRESS, FORCE}, {idle}},
	{{BUSY, "hello$", FORCE}, {"In a few moment I will have my attention %N.", queue}},
	{{BUSY, "hi$", FORCE}, {previous}},
	{{BUSY, "ashari$", FORCE}, {previous}},
	{{BUSY, FORCE}, {nop}},
	{{VANISH, FORCE}, {"Asha Thrazi."}},

	{{"bye"}, {"Asha Thrazi.", idle}},
	{{"asha", "thrazi"}, {previous}},
	{{"farewell"}, {previous}},

	{{"job"}, {"I am the master archer of the arena. I train distance fighters and sell them equipment."}},
	{{"name"}, {"I am Dario of Ab'Dendriel."}},
	{{"time"}, {"Time is unimportant to me."}},
	{{"temple"}, {"The temple is somewhere south at the coast."}},
	{{"arkhothep"}, {"The pharaoh seems to be mighty beyond imagination."}},
	{{"ashmunrah"}, {"There was some fighting long ago. The old pharaoh lost his power to his son Arkhothep."}},
	{{"scarab"}, {"Scarabs are dangerous. They are quick, resistant to poison and theis shells are hard as steel."}},
	{{"tibia"}, {"I travel a lot to see everything. For now I settle here for some time."}},
	{{"carlin"}, {"I was there some time ago. It was lovely and reminded me of my home Ab'Dendriel."}},
	{{"thais"}, {"Thais is too crowded for my taste."}},
	{{"edron"}, {"I think Edron is quite typical for a human settlement."}},
	{{"venore"}, {"I did not like the greedy attitude of the people there."}},
	{{"kazordoon"}, {"The small people are too hectic and greedy. They don't understand the harmony of nature."}},
	{{"dwarves"}, {previous}},
	{{"dwarfes"}, {previous}},
	{{"ab'dendriel"}, {"Sometimes I miss my brethren and sisters. But for now I want to see the world and travel around."}},
	{{"elves"}, {previous}},
	{{"elfes"}, {previous}},
	{{"darama"}, {"This continent is hard and challenging. I like challenges."}},
	{{"darashia"}, {"The city seemed a bit dull and peacefull to me, so I left for Ankrahmun."}},
	{{"daraman"}, {"You should ask about him in Darashia. People there talked a lot about him."}},
	{{"ankrahmun"}, {"Ankrahmun is unlike any other city I've seen. Sometimes it gives me shivers ... on the other hand it makes me stay on guard and feel alive, despite the undeath cult."}},

	{{"ascension"}, {"I don't care for this human concepts."}},
	{{"Akh'rah", "Uthun"}, {previous}},
	{{"Akh"}, {previous}},
	{{"Rah"}, {previous}},
	{{"uthun"}, {previous}},
	{{"undead"}, {"I don't understand this cult yet. Just ask around and people will tell you."}},
	{{"undeath"}, {previous}},
	{{"arena"}, {"People who fight here do it on their own choice. So I don't care."}},
	{{"palace"}, {"Under the palace are crypts, full of minor undead and creatures that have failed the pharaoh. He allows everyone to slay them as they see it fit."}},

	{{"spell", PALADIN}, {"I can teach you rune spells and instant spells. What kind of spell do you wish to learn? You can also tell me for which level you would like to learn a spell, if you prefer that.", setTopic(2)}},
	{{"spell"}, {"Sorry, I only sell spells to paladins."}},

	{{eq(TOPIC, 2), "rune", "spell"}, {"I sell 'Light Magic Missile', 'Heavy Magic Missile', 'Fireball' and 'Destroy Field'."}},
	{{eq(TOPIC, 2), "instant", "spell"}, {"I sell healing spells, supply spells and support spells. Which of these interests you most?"}},
	{{eq(TOPIC, 2), "level"}, {"For which level would you like to learn a spell?", setTopic(2)}},
	{{eq(TOPIC, 2), "bye"}, {"Asha Thrazi.", idle}},

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
	{{eq(TOPIC, 3), "yes", lt(LEVEL, spellLevel(TYPE))}, {setAmount(spellLevel(TYPE)), "You have to be level %A to learn this spell."}},
	{{eq(TOPIC, 3), "yes", lt(COUNTMONEY, PRICE)}, {"Return when you have enough gold."}},
	{{eq(TOPIC, 3), "yes"}, {"Here you are. Look in your spellbook for the pronunciation of this spell.", deleteMoney, effectOpp(13), teachSpell(TYPE)}},
	{{eq(TOPIC, 3)}, {"Maybe next time."}},

	{{"buy"}, {"I am selling bows, crossbows, and ammunition. Do you need anything?"}},
	{{"do", "you", "sell"}, {previous}},
	{{"do", "you", "have"}, {previous}},
	{{"offer"}, {previous}},
	{{"goods"}, {previous}},
	{{"ammo"}, {"Do you need arrows for a bow, or bolts for a crossbow?"}},
	{{"ammunition"}, {previous}},

	{{"sell", "bow"}, {"I don't buy used bows."}},
	{{"sell", "crossbow"}, {"I don't buy used crossbows."}},

	{{"bow"}, {setType(3350), setAmount(1), setPrice(400), "Do you want to buy a bow for %P gold?", setTopic(4)}},
	{{"crossbow"}, {setType(3349), setAmount(1), setPrice(500), "Do you want to buy a crossbow for %P gold?", setTopic(4)}},
	{{"arrow"}, {setType(3447), setAmount(1), setPrice(2), "Do you want to buy an arrow for %P gold?", setTopic(4)}},
	{{"bolt"}, {setType(3446), setAmount(1), setPrice(3), "Do you want to buy a bolt for %P gold?", setTopic(4)}},

	{{P1, lt(1, P1), "bow"}, {setType(3350), setAmount(P1), setPrice(mul(400, P1)), "Do you want to buy %A bows for %P gold?", setTopic(4)}},
	{{P1, lt(1, P1), "crossbow"}, {setType(3349), setAmount(P1), setPrice(mul(500, P1)), "Do you want to buy %A crossbows for %P gold?", setTopic(4)}},
	{{P1, lt(1, P1), "arrow"}, {setType(3447), setAmount(P1), setPrice(mul(2, P1)), "Do you want to buy %A arrows for %P gold?", setTopic(4)}},
	{{P1, lt(1, P1), "bolt"}, {setType(3446), setAmount(P1), setPrice(mul(3, P1)), "Do you want to buy %A bolts for %P gold?", setTopic(4)}},

	{{eq(TOPIC, 4), "yes", ge(COUNTMONEY, PRICE)}, {"Here you are.", deleteMoney, create(TYPE)}},
	{{eq(TOPIC, 4), "yes"}, {"Come back, when you have enough money."}},
	{{eq(TOPIC, 4)}, {"Hmm, but next time."}},
}

function onCreatureSay(npc, creature, text, situation)
	behaviour.react(npc, rules, text, situation)
end

function onCreatureDisappear(npc, creature)
	behaviour.react(npc, rules, "", NPC_SITUATION_VANISH)
end
