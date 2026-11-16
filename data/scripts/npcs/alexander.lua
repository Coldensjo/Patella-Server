-- npcs/alexander.lua: the behaviour of the NPC Alexander, converted from
-- alexander.npc by patella-convert. The NPC is its [[npc]] entry in npcs.toml;
-- lib/npc.lua matches and runs the rules below, in the order of the file
-- (scripting.hh, "NPCs"). The comments are those of alexander.npc.
--
-- GIMUD - Graphical Interface Multi User Dungeon
-- alexander.npc: Datenbank fuer den Magiehaendler Alexander

local behaviour = require("lib.npc")
local genTRunesPremS = require("lib.npcs.gen-t-runes-prem-s")
local genTWandsPremS = require("lib.npcs.gen-t-wands-prem-s")
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
	{{ADDRESS, "hello$", FORCE}, {"Hi there %N."}},
	{{ADDRESS, "hi$", FORCE}, {previous}},
	{{ADDRESS, FORCE}, {idle}},
	{{BUSY, "hello$", FORCE}, {"Just wait, %N.", queue}},
	{{BUSY, "hi$", FORCE}, {previous}},
	{{BUSY, FORCE}, {nop}},
	{{VANISH, FORCE}, {"See you."}},

	{{"bye"}, {"See you.", idle}},
	{{"name"}, {"I am Alexander."}},
	{{"job"}, {"I trade with runes and other magic items."}},
	{{"time"}, {"It's %T right now."}},
	{{"king"}, {"The king has not much interest in magic items as far as I know."}},
	{{"tibianus"}, {previous}},
	{{"army"}, {"The army uses weapons and armor rather then items of magic."}},
	{{"ferumbras"}, {"A hero has to be well prepared to face this threat."}},
	{{"excalibug"}, {"Ah, I would trade a fortune for this fabulous item."}},
	{{"thais"}, {"I am glad the king founded this academy far away from the mundane troubles of Thais"}},
	{{"tibia"}, {"The world is filled with wonderous places and items."}},
	{{"carlin"}, {"I heard it's a city of druids."}},
	{{"edron"}, {"In our town, science and arts are thriving."}},
	{{"news"}, {"Ask for news and rumors in the tavern."}},
	{{"rumors"}, {previous}},

	{{"offer"}, {"I'm selling runes, life rings, wands, rods and crystal balls."}},
	{{"goods"}, {previous}},
	{{"do", "you", "sell"}, {previous}},
	{{"do", "you", "have"}, {previous}},

	{{"rune"}, {"I sell blank runes and spell runes."}},

	{{"blank", "rune"}, {setType(3147), setAmount(1), setPrice(10), "Do you want to buy a blank rune for %P gold?", setTopic(1)}},
	{{P1, lt(1, P1), "blank", "rune"}, {setType(3147), setAmount(P1), setPrice(mul(10, P1)), "Do you want to buy %A blank runes for %P gold?", setTopic(1)}},
	{{"life", "ring"}, {setType(3052), setAmount(1), setPrice(900), "Do you want to buy a life ring for %P gold?", setTopic(1)}},
	{{"crystal", "ball"}, {setType(3076), setAmount(1), setPrice(530), "Do you want to buy a crystal ball for %P gold?", setTopic(1)}},

	{{P1, lt(1, P1), "life", "ring"}, {setType(3052), setAmount(P1), setPrice(mul(900, P1)), "Do you want to buy %A life rings for %P gold?", setTopic(1)}},
	{{P1, lt(1, P1), "crystal", "ball"}, {setType(3076), setAmount(P1), setPrice(mul(530, P1)), "Do you want to buy %A crystal balls for %P gold?", setTopic(1)}},

	{{"sell", "life", "crystal"}, {setType(3061), setAmount(1), setPrice(85), "Do you want to sell a life crystal for %P gold?", setTopic(2)}},
	{{"sell", "mind", "stone"}, {setType(3062), setAmount(1), setPrice(170), "Do you want to sell a mind stone for %P gold?", setTopic(2)}},
	{{"sell", "crystal", "ball"}, {setType(3076), setAmount(1), setPrice(190), "Do you want to sell a crystal ball for %P gold?", setTopic(2)}},

	{{"sell", P1, lt(1, P1), "life", "crystal"}, {setType(3061), setAmount(P1), setPrice(mul(85, P1)), "Do you want to sell %A life crystals for %P gold?", setTopic(2)}},
	{{"sell", P1, lt(1, P1), "mind", "stone"}, {setType(3062), setAmount(P1), setPrice(mul(170, P1)), "Do you want to sell %A mind stones for %P gold?", setTopic(2)}},
	{{"sell", P1, lt(1, P1), "crystal", "ball"}, {setType(3076), setAmount(P1), setPrice(mul(190, P1)), "Do you want to sell %A crystal balls for %P gold?", setTopic(2)}},

	{{eq(TOPIC, 1), "yes", ge(COUNTMONEY, PRICE)}, {"Here you are.", deleteMoney, create(TYPE)}},
	{{eq(TOPIC, 1), "yes"}, {"Come back, when you have enough money."}},
	{{eq(TOPIC, 1)}, {"Hmm, but next time."}},

	{{eq(TOPIC, 2), "yes", ge(count(TYPE), AMOUNT)}, {"Ok. Here is your money.", delete(TYPE), createMoney}},
	{{eq(TOPIC, 2), "yes"}, {"Sorry, you do not have one."}},
	{{eq(TOPIC, 2), "yes", gt(AMOUNT, 1)}, {"Sorry, you do not have so many."}},
	{{eq(TOPIC, 2)}, {"Maybe next time."}},

	genTRunesPremS,
	genTWandsPremS,
}

function onCreatureSay(npc, creature, text, situation)
	behaviour.react(npc, rules, text, situation)
end

function onCreatureDisappear(npc, creature)
	behaviour.react(npc, rules, "", NPC_SITUATION_VANISH)
end
