-- npcs/olrik.lua: the behaviour of the NPC Olrik, converted from
-- olrik.npc by patella-convert. The NPC is its [[npc]] entry in npcs.toml;
-- lib/npc.lua matches and runs the rules below, in the order of the file
-- (scripting.hh, "NPCs"). The comments are those of olrik.npc.
--
-- GIMUD - Graphical Interface Multi User Dungeon
-- olrik.npc: Datenbank für den Diener und Postillion Olrik (Elfenstadt)

local behaviour = require("lib.npc")
local genPost = require("lib.npcs.gen-post")
local ADDRESS = behaviour.ADDRESS
local BUSY = behaviour.BUSY
local VANISH = behaviour.VANISH
local MALE = behaviour.MALE
local FEMALE = behaviour.FEMALE
local FORCE = behaviour.FORCE
local eq = behaviour.eq
local lt = behaviour.lt
local gt = behaviour.gt
local ge = behaviour.ge
local TOPIC = behaviour.TOPIC
local AMOUNT = behaviour.AMOUNT
local COUNTMONEY = behaviour.COUNTMONEY
local questValue = behaviour.questValue
local random = behaviour.random
local add = behaviour.add
local setTopic = behaviour.setTopic
local setPrice = behaviour.setPrice
local setAmount = behaviour.setAmount
local setQuestValue = behaviour.setQuestValue
local deleteMoney = behaviour.deleteMoney
local queue = behaviour.queue
local idle = behaviour.idle
local nop = behaviour.nop
local previous = behaviour.previous

