-- npcs/bozo.lua: the behaviour of the NPC Bozo, converted from
-- bozo.npc by patella-convert. The NPC is its [[npc]] entry in npcs.toml;
-- lib/npc.lua matches and runs the rules below, in the order of the file
-- (scripting.hh, "NPCs"). The comments are those of bozo.npc.
--
-- GIMUD - Graphical Interface Multi User Dungeon
-- bozo.npc: Datenbank für den Hofnarren Bozo

local behaviour = require("lib.npc")
local ADDRESS = behaviour.ADDRESS
local BUSY = behaviour.BUSY
local VANISH = behaviour.VANISH
local MALE = behaviour.MALE
local FEMALE = behaviour.FEMALE
local KNIGHT = behaviour.KNIGHT
local PALADIN = behaviour.PALADIN
local SORCERER = behaviour.SORCERER
local DRUID = behaviour.DRUID
local FORCE = behaviour.FORCE
local eq = behaviour.eq
local ge = behaviour.ge
local TOPIC = behaviour.TOPIC
local PRICE = behaviour.PRICE
local TYPE = behaviour.TYPE
local COUNTMONEY = behaviour.COUNTMONEY
local setTopic = behaviour.setTopic
local setPrice = behaviour.setPrice
local setAmount = behaviour.setAmount
local setType = behaviour.setType
local effectMe = behaviour.effectMe
local create = behaviour.create
local deleteMoney = behaviour.deleteMoney
local idle = behaviour.idle
local nop = behaviour.nop
local previous = behaviour.previous

