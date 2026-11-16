-- npcs/omur.lua: the behaviour of the NPC Omur, converted from
-- omur.npc by patella-convert. The NPC is its [[npc]] entry in npcs.toml;
-- lib/npc.lua matches and runs the rules below, in the order of the file
-- (scripting.hh, "NPCs"). The comments are those of omur.npc.
--
-- GIMUD - Graphical Interface Multi User Dungeon
-- Omur.npc: Datenbank für den Gemüsehändler Omur

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
	{{ADDRESS, "hello$", FORCE}, {"Welcome at the humble booth of Omur, %N."}},
	{{ADDRESS, "hi$", FORCE}, {previous}},
	{{ADDRESS, FORCE}, {idle}},
	{{BUSY, "hello$", FORCE}, {"Please give me a minute to finish this deal, %N.", queue}},
	{{BUSY, "hi$", FORCE}, {previous}},
	{{BUSY, FORCE}, {nop}},
	{{VANISH, FORCE}, {"May your soul flourish like dunegrass after a rainfall."}},

	{{"bye"}, {"May your soul flourish like dunegrass after a rainfall.", idle}},
	{{"name"}, {"I am called Omur."}}, --## Ibn ??? ###
	{{"job"}, {"I sell rare fruits and vegatables from our lands and distant places."}},
	{{"time"}, {"Don't become a slave of a watch."}},
	{{"caliph"}, {"Ah, Caliph Kazzan; thrice praised be his name. May his life be as long as the beard of the king of all djinns."}},
	{{"kazzan"}, {previous}},
	{{"ferumbras"}, {"I think I have heard a traveller from the west mention that name."}},
	{{"excalibug"}, {"Is that the name of a djinn?"}},
	{{"thais"}, {"We import some goods from there in exchange for ours."}},
	{{"tibia"}, {"The world is nothing but a vain seduction."}},
	{{"carlin"}, {"I know almost nothing about that town. It must be exotic and entertaining. A place of distractions from the true path."}},
	{{"news"}, {"Sometimes the desertwind carries the crys and mourning of the tortured souls from Drefia far into the desert."}},
	{{"rumour"}, {previous}},
	{{"desert"}, {"It's not called the Devourer for nothing."}},

	{{"do", "you", "sell"}, {"I can offer you fruits and vegetables."}},
	{{"do", "you", "have"}, {previous}},
	{{"offer"}, {previous}},
	{{"food"}, {previous}},
	{{"fruit"}, {"I have oranges, bananas, grapes, and melons. What do you want?"}},
	{{"vegetable"}, {"I have carrots, pumpkins and tomatoes. What do you want?"}},

	{{"orange"}, {setType(3586), setAmount(1), setPrice(7), "Do you want to buy an orange for %P gold?", setTopic(1)}},
	{{"banana"}, {setType(3587), setAmount(1), setPrice(3), "Do you want to buy a banana for %P gold?", setTopic(1)}},
	{{"grape"}, {setType(3592), setAmount(1), setPrice(5), "Do you want to buy grapes for %P gold?", setTopic(1)}},
	{{"melon"}, {setType(3593), setAmount(1), setPrice(10), "Do you want to buy a melon for %P gold?", setTopic(1)}},
	{{"carrot"}, {setType(3595), setAmount(1), setPrice(4), "Do you want to buy a carrot for %P gold?", setTopic(1)}},
	{{"tomato"}, {setType(3596), setAmount(1), setPrice(5), "Do you want to buy a tomato for %P gold?", setTopic(1)}},
	{{"pumpkin"}, {setType(3594), setAmount(1), setPrice(10), "Do you want to buy a pumpkin for %P gold?", setTopic(1)}},

	{{P1, lt(1, P1), "orange"}, {setType(3586), setAmount(P1), setPrice(mul(7, P1)), "Do you want to buy %A oranges for %P gold?", setTopic(1)}},
	{{P1, lt(1, P1), "banana"}, {setType(3587), setAmount(P1), setPrice(mul(3, P1)), "Do you want to buy %A bananas for %P gold?", setTopic(1)}},
	{{P1, lt(1, P1), "grape"}, {setType(3592), setAmount(P1), setPrice(mul(5, P1)), "Do you want to buy %A grapes for %P gold?", setTopic(1)}},
	{{P1, lt(1, P1), "melon"}, {setType(3593), setAmount(P1), setPrice(mul(10, P1)), "Do you want to buy %A melons for %P gold?", setTopic(1)}},
	{{P1, lt(1, P1), "carrot"}, {setType(3595), setAmount(P1), setPrice(mul(4, P1)), "Do you want to buy %A carrots for %P gold?", setTopic(1)}},
	{{P1, lt(1, P1), "tomato"}, {setType(3596), setAmount(P1), setPrice(mul(5, P1)), "Do you want to buy %A tomatos for %P gold?", setTopic(1)}},
	{{P1, lt(1, P1), "pumpkin"}, {setType(3594), setAmount(P1), setPrice(mul(10, P1)), "Do you want to buy %A pumpkins for %P gold?", setTopic(1)}},

	{{eq(TOPIC, 1), "yes", ge(COUNTMONEY, PRICE)}, {"Here you are.", deleteMoney, create(TYPE)}},
	{{eq(TOPIC, 1), "yes"}, {"Oh, I'm sorry, but I can't give you credit."}},
	{{eq(TOPIC, 1)}, {"Don't you like my wares?"}},
}

function onCreatureSay(npc, creature, text, situation)
	behaviour.react(npc, rules, text, situation)
end

function onCreatureDisappear(npc, creature)
	behaviour.react(npc, rules, "", NPC_SITUATION_VANISH)
end
