-- npcs/bansheequeen.lua: the behaviour of the NPC The Queen of the Banshee, converted from
-- bansheequeen.npc by patella-convert. The NPC is its [[npc]] entry in npcs.toml;
-- lib/npc.lua matches and runs the rules below, in the order of the file
-- (scripting.hh, "NPCs"). The comments are those of bansheequeen.npc.
--
-- GIMUD - Graphical Interface Multi User Dungeon
-- bansheequeen.npc: Datenbank für die Bansheequeen

local behaviour = require("lib.npc")
local ADDRESS = behaviour.ADDRESS
local BUSY = behaviour.BUSY
local VANISH = behaviour.VANISH
local PZBLOCK = behaviour.PZBLOCK
local FORCE = behaviour.FORCE
local eq = behaviour.eq
local lt = behaviour.lt
local gt = behaviour.gt
local TOPIC = behaviour.TOPIC
local LEVEL = behaviour.LEVEL
local questValue = behaviour.questValue
local add = behaviour.add
local setTopic = behaviour.setTopic
local setQuestValue = behaviour.setQuestValue
local effectOpp = behaviour.effectOpp
local idle = behaviour.idle
local nop = behaviour.nop
local teleport = behaviour.teleport
local previous = behaviour.previous

local rules = behaviour.rules {
	{{ADDRESS, "hello$", FORCE}, {"Be greeted, dear visitor. Come and stay ... a while."}},
	{{ADDRESS, "hi$", FORCE}, {previous}},
	{{ADDRESS, FORCE}, {idle}},
	{{BUSY, "hello$", FORCE}, {"Wait as patiently as death is waiting for you!"}},
	{{BUSY, "hi$", FORCE}, {previous}},
	{{BUSY, FORCE}, {nop}},
	{{VANISH, FORCE}, {"Yes, flee from death. But know it shall be always one step behind you."}},

	{{"bye"}, {"We will meet again.", idle}},
	{{"farewell"}, {previous}},

	{{"name"}, {"It hurts me to even think about my mortal past. Its long lost and forgotten. So don't ask me about it!"}},
	{{"job"}, {"It is my curse to be the eternal guardian of this ancient place."}},
	{{"place"}, {"It served as a temple, a source of power and ... as a sender for an ancient race in time long gone by and forgotten."}},
	{{"race"}, {"The race that built this edifice came to this place from the stars. They ran from an enemy even more horrible than even themselves. But they carried the seed of their own destruction in them."}},
	{{"seed"}, {"This ancient race was annihilated by its own doings, that's all I know. Aeons have passed since then, but the sheer presence of this complex is still defiling and desecrating this area."}},
	{{"destruction"}, {previous}},
	{{"complex"}, {"Its constructors were too strange for you or even me to understand. We cannot know what this ... thing they have built was supposed to be good for. All I can feel is a constant twisting and binding of souls, though that is probably only a side-effect."}},
	{{"ghostlands"}, {"The place you know as the Ghostlands had a different name once ... and many names thereafter. Too many for me to remember them all."}},
	{{"banshee"}, {"They are my maidens. They give me comfort in my eternal vigil over the last seal."}},
	{{"seal"}, {"I am the guardian of the SEVENTH and final seal. The seal to open the last door before ... but perhaps it is better you see it with your own eyes."}},
	{{"guardian"}, {previous}},
	{{"seventh", lt(LEVEL, 60), FORCE}, {"You are not experienced enough to master the challenges ahead or to receive knowledge about the seventh seal. Go and learn more before asking me again."}},
	{{"seventh", gt(LEVEL, 59), FORCE}, {"If you have passed the first six seals and entered the blue fires that lead to the chamber of the seal you might receive my kiss ... It will open the last seal. Do you think you are ready?", setTopic(2)}},
	{{"last"}, {previous}},

	{{"kiss", PZBLOCK, FORCE}, {"You have spilled too much blood recently and the dead are hungry for your soul. Perhaps return when you regained you inner balance."}},

	{{"kiss", eq(TOPIC, 8), lt(questValue(11), 1)}, {"Are you prepared to receive my kiss, even though this will mean that your death as well as a part of your soul will forever belong to me, my dear?", setTopic(1)}},

	{{"kiss", gt(questValue(11), 0)}, {"You have already received my kiss. You should know better then to ask for it."}},
	{{"kiss"}, {"To receive my kiss you have to pass all other seals first."}},
	{{"yes", eq(TOPIC, 1)}, {"So be it! Hmmmmmm...", setQuestValue(11, 1), setQuestValue(12, add(questValue(12), 1)), teleport(32202, 31812, 8), effectOpp(14)}},
	{{"no", eq(TOPIC, 1)}, {"Perhaps it is the better choice for you, my dear."}},

	{{"yes", eq(TOPIC, 2), eq(questValue(4), 1)}, {"Yessss, I can sense you have passed the seal of sacrifice. Have you passed any other seal yet?", setTopic(3)}},
	{{"yes", eq(TOPIC, 2), lt(questValue(4), 1)}, {"You have not passed the seal of sacrifice yet. Return to me when you are better prepared."}},
	{{"no", eq(TOPIC, 2)}, {"Then try to be better prepared next time we meet."}},

	{{"yes", eq(TOPIC, 3), eq(questValue(5), 1)}, {"I sense you have passed the hidden seal as well. Have you passed any other seal yet?", setTopic(4)}},
	{{"yes", eq(TOPIC, 3), lt(questValue(5), 1)}, {"You have not found the hidden seal yet. Return when you are better prepared."}},
	{{"no", eq(TOPIC, 3)}, {"Then try to be better prepared next time we meet."}},

	{{"yes", eq(TOPIC, 4), eq(questValue(6), 1)}, {"Oh yes, you have braved the plagueseal. Have you passed any other seal yet?", setTopic(5)}},
	{{"yes", eq(TOPIC, 4), lt(questValue(6), 1)}, {"You have not faced the plagueseal yet. Return to me when you are better prepared."}},
	{{"no", eq(TOPIC, 4)}, {"Then try to be better prepared next time we meet."}},

	{{"yes", eq(TOPIC, 5), eq(questValue(7), 1)}, {"Ah, I can sense the power of the seal of demonrage burning in your heart. Have you passed any other seal yet?", setTopic(6)}},
	{{"yes", eq(TOPIC, 5), lt(questValue(7), 1)}, {"You are not filled with the fury of the imprisoned demon. Return when you are better prepared."}},
	{{"no", eq(TOPIC, 5)}, {"Then try to be better prepared next time we meet."}},

	{{"yes", eq(TOPIC, 6), eq(questValue(9), 1)}, {"So, you have managed to pass the seal of the true path. Have you passed any other seal yet?", setTopic(7)}},
	{{"yes", eq(TOPIC, 6), lt(questValue(9), 1)}, {"You have not found your true path yet. Return when you are better prepared."}},
	{{"no", eq(TOPIC, 6)}, {"Then try to be better prepared next time we meet."}},

	{{"yes", eq(TOPIC, 7), eq(questValue(10), 1)}, {"I see! You have mastered the seal of logic. You have made the sacrifice, you have seen the unseen, you possess fortitude, you have filled yourself with power and found your path. You may ask me for my kiss now.", setTopic(8)}},
	{{"yes", eq(TOPIC, 7), lt(questValue(10), 1)}, {"You have not found your true path yet. Return to meh when you are better prepared."}},
	{{"no", eq(TOPIC, 7)}, {"Then try to be better prepared next time we meet."}},

	{{"spectral", "dress"}, {"Your wish for a spectral dress is silly. Allthough I will grant you the permission to take one. My maidens left one in a box in a room, directly south of here.", setQuestValue(327, 1)}},
}

function onCreatureSay(npc, creature, text, situation)
	behaviour.react(npc, rules, text, situation)
end

function onCreatureDisappear(npc, creature)
	behaviour.react(npc, rules, "", NPC_SITUATION_VANISH)
end
