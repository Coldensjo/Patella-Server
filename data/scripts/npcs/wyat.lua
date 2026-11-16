-- npcs/wyat.lua: the behaviour of the NPC Wyat, converted from
-- wyat.npc by patella-convert. The NPC is its [[npc]] entry in npcs.toml;
-- lib/npc.lua matches and runs the rules below, in the order of the file
-- (scripting.hh, "NPCs"). The comments are those of wyat.npc.
--
-- GIMUD - Graphical Interface Multi User Dungeon
-- wyat.npc: Datenbank für den Sheriff von Thais Wyat

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

local rules = behaviour.rules {
	{{ADDRESS, "hail$", FORCE}, {"Salutations!"}},
	{{ADDRESS, "salutations$", FORCE}, {previous}},
	{{ADDRESS, "hello$", FORCE}, {previous}},
	{{ADDRESS, "hi$", FORCE}, {previous}},
	{{ADDRESS, FORCE}, {idle}},
	{{BUSY, "hello$", FORCE}, {"I am busy!"}},
	{{BUSY, "hi$", FORCE}, {previous}},
	{{BUSY, "hail$", FORCE}, {previous}},
	{{BUSY, "salutations$", FORCE}, {previous}},
	{{BUSY, FORCE}, {nop}},
	{{VANISH, FORCE}, {"LONG LIVE THE KING!"}},

	{{"bye"}, {"LONG LIVE THE KING!", idle}},
	{{"news"}, {"I have no news for the public."}},
	{{"king"}, {"HAIL TO KING TIBIANUS!"}},
	{{"leader"}, {previous}},
	{{"job"}, {"I am the sheriff of the Thaian territory."}},
	{{"how", "are", "you"}, {"I am fine, thanks."}},
	{{"sell"}, {"I am in the safety business."}},
	{{"army"}, {"I usually work with the townguards only."}},
	{{"guard"}, {previous}},
	{{"general"}, {"Old Bloodblade does a fine job."}},
	{{"enemies"}, {"Our enemies are numerous and not all are obvious."}},
	{{"enemy"}, {previous}},
	{{"criminal"}, {previous}},
	{{"murderer"}, {previous}},
	{{"castle"}, {"The castle should be relatively safe from criminal transgressions."}},
	{{"subject"}, {"There are certain criminal objects in the population of our town."}},
	{{"red", "guard"}, {"Most of the red guards serve as cityguards, some work for the TBI though."}},
	{{"secret", "police"}, {"All i can tell you is, that it's known as the TBI."}},
	{{"tbi$"}, {"The Tibian Bureau of Investigation. If you want to know more, ask Chester Kahs about it, but I doubt you'll get any vital information."}},
	{{"chester"}, {"His bureau is at the northgate."}},
	{{"silver", "guard"}, {"They are our elite forces."}},
	{{"city"}, {"The city is not as bad as some people might claim, but we certainly have our problems here."}},
	{{"problem"}, {"We will handle each problem with care."}},
	{{"stutch"}, {"A fine warrior, indeed. He is one of the king's bodyguards."}},
	{{"harsky"}, {previous}},
	{{"bozo"}, {"He's so funny, I could listen to his jokes for hours."}},
	{{"sam"}, {"Sam, the Thaian smith, is a man of great diligence. Whenever in need of weapons or armor, just ask him."}},
	{{"weapon"}, {previous}},
	{{"armor"}, {previous}},
	{{"elane"}, {"A woman of great skill and courage. No one deserves the title of a Grandmaster of the Paladins more then her."}},
	{{"gorn"}, {"He was a rowdy in his youth, but now he's a fine citizen as far as I can tell."}},
	{{"benjamin"}, {"The poor fool lost his mind some years ago. It's a good thing they gave him a job in the post office."}},
	{{"ferumbras"}, {"He attacked our town at several occasions but was repelled each time."}},
	{{"quest"}, {"Look up our 'Tibia's most wanted' lists."}},
	{{"mission"}, {previous}},
	{{"god"}, {"I am follower of Banor."}},
	{{"banor"}, {"He is the patron of justice and bravery."}},
	{{"zathroth"}, {"Don't mention this name!"}},
	{{"brog"}, {"The more primitive races such as orcs often worship the raging one."}},
	{{"monster"}, {"Thais should be relatively safe from direct assaults of monsters."}},
	{{"excalibug"}, {"If you have any news about the whereabouts of that blade, report it to me."}},
	{{"rebellion"}, {"Luckily that's nothing I have to care about."}},

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
