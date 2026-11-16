-- npcs/ulrik.lua: the behaviour of the NPC Ulrik, converted from
-- ulrik.npc by patella-convert. The NPC is its [[npc]] entry in npcs.toml;
-- lib/npc.lua matches and runs the rules below, in the order of the file
-- (scripting.hh, "NPCs"). The comments are those of ulrik.npc.
--
-- GIMUD - Graphical Interface Multi User Dungeon
-- ulrik.npc: Datenbank für den Schmied Ulrik in Greenshore

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
	{{ADDRESS, "hello$", FORCE}, {"Hello %N. What can I do for you?"}},
	{{ADDRESS, "hi$", FORCE}, {previous}},
	{{ADDRESS, FORCE}, {idle}},
	{{BUSY, "hello$", FORCE}, {"One moment please, %N.", queue}},
	{{BUSY, "hi$", FORCE}, {previous}},
	{{BUSY, FORCE}, {nop}},
	{{VANISH, FORCE}, {"Good bye for now."}},

	{{"bye"}, {"Good bye for now.", idle}},
	{{"job"}, {"I am a smith. Do you need anything I make?"}},
	{{"shop"}, {previous}},
	{{"name"}, {"My name is Ulrik."}},
	{{"time"}, {"It is %T."}},
	{{"king"}, {"What can a simple man as me say about a king?"}},
	{{"tibianus"}, {previous}},
	{{"ferumbras"}, {"Oh, I only heard frigthening tales about him."}},
	{{"excalibug"}, {"Every now and then an adventurer like you comes here looking for it."}},
	{{"news"}, {"We live too far away from Thais to hear anything that truly is 'new'."}},
	{{"help"}, {"Sorry, I have no clue how to help you."}},
	{{"monster"}, {"Most monsters live far away, so you can feel safe here in Greenshore."}},
	{{"dungeon"}, {"They say north of Thais is a deep dungeon."}},
	{{"thanks"}, {"You are welcome."}},
	{{"thank", "you"}, {previous}},

	{{"buy"}, {"What do you need? I sell weapons and armor."}},
	{{"do", "you", "sell"}, {previous}},
	{{"do", "you", "have"}, {previous}},
	{{"offer"}, {"My offers are weapons and armor."}},
	{{"weapon"}, {"I have longswords, battle hammers, and battle axes. What do you want?"}},
	{{"armor"}, {"I have scale armor, soldier helmets, and steel shields. What do you want?"}},
	{{"sell"}, {"I'm sorry, but I don't buy used equipment."}},

	{{"longsword"}, {setType(3285), setAmount(1), setPrice(160), "Do you want to buy a longsword for %P gold?", setTopic(1)}},
	{{"battle", "hammer"}, {setType(3305), setAmount(1), setPrice(350), "Do you want to buy a battle hammer for %P gold?", setTopic(1)}},
	{{"battle", "axe"}, {setType(3266), setAmount(1), setPrice(235), "Do you want to buy a battle axe for %P gold?", setTopic(1)}},
	{{"scale", "armor"}, {setType(3377), setAmount(1), setPrice(260), "Do you want to buy a scale armor for %P gold?", setTopic(1)}},
	{{"soldier", "helmet"}, {setType(3375), setAmount(1), setPrice(110), "Do you want to buy a soldier helmet for %P gold?", setTopic(1)}},
	{{"steel", "shield"}, {setType(3409), setAmount(1), setPrice(240), "Do you want to buy a steel shield for %P gold?", setTopic(1)}},

	{{P1, lt(1, P1), "longsword"}, {setType(3285), setAmount(P1), setPrice(mul(160, P1)), "Do you want to buy %A longswords for %P gold?", setTopic(1)}},
	{{P1, lt(1, P1), "battle", "hammer"}, {setType(3305), setAmount(P1), setPrice(mul(350, P1)), "Do you want to buy %A battle hammers for %P gold?", setTopic(1)}},
	{{P1, lt(1, P1), "battle", "axe"}, {setType(3266), setAmount(P1), setPrice(mul(235, P1)), "Do you want to buy %A battle axes for %P gold?", setTopic(1)}},
	{{P1, lt(1, P1), "scale", "armor"}, {setType(3377), setAmount(P1), setPrice(mul(260, P1)), "Do you want to buy %A scale armors for %P gold?", setTopic(1)}},
	{{P1, lt(1, P1), "soldier", "helmet"}, {setType(3375), setAmount(P1), setPrice(mul(110, P1)), "Do you want to buy %A soldier helmets for %P gold?", setTopic(1)}},
	{{P1, lt(1, P1), "steel", "shield"}, {setType(3409), setAmount(P1), setPrice(mul(240, P1)), "Do you want to buy %A steel shields for %P gold?", setTopic(1)}},

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
