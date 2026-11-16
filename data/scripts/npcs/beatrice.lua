-- npcs/beatrice.lua: the behaviour of the NPC Beatrice, converted from
-- beatrice.npc by patella-convert. The NPC is its [[npc]] entry in npcs.toml;
-- lib/npc.lua matches and runs the rules below, in the order of the file
-- (scripting.hh, "NPCs"). The comments are those of beatrice.npc.
--
-- GIMUD - Graphical Interface Multi User Dungeon
-- beatrice.npc: Datenbank fuer die Ausruestungshaendlerin Beatrice

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
	{{ADDRESS, "hello$", FORCE}, {"Hello, hiho, and ashari %N."}},
	{{ADDRESS, "hi$", FORCE}, {previous}},
	{{ADDRESS, "ashari$", FORCE}, {previous}},
	{{ADDRESS, FORCE}, {idle}},
	{{BUSY, "hello$", FORCE}, {"You're next, %N.", queue}},
	{{BUSY, "hi$", FORCE}, {previous}},
	{{BUSY, "ashari$", FORCE}, {previous}},
	{{BUSY, FORCE}, {nop}},
	{{VANISH, FORCE}, {"See you later."}},

	{{"bye"}, {"See you later.", idle}},
	{{"name"}, {"I am called Beatrice."}},
	{{"job"}, {"My job is to sell all kind of useful equipment."}},
	{{"time"}, {"It's %T right now."}},
	{{"king"}, {"I have seen him once. What a handsome man he is."}},
	{{"tibianus"}, {previous}},
	{{"army"}, {"I supply them with some basic stuff."}},
	{{"ferumbras"}, {"I vaguely remember that name."}},
	{{"excalibug"}, {"A myth like the screwdriver of Kurik or the endless vial of manafluid."}},
	{{"thais"}, {"We are no longer in need to be supplied from there."}},
	{{"tibia"}, {"I don't like travelling much. I prefer to live in the safety of our city."}},
	{{"carlin"}, {"Though they rebelled against our king it's said that the city is very lovely."}},
	{{"edron"}, {"It's the best place to live at."}},
	{{"news"}, {"There are always rumors about the dangers in the far north of Edron."}},
	{{"rumors"}, {previous}},

	{{"offer"}, {"My inventory is large, just have a look at the blackboard."}},
	{{"goods"}, {previous}},
	{{"do", "you", "sell"}, {previous}},
	{{"do", "you", "have"}, {previous}},
	{{"equipment"}, {previous}},

	{{"torch"}, {setType(2920), setAmount(1), setPrice(2), "Do you want to buy a torch for %P gold?", setTopic(1)}},
	{{"bag"}, {setType(2861), setAmount(1), setPrice(5), "Do you want to buy a bag for %P gold?", setTopic(1)}},
	{{"backpack"}, {setType(2869), setAmount(1), setPrice(20), "Do you want to buy a backpack for %P gold?", setTopic(1)}},
	{{"bucket"}, {setType(2873), setAmount(1), setPrice(4), "Do you want to buy a bucket for %P gold?", setTopic(1)}},
	{{"watch"}, {setType(2906), setAmount(1), setPrice(20), "Do you want to buy a watch for %P gold?", setTopic(1)}},
	{{"rope"}, {setType(3003), setAmount(1), setPrice(50), "Do you want to buy a rope for %P gold?", setTopic(1)}},
	{{"scythe"}, {setType(3453), setAmount(1), setPrice(50), "Do you want to buy a scythe for %P gold?", setTopic(1)}},
	{{"pick"}, {setType(3456), setAmount(1), setPrice(50), "Do you want to buy a pick for %P gold?", setTopic(1)}},
	{{"shovel"}, {setType(3457), setAmount(1), setPrice(50), "Do you want to buy a shovel for %P gold?", setTopic(1)}},
	{{"rod"}, {setType(3483), setAmount(1), setPrice(150), "Do you want to buy a fishing rod for %P gold?", setTopic(1)}},
	{{"crowbar"}, {setType(3304), setAmount(1), setPrice(260), "Do you want to buy a crowbar for %P gold?", setTopic(1)}},
	{{"lamp"}, {setType(2914), setAmount(1), setPrice(8), "Do you want to buy a lamp for %P gold?", setTopic(1)}},
	{{"candlestick"}, {setType(2917), setAmount(1), setPrice(2), "Do you want to buy a candlestick for %P gold?", setTopic(1)}},
	{{"basket"}, {setType(2855), setAmount(1), setPrice(6), "Do you want to buy a basket for %P gold?", setTopic(1)}},
	{{"trap"}, {setType(3481), setAmount(1), setPrice(280), "Do you want to buy a trap for %P gold?", setTopic(1)}},
	{{"football"}, {setType(2990), setAmount(1), setPrice(111), "Do you want to buy a football for %P gold?", setTopic(1)}},

	{{P1, lt(1, P1), "torch"}, {setType(2920), setAmount(P1), setPrice(mul(2, P1)), "Do you want to buy %A torches for %P gold?", setTopic(1)}},
	{{P1, lt(1, P1), "bag"}, {setType(2861), setAmount(P1), setPrice(mul(5, P1)), "Do you want to buy %A bags for %P gold?", setTopic(1)}},
	{{P1, lt(1, P1), "backpack"}, {setType(2869), setAmount(P1), setPrice(mul(20, P1)), "Do you want to buy %A backpacks for %P gold?", setTopic(1)}},
	{{P1, lt(1, P1), "bucket"}, {setType(2873), setAmount(P1), setPrice(mul(4, P1)), "Do you want to buy %A buckets for %P gold?", setTopic(1)}},
	{{P1, lt(1, P1), "watch"}, {setType(2906), setAmount(P1), setPrice(mul(20, P1)), "Do you want to buy %A watches for %P gold?", setTopic(1)}},
	{{P1, lt(1, P1), "rope"}, {setType(3003), setAmount(P1), setPrice(mul(50, P1)), "Do you want to buy %A ropes for %P gold?", setTopic(1)}},
	{{P1, lt(1, P1), "scythe"}, {setType(3453), setAmount(P1), setPrice(mul(50, P1)), "Do you want to buy %A scythes for %P gold?", setTopic(1)}},
	{{P1, lt(1, P1), "pick"}, {setType(3456), setAmount(P1), setPrice(mul(50, P1)), "Do you want to buy %A picks for %P gold?", setTopic(1)}},
	{{P1, lt(1, P1), "shovel"}, {setType(3457), setAmount(P1), setPrice(mul(50, P1)), "Do you want to buy %A shovels for %P gold?", setTopic(1)}},
	{{P1, lt(1, P1), "rod"}, {setType(3483), setAmount(P1), setPrice(mul(150, P1)), "Do you want to buy %A fishing rods for %P gold?", setTopic(1)}},
	{{P1, lt(1, P1), "crowbar"}, {setType(3304), setAmount(P1), setPrice(mul(260, P1)), "Do you want to buy %A crowbars for %P gold?", setTopic(1)}},
	{{P1, lt(1, P1), "lamp"}, {setType(2914), setAmount(P1), setPrice(mul(8, P1)), "Do you want to buy %A lamps for %P gold?", setTopic(1)}},
	{{P1, lt(1, P1), "candlestick"}, {setType(2917), setAmount(P1), setPrice(mul(2, P1)), "Do you want to buy %A candlesticks for %P gold?", setTopic(1)}},
	{{P1, lt(1, P1), "basket"}, {setType(2855), setAmount(P1), setPrice(mul(6, P1)), "Do you want to buy %A baskets for %P gold?", setTopic(1)}},
	{{P1, lt(1, P1), "trap"}, {setType(3481), setAmount(P1), setPrice(mul(280, P1)), "Do you want to buy %A traps for %P gold?", setTopic(1)}},
	{{P1, lt(1, P1), "football"}, {setType(2990), setAmount(P1), setPrice(mul(111, P1)), "Do you want to buy %A footballs for %P gold?", setTopic(1)}},

	{{"sell", "watch"}, {setType(2906), setAmount(1), setPrice(6), "Do you want to sell a watch for %P gold?", setTopic(2)}},
	{{"sell", "rope"}, {setType(3003), setAmount(1), setPrice(15), "Do you want to sell a rope for %P gold?", setTopic(2)}},
	{{"sell", "scythe"}, {setType(3453), setAmount(1), setPrice(12), "Do you want to sell a scythe for %P gold?", setTopic(2)}},
	{{"sell", "pick"}, {setType(3456), setAmount(1), setPrice(15), "Do you want to sell a pick for %P gold?", setTopic(2)}},
	{{"sell", "shovel"}, {setType(3457), setAmount(1), setPrice(8), "Do you want to sell a shovel for %P gold?", setTopic(2)}},
	{{"sell", "mirror"}, {setType(3463), setAmount(1), setPrice(10), "Do you want to sell a mirror for %P gold?", setTopic(2)}},
	{{"sell", "rod"}, {setType(3483), setAmount(1), setPrice(40), "Do you want to sell a fishing rod for %P gold?", setTopic(2)}},
	{{"sell", "inkwell"}, {setType(3509), setAmount(1), setPrice(8), "Do you want to sell an inkwell for %P gold?", setTopic(2)}},
	{{"sell", "sickle"}, {setType(3293), setAmount(1), setPrice(3), "Do you want to sell a sickle for %P gold?", setTopic(2)}},
	{{"sell", "crowbar"}, {setType(3304), setAmount(1), setPrice(50), "Do you want to sell a crowbar for %P gold?", setTopic(2)}},
	{{"sell", "trap"}, {setType(3481), setAmount(1), setPrice(75), "Do you want to sell a trap for %P gold?", setTopic(2)}},

	{{"sell", P1, lt(1, P1), "watch"}, {setType(2906), setAmount(P1), setPrice(mul(6, P1)), "Do you want to sell %A watches for %P gold?", setTopic(2)}},
	{{"sell", P1, lt(1, P1), "rope"}, {setType(3003), setAmount(P1), setPrice(mul(15, P1)), "Do you want to sell %A ropes for %P gold?", setTopic(2)}},
	{{"sell", P1, lt(1, P1), "scythe"}, {setType(3453), setAmount(P1), setPrice(mul(12, P1)), "Do you want to sell %A scythes for %P gold?", setTopic(2)}},
	{{"sell", P1, lt(1, P1), "pick"}, {setType(3456), setAmount(P1), setPrice(mul(15, P1)), "Do you want to sell %A picks for %P gold?", setTopic(2)}},
	{{"sell", P1, lt(1, P1), "shovel"}, {setType(3457), setAmount(P1), setPrice(mul(8, P1)), "Do you want to sell %A shovels for %P gold?", setTopic(2)}},
	{{"sell", P1, lt(1, P1), "mirror"}, {setType(3463), setAmount(P1), setPrice(mul(10, P1)), "Do you want to sell %A mirrors for %P gold?", setTopic(2)}},
	{{"sell", P1, lt(1, P1), "rod"}, {setType(3483), setAmount(P1), setPrice(mul(40, P1)), "Do you want to sell %A fishing rods for %P gold?", setTopic(2)}},
	{{"sell", P1, lt(1, P1), "inkwell"}, {setType(3509), setAmount(P1), setPrice(mul(8, P1)), "Do you want to sell %A inkwells for %P gold?", setTopic(2)}},
	{{"sell", P1, lt(1, P1), "sickle"}, {setType(3293), setAmount(P1), setPrice(mul(3, P1)), "Do you want to sell %A sickles for %P gold?", setTopic(2)}},
	{{"sell", P1, lt(1, P1), "crowbar"}, {setType(3304), setAmount(P1), setPrice(mul(50, P1)), "Do you want to sell %A crowbars for %P gold?", setTopic(2)}},
	{{"sell", P1, lt(1, P1), "trap"}, {setType(3481), setAmount(P1), setPrice(mul(75, P1)), "Do you want to sell %A traps for %P gold?", setTopic(2)}},

	{{"worm"}, {"I sell worms only in sixpacks for 5 gold each, how many sixpacks of worms do you want to buy?"}},
	{{"worms"}, {previous}},
	{{P1, "worms"}, {setType(3492), setAmount(mul(P1, 6)), setPrice(mul(5, P1)), "So you want to buy some of my sixpacks with altogether %A worms for %P gold?", setTopic(1)}},
	{{P1, "six", "pack"}, {previous}},

	{{eq(TOPIC, 1), "yes", ge(COUNTMONEY, PRICE)}, {"Here you are.", deleteMoney, create(TYPE)}},
	{{eq(TOPIC, 1), "yes"}, {"Come back, when you have enough money."}},
	{{eq(TOPIC, 1)}, {"Hmm, but next time."}},

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
