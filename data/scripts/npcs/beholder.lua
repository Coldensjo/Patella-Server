-- npcs/beholder.lua: the behaviour of the NPC A Wrinkled Beholder, converted from
-- beholder.npc by patella-convert. The NPC is its [[npc]] entry in npcs.toml;
-- lib/npc.lua matches and runs the rules below, in the order of the file
-- (scripting.hh, "NPCs"). The comments are those of beholder.npc.
--
-- GIMUD - Graphical Interface Multi User Dungeon
-- beholder.npc: Datenbank für den Bibliotheksbeholder (Elfenstadt)

local behaviour = require("lib.npc")
local ADDRESS = behaviour.ADDRESS
local BUSY = behaviour.BUSY
local VANISH = behaviour.VANISH
local FORCE = behaviour.FORCE
local burning = behaviour.burning
local effectMe = behaviour.effectMe
local effectOpp = behaviour.effectOpp
local queue = behaviour.queue
local idle = behaviour.idle
local nop = behaviour.nop
local previous = behaviour.previous

local rules = behaviour.rules {
	{{ADDRESS, "hello$", FORCE}, {"What is this? An optically challenged entity called %N. How fascinating!"}},
	{{ADDRESS, "hi$", FORCE}, {previous}},
	{{ADDRESS, FORCE}, {idle}},
	{{BUSY, "hello$", FORCE}, {"Wait. I will eat you later, %N.", queue}},
	{{BUSY, "hi$", FORCE}, {previous}},
	{{BUSY, FORCE}, {nop}},
	{{VANISH, FORCE}, {"Strange entity. I will record this encounter."}},

	{{"bye"}, {"Wait right there. I will eat you after writing down what I found out.", idle}},
	{{"farewell"}, {previous}},
	{{"job"}, {"I am the great librarian."}},
	{{"name"}, {"I am 486486 and NOT 'Blinky' as some people called me ... before they died."}},
	{{"blinky"}, {"How interesting you are that stupid. Let me apply this on you and see how long you last", burning(10, 25), effectOpp(5), effectMe(8)}},
	{{"tibia"}, {"It's 1, not 'Tibia', silly."}},
	{{"ab'dendriel"}, {"I heard that elves moved in upstairs."}},
	{{"elves"}, {"These fools and their superstitious life cult don't understand anything of importance."}},
	{{"humans"}, {"Good tools to work with ... After their death, that is."}},
	{{"orcs"}, {"Noisy pests."}},
	{{"minotaurs"}, {"Their mages are so close to the truth. Closer then they know and closer then it's good for them."}},
	{{"god"}, {"They will mourn the day they abandoned us."}},
	{{"death"}, {"Yes, yes, I will kill you soon enough, now let me continue my investigation on you."}},
	{{"numbers"}, {"Numbers are essential. They are the secret behind the scenes. If you are a master of mathematics you are a master over life and death."}},
	{{"library"}, {"It's a fine library, isn't it?"}},
	{{"books"}, {"Our books are written in 469, of course you can't understand them."}},
	{{"469"}, {"The language of my kind. Superior to any other language and only to be spoken by entities with enough eyes to blink it."}},
	{{"cyclops"}, {"Uglyness incarnate. One eye! Imagine that! Horrible!"}},
	{{"excalibug"}, {"Only inferior species need weapons."}},
}

function onCreatureSay(npc, creature, text, situation)
	behaviour.react(npc, rules, text, situation)
end

function onCreatureDisappear(npc, creature)
	behaviour.react(npc, rules, "", NPC_SITUATION_VANISH)
end
