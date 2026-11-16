-- npcs/phillip.lua: the behaviour of the NPC Phillip, converted from
-- phillip.npc by patella-convert. The NPC is its [[npc]] entry in npcs.toml;
-- lib/npc.lua matches and runs the rules below, in the order of the file
-- (scripting.hh, "NPCs"). The comments are those of phillip.npc.
--
-- GIMUD - Graphical Interface Multi User Dungeon
-- phillip.npc: Datenbank für den Lehrer Phillip

local behaviour = require("lib.npc")
local ADDRESS = behaviour.ADDRESS
local BUSY = behaviour.BUSY
local VANISH = behaviour.VANISH
local MALE = behaviour.MALE
local FEMALE = behaviour.FEMALE
local FORCE = behaviour.FORCE
local lt = behaviour.lt
local LEVEL = behaviour.LEVEL
local queue = behaviour.queue
local idle = behaviour.idle
local nop = behaviour.nop
local previous = behaviour.previous

local rules = behaviour.rules {
	{{ADDRESS, "hello$", lt(LEVEL, 4), FORCE}, {"Hello, pupil %N. I hope I can help you on your quest for knowledge."}},
	{{ADDRESS, "hi$", lt(LEVEL, 4), FORCE}, {previous}},
	{{ADDRESS, "hello$", lt(LEVEL, 15), FORCE}, {"Hello, seeker of knowledge %N. How may I assist you?"}},
	{{ADDRESS, "hi$", lt(LEVEL, 15), FORCE}, {previous}},
	{{ADDRESS, "hello$", lt(LEVEL, 25), FORCE}, {"Hello, mighty adventurer %N. Can I teach you something you don't know?"}},
	{{ADDRESS, "hi$", lt(LEVEL, 25), FORCE}, {previous}},
	{{ADDRESS, "hello$", FORCE}, {"Hello, famous %N. It should be you teaching me!"}},
	{{ADDRESS, "hi$", FORCE}, {previous}},
	{{ADDRESS, FORCE}, {idle}},
	{{BUSY, "hi$", FORCE}, {"Patience, %N. Listen to my words and learn.", queue}},
	{{BUSY, "hello$", FORCE}, {previous}},
	{{BUSY, FORCE}, {nop}},
	{{VANISH, FORCE}, {"Did the bell ring?"}},

	{{"bye"}, {"Go and be careful. Remember what you have learned!", idle}},
	{{"farewell"}, {previous}},
	{{"how", "are", "you"}, {"I am fine, thank you very much."}},
	{{"sell"}, {"My business is knowlegde and it is for free."}},
	{{"job"}, {"I am honored to be teacher in this school."}},
	{{"teacher"}, {"I run this school, there are other travelling teachers who we call Loremasters."}},
	{{"loremaster"}, {"If you are lucky you'll meet one in your journeys."}},
	{{"name"}, {"My name is Phillip."}},
	{{"time"}, {"It is %T."}},
	{{"help"}, {"I will provide you with all knowledge I have."}},
	{{"monster"}, {"Monsters come in different shape and power. It's said there is a zoo in the dwarfs' town."}},
	{{"dungeon"}, {"Dungeons are places of danger and puzzles. In some of them a bright mind will serve you more then a blade."}},
	{{"sewer", FEMALE}, {"The sewers of Carlin are a disgusting place. Better never crawl around in these stinking tunnels."}},
	{{"sewer", MALE}, {"An interesting place you should consider to visit."}},
	{{"thank", "you"}, {"You don't have to thank me, it's only my duty."}},
	{{"god"}, {"To learn about gods, visit the temples and talk to the priests."}},
	{{"king"}, {"The southern king is called Tibianus. He and our queen Eloise are in a constant struggle."}},
	{{"queen"}, {previous}},
	{{"rumour"}, {"I don't like rumours."}},
	{{"gossip"}, {previous}},
	{{"news"}, {previous}},
	{{"weapon"}, {"To learn about weapons read appropriate books or talk to the smiths."}},
	{{"magic"}, {"To learn about magic talk to the guild leaders."}},
	{{"rebellion"}, {"Rebellion? What for? We are contend with our situation."}},
	{{"in", "tod", "we", "trust"}, {"Tod will come and save us all. He will bring freedom and beer to the men of Carlin."}},
	{{"lugri"}, {"This servant of evil is protected by the dark gods and can't be harmed."}},
	{{"ferumbras"}, {"He is a follower of evil. His powers were boosted by a sinister force and he is beyond human restrictions now."}},
	{{"excalibug"}, {"This weapon is said to be very powerful and unique. It was hidden in ancient times and now is thought to be lost."}},
}

function onCreatureSay(npc, creature, text, situation)
	behaviour.react(npc, rules, text, situation)
end

function onCreatureDisappear(npc, creature)
	behaviour.react(npc, rules, "", NPC_SITUATION_VANISH)
end
