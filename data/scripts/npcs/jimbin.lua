-- npcs/jimbin.lua: the behaviour of the NPC Jimbin, converted from
-- jimbin.npc by patella-convert. The NPC is its [[npc]] entry in npcs.toml;
-- lib/npc.lua matches and runs the rules below, in the order of the file
-- (scripting.hh, "NPCs"). The comments are those of jimbin.npc.
--
-- GIMUD - Graphical Interface Multi User Dungeon
-- jimbin.npc: Datenbank für den Wirt Jimbin

local behaviour = require("lib.npc")
local ADDRESS = behaviour.ADDRESS
local BUSY = behaviour.BUSY
local VANISH = behaviour.VANISH
local FORCE = behaviour.FORCE
local P1 = behaviour.P1
local eq = behaviour.eq
local lt = behaviour.lt
local ge = behaviour.ge
local TOPIC = behaviour.TOPIC
local PRICE = behaviour.PRICE
local TYPE = behaviour.TYPE
local COUNTMONEY = behaviour.COUNTMONEY
local mul = behaviour.mul
local setTopic = behaviour.setTopic
local setPrice = behaviour.setPrice
local setAmount = behaviour.setAmount
local setType = behaviour.setType
local setData = behaviour.setData
local create = behaviour.create
local deleteMoney = behaviour.deleteMoney
local queue = behaviour.queue
local idle = behaviour.idle
local nop = behaviour.nop
local previous = behaviour.previous

local rules = behaviour.rules {
	{{ADDRESS, "hello$", "jimbin", FORCE}, {"Welcome to the Jolly Axeman, have a good time, %N!"}},
	{{ADDRESS, "hi$", "jimbin", FORCE}, {previous}},
	{{ADDRESS, "hello$", FORCE}, {"Talking to me, %N?", idle}},
	{{ADDRESS, "hi$", FORCE}, {previous}},
	{{ADDRESS, FORCE}, {idle}},
	{{BUSY, "hello$", "jimbin", FORCE}, {"Gimme a minute, %N.", queue}},
	{{BUSY, "hi$", "jimbin", FORCE}, {previous}},
	{{BUSY, "hello$", FORCE}, {"Talking to me, %N?"}},
	{{BUSY, "hi$", FORCE}, {previous}},
	{{BUSY, FORCE}, {nop}},
	{{VANISH, FORCE}, {"Come back if you enjoyed my tavern, if not ... well, get eaten by a dragon, jawoll."}},

	{{"bye"}, {"Come back if you enjoyed my tavern, if not ... well, get eaten by a dragon, jawoll.", idle}},
	{{"farewell"}, {previous}},
	{{"hello$", "maryza", FORCE}, {previous}},
	{{"hi$", "maryza", FORCE}, {previous}},
	{{"job"}, {"I'm runing the Jolly Axeman together with my wife Maryza."}},
	{{"tavern"}, {previous}},
	{{"maryza"}, {"She's a fine cook; likes it bloddy, though. Humans call her Bloody Mary, but don't mention that to her if you're smart."}},
	{{"name"}, {"I am Jimbin Luckythroat, son of Earth, from the Molten Rock."}},
	{{"time"}, {"It is about %T."}},
	{{"king"}, {"The king orders huge amounts of mushroombeer for festivities."}},
	{{"army"}, {"I supply the army with dwarfish beer to keep morals high."}},
	{{"ferumbras"}, {"Hah! He never dares to trespass our realm."}},
	{{"general"}, {"The general is a fine man. Can drink as much as he wants and still is sober."}},
	{{"excalibug"}, {"Actually I belive it's more than a taverntale."}},
	{{"thais"}, {"Bah! Humans, can't stand a drink, jawoll."}},
	{{"tibia"}, {"The Tibia our race was born into was even more fierce than the world you young ones know."}},
	{{"carlin"}, {"Silly town. Alcohol is forbidden there and elves visit this town quite often, what certainly suggests nothing good about a town, jawoll."}},
	{{"news"}, {"Oh well, many hidden places of ancient times appear seemingly out of nowhere in these times."}},
	{{"rumour"}, {previous}},
	{{"rumor"}, {previous}},
	{{"book"}, {"The cookbook? It belongs to maryza. I think she has a few copies for sale."}},
	{{"cookbook"}, {previous}},

	{{"beer"}, {setType(2880), setData(3), setAmount(1), setPrice(2), "Do you want to buy a mug of beer for %P gold?", setTopic(1)}},
	{{"water"}, {setType(2880), setData(1), setAmount(1), setPrice(1), "Do you want to buy a mug of water for %P gold?", setTopic(1)}},

	{{P1, lt(1, P1), "beer"}, {setType(2880), setData(3), setAmount(P1), setPrice(mul(2, P1)), "Do you want to buy %A mugs of beer for %P gold?", setTopic(1)}},
	{{P1, lt(1, P1), "water"}, {setType(2880), setData(1), setAmount(P1), setPrice(mul(1, P1)), "Do you want to buy %A mugs of water for %P gold?", setTopic(1)}},

	{{eq(TOPIC, 1), "yes", ge(COUNTMONEY, PRICE)}, {"Here it is.", deleteMoney, create(TYPE)}},
	{{eq(TOPIC, 1), "yes"}, {"I am sorry, but you do not have enough gold."}},
	{{eq(TOPIC, 1)}, {"Maybe later."}},

	{{"buy"}, {"I can offer you beer ... or water if you are sick."}},
	{{"do", "you", "sell"}, {previous}},
	{{"do", "you", "have"}, {previous}},
	{{"food"}, {"Ask my wife Maryza for food."}},
}

function onCreatureSay(npc, creature, text, situation)
	behaviour.react(npc, rules, text, situation)
end

function onCreatureDisappear(npc, creature)
	behaviour.react(npc, rules, "", NPC_SITUATION_VANISH)
end
