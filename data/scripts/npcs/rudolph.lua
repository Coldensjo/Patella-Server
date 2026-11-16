-- npcs/rudolph.lua: the behaviour of the NPC Rudolph, converted from
-- rudolph.npc by patella-convert. The NPC is its [[npc]] entry in npcs.toml;
-- lib/npc.lua matches and runs the rules below, in the order of the file
-- (scripting.hh, "NPCs"). The comments are those of rudolph.npc.
--
-- GIMUD - Graphical Interface Multi User Dungeon
-- rudolph.npc: Datenbank für den Schneider Rudolph

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
	{{ADDRESS, "hello$", FORCE}, {"Oh, a customer. Hello %N."}},
	{{ADDRESS, "hi$", FORCE}, {previous}},
	{{ADDRESS, FORCE}, {idle}},
	{{BUSY, "hello$", FORCE}, {"Oh, so wait a little, %N.", queue}},
	{{BUSY, "hi$", FORCE}, {previous}},
	{{BUSY, FORCE}, {nop}},
	{{VANISH, FORCE}, {"Oh, good bye."}},

	{{"bye"}, {"Oh, good bye.", idle}},
	{{"name"}, {"I'm Rudolph, you know."}},
	{{"job"}, {"Oh, I am a tailor, can't you see?"}},
	{{"time"}, {"Oh, now it's %T."}},
	{{"king"}, {"Oh, the king. What a well dressed man he is."}},
	{{"tibianus"}, {previous}},
	{{"army"}, {"Oh, such handsome guys and such ugly uniforms."}},
	{{"ferumbras"}, {"Oh, dear."}},
	{{"excalibug"}, {"Oh, that thing must be dangerous. One could hurt himself quite badly with it I guess."}},
	{{"thais"}, {"Oh, what a lovely city it was once."}},
	{{"tibia"}, {"Oh, there is not much sense for fashion in this world."}},
	{{"carlin"}, {"Oh, these women ... oh, go away."}},
	{{"edron"}, {"Oh, what a lovely city it is."}},
	{{"news"}, {"Oh, we tailors learn much, but don't talk about it. It's the tailors' code of honor, you know."}},
	{{"rumors"}, {previous}},

	{{"offer"}, {"Oh, I have wonderful clothes and shoes."}},
	{{"goods"}, {previous}},
	{{"do", "you", "sell"}, {previous}},
	{{"do", "you", "have"}, {previous}},
	{{"clothes"}, {"Oh, I have wonderful jackets, capes, tunics, leather legs, and scarfs."}},
	{{"shoes"}, {"Oh, I have wonderful leather boots and sandals."}},

	{{"jacket"}, {setType(3561), setAmount(1), setPrice(12), "Oh, do you want to buy one of my wonderful jackets for %P gold?", setTopic(1)}},
	{{"tunic"}, {setType(3563), setAmount(1), setPrice(10), "Oh, do you want to buy one of my wonderful green tunics for %P gold?", setTopic(1)}},
	{{"cape"}, {setType(3565), setAmount(1), setPrice(9), "Oh, do you want to buy one of my wonderful capes for %P gold?", setTopic(1)}},
	{{"leather", "legs"}, {setType(3559), setAmount(1), setPrice(10), "Oh, do you want to buy one of my wonderful leather legs for %P gold?", setTopic(1)}},
	{{"scarf"}, {setType(3572), setAmount(1), setPrice(15), "Oh, do you want to buy one of my wonderful silky scarfs for %P gold?", setTopic(1)}},
	{{"sandals"}, {setType(3551), setAmount(1), setPrice(2), "Oh, do you want to buy one of my wonderful sandals for %P gold?", setTopic(1)}},
	{{"leather", "boot"}, {setType(3552), setAmount(1), setPrice(2), "Oh, do you want to buy one of my wonderful leather boots for %P gold?", setTopic(1)}},

	{{P1, lt(1, P1), "jacket"}, {setType(3561), setAmount(P1), setPrice(mul(12, P1)), "Oh, do you want to buy %A of my wonderful jackets for %P gold?", setTopic(1)}},
	{{P1, lt(1, P1), "tunic"}, {setType(3563), setAmount(P1), setPrice(mul(10, P1)), "Oh, do you want to buy %A of my wonderful green tunics for %P gold?", setTopic(1)}},
	{{P1, lt(1, P1), "cape"}, {setType(3565), setAmount(P1), setPrice(mul(9, P1)), "Oh, do you want to buy %A of my wonderful capes for %P gold?", setTopic(1)}},
	{{P1, lt(1, P1), "leather", "legs"}, {setType(3559), setAmount(P1), setPrice(mul(10, P1)), "Oh, do you want to buy %A of my wonderful leather legs for %P gold?", setTopic(1)}},
	{{P1, lt(1, P1), "scar"}, {setType(3572), setAmount(P1), setPrice(mul(15, P1)), "Oh, do you want to buy %A of my wonderful silky scarves for %P gold?", setTopic(1)}},
	{{P1, lt(1, P1), "sandals"}, {setType(3551), setAmount(P1), setPrice(mul(2, P1)), "Oh, do you want to buy %A of my wonderful sandals for %P gold?", setTopic(1)}},
	{{P1, lt(1, P1), "leather", "boot"}, {setType(3552), setAmount(P1), setPrice(mul(2, P1)), "Oh, do you want to buy %A of my wonderful leather boots for %P gold?", setTopic(1)}},

	{{eq(TOPIC, 1), "yes", ge(COUNTMONEY, PRICE)}, {"Oh, here you are.", deleteMoney, create(TYPE)}},
	{{eq(TOPIC, 1), "yes"}, {"Oh, you do not have enough money."}},
	{{eq(TOPIC, 1)}, {"Oh, but next time."}},
}

function onCreatureSay(npc, creature, text, situation)
	behaviour.react(npc, rules, text, situation)
end

function onCreatureDisappear(npc, creature)
	behaviour.react(npc, rules, "", NPC_SITUATION_VANISH)
end
