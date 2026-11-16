-- lib/npcs/gen-xmas.lua: the rule block gen-xmas.ndb, converted by patella-convert.
-- An NPC that included it (@"gen-xmas.ndb") has it in its rules where the include
-- was; lib/npc.lua matches and runs them (scripting.hh, "NPCs"). The comments
-- are those of gen-xmas.ndb.
--
-- GIMUD - Graphical Interface Multi User Dungeon
-- gen-xmas.ndb: Datenbank für generische Weihnachtsmänner

local behaviour = require("lib.npc")
local ADDRESS = behaviour.ADDRESS
local BUSY = behaviour.BUSY
local VANISH = behaviour.VANISH
local FORCE = behaviour.FORCE
local eq = behaviour.eq
local lt = behaviour.lt
local gt = behaviour.gt
local AMOUNT = behaviour.AMOUNT
local TYPE = behaviour.TYPE
local questValue = behaviour.questValue
local random = behaviour.random
local setAmount = behaviour.setAmount
local setType = behaviour.setType
local setQuestValue = behaviour.setQuestValue
local create = behaviour.create
local idle = behaviour.idle
local nop = behaviour.nop
local previous = behaviour.previous

return behaviour.rules {
	{{ADDRESS, "hello$", FORCE}, {"Merry Christmas, little %N!", setAmount(random(1, 10000))}},
	{{ADDRESS, "hi$", FORCE}, {previous}},
	{{ADDRESS, FORCE}, {idle}},
	{{BUSY, "hello$", FORCE}, {"Please wait! I am ready for you soon!"}},
	{{BUSY, "hi$", FORCE}, {previous}},
	{{BUSY, FORCE}, {nop}},
	{{VANISH, FORCE}, {"Good bye, little %N!"}},

	{{"bye"}, {"Farewell, %N!", idle}},
	{{"farewell"}, {previous}},
	{{"job"}, {"Ho ho ho! You don't know Santa Claus? Never mind. You may ask me for a present."}},
	{{"name"}, {"Sorry, I don't have time to chat. Please ask for your present."}},

	--verteilt orange
	{{"present", lt(questValue(217), 3), lt(AMOUNT, 1000), FORCE}, {"Here is your present! Enjoy!", setQuestValue(217, 3), setType(3586), setAmount(10), create(TYPE), idle}},
	--verteilt candy canes
	{{"present", lt(questValue(217), 3), lt(AMOUNT, 3500), gt(AMOUNT, 999), FORCE}, {"Here is your present! Enjoy!", setQuestValue(217, 3), setType(3599), setAmount(8), create(TYPE), idle}},
	--verteilt apfel
	{{"present", lt(questValue(217), 3), lt(AMOUNT, 5000), gt(AMOUNT, 3499), FORCE}, {"Here is your present! Enjoy!", setQuestValue(217, 3), setType(3585), setAmount(15), create(TYPE), idle}},
	--verteilt kekse
	{{"present", lt(questValue(217), 3), lt(AMOUNT, 6000), gt(AMOUNT, 4999), FORCE}, {"Here is your present! Enjoy!", setQuestValue(217, 3), setType(3598), setAmount(8), create(TYPE), idle}},
	--verteilt schneeball
	{{"present", lt(questValue(217), 3), lt(AMOUNT, 9950), gt(AMOUNT, 5999), FORCE}, {"Here is your present! Enjoy!", setQuestValue(217, 3), setType(2992), setAmount(5), create(TYPE), idle}},
	--verteilt doll
	{{"present", lt(questValue(217), 3), lt(AMOUNT, 9999), gt(AMOUNT, 9949), FORCE}, {"Here is your present! Enjoy!", setQuestValue(217, 3), setType(2991), setAmount(1), create(TYPE), idle}},
	--verteilt teddy
	{{"present", lt(questValue(217), 3), eq(AMOUNT, 10000), FORCE}, {"Here is your present! Enjoy!", setQuestValue(217, 3), setType(2993), setAmount(1), create(TYPE), idle}},

	{{"present", eq(questValue(217), 3)}, {"You already got your present! Next please!", idle}},
}
