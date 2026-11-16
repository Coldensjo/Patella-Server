-- npcs/hoggle.lua: the behaviour of the NPC Hoggle, converted from
-- hoggle.npc by patella-convert. The NPC is its [[npc]] entry in npcs.toml;
-- lib/npc.lua matches and runs the rules below, in the order of the file
-- (scripting.hh, "NPCs"). The comments are those of hoggle.npc.
--
-- GIMUD - Graphical Interface Multi User Dungeon
-- hoggle.npc: Datenbank fuer den Fischer Hoggle

local behaviour = require("lib.npc")
local ADDRESS = behaviour.ADDRESS
local BUSY = behaviour.BUSY
local VANISH = behaviour.VANISH
local FORCE = behaviour.FORCE
local idle = behaviour.idle
local nop = behaviour.nop
local previous = behaviour.previous

local rules = behaviour.rules {
	{{ADDRESS, "hello$", FORCE}, {"Welcome to my humble home!"}},
	{{ADDRESS, "hi$", FORCE}, {previous}},
	{{ADDRESS, FORCE}, {idle}},
	{{BUSY, "hello$", FORCE}, {"Can't you see I'm talking to someone?"}},
	{{BUSY, "hi$", FORCE}, {previous}},
	{{BUSY, FORCE}, {nop}},
	{{VANISH, FORCE}, {"Good bye."}},

	{{"bye"}, {"Good bye.", idle}},
	{{"farewell"}, {previous}},
	{{"job"}, {"I am just a poor fisherman."}},
	{{"name"}, {"I am Hoggle. I live in this house."}},
	{{"time"}, {"No, this is not the time to go fishing."}},
	{{"mountain"}, {"Yes, there is a mountain to the north, but it's of no interest. There isn't any fish on it."}},
	{{"food"}, {"If you are hungry you can go downstairs, perhaps you will find some fish. You can also take some shoes if you want."}},
	{{"map"}, {"If you go north-west you will find Lubo and his adventurer shop. I think he sells maps."}},
	{{"fisherman"}, {"It's a very hard job, cause without a boat I have to swim and fish at the same time!"}},
	{{"boat"}, {"My boat sunk. I thought it would be more aerodynamic with holes in it."}},
	{{"fish"}, {"I think they can talk, but they are wise enough to be silent. Once I saw a mermaid."}},
	{{"mermaid"}, {"I saw one! She had the body of a fish, and also the head of a fish. Amazing!"}},
	{{"stupid"}, {"My mom always said, stupid is who stupid does."}},
	{{"secret"}, {"Can you keep a secret? I think fish can't breath on land!"}},
	{{"thais"}, {"I know this city. Sometimes I sell fish to Frodo."}},
	{{"frodo"}, {"He buys my fish."}},
	{{"finger"}, {"No, fish don't have fingers."}},
	{{"pet", "name"}, {"Once there was a magician who named all his creatures like their species read backward."}},
	{{"carlin"}, {"There are stories about a city behind the mountain, but why should I go there? There is enough fish here."}},
}

function onCreatureSay(npc, creature, text, situation)
	behaviour.react(npc, rules, text, situation)
end

function onCreatureDisappear(npc, creature)
	behaviour.react(npc, rules, "", NPC_SITUATION_VANISH)
end
