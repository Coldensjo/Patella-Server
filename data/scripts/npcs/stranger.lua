-- npcs/stranger.lua: the behaviour of the NPC A Strange Fellow, converted from
-- stranger.npc by patella-convert. The NPC is its [[npc]] entry in npcs.toml;
-- lib/npc.lua matches and runs the rules below, in the order of the file
-- (scripting.hh, "NPCs"). The comments are those of stranger.npc.
--
-- GIMUD - Graphical Interface Multi User Dungeon
-- stranger.npc: Datenbank für den säumigen schuldner david brassacres

local behaviour = require("lib.npc")
local ADDRESS = behaviour.ADDRESS
local BUSY = behaviour.BUSY
local VANISH = behaviour.VANISH
local FORCE = behaviour.FORCE
local eq = behaviour.eq
local lt = behaviour.lt
local gt = behaviour.gt
local ge = behaviour.ge
local TOPIC = behaviour.TOPIC
local AMOUNT = behaviour.AMOUNT
local TYPE = behaviour.TYPE
local count = behaviour.count
local questValue = behaviour.questValue
local setTopic = behaviour.setTopic
local setAmount = behaviour.setAmount
local setType = behaviour.setType
local setQuestValue = behaviour.setQuestValue
local summon = behaviour.summon
local delete = behaviour.delete
local idle = behaviour.idle
local nop = behaviour.nop
local previous = behaviour.previous

local rules = behaviour.rules {
	{{ADDRESS, "hello$", FORCE}, {"Uh? What do you want?!"}},
	{{ADDRESS, "hi$", FORCE}, {previous}},

	{{ADDRESS, FORCE}, {idle}},
	{{BUSY, "hello$", FORCE}, {"Just wait a minute!"}},
	{{BUSY, "hi$", FORCE}, {previous}},
	{{BUSY, FORCE}, {nop}},
	{{VANISH, FORCE}, {"Still I am better in vanishing!"}},

	{{"bye"}, {"Good riddance.", idle}},
	{{"farewell"}, {previous}},

	{{"name"}, {"My name is not of your concern."}},
	{{"job"}, {"That's only my business, not yours."}},
	{{gt(questValue(229), 1), "david"}, {"Yes, yes... Its me .. you exposed me! Stop nagging me with that."}},
	{{gt(questValue(229), 1), "brassacres"}, {previous}},
	{{"david"}, {"I never heard that name and now get lost."}},
	{{"brassacres"}, {previous}},

	{{eq(questValue(229), 2), "bill"}, {setType(3216), setAmount(1), "A bill? Oh boy so you are delivering another bill to poor me?", setTopic(5)}},

	{{eq(TOPIC, 5), "yes", ge(count(TYPE), AMOUNT)}, {"Ok, ok, I'll take it. I guess I have no other choice anyways. And now leave me alone in my misery please.", delete(TYPE), setQuestValue(229, 3)}},
	{{eq(TOPIC, 5), "yes", lt(count(TYPE), AMOUNT)}, {"Ha Ha! You have none!! Naanaanaanaaanaaaa!", idle}},
	{{eq(TOPIC, 5)}, {"Hoooraaaay! Uhm... I mean, thats fine..."}},

	{{"bill"}, {"Thats not my concern, you are probably looking for someone else and now get lost!", idle}},

	{{eq(TOPIC, 1), "hat"}, {"Stop bugging me about that hat, do you listen?", setTopic(2)}},

	{{eq(TOPIC, 2), "hat"}, {"Hey! Don't touch that hat! Leave it alone!!! Don't do this!!!!", setTopic(3)}},
	{{eq(TOPIC, 3), "hat"}, {"Noooooo! Argh, ok, ok, I guess I can't deny it anymore, I am David Brassacres, the magnificent, so what do you want?", summon(74), summon(74), summon(74), summon(74), setQuestValue(229, 2)}},
	{{eq(questValue(229), 1), "hat"}, {"What? My hat?? Theres... nothing special about it!", setTopic(1)}},

	{{"hat"}, {"Get lost!", idle}},
}

function onCreatureSay(npc, creature, text, situation)
	behaviour.react(npc, rules, text, situation)
end

function onCreatureDisappear(npc, creature)
	behaviour.react(npc, rules, "", NPC_SITUATION_VANISH)
end