local rules = behaviour.rules {
	{{ADDRESS, "hello$", FEMALE, FORCE}, {"Hello, hello, hello, little lady %N!"}},
	{{ADDRESS, "hi$", FEMALE, FORCE}, {"Hello, hello, hello, little lady %N!"}},
	{{ADDRESS, "hello$", MALE, FORCE}, {"Hi there, how's it hanging, %N!"}},
	{{ADDRESS, "hi$", MALE, FORCE}, {"Hi there, how's it hanging, %N!"}},
	{{ADDRESS, FORCE}, {idle}},
	{{BUSY, "hello$", FORCE}, {"Wait and listen to my jokes, I am sooooo funny!"}},
	{{BUSY, "hi$", FORCE}, {"Wait and listen to my jokes, I am sooooo funny!"}},
	{{BUSY, FORCE}, {nop}},
	{{VANISH, MALE, FORCE}, {"How did he do that??"}},
	{{VANISH, FORCE}, {"Women! They all do that to me!"}},

	{{"bye"}, {"Remember: A joke a day keeps the ghouls away!", idle}},
	{{"farewell"}, {previous}},
	{{"job"}, {setPrice(50), "I am the royal jes ... uhm ... the royal tax-collector! Do you want to pay your taxes?", setTopic(1)}},
	{{"news"}, {"I know the newest jokes in tibia."}},
	{{"how", "are", "you"}, {"Thank you, I'm fine, the gods are with me."}},
	{{"sell"}, {"Sell? Hmm, I know a little about magic and by chance I can sell you a truly unusual weapon."}},
	{{"durin"}, {"Isn't he the author of the book 'fun with demons'?"}},
	{{"stephan"}, {"He is kind of a father figure to me. Of course he denies all kinship to me."}},
	{{"steve"}, {"He's a smart one. I heared he hid in a foreign country as the first bugs showed up."}},
	{{"name"}, {"My name is Bozo. But it's more than a name, it's a lifestyle to me!"}},
	{{"time"}, {"Since you met me it is happy hour for you."}},
	{{"help"}, {"I am a jester, not a doctor!"}},
	{{"jester"}, {"Do you wish to join the fools' guild?", setTopic(6)}},
	{{"fool"}, {"Do you wish to join the fools' guild?", setTopic(6)}},
	{{"joke"}, {"I know some 'monstrous' jokes!"}},
	{{"idiot"}, {"To me it's just a profession, but for you it's a state of mind!"}},
	{{"wish"}, {"If you have a wish to CIP just write a letter and place it in a dustbin of your choice."}},
	{{"excalibug"}, {"I am not foolish enough to believe in the existence of this weapon."}},
	{{"wallcarving"}, {"Oh, I saw some demoncarvings in the dungeons as I hid there after a little joke on old Stutch."}},
	{{"demoncarving"}, {"Yes, they showed demons, seven actually, dancing around a sword! In a flaming pit of some kind."}},
	{{"flaming", "pit"}, {"Ah, don't ask me! Usually mages and mystics know more about such stuff."}},

	{{"monster"}, {"I know a lot of monster jokes. Just tell me a monster's name, come on."}},
	{{"demon"}, {"Why are the experienced heroes quicker than others? ... The demons love fast food!"}},
	{{"ghoul"}, {"Where do the ghouls buy their robes? ... In a Boooohtique!"}},
	{{"dragon"}, {"Why do dragons breathe fire? ... They ate too many sorcerers in chili sauce!"}},
	{{"skeleton"}, {"Why do skeletons flee if wounded? ... They are so spineless!"}},
	{{"orc"}, {"Why do orcs have green skin? ... They ate at Frodo's!"}},
	{{"cyclops"}, {"How many eyes does a cyclops have? ... One for each IQ point of their opponents!"}},
	{{"beholder"}, {"Why are beholders so ugly? ... Because their mom and dad were beholders, too!"}},
	{{"rat"}, {"Why does the rat have a wooden leg? ... Because it is a former pirate!"}},
	{{"spider"}, {"Why did the spider cross the road? ... Because it ... oh you already know this one!?"}},
	{{"troll"}, {"Why do trolls live underground? ... Because on the ground there are so many PKs!"}},
	{{"wolf"}, {"Why do the wolves howl? ... Hey, if you're online that long you can't help but behave that way!"}},
	{{"mino"}, {"What do all little minotaurs want to become when they are grown-ups? ... Cowboys, of course!"}},
	{{"dungeon"}, {"If you are a bad jester you get a chance to visit them now and then."}},
	{{"sewer"}, {"Good place for picking up apples and women."}},
	{{"oswald"}, {"If you believe half the rumours he's spreading, you are going to get in a lot of trouble."}},
	{{"update"}, {"Hey! I am supposed to make the jokes here!"}},
	{{"god"}, {"I better make no jokes about THIS matter."}},
	{{"king"}, {"Nah, no jests about His Royal Highness."}},
	{{"sam"}, {"Did you know that he now sells a 'power axe of doom'? Run and buy it, he has only three in store."}},
	{{"benjamin"}, {"He would make a fine jester, too."}},
	{{"gorn"}, {"He sells spell scrolls each day at midnight, but you have to address him that very second."}},
	{{"quentin"}, {"He's my baby brother. If you tell him I sent you, he will grant you an extra spell or two."}},
	{{"bozo"}, {"Thats me: Bozo, the jester!"}},
	{{"weapon"}, {setType(3473), setAmount(1), setPrice(250), "Do you want to buy a 'mace of the fury' for 250 gold?", setTopic(3)}},
	{{"magic"}, {setPrice(200), "I actually know some spells! Do you want to learn how to 'lessen your load' for %P gold?", setTopic(2)}},
	{{"spell"}, {setPrice(200), "I actually know some spells! Do you want to learn how to 'lessen your load' for %P gold?", setTopic(2)}},
	{{"tibia"}, {"I rarely leave the castle. It's a real stress to be popular like me."}},
	{{"castle"}, {"The castle is my home. A place fit for a jester and all other fools. Feel welcome."}},
	{{"muriel"}, {"Better don't mess with sorcerers!"}},
	{{"elane"}, {"She's pretty but has a kind of too burning affection for my taste."}},
	{{"marvik"}, {"Humourless old guy! Once turned me into a frog for painting his distasteful cave in pink."}},
	{{"gregor"}, {"A man of steel, with a stomach of wax. Never offer him a beer!"}},
	{{"paladin", PALADIN}, {"I wanted to become a paladin, too, but I was overqualified!"}},
	{{"paladin"}, {"They are the king's favourites, because they know how to 'bow'."}},
	{{"sorcerer", SORCERER}, {"I wanted to become a sorcerer, too, but I was overqualified!"}},
	{{"sorcerer"}, {"The good thing about them is that they can't be at two places at the same time."}},
	{{"druid", DRUID}, {"I wanted to become a Druid, too, but I was overqualified!"}},
	{{"druid"}, {"If you are in Druidville, do as the rabbits do."}},
	{{"knight", KNIGHT}, {"I wanted to become a knight, too, but I was overqualified!"}},
	{{"knight"}, {"Did you notice that old knights have their scars just on their backs?"}},
	{{"noodles"}, {"Hey, the little one is almost as funny as me!"}},
	{{"dog"}, {"Are we talking about Noodles?"}},
	{{"poodle"}, {"Are we talking about Noodles?"}},
	{{"guild"}, {"Since the first guild showed up there's a great demand for jesters and fools to join them."}},
	{{"necromant", "nectar"}, {"Peeew! That sounds disgusting! Are you a cook at Frodo's?"}},
	{{"necromant"}, {"Don't feed the necromants."}},
	{{"lady", MALE}, {"Well, you don't behave ladylike just because you dress like one!"}},
	{{"lady", FEMALE}, {"Has any man said to you that you're not only beautiful but also intelligent?", setTopic(5)}},
	{{"kiss", MALE}, {"Uh, go away!", idle}},
	{{"kiss", FEMALE}, {"Do you want to kiss me?", setTopic(4)}},
	{{"hugo"}, {"I had a cousin named like that."}},
	{{"cousin"}, {"He died some years ago."}},

	{{eq(TOPIC, 1), "yes", ge(COUNTMONEY, PRICE)}, {"Thank you very much. I will have a drink or two on your health!", deleteMoney}},
	{{eq(TOPIC, 1), "yes"}, {"Come back, when you have enough money."}},
	{{eq(TOPIC, 1)}, {"Well, perhaps later."}},
	{{eq(TOPIC, 2), "yes", ge(COUNTMONEY, PRICE)}, {"Here you are, I already lessened your load.", deleteMoney}},
	{{eq(TOPIC, 2), "yes"}, {"Come back, when you have enough money."}},
	{{eq(TOPIC, 2)}, {"You don't know what offer you have passed!"}},
	{{eq(TOPIC, 3), "yes", ge(COUNTMONEY, PRICE)}, {"And here it is, it suits you well!", deleteMoney, create(TYPE)}},
	{{eq(TOPIC, 3), "yes"}, {"Come back, when you have enough money."}},
	{{eq(TOPIC, 3)}, {"You dont know what offer you have passed!"}},
	{{eq(TOPIC, 4), "yes"}, {"Uh, oh! ... I am seeing stars!", effectMe(13)}},
	{{eq(TOPIC, 4)}, {"Pah, I didn't want to kiss you anyway!"}},
	{{eq(TOPIC, 5), "yes"}, {"This is a world of fantasy and full of surprises!"}},
	{{eq(TOPIC, 5)}, {"Well, think about it!"}},
	{{eq(TOPIC, 6), "yes"}, {"Sorry, you already are a member."}},
	{{eq(TOPIC, 6)}, {"Well, you are already a member anyway."}},
}

function onCreatureSay(npc, creature, text, situation)
	behaviour.react(npc, rules, text, situation)
end

function onCreatureDisappear(npc, creature)
	behaviour.react(npc, rules, "", NPC_SITUATION_VANISH)
end
