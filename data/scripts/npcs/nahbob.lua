-- npcs/nahbob.lua: the behaviour of the NPC Nah'bob, converted from
-- nahbob.npc by patella-convert. The NPC is its [[npc]] entry in npcs.toml;
-- lib/npc.lua matches and runs the rules below, in the order of the file
-- (scripting.hh, "NPCs"). The comments are those of nahbob.npc.
--
-- GIMUD - Graphical Interface Multi User Dungeon
-- nahbob.npc: Datenbank für den Maridhändler Nah'bob (Waffen und Rüstungen, Marid)

local behaviour = require("lib.npc")
local ADDRESS = behaviour.ADDRESS
local BUSY = behaviour.BUSY
local VANISH = behaviour.VANISH
local MALE = behaviour.MALE
local FEMALE = behaviour.FEMALE
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
local create = behaviour.create
local delete = behaviour.delete
local createMoney = behaviour.createMoney
local deleteMoney = behaviour.deleteMoney
local queue = behaviour.queue
local idle = behaviour.idle
local nop = behaviour.nop
local previous = behaviour.previous

local rules = behaviour.rules {
	{{ADDRESS, "hello$", eq(questValue(278), 2), FORCE}, {"<Sighs> Another customer! I've only just sat down! What is it, %N?"}},
	{{ADDRESS, "hi$", eq(questValue(278), 2), FORCE}, {previous}},
	{{ADDRESS, "greetings$", eq(questValue(278), 2), FORCE}, {previous}},
	{{ADDRESS, "djanni'hah$", eq(questValue(278), 2), FORCE}, {previous}},
	{{ADDRESS, FORCE}, {idle}},
	{{BUSY, "hello$", eq(questValue(278), 2), FORCE}, {"Whoa, %N! Easy! Old Bob is busy right now!", queue}},
	{{BUSY, "hi$", eq(questValue(278), 2), FORCE}, {previous}},
	{{ADDRESS, "greetings$", eq(questValue(278), 2), FORCE}, {previous}},
	{{BUSY, "djanni'hah$", eq(questValue(278), 2), FORCE}, {previous}},
	{{BUSY, FORCE}, {nop}},
	{{VANISH}, {"Bye then."}},

	{{"bye"}, {"Bye now. Visit old Bob again one day!", idle}},
	{{"farewell"}, {previous}},
	{{"name"}, {"Well, my name is Nah'bob, but I don't like it. Just call me Bob."}},
	{{"Bob"}, {"Yep, that's me."}},
	{{"nah'bob"}, {"I don't like that name. Call me Bob."}},
	{{"job", MALE}, {"I'm a trader, mate. You know... selling stuff, buying stuff... that kind of thing. Quite a hassle, actually. ...", "If you have the permission to trade with us, check out my latest offers."}},
	{{"job", FEMALE}, {"I'm a trader, lady. You know... selling stuff, buying stuff... that kind of thing. Quite a hassle, actually. ...", "If you have the permission to trade with us, check out my latest offers."}},
	{{"trade", MALE}, {"You want to buy something? Well, be my guest, Mister. Just ask me for my offers!"}},
	{{"trade", FEMALE}, {"You want to buy something? Well, be my guest, Lady. Just ask me for my offers!"}},
	{{"permission"}, {"I am not allowed to trade with you unless Gabel gave you the permission to trade with us."}},

	{{"gabel"}, {"He's the boss around here. You know - the big djinn. He's all right."}},
	{{"king"}, {"King? We do not have kings. Not anymore."}},
	{{"djinn"}, {"Yes, I am a djinn. How long did it take you to notice?"}},
	{{"efreet"}, {"The Efreet are a bunch of freaked out loonies, believe me. Nothing but 'war!' and 'domination!' and 'world rule!' all day long. Those people shoud chill out a bit."}},
	{{"marid"}, {"I guess I am one. I don't know why because I'm not really into all that Daraman stuff about ascetism, you know. ...", "But then, I don't like the Efreets' gibberish about war and world domination, either. ...", "In the end I chose the side which seemed more tolerant towards attitudes that are a bit... different. And that was clearly the Marid. ...", "Besides, Bo'ques chose to side with Gabel. And I would have hated never to eat his cooking again. Did you ever try his Chat d'Aramignon?"}},
	{{"malor"}, {"Now THAT djnn really means stress. Too bad that doofus of an orc king freed him from his prison in the lamp. Gabel should have gotten rid of Malor for good when had the chance to do so."}},
	{{"mal'ouquah"}, {"Mal'ouquah the Efreet's fortress. I have never seen it, and I don't feel any particular desire to do so."}},
	{{"ashta'daramai"}, {"Ashta'daramai is the name of this place - 'Daraman's gift'. It's actually a bit misleading, because it was not Daraman who built it. The name refer's to Daraman's teachings, you know."}},
	{{"human"}, {"I have nothing against humans. ...", "Sure, there are always those who want to steal from you. ...", "Or blackmail you. ...", "Or torture you. ...", "Or just kill you for no reason. ...", "But all in all I think humans are amusing little people."}},
	{{"zathroth"}, {"They say Zathroth created us because he wanted us to be mindless, greedy killing machines. Well, I'm not so sure about some of the Efreet, but as for me I think he's done a lousy job."}},
	{{"tibia"}, {"It's a mad, mad world."}},
	{{"daraman"}, {"Ah yes. Daraman. The old prophet. Well - he was all right, I guess. But I am not sure I agree with him on all matters. I mean - that thing about asceticism... I think he was a bit extreme there."}},
	{{"darashia"}, {"They say Darashia is full of relaxed people. To me that sounds like a good place to be."}},
	{{"scarab"}, {"Yuk! I wouldn't eat that!"}},
	{{"edron"}, {"You are talking about some human place, aren't you?"}},
	{{"thais"}, {previous}},
	{{"venore"}, {previous}},
	{{"kazordoon"}, {previous}},
	{{"carlin"}, {previous}},
	{{"ab'dendriel"}, {previous}},
	{{"ankrahmun"}, {"I know Ankrahmun - I've been there before. Lots of pyramids and stuff. Wicked."}},
	{{"pharaoh"}, {"The pharaoh? I have heard about that guy. That man is as mad as a march hare."}},
	{{"palace"}, {"The palace in Ankrahmun used to be a place of orgies and of all kinds of debauchery. Aah - those were the days...!"}},
	{{"ascension"}, {"Yeah right. I don't know what it is about you humans. I have rarely met a human who did not have his head filled with some crazy religious idea."}},
	{{"kha'zeel"}, {"I like the Kha'zeel mountains. It is true they are boring as hell, but personally I don't mind boredom. I hate excitement. Whenever somebody said 'Oh-that's-gonna-be-so-exciting' some really, really bad thing happened."}},
	{{"kha'labal"}, {"Did you know that desert used to be fertile land? You should have seen it. It was all lush and verdant - a veritable paradise. ...", "There were flowers and birds everywhere, and more delicious food anybody could ever eat. ...", "But then that blockhead Baa'leal decided it was a good idea to burn it all down. Damn those stupid Efreet!"}},
	{{"melchior"}, {"Of course I remember old Melchior. He used to come up here regularly. Is he dead?", setTopic(1)}},
	{{eq(TOPIC, 1), "yes"}, {"Oh. Well, I suppose that is bad. For him, anyway."}},
	{{eq(TOPIC, 1), "no"}, {"So the old skinflint is still alive? Amazing! I could have sworn the Efreet knocked the stuffing out of him. No human can fool a djinn for long, you know."}},
	{{"alesar"}, {"I know Alesar. Ran off to the Efreet, didn't he? Weird story. Always used to be such fanatic followers of Daraman, he and his brother. ...", "I guess that happens when you get all freaky about some silly idea. I'm sure that wouldn't have happened if he had taken a more relaxed approach to things. ...", "I remember how I always told him to chill out. But he was stubborn as a headstrong donkey. Just like his brother."}},
	{{"brother"}, {"Haroun is Alesar's brother. Don't talk to him about it, though. Haroun may be a zealous fool, but he's all right. He has suffered enough."}},
	{{"fa'hradin"}, {"I think Fa'hradin has stopped taking things too seriously long time ago. You just got to love the old cynic."}},
	{{"bo'ques"}, {"Honestly, if I did not have any other reason for hanging out here Bo'ques's cooking would make me stay."}},
	{{"djem"}, {"The little girl djema is bored out of her mind here so she often comes round. We've always had a barrel of laughs together, she and me - playing chess, telling stories, playing tricks on Haroun. ...", "I really like her. I will miss her lots when she's gone. <clears his throat> Hey! Don't tell her I have said that, ok?"}},
	{{"haroun"}, {"Ah yes - Haroun. My esteemed fellow trader. Stiff like a butler and boring like a tax collector. He is a devout follower of Daraman, but I think he seriously misunderstood his teachings. ...", "I don't think it was the old prophet's intention to rid this world of laughter and of smiles."}},
	{{"baa'leal"}, {"Baa'leal? Of course I remember old bulldog face. I wasn't surprised at all when I heard that he sided with Malor. Wasn't a great loss anyway. ...", "He may be one mean djinn, but he is thicker than a senile ox. To think Malor made him his lieutenant <laughs>!"}},

	{{"wares"}, {"My job around here is to buy and sell weapons, armors, helmets, legs, and shields."}},
	{{"offer"}, {previous}},
	{{"goods"}, {previous}},
	{{"equipment"}, {previous}},
	{{"do", "you", "sell"}, {previous}},
	{{"do", "you", "have"}, {previous}},

	{{"weapon"}, {"I'm selling and buying spike swords, war hammers and obsidian lances. Furthermore I would buy fire swords, ice rapiers, dragon lances, fire axes and broadswords from you, if you have any."}},
	{{"shield"}, {"I'm just selling beholder shields. But I'm buying guardian shields, dragon shields, beholder shields, crown shields and phoenix shields."}},
	{{"armor"}, {"I'm selling and buying noble armors. Furthermore I'm buying crown armors and blue robes."}},
	{{"helmet"}, {"At this time I'm not selling any helmets. I'm only buying crown helmets, crusader helmets and royal helmets."}},
	{{"trousers"}, {"At this time I'm only buying crown legs. Oh, and I'm also looking for boots of haste!"}},
	{{"legs"}, {previous}},

	{{"spike", "sword"}, {setType(3271), setAmount(1), setPrice(8000), "Do you want to buy a spike sword for %P gold?", setTopic(10)}},
	{{"war", "hammer"}, {setType(3279), setAmount(1), setPrice(10000), "Do you want to buy a war hammer for %P gold?", setTopic(10)}},
	{{"obsidian", "lance"}, {setType(3313), setAmount(1), setPrice(3000), "Do you want to buy an obsidian lance for %P gold?", setTopic(10)}},
	{{"beholder", "shield"}, {setType(3418), setAmount(1), setPrice(7000), "Do you want to buy a beholder shield for %P gold?", setTopic(10)}},
	{{"noble", "armor"}, {setType(3380), setAmount(1), setPrice(8000), "Do you want to buy a noble armor for %P gold?", setTopic(10)}},

	{{P1, lt(1, P1), "spike", "sword"}, {setType(3271), setAmount(P1), setPrice(mul(8000, P1)), "Do you want to buy %A spike swords for %P gold?", setTopic(10)}},
	{{P1, lt(1, P1), "war", "hammer"}, {setType(3279), setAmount(P1), setPrice(mul(10000, P1)), "Do you want to buy %A war hammers for %P gold?", setTopic(10)}},
	{{P1, lt(1, P1), "obsidian", "lance"}, {setType(3313), setAmount(P1), setPrice(mul(3000, P1)), "Do you want to buy %A obsidian lances for %P gold?", setTopic(10)}},
	{{P1, lt(1, P1), "beholder", "shield"}, {setType(3418), setAmount(P1), setPrice(mul(7000, P1)), "Do you want to buy %A beholder shields for %P gold?", setTopic(10)}},
	{{P1, lt(1, P1), "noble", "armor"}, {setType(3380), setAmount(P1), setPrice(mul(8000, P1)), "Do you want to buy %A noble armors for %P gold?", setTopic(10)}},

	{{"sell", "spike", "sword"}, {setType(3271), setAmount(1), setPrice(1000), "Do you want to sell a spike sword for %P gold?", setTopic(11)}},
	{{"sell", "fire", "sword"}, {setType(3280), setAmount(1), setPrice(4000), "Do you want to sell a fire sword for %P gold?", setTopic(11)}},
	{{"sell", "war", "hammer"}, {setType(3279), setAmount(1), setPrice(1200), "Do you want to sell a war hammer for %P gold?", setTopic(11)}},
	{{"sell", "ice", "rapier"}, {setType(3284), setAmount(1), setPrice(1000), "Do you want to sell an ice rapier for %P gold?", setTopic(11)}},
	{{"sell", "broad", "sword"}, {setType(3301), setAmount(1), setPrice(500), "Do you want to sell a broad sword for %P gold?", setTopic(11)}},
	{{"sell", "dragon", "lance"}, {setType(3302), setAmount(1), setPrice(9000), "Do you want to sell a dragon lance for %P gold?", setTopic(11)}},
	{{"sell", "obsidian", "lance"}, {setType(3313), setAmount(1), setPrice(500), "Do you want to sell an obsidian lance for %P gold?", setTopic(11)}},
	{{"sell", "fire", "axe"}, {setType(3320), setAmount(1), setPrice(8000), "Do you want to sell a fire axe for %P gold?", setTopic(11)}},

	{{"sell", "guardian", "shield"}, {setType(3415), setAmount(1), setPrice(2000), "Do you want to sell a guardian shield for %P gold?", setTopic(11)}},
	{{"sell", "dragon", "shield"}, {setType(3416), setAmount(1), setPrice(4000), "Do you want to sell a dragon shield for %P gold?", setTopic(11)}},
	{{"sell", "beholder", "shield"}, {setType(3418), setAmount(1), setPrice(1200), "Do you want to sell a beholder shield for %P gold?", setTopic(11)}},
	{{"sell", "crown", "shield"}, {setType(3419), setAmount(1), setPrice(8000), "Do you want to sell a crown shield for %P gold?", setTopic(11)}},
	{{"sell", "phoenix", "shield"}, {setType(3439), setAmount(1), setPrice(16000), "Do you want to sell a phoenix shield for %P gold?", setTopic(11)}},

	{{"sell", "noble", "armor"}, {setType(3380), setAmount(1), setPrice(900), "Do you want to sell a noble armor for %P gold?", setTopic(11)}},
	{{"sell", "crown", "armor"}, {setType(3381), setAmount(1), setPrice(12000), "Do you want to sell a crown armor for %P gold?", setTopic(11)}},
	{{"sell", "crown", "legs"}, {setType(3382), setAmount(1), setPrice(12000), "Do you want to sell a pair of crown legs for %P gold?", setTopic(11)}},
	{{"sell", "crown", "helmet"}, {setType(3385), setAmount(1), setPrice(2500), "Do you want to sell a crown helmet for %P gold?", setTopic(11)}},
	{{"sell", "crusader", "helmet"}, {setType(3391), setAmount(1), setPrice(6000), "Do you want to sell a crusader helmet for %P gold?", setTopic(11)}},
	{{"sell", "royal", "helmet"}, {setType(3392), setAmount(1), setPrice(30000), "Do you want to sell a royal helmet for %P gold?", setTopic(11)}},
	{{"sell", "blue", "robe"}, {setType(3567), setAmount(1), setPrice(10000), "Do you want to sell a blue robe for %P gold?", setTopic(11)}},
	{{"sell", "boots", "of", "haste"}, {setType(3079), setAmount(1), setPrice(30000), "Do you want to sell a boots of haste for %P gold?", setTopic(11)}},

	{{"sell", P1, lt(1, P1), "spike", "sword"}, {setType(3271), setAmount(P1), setPrice(mul(1000, P1)), "Do you want to sell %A spike swords for %P gold?", setTopic(11)}},
	{{"sell", P1, lt(1, P1), "fire", "sword"}, {setType(3280), setAmount(P1), setPrice(mul(4000, P1)), "Do you want to sell %A fire swords for %P gold?", setTopic(11)}},
	{{"sell", P1, lt(1, P1), "war", "hammer"}, {setType(3279), setAmount(P1), setPrice(mul(1200, P1)), "Do you want to sell %A war hammers for %P gold?", setTopic(11)}},
	{{"sell", P1, lt(1, P1), "ice", "rapier"}, {setType(3284), setAmount(P1), setPrice(mul(1000, P1)), "Do you want to sell %A ice rapiers for %P gold?", setTopic(11)}},
	{{"sell", P1, lt(1, P1), "broad", "sword"}, {setType(3301), setAmount(P1), setPrice(mul(500, P1)), "Do you want to sell %A broad swords for %P gold?", setTopic(11)}},
	{{"sell", P1, lt(1, P1), "dragon", "lance"}, {setType(3302), setAmount(P1), setPrice(mul(9000, P1)), "Do you want to sell %A dragon lances for %P gold?", setTopic(11)}},
	{{"sell", P1, lt(1, P1), "obsidian", "lance"}, {setType(3313), setAmount(P1), setPrice(mul(500, P1)), "Do you want to sell %A obsidian lances for %P gold?", setTopic(11)}},
	{{"sell", P1, lt(1, P1), "fire", "axe"}, {setType(3320), setAmount(P1), setPrice(mul(8000, P1)), "Do you want to sell %A fire axes for %P gold?", setTopic(11)}},

	{{"sell", P1, lt(1, P1), "guardian", "shield"}, {setType(3415), setAmount(P1), setPrice(mul(2000, P1)), "Do you want to sell %A guardian shields for %P gold?", setTopic(11)}},
	{{"sell", P1, lt(1, P1), "dragon", "shield"}, {setType(3416), setAmount(P1), setPrice(mul(4000, P1)), "Do you want to sell %A dragon shields for %P gold?", setTopic(11)}},
	{{"sell", P1, lt(1, P1), "beholder", "shield"}, {setType(3418), setAmount(P1), setPrice(mul(1200, P1)), "Do you want to sell %A beholder shields for %P gold?", setTopic(11)}},
	{{"sell", P1, lt(1, P1), "crown", "shield"}, {setType(3419), setAmount(P1), setPrice(mul(8000, P1)), "Do you want to sell %A crown shields for %P gold?", setTopic(11)}},
	{{"sell", P1, lt(1, P1), "phoenix", "shield"}, {setType(3439), setAmount(P1), setPrice(mul(16000, P1)), "Do you want to sell %A phoenix shields for %P gold?", setTopic(11)}},

	{{"sell", P1, lt(1, P1), "noble", "armor"}, {setType(3380), setAmount(P1), setPrice(mul(900, P1)), "Do you want to sell %A noble armors for %P gold?", setTopic(11)}},
	{{"sell", P1, lt(1, P1), "crown", "armor"}, {setType(3381), setAmount(P1), setPrice(mul(12000, P1)), "Do you want to sell %A crown armors for %P gold?", setTopic(11)}},
	{{"sell", P1, lt(1, P1), "crown", "legs"}, {setType(3382), setAmount(P1), setPrice(mul(12000, P1)), "Do you want to sell %A pairs of crown legs for %P gold?", setTopic(11)}},
	{{"sell", P1, lt(1, P1), "crown", "helmet"}, {setType(3385), setAmount(P1), setPrice(mul(2500, P1)), "Do you want to sell %A crown helmets for %P gold?", setTopic(11)}},
	{{"sell", P1, lt(1, P1), "crusader", "helmet"}, {setType(3391), setAmount(P1), setPrice(mul(6000, P1)), "Do you want to sell %A crusader helmets for %P gold?", setTopic(11)}},
	{{"sell", P1, lt(1, P1), "royal", "helmet"}, {setType(3392), setAmount(P1), setPrice(mul(30000, P1)), "Do you want to sell %A royal helmets for %P gold?", setTopic(11)}},
	{{"sell", P1, lt(1, P1), "blue", "robe"}, {setType(3567), setAmount(P1), setPrice(mul(10000, P1)), "Do you want to sell %A blue robes for %P gold?", setTopic(11)}},
	{{"sell", P1, lt(1, P1), "boots", "of", "haste"}, {setType(3079), setAmount(P1), setPrice(mul(30000, P1)), "Do you want to sell %A boots of haste for %P gold?", setTopic(11)}},

	{{eq(TOPIC, 10), lt(questValue(283), 3), FORCE}, {"I'm sorry, pal. But you need Gabel's permission to trade with me."}},
	{{eq(TOPIC, 10), "yes", ge(COUNTMONEY, PRICE)}, {"Ok. Here you are!", deleteMoney, create(TYPE)}},
	{{eq(TOPIC, 10), "yes"}, {"Well, come back if you have enough gold."}},
	{{eq(TOPIC, 10)}, {"Well, obviously not."}},

	{{eq(TOPIC, 11), lt(questValue(283), 3), FORCE}, {"I'm sorry, pal. You don't have Gabel's permission to trade with me."}},
	{{eq(TOPIC, 11), "yes", ge(count(TYPE), AMOUNT)}, {"Ok. Here is your gold.", delete(TYPE), createMoney}},
	{{eq(TOPIC, 11), "yes"}, {"You do not have one!"}},
	{{eq(TOPIC, 11), "yes", gt(AMOUNT, 1)}, {"You do not have that many!"}},
	{{eq(TOPIC, 11)}, {"Well, obviously not."}},
}

function onCreatureSay(npc, creature, text, situation)
	behaviour.react(npc, rules, text, situation)
end

function onCreatureDisappear(npc, creature)
	behaviour.react(npc, rules, "", NPC_SITUATION_VANISH)
end
