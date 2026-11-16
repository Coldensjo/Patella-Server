-- npcs/bonifacius.lua: the behaviour of the NPC Bonifacius, converted from
-- bonifacius.npc by patella-convert. The NPC is its [[npc]] entry in npcs.toml;
-- lib/npc.lua matches and runs the rules below, in the order of the file
-- (scripting.hh, "NPCs"). The comments are those of bonifacius.npc.
--
-- GIMUD - Graphical Interface Multi User Dungeon
-- bonifacius.npc: Datenbank für den Lebensmittelhändler Bonifacius

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
	{{ADDRESS, "hello$", FORCE}, {"Thousands greetings, %N. How may I help you?"}},
	{{ADDRESS, "hi$", FORCE}, {previous}},
	{{ADDRESS, FORCE}, {idle}},
	{{BUSY, "hello$", FORCE}, {"I am deeply sorry, I am busy right now. I'll tell you when I'm done %N.", queue}},
	{{BUSY, "hi$", FORCE}, {previous}},
	{{BUSY, FORCE}, {nop}},
	{{VANISH, FORCE}, {"May the gods bless your travels."}},

	{{"bye"}, {"May the gods bless your travels.", idle}},
	{{"name"}, {"My name is Bonifacius."}},
	{{"job"}, {"I sell delicious food. May I be at your service?"}},
	{{"time"}, {"It is %T right now."}},
	{{"king"}, {"Our wise king, Tibianus, be praised!"}},
	{{"tibianus"}, {previous}},
	{{"army"}, {"I am glad about their healthy appetite."}},
	{{"ferumbras"}, {"Is that a new, exotic vegetable?"}},
	{{"excalibug"}, {"Uh, I hate bugs of all kind."}},
	{{"thais"}, {"We recive food from thais with every arriving ship."}},
	{{"tibia"}, {"The world provides us with all kinds of delicious food."}},
	{{"carlin"}, {"We do not buy any wares there. Our food is of high quality, Thaian origin."}},
	{{"edron"}, {"Our climate is quite rough, so we can only grow wheat here, but no fruits."}},
	{{"news"}, {"I heard the corn prices in Thais are going to be increased."}},
	{{"rumors"}, {previous}},

	{{"buy"}, {"I can offer you meat, salmons, fruits, cookies, rolls, eggs, and cheese."}},
	{{"do", "you", "sell"}, {previous}},
	{{"do", "you", "have"}, {previous}},
	{{"offer"}, {previous}},
	{{"food"}, {previous}},
	{{"fruit"}, {"I have oranges, bananas, grapes, pumpkins and melons. What do you want?"}},

	{{"meat"}, {setType(3577), setAmount(1), setPrice(5), "Do you want to buy meat for %P gold?", setTopic(1)}},
	{{"salmon"}, {setType(3579), setAmount(1), setPrice(4), "Do you want to buy a salmon for %P gold?", setTopic(1)}},
	{{"orange"}, {setType(3586), setAmount(1), setPrice(5), "Do you want to buy an orange for %P gold?", setTopic(1)}},
	{{"banana"}, {setType(3587), setAmount(1), setPrice(2), "Do you want to buy a banana for %P gold?", setTopic(1)}},
	{{"grape"}, {setType(3592), setAmount(1), setPrice(3), "Do you want to buy grapes for %P gold?", setTopic(1)}},
	{{"melon"}, {setType(3593), setAmount(1), setPrice(8), "Do you want to buy a melon for %P gold?", setTopic(1)}},
	{{"pumpkin"}, {setType(3594), setAmount(1), setPrice(10), "Do you want to buy a pumpkin for %P gold?", setTopic(1)}},
	{{"cookie"}, {setType(3598), setAmount(1), setPrice(2), "Do you want to buy a cookie for %P gold?", setTopic(1)}},
	{{"roll"}, {setType(3601), setAmount(1), setPrice(2), "Do you want to buy a roll for %P gold?", setTopic(1)}},
	{{"egg"}, {setType(3606), setAmount(1), setPrice(2), "Do you want to buy an egg for %P gold?", setTopic(1)}},
	{{"cheese"}, {setType(3607), setAmount(1), setPrice(5), "Do you want to buy cheese for %P gold?", setTopic(1)}},

	{{P1, lt(1, P1), "meat"}, {setType(3577), setAmount(P1), setPrice(mul(5, P1)), "Do you want to buy %A meat for %P gold?", setTopic(1)}},
	{{P1, lt(1, P1), "salmon"}, {setType(3579), setAmount(P1), setPrice(mul(4, P1)), "Do you want to buy %A salmon for %P gold?", setTopic(1)}},
	{{P1, lt(1, P1), "orange"}, {setType(3586), setAmount(P1), setPrice(mul(5, P1)), "Do you want to buy %A oranges for %P gold?", setTopic(1)}},
	{{P1, lt(1, P1), "banana"}, {setType(3587), setAmount(P1), setPrice(mul(2, P1)), "Do you want to buy %A bananas for %P gold?", setTopic(1)}},
	{{P1, lt(1, P1), "grape"}, {setType(3592), setAmount(P1), setPrice(mul(3, P1)), "Do you want to buy %A grapes for %P gold?", setTopic(1)}},
	{{P1, lt(1, P1), "melon"}, {setType(3593), setAmount(P1), setPrice(mul(8, P1)), "Do you want to buy %A melons for %P gold?", setTopic(1)}},
	{{P1, lt(1, P1), "pumpkin"}, {setType(3594), setAmount(P1), setPrice(mul(10, P1)), "Do you want to buy %A pumpkins for %P gold?", setTopic(1)}},
	{{P1, lt(1, P1), "cookie"}, {setType(3598), setAmount(P1), setPrice(mul(2, P1)), "Do you want to buy %A cookies for %P gold?", setTopic(1)}},
	{{P1, lt(1, P1), "roll"}, {setType(3601), setAmount(P1), setPrice(mul(2, P1)), "Do you want to buy %A rolls for %P gold?", setTopic(1)}},
	{{P1, lt(1, P1), "egg"}, {setType(3606), setAmount(P1), setPrice(mul(2, P1)), "Do you want to buy %A eggs for %P gold?", setTopic(1)}},
	{{P1, lt(1, P1), "cheese"}, {setType(3607), setAmount(P1), setPrice(mul(5, P1)), "Do you want to buy %A cheeses for %P gold?", setTopic(1)}},

	{{eq(TOPIC, 1), "yes", ge(COUNTMONEY, PRICE)}, {"Here you are.", deleteMoney, create(TYPE)}},
	{{eq(TOPIC, 1), "yes"}, {"Oh, I'm sorry, but I can't give you credit."}},
	{{eq(TOPIC, 1)}, {"Don't you like my wares?."}},
}

function onCreatureSay(npc, creature, text, situation)
	behaviour.react(npc, rules, text, situation)
end

function onCreatureDisappear(npc, creature)
	behaviour.react(npc, rules, "", NPC_SITUATION_VANISH)
end