local rules = behaviour.rules {
	{{ADDRESS, "hello$", MALE, FORCE}, {"Greetings, %N."}},
	{{ADDRESS, "hello$", FEMALE, FORCE}, {"Greetings, %N. May I help you?"}},
	{{ADDRESS, "hi$", MALE, FORCE}, {"Greetings, %N."}},
	{{ADDRESS, "hi$", FEMALE, FORCE}, {"Greetings, %N. May I help you?"}},
	{{ADDRESS, "ashari$", FORCE}, {"Greetings."}},
	{{ADDRESS, FORCE}, {idle}},
	{{BUSY, "hello$", MALE, FORCE}, {"A moment please, %N.", queue}},
	{{BUSY, "hi$", MALE, FORCE}, {previous}},
	{{BUSY, "ashari$", MALE, FORCE}, {previous}},
	{{BUSY, "hello$", FEMALE, FORCE}, {"%N! A moment please, my lady. I look forward to talk to you soon.", queue}},
	{{BUSY, "hi$", FEMALE, FORCE}, {previous}},
	{{BUSY, "ashari$", FEMALE, FORCE}, {previous}},
	{{BUSY, FORCE}, {nop}},
	{{VANISH, FORCE}, {"Farewell."}},

	{{"bye"}, {"Farewell.", idle}},
	{{"farewell"}, {previous}},
	{{"asha", "thrazi"}, {previous}},

	{{"kevin"}, {"He is the boss. Ther is allways some boss, so I don't bother about that."}},
	{{"postner"}, {previous}},
	{{"postmasters", "guild"}, {"Ah, they pay me and I do my work and thats it. Its some nice extra cash but I don't care much about that guild."}},
	{{"join"}, {"If you realy think you have to join the postmasters guild ask Kevin Postner at the headquarter."}},
	{{"headquarter"}, {"The postmasters guild headquarter is located south of the mountain known as the big old one, were the city of kazordoon can be found."}},

	{{"measurements", gt(questValue(234), 0), lt(questValue(240), 1)}, {"My measurements? Listen, lets make that a bit more exciting ... No, no, not what you think! I mean let's gamble. I will roll a dice. If I roll a 6 you win and I'll tell you what you need to know, else I win and get 5 gold. Deal?", setAmount(random(1, 6)), setTopic(5)}},
	{{eq(TOPIC, 5), "no"}, {"This way you'll never get my measurements."}},

	{{eq(TOPIC, 5), "yes", ge(COUNTMONEY, 5), eq(AMOUNT, 6)}, {setPrice(5), "Ok, here we go ... 6! You have won! How lucky you are! So listen ...<tells you what you need to know>", setQuestValue(234, add(questValue(234), 1)), setQuestValue(240, 1)}},

	{{eq(TOPIC, 5), ge(COUNTMONEY, 5)}, {setPrice(5), "Ok, and its ... %A! You have lost. He he. Another game?", deleteMoney, setTopic(6)}},
	{{eq(TOPIC, 5), lt(COUNTMONEY, 5)}, {"I am sorry, but you don't have so much money."}},

	{{eq(TOPIC, 6), "yes"}, {"Ok, no weights in the dice, no dirty tricks, are you ready?", setAmount(random(1, 6)), setTopic(7)}},
	{{eq(TOPIC, 6)}, {"This way you'll never get my measurements."}},

	{{eq(TOPIC, 7), "yes", ge(COUNTMONEY, 5), eq(AMOUNT, 6)}, {setPrice(5), "Ok, here we go ... 6! You have won! How lucky you are! So listen ...<tells you what you need to know>", setQuestValue(234, add(questValue(234), 1)), setQuestValue(240, 1)}},

	{{eq(TOPIC, 7), "no"}, {"This way you'll never get my measurements."}},

	{{eq(TOPIC, 7), ge(COUNTMONEY, 5)}, {setPrice(5), "Ok, and its ... %A! You have lost. He he. Another game?", deleteMoney, setTopic(6)}},
	{{eq(TOPIC, 7), lt(COUNTMONEY, 5)}, {"I am sorry, but you don't have so much money."}},

	{{"job"}, {"I am a servant of the ambassador and running the post office. Ask me if you have questions about the Royal Tibia Mail System."}},
	{{"name"}, {"My name is Olrik."}},
	{{"time"}, {"It's %T."}},

	{{"elves"}, {"What a noble and graceful race."}},
	{{"dwarfs"}, {"Uhm, let's say I prefer the company of elves."}},
	{{"carlin"}, {"A city full of women who surly only see whimps instead of real men like me has some appeal."}},
	{{"venore"}, {"Those generous merchants are charming people. I am always looking forward their visits."}},
	{{"humans"}, {"I feel so clumsy around those elves."}},
	{{"troll"}, {"What nasty creatures."}},
	{{"cenath"}, {"They are so wise and have an aura of mystic around them."}},
	{{"kuridai"}, {"They are so diligent in the things they do and such awesome fighters."}},
	{{"deraisim"}, {"They are so familiar with the woods and move with unparalleled grace."}},
	{{"abdaisim"}, {"I look forward to meet them one day."}},
	{{"teshial"}, {"I wonder where they have gone."}},
	{{"ferumbras"}, {"I heared he is dead, which is a good thing."}},
	{{"crunor"}, {"I honor all gods."}},
	{{"tibianus"}, {"He is our beloved ruler."}},
	{{"roderick"}, {"The ambassador is the best choice for this position of responsibility."}},
	{{"excalibug"}, {"Pardon?"}},
	{{"news"}, {"We learn few important things about the tides of time here."}},
	{{"magic"}, {"You should talk to the elves about that."}},

	genPost,

	--"mail"          -> "Our mail system is unique! And everyone can use it. Do you want to know more about it?", Topic=1
	--Topic=1,"yes"   -> "The Tibia Mail System enables you to send and receive letters and parcels. You can buy them here if you want."
	--Topic=1         -> "Is there anything else I can do for you?"

	--"letter" -> Amount=1, Price=5,  "Do you want to buy a letter for %P gold?", Topic=2
	--"parcel" -> Amount=1, Price=10, "Do you want to buy a parcel for %P gold?", Topic=3

	--Topic=2,"yes",CountMoney>=Price -> "Here it is. Don't forget to write the name of the receiver in the first line and the address in the second one before you put the letter in a mailbox.", DeleteMoney, Create(3505)
	--Topic=2,"yes"                   -> "Oh, you do not have enough gold to buy a letter."
	--Topic=2                         -> "Ok."

	--Topic=3,"yes",CountMoney>=Price -> "Here you are. Don't forget to write the name and the address of the receiver on the label. The label has to be in the parcel before you put the parcel in a mailbox.", DeleteMoney, Create(3503), Create(3507)
	--Topic=3,"yes"                   -> "Oh, you do not have enough gold to buy a parcel."
	--Topic=3                         -> "Ok."
}

function onCreatureSay(npc, creature, text, situation)
	behaviour.react(npc, rules, text, situation)
end

function onCreatureDisappear(npc, creature)
	behaviour.react(npc, rules, "", NPC_SITUATION_VANISH)
end
