-- npcs/budrik.lua: the behaviour of the NPC Budrik, converted from
-- budrik.npc by patella-convert. The NPC is its [[npc]] entry in npcs.toml;
-- lib/npc.lua matches and runs the rules below, in the order of the file
-- (scripting.hh, "NPCs"). The comments are those of budrik.npc.
--
-- GIMUD - Graphical Interface Multi User Dungeon
-- budrik.npc: Datenbank für den Minenvorsteher Budrik 

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
	{{ADDRESS, "hello$", FORCE}, {"Hiho, Hiho %N."}},
	{{ADDRESS, "hi$", FORCE}, {previous}},
	{{ADDRESS, "hiho$", FORCE}, {previous}},
	{{ADDRESS, FORCE}, {idle}},
	{{BUSY, "hello$", FORCE}, {"Just a minute %N!", queue}},
	{{BUSY, "hi$", FORCE}, {previous}},
	{{BUSY, "hiho$", FORCE}, {previous}},
	{{BUSY, FORCE}, {nop}},
	{{VANISH, FORCE}, {"Bye, bye."}},

	{{"bye"}, {"Bye, bye.", idle}},
	{{"job"}, {"I am the foreman of this mine."}},
	{{"shop"}, {previous}},
	{{"name"}, {"My name is Budrik Deepdigger, son of Earth, from the Molten Rock."}},
	{{"time"}, {"Precisely %T, young one."}},
	{{"help"}, {"I am a miner, ask someone else."}},
	{{"dwarfs"}, {"We understand the ways of the earth like nobody else does."}},
	{{"monster"}, {"In the deeper mines we discover some nasty beasts now and then."}},
	{{"dungeon"}, {"This is no funhouse. Leave the miners and their drilling-worms alone and get out! We have already enough trouble without you."}},
	{{"mines"}, {previous}},
	{{"trouble"}, {"The Horned Fox is leading his bandits in sneak attacks and raids on us."}},
	{{"horned", "fox"}, {"A minotaur they threw out at Mintwallin. He must have some kind of hideout nearby."}},
	{{"hideout"}, {"The hideout of the Horned Fox is probably a dangerous if not lethal place for the unexperienced ones."}},
}

function onCreatureSay(npc, creature, text, situation)
	behaviour.react(npc, rules, text, situation)
end

function onCreatureDisappear(npc, creature)
	behaviour.react(npc, rules, "", NPC_SITUATION_VANISH)
end
