-- npcs/aruda.lua: the behaviour of the NPC Aruda, converted from
-- aruda.npc by patella-convert. The NPC is its [[npc]] entry in npcs.toml;
-- lib/npc.lua matches and runs the rules below, in the order of the file
-- (scripting.hh, "NPCs"). The comments are those of aruda.npc.
--
-- GIMUD - Graphical Interface Multi User Dungeon
-- aruda.npc: Datenbank fuer die Diebin Aruda

local behaviour = require("lib.npc")
local ADDRESS = behaviour.ADDRESS
local BUSY = behaviour.BUSY
local VANISH = behaviour.VANISH
local MALE = behaviour.MALE
local FEMALE = behaviour.FEMALE
local FORCE = behaviour.FORCE
local eq = behaviour.eq
local lt = behaviour.lt
local ge = behaviour.ge
local TOPIC = behaviour.TOPIC
local PRICE = behaviour.PRICE
local AMOUNT = behaviour.AMOUNT
local TYPE = behaviour.TYPE
local COUNTMONEY = behaviour.COUNTMONEY
local count = behaviour.count
local setTopic = behaviour.setTopic
local setPrice = behaviour.setPrice
local setAmount = behaviour.setAmount
local setType = behaviour.setType
local delete = behaviour.delete
local deleteMoney = behaviour.deleteMoney
local queue = behaviour.queue
local idle = behaviour.idle
local nop = behaviour.nop
local previous = behaviour.previous

