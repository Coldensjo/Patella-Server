-- lib/npcs/gen-bank.lua: the rule block gen-bank.ndb, converted by patella-convert.
-- An NPC that included it (@"gen-bank.ndb") has it in its rules where the include
-- was; lib/npc.lua matches and runs them (scripting.hh, "NPCs"). The comments
-- are those of gen-bank.ndb.
--
-- GIMUD - Graphical Interface Multi User Dungeon
-- gen-post.ndb:  Datenbank für generischen Banker
-- Verwendete Topics: 91 bis 99
-- Topic 91(96): Gold -> Platinum
-- Topic 92(97):         Platinum <- Crystal
-- Topic 93(98): Gold <- Platinum
-- Topic 94(99):         Platinum -> Crystal

local behaviour = require("lib.npc")
local FORCE = behaviour.FORCE
local P1 = behaviour.P1
local eq = behaviour.eq
local lt = behaviour.lt
local ge = behaviour.ge
local TOPIC = behaviour.TOPIC
local PRICE = behaviour.PRICE
local AMOUNT = behaviour.AMOUNT
local count = behaviour.count
local mul = behaviour.mul
local setTopic = behaviour.setTopic
local setPrice = behaviour.setPrice
local setAmount = behaviour.setAmount
local create = behaviour.create
local delete = behaviour.delete
local previous = behaviour.previous

return behaviour.rules {
	{{"bank"}, {"We can change money for you."}},
	{{"offer"}, {"We exchange gold, platinum and crystal coins."}},
	{{"sell"}, {previous}},
	{{"do", "you", "have"}, {previous}},
	{{"buy"}, {previous}},
	{{"money"}, {previous}},
	{{"change"}, {previous}},
	{{"exchange"}, {previous}},

	{{"change", "gold", FORCE}, {"How many platinum coins do you want to get?", setTopic(91)}},
	{{"exchange", "gold", FORCE}, {previous}},
	{{"change", "platinum", FORCE}, {"Do you want to change your platinum coins to gold or crystal?", setTopic(95)}},
	{{"exchange", "platinum", FORCE}, {previous}},
	{{"change", "crystal", FORCE}, {"How many crystal coins do you want to change to platinum?", setTopic(92)}},
	{{"exchange", "crystal", FORCE}, {previous}},
	{{"change"}, {"Do you want to exchange gold, platinum or crystal coins?"}},

	{{eq(TOPIC, 95), "gold", FORCE}, {"How many platinum coins do you want to change to gold?", setTopic(93)}},
	{{eq(TOPIC, 95), "crystal", FORCE}, {"How many crystal coins do you want to get?", setTopic(94)}},
	{{eq(TOPIC, 95)}, {"Well, can I help you with something else?"}},

	{{eq(TOPIC, 91), P1, lt(0, P1)}, {setAmount(mul(P1, 100)), setPrice(P1), "So I should change %A of your gold coins to %P platinum coins for you?", setTopic(96)}},
	{{eq(TOPIC, 91)}, {"Hmm, can I help you with something else?"}},

	{{eq(TOPIC, 92), P1, lt(0, P1)}, {setAmount(P1), setPrice(mul(P1, 100)), "So I should change %A of your crystal coins to %P platinum coins for you?", setTopic(97)}},
	{{eq(TOPIC, 92)}, {"Well, can I help you with something else?"}},

	{{eq(TOPIC, 93), P1, lt(0, P1)}, {setAmount(P1), setPrice(mul(P1, 100)), "So I should change %A of your platinum coins to %P gold coins for you?", setTopic(98)}},
	{{eq(TOPIC, 93)}, {"Well, can I help you with something else?"}},

	{{eq(TOPIC, 94), P1, lt(0, P1)}, {setAmount(mul(P1, 100)), setPrice(P1), "So I should change %A of your platinum coins to %P crystal coins for you?", setTopic(99)}},
	{{eq(TOPIC, 94)}, {"Well, can I help you with something else?"}},

	{{eq(TOPIC, 96), "yes", ge(count(3031), AMOUNT)}, {"Here you are.", delete(3031), setAmount(PRICE), create(3035)}},
	{{eq(TOPIC, 96), "yes"}, {"Sorry, you don't have enough gold coins."}},
	{{eq(TOPIC, 96)}, {"Well, can I help you with something else?"}},

	{{eq(TOPIC, 97), "yes", ge(count(3043), AMOUNT)}, {"Here you are.", delete(3043), setAmount(PRICE), create(3035)}},
	{{eq(TOPIC, 97), "yes"}, {"Sorry, you don't have so many crystal coins."}},
	{{eq(TOPIC, 97)}, {"Well, can I help you with something else?"}},

	{{eq(TOPIC, 98), "yes", ge(count(3035), AMOUNT)}, {"Here you are.", delete(3035), setAmount(PRICE), create(3031)}},
	{{eq(TOPIC, 98), "yes"}, {"Sorry, you don't have so many platinum coins."}},
	{{eq(TOPIC, 98)}, {"Well, can I help you with something else?"}},

	{{eq(TOPIC, 99), "yes", ge(count(3035), AMOUNT)}, {"Here you are.", delete(3035), setAmount(PRICE), create(3043)}},
	{{eq(TOPIC, 99), "yes"}, {"Sorry, you don't have so many platinum coins."}},
	{{eq(TOPIC, 99)}, {"Well, can I help you with something else?"}},

	--Topic=91,%1,0<%1 -> Amount=%1, Price=100*%1, "So I should change %P gold coins to %A platinum coins for you?", Topic=96
	--Topic=91         -> "Hmm, can I help you with something else?"

	--Topic=92,%1,0<%1 -> Amount=%1*100, Price=%1, "So I should change %P crystal coins to %A platinum coins for you?", Topic=97
	--Topic=92         -> "Well, can I help you with something else?"

	--Topic=93,%1,0<%1 -> Amount=%1*100, Price=%1, "So I should change %P platinum coins to %A gold coins for you?", Topic=98
	--Topic=93         -> "Well, can I help you with something else?"

	--Topic=94,%1,0<%1 -> Amount=%1, Price=%1*100, "So I should change %P platinum coins to %A crystal coins for you?", Topic=99
	--Topic=94         -> "Well, can I help you with something else?"

	--Topic=96,"yes",Count(3031)>=Price  -> "Here you are.", Create(3035), Amount=Price, Delete(3031)
	--Topic=96,"yes"                     -> "Sorry, you don't have enough gold coins."
	--Topic=96                           -> "Well, can I help you with something else?"

	--Topic=97,"yes",Count(3043)>=Price -> "Here you are.", Create(3035), Amount=Price, Delete(3043)
	--Topic=97,"yes"                     -> "Sorry, you don't have so many crystal coins."
	--Topic=97                           -> "Well, can I help you with something else?"

	--Topic=98,"yes",Count(3035)>=Price  -> "Here you are.", Create(3031), Amount=Price, Delete(3035)
	--Topic=98,"yes"                     -> "Sorry, you don't have so many platinum coins."
	--Topic=98                           -> "Well, can I help you with something else?"

	--Topic=99,"yes",Count(3035)>=Price  -> "Here you are.", Create(3043), Amount=Price, Delete(3035)
	--Topic=99,"yes"                     -> "Sorry, you don't have so many platinum coins."
	--Topic=99                           -> "Well, can I help you with something else?"
}
