-- npcs/riddler.lua: the behaviour of the NPC Riddler, converted from
-- riddler.npc by patella-convert. The NPC is its [[npc]] entry in npcs.toml;
-- lib/npc.lua matches and runs the rules below, in the order of the file
-- (scripting.hh, "NPCs"). The comments are those of riddler.npc.
--
-- GIMUD - Graphical Interface Multi User Dungeon
-- riddler.npc: Datenbank für den Rätselgeist Riddler
--ACHTUNG TOPIC 13 und 14 mittendrin benutzt, da nachträglich!!
--WICHTIG: Riddler MUSSS bei rätseln nach EINEM GANZEN WORT fragen, sonst geht cheat

local behaviour = require("lib.npc")
local ADDRESS = behaviour.ADDRESS
local BUSY = behaviour.BUSY
local VANISH = behaviour.VANISH
local FORCE = behaviour.FORCE
local eq = behaviour.eq
local ne = behaviour.ne
local TOPIC = behaviour.TOPIC
local questValue = behaviour.questValue
local setTopic = behaviour.setTopic
local setQuestValue = behaviour.setQuestValue
local effectOpp = behaviour.effectOpp
local queue = behaviour.queue
local idle = behaviour.idle
local nop = behaviour.nop
local teleport = behaviour.teleport
local previous = behaviour.previous

