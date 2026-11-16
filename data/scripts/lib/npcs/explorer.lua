-- lib/npcs/explorer.lua: the rule block explorer.ndb, converted by patella-convert.
-- An NPC that included it (@"explorer.ndb") has it in its rules where the include
-- was; lib/npc.lua matches and runs them (scripting.hh, "NPCs"). The comments
-- are those of explorer.ndb.
--
-- GIMUD - Graphical Interface Multi User Dungeon
-- gen-expl.ndb:  Datenbank für die Explorers Society

local behaviour = require("lib.npc")
local P1 = behaviour.P1
local eq = behaviour.eq
local lt = behaviour.lt
local gt = behaviour.gt
local ge = behaviour.ge
local TOPIC = behaviour.TOPIC
local PRICE = behaviour.PRICE
local AMOUNT = behaviour.AMOUNT
local TYPE = behaviour.TYPE
local COUNTMONEY = behaviour.COUNTMONEY
local count = behaviour.count
local questValue = behaviour.questValue
local add = behaviour.add
local mul = behaviour.mul
local setTopic = behaviour.setTopic
local setPrice = behaviour.setPrice
local setAmount = behaviour.setAmount
local setType = behaviour.setType
local setQuestValue = behaviour.setQuestValue
local effectOpp = behaviour.effectOpp
local create = behaviour.create
local delete = behaviour.delete
local createMoney = behaviour.createMoney
local deleteMoney = behaviour.deleteMoney
local previous = behaviour.previous

