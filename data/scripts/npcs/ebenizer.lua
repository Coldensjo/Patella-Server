-- npcs/ebenizer.lua: the behaviour of the NPC Ebenizer, converted from
-- ebenizer.npc by patella-convert. The NPC is its [[npc]] entry in npcs.toml;
-- lib/npc.lua matches and runs the rules below, in the order of the file
-- (scripting.hh, "NPCs"). The comments are those of ebenizer.npc.
--
-- GIMUD - Graphical Interface Multi User Dungeon
-- ebenizer.npc: Datenbank für den Bankier Ebenizer

local behaviour = require("lib.npc")
local genBank = require("lib.npcs.gen-bank")
local ADDRESS = behaviour.ADDRESS
local BUSY = behaviour.BUSY
local VANISH = behaviour.VANISH
local FORCE = behaviour.FORCE
local queue = behaviour.queue
local idle = behaviour.idle
local nop = behaviour.nop
local previous = behaviour.previous

local rules = behaviour.rules {
	{{ADDRESS, "hello$", FORCE}, {"Yes? What may I do for you, %N?"}},
	{{ADDRESS, "hi$", FORCE}, {previous}},
	{{ADDRESS, FORCE}, {idle}},
	{{BUSY, "hello$", FORCE}, {"It's not your turn %N. Wait please.", queue}},
	{{BUSY, "hi$", FORCE}, {previous}},
	{{BUSY, FORCE}, {nop}},
	{{VANISH, FORCE}, {"Have a nice day."}},

	{{"bye"}, {"Have a nice day.", idle}},
	{{"farewell"}, {previous}},
	{{"name"}, {"My name? Ebenizer!"}},
	{{"job"}, {"I am running this Bank"}},
	{{"time"}, {"It is %T, precisely."}},
	{{"king"}, {"Hail to the king!"}},
	{{"tibianus"}, {previous}},
	{{"army"}, {"Soldiers have not that much money that I would care about."}},
	{{"ferumbras"}, {"A true threat to wealth and trade."}},
	{{"excalibug"}, {"This weapon, if real, might be worth a lot."}},
	{{"thais"}, {"We are in constant contact with the city of Thais."}},
	{{"tibia"}, {"There are countless ways of profit in this world."}},
	{{"carlin"}, {"It's underdeveloped and economically insignificant."}},
	{{"edron"}, {"The riches of our isle are its mineral resources."}},
	{{"news"}, {"I only care about financial news."}},
	{{"rumors"}, {previous}},

	genBank,
}

function onCreatureSay(npc, creature, text, situation)
	behaviour.react(npc, rules, text, situation)
end

function onCreatureDisappear(npc, creature)
	behaviour.react(npc, rules, "", NPC_SITUATION_VANISH)
end
