-- npcs/livielle.lua: the behaviour of the NPC Livielle, converted from
-- livielle.npc by patella-convert. The NPC is its [[npc]] entry in npcs.toml;
-- lib/npc.lua matches and runs the rules below, in the order of the file
-- (scripting.hh, "NPCs"). The comments are those of livielle.npc.
--
-- GIMUD - Graphical Interface Multi User Dungeon
-- Livielle.npc: Datenbank für die Nahrungsmittelhändlerin Livielle

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
local idle = behaviour.idle
local nop = behaviour.nop
local previous = behaviour.previous

local rules = behaviour.rules {
	{{ADDRESS, "hello$", MALE, FORCE}, {"Ah, 'ello, %N! I can see you're longing for my delicious fruits, ch\233ri."}},
	{{ADDRESS, "hi$", MALE, FORCE}, {previous}},
	{{ADDRESS, "salut$", MALE, FORCE}, {previous}},
	{{ADDRESS, "hello$", FEMALE, FORCE}, {"Bienvenue, %N! My fruits will complete the icing on your cake."}},
	{{ADDRESS, "hi$", FEMALE, FORCE}, {previous}},
	{{ADDRESS, "salut$", FEMALE, FORCE}, {previous}},
	{{ADDRESS, FORCE}, {idle}},
	{{BUSY, "hello$", FORCE}, {"Don't push me, don't push me. One after another, %N."}},
	{{BUSY, "hi$", FORCE}, {previous}},
	{{BUSY, "salut$", FORCE}, {previous}},
	{{BUSY, FORCE}, {nop}},
	{{VANISH, FORCE}, {"Aww, I don't even deserve a farewell?"}},

	{{"bye"}, {"Bon app\233tit, and come back soon for your daily dose of vitamins!", idle}},
	{{"au", "revoir"}, {"Bon app\233tit, and come back soon for your daily dose of vitamins!", idle}},
	{{"job"}, {"Alors, guess what my job might be, standing 'ere in the middle of all these juicy exotic fruits?"}},
	{{"shop"}, {previous}},
	{{"name", MALE}, {"Moi? Livielle for you, ch\233ri. "}},
	{{"name", FEMALE}, {"I'm Livielle Delacroix, madame."}},
	{{"time"}, {"Time is %T now."}},
	{{"help"}, {"Oh, for sure will my fruits 'elp you driving off all these nasty diseases and strengthen your immune system!"}},
	{{"thanks"}, {"You're welcome, enjoy."}},
	{{"thank", "you"}, {previous}},

	{{"buy"}, {"What's your favorite flavour today? I offer all sorts of exotic fruits."}},
	{{"do", "you", "sell"}, {previous}},
	{{"do", "you", "have"}, {previous}},
	{{"offer"}, {previous}},
	{{"fruits"}, {"I offer you bananas, melons, pumpkins, white mushrooms, oranges, strawberries, and blueberries."}},

	{{"banana"}, {setType(3587), setAmount(1), setPrice(5), "Do you want to buy a banana for %P gold?", setTopic(1)}},
	{{"white", "mushroom"}, {setType(3723), setAmount(1), setPrice(10), "Do you want to buy one of the white mushrooms for %P gold?", setTopic(1)}},
	{{"orange"}, {setType(3586), setAmount(1), setPrice(10), "Do you want to buy an orange for %P gold?", setTopic(1)}},
	{{"strawberr"}, {setType(3591), setAmount(1), setPrice(2), "Do you want to buy a strawberry for %P gold?", setTopic(1)}},
	{{"melon"}, {setType(3593), setAmount(1), setPrice(10), "Do you want to buy a melon for %P gold?", setTopic(1)}},
	{{"pumpkin"}, {setType(3594), setAmount(1), setPrice(10), "Do you want to buy a pumpkin for %P gold?", setTopic(1)}},
	{{"blueberr"}, {setType(3588), setAmount(1), setPrice(1), "Do you want to buy a blueberry for %P gold?", setTopic(1)}},

	{{P1, lt(1, P1), "banana"}, {setType(3587), setAmount(P1), setPrice(mul(5, P1)), "Do you want to buy %A bananas for %P gold?", setTopic(1)}},
	{{P1, lt(1, P1), "white", "mushroom"}, {setType(3723), setAmount(P1), setPrice(mul(10, P1)), "Do you want to buy %A of the white mushrooms for %P gold?", setTopic(1)}},
	{{P1, lt(1, P1), "orange"}, {setType(3586), setAmount(P1), setPrice(mul(10, P1)), "Do you want to buy %A oranges for %P gold?", setTopic(1)}},
	{{P1, lt(1, P1), "strawberr"}, {setType(3591), setAmount(P1), setPrice(mul(2, P1)), "Do you want to buy %A strawberries for %P gold?", setTopic(1)}},
	{{P1, lt(1, P1), "melon"}, {setType(3593), setAmount(P1), setPrice(mul(10, P1)), "Do you want to buy %A melons for %P gold?", setTopic(1)}},
	{{P1, lt(1, P1), "pumpkin"}, {setType(3594), setAmount(P1), setPrice(mul(10, P1)), "Do you want to buy %A pumpkins for %P gold?", setTopic(1)}},
	{{P1, lt(1, P1), "blueberr"}, {setType(3588), setAmount(P1), setPrice(mul(1, P1)), "Do you want to buy %A blueberries for %P gold?", setTopic(1)}},

	{{eq(TOPIC, 1), "yes", ge(COUNTMONEY, PRICE)}, {"Merci, 'ere you go.", deleteMoney, create(TYPE)}},
	{{eq(TOPIC, 1), "yes"}, {"Sorry, but that's not enough money, please count again."}},
	{{eq(TOPIC, 1)}, {"You should really prefer my fruits over all this fat meat offered elsewhere. They keep you lithe and lissom."}},
}

function onCreatureSay(npc, creature, text, situation)
	behaviour.react(npc, rules, text, situation)
end

function onCreatureDisappear(npc, creature)
	behaviour.react(npc, rules, "", NPC_SITUATION_VANISH)
end
