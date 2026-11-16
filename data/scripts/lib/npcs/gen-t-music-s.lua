-- lib/npcs/gen-t-music-s.lua: the rule block gen-t-music-s.ndb, converted by patella-convert.
-- An NPC that included it (@"gen-t-music-s.ndb") has it in its rules where the include
-- was; lib/npc.lua matches and runs them (scripting.hh, "NPCs"). The comments
-- are those of gen-t-music-s.ndb.
--
-- GIMUD - Graphical Interface Multi User Dungeon
-- genTshieldV.ndb: Datenbank für generischen Instrumentenverkauf

local behaviour = require("lib.npc")
local eq = behaviour.eq
local ge = behaviour.ge
local TOPIC = behaviour.TOPIC
local PRICE = behaviour.PRICE
local TYPE = behaviour.TYPE
local COUNTMONEY = behaviour.COUNTMONEY
local setTopic = behaviour.setTopic
local setPrice = behaviour.setPrice
local setAmount = behaviour.setAmount
local setType = behaviour.setType
local create = behaviour.create
local deleteMoney = behaviour.deleteMoney
local previous = behaviour.previous

return behaviour.rules {
	{{"offer"}, {"You can buy a lyre, lute, drum, and simple fanfare."}},
	{{"goods"}, {previous}},
	{{"do", "you", "sell"}, {previous}},
	{{"do", "you", "have"}, {previous}},
	{{"instrument"}, {previous}},

	{{"lyre"}, {setType(2949), setAmount(1), setPrice(120), "Do you want to buy a lyre for %P gold?", setTopic(38)}},
	{{"lute"}, {setType(2950), setAmount(1), setPrice(195), "Do you want to buy a lute for %P gold?", setTopic(38)}},
	{{"drum"}, {setType(2952), setAmount(1), setPrice(140), "Do you want to buy a drum for %P gold?", setTopic(38)}},
	{{"simple", "fanfare"}, {setType(2954), setAmount(1), setPrice(150), "Do you want to buy a simple fanfare for %P gold?", setTopic(38)}},

	{{eq(TOPIC, 38), "yes", ge(COUNTMONEY, PRICE)}, {"Here you are.", deleteMoney, create(TYPE)}},
	{{eq(TOPIC, 38), "yes"}, {"Come back, when you have enough money."}},
	{{eq(TOPIC, 38)}, {"Hmm, but next time."}},
}
