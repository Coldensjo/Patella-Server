-- npcs/olddragon.lua: the behaviour of the NPC An Old Dragonlord, converted from
-- olddragon.npc by patella-convert. The NPC is its [[npc]] entry in npcs.toml;
-- lib/npc.lua matches and runs the rules below, in the order of the file
-- (scripting.hh, "NPCs"). The comments are those of olddragon.npc.
--
-- GIMUD - Graphical Interface Multi User Dungeon
-- olddragon.npc: Datenbank für den alten Drachenlord

local behaviour = require("lib.npc")
local ADDRESS = behaviour.ADDRESS
local FORCE = behaviour.FORCE
local eq = behaviour.eq
local lt = behaviour.lt
local ge = behaviour.ge
local count = behaviour.count
local questValue = behaviour.questValue
local setAmount = behaviour.setAmount
local setQuestValue = behaviour.setQuestValue
local create = behaviour.create
local delete = behaviour.delete
local idle = behaviour.idle
local previous = behaviour.previous

local rules = behaviour.rules {
	{{ADDRESS, "hello$", ge(count(3723), 1), lt(questValue(66), 1), FORCE}, {"AHHH MUSHRRROOOMSSS! NOW MY PAIN WILL BE EASSSED FOR A WHILE! TAKE THISS AND LEAVE THE DRAGONSSS' CEMETERY AT ONCE!", setAmount(1), delete(3723), create(3206), setQuestValue(66, 1), idle}},
	{{ADDRESS, "hi$", ge(count(3723), 1), lt(questValue(66), 1), FORCE}, {previous}},
	{{ADDRESS, eq(questValue(66), 1), FORCE}, {"LEAVE THE DRAGONS' CEMETERY AT ONCE!", idle}},

	{{ADDRESS}, {"AHHHH THE PAIN OF AGESSS! I NEED MUSSSSHRROOOMSSS TO EASSSE MY PAIN! BRRRING ME MUSHRRROOOMSSS!", idle}},
	{{ADDRESS}, {previous}},
}

function onCreatureSay(npc, creature, text, situation)
	behaviour.react(npc, rules, text, situation)
end

function onCreatureDisappear(npc, creature)
	behaviour.react(npc, rules, "", NPC_SITUATION_VANISH)
end
