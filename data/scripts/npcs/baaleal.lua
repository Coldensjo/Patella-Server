-- npcs/baaleal.lua: the behaviour of the NPC Baa'leal, converted from
-- baaleal.npc by patella-convert. The NPC is its [[npc]] entry in npcs.toml;
-- lib/npc.lua matches and runs the rules below, in the order of the file
-- (scripting.hh, "NPCs"). The comments are those of baaleal.npc.
--
-- GIMUD - Graphical Interface Multi User Dungeon
-- baaleal.npc: Datenbank für den Efreetgeneral Baa'leal

local behaviour = require("lib.npc")
local ADDRESS = behaviour.ADDRESS
local BUSY = behaviour.BUSY
local VANISH = behaviour.VANISH
local FORCE = behaviour.FORCE
local eq = behaviour.eq
local lt = behaviour.lt
local gt = behaviour.gt
local TOPIC = behaviour.TOPIC
local questValue = behaviour.questValue
local setTopic = behaviour.setTopic
local setAmount = behaviour.setAmount
local burning = behaviour.burning
local setQuestValue = behaviour.setQuestValue
local effectMe = behaviour.effectMe
local effectOpp = behaviour.effectOpp
local create = behaviour.create
local queue = behaviour.queue
local idle = behaviour.idle
local nop = behaviour.nop
local previous = behaviour.previous

