-- npcs/brasith.lua: the behaviour of the NPC Brasith, converted from
-- brasith.npc by patella-convert. The NPC is its [[npc]] entry in npcs.toml;
-- lib/npc.lua matches and runs the rules below, in the order of the file
-- (scripting.hh, "NPCs"). The comments are those of brasith.npc.
--
-- GIMUD - Graphical Interface Multi User Dungeon
-- brasith.npc: Datenbank für den Obsthändler Brasith

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
	{{ADDRESS, "hello$", FORCE}, {"Ashari %N."}},
	{{ADDRESS, "hi$", FORCE}, {previous}},
	{{ADDRESS, "Ashari$", FORCE}, {previous}},
	{{ADDRESS, FORCE}, {idle}},
	{{BUSY, "hello$", FORCE}, {"Wait %N.", queue}},
	{{BUSY, "hi$", FORCE}, {previous}},
	{{BUSY, FORCE}, {nop}},
	{{VANISH, FORCE}, {"Asha Thrazi."}},

	{{"bye"}, {"Asha Thrazi.", idle}},
	{{"farewell"}, {previous}},
	{{"name"}, {"I am Brasith Seedsinger."}},
	{{"job"}, {"You may buy all the things we grow or gather at this place."}},
	{{"time"}, {"Sorry, I can't help you."}},

	{{"carlin"}, {"The humans of Carlin at least ry to live in harmony with nature."}},
	{{"thais"}, {"I heared only terrible storys about that city."}},
	{{"venore"}, {"Their traders seem suspiciously freindly. I don't trust them."}},
	{{"roderick"}, {"His house is an impurity in our city in unity with nature."}},
	{{"olrik"}, {"This poor humans seems to think he might become one of us by spendig time with us."}},

	{{"elf"}, {"Our race lacks unity, which is a very sad thing. And the differences we have will grow and grow until eventually there is no race left."}},
	{{"elves"}, {previous}},
	{{"dwarf"}, {"They work the earth and claim knowledge about it, but they know only about minerals, not about the life it stands for."}},
	{{"human"}, {"They are so many, so planless, so divided. They have choosen a path I do not want for my own race"}},
	{{"troll"}, {"I don't claim to understand this creatures but sometimes they are more close to the roots than we are."}},
	{{"cenath"}, {"The Cenath forgot as many as they learned. I doubt they find the wisdom they are looking for without the things they neglected in their pursuit of knowledge."}},
	{{"kuridai"}, {"The Kuridai left the true path and can't see their error. Their way of living may have been suitable in the past, but if they don't come back to us, their path will lead into darkness."}},
	{{"deraisim"}, {"We have still much to learn but we are on the correct path at least."}},
	{{"abdaisim"}, {"The Abdaisim are true to the ways of our race, maybe even more close than we. But by abandoning the other elves they harm themselves more than they know."}},
	{{"teshial"}, {"They are lost, and if they still exist they are alone in the cold and the darkness."}},
	{{"ferumbras"}, {"He thinks that he is incredibly powerful, but his is only the mindless power of destruction."}},
	{{"crunor"}, {"We abandoned the gods a long time ago. A short time after they abandoned us."}},
	{{"plant"}, {"Life takes many forms. Plants are a very basic form of life. Its simplicity makes them close to the core of nature."}},
	{{"tree"}, {previous}},
	{{"forest"}, {"The beauty of a forest is something easy to be missed by the unobservant."}},
	{{"field"}, {"With the growth of a community comes the need to 'use' nature rather then to 'flow' with nature. This is sad but necessary."}},

	{{"offer"}, {"I sell corncobs, cherries, grapes, melons, pumpkins, bananas, strawberries, and carrots."}},
	{{"goods"}, {previous}},
	{{"do", "you", "sell"}, {previous}},
	{{"do", "you", "have"}, {previous}},
	{{"food"}, {previous}},

	{{"corncob"}, {setType(3597), setAmount(1), setPrice(3), "Do you want to buy a corncob for %P gold?", setTopic(1)}},
	{{"cherry"}, {setType(3590), setAmount(1), setPrice(1), "Do you want to buy a cherry for %P gold?", setTopic(1)}},
	{{"grapes"}, {setType(3592), setAmount(1), setPrice(3), "Do you want to buy grapes for %P gold?", setTopic(1)}},
	{{"melon"}, {setType(3593), setAmount(1), setPrice(8), "Do you want to buy a melon for %P gold?", setTopic(1)}},
	{{"banana"}, {setType(3587), setAmount(1), setPrice(2), "Do you want to buy a banana for %P gold?", setTopic(1)}},
	{{"strawberry"}, {setType(3591), setAmount(1), setPrice(1), "Do you want to buy a strawberry for %P gold?", setTopic(1)}},
	{{"carrot"}, {setType(3595), setAmount(1), setPrice(3), "Do you want to buy a carrot for %P gold?", setTopic(1)}},
	{{"pumpkin"}, {setType(3594), setAmount(1), setPrice(10), "Do you want to buy a pumpkin for %P gold?", setTopic(1)}},

	{{P1, lt(1, P1), "corncob"}, {setType(3597), setAmount(P1), setPrice(mul(3, P1)), "Do you want to buy %A corncobs for %P gold?", setTopic(1)}},
	{{P1, lt(1, P1), "cherr"}, {setType(3590), setAmount(P1), setPrice(mul(1, P1)), "Do you want to buy %A cherries for %P gold?", setTopic(1)}},
	{{P1, lt(1, P1), "grapes"}, {setType(3592), setAmount(P1), setPrice(mul(3, P1)), "Do you want to buy %A grapes for %P gold?", setTopic(1)}},
	{{P1, lt(1, P1), "melon"}, {setType(3593), setAmount(P1), setPrice(mul(8, P1)), "Do you want to buy %A melons for %P gold?", setTopic(1)}},
	{{P1, lt(1, P1), "banana"}, {setType(3587), setAmount(P1), setPrice(mul(2, P1)), "Do you want to buy %A bananas for %P gold?", setTopic(1)}},
	{{P1, lt(1, P1), "strawberries"}, {setType(3591), setAmount(P1), setPrice(mul(1, P1)), "Do you want to buy %A strawberries for %P gold?", setTopic(1)}},
	{{P1, lt(1, P1), "carrot"}, {setType(3595), setAmount(P1), setPrice(mul(3, P1)), "Do you want to buy %A carrots for %P gold?", setTopic(1)}},
	{{P1, lt(1, P1), "pumpkin"}, {setType(3594), setAmount(P1), setPrice(mul(10, P1)), "Do you want to buy %A pumpkins for %P gold?", setTopic(1)}},

	{{"bugmilk"}, {setType(2875), setData(9), setAmount(1), setPrice(15), "Do you want to buy a bottle of bugmilk for %P gold?", setTopic(1)}},

	{{eq(TOPIC, 1), "yes", ge(COUNTMONEY, PRICE)}, {"Thank you. Here it is.", deleteMoney, create(TYPE)}},
	{{eq(TOPIC, 1), "yes"}, {"Sorry, you do not have enough gold."}},
	{{eq(TOPIC, 1)}, {"Maybe you will buy it another time."}},
}

function onCreatureSay(npc, creature, text, situation)
	behaviour.react(npc, rules, text, situation)
end

function onCreatureDisappear(npc, creature)
	behaviour.react(npc, rules, "", NPC_SITUATION_VANISH)
end
