-- npcs/topsy.lua: the behaviour of the NPC Topsy, converted from
-- topsy.npc by patella-convert. The NPC is its [[npc]] entry in npcs.toml;
-- lib/npc.lua matches and runs the rules below, in the order of the file
-- (scripting.hh, "NPCs"). The comments are those of topsy.npc.
--
-- GIMUD - Graphical Interface Multi User Dungeon
-- topsy.npc: Datenbank für shopkeeper Topsy (magic)

local behaviour = require("lib.npc")
local genTRunesFreeS = require("lib.npcs.gen-t-runes-free-s")
local genTWandsFreeS = require("lib.npcs.gen-t-wands-free-s")
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
local mul = behaviour.mul
local setTopic = behaviour.setTopic
local setPrice = behaviour.setPrice
local setAmount = behaviour.setAmount
local setType = behaviour.setType
local setData = behaviour.setData
local create = behaviour.create
local delete = behaviour.delete
local createMoney = behaviour.createMoney
local deleteMoney = behaviour.deleteMoney
local queue = behaviour.queue
local idle = behaviour.idle
local nop = behaviour.nop
local previous = behaviour.previous

local rules = behaviour.rules {
	{{ADDRESS, "hi$", FORCE}, {"Hello, dear %N. How can I help you?"}},
	{{ADDRESS, "hello$", FORCE}, {previous}},
	{{ADDRESS, FORCE}, {idle}},
	{{BUSY, "hi$", FORCE}, {"My apologies, but I really must talk to this customer first. Sugar, I am busy with a customer at the moment. Please have some patience.", queue}},
	{{BUSY, "hello$", FORCE}, {previous}},
	{{BUSY, FORCE}, {nop}},
	{{VANISH, FORCE}, {"Good bye. Do come again!"}},

	{{"bye"}, {"Good bye. Do come again!", idle}},
	{{"farewell"}, {previous}},
	{{"how", "are", "you"}, {"I'm just wonderful - thank you for asking."}},
	{{"job"}, {"I sell runes, life and mana fluids - your best friends in any dungeon!"}},
	{{"name"}, {"My name is Topsy."}},
	{{"time"}, {"Time waits for no one! Not even you, sweetheart, so please do hurry up."}},
	{{"help"}, {"I'd love to help, but I have a business to run - I am busy busy busy!."}},
	{{"monster"}, {"Better buy and charge a lot of runes before facing any monster."}},
	{{"dungeon"}, {"Dungeons - dank cold places if you ask me. They lead to rusty armour, severe colds and death ... on the other hand you use a lot of runes there ... so just think about the treasures you'll surely find there."}},
	{{"sewer"}, {"The Thais sewerage system is a model of modern rat breeding and for some reason is very popular with young adventurers such as yourself."}},
	{{"boss"}, {"I had one once. He should have bought better armour. Actually - he's upstairs."}},
	{{"thank", "you"}, {"Oh, such a sweetie ... and so polite. I thought politeness was out of fashion, these days."}},
	{{"god"}, {"Gods - if we didn't have them, we would have invented them."}},
	{{"king"}, {"<mutters> Here we go again  ... Hail to King Tibianus! ... Don't make me do that again!"}},
	{{"sam"}, {"Ah, such a sweetie.  A simple man, with simple tastes and a simple mind."}},
	{{"benjamin"}, {"Bless him, he stood, he fought, and then left his sanity on the battlefield."}},
	{{"gorn"}, {"Ah yes ... Gorn ... the used-cart salesman of scrolls."}},
	{{"quentin"}, {"I can't tell much about that old monk."}},
	{{"bozo"}, {"He wanted to be the court jester but got upset when people laughed at him"}},
	{{"rumour"}, {"I'm all ears", setTopic(4)}},
	{{"gossip"}, {previous}},
	{{"news"}, {previous}},
	{{"Gamon"}, {"I think he is a spy ... so I smile at him the whole day. He won't get anything out of me!"}},
	{{"carlin"}, {"I went there on holiday once. Just goes to show that women are much better at running a place than men. King Tibianus could learn a thing or two from Queen Eloise."}},
	{{"weapon"}, {"Wrong shop - go to my sister, silly!"}},
	{{"magic"}, {"Magic will only protect you - a rune and some magic fluids."}},
	{{"power"}, {"There is a power struggle between Venore and Thais."}},
	{{"rebellion"}, {"There is talk of a rebellion in Venore to gain independence from the Oppressor - I mean - King of Thais - it can only help business."}},
	{{"quest"}, {"I sell magic stuff, my dear. If you want a quest, you've come to the wrong shop."}},
	{{"spells"}, {"You never know when you run out of mana. All the more reason to buy some good runes or fluids."}},
	{{"muriel"}, {"You should ask this guy Oswald about him ... or other pointless rumors."}},
	{{"elane"}, {"Some call her the preying mantis - apparently she has killed over a dozen husbands already."}},
	{{"venore"}, {"A marvellous city! Modern! Prosperous! Thais could learn a thing or two from Venore"}},
	{{"marvik"}, {"Who knows what the old man is up to in his hideout when no one is watching?"}},
	{{"gregor"}, {"Ah Knight ... can't expect much from those guys."}},
	{{"lugri"}, {"I only heared rumours about him, isn't he a hermit somewhere in the north?"}},
	{{"excalibug"}, {"If you want to find out about excalibug you should ask the more sinister characters in Thais not a respectable woman like myself!"}},
	{{"chester"}, {"I have never seen him at all. I only heared hes kind of the townsguards chief or such."}},
	{{"ardua"}, {"What a strange woman. She lingers in our shop now and then ... I wonder what shes up to."}},
	{{"partos"}, {"I heared he was a thief. Good thing he was caught."}},
	{{"gamel"}, {"He hung around with that partos a lot. I wouldn't be suprised if he's a thief too. He is not allowed to enter our markethall."}},
	{{"quest"}, {"Sure. Just ask my sister Turvy."}},
	{{eq(TOPIC, 4)}, {"Really? Tststs."}},

	{{"offer"}, {"I'm selling life and mana fluids, runes, wands, rods and spellbooks."}},
	{{"goods"}, {previous}},
	{{"do", "you", "sell"}, {previous}},
	{{"do", "you", "have"}, {previous}},

	{{"rune"}, {"I sell blank runes and spell runes."}},
	{{"life", "fluid"}, {setType(2874), setData(11), setAmount(1), setPrice(60), "Do you want to buy life fluid for %P gold?", setTopic(2)}},
	{{"mana", "fluid"}, {setType(2874), setData(10), setAmount(1), setPrice(55), "Do you want to buy mana fluid for %P gold?", setTopic(2)}},
	{{"blank", "rune"}, {setType(3147), setAmount(1), setPrice(10), "Do you want to buy a blank rune for %P gold?", setTopic(1)}},
	{{"spellbook"}, {setType(3059), setAmount(1), setPrice(150), "Do you want to buy a spellbook for %P gold?", setTopic(1)}},

	{{P1, lt(1, P1), "life", "fluid"}, {setType(2874), setData(11), setAmount(P1), setPrice(mul(60, P1)), "Do you want to buy %A potions of life fluid for %P gold?", setTopic(2)}},
	{{P1, lt(1, P1), "mana", "fluid"}, {setType(2874), setData(10), setAmount(P1), setPrice(mul(55, P1)), "Do you want to buy %A potions of mana fluid for %P gold?", setTopic(2)}},
	{{P1, lt(1, P1), "blank", "rune"}, {setType(3147), setAmount(P1), setPrice(mul(10, P1)), "Do you want to buy %A blank runes for %P gold?", setTopic(1)}},
	{{P1, lt(1, P1), "spellbook"}, {setType(3059), setAmount(P1), setPrice(mul(150, P1)), "Do you want to buy %A spellbooks for %P gold?", setTopic(1)}},

	{{"deposit"}, {"I will pay you 5 gold for every empty vial. Ok?", setData(0), setTopic(3)}},
	{{"vial"}, {previous}},
	{{"flask"}, {previous}},

	{{eq(TOPIC, 1), "yes", ge(COUNTMONEY, PRICE)}, {"Here you are.", deleteMoney, create(TYPE)}},
	{{eq(TOPIC, 1), "yes"}, {"Come back, when you have enough money."}},
	{{eq(TOPIC, 1)}, {"Hmm, but next time."}},

	{{eq(TOPIC, 2), "yes", ge(COUNTMONEY, PRICE)}, {"Here you are. There is a deposit of 5 gold on the vial.", deleteMoney, create(TYPE)}},
	{{eq(TOPIC, 2), "yes"}, {"Come back, when you have enough money."}},
	{{eq(TOPIC, 2)}, {"Hmm, but next time."}},

	{{eq(TOPIC, 3), "yes", gt(count(2874), 0)}, {setAmount(count(2874)), setPrice(mul(AMOUNT, 5)), "Here you are ... %P gold.", delete(2874), createMoney}},
	{{eq(TOPIC, 3), "yes"}, {"You don't have any empty vials."}},
	{{eq(TOPIC, 3)}, {"Hmm, but please keep Tibia litter free."}},

	genTRunesFreeS,
	genTWandsFreeS,
}

function onCreatureSay(npc, creature, text, situation)
	behaviour.react(npc, rules, text, situation)
end

function onCreatureDisappear(npc, creature)
	behaviour.react(npc, rules, "", NPC_SITUATION_VANISH)
end
