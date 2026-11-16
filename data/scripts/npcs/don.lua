-- npcs/don.lua: the behaviour of the NPC Donald McRonald, converted from
-- don.npc by patella-convert. The NPC is its [[npc]] entry in npcs.toml;
-- lib/npc.lua matches and runs the rules below, in the order of the file
-- (scripting.hh, "NPCs"). The comments are those of don.npc.
--
-- GIMUD - Graphical Interface Multi User Dungeon
-- don.npc: Datenbank fuer den Bauern Donald McRonald

local behaviour = require("lib.npc")
local ADDRESS = behaviour.ADDRESS
local BUSY = behaviour.BUSY
local VANISH = behaviour.VANISH
local KNIGHT = behaviour.KNIGHT
local SORCERER = behaviour.SORCERER
local DRUID = behaviour.DRUID
local FORCE = behaviour.FORCE
local P1 = behaviour.P1
local eq = behaviour.eq
local lt = behaviour.lt
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
local idle = behaviour.idle
local nop = behaviour.nop
local previous = behaviour.previous

local rules = behaviour.rules {
	{{ADDRESS, "hello$", DRUID, FORCE}, {"Hello, Druid %N!"}},
	{{ADDRESS, "hi$", DRUID, FORCE}, {previous}},
	{{ADDRESS, "hello$", FORCE}, {"Hmmm, well, hello %N."}},
	{{ADDRESS, "hi$", FORCE}, {previous}},
	{{ADDRESS, FORCE}, {idle}},
	{{BUSY, "hello$", FORCE}, {"Hmm, I'm busy %N."}},
	{{BUSY, "hi$", FORCE}, {previous}},
	{{BUSY, FORCE}, {nop}},
	{{VANISH, FORCE}, {"?"}},

	{{"bye", DRUID}, {"May Crunor bless you, Druid %N!", idle}},
	{{"farewell", DRUID}, {previous}},
	{{"job", DRUID}, {"My wife and I run this farm as good as we can."}},
	{{"wife", DRUID}, {"Sherry is my beloved wife."}},
	{{"donald", DRUID}, {"I was named Donald, like my grandfather."}},
	{{"farm", DRUID}, {"It's a hard but rewarding task to run this farm."}},
	{{"name", DRUID}, {"My name is Donald McRonald, noble druid."}},
	{{"time", DRUID}, {"Unfortunately I can't help you with that, noble druid."}},

	{{"bye"}, {"Yes, bye!", idle}},
	{{"farewell"}, {"Yes, farewell!", idle}},
	{{"job"}, {"I run a farm, what else?!"}},
	{{"wife"}, {"Sherry is my wife."}},
	{{"donald"}, {"I am Donald."}},
	{{"farm"}, {"It is my farm, yes."}},
	{{"name"}, {"Donald McRonald."}},
	{{"time"}, {"Who cares?"}},
	{{"weather"}, {"Weather is good enough to work on the fields."}},
	{{"crops"}, {"It is hard to grow but worth the effort."}},
	{{"field"}, {"My fields are enchanted by the druids and the wheat grows very quickly."}},

	{{"city"}, {"The city is to the north."}},
	{{"crops"}, {"I take care of our crops"}},
	{{"mill"}, {"I somtimes have to bring the wheat there."}},
	{{"spooked"}, {"I dont know."}},
	{{"king"}, {"King Tibianus is our king."}},
	{{"frodo"}, {"Frodo? He is a friend of mine."}},
	{{"oswald"}, {"He ignores us and we ignore him."}},
	{{"bloodblade"}, {"A general in the army."}},
	{{"muriel", SORCERER}, {"I dont trust sorcerers like you."}},
	{{"muriel"}, {"I dont trust sorcerers."}},
	{{"elane"}, {"Too noble to care about us."}},
	{{"gregor"}, {"Knights always feel superior to us farmers."}},
	{{"gregor", KNIGHT}, {"Knights like you always feel superior to us farmers."}},
	{{"marvik"}, {"Druids are a great help for us, they know much about nature."}},
	{{"marvik", DRUID}, {"Druids like you are a great help for us, they know much about nature."}},
	{{"gorn"}, {"Hardly know him."}},
	{{"sam"}, {"A blacksmith, eh?"}},
	{{"quentin"}, {"A generous person."}},
	{{"lynda"}, {"She has a good soul."}},
	{{"spider"}, {setType(3988), setAmount(1), setPrice(2), "I will give you %P gold for every spider you bring me. But not a rotten spider that was already dead for some time. Do you have any with you?", setTopic(2)}},

	{{"buy"}, {"I can offer you wheat, cheese, carrots, and corncobs."}},
	{{"do", "you", "sell"}, {previous}},
	{{"do", "you", "have"}, {previous}},
	{{"food"}, {"Are you looking for food? I have wheat, cheese, carrots, and corn to sell. If you want to sell bread, talk to my wife, Sherry."}},
	{{"bread"}, {"If you want to sell bread, talk to my wife, Sherry."}},

	{{"wheat"}, {setType(3605), setAmount(1), setPrice(1), "Do you want to buy wheat for %P gold?", setTopic(1)}},
	{{"cheese"}, {setType(3607), setAmount(1), setPrice(5), "Do you want to buy cheese for %P gold?", setTopic(1)}},
	{{"carrot"}, {setType(3595), setAmount(1), setPrice(3), "Do you want to buy a carrot for %P gold?", setTopic(1)}},
	{{"corncob"}, {setType(3597), setAmount(1), setPrice(3), "Do you want to buy a corncob for %P gold?", setTopic(1)}},

	{{P1, lt(1, P1), "wheat"}, {setType(3605), setAmount(P1), setPrice(mul(1, P1)), "Do you want to buy %A wheat for %P gold?", setTopic(1)}},
	{{P1, lt(1, P1), "cheese"}, {setType(3607), setAmount(P1), setPrice(mul(5, P1)), "Do you want to buy %A cheeses for %P gold?", setTopic(1)}},
	{{P1, lt(1, P1), "carrot"}, {setType(3595), setAmount(P1), setPrice(mul(3, P1)), "Do you want to buy %A carrots for %P gold?", setTopic(1)}},
	{{P1, lt(1, P1), "corncob"}, {setType(3597), setAmount(P1), setPrice(mul(3, P1)), "Do you want to buy %A corncobs for %P gold?", setTopic(1)}},

	{{eq(TOPIC, 1), "yes", ge(COUNTMONEY, PRICE)}, {"Here it is.", deleteMoney, create(TYPE)}},
	{{eq(TOPIC, 1), "yes"}, {"I am sorry, but you do not have enough gold."}},
	{{eq(TOPIC, 1)}, {"Maybe later."}},

	{{eq(TOPIC, 2), "yes", ge(count(TYPE), AMOUNT)}, {"Here you are.", delete(TYPE), createMoney}},
	{{eq(TOPIC, 2), "yes"}, {"You have no spider that died recently."}},
	{{eq(TOPIC, 2)}, {"Hmpf."}},
}

function onCreatureSay(npc, creature, text, situation)
	behaviour.react(npc, rules, text, situation)
end

function onCreatureDisappear(npc, creature)
	behaviour.react(npc, rules, "", NPC_SITUATION_VANISH)
end
