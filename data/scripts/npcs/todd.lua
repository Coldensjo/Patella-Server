-- npcs/todd.lua: the behaviour of the NPC Todd, converted from
-- todd.npc by patella-convert. The NPC is its [[npc]] entry in npcs.toml;
-- lib/npc.lua matches and runs the rules below, in the order of the file
-- (scripting.hh, "NPCs"). The comments are those of todd.npc.
--
-- GIMUD - Graphical Interface Multi User Dungeon
-- todd.npc: Datenbank für den betrügerischen schmuggler todd

local behaviour = require("lib.npc")
local ADDRESS = behaviour.ADDRESS
local BUSY = behaviour.BUSY
local VANISH = behaviour.VANISH
local FORCE = behaviour.FORCE
local queue = behaviour.queue
local idle = behaviour.idle
local nop = behaviour.nop
local previous = behaviour.previous

local rules = behaviour.rules {
	{{ADDRESS, "hi$", FORCE}, {"Uhm oh hello... not so loud please... my head..."}},
	{{ADDRESS, "hello$", FORCE}, {previous}},
	{{ADDRESS, FORCE}, {idle}},
	{{BUSY, "hi$", FORCE}, {"Please wait... in silence if possible <uuuhh>.", queue}},
	{{BUSY, "hello$", FORCE}, {previous}},
	{{BUSY, FORCE}, {nop}},
	{{VANISH, FORCE}, {"Silence at last."}},

	{{"bye"}, {"Yes, goodbye, just leave me alone.", idle}},

	{{"how", "are", "you"}, {"Oh, this headache, one of the beers frodo served me must have been foul."}},
	{{"job"}, {"I am... a traveller."}},
	{{"karl"}, {"Uhm, never heared about him... and you can't proof otherwise.", idle}},
	{{"name"}, {"My Name? I am To... ahm... hum... My name is Hugo."}},
	{{"Hugo"}, {"Yes, thats my name of course."}},
	{{"smuggler"}, {"I am a honest person and don't like to be insulted!", idle}},
	{{"carlin"}, {"I never was there. Now leave me alone.", idle}},
	{{"resistance"}, {"Resistance is futile... uhm... I wonder where I picked that saying up. Oh my head..."}},
	{{"head"}, {"Uhhh Ohhhh one of the beers yesterday must have been bad."}},

	{{"thais"}, {"I love that city."}},
	{{"william"}, {"Thats a common name, perhaps I met a William, not sure about that."}},
	{{"money"}, {"I don't know anything about money, missing or not."}},
	{{"todd"}, {"Uh .. I... I met a Todd on the road. He told me he was traveling to Venore, look there for your Todd."}},
}

function onCreatureSay(npc, creature, text, situation)
	behaviour.react(npc, rules, text, situation)
end

function onCreatureDisappear(npc, creature)
	behaviour.react(npc, rules, "", NPC_SITUATION_VANISH)
end
