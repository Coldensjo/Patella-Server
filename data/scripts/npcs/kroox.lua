-- npcs/kroox.lua: the behaviour of the NPC Kroox, converted from
-- kroox.npc by patella-convert. The NPC is its [[npc]] entry in npcs.toml;
-- lib/npc.lua matches and runs the rules below, in the order of the file
-- (scripting.hh, "NPCs"). The comments are those of kroox.npc.
--
-- GIMUD - Graphical Interface Multi User Dungeon
-- kroox.npc: Datenbank für den Rüstungshändler Kroox

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
local add = behaviour.add
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
	{{ADDRESS, "hello$", FORCE}, {"Welcome to Kroox Quality Armor, %N!"}},
	{{ADDRESS, "hi$", FORCE}, {previous}},
	{{ADDRESS, "hiho$", FORCE}, {previous}},
	{{ADDRESS, FORCE}, {idle}},
	{{BUSY, "hello$", FORCE}, {"You next %N, jawoll!", queue}},
	{{BUSY, "hi$", FORCE}, {previous}},
	{{BUSY, "hiho$", FORCE}, {previous}},
	{{BUSY, FORCE}, {nop}},
	{{VANISH, FORCE}, {"Good bye. Come back soon."}},

	{{"bye"}, {"Good bye. Come back soon.", idle}},
	{{"farewell"}, {previous}},

	{{"measurements", gt(questValue(235), 0), lt(questValue(237), 1)}, {"Hm, well I guess its ok to tell you ... <tells you about Lokurs measurements>", setQuestValue(234, add(questValue(234), 1)), setQuestValue(237, 1)}},

	{{"measurements"}, {"UH? No clue what you are talking about, jawoll."}},

	{{"job"}, {"I sell best armor in land. My armor save you life. Better buy much."}},
	{{"shop"}, {previous}},
	{{"name"}, {"My name is Kroox Shieldbearer, son of Earth, from the Molten Rock."}},
	{{"time"}, {"It's %T now."}},
	{{"help"}, {"I sell and buy all kinds of armor. Dwarfish are the best, jawoll!"}},
	{{"dwarf$"}, {"We are proud fellows."}},
	{{"monster"}, {"You not be afraid, here you be save."}},
	{{"dungeon"}, {"Much fun you can have in dungeons. Much battle and much gold, jawoll!"}},
	{{"mines"}, {"Foreigners not welcome in mines. An evil basilisk rob our deeper mines."}},
	{{"thanks"}, {"I you thank, too."}},
	{{"thank", "you"}, {previous}},

	{{"buy"}, {"What you need? I sell armor, helmets, shields, and legs."}},
	{{"do", "you", "sell"}, {previous}},
	{{"do", "you", "have"}, {previous}},
	{{"offer"}, {"I offer armor, helmets, legs, and shields."}},
	{{"weapon"}, {"Ask in the shop next tunnel about that."}},
	{{"helmet"}, {"I sell chain helmets, brass helmets, iron helmets, and steel helmets. What you want?"}},
	{{"armor"}, {"I sell chain armor, brass armor, and plate armor. What you need?"}},
	{{"shield"}, {"I sell steel shields, dwarven shields, brass shields, and plate shields. What you want?"}},
	{{"trousers"}, {"I am selling chain legs, and brass legs. What you need?"}},
	{{"legs"}, {previous}},
	{{"you", "buy"}, {"You want sell any armor?"}},

	{{"chain", "armor"}, {setType(3358), setAmount(1), setPrice(200), "You want buy a chain armor for %P gold?", setTopic(1)}},
	{{"brass", "armor"}, {setType(3359), setAmount(1), setPrice(450), "You want buy a brass armor for %P gold?", setTopic(1)}},
	{{"plate", "armor"}, {setType(3357), setAmount(1), setPrice(1200), "You want buy a plate armor for %P gold?", setTopic(1)}},
	{{"chain", "helmet"}, {setType(3352), setAmount(1), setPrice(52), "You want buy a chain helmet for %P gold?", setTopic(1)}},
	{{"brass", "helmet"}, {setType(3354), setAmount(1), setPrice(120), "You want buy a brass helmet for %P gold?", setTopic(1)}},
	{{"iron", "helmet"}, {setType(3353), setAmount(1), setPrice(390), "You want buy an iron helmet for %P gold?", setTopic(1)}},
	{{"steel", "helmet"}, {setType(3351), setAmount(1), setPrice(580), "You want buy a steel helmet for %P gold?", setTopic(1)}},
	{{"steel", "shield"}, {setType(3409), setAmount(1), setPrice(240), "You want buy a steel shield for %P gold?", setTopic(1)}},
	{{"brass", "shield"}, {setType(3411), setAmount(1), setPrice(65), "You want buy a brass shield for %P gold?", setTopic(1)}},
	{{"plate", "shield"}, {setType(3410), setAmount(1), setPrice(125), "You want buy a plate shield for %P gold?", setTopic(1)}},
	{{"dwarven", "shield"}, {setType(3425), setAmount(1), setPrice(500), "You want buy a dwarven shield for %P gold?", setTopic(1)}},
	{{"chain", "legs"}, {setType(3558), setAmount(1), setPrice(80), "You want buy chain legs for %P gold?", setTopic(1)}},
	{{"brass", "legs"}, {setType(3372), setAmount(1), setPrice(195), "You want buy brass legs for %P gold?", setTopic(1)}},

	{{P1, lt(1, P1), "chain", "armor"}, {setType(3358), setAmount(P1), setPrice(mul(200, P1)), "You want buy %A chain armors for %P gold?", setTopic(1)}},
	{{P1, lt(1, P1), "brass", "armor"}, {setType(3359), setAmount(P1), setPrice(mul(450, P1)), "You want buy %A brass armors for %P gold?", setTopic(1)}},
	{{P1, lt(1, P1), "plate", "armor"}, {setType(3357), setAmount(P1), setPrice(mul(1200, P1)), "You want buy %A plate armors for %P gold?", setTopic(1)}},
	{{P1, lt(1, P1), "chain", "helmet"}, {setType(3352), setAmount(P1), setPrice(mul(52, P1)), "You want buy %A chain helmets for %P gold?", setTopic(1)}},
	{{P1, lt(1, P1), "brass", "helmet"}, {setType(3354), setAmount(P1), setPrice(mul(120, P1)), "You want buy %A brass helmets for %P gold?", setTopic(1)}},
	{{P1, lt(1, P1), "iron", "helmet"}, {setType(3353), setAmount(P1), setPrice(mul(390, P1)), "You want buy %A iron helmets for %P gold?", setTopic(1)}},
	{{P1, lt(1, P1), "steel", "helmet"}, {setType(3351), setAmount(P1), setPrice(mul(580, P1)), "You want buy %A steel helmets for %P gold?", setTopic(1)}},
	{{P1, lt(1, P1), "steel", "shield"}, {setType(3409), setAmount(P1), setPrice(mul(240, P1)), "You want buy %A steel shields for %P gold?", setTopic(1)}},
	{{P1, lt(1, P1), "brass", "shield"}, {setType(3411), setAmount(P1), setPrice(mul(65, P1)), "You want buy %A brass shields for %P gold?", setTopic(1)}},
	{{P1, lt(1, P1), "plate", "shield"}, {setType(3410), setAmount(P1), setPrice(mul(125, P1)), "You want buy %A plate shields for %P gold?", setTopic(1)}},
	{{P1, lt(1, P1), "dwarven", "shield"}, {setType(3425), setAmount(P1), setPrice(mul(500, P1)), "You want buy %A dwarven shields for %P gold?", setTopic(1)}},
	{{P1, lt(1, P1), "chain", "legs"}, {setType(3558), setAmount(P1), setPrice(mul(80, P1)), "You want buy %A chain legs for %P gold?", setTopic(1)}},
	{{P1, lt(1, P1), "brass", "legs"}, {setType(3372), setAmount(P1), setPrice(mul(195, P1)), "You want buy %A brass legs for %P gold?", setTopic(1)}},

	{{"sell", "chain", "armor"}, {setType(3358), setAmount(1), setPrice(40), "You want sell a chain armor for %P gold?", setTopic(2)}},
	{{"sell", "brass", "armor"}, {setType(3359), setAmount(1), setPrice(112), "You want sell a brass armor for %P gold?", setTopic(2)}},
	{{"sell", "plate", "armor"}, {setType(3357), setAmount(1), setPrice(240), "You want sell a plate armor for %P gold?", setTopic(2)}},
	{{"sell", "steel", "helmet"}, {setType(3351), setAmount(1), setPrice(293), "You want sell a steel helmet for %P gold?", setTopic(2)}},
	{{"sell", "chain", "helmet"}, {setType(3352), setAmount(1), setPrice(12), "You want sell a chain helmet for %P gold?", setTopic(2)}},
	{{"sell", "brass", "helmet"}, {setType(3354), setAmount(1), setPrice(30), "You want sell a brass helmet for %P gold?", setTopic(2)}},
	{{"sell", "viking", "helmet"}, {setType(3367), setAmount(1), setPrice(66), "You want sell a viking helmet for %P gold?", setTopic(2)}},
	{{"sell", "iron", "helmet"}, {setType(3353), setAmount(1), setPrice(145), "You want sell a iron helmet for %P gold?", setTopic(2)}},
	{{"sell", "devil", "helmet"}, {setType(3356), setAmount(1), setPrice(450), "You want sell a devil's helmet for %P gold?", setTopic(2)}},
	{{"sell", "warrior", "helmet"}, {setType(3369), setAmount(1), setPrice(696), "You want sell a warrior helmet for %P gold?", setTopic(2)}},
	{{"sell", "dwarven", "shield"}, {setType(3425), setAmount(1), setPrice(100), "You want sell a dwarven shield for %P gold?", setTopic(2)}},
	{{"sell", "plate", "shield"}, {setType(3410), setAmount(1), setPrice(45), "You want sell a plate shield for %P gold?", setTopic(2)}},
	{{"sell", "brass", "shield"}, {setType(3411), setAmount(1), setPrice(16), "You want sell a brass shield for %P gold?", setTopic(2)}},
	{{"sell", "wooden", "shield"}, {setType(3412), setAmount(1), setPrice(3), "You want sell a wooden shield for %P gold?", setTopic(2)}},
	{{"sell", "battle", "shield"}, {setType(3413), setAmount(1), setPrice(60), "You want sell a battle shield for %P gold?", setTopic(2)}},
	{{"sell", "brass", "legs"}, {setType(3372), setAmount(1), setPrice(49), "You want sell brass legs for %P gold?", setTopic(2)}},
	{{"sell", "chain", "legs"}, {setType(3558), setAmount(1), setPrice(20), "You want sell chain legs for %P gold?", setTopic(2)}},
	{{"sell", "plate", "legs"}, {setType(3557), setAmount(1), setPrice(115), "You want sell plate legs for %P gold?", setTopic(2)}},
	{{"sell", "knight", "legs"}, {setType(3371), setAmount(1), setPrice(375), "You want sell knight legs for %P gold?", setTopic(2)}},

	{{"sell", P1, lt(1, P1), "chain", "armor"}, {setType(3358), setAmount(P1), setPrice(mul(40, P1)), "You want sell %A chain armors for %P gold?", setTopic(2)}},
	{{"sell", P1, lt(1, P1), "brass", "armor"}, {setType(3359), setAmount(P1), setPrice(mul(112, P1)), "You want sell %A brass armors for %P gold?", setTopic(2)}},
	{{"sell", P1, lt(1, P1), "plate", "armor"}, {setType(3357), setAmount(P1), setPrice(mul(240, P1)), "You want sell %A plate armors for %P gold?", setTopic(2)}},
	{{"sell", P1, lt(1, P1), "steel", "helmet"}, {setType(3351), setAmount(P1), setPrice(mul(293, P1)), "You want sell %A steel helmets for %P gold?", setTopic(2)}},
	{{"sell", P1, lt(1, P1), "chain", "helmet"}, {setType(3352), setAmount(P1), setPrice(mul(12, P1)), "You want sell %A chain helmets for %P gold?", setTopic(2)}},
	{{"sell", P1, lt(1, P1), "brass", "helmet"}, {setType(3354), setAmount(P1), setPrice(mul(30, P1)), "You want sell %A brass helmets for %P gold?", setTopic(2)}},
	{{"sell", P1, lt(1, P1), "viking", "helmet"}, {setType(3367), setAmount(P1), setPrice(mul(66, P1)), "You want sell %A viking helmets for %P gold?", setTopic(2)}},
	{{"sell", P1, lt(1, P1), "iron", "helmet"}, {setType(3353), setAmount(P1), setPrice(mul(145, P1)), "You want sell %A iron helmets for %P gold?", setTopic(2)}},
	{{"sell", P1, lt(1, P1), "devil", "helmet"}, {setType(3356), setAmount(P1), setPrice(mul(450, P1)), "You want sell %A devil's helmets for %P gold?", setTopic(2)}},
	{{"sell", P1, lt(1, P1), "warrior", "helmet"}, {setType(3369), setAmount(P1), setPrice(mul(696, P1)), "You want sell %A warrior helmets for %P gold?", setTopic(2)}},
	{{"sell", P1, lt(1, P1), "dwarven", "shield"}, {setType(3425), setAmount(P1), setPrice(mul(100, P1)), "You want sell %A dwarven shields for %P gold?", setTopic(2)}},
	{{"sell", P1, lt(1, P1), "plate", "shield"}, {setType(3410), setAmount(P1), setPrice(mul(45, P1)), "You want sell %A plate shields for %P gold?", setTopic(2)}},
	{{"sell", P1, lt(1, P1), "brass", "shield"}, {setType(3411), setAmount(P1), setPrice(mul(16, P1)), "You want sell %A brass shields for %P gold?", setTopic(2)}},
	{{"sell", P1, lt(1, P1), "wooden", "shield"}, {setType(3412), setAmount(P1), setPrice(mul(3, P1)), "You want sell %A wooden shields for %P gold?", setTopic(2)}},
	{{"sell", P1, lt(1, P1), "battle", "shield"}, {setType(3413), setAmount(P1), setPrice(mul(60, P1)), "You want sell %A battle shields for %P gold?", setTopic(2)}},
	{{"sell", P1, lt(1, P1), "brass", "legs"}, {setType(3372), setAmount(P1), setPrice(mul(49, P1)), "You want sell %A brass legs for %P gold?", setTopic(2)}},
	{{"sell", P1, lt(1, P1), "chain", "legs"}, {setType(3558), setAmount(P1), setPrice(mul(20, P1)), "You want sell %A chain legs for %P gold?", setTopic(2)}},
	{{"sell", P1, lt(1, P1), "plate", "legs"}, {setType(3557), setAmount(P1), setPrice(mul(115, P1)), "You want sell %A plate legs for %P gold?", setTopic(2)}},
	{{"sell", P1, lt(1, P1), "knight", "legs"}, {setType(3371), setAmount(P1), setPrice(mul(375, P1)), "You want sell %A knight legs for %P gold?", setTopic(2)}},

	{{eq(TOPIC, 1), "yes", ge(COUNTMONEY, PRICE)}, {"Thank you. Here, you take it.", deleteMoney, create(TYPE)}},
	{{eq(TOPIC, 1), "yes"}, {"No, no, you do not hold enough gold."}},
	{{eq(TOPIC, 1)}, {"I think you will buy it another time."}},

	{{eq(TOPIC, 2), "yes", ge(count(TYPE), AMOUNT)}, {"Ok. Here is your money.", delete(TYPE), createMoney}},
	{{eq(TOPIC, 2), "yes"}, {"You not have one."}},
	{{eq(TOPIC, 2), "yes", gt(AMOUNT, 1)}, {"Sorry, you do not have that many."}},
	{{eq(TOPIC, 2)}, {"Maybe you sell it next time."}},
	{{"sam", "sen", eq(questValue(289), 1)}, {"Oh, so its you, he wrote me about? Sadly I have no dwarven armor in stock. But I give you the permission to retrive one from the mines. ...", "The problem is, some giant spiders made the tunnels where the storage is their new home. Good luck.", setQuestValue(289, 2)}},
}

function onCreatureSay(npc, creature, text, situation)
	behaviour.react(npc, rules, text, situation)
end

function onCreatureDisappear(npc, creature)
	behaviour.react(npc, rules, "", NPC_SITUATION_VANISH)
end
