-- lib/npcs/guards-carlin.lua: the rule block guards-carlin.ndb, converted by patella-convert.
-- An NPC that included it (@"guards-carlin.ndb") has it in its rules where the include
-- was; lib/npc.lua matches and runs them (scripting.hh, "NPCs"). The comments
-- are those of guards-carlin.ndb.
--
-- GIMUD - Graphical Interface Multi User Dungeon
-- guards-carlin.ndb: Datenbank für die Stadtwachen von Carlin

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
	{{ADDRESS, "hello$", FORCE}, {"LONG LIVE THE QUEEN!"}},
	{{ADDRESS, "hi$", FORCE}, {previous}},
	{{ADDRESS, FORCE}, {idle}},
	{{BUSY, "hello$"}, {"I am busy right now!"}},
	{{BUSY, "hi$"}, {previous}},
	{{BUSY, FORCE}, {nop}},
	{{VANISH, FORCE}, {"Hrmpf!"}},

	{{"bye"}, {"LONG LIVE THE QUEEN!", idle}},
	{{"farewell"}, {previous}},
	{{"news"}, {"I am busy. Please ask the citizens for news."}},
	{{"how", "are", "you"}, {"I am healthy and vigilant."}},
	{{"sell"}, {"Visit Carlin's shopkeepers to buy their fine wares."}},
	{{"queen"}, {"Queen Eloise is our beloved sovereign!"}},
	{{"leader"}, {previous}},
	{{"job"}, {"It's my duty to protect our fair city."}},
	{{"army"}, {"Of course, we guards are members of the army."}},
	{{"guard"}, {"I am a guard and proud of it."}},
	{{"name"}, {"It's Miss Bonecrusher to you!"}},
	{{"bonecrusher"}, {"The bonecrusher family has been serving in the army of Carlin for generations. My sister Bunny is the general of our army."}},
	{{"army"}, {"Ask my sister Bunny for details."}},
	{{"castle"}, {"The castle is in the northwest corner of the city."}},
	{{"green", "ferrets"}, {"Brave warriors, indeed."}},
	{{"knights", "of", "noodles"}, {"WOOF! WOOF! Go away with theese puny king's puppys."}},
	{{"secret", "police"}, {"Ask a higher offical about that."}},
	{{"city"}, {"Behave well, while in the city, or we'll get you! Do you want to know where to find a shop or a guild?"}},
	{{"shop"}, {"There's a smith, a provisioner, a tavern, a magic shop, and the royal post office, of course."}},
	{{"guild"}, {"In the city you will find the guildhouses of the Knights, the Paladins, the Druids, and the Sorcerers."}},
	{{"scum"}, {"We will get rid of all scum."}},
	{{"barbara"}, {"Fine warrioress, even if not of our family."}},
	{{"fenbala"}, {previous}},
	{{"bunny"}, {"My sister is in charge of our mighty army."}},
	{{"busty"}, {"We are Bonecrushers. Mess with one of us, and we all will come for you!"}},
	{{"bambi"}, {previous}},
	{{"blossom"}, {previous}},
	{{"banor"}, {"Praise Banor! May the great warrior be with us!"}},
	{{"graveyard"}, {"Stay away from the graveyard, it's haunted!"}},
	{{"crypt"}, {previous}},
	{{"haunted"}, {"Strange things happen in our graveyard, and sometimes there are .. noises."}},
	{{"noise"}, {"I never heared them myself, but people, one can trust, did."}},
	{{"male$"}, {"Bah! Who cares about males? Let them do males' work, cleaning the sewers for example."}},
	{{"sewer"}, {"Sewers are filthy and disgusting. We let the men take care of them."}},
	{{"rebellion"}, {"What a joke. The men have no guts for a rebellion."}},
	{{"resistance"}, {previous}},
	{{"tod$"}, {"There once was an adventurer with this name in town, made some trouble, got kicked out. No big deal."}},
	{{"ghostlands"}, {"In theory the ghostlands are of limits. But we don't enforce that. Anyone stupid enough to go there will meet his deserved fate."}},

	{{"fletcher"}, {"You can find the weapon and armour shops just west of the towncenter."}},
	{{"smith"}, {previous}},
	{{"weapon"}, {previous}},
	{{"armor"}, {previous}},
	{{"sarina"}, {"Sarina is our provisioner. You can find her in the towncenter, south of the weaponshops."}},
	{{"provision"}, {previous}},
	{{"dane"}, {"Dane runs the local tavern and hotel. You can find it at the southwest beach of Carlin."}},
	{{"tavern"}, {previous}},
	{{"liane"}, {"Liane is a kind person. She runs the post office in the town's center."}},
	{{"post"}, {previous}},
	{{"depot"}, {"We have some depots in our town. You can't miss them."}},
	{{"lea$"}, {"Lea is the head of the local sorcerers guild. You can find it south of the towncenter at the magic shop of Rachel."}},
	{{"sorcerer"}, {previous}},
	{{"legola"}, {"Legola runs the local paladins guild, it's near the westgate."}},
	{{"paladin"}, {previous}},
	{{"padreia"}, {"Padreia is the greatest druid of the continent. You find the guild of the benevolent druids in the southwest of the city."}},
	{{"druid"}, {previous}},
	{{"trisha"}, {"Trisha is the leader of our knights guild. It is in the north of the town, near the gate."}},
	{{"knight"}, {previous}},
	{{"rachel"}, {"Rachel sells equipment for all magic users in her shop. There is also the sorcerer guild in the second floor."}},
	{{"magic"}, {previous}},
	{{"spell"}, {previous}},

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
