-- npcs/james.lua: the behaviour of the NPC James, converted from
-- james.npc by patella-convert. The NPC is its [[npc]] entry in npcs.toml;
-- lib/npc.lua matches and runs the rules below, in the order of the file
-- (scripting.hh, "NPCs"). The comments are those of james.npc.
--
-- GIMUD - Graphical Interface Multi User Dungeon
-- james.npc: Datenbank für den Bauern James auf Edron

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
	{{ADDRESS, "hello$", FORCE}, {"Hi there %N."}},
	{{ADDRESS, "hi$", FORCE}, {previous}},
	{{ADDRESS, FORCE}, {idle}},
	{{BUSY, "hello$", FORCE}, {"Talking right now, %N. Wait a second.", queue}},
	{{BUSY, "hi$", FORCE}, {previous}},
	{{BUSY, FORCE}, {nop}},
	{{VANISH, FORCE}, {"Bye."}},

	{{"bye"}, {"Bye.", idle}},
	{{"how", "are", "you"}, {"Fine, thanks."}},
	{{"sell"}, {"I sell some food."}},
	{{"job"}, {"I am a humble farmer."}},
	{{"name"}, {"James."}},
	{{"time"}, {"I am too poor to afford a watch."}},
	{{"help"}, {"Sorry, can't offer you any help."}},
	{{"monster"}, {"There are dangerous monsters and renegade knights in the northwest behind the river and the mountain."}},
	{{"dungeon"}, {"I stay away from dungeons as far as I can."}},
	{{"god"}, {"May Crunor bless our harvests."}},
	{{"king"}, {"I never saw him in person."}},
	{{"daniel"}, {"A brave warrior as far as a farmer like me can tell."}},
	{{"avar$"}, {"He scares me a little."}},
	{{"academy"}, {"The mages and druids have quite an appetite. They buy much from me and summon even more food."}},
	{{"magic"}, {"I am nothing but a humble farmer and know nothing about that."}},
	{{"weapon"}, {previous}},
	{{"spell"}, {previous}},
	{{"tibia"}, {"If I were you, I would stay here."}},
	{{"thais"}, {"I was born in Thais, but my family moved to Edron among the first settlers."}},
	{{"edron"}, {previous}},

	{{"bread"}, {setType(3600), setAmount(1), setPrice(3), "Do you want to buy a bread for %P gold?", setTopic(1)}},
	{{"cheese"}, {setType(3607), setAmount(1), setPrice(5), "Do you want to buy a cheese for %P gold?", setTopic(1)}},
	{{"meat"}, {setType(3577), setAmount(1), setPrice(5), "Do you want to buy meat for %P gold?", setTopic(1)}},
	{{"ham"}, {setType(3582), setAmount(1), setPrice(8), "Do you want to buy a ham for %P gold?", setTopic(1)}},
	{{"apple"}, {setType(3585), setAmount(1), setPrice(3), "Do you want to buy an apple for %P gold?", setTopic(1)}},

	{{P1, lt(1, P1), "bread"}, {setType(3600), setAmount(P1), setPrice(mul(3, P1)), "Do you want to buy %A breads for %P gold?", setTopic(1)}},
	{{P1, lt(1, P1), "cheese"}, {setType(3607), setAmount(P1), setPrice(mul(5, P1)), "Do you want to buy %A cheese for %P gold?", setTopic(1)}},
	{{P1, lt(1, P1), "meat"}, {setType(3577), setAmount(P1), setPrice(mul(5, P1)), "Do you want to buy %A meat for %P gold?", setTopic(1)}},
	{{P1, lt(1, P1), "ham"}, {setType(3582), setAmount(P1), setPrice(mul(8, P1)), "Do you want to buy %A ham for %P gold?", setTopic(1)}},
	{{P1, lt(1, P1), "apple"}, {setType(3585), setAmount(P1), setPrice(mul(3, P1)), "Do you want to buy %A apples for %P gold?", setTopic(1)}},

	{{"buy"}, {"I can offer you bread, cheese, ham, meat, and apples."}},
	{{"food"}, {"Are you looking for food? I have bread, cheese, ham, meat, and apples."}},

	{{eq(TOPIC, 1), "yes", ge(COUNTMONEY, PRICE)}, {"Here it is.", deleteMoney, create(TYPE)}},
	{{eq(TOPIC, 1), "yes"}, {"I am sorry, but you do not have enough gold."}},
	{{eq(TOPIC, 1)}, {"Maybe later."}},
}

function onCreatureSay(npc, creature, text, situation)
	behaviour.react(npc, rules, text, situation)
end

function onCreatureDisappear(npc, creature)
	behaviour.react(npc, rules, "", NPC_SITUATION_VANISH)
end
