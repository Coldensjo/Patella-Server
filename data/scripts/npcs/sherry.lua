-- npcs/sherry.lua: the behaviour of the NPC Sherry McRonald, converted from
-- sherry.npc by patella-convert. The NPC is its [[npc]] entry in npcs.toml;
-- lib/npc.lua matches and runs the rules below, in the order of the file
-- (scripting.hh, "NPCs"). The comments are those of sherry.npc.
--
-- GIMUD - Graphical Interface Multi User Dungeon
-- sherry.npc: Datenbank für die Bäuerin Sherry McRonald

local behaviour = require("lib.npc")
local ADDRESS = behaviour.ADDRESS
local BUSY = behaviour.BUSY
local VANISH = behaviour.VANISH
local MALE = behaviour.MALE
local FEMALE = behaviour.FEMALE
local FORCE = behaviour.FORCE
local P1 = behaviour.P1
local eq = behaviour.eq
local lt = behaviour.lt
local gt = behaviour.gt
local ge = behaviour.ge
local TOPIC = behaviour.TOPIC
local PRICE = behaviour.PRICE
local AMOUNT = behaviour.AMOUNT
local TYPE = behaviour.TYPE
local COUNTMONEY = behaviour.COUNTMONEY
local count = behaviour.count
local mul = behaviour.mul
local setTopic = behaviour.setTopic
local setPrice = behaviour.setPrice
local setAmount = behaviour.setAmount
local setType = behaviour.setType
local create = behaviour.create
local delete = behaviour.delete
local createMoney = behaviour.createMoney
local deleteMoney = behaviour.deleteMoney
local queue = behaviour.queue
local idle = behaviour.idle
local nop = behaviour.nop
local previous = behaviour.previous

local rules = behaviour.rules {
	{{ADDRESS, "hello$", FORCE}, {"Hello %N! Welcome to our humble farm."}},
	{{ADDRESS, "hi$", FORCE}, {previous}},
	{{ADDRESS, FORCE}, {idle}},
	{{BUSY, "hello$", FORCE}, {"Please wait a minute, %N.", queue}},
	{{BUSY, "hi$", FORCE}, {previous}},
	{{BUSY, FORCE}, {nop}},
	{{VANISH, FORCE}, {"What a strange person."}},

	{{"bye"}, {"Grace our home with another visit soon.", idle}},
	{{"farewell"}, {previous}},
	{{"job"}, {"I and my husband run this farm."}},
	{{"husband"}, {"My husband Donald is busy on the fields almost all night and day."}},
	{{"donald"}, {previous}},
	{{"farm"}, {"It is a hard work, but the city needs us."}},
	{{"name"}, {"I am Sherry McRonald."}},
	{{"time"}, {"Sorry, I don't have a watch."}},
	{{"weather"}, {"The weather is the best friend and the worst enemy of a farmer."}},
	{{"field"}, {"The druids helped us by placing a blessing on our fields."}},
	{{"city"}, {"The city needs our crops."}},
	{{"crops"}, {"It's hard to harvest it, carry it to the mill in the north and make flour. If you can bake some bread I will buy it for 2 gold."}},
	{{"mill"}, {"The miller is a lazy fellow and afraid of his own mill, because he thinks it is spooked."}},
	{{"spooked"}, {"I don't know for sure. The miller claims that his mill is threatened by some monsters sometimes."}},
	{{"king"}, {"King Tibianus granted us this farm to earn a living."}},
	{{"tibianus"}, {previous}},
	{{"frodo"}, {"He is a friend of my husband."}},
	{{"oswald"}, {"This lazy fellow has nothing better to do than to spread rumours."}},
	{{"bloodblade"}, {"He is an impressive warrior as far as I can tell."}},
	{{"muriel"}, {"We a mere peasants and don't know much about the guild leaders."}},
	{{"elane"}, {previous}},
	{{"gregor"}, {previous}},
	{{"marvik"}, {previous}},
	{{"gorn"}, {"He doesn't talk much to us."}},
	{{"sam"}, {"He is too busy to care much about farmers like us."}},
	{{"quentin"}, {"What a nice person he is."}},
	{{"lynda"}, {"She is sooo charming. I can't believe she is not married yet! Have you met her?", setTopic(2)}},
	{{"spider"}, {"Spiders infested the sewers beneath our farm. We need some help to exterminate them. My husband pays a reward for killed spiders."}},
	{{"monster"}, {previous}},
	{{"help"}, {previous}},

	{{"buy"}, {"I can offer you cheese, cherries, and melons."}},
	{{"do", "you", "sell"}, {previous}},
	{{"do", "you", "have"}, {previous}},
	{{"food"}, {"Are you looking for food? I have cheese, cherries, pumpkins and melons."}},

	{{"cheese"}, {setType(3607), setAmount(1), setPrice(5), "Do you want to buy cheese for %P gold?", setTopic(1)}},
	{{"cherry"}, {setType(3590), setAmount(1), setPrice(1), "Do you want to buy a cherry for %P gold?", setTopic(1)}},
	{{"melon"}, {setType(3593), setAmount(1), setPrice(8), "Do you want to buy a melon for %P gold?", setTopic(1)}},
	{{"pumpkin"}, {setType(3594), setAmount(1), setPrice(10), "Do you want to buy a pumpkin for %P gold?", setTopic(1)}},

	{{P1, lt(1, P1), "cheese"}, {setType(3607), setAmount(P1), setPrice(mul(5, P1)), "Do you want to buy %A cheeses for %P gold?", setTopic(1)}},
	{{P1, lt(1, P1), "cherr"}, {setType(3590), setAmount(P1), setPrice(mul(1, P1)), "Do you want to buy %A cherries for %P gold?", setTopic(1)}},
	{{P1, lt(1, P1), "melon"}, {setType(3593), setAmount(P1), setPrice(mul(8, P1)), "Do you want to buy %A melons for %P gold?", setTopic(1)}},
	{{P1, lt(1, P1), "pumpkin"}, {setType(3594), setAmount(P1), setPrice(mul(10, P1)), "Do you want to buy %A pumpkins for %P gold?", setTopic(1)}},

	{{"sell", "bread"}, {"I will pay 2 gold for every bread, is that ok?", setTopic(3)}},

	{{eq(TOPIC, 1), "yes", ge(COUNTMONEY, PRICE)}, {"Here it is.", deleteMoney, create(TYPE)}},
	{{eq(TOPIC, 1), "yes"}, {"I am sorry, but you do not have enough gold."}},
	{{eq(TOPIC, 1)}, {"Maybe later."}},

	{{eq(TOPIC, 2), "yes", MALE}, {"She really should find a husband with ease! You should ask her for a date."}},
	{{eq(TOPIC, 2), "yes", FEMALE}, {"She really should find a husband with ease! If you know a bachelor, introduce him to her."}},
	{{eq(TOPIC, 2)}, {"Oh, if you say so."}},

	{{eq(TOPIC, 3), "yes", gt(count(3600), 0)}, {setAmount(count(3600)), setPrice(mul(AMOUNT, 2)), "Here you are ... %P gold.", delete(3600), createMoney}},
	{{eq(TOPIC, 3), "yes"}, {"Sorry, you don't have any bread."}},
	{{eq(TOPIC, 3)}, {"Maybe another time."}},
}

function onCreatureSay(npc, creature, text, situation)
	behaviour.react(npc, rules, text, situation)
end

function onCreatureDisappear(npc, creature)
	behaviour.react(npc, rules, "", NPC_SITUATION_VANISH)
end
