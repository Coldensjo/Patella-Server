-- npcs/mortimer.lua: the behaviour of the NPC Mortimer, converted from
-- mortimer.npc by patella-convert. The NPC is its [[npc]] entry in npcs.toml;
-- lib/npc.lua matches and runs the rules below, in the order of the file
-- (scripting.hh, "NPCs"). The comments are those of mortimer.npc.
--
-- GIMUD - Graphical Interface Multi User Dungeon 
-- angus: Datenbank für den Teamassitstenten der explorers society Mortimer 

local behaviour = require("lib.npc")
local explorer = require("lib.npcs.explorer")
local ADDRESS = behaviour.ADDRESS
local BUSY = behaviour.BUSY
local VANISH = behaviour.VANISH
local FORCE = behaviour.FORCE
local eq = behaviour.eq
local lt = behaviour.lt
local TOPIC = behaviour.TOPIC
local questValue = behaviour.questValue
local setTopic = behaviour.setTopic
local setQuestValue = behaviour.setQuestValue
local create = behaviour.create
local queue = behaviour.queue
local idle = behaviour.idle
local nop = behaviour.nop
local previous = behaviour.previous

local rules = behaviour.rules {
	{{ADDRESS, "hello$", FORCE}, {"Greetings, what can I do for you?"}},
	{{ADDRESS, "hi$", FORCE}, {previous}},

	{{ADDRESS, FORCE}, {idle}},
	{{BUSY, "hello$", FORCE}, {"Please wait a moment.", queue}},
	{{BUSY, "hi$", FORCE}, {previous}},
	{{BUSY, FORCE}, {nop}},
	{{VANISH, FORCE}, {"How rude!"}},

	{{"bye"}, {"Good bye.", idle}},
	{{"farewell"}, {previous}},

	--############### Später ab hier besser bd nutzen 

	explorer,

	--###############

	{{"mission", eq(questValue(300), 12), lt(questValue(320), 1)}, {"With the objects you've provided our researchers will make steady progress. Still we are missing some test results from fellow explorers ...", "Please travel to our base in Port Hope and ask them to mail us their latest research reports. Then return here and ask about new missions.", setQuestValue(320, 2)}},

	{{"research", "reports", eq(questValue(320), 1)}, {"Oh, yes! Tell our fellow explorer that the papers are in the mail already.", setQuestValue(320, 3)}},
	{{"mission", eq(questValue(320), 1)}, {previous}},

	--####
	{{"mission", eq(questValue(320), 4)}, {"The reports from Port Hope have already arrived here and our progress is astonishing. We think it is possible to create an astral bridge between our bases. Are you interested to assist us with this?", setTopic(33)}},
	--####
	{{"no", eq(TOPIC, 33)}, {"Perhaps you are interested some other time."}},
	{{"yes", eq(TOPIC, 33)}, {"Good, just take this spectral essence and use it on the strange carving in this building as well as on the corresponding tile in our base at Port Hope ...", "As soon as you have charged the portal tiles that way, report about the spectral portals.", create(4840), setQuestValue(320, 5)}},

	--#### topic 34 verwendet
}

function onCreatureSay(npc, creature, text, situation)
	behaviour.react(npc, rules, text, situation)
end

function onCreatureDisappear(npc, creature)
	behaviour.react(npc, rules, "", NPC_SITUATION_VANISH)
end
