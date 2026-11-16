-- npcs/albert.lua: the behaviour of the NPC Albert, converted from
-- albert.npc by patella-convert. The NPC is its [[npc]] entry in npcs.toml;
-- lib/npc.lua matches and runs the rules below, in the order of the file
-- (scripting.hh, "NPCs"). The comments are those of albert.npc.
--
-- GIMUD - Graphical Interface Multi User Dungeon
-- albert.npc: Datenbank für den Heiler Albert

local behaviour = require("lib.npc")
local ADDRESS = behaviour.ADDRESS
local BUSY = behaviour.BUSY
local VANISH = behaviour.VANISH
local FORCE = behaviour.FORCE
local lt = behaviour.lt
local gt = behaviour.gt
local HP = behaviour.HP
local BURNING = behaviour.BURNING
local POISON = behaviour.POISON
local setHp = behaviour.setHp
local burning = behaviour.burning
local poison = behaviour.poison
local effectOpp = behaviour.effectOpp
local queue = behaviour.queue
local idle = behaviour.idle
local nop = behaviour.nop
local previous = behaviour.previous

local rules = behaviour.rules {
	{{ADDRESS, "hello$", FORCE}, {"Welcome in my humble hut, %N."}},
	{{ADDRESS, "hi$", FORCE}, {previous}},
	{{ADDRESS, FORCE}, {idle}},
	{{BUSY, "hello$", FORCE}, {"Please wait a moment, %N.", queue}},
	{{BUSY, "hi$", FORCE}, {previous}},
	{{BUSY, "heal$", gt(BURNING, 0)}, {"You are burning. I will help you.", burning(0, 0), effectOpp(15)}},
	{{BUSY, "heal$", gt(POISON, 0)}, {"You are poisoned. I will help you.", poison(0, 0), effectOpp(14)}},
	{{BUSY, "heal$", lt(HP, 40)}, {"You are looking really bad. Let me heal your wounds.", setHp(40), effectOpp(13)}},
	{{BUSY, "heal$"}, {"You aren't looking really bad. Sorry, I can't help you."}},
	{{BUSY, FORCE}, {nop}},
	{{VANISH, FORCE}, {"If you are heavily wounded or poisoned, feel free to return for a healing."}},

	{{"bye"}, {"Good bye, %N!", idle}},
	{{"job"}, {"I am a healer."}},
	{{"name"}, {"My Name is Albert Fibulanian."}},
	{{"tibia"}, {"Tibia is a world big enough for everyone. I wish people would realize that."}},
	{{"thais"}, {"The sinful city of Thais is a monument of corruption and murder. I am glad I left for Edron and thank the gods every day for this isle."}},
	{{"edron"}, {previous}},
	{{"god"}, {"The gods of good take care of us."}},
	{{"king"}, {"The king does much to enhance the life of his people, but he could do more."}},
	{{"tibianus"}, {previous}},
	{{"army"}, {"I dream of times which see no need for armies or warriors."}},
	{{"banor"}, {previous}},
	{{"ferumbras"}, {"The fallen one is a perfect example where evil leads us to."}},
	{{"excalibug"}, {"It's only another instrument of pain and destruction."}},
	{{"news"}, {"I have only news about weather, taxes, and harvests. I heared nothing that might interest a traveller like you."}},
	{{"daniel"}, {"I healed his wounds, but nothing can heal his soul after the betrayal of some of his knightly brethren."}},
	{{"kaine"}, {"Another victim of his own ambitions. I mourn for his soul."}},

	{{"heal$", gt(BURNING, 0)}, {"You are burning. I will help you.", burning(0, 0), effectOpp(15)}},
	{{"heal$", gt(POISON, 0)}, {"You are poisoned. I will help you.", poison(0, 0), effectOpp(14)}},
	{{"heal$", lt(HP, 40)}, {"You are looking really bad. Let me heal your wounds.", setHp(40), effectOpp(13)}},
	{{"heal$"}, {"You aren't looking really bad. Sorry, I can't help you."}},
	{{"time"}, {"Now, it is %T."}},
}

function onCreatureSay(npc, creature, text, situation)
	behaviour.react(npc, rules, text, situation)
end

function onCreatureDisappear(npc, creature)
	behaviour.react(npc, rules, "", NPC_SITUATION_VANISH)
end
