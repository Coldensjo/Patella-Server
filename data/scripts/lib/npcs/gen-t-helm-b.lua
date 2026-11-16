-- lib/npcs/gen-t-helm-b.lua: the rule block gen-t-helm-b.ndb, converted by patella-convert.
-- An NPC that included it (@"gen-t-helm-b.ndb") has it in its rules where the include
-- was; lib/npc.lua matches and runs them (scripting.hh, "NPCs"). The comments
-- are those of gen-t-helm-b.ndb.
--
-- GIMUD - Graphical Interface Multi User Dungeon
-- gen-t-helm-b.ndb:  Datenbank für generischen Helmeinkauf

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
	{{"sell", "leather", "helmet"}, {setType(3355), setAmount(1), setPrice(4), "Do you want to sell a leather helmet for %P gold?", setTopic(50)}},
	{{"sell", "chain", "helmet"}, {setType(3352), setAmount(1), setPrice(17), "Do you want to sell a chain helmet for %P gold?", setTopic(50)}},
	{{"sell", "steel", "helmet"}, {setType(3351), setAmount(1), setPrice(190), "Do you want to sell a steel helmet for %P gold?", setTopic(50)}},
	{{"sell", "brass", "helmet"}, {setType(3354), setAmount(1), setPrice(30), "Do you want to sell a brass helmet for %P gold?", setTopic(50)}},
	{{"sell", "viking", "helmet"}, {setType(3367), setAmount(1), setPrice(66), "Do you want to sell a viking helmet for %P gold?", setTopic(50)}},
	{{"sell", "iron", "helmet"}, {setType(3353), setAmount(1), setPrice(145), "You want sell a iron helmet for %P gold?", setTopic(50)}},
	{{"sell", "devil", "helmet"}, {setType(3356), setAmount(1), setPrice(450), "You want sell a devil's helmet for %P gold?", setTopic(50)}},
	{{"sell", "warrior", "helmet"}, {setType(3369), setAmount(1), setPrice(696), "You want sell a warrior helmet for %P gold?", setTopic(50)}},

	{{"sell", P1, lt(1, P1), "leather", "helm"}, {setType(3355), setAmount(P1), setPrice(mul(4, P1)), "Do you want to sell %A leather helmets for %P gold?", setTopic(50)}},
	{{"sell", P1, lt(1, P1), "chain", "helm"}, {setType(3352), setAmount(P1), setPrice(mul(17, P1)), "Do you want to sell %A chain helmets for %P gold?", setTopic(50)}},
	{{"sell", P1, lt(1, P1), "steel", "helm"}, {setType(3351), setAmount(P1), setPrice(mul(190, P1)), "Do you want to sell %A steel helmets for %P gold?", setTopic(50)}},
	{{"sell", P1, lt(1, P1), "brass", "helm"}, {setType(3354), setAmount(P1), setPrice(mul(30, P1)), "Do you want to sell %A brass helmets for %P gold?", setTopic(50)}},
	{{"sell", P1, lt(1, P1), "viking", "helm"}, {setType(3367), setAmount(P1), setPrice(mul(66, P1)), "Do you want to sell %A viking helmets for %P gold?", setTopic(50)}},
	{{"sell", P1, lt(1, P1), "iron", "helmet"}, {setType(3353), setAmount(P1), setPrice(mul(145, P1)), "You want sell %A iron helmets for %P gold?", setTopic(50)}},
	{{"sell", P1, lt(1, P1), "devil", "helmet"}, {setType(3356), setAmount(P1), setPrice(mul(450, P1)), "You want sell %A devil's helmets for %P gold?", setTopic(50)}},
	{{"sell", P1, lt(1, P1), "warrior", "helmet"}, {setType(3369), setAmount(P1), setPrice(mul(696, P1)), "You want sell %A warrior helmets for %P gold?", setTopic(50)}},

	{{eq(TOPIC, 50), "yes", ge(count(TYPE), AMOUNT)}, {"Ok. Here is your money.", delete(TYPE), createMoney}},
	{{eq(TOPIC, 50), "yes"}, {"Sorry, you do not have one."}},
	{{eq(TOPIC, 50), "yes", gt(AMOUNT, 1)}, {"Sorry, you do not have so much."}},
	{{eq(TOPIC, 50)}, {"Maybe next time."}},
}
