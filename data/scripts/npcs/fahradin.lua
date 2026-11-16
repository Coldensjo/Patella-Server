-- npcs/fahradin.lua: the behaviour of the NPC Fa'hradin, converted from
-- fahradin.npc by patella-convert. The NPC is its [[npc]] entry in npcs.toml;
-- lib/npc.lua matches and runs the rules below, in the order of the file
-- (scripting.hh, "NPCs"). The comments are those of fahradin.npc.
--
-- GIMUD - Graphical Interface Multi User Dungeon
-- fahradin.npc: Datenbank für den Maridzauberer Fa'hradin

local behaviour = require("lib.npc")
local ADDRESS = behaviour.ADDRESS
local BUSY = behaviour.BUSY
local VANISH = behaviour.VANISH
local FORCE = behaviour.FORCE
local eq = behaviour.eq
local lt = behaviour.lt
local gt = behaviour.gt
local TOPIC = behaviour.TOPIC
local count = behaviour.count
local questValue = behaviour.questValue
local setTopic = behaviour.setTopic
local setAmount = behaviour.setAmount
local setQuestValue = behaviour.setQuestValue
local delete = behaviour.delete
local queue = behaviour.queue
local idle = behaviour.idle
local nop = behaviour.nop
local previous = behaviour.previous

