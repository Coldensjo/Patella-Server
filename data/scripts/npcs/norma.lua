-- npcs/norma.lua: the behaviour of the NPC Norma, converted from
-- norma.npc by patella-convert. The NPC is its [[npc]] entry in npcs.toml;
-- lib/npc.lua matches and runs the rules below, in the order of the file
-- (scripting.hh, "NPCs"). The comments are those of norma.npc.
--
-- GIMUD - Graphical Interface Multi User Dungeon
-- norma.npc: Datenbank für die Händlerin Norma (Newbie)

local behaviour = require("lib.npc")
local ADDRESS = behaviour.ADDRESS
local BUSY = behaviour.BUSY
local VANISH = behaviour.VANISH
local PREMIUM = behaviour.PREMIUM
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
local create = behaviour.create
local delete = behaviour.delete
local createMoney = behaviour.createMoney
local deleteMoney = behaviour.deleteMoney
local queue = behaviour.queue
local idle = behaviour.idle
local nop = behaviour.nop
local previous = behaviour.previous

local rules = behaviour.rules {
	{{ADDRESS, "hello$", PREMIUM, FORCE}, {"Hello, hello, %N! Please come in, look, and buy!"}},
	{{ADDRESS, "hi$", PREMIUM, FORCE}, {previous}},
	{{ADDRESS, "hello$", FORCE}, {"I'm sorry %N, but I only serve premium account customers.", idle}},
	{{ADDRESS, "hi$", FORCE}, {previous}},
	{{ADDRESS, FORCE}, {idle}},
	{{BUSY, "hello$", PREMIUM, FORCE}, {"Not now, not now, sorry %N. Please wait a moment.", queue}},
	{{BUSY, "hi$", PREMIUM, FORCE}, {previous}},
	{{BUSY, "hello$", FORCE}, {"I'm sorry %N, but I only serve premium account customers."}},
	{{BUSY, "hi$", FORCE}, {previous}},
	{{BUSY, FORCE}, {nop}},
	{{VANISH, FORCE}, {"Bye, bye."}},

	{{"bye"}, {"Bye, bye.", idle}},
	{{"farewell"}, {previous}},
	{{"how", "are", "you"}, {"I am fine. I'm so glad to have you here as my customer."}},
	{{"sell"}, {"I sell much. Just read the blackboards for my awesome wares or just ask me."}},
	{{"job"}, {"I am a merchant. What can I do for you?"}},
	{{"name"}, {"My name is Norma. Do you want to buy something?"}},
	{{"time"}, {"It is about %T. I am so sorry, I have no watches to sell. Do you want to buy something else?"}},
	{{"help"}, {"I sell stuff to prices that low, that all other merchants would mock at my stupidity."}},
	{{"monster"}, {"If you want to challenge the monsters, you need some weapons and armor I sell. You need them definitely!"}},
	{{"dungeon"}, {"If you want to explore the dungeons, you have to equip yourself with the vital stuff I am selling. It's vital in the deepest sense of the word."}},
	{{"sewer"}, {"Oh, our sewer system is very primitive; so primitive it's overrun by rats. But the stuff I sell is safe from them. Do you want to buy some of it?"}},
	{{"king"}, {"The king encouraged salesmen to travel here, but only some dared to take the risk, and a risk it was!"}},
	{{"dallheim"}, {"Some call him a hero."}},
	{{"bug"}, {"Bugs plague this isle, but my wares are bugfree, totally bugfree."}},
	{{"stuff"}, {"I sell equipment of all kinds, all kind available on this isle. Just ask me about my wares if you are interested."}},
	{{"tibia"}, {"One day I will return to the continent as a rich, a very rich woman!"}},
	{{"thais"}, {"Thais is a crowded town."}},

	{{"wares"}, {"I sell weapons, shields, armor, helmets, and equipment. For what do you want to ask?"}},
	{{"offer"}, {previous}},
	{{"weapon"}, {"I sell spears, rapiers, sabres, daggers, hand axes, axes, and short swords. Just tell me what you want to buy."}},
	{{"armor"}, {"I sell jackets, coats, doublets, leather armor, and leather legs. Just tell me what you want to buy."}},
	{{"helmet"}, {"I sell leather helmets, studded helmets, and chain helmets. Just tell me what you want to buy."}},
	{{"shield"}, {"I sell wooden shields and studded shields. Just tell me what you want to buy."}},
	{{"equipment"}, {"I sell torches, bags, scrolls, shovels, picks, backpacks, sickles, scythes, ropes, fishing rods and sixpacks of worms. Just tell me what you want to buy."}},
	{{"do", "you", "sell"}, {"What do you need? I sell weapons, armor, helmets, shields, and equipment."}},
	{{"do", "you", "have"}, {previous}},

	{{"spear"}, {setType(3277), setAmount(1), setPrice(10), "Do you want to buy a spear for %P gold?", setTopic(1)}},
	{{"rapier"}, {setType(3272), setAmount(1), setPrice(15), "Do you want to buy a rapier for %P gold?", setTopic(1)}},
	{{"sabre"}, {setType(3273), setAmount(1), setPrice(25), "Do you want to buy a sabre for %P gold?", setTopic(1)}},
	{{"dagger"}, {setType(3267), setAmount(1), setPrice(5), "Do you want to buy a dagger for %P gold?", setTopic(1)}},
	{{"sickle"}, {setType(3293), setAmount(1), setPrice(8), "Do you want to buy a sickle for %P gold?", setTopic(1)}},
	{{"hand", "axe"}, {setType(3268), setAmount(1), setPrice(8), "Do you want to buy a hand axe for %P gold?", setTopic(1)}},
	{{"axe"}, {setType(3274), setAmount(1), setPrice(20), "Do you want to buy an axe for %P gold?", setTopic(1)}},
	{{"short", "sword"}, {setType(3294), setAmount(1), setPrice(30), "Do you want to buy a short sword for %P gold?", setTopic(1)}},
	{{"jacket"}, {setType(3561), setAmount(1), setPrice(10), "Do you want to buy a jacket for %P gold?", setTopic(1)}},
	{{"coat"}, {setType(3562), setAmount(1), setPrice(8), "Do you want to buy a coat for %P gold?", setTopic(1)}},
	{{"doublet"}, {setType(3379), setAmount(1), setPrice(16), "Do you want to buy a dublet for %P gold?", setTopic(1)}},
	{{"leather", "armor"}, {setType(3361), setAmount(1), setPrice(25), "Do you want to buy a leather armor for %P gold?", setTopic(1)}},
	{{"leather", "legs"}, {setType(3559), setAmount(1), setPrice(10), "Do you want to buy leather legs for %P gold?", setTopic(1)}},
	{{"leather", "helmet"}, {setType(3355), setAmount(1), setPrice(12), "Do you want to buy a leather helmet for %P gold?", setTopic(1)}},
	{{"studded", "helmet"}, {setType(3376), setAmount(1), setPrice(63), "Do you want to buy a studded helmet for %P gold?", setTopic(1)}},
	{{"chain", "helmet"}, {setType(3352), setAmount(1), setPrice(52), "Do you want to buy a chain helmet for %P gold?", setTopic(1)}},
	{{"wooden", "shield"}, {setType(3412), setAmount(1), setPrice(15), "Do you want to buy a wooden shield for %P gold?", setTopic(1)}},
	{{"studded", "shield"}, {setType(3426), setAmount(1), setPrice(50), "Do you want to buy a studded shield for %P gold?", setTopic(1)}},
	{{"torch"}, {setType(2920), setAmount(1), setPrice(2), "Do you want to buy a torch for %P gold?", setTopic(1)}},
	{{"bag"}, {setType(2853), setAmount(1), setPrice(4), "Do you want to buy a bag for %P gold?", setTopic(1)}},
	{{"scroll"}, {setType(2815), setAmount(1), setPrice(5), "Do you want to buy a scroll for %P gold?", setTopic(1)}},
	{{"shovel"}, {setType(3457), setAmount(1), setPrice(10), "Do you want to buy a shovel for %P gold?", setTopic(1)}},
	{{"pick"}, {"Sorry, I fear Al Dee owns the last ones on this isle."}},
	{{"backpack"}, {setType(2854), setAmount(1), setPrice(10), "Do you want to buy a backpack for %P gold?", setTopic(1)}},
	{{"scythe"}, {setType(3453), setAmount(1), setPrice(12), "Do you want to buy a scythe for %P gold?", setTopic(1)}},
	{{"rope"}, {setType(3003), setAmount(1), setPrice(50), "Do you want to buy a rope for %P gold?", setTopic(1)}},
	{{"rod"}, {setType(3483), setAmount(1), setPrice(150), "Do you want to buy a fishing rod for %P gold?", setTopic(1)}},

	{{P1, lt(1, P1), "spear"}, {setType(3277), setAmount(P1), setPrice(mul(10, P1)), "Do you want to buy %A spears for %P gold?", setTopic(1)}},
	{{P1, lt(1, P1), "rapier"}, {setType(3272), setAmount(P1), setPrice(mul(15, P1)), "Do you want to buy %A rapiers for %P gold?", setTopic(1)}},
	{{P1, lt(1, P1), "sabre"}, {setType(3273), setAmount(P1), setPrice(mul(25, P1)), "Do you want to buy %A sabres for %P gold?", setTopic(1)}},
	{{P1, lt(1, P1), "dagger"}, {setType(3267), setAmount(P1), setPrice(mul(5, P1)), "Do you want to buy %A daggers for %P gold?", setTopic(1)}},
	{{P1, lt(1, P1), "sickle"}, {setType(3293), setAmount(P1), setPrice(mul(8, P1)), "Do you want to buy %A sickles for %P gold?", setTopic(1)}},
	{{P1, lt(1, P1), "hand", "axe"}, {setType(3268), setAmount(P1), setPrice(mul(8, P1)), "Do you want to buy %A hand axes for %P gold?", setTopic(1)}},
	{{P1, lt(1, P1), "axe"}, {setType(3274), setAmount(P1), setPrice(mul(20, P1)), "Do you want to buy %A axes for %P gold?", setTopic(1)}},
	{{P1, lt(1, P1), "short", "sword"}, {setType(3294), setAmount(P1), setPrice(mul(30, P1)), "Do you want to buy %A short swords for %P gold?", setTopic(1)}},
	{{P1, lt(1, P1), "jacket"}, {setType(3561), setAmount(P1), setPrice(mul(10, P1)), "Do you want to buy %A jackets for %P gold?", setTopic(1)}},
	{{P1, lt(1, P1), "coat"}, {setType(3562), setAmount(P1), setPrice(mul(8, P1)), "Do you want to buy %A coats for %P gold?", setTopic(1)}},
	{{P1, lt(1, P1), "doublet"}, {setType(3379), setAmount(P1), setPrice(mul(16, P1)), "Do you want to buy %A dublets for %P gold?", setTopic(1)}},
	{{P1, lt(1, P1), "leather", "armor"}, {setType(3361), setAmount(P1), setPrice(mul(25, P1)), "Do you want to buy %A leather armors for %P gold?", setTopic(1)}},
	{{P1, lt(1, P1), "leather", "legs"}, {setType(3559), setAmount(P1), setPrice(mul(10, P1)), "Do you want to buy %A leather legs for %P gold?", setTopic(1)}},
	{{P1, lt(1, P1), "leather", "helmet"}, {setType(3355), setAmount(P1), setPrice(mul(12, P1)), "Do you want to buy %A leather helmets for %P gold?", setTopic(1)}},
	{{P1, lt(1, P1), "studded", "helmet"}, {setType(3376), setAmount(P1), setPrice(mul(63, P1)), "Do you want to buy %A studded helmets for %P gold?", setTopic(1)}},
	{{P1, lt(1, P1), "chain", "helmet"}, {setType(3352), setAmount(P1), setPrice(mul(52, P1)), "Do you want to buy %A chain helmets for %P gold?", setTopic(1)}},
	{{P1, lt(1, P1), "wooden", "shield"}, {setType(3412), setAmount(P1), setPrice(mul(15, P1)), "Do you want to buy %A wooden shields for %P gold?", setTopic(1)}},
	{{P1, lt(1, P1), "studded", "shield"}, {setType(3426), setAmount(P1), setPrice(mul(50, P1)), "Do you want to buy %A studded shields for %P gold?", setTopic(1)}},
	{{P1, lt(1, P1), "torch"}, {setType(2920), setAmount(P1), setPrice(mul(2, P1)), "Do you want to buy %A torches for %P gold?", setTopic(1)}},
	{{P1, lt(1, P1), "bag"}, {setType(2853), setAmount(P1), setPrice(mul(4, P1)), "Do you want to buy %A bags for %P gold?", setTopic(1)}},
	{{P1, lt(1, P1), "scroll"}, {setType(2815), setAmount(P1), setPrice(mul(5, P1)), "Do you want to buy %A scrolls for %P gold?", setTopic(1)}},
	{{P1, lt(1, P1), "shovel"}, {setType(3457), setAmount(P1), setPrice(mul(10, P1)), "Do you want to buy %A shovels for %P gold?", setTopic(1)}},
	{{P1, lt(1, P1), "backpack"}, {setType(2854), setAmount(P1), setPrice(mul(10, P1)), "Do you want to buy %A backpacks for %P gold?", setTopic(1)}},
	{{P1, lt(1, P1), "scythe"}, {setType(3453), setAmount(P1), setPrice(mul(12, P1)), "Do you want to buy %A scythes for %P gold?", setTopic(1)}},
	{{P1, lt(1, P1), "rope"}, {setType(3003), setAmount(P1), setPrice(mul(50, P1)), "Do you want to buy %A ropes for %P gold?", setTopic(1)}},
	{{P1, lt(1, P1), "rod"}, {setType(3483), setAmount(P1), setPrice(mul(150, P1)), "Do you want to buy %A fishing rods for %P gold?", setTopic(1)}},

	{{"worm"}, {"I sell worms only in sixpacks for 5 gold each, how many sixpacks of worms do you want to buy?"}},
	{{"worms"}, {previous}},
	{{P1, "worms"}, {setType(3492), setAmount(mul(P1, 6)), setPrice(mul(5, P1)), "So you want to buy some of my sixpacks with altogether %A worms for %P gold?", setTopic(1)}},
	{{P1, "six", "pack"}, {previous}},

	{{"sell", "club"}, {"I don't buy this garbage!"}},
	{{"sell", "dagger"}, {setType(3267), setAmount(1), setPrice(2), "Do you want to sell this garbage? I give you %P gold, ok?", setTopic(2)}},
	{{"sell", "spear"}, {setType(3277), setAmount(1), setPrice(3), "Do you want to sell this garbage? I give you %P gold, ok?", setTopic(2)}},
	{{"sell", "hand", "axe"}, {setType(3268), setAmount(1), setPrice(4), "Do you want to sell a hand axe for %P gold?", setTopic(2)}},
	{{"sell", "rapier"}, {setType(3272), setAmount(1), setPrice(5), "Do you want to sell a rapier for %P gold?", setTopic(2)}},
	{{"sell", "axe"}, {setType(3274), setAmount(1), setPrice(7), "Do you want to sell an axe for %P gold?", setTopic(2)}},
	{{"sell", "hatchet"}, {setType(3276), setAmount(1), setPrice(25), "Do you want to sell a hatchet for %P gold?", setTopic(2)}},
	{{"sell", "sabre"}, {setType(3273), setAmount(1), setPrice(12), "Do you want to sell a sabre for %P gold?", setTopic(2)}},
	{{"sell", "sword"}, {setType(3264), setAmount(1), setPrice(25), "Do you want to sell a sword for %P gold?", setTopic(2)}},
	{{"sell", "mace"}, {setType(3286), setAmount(1), setPrice(30), "Do you want to sell a mace for %P gold?", setTopic(2)}},
	{{"sell", "short", "sword"}, {setType(3294), setAmount(1), setPrice(10), "Do you want to sell a short sword for %P gold?", setTopic(2)}},
	{{"sell", "doublet"}, {setType(3379), setAmount(1), setPrice(3), "Do you want to sell a doublet for %P gold?", setTopic(2)}},
	{{"sell", "leather", "armor"}, {setType(3361), setAmount(1), setPrice(5), "Do you want to sell a leather armor for %P gold?", setTopic(2)}},
	{{"sell", "chain", "armor"}, {setType(3358), setAmount(1), setPrice(40), "Do you want to sell a chain armor for %P gold?", setTopic(2)}},
	{{"sell", "brass", "armor"}, {setType(3359), setAmount(1), setPrice(150), "Do you want to sell a brass armor for %P gold?", setTopic(2)}},
	{{"sell", "leather", "helmet"}, {setType(3355), setAmount(1), setPrice(3), "Do you want to sell a leather helmet for %P gold?", setTopic(2)}},
	{{"sell", "chain", "helmet"}, {setType(3352), setAmount(1), setPrice(12), "Do you want to sell a chain helmet for %P gold?", setTopic(2)}},
	{{"sell", "studded", "helmet"}, {setType(3376), setAmount(1), setPrice(20), "Do you want to sell a studded helmet for %P gold?", setTopic(2)}},
	{{"sell", "wooden", "shield"}, {setType(3412), setAmount(1), setPrice(3), "Do you want to sell a wooden shield for %P gold?", setTopic(2)}},
	{{"sell", "studded", "shield"}, {setType(3426), setAmount(1), setPrice(16), "Do you want to sell a studded shield for %P gold?", setTopic(2)}},
	{{"sell", "brass", "shield"}, {setType(3411), setAmount(1), setPrice(25), "Do you want to sell a brass shield for %P gold?", setTopic(2)}},
	{{"sell", "plate", "shield"}, {setType(3410), setAmount(1), setPrice(40), "Do you want to sell a plate shield for %P gold?", setTopic(2)}},
	{{"sell", "copper", "shield"}, {setType(3430), setAmount(1), setPrice(50), "Do you want to sell a copper shield for %P gold?", setTopic(2)}},
	{{"sell", "leather", "boots"}, {setType(3552), setAmount(1), setPrice(2), "Do you want to sell a pair of leather boots for %P gold?", setTopic(2)}},
	{{"sell", "rope"}, {setType(3003), setAmount(1), setPrice(8), "Do you want to sell a rope for %P gold?", setTopic(2)}},

	{{"sell", P1, lt(1, P1), "dagger"}, {setType(3267), setAmount(P1), setPrice(mul(2, P1)), "Do you want to sell %A daggers for %P gold?", setTopic(2)}},
	{{"sell", P1, lt(1, P1), "spear"}, {setType(3277), setAmount(P1), setPrice(mul(3, P1)), "Do you want to sell %A spears for %P gold?", setTopic(2)}},
	{{"sell", P1, lt(1, P1), "hand", "axe"}, {setType(3268), setAmount(P1), setPrice(mul(4, P1)), "Do you want to sell %A hand axes for %P gold?", setTopic(2)}},
	{{"sell", P1, lt(1, P1), "rapier"}, {setType(3272), setAmount(P1), setPrice(mul(5, P1)), "Do you want to sell %A rapiers for %P gold?", setTopic(2)}},
	{{"sell", P1, lt(1, P1), "axe"}, {setType(3274), setAmount(P1), setPrice(mul(7, P1)), "Do you want to sell %A axes for %P gold?", setTopic(2)}},
	{{"sell", P1, lt(1, P1), "hatchet"}, {setType(3276), setAmount(P1), setPrice(mul(25, P1)), "Do you want to sell %A hatchets for %P gold?", setTopic(2)}},
	{{"sell", P1, lt(1, P1), "sabre"}, {setType(3273), setAmount(P1), setPrice(mul(12, P1)), "Do you want to sell %A sabres for %P gold?", setTopic(2)}},
	{{"sell", P1, lt(1, P1), "sword"}, {setType(3264), setAmount(P1), setPrice(mul(25, P1)), "Do you want to sell %A swords for %P gold?", setTopic(2)}},
	{{"sell", P1, lt(1, P1), "mace"}, {setType(3286), setAmount(P1), setPrice(mul(30, P1)), "Do you want to sell %A maces for %P gold?", setTopic(2)}},
	{{"sell", P1, lt(1, P1), "short", "sword"}, {setType(3294), setAmount(P1), setPrice(mul(10, P1)), "Do you want to sell %A short swords for %P gold?", setTopic(2)}},
	{{"sell", P1, lt(1, P1), "doublet"}, {setType(3379), setAmount(P1), setPrice(mul(3, P1)), "Do you want to sell %A doublets for %P gold?", setTopic(2)}},
	{{"sell", P1, lt(1, P1), "leather", "armor"}, {setType(3361), setAmount(P1), setPrice(mul(5, P1)), "Do you want to sell %A leather armors for %P gold?", setTopic(2)}},
	{{"sell", P1, lt(1, P1), "chain", "armor"}, {setType(3358), setAmount(P1), setPrice(mul(40, P1)), "Do you want to sell %A chain armors for %P gold?", setTopic(2)}},
	{{"sell", P1, lt(1, P1), "brass", "armor"}, {setType(3359), setAmount(P1), setPrice(mul(150, P1)), "Do you want to sell %A brass armors for %P gold?", setTopic(2)}},
	{{"sell", P1, lt(1, P1), "leather", "helmet"}, {setType(3355), setAmount(P1), setPrice(mul(3, P1)), "Do you want to sell %A leather helmets for %P gold?", setTopic(2)}},
	{{"sell", P1, lt(1, P1), "chain", "helmet"}, {setType(3352), setAmount(P1), setPrice(mul(12, P1)), "Do you want to sell %A chain helmets for %P gold?", setTopic(2)}},
	{{"sell", P1, lt(1, P1), "studded", "helmet"}, {setType(3376), setAmount(P1), setPrice(mul(20, P1)), "Do you want to sell %A studded helmets for %P gold?", setTopic(2)}},
	{{"sell", P1, lt(1, P1), "wooden", "shield"}, {setType(3412), setAmount(P1), setPrice(mul(3, P1)), "Do you want to sell %A wooden shields for %P gold?", setTopic(2)}},
	{{"sell", P1, lt(1, P1), "studded", "shield"}, {setType(3426), setAmount(P1), setPrice(mul(16, P1)), "Do you want to sell %A studded shields for %P gold?", setTopic(2)}},
	{{"sell", P1, lt(1, P1), "brass", "shield"}, {setType(3411), setAmount(P1), setPrice(mul(25, P1)), "Do you want to sell %A brass shields for %P gold?", setTopic(2)}},
	{{"sell", P1, lt(1, P1), "plate", "shield"}, {setType(3410), setAmount(P1), setPrice(mul(40, P1)), "Do you want to sell %A plate shields for %P gold?", setTopic(2)}},
	{{"sell", P1, lt(1, P1), "copper", "shield"}, {setType(3430), setAmount(P1), setPrice(mul(50, P1)), "Do you want to sell %A copper shields for %P gold?", setTopic(2)}},
	{{"sell", P1, lt(1, P1), "leather", "boots"}, {setType(3552), setAmount(P1), setPrice(mul(2, P1)), "Do you want to sell %A pairs of leather boots for %P gold?", setTopic(2)}},
	{{"sell", P1, lt(1, P1), "rope"}, {setType(3003), setAmount(P1), setPrice(mul(8, P1)), "Do you want to sell %A ropes for %P gold?", setTopic(2)}},

	{{eq(TOPIC, 1), "yes", ge(COUNTMONEY, PRICE)}, {"Thank you. Here it is.", deleteMoney, create(TYPE)}},
	{{eq(TOPIC, 1), "yes"}, {"Sorry, you do not have enough gold."}},
	{{eq(TOPIC, 1)}, {"Maybe you will buy it another time."}},

	{{eq(TOPIC, 2), "yes", ge(count(TYPE), AMOUNT)}, {"Ok. Here is your money.", delete(TYPE), createMoney}},
	{{eq(TOPIC, 2), "yes"}, {"Sorry, you do not have one."}},
	{{eq(TOPIC, 2), "yes", gt(AMOUNT, 1)}, {"Sorry, you do not have that many."}},
	{{eq(TOPIC, 2)}, {"Maybe next time."}},
}

function onCreatureSay(npc, creature, text, situation)
	behaviour.react(npc, rules, text, situation)
end

function onCreatureDisappear(npc, creature)
	behaviour.react(npc, rules, "", NPC_SITUATION_VANISH)
end
