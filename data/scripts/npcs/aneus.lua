-- npcs/aneus.lua: the behaviour of the NPC Aneus, converted from
-- aneus.npc by patella-convert. The NPC is its [[npc]] entry in npcs.toml;
-- lib/npc.lua matches and runs the rules below, in the order of the file
-- (scripting.hh, "NPCs"). The comments are those of aneus.npc.
--
-- GIMUD - Graphical Interface Multi User Dungeon
-- aneus.npc: Datenbank für den Geschichtenerzähler Aneus (Fields)

local behaviour = require("lib.npc")
local ADDRESS = behaviour.ADDRESS
local BUSY = behaviour.BUSY
local VANISH = behaviour.VANISH
local FORCE = behaviour.FORCE
local eq = behaviour.eq
local TOPIC = behaviour.TOPIC
local setTopic = behaviour.setTopic
local queue = behaviour.queue
local idle = behaviour.idle
local nop = behaviour.nop
local previous = behaviour.previous

local rules = behaviour.rules {
	{{ADDRESS, "hello$", FORCE}, {"Greetings adventurer %N. What leads you to me?"}},
	{{ADDRESS, "hi$", FORCE}, {previous}},
	{{ADDRESS, FORCE}, {idle}},
	{{BUSY, "hello$", FORCE}, {"One moment please, %N.", queue}},
	{{BUSY, "hi$", FORCE}, {previous}},
	{{BUSY, FORCE}, {nop}},
	{{VANISH, FORCE}, {"Good bye and take care!"}},

	{{"bye"}, {"Good bye and take care of you!", idle}},
	{{"farewell"}, {previous}},
	{{"name"}, {"My name is Aneus, the storyteller."}},
	{{"bruno"}, {"I don't know much about him. I only know that he is selling fish in the village."}},
	{{"marlene"}, {"A lovely woman. But I give you a hint: Better keep away from her. *grin*"}},
	{{"graubart"}, {"I don't know much about him. But he sails much and has seen nearly the whole world."}},
	{{"job"}, {"I'm a storyteller."}},
	{{"storyteller", FORCE}, {"Well, if you wish I can tell you the story about this place here. The story about the Fields of Glory!"}},

	{{"story", FORCE}, {"Ok, sit down and listen. Back in the early days, one of the ancestors of our king Tibianus III wanted to build the best city in whole of Tibia.", setTopic(2)}},
	{{"fields", "of", "glory", FORCE}, {"Ok, sit down and listen. Back in the early days, one of the ancestors of our king Tibianus III wanted to build the best city in whole of Tibia.", setTopic(2)}},

	{{eq(TOPIC, 2), "ancestor", FORCE}, {"Please forgive me. I forgot his name. I'm not that young anymore.", setTopic(2)}},
	{{eq(TOPIC, 2), "city", FORCE}, {"The works on this new city began and the king sent his best soldiers to protect the workers from orcs and to make them work harder.", setTopic(3)}},

	{{eq(TOPIC, 3), "soldier", FORCE}, {"It was the elite of the whole army. They were called the Red Legion (also known as the Bloody Legion).", setTopic(3)}},
	{{eq(TOPIC, 3), "orc"}, {"The orcs attacked the workers from time to time and so they disturbed the works on the city.", setTopic(4)}},
	{{eq(TOPIC, 3), "work", "harder", FORCE}, {"The soldiers treated them like slaves.", setTopic(4)}},

	{{eq(TOPIC, 4), "slave", FORCE}, {"You dont know what a slave is? I really hope that you will never have to make this experience.", setTopic(3)}},
	{{eq(TOPIC, 4), "works", FORCE}, {"The development of the city was fine. Also a giant castle was build northeast of the city. ...", "But more and more workers started to rebel because of the bad conditions.", setTopic(5)}},

	{{eq(TOPIC, 5), "rebel", FORCE}, {"All rebels were brought to the giant castle. Guarded by the Red Legion, they had to work and live in even worse conditions. ...", "Also some friends of the king's sister were brought there.", setTopic(6)}},

	{{eq(TOPIC, 6), "friends", FORCE}, {"The king's sister was pretty upset about the situation there but her brother didn't want to do anything about this matter. ...", "So she made a plan to destroy the Red Legion for their cruelty forever.", setTopic(7)}},

	{{eq(TOPIC, 7), "cruelty", FORCE}, {"The soldiers treated the workers like slaves.", setTopic(7)}},
	{{eq(TOPIC, 7), "plan", FORCE}, {"She ordered her loyal druids and hunters to disguise themselves as orcs from the near island and to attack the Red Legion by night over and over again.", setTopic(8)}},

	{{eq(TOPIC, 8), "island", FORCE}, {"The General of the Red Legion became very angry about these attacks and after some months he stroke back!", setTopic(9)}},
	{{eq(TOPIC, 8), "attack", FORCE}, {previous}},

	{{eq(TOPIC, 9), "stroke", FORCE}, {"Most of the Red Legion went to the island by night. The orcs were not prepared and the Red Legion killed hundreds of orcs with nearly no loss. ...", "After they were satisfied they walked back to the castle.", setTopic(10)}},

	{{eq(TOPIC, 10), "back", FORCE}, {"It is said that the orcish shamans cursed the Red Legion. ...", "Nobody knows. But one third of the soldiers died by a disease on the way back. ...", "And the orcs wanted to take revenge, and after some days they stroke back! ...", "The orcs and many allied cyclopses and minotaurs from all over Tibia came to avenge their friends, and they killed nearly all workers and soldiers in the castle. ...", "The help of the king's sister came too late.", setTopic(11)}},

	{{eq(TOPIC, 10), "walk", FORCE}, {"It is said that the orcish shamans cursed the Red Legion. ...", "Nobody knows. But one third of the soldiers died by a disease on the way back. ...", "And the orcs wanted to take revenge, and after some days they stroke back! ...", "The orcs and many allied cyclopses and minotaurs from all over Tibia came to avenge their friends, and they killed nearly all workers and soldiers in the castle. ...", "The help of the king's sister came too late.", setTopic(11)}},

	{{eq(TOPIC, 11), "help", FORCE}, {"She tried to rescue the workers but it was too late. The orcs started immediately to attack her troops, too. ...", "Her royal troops went back to the city. A trick saved the city from destruction.", setTopic(12)}},

	{{eq(TOPIC, 12), "trick"}, {"They used the same trick as against the Red Legion and the orcs started to fight their non-orcish-allies. ...", "After a bloody long fight the orcs went back to their cities. The city of Carlin was rescued. ...", "Since then, a woman has always been ruling over Carlin and this statue was made to remind us of their great tactics against the orcs and the Red Legion. ...", "So that was the story of Carlin and these Fields of Glory. I hope you liked it. *smiles*"}},

	{{eq(TOPIC, 12), "destruction"}, {"They used the same trick as against the Red Legion and the orcs started to fight their non-orcish-allies. ...", "After a bloody long fight the orcs went back to their cities. The city of Carlin was rescued. ...", "Since then, a woman has always been ruling over Carlin and this statue was made to remind us of their great tactics against the orcs and the Red Legion. ...", "So that was the story of Carlin and these Fields of Glory. I hope you liked it. *smiles*"}},
}

function onCreatureSay(npc, creature, text, situation)
	behaviour.react(npc, rules, text, situation)
end

function onCreatureDisappear(npc, creature)
	behaviour.react(npc, rules, "", NPC_SITUATION_VANISH)
end
