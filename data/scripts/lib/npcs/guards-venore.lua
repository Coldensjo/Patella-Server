-- lib/npcs/guards-venore.lua: the rule block guards-venore.ndb, converted by patella-convert.
-- An NPC that included it (@"guards-venore.ndb") has it in its rules where the include
-- was; lib/npc.lua matches and runs them (scripting.hh, "NPCs"). The comments
-- are those of guards-venore.ndb.
--
-- GIMUD - Graphical Interface Multi User Dungeon
-- guards-venore.ndb: Datenbank für die Stadtwachen von Venore

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
	{{ADDRESS, "hello$", FORCE}, {"LONG LIVE THE KING!"}},
	{{ADDRESS, "hi$", FORCE}, {previous}},
	{{ADDRESS, FORCE}, {idle}},
	{{BUSY, "hello$"}, {"Not now, I am busy!"}},
	{{BUSY, "hi$"}, {previous}},
	{{BUSY, FORCE}, {nop}},
	{{VANISH}, {"LONG LIVE THE KING!"}},

	{{"bye"}, {"LONG LIVE THE KING!", idle}},
	{{"news"}, {"Look for the Hard Rock Tavern to learn the latest news."}},
	{{"how", "are", "you"}, {"That's classified information!"}},
	{{"sell"}, {"Ask the merchants of the city. There are more then enough."}},
	{{"buy"}, {previous}},
	{{"tibianus"}, {"Tibianus III is our beloved king!"}},
	{{"king"}, {previous}},
	{{"leader"}, {previous}},
	{{"job"}, {"I am a protector of the people of Venore."}},
	{{"army"}, {"This information is confidential."}},
	{{"guard"}, {previous}},
	{{"army"}, {"All guards are members of the army."}},
	{{"guard"}, {"I am a guard and proud of it."}},
	{{"battlegroups"}, {"I doubt you have the security clearance to ask that."}},
	{{"castle"}, {"The castle is in Thais, the crown of the kingdom."}},
	{{"dogs", "of", "war"}, {"They are our rolemodells."}},
	{{"knights", "of", "noodles"}, {"Every guard dreams of becoming one of them one day."}},
	{{"red", "guard"}, {"We of the red guard are the special forces and town guards."}},
	{{"secret", "police"}, {"This information is confidential."}},
	{{"silver", "guard"}, {"Only the best of the best serve as silver guards."}},
	{{"city"}, {"This city, a member of the Thaian kingdom, is under the protection of the Thaian army."}},

	{{"venore"}, {"The harbour is to the north, the weapon market in the south, the general market to the west, and the bank to the east. You will find other shops and the Hard Rock Tavern in the center."}},
	{{"swampel"}, {"Those elves hide in the swamps and are trying to kill all humans in this area."}},
	{{"amazon"}, {"They are the best example for the results of the Carlin madness."}},
	{{"swamptroll"}, {"This hideous creatures are even more ugly than the normal trolls. They are treacherous and use several poisons."}},
	{{"swamp"}, {"The swamp is a dangerous place and full of monsters, not to mention all those swampelves living at shadowthorn, amazons, and swamptrolls."}},
	{{"monsters"}, {"The swamp is full of nasty snakes and there's a dragon breeding ground somewhere in the swamps."}},

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
