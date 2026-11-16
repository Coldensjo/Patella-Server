-- lib/npcs/gen-t-fruit-s.lua: the rule block gen-t-fruit-s.ndb, converted by patella-convert.
-- An NPC that included it (@"gen-t-fruit-s.ndb") has it in its rules where the include
-- was; lib/npc.lua matches and runs them (scripting.hh, "NPCs"). The comments
-- are those of gen-t-fruit-s.ndb.
--
-- GIMUD - Graphical Interface Multi User Dungeon
-- genTfruitV.ndb:  Datenbank für generischen Früchteverkauf

local behaviour = require("lib.npc")
local P1 = behaviour.P1
local eq = behaviour.eq
local lt = behaviour.lt
local ge = behaviour.ge
local TOPIC = behaviour.TOPIC
local PRICE = behaviour.PRICE
local TYPE = behaviour.TYPE
local COUNTMONEY = behaviour.COUNTMONEY
local mul = behaviour.mul
local setTopic = behaviour.setTopic
local setPrice = behaviour.setPrice
local setAmount = behaviour.setAmount
local setType = behaviour.setType
local create = behaviour.create
local deleteMoney = behaviour.deleteMoney

return behaviour.rules {
	{{"corncob"}, {setType(3597), setAmount(1), setPrice(3), "Do you want to buy a corncob for %P gold?", setTopic(24)}},
	{{"cherry"}, {setType(3590), setAmount(1), setPrice(1), "Do you want to buy a cherry for %P gold?", setTopic(24)}},
	{{"grapes"}, {setType(3592), setAmount(1), setPrice(3), "Do you want to buy grapes for %P gold?", setTopic(24)}},
	{{"melon"}, {setType(3593), setAmount(1), setPrice(8), "Do you want to buy a melon for %P gold?", setTopic(24)}},
	{{"banana"}, {setType(3587), setAmount(1), setPrice(2), "Do you want to buy a banana for %P gold?", setTopic(24)}},
	{{"strawberry"}, {setType(3591), setAmount(1), setPrice(1), "Do you want to buy a strawberry for %P gold?", setTopic(24)}},
	{{"carrot"}, {setType(3595), setAmount(1), setPrice(3), "Do you want to buy a carrot for %P gold?", setTopic(24)}},
	{{"apple"}, {setType(3585), setAmount(1), setPrice(3), "Do you want to buy an apple for %P gold?", setTopic(24)}},
	{{"pear"}, {setType(3584), setAmount(1), setPrice(4), "Do you want to buy a pear for %P gold?", setTopic(24)}},
	{{"blueberry"}, {setType(3588), setAmount(1), setPrice(1), "Do you want to buy a blueberry for %P gold?", setTopic(24)}},

	{{"white", "mushroom"}, {setType(3723), setAmount(1), setPrice(10), "Do you want to buy one of the white mushrooms for %P gold?", setTopic(24)}},

	{{P1, lt(1, P1), "corncob"}, {setType(3597), setAmount(P1), setPrice(mul(3, P1)), "Do you want to buy %A corncobs for %P gold?", setTopic(24)}},
	{{P1, lt(1, P1), "cherry"}, {setType(3590), setAmount(P1), setPrice(mul(1, P1)), "Do you want to buy %A cherrys for %P gold?", setTopic(24)}},
	{{P1, lt(1, P1), "grapes"}, {setType(3592), setAmount(P1), setPrice(mul(3, P1)), "Do you want to buy %A grapes for %P gold?", setTopic(24)}},
	{{P1, lt(1, P1), "melon"}, {setType(3593), setAmount(P1), setPrice(mul(8, P1)), "Do you want to buy %A melons for %P gold?", setTopic(24)}},
	{{P1, lt(1, P1), "banana"}, {setType(3587), setAmount(P1), setPrice(mul(2, P1)), "Do you want to buy %A bananas for %P gold?", setTopic(24)}},
	{{P1, lt(1, P1), "strawberries"}, {setType(3591), setAmount(P1), setPrice(mul(1, P1)), "Do you want to buy %A strawberries for %P gold?", setTopic(24)}},
	{{P1, lt(1, P1), "carrot"}, {setType(3595), setAmount(P1), setPrice(mul(3, P1)), "Do you want to buy %A carrots for %P gold?", setTopic(24)}},
	{{P1, lt(1, P1), "apple"}, {setType(3585), setAmount(P1), setPrice(mul(3, P1)), "Do you want to buy %A apples for %P gold?", setTopic(24)}},
	{{P1, lt(1, P1), "pear"}, {setType(3584), setAmount(P1), setPrice(mul(4, P1)), "Do you want to buy %A pears for %P gold?", setTopic(24)}},
	{{P1, lt(1, P1), "blueberries"}, {setType(3588), setAmount(P1), setPrice(mul(1, P1)), "Do you want to buy %A blueberries for %P gold?", setTopic(24)}},

	{{P1, lt(1, P1), "white", "mushroom"}, {setType(3723), setAmount(P1), setPrice(mul(10, P1)), "Do you want to buy %A of the white mushrooms for %P gold?", setTopic(24)}},

	{{eq(TOPIC, 24), "yes", ge(COUNTMONEY, PRICE)}, {"Thank you. Here it is.", deleteMoney, create(TYPE)}},
	{{eq(TOPIC, 24), "yes"}, {"Sorry, you do not have enough gold."}},
	{{eq(TOPIC, 24)}, {"Maybe you will buy it another time."}},
}
