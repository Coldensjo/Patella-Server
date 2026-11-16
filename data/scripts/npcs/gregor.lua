-- npcs/gregor.lua: the behaviour of the NPC Gregor, converted from
-- gregor.npc by patella-convert. The NPC is its [[npc]] entry in npcs.toml;
-- lib/npc.lua matches and runs the rules below, in the order of the file
-- (scripting.hh, "NPCs"). The comments are those of gregor.npc.
--
-- GIMUD - Graphical Interface Multi User Dungeon
-- gregor.npc: Datenbank fuer den Ritter Gregor

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
	{{ADDRESS, KNIGHT, "hello$", FORCE}, {"Welcome home, Knight %N!"}},
	{{ADDRESS, KNIGHT, "hi$", FORCE}, {previous}},
	{{ADDRESS, "hello$", FORCE}, {"Greetings, %N. What do you want?"}},
	{{ADDRESS, "hi$", FORCE}, {previous}},
	{{ADDRESS, FORCE}, {idle}},
	{{BUSY, "hello$", FORCE}, {"Please wait a moment, %N.", queue}},
	{{BUSY, "hi$", FORCE}, {previous}},
	{{BUSY, FORCE}, {nop}},
	{{VANISH, FORCE}, {"Be careful on your journeys."}},

	{{"bye"}, {"Be careful on your journeys.", idle}},
	{{"farewell"}, {previous}},
	{{"job"}, {"I am the first knight. I trained some of the greatest heroes of Tibia."}},
	{{"name"}, {"You are joking, eh? Of course, you know me. I am Gregor, the first knight."}},
	{{"time"}, {"It is time to join the Knights!"}},
	{{"king"}, {"Hail to our King!"}},
	{{"tibianus"}, {previous}},
	{{"quentin"}, {"I will never understand this peaceful monks and priests."}},
	{{"lynda"}, {"Before she became a priest she won the Miss Tibia contest three times in a row."}},
	{{"harkath"}, {"One of Tibia's greatest warriors and strategists."}},
	{{"army"}, {"I teached many of the guards personally."}},
	{{"ferumbras"}, {"A fine game to hunt. But be careful, he cheats!"}},
	{{"general"}, {"General Harkath Bloodblade, a rolemodel."}},
	{{"sam"}, {"He has the muscles, but lacks the guts."}},
	{{"gorn"}, {"Always concerned with his profit. What a loss! He was adventuring with baxter in the old days."}},
	{{"frodo"}, {"I and my students often share a cask of beer or wine at Frodo's hut."}},
	{{"elane"}, {"A bow might be a fine weapon for someone not strong enough to wield a REAL weapon."}},
	{{"muriel"}, {"Bah, go away with these sorcerer tricks. Only cowards use tricks."}},
	{{"gregor"}, {"A great name, isn't it?"}},
	{{"marvik"}, {"Old Marvik saved life and limb of many of my boys and girls."}},
	{{"bozo"}, {"Some day someone will make something happen to him..."}},
	{{"baxter"}, {"He was an adventurer once."}},
	{{"oswald"}, {"What an idiot."}},
	{{"sherry"}, {"Peaceful farmers."}},
	{{"donald"}, {previous}},
	{{"mcronald"}, {previous}},
	{{"lugri"}, {"If he would have some guts he would fight for what he's talking about."}},
	{{"excalibug"}, {"Many brave warriors died on the quest to find that fabled weapon."}},
	{{"news"}, {"Times of war are at hand."}},

	{{"hero"}, {"Of course, you heard of them. Knights are the best fighters in Tibia."}},
	{{"tibia"}, {"Beautiful Tibia. And with our help everyone is safe."}},
	{{"knight"}, {"Knights are the warriors of Tibia. Without us, no one would be safe. Every brave and strong man or woman can join us."}},
	{{"vocation"}, {"Your vocation is your profession. There are four vocations in Tibia: Knights, paladins, sorcerers, and druids."}},
	{{"spellbook"}, {"In a spellbook, your spells are listed. There you will find the pronunciation of each spell. If you want to buy one, visit Muriel, the sorcerer."}},

	{{KNIGHT, "spell"}, {"I can teach you healing spells and support spells. What kind of spell do you wish to learn? You can also tell me for which level you would like to learn a spell, if you prefer that.", setTopic(2)}},
	{{"spell"}, {"Sorry, I only sell spells to knights."}},

	{{KNIGHT, "instant", "spell"}, {"I can teach you healing spells and support spells. What kind of spell do you wish to learn?"}},
	{{KNIGHT, "healing", "spell"}, {"In this category I have 'Light Healing' and 'Antidote'."}},
	{{KNIGHT, "support", "spell"}, {"In this category I have 'Light', 'Find Person' and 'Great Light'."}},

	{{eq(TOPIC, 2), "healing", "spell"}, {"In this category I have 'Light Healing' and 'Antidote'."}},
	{{eq(TOPIC, 2), "support", "spell"}, {"In this category I have 'Light', 'Find Person' and 'Great Light'."}},
	{{eq(TOPIC, 2), "bye"}, {"Be careful on your journeys.", idle}},

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
