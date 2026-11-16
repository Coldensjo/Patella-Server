-- npcs/crone.lua: the behaviour of the NPC The Crone, converted from
-- crone.npc by patella-convert. The NPC is its [[npc]] entry in npcs.toml;
-- lib/npc.lua matches and runs the rules below, in the order of the file
-- (scripting.hh, "NPCs"). The comments are those of crone.npc.
--
-- GIMUD - Graphical Interface Multi User Dungeon
-- crone.npc: Bansheevettel in Vashresamuns Grabmal

local behaviour = require("lib.npc")
local ADDRESS = behaviour.ADDRESS
local BUSY = behaviour.BUSY
local VANISH = behaviour.VANISH
local FORCE = behaviour.FORCE
local queue = behaviour.queue
local idle = behaviour.idle
local nop = behaviour.nop
local previous = behaviour.previous

local rules = behaviour.rules {
	{{ADDRESS, "hello$", FORCE}, {"Be greeted, %N... mortal"}},
	{{ADDRESS, "hi$", FORCE}, {previous}},
	{{ADDRESS, FORCE}, {idle}},
	{{BUSY, "hello$", FORCE}, {"Patience, mortal %N.", queue}},
	{{BUSY, "hi$", FORCE}, {previous}},
	{{BUSY, FORCE}, {nop}},
	{{VANISH, FORCE}, {"Good bye."}},

	{{"bye"}, {"Good bye.", idle}},
	{{"farewell"}, {previous}},
	{{"name"}, {"I don't remember my name, neither my days as a mortal."}},
	{{"job"}, {"Once I was Vashresamun's favourite handmaiden. But I have fallen from her grace and now I am exiled from her tomb."}},
	{{"grace"}, {"Do not ask about that, mortal. Memories bring too much grief."}},
	{{"fallen"}, {previous}},
	{{"exiled"}, {previous}},

	{{"vashresamun"}, {"I mourn the dark day I was exiled from her tomb."}},
	{{"tomb"}, {"Her tomb is sealed and can only be entered by a certain melody."}},
	{{"melody"}, {"Vashresamun erased the memory of the tune from my mind, I only remember its name: the secret of the rose garden."}},
}

function onCreatureSay(npc, creature, text, situation)
	behaviour.react(npc, rules, text, situation)
end

function onCreatureDisappear(npc, creature)
	behaviour.react(npc, rules, "", NPC_SITUATION_VANISH)
end
