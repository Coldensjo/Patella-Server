-- npcs/jezzara.lua: the behaviour of the NPC Jezzara, converted from
-- jezzara.npc by patella-convert. The NPC is its [[npc]] entry in npcs.toml;
-- lib/npc.lua matches and runs the rules below, in the order of the file
-- (scripting.hh, "NPCs"). The comments are those of jezzara.npc.
--
-- GIMUD - Graphical Interface Multi User Dungeon
-- jezzara.npc: Datenbank für die pyramidenhändlerin Jezzara

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
	{{ADDRESS, "hello$", FORCE}, {"Be mourned pilgrim in flesh."}},
	{{ADDRESS, "hi$", FORCE}, {"Be mourned pilgrim in flesh."}},
	{{ADDRESS, "be", "mourned", "pilgrim", "flesh", FORCE}, {"Be mourned pilgrim in flesh."}},
	{{ADDRESS, FORCE}, {idle}},

	{{BUSY, "hello$", FORCE}, {"Mourned %N, know that I am busy.", queue}},
	{{BUSY, "hi$", FORCE}, {previous}},
	{{BUSY, "be", "mourned", "pilgrim", "flesh", FORCE}, {previous}},
	{{BUSY, FORCE}, {nop}},
	{{VANISH, FORCE}, {"May enlightenment be your path."}},

	{{"bye"}, {"May enlightenment be your path.", idle}},
	{{"farewell"}, {previous}},
	{{"job"}, {"I sell food of various kinds."}},
	{{"name"}, {"I am the mourned Jezzara."}},
	{{"time"}, {"You can buy watches here in the markethall."}},
	{{"temple"}, {"The temple can be found in the northeast of the city."}},
	{{"pharaoh"}, {"The pharaoh is our godking and the founder of our religion."}},
	{{"oldpharaoh"}, {"He was entombed in undead state. It is said that this will finally teach him to to strive for ascension."}},
	{{"scarab"}, {"I am not afraid of something that attacks only my physical form. But they stay away from the city anyway."}},
	{{"chosen"}, {"The chosen are those who are granted undeath after a life of service to the pharaoh."}},
	{{"tibia"}, {"The world can be a dangerous place for the whole of the Akh'rah Uthun."}},
	{{"carlin"}, {"The cities of the Tibian continent have little contact with us."}},
	{{"thais"}, {previous}},
	{{"edron"}, {previous}},
	{{"venore"}, {previous}},
	{{"kazordoon"}, {"Dwarves are not really fond of the endless sands of the desert. I must say I can't blame them for it."}},
	{{"dwarves"}, {previous}},
	{{"dwarfes"}, {previous}},
	{{"ab'dendriel"}, {"Elves don't like this land too much so we have little contact with them."}},
	{{"elves"}, {previous}},
	{{"elfes"}, {previous}},
	{{"darama"}, {"This continent is very diverse. There are deserts and mountains as well as a large jungle region."}},
	{{"darashia"}, {"We are usually not allowed to travel that far without explicit permission, so we know this city mostly from the tales of travellers."}},
	{{"daraman"}, {"I know little of his teachings. The priests say his conclusions were inconsequent."}},
	{{"ankrahmun"}, {"This city is a safe haven that gives shelter from the dangers of the desert."}},

	{{"mortality"}, {"The priests teach that mortality is a curse. I find it hard to understand but the priests will know best."}},
	{{"false", "gods"}, {"As far as I understand the gods worshipped by other nations are nothing but imposters."}},

	{{"ascension"}, {"Ascension is difficult to achive. Too difficult to achieve as long as you are still alive."}},
	{{"Akh'rah", "Uthun"}, {"Well its just the Akh, the Rah and the Unthun."}},
	{{"Akh"}, {"That is the body."}},

	{{"undead"}, {"Those who follow the pharaoh might become undead one day."}},
	{{"undeath"}, {previous}},
	{{"Rah"}, {"The Rah is the spiritual part of a being."}},
	{{"uthun"}, {"The Uthun is the sentient part of all living things."}},
	{{"mourn"}, {"The priests say we are to be mourned while we are still alive."}},

	{{"arena"}, {"Sometimes spectacular battles are fought in the local arena."}},
	{{"palace"}, {"The palace is where the mighty pharaoh resides."}},

	{{"buy"}, {"I can offer you meat, ham, salmon, fish, fruits and vegetables."}},
	{{"do", "you", "sell"}, {previous}},
	{{"do", "you", "have"}, {previous}},
	{{"offer"}, {previous}},
	{{"food"}, {previous}},
	{{"fruit"}, {"I have oranges, bananas, grapes, pumpkins and melons. What do you want?"}},
	{{"vegetable"}, {"I have carrots and tomatoes. What do you want?"}},

	{{"dragon", "ham"}, {setType(3583), setAmount(1), setPrice(25), "Do you want to buy a dragon ham for %P gold?", setTopic(1)}},
	{{"meat"}, {setType(3577), setAmount(1), setPrice(10), "Do you want to buy meat for %P gold?", setTopic(1)}},
	{{"ham"}, {setType(3582), setAmount(1), setPrice(15), "Do you want to buy a ham for %P gold?", setTopic(1)}},
	{{"salmon"}, {setType(3579), setAmount(1), setPrice(7), "Do you want to buy a salmon for %P gold?", setTopic(1)}},
	{{"fish"}, {setType(3578), setAmount(1), setPrice(6), "Do you want to buy a fish for %P gold?", setTopic(1)}},

	{{P1, lt(1, P1), "dragon", "ham"}, {setType(3583), setAmount(P1), setPrice(mul(25, P1)), "Do you wanna buy %A dragon ham for %P gold?", setTopic(1)}},
	{{P1, lt(1, P1), "meat"}, {setType(3577), setAmount(P1), setPrice(mul(10, P1)), "Do you wanna buy %A meat for %P gold?", setTopic(1)}},
	{{P1, lt(1, P1), "ham"}, {setType(3582), setAmount(P1), setPrice(mul(15, P1)), "Do you wanna buy %A ham for %P gold?", setTopic(1)}},
	{{P1, lt(1, P1), "salmon"}, {setType(3579), setAmount(P1), setPrice(mul(7, P1)), "Do you want to buy %A salmon for %P gold?", setTopic(1)}},
	{{P1, lt(1, P1), "fish"}, {setType(3578), setAmount(P1), setPrice(mul(6, P1)), "Do you want to buy %A fish for %P gold?", setTopic(1)}},
	{{"fruit"}, {"I have oranges, bananas, grapes, and melons. What do you want?"}},
	{{"vegetable"}, {"I have carrots and tomatoes. What do you want?"}},

	{{"orange"}, {setType(3586), setAmount(1), setPrice(9), "Do you want to buy an orange for %P gold?", setTopic(1)}},
	{{"banana"}, {setType(3587), setAmount(1), setPrice(5), "Do you want to buy a banana for %P gold?", setTopic(1)}},
	{{"grape"}, {setType(3592), setAmount(1), setPrice(8), "Do you want to buy grapes for %P gold?", setTopic(1)}},
	{{"melon"}, {setType(3593), setAmount(1), setPrice(13), "Do you want to buy a melon for %P gold?", setTopic(1)}},
	{{"carrot"}, {setType(3595), setAmount(1), setPrice(8), "Do you want to buy a carrot for %P gold?", setTopic(1)}},
	{{"tomato"}, {setType(3596), setAmount(1), setPrice(10), "Do you want to buy a tomato for %P gold?", setTopic(1)}},
	{{"pumpkin"}, {setType(3594), setAmount(1), setPrice(10), "Do you want to buy a pumpkin for %P gold?", setTopic(1)}},

	{{P1, lt(1, P1), "orange"}, {setType(3586), setAmount(P1), setPrice(mul(9, P1)), "Do you want to buy %A oranges for %P gold?", setTopic(1)}},
	{{P1, lt(1, P1), "banana"}, {setType(3587), setAmount(P1), setPrice(mul(5, P1)), "Do you want to buy %A bananas for %P gold?", setTopic(1)}},
	{{P1, lt(1, P1), "grape"}, {setType(3592), setAmount(P1), setPrice(mul(8, P1)), "Do you want to buy %A grapes for %P gold?", setTopic(1)}},
	{{P1, lt(1, P1), "melon"}, {setType(3593), setAmount(P1), setPrice(mul(13, P1)), "Do you want to buy %A melons for %P gold?", setTopic(1)}},
	{{P1, lt(1, P1), "carrot"}, {setType(3595), setAmount(P1), setPrice(mul(8, P1)), "Do you want to buy %A carrots for %P gold?", setTopic(1)}},
	{{P1, lt(1, P1), "tomato"}, {setType(3596), setAmount(P1), setPrice(mul(10, P1)), "Do you want to buy %A tomatoes for %P gold?", setTopic(1)}},
	{{P1, lt(1, P1), "pumpkin"}, {setType(3594), setAmount(P1), setPrice(mul(10, P1)), "Do you want to buy %A pumpkins for %P gold?", setTopic(1)}},

	{{eq(TOPIC, 1), "yes", ge(COUNTMONEY, PRICE)}, {"Here you are.", deleteMoney, create(TYPE)}},
	{{eq(TOPIC, 1), "yes"}, {"Come back, when you have enough money."}},
	{{eq(TOPIC, 1)}, {"Hmm, but next time."}},
}

function onCreatureSay(npc, creature, text, situation)
	behaviour.react(npc, rules, text, situation)
end

function onCreatureDisappear(npc, creature)
	behaviour.react(npc, rules, "", NPC_SITUATION_VANISH)
end
