-- npcs/rodney.lua: the behaviour of the NPC Rodney, converted from
-- rodney.npc by patella-convert. The NPC is its [[npc]] entry in npcs.toml;
-- lib/npc.lua matches and runs the rules below, in the order of the file
-- (scripting.hh, "NPCs"). The comments are those of rodney.npc.
--
-- GIMUD - Graphical Interface Multi User Dungeon
-- rodney.npc: Datenbank für den Lebensmittelhändler Rodney

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
local create = behaviour.create
local deleteMoney = behaviour.deleteMoney
local queue = behaviour.queue
local idle = behaviour.idle
local nop = behaviour.nop
local previous = behaviour.previous

local rules = behaviour.rules {
	{{ADDRESS, "hello$", FORCE}, {"Greetings, %N. Can I help you?"}},
	{{ADDRESS, "hi$", FORCE}, {previous}},
	{{ADDRESS, FORCE}, {idle}},
	{{BUSY, "hello$", FORCE}, {"Just a moment, %N.", queue}},
	{{BUSY, "hi$", FORCE}, {previous}},
	{{BUSY, FORCE}, {nop}},
	{{VANISH, FORCE}, {"Good bye, %N."}},

	{{"bye"}, {"Good bye, %N.", idle}},
	{{"name"}, {"My name is Rodney."}},
	{{"job"}, {"I sell some local fruits."}},
	{{"time"}, {"It is %T right now."}},
	{{"king"}, {"King Tibianus! May the gods bless him!"}},
	{{"tibianus"}, {previous}},
	{{"army"}, {"I am glad that the Thaian garrison is here to protect us."}},
	{{"ferumbras"}, {"Sounds like the name of a foreign soup to me."}},
	{{"excalibug"}, {"What is that? Is it tasty?"}},
	{{"thais"}, {"The Thaian protectorate serves our beloved Venore well."}},
	{{"tibia"}, {"I have seen only little of it."}},
	{{"carlin"}, {"We have little contact with far Carlin, but I heared some renegade Amazons are terrorizing the area."}},
	{{"edron"}, {"Our climate is quite rough, so we can only grow wheat here, but no fruits."}},
	{{"news"}, {"Rumour goes that some amazons, who where banished from Carlin, took refuge near the swamps."}},
	{{"rumour"}, {previous}},
	{{"rumor"}, {previous}},
	{{"amazon"}, {previous}},

	{{"buy"}, {"I can offer some of the local fruits."}},
	{{"do", "you", "sell"}, {previous}},
	{{"do", "you", "have"}, {previous}},
	{{"offer"}, {previous}},
	{{"food"}, {previous}},
	{{"fruit"}, {"I have apples, cherries, grapes, and pears. What would you like?"}},

	{{"grape"}, {setType(3592), setAmount(1), setPrice(3), "Do you want to buy grapes for %P gold?", setTopic(1)}},
	{{"cherry"}, {setType(3590), setAmount(1), setPrice(1), "Do you want to buy a cherry for %P gold?", setTopic(1)}},
	{{"apple"}, {setType(3585), setAmount(1), setPrice(3), "Do you want to buy an apple for %P gold?", setTopic(1)}},
	{{"pear"}, {setType(3584), setAmount(1), setPrice(4), "Do you want to buy a pear for %P gold?", setTopic(1)}},

	{{P1, lt(1, P1), "grape"}, {setType(3592), setAmount(P1), setPrice(mul(3, P1)), "Do you want to buy %A grapes for %P gold?", setTopic(1)}},
	{{P1, lt(1, P1), "cherry"}, {setType(3590), setAmount(P1), setPrice(mul(1, P1)), "Do you want to buy %A cherries for %P gold?", setTopic(1)}},
	{{P1, lt(1, P1), "cherries"}, {setType(3590), setAmount(P1), setPrice(mul(1, P1)), "Do you want to buy %A cherries for %P gold?", setTopic(1)}},
	{{P1, lt(1, P1), "apple"}, {setType(3585), setAmount(P1), setPrice(mul(3, P1)), "Do you want to buy %A apples for %P gold?", setTopic(1)}},
	{{P1, lt(1, P1), "pear"}, {setType(3584), setAmount(P1), setPrice(mul(4, P1)), "Do you want to buy %A pears for %P gold?", setTopic(1)}},

	{{eq(TOPIC, 1), "yes", ge(COUNTMONEY, PRICE)}, {"Here.", deleteMoney, create(TYPE)}},
	{{eq(TOPIC, 1), "yes"}, {"I'm sorry, you are out of funds."}},
	{{eq(TOPIC, 1)}, {"Too bad."}},
}

function onCreatureSay(npc, creature, text, situation)
	behaviour.react(npc, rules, text, situation)
end

function onCreatureDisappear(npc, creature)
	behaviour.react(npc, rules, "", NPC_SITUATION_VANISH)
end
