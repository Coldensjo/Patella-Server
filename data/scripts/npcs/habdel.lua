-- npcs/habdel.lua: the behaviour of the NPC Habdel, converted from
-- habdel.npc by patella-convert. The NPC is its [[npc]] entry in npcs.toml;
-- lib/npc.lua matches and runs the rules below, in the order of the file
-- (scripting.hh, "NPCs"). The comments are those of habdel.npc.
--
-- GIMUD - Graphical Interface Multi User Dungeon
-- Habdel.npc: Datenbank für den Waffenhändler Habdel

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
	{{ADDRESS, "hello$", FORCE}, {"Welcome %N! See the fine weapons I sell."}},
	{{ADDRESS, "hi$", FORCE}, {previous}},
	{{ADDRESS, FORCE}, {idle}},
	{{BUSY, "hello$", FORCE}, {"I will have finished soon %N.", queue}},
	{{BUSY, "hi$", FORCE}, {previous}},
	{{BUSY, FORCE}, {nop}},
	{{VANISH, FORCE}, {"Good bye and please come back soon.", idle}},

	{{"bye"}, {"Good bye. Come back soon.", idle}},
	{{"job"}, {"I sell weapons that are as lethal as the bite of the desertlion and as quick as the sandwasp."}},
	{{"shop"}, {previous}},
	{{"name"}, {"My name is Habdel Ibn Haqui."}},
	{{"time"}, {"Don't worry, there is enough time left to finish our deal."}},
	{{"help"}, {"I sell and buy weapons. Just ask what you need or tell me what you offer."}},
	{{"monster"}, {"With my weapons you have to fear the monsters no longer and you will brave any danger or dungeon!"}},
	{{"dungeon"}, {previous}},
	{{"drefia"}, {"Even the undead will fall a second time for the weapons you buy from me."}},
	{{"thanks"}, {"You are welcome."}},
	{{"thank", "you"}, {previous}},
	{{"do", "you", "sell"}, {"Which of my powerful weapons do you need?"}},
	{{"do", "you", "have"}, {previous}},
	{{"offer"}, {"My offers are light and heavy weapons."}},
	{{"weapon"}, {previous}},
	{{"light"}, {"I have clubs, daggers, spears, swords, maces, rapiers, morning stars, and sabres. What's your choice?"}},
	{{"light", "weapon"}, {previous}},
	{{"heavy"}, {"I have the best two handed swords in Tibia. I also sell battle hammers and battle axes. What's your choice?"}},
	{{"heavy", "weapon"}, {previous}},
	{{"armor"}, {"I sell only weapons. For armor, ask Azil in the other shop."}},
	{{"shield"}, {previous}},
	{{"helmet"}, {previous}},
	{{"trousers"}, {previous}},
	{{"legs"}, {previous}},

	{{"spear"}, {setType(3277), setAmount(1), setPrice(10), "Do you want to buy a spear for %P gold?", setTopic(1)}},
	{{"rapier"}, {setType(3272), setAmount(1), setPrice(15), "Do you want to buy a rapier for %P gold?", setTopic(1)}},
	{{"sabre"}, {setType(3273), setAmount(1), setPrice(25), "Do you want to buy a sabre for %P gold?", setTopic(1)}},
	{{"sword"}, {setType(3264), setAmount(1), setPrice(85), "Do you want to buy a sword for %P gold?", setTopic(1)}},
	{{"battle", "axe"}, {setType(3266), setAmount(1), setPrice(235), "Do you want to buy a battle axe for %P gold?", setTopic(1)}},
	{{"battle", "hammer"}, {setType(3305), setAmount(1), setPrice(350), "Do you want to buy a battle hammer for %P gold?", setTopic(1)}},
	{{"morning", "star"}, {setType(3282), setAmount(1), setPrice(430), "Do you want to buy a morning star for %P gold?", setTopic(1)}},
	{{"two", "handed", "sword"}, {setType(3265), setAmount(1), setPrice(950), "Do you want to buy a two handed sword for %P gold?", setTopic(1)}},
	{{"club"}, {setType(3270), setAmount(1), setPrice(5), "Do you want to buy a club for %P gold?", setTopic(1)}},
	{{"dagger"}, {setType(3267), setAmount(1), setPrice(5), "Do you want to buy a dagger for %P gold?", setTopic(1)}},
	{{"mace"}, {setType(3286), setAmount(1), setPrice(90), "Do you want to buy a mace for %P gold?", setTopic(1)}},
	{{"throwing", "star"}, {setType(3287), setAmount(1), setPrice(50), "Do you want to buy a throwing star for %P gold?", setTopic(1)}},

	{{P1, lt(1, P1), "spear"}, {setType(3277), setAmount(P1), setPrice(mul(10, P1)), "Do you want to buy %A spears for %P gold?", setTopic(1)}},
	{{P1, lt(1, P1), "rapier"}, {setType(3272), setAmount(P1), setPrice(mul(15, P1)), "Do you want to buy %A rapiers for %P gold?", setTopic(1)}},
	{{P1, lt(1, P1), "sabre"}, {setType(3273), setAmount(P1), setPrice(mul(25, P1)), "Do you want to buy %A sabres for %P gold?", setTopic(1)}},
	{{P1, lt(1, P1), "sword"}, {setType(3264), setAmount(P1), setPrice(mul(85, P1)), "Do you want to buy %A swords for %P gold?", setTopic(1)}},
	{{P1, lt(1, P1), "battle", "axe"}, {setType(3266), setAmount(P1), setPrice(mul(235, P1)), "Do you want to buy %A battle axes for %P gold?", setTopic(1)}},
	{{P1, lt(1, P1), "battle", "hammer"}, {setType(3305), setAmount(P1), setPrice(mul(350, P1)), "Do you want to buy %A battle hammers for %P gold?", setTopic(1)}},
	{{P1, lt(1, P1), "morning", "star"}, {setType(3282), setAmount(P1), setPrice(mul(430, P1)), "Do you want to buy %A morning stars for %P gold?", setTopic(1)}},
	{{P1, lt(1, P1), "two", "handed", "sword"}, {setType(3265), setAmount(P1), setPrice(mul(950, P1)), "Do you want to buy %A two handed swords for %P gold?", setTopic(1)}},
	{{P1, lt(1, P1), "club"}, {setType(3270), setAmount(P1), setPrice(mul(5, P1)), "Do you want to buy %A clubs for %P gold?", setTopic(1)}},
	{{P1, lt(1, P1), "dagger"}, {setType(3267), setAmount(P1), setPrice(mul(5, P1)), "Do you want to buy %A daggers for %P gold?", setTopic(1)}},
	{{P1, lt(1, P1), "mace"}, {setType(3286), setAmount(P1), setPrice(mul(90, P1)), "Do you want to buy %A maces for %P gold?", setTopic(1)}},
	{{P1, lt(1, P1), "throwing", "star"}, {setType(3287), setAmount(P1), setPrice(mul(50, P1)), "Do you want to buy %A throwing stars for %P gold?", setTopic(1)}},

	{{"sell", "mace"}, {setType(3286), setAmount(1), setPrice(23), "Do you want to sell a mace for %P gold?", setTopic(2)}},
	{{"sell", "dagger"}, {setType(3267), setAmount(1), setPrice(1), "Do you want to sell a dagger for %P gold?", setTopic(2)}},
	{{"sell", "carlin", "sword"}, {setType(3283), setAmount(1), setPrice(118), "Do you want to sell a carlin sword for %P gold?", setTopic(2)}},
	{{"sell", "club"}, {setType(3270), setAmount(1), setPrice(1), "Are you sure, you want to sell this garbage? Hmm, I give you %P gold, ok?", setTopic(2)}},
	{{"sell", "spear"}, {setType(3277), setAmount(1), setPrice(1), "Do you want to sell this garbage? I give you %P gold, ok?", setTopic(2)}},
	{{"sell", "rapier"}, {setType(3272), setAmount(1), setPrice(3), "Do you want to sell a rapier for %P gold?", setTopic(2)}},
	{{"sell", "sabre"}, {setType(3273), setAmount(1), setPrice(5), "Do you want to sell a sabre for %P gold?", setTopic(2)}},
	{{"sell", "sword"}, {setType(3264), setAmount(1), setPrice(15), "Do you want to sell a sword for %P gold?", setTopic(2)}},
	{{"sell", "battle", "axe"}, {setType(3266), setAmount(1), setPrice(75), "Do you want to sell a battle axe for %P gold?", setTopic(2)}},
	{{"sell", "battle", "hammer"}, {setType(3305), setAmount(1), setPrice(50), "Do you want to sell a battle hammer for %P gold?", setTopic(2)}},
	{{"sell", "morning", "star"}, {setType(3282), setAmount(1), setPrice(100), "Do you want to sell a morning star for %P gold?", setTopic(2)}},
	{{"sell", "two", "handed", "sword"}, {setType(3265), setAmount(1), setPrice(190), "Do you want to sell a two handed sword for %P gold?", setTopic(2)}},
	{{"sell", "halberd"}, {setType(3269), setAmount(1), setPrice(310), "Do you want to sell a halberd for %P gold?", setTopic(2)}},
	{{"sell", "double", "axe"}, {setType(3275), setAmount(1), setPrice(260), "Do you want to sell a double axe for %P gold?", setTopic(2)}},
	{{"sell", "war", "hammer"}, {setType(3279), setAmount(1), setPrice(470), "Do you want to sell a war hammer for %P gold?", setTopic(2)}},

	{{"sell", P1, lt(1, P1), "mace"}, {setType(3286), setAmount(P1), setPrice(mul(23, P1)), "Do you want to sell %A maces for %P gold?", setTopic(2)}},
	{{"sell", P1, lt(1, P1), "dagger"}, {setType(3267), setAmount(P1), setPrice(mul(1, P1)), "Do you want to sell %A daggers for %P gold?", setTopic(2)}},
	{{"sell", P1, lt(1, P1), "carlin", "sword"}, {setType(3283), setAmount(P1), setPrice(mul(118, P1)), "Do you want to sell %A carlin swords for %P gold?", setTopic(2)}},
	{{"sell", P1, lt(1, P1), "club"}, {setType(3270), setAmount(P1), setPrice(mul(1, P1)), "Do you want to sell %A clubs for %P gold?", setTopic(2)}},
	{{"sell", P1, lt(1, P1), "spear"}, {setType(3277), setAmount(P1), setPrice(mul(1, P1)), "Do you want to sell %A spears for %P gold", setTopic(2)}},
	{{"sell", P1, lt(1, P1), "rapier"}, {setType(3272), setAmount(P1), setPrice(mul(3, P1)), "Do you want to sell %A rapiers for %P gold?", setTopic(2)}},
	{{"sell", P1, lt(1, P1), "sabre"}, {setType(3273), setAmount(P1), setPrice(mul(5, P1)), "Do you want to sell %A sabres for %P gold?", setTopic(2)}},
	{{"sell", P1, lt(1, P1), "sword"}, {setType(3264), setAmount(P1), setPrice(mul(15, P1)), "Do you want to sell %A swords for %P gold?", setTopic(2)}},
	{{"sell", P1, lt(1, P1), "battle", "axe"}, {setType(3266), setAmount(P1), setPrice(mul(75, P1)), "Do you want to sell %A battle axes for %P gold?", setTopic(2)}},
	{{"sell", P1, lt(1, P1), "battle", "hammer"}, {setType(3305), setAmount(P1), setPrice(mul(50, P1)), "Do you want to sell %A battle hammers for %P gold?", setTopic(2)}},
	{{"sell", P1, lt(1, P1), "morning", "star"}, {setType(3282), setAmount(P1), setPrice(mul(100, P1)), "Do you want to sell %A morning stars for %P gold?", setTopic(2)}},
	{{"sell", P1, lt(1, P1), "two", "handed", "sword"}, {setType(3265), setAmount(P1), setPrice(mul(190, P1)), "Do you want to sell %A two handed swords for %P gold?", setTopic(2)}},
	{{"sell", P1, lt(1, P1), "halberd"}, {setType(3269), setAmount(P1), setPrice(mul(310, P1)), "Do you want to sell %A halberds for %P gold?", setTopic(2)}},
	{{"sell", P1, lt(1, P1), "double", "axe"}, {setType(3275), setAmount(P1), setPrice(mul(260, P1)), "Do you want to sell %A double axes for %P gold?", setTopic(2)}},
	{{"sell", P1, lt(1, P1), "war", "hammer"}, {setType(3279), setAmount(P1), setPrice(mul(470, P1)), "Do you want to sell %A war hammers for %P gold?", setTopic(2)}},

	{{eq(TOPIC, 1), "yes", ge(COUNTMONEY, PRICE)}, {"Thank you. Here it is.", deleteMoney, create(TYPE)}},
	{{eq(TOPIC, 1), "yes"}, {"Sorry, you do not have enough gold."}},
	{{eq(TOPIC, 1)}, {"Maybe you will buy it another time."}},

	{{eq(TOPIC, 2), "yes", ge(count(TYPE), AMOUNT)}, {"Fine. Here is your money.", delete(TYPE), createMoney}},
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
