-- npcs/snakeeye.lua: the behaviour of the NPC Snake Eye, converted from
-- snakeeye.npc by patella-convert. The NPC is its [[npc]] entry in npcs.toml;
-- lib/npc.lua matches and runs the rules below, in the order of the file
-- (scripting.hh, "NPCs"). The comments are those of snakeeye.npc.
--
-- GIMUD - Graphical Interface Multi User Dungeon
-- snakeeye.npc: Datenbank für den Wirt im Kriminellencamp

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
local TYPE = behaviour.TYPE
local COUNTMONEY = behaviour.COUNTMONEY
local count = behaviour.count
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
	{{ADDRESS, "hello$", FORCE}, {"Hi %N. Come in and have a drink.", setData(3303)}},
	{{ADDRESS, "hi$", FORCE}, {previous}},
	{{ADDRESS, FORCE}, {idle}},
	{{BUSY, "hello$", FORCE}, {"Hang on a second, %N. I'm talking!", queue}},
	{{BUSY, "hi$", FORCE}, {previous}},
	{{BUSY, FORCE}, {nop}},
	{{VANISH, FEMALE, FORCE}, {"Get lost, stinky dragon."}},
	{{VANISH, FORCE}, {"Bye"}},

	{{"bye", FEMALE}, {"Get lost, stinky dragon.", idle}},
	{{"bye", MALE}, {"Bye.", idle}},
	{{"job"}, {"Well, I'm the boss of this tavern."}},
	{{"name"}, {"I'm Snake Eye."}},
	{{"eye"}, {"Well, I had a dispute with a snake once. And the snake won. Bit my left eye out. Therefore, Snake Eye."}},
	{{"snake"}, {previous}},
	{{"boss"}, {"Yeah. I'm the boss. So don't bother me."}},
	{{"tavern"}, {"It's a great tavern. No closing time. No problems with kings or other rulers. Best place in Tibia."}},
	{{"tibia"}, {"There's already too much order in Tibia. We don't need kings or whatever."}},
	{{"king"}, {"We don't need one."}},
	{{"ruler"}, {previous}},
	{{"tibianus"}, {"We don't need him."}},
	{{"time"}, {"Go and get a watch."}},

	{{"god"}, {"The Gods of Tibia! What a crap! It's all superstition!"}},
	{{"crap"}, {"Crap. Crap! CRAP! It's all CRAP!"}},
	{{"superstition"}, {"Believe me! There are no gods."}},
	{{"durin"}, {"He's the worst. The so called god of the dwarfs. I don't believe it. It's all crap."}},
	{{"steve"}, {"Never heard of him."}},
	{{"guido"}, {previous}},
	{{"stephan"}, {previous}},
	{{"cip"}, {"Cip sux!"}},

	{{"thais"}, {"In the beginning, it was a nice encampment. Now it's an overcrowded, polluted city. I hate it!"}},
	{{"carlin"}, {"I've never been there. Don't know anything about it."}},
	{{"kazordoon"}, {"Kazordoon is alright. Except the dwarfs. I don't like them. But the mountains are a good place. Been there once."}},
	{{"ab'dendriel"}, {"I've never been there. I don't like the elves anyway."}},
	{{"edron"}, {"That's a place for wealthy toffs!"}},

	{{"wild", "warrior"}, {"There are a lot of wild warriors around. They built this camp."}},
	{{"camp"}, {"Well, the real wild warriors don't live here. They hide in the woods."}},
	{{"hide"}, {"Well. I know of a small camp to the south."}},
	{{"south"}, {"It's abandoned. But I bet that something is hidden there!"}},
	{{"hidden"}, {"Go and find out yourself. You can tell me if you find something."}},
	{{"copper", "key", gt(count(2970), 0)}, {"Hmmm. A copper key. You should ask H.L. about it."}},
	{{"key", gt(count(2970), 0)}, {previous}},
	{{"h.l."}, {"He is a wild warrior. Nobody knows his real name. We just call im H.L. You can find him in the small armory shop."}},
	{{"hl"}, {previous}},
	{{"wood"}, {"It's the best place to live. By the way, there's an old wild warrior building to the southwest. It might be interesting for you."}},
	{{"building"}, {"Go and ask H.L. about it."}},

	{{"buy"}, {"Do you want to eat or drink?"}},
	{{"eat"}, {"Ok, I have fish, meat, and bread. What do you want?"}},
	{{"drink"}, {"I can offer you beer, wine, and water. Water is for free."}},

	{{"fish"}, {setType(3578), setAmount(1), setPrice(5), "Do you want to buy fish for %P gold?", setTopic(1)}},
	{{"meat"}, {setType(3577), setAmount(1), setPrice(6), "Do you want to buy meat for %P gold?", setTopic(1)}},
	{{"bread"}, {setType(3602), setAmount(1), setPrice(4), "Do you want to buy a bread for %P gold?", setTopic(1)}},
	{{"beer"}, {setType(2880), setData(3), setAmount(1), setPrice(5), "A beer for %P gold?", setTopic(1)}},
	{{"wine"}, {setType(2880), setData(2), setAmount(1), setPrice(6), "Wine for %P gold?", setTopic(1)}},
	{{"water"}, {setType(2880), setData(1), setAmount(1), setPrice(0), "Water is for free. Do you want some?", setTopic(1)}},

	{{P1, lt(1, P1), "fish"}, {setType(3578), setAmount(P1), setPrice(mul(5, P1)), "Do you want to buy %A fish for %P gold?", setTopic(1)}},
	{{P1, lt(1, P1), "meat"}, {setType(3577), setAmount(P1), setPrice(mul(6, P1)), "Do you want to buy %A pieces of meat for %P gold?", setTopic(1)}},
	{{P1, lt(1, P1), "bread"}, {setType(3602), setAmount(P1), setPrice(mul(4, P1)), "Do you want to buy a %A breads for %P gold?", setTopic(1)}},
	{{P1, lt(1, P1), "beer"}, {setType(2880), setData(3), setAmount(P1), setPrice(mul(5, P1)), "%A beers for %P gold?", setTopic(1)}},
	{{P1, lt(1, P1), "wine"}, {setType(2880), setData(2), setAmount(P1), setPrice(mul(6, P1)), "%A Wines for %P gold?", setTopic(1)}},

	{{eq(TOPIC, 1), "yes", ge(COUNTMONEY, PRICE)}, {"Here.", deleteMoney, create(TYPE)}},
	{{eq(TOPIC, 1), "yes"}, {"What? You don't have the money. You crook. Get lost.", idle}},
	{{eq(TOPIC, 1)}, {"OK, then not."}},
}

function onCreatureSay(npc, creature, text, situation)
	behaviour.react(npc, rules, text, situation)
end

function onCreatureDisappear(npc, creature)
	behaviour.react(npc, rules, "", NPC_SITUATION_VANISH)
end
