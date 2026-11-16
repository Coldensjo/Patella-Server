-- npcs/cornelia.lua: the behaviour of the NPC Cornelia, converted from
-- cornelia.npc by patella-convert. The NPC is its [[npc]] entry in npcs.toml;
-- lib/npc.lua matches and runs the rules below, in the order of the file
-- (scripting.hh, "NPCs"). The comments are those of cornelia.npc.
--
-- GIMUD - Graphical Interface Multi User Dungeon
-- cornelia.npc: Datenbank für die Rüstungshändlerin Cornelia

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
	{{ADDRESS, "hello$", FORCE}, {"Welcome to the finest armorshop in the land, %N!"}},
	{{ADDRESS, "hi$", FORCE}, {previous}},
	{{ADDRESS, FORCE}, {idle}},
	{{BUSY, "hello$", FORCE}, {"One moment please, %N.", queue}},
	{{BUSY, "hi$", FORCE}, {"One moment please, %N.", queue}},
	{{BUSY, FORCE}, {nop}},
	{{VANISH, FORCE}, {"Good bye. Come back soon."}},

	{{"bye"}, {"Good bye. Come back soon.", idle}},
	{{"farewell"}, {previous}},
	{{"job"}, {"I run this armoury. If you want to proctect your life you'd better buy my wares."}},
	{{"shop"}, {previous}},
	{{"name"}, {"My name is Cornelia."}},
	{{"time"}, {"It's %T right now."}},
	{{"help"}, {"I sell and buy armor, helmets, and shields. Only the dwarfs can make better ones."}},
	{{"dwarfs"}, {"The ancient dwarfen clan halls are far to the east from here."}},
	{{"monster"}, {"With my armor you need not fear any monsters!"}},
	{{"dungeon"}, {"While exploring the dungeons of the land you will learn how important a good armor is."}},
	{{"sewer"}, {"Sewers are males' business."}},
	{{"thanks"}, {"You are welcome."}},
	{{"thank", "you"}, {"You are welcome."}},
	{{"ghostlands"}, {"THE GHOSTLANDS??? Make sure to buy the best protection in store before you get even close to them."}},

	{{"buy"}, {"What do you need? I sell armor, helmets, shields, and trousers."}},
	{{"do", "you", "sell"}, {previous}},
	{{"do", "you", "have"}, {previous}},
	{{"offer"}, {"My offers are armor, helmets, trousers, and shields."}},

	{{"weapon"}, {"Ask Rowenna in the other shop about it."}},
	{{"helmet"}, {"I am selling leather helmets, chain helmets, brass helmets, and viking helmets. What do you want?"}},
	{{"armor"}, {"I am selling leather armor, chain armor, and brass armor. What do you need?"}},
	{{"shield"}, {"I am selling wooden shields, brass shields, and plate shields. What do you want?"}},
	{{"trousers"}, {"I am selling chain legs and brass legs. What do you need?"}},
	{{"legs"}, {previous}},

	{{"leather", "armor"}, {setType(3361), setAmount(1), setPrice(35), "Do you want to buy a leather armor for %P gold?", setTopic(1)}},
	{{"chain", "armor"}, {setType(3358), setAmount(1), setPrice(200), "Do you want to buy a chain armor for %P gold?", setTopic(1)}},
	{{"brass", "armor"}, {setType(3359), setAmount(1), setPrice(450), "Do you want to buy a brass armor for %P gold?", setTopic(1)}},
	{{"leather", "helmet"}, {setType(3355), setAmount(1), setPrice(12), "Do you want to buy a leather helmet for %P gold?", setTopic(1)}},
	{{"chain", "helmet"}, {setType(3352), setAmount(1), setPrice(52), "Do you want to buy a chain helmet for %P gold?", setTopic(1)}},
	{{"brass", "helmet"}, {setType(3354), setAmount(1), setPrice(120), "Do you want to buy a brass helmet for %P gold?", setTopic(1)}},
	{{"viking", "helmet"}, {setType(3367), setAmount(1), setPrice(265), "Do you want to buy a viking helmet for %P gold?", setTopic(1)}},
	{{"wooden", "shield"}, {setType(3412), setAmount(1), setPrice(15), "Do you want to buy a wooden shield for %P gold?", setTopic(1)}},
	{{"brass", "shield"}, {setType(3411), setAmount(1), setPrice(65), "Do you want to buy a brass shield for %P gold?", setTopic(1)}},
	{{"plate", "shield"}, {setType(3410), setAmount(1), setPrice(125), "Do you want to buy a plate shield for %P gold?", setTopic(1)}},
	{{"brass", "legs"}, {setType(3372), setAmount(1), setPrice(195), "Do you want to buy brass legs for %P gold?", setTopic(1)}},
	{{"chain", "legs"}, {setType(3558), setAmount(1), setPrice(80), "Do you want to buy chain legs for %P gold?", setTopic(1)}},

	{{P1, lt(1, P1), "leather", "armor"}, {setType(3361), setAmount(P1), setPrice(mul(35, P1)), "Do you want to buy %A leather armors for %P gold?", setTopic(1)}},
	{{P1, lt(1, P1), "chain", "armor"}, {setType(3358), setAmount(P1), setPrice(mul(200, P1)), "Do you want to buy %A chain armors for %P gold?", setTopic(1)}},
	{{P1, lt(1, P1), "brass", "armor"}, {setType(3359), setAmount(P1), setPrice(mul(450, P1)), "Do you want to buy %A brass armors for %P gold?", setTopic(1)}},
	{{P1, lt(1, P1), "leather", "helmet"}, {setType(3355), setAmount(P1), setPrice(mul(12, P1)), "Do you want to buy %A leather helmets for %P gold?", setTopic(1)}},
	{{P1, lt(1, P1), "chain", "helmet"}, {setType(3352), setAmount(P1), setPrice(mul(52, P1)), "Do you want to buy %A chain helmets for %P gold?", setTopic(1)}},
	{{P1, lt(1, P1), "brass", "helmet"}, {setType(3354), setAmount(P1), setPrice(mul(120, P1)), "Do you want to buy %A brass helmets for %P gold?", setTopic(1)}},
	{{P1, lt(1, P1), "viking", "helmet"}, {setType(3367), setAmount(P1), setPrice(mul(265, P1)), "Do you want to buy %A viking helmets for %P gold?", setTopic(1)}},
	{{P1, lt(1, P1), "wooden", "shield"}, {setType(3412), setAmount(P1), setPrice(mul(15, P1)), "Do you want to buy %A wooden shields for %P gold?", setTopic(1)}},
	{{P1, lt(1, P1), "brass", "shield"}, {setType(3411), setAmount(P1), setPrice(mul(65, P1)), "Do you want to buy %A brass shields for %P gold?", setTopic(1)}},
	{{P1, lt(1, P1), "plate", "shield"}, {setType(3410), setAmount(P1), setPrice(mul(125, P1)), "Do you want to buy %A plate shields for %P gold?", setTopic(1)}},
	{{P1, lt(1, P1), "brass", "legs"}, {setType(3372), setAmount(P1), setPrice(mul(195, P1)), "Do you want to buy %A brass legs for %P gold?", setTopic(1)}},
	{{P1, lt(1, P1), "chain", "legs"}, {setType(3558), setAmount(P1), setPrice(mul(80, P1)), "Do you want to buy %A chain legs for %P gold?", setTopic(1)}},

	{{"sell", "leather", "armor"}, {setType(3361), setAmount(1), setPrice(5), "Do you want to sell leather armor for %P gold?", setTopic(2)}},
	{{"sell", "chain", "armor"}, {setType(3358), setAmount(1), setPrice(40), "Do you want to sell chain armor for %P gold?", setTopic(2)}},
	{{"sell", "brass", "armor"}, {setType(3359), setAmount(1), setPrice(112), "Do you want to sell brass armor for %P gold?", setTopic(2)}},
	{{"sell", "plate", "armor"}, {setType(3357), setAmount(1), setPrice(240), "Do you want to sell plate armor for %P gold?", setTopic(2)}},
	{{"sell", "steel", "helmet"}, {setType(3351), setAmount(1), setPrice(293), "Do you want to sell a steel helmet for %P gold?", setTopic(2)}},
	{{"sell", "chain", "helmet"}, {setType(3352), setAmount(1), setPrice(12), "Do you want to sell a chain helmet for %P gold?", setTopic(2)}},
	{{"sell", "brass", "helmet"}, {setType(3354), setAmount(1), setPrice(30), "Do you want to sell a brass helmet for %P gold?", setTopic(2)}},
	{{"sell", "leather", "helmet"}, {setType(3355), setAmount(1), setPrice(3), "Do you want to sell a leather helmet for %P gold?", setTopic(2)}},
	{{"sell", "viking", "helmet"}, {setType(3367), setAmount(1), setPrice(66), "Do you want to sell a viking helmet for %P gold?", setTopic(2)}},
	{{"sell", "plate", "shield"}, {setType(3410), setAmount(1), setPrice(45), "Do you want to sell a plate shield for %P gold?", setTopic(2)}},
	{{"sell", "brass", "shield"}, {setType(3411), setAmount(1), setPrice(16), "Do you want to sell a brass shield for %P gold?", setTopic(2)}},
	{{"sell", "wooden", "shield"}, {setType(3412), setAmount(1), setPrice(3), "Do you want to sell a wooden shield for %P gold?", setTopic(2)}},
	{{"sell", "battle", "shield"}, {setType(3413), setAmount(1), setPrice(60), "Do you want to sell a battle shield for %P gold?", setTopic(2)}},
	{{"sell", "brass", "legs"}, {setType(3372), setAmount(1), setPrice(49), "Do you want to sell brass legs for %P gold?", setTopic(2)}},
	{{"sell", "chain", "legs"}, {setType(3558), setAmount(1), setPrice(20), "Do you want to sell chain legs for %P gold?", setTopic(2)}},

	{{"sell", P1, lt(1, P1), "leather", "armor"}, {setType(3361), setAmount(P1), setPrice(mul(5, P1)), "Do you want to sell %A leather armors for %P gold?", setTopic(2)}},
	{{"sell", P1, lt(1, P1), "chain", "armor"}, {setType(3358), setAmount(P1), setPrice(mul(40, P1)), "Do you want to sell %A chain armors for %P gold?", setTopic(2)}},
	{{"sell", P1, lt(1, P1), "brass", "armor"}, {setType(3359), setAmount(P1), setPrice(mul(112, P1)), "Do you want to sell %A brass armors for %P gold?", setTopic(2)}},
	{{"sell", P1, lt(1, P1), "plate", "armor"}, {setType(3357), setAmount(P1), setPrice(mul(240, P1)), "Do you want to sell %A plate armors for %P gold?", setTopic(2)}},
	{{"sell", P1, lt(1, P1), "steel", "helmet"}, {setType(3351), setAmount(P1), setPrice(mul(293, P1)), "Do you want to sell %A steel helmets for %P gold?", setTopic(2)}},
	{{"sell", P1, lt(1, P1), "chain", "helmet"}, {setType(3352), setAmount(P1), setPrice(mul(12, P1)), "Do you want to sell %A chain helmets for %P gold?", setTopic(2)}},
	{{"sell", P1, lt(1, P1), "brass", "helmet"}, {setType(3354), setAmount(P1), setPrice(mul(30, P1)), "Do you want to sell %A brass helmets for %P gold?", setTopic(2)}},
	{{"sell", P1, lt(1, P1), "leather", "helmet"}, {setType(3355), setAmount(P1), setPrice(mul(3, P1)), "Do you want to sell %A leather helmets for %P gold?", setTopic(2)}},
	{{"sell", P1, lt(1, P1), "viking", "helmet"}, {setType(3367), setAmount(P1), setPrice(mul(66, P1)), "Do you want to sell %A viking helmets for %P gold?", setTopic(2)}},
	{{"sell", P1, lt(1, P1), "plate", "shield"}, {setType(3410), setAmount(P1), setPrice(mul(45, P1)), "Do you want to sell %A plate shields for %P gold?", setTopic(2)}},
	{{"sell", P1, lt(1, P1), "brass", "shield"}, {setType(3411), setAmount(P1), setPrice(mul(16, P1)), "Do you want to sell %A brass shields for %P gold?", setTopic(2)}},
	{{"sell", P1, lt(1, P1), "wooden", "shield"}, {setType(3412), setAmount(P1), setPrice(mul(3, P1)), "Do you want to sell %A wooden shields for %P gold?", setTopic(2)}},
	{{"sell", P1, lt(1, P1), "battle", "shield"}, {setType(3413), setAmount(P1), setPrice(mul(60, P1)), "Do you want to sell %A battle shields for %P gold?", setTopic(2)}},
	{{"sell", P1, lt(1, P1), "brass", "legs"}, {setType(3372), setAmount(P1), setPrice(mul(49, P1)), "Do you want to sell %A brass legs for %P gold?", setTopic(2)}},
	{{"sell", P1, lt(1, P1), "chain", "legs"}, {setType(3558), setAmount(P1), setPrice(mul(20, P1)), "Do you want to sell %A chain legs for %P gold?", setTopic(2)}},

	{{eq(TOPIC, 1), "yes", ge(COUNTMONEY, PRICE)}, {"Thank you. Here it is.", deleteMoney, create(TYPE)}},
	{{eq(TOPIC, 1), "yes"}, {"Sorry, you do not have enough gold."}},
	{{eq(TOPIC, 1)}, {"Maybe you will buy it another time."}},

	{{eq(TOPIC, 2), "yes", ge(count(TYPE), AMOUNT)}, {"Ok. Here is your money.", delete(TYPE), createMoney}},
	{{eq(TOPIC, 2), "yes"}, {"Sorry, you do not have one."}},
	{{eq(TOPIC, 2), "yes", gt(AMOUNT, 1)}, {"Sorry, you do not have so many."}},
	{{eq(TOPIC, 2)}, {"Maybe next time."}},
}

function onCreatureSay(npc, creature, text, situation)
	behaviour.react(npc, rules, text, situation)
end

function onCreatureDisappear(npc, creature)
	behaviour.react(npc, rules, "", NPC_SITUATION_VANISH)
end
