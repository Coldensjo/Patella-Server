-- lib/npcs/guards-thais.lua: the rule block guards-thais.ndb, converted by patella-convert.
-- An NPC that included it (@"guards-thais.ndb") has it in its rules where the include
-- was; lib/npc.lua matches and runs them (scripting.hh, "NPCs"). The comments
-- are those of guards-thais.ndb.
--
-- GIMUD - Graphical Interface Multi User Dungeon
-- guards-thais.ndb: Datenbank für die Stadtwachen von Thais

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
	{{BUSY, "hello$"}, {"I am busy right now!."}},
	{{BUSY, "hi$"}, {previous}},
	{{BUSY, FORCE}, {nop}},
	{{VANISH, FORCE}, {"Hrmpf!"}},

	{{"bye"}, {"LONG LIVE THE KING!", idle}},
	{{"farewell"}, {previous}},
	{{"news"}, {"I am busy. Please ask the citizens for news."}},
	{{"how", "are", "you"}, {"I am healthy and vigilant."}},
	{{"sell"}, {"Visit Tibia's shopkeepers to buy their fine wares."}},
	{{"tibianus"}, {"Tibianus III is our beloved king!"}},
	{{"king"}, {previous}},
	{{"leader"}, {previous}},
	{{"job"}, {"It's my duty to protect the city."}},
	{{"army"}, {"Of course we guards are members of the army."}},
	{{"guard"}, {"I am a guard and proud of it."}},
	{{"battlegroups"}, {"Ask higher officials about that, please."}},
	{{"castle"}, {"The castle is at the west of the city."}},
	{{"dogs", "of", "war"}, {"Brave warriors, indeed."}},
	{{"knights", "of", "noodles"}, {"Every guard dreams of becoming one of them one day."}},
	{{"red", "guard"}, {"We of the red guard are the special forces."}},
	{{"secret", "police"}, {"Ask a higher offical about that."}},
	{{"silver", "guard"}, {"Only the best of the best serve as silver guards."}},
	{{"city"}, {"Behave while in the city or we get you! Do you want to know where to find a shop or a guild?"}},
	{{"shop"}, {"There's a smith, a provisioner, and a tavern."}},
	{{"guild"}, {"In the city you will find the guildhouses of the knights, paladins, druids, and sorcerers."}},
	{{"scum"}, {"We will get rid of all scum."}},
	{{"stutch"}, {"He is a soldier in the silver guard."}},
	{{"harsky"}, {previous}},
	{{"baxter"}, {"He is a role model for us."}},
	{{"bozo"}, {"The royal jester."}},
	{{"harkath$"}, {"The royal general. A warrior worth Banor's blessings."}},
	{{"bloodblade$"}, {previous}},
	{{"general$"}, {previous}},
	{{"banor"}, {"Praise Banor! May the great warrior be with us!"}},
	{{"sam"}, {"Sam is our blacksmith. You'll find him north of the main crossroads. His shop is to the left."}},
	{{"smith"}, {previous}},
	{{"weapon"}, {previous}},
	{{"armor"}, {previous}},
	{{"gorn"}, {"Gorn is our provisioner. You'll find him north of the main crossroads. His shop is to the right."}},
	{{"provision"}, {previous}},
	{{"frodo"}, {"Frodo runs the local tavern. You'll find it at the main crossroads to the north-west."}},
	{{"tavern"}, {previous}},
	{{"benjamin"}, {"Benjamin was a brave fighter. He runs the post office in the west of the city."}},
	{{"post"}, {previous}},
	{{"depot"}, {"The depot is at the post office in the west of the city."}},
	{{"muriel"}, {"Muriel is the head of the local sorcerers' guild. You'll find it in the south-west of the city"}},
	{{"sorcerer"}, {previous}},
	{{"elane"}, {"Elane is responsible for the local paladins' guild. It's in the west of the town, directly south of the post office."}},
	{{"paladin"}, {previous}},
	{{"marvik"}, {"Marvik is the great druid of the local guild. You'll find him by climbing up the citywalls at the east."}},
	{{"druid"}, {previous}},
	{{"gregor"}, {"The high knight of the knights' guild. It is in north-east of the town."}},
	{{"knight"}, {previous}},

	{{"fuck"}, {"Take this!", burning(5, 1), effectOpp(5), effectMe(8)}},
	{{"idiot"}, {"Take this!", burning(5, 1), effectOpp(5), effectMe(8)}},
	{{"asshole"}, {"Take this!", burning(5, 1), effectOpp(5), effectMe(8)}},
	{{"ass$"}, {"Take this!", burning(5, 1), effectOpp(5), effectMe(8)}},
	{{"fag$"}, {"Take this!", burning(5, 1), effectOpp(5), effectMe(8)}},
	{{"stupid"}, {"Take this!", burning(5, 1), effectOpp(5), effectMe(8)}},
	{{"tyrant"}, {"Take this!", burning(5, 1), effectOpp(5), effectMe(8)}},
	{{"shit"}, {"Take this!", burning(5, 1), effectOpp(5), effectMe(8)}},
	{{"lunatic"}, {"Take this!", burning(5, 1), effectOpp(5), effectMe(8)}},
}
