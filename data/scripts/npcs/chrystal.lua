-- npcs/chrystal.lua: the behaviour of the NPC Chrystal, converted from
-- chrystal.npc by patella-convert. The NPC is its [[npc]] entry in npcs.toml;
-- lib/npc.lua matches and runs the rules below, in the order of the file
-- (scripting.hh, "NPCs"). The comments are those of chrystal.npc.
--
-- GIMUD - Graphical Interface Multi User Dungeon
-- chyrstal.npc: Datenbank für die Postfrau Chrystal

local behaviour = require("lib.npc")
local genPost = require("lib.npcs.gen-post")
local ADDRESS = behaviour.ADDRESS
local BUSY = behaviour.BUSY
local VANISH = behaviour.VANISH
local FORCE = behaviour.FORCE
local lt = behaviour.lt
local gt = behaviour.gt
local questValue = behaviour.questValue
local add = behaviour.add
local setQuestValue = behaviour.setQuestValue
local queue = behaviour.queue
local idle = behaviour.idle
local nop = behaviour.nop
local previous = behaviour.previous

local rules = behaviour.rules {
	{{ADDRESS, "hello$", FORCE}, {"At your service %N."}},
	{{ADDRESS, "hi$", FORCE}, {previous}},
	{{ADDRESS, FORCE}, {idle}},
	{{BUSY, "hello$", FORCE}, {"Please be patient %N. I'll be with you in moment.", queue}},
	{{BUSY, "hi$", FORCE}, {previous}},
	{{BUSY, FORCE}, {nop}},
	{{VANISH, FORCE}, {"Who is next?"}},

	{{"bye"}, {"Who is next?", idle}},
	{{"name"}, {"My name is Chrystal."}},
	{{"job"}, {"I am responsible for this post office."}},

	{{"kevin"}, {"Mr. Postner is the leader of our guild and the most prominent postofficer in the whole land."}},
	{{"postner"}, {previous}},
	{{"postmasters", "guild"}, {"Yes, our guild is the lifeblood of the tibia cominity so to say."}},
	{{"join"}, {"You can apply to join only at our headquarter."}},
	{{"headquarter"}, {"You can find it on the road from Thais to Kazordoon."}},

	{{"measurements", gt(questValue(234), 0), lt(questValue(241), 1)}, {"If its necessary ... <tells you her measurements>", setQuestValue(234, add(questValue(234), 1)), setQuestValue(241, 1)}},

	{{"time"}, {"It is %T right now."}},
	{{"king"}, {"Hail to the king!"}},
	{{"tibianus"}, {previous}},
	{{"army"}, {"The army ensures the safety of the traderoutes and of our mail system."}},
	{{"ferumbras"}, {"I bet he never gets any letters."}},
	{{"excalibug"}, {"Better ask knights about that."}},
	{{"tibia"}, {"Our post system spans the entire known world."}},
	{{"thais"}, {"We deliver letters and parcels even there."}},
	{{"carlin"}, {previous}},
	{{"kazordoon"}, {previous}},
	{{"ab'dendriel"}, {previous}},
	{{"edron"}, {"Our post system even delivers letters and parcels to and from this isle."}},
	{{"news"}, {"Sorry, that's postal secret."}},
	{{"rumors"}, {previous}},

	genPost,

	--"letter" -> Amount=1, Price=5,  "Do you want to buy a letter for %P gold?", Topic=2
	--"parcel" -> Amount=1, Price=10, "Do you want to buy a parcel for %P gold?", Topic=3

	--Topic=2,"yes",CountMoney>=Price -> "Here it is. Don't forget to write the name of the receiver in the first line and the address in the second one before you put the letter in a mailbox.", DeleteMoney, Create(110,8)
	--Topic=2,"yes"                   -> "Oh, you have not enough gold to buy a letter."
	--Topic=2                         -> "Ok."

	--Topic=3,"yes",CountMoney>=Price -> "Here you are. Don't forget to write the name and the address of the receiver on the label. The label has to be in the parcel before you put the parcel in a mailbox.", DeleteMoney, Create(110,6), Create(110,10)
	--Topic=3,"yes"                   -> "Oh, you have not enough gold to buy a parcel."
	--Topic=3                         -> "Ok."
}

function onCreatureSay(npc, creature, text, situation)
	behaviour.react(npc, rules, text, situation)
end

function onCreatureDisappear(npc, creature)
	behaviour.react(npc, rules, "", NPC_SITUATION_VANISH)
end
