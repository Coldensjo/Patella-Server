-- npcs/aldo.lua: the behaviour of the NPC Aldo, converted from
-- aldo.npc by patella-convert. The NPC is its [[npc]] entry in npcs.toml;
-- lib/npc.lua matches and runs the rules below, in the order of the file
-- (scripting.hh, "NPCs"). The comments are those of aldo.npc.
--
-- GIMUD - Graphical Interface Multi User Dungeon
-- aldo.npc: Datenbank für den Schuhverkäufer Aldo

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
	{{ADDRESS, "hello$", FORCE}, {"Just great, another ... 'customer'. Hello, %N."}},
	{{ADDRESS, "hi$", FORCE}, {previous}},
	{{ADDRESS, FORCE}, {idle}},
	{{BUSY, "hello$", FORCE}, {"Yes, yes, I can only talk to one after the other! You will have to wait, %N.", queue}},
	{{BUSY, "hi$", FORCE}, {previous}},
	{{BUSY, FORCE}, {nop}},
	{{VANISH, FORCE}, {"Yeah, get lost."}},

	{{"bye"}, {"That's music in my ears.", idle}},
	{{"name"}, {"I'm Aldo. No one calls me 'lucky Aldo' though, guess why!"}},
	{{"job"}, {"I am a salesman, I sell headgear ... uhm ... oh well, and shoes."}},
	{{"time"}, {"Is it time for lunch already? Hey, stop making fun of me!"}},
	{{"king"}, {"One day I will sell the king a pair of shoes made by me and will get out of that stinky hole I live in and my family will never find me. HE, HE!"}},
	{{"tibianus"}, {previous}},
	{{"army"}, {"So many feet ... so many ... a nightmare!"}},
	{{"ferumbras"}, {"Can't be worse than my wife."}},
	{{"wife"}, {"Leave me alone with her while I am working at least. My only pleasure around here!"}},
	{{"excalibug"}, {"I have other stuff to worry about, like paying my bills."}},
	{{"bill"}, {"Yes, I have to pay o lot of bills, and some georges, and a john, and several steves."}},
	{{"thais"}, {"I will never in my life make it there."}},
	{{"tibia"}, {"I doubt I will ever see much of it. It's like i am cursed to haunt this site here for the rest of my life."}},
	{{"carlin"}, {"A city ruled by women!? Could anything be worse?"}},
	{{"amazon"}, {"I heard that chicks wear some revealing pieces of armor!"}},
	{{"news"}, {"Hey, I am a man. Look for some women to share gossip."}},
	{{"rumour"}, {previous}},
	{{"rumor"}, {previous}},
	{{"hugo"}, {"My boss, an evil slaver of good people like me."}},

	{{"offer"}, {"I am damned to sell headgear, trousers, and shoes for the rest of my life."}},
	{{"goods"}, {previous}},
	{{"do", "you", "sell"}, {previous}},
	{{"do", "you", "have"}, {previous}},
	{{"headgear"}, {"We have leather helmets and studded helmets."}},
	{{"shoes"}, {"<sigh> We sell leather boots and sandals."}},
	{{"trouser"}, {"We offer leather legs and studded legs."}},

	{{"leather", "helmet"}, {setType(3355), setAmount(1), setPrice(12), "Do you want to buy a leather helmet for %P gold?", setTopic(1)}},
	{{"studded", "helmet"}, {setType(3376), setAmount(1), setPrice(63), "Do you want to buy a studded helmet for %P gold?", setTopic(1)}},
	{{"sandals"}, {setType(3551), setAmount(1), setPrice(2), "Do you want to buy one of my wonderful sandals for %P gold?", setTopic(1)}},
	{{"leather", "boot"}, {setType(3552), setAmount(1), setPrice(2), "Do you want to buy one of my wonderful leather boots for %P gold?", setTopic(1)}},
	{{"leather", "legs"}, {setType(3559), setAmount(1), setPrice(10), "Do you want to buy leather legs for %P gold?", setTopic(1)}},
	{{"studded", "legs"}, {setType(3362), setAmount(1), setPrice(60), "Do you want to buy studded legs for %P gold?", setTopic(1)}},

	{{P1, lt(1, P1), "leather", "helmet"}, {setType(3355), setAmount(P1), setPrice(mul(12, P1)), "Do you want to buy %A leather helmets for %P gold?", setTopic(1)}},
	{{P1, lt(1, P1), "studded", "helmet"}, {setType(3376), setAmount(P1), setPrice(mul(63, P1)), "Do you want to buy %A studded helmets for %P gold?", setTopic(1)}},
	{{P1, lt(1, P1), "sandals"}, {setType(3551), setAmount(P1), setPrice(mul(2, P1)), "Do you want to buy %A of my wonderful sandals for %P gold?", setTopic(1)}},
	{{P1, lt(1, P1), "leather", "boot"}, {setType(3552), setAmount(P1), setPrice(mul(2, P1)), "Do you want to buy %A of my wonderful leather boots for %P gold?", setTopic(1)}},
	{{P1, lt(1, P1), "leather", "legs"}, {setType(3559), setAmount(P1), setPrice(mul(10, P1)), "Do you want to buy %A leather legs for %P gold?", setTopic(1)}},
	{{P1, lt(1, P1), "studded", "legs"}, {setType(3362), setAmount(P1), setPrice(mul(60, P1)), "Do you want to buy %A studded legs for %P gold?", setTopic(1)}},

	{{eq(TOPIC, 1), "yes", ge(COUNTMONEY, PRICE)}, {"Here. I hope that's it now.", deleteMoney, create(TYPE)}},
	{{eq(TOPIC, 1), "yes"}, {"At last, someone poorer than me."}},
	{{eq(TOPIC, 1), "no"}, {"Good decision!"}},
}

function onCreatureSay(npc, creature, text, situation)
	behaviour.react(npc, rules, text, situation)
end

function onCreatureDisappear(npc, creature)
	behaviour.react(npc, rules, "", NPC_SITUATION_VANISH)
end
