-- npcs/haroun.lua: the behaviour of the NPC Haroun, converted from
-- haroun.npc by patella-convert. The NPC is its [[npc]] entry in npcs.toml;
-- lib/npc.lua matches and runs the rules below, in the order of the file
-- (scripting.hh, "NPCs"). The comments are those of haroun.npc.
--
-- GIMUD - Graphical Interface Multi User Dungeon
-- haroun.npc: Datenbank für den Maridhändler Haroun (Magische Gegenstände, Marid)

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
local create = behaviour.create
local delete = behaviour.delete
local createMoney = behaviour.createMoney
local deleteMoney = behaviour.deleteMoney
local queue = behaviour.queue
local idle = behaviour.idle
local nop = behaviour.nop
local previous = behaviour.previous

local rules = behaviour.rules {
	{{ADDRESS, "hello$", eq(questValue(278), 2), FORCE}, {"Be greeted, human %N. How can a humble djinn be of service?"}},
	{{ADDRESS, "hi$", eq(questValue(278), 2), FORCE}, {previous}},
	{{ADDRESS, "greetings$", eq(questValue(278), 2), FORCE}, {previous}},
	{{ADDRESS, "djanni'hah$", eq(questValue(278), 2), FORCE}, {previous}},
	{{ADDRESS, FORCE}, {idle}},
	{{BUSY, "hello$", eq(questValue(278), 2), FORCE}, {"Please wait, human %N. I'll be with you in a minute.", queue}},
	{{BUSY, "hi$", eq(questValue(278), 2), FORCE}, {previous}},
	{{BUSY, "greetings$", eq(questValue(278), 2), FORCE}, {previous}},
	{{BUSY, "djanni'hah$", eq(questValue(278), 2), FORCE}, {previous}},
	{{BUSY, FORCE}, {nop}},
	{{VANISH}, {"Farewell, human."}},

	{{"bye"}, {"Farewell! May the serene light of the enlightened one rest shine on your travels.", idle}},
	{{"farewell"}, {previous}},
	{{"name"}, {"My name is Haroun. At your service."}},
	{{"haroun"}, {"That is the name my honourable father chose for me."}},
	{{"job"}, {"I am a merchant. Though I have not chosen this position for myself I have accepted the task with humility and devotion. ...", "If you have the permission of Gabel to trade with me, I can sell you some useful magical equipment."}},
	{{"permission"}, {"I am not allowed to trade with you unless Gabel gave you the permission to trade with us."}},

	{{"trade"}, {"It is my mission to trade items on behalf of our community. ...", "Just ask me for my current offers."}},
	{{"merchant"}, {previous}},
	{{"gabel"}, {"He is our leader. Not because he desires power or distinction, of course, but because the community needs a guide in spiritual and in worldly matters. ...", "He has accepted this burden with the modesty that is required of a true follower of Daraman."}},
	{{"king"}, {"We do not have kings. Of the members of our community, Gabel is closest to being what you would consider a leader, but he would resent being called by that presumptuous title."}},
	{{"djinn"}, {"That is my race. I like to compare it to iron. It has the potential to be forged into the finest steel, but it does need a lot of work and devotion. ...", "Without that it is nothing but worthless scrap metal. Like our master said in the scriptures: Forge thyself! Be a blade of the true creed! (Book VI, chapter 14, verses 3 and 4)."}},
	{{"efreet"}, {"Daraman has taught us restraint. But I am sure one day those apostates will pay dearly for their sinful rebellion!"}},
	{{"marid"}, {"We are the chosen ones. Those who walk the path to enlightenment with serenity and humility."}},
	{{"malor"}, {"That filthy heretic! To me he is the very embodiment of all the evilness and the baseness we djinn must leave behind."}},
	{{"mal'ouquah"}, {"That is the name of the Efreets' hideout. It certainly is one filthy hotbed of vice and degeneration."}},
	{{"ashta'daramai"}, {"This is the name of the humble abode you are currently visiting. After Daraman had shown him the true way Gabel decided to tear down his former palace and to erect the present structure. ...", "Its name is meant as a tribute to the invaluable service Daraman has done to our race."}},
	{{"human"}, {"Back in the dark days we used to be enemies, the humans and us. But then a human came and radically changed the way we see things. ...", "He taught us to see your race as brothers. Of course... of course we still have lessons to learn, I suppose."}},

	{{"zathroth"}, {"An evil, evil god. He was the creator of my race, but he rejected us because we were not evil enough and far too independent to his taste. For this reason the djinn do not worship him anymore, not even those depraved efreet. ...", "It is difficult, you know. We djinn had to learn to find our own way, to create a destiny of our own. And although we are not evil by nature the fact that we were created by this fiend was lying like a curse on us. ...", "For a long time we walked in darkness, filled with hate of ourselves and of all creation. But then everything changed. A human came and made us realise how blind we were. ...", "Praise the gods who have spoken to us through Daraman! That human taught us how we could overcome our grim heritage and heal the scars that marred our souls. Humility and asceticism are the keys to a fuller existence. ...", "The task may be tedious, but the price that awaits the disciples of the true creed is worth it all."}},

	{{"tibia"}, {"Despite all the evil that stalks this world it is still a source of endless marvel and joy to me. Like the master said: ...", "'Look at the world and reflect in admiration. For there is peace in a sunset and wisdom in the ocean's neverending melody.' (Book I, chapter 3, verses 2 and 3)."}},
	{{"daraman"}, {"How could my unworthy tongue ever find words to praise the enlightened one the way he deserves to be praised? Daraman took the veil from our eyes. He made us realise the extent of our misery. ...", "However, he did much more than that. He showed us a way out. He gave meaning to our lives. The service he has done to my race will never be forgotten."}},
	{{"darashia"}, {"I understand the humans living there are self-indulgent and decadent. I am sure Daraman would disapprove of their ways."}},
	{{"scarab"}, {"The scarab is an ancient species. It's chitinous shell can be turned into excellent armour."}},
	{{"edron"}, {"They say the humans have founded impressive communities to the north. I hope their spiritual integrity matches their entrepreneurial spirit."}},
	{{"thais"}, {previous}},
	{{"venore"}, {previous}},
	{{"kazordoon"}, {previous}},
	{{"carlin"}, {previous}},
	{{"ab'dendriel"}, {previous}},
	{{"ankrahmun"}, {"Ankrahmun has a venerable tradition, but now that heretic fool has acceeded to the throne it has degenerated into a breeding place for wicked heresy."}},
	{{"pharaoh"}, {"I have heard many a gruesome stories about this presumptuous fool, who calls himself a pharaoh. Such hubris! Daraman himself, who was certainly the most divine mortal that ever walked Tibia, would never have dared to call himself a god."}},
	{{"palace"}, {"I am a peaceful djinn, but with all those stories I have heard I really feel like visiting the palace and to cleanse this place from all the evil and the misbelief infesting that place."}},
	{{"ascension"}, {"I have heard this is what the pharaoh has promised his followers. It is sad to see this wicked sinner is still allowed to spread his heresy!"}},
	{{"rah"}, {"Another one of the pharaoh's confused ideas."}},
	{{"uthun"}, {previous}},
	{{"akh"}, {previous}},

	{{"kha'zeel"}, {"The Kha'zeel mountains have been created by the gods as a vantage point from where they could watch their creation. ...", "Isn't it uplifting to think that even now we are standing on ground that was blessed by the great creators."}},
	{{"kha'labal"}, {"Kha'labal. It makes me sad just to hear that name. Long, long time ago this used to be a garden, you know? ...", "You should have seen it then. You should have seen the springs and the meadows and the flowers. It was a paradise. <sighs>. Oh, this terrible war."}},
	{{"melchior"}, {"I remember Malchior. I used to meet him regularly, but strictly in matters of business. He was not really a likeable fellow then. ...", "All money and profit he was, and not as quite as clever as he thought. He made quite a pretty penny by doing business both with us and the efreet. ...", "He thought we wouldn't notice, but the truth is we just chose to look the other way."}},
	{{"alesar"}, {"Who? Yes, I... I have never... I mean, I don't know who that is."}},
	{{"fa'hradin"}, {"Fa'hradin is a learned man, and this alone is enough to make me respect him. However, I sometimes can't help the feeling that he is lacking spiritual strength."}},
	{{"bo'ques"}, {"Bo'ques is a cook. Cooking! What a useless craft. If I were Gabel I would have little use for a cook such as him, but perhaps not all djinns are as spiritual as Daraman would have them be."}},
	{{"lamp"}, {"The lamp is a djinn's resting place of choice."}},

	{{"weapon"}, {"I'm afraid I do not trade with weapons or armour. Nah'bob only deals with magical equipment."}},
	{{"armor"}, {previous}},
	{{"armour"}, {previous}},
	{{"wares"}, {"I only deal with magical equipment. Our range of goods include amulets, rings, wands and some special items."}},
	{{"offer"}, {previous}},
	{{"goods"}, {previous}},
	{{"equipment"}, {previous}},
	{{"magical"}, {previous}},
	{{"do", "you", "sell"}, {previous}},
	{{"do", "you", "have"}, {previous}},

	{{"amulets"}, {"I'm selling and buying bronze amulets, stone skin amulets, elven amulets and garlic necklaces."}},
	{{"rings"}, {"I'm selling and buying stealth rings, power rings, sword rings, axe rings, and club rings."}},
	{{"wands"}, {"I'm buying wands of vortex, wands of dragonbreath, wands of plague, wands of cosmic energy and wands of inferno as well as magic light wands."}},
	{{"rods"}, {"I'm not interested in rods."}},
	{{"special"}, {"I'm selling and buying magic light wands. I'm currently also looking for mind stones, life crystals and orbs."}},

	{{"light"}, {setType(3046), setAmount(1), setPrice(120), "Do you want to buy a magic light wand for %P gold?", setTopic(10)}},
	{{"sword", "ring"}, {setType(3091), setAmount(1), setPrice(500), "Do you want to buy a sword ring for %P gold?", setTopic(10)}},
	{{"axe", "ring"}, {setType(3092), setAmount(1), setPrice(500), "Do you want to buy an axe ring for %P gold?", setTopic(10)}},
	{{"club", "ring"}, {setType(3093), setAmount(1), setPrice(500), "Do you want to buy a club ring for %P gold?", setTopic(10)}},
	{{"stone", "skin", "amulet"}, {setType(3081), setAmount(1), setPrice(5000), "Do you want to buy a stone skin amulet for %P gold?", setTopic(10)}},
	{{"elven", "amulet"}, {setType(3082), setAmount(1), setPrice(500), "Do you want to buy an elven amulet for %P gold?", setTopic(10)}},
	{{"garlic", "necklace"}, {setType(3083), setAmount(1), setPrice(100), "Do you want to buy a garlic necklace for %P gold?", setTopic(10)}},
	{{"bronze", "amulet"}, {setType(3056), setAmount(1), setPrice(100), "Do you want to buy a bronze amulet for %P gold?", setTopic(10)}},
	{{"stealth", "ring"}, {setType(3049), setAmount(1), setPrice(5000), "Do you want to buy a stealth ring for %P gold?", setTopic(10)}},
	{{"power", "ring"}, {setType(3050), setAmount(1), setPrice(100), "Do you want to buy a power ring for %P gold?", setTopic(10)}},

	{{P1, lt(1, P1), "light"}, {setType(3046), setAmount(P1), setPrice(mul(120, P1)), "Do you want to buy %A magic light wands for %P gold?", setTopic(10)}},
	{{P1, lt(1, P1), "power", "ring"}, {setType(3050), setAmount(P1), setPrice(mul(100, P1)), "Do you want to buy %A power rings for %P gold?", setTopic(10)}},
	{{P1, lt(1, P1), "bronze", "amulet"}, {setType(3056), setAmount(P1), setPrice(mul(100, P1)), "Do you want to buy %A bronze amulets for %P gold?", setTopic(10)}},
	{{P1, lt(1, P1), "sword", "ring"}, {setType(3091), setAmount(P1), setPrice(mul(500, P1)), "Do you want to buy %A sword rings for %P gold?", setTopic(10)}},
	{{P1, lt(1, P1), "axe", "ring"}, {setType(3092), setAmount(P1), setPrice(mul(500, P1)), "Do you want to buy %A axe rings for %P gold?", setTopic(10)}},
	{{P1, lt(1, P1), "club", "ring"}, {setType(3093), setAmount(P1), setPrice(mul(500, P1)), "Do you want to buy %A club rings for %P gold?", setTopic(10)}},
	{{P1, lt(1, P1), "elven", "amulet"}, {setType(3082), setAmount(P1), setPrice(mul(500, P1)), "Do you want to buy %A elven amulets for %P gold?", setTopic(10)}},
	{{P1, lt(1, P1), "garlic", "necklace"}, {setType(3083), setAmount(P1), setPrice(mul(100, P1)), "Do you want to buy %A garlic necklaces for %P gold?", setTopic(10)}},
	{{P1, lt(1, P1), "stone", "skin", "amulet"}, {setType(3081), setAmount(P1), setPrice(mul(5000, P1)), "Do you want to buy %A stone skin amulets for %P gold?", setTopic(10)}},
	{{P1, lt(1, P1), "stealth", "ring"}, {setType(3049), setAmount(P1), setPrice(mul(5000, P1)), "Do you want to buy %A stealth rings for %P gold?", setTopic(10)}},

	{{"sell", "light"}, {setType(3046), setAmount(1), setPrice(35), "Do you want to sell a magic light wand for %P gold?", setTopic(11)}},
	{{"sell", "sword", "ring"}, {setType(3091), setAmount(1), setPrice(100), "Do you want to sell a sword ring for %P gold?", setTopic(11)}},
	{{"sell", "axe", "ring"}, {setType(3092), setAmount(1), setPrice(100), "Do you want to sell an axe ring for %P gold?", setTopic(11)}},
	{{"sell", "club", "ring"}, {setType(3093), setAmount(1), setPrice(100), "Do you want to sell a club ring for %P gold?", setTopic(11)}},
	{{"sell", "stone", "skin", "amulet"}, {setType(3081), setAmount(1), setPrice(500), "Do you want to sell a stone skin amulet for %P gold?", setTopic(11)}},
	{{"sell", "elven", "amulet"}, {setType(3082), setAmount(1), setPrice(100), "Do you want to sell an elven amulet for %P gold?", setTopic(11)}},
	{{"sell", "garlic", "necklace"}, {setType(3083), setAmount(1), setPrice(50), "Do you want to sell a garlic necklace for %P gold?", setTopic(11)}},
	{{"sell", "bronze", "amulet"}, {setType(3056), setAmount(1), setPrice(50), "Do you want to sell a bronze amulet for %P gold?", setTopic(11)}},
	{{"sell", "stealth", "ring"}, {setType(3049), setAmount(1), setPrice(200), "Do you want to sell a stealth ring for %P gold?", setTopic(11)}},
	{{"sell", "power", "ring"}, {setType(3050), setAmount(1), setPrice(50), "Do you want to sell a power ring for %P gold?", setTopic(11)}},
	{{"sell", "mind", "stone"}, {setType(3062), setAmount(1), setPrice(100), "Do you want to sell a mind stone for %P gold?", setTopic(11)}},
	{{"sell", "life", "crystal"}, {setType(3061), setAmount(1), setPrice(50), "Do you want to sell a life crystal for %P gold?", setTopic(11)}},
	{{"sell", "orb"}, {setType(3060), setAmount(1), setPrice(750), "Do you want to sell an orb for %P gold?", setTopic(11)}},

	{{"sell", P1, lt(1, P1), "light"}, {setType(3046), setAmount(P1), setPrice(mul(35, P1)), "Do you want to sell %A magic light wands for %P gold?", setTopic(11)}},
	{{"sell", P1, lt(1, P1), "power", "ring"}, {setType(3050), setAmount(P1), setPrice(mul(50, P1)), "Do you want to sell %A power rings for %P gold?", setTopic(11)}},
	{{"sell", P1, lt(1, P1), "bronze", "amulet"}, {setType(3056), setAmount(P1), setPrice(mul(50, P1)), "Do you want to sell %A bronze amulets for %P gold?", setTopic(11)}},
	{{"sell", P1, lt(1, P1), "sword", "ring"}, {setType(3091), setAmount(P1), setPrice(mul(100, P1)), "Do you want to sell %A sword rings for %P gold?", setTopic(11)}},
	{{"sell", P1, lt(1, P1), "axe", "ring"}, {setType(3092), setAmount(P1), setPrice(mul(100, P1)), "Do you want to sell %A axe rings for %P gold?", setTopic(11)}},
	{{"sell", P1, lt(1, P1), "club", "ring"}, {setType(3093), setAmount(P1), setPrice(mul(100, P1)), "Do you want to sell %A club rings for %P gold?", setTopic(11)}},
	{{"sell", P1, lt(1, P1), "elven", "amulet"}, {setType(3082), setAmount(P1), setPrice(mul(100, P1)), "Do you want to sell %A elven amulets for %P gold?", setTopic(11)}},
	{{"sell", P1, lt(1, P1), "garlic", "necklace"}, {setType(3083), setAmount(P1), setPrice(mul(50, P1)), "Do you want to sell %A garlic necklaces for %P gold?", setTopic(11)}},
	{{"sell", P1, lt(1, P1), "stone", "skin", "amulet"}, {setType(3081), setAmount(P1), setPrice(mul(500, P1)), "Do you want to sell %A stone skin amulets for %P gold?", setTopic(11)}},
	{{"sell", P1, lt(1, P1), "stealth", "ring"}, {setType(3049), setAmount(P1), setPrice(mul(200, P1)), "Do you want to sell %A stealth rings for %P gold?", setTopic(11)}},
	{{"sell", P1, lt(1, P1), "mind", "stone"}, {setType(3062), setAmount(P1), setPrice(mul(100, P1)), "Do you want to sell %A mind stones for %P gold?", setTopic(11)}},
	{{"sell", P1, lt(1, P1), "life", "crystal"}, {setType(3061), setAmount(P1), setPrice(mul(50, P1)), "Do you want to sell %A life crystals for %P gold?", setTopic(11)}},
	{{"sell", P1, lt(1, P1), "orb"}, {setType(3060), setAmount(P1), setPrice(mul(750, P1)), "Do you want to sell %A orbs for %P gold?", setTopic(11)}},

	{{"sell", "wand", "of", "vortex"}, {setType(3074), setAmount(1), setPrice(100), "Do you want to sell a wand of vortex for %P gold?", setTopic(11)}},
	{{"sell", "wand", "of", "dragonbreath"}, {setType(3075), setAmount(1), setPrice(200), "Do you want to sell a wand of dragonbreath for %P gold?", setTopic(11)}},
	{{"sell", "wand", "of", "plague"}, {setType(3072), setAmount(1), setPrice(1000), "Do you want to sell a wand of plague for %P gold?", setTopic(11)}},
	{{"sell", "wand", "of", "cosmic", "energy"}, {setType(3073), setAmount(1), setPrice(2000), "Do you want to sell a wand of cosmic energy for %P gold?", setTopic(11)}},
	{{"sell", "wand", "of", "inferno"}, {setType(3071), setAmount(1), setPrice(3000), "Do you want to sell a wand of inferno for %P gold?", setTopic(11)}},

	{{"sell", P1, lt(1, P1), "wand", "of", "vortex"}, {setType(3074), setAmount(P1), setPrice(mul(100, P1)), "Do you want to sell %A wands of vortex for %P gold?", setTopic(11)}},
	{{"sell", P1, lt(1, P1), "wand", "of", "dragonbreath"}, {setType(3075), setAmount(P1), setPrice(mul(200, P1)), "Do you want to sell %A wands of dragonbreath for %P gold?", setTopic(11)}},
	{{"sell", P1, lt(1, P1), "wand", "of", "plague"}, {setType(3072), setAmount(P1), setPrice(mul(1000, P1)), "Do you want to sell %A wands of plague for %P gold?", setTopic(11)}},
	{{"sell", P1, lt(1, P1), "wand", "of", "cosmic", "energy"}, {setType(3073), setAmount(P1), setPrice(mul(2000, P1)), "Do you want to sell %A wands of cosmic energy for %P gold?", setTopic(11)}},
	{{"sell", P1, lt(1, P1), "wand", "of", "inferno"}, {setType(3071), setAmount(P1), setPrice(mul(3000, P1)), "Do you want to sell %A wands of inferno for %P gold?", setTopic(11)}},

	{{eq(TOPIC, 10), lt(questValue(283), 3), FORCE}, {"I'm sorry, human. But you need Gabel's permission to trade with me."}},
	{{eq(TOPIC, 10), "yes", ge(COUNTMONEY, PRICE)}, {"Thank you. May it bring you luck!", deleteMoney, create(TYPE)}},
	{{eq(TOPIC, 10), "yes"}, {"I hate to disappoint you, but it seems you do not have enough gold."}},
	{{eq(TOPIC, 10)}, {"I understand. Perhaps another time then."}},

	{{eq(TOPIC, 11), lt(questValue(283), 3), FORCE}, {"I'm sorry, human. But you need Gabel's permission to trade with me."}},
	{{eq(TOPIC, 11), "yes", ge(count(TYPE), AMOUNT)}, {"Thank you. Here is your money.", delete(TYPE), createMoney}},
	{{eq(TOPIC, 11), "yes"}, {"You do not have one."}},
	{{eq(TOPIC, 11), "yes", gt(AMOUNT, 1)}, {"You do not have that many."}},
	{{eq(TOPIC, 11)}, {"I understand. Perhaps another time then."}},
}

function onCreatureSay(npc, creature, text, situation)
	behaviour.react(npc, rules, text, situation)
end

function onCreatureDisappear(npc, creature)
	behaviour.react(npc, rules, "", NPC_SITUATION_VANISH)
end
