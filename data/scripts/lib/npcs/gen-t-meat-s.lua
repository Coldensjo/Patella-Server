-- lib/npcs/gen-t-meat-s.lua: the rule block gen-t-meat-s.ndb, converted by patella-convert.
-- An NPC that included it (@"gen-t-meat-s.ndb") has it in its rules where the include
-- was; lib/npc.lua matches and runs them (scripting.hh, "NPCs"). The comments
-- are those of gen-t-meat-s.ndb.
--
-- GIMUD - Graphical Interface Multi User Dungeon
-- gen-tmeat-s.ndb:  Datenbank für generischen Fleischverkauf

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
	{{"meat"}, {setType(3577), setAmount(1), setPrice(5), "Do you want to buy meat for %P gold?", setTopic(39)}},
	{{"ham"}, {setType(3582), setAmount(1), setPrice(8), "Do you want to buy a ham for %P gold?", setTopic(39)}},

	{{P1, lt(1, P1), "meat"}, {setType(3577), setAmount(P1), setPrice(mul(5, P1)), "Do you want to buy %A meat for %P gold?", setTopic(39)}},
	{{P1, lt(1, P1), "ham"}, {setType(3582), setAmount(P1), setPrice(mul(8, P1)), "Do you want to buy %A ham for %P gold?", setTopic(39)}},

	{{eq(TOPIC, 39), "yes", ge(COUNTMONEY, PRICE)}, {"Here it is.", deleteMoney, create(TYPE)}},
	{{eq(TOPIC, 39), "yes"}, {"I am sorry, but you do not have enough gold."}},
	{{eq(TOPIC, 39)}, {"Maybe later."}},
}
