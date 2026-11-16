-- npcs/alesar.lua: the behaviour of the NPC Alesar, converted from
-- alesar.npc by patella-convert. The NPC is its [[npc]] entry in npcs.toml;
-- lib/npc.lua matches and runs the rules below, in the order of the file
-- (scripting.hh, "NPCs"). The comments are those of alesar.npc.
--
-- GIMUD - Graphical Interface Multi User Dungeon
-- alesar.npc: Datenbank für den Djinnschmied Alesar (Waffen und Rüstungen, Efreet)

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
local COUNTMONEY = behaviour.COUNTMONEY
local count = behaviour.count
local questValue = behaviour.questValue
local mul = behaviour.mul
local setTopic = behaviour.setTopic
local setPrice = behaviour.setPrice
local setAmount = behaviour.setAmount
local setType = behaviour.setType
local setQuestValue = behaviour.setQuestValue
local create = behaviour.create
local delete = behaviour.delete
local createMoney = behaviour.createMoney
local deleteMoney = behaviour.deleteMoney
local queue = behaviour.queue
local idle = behaviour.idle
local nop = behaviour.nop
local previous = behaviour.previous

local rules = behaviour.rules {
	{{ADDRESS, "hello$", eq(questValue(278), 3), FORCE}, {"What do you want from me, %N?"}},
	{{ADDRESS, "hi$", eq(questValue(278), 3), FORCE}, {previous}},
	{{ADDRESS, "greetings$", eq(questValue(278), 3), FORCE}, {previous}},
	{{ADDRESS, "djanni'hah$", eq(questValue(278), 3), FORCE}, {previous}},
	{{ADDRESS, FORCE}, {idle}},
	{{BUSY, "hello$", eq(questValue(278), 3), FORCE}, {"I am already talking to one of you creeps. So shut up until it is your turn, %N.", queue}},
	{{BUSY, "hi$", eq(questValue(278), 3), FORCE}, {previous}},
	{{BUSY, "greetings$", eq(questValue(278), 3), FORCE}, {previous}},
	{{BUSY, "djanni'hah$", eq(questValue(278), 3), FORCE}, {previous}},
	{{BUSY, FORCE}, {nop}},
	{{VANISH}, {"Humans..."}},

	{{"bye"}, {"Finally.", idle}},
	{{"farewell"}, {previous}},
	{{"name"}, {"My name is none of your business."}},
	{{"alesar"}, {"That is my name. So what!"}},
	{{"job"}, {"What does it look like, fool? I'm a smith! But I won't sell you anything until Malor orders me otherwise."}},
	{{"trade"}, {"I won't sell you anything, human. Malor doesn't want me to trade with strangers."}},
	{{"permission"}, {previous}},

	{{"daraman"}, {"Don't you dare mention Daraman in my presence, human. I am through with his insidious lies and through with your accursed race!"}},
	{{"haroun"}, {"Haroun? What? You know ... where do you know that name from? Did he send you?", setTopic(1)}},
	{{eq(TOPIC, 1), "yes"}, {"Treacherous liar! You would not be here if you had really talked to him. Get out of my sight or I will test my latest sabre on you!", idle}},
	{{eq(TOPIC, 1)}, {"<sighs> Of course not. How could you ... Well, at least you are honest, human. I appreciate that."}},

	{{"human"}, {"I used to have illusions about you humans. I thought humans were good, noble creatures. ...", "I thought djinns and humans shared a destiny, and that we could live side by side peacefully. ...", "But now I have learnt my lesson. I have had the privilege to look deep into the human mind, much deeper than most of my brothers. ...", "And guess what! I did not like what I see. You are nothing but a race of cruel, perfidious bloodsuckers who hide their wickedness behind a thin layer of civilisation and so-called humanity. ...", "Your race is a blemish on the face of Tibia. The sooner it is gone the better!"}},
	{{"djinn"}, {"One day we will teach your race a lesson it will never forget."}},
	{{"efreet"}, {"The efreet are those djinn who never fell for Daraman's insidious propaganda. I wish I would have been as smart from the start. ...", "But errors can be corrected!"}},
	{{"marid"}, {"Those among my brothers and sisters who still do not see the truth call themselves the Marid. I used to be one of them, but I left them when the truth dawned upon me. ...", "Now I follow Malor, although I would never fight against my kind."}},
	{{"gabel"}, {"Gabel is a kind-hearted, honest djinn. I would hate to see him die just because he believes in Daraman's lies. ...", "After all, I believed them myself. "}},
	{{"king"}, {"We need a strong king to unite us in our struggle against the humans."}},
	{{"malor"}, {"Malor is overambitious and unnecessarily cruel, but he is the only djinn who could unite our race, so I follow him. ...", "The truth is I despise him, but that is of no importance as long as you humans will be exterminated."}},
	{{"mal'ouquah"}, {"I do not like this place. But then it does not really matter where I am. I have a forge and I don't see any humans. That's all I need. ...", "Of course, now you are here. Doesn't help me to feel myself at home here."}},
	{{"ashta'daramai", eq(questValue(287), 0)}, {"I used to live in Ashta'daramai. That was before I realised the extent of my blindness."}},
	{{"ashta'daramai", gt(questValue(287), 0)}, {"Ashta'daramai is Gabel's fortress which lies to the north. ...", "Of course you cannot enter it through the front door. ...", "But from my time there, I know that there is also an unguarded back door in the north of the fortress."}},
	{{"zathroth"}, {"Legend has it that Zathroth was trying to make us beings of unalloyed evil, but he found us to be impure, so he abandoned us and started over. ...", "It is not flattering to think we are nothing but examples of bad workmanship, but I see it from a different perspective: Since our god left us on our own it is up to ourselves to forge our destiny. ...", "One day Zathroth will look at us in amazement."}},
	{{"tibia"}, {"One day we djinn will rid this world of evil."}},
	{{"darashia"}, {"I don't care about human cities. If I had my way, they would all be burnt to down today."}},
	{{"edron"}, {previous}},
	{{"thais"}, {previous}},
	{{"venore"}, {previous}},
	{{"kazordoon"}, {previous}},
	{{"carlin"}, {previous}},
	{{"ab'dendriel"}, {previous}},
	{{"ankrahmun"}, {previous}},

	{{"pharaoh"}, {"The pharaoh in Ankrahmun is a dangerous fool. Just your typical human, in other words."}},
	{{"palace"}, {"So?"}},
	{{"ascension"}, {"What are you talking about? More human pseudo-philosophical flapdoodle?"}},
	{{"rah"}, {previous}},
	{{"uthun"}, {previous}},
	{{"akh"}, {previous}},
	{{"scarab"}, {"I like them. They are peaceful, but if they are provoked they fight ferociously. And they are know to eat humans!"}},
	{{"kha'zeel"}, {"These mountains are our refuge from those pesky humans. Too bad there are always some who come up here anyway. You, for example."}},
	{{"kha'labal"}, {"The desert Kha'labal was once a beautiful land, but it was devastated in the course of the war. Damn humans! This is all your fault!"}},
	{{"melchior"}, {"I remember him. He was a greedy, double-dealing hyena. As far as I know his bleached bones are now lying somewhere in the Kha'labal."}},
	{{"djema"}, {"Djema? Well - I suppose she is the only human I still like. But she has been brought up by djinns. Who knows - perhaps humans can learn."}},
	{{"baa'leal"}, {"Baa'leal is Malor's lieutenant. Unflinchingly loyal, but not quite as clever as he thinks he is."}},
	{{"bo'ques"}, {"I miss Bo'ques' cooking, but not his pompous airs and graces."}},
	{{"fa'hradin"}, {"Fa'hradin, that old cynic is way too smart to believe in Daraman's lies. He should reconsider his loyalties."}},

	{{"wares"}, {"I sell and buy weapons, armors, helmets, legs, and shields."}},
	{{"offer"}, {previous}},
	{{"goods"}, {previous}},
	{{"smith"}, {previous}},
	{{"equipment"}, {previous}},
	{{"do", "you", "sell"}, {previous}},
	{{"do", "you", "have"}, {previous}},

	{{"weapon"}, {"At this time I'm only selling ice rapiers and serpent swords. But I would buy scimitars, giant swords, serpent swords, poison daggers, knight axes, dragon hammers and skull staffs from you."}},
	{{"shield"}, {"I am selling only ancient shields. But I buy tower shields, black shields, ancient shields and vampire shields."}},
	{{"armor"}, {"I am buying and selling dark armors. But I would also buy a knight armor from you."}},
	{{"helmet"}, {"I am buying and selling dark helmets. Furthermore I'm buying warrior helmets, strange helmets and mystic turbans."}},
	{{"trousers"}, {"At this time I'm only buying knight legs."}},
	{{"legs"}, {previous}},

	{{"ice", "rapier"}, {setType(3284), setAmount(1), setPrice(5000), "Do you want to buy an ice rapier for %P gold?", setTopic(10)}},
	{{"serpent", "sword"}, {setType(3297), setAmount(1), setPrice(6000), "Do you want to buy a serpent sword for %P gold?", setTopic(10)}},
	{{"ancient", "shield"}, {setType(3432), setAmount(1), setPrice(5000), "Do you want to buy an ancient shield for %P gold?", setTopic(10)}},
	{{"dark", "armor"}, {setType(3383), setAmount(1), setPrice(1500), "Do you want to buy a dark armor for %P gold?", setTopic(10)}},
	{{"dark", "helmet"}, {setType(3384), setAmount(1), setPrice(1000), "Do you want to buy a dark helmet for %P gold?", setTopic(10)}},

	{{P1, lt(1, P1), "ice", "rapier"}, {setType(3284), setAmount(P1), setPrice(mul(5000, P1)), "Do you want to buy %A ice rapiers for %P gold?", setTopic(10)}},
	{{P1, lt(1, P1), "serpent", "sword"}, {setType(3297), setAmount(P1), setPrice(mul(6000, P1)), "Do you want to buy %A serpent swords for %P gold?", setTopic(10)}},
	{{P1, lt(1, P1), "ancient", "shield"}, {setType(3432), setAmount(P1), setPrice(mul(5000, P1)), "Do you want to buy %A ancient shields for %P gold?", setTopic(10)}},
	{{P1, lt(1, P1), "dark", "armor"}, {setType(3383), setAmount(P1), setPrice(mul(1500, P1)), "Do you want to buy %A dark armors for %P gold?", setTopic(10)}},
	{{P1, lt(1, P1), "dark", "helmet"}, {setType(3384), setAmount(P1), setPrice(mul(1000, P1)), "Do you want to buy %A dark helmets for %P gold?", setTopic(10)}},

	{{"sell", "serpent", "sword"}, {setType(3297), setAmount(1), setPrice(900), "Do you want to sell a serpent sword for %P gold?", setTopic(11)}},
	{{"sell", "dragon", "hammer"}, {setType(3322), setAmount(1), setPrice(2000), "Do you want to sell a dragon hammer for %P gold?", setTopic(11)}},
	{{"sell", "giant", "sword"}, {setType(3281), setAmount(1), setPrice(17000), "Do you want to sell a giant sword for %P gold?", setTopic(11)}},
	{{"sell", "poison", "dagger"}, {setType(3299), setAmount(1), setPrice(50), "Do you want to sell a poison dagger for %P gold?", setTopic(11)}},
	{{"sell", "scimitar"}, {setType(3307), setAmount(1), setPrice(150), "Do you want to sell a scimitar for %P gold?", setTopic(11)}},
	{{"sell", "skull", "staff"}, {setType(3324), setAmount(1), setPrice(6000), "Do you want to sell a skull staff for %P gold?", setTopic(11)}},
	{{"sell", "knight", "axe"}, {setType(3318), setAmount(1), setPrice(2000), "Do you want to sell a knight axe for %P gold?", setTopic(11)}},

	{{"sell", "tower", "shield"}, {setType(3428), setAmount(1), setPrice(8000), "Do you want to sell a tower shield for %P gold?", setTopic(11)}},
	{{"sell", "black", "shield"}, {setType(3429), setAmount(1), setPrice(800), "Do you want to sell a black shield for %P gold?", setTopic(11)}},
	{{"sell", "ancient", "shield"}, {setType(3432), setAmount(1), setPrice(900), "Do you want to sell an ancient shield for %P gold?", setTopic(11)}},
	{{"sell", "vampire", "shield"}, {setType(3434), setAmount(1), setPrice(15000), "Do you want to sell a vampire shield for %P gold?", setTopic(11)}},

	{{"sell", "warrior", "helmet"}, {setType(3369), setAmount(1), setPrice(5000), "Do you want to sell a warrior helmet for %P gold?", setTopic(11)}},
	{{"sell", "knight", "armor"}, {setType(3370), setAmount(1), setPrice(5000), "Do you want to sell a knight armor for %P gold?", setTopic(11)}},
	{{"sell", "knight", "legs"}, {setType(3371), setAmount(1), setPrice(5000), "Do you want to sell a pair of knight legs for %P gold?", setTopic(11)}},
	{{"sell", "strange", "helmet"}, {setType(3373), setAmount(1), setPrice(500), "Do you want to sell a strange helmet for %P gold?", setTopic(11)}},
	{{"sell", "dark", "armor"}, {setType(3383), setAmount(1), setPrice(400), "Do you want to sell a dark armor for %P gold?", setTopic(11)}},
	{{"sell", "dark", "helmet"}, {setType(3384), setAmount(1), setPrice(250), "Do you want to sell a dark helmet for %P gold?", setTopic(11)}},
	{{"sell", "mystic", "turban"}, {setType(3574), setAmount(1), setPrice(150), "Do you want to sell a mystic turban for %P gold?", setTopic(11)}},

	{{"sell", P1, lt(1, P1), "serpent", "sword"}, {setType(3297), setAmount(P1), setPrice(mul(900, P1)), "Do you want to sell %A serpent swords for %P gold?", setTopic(11)}},
	{{"sell", P1, lt(1, P1), "dragon", "hammer"}, {setType(3322), setAmount(P1), setPrice(mul(2000, P1)), "Do you want to sell %A dragon hammers for %P gold?", setTopic(11)}},
	{{"sell", P1, lt(1, P1), "giant", "sword"}, {setType(3281), setAmount(P1), setPrice(mul(17000, P1)), "Do you want to sell %A giant swords for %P gold?", setTopic(11)}},
	{{"sell", P1, lt(1, P1), "poison", "dagger"}, {setType(3299), setAmount(P1), setPrice(mul(50, P1)), "Do you want to sell %A poison daggers for %P gold?", setTopic(11)}},
	{{"sell", P1, lt(1, P1), "scimitar"}, {setType(3307), setAmount(P1), setPrice(mul(150, P1)), "Do you want to sell %A scimitars for %P gold?", setTopic(11)}},
	{{"sell", P1, lt(1, P1), "skull", "staff"}, {setType(3324), setAmount(P1), setPrice(mul(6000, P1)), "Do you want to sell %A skull staffs for %P gold?", setTopic(11)}},
	{{"sell", P1, lt(1, P1), "knight", "axe"}, {setType(3318), setAmount(P1), setPrice(mul(2000, P1)), "Do you want to sell %A knight axes for %P gold?", setTopic(11)}},

	{{"sell", P1, lt(1, P1), "tower", "shield"}, {setType(3428), setAmount(P1), setPrice(mul(8000, P1)), "Do you want to sell %A tower shields for %P gold?", setTopic(11)}},
	{{"sell", P1, lt(1, P1), "black", "shield"}, {setType(3429), setAmount(P1), setPrice(mul(800, P1)), "Do you want to sell %A black shields for %P gold?", setTopic(11)}},
	{{"sell", P1, lt(1, P1), "ancient", "shield"}, {setType(3432), setAmount(P1), setPrice(mul(900, P1)), "Do you want to sell %A ancient shields for %P gold?", setTopic(11)}},
	{{"sell", P1, lt(1, P1), "vampire", "shield"}, {setType(3434), setAmount(P1), setPrice(mul(15000, P1)), "Do you want to sell %A vampire shields for %P gold?", setTopic(11)}},

	{{"sell", P1, lt(1, P1), "strange", "helmet"}, {setType(3373), setAmount(P1), setPrice(mul(500, P1)), "Do you want to sell %A strange helmets for %P gold?", setTopic(11)}},
	{{"sell", P1, lt(1, P1), "dark", "armor"}, {setType(3383), setAmount(P1), setPrice(mul(400, P1)), "Do you want to sell %A dark armors for %P gold?", setTopic(11)}},
	{{"sell", P1, lt(1, P1), "dark", "helmet"}, {setType(3384), setAmount(P1), setPrice(mul(250, P1)), "Do you want to sell %A dark helmets for %P gold?", setTopic(11)}},
	{{"sell", P1, lt(1, P1), "warrior", "helmet"}, {setType(3369), setAmount(P1), setPrice(mul(5000, P1)), "Do you want to sell %A warrior helmets for %P gold?", setTopic(11)}},
	{{"sell", P1, lt(1, P1), "knight", "armor"}, {setType(3370), setAmount(P1), setPrice(mul(5000, P1)), "Do you want to sell %A knight armors for %P gold?", setTopic(11)}},
	{{"sell", P1, lt(1, P1), "knight", "legs"}, {setType(3371), setAmount(P1), setPrice(mul(5000, P1)), "Do you want to sell %A pairs of knight legs for %P gold?", setTopic(11)}},
	{{"sell", P1, lt(1, P1), "mystic", "turban"}, {setType(3574), setAmount(P1), setPrice(mul(150, P1)), "Do you want to sell %A mystic turbans for %P gold?", setTopic(11)}},

	{{eq(TOPIC, 10), lt(questValue(288), 3), FORCE}, {"No chance, human. Malor doesn't want me to trade with strangers."}},
	{{eq(TOPIC, 10), "yes", ge(COUNTMONEY, PRICE)}, {"Thank you. Here you are.", deleteMoney, create(TYPE)}},
	{{eq(TOPIC, 10), "yes"}, {"You do not have enough gold."}},
	{{eq(TOPIC, 10)}, {"Well, obviously not."}},

	{{eq(TOPIC, 11), lt(questValue(288), 3), FORCE}, {"No chance, human. Malor doesn't want me to trade with strangers."}},
	{{eq(TOPIC, 11), "yes", ge(count(TYPE), AMOUNT)}, {"Ok. Here is your gold.", delete(TYPE), createMoney}},
	{{eq(TOPIC, 11), "yes"}, {"You don't have one."}},
	{{eq(TOPIC, 11), "yes", gt(AMOUNT, 1)}, {"You don't have that many."}},
	{{eq(TOPIC, 11)}, {"Well, obviously not."}},

	{{"mission", eq(questValue(286), 3), eq(questValue(287), 0)}, {"So Baa'leal thinks you are up to do a mission for us? ...", "I think he is getting old, entrusting human scum such as you are with an important mission like that. ...", "Personally, I don't understand why you haven't been slaughtered right at the gates. ...", "Anyway. Are you prepared to embark on a dangerous mission for us?", setTopic(2)}},
	{{"baa'leal", eq(questValue(286), 3), eq(questValue(287), 0)}, {previous}},

	{{eq(TOPIC, 2), "yes"}, {"All right then, human. Have you ever heard of the 'Tears of Daraman'? ...", "They are precious gemstones made of some unknown blue mineral and possess enormous magical power. ...", "If you want to learn more about these gemstones don't forget to visit our library. ...", "Anyway, one of them is enough to create thousands of our mighty djinn blades. ...", "Unfortunately my last gemstone broke and therefore I'm not able to create new blades anymore. ...", "To my knowledge there is only one place where you can find these gemstones - I know for a fact that the Marid have at least one of them. ...", "Well... to cut a long story short, your mission is to sneak into Ashta'daramai and to steal it. ...", "Needless to say, the Marid won't be too eager to part with it. Try not to get killed until you have delivered the stone to me.", setQuestValue(287, 1)}},
	{{eq(TOPIC, 2)}, {"Then not."}},

	{{"mission", gt(questValue(287), 0), lt(questValue(287), 3)}, {"Did you find the tear of Daraman?", setTopic(3)}},
	{{"gem", gt(questValue(287), 0), lt(questValue(287), 3)}, {previous}},
	{{"tear", gt(questValue(287), 0), lt(questValue(287), 3)}, {previous}},

	{{eq(TOPIC, 3), "yes", eq(questValue(287), 2), gt(count(3233), 0)}, {"So you have made it? You have really managed to steal a Tear of Daraman? ...", "Amazing how you humans are just impossible to get rid of. Incidentally, you have this character trait in common with many insects and with other vermin. ...", "Nevermind. I hate to say it, but it you have done us a favour, human. That gemstone will serve us well. ...", "Baa'leal, wants you to talk to Malor concerning some new mission. ...", "Looks like you have managed to extended your life expectancy - for just a bit longer.", setAmount(1), delete(3233), setQuestValue(287, 3)}},
	{{eq(TOPIC, 3)}, {"As I expected. You haven't got the stone. Shall I explain your mission again?", setTopic(2)}},

	{{"mission", eq(questValue(287), 3)}, {"Don't forget to talk to Malor concerning your next mission."}},
	{{"work", eq(questValue(287), 3)}, {previous}},
}

function onCreatureSay(npc, creature, text, situation)
	behaviour.react(npc, rules, text, situation)
end

function onCreatureDisappear(npc, creature)
	behaviour.react(npc, rules, "", NPC_SITUATION_VANISH)
end
