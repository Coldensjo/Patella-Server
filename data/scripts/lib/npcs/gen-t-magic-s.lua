-- lib/npcs/gen-t-magic-s.lua: the rule block gen-t-magic-s.ndb, converted by patella-convert.
-- An NPC that included it (@"gen-t-magic-s.ndb") has it in its rules where the include
-- was; lib/npc.lua matches and runs them (scripting.hh, "NPCs"). The comments
-- are those of gen-t-magic-s.ndb.
--
-- GIMUD - Graphical Interface Multi User Dungeon
-- gen-t-gems-s.ndb: Datenbank für generischen Edelsteinverkauf

local behaviour = require("lib.npc")
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
local previous = behaviour.previous

return behaviour.rules {
	{{"offer"}, {"I'm selling life and mana fluids, runes, and spellbooks."}},
	{{"goods"}, {previous}},
	{{"do", "you", "sell"}, {previous}},
	{{"do", "you", "have"}, {previous}},

	{{"life", "fluid"}, {setType(2874), setData(11), setAmount(1), setPrice(60), "Do you want to buy life fluid for %P gold?", setTopic(30)}},
	{{"mana", "fluid"}, {setType(2874), setData(10), setAmount(1), setPrice(100), "Do you want to buy mana fluid for %P gold?", setTopic(30)}},
	{{"rune"}, {setType(3147), setAmount(1), setPrice(10), "Do you want to buy a blank rune for %P gold?", setTopic(31)}},
	{{"spellbook"}, {setType(3059), setAmount(1), setPrice(150), "Do you want to buy a spellbook for %P gold?", setTopic(31)}},
	{{P1, lt(0, P1), "life", "fluid"}, {setType(2874), setData(11), setAmount(P1), setPrice(mul(60, P1)), "Do you want to buy %A potions of life fluid for %P gold?", setTopic(30)}},
	{{P1, lt(0, P1), "mana", "fluid"}, {setType(2874), setData(10), setAmount(P1), setPrice(mul(100, P1)), "Do you want to buy %A potions of mana fluid for %P gold?", setTopic(30)}},
	{{P1, lt(0, P1), "runes"}, {setType(3147), setAmount(P1), setPrice(mul(10, P1)), "Do you want to buy %A blank runes for %P gold?", setTopic(31)}},

	{{"deposit"}, {"I will pay you 5 gold for every empty vial. Ok?", setData(0), setTopic(32)}},
	{{"vial"}, {previous}},
	{{"flask"}, {previous}},

	{{eq(TOPIC, 31), "yes", ge(COUNTMONEY, PRICE)}, {"Here you are.", deleteMoney, create(TYPE)}},
	{{eq(TOPIC, 31), "yes"}, {"Come back, when you have enough money."}},
	{{eq(TOPIC, 31)}, {"Hmm, but next time."}},

	{{eq(TOPIC, 30), "yes", ge(COUNTMONEY, PRICE)}, {"Here you are. There is a deposit of 5 gold on the vial.", deleteMoney, create(TYPE)}},
	{{eq(TOPIC, 30), "yes"}, {"Come back, when you have enough money."}},
	{{eq(TOPIC, 30)}, {"Hmm, but next time."}},

	{{eq(TOPIC, 32), "yes", gt(count(2874), 0)}, {setAmount(count(2874)), setPrice(mul(AMOUNT, 5)), "Here you are ... %P gold.", delete(2874), createMoney}},
	{{eq(TOPIC, 32), "yes"}, {"You don't have any empty vials."}},
	{{eq(TOPIC, 32)}, {"Hmm, but please keep Tibia litter free."}},
}
