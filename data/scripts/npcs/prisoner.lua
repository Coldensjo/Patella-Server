-- npcs/prisoner.lua: the behaviour of the NPC A Prisoner, converted from
-- prisoner.npc by patella-convert. The NPC is its [[npc]] entry in npcs.toml;
-- lib/npc.lua matches and runs the rules below, in the order of the file
-- (scripting.hh, "NPCs"). The comments are those of prisoner.npc.
--
-- GIMUD - Graphical Interface Multi User Dungeon
-- prisoner.npc: Datenbank für den Prisoner alias Mad Mage

local behaviour = require("lib.npc")
local ADDRESS = behaviour.ADDRESS
local BUSY = behaviour.BUSY
local VANISH = behaviour.VANISH
local FORCE = behaviour.FORCE
local eq = behaviour.eq
local lt = behaviour.lt
local gt = behaviour.gt
local ge = behaviour.ge
local TOPIC = behaviour.TOPIC
local AMOUNT = behaviour.AMOUNT
local TYPE = behaviour.TYPE
local count = behaviour.count
local questValue = behaviour.questValue
local random = behaviour.random
local setTopic = behaviour.setTopic
local setPrice = behaviour.setPrice
local setAmount = behaviour.setAmount
local setType = behaviour.setType
local setData = behaviour.setData
local setQuestValue = behaviour.setQuestValue
local create = behaviour.create
local delete = behaviour.delete
local createMoney = behaviour.createMoney
local idle = behaviour.idle
local nop = behaviour.nop
local previous = behaviour.previous

