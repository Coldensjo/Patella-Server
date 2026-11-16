-- npcs/queen.lua: the behaviour of the NPC Queen Eloise, converted from
-- queen.npc by patella-convert. The NPC is its [[npc]] entry in npcs.toml;
-- lib/npc.lua matches and runs the rules below, in the order of the file
-- (scripting.hh, "NPCs"). The comments are those of queen.npc.
--
-- GIMUD - Graphical Interface Multi User Dungeon
-- queen.npc: Datenbank für die Königin von Carlin

local behaviour = require("lib.npc")
local ADDRESS = behaviour.ADDRESS
local BUSY = behaviour.BUSY
local VANISH = behaviour.VANISH
local MALE = behaviour.MALE
local FEMALE = behaviour.FEMALE
local PREMIUM = behaviour.PREMIUM
local PROMOTED = behaviour.PROMOTED
local FORCE = behaviour.FORCE
local eq = behaviour.eq
local lt = behaviour.lt
local TOPIC = behaviour.TOPIC
local PRICE = behaviour.PRICE
local LEVEL = behaviour.LEVEL
local COUNTMONEY = behaviour.COUNTMONEY
local questValue = behaviour.questValue
local setTopic = behaviour.setTopic
local setPrice = behaviour.setPrice
local setQuestValue = behaviour.setQuestValue
local profession = behaviour.profession
local deleteMoney = behaviour.deleteMoney
local idle = behaviour.idle
local nop = behaviour.nop
local previous = behaviour.previous

