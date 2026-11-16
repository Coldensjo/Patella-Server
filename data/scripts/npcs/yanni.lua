-- npcs/yanni.lua: the behaviour of the NPC Yanni, converted from
-- yanni.npc by patella-convert. The NPC is its [[npc]] entry in npcs.toml;
-- lib/npc.lua matches and runs the rules below, in the order of the file
-- (scripting.hh, "NPCs"). The comments are those of yanni.npc.
--
-- GIMUD - Graphical Interface Multi User Dungeon
-- yanni.npc: Datenbank für den Waffenhändler Yanni

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
	{{ADDRESS, "hello$", FORCE}, {"Salutations, %N. Welcome to the Ironhouse, warehouse of Abran Ironeye."}},
	{{ADDRESS, "hi$", FORCE}, {previous}},
	{{ADDRESS, "hello$", FORCE}, {previous}},
	{{ADDRESS, "hi$", FORCE}, {previous}},
	{{ADDRESS, FORCE}, {idle}},
	{{BUSY, "hello$", FORCE}, {"One moment please, %N.", queue}},
	{{BUSY, "hi$", FORCE}, {previous}},
	{{BUSY, FORCE}, {nop}},
	{{VANISH, FORCE}, {"How rude..."}},

	{{"bye"}, {"Thanks for doing business with us, traveller!", idle}},
	{{"farewell"}, {previous}},
	{{"job"}, {"I work for Abran Ironeye in the armor department."}},
	{{"shop"}, {"This shop belongs to Abran Ironeye."}},
	{{"market"}, {previous}},
	{{"ironhouse"}, {previous}},
	{{"name"}, {"My name is Yanni."}},
	{{"time"}, {"It is %T."}},
	{{"Abran", "Ironeye"}, {"A very hard boss, but if you ever talk to him, tell him I said he's a great man."}},
	{{"excalibug"}, {"I don't believe in the excalibug myth."}},
	{{"news"}, {"I know nothing of interest."}},
	{{"help"}, {"If you'd like to help, then buy something, because I'm paid on a commisioned basis."}},
	{{"commision"}, {"Commison is when you get paid only when you sell something. If I don't make some sales soon, my kids will get hungry!"}},
	{{"monster"}, {"Monsters? Where! I'm horrified of monsters."}},
	{{"thanks"}, {"You're welcome."}},
	{{"thank", "you"}, {previous}},
	{{"news"}, {"Don't buy from Xed. He is a thief. He steals my business all the time!"}},
	{{"offer"}, {"I sell armor, legs, helmets, and shields."}},
	{{"do", "you", "sell"}, {previous}},
	{{"do", "you", "have"}, {previous}},
	{{"helmet"}, {"I am selling chain helmets. Do you want to buy any?"}},
	{{"armor"}, {"I am selling chain and brass armor. What do you need?"}},
	{{"shield"}, {"I am selling wooden shields and steel shields. What do you want?"}},
	{{"trousers"}, {"I am selling chain legs. Do you want to buy any?"}},
	{{"legs"}, {previous}},

	{{"chain", "armor"}, {setType(3358), setAmount(1), setPrice(200), "Do you want to buy a chain armor for %P gold?", setTopic(1)}},
	{{"brass", "armor"}, {setType(3359), setAmount(1), setPrice(450), "Do you want to buy a brass armor for %P gold?", setTopic(1)}},
	{{"chain", "helmet"}, {setType(3352), setAmount(1), setPrice(52), "Do you want to buy a chain helmet for %P gold?", setTopic(1)}},
	{{"steel", "shield"}, {setType(3409), setAmount(1), setPrice(240), "Do you want to buy a steel shield for %P gold?", setTopic(1)}},
	{{"wooden", "shield"}, {setType(3412), setAmount(1), setPrice(15), "Do you want to buy a wooden shield for %P gold?", setTopic(1)}},
	{{"chain", "legs"}, {setType(3558), setAmount(1), setPrice(80), "Do you want to buy chain legs for %P gold?", setTopic(1)}},

	{{P1, lt(1, P1), "chain", "armor"}, {setType(3358), setAmount(P1), setPrice(mul(200, P1)), "Do you want to buy %A chain armors for %P gold?", setTopic(1)}},
	{{P1, lt(1, P1), "brass", "armor"}, {setType(3359), setAmount(P1), setPrice(mul(450, P1)), "Do you want to buy %A brass armors for %P gold?", setTopic(1)}},
	{{P1, lt(1, P1), "chain", "helmet"}, {setType(3352), setAmount(P1), setPrice(mul(52, P1)), "Do you want to buy %A chain helmets for %P gold?", setTopic(1)}},
	{{P1, lt(1, P1), "steel", "shield"}, {setType(3409), setAmount(P1), setPrice(mul(240, P1)), "Do you want to buy %A steel shields for %P gold?", setTopic(1)}},
	{{P1, lt(1, P1), "wooden", "shield"}, {setType(3412), setAmount(P1), setPrice(mul(15, P1)), "Do you want to buy %A wooden shields for %P gold?", setTopic(1)}},
	{{P1, lt(1, P1), "chain", "legs"}, {setType(3558), setAmount(P1), setPrice(mul(80, P1)), "Do you want to buy %A chain legs for %P gold?", setTopic(1)}},

	{{"sell", "leather", "armor"}, {setType(3361), setAmount(1), setPrice(12), "Do you want to sell a leather armor for %P gold?", setTopic(2)}},
	{{"sell", "chain", "armor"}, {setType(3358), setAmount(1), setPrice(70), "Do you want to sell a chain armor for %P gold?", setTopic(2)}},
	{{"sell", "brass", "armor"}, {setType(3359), setAmount(1), setPrice(150), "Do you want to sell a brass armor for %P gold?", setTopic(2)}},
	{{"sell", "plate", "armor"}, {setType(3357), setAmount(1), setPrice(400), "Do you want to sell a plate armor for %P gold?", setTopic(2)}},
	{{"sell", "chain", "legs"}, {setType(3558), setAmount(1), setPrice(25), "Do you want to sell chain legs for %P gold?", setTopic(2)}},
	{{"sell", "leather", "helmet"}, {setType(3355), setAmount(1), setPrice(4), "Do you want to sell a leather helmet for %P gold?", setTopic(2)}},
	{{"sell", "chain", "helmet"}, {setType(3352), setAmount(1), setPrice(17), "Do you want to sell a chain helmet for %P gold?", setTopic(2)}},
	{{"sell", "steel", "helmet"}, {setType(3351), setAmount(1), setPrice(190), "Do you want to sell a steel helmet for %P gold?", setTopic(2)}},
	{{"sell", "wooden", "shield"}, {setType(3412), setAmount(1), setPrice(5), "Do you want to sell a wooden shield for %P gold?", setTopic(2)}},
	{{"sell", "battle", "shield"}, {setType(3413), setAmount(1), setPrice(95), "Do you want to sell a battle shield for %P gold?", setTopic(2)}},
	{{"sell", "steel", "shield"}, {setType(3409), setAmount(1), setPrice(80), "Do you want to sell a steel shield for %P gold?", setTopic(2)}},

	{{"sell", P1, lt(1, P1), "leather", "armor"}, {setType(3361), setAmount(P1), setPrice(mul(12, P1)), "Do you want to sell %A leather armors for %P gold?", setTopic(2)}},
	{{"sell", P1, lt(1, P1), "chain", "armor"}, {setType(3358), setAmount(P1), setPrice(mul(70, P1)), "Do you want to sell %A chain armors for %P gold?", setTopic(2)}},
	{{"sell", P1, lt(1, P1), "brass", "armor"}, {setType(3359), setAmount(P1), setPrice(mul(150, P1)), "Do you want to sell %A brass armors for %P gold?", setTopic(2)}},
	{{"sell", P1, lt(1, P1), "plate", "armor"}, {setType(3357), setAmount(P1), setPrice(mul(400, P1)), "Do you want to sell %A plate armors for %P gold?", setTopic(2)}},
	{{"sell", P1, lt(1, P1), "chain", "legs"}, {setType(3558), setAmount(P1), setPrice(mul(25, P1)), "Do you want to sell %A chain legs for %P gold?", setTopic(2)}},
	{{"sell", P1, lt(1, P1), "leather", "helmet"}, {setType(3355), setAmount(P1), setPrice(mul(4, P1)), "Do you want to sell %A leather helmets for %P gold?", setTopic(2)}},
	{{"sell", P1, lt(1, P1), "chain", "helmet"}, {setType(3352), setAmount(P1), setPrice(mul(17, P1)), "Do you want to sell %A chain helmets for %P gold?", setTopic(2)}},
	{{"sell", P1, lt(1, P1), "steel", "helmet"}, {setType(3351), setAmount(P1), setPrice(mul(190, P1)), "Do you want to sell %A steel helmets for %P gold?", setTopic(2)}},
	{{"sell", P1, lt(1, P1), "wooden", "shield"}, {setType(3412), setAmount(P1), setPrice(mul(5, P1)), "Do you want to sell %A wooden shields for %P gold?", setTopic(2)}},
	{{"sell", P1, lt(1, P1), "battle", "shield"}, {setType(3413), setAmount(P1), setPrice(mul(95, P1)), "Do you want to sell %A battle shields for %P gold?", setTopic(2)}},
	{{"sell", P1, lt(1, P1), "steel", "shield"}, {setType(3409), setAmount(P1), setPrice(mul(80, P1)), "Do you want to sell %A steel shields for %P gold?", setTopic(2)}},

	{{eq(TOPIC, 1), "yes", ge(COUNTMONEY, PRICE)}, {"Thank you. Here it is.", deleteMoney, create(TYPE)}},
	{{eq(TOPIC, 1), "yes"}, {"Sorry, you do not have enough gold."}},
	{{eq(TOPIC, 1)}, {"Maybe you will buy it another time."}},

	{{eq(TOPIC, 2), "yes", ge(count(TYPE), AMOUNT)}, {"Ok. Here is your money.", delete(TYPE), createMoney}},
	{{eq(TOPIC, 2), "yes"}, {"Sorry, you do not have one."}},
	{{eq(TOPIC, 2), "yes", gt(AMOUNT, 1)}, {"Sorry, you do not have so much."}},
	{{eq(TOPIC, 2)}, {"Maybe next time."}},
}

function onCreatureSay(npc, creature, text, situation)
	behaviour.react(npc, rules, text, situation)
end

function onCreatureDisappear(npc, creature)
	behaviour.react(npc, rules, "", NPC_SITUATION_VANISH)
end
