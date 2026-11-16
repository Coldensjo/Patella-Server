-- npcs/norbert.lua: the behaviour of the NPC Norbert, converted from
-- norbert.npc by patella-convert. The NPC is its [[npc]] entry in npcs.toml;
-- lib/npc.lua matches and runs the rules below, in the order of the file
-- (scripting.hh, "NPCs"). The comments are those of norbert.npc.
--
-- GIMUD - Graphical Interface Multi User Dungeon
-- norbert.npc: Datenbank für den Schneider Norbert

local behaviour = require("lib.npc")
local ADDRESS = behaviour.ADDRESS
local BUSY = behaviour.BUSY
local VANISH = behaviour.VANISH
local FORCE = behaviour.FORCE
local P1 = behaviour.P1
local eq = behaviour.eq
local lt = behaviour.lt
local gt = behaviour.gt
local ge = behaviour.ge
local TOPIC = behaviour.TOPIC
local PRICE = behaviour.PRICE
local TYPE = behaviour.TYPE
local LEVEL = behaviour.LEVEL
local COUNTMONEY = behaviour.COUNTMONEY
local mul = behaviour.mul
local setTopic = behaviour.setTopic
local setPrice = behaviour.setPrice
local setAmount = behaviour.setAmount
local setType = behaviour.setType
local create = behaviour.create
local deleteMoney = behaviour.deleteMoney
local queue = behaviour.queue
local idle = behaviour.idle
local nop = behaviour.nop
local previous = behaviour.previous

local rules = behaviour.rules {
	{{ADDRESS, "hello$", gt(LEVEL, 40), FORCE}, {"Wow! The great %N visiting our shop! LOOK PEOPLE, LOOK HERE!."}},
	{{ADDRESS, "hi$", gt(LEVEL, 40), FORCE}, {previous}},
	{{ADDRESS, "hello$", FORCE}, {"Welcome, %N."}},
	{{ADDRESS, "hi$", FORCE}, {previous}},
	{{ADDRESS, FORCE}, {idle}},
	{{BUSY, "hello$", FORCE}, {"Please give me another minute with this customer here, dear %N.", queue}},
	{{BUSY, "hi$", FORCE}, {previous}},
	{{BUSY, FORCE}, {nop}},
	{{VANISH, FORCE}, {"Bye, bye."}},

	{{"bye"}, {"Bye, bye.", idle}},
	{{"name"}, {"I am Norbert."}},
	{{"job"}, {"I am a salesperson here, but one day I might become a tailor, or a supermodel perhaps!"}},
	{{"time"}, {"Now it's %T. Did you notice this is a xelor watch I am wearing?"}},
	{{"xelor"}, {"Xelor, the dwarf of the chromancers guild, makes the most stylish watches in all the land."}},
	{{"king"}, {"Even the king of Thais, blessed be his name, can't buy a better wardrobe then ours."}},
	{{"tibianus"}, {previous}},
	{{"army"}, {"I don't think they dress that well."}},
	{{"ferumbras"}, {"Those evil mages dress so ugly."}},
	{{"excalibug"}, {"I fear such a weapon will ruin a silk shirt with one blow."}},
	{{"thais"}, {"Thaian wear is not that stylish anymore."}},
	{{"tibia"}, {"Our tailors are influenced by styles of the whole known world."}},
	{{"carlin"}, {"Women could do better then to wear armor. Women in leather scare my in particular."}},
	{{"hugo"}, {"He's our boss, a great tailor and designer."}},
	{{"chief"}, {previous}},
	{{"news"}, {"I heared the colour of the next season will be orange."}},
	{{"rumour"}, {previous}},
	{{"rumor"}, {previous}},

	{{"offer"}, {"I sell very stylish clothes indeed."}},
	{{"goods"}, {previous}},
	{{"do", "you", "sell"}, {previous}},
	{{"do", "you", "have"}, {previous}},
	{{"clothes"}, {"I have wonderful jackets, coats, lovely doublets, even warlike leather armor, and impressive studded armor."}},

	{{"coat"}, {setType(3562), setAmount(1), setPrice(8), "Do you want to buy a coat for %P gold?", setTopic(1)}},
	{{"jacket"}, {setType(3561), setAmount(1), setPrice(12), "Oh, do you want to buy one of my wonderful jackets for %P gold?", setTopic(1)}},
	{{"doublet"}, {setType(3379), setAmount(1), setPrice(16), "Do you want to buy a doublet for %P gold?", setTopic(1)}},
	{{"leather", "armor"}, {setType(3361), setAmount(1), setPrice(25), "Do you want to buy a leather armor for %P gold?", setTopic(1)}},
	{{"studded", "armor"}, {setType(3378), setAmount(1), setPrice(90), "Do you want to buy a studded armor for %P gold?", setTopic(1)}},

	{{P1, lt(1, P1), "coat"}, {setType(3562), setAmount(P1), setPrice(mul(8, P1)), "Do you want to buy %A coats for %P gold?", setTopic(1)}},
	{{P1, lt(1, P1), "jacket"}, {setType(3561), setAmount(P1), setPrice(mul(12, P1)), "Oh, do you want to buy %A of my wonderful jackets for %P gold?", setTopic(1)}},
	{{P1, lt(1, P1), "doublet"}, {setType(3379), setAmount(P1), setPrice(mul(16, P1)), "Do you want to buy %A doublets for %P gold?", setTopic(1)}},
	{{P1, lt(1, P1), "leather", "armor"}, {setType(3361), setAmount(P1), setPrice(mul(25, P1)), "Do you want to buy %A leather armors for %P gold?", setTopic(1)}},
	{{P1, lt(1, P1), "studded", "armor"}, {setType(3378), setAmount(P1), setPrice(mul(90, P1)), "Do you want to buy %A studded armors for %P gold?", setTopic(1)}},

	{{eq(TOPIC, 1), "yes", ge(COUNTMONEY, PRICE)}, {"And here you are.", deleteMoney, create(TYPE)}},
	{{eq(TOPIC, 1), "yes"}, {"Sorry, you do not have enough money."}},
	{{eq(TOPIC, 1)}, {"What a pity, perhaps next time."}},
}

function onCreatureSay(npc, creature, text, situation)
	behaviour.react(npc, rules, text, situation)
end

function onCreatureDisappear(npc, creature)
	behaviour.react(npc, rules, "", NPC_SITUATION_VANISH)
end