return behaviour.rules {
	{{"job"}, {"I am the local representative of the explorer society."}},

	{{"explorer", "society"}, {"Our noble society is dedicated to explore the unknown. No location is too remote for our members to travel there ...", "No beast is too wild to be hunted. No treasure buried too deep to be unearthed ...", "Only the most dedicated and fearless adventurers may join our ranks."}},
	{{"base"}, {"Currently we maintain public bases in Port Hope and Northport."}},
	{{"join", gt(questValue(300), 0)}, {"But you are already a member, %N"}},
	{{"join", lt(questValue(300), 1)}, {"Do you want to join the explorer society?", setTopic(1)}},
	{{"no", eq(TOPIC, 1)}, {"I see. Not everyone has the guts it takes to become a professional explorer."}},
	{{"yes", eq(TOPIC, 1)}, {"Fine, though it takes more than a mere lip service to join our ranks. To prove your dedication to the cause you will have to acquire an item for us ...", "The mission should be simple to fulfil. For our excavations we have ordered a sturdy pickaxe in Kazordoon. You would have to seek out this trader Uzgod and get the pickaxe for us ...", "Simple enough? Are you interested in this task?", setTopic(2)}},

	{{"no", eq(TOPIC, 2)}, {"I see. Not everyone has the guts it takes to become a professional explorer."}},
	{{"yes", eq(TOPIC, 2)}, {"We will see if you can handle this simple task. Get the pickaxe from Uzgod in Kazordoon and bring it to one of our bases. Report there about the pickaxe.", setQuestValue(325, 1)}},

	{{"pickaxe", lt(questValue(300), 1)}, {setType(4845), setAmount(1), "Did you get the requested pickaxe from Uzgod in Kazordoon?", setTopic(9)}},
	--################  
	{{"no", eq(TOPIC, 9)}, {"Get that special pickaxe from Uzgod in Kazordoon."}},
	{{"yes", eq(TOPIC, 9), lt(count(TYPE), AMOUNT)}, {"Get that special pickaxe from Uzgod in Kazordoon."}},
	{{"yes", eq(TOPIC, 9), ge(count(TYPE), AMOUNT)}, {"Excellent, you brought just the tool we need! Of course it was only a simple task. However ...", "I officially welcome you to the explorer society. From now on you can ask for missions to improve your rank.", delete(TYPE), setQuestValue(300, 1)}},

	{{"rank", gt(questValue(300), 0), lt(questValue(300), 4)}, {"You are a novice of the explorer society."}},
	{{"rank", gt(questValue(300), 3), lt(questValue(300), 7)}, {"You are a journeyman of the explorer society."}},
	{{"rank", gt(questValue(300), 6), lt(questValue(300), 9)}, {"You are a relic hunter of the explorer society."}},
	{{"rank", gt(questValue(300), 8)}, {"You are an explorer of the explorer society."}},
	--################# Allgemeine Missionen 
	{{"ratha", eq(questValue(302), 0)}, {"Ratha was a great explorer and even greater ladies' man. Sadly he never returned from a visit to the amazons. Probably he is dead ...", "The society offers a substantial reward for the retrieval of Ratha or his remains. Do you have any news about Ratha?", setType(3207), setAmount(1), setPrice(250), setTopic(3)}},
	{{"ratha", eq(questValue(302), 1)}, {"Ratha was a great explorer and even greater ladies' man. Thank you for returning his remains."}},

	{{"no", eq(TOPIC, 3)}, {"If you ever stumble across some information about Ratha, let us know."}},
	{{"yes", eq(TOPIC, 3), eq(questValue(302), 0), lt(count(TYPE), AMOUNT)}, {"What are you talking about? You have nothing to clarify Ratha's whereabouts."}},
	{{"yes", eq(TOPIC, 3), eq(questValue(302), 0), ge(count(TYPE), AMOUNT)}, {"Poor Ratha. Thank you for returning this skull to the society. We will see to a honourable burial of Ratha.", delete(TYPE), createMoney, setQuestValue(302, 1)}},

	{{"brooch"}, {"Our members travel to far away places and cross dangerous areas, many fall prey to enemies or the land ...", "Sometimes the personal explorer brooches can be recovered. That way we learn about the fate of our members ...", "We offer a reward for each brooch returned to us. Have you found an explorer brooch?", setType(3005), setAmount(1), setPrice(50), setTopic(53)}},

	{{"no", eq(TOPIC, 53)}, {"Sometimes no news is good news."}},
	{{"yes", eq(TOPIC, 53), ge(count(TYPE), AMOUNT)}, {"It's always a sad day when we learn about the death of a member. But at least we learnt about his fate. Thank you, here is your reward.", delete(TYPE), createMoney}},
	{{"yes", eq(TOPIC, 53), lt(count(TYPE), AMOUNT)}, {"You don't have any lost brooch with you. This is not a topic to make fun of."}},

	{{"smith", "hammer"}, {"The explorer society is looking for a genuine giant smith hammer for our collection. It is rumoured the cyclopses of the Plains of Havoc might be using one. Did you by chance obtain such a hammer?", setType(3208), setAmount(1), setPrice(250), setTopic(4)}},
	{{"smith", "hammer", eq(questValue(303), 1)}, {"The explorer society was looking for a genuine giant smith hammer until you brought us one. Thank you again."}},

	{{"yes", eq(TOPIC, 4), ge(count(TYPE), AMOUNT)}, {"Marvellous! You brought a giant smith hammer for the explorer society!", delete(TYPE), createMoney, setQuestValue(303, 1)}},
	{{"yes", eq(TOPIC, 4), lt(count(TYPE), AMOUNT)}, {"This is no giant smith hammer."}},
	{{"no", eq(TOPIC, 4)}, {"Just as you like. But think about the reward!"}},

	--######

	{{"hydra", "egg"}, {"The examination of hydra eggs is a valuable source of information. We buy hydra eggs for 500 gold. Are you interested in selling one?", setType(4839), setAmount(1), setPrice(500), setTopic(40)}},

	{{"sell", P1, lt(1, P1), "hydra", "egg"}, {setType(4839), setAmount(P1), setPrice(mul(500, P1)), "The examination of hydra eggs is a valuable source of information. We buy hydra eggs for 500 gold each. Do you want to sell %A hydra eggs for %P gold?", setTopic(40)}},

	{{"no", eq(TOPIC, 40)}, {"If you ever aquire a hydra egg, bring it here."}},
	{{"yes", eq(TOPIC, 40), lt(count(TYPE), AMOUNT)}, {"What are you talking about? You don't own that many hydra eggs."}},
	{{"yes", eq(TOPIC, 40), ge(count(TYPE), AMOUNT)}, {"Thank you in the name of science.", delete(TYPE), createMoney}},
	--###
	{{"scroll", "lizard"}, {"The examination of scrolls with lizard writings is a valuable source of information. We buy such scrolls for 500 gold. Are you interested to sell one?", setType(4831), setAmount(1), setPrice(500), setTopic(41)}},
	{{"parchment"}, {previous}},

	{{"no", eq(TOPIC, 41)}, {"If you ever aquire a parchment with lizard writings, bring it here."}},
	{{"yes", eq(TOPIC, 41), lt(count(TYPE), AMOUNT)}, {"What are you talking about? You own no parchment with lizard writings at all."}},
	{{"yes", eq(TOPIC, 41), ge(count(TYPE), AMOUNT)}, {"Thank you in the name of science.", delete(TYPE), createMoney}},

	--#####

	--####################### Missionen 

	{{"mission", gt(questValue(300), 0), lt(questValue(300), 4)}, {"The missions available for your rank are the butterfly hunt, plant collection and ice delivery."}},

	{{"butterfly", "hunt", gt(questValue(300), 0), eq(questValue(304), 0)}, {"The mission asks you to collect some species of butterflies, are you interested?", setTopic(5)}},
	{{"no", eq(TOPIC, 5)}, {"Perhaps another mission suits you more."}},
	{{"yes", eq(TOPIC, 5)}, {"This preparation kit will allow you to collect a purple butterfly you have killed ...", "Just use it on the fresh corpse of a purple butterfly, return the prepared butterfly to me and give me a report of your butterfly hunt.", create(4863), setQuestValue(304, 1)}},

	{{"butterfly", "hunt", eq(questValue(304), 1)}, {"Did you acquire the purple butterfly we are looking for?", setType(4865), setAmount(1), setTopic(6)}},
	{{"no", eq(TOPIC, 6)}, {"Then go and look for one."}},
	{{"yes", eq(TOPIC, 6), lt(count(TYPE), AMOUNT)}, {"I can't see a purple butterfly. Perhaps you lost it somewhere."}},
	{{"yes", eq(TOPIC, 6), ge(count(TYPE), AMOUNT)}, {"A little bit battered but it will do. Thank you! If you think you are ready, ask for another butterfly hunt.", delete(TYPE), setQuestValue(304, 2)}},

	{{"butterfly", "hunt", eq(questValue(304), 2)}, {"This preparation kit will allow you to collect a blue butterfly you have killed ...", "Just use it on the fresh corpse of a blue butterfly, return the prepared butterfly to me and give me a report of your butterfly hunt.", create(4863), setQuestValue(304, 3)}},

	{{"butterfly", "hunt", eq(questValue(304), 3)}, {"Did you acquire the blue butterfly we are looking for?", setType(4866), setAmount(1), setTopic(7)}},
	{{"no", eq(TOPIC, 7)}, {"Then go and look for one."}},
	{{"yes", eq(TOPIC, 7), lt(count(TYPE), AMOUNT)}, {"I can't see a blue butterfly. Perhaps you lost it somewhere."}},
	{{"yes", eq(TOPIC, 7), ge(count(TYPE), AMOUNT)}, {"Again I think it will do. Thank you! If you think you are ready, ask for another butterfly hunt.", delete(TYPE), setQuestValue(304, 4)}},

	{{"butterfly", "hunt", eq(questValue(304), 4)}, {"This preparation kit will allow you to collect a red butterfly you have killed ...", "Just use it on the fresh corpse of a red butterfly, return the prepared butterfly to me and give me a report of your butterfly hunt.", create(4863), setQuestValue(304, 5)}},

	{{"butterfly", "hunt", eq(questValue(304), 5)}, {"Did you acquire the red butterfly we are looking for?", setType(4864), setAmount(1), setTopic(8)}},
	{{"no", eq(TOPIC, 8)}, {"Then go and look for one."}},
	{{"yes", eq(TOPIC, 8), lt(count(TYPE), AMOUNT)}, {"I can't see a red butterfly. Perhaps you lost it somewhere."}},
	{{"yes", eq(TOPIC, 8), ge(count(TYPE), AMOUNT)}, {"That is an extraordinary species you have brought. Thank you! That was the last butterfly we needed.", delete(TYPE), setQuestValue(304, 6), setQuestValue(300, add(questValue(300), 1))}},
	{{"butterfly", "hunt", eq(questValue(304), 6)}, {"You have already finished the butterfly hunt. Of course you can always ask me what missions are available."}},

	--### topic 9 ist verwendet 

	{{"plant", "collection", gt(questValue(300), 0), eq(questValue(305), 0)}, {"In this mission we require you to get us some plant samples from Tiquandan plants. Would you like to fulfil this mission?", setTopic(10)}},
	{{"no", eq(TOPIC, 10)}, {"Perhaps another mission suits you more."}},
	{{"yes", eq(TOPIC, 10)}, {"Fine! Here take this botanist's container. Use it on a jungle bells plant to collect a sample for us. Report about your plant collection when you have been successful.", create(4867), setQuestValue(305, 1)}},

	{{"plant", "collection", eq(questValue(305), 1)}, {"Did you acquire the sample of the jungle bells plant we are looking for?", setType(4868), setAmount(1), setTopic(11)}},
	{{"no", eq(TOPIC, 11)}, {"Then go and look for one."}},
	{{"yes", eq(TOPIC, 11), lt(count(TYPE), AMOUNT)}, {"Sorry, you don't have a useful sample."}},
	{{"yes", eq(TOPIC, 11), ge(count(TYPE), AMOUNT)}, {"I see. It seems you've got some quite useful sample by sheer luck. Thank you! Just tell me when you are ready to continue with the plant collection.", delete(TYPE), setQuestValue(305, 2)}},

	{{"plant", "collection", eq(questValue(305), 2)}, {"Use this botanist's container on a witches cauldron to collect a sample for us. Bring it here and report about your plant collection.", create(4867), setQuestValue(305, 3)}},

	{{"plant", "collection", eq(questValue(305), 3)}, {"Did you acquire the sample of the witches cauldron we are looking for?", setType(4870), setAmount(1), setTopic(12)}},
	{{"no", eq(TOPIC, 12)}, {"Then go and look for one."}},
	{{"yes", eq(TOPIC, 12), lt(count(TYPE), AMOUNT)}, {"Sorry, you don't have any useful sample."}},
	{{"yes", eq(TOPIC, 12), ge(count(TYPE), AMOUNT)}, {"Ah, finally. I started to wonder what took you so long. But thank you! Another fine sample, indeed. Just tell me when you are ready to continue with the plant collection.", delete(TYPE), setQuestValue(305, 4)}},

	{{"plant", "collection", eq(questValue(305), 4)}, {"Use this botanist's container on a giant jungle rose to obtain a sample for us. Bring it here and report about your plant collection.", create(4867), setQuestValue(305, 5)}},

	{{"plant", "collection", eq(questValue(305), 5)}, {"Did you acquire the sample of the giant jungle rose we are looking for?", setType(4869), setAmount(1), setTopic(13)}},
	{{"no", eq(TOPIC, 13)}, {"Then go and look for one. Keep in mind we need samples of the giant jungle rose, not the small one."}},
	{{"yes", eq(TOPIC, 13), lt(count(TYPE), AMOUNT)}, {"Sorry you don't have any useful sample. Perhaps you did not use the botanist's container on a giant jungle rose as requested, but on a small one."}},
	{{"yes", eq(TOPIC, 13), ge(count(TYPE), AMOUNT)}, {"What a lovely sample! With that you have finished your plant collection missions.", delete(TYPE), setQuestValue(305, 6), setQuestValue(300, add(questValue(300), 1))}},

	{{"plant", "collection", eq(questValue(305), 6)}, {"You have already finished the plant collection missions. Of course you can always ask me what missions are available."}},

	{{"ice", "delivery", gt(questValue(300), 0), eq(questValue(306), 0)}, {"Our finest minds came up with the theory that deep beneath the ice island of Folda ice can be found that is ancient. To prove this theory we would need a sample of the aforesaid ice ...", "Of course the ice melts away quickly so you would need to hurry to bring it here ...", "Would you like to accept this mission?", setTopic(14)}},

	{{"no", eq(TOPIC, 14)}, {"Perhaps another mission suits you more."}},
	{{"yes", eq(TOPIC, 14)}, {"So listen please: Take this ice pick and use it on a block of ice in the caves beneath Folda. Get some ice and bring it here as fast as you can ...", "Should the ice melt away, report on your ice delivery mission anyway. I will then tell you if the time is right to start another mission.", create(4872), setQuestValue(306, 1)}},
	{{"ice", "delivery", eq(questValue(306), 1)}, {"Did you get the ice we are looking for?", setType(4837), setAmount(1), setTopic(15)}},
	{{"no", eq(TOPIC, 15)}, {"Did it melt away?", setTopic(16)}},
	{{"no", eq(TOPIC, 16)}, {"Then don't waste your time and go to the caves on Folda to get some ice."}},
	{{"yes", eq(TOPIC, 16), eq(questValue(307), 0)}, {"I think you are wrong, just try to get that ice as you were told."}},
	{{"yes", eq(TOPIC, 16), eq(questValue(307), 1), gt(count(TYPE), AMOUNT)}, {"What are you talking about, I can see you still have some ice on you."}},
	{{"yes", eq(TOPIC, 16), eq(questValue(307), 1), lt(count(TYPE), AMOUNT)}, {"*Sigh* I think the time is right to grant you another chance to get that ice. Hurry up this time.", setQuestValue(307, 0), effectOpp(13)}},

	{{"yes", eq(TOPIC, 15), lt(count(TYPE), AMOUNT)}, {"Sorry, you don't have any ice on you."}},
	{{"yes", eq(TOPIC, 15), ge(count(TYPE), AMOUNT), eq(questValue(307), 0)}, {"This ice looks odd. I don't think it's from Folda. Make sure to travel there to get the ice we are looking for."}},
	{{"yes", eq(TOPIC, 15), ge(count(TYPE), AMOUNT), eq(questValue(307), 1)}, {"Just in time. Sadly not much ice is left over but it will do. Thank you again.", setQuestValue(306, 2), setQuestValue(300, add(questValue(300), 1)), delete(TYPE)}},

	{{"ice", "delivery", eq(questValue(306), 2)}, {"You already brought us the ice sample that we needed. Of course you can always ask me what missions are available."}},

	--########
	-- Debug #
	--########

	--"advancement", QuestValue(300)>0, QuestValue(304)<6  -> "You have not even finished your butterfly hunt. Perform some missions and ask again."
	--"advancement", QuestValue(300)>0, QuestValue(304)=6  -> "So you think you are worthy of an advancement?",topic=55
	--"no",topic=55 -> "Then stop bothering me with it."
	--"yes",topic=55, QuestValue(305)<6 -> "You have not even finished your plant collection. Perform some missions and ask again."
	--"yes",topic=55, QuestValue(305)=6 -> "Well, you performed some simple tasks at least but do you realy think you deserve an advancement?",Topic=56
	--"no",topic=56 -> "Then stop bothering me with it."
	--"yes",topic=56, QuestValue(306)<2 -> "First finish the ice delivery mission. Then we might talk about advancement."
	--"yes",topic=56, QuestValue(306)=2, QuestValue(300)>3 -> "Sorry but your rank mirrors your performance perfectly."
	--"yes",topic=56, QuestValue(306)=2, QuestValue(300)<4 -> "This is odd indeed. I must have confused something in my papers, sorry. I grant you the rank of a journeyman.",SetQuestValue(300,4)

	--#######################
	-- NEUE MISSIONEN (4-6) #
	--#######################

	{{"mission", gt(questValue(300), 3), lt(questValue(300), 7)}, {"The missions available for your rank are lizard urn, beholder secrets and orc powder."}},

	{{"lizard", "urn", eq(questValue(308), 0), gt(questValue(300), 3)}, {"The explorer society would like to acquire an ancient urn which is some sort of relic to the lizard people of Tiquanda. Would you like to accept this mission?", setTopic(17)}},

	{{"no", eq(TOPIC, 17)}, {"Perhaps another mission suits you more."}},
	{{"yes", eq(TOPIC, 17)}, {"You have indeed the spirit of an adventurer! In the south-east of Tiquanda is a small settlement of the lizard people ...", "Beneath the newly constructed temple there, the lizards hide the said urn. Our attempts to acquire this item were without success ...", "Perhaps you are more successful.", setQuestValue(308, 1)}},

	{{"lizard", "urn", eq(questValue(308), 1)}, {"Did you manage to get the ancient urn?", setType(4847), setAmount(1), setTopic(18)}},

	{{"no", eq(TOPIC, 18)}, {"It must be somewhere beneath the newly constructed lizard temple in the south-east of Tiquanda."}},
	{{"yes", eq(TOPIC, 18), lt(count(TYPE), AMOUNT)}, {"Sorry, you don't have the urn. It has to be somewhere beneath the newly constructed lizard temple in the south-east of Tiquanda."}},
	{{"yes", eq(TOPIC, 18), ge(count(TYPE), AMOUNT)}, {"Yes, that is the prized relic we have been looking for so long. You did a great job, thank you.", delete(TYPE), setQuestValue(308, 2), setQuestValue(300, add(questValue(300), 1))}},

	{{"lizard", "urn", eq(questValue(308), 2)}, {"You already retrieved the urn for us. Of course you can always ask me what missions are available."}},

	--#####################

	{{"beholder", "secret", eq(questValue(309), 0), gt(questValue(300), 3)}, {"We want to learn more about the ancient race of beholders. We believe the black pyramid north east of Darashia was originally built by them ...", "We ask you to explore the ruins of the black pyramid and look for any signs that prove our theory. You might probably find some document with the numeric beholder language ...", "That would be sufficient proof. Would you like to accept this mission?", setTopic(19)}},

	{{"no", eq(TOPIC, 19)}, {"Perhaps another mission suits you more."}},
	{{"yes", eq(TOPIC, 19)}, {"Excellent! So travel to the city of Darashia and then head north-east for the pyramid ...", "If any documents are left, you probably find them in the catacombs beneath. Good luck!", setQuestValue(309, 1)}},

	{{"beholder", "secret", eq(questValue(309), 1)}, {"Have you found any proof that the pyramid was built by beholders?", setType(4846), setAmount(1), setTopic(20)}},
	{{"no", eq(TOPIC, 20)}, {"We are sure there is some document left. Probably in the deepest catacombs beneath the black pyramid."}},
	{{"yes", eq(TOPIC, 20), lt(count(TYPE), AMOUNT)}, {"Sorry, whatever you have found is no true proof for our theory. Please return to the black pyramid and explore it carefully."}},
	{{"yes", eq(TOPIC, 20), ge(count(TYPE), AMOUNT)}, {"You did it! Excellent! The scientific world will be shaken by this discovery!", delete(TYPE), setQuestValue(309, 2), setQuestValue(300, add(questValue(300), 1))}},

	{{"beholder", "secret", eq(questValue(309), 2)}, {"You already recovered the scroll. Of course you can always ask me what missions are available."}},

	--####################

	{{"orc", "powder", eq(questValue(310), 0), gt(questValue(300), 3)}, {"It is commonly known that orcs of Uldereks Rock use some sort of powder to increase the fierceness of their war wolves and berserkers ...", "What we do not know are the ingredients of this powder and its effect on humans ...", "So we would like you to get a sample of the aforesaid powder. Do you want to accept this mission?", setTopic(21)}},

	{{"no", eq(TOPIC, 21)}, {"Perhaps another mission suits you more."}},
	{{"yes", eq(TOPIC, 21)}, {"You are a brave soul. As far as we can tell, the orcs maintain some sort of training facility in some hill in the north-east of their city ...", "There you should find lots of their war wolves and hopefully also some of the orcish powder. Good luck!", setQuestValue(310, 1)}},

	{{"orc", "powder", eq(questValue(310), 1)}, {"Did you acquire some of the orcish powder?", setType(4838), setAmount(1), setTopic(34)}},
	{{"no", eq(TOPIC, 34)}, {"Make sure to search in the hill they use to raise and train their war wolves."}},
	{{"yes", eq(TOPIC, 34), lt(count(TYPE), AMOUNT)}, {"Sorry, you have nothing with you that would fit the descriptions we got of the powder."}},
	{{"yes", eq(TOPIC, 34), ge(count(TYPE), AMOUNT)}, {"You really got it? Amazing! Thank you for your efforts.", delete(TYPE), setQuestValue(310, 2), setQuestValue(300, add(questValue(300), 1))}},
	{{"orc", "powder", eq(questValue(310), 2)}, {"You already brought us some orcish powder. Of course you can always ask me what missions are available."}},

	--#########################
	--  NEUE MISSIONEN (7-9)  #
	--#########################

	{{"mission", gt(questValue(300), 6), lt(questValue(300), 10)}, {"The missions available for your rank are elven poetry, memory stone and rune writings."}},

	{{"elven", "poetry", eq(questValue(311), 0), gt(questValue(300), 6)}, {"Some high ranking members would like to study elven poetry. They want the rare book 'Songs of the Forest' ...", "For sure someone in Ab'Dendriel will own a copy. So you would just have to ask around there. Are you willing to accept this mission?", setTopic(22)}},
	{{"no", eq(TOPIC, 22)}, {"Perhaps another mission suits you more."}},
	{{"yes", eq(TOPIC, 22)}, {"Excellent. This mission is easy but nonetheless vital. Travel to Ab'Dendriel and get the book.", setQuestValue(311, 1)}},

	{{"song", "forest", eq(questValue(311), 1)}, {"Did you acquire a copy of 'Songs of the Forest' for us?", setType(4844), setAmount(1), setTopic(23)}},
	{{"elven", "poetry", eq(questValue(311), 1)}, {previous}},
	{{"book", eq(questValue(311), 1)}, {previous}},
	{{"no", eq(TOPIC, 23)}, {"Then try harder. Someone might own it. If you lost a copy, ask around again."}},
	{{"yes", eq(TOPIC, 23), lt(count(TYPE), AMOUNT)}, {"Whatever you thought you acquired, it's not the book we are looking for! If you lost the copy, ask around again."}},
	{{"yes", eq(TOPIC, 23), ge(count(TYPE), AMOUNT), eq(questValue(312), 0)}, {"It can easily be seen that this book was forged by a dwarf. Try to get a real copy somewhere."}},
	{{"yes", eq(TOPIC, 23), ge(count(TYPE), AMOUNT), eq(questValue(312), 1)}, {"Let me have a look! Yes, that's what we wanted. A copy of 'Songs of the Forest'. I won't ask any questions about those bloodstains.", delete(TYPE), setQuestValue(311, 2), setQuestValue(300, add(questValue(300), 1))}},

	{{"elven", "poetry", eq(questValue(311), 2)}, {"You already pleased our leadership by acquiring a copy of that book. Of course you can always ask me what missions are available."}},

	--#########################

	{{"memory", "stone", eq(questValue(313), 0), gt(questValue(300), 6)}, {"We acquired some knowledge about special magic stones. Some lost civilisations used it to store knowledge and lore, just like we use books ...", "The wisdom in such stones must be immense, but so are the dangers faced by every person who tries to obtain one...", "As far as we know the ruins found in the north-west of Edron were once inhabited by beings who used such stones. Do you have the heart to go there and to get us such a stone?", setTopic(24)}},
	{{"no", eq(TOPIC, 24)}, {"Perhaps another mission suits you more."}},
	{{"yes", eq(TOPIC, 24)}, {"In the ruins of north-western Edron you should be able to find a memory stone. Good luck.", setQuestValue(313, 1)}},

	{{"memory", "stone", eq(questValue(313), 1)}, {"Were you able to acquire a memory stone for our society?", setType(4841), setAmount(1), setTopic(25)}},
	{{"no", eq(TOPIC, 25)}, {"Try harder. We are sure there are memory stones left in the north-western dungeons of Edron."}},
	{{"yes", eq(TOPIC, 25), lt(count(TYPE), AMOUNT)}, {"You don't have any memory stone!"}},
	{{"yes", eq(TOPIC, 25), ge(count(TYPE), AMOUNT), eq(questValue(314), 0)}, {"This memory stone looks damaged. Probably you bought it from some suspicious individual. Travel to Edron and get one on your own."}},
	{{"yes", eq(TOPIC, 25), ge(count(TYPE), AMOUNT), eq(questValue(314), 1)}, {"A flawless memory stone! Incredible! It will take years even to figure out how it works but what an opportunity for science, thank you!", delete(TYPE), setQuestValue(313, 2), setQuestValue(300, add(questValue(300), 1))}},

	{{"memory", "stone", eq(questValue(313), 2)}, {"You already brought us a memory stone. It will take years to make it work and probably decades to decipher and understand the knowledge the stone contains."}},

	--########################

	{{"rune", "writing", eq(questValue(315), 0), gt(questValue(300), 6)}, {"We would like to study some ancient runes that were used by the lizard race. We suspect some relation of the lizards to the founders of Ankrahmun ...", "Somewhere under the ape infested city of Banuta, one can find dungeons that were once inhabited by lizards ...", "Look there for an atypical structure that would rather fit to Ankrahmun and its tombs. Copy the runes you will find on this structure ...", "Are you up to that challenge?", setTopic(26)}},
	{{"no", eq(TOPIC, 26)}, {"Perhaps you are interested at another time."}},
	{{"yes", eq(TOPIC, 26)}, {"Excellent! Here, take this tracing paper and use it on the object you will find there to create a copy of the ancient runes.", create(4842), setQuestValue(315, 1)}},

	{{"rune", "writing", eq(questValue(315), 1)}, {"Did you create a copy of the ancient runes as requested?", setType(4843), setAmount(1), setTopic(27)}},
	{{"no", eq(TOPIC, 27)}, {"Please remember, somewhere in the dungeons beneath Banuta has to be some structure that seemingly does not belong there. Use the copy paper on it."}},
	{{"yes", eq(TOPIC, 27), lt(count(TYPE), AMOUNT)}, {"You don't have any copy of the runes that we need!"}},
	{{"yes", eq(TOPIC, 27), ge(count(TYPE), AMOUNT), eq(questValue(316), 0)}, {"This copy is ruined by sweat and blood, sorry. Travel to Banuta and make sure to make a clean copy."}},
	{{"yes", eq(TOPIC, 27), ge(count(TYPE), AMOUNT), eq(questValue(316), 1)}, {"It's a bit wrinkled but it will do. Thanks again.", delete(TYPE), setQuestValue(315, 2), setQuestValue(300, add(questValue(300), 1))}},

	{{"rune", "writing", eq(questValue(315), 2)}, {"You already brought us a copy of those runes which gave us some interesting insights."}},

	--#######################

	{{"mission", eq(questValue(300), 10), eq(questValue(317), 0)}, {"The explorer society needs a great deal of help in the research of astral travel. Are you willing to help?", setTopic(28)}},
	{{"no", eq(TOPIC, 28)}, {"Perhaps you are interested at another time"}},
	{{"yes", eq(TOPIC, 28)}, {"Fine. The society is looking for new means to travel. Some of our most brilliant minds have some theories about astral travel that they want to research further ...", "Therefore we need you to collect some ectoplasm from the corpse of a ghost. We will supply you with a collector that you can use on the body of a slain ghost ...", "Do you think you are ready for that mission?", setTopic(29)}},
	{{"no", eq(TOPIC, 29)}, {"Perhaps you are interested some other time."}},
	{{"yes", eq(TOPIC, 29)}, {"Good! Take this container and use it on a ghost that was recently slain. Return with the collected ectoplasm and hand me that container ...", "Don't lose the container. They are expensive!", create(4852), setQuestValue(317, 1)}},

	{{"ectoplasm", eq(questValue(317), 1)}, {"Do you have some collected ectoplasm with you?", setType(4853), setAmount(1), setTopic(30)}},
	{{"container", eq(questValue(317), 1)}, {previous}},
	{{"mission", eq(questValue(317), 1)}, {previous}},
	{{"no", eq(TOPIC, 30)}, {"Just use the container on a slain ghost. Make sure it lost its unholy mockery of life before you fulfil your task. Be quick, a partly decomposed corpse will not contain enough ectoplasm."}},
	{{"yes", eq(TOPIC, 30), lt(count(TYPE), AMOUNT)}, {"Sorry, but you have no ectoplasm at all!"}},
	{{"yes", eq(TOPIC, 30), ge(count(TYPE), AMOUNT)}, {"Phew, I had no idea that ectoplasm would smell that ... oh, it's you, well, sorry. Thank you for the ectoplasm.", delete(TYPE), setQuestValue(317, 2), setQuestValue(300, 11)}},

	{{"ectoplasm", eq(questValue(317), 2)}, {"You already brought us enough ectoplasm for our research."}},

	--########################

	{{"mission", eq(questValue(300), 11), eq(questValue(318), 0)}, {"The research on ectoplasm makes good progress. Now we need some spectral article. Our scientists think a spectral dress would be a perfect object for their studies ...", "The bad news is that the only source to got such a dress is the queen of the banshees. Do you dare to seek her out?", setTopic(31)}},

	{{"no", eq(TOPIC, 31)}, {"Perhaps you are interested in it some other time."}},
	{{"yes", eq(TOPIC, 31)}, {"That is quite courageous. We know, it's much we are asking for. The queen of the banshees lives in the so called Ghostlands, south west of Carlin. It is rumoured that her lair is located in the deepest dungeons beneath that cursed place ...", "Any violence will probably be futile, you will have to negotiate with her. Try to get a spectral dress from her. Good luck.", setQuestValue(318, 1)}},

	{{"spectral", "dress", eq(questValue(318), 1)}, {"Have you acquired the spectral dress we need?", setType(4836), setAmount(1), setTopic(32)}},
	{{"mission", eq(questValue(318), 1)}, {previous}},

	{{"no", eq(TOPIC, 32)}, {"Remember, the queen of the banshees lives in the so called Ghostlands, south west of Carlin. Her lair is rumoured to be in the deepest dungeons beneath that cursed place ...", "Any violence will probably be futile and you will have to negotiate with her. Try to get a spectral dress from her."}},
	{{"yes", eq(TOPIC, 32), lt(count(TYPE), AMOUNT)}, {"Sorry, you don't have a spectral dress!"}},
	{{"yes", eq(TOPIC, 32), ge(count(TYPE), AMOUNT), eq(questValue(319), 0)}, {"Sorry, this dress is infested with spectral moths. Get a spectral dress from the banshee queen."}},
	{{"yes", eq(TOPIC, 32), ge(count(TYPE), AMOUNT), eq(questValue(319), 1)}, {"Just in time! With this spectral article we can start the final phase of our research.", delete(TYPE), setQuestValue(318, 2), setQuestValue(300, 12)}},

	{{"spectral", eq(questValue(318), 2)}, {"You provided us with a spectral dress and we are ready for the final stages of our research project."}},

	--####################

	{{"portals", eq(questValue(320), 5)}, {"Sorry, you did not charge both floor tiles as requested."}},
	{{"mission", eq(questValue(320), 5)}, {previous}},

	{{"portals", eq(questValue(320), 5), eq(questValue(321), 1), eq(questValue(322), 1), eq(questValue(323), 0)}, {setAmount(6), setType(5021), "Both carvings are now charged and harmonised. In theory you should be able to travel in zero time from one base to the other ...", "However, you will need to have an orichalcum pearl in your possession to use it as power source. It will be destroyed during the process. I will give you 6 of such pearls and you can buy new ones in our bases ...", "In addition, you need to be a premium explorer to use the astral travel ...", "And remember: it's a small teleport for you, but a big teleport for all Tibians.", setQuestValue(323, 1), create(TYPE)}},
	{{"mission", eq(questValue(320), 5), eq(questValue(321), 1), eq(questValue(322), 1), eq(questValue(323), 0)}, {previous}},

	{{"portals", eq(questValue(320), 5), eq(questValue(321), 1), eq(questValue(322), 1), eq(questValue(323), 1)}, {"The portals should be ready to be used now. If you have an orichalcum pearl with you, enter the portal."}},
	{{"mission", eq(questValue(320), 5), eq(questValue(321), 1), eq(questValue(322), 1), eq(questValue(323), 1)}, {"There are no new missions avaliable right now."}},
	--#####################
	{{"orichalcum"}, {setAmount(1), setType(5021), setPrice(80), "Do you want to buy an orichalcum pearl for %P gold?", setTopic(51)}},

	{{eq(TOPIC, 51), "yes", ge(COUNTMONEY, PRICE)}, {"Here, use it wisely.", deleteMoney, create(TYPE)}},
	{{eq(TOPIC, 51), "yes"}, {"Sorry, you don't have the money for this transaction."}},
	{{eq(TOPIC, 51)}, {"Is there anything else I can do for you?"}},

	{{P1, lt(1, P1), "orichalcum"}, {setAmount(P1), setType(5021), setPrice(mul(80, P1)), "Do you want to buy %A orichalcum pearls for %P gold?", setTopic(52)}},

	{{eq(TOPIC, 52), "yes", ge(COUNTMONEY, PRICE)}, {"Here, use them wisely.", deleteMoney, create(TYPE)}},
	{{eq(TOPIC, 52), "yes"}, {"Sorry, you don't have the money for this transaction."}},
	{{eq(TOPIC, 52)}, {"Is there anything else I can do for you?"}},

	--#####################

	{{"botanist", "container"}, {setAmount(1), setType(4867), setPrice(500), "Do you want to buy a botanist container for %P gold?", setTopic(56)}},
	{{"preparation", "kit"}, {setAmount(1), setType(4863), setPrice(250), "Do you want to buy a preparation kit for %P gold?", setTopic(56)}},
	{{"ectoplasm", "container"}, {setAmount(1), setType(4852), setPrice(750), "Do you want to buy an ectoplasm container for %P gold?", setTopic(56)}},

	{{eq(TOPIC, 56), "yes", ge(COUNTMONEY, PRICE)}, {"Here, better don't lose it.", deleteMoney, create(TYPE)}},
	{{eq(TOPIC, 56), "yes"}, {"Sorry, you don't have the money for this transaction."}},
	{{eq(TOPIC, 56)}, {"Is there anything else I can do for you?"}},
}
