-- npcs/kevin.lua: the behaviour of the NPC Kevin, converted from
-- kevin.npc by patella-convert. The NPC is its [[npc]] entry in npcs.toml;
-- lib/npc.lua matches and runs the rules below, in the order of the file
-- (scripting.hh, "NPCs"). The comments are those of kevin.npc.
--
-- GIMUD - Graphical Interface Multi User Dungeon
-- kevin.npc: Datenbank für Kevin Postner den Anführer der Postlergilde

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
local add = behaviour.add
local sub = behaviour.sub
local setTopic = behaviour.setTopic
local setAmount = behaviour.setAmount
local setType = behaviour.setType
local setQuestValue = behaviour.setQuestValue
local create = behaviour.create
local delete = behaviour.delete
local idle = behaviour.idle
local nop = behaviour.nop
local previous = behaviour.previous

local rules = behaviour.rules {
	{{ADDRESS, "hello$", FORCE}, {"Greetings %N, what brings you here?"}},
	{{ADDRESS, "hi$", FORCE}, {previous}},

	{{ADDRESS, FORCE}, {idle}},
	{{BUSY, "hello$", FORCE}, {"Please wait a moment."}},
	{{BUSY, "hi$", FORCE}, {previous}},
	{{BUSY, FORCE}, {nop}},
	{{VANISH, FORCE}, {"How rude!"}},

	-- TOPICS BIS EINSCHL. 25 BENUTZT

	{{"bye"}, {"Good bye.", idle}},
	{{"farewell"}, {previous}},

	{{"name"}, {"My name is Kevin. Kevin Postner, that is."}},
	{{"job"}, {"I am the head of the Tibian Postmaster's Guild."}},
	{{"guild"}, {"We are a powerful organisation that is vital part of Tibian society. It's a honour and a privilege to be a member."}},
	{{"postmaster"}, {previous}},
	{{"academy"}, {"Ah, yes! One day I will found a postofficers academy. Perhaps with the help of our most able members. But rhats a task for another day."}},
	{{"wally"}, {"Wally is my right hand, so to say. He is the head to our guilds office branch."}},
	{{"branch"}, {"Well, thers the depot branch, the office brance, the delivery branch and some officers for special operations."}},
	{{"special", "operations", lt(questValue(250), 4)}, {"Sorry but I won't talk about this matter with someone of your rank."}},
	{{"special", "operations", gt(questValue(250), 3)}, {"We have a secret branch, called 'the stamps of the gods'. They secretly supervise our members reliability and have an eye on certain groups of interest that want to gain influence over our guild."}},

	{{"member", lt(questValue(227), 1)}, {"We have high standards for our members. To rise in our guild is a difficult but rewarding task. Why do you ask? Are you interested in joining?", setTopic(1)}},
	{{"member", gt(questValue(227), 0)}, {"You are already a member, %N."}},
	{{"mission", lt(questValue(227), 1)}, {"You are not a member of our guild yet! We have high standards for our members. To rise in our guild is a difficult but rewarding task. Are you interested in joining?", setTopic(1)}},

	{{"advancement", lt(questValue(250), questValue(249))}, {"You are worthy indeed. Do you want to advance in our guild?", setTopic(23)}},
	{{"advancement", ge(questValue(250), questValue(249))}, {"Sorry, but you are not yet ready for advancement."}},
	{{"yes", eq(TOPIC, 23), eq(questValue(249), 2)}, {"I grant you the title of postman. You are now a full member of our guild. Here have your own officers hat and wear it with pride.", setQuestValue(250, 2), setAmount(1), create(3576)}},
	{{"yes", eq(TOPIC, 23), eq(questValue(249), 3)}, {"From now on it shall be known that you are a grand postman. You are now a privileged member until the end of days. Most captains around the world have an agreement with our guild to transport our privileged members, like you, for less gold.", setQuestValue(250, 3)}},
	{{"yes", eq(TOPIC, 23), eq(questValue(249), 4)}, {"From now on you are a grand postman for special operations. You are an honoured member of our guild and earned the privilege of your own post horn. Here, take it.", setQuestValue(250, 4), setAmount(1), create(2957)}},
	{{"yes", eq(TOPIC, 23), eq(questValue(249), 5)}, {"I grant you the title of archpostman. You are a legend in our guild. As privilege of your newly aquired status you are allowed to make use of certain mailboxes in dangerous areas. Just look out for them and you'll see.", setQuestValue(250, 5)}},

	{{"mission", eq(questValue(248), 1), lt(questValue(250), questValue(249))}, {"Your eagerness is a virtue, young one, but first lets talk about advancement."}},

	{{"mission", eq(questValue(248), 1)}, {"You have mastered all our current missions. There's nothing left to be done ... for now."}},

	{{"mission", eq(questValue(245), 1)}, {"You are not done with your current mission. Deliver that letter to king Markwin. Please report back when you are ready."}},
	{{"mission", eq(questValue(245), 2)}, {"You have delivered that letter? You are a true postofficer. All over the land bards shall praise your name. There are no missions for you left right now.", setQuestValue(248, 1), setQuestValue(249, 5)}},

	{{"mission", eq(questValue(244), 1)}, {"You are not done with your current mission. Deliver those letters to Santa. Please report back when you are ready."}},
	{{"mission", eq(questValue(244), 2)}, {"You did it? I hope you did not catch a flu in the cold! However theres another mission for you. Are you interested?", setTopic(21)}},
	{{eq(TOPIC, 21), "yes"}, {"Excellent. Here is a letter for you to deliver. Well, to be honest, no one else volunteered. It's a letter from the mother of Markwin, the king of Mintwallin. Deliver that letter to him, but note that you will not be welcome there.", setQuestValue(245, 1), setAmount(1), create(3220)}},
	{{eq(TOPIC, 21)}, {"Too bad, perhaps another time then."}},

	{{"mission", eq(questValue(246), 1), eq(questValue(244), 0)}, {"So are you ready for another Mission?", setTopic(20)}},

	{{"mission", eq(questValue(243), 0), eq(questValue(242), 1)}, {"You are not done with your current mission. Search for the whereabout of Postofficer Waldo. Please report back when you are ready."}},
	{{"mission", eq(questValue(243), 1)}, {"So Waldo is dead? This is grave news indeed. Did you recover his posthorn?", setTopic(19), setType(3219), setAmount(1)}},
	{{eq(TOPIC, 19), "yes", ge(count(TYPE), AMOUNT)}, {"Thank you. We will honour this. Your next mission will be a very special one. Good thing you are a special person as well. Are you ready?", delete(TYPE), setQuestValue(246, 1), setQuestValue(249, 4), setTopic(20)}},
	{{eq(TOPIC, 19), "yes"}, {"Hm, no, you don't have it. Too bad, go and look for it."}},
	{{eq(TOPIC, 19)}, {"Too bad, go and look for it."}},

	{{eq(TOPIC, 20), "yes", lt(questValue(250), questValue(249))}, {"Your eagerness is a virtue, young one, but first lets talk about advancement."}},
	{{eq(TOPIC, 20), "yes"}, {"So listen well. Behind the lower left door you will find a bag. The letters in the bag are for none other than Santa Claus! Deliver them to his house on the isle of Vega, USE the bag on his mailbox and report back here.", setQuestValue(244, 1)}},
	{{eq(TOPIC, 20)}, {"Too bad, perhaps another time then."}},

	{{"mission", gt(questValue(234), 0), lt(questValue(234), 7)}, {"You are not done with your current mission. We still need the measurements of several postofficers. Please report back when you are ready."}},
	{{"mission", eq(questValue(234), 7)}, {"Once more you have impressed me! Are you willing to do another job?", setTopic(17)}},
	{{eq(TOPIC, 17), "yes"}, {"Ok but your next assignment might be dangerous. Our Courier Waldo has been missing for a while. I must assume he is dead. Can you follow me so far?", setTopic(18)}},
	{{eq(TOPIC, 17)}, {"Too bad, perhaps another time then."}},

	{{eq(TOPIC, 18), "yes"}, {"Find out about his whereabouts and retrieve him or at least his posthorn. He was looking for a new underground passage that is rumoured to be found underneath the troll-infested Mountain east of Thais.", setQuestValue(242, 1)}},
	{{eq(TOPIC, 18)}, {"Too bad, perhaps you will try some other time then."}},

	{{"mission", gt(questValue(233), 0), lt(questValue(233), 10)}, {"You are not done with your current mission. Make sure Hugo chief is tailoring our new uniforms. Please report back when you are ready."}},
	{{"mission", eq(questValue(233), 10)}, {"Excellent! Another job well done! Would you accept another mission?", setQuestValue(249, 3), setTopic(16)}},

	{{eq(TOPIC, 16), "yes", lt(questValue(250), questValue(249))}, {"Your eagerness is a virtue, young one, but first let's talk about advancement."}},
	{{eq(TOPIC, 16), "yes"}, {"Good, so listen. Hugo Chief informed me that he needs the measurements of our postofficers. Go and bring me the measurements of Ben, Lokur, Dove, Liane, Chrystal and Olrik.", setQuestValue(234, 1)}},
	{{eq(TOPIC, 16)}, {"Too bad, perhaps another time then."}},

	{{"dress", "pattern", eq(questValue(233), 8)}, {"Fine, fine. I think that should do it. Tell Hugo that we order those uniforms. The completed dress pattern will soon arrive in Venore. Report to me when you have talked to him.", setQuestValue(233, 9)}},

	{{"dress", "pattern", eq(questValue(233), 6)}, {"The queen has sent me the samples we needed. The next part is tricky. We need the uniforms to emanate some odor that dogs hate.The dog with the best 'taste' in that field is Noodles, the dog of King Tibianus. Do you understand so far?", setTopic(15), setAmount(random(1, 3))}},
	{{"uniform", eq(questValue(232), 6)}, {previous}},
	{{eq(TOPIC, 15), "yes"}, {"Good. Go there and find out what taste he dislikes most: moldy cheese, a piece of fur or a bananaskin. Tell him to SNIFF, then the object. Show him the object and ask 'Do you like that?'. DONT let the guards know what you are doing.", setQuestValue(233, 7), setQuestValue(251, AMOUNT)}},
	{{eq(TOPIC, 15)}, {"Too bad, perhaps you can try doing it some other time then."}},

	{{"dress", "pattern", eq(questValue(233), 2)}, {"Oh yes, where did we get that from ...? Let's see, first ask the great technomancer in Kazordoon for the technical details. Return here afterwards.", setQuestValue(233, 3)}},

	{{"dress", "pattern", eq(questValue(233), 4)}, {"The mail with Talphion's instructions just arrived. I remember we asked Queen Eloise of Carlin for the perfect colours. Go there, ask her about the UNIFORMS and report back here.", setQuestValue(233, 5)}},

	{{"mission", eq(questValue(231), 1)}, {"You are not done with your current mission. Deliver that present to Fibula. Please report back when you are ready."}},
	{{"mission", eq(questValue(231), 2)}, {"Splendid, I knew we could trust you. I would like to ask for your help in another matter. Are you interested?", setTopic(14)}},
	{{eq(TOPIC, 14), "yes"}, {"Ok. We need a new set of uniforms, and only the best will do for us. Please travel to Venore and negotiate with Hugo Chief a contract for new uniforms.", setQuestValue(233, 1)}},
	{{eq(TOPIC, 14)}, {"Too bad, perhaps another time then."}},

	-- BEFÖRDERUNG 2 
	{{eq(TOPIC, 13), "yes", lt(questValue(250), questValue(249))}, {"Your eagerness is a virtue, young one, but first lets talk about advancement."}},
	{{eq(TOPIC, 13), "yes"}, {"Since I am convinced I can trust you, this time you must deliver a valuable present to Dermot on Fibula. Do NOT open it!!! You will find the present behind the door here on the lower right side of this room.", setQuestValue(231, 1)}},

	{{eq(TOPIC, 13)}, {"Too bad, perhaps another time then."}},

	{{"mission", eq(questValue(230), 1)}, {setType(3115), setAmount(1), "Do you bring ONE bone for our officers' safety fund or ALL bones at once?", setTopic(24)}},
	{{"mission", gt(questValue(230), 1), lt(questValue(230), 20)}, {setType(3115), setAmount(1), "Do you bring a bone for our officers' safety fund?", setTopic(12)}},
	{{"mission", gt(questValue(230), 1), eq(questValue(230), 20)}, {setType(3115), setAmount(1), "Do you bring a bone for our officers' safety fund?", setTopic(22)}},
	{{"mission", gt(questValue(230), 20)}, {"You have made it! We have enough bones for the fund! You remind me of myself when I was young! Interested in another mission?", setTopic(13)}},

	{{eq(TOPIC, 12), "yes", ge(count(TYPE), AMOUNT)}, {delete(TYPE), setQuestValue(230, add(questValue(230), 1)), setAmount(sub(questValue(230), 1)), "Excellent! You have collected %A bones. Just report about your mission again if you find  more."}},
	{{eq(TOPIC, 12)}, {"You have no suitable bone with you. Too bad, but you surely will find some more."}},
	{{eq(TOPIC, 22), "yes", ge(count(TYPE), AMOUNT)}, {delete(TYPE), setQuestValue(230, add(questValue(230), 1)), setAmount(sub(questValue(230), 1)), "You have collected all the %A bones needed. Excellent! Now let's talk about further missions if you are interested.", setQuestValue(249, 2)}},
	{{eq(TOPIC, 22)}, {"You have no suitable bone with you. Too bad, but you surely will find some more."}},

	{{eq(TOPIC, 24), "one", ge(count(TYPE), AMOUNT)}, {delete(TYPE), setQuestValue(230, add(questValue(230), 1)), setAmount(sub(questValue(230), 1)), "Excellent! You have collected %A bones. Just report about your mission again if you find more."}},
	{{eq(TOPIC, 24), "all"}, {setType(3115), setAmount(20), "Are you sure you have collected all the 20 bones needed?", setTopic(25)}},

	{{eq(TOPIC, 25), "yes", ge(count(TYPE), AMOUNT)}, {delete(TYPE), setQuestValue(230, 21), "You have collected all the 20 bones needed. Excellent! Now let's talk about further missions if you are interested.", setQuestValue(249, 2)}},
	{{eq(TOPIC, 25)}, {"You have not enough bones with you. Too bad, but you surely will find some more."}},

	{{"mission", gt(questValue(229), 0), lt(questValue(229), 3)}, {"You are not done with your current mission. Find David Brassacres and hand him that bill. Report when you are ready."}},
	{{"mission", eq(questValue(229), 3)}, {"You truly got him? Quite impressive. You are a very prommising candidate! I think I have another mission for you. Are you interested?", setTopic(11)}},

	{{"mission", eq(questValue(228), 1)}, {"You are not done with your current mission. The mailbox is still not fixed. Report if you are ready."}},
	{{"mission", eq(questValue(228), 2)}, {"Excellent, you got it fixed! This will teach this mailbox a lesson indeed! Are you interested in another assignment?", setTopic(9)}},

	{{"mission", lt(questValue(227), 5)}, {"You are not done with your current mission. Make sure all the passages are secure. Report if you are ready."}},
	{{"mission", eq(questValue(227), 5)}, {"So you have finally made it! I did not think that you would have it in you ... However: are you ready for another assignment?", setTopic(8)}},

	{{eq(TOPIC, 11), "yes"}, {"Ok, listen: we have some serious trouble with agressive dogs lately. We have accumulated some bones as a sort of pacifier but we need more. Collect 20 Bones like the one in my room to the left and report here.", setQuestValue(230, 1)}},
	{{eq(TOPIC, 11)}, {"Too bad, perhaps another time then."}},

	{{eq(TOPIC, 9), "yes"}, {"For your noble deeds I grant you the title Assistant Postofficer. All Postofficers will charge you less money from now on. After every second mission ask me for an ADVANCEMENT. Your next task will be a bit more challenging. Do you feel ready for it?", setQuestValue(250, 1), setQuestValue(249, 1), setTopic(10)}},
	{{eq(TOPIC, 9)}, {"Too bad, perhaps another time then."}},

	{{eq(TOPIC, 10), "yes"}, {"I need you to deliver a bill to the stage magician David Brassacres. He's hiding from his creditors somewhere in Venore. It's likely you will have to trick him somehow to reveal his identity. Report back when you delivered this bill.", setAmount(1), create(3216), setQuestValue(229, 1)}},
	{{eq(TOPIC, 10)}, {"Too bad, perhaps another time then."}},

	-- Topic=8,"yes"     -> "I am glad to hear that. One of our mailboxes was reported to be jammed. It is located on the so called 'mountain' on the isle Folda. Get a crowbar and fix the mailbox. Report about your mission when you have done so.",Topic=9,SetQuestValue(228,1)
	{{eq(TOPIC, 8), "yes"}, {"I am glad to hear that. One of our mailboxes was reported to be jammed. It is located on the so called 'mountain' on the isle Folda. Get a crowbar and fix the mailbox. Report about your mission when you have done so.", setQuestValue(228, 1)}},
	{{eq(TOPIC, 8)}, {"I thought so. The mail service is not for just anyone."}},

	{{eq(TOPIC, 1), "yes"}, {"Hm, I might consider your proposal, but first you will have to prove your worth by doing some tasks for us. Are you willing to do that?", setTopic(2)}},
	{{eq(TOPIC, 1)}, {"I thought so. The mail service is not for just anyone."}},

	{{eq(TOPIC, 2), "yes"}, {"Excellent! Your first task will be quite simple. But you should better write my instructions down anyways. You can read and write?", setTopic(3)}},
	{{eq(TOPIC, 2)}, {"I thought so. The mail service is not for just anyone."}},

	{{eq(TOPIC, 3), "yes"}, {"So listen, you will check certain tours our members have to take to see if there is some trouble. First travel with Captain Bluebear's ship from Thais to Carlin, understood?", setTopic(4)}},
	{{eq(TOPIC, 3)}, {"I am sorry, but being illiterate disqualifies you from joining."}},

	{{eq(TOPIC, 4), "yes"}, {"Excellent! Once you have done that you will travel with Uzon to Edron. You will find him in the Femor Hills. Understood?", setTopic(5)}},
	{{eq(TOPIC, 4)}, {"I thought so. The mail service is not for just anyone."}},

	{{eq(TOPIC, 5), "yes"}, {"Fine, fine! Next, travel with Captain Seahorse to the city of Venore. Understood?", setTopic(6)}},
	{{eq(TOPIC, 5)}, {"I thought so. The mail service is not for just anyone."}},

	{{eq(TOPIC, 6), "yes"}, {"Good! Finally, find the technomancer Brodrosch and travel with him to the Isle of Cormaya. After this passage report back to me here. Understood?", setTopic(7)}},
	{{eq(TOPIC, 6)}, {"I thought so. The mail service is not for just anyyone."}},

	{{eq(TOPIC, 7), "yes"}, {"Ok, remember: the Tibian mail service puts trust in you! Don't fail and report back soon. Just tell me about your MISSION.", setQuestValue(227, 1)}},
	{{eq(TOPIC, 7)}, {"I thought so. The mail service is not for just anyone."}},
}

function onCreatureSay(npc, creature, text, situation)
	behaviour.react(npc, rules, text, situation)
end

function onCreatureDisappear(npc, creature)
	behaviour.react(npc, rules, "", NPC_SITUATION_VANISH)
end
