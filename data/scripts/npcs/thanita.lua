-- npcs/thanita.lua: the behaviour of the NPC Thanita, converted from
-- thanita.npc by patella-convert. The NPC is its [[npc]] entry in npcs.toml;
-- lib/npc.lua matches and runs the rules below, in the order of the file
-- (scripting.hh, "NPCs"). The comments are those of thanita.npc.
--
-- GIMUD - Graphical Interface Multi User Dungeon
-- Thanita.npc: Datenbank für die Amazonenwächterin des Turms

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
	{{ADDRESS, "hi$", FORCE}, {"How could you sneak up on me like this? I thought you were one of THEM! Well, since you are not, what brings you to this wilderness?"}},
	{{ADDRESS, "hello$", FORCE}, {previous}},
	{{ADDRESS, FORCE}, {idle}},
	{{BUSY, "hi$", FORCE}, {"Please wait.", queue}},
	{{BUSY, "hello$", FORCE}, {previous}},
	{{BUSY, FORCE}, {nop}},
	{{VANISH, FORCE}, {"Take good care of yourself traveler!"}},

	{{"bye"}, {"Take good care of yourself traveler. Would be a shame to lose such a courageous wanderer to those green monsters.", idle}},
	{{"farewell"}, {previous}},
	{{"job"}, {"I'm an amazon guard. It's my job to keep my eyes open and to keep enemies from passing by. My job here truely is one of the toughest. All because of these nerve-racking beasts."}},
	{{"name"}, {"My name is Thanita. Nice to meet you."}},

	{{"beasts"}, {"These green, orcish raiders come in masses. Hundreds of them. They are worse than those goblins I have to deal with from time to time. ...", "Some of them come on beastly warwolves, others shoot fireballs at you and some are just plain ugly. ...", "They have never succeeded though in capturing this tower. So far I have always been able to put them to flight."}},
	{{"orcs"}, {previous}},
	{{"them"}, {previous}},
	{{"raid"}, {previous}},

	{{"mission"}, {"Well, I cannot provide you with a mission, I have a mission to fulfill myself. ...", "However, when the orcs attack, you are more than welcome to help me to defeat them. ...", "I'll even let you have all the stuff they carry with them. Even if they looted it from the tower here."}},
	{{"quest"}, {previous}},

	{{"fight"}, {"To get rid of them, you need to be quite good in different martial arts."}},
	{{"defeat"}, {previous}},

	{{"femor", "hills"}, {"Are you kidding me? This is femor hills. Probably one of the roughest areas in Tibia. There is nobody else could do my job."}},

	{{"tower"}, {"This is a watchtower of the city of carlin. From here I can see pretty much all of the surrounding lands. Hardly anybody can startle me up here. I see all enemies long before they can see me."}},

	{{"enemies"}, {"The enemies I fear most here are these nasty orcs."}},

	{{"amazon"}, {"I see you have heard of amazons before. Well let me tell you, probably everything you heard is true. We are much stronger and tougher than people think. Also, we know how to fight and could teach many men how to handle a weapon. ...", "Not that we would do such a foolish thing."}},

	{{"bye"}, {"Take good care of yourself traveler. Would be a shame to lose such a courageous wanderer to those green monsters."}},
}

function onCreatureSay(npc, creature, text, situation)
	behaviour.react(npc, rules, text, situation)
end

function onCreatureDisappear(npc, creature)
	behaviour.react(npc, rules, "", NPC_SITUATION_VANISH)
end
