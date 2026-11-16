-- npcs/sandra.lua: the behaviour of the NPC Sandra, converted from
-- sandra.npc by patella-convert. The NPC is its [[npc]] entry in npcs.toml;
-- lib/npc.lua matches and runs the rules below, in the order of the file
-- (scripting.hh, "NPCs"). The comments are those of sandra.npc.
--
-- GIMUD - Graphical Interface Multi User Dungeon
-- sandra.npc: Datenbank für die Trankhändlerin Sandra

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
	{{ADDRESS, "hello$", FORCE}, {"Hello %N."}},
	{{ADDRESS, "hi$", FORCE}, {previous}},
	{{ADDRESS, FORCE}, {idle}},
	{{BUSY, "hello$", FORCE}, {"Sorry %N, I am busy right now. Please wait a moment.", queue}},
	{{BUSY, "hi$", FORCE}, {previous}},
	{{BUSY, FORCE}, {nop}},
	{{VANISH, FORCE}, {"Good bye and please come back soon."}},

	{{"bye"}, {"Good bye and please come back soon.", idle}},
	{{"name"}, {"I am lady Sandra Astralian."}},
	{{"job"}, {"I sell potions and magic fluids."}},
	{{"time"}, {"Buy a watch."}},
	{{"king"}, {"I was guest at his castle on my visits to Thais."}},
	{{"tibianus"}, {previous}},
	{{"army"}, {"Do I look as if I'd fraternize with such people?"}},
	{{"ferumbras"}, {"A disgusting person indeed."}},
	{{"excalibug"}, {"I am not interested in tales only kids belive in."}},
	{{"thais"}, {"A city full of disgusting people with ill manners."}},
	{{"tibia"}, {"The world is a place of barbarianism."}},
	{{"carlin"}, {"I plan to visit this city one day."}},
	{{"edron"}, {"Isn't it a wonderful town?"}},
	{{"news"}, {"Nothing I would talk to you about."}},
	{{"rumors"}, {previous}},

	{{"offer"}, {"I'm selling life and mana fluids and several potions."}},
	{{"goods"}, {previous}},
	{{"do", "you", "sell"}, {previous}},
	{{"do", "you", "have"}, {previous}},
	{{"fluid"}, {previous}},
	{{"potion"}, {"I'm selling potions of slime, blood, urine, oil, and distilled water."}},

	{{"life", "fluid"}, {setType(2874), setData(11), setAmount(1), setPrice(60), "Do you want to buy life fluid for %P gold?", setTopic(2)}},
	{{"mana", "fluid"}, {setType(2874), setData(10), setAmount(1), setPrice(55), "Do you want to buy mana fluid for %P gold?", setTopic(2)}},
	{{P1, lt(1, P1), "life", "fluid"}, {setType(2874), setData(11), setAmount(P1), setPrice(mul(60, P1)), "Do you want to buy %A potions of life fluid for %P gold?", setTopic(2)}},
	{{P1, lt(1, P1), "mana", "fluid"}, {setType(2874), setData(10), setAmount(P1), setPrice(mul(55, P1)), "Do you want to buy %A potions of mana fluid for %P gold?", setTopic(2)}},
	{{"slime"}, {setType(2874), setData(6), setAmount(1), setPrice(12), "Do you want to buy a potion of slime for %P gold?", setTopic(2)}},
	{{"blood"}, {setType(2874), setData(5), setAmount(1), setPrice(15), "Do you want to buy a potion of blood for %P gold?", setTopic(2)}},
	{{"urine"}, {setType(2874), setData(8), setAmount(1), setPrice(10), "Do you want to buy a potion of urine for %P gold?", setTopic(2)}},
	{{"oil"}, {setType(2874), setData(7), setAmount(1), setPrice(20), "Do you want to buy a potion of oil for %P gold?", setTopic(2)}},
	{{"water"}, {setType(2874), setData(1), setAmount(1), setPrice(8), "Do you want to buy a potion of distilled water for %P gold?", setTopic(2)}},

	{{"deposit"}, {"I will pay you 5 gold for every empty vial. Ok?", setData(0), setTopic(3)}},
	{{"vial"}, {previous}},
	{{"flask"}, {previous}},

	{{eq(TOPIC, 2), "yes", ge(COUNTMONEY, PRICE)}, {"Here you are. There is a deposit of 5 gold on the vial.", deleteMoney, create(TYPE)}},
	{{eq(TOPIC, 2), "yes"}, {"Come back, when you have enough money."}},
	{{eq(TOPIC, 2)}, {"Hmm, but next time."}},

	{{eq(TOPIC, 3), "yes", gt(count(2874), 0)}, {setAmount(count(2874)), setPrice(mul(AMOUNT, 5)), "Here you are ... %P gold.", delete(2874), createMoney}},
	{{eq(TOPIC, 3), "yes"}, {"You don't have any empty vials."}},
	{{eq(TOPIC, 3)}, {"Hmm, but please keep Tibia litter free."}},
}

function onCreatureSay(npc, creature, text, situation)
	behaviour.react(npc, rules, text, situation)
end

function onCreatureDisappear(npc, creature)
	behaviour.react(npc, rules, "", NPC_SITUATION_VANISH)
end
