-- lib/npcs/gen-t-armor-b.lua: the rule block gen-t-armor-b.ndb, converted by patella-convert.
-- An NPC that included it (@"gen-t-armor-b.ndb") has it in its rules where the include
-- was; lib/npc.lua matches and runs them (scripting.hh, "NPCs"). The comments
-- are those of gen-t-armor-b.ndb.
--
-- GIMUD - Graphical Interface Multi User Dungeon
-- gen-t-armor-b.ndb: Datenbank für generischen Rüstungskauf

local behaviour = require("lib.npc")
local P1 = behaviour.P1
local eq = behaviour.eq
local lt = behaviour.lt
local gt = behaviour.gt
local ge = behaviour.ge
local TOPIC = behaviour.TOPIC
local AMOUNT = behaviour.AMOUNT
local TYPE = behaviour.TYPE
local count = behaviour.count
local mul = behaviour.mul
local setTopic = behaviour.setTopic
local setPrice = behaviour.setPrice
local setAmount = behaviour.setAmount
local setType = behaviour.setType
local delete = behaviour.delete
local createMoney = behaviour.createMoney

return behaviour.rules {
	{{"sell", "coat"}, {setType(3562), setAmount(1), setPrice(1), "Do you want to sell this coat for %P gold?", setTopic(51)}},
	{{"sell", "jacket"}, {setType(3561), setAmount(1), setPrice(1), "Do you want to sell this jacket for %P gold?", setTopic(51)}},
	{{"sell", "leather", "armor"}, {setType(3361), setAmount(1), setPrice(12), "Do you want to sell a leather armor for %P gold?", setTopic(51)}},
	{{"sell", "chain", "armor"}, {setType(3358), setAmount(1), setPrice(70), "Do you want to sell a chain armor for %P gold?", setTopic(51)}},
	{{"sell", "brass", "armor"}, {setType(3359), setAmount(1), setPrice(150), "Do you want to sell a brass armor for %P gold?", setTopic(51)}},
	{{"sell", "plate", "armor"}, {setType(3357), setAmount(1), setPrice(400), "Do you want to sell a plate armor for %P gold?", setTopic(51)}},
	{{"sell", "knight", "armor"}, {setType(3370), setAmount(1), setPrice(875), "Do you want to sell this knights armor for %P gold?", setTopic(51)}},
	{{"sell", "golden", "armor"}, {setType(3360), setAmount(1), setPrice(1500), "Do you want to sell this golden armor for %P gold?", setTopic(51)}},

	{{"sell", P1, lt(1, P1), "coat"}, {setType(3562), setAmount(P1), setPrice(mul(1, P1)), "Do you want to sell this coats for %P gold?", setTopic(51)}},
	{{"sell", P1, lt(1, P1), "jacket"}, {setType(3561), setAmount(P1), setPrice(mul(1, P1)), "Do you want to sell this jackets for %P gold?", setTopic(51)}},
	{{"sell", P1, lt(1, P1), "leather", "armor"}, {setType(3361), setAmount(P1), setPrice(mul(12, P1)), "Do you want to sell a leather armors for %P gold?", setTopic(51)}},
	{{"sell", P1, lt(1, P1), "chain", "armor"}, {setType(3358), setAmount(P1), setPrice(mul(70, P1)), "Do you want to sell a chain armors for %P gold?", setTopic(51)}},
	{{"sell", P1, lt(1, P1), "brass", "armor"}, {setType(3359), setAmount(P1), setPrice(mul(150, P1)), "Do you want to sell a brass armors for %P gold?", setTopic(51)}},
	{{"sell", P1, lt(1, P1), "plate", "armor"}, {setType(3357), setAmount(P1), setPrice(mul(400, P1)), "Do you want to sell a plate armors for %P gold?", setTopic(51)}},
	{{"sell", P1, lt(1, P1), "knight", "armor"}, {setType(3370), setAmount(P1), setPrice(mul(875, P1)), "Do you want to sell this knights armors for %P gold?", setTopic(51)}},
	{{"sell", P1, lt(1, P1), "golden", "armor"}, {setType(3360), setAmount(P1), setPrice(mul(1500, P1)), "Do you want to sell this golden armors for %P gold?", setTopic(51)}},

	{{eq(TOPIC, 51), "yes", ge(count(TYPE), AMOUNT)}, {"Ok. Here is your money.", delete(TYPE), createMoney}},
	{{eq(TOPIC, 51), "yes"}, {"Sorry, you do not have one."}},
	{{eq(TOPIC, 51), "yes", gt(AMOUNT, 1)}, {"Sorry, you do not have so much."}},
	{{eq(TOPIC, 51)}, {"Maybe next time."}},
}