local rules = behaviour.rules {
	{{ADDRESS, "hello$", eq(questValue(278), 2), FORCE}, {"Aaaah... what have we here. A human - interesting. And such an ugly specimen, too... All right, human %N. How can I help you?"}},
	{{ADDRESS, "hi$", eq(questValue(278), 2), FORCE}, {previous}},
	{{ADDRESS, "greetings$", eq(questValue(278), 2), FORCE}, {previous}},
	{{ADDRESS, "djanni'hah$", eq(questValue(278), 2), FORCE}, {previous}},
	{{ADDRESS, FORCE}, {idle}},
	{{BUSY, "hello$", eq(questValue(278), 2), FORCE}, {"Wait human. I'll take care of you in a minute, %N.", queue}},
	{{BUSY, "hi$", eq(questValue(278), 2), FORCE}, {previous}},
	{{BUSY, "greetings$", eq(questValue(278), 2), FORCE}, {previous}},
	{{BUSY, "djanni'hah$", eq(questValue(278), 2), FORCE}, {previous}},
	{{BUSY, FORCE}, {nop}},
	{{VANISH, FORCE}, {"Farewell, human."}},

	{{"bye"}, {"Farewell, human. I will always remember you. Unless I forget you, of course.", idle}},
	{{"farewell"}, {previous}},
	{{"name"}, {"I am known as Fa'hradin."}},
	{{"fa'hradin"}, {"Yes, that is me. It seems you have heard my name before."}},
	{{"job"}, {"Well, you could say I am the wizard of the Marid. Of course, I know that all djinn are magical creatures. But let us put it this way: I am slightly better at wielding magic then your average djinn in the street."}},
	{{"djinn"}, {"Our race is in a deplorable state at the moment. However, it is interesting from a scientific point of view. I am really curious to see if the Efreet and the Marid are really going to develop into two completely different species..."}},
	{{"efreet"}, {"I have not be been able to figure out exactly why the Efreet have developed a different skin colour. ...", "This poses an interesting scientific problem, you know. Perhaps it is a magical effect, but I have a feeling that there are other forces at work here."}},
	{{"marid"}, {"That is what we call ourselves. We like to think of ourselves as the true inheritors of the djinn legacy."}},

	{{"gabel"}, {"He is our leader. He does have his mistakes, but then he always tries to do what he thinks is right, and I suppose that makes him a good leader."}},
	{{"king"}, {"Djinns do not have kings. Gabel has long abdicated the title because of his convictions, and Malor... Well, I suppose he would not refuse to take the crown, but I doubt he will ever get a chance to do so."}},
	{{"malor"}, {"That treacherous snake has been waiting for a chance to seize power for as long as I can remember. He and Gabel used to be as close as brothers, you know."}},
	{{"mal'ouquah", eq(questValue(281), 0)}, {"Mal'ouquah is Malor's fortress which lies to the south. I know it well even though I have never been there myself. Insider information, you know."}},
	{{"mal'ouquah", gt(questValue(281), 0)}, {"Mal'ouquah is Malor's fortress which lies to the south. Of course you cannot enter it through the front door. But there's also an unguarded back door in the north-west corner of the fortress..."}},
	{{"ashta'daramai"}, {"That is what this place is called. It is not difficult to guess that that name was not my idea."}},
	{{"human"}, {"You are a curious species: Weak, yet strong. Stupid, yet clever. Evil, yet good. Fascinating, really. ...", "For thousands of years we regarded the northern continent as barbaric and wild. ...", "And all of sudden there are roads and pastures and mighty cities. The problem with us djinn is that we always underestimate other species, especially humans."}},
	{{"zathroth"}, {"He created our race, but we find it hard to love him. Sometimes I think that whole war has erupted because there is something like a design flaw in us djinns, an inconsistency in the way we are. ...", "I have never been able to put my finger on it, but it keeps me wondering..."}},
	{{"tibia"}, {"Eons ago when I was still young I felt the world was a place of wonder and joy. Now all I see is a badly working system full of design flaws. ...", "Must have been the first world the gods have created. Who knows? Perhaps they have learnt from their mistakes, and they are creating a better world somewhere else?"}},
	{{"daraman"}, {"I have met him myself. He was a sharp thinker and a charismatic conversationalist. ...", "I suppose he never managed to convince me quite as thoroughly as he managed to convince Gabel, but I came to admire his amazing personal integrity. ...", "In the end I chose to follow his creed because I felt that we djinn lacked something, and I thought that perhaps Daraman had an answer."}},
	{{"darashia"}, {"Darashia is comparatively young. The local ruler managed to establish his own little caliphate thanks to the riches he accumulated. ...", "As long as he continues to control the eastern trade routes Darashia will continue to flourish."}},
	{{"scarab"}, {"An interesting species. Oh, they are as thick as two short planks, of course, but there is definitely something magic about them. ...", "I have not carried out many studies, however, because dissecting scarabs is a real hassle."}},
	{{"edron"}, {"I am not much of a traveller, but I would like to see the northern cities everbody is talking about. Perhaps one day I will do that. Oh, I will use some kind of magical disguise, of course."}},
	{{"thais"}, {previous}},
	{{"venore"}, {previous}},
	{{"kazordoon"}, {previous}},
	{{"carlin"}, {previous}},
	{{"ab'dendriel"}, {previous}},
	{{"ankrahmun"}, {"That is one of the oldest human settlements in the whole of Tibia. I understand it is currently ruled by the pharaoh - some sort of undead priest-king. I am sure that must be a charming fellow."}},
	{{"pharaoh"}, {"Apparently the whole issue of dying in order to extend the natural life span was his idea. Those humans. You never know what they come up with next!"}},

	{{"palace"}, {"The pharaoh's palace in Ankrahmun is an impressive building. At least that is how I remember it to be. ...", "I suppose it is a little less cheerful these days, with all that undead riff-raff roaming its halls."}},
	{{"ascension"}, {"A fundamental part of the pharaoh's cult. I have not studied it in any detail, though."}},
	{{"rah"}, {"Another cornerstone of the undead pharaoh's theological theories. I do not know much more about it, I'm afraid."}},
	{{"uthun"}, {previous}},
	{{"akh"}, {previous}},

	{{"kha'zeel"}, {"None of the mountains here has developed naturally. The whole range has been raised by using powerful magic, and a lot of this magic lingers to this very day. A great place to be a wizard, but a dangerous place to travel."}},
	{{"kha'labal"}, {"The Kha'labal used to be a paradise. I remember it well. The fact that it is a barren desert today might give you an idea of the things that happened during the war."}},
	{{"war"}, {"For a long time it seemed that the war was over for good. But now that Malor is free again he will surely kindle the flame of war again. ...", "Damn that foolish orc. If I manage to get hold of him he will be turned into something much worse than a slime."}},
	{{"melchior"}, {"Ah. I remember him. A trader, was he not? I haven't seen him for a long time."}},
	{{"alesar"}, {"That name brings up bad memories. I never really liked him, but you just had to admire his skills at the forge. His desertion was a great loss for our cause."}},
	{{"baa'leal"}, {"Oh, you just have to love that djinn. We have met on the battlefield on half a dozen occasions, and he lost each single one of these battles. ...", "To be sure, he is a great warrior, but he is also lousy general and a complete dunce. As long as he is Malor's commander-in-chief I think we're safe."}},
	{{"lamp"}, {"Ah yes. We djinn sleep in lamps. We have a natural ability to dematerialise, you see."}},
	{{"rata'mari"}, {"Ah yes. Have you seen him? One of my best works so far. Nobody will ever suspect he is in fact a transformed djinn. The only problem is I'm much better with transforming people into other forms than with transforming them back. Poor fellow."}},
	{{"fa'hradin", "lamp"}, {"I hate to flatter myself, but that lamp was a masterpiece. Malor would have been imprisoned in it for the rest of his miserable life if it had not been for that nincompoop who calls himself an orc king. That foolish troglodyte!"}},

	{{"work", lt(questValue(280), 2)}, {"Looking for work, are you? Well, it's very tempting, you know, but I'm afraid we do not really employ beginners. Perhaps our cook could need a helping hand in the kitchen."}},
	{{"mission", lt(questValue(280), 2)}, {previous}},

	{{"work", eq(questValue(280), 2), eq(questValue(281), 0)}, {"I have heard some good things about you from Bo'ques. But I don't know. ...", "Well, all right. I do have a job for you. ...", "In order to stay informed about our enemy's doings, we have managed to plant a spy in Mal'ouquah. ...", "He has kept the Efreet and Malor under surveillance for quite some time. ...", "But unfortunately, I have lost contact with him months ago. ...", "I do not fear for his safety because his cover is foolproof, but I cannot contact him either. This is where you come in. ...", "I need you to infiltrate Mal'ouqhah, contact our man there and get his latest spyreport. The password is PIEDPIPER. Remember it well! ...", "I do not have to add that this is a dangerous mission, do I? If you are discovered expect to be attacked! So good luck, human!", setQuestValue(281, 1)}},
	{{"bo'ques", eq(questValue(280), 2), eq(questValue(281), 0)}, {previous}},
	{{"mission", eq(questValue(280), 2), eq(questValue(281), 0)}, {previous}},

	{{"work", eq(questValue(281), 1)}, {"Did you already retrieve the spyreport?", setTopic(1)}},
	{{"mission", eq(questValue(281), 1)}, {previous}},
	{{"report", eq(questValue(281), 1)}, {previous}},
	{{"spy", eq(questValue(281), 1)}, {previous}},

	{{eq(TOPIC, 1), "yes", eq(count(3232), 1), FORCE}, {"You really have made it? You have the report? How come you did not get slaughtered? I must say I'm impressed. Your race will never cease to surprise me. ...", "Well, let's see. ...", "I think I need to talk to Gabel about this. I am sure he will know what to do. Perhaps you should have a word with him, too.", setAmount(1), delete(3232), setQuestValue(281, 2)}},
	{{eq(TOPIC, 1)}, {"Don't waste any more time. We need the spyreport of our man in Mal'ouquah as soon as possible! ...", "Also don't forget the password to contact our man: PIEDPIPER!"}},

	{{"work", eq(questValue(281), 2)}, {"Did you already talk to Gabel about the report? I think he will have further instructions for you."}},
	{{"mission", eq(questValue(281), 2)}, {previous}},
	{{"report", eq(questValue(281), 2)}, {previous}},
	{{"spy", eq(questValue(281), 2)}, {previous}},
}

function onCreatureSay(npc, creature, text, situation)
	behaviour.react(npc, rules, text, situation)
end

function onCreatureDisappear(npc, creature)
	behaviour.react(npc, rules, "", NPC_SITUATION_VANISH)
end
