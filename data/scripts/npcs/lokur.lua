-- npcs/lokur.lua: the behaviour of the NPC Lokur, converted from
-- lokur.npc by patella-convert. The NPC is its [[npc]] entry in npcs.toml;
-- lib/npc.lua matches and runs the rules below, in the order of the file
-- (scripting.hh, "NPCs"). The comments are those of lokur.npc.
--
-- GIMUD - Graphical Interface Multi User Dungeon
-- lokur.npc: Datenbank für den Postzwerg Lokur

local behaviour = require("lib.npc")
local genPost = require("lib.npcs.gen-post")
local ADDRESS = behaviour.ADDRESS
local BUSY = behaviour.BUSY
local VANISH = behaviour.VANISH
local FORCE = behaviour.FORCE
local eq = behaviour.eq
local gt = behaviour.gt
local TOPIC = behaviour.TOPIC
local questValue = behaviour.questValue
local setTopic = behaviour.setTopic
local setQuestValue = behaviour.setQuestValue
local queue = behaviour.queue
local idle = behaviour.idle
local nop = behaviour.nop
local previous = behaviour.previous

local rules = behaviour.rules {
	{{ADDRESS, "hello$", FORCE}, {"Hiho %N. May I help you?"}},
	{{ADDRESS, "hi$", FORCE}, {"Hiho %N. May I help you?"}},
	{{ADDRESS, FORCE}, {idle}},
	{{BUSY, "hello$", FORCE}, {"Sorry %N, I am busy right now. One moment please.", queue}},
	{{BUSY, "hi$", FORCE}, {previous}},
	{{BUSY, FORCE}, {nop}},
	{{VANISH, FORCE}, {"Come back if you need my services."}},

	{{"bye"}, {"Come back if you need my services.", idle}},
	{{"farewell"}, {previous}},
	{{"job"}, {"I am the royal postdwarf and damned proud of it."}},
	{{"office"}, {"It's not big but I like the company."}},
	{{"name"}, {"My name is Lokur Stampsmasher, son of Earth of the Dragoneaters."}},
	{{"time"}, {"Too bad, I forgot my watch at home."}},
	{{"mail"}, {"The mail system was invented by dwarfs! Do you want me to tell you about it?", setTopic(1)}},
	--"depot"    -> "Just walk towards them and you will find the items you stored in them during your last visit."
	{{"king"}, {"Our king has a treasure room and does not need a depot."}},
	{{"carlin"}, {"Imagine, they have a postoffice their too, jawoll."}},
	{{"thais"}, {previous}},

	{{"kevin"}, {"Ah, this human is persistant as a dwarf. A worthy leader indeed, jawoll."}},
	{{"postner"}, {previous}},
	{{"postmasters", "guild"}, {"The guild keeps things running. Organized and reliable. I appreciate that, jawoll."}},
	{{"join"}, {"Our members are handpicked by Kevin postner in our headquarter."}},
	{{"headquarter"}, {"Its south of kazordoon. Just follow that road, can't miss it."}},

	{{"measurements", gt(questValue(235), 0)}, {"Ask Kroox about that stuff."}},
	{{"measurements", gt(questValue(234), 0)}, {"Come on, I have no clue what they are. Better ask my armorer Kroox for such nonsense. Go and ask him for good ol' Lokurs measurements, he'll know.", setQuestValue(235, 1)}},

	genPost,

	--"letter" -> Amount=1, Price=5,  "So you want to buy a letter for %P gold?", Topic=2
	--"parcel" -> Amount=1, Price=10, "So you want to buy a parcel for %P gold?", Topic=3

	{{eq(TOPIC, 1), "yes"}, {"The mail system enables you to send and receive letters and parcels. You can buy them here if you want."}},
	{{eq(TOPIC, 1)}, {"Is there anything else, I can do for you?"}},

	--Topic=2,"yes",CountMoney>=Price -> "Here it is. Don't forget to write the name of the receiver in the first line and the address in the second one before you put the letter in a mailbox.", DeleteMoney, Create(3505)
	--Topic=2,"yes"                   -> "Oh, you have not enough gold to buy a letter."
	--Topic=2                         -> "Ok."

	--Topic=3,"yes",CountMoney>=Price -> "Here you are. Don't forget to write the name and the address of the receiver on the label. The label has to be in the parcel before you put the parcel in a mailbox.", DeleteMoney, Create(3503), Create(3507)
	--Topic=3,"yes"                   -> "Oh, you have not enough gold to buy a parcel."
	--Topic=3                         -> "Ok."
}

function onCreatureSay(npc, creature, text, situation)
	behaviour.react(npc, rules, text, situation)
end

function onCreatureDisappear(npc, creature)
	behaviour.react(npc, rules, "", NPC_SITUATION_VANISH)
end
