-- npcs/penny.lua: the behaviour of the NPC Penny, converted from
-- penny.npc by patella-convert. The NPC is its [[npc]] entry in npcs.toml;
-- lib/npc.lua matches and runs the rules below, in the order of the file
-- (scripting.hh, "NPCs"). The comments are those of penny.npc.
--
-- GIMUD - Graphical Interface Multi User Dungeon
-- penny.npc: Datenbank für die GM-Gehilfin Penny

local behaviour = require("lib.npc")
local ADDRESS = behaviour.ADDRESS
local BUSY = behaviour.BUSY
local VANISH = behaviour.VANISH
local MALE = behaviour.MALE
local FEMALE = behaviour.FEMALE
local FORCE = behaviour.FORCE
local create = behaviour.create
local queue = behaviour.queue
local idle = behaviour.idle
local nop = behaviour.nop
local previous = behaviour.previous

local rules = behaviour.rules {
	{{ADDRESS, "hello$", MALE, FORCE}, {"Welcome home, Sir %N."}},
	{{ADDRESS, "hi$", MALE, FORCE}, {previous}},
	{{ADDRESS, "hello$", FEMALE, FORCE}, {"Welcome home, Lady %N."}},
	{{ADDRESS, "hi$", FEMALE, FORCE}, {previous}},
	{{ADDRESS, FORCE}, {idle}},
	{{BUSY, "hi$", MALE, FORCE}, {"Just a minute, Sir %N.", queue}},
	{{BUSY, "hello$", MALE, FORCE}, {previous}},
	{{BUSY, "hi$", FEMALE, FORCE}, {"Just a minute, Lady %N.", queue}},
	{{BUSY, "hello$", FEMALE, FORCE}, {previous}},
	{{BUSY, FORCE}, {nop}},
	{{VANISH, FORCE}, {"May Justice be with you!"}},

	{{"bye"}, {"May Justice be with you!", idle}},
	{{"farewell"}, {previous}},
	{{"name"}, {"I am miss Penny, your secretary."}},
	{{"job"}, {"I'm your secretary. I'm organizing all those criminal records and your mail."}},
	{{"criminal"}, {"<Sigh> It's an evil world, isn't it?"}},
	{{"record"}, {previous}},
	{{"mail"}, {"You can get a letter from me."}},
	{{"letter"}, {"Here you are.", create(3505)}},
}

function onCreatureSay(npc, creature, text, situation)
	behaviour.react(npc, rules, text, situation)
end

function onCreatureDisappear(npc, creature)
	behaviour.react(npc, rules, "", NPC_SITUATION_VANISH)
end