local rules = behaviour.rules {
	{{ADDRESS, "hello$", MALE, FORCE}, {"Oh, hello, handsome! It's a pleasure to meet you %N."}},
	{{ADDRESS, "hi$", MALE, FORCE}, {previous}},
	{{ADDRESS, "hello$", FEMALE, FORCE}, {"Oh, hello %N, your hair looks great! Who did it for you?", setTopic(1)}},
	{{ADDRESS, "hi$", FEMALE, FORCE}, {previous}},
	{{ADDRESS, FORCE}, {idle}},
	{{BUSY, "hi$", FORCE}, {"Please be nice and wait a minute. I'll be right with you %N.", queue}},
	{{BUSY, "hello$", FORCE}, {previous}},
	{{BUSY, FORCE}, {nop}},
	{{VANISH, FORCE}, {"I hope to see you soon."}},

	{{"bye"}, {"Good bye. I really hope we'll talk again soon.", idle}},
	{{"farewell"}, {previous}},
	{{"how", "are", "you"}, {"Thank you very much. How kind of you to care about me. I am fine, thank you.", setPrice(5), setTopic(2)}},
	{{"sell"}, {"Sorry, I have nothing to sell.", setPrice(5), setTopic(2)}},
	{{"job"}, {"I do some work now and then. Nothing unusual, though.", setPrice(5), setTopic(2)}},
	{{"news"}, {"You should ask Oswald about news. He loves them."}},
	{{"name", MALE}, {"I am a little sad, that you seem to have forgotten me, handsome. I am Aruda.", setPrice(5), setTopic(2)}},
	{{"name", FEMALE}, {"I am Aruda.", setPrice(5), setTopic(2)}},
	{{"aruda", MALE}, {"Oh, I like it, how you say my name.", setPrice(5), setTopic(2)}},
	{{"aruda", FEMALE}, {"Yes, that's me!", setPrice(5), setTopic(2)}},
	{{"time"}, {setType(2906), setAmount(1), "Please don't be so rude to look for the time if you are talking to me.", setTopic(3)}},
	{{"help"}, {"I am deeply sorry, I can't help you.", setPrice(5), setTopic(2)}},
	{{"monster"}, {"UH! What a terrifying topic. Please let us speak about something more pleasant, I am a weak and small woman after all.", setPrice(5), setTopic(2)}},
	{{"dungeon"}, {previous}},
	{{"sewer"}, {"What gives you the impression, I am the kind of women, you find in sewers?", setPrice(5), setTopic(2)}},
	{{"god"}, {"You should ask about that in one of the temples.", setPrice(5), setTopic(2)}},
	{{"king"}, {"The king, that lives in this fascinating castle? I think he does look kind of cute in his luxurious robes, doesn't he?", setPrice(10), setTopic(2)}},
	{{"sam", MALE}, {"He is soooo strong! What muscles! What a body! On the other hand, compared to you he looks quite puny.", setPrice(5), setTopic(2)}},
	{{"sam"}, {"He is soooo strong! What muscles! What a body! Did you ask him for a date?", setPrice(5), setTopic(2)}},
	{{"benjamin"}, {"He is a little simple minded but always nice and well dressed.", setPrice(5), setTopic(2)}},
	{{"gorn"}, {"He should really sell some stylish gowns or something like that. We Tibians never get some clothing of the latest fashion. It's a shame.", setPrice(5), setTopic(2)}},
	{{"quentin"}, {"I don't understand this lonely monks. I love company too much to become one. He, he, he!", setPrice(5), setTopic(2)}},
	{{"bozo"}, {"Oh, isn't he funny? I could listen to him the whole day.", setPrice(5), setTopic(2)}},
	{{"oswald"}, {"As far as I know, he is working in the castle."}},
	{{"rumour"}, {"I am a little shy and so don't hear many rumors", setPrice(5), setTopic(2)}},
	{{"rumor"}, {previous}},
	{{"gossip"}, {previous}},
	{{"fuck", MALE}, {"Oh, you little devil, stop talking like that! <blush>", setPrice(20), setTopic(2)}},
	{{"kiss", MALE}, {previous}},
	{{"fuck", FEMALE}, {"Uhm, let us change the subject, please.", setPrice(20), setTopic(2)}},
	{{"weapon"}, {"I know so little about weapons, so tell me something about weapons, please.", setPrice(5), setTopic(2)}},
	{{"magic"}, {"I believe that love is stronger then all magic, don't you agree?", setPrice(5), setTopic(2)}},
	{{"thief"}, {"Oh, sorry, I have to hurry, bye!", idle}},
	{{"theft"}, {previous}},
	{{"tibia"}, {"I would like to visit the beach more often, but I guess it's too dangerous.", setPrice(5), setTopic(2)}},
	{{"castle"}, {"I love this castle! It's so beautiful.", setPrice(5), setTopic(2)}},
	{{"muriel"}, {"Powerful sorcerers frighten me a little.", setPrice(5), setTopic(2)}},
	{{"elane"}, {"I personally think it's inappropriate for a woman to become a warrior, what do you think about that?", setPrice(5), setTopic(2)}},
	{{"marvik"}, {"Druids seldom visit a town, what do you know about druids?", setPrice(5), setTopic(2)}},
	{{"gregor"}, {"I like brave fighters like him.", setPrice(5), setTopic(2)}},
	{{"noodles"}, {"Oh, he is sooooo cute!", setPrice(5), setTopic(2)}},
	{{"dog"}, {"I like dogs, the little ones at least. Do you like dogs, too?", setPrice(5), setTopic(2)}},
	{{"poodle"}, {previous}},
	{{"excalibug"}, {"Oh, I am just a girl and know nothing about magic swords and such things.", setPrice(10), setTopic(2)}},
	{{"partos"}, {"I ... don't know someone named like that.", setTopic(4)}},
	{{"yenny"}, {"Yenny? I know no Yenny, nor have I ever used that name! You have mistook me with someone else.", idle}},

	{{eq(TOPIC, 1)}, {"I would never have guessed that."}},
	{{eq(TOPIC, 2), ge(COUNTMONEY, PRICE)}, {"Oh, sorry, I was distracted, what did you say?", deleteMoney}},
	{{eq(TOPIC, 2), lt(COUNTMONEY, PRICE)}, {"Oh, I just remember I have some work to do, sorry. Bye!", idle}},
	{{eq(TOPIC, 3), ge(count(TYPE), AMOUNT)}, {"Take some time to talk to me!", delete(TYPE)}},
	{{eq(TOPIC, 4), "spouse"}, {"Well ... I  might have known him a little .. but there was nothing serious.", setTopic(5)}},
	{{eq(TOPIC, 4), "girlfriend"}, {previous}},
	{{eq(TOPIC, 5), "fruit"}, {"I remember that grapes were his favourites. He was almost addicted to them."}},
}

function onCreatureSay(npc, creature, text, situation)
	behaviour.react(npc, rules, text, situation)
end

function onCreatureDisappear(npc, creature)
	behaviour.react(npc, rules, "", NPC_SITUATION_VANISH)
end