local rules = behaviour.rules {
	{{ADDRESS, "hello$", FORCE}, {"%N! HEHEHEHE! Another fool! Excellent!"}},
	{{ADDRESS, "hi$", FORCE}, {previous}},
	{{ADDRESS, FORCE}, {idle}},
	{{BUSY, "hello$", FORCE}, {"Wait %N, you'll die next!", queue}},
	{{BUSY, "hi$", FORCE}, {previous}},
	{{BUSY, FORCE}, {nop}},
	{{VANISH, FORCE}, {"COWARD! CHICKEN! HEHEHEHE!"}},

	{{"bye"}, {"HEHEHE! I knew you don't have the stomach.", idle}},
	{{"name"}, {"I am known as the riddler. That is all you need to know."}},
	{{"job"}, {"I am the guardian of the paradox tower."}},
	{{"time"}, {"It is the age of the talon."}},
	{{"tower"}, {"This tower, of course, silly one. It holds my master's treasure."}},
	{{"paradox"}, {previous}},
	{{"master"}, {"His name is none of your business."}},
	{{"guard"}, {"I am guarding the treasures of the tower. Only those who pass the test of the three sigils may pass."}},
	{{"treasure"}, {previous}},
	{{"test"}, {"Death awaits those who fail the test of the three seals! Do you really want me to test you?", setTopic(1)}},
	{{"sigil"}, {previous}},
	{{"key"}, {"The key of this tower! You will never find it! A malicious plant spirit is guarding it!"}},
	{{"door"}, {previous}},

	{{eq(TOPIC, 1), "yes", eq(questValue(212), 0)}, {"FOOL! Now you're doomed! But well ... So be it! Let's start out with the Seal of Knowledge and the first question: What name did the necromant king choose for himself?", setQuestValue(212, 1), setTopic(2)}},
	{{eq(TOPIC, 1), "yes", eq(questValue(212), 1)}, {"So you think you're smart! But well ... So be it! Let's start out with the Seal of Knowledge and the first question: What do I have in my pocket?", setTopic(14)}},
	{{eq(TOPIC, 1), "no"}, {"HEHEHE! I knew you wouldn't have the stomach.", idle}},

	{{eq(TOPIC, 14), FORCE}, {"WRONG!", idle, effectOpp(11), setQuestValue(212, 0), teleport(32725, 31589, 12), effectOpp(11)}},

	{{eq(TOPIC, 2), "goshnar", FORCE}, {"HOHO! You have learned your lesson well. Question number two then: Who or what is the feared Hugo?", setTopic(3)}},
	{{eq(TOPIC, 2), FORCE}, {"WRONG!", idle, effectOpp(11), setQuestValue(212, 0), teleport(32725, 31589, 12), effectOpp(11)}},

	{{eq(TOPIC, 3), eq(questValue(211), 4), "demonbunny", FORCE}, {"HOHO! Right again. All right. The final question of the first seal: Who was the first warrior to follow the path of the Mooh'Tah?", setTopic(4)}},
	{{eq(TOPIC, 3), ne(questValue(211), 4), "demonbunny", FORCE}, {"Hmmm, so you think cheating will get you through that test? Then your final question of the first seal is: What is the meaning of life?", setTopic(13)}},
	{{eq(TOPIC, 3), eq(questValue(211), 4), "demonrabbit", FORCE}, {"HOHO! Right again. All right. The final question of the first seal: Who was the first warrior to follow the path of the Mooh'Tah?", setTopic(4)}},
	{{eq(TOPIC, 3), ne(questValue(211), 4), "demonrabbit", FORCE}, {"Hmmm, so you think cheating will get you through that test? Then your final question of the first seal is: What is the meaning of life?", setTopic(13)}},
	{{eq(TOPIC, 3), FORCE}, {"WRONG!", idle, effectOpp(11), setQuestValue(212, 0), teleport(32725, 31589, 12), effectOpp(11)}},
	{{eq(TOPIC, 13), FORCE}, {"WRONG! Next time get your own answers. To hell with thee, cheater!", idle, effectOpp(11), setQuestValue(212, 0), teleport(32725, 31589, 12), effectOpp(11)}},

	{{eq(TOPIC, 4), "Tha'kull", FORCE}, {"HOHO! Lucky you. You have passed the first seal! So ... would you like to continue with the Seal of the Mind?", setTopic(5)}},
	{{eq(TOPIC, 4), FORCE}, {"WRONG!", idle, effectOpp(11), setQuestValue(212, 0), teleport(32725, 31589, 12), effectOpp(11)}},

	{{eq(TOPIC, 5), "yes"}, {"As you wish, foolish one! Here is my first question: Its lighter then a feather but no living creature can hold it for ten minutes?", setTopic(6)}},
	{{eq(TOPIC, 5)}, {"HEHEHE! I knew you don't have the stomach.", setQuestValue(212, 0), idle}},

	{{eq(TOPIC, 6), "breath", FORCE}, {"That was an easy one. Let's try the second: If you name it, you break it.", setTopic(7)}},
	{{eq(TOPIC, 6), FORCE}, {"WRONG!", idle, effectOpp(11), setQuestValue(212, 0), teleport(32725, 31589, 12), effectOpp(11)}},

	{{eq(TOPIC, 7), "silence", FORCE}, {"Hm. I bet you think you're smart. All right. How about this: What does everybody want to become but nobody to be?", setTopic(8)}},
	{{eq(TOPIC, 7), FORCE}, {"WRONG!", idle, effectOpp(11), setQuestValue(212, 0), teleport(32725, 31589, 12), effectOpp(11)}},

	{{eq(TOPIC, 8), "old", FORCE}, {"ARGH! You did it again! Well all right. Do you wish to break the Seal of Madness?", setTopic(9)}},
	{{eq(TOPIC, 8), FORCE}, {"WRONG!", idle, effectOpp(11), setQuestValue(212, 0), teleport(32725, 31589, 12), effectOpp(11)}},

	{{eq(TOPIC, 9), "yes"}, {"GOOD! So I will get you at last. Answer this: What is your favourite colour?", setTopic(10)}},
	{{eq(TOPIC, 9)}, {"HEHEHE! I knew you don't have the stomach.", setQuestValue(212, 0), idle}},

	{{eq(TOPIC, 10), "red", FORCE}, {"UHM UH OH ... How could you guess that? Are you mad??? All right. Question number two: What is the opposite?", setQuestValue(2, 1), setTopic(11)}},
	{{eq(TOPIC, 10), "blue", FORCE}, {"UHM UH OH ... How could you guess that? Are you mad??? All right. Question number two: What is the opposite?", setQuestValue(2, 2), setTopic(11)}},
	{{eq(TOPIC, 10), "black", FORCE}, {"UHM UH OH ... How could you guess that? Are you mad??? All right. Question number two: What is the opposite?", setQuestValue(2, 3), setTopic(11)}},
	{{eq(TOPIC, 10), "white", FORCE}, {"UHM UH OH ... How could you guess that? Are you mad??? All right. Question number two: What is the opposite?", setQuestValue(2, 4), setTopic(11)}},
	{{eq(TOPIC, 10), "orange", FORCE}, {"UHM UH OH ... How could you guess that? Are you mad??? All right. Question number two: What is the opposite?", setQuestValue(2, 5), setTopic(11)}},
	{{eq(TOPIC, 10), "green", FORCE}, {"UHM UH OH ... How could you guess that? Are you mad??? All right. Question number two: What is the opposite?", setQuestValue(2, 6), setTopic(11)}},
	{{eq(TOPIC, 10), "yellow", FORCE}, {"UHM UH OH ... How could you guess that? Are you mad??? All right. Question number two: What is the opposite?", setQuestValue(2, 7), setTopic(11)}},
	{{eq(TOPIC, 10), "brown", FORCE}, {"UHM UH OH ... How could you guess that? Are you mad??? All right. Question number two: What is the opposite?", setQuestValue(2, 8), setTopic(11)}},
	{{eq(TOPIC, 10), "violet", FORCE}, {"UHM UH OH ... How could you guess that? Are you mad??? All right. Question number two: What is the opposite?", setQuestValue(2, 9), setTopic(11)}},
	{{eq(TOPIC, 10), "pink", FORCE}, {"UHM UH OH ... How could you guess that? Are you mad??? All right. Question number two: What is the opposite?", setQuestValue(2, 10), setTopic(11)}},
	{{eq(TOPIC, 10), "silver", FORCE}, {"UHM UH OH ... How could you guess that? Are you mad??? All right. Question number two: What is the opposite?", setQuestValue(2, 11), setTopic(11)}},
	{{eq(TOPIC, 10), "gold", FORCE}, {"UHM UH OH ... How could you guess that? Are you mad??? All right. Question number two: What is the opposite?", setQuestValue(2, 12), setTopic(11)}},
	{{eq(TOPIC, 10), "grey", FORCE}, {"UHM UH OH ... How could you guess that? Are you mad??? All right. Question number two: What is the opposite?", setQuestValue(2, 13), setTopic(11)}},

	{{eq(TOPIC, 10), FORCE}, {"WRONG!", idle, effectOpp(11), setQuestValue(212, 0), teleport(32725, 31589, 12), effectOpp(11)}},

	{{eq(TOPIC, 11), "nothing", FORCE}, {"NO! NO! NO! That can't be true. You're not only mad, you are a complete idiot! Ah well. Here is the last question: What is 1 plus 1?", setTopic(12)}},
	{{eq(TOPIC, 11), "none", FORCE}, {previous}},
	{{eq(TOPIC, 11), FORCE}, {"SORRY I AM NOT ALLOWD TO HELP HEHEHE!", idle, effectOpp(11), setQuestValue(212, 0), teleport(32725, 31589, 12), effectOpp(11)}},

	{{eq(TOPIC, 12), eq(questValue(1), 1), "49$", FORCE}, {"DAMN YOUUUUUUUUUUUUUUUUUUUUUU!", idle, effectOpp(11), setQuestValue(212, 0), teleport(32478, 31905, 1), effectOpp(11)}},
	{{eq(TOPIC, 12), eq(questValue(1), 2), "94$", FORCE}, {"DAMN YOUUUUUUUUUUUUUUUUUUUUUU!", idle, effectOpp(11), setQuestValue(212, 0), teleport(32478, 31905, 1), effectOpp(11)}},
	{{eq(TOPIC, 12), eq(questValue(1), 3), "13$", FORCE}, {"DAMN YOUUUUUUUUUUUUUUUUUUUUUU!", idle, effectOpp(11), setQuestValue(212, 0), teleport(32478, 31905, 1), effectOpp(11)}},
	{{eq(TOPIC, 12), eq(questValue(1), 4), "1$", FORCE}, {"DAMN YOUUUUUUUUUUUUUUUUUUUUUU!", idle, effectOpp(11), setQuestValue(212, 0), teleport(32478, 31905, 1), effectOpp(11)}},
	{{eq(TOPIC, 12), FORCE}, {"WRONG!", idle, effectOpp(11), setQuestValue(212, 0), teleport(32725, 31589, 12), effectOpp(11)}},
}

function onCreatureSay(npc, creature, text, situation)
	behaviour.react(npc, rules, text, situation)
end

function onCreatureDisappear(npc, creature)
	behaviour.react(npc, rules, "", NPC_SITUATION_VANISH)
end
