-- npcs/ferryman2.lua: the behaviour of the NPC Anderson, converted from
-- ferryman2.npc by patella-convert. The NPC is its [[npc]] entry in npcs.toml;
-- lib/npc.lua matches and runs the rules below, in the order of the file
-- (scripting.hh, "NPCs"). The comments are those of ferryman2.npc.
--
-- GIMUD - Graphical Interface Multi User Dungeon
-- ferryman2.npc: Fährmann Anderson auf Senja (Ice)

local behaviour = require("lib.npc")
local ADDRESS = behaviour.ADDRESS
local BUSY = behaviour.BUSY
local VANISH = behaviour.VANISH
local MALE = behaviour.MALE
local FEMALE = behaviour.FEMALE
local PZBLOCK = behaviour.PZBLOCK
local FORCE = behaviour.FORCE
local eq = behaviour.eq
local ge = behaviour.ge
local TOPIC = behaviour.TOPIC
local PRICE = behaviour.PRICE
local COUNTMONEY = behaviour.COUNTMONEY
local setTopic = behaviour.setTopic
local setPrice = behaviour.setPrice
local effectOpp = behaviour.effectOpp
local deleteMoney = behaviour.deleteMoney
local queue = behaviour.queue
local idle = behaviour.idle
local nop = behaviour.nop
local teleport = behaviour.teleport
local previous = behaviour.previous

local rules = behaviour.rules {
	{{ADDRESS, "hello$", MALE, FORCE}, {"Ahoi, young man %N and welcome to the Nordic Tibia Ferries."}},
	{{ADDRESS, "hi$", MALE, FORCE}, {previous}},
	{{ADDRESS, "hello$", FEMALE, FORCE}, {"Ahoi, young lady %N and welcome to the Nordic Tibia Ferries."}},
	{{ADDRESS, "hi$", FEMALE, FORCE}, {previous}},
	{{ADDRESS, FORCE}, {idle}},
	{{BUSY, "hello$", FORCE}, {"One moment please, %N.", queue}},
	{{BUSY, "hi$", FORCE}, {previous}},
	{{BUSY, FORCE}, {nop}},
	{{VANISH, FORCE}, {"Good bye. You are welcome."}},

	{{"bye"}, {"Good bye. You are welcome.", idle}},
	{{"farewell"}, {previous}},
	{{"name"}, {"My name is Anderson from the Nordic Tibia Ferries."}},
	{{"anderson"}, {"The four of us are the captains of the Nordic Tibia Ferries."}},
	{{"svenson"}, {previous}},
	{{"carlson"}, {previous}},
	{{"nielson"}, {previous}},
	{{"job"}, {"We are ferrymen. We transport goods and passengers to the Ice Islands."}},
	{{"captain"}, {previous}},
	{{"ship"}, {"Our ferries are strong enough to stand the high waves of the Nordic Ocean."}},
	{{"ferry"}, {previous}},
	{{"ferries"}, {previous}},
	{{"water"}, {previous}},
	{{"good"}, {"We can transport everything you want."}},
	{{"passenger"}, {"We would like to welcome you on board our ferries."}},
	{{"trip"}, {"Where do you want to go today? We serve the routes to Senja, Folda, and Vega, and back to Tibia."}},
	{{"passage"}, {previous}},
	{{"round", "trip"}, {"The fee for the trip back to Tibia is included."}},
	{{"island"}, {"We serve the routes to Senja, Folda, and Vega, and back to Tibia."}},
	{{"route"}, {previous}},

	{{"senja"}, {"This island is Senja."}},
	{{"folda"}, {setPrice(10), "Do you want a passage to Folda for %P gold?", setTopic(1)}},
	{{"vega"}, {setPrice(10), "Do you want a passage to Vega for %P gold?", setTopic(2)}},
	{{"tibia"}, {setPrice(0), "Do you want a free passage back to Tibia?", setTopic(3)}},

	{{eq(TOPIC, 1), "yes", PZBLOCK, FORCE}, {"First get rid of those blood stains! You are not going to ruin my vehicle!"}},
	{{eq(TOPIC, 2), "yes", PZBLOCK, FORCE}, {previous}},
	{{eq(TOPIC, 3), "yes", PZBLOCK, FORCE}, {previous}},

	{{eq(TOPIC, 1), "yes", ge(COUNTMONEY, PRICE)}, {"Have a nice trip!", deleteMoney, idle, effectOpp(11), teleport(32048, 31582, 7), effectOpp(11)}},
	{{eq(TOPIC, 1), "yes"}, {"You don't have enough money."}},
	{{eq(TOPIC, 1)}, {"You shouldn't miss the experience."}},

	{{eq(TOPIC, 2), "yes", ge(COUNTMONEY, PRICE)}, {"Have a nice trip!", deleteMoney, idle, effectOpp(11), teleport(32025, 31692, 7), effectOpp(11)}},
	{{eq(TOPIC, 2), "yes"}, {"You don't have enough money."}},
	{{eq(TOPIC, 2)}, {"You shouldn't miss the experience."}},

	{{eq(TOPIC, 3), "yes", ge(COUNTMONEY, PRICE)}, {"Have a nice trip!", deleteMoney, idle, effectOpp(11), teleport(32236, 31677, 7), effectOpp(11)}},
	{{eq(TOPIC, 3), "yes"}, {"You don't have enough money."}},
	{{eq(TOPIC, 3)}, {"You shouldn't miss the experience."}},
}

function onCreatureSay(npc, creature, text, situation)
	behaviour.react(npc, rules, text, situation)
end

function onCreatureDisappear(npc, creature)
	behaviour.react(npc, rules, "", NPC_SITUATION_VANISH)
end
