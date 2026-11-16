-- lib/npcs/guards-darama.lua: the rule block guards-darama.ndb, converted by patella-convert.
-- An NPC that included it (@"guards-darama.ndb") has it in its rules where the include
-- was; lib/npc.lua matches and runs them (scripting.hh, "NPCs"). The comments
-- are those of guards-darama.ndb.
--
-- GIMUD - Graphical Interface Multi User Dungeon
-- guards-darama.ndb: Datenbank für die Stadtwachen von Darama

local behaviour = require("lib.npc")
local ADDRESS = behaviour.ADDRESS
local BUSY = behaviour.BUSY
local VANISH = behaviour.VANISH
local FORCE = behaviour.FORCE
local burning = behaviour.burning
local effectMe = behaviour.effectMe
local effectOpp = behaviour.effectOpp
local idle = behaviour.idle
local nop = behaviour.nop
local previous = behaviour.previous

return behaviour.rules {
	{{ADDRESS, "hello$", FORCE}, {"Daraman's blessings!"}},
	{{ADDRESS, "hi$", FORCE}, {previous}},
	{{ADDRESS, FORCE}, {idle}},
	{{BUSY, "hello$"}, {"I am busy!"}},
	{{BUSY, "hi$"}, {previous}},
	{{BUSY, FORCE}, {nop}},
	{{VANISH, FORCE}, {"Daraman's blessings!"}},

	{{"bye"}, {"Daraman's blessings!", idle}},
	{{"news"}, {"The evil of Drefia is strong in these days."}},
	{{"how", "are", "you"}, {"I am on guard and my soul is strong."}},
	{{"sell"}, {"Go to the bazaar in the middle of the town."}},
	{{"buy"}, {previous}},
	{{"caliph"}, {"Our leader is enlightened by Daraman, thrice praised be his name!"}},
	{{"leader"}, {previous}},
	{{"job"}, {"I am a protector of the people of Darashia."}},
	{{"army"}, {"This information is confidential."}},
	{{"guard"}, {previous}},
	{{"daraman"}, {"The ancient prophet brought our people here from a place long forgotten."}},
	{{"pilgrim"}, {"Darama led his people on a pilgrimage to the holy lands of Darama to escape temptations and distractions."}},
	{{"necromancer"}, {"It's said they fled from the corrupted continent of the thaian empire after losing some battles."}},
	{{"city"}, {"This city is lovely Darashia, pearl of Darama."}},
	{{"darashia"}, {previous}},
	{{"darama"}, {"Darashia is the heart of Darama. In the northeast, across the Devourer, there is the dark pyramid. In the west, across the Plague Spike, are the cursed ruins of Drefia."}},
	{{"plague", "spike"}, {"Mountain of Poison, Scorpions Rock, it's known by many names. It's to the west of Darashia."}},
	{{"devourer"}, {"The great desert devoured many of the first pilgrims. We learned the ways of the devourer and thus conquered it."}},
	{{"pyramid"}, {"The minotaurs took shelter in the ancient pyramid, which our people avoid for the dark powers that might be present there."}},
	{{"drefia"}, {"The ruins of Drefia in the far west are the hideout of the vile necromancer cult that once corrupted this city."}},

	{{"fuck"}, {"Take this!", burning(10, 1), effectOpp(5), effectMe(8)}},
	{{"idiot"}, {"Take this!", burning(10, 1), effectOpp(5), effectMe(8)}},
	{{"asshole"}, {"Take this!", burning(10, 1), effectOpp(5), effectMe(8)}},
	{{"ass$"}, {"Take this!", burning(10, 1), effectOpp(5), effectMe(8)}},
	{{"fag$"}, {"Take this!", burning(10, 1), effectOpp(5), effectMe(8)}},
	{{"stupid"}, {"Take this!", burning(10, 1), effectOpp(5), effectMe(8)}},
	{{"tyrant"}, {"Take this!", burning(10, 1), effectOpp(5), effectMe(8)}},
	{{"shit"}, {"Take this!", burning(10, 1), effectOpp(5), effectMe(8)}},
	{{"lunatic"}, {"Take this!", burning(10, 1), effectOpp(5), effectMe(8)}},
}