local rules = behaviour.rules {
	{{ADDRESS, "hello$", FORCE}, {"Huh? What? I can see! Wow! A non-mino. Did they capture you as well?"}},
	{{ADDRESS, "hi$", FORCE}, {previous}},
	{{ADDRESS, FORCE}, {idle}},
	{{BUSY, "hello$", FORCE}, {"Not much space right here. Hehe. I haven\180t had a visitor for some time and right now there are two! Hehehe! Great!"}},
	{{BUSY, "hi$", FORCE}, {previous}},
	{{BUSY, FORCE}, {nop}},
	{{VANISH, gt(questValue(1), 0), FORCE}, {"Good bye! Don't forget about the secrets of mathemagics."}},
	{{VANISH, eq(questValue(1), 0), FORCE}, {"Wait! Don't leave! I want to tell you about my surreal numbers."}},

	{{"bye"}, {"Next time we should talk about my surreal numbers.", idle}},
	{{"farewell"}, {previous}},
	{{"job"}, {"Job? JOB? Hey man - I am in prison! But you know - once upon a time - I was a powerful mage! A mage ... come to think of it .., what is that - a mage?"}},
	{{"name"}, {"My name is - uhm - hang on? I knew it yesterday, didn't I? Doesn't matter!"}},
	{{"time"}, {"Better save time than comitting a crime. I am a poet and I know it!"}},
	{{"sorcerer"}, {"I am the mightiest sorcerer from here to there! Yeah!"}},
	{{"power"}, {"Power. Hmmm. Once while we were crossing the mountains together a man named Aureus said to me that parcels are equal to power. Any idea what that meant?"}},
	{{"books"}, {"I have many books in my home. But only powerful people can read them. I bet you will only see three dots after the headline! Hehehe! Hahaha! Excellent!"}},
	{{"mad", "mage"}, {"Hey! That's me! You got it! Thanks mate - now I remember my name!"}},
	{{"riddle"}, {"Great riddle, isn\180t it? If you can tell me the correct answer, I will give you something. Hehehe!"}},
	{{"something"}, {"No! I won\180t tell you. Shame coz it would be useful for you - hehehe."}},
	{{"apple"}, {setType(3585), setAmount(1), "Apples! Real apples! Man I love them! Can I have one? Oh please say yes!", setTopic(1)}},
	{{"escape"}, {"How could I escape? They only give me rotten food here. I can\180t regain my powers because I have no mana!"}},
	{{"key"}, {"Sure I have the key! Hehehe! Perhaps I will give it to you. IF you can solve my riddle."}},
	{{"mino"}, {"They are trying to capture me! Or hang on! Haven't they already captured me? Hmmm - I will have to think about this."}},
	{{"markwin"}, {"He is the worst of them all! He is the king of the minos! May he burn in hell!"}},
	{{"labyrinth"}, {"It\180s easy to find your way through it! Just follow the pools of mud. Hehe - useful hint, isn\180t it?"}},
	{{"way"}, {previous}},
	{{"palkar"}, {"He is the leader of the outcasts. I hope he will never conquer the city of Mintwallin. That would be the end of me!"}},
	{{"karl"}, {"Tataah!"}},
	{{"demon"}, {"The only monster I cannot conjure. But soon I will be powerful enough!"}},
	{{"monster"}, {"Yeah! There are many monsters guarding my home. Only the bravest hero will be able to slay them!"}},
	{{"conjure"}, {previous}},
	{{"home"}, {previous}},

	{{"number", lt(questValue(1), 1)}, {"My surreal numbers are based on astonishing facts. Are you interested in learning the secret of mathemagics?", setTopic(7), setAmount(random(1, 4))}},
	{{"math", lt(questValue(1), 1)}, {previous}},
	{{"1+1$", lt(questValue(1), 1)}, {previous}},
	{{"1$", "+$", "1$", lt(questValue(1), 1)}, {previous}},
	{{"1$", "plus", "1$", lt(questValue(1), 1)}, {previous}},
	{{"one", "plus", "one", lt(questValue(1), 1)}, {previous}},

	{{"number", gt(questValue(1), 0)}, {"You already know the secrets of mathemagics! Now go and use them to learn."}},
	{{"math", gt(questValue(1), 0)}, {previous}},
	{{"1+1$", gt(questValue(1), 0)}, {previous}},
	{{"1$", "+$", "1$", gt(questValue(1), 0)}, {previous}},
	{{"1$", "plus", "1$", gt(questValue(1), 0)}, {previous}},
	{{"one", "plus", "one", gt(questValue(1), 0)}, {previous}},
	{{eq(TOPIC, 7), "yes"}, {"But first tell me your favourite colour please!", setTopic(8)}},
	{{eq(TOPIC, 8), eq(questValue(2), 1), "red", FORCE}, {"Very interesting. So are you ready to proceed in you lesson in mathemagics?", setTopic(9)}},
	{{eq(TOPIC, 8), eq(questValue(2), 2), "blue", FORCE}, {previous}},
	{{eq(TOPIC, 8), eq(questValue(2), 3), "black", FORCE}, {previous}},
	{{eq(TOPIC, 8), eq(questValue(2), 4), "white", FORCE}, {previous}},
	{{eq(TOPIC, 8), eq(questValue(2), 5), "orange", FORCE}, {previous}},
	{{eq(TOPIC, 8), eq(questValue(2), 6), "green", FORCE}, {previous}},
	{{eq(TOPIC, 8), eq(questValue(2), 7), "yellow", FORCE}, {previous}},
	{{eq(TOPIC, 8), eq(questValue(2), 8), "brown", FORCE}, {previous}},
	{{eq(TOPIC, 8), eq(questValue(2), 9), "violet", FORCE}, {previous}},
	{{eq(TOPIC, 8), eq(questValue(2), 10), "pink", FORCE}, {previous}},
	{{eq(TOPIC, 8), eq(questValue(2), 11), "silver", FORCE}, {previous}},
	{{eq(TOPIC, 8), eq(questValue(2), 12), "gold", FORCE}, {previous}},
	{{eq(TOPIC, 8), eq(questValue(2), 13), "grey", FORCE}, {previous}},

	{{eq(TOPIC, 8), FORCE}, {"I think you are not in touch with yourself, come back if you have tuned in on your own feelings."}},

	{{eq(TOPIC, 9), "yes", eq(AMOUNT, 1)}, {"So know that everthing is based on the simple fact that 1 + 1 = 49!", setQuestValue(1, 1)}},
	{{eq(TOPIC, 9), "yes", eq(AMOUNT, 2)}, {"So know that everthing is based on the simple fact that 1 + 1 = 94!", setQuestValue(1, 2)}},
	{{eq(TOPIC, 9), "yes", eq(AMOUNT, 3)}, {"So know that everthing is based on the simple fact that 1 + 1 = 13!", setQuestValue(1, 3)}},
	{{eq(TOPIC, 9), "yes", eq(AMOUNT, 4)}, {"So know that everthing is based on the simple fact that 1 + 1 = 1!", setQuestValue(1, 4)}},

	{{"sell", "rune"}, {setType(3147), setAmount(1), setPrice(10), "You want to sell me blank runes! I will give you 50000 gold for each rune! Interested?", setTopic(2)}},

	{{"dp-d-ks-p-dp"}, {setType(3585), setAmount(7), "Hurray! For that I will give you my key for - hmm - let\180s say ... some apples. Interested?", setTopic(3)}},
	{{"dp-d-sk-p-dp"}, {setType(3585), setAmount(7), "Hurray! For that I will give you my key for - hmm - let\180s say ... some apples. Interested?", setTopic(3)}},
	{{"pd-d-ks-p-pd"}, {setType(3585), setAmount(7), "Hurray! For that I will give you my key for - hmm - let\180s say ... some apples. Interested?", setTopic(3)}},
	{{"pd-d-sk-p-pd"}, {setType(3585), setAmount(7), "Hurray! For that I will give you my key for - hmm - let\180s say ... some apples. Interested?", setTopic(3)}},
	{{"dp-p-ks-d-dp"}, {setType(3585), setAmount(7), "Hurray! For that I will give you my key for - hmm - let\180s say ... some apples. Interested?", setTopic(3)}},
	{{"dp-p-sk-d-dp"}, {setType(3585), setAmount(7), "Hurray! For that I will give you my key for - hmm - let\180s say ... some apples. Interested?", setTopic(3)}},
	{{"pd-p-ks-d-pd"}, {setType(3585), setAmount(7), "Hurray! For that I will give you my key for - hmm - let\180s say ... some apples. Interested?", setTopic(3)}},
	{{"pd-p-sk-d-pd"}, {setType(3585), setAmount(7), "Hurray! For that I will give you my key for - hmm - let\180s say ... some apples. Interested?", setTopic(3)}},

	{{eq(TOPIC, 1), "yes", ge(count(TYPE), AMOUNT)}, {"Mnjam. Excellent! Thanks, man!", delete(TYPE)}},
	{{eq(TOPIC, 1), "yes"}, {"Do you want to trick me? You don\180t have one lousy apple!"}},
	{{eq(TOPIC, 1)}, {"Ooooooooooo.<sniff>"}},

	{{eq(TOPIC, 2), "yes", ge(count(TYPE), AMOUNT)}, {"Ok. Take my money. I can summon new money anytime - hehehe.", delete(TYPE), createMoney}},
	{{eq(TOPIC, 2), "yes"}, {"You do not have one."}},
	{{eq(TOPIC, 2)}, {"Hmmmmm."}},

	{{eq(TOPIC, 3), "yes", ge(count(TYPE), AMOUNT)}, {"Mnjam - excellent apples. Now - about that key. You are sure want it?", delete(TYPE), setTopic(4)}},
	{{eq(TOPIC, 3), "yes"}, {"Get some more apples first!"}},
	{{eq(TOPIC, 3)}, {"Then go away!", idle}},

	{{eq(TOPIC, 4), "yes"}, {"Really, really?", setTopic(5)}},
	{{eq(TOPIC, 4)}, {"Then go away!", idle}},

	{{eq(TOPIC, 5), "yes"}, {"Really, really, really, really?", setTopic(6)}},
	{{eq(TOPIC, 5)}, {"Then go away!", idle}},

	{{eq(TOPIC, 6), "yes"}, {setType(2969), setData(3666), setAmount(1), "Then take it and get happy - or die, hehe.", create(TYPE)}},
	{{eq(TOPIC, 6)}, {"Then go away!", idle}},
}

function onCreatureSay(npc, creature, text, situation)
	behaviour.react(npc, rules, text, situation)
end

function onCreatureDisappear(npc, creature)
	behaviour.react(npc, rules, "", NPC_SITUATION_VANISH)
end
