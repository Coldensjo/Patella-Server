-- npcs/dove.lua: the behaviour of the NPC Dove, converted from
-- dove.npc by patella-convert. The NPC is its [[npc]] entry in npcs.toml;
-- lib/npc.lua matches and runs the rules below, in the order of the file
-- (scripting.hh, "NPCs"). The comments are those of dove.npc.
--
-- GIMUD - Graphical Interface Multi User Dungeon
-- dove.npc: Datenbank für die Postbeamtin Dove

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
	{{ADDRESS, "hello$", FORCE}, {"Be greeted, noble %N."}},
	{{ADDRESS, "hi$", FORCE}, {previous}},
	{{ADDRESS, FORCE}, {idle}},
	{{BUSY, "hello$", FORCE}, {"Please wait a moment, %N.", queue}},
	{{BUSY, "hi$", FORCE}, {previous}},
	{{BUSY, FORCE}, {nop}},
	{{VANISH, FORCE}, {"Come back soon, noble %N."}},

	{{"bye"}, {"Come back soon, noble %N.", idle}},
	{{"farewell"}, {previous}},

	{{"kevin"}, {"Mr. Postner is one of the most honorable men I know."}},
	{{"postner"}, {previous}},
	{{"postmasters", "guild"}, {"As long as everyone lives up to our standarts our guild will be fine."}},
	{{"join"}, {"We are always looking able recruits. Just speak to Mr.Postner in our headquarter."}},
	{{"headquarter"}, {"Its easy to be found. Its on the road from Thais to Kazordoon and Ab'dendriel."}},

	{{"measurements", gt(questValue(234), 0), lt(questValue(238), 1)}, {"Oh no! I knew that day would come! I am slightly above the allowed weight and if you can't supply me with some grapes to slim down I will get fired. Do you happen to have some grapes with you?", setType(3592), setAmount(1), setTopic(5)}},

	{{"grapes", gt(questValue(234), 0), lt(questValue(238), 1)}, {"Do you happen to have some grapes with you?", setType(3592), setAmount(1), setTopic(5)}},

	{{eq(TOPIC, 5), "yes", ge(count(TYPE), AMOUNT)}, {"Oh thank you! Thank you so much! So listen ... <whispers her measurements>", delete(TYPE), setQuestValue(234, add(questValue(234), 1)), setQuestValue(238, 1)}},
	{{eq(TOPIC, 5), "yes"}, {"Don't tease me! You don't have any."}},
	{{eq(TOPIC, 5)}, {"Oh, no! I might loose my job."}},

	{{"job"}, {"I am responsible for this post office. If you have questions about the mail system or the depots, just ask me."}},
	{{"name"}, {"My name is Dove."}},
	{{"dove"}, {"Yes, like the bird. <giggles>"}},
	{{"time"}, {"Now it's %T."}},
	--"mail"       -> "The Tibian mail system is unique! And everyone can use it. Do you want to know more about it?", Topic=1
	--"depot"      -> "The depots are very easy to use. Just step in front of them and you will find your items in them. They are free for all Tibian citizens."
	{{"king"}, {"Even the king can be reached by the mailsystem."}},
	{{"tibianus"}, {previous}},
	{{"army"}, {"The soldiers get a lot of letters and parcels from Thais each week."}},
	{{"ferumbras"}, {"Try to contact him by mail."}},
	{{"general"}, {previous}},
	{{"sam"}, {"Ham? No thanks, I ate fish already."}},
	{{"excalibug"}, {"If i find it in an undeliverable parcel, I will contact you."}},
	{{"news"}, {"Well, there are rumours about the swampelves and the amazons, as usual."}},
	{{"thais"}, {"All cities are covered by our mail system."}},
	{{"carlin"}, {previous}},
	{{"swampelves"}, {"They live somewhere in the swamp and usually stay out of our city. Only now and then some of them dare to interfere with us."}},
	{{"amazon"}, {"These women are renegades from Carlin, and one of their hidden villages or hideouts might be in the swamp."}},

	genPost,

	--"letter" -> Amount=1, Price=5,  "Do you want to buy a letter for %P gold?", Topic=2
	--"parcel" -> Amount=1, Price=10, "Do you want to buy a parcel for %P gold?", Topic=3

	--Topic=1,"yes" -> "The Tibia Mail System enables you to send and receive letters and parcels. You can buy them here if you want."
	--Topic=1       -> "Is there anything else I can do for you?"

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
