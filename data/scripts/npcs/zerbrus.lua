-- npcs/zerbrus.lua: the behaviour of the NPC Zerbrus, converted from
-- zerbrus.npc by patella-convert. The NPC is its [[npc]] entry in npcs.toml;
-- lib/npc.lua matches and runs the rules below, in the order of the file
-- (scripting.hh, "NPCs"). The comments are those of zerbrus.npc.
--
-- GIMUD - Graphical Interface Multi User Dungeon
-- zerbrus.npc: Datenbank für den Dorfwächter Zerbrus (Rookgaard)

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
	{{ADDRESS, "hello$", FORCE}, {"Greetings young traveller."}},
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
	{{"sell"}, {"Ask the shopowners for their wares."}},
	{{"advice", lt(LEVEL, 4)}, {"Be careful out there and avoid the dungeons."}},
	{{"advice", gt(LEVEL, 3)}, {"Be careful out there."}},
	{{"job"}, {"I am the bridgeguard. I defend Rookgaard against the beasts of the wilderness and the dungeons!"}},
	{{"name"}, {"Zerbrus at your service."}},
	{{"time"}, {"My duty is eternal. Time is of no importance."}},
	{{"help"}, {"I have to stay here, sorry, but I can heal you if you are wounded."}},
	{{"monster"}, {"I will slay all monsters who dare to attack this little town."}},
	{{"dungeon"}, {"Dungeons are dangerous, be prepared."}},
	{{"wilderness"}, {"There are wolves, bears, snakes, deers, and spiders. You can find some dungeon entrances there, too."}},
	{{"sewer"}, {"In the sewers are crowded with rats. They make fine targets for young heroes."}},
	{{"god"}, {"I am a follower of Banor."}},
	{{"dallheim"}, {"He does a fine job."}},
	{{"banor"}, {"The heavenly warrior! Read books to learn about him."}},
	{{"king"}, {"HAIL TO THE KING!"}},
	{{"seymour"}, {"His job to teach the young heroes is important for our all survival."}},
	{{"willie"}, {"He can swear and curse as good as the rowdyest seaman I met."}},
	{{"amber"}, {"Shes verry attractive. To bad my duty leaves me no time to date her."}},
	{{"hyacinth"}, {"One of theese reclusive druids."}},
	{{"weapon"}, {"My weapon is property of the royal army. Find your own one."}},
	{{"magic"}, {"You will learn about magic soon enough."}},
	{{"tibia"}, {"In the world of tibia many challenges await the brave adventurers."}},
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
