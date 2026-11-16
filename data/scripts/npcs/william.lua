-- npcs/william.lua: the behaviour of the NPC William, converted from
-- william.npc by patella-convert. The NPC is its [[npc]] entry in npcs.toml;
-- lib/npc.lua matches and runs the rules below, in the order of the file
-- (scripting.hh, "NPCs"). The comments are those of william.npc.
--
-- GIMUD - Graphical Interface Multi User Dungeon
-- william.npc: Datenbank für den carlin trunkenbold William

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
	{{ADDRESS, "hi$", FORCE}, {"Oh, hello! <hicks>"}},
	{{ADDRESS, "hello$", FORCE}, {previous}},
	{{ADDRESS, FORCE}, {idle}},
	{{BUSY, "hi$", FORCE}, {"<hicks> Not right now...", queue}},
	{{BUSY, "hello$", FORCE}, {previous}},
	{{BUSY, FORCE}, {nop}},
	{{VANISH, FORCE}, {"<hicks> Damn, I am starting to imagine things... again."}},

	{{"bye"}, {"Bye bye <hicks>.", idle}},
	{{"how", "are", "you"}, {"Thanks I am <hicks> drunk <giggles>."}},
	{{"sell"}, {"Hey! Thats my drink, buy your own!"}},
	{{"job"}, {"I forgot <hicks> what a job I have."}},
	{{"karl"}, {"A good guy with <hicks> good beer."}},
	{{"name"}, {"My Name? Uh... Wait! 'Bring down the trash, William you...' William, my name is William!"}},
	{{"time"}, {"Its precisely <hicks> after <hicks>."}},
	{{"help"}, {"I need another drink, then I'll help you. Promise."}},
	{{"carlin"}, {"I whish I'd live in Thais, the city of alcohol."}},
	{{"thais"}, {previous}},
	{{"sewer"}, {"The sewers are our last refuge."}},
	{{"refuge"}, {"Yes, refuge from <hicks> womanhood."}},
	{{"todd"}, {"In Todd we trust! TODD! TODD! TODD!"}},
}

function onCreatureSay(npc, creature, text, situation)
	behaviour.react(npc, rules, text, situation)
end

function onCreatureDisappear(npc, creature)
	behaviour.react(npc, rules, "", NPC_SITUATION_VANISH)
end
