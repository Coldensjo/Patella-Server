-- npcs/tulf.lua: the behaviour of the NPC Tulf, converted from
-- tulf.npc by patella-convert. The NPC is its [[npc]] entry in npcs.toml;
-- lib/npc.lua matches and runs the rules below, in the order of the file
-- (scripting.hh, "NPCs"). The comments are those of tulf.npc.
--
-- GIMUD - Graphical Interface Multi User Dungeon
-- tulf.npc: Datenbank für den Wachoffizier Tulf 

local behaviour = require("lib.npc")
local ADDRESS = behaviour.ADDRESS
local BUSY = behaviour.BUSY
local VANISH = behaviour.VANISH
local FORCE = behaviour.FORCE
local idle = behaviour.idle
local nop = behaviour.nop
local previous = behaviour.previous

local rules = behaviour.rules {
	{{ADDRESS, "hello$", FORCE}, {"Hiho! <mumbles>"}},
	{{ADDRESS, "hi$", FORCE}, {previous}},
	{{ADDRESS, "hiho$", FORCE}, {previous}},
	{{ADDRESS, FORCE}, {idle}},
	{{BUSY, "hello$", FORCE}, {"Silence youngster!"}},
	{{BUSY, "hi$", FORCE}, {previous}},
	{{BUSY, "hiho$", FORCE}, {previous}},
	{{BUSY, FORCE}, {nop}},
	{{VANISH, FORCE}, {"Yeah, bye."}},

	{{"bye"}, {"Yeah, bye.", idle}},
	{{"job"}, {"I am the captain of the emperor's personal bodyguards."}},
	{{"shop"}, {previous}},
	{{"name"}, {"My name is Tulf Beardweaver, son of Earth, from the Dragoneaters."}},
	{{"time"}, {"It's %T right now."}},
	{{"help"}, {"Sorry, I am on duty!"}},
	{{"dwarfs"}, {"If you go to a dwarfs' city, do as the dwarfs do."}},
	{{"monster"}, {"I doubt anyone can make it through our lines of defense."}},
	{{"dungeon"}, {"I despise the habit to challenge the prisoners of Dwarcatra or provoke the boys in the mines."}},
	{{"mines"}, {"The mines aren't meant for foreigners. The miners there have enough troubles."}},
	{{"bodyguard"}, {"We keep up law and order here, though the boys would rather need some practice like taking care for the trouble in the mines."}},
	{{"trouble"}, {"The mines are raided again and again by the bandits of the Horned Fox."}},
	{{"horned", "fox"}, {"It's a renegade minotaur who has a hidden lair somewhere near our mines."}},
	{{"lair"}, {"The lair of the Horned Fox is surely well guarded and even better hidden."}},
}

function onCreatureSay(npc, creature, text, situation)
	behaviour.react(npc, rules, text, situation)
end

function onCreatureDisappear(npc, creature)
	behaviour.react(npc, rules, "", NPC_SITUATION_VANISH)
end
