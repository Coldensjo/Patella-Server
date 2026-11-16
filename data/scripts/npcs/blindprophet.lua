-- npcs/blindprophet.lua: the behaviour of the NPC The Blind Prophet, converted from
-- blindprophet.npc by patella-convert. The NPC is its [[npc]] entry in npcs.toml;
-- lib/npc.lua matches and runs the rules below, in the order of the file
-- (scripting.hh, "NPCs"). The comments are those of blindprophet.npc.
--
-- GIMUD - Graphical Interface Multi User Dungeon
-- blindprophet.npc: Datenbank für den blinden affenpropheten

local behaviour = require("lib.npc")
local ADDRESS = behaviour.ADDRESS
local BUSY = behaviour.BUSY
local VANISH = behaviour.VANISH
local PZBLOCK = behaviour.PZBLOCK
local FORCE = behaviour.FORCE
local eq = behaviour.eq
local lt = behaviour.lt
local gt = behaviour.gt
local TOPIC = behaviour.TOPIC
local questValue = behaviour.questValue
local setTopic = behaviour.setTopic
local effectOpp = behaviour.effectOpp
local idle = behaviour.idle
local nop = behaviour.nop
local teleport = behaviour.teleport
local previous = behaviour.previous

local rules = behaviour.rules {
	{{ADDRESS, "hello$", gt(questValue(293), 14), FORCE}, {"Be greeted, friend of the apes."}},
	{{ADDRESS, "hi$", gt(questValue(293), 14), FORCE}, {previous}},

	{{ADDRESS, "hello$", FORCE}, {"You not should be here! You go! You go!", idle}},
	{{ADDRESS, "hi$", FORCE}, {previous}},

	{{ADDRESS, "hello$", FORCE}, {"Be greeted, friend of the apes."}},
	{{ADDRESS, "hi$", FORCE}, {previous}},

	{{ADDRESS, FORCE}, {idle}},

	{{BUSY, "hello$", FORCE}, {"Sorry, I am busy."}},
	{{BUSY, "hi$", FORCE}, {previous}},
	{{BUSY, FORCE}, {nop}},
	{{VANISH, FORCE}, {"Good bye."}},

	{{"bye"}, {"Good bye.", idle}},
	{{"name"}, {"Me put name away name long ago. Now only blind prophet of ape people are."}},
	{{"job"}, {"Me prophet and guardian is."}},
	{{"prophet"}, {"Me is who in dreams speak to holy banana. Me divine the will of banana."}},
	{{"guardian"}, {"Me guard the forbidden land behind the great palisade. If any want to enter, he must ask me for transport."}},
	{{"forbidden"}, {previous}},

	{{"transport", gt(questValue(293), 14)}, {"You want me to transport you to forbidden land?", setTopic(1)}},
	{{"transport", lt(questValue(293), 15)}, {"No!"}},
	{{"yes", eq(TOPIC, 1), PZBLOCK, FORCE}, {"Anger of battle is burning in you! First calm down."}},
	{{"yes", eq(TOPIC, 1)}, {"Take care!", teleport(33026, 32580, 6), effectOpp(11)}},
	{{"no", eq(TOPIC, 1)}, {"Wise decision maybe."}},

	{{"Hairycles"}, {"Good ape he is. Has to work hard to make other apes listen but you helped a lot."}},
	{{"excalibug"}, {"Me not know. Me seldom have visions of not banana related objects."}},
	{{"bong"}, {"Our holy ancestor he is. Big as mountain. Lizards say they built palisade to keep him but we not believe ...", "We think Bong palisade built to have peace from pesky lizards. We respect peace of Bong, keep people away from forbidden land."}},
	{{"ape"}, {"Our people a lot to learn have. One day we might live in peace with you hairless apes, who knows."}},
	{{"port", "hope"}, {"Hairless apes strange people are. "}},
	{{"lizard"}, {"The lizards evil and vengeful are. Ape people on guard must be."}},
	{{"hair"}, {"Me visions show hair in the far north west of forbidden land. Near coast look for signs of Bongs presence."}},
}

function onCreatureSay(npc, creature, text, situation)
	behaviour.react(npc, rules, text, situation)
end

function onCreatureDisappear(npc, creature)
	behaviour.react(npc, rules, "", NPC_SITUATION_VANISH)
end
