-- npcs/hairycles.lua: the behaviour of the NPC Hairycles, converted from
-- hairycles.npc by patella-convert. The NPC is its [[npc]] entry in npcs.toml;
-- lib/npc.lua matches and runs the rules below, in the order of the file
-- (scripting.hh, "NPCs"). The comments are those of hairycles.npc.
--
-- GIMUD - Graphical Interface Multi User Dungeon
-- hairycles.npc: Datenbank für den Affen Hairycles

local behaviour = require("lib.npc")
local ADDRESS = behaviour.ADDRESS
local BUSY = behaviour.BUSY
local VANISH = behaviour.VANISH
local FORCE = behaviour.FORCE
local P1 = behaviour.P1
local eq = behaviour.eq
local lt = behaviour.lt
local gt = behaviour.gt
local ge = behaviour.ge
local TOPIC = behaviour.TOPIC
local PRICE = behaviour.PRICE
local AMOUNT = behaviour.AMOUNT
local TYPE = behaviour.TYPE
local HP = behaviour.HP
local BURNING = behaviour.BURNING
local POISON = behaviour.POISON
local COUNTMONEY = behaviour.COUNTMONEY
local count = behaviour.count
local questValue = behaviour.questValue
local mul = behaviour.mul
local setTopic = behaviour.setTopic
local setPrice = behaviour.setPrice
local setAmount = behaviour.setAmount
local setType = behaviour.setType
local setHp = behaviour.setHp
local burning = behaviour.burning
local poison = behaviour.poison
local setQuestValue = behaviour.setQuestValue
local effectOpp = behaviour.effectOpp
local create = behaviour.create
local delete = behaviour.delete
local deleteMoney = behaviour.deleteMoney
local idle = behaviour.idle
local nop = behaviour.nop
local previous = behaviour.previous

