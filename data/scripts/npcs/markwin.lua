-- npcs/markwin.lua: the behaviour of the NPC Markwin, converted from
-- markwin.npc by patella-convert. The NPC is its [[npc]] entry in npcs.toml;
-- lib/npc.lua matches and runs the rules below, in the order of the file
-- (scripting.hh, "NPCs"). The comments are those of markwin.npc.
--
-- GIMUD - Graphical Interface Multi User Dungeon
-- markwin.npc: Datenbank für den Minokönig Markwin

local behaviour = require("lib.npc")
local ADDRESS = behaviour.ADDRESS
local BUSY = behaviour.BUSY
local VANISH = behaviour.VANISH
local FORCE = behaviour.FORCE
local eq = behaviour.eq
local ge = behaviour.ge
local TOPIC = behaviour.TOPIC
local AMOUNT = behaviour.AMOUNT
local TYPE = behaviour.TYPE
local count = behaviour.count
local questValue = behaviour.questValue
local setTopic = behaviour.setTopic
local setAmount = behaviour.setAmount
local setType = behaviour.setType
local burning = behaviour.burning
local setQuestValue = behaviour.setQuestValue
local effectOpp = behaviour.effectOpp
local summon = behaviour.summon
local delete = behaviour.delete
local idle = behaviour.idle
local nop = behaviour.nop
local previous = behaviour.previous

local rules = behaviour.rules {
	{{ADDRESS, "hello$", eq(questValue(222), 0), FORCE}, {"No! The hornless have reached my city! BODYGUARDS TO ME!", setQuestValue(222, 1), summon(24), summon(24), summon(24), summon(24), summon(29), summon(29), summon(23), idle}},
	{{ADDRESS, "hi$", eq(questValue(222), 0), FORCE}, {previous}},
	{{ADDRESS, "hello$", eq(questValue(245), 2), FORCE}, {"Oh, it's you again. What do you want, hornless messenger?"}},
	{{ADDRESS, "hi$", eq(questValue(245), 2), FORCE}, {previous}},
	{{ADDRESS, "hello$", FORCE}, {"Well ... you defeated my guards! Now everything is over! I guess I will have to answer your questions now."}},
	{{ADDRESS, "hi$", FORCE}, {previous}},
	{{ADDRESS, FORCE}, {idle}},
	{{BUSY, "hello$", FORCE}, {"One more human. I hate them."}},
	{{BUSY, "hi$", FORCE}, {previous}},
	{{BUSY, FORCE}, {nop}},
	{{VANISH, FORCE}, {"Yes! Leave me alone. Vanish to dust."}},

	{{"bye", eq(questValue(245), 2)}, {"Hm ... good bye.", idle}},
	{{"farewell", eq(questValue(245), 2)}, {previous}},
	{{"bye"}, {"Go to hell! Burn!", burning(20, 20), effectOpp(6), idle}},
	{{"farewell"}, {previous}},

	{{"letter", eq(questValue(245), 1)}, {"A letter from my Moohmy?? Do you have a letter from my Moohmy to me?", setType(3220), setAmount(1), setTopic(1)}},
	{{eq(TOPIC, 1), "yes", ge(count(TYPE), AMOUNT)}, {"Uhm, well thank you, hornless beeing.", setQuestValue(245, 2), delete(3220)}},
	{{eq(TOPIC, 1), "yes"}, {"Don't mock the king of the minotaurs or you will regret that!"}},
	{{eq(TOPIC, 1)}, {"Uh? What??"}},

	{{"job"}, {"I am the king of all minotaurs. I have been the king for more than 320 years."}},
	{{"real"}, {"Yes, I am the real king. Palkar is the leader of the outcasts."}},
	{{"name"}, {"I am Markwin, the old and real king of this city."}},
	{{"time"}, {"Don't ask me such stupid questions. My time is over right now."}},
	{{"tibianus"}, {"I am the real king!"}},
	{{"king"}, {previous}},
	{{"outcast"}, {"Those are no minos any longer. They left the city and killed their brothers. And they stole the key to my secret lab."}},
	{{"mintwallin"}, {"The former glorious city lies in the dirt. It is my home. I founded it about 180 years ago, when we found this lovely place."}},
	{{"city"}, {previous}},
	{{"chronicle"}, {"I am one of the minotaurs that are able to write. So I wrote most of the history of my beloved city Mintwallin in some books."}},
	{{"books"}, {previous}},
	{{"prisoner"}, {"He is totally mad. I don't know how he could find the way through the labyrinth. I arrested him in the prison."}},
	{{"human"}, {"I hate them all. Minotaurs have no own spelling, so I used the speech of the humans. Once I was a prisoner of them. Since then I hate them - and since then I can speak and write in their language."}},
	{{"labyrinth"}, {"It protected us for a long time. There are lots of traps in it. And many long tunnels. There haven't been many foes that found their way through it. Only that prisoner once arrived."}},
	{{"kaplar"}, {"I really don't know what it means. But ALL minos say it! Terrible!"}},
	{{"secret", "lab"}, {"Hehe - you will never find a way to enter it. The outcast stole the key. You are too weak to conquer it. HARHARHAR."}},
	{{"enter"}, {"To enter the laboratory is pretty difficult."}},
	{{"enter", "lab"}, {"First of all you will need a second fellow to help you."}},
	{{"second", "fellow"}, {"Yeah - he has to step on a special tile and an entrance will appear at a very poisenous place!"}},
	{{"place"}, {"Na! You will have to find it yourself!"}},
	{{"second"}, {"After you entered the first area you will need the key from the outcasts."}},
	{{"minotaurs"}, {"My fellows all are minotaurs. It is my folk. I am the king of all minos."}},
	{{"minos"}, {previous}},
	{{"key"}, {"There are many keys. The outcast stole the key to our secret lab! They should burn!"}},
	{{"demon"}, {"He was the beginning of our end. He is mighty and powerful. He killed many brave minos and after his arrival we weren't able to go up to the surface."}},
	{{"light"}, {"I would like to see the light of the sun again, but you will probably kill me. Go away!"}},
	{{"sun"}, {previous}},
	{{"surface"}, {previous}},
	{{"palkar"}, {"He is the leader of the outcast. In former times he was my best warrior, but now he is my worst enemy."}},
	{{"riddle"}, {"Riddle? I don't know riddles!"}},
	{{"karl"}, {"The man who explored this part of the map first. Strange guy. He likes to be announced as hunter. I don't like him. He is a human."}},
	{{"milk"}, {"No! I won't tell you the powers of our milk!"}},
}

function onCreatureSay(npc, creature, text, situation)
	behaviour.react(npc, rules, text, situation)
end

function onCreatureDisappear(npc, creature)
	behaviour.react(npc, rules, "", NPC_SITUATION_VANISH)
end
