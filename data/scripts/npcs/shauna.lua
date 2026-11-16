-- npcs/shauna.lua: the behaviour of the NPC Shauna, converted from
-- shauna.npc by patella-convert. The NPC is its [[npc]] entry in npcs.toml;
-- lib/npc.lua matches and runs the rules below, in the order of the file
-- (scripting.hh, "NPCs"). The comments are those of shauna.npc.
--
-- GIMUD - Graphical Interface Multi User Dungeon
-- shauna.npc: Datenbank für den Sheriff von Carlin Shauna

local behaviour = require("lib.npc")
local ADDRESS = behaviour.ADDRESS
local BUSY = behaviour.BUSY
local VANISH = behaviour.VANISH
local FORCE = behaviour.FORCE
local eq = behaviour.eq
local TOPIC = behaviour.TOPIC
local setTopic = behaviour.setTopic
local burning = behaviour.burning
local effectMe = behaviour.effectMe
local effectOpp = behaviour.effectOpp
local idle = behaviour.idle
local nop = behaviour.nop
local previous = behaviour.previous

local rules = behaviour.rules {
	{{ADDRESS, "hail$", FORCE}, {"Howdy!"}},
	{{ADDRESS, "salutations$", FORCE}, {previous}},
	{{ADDRESS, "hello$", FORCE}, {previous}},
	{{ADDRESS, "hi$", FORCE}, {previous}},
	{{ADDRESS, FORCE}, {idle}},
	{{BUSY, "hello$", FORCE}, {"Shut up!"}},
	{{BUSY, "hi$", FORCE}, {previous}},
	{{BUSY, "hail$", FORCE}, {previous}},
	{{BUSY, "salutations$", FORCE}, {previous}},
	{{BUSY, FORCE}, {nop}},
	{{VANISH, FORCE}, {"LONG LIVE THE QUEEN!"}},

	{{"bye"}, {"LONG LIVE THE QUEEN!", idle}},
	{{"news"}, {"No news are good news."}},
	{{"queen"}, {"HAIL TO QUEEN ELOISE!"}},
	{{"leader"}, {previous}},
	{{"job"}, {"What do you think? I am the sheriff of Carlin."}},
	{{"how", "are", "you"}, {"Just fine."}},

	{{"sell"}, {"Would you like to buy the general key to the town?", setTopic(1)}},
	{{eq(TOPIC, 1), "yes"}, {"Yeah, I bet you'd like to do that! HO, HO, HO!"}},

	{{"army"}, {"If they make trouble, I'll put them behind bars like all others."}},
	{{"guard"}, {previous}},
	{{"general"}, {"The Bonecrusher family is ideally suited for military jobs."}},
	{{"bonecrusher"}, {previous}},
	{{"enemies"}, {"If you have a crime to report and clues, then do it, but dont waste my time."}},
	{{"enemy"}, {previous}},
	{{"criminal"}, {previous}},
	{{"murderer"}, {previous}},
	{{"castle"}, {"The castle is one of the safest places in Carlin."}},
	{{"subject"}, {"Our people are fine and peaceful."}},
	{{"tbi$"}, {"I bet they spy on us... not my business, however."}},
	{{"todd$"}, {"I scared this bigmouth so much that he left the town by night. HO, HO, HO!"}},
	{{"city"}, {"The city is is a peacful place, and it's up to me to keep it this way."}},
	{{"hain$"}, {"He is the guy responsible to keep the sewers working. Someone has to do such kind of jobs. I can't handle all the garbage of the city myself."}},
	{{"rowenna$"}, {"Rowenna is one of our local smiths. When you look for weapons, look for Rowenna."}},
	{{"weapon"}, {previous}},
	{{"Cornelia"}, {"Cornelia is one of our local smiths. When you look for armor, look for Rowenna."}},
	{{"armor"}, {previous}},
	{{"legola"}, {"She has the sharpest eye in the region, I'd say."}},
	{{"padreia"}, {"Her peacefulness is sometimes near stupidity."}},
	{{"god"}, {"I worship Banor of course."}},
	{{"banor"}, {"For me, he's the god of justice."}},
	{{"zathroth"}, {"His cult is forbidden in our town."}},
	{{"brog"}, {"Wouldn't wonder if some males worship him secretly. HO, HO, HO!"}},
	{{"monster"}, {"I deal more with the human mosters, you know? HO, HO, HO!"}},
	{{"excalibug"}, {"Would certainly make a good butterknife. HO, HO, HO!"}},
	{{"rebellion"}, {"The only thing that rebels here now and then is the stomach of a male after trying to make illegal alcohol. HO, HO, HO!"}},
	{{"alcohol"}, {"For obvious reasons it's forbidden in our city."}},

	{{"waterpipe"}, {"Oh, there's a waterpipe in one of my cells? ...", "I guess my last prisoner forgot it there."}},
	{{"pipe"}, {previous}},
	{{"prisoner"}, {"My last prisoner? Hmm. ...", "I think he was some guy from Darama. Can't remember his name. ...", "He was here just for one night, because he got drunk and annoyed our citizens. ...", "Obviously he wasn't pleased with this place, because he headed for Thais the next day. ...", "Something tells me that he won't stay out of trouble for too long."}},

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

function onCreatureSay(npc, creature, text, situation)
	behaviour.react(npc, rules, text, situation)
end

function onCreatureDisappear(npc, creature)
	behaviour.react(npc, rules, "", NPC_SITUATION_VANISH)
end
