-- npcs/avar.lua: the behaviour of the NPC Avar Tar, converted from
-- avar.npc by patella-convert. The NPC is its [[npc]] entry in npcs.toml;
-- lib/npc.lua matches and runs the rules below, in the order of the file
-- (scripting.hh, "NPCs"). The comments are those of avar.npc.
--
-- GIMUD - Graphical Interface Multi User Dungeon
-- avar.npc: Datenbank für den Abenteurer Avar Tar

local behaviour = require("lib.npc")
local ADDRESS = behaviour.ADDRESS
local BUSY = behaviour.BUSY
local VANISH = behaviour.VANISH
local FORCE = behaviour.FORCE
local idle = behaviour.idle
local nop = behaviour.nop
local previous = behaviour.previous

local rules = behaviour.rules {
	{{ADDRESS, "hello$", FORCE}, {"Greetings, traveler %N!"}},
	{{ADDRESS, "hi$", FORCE}, {previous}},
	{{ADDRESS, FORCE}, {idle}},
	{{BUSY, "hello$", FORCE}, {"Only one chat at the same time, sorry."}},
	{{BUSY, "hi$", FORCE}, {previous}},
	{{BUSY, FORCE}, {nop}},
	{{VANISH, FORCE}, {"See you later."}},

	{{"bye"}, {"See you later.", idle}},
	{{"name"}, {"I am Avar Tar, slayer of monsters, saviour of princesses, and defender of the weak."}},
	{{"job"}, {"I am a professional hero."}},
	{{"time"}, {"It's %T right now."}},
	{{"king"}, {"I am on a quest for the Thaian king ... as usual, of course."}},
	{{"tibianus"}, {previous}},
	{{"quest"}, {previous}},
	{{"army"}, {"Where the army fails a hero like me is needed."}},
	{{"ferumbras"}, {"I fought him serveral times, sometimes he killed me, sometimes I killed him, I would say we are even right now, but I am getting better and more powerful each day."}},
	{{"excalibug"}, {"I am sure it's hidden in a vault of the Nightmare Knights beneath the Plains of Havoc. I plan an expedition to go there and rout out the Ruthless Seven, but I have to save the world first."}},
	{{"thais"}, {"If I had the time I would restore peace in this once proud city, but there's too much to do before I can start that quest."}},
	{{"tibia"}, {"I've seen it all and done it all ... at least twice."}},
	{{"carlin"}, {"I saved the women there once or twice."}},
	{{"news"}, {"There is a great evil lurking beneath this isle ... and beneath the Plains of Havoc and in the ancient necropolis and beneath the Ghostlands ... well everywhere basically."}},
	{{"rumors"}, {previous}},
}

function onCreatureSay(npc, creature, text, situation)
	behaviour.react(npc, rules, text, situation)
end

function onCreatureDisappear(npc, creature)
	behaviour.react(npc, rules, "", NPC_SITUATION_VANISH)
end
