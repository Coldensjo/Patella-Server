-- npcs/uso.lua: the behaviour of the NPC Uso, converted from
-- uso.npc by patella-convert. The NPC is its [[npc]] entry in npcs.toml;
-- lib/npc.lua matches and runs the rules below, in the order of the file
-- (scripting.hh, "NPCs"). The comments are those of uso.npc.
--
-- GIMUD - Graphical Interface Multi User Dungeon
-- uso.npc: Datenbank für den Zwerg Uso der auch Knights trainiert

local behaviour = require("lib.npc")
local ADDRESS = behaviour.ADDRESS
local BUSY = behaviour.BUSY
local VANISH = behaviour.VANISH
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
	{{ADDRESS, "hello$", FORCE}, {"Hiho."}},
	{{ADDRESS, "hi$", FORCE}, {previous}},
	{{ADDRESS, "hiho$", FORCE}, {previous}},
	{{ADDRESS, FORCE}, {idle}},

	{{BUSY, "hello$", FORCE}, {"Hey, just wait, ok?", queue}},
	{{BUSY, "hi$", FORCE}, {previous}},
	{{BUSY, "hiho$", FORCE}, {previous}},
	{{BUSY, FORCE}, {nop}},
	{{VANISH, FORCE}, {"So long."}},

	{{"bye"}, {"So long.", idle}},
	{{"farewell"}, {previous}},
	{{"job"}, {"I am the head of a little mining operation here and I train knights in my spare time to prevent my old body from rusting."}},
	{{"name"}, {"I am called Uso Oredigger, son of the flame from the dragoneater fellowship."}},
	{{"time"}, {"Time is of little importance at this forsaken place."}},
	{{"temple"}, {"There is a somewhat provisional temple of the humans here."}},
	{{"king"}, {"Human kings are not of my concern."}},
	{{"venore"}, {"To me one human is like the other. I don't care what city they are from."}},
	{{"thais"}, {previous}},
	{{"carlin"}, {previous}},
	{{"edron"}, {previous}},
	{{"jungle"}, {"All those trees and the heat are horrible. If it wasn't for the gold, we wouldn't be here, jawoll."}},
	{{"gold"}, {"There IS gold out there. And a lot of it. I can feel it in my old bones."}},

	{{"tibia"}, {"The world is big but the random places where gold and treasures can be found are the ones that are of importance, jawoll."}},

	{{"kazordoon"}, {"We dwarves call it our home since the dawn of time. I miss Kazordoon a lot, but gold is of more importance. After I have made a fortune here I will return home and might settle down."}},
	{{"dwarves"}, {"We are a proud race. Dwarves are strong and fearless. Even in this forsaken jungle we can survive, jawoll."}},
	{{"dwarfs"}, {previous}},
	{{"ab'dendriel"}, {"That is no city but just a bunch of trees."}},
	{{"elves"}, {"I wonder why we see so few elves over here. Those tree people should love this cursed jungle. But even they stay away from here. It makes me wonder why."}},
	{{"elfs"}, {previous}},
	{{"darama"}, {"I wonder which part of Darama is worst, that jungle or that desert. This whole continent is a dwarf's nightmare."}},
	{{"ankrahmun"}, {"Another monument of the madness of the human race. Worship of death or undeath or whatever ... I wonder how often I must be slammed on my head to think of such a crazy idea."}},
	{{"ferumbras"}, {"He would be at least a diversion from all those crawling and flying insects."}},
	{{"excalibug"}, {"If it ever had been in this area, it surely would be rusted through by now."}},

	{{"apes"}, {"It wouldn't surprise me if they were only elves disguised with furs."}},
	{{"lizard"}, {"They say the lizards had a lot of gold in their ancient cities. Perhaps one day we will go there and look for it."}},
	{{"dworcs"}, {"I recently broke the nose of a guy who dared to claim that those headhunters are related to dwarves."}},

	{{KNIGHT, "spell"}, {"I can teach you healing spells and support spells. What kind of spell do you wish to learn? You can also tell me for which level you would like to learn a spell, if you prefer that.", setTopic(2)}},
	{{"spell"}, {"Sorry, I only sell spells to knights."}},

	{{KNIGHT, "instant", "spell"}, {"I can teach you healing spells and support spells. What kind of spell do you wish to learn?"}},
	{{KNIGHT, "healing", "spell"}, {"In this category I have 'Light Healing' and 'Antidote'."}},
	{{KNIGHT, "support", "spell"}, {"In this category I have 'Light', 'Find Person' and 'Great Light'."}},

	{{eq(TOPIC, 2), "healing", "spell"}, {"In this category I have 'Light Healing' and 'Antidote'."}},
	{{eq(TOPIC, 2), "support", "spell"}, {"In this category I have 'Light', 'Find Person' and 'Great Light'."}},
	{{eq(TOPIC, 2), "bye"}, {"So long.", idle}},

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
