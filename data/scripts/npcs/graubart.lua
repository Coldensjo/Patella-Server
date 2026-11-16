-- npcs/graubart.lua: the behaviour of the NPC Graubart, converted from
-- graubart.npc by patella-convert. The NPC is its [[npc]] entry in npcs.toml;
-- lib/npc.lua matches and runs the rules below, in the order of the file
-- (scripting.hh, "NPCs"). The comments are those of graubart.npc.
--
-- GIMUD - Graphical Interface Multi User Dungeon
-- graubart.npc : Kapitän Graubart von der Seahawk (Fields)

local behaviour = require("lib.npc")
local ADDRESS = behaviour.ADDRESS
local BUSY = behaviour.BUSY
local VANISH = behaviour.VANISH
local MALE = behaviour.MALE
local FEMALE = behaviour.FEMALE
local FORCE = behaviour.FORCE
local queue = behaviour.queue
local idle = behaviour.idle
local nop = behaviour.nop
local previous = behaviour.previous

local rules = behaviour.rules {
	{{ADDRESS, "hello$", MALE, FORCE}, {"Ahoi, young man %N. Looking for work on my ship?"}},
	{{ADDRESS, "hi$", MALE, FORCE}, {previous}},
	{{ADDRESS, "hello$", FEMALE, FORCE}, {"Ahoi, young lady %N. Looking for work on my ship?"}},
	{{ADDRESS, "hi$", FEMALE, FORCE}, {previous}},
	{{ADDRESS, FORCE}, {idle}},
	{{BUSY, "hello$", FORCE}, {"One moment please, %N.", queue}},
	{{BUSY, "hi$", FORCE}, {previous}},
	{{BUSY, FORCE}, {nop}},
	{{VANISH, FORCE}, {"Good bye and don't forget me!"}},

	{{"bye"}, {"Good bye and don't forget me!", idle}},
	{{"farewell"}, {previous}},
	{{"work"}, {"I'm sorry, but it is too dangerous nowadays. Too many storms out there. Come back in some months and we will see."}},
	{{"name"}, {"My name is Graubart, captain of the great Seahawk!"}},
	{{"job"}, {"I'm a merchant. I sail all over the world with my ship and trade with many different races!"}},
	{{"races"}, {"You know; elves, dwarfs, lizardmen, minotaurs and many others."}},
	{{"ship"}, {"Ah, my whole proud: My ship named Seahawk. We rode out so many stormy nights together. I think I couldn't live without it."}},
	{{"seahawk"}, {previous}},
	{{"trade"}, {"I trade nearly everything, for example weapons, food, water, and even magic runes."}},
	{{"merchant"}, {"A merchant is someone who trades goods with other people and tries to make a little profit. *laughs*"}},
	{{"weapons"}, {"Sorry, sold out."}},
	{{"food"}, {"Sorry, sold out. Ask Bruno."}},
	{{"water"}, {"Sorry, sold out."}},
	{{"magic", "runes"}, {"Sorry, sold out."}},
	{{"bruno"}, {"Bruno is one of the best sailors I know. He is nearly as good as me. *laughs loudly*"}},
	{{"aneus"}, {"Hmm, I don't know him very well. But he has a very nice story to tell."}},
	{{"marlene"}, {"Pssst. Marlene is not near right now...? You know... she is a lovely woman, but she talks too much! So I always try to keep distance from her because she can't stop talking."}},
}

function onCreatureSay(npc, creature, text, situation)
	behaviour.react(npc, rules, text, situation)
end

function onCreatureDisappear(npc, creature)
	behaviour.react(npc, rules, "", NPC_SITUATION_VANISH)
end
