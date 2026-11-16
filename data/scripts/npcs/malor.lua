-- npcs/malor.lua: the behaviour of the NPC Malor, converted from
-- malor.npc by patella-convert. The NPC is its [[npc]] entry in npcs.toml;
-- lib/npc.lua matches and runs the rules below, in the order of the file
-- (scripting.hh, "NPCs"). The comments are those of malor.npc.
--
-- GIMUD - Graphical Interface Multi User Dungeon
-- malor.npc: Datenbank für den Efreetkönig Malor

local behaviour = require("lib.npc")
local ADDRESS = behaviour.ADDRESS
local BUSY = behaviour.BUSY
local VANISH = behaviour.VANISH
local FORCE = behaviour.FORCE
local eq = behaviour.eq
local lt = behaviour.lt
local TOPIC = behaviour.TOPIC
local questValue = behaviour.questValue
local setTopic = behaviour.setTopic
local setQuestValue = behaviour.setQuestValue
local queue = behaviour.queue
local idle = behaviour.idle
local nop = behaviour.nop
local previous = behaviour.previous

local rules = behaviour.rules {
	{{ADDRESS, "hello$", eq(questValue(278), 3), FORCE}, {"Greetings, human %N. My patience with your kind is limited, so speak quickly and choose your words well."}},
	{{ADDRESS, "hi$", eq(questValue(278), 3), FORCE}, {previous}},
	{{ADDRESS, "greetings$", eq(questValue(278), 3), FORCE}, {previous}},
	{{ADDRESS, "djanni'hah$", eq(questValue(278), 3), FORCE}, {previous}},
	{{ADDRESS, FORCE}, {idle}},
	{{BUSY, "hello$", eq(questValue(278), 3), FORCE}, {"It might have escaped your limited human perception, but I am already talking to somebody else.", queue}},
	{{BUSY, "hi$", eq(questValue(278), 3), FORCE}, {previous}},
	{{BUSY, "greetings$", eq(questValue(278), 3), FORCE}, {previous}},
	{{BUSY, "djanni'hah$", eq(questValue(278), 3), FORCE}, {previous}},
	{{BUSY, FORCE}, {nop}},
	{{VANISH}, {"Farewell, human."}},

	{{"bye"}, {"Farewell, human. When I have taken my rightful place I shall remember those who served me well. Even if they are only humans.", idle}},
	{{"farewell"}, {previous}},

	{{"name"}, {"Is it true you don't know who I am? Well, then listen. My name is Malor. ...", "You should better memorise that name because you are bound to hear it more often in future."}},
	{{"job"}, {"I am the true leader of all djinn - perhaps not by birth, but certainly by merit. One day all djinn will come to recognise that I alone deserve to be king."}},
	{{"king"}, {"I may not have reached my goal yet, but neither has that accursed Gabel. As long as the Marid and Efreet are disunited neither of us can call himself the king of all djinn."}},
	{{"djinn"}, {"We are strong and proud. One day we will take our rightful place on the throne of creation, and your vulgar race will either serve us or perish. ...", "Nothing personal, human. It is a natural process. And you humans will find that the djinn can be just masters."}},
	{{"gabel"}, {"That fool. He thought he'd got rid of me for good. But I'm back, and this time I will finish what I have begun. That weak-willed wimp has held on to power far too long."}},
	{{"efreet"}, {"We are djinn! The true djinn! Those who have not let themselves be fooled by the silver-tongued blathering of that perfidious snake called Daraman."}},
	{{"marid"}, {"The so-called Marid have forgotten what it is like to be djinn! They are weak!"}},
	{{"malor"}, {"That is me. I was away for a long time, but now I am back with a vengeance."}},
	{{"daraman"}, {"Of all human liars and schemers he was the worst. This self-styled prophet single-handedly managed to disunite my race and to spark a bloody civil war. ...", "If somebody fulfilled a wish for me for a change I would bring him back to life and make him pay."}},
	{{"human"}, {"Your race is weak, but incurably treacherous. I will never forgive humanity the fact that it was one of your kind who spread the seed of dissent among the djinn."}},

	{{"mal'ouquah"}, {"Do you like this place? I have built Mal'ouquah as a home for those among my kind who did not fall for Daraman's sugar covered lies. From here I shall rule the world when the time has come."}},
	{{"ashta'daramai"}, {"Ashta'daramai is the fortress of our sworn enemies, the oh so powerful Marid. The day will come when I see its smouldering walls collapse."}},
	{{"orc", "king"}, {"Ah yes. My good old friend the foolish orc. He has rendered me a great service, you know? He released me from that accursed lamp! ...", "In return I have fulfilled three wishes for him, but somehow I can't help the feeling that he is not wonderfully happy about the way things have turned out."}},
	{{"zathroth"}, {"Our father. He made us a race of masters, not of servants. We will live to fulfill his promise or die trying."}},
	{{"gods"}, {"Are not the creators reflected in their creations? Look around! What do you see? There is nothing but cowardice and treachery in the world of humans. ...", "How low the gods must be who made them. I have no respect for them."}},
	{{"tibia"}, {"The world of Tibia is ours by right. I will not rest until we have conquered it."}},
	{{"darashia"}, {"Darashia is a very rich city. Once this war is won I will drop by at the Caliph's palace and pay my respects, if you know what I mean."}},
	{{"scarab"}, {"Scarabs are ancient creatures, which is why I respect them. But I will never allow any of these critters to undermine the foundations of my fortress."}},
	{{"edron"}, {"I hear the humans have built impressive cities on the great continent. It looks like many things have changed while I was caught in that stupid lamp."}},
	{{"thais"}, {previous}},
	{{"venore"}, {previous}},
	{{"kazordoon"}, {previous}},
	{{"carlin"}, {previous}},
	{{"ab'dendriel"}, {previous}},
	{{"ankrahmun"}, {"Even though it always was a human settlement I have always had a soft spot for the place. I am even thinking about making it my capital once I have taken over the world."}},
	{{"pharaoh"}, {"I have heard that pompous pharaoh believes himself to be some sort of deity. That pathetic bonehead a god? Don't make me laugh!"}},
	{{"ascension"}, {"Ascension? That does ring a bell. Isn't that an element of the pharaoh's doctrines."}},
	{{"rah"}, {"Another one of that loony pharaoh's bright ideas. Nothing but nonsense and balderdash."}},
	{{"uthun"}, {previous}},
	{{"akh"}, {previous}},

	{{"kha'zeel"}, {"They say the Kha'zeel mountains have been made by gods. If that is true they must have left long ago, because I have lived here for eons, and I have never met one of them."}},
	{{"kha'labal"}, {"Kha'labal. I do not like that desert. Looking at it brings up bad memories."}},
	{{"war"}, {"Gabel and Fa'hradin thought the war was over when they managed to trap me in that accursed lamp. But they have been a bit rash. After all those years I'm still here, and my thirst for revenge is stronger than ever!"}},
	{{"melchior"}, {"Melchior! Hah, that fool! Is he still alive? I never thought the old wretch would make it after I gave him my special treatment and sent him out into the Kha'labal. ...", "Amazing, really. It has often occurred to me how much humans resemble rats - they are just as hard to kill!"}},
	{{"baa'leal"}, {"I suppose you have met Baa'leal already? The fact that you have survived that encounter shows that you are surprisingly strong for a human. ...", "I almost feel some respect for you... Well, almost."}},
	{{"alesar"}, {"Oh yes, Alesar! I bet Gabel went mad when he learnt that Alesar switched sides. If only I had been there to watch his face."}},
	{{"fa'hradin"}, {"Fa'hradin is Gabel's lieutenant. I have known him for a long time, and I have always respected him. ...", "Unfortunately he chose the wrong side when the time to chose sides came. I have not given up hope of winning him over for some reason, but if I meet him on the battlefield I will not hesitate to kill him myself."}},
	{{"lamp"}, {"We djinn use them to sleep."}},

	{{"permission", lt(questValue(288), 3)}, {"I have no reason to give you my permission to trade with Alesar or Yaman."}},
	{{"permission", eq(questValue(288), 3)}, {"You are welcome to trade with Alesar and Yaman whenever you want to, %N!"}},

	{{"work", lt(questValue(287), 3)}, {"So you would like to fight for us. Hmm. ...", "You show true courage, human, but I will not accept your offer at this point of time."}},
	{{"mission", lt(questValue(287), 3)}, {previous}},

	{{"alesar", eq(questValue(287), 3), eq(questValue(288), 0)}, {"I guess this is the first time I entrust a human with a mission. And such an important mission, too. But well, we live in hard times, and I am a bit short of adequate staff. ...", "Besides, Baa'leal told me you have distinguished yourself well in previous missions, so I think you might be the right person for the job. ...", "But think carefully, human, for this mission will bring you close to certain death. Are you prepared to embark on this mission?", setTopic(1)}},
	{{"work", eq(questValue(287), 3), eq(questValue(288), 0)}, {previous}},
	{{"mission", eq(questValue(287), 3), eq(questValue(288), 0)}, {previous}},

	{{"work", eq(questValue(288), 1)}, {"You haven't finished your final mission yet. Shall I explain it again to you?", setTopic(1)}},
	{{"mission", eq(questValue(288), 1)}, {previous}},
	{{"lamp", eq(questValue(288), 1)}, {previous}},

	{{eq(TOPIC, 1), "yes"}, {"Well, listen. We are trying to acquire the ultimate weapon to defeat Gabel: Fa'hradin's lamp! ...", "At the moment it is still in the possession of that good old friend of mine, the Orc King, who kindly released me from it. ...", "However, for some reason he is not as friendly as he used to be. You better watch out, human, because I don't think you will get the lamp without a fight. ...", "Once you have found the lamp you must enter Ashta'daramai again. Sneak into Gabel's personal chambers and exchange his sleeping lamp with Fa'hradin's lamp! ...", "If you succeed, the war could be over one night later!", setQuestValue(288, 1)}},
	{{eq(TOPIC, 1)}, {"Your choice."}},

	{{"work", eq(questValue(288), 2)}, {"Have you found Fa'hradin's lamp and placed it in Malor's personal chambers? ", setTopic(2)}},
	{{"mission", eq(questValue(288), 2)}, {previous}},
	{{"lamp", eq(questValue(288), 2)}, {previous}},

	{{eq(TOPIC, 2), "yes"}, {"Well well, human. So you really have made it - you have smuggled the modified lamp into Gabel's bedroom! ...", "I never thought I would say this to a human, but I must confess I am impressed. ...", "Perhaps I have underestimated you and your kind after all. ...", "I guess I will take this as a lesson to keep in mind when I meet you on the battlefield. ...", "But that's in the future. For now, I will confine myself to give you the permission to trade with my people whenever you want to. ...", "Farewell, human!", setQuestValue(288, 3), idle}},
	{{eq(TOPIC, 2)}, {"Just do it!"}},
}

function onCreatureSay(npc, creature, text, situation)
	behaviour.react(npc, rules, text, situation)
end

function onCreatureDisappear(npc, creature)
	behaviour.react(npc, rules, "", NPC_SITUATION_VANISH)
end