local rules = behaviour.rules {
	{{ADDRESS, "hello", "queen", FORCE}, {"I greet thee, my loyal subject."}},
	{{ADDRESS, "hail", "queen", FORCE}, {previous}},
	{{ADDRESS, "salutations", "queen", FORCE}, {previous}},
	{{ADDRESS, FORCE}, {idle}},
	{{BUSY, FORCE}, {nop}},
	{{VANISH, MALE, FORCE}, {"Typical behaviour for males!"}},
	{{VANISH, FORCE}, {"What a strange behaviour for a lady!"}},

	{{"uniform", eq(questValue(233), 5)}, {"I remember about those uniforms, they had a camouflage inlay so they could be worn the inside out too. I will send some colorsamples via mail to Mr. Postner.", setQuestValue(233, 6)}},
	{{"uniform"}, {"The uniforms of our guards and soldiers are of unparraleled quality of course."}},

	{{"bye"}, {"Farewell, %N!", idle}},
	{{"farewell"}, {previous}},
	{{"job"}, {"I am Queen Eloise. It is my duty to reign over this marvellous city and the lands of the north."}},
	{{"justice"}, {"We women try to bring justice and wisdom to all, even to males."}},
	{{"name"}, {"I am Queen Eloise. For you it's 'My Queen' or 'Your Majesty', of course."}},
	{{"news"}, {"I don't care about gossip like a simpleminded male would do."}},
	{{"tibia"}, {"Soon the whole land will be ruled by women at last!"}},
	{{"land"}, {previous}},
	{{"how", "are", "you"}, {"Thank you, I'm fine."}},
	{{"castle"}, {"It's my humble domain."}},
	{{"sell", MALE}, {"Sell? Your question shows that you are a typical member of your gender!"}},
	{{"sell", FEMALE}, {"I beg you pardon? A queen that sells things? Be serious!"}},
	{{"god"}, {"We honor the gods of good in our fair city, especially Crunor, of course."}},
	{{"citizen"}, {"All citizens of Carlin are my subjects. I see them more as my childs, though, epecially the male population."}},
	{{"noodles"}, {"This beast scared my cat away on my last diplomatic mission in this filthy town."}},
	{{"ferumbras"}, {"He is the scourge of the whole continent!"}},
	{{"treasure"}, {"The royal treasure is hidden beyond the grasps of any thieves by magical means."}},
	{{"monster"}, {"Go and hunt them! For queen and country!"}},
	{{"help"}, {"Visit the church or the townguards for help."}},
	{{"quest"}, {"I will call for heroes as soon as the need arises again."}},
	{{"mission"}, {previous}},
	{{"gold"}, {"Our city is rich and prospering."}},
	{{"money"}, {previous}},
	{{"tax"}, {previous}},
	{{"sewer"}, {"I don't want to talk about 'sewers'."}},
	{{"dungeon"}, {"Dungeons are places where males crawl around and look for trouble."}},
	{{"equipment"}, {"Feel free to visit our town's magnificent shops."}},
	{{"food"}, {previous}},
	{{"time"}, {"Don't worry about time in the presence of your Queen."}},
	{{"hero"}, {"We need the assistance of heroes now and then. Even males prove useful now and then."}},
	{{"adventurer"}, {previous}},
	{{"tax", "collector"}, {"The taxes in Carlin are not high, more a symbol than a sacrifice."}},
	{{"queen"}, {"I am the Queen, the only rightful ruler on the continent!"}},
	{{"army"}, {"Ask one of the soldiers about that."}},
	{{"enemy"}, {"Our enemies are numerous. We have to fight vile monsters and have to watch this silly king in the south carefully."}},
	{{"enemies"}, {previous}},
	{{"thais"}, {"They dare to reject my reign over them!"}},
	{{"city", "south"}, {previous}},
	{{"carlin"}, {"Isn't our city marvellous? Have you noticed the lovely gardens on the roofs?"}},
	{{"city"}, {previous}},
	{{"shop"}, {"My subjects maintain many fine shops. Go and have a look at their wares."}},
	{{"merchant"}, {"Ask around about them."}},
	{{"craftsmen"}, {previous}},
	{{"guild"}, {"The four major guilds are the Knights, the Paladins, the Druids, and the Sorcerers."}},
	{{"minotaur"}, {"They havn't troubled our city lately. I guess, they fear the wrath of our druids."}},
	{{"paladin"}, {"The paladins are great hunters."}},
	{{"legola"}, {previous}},
	{{"elane"}, {"It's a shame that the High Paladin does not reside in Carlin."}},
	{{"knight"}, {"The knights of Carlin are the bravest."}},
	{{"trisha"}, {previous}},
	{{"sorceror"}, {"The sorcerers have a small isle for their guild. So if they blow something up it does not burn the whole city to ruins."}},
	{{"lea$"}, {previous}},
	{{"druid"}, {"The druids of Carlin are our protectors and advisors. Their powers provide us with wealth and food."}},
	{{"padreia"}, {previous}},
	{{"good"}, {"Carlin is a center of the forces of good, of course."}},
	{{"evil"}, {"The forces of evil have a firm grip on this puny city to the south."}},
	{{"order"}, {"The order, Crunor gives the world, is essential for survival."}},
	{{"chaos"}, {"Chaos is common in the southern regions, where they allow a man to reign over a realm."}},
	{{"excalibug"}, {"A mans tale ... that means 'nonsense', of course."}},
	{{"reward"}, {"If you want a reward, go and bring me something this silly King Tibianus wants dearly!"}},
	{{"tbi"}, {"A dusgusting organisation, which could be only created by men."}},

	{{"promot"}, {setPrice(20000), "Do you want to be promoted in your vocation for %P gold?", setTopic(4)}},
	{{eq(TOPIC, 4), "yes", PROMOTED, FORCE}, {"You are already promoted."}},
	{{eq(TOPIC, 4), "yes", lt(LEVEL, 20), FORCE}, {"You need to be at least level 20 in order to be promoted."}},
	{{eq(TOPIC, 4), "yes", lt(COUNTMONEY, PRICE), FORCE}, {"You do not have enough money."}},
	{{eq(TOPIC, 4), "yes", PREMIUM}, {"Congratulations! You are now promoted. Visit the sage Eremo for new spells.", profession(10), deleteMoney}},
	{{eq(TOPIC, 4), "yes"}, {"You need a premium account in order to promote."}},
	{{eq(TOPIC, 4)}, {"Ok, then not."}},
	{{"eremo"}, {"It is said that he lives on a small island near Edron. Maybe the people there know more about him."}},
}

function onCreatureSay(npc, creature, text, situation)
	behaviour.react(npc, rules, text, situation)
end

function onCreatureDisappear(npc, creature)
	behaviour.react(npc, rules, "", NPC_SITUATION_VANISH)
end
