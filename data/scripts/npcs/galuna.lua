-- npcs/galuna.lua: the behaviour of the NPC Galuna, converted from
-- galuna.npc by patella-convert. The NPC is its [[npc]] entry in npcs.toml;
-- lib/npc.lua matches and runs the rules below, in the order of the file
-- (scripting.hh, "NPCs"). The comments are those of galuna.npc.
--
-- GIMUD - Graphical Interface Multi User Dungeon
-- galuna.npc: Datenbank für die Bogenmacherin Galuna

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
	{{ADDRESS, "hello$", FORCE}, {"Oh, please come in, %N. What do you need?"}},
	{{ADDRESS, "hi$", FORCE}, {previous}},
	{{ADDRESS, FORCE}, {idle}},
	{{BUSY, "hello$", FORCE}, {"Sorry %N, I am already talking to a customer. Please wait", queue}},
	{{BUSY, "hi$", FORCE}, {previous}},
	{{BUSY, FORCE}, {nop}},
	{{VANISH, FORCE}, {"Good bye."}},

	{{"bye"}, {"Good bye.", idle}},
	{{"farewell"}, {previous}},
	{{"job"}, {"I am the local fletcher. I am selling bows, crossbows, and ammunition. Do you need anything?"}},
	{{"fletcher"}, {previous}},
	{{"name"}, {"I am Galuna, paladin and fletcher."}},
	{{"paladin"}, {"We are feared warriors and good marksmen. Ask Elane if want to know more about the guild."}},
	{{"elane"}, {"She is the leader of all paladins."}},
	{{"gorn"}, {"I supplied him with my goods in the past, now I sell them myself."}},
	{{"time"}, {"Don't bother me. Go and buy a watch."}},
	{{"tibia"}, {"Tibia, a green island. Here it is wunderful to walk into the forests and to hunt with a bow."}},
	{{"forest"}, {previous}},
	{{"thais"}, {"We have visitors of all kind in Thais, only elves show up seldom."}},
	{{"elf"}, {"It is rumored that they live in the northeast of Tibia. They are the best in archery."}},
	{{"elves"}, {previous}},

	{{"buy"}, {"I am selling bows, crossbows, and ammunition. Do you need anything?"}},
	{{"do", "you", "sell"}, {previous}},
	{{"do", "you", "have"}, {previous}},
	{{"offer"}, {previous}},
	{{"goods"}, {previous}},
	{{"ammo"}, {"Do you need arrows for a bow, or bolts for a crossbow?"}},
	{{"ammunition"}, {previous}},

	{{"sell", "bow"}, {"I don't buy used bows."}},
	{{"sell", "crossbow"}, {"I don't buy used crossbows."}},

	{{"bow"}, {setType(3350), setAmount(1), setPrice(400), "Do you want to buy a bow for %P gold?", setTopic(1)}},
	{{"crossbow"}, {setType(3349), setAmount(1), setPrice(500), "Do you want to buy a crossbow for %P gold?", setTopic(1)}},
	{{"arrow"}, {setType(3447), setAmount(1), setPrice(2), "Do you want to buy an arrow for %P gold?", setTopic(1)}},
	{{"bolt"}, {setType(3446), setAmount(1), setPrice(3), "Do you want to buy a bolt for %P gold?", setTopic(1)}},

	{{P1, lt(1, P1), "bow"}, {setType(3350), setAmount(P1), setPrice(mul(400, P1)), "Do you want to buy %A bows for %P gold?", setTopic(1)}},
	{{P1, lt(1, P1), "crossbow"}, {setType(3349), setAmount(P1), setPrice(mul(500, P1)), "Do you want to buy %A crossbows for %P gold?", setTopic(1)}},
	{{P1, lt(1, P1), "arrow"}, {setType(3447), setAmount(P1), setPrice(mul(2, P1)), "Do you want to buy %A arrows for %P gold?", setTopic(1)}},
	{{P1, lt(1, P1), "bolt"}, {setType(3446), setAmount(P1), setPrice(mul(3, P1)), "Do you want to buy %A bolts for %P gold?", setTopic(1)}},

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
