-- npcs/dallheim.lua: the behaviour of the NPC Dallheim, converted from
-- dallheim.npc by patella-convert. The NPC is its [[npc]] entry in npcs.toml;
-- lib/npc.lua matches and runs the rules below, in the order of the file
-- (scripting.hh, "NPCs"). The comments are those of dallheim.npc.
--
-- GIMUD - Graphical Interface Multi User Dungeon
-- dallheim.npc: Datenbank für den Dorfwächter Dallheim (Rookgaard)

local behaviour = require("lib.npc")
local ADDRESS = behaviour.ADDRESS
local BUSY = behaviour.BUSY
local VANISH = behaviour.VANISH
local FORCE = behaviour.FORCE
local lt = behaviour.lt
local gt = behaviour.gt
local LEVEL = behaviour.LEVEL
local HP = behaviour.HP
local BURNING = behaviour.BURNING
local POISON = behaviour.POISON
local setHp = behaviour.setHp
local burning = behaviour.burning
local poison = behaviour.poison
local effectOpp = behaviour.effectOpp
local idle = behaviour.idle
local nop = behaviour.nop
local previous = behaviour.previous

local rules = behaviour.rules {
	{{ADDRESS, "hello$", FORCE}, {"<nods silently>"}},
	{{ADDRESS, "hi$", FORCE}, {previous}},
	{{ADDRESS, FORCE}, {idle}},
	{{BUSY, "hello$", FORCE}, {"Not now."}},
	{{BUSY, "hi$", FORCE}, {previous}},
	{{BUSY, "heal$", gt(BURNING, 0)}, {"You are burning. I will help you.", burning(0, 0), effectOpp(15)}},
	{{BUSY, "heal$", gt(POISON, 0)}, {"You are poisoned. I will help you.", poison(0, 0), effectOpp(14)}},
	{{BUSY, "heal$", lt(HP, 65)}, {"You are looking really bad. Let me heal your wounds.", setHp(65), effectOpp(13)}},
	{{BUSY, "heal$"}, {"You aren't looking really bad. Sorry, I can't help you."}},
	{{BUSY, FORCE}, {nop}},
	{{VANISH, FORCE}, {"Hm."}},

	{{"bye"}, {"Bye.", idle}},
	{{"farewell"}, {previous}},
	{{"how", "are", "you"}, {"Fine."}},
	{{"sell"}, {"I sell nothing."}},
	{{"advice", lt(LEVEL, 4)}, {"Be careful out there and avoid the dungeons."}},
	{{"advice", gt(LEVEL, 3)}, {"Be careful out there."}},
	{{"job"}, {"I am the bridgeguard. I defend Rookgaard against the beasts from the wilderness and the dungeons!"}},
	{{"name"}, {"Dallheim."}},
	{{"time"}, {"No idea."}},
	{{"help"}, {"I have to stay here, sorry, but I can heal you if you are wounded."}},
	{{"monster"}, {"I will crush all monsters who dare to attack our base."}},
	{{"dungeon"}, {"Dungeons are dangerous, be prepared."}},
	{{"wilderness"}, {"There are wolves, bears, snakes, deers, and spiders. You can find some dungeon entrances there, too."}},
	{{"sewer"}, {"In the sewers there are some rats, fine targets for young heroes."}},
	{{"god"}, {"I am a follower of Banor."}},
	{{"banor"}, {"The great one! Read books to learn about him."}},
	{{"king"}, {"HAIL TO THE KING!"}},
	{{"seymour"}, {"Leave me alone with this whimp."}},
	{{"willie"}, {"A fine cook and farmer he is."}},
	{{"amber"}, {"I don't trust her."}},
	{{"hyacinth"}, {"Strange Fellow, hides somewhere in the mountains of the isle."}},
	{{"weapon"}, {"With my spikesword I slice even a cyclops in pieces."}},
	{{"magic"}, {"Not interested in such party tricks."}},
	{{"tibia"}, {"A nice place for a hero, but nothing for whelps."}},
	{{"heal$", gt(BURNING, 0)}, {"You are burning. I will help you.", burning(0, 0), effectOpp(15)}},
	{{"heal$", gt(POISON, 0)}, {"You are poisoned. I will help you.", poison(0, 0), effectOpp(14)}},
	{{"heal$", lt(HP, 65)}, {"You are looking really bad. Let me heal your wounds.", setHp(65), effectOpp(13)}},
	{{"heal$"}, {"You aren't looking really bad. Sorry, I can't help you."}},
}

function onCreatureSay(npc, creature, text, situation)
	behaviour.react(npc, rules, text, situation)
end

function onCreatureDisappear(npc, creature)
	behaviour.react(npc, rules, "", NPC_SITUATION_VANISH)
end
