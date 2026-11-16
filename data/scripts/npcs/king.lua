-- npcs/king.lua: the behaviour of the NPC King Tibianus, converted from
-- king.npc by patella-convert. The NPC is its [[npc]] entry in npcs.toml;
-- lib/npc.lua matches and runs the rules below, in the order of the file
-- (scripting.hh, "NPCs"). The comments are those of king.npc.
--
-- GIMUD - Graphical Interface Multi User Dungeon
-- king.npc: Datenbank für den König von Tibia

local behaviour = require("lib.npc")
local ADDRESS = behaviour.ADDRESS
local BUSY = behaviour.BUSY
local VANISH = behaviour.VANISH
local PREMIUM = behaviour.PREMIUM
local PROMOTED = behaviour.PROMOTED
local FORCE = behaviour.FORCE
local eq = behaviour.eq
local lt = behaviour.lt
local TOPIC = behaviour.TOPIC
local PRICE = behaviour.PRICE
local LEVEL = behaviour.LEVEL
local COUNTMONEY = behaviour.COUNTMONEY
local setTopic = behaviour.setTopic
local setPrice = behaviour.setPrice
local profession = behaviour.profession
local deleteMoney = behaviour.deleteMoney
local idle = behaviour.idle
local nop = behaviour.nop
local previous = behaviour.previous

local rules = behaviour.rules {
	{{ADDRESS, "hello", "king", FORCE}, {"I greet thee, my loyal subject."}},
	{{ADDRESS, "hail", "king", FORCE}, {previous}},
	{{ADDRESS, "salutations", "king", FORCE}, {previous}},
	{{ADDRESS, FORCE}, {idle}},
	{{BUSY, FORCE}, {nop}},
	{{VANISH, FORCE}, {"What a lack of manners!"}},

	{{"bye"}, {"Good bye, %N!", idle}},
	{{"farewell"}, {previous}},
	{{"job"}, {"I am your sovereign, King Tibianus III, and it's my duty to provide justice and guidance for my subjects."}},
	{{"justice"}, {"I try my best to be just and fair to our citizens. The army and the TBI are a great help for fulfilling this duty."}},
	{{"name"}, {"It's hard to believe that you don't know your own king!"}},
	{{"news"}, {"The latest news are usually brought to our magnificent town by brave adventurers. They spread tales of their journeys at Frodo's tavern."}},
	{{"tibia"}, {"Soon the whole land will be ruled by me once again!"}},
	{{"land"}, {previous}},
	{{"how", "are", "you"}, {"Thank you, I'm fine."}},
	{{"castle"}, {"Rain Castle is my home."}},
	{{"sell"}, {"Sell? Sell what? My kingdom isn't for sale!"}},
	{{"god"}, {"Honor the gods and pay your taxes."}},
	{{"zathroth"}, {"Please ask a priest about the gods."}},
	{{"citizen"}, {"The citizens of Tibia are my subjects. Ask the old monk Quentin to learn more about them."}},
	{{"sam"}, {"He is a skilled blacksmith and a loyal subject."}},
	{{"frodo"}, {"He is the owner of Frodo's Hut and a faithful tax-payer."}},
	{{"gorn"}, {"He was once one of Tibia's greatest fighters. Now he is selling equipment."}},
	{{"benjamin"}, {"He was once my greatest general. Now he is very old and senile but we entrusted him with work for the Royal Tibia Mail."}},
	{{"harkath"}, {"Harkath Bloodblade is the general of our glorious army."}},
	{{"bloodblade"}, {previous}},
	{{"general"}, {previous}},
	{{"noodles"}, {"The royal poodle Noodles is my greatest treasure!"}},
	{{"ferumbras"}, {"He is a follower of the evil god Zathroth and responsible for many attacks on us. Kill him on sight!"}},
	{{"bozo"}, {"He is my royal jester and cheers me up now and then."}},
	{{"treasure"}, {"The royal poodle Noodles is my greatest treasure!"}},
	{{"monster"}, {"Go and hunt them! For king and country!"}},
	{{"help"}, {"Visit Quentin, the monk, for help."}},
	{{"quest"}, {"I will call for heroes as soon as the need arises again and then reward them appropriately."}},
	{{"mission"}, {previous}},
	{{"gold"}, {"To pay your taxes, visit the royal tax collector."}},
	{{"money"}, {previous}},
	{{"tax"}, {previous}},
	{{"sewer"}, {"What a disgusting topic!"}},
	{{"dungeon"}, {"Dungeons are no places for kings."}},
	{{"equipment"}, {"Feel free to buy it in our town's fine shops."}},
	{{"food"}, {"Ask the royal cook for some food."}},
	{{"time"}, {"It's a time for heroes, that's for sure!"}},
	{{"heroes"}, {previous}},
	{{"hero$"}, {previous}},
	{{"adventurer"}, {previous}},
	{{"tax", "collector"}, {"He has been lazy lately. I bet you have not payed any taxes at all."}},
	{{"king"}, {"I am the king, so mind your words!"}},
	{{"army"}, {"Ask the soldiers about that topic."}},
	{{"enemy"}, {"Our enemies are numerous. The evil minotaurs, Ferumbras, and the renegade city of Carlin to the north are just some of them."}},
	{{"enemies"}, {previous}},
	{{"city", "north"}, {"They dare to reject my reign over the whole continent!"}},
	{{"carlin"}, {previous}},
	{{"thais"}, {"Our beloved city has some fine shops, guildhouses, and a modern system of sewers."}},
	{{"city"}, {previous}},
	{{"shop"}, {"Visit the shops of our merchants and craftsmen."}},
	{{"merchant"}, {"Ask around about them."}},
	{{"craftsmen"}, {previous}},
	{{"guild"}, {"The four major guilds are the knights, the paladins, the druids, and the sorcerers."}},
	{{"minotaur"}, {"Vile monsters, but I must admit they are strong and sometimes even cunning ... in their own bestial way."}},
	{{"paladin"}, {"The paladins are great protectors for Thais."}},
	{{"elane"}, {previous}},
	{{"knight"}, {"The brave knights are necessary for human survival in Thais."}},
	{{"gregor"}, {previous}},
	{{"sorcerer"}, {"The magic of the sorcerers is a powerful tool to smite our enemies."}},
	{{"muriel"}, {previous}},
	{{"druid"}, {"We need the druidic healing powers to fight evil."}},
	{{"marvik"}, {previous}},
	{{"good"}, {"The forces of good are hard pressed in these dark times."}},
	{{"evil"}, {"We need all strength we can muster to smite evil!"}},
	{{"order"}, {"We need order to survive!"}},
	{{"chaos"}, {"Chaos arises from selfishness, and that's its weakness."}},
	{{"excalibug"}, {"It's the sword of the kings. If you could return this weapon to me I would reward you beyond your dreams."}},
	{{"reward"}, {"Well, if you want a reward, go on a quest to bring me Excalibug!"}},
	{{"chester"}, {"A very competent person. A little nervous but very competent."}},
	{{"tbi$"}, {"This organisation is important in holding our enemies in check. Its headquarter is located in the bastion in the northwall."}},

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