local rules = behaviour.rules {
	{{ADDRESS, "hello$", gt(questValue(293), 11), FORCE}, {"Be greeted, friend of the ape people. If you want to trade, just ask for my offers. If you are injured, ask for healing."}},
	{{ADDRESS, "hi$", gt(questValue(293), 11), FORCE}, {previous}},

	{{ADDRESS, "hello$", FORCE}, {"Oh! Hello! Hello! Did not notice!"}},
	{{ADDRESS, "hi$", FORCE}, {previous}},
	{{ADDRESS, FORCE}, {idle}},
	{{BUSY, "hello$", FORCE}, {"Wait! Wait! Time I no have!"}},
	{{BUSY, "hi$", FORCE}, {previous}},
	{{BUSY, FORCE}, {nop}},
	{{VANISH, FORCE}, {"Bye, bye."}},

	{{"bye"}, {"Bye, bye.", idle}},
	{{"farewell"}, {previous}},
	{{"how", "are", "you"}, {"Me fine, me fine."}},
	{{"advice"}, {"You stay away from other apes. We not like foreigners. Especially with so little hair."}},
	{{"job"}, {"Me great wizard. Me great doctor. Me know many plants. Me old and me have seen many things."}},
	{{"name"}, {"Me is Hairycles."}},
	{{"time"}, {"You look to suns or moon and time you know."}},
	{{"help"}, {"Me not help you can. Other apes would get mad at me."}},
	{{"jungle"}, {"Jungle is dangerous. Jungle also provides us food. Take care when in jungle and safe you be."}},
	{{"city"}, {"City now our is. Chasing away evil snakemen."}},
	{{"snakemen"}, {"Evil snakemen mean to apes and making them work and holding them captive since apes can think. But then Spartaky came."}},
	{{"spartaky"}, {"He great ape was. He fled to jungle, taught other apes of snakemen secrets. Came back with other apes and together we chased snake people away. Made city our home."}},
	{{"port", "hope"}, {"Strange hairless ape people there live. We go and get funny things from strange people."}},
	{{"ape", "people"}, {"We be kongra, sibang and merlkin. Strange hairless ape people live in city called Port Hope."}},
	{{"kongra"}, {"Kongra verry strong. Kongra verry angry verry fast. Take care when kongra comes. Better climb on highest tree."}},
	{{"sibang"}, {"Sibang verry fast and funny. Sibang good gather food. Sibang know jungle well."}},
	{{"merlkin"}, {"Merlkin we are. Merlkin verry wise, merlkin learn many things quick. Teach other apes things a lot. Making heal and making magic."}},
	{{"magic"}, {"We see many things and learning quick. Merlkin magic learn quick, quick. We just watch and learn. Sometimes we try and learn."}},
	{{"weapon"}, {"We weapons not need much. Take what is around we do. Tools we more need."}},
	{{"tools"}, {"Lot of tools snakemen left when run away. But tools go break. New tools we get where we find. Like taking banana."}},
	{{"tibia"}, {"Me know Tibia is all we see."}},

	{{"heal"}, {"You look for food and rest."}},
	{{"heal$", gt(questValue(293), 11), gt(BURNING, 0)}, {"You are burning. Me will help you.", burning(0, 0), effectOpp(15)}},
	{{"heal$", gt(questValue(293), 11), gt(POISON, 0)}, {"You are poisoned. Me will help you.", poison(0, 0), effectOpp(14)}},
	{{"heal$", gt(questValue(293), 11), lt(HP, 50)}, {"You are looking really bad. Let Hairycles heal wounds.", setHp(50), effectOpp(13)}},

	{{"offers", lt(questValue(293), 11)}, {"Me nothing have to offer you now. Perhaps ask later, when we know better."}},

	{{"banana"}, {"Banana is good. Is magic fruit. Banana makes happy. Banana means life. Banana is everything."}},
	{{"language"}, {"Strange hairless ape in loincloth came here. Zantar his name was. Brought many banana. We him liked. He here lived. Taught Hairycles funny language."}},
	-- "mission"         -> "Perhaps help you can. But not now. Later you ask again. Me fire light when help is needed."

	{{"mission", lt(questValue(293), 1)}, {"These are dire times for our people. Problems plenty are in this times. But me people not grant trust easy. Are you willing to prove you friend of ape people?", setTopic(1)}},
	{{eq(TOPIC, 1), "no"}, {"Hairycles sad is now. But perhaps you will change mind one day."}},
	{{eq(TOPIC, 1), "yes"}, {"To become friend of ape people a long and difficult way is. We do not trust easy but help is needed. Will you listen to story of Hairycles?", setTopic(2)}},

	{{"no", eq(TOPIC, 2)}, {"Hairycles thought better of you."}},
	{{"yes", eq(TOPIC, 2)}, {"So listen, little ape was struck by plague. Hairycles not does know what plague it is. That is strange. Hairycles should know. But Hairycles learnt lots and lots ...", "Me sure to make cure so strong to drive away all plague. But to create great cure me need powerful components ...", "Me need whisper moss. Whisper moss growing south of human settlement is. Problem is, evil little dworcs harvest all whisper moss immediately ...", "Me know they hoard some in their underground lair. My people raided dworcs often before humans came. So we know the moss is hidden in east of upper level of dworc lair ...", "You go there and take good moss from evil dworcs. Talk with me about mission when having moss.", setQuestValue(293, 1)}},

	{{eq(TOPIC, 2)}, {"Uh?"}},

	{{"mission", eq(questValue(293), 1), eq(questValue(294), 0)}, {"Please hurry. Bring me whisper moss from dworc lair. Make sure it is from dworc lair! Take it yourself only! If you need to hear background of all again, ask Hairycles for background."}},
	{{"background", eq(questValue(293), 1)}, {"So listen, little ape was struck by plague. Hairycles not does know what plague it is. That is strange. Hairycles should know. But Hairycles learnt lots and lots ...", "Me sure to make cure so strong to drive away all plague. But to create great cure me need powerful components ...", "Me need whisper moss. Whisper moss growing south of human settlement is. Problem is, evil little dworcs harvest all whisper moss immediately ...", "Me know they hoard some in their underground lair. My people raided dworcs often before humans came. So we know the moss is hidden in east of upper level of dworc lair ...", "You go there and take good moss from evil dworcs. Talk with me about mission when having moss."}},

	{{"mission", eq(questValue(293), 1), eq(questValue(294), 1)}, {setType(4827), setAmount(1), "Oh, you brought me whisper moss? Good hairless ape you are! Can me take it?", setTopic(3)}},

	{{"no", eq(TOPIC, 3)}, {"Strange being you are! Our people need help!", idle}},
	{{"yes", eq(TOPIC, 3), lt(count(TYPE), AMOUNT)}, {"Stupid, you no have the moss me need. Go get it. It's somewhere in dworc lair. If you lost it, they might restocked it meanwhile. If you need to hear background of all again, ask Hairycles for background.", setQuestValue(294, 0)}},

	{{"yes", ge(count(TYPE), AMOUNT), eq(TOPIC, 3)}, {"Ah yes! That's it. Thank you for bringing mighty whisper moss to Hairycles. It will help but still much is to be done. Just ask for other mission if you ready.", delete(TYPE), setQuestValue(293, 2)}},

	{{"mission", eq(questValue(293), 2)}, {"Whisper moss strong is, but me need liquid that humans have to make it work ...", "Our raiders brought it from human settlement, it's called cough syrup. Go ask healer there for it.", setQuestValue(293, 3)}},

	{{"mission", eq(questValue(293), 3)}, {setType(4828), setAmount(1), "You brought me that cough syrup from human healer me asked for?", setTopic(4)}},

	{{"no", eq(TOPIC, 4)}, {"Please hurry, urgent it is!"}},
	{{"yes", eq(TOPIC, 4), lt(count(TYPE), AMOUNT)}, {"No no, not right syrup you have. Go get other, get right health syrup."}},
	{{"yes", eq(TOPIC, 4), ge(count(TYPE), AMOUNT)}, {"You so good! Brought syrup to me! Thank you, will prepare cure now. Just ask for mission if you want help again.", delete(TYPE), setQuestValue(293, 4)}},

	{{"mission", eq(questValue(293), 4)}, {"Little ape should be healthy soon. Me so happy is. Thank you again! But me suspect we in more trouble than we thought. Will you help us again?", setTopic(5)}},

	{{"no", eq(TOPIC, 5)}, {"Me sad. Me expected better from you!", idle}},
	{{"yes", eq(TOPIC, 5)}, {"So listen, please. Plague was not ordinary plague. That's why Hairycles could not heal at first. It is new curse of evil lizard people ...", "I think curse on little one was only a try. We have to be prepared for big strike ...", "Me need papers of lizard magician! For sure you find it in his hut in their dwelling. It's south east of jungle. Go look there please! Are you willing to go?", setTopic(6)}},
	{{"no", eq(TOPIC, 6)}, {"Me sad. Me expected better from you!", idle}},
	{{"yes", eq(TOPIC, 6)}, {"Good thing that is! Report about your mission when have scroll.", setQuestValue(293, 5)}},
	{{"mission", eq(questValue(293), 5)}, {setType(4831), setAmount(1), "You got scroll from lizard village in south east?", setTopic(7)}},

	{{"no", eq(TOPIC, 7)}, {"That's bad news. If you lost it, only way to get other is to kill holy serpents. But you can't go there so you must ask adventurers who can."}},

	--### Einschub, notwendige Erklärung
	{{"holy", "serpent"}, {"Ugly beasts that are holy to lizard people. Only found in ancient temple under Banuta. But me can not allow you to go there."}},

	{{"yes", eq(TOPIC, 7), eq(questValue(295), 0), lt(count(TYPE), AMOUNT)}, {"No! That not scroll me looking for. Silly hairless ape you are. Go to village of lizards and get it there on your own!"}},
	{{"yes", eq(TOPIC, 7), eq(questValue(295), 1), lt(count(TYPE), AMOUNT)}, {"Oh, you seem to have lost scroll? That's bad news. If you lost it, only way to get other is to kill holy serpents. But you can't go there so you must ask adventurers who can."}},
	{{"yes", eq(TOPIC, 7), eq(questValue(295), 1), ge(count(TYPE), AMOUNT)}, {"You brought scroll with lizard text? Good! I will see what text tells me! Come back when ready for other mission.", delete(TYPE), setQuestValue(293, 6), idle}},

	{{"mission", eq(questValue(293), 6)}, {"Ah yes that scroll. Sadly me not could read it yet. But the holy banana me insight gave! In dreams Hairycles saw where to find solution ...", "Me saw a stone with lizard signs and other signs at once. If you read signs and tell Hairycles, me will know how to read signs ...", "You go east to big desert. In desert there city. East of city under sand hidden tomb is. You will have to dig until you find it, so take shovel ...", "Go down in tomb until come to big level and then go down another. There you find a stone with signs between two huge red stones ...", "Read it and return to me. Are you up to that challenge?", setTopic(8)}},

	{{"no", eq(TOPIC, 8)}, {"Me sad. Me expected better from you!", idle}},
	{{"yes", eq(TOPIC, 8)}, {"Good thing that is! Report about mission when you have read those signs.", setQuestValue(293, 7)}},

	{{"mission", eq(questValue(293), 7), eq(questValue(296), 0)}, {"You still don't know signs on stone, go and look for it in tomb east in desert."}},
	{{"mission", eq(questValue(293), 7), eq(questValue(296), 1)}, {"Ah yes, you read the signs in tomb? Good! May me look into your mind to see what you saw?", setTopic(9)}},

	{{"no", eq(TOPIC, 9)}, {"Me need to see it in your mind, other there is no way to proceed."}},
	{{"yes", eq(TOPIC, 9)}, {effectOpp(13), "Oh, so clear is all now! Easy it will be to read the signs now! Soon we will know what to do! Thank you again! Ask for mission if you feel ready.", setQuestValue(293, 8), idle}},

	{{"mission", eq(questValue(293), 8)}, {"So much there is to do for Hairycles to prepare charm that will protect all ape people ...", "You can help more. To create charm of life me need mighty token of life! Best is egg of a regenerating beast as a hydra is ...", "Bring me egg of hydra please. You may fight it in lair of Hydra at little lake south east of our lovely city Banuta! You think you can do?", setTopic(10)}},

	{{"no", eq(TOPIC, 10)}, {"Me sad. Me expected better from you!", idle}},
	{{"yes", eq(TOPIC, 10)}, {"You brave hairless ape! Get me hydra egg. If you lose egg, you probably have to fight many, many hydras to get another.", setQuestValue(293, 9)}},

	{{"mission", eq(questValue(293), 9)}, {setType(4839), setAmount(1), "You bring Hairycles egg of hydra?", setTopic(11)}},
	{{"no", eq(TOPIC, 11)}, {"Please hurry. Hairycles not knows when evil lizards strike again."}},
	{{"yes", eq(TOPIC, 11), lt(count(TYPE), AMOUNT)}, {"You not have egg of hydra. Please get one!"}},
	{{"yes", eq(TOPIC, 11), ge(count(TYPE), AMOUNT)}, {"Ah, the egg! Mighty warrior you be! Thank you. Hairycles will put it at safe place immediately.", delete(TYPE), setQuestValue(293, 10), idle}},

	{{"mission", eq(questValue(293), 10)}, {"Last ingredient for charm of life is thing to lure magic. Only thing me know like that is mushroom called witches' cap. Me was told it be found in isle called Fibula, where humans live ...", "Hidden under Fibula is a secret dungeon. There you will find witches' cap. Are you willing to go there for good ape people?", setTopic(12)}},

	{{"no", eq(TOPIC, 12)}, {"Me sad. Me expected better from you!", idle}},
	{{"yes", eq(TOPIC, 12)}, {"Long journey it will take, good luck to you.", setQuestValue(293, 11)}},

	{{"mission", eq(questValue(293), 11)}, {setType(4829), setAmount(1), "You brought Hairycles witches' cap from Fibula?", setTopic(18)}},
	{{"no", eq(TOPIC, 18)}, {"Please try to find me a witches' cap on Fibula."}},
	{{"yes", eq(TOPIC, 18), lt(count(TYPE), AMOUNT)}, {"Not right mushroom you have. Find me a witches' cap on Fibula!"}},
	{{"Yes", eq(TOPIC, 18), ge(count(TYPE), AMOUNT)}, {"Incredible, you brought a witches' cap! Now me can prepare mighty charm of life. Yet still other missions will await you, friend.", setQuestValue(293, 12), delete(TYPE)}},
	--###(begrüßung ändert sich nun)

	{{"mission", eq(questValue(293), 12)}, {"Mighty life charm is protecting us now! But my people are still in danger. Danger from within ...", "Some of my people try to mimic lizards to become strong. Like lizards did before, this cult drinks strange fluid that lizards left when fled ...", "Under the city still the underground temple of lizards is. There you find casks with red fluid. Take crowbar and destroy three of them to stop this madness. Are you willing to do that?", setTopic(13)}},

	{{"no", eq(TOPIC, 13)}, {"Me sad. Please reconsider."}},
	{{"yes", eq(TOPIC, 13)}, {"Hairycles sure you will make it. Good luck, friend.", setQuestValue(293, 13)}},

	{{"mission", eq(questValue(293), 13), lt(questValue(297), 3)}, {"Please destroy three casks in the complex beneath Banuta, so my people will come to senses again."}},
	{{"mission", eq(questValue(293), 13), gt(questValue(297), 2)}, {"You do please Hairycles again, friend. Me hope madness will not spread further now. Perhaps you are ready for other mission.", setQuestValue(293, 14)}},

	{{"mission", eq(questValue(293), 14)}, {"Now that the false cult was stopped, we need to strengthen the spirit of my people. We need a symbol of our faith that ape people can see and touch ...", "Since you have proven a friend of the ape people I will grant you permission to enter the forbidden land ...", "To enter the forbidden land in the north-east of the jungle, look for a cave in the mountains east of it. There you will find the blind prophet ...", "Tell him Hairycles you sent and he will grant you entrance ...", "Forbidden land is home of Bong. Holy giant ape big as mountain. Don't annoy him in any way but look for a hair of holy ape ...", "You might find at places he has been, should be easy to see them since Bong is big ...", "Return a hair of the holy ape to me. Will you do this for Hairycles?", setTopic(14)}},

	{{"no", eq(TOPIC, 14)}, {"Me sad. Please reconsider."}},
	{{"yes", eq(TOPIC, 14)}, {"Hairycles proud of you. Go and find holy hair. Good luck, friend.", setQuestValue(293, 15)}},

	{{"mission", eq(questValue(293), 15), eq(questValue(298), 0)}, {"Get a hair of holy ape from forbidden land in east. Speak with blind prophet in cave."}},
	{{"mission", eq(questValue(293), 15), eq(questValue(298), 1)}, {setType(4832), setAmount(1), "You brought hair of holy ape?", setTopic(15)}},

	{{"no", eq(TOPIC, 15)}, {"Go to forbidden land in east to find hair."}},
	{{"yes", eq(TOPIC, 15), lt(count(TYPE), AMOUNT)}, {"You no have hair. You lost it? Go and look again.", setQuestValue(298, 0)}},
	{{"yes", eq(TOPIC, 15), ge(count(TYPE), AMOUNT)}, {"Incredible! You got a hair of holy Bong! This will raise the spirit of my people. You are truly a friend. But one last mission awaits you.", setQuestValue(293, 16), delete(TYPE)}},

	{{"mission", eq(questValue(293), 16)}, {"You have proven yourself a friend, me will grant you permission to enter the deepest catacombs under Banuta which we have sealed in the past ...", "Me still can sense the evil presence there. We did not dare to go deeper and fight creatures of evil there ...", "You may go there, fight the evil and find the monument of the serpent god and destroy it with hammer me give to you ...", "Only then my people will be safe. Please tell Hairycles, will you go there?", setTopic(16)}},

	{{"no", eq(TOPIC, 16)}, {"Me sad. Please reconsider."}},
	{{"yes", eq(TOPIC, 16)}, {"Hairycles sure you will make it. Just use hammer on all that looks like snake or lizard. Tell Hairycles if you succeed with mission.", create(4835), setQuestValue(293, 17)}},

	{{"mission", eq(questValue(293), 17), eq(questValue(299), 0)}, {"Me know its much me asked for but go into the deepest catacombs under Banuta and destroy the monument of the serpent god."}},
	{{"mission", eq(questValue(293), 17), eq(questValue(299), 1)}, {"Finally my people are safe! You have done incredible good for ape people and one day even me brethren will recognise that ...", "I wish I could speak for all when me call you true friend but my people need time to get accustomed to change ...", "Let us hope one day whole Banuta will greet you as a friend. Perhaps you want to check me offers for special friends.", setQuestValue(293, 18)}},
	{{"mission", eq(questValue(293), 18)}, {"No more missions await you right now, friend. Perhaps you want to check me offers for special friends."}},

	--## ACHTUNG TOPIC 18 OBEN VERWENDET
	{{"offer", gt(questValue(293), 11), lt(questValue(293), 17)}, {"Me offer tasty bananas."}},
	{{"offer", gt(questValue(293), 17)}, {"Me offer tasty bananas. Me also sell statues of holy apes of wisdom. Statue of no talking, statue of no hearing, statue of no seeing."}},
	{{"furniture", gt(questValue(293), 17)}, {previous}},
	{{"goods", gt(questValue(293), 17)}, {previous}},
	{{"do", "you", "sell", gt(questValue(293), 17)}, {previous}},
	{{"do", "you", "have", gt(questValue(293), 17)}, {previous}},

	{{"statue", gt(questValue(293), 17)}, {"Me sell statues of holy apes of wisdom. Statue of no speaking, statue of no hearing, statue of no seeing."}},
	{{"speaking", gt(questValue(293), 17)}, {setType(5088), setAmount(1), setPrice(65), "You want buy this holy statue for %P gold?", setTopic(81)}},
	{{"seeing", gt(questValue(293), 17)}, {setType(5086), setAmount(1), setPrice(65), "You want buy this holy statue for %P gold?", setTopic(81)}},
	{{"hearing", gt(questValue(293), 17)}, {setType(5087), setAmount(1), setPrice(65), "You want buy this holy statue for %P gold?", setTopic(81)}},
	{{"banana", gt(questValue(293), 11)}, {setType(3587), setAmount(1), setPrice(2), "You want buy this banana for %P gold?", setTopic(81)}},
	{{P1, lt(1, P1), "banana", gt(questValue(293), 11)}, {setType(3587), setAmount(P1), setPrice(mul(P1, 2)), "You want buy %A bananas for %P gold?", setTopic(81)}},

	{{eq(TOPIC, 81), "yes", ge(COUNTMONEY, PRICE)}, {"Here is what you want.", deleteMoney, create(TYPE)}},
	{{eq(TOPIC, 81), "yes"}, {"Me sorry, you no money."}},
	{{eq(TOPIC, 81)}, {"As you whish, but no better in whole jungle you will find."}},
}

function onCreatureSay(npc, creature, text, situation)
	behaviour.react(npc, rules, text, situation)
end

function onCreatureDisappear(npc, creature)
	behaviour.react(npc, rules, "", NPC_SITUATION_VANISH)
end