local rules = behaviour.rules {
	{{ADDRESS, eq(questValue(285), 0), "djanni'hah$", FORCE}, {"You know the code human! Very well then... What do you want, %N?", setQuestValue(285, 1)}},
	{{ADDRESS, eq(questValue(285), 0), FORCE}, {"A human! TAKE THIS!", setQuestValue(285, 1), burning(150, 4), effectOpp(5), effectMe(8), idle}},

	{{ADDRESS, "hello$", eq(questValue(278), 3), FORCE}, {"You are still alive, %N? Well, what do you want?"}},
	{{ADDRESS, "hi$", eq(questValue(278), 3), FORCE}, {previous}},
	{{ADDRESS, "greetings$", eq(questValue(278), 3), FORCE}, {previous}},
	{{ADDRESS, "djanni'hah$", eq(questValue(278), 3), FORCE}, {previous}},
	{{ADDRESS, FORCE}, {idle}},

	{{BUSY, "hello$", eq(questValue(278), 3), FORCE}, {"Can't you see I am already talking to somebody here, %N? You civilians don't understand the concept of discipline at all, do you!", queue}},
	{{BUSY, "hi$", eq(questValue(278), 3), FORCE}, {previous}},
	{{BUSY, "greetings$", eq(questValue(278), 3), FORCE}, {previous}},
	{{BUSY, "djanni'hah$", eq(questValue(278), 3), FORCE}, {previous}},
	{{BUSY, FORCE}, {nop}},

	{{VANISH}, {"Hail Malor!"}},

	{{"bye"}, {"Stand down, soldier!", idle}},
	{{"farewell"}, {previous}},
	{{"name"}, {"I'm general Baa'leal. What do you want in Mal'ouquah?"}},
	{{"general"}, {previous}},
	{{"baa'leal"}, {"That is GENERAL Baa'leal for you, human."}},
	{{"job"}, {"I am commander-in-chief of the armed forces of the UDLA, all branches of service. ...", "Hence I'm responsible for all operations in the enemy's territory."}},
	{{"udla"}, {"Yes. The United Djinn Liberation Army. ...", "The title has been given to our valiant armed forces in order to stress both the revolutionary focus of our agenda and the universalist nature of our political approach. ...", "Don't ask me what that means. Wasn't my idea."}},

	{{"ubaid", eq(questValue(286), 0), FORCE}, {"Ubaid told you to speak with me? Hmmm... maybe there is something you could help us with. Are you interested, human?", setTopic(1)}},
	{{"work", eq(questValue(286), 0), FORCE}, {previous}},
	{{"operation", eq(questValue(286), 0), FORCE}, {"Each mission and operation is a crucial step towards our victory! ...", "Now that we speak of it ...", "Since you are no djinn, there is something you could help us with. Are you interested, human?", setTopic(1)}},
	{{"mission", eq(questValue(286), 0), FORCE}, {previous}},

	{{eq(TOPIC, 1), "yes"}, {"Well ... All right. You may only be a human, but you do seem to have the right spirit. ...", "Listen! Since our base of operations is set in this isolated spot we depend on supplies from outside. These supplies are crucial for us to win the war. ...", "Unfortunately, it has happened that some of our supplies have disappeared on their way to this fortress. At first we thought it was the Marid, but intelligence reports suggest a different explanation. ...", "We now believe that a human was behind the theft! ...", "His identity is still unknown but we have been told that the thief fled to the human settlement called Carlin. I want you to find him and report back to me. Nobody messes with the Efreet and lives to tell the tale! ...", "Now go! Travel to the northern city Carlin! Keep your eyes open and look around for something that might give you a clue!", setQuestValue(286, 1)}},
	{{eq(TOPIC, 1)}, {"After all, you're just a human."}},

	{{"operation", gt(questValue(286), 0), lt(questValue(286), 3)}, {"Did you find the thief of our supplies?", setTopic(2)}},
	{{"mission", gt(questValue(286), 0), lt(questValue(286), 3)}, {previous}},
	{{"work", gt(questValue(286), 0), lt(questValue(286), 3)}, {previous}},
	{{"thief", gt(questValue(286), 0), lt(questValue(286), 3)}, {previous}},

	{{eq(TOPIC, 2), "yes"}, {"Finally! What is his name then?", setTopic(3)}},
	{{eq(TOPIC, 2)}, {"Then go to Carlin and search for him! Look for something that might give you a clue!"}},

	{{eq(TOPIC, 3), "partos", eq(questValue(286), 2), FORCE}, {"You found the thief! Excellent work, soldier! You are doing well - for a human, that is. Here - take this as a reward. ...", "Since you have proven to be a capable soldier, we have another mission for you. ...", "If you are interested go to Alesar and ask him about it.", setAmount(6), create(3035), setQuestValue(286, 3)}},
	{{eq(TOPIC, 3), FORCE}, {"Hmmm... I don't think so. Return to Carlin and continue your search!"}},

	{{"operation", eq(questValue(286), 3)}, {"Did you already talk to Alesar? He has another mission for you!"}},
	{{"mission", eq(questValue(286), 3)}, {previous}},

	{{"mal'ouquah"}, {"At the moment Mal'ouquah is our headquarter. However, I am already working on a cunning plan to move our base of operations deep into the enemy's territory."}},
	{{"ashta'daramai"}, {"Ashta'daramai is the enemy's base of operations. I am looking forward to the moment when we raise our flag there!"}},
	{{"gabel"}, {"He is weak. Much too weak to be our leader."}},
	{{"king"}, {"The UDLA does not serve a king because there isn't any. Of course, that is bound to change."}},
	{{"djinn"}, {"We are a race of warriors! We Efreets are destined to rule and to conquer."}},
	{{"efreet"}, {"We are the true djinn! We do not live in denial of our true nature like those damn liberals, the Marid."}},
	{{"marid"}, {"Nothing but a bunch of mealy-mouthed, mollycoddled wimps and milksops the lot of them. They may be superior in numbers, but we will win anyway because of our superior strategic thinking."}},
	{{"malor"}, {"Hail to our great leader!"}},
	{{"human"}, {"No offence, but your race is weak. You lack both the physical strength and the true warrior spirit. And worst of all, you have no strategic thinking."}},
	{{"zathroth"}, {"I understand he created us. Must have been a great general."}},
	{{"tibia"}, {"It is our mission to achieve total and decisive dominion of this world within two years. Well perhaps ... three. Always be realistic, that's what I say."}},
	{{"daraman"}, {"Damn that liberal peacenik, that treacherous mealy-mouthed double-faced good-for-nothing surrender monkey! ...", "He has infected this proud people's minds with his peace-for-all blabber."}},
	{{"darashia"}, {"The humans living in the northern deserts used to be nomads. Even though they are just humans they used to be respectable fighters. ...", "However, now they are living in this city they have grown fat and decadent. They will be easy prey."}},
	{{"scarab"}, {"Impressive animals. I have this idea of training them as battle steeds. Imagine this: Djinns mounted on scarabs! With a battalion of those I would crush the enemy in the blink of an eye!"}},
	{{"edron"}, {"They say the humans have built some big cities over there. I am looking forward to see them burn."}},
	{{"thais"}, {previous}},
	{{"venore"}, {previous}},
	{{"kazordoon"}, {previous}},
	{{"carlin"}, {previous}},
	{{"ab'dendriel"}, {previous}},
	{{"ankrahmun"}, {"That old city has some impressive defensive structures. But I swear I will bring it down one day... I have a cunning plan already! ...", "I am thinking of a huge wooden camel."}},
	{{"pharaoh"}, {"Ankrahmun's pharaoh apparently believes himself to be some sort of god. Ah well. A solid blow with my scimitar will bring him back to earth soon enough!"}},
	{{"palace"}, {"I suppose the palace is where the pharaoh resides. I have a distinct feeling I shall see it burn rather soon."}},
	{{"ascension"}, {"Apparently, ascension is what the followers of the pharaoh are after. No idea what exactly that is, though."}},
	{{"rah"}, {"Spare me that pseudo-theological hogwash."}},
	{{"uthun"}, {previous}},
	{{"akh"}, {previous}},

	{{"kha'zeel"}, {"It was an excellent idea to build our headquarter in the mountains of kha'zeel. Easy to defend, you know. Too bad the enemy had the same idea."}},
	{{"kha'labal"}, {"Kha'labal? Yes, it was me who devastated it. Couldn't leave it to the enemy, you see? We had to destroy it in order to save it!"}},
	{{"war"}, {"War is the father of things, and I live and breathe it. Ok, it's a tad bit silly that we are forced to fight against our own kind, but as a good soldier I will do my duty! <salutes> ...", "And if I hear anybody talking about 'peace' he will be court-martialled and summarily executed! Or vice versa!"}},
	{{"melchior"}, {"Melchior! I remember that greedy little civilian. I would have court-martialled him, but I suppose it is just as well the way it is."}},
	{{"alesar"}, {"Ah yes, Alesar! Excellent smith, that man!"}},
	{{"fa'hradin"}, {"He is Gabel's lieutenant and confidant. He is a powerful wizard, one has to admit that - and that's the only reason he is still alive. Without all his magical mumbo jumbo we would have long since won this war."}},
	{{"lamp"}, {"We sleep in those lamps. I like them - they are small and functional. We do not need cozy beds and fluffy duvets like decadent humans."}},

	--"mission", Questvalue(###)=### -> "A volunteer, hm? Well ... All right. You may only be a human, but you do seem to have the right spirit, and I like that. That's what we need around here!","Listen. Since our base of operations is set in this isolated spot we depend on supplies from outside. These supplies are crucial for us to win the war.","Unfortunately, it has happened that some of our supplies have disappeared on their way to this fortress. At first we thought it was the Marid, but intelligence reports suggest a different explanation. We now believe that a group of humans was behind the theft.","Unfortunately, we do not have much further specific information. All we know is that the thieves' hideout is somewhere in a northern city. However, you are a human, so you might stand a good chance to find those thieving jerks. I want them punished. Nobody messes with the Efreet and lives to tell the tale!","Now go! Travel to the northern cities! Look around for something that might give you a clue!", SetQuestValue
	--"mission", Questvalue(###)=### -> "You found the thieves? Excellent work, soldier! You are doing well - for a human, that is. Here - take this. Since you are not a regular member of the UDLA you have deserve some compensation.", Create (###,###), SetQuestValue(###,###)
	--"mission", Questvalue(###)=### -> "Still feeling adventurous, are we? Well, as I matter of fact there is something else you could do for us. Something very special.","Listen: We have sent a spy Ashta'Daramai, our enemy's fortress. He was on a mission to steal certain documents for us. Unfortunately, we have lost contact with him.","We must find out what happened to him and, more importantly, to the documents. Go to the to the fortress and try to find him. Above all, find the documents and bring them to me.","A word of advice: Our spy entered the fortress through a network of underground tunnels. Perhaps you should sneak into Ashta'Daramai using the same route. That way he should be easy to find.","Go now and don't come back without the documents.", SetQuestValue (###,###)
	--"mission", Questvalue(###)=### -> "Well, blast my buttocks with a blunderbuss###! The documents! Outstanding work, soldier! Malor will be pleased!","It appears you do deserve our trust after all. Tell you what, soldier - I will order that stubborn Alesar to fully cooperate with you. He's got some pretty nice items to sell. He won't like it, of course, but I'm sure he will do as he is told. He is a namby-pamby civilian after all.","Oh, and ask him about a mission, too. I think he and Malor were talking about a plan.", SetQuestValue(###,###)
}

function onCreatureSay(npc, creature, text, situation)
	behaviour.react(npc, rules, text, situation)
end

function onCreatureDisappear(npc, creature)
	behaviour.react(npc, rules, "", NPC_SITUATION_VANISH)
end
