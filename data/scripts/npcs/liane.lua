-- npcs/liane.lua: the behaviour of the NPC Liane, converted from
-- liane.npc by patella-convert. The NPC is its [[npc]] entry in npcs.toml;
-- lib/npc.lua matches and runs the rules below, in the order of the file
-- (scripting.hh, "NPCs"). The comments are those of liane.npc.
--
-- GIMUD - Graphical Interface Multi User Dungeon
-- liane.npc: Datenbank für die Postfrau Liane

local behaviour = require("lib.npc")
local genPost = require("lib.npcs.gen-post")
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
local add = behaviour.add
local setTopic = behaviour.setTopic
local setAmount = behaviour.setAmount
local setType = behaviour.setType
local setQuestValue = behaviour.setQuestValue
local delete = behaviour.delete
local queue = behaviour.queue
local idle = behaviour.idle
local nop = behaviour.nop
local previous = behaviour.previous

local rules = behaviour.rules {
	{{ADDRESS, "hello$", FORCE}, {"Hello %N. May I help you?"}},
	{{ADDRESS, "hi$", FORCE}, {previous}},
	{{ADDRESS, FORCE}, {idle}},
	{{BUSY, "hello$", FORCE}, {"Please wait a minute, %N. I am busy.", queue}},
	{{BUSY, "hi$", FORCE}, {previous}},
	{{BUSY, FORCE}, {nop}},
	{{VANISH, FORCE}, {"It was a pleasure to help you."}},

	{{"bye"}, {"It was a pleasure to help you.", idle}},
	{{"farewell"}, {previous}},

	{{"kevin"}, {"Kevin Postner was already leader of the guild as I joined. I can't imagine anyone better for that position."}},
	{{"postner"}, {previous}},
	{{"postmasters", "guild"}, {"Our guild relys heavily on the honor and trustworthyness of its members."}},
	{{"join"}, {"You might apply for a membership in our haedquarter."}},
	{{"headquarter"}, {"Its just south oh Kazordoon. Follow the road and you will run right into it."}},

	{{"measurements", gt(questValue(234), 0), lt(questValue(239), 1)}, {"I have more urgent problem to attend then that. Those hawks are hunting my carrier pigeons. Bring me 12 arrows and I'll see if I have the time for this nonsense. Do you have 12 arrows with you?", setType(3447), setAmount(12), setTopic(5)}},
	{{"arrows", gt(questValue(234), 0), lt(questValue(239), 1)}, {"Do you have 12 arrows with you?", setType(3447), setAmount(12), setTopic(5)}},

	{{eq(TOPIC, 5), "yes", ge(count(TYPE), AMOUNT)}, {"Great! Now I'll teach them a lesson ... For those measurements ... <tells you her measurements>", delete(TYPE), setQuestValue(234, add(questValue(234), 1)), setQuestValue(239, 1)}},
	{{eq(TOPIC, 5), "yes"}, {"Fool, you have no 12 arrows."}},
	{{eq(TOPIC, 5)}, {"Don't waste my time."}},

	{{"job"}, {"I am working here at the post office. If you have questions about the Royal Carlin Mail System or the depots ask me."}},
	{{"office"}, {"I rarely leave my office. You are welcome at any time."}},
	{{"name"}, {"My name is Liane."}},
	{{"time"}, {"Now it's %T."}},
	--"mail"     -> "Our mail system is unique! And so simple even males can use it. Do you want to know more about it?", Topic=1
	--"depot"    -> "The depots are very easily to use. Just step in front of them and you will find your items in them. They are free for all citizens. Hail our Queen!"
	{{"queen"}, {"Our Queen's rule makes Carlin prosper."}},
	{{"carlin"}, {"Our wonderful town is protected by the wise Queen Eloise."}},
	{{"thais"}, {"A town ruled by men, a dangerous place. Anyway, we bring also letters and parcels there."}},
	{{"benjamin"}, {"He is the postman in Thais and somewhat stupid. But he never sents wrong letters or parcels."}},
	{{"ghostlands"}, {"We don't deliver letters or parcels there, sorry."}},
	{{"wally"}, {"Wally and I became pen-pals in the course of years."}},

	genPost,

	--"letter" -> Amount=1, Price=5,  "Do you want to buy a letter for %P gold?", Topic=2
	--"parcel" -> Amount=1, Price=10, "Do you want to buy a parcel for %P gold?", Topic=3

	--Topic=1,"yes" -> "The Mail System enables you to send and receive letters and parcels. You can buy them here if you want."
	--Topic=1       -> "Is there anything else, I can do for you?"

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
