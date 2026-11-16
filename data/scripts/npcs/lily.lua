-- npcs/lily.lua: the behaviour of the NPC Lily, converted from
-- lily.npc by patella-convert. The NPC is its [[npc]] entry in npcs.toml;
-- lib/npc.lua matches and runs the rules below, in the order of the file
-- (scripting.hh, "NPCs"). The comments are those of lily.npc.
--
-- GIMUD - Graphical Interface Multi User Dungeon
-- lily.npc: Datenbank fuer die Druidin Lily

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
local AMOUNT = behaviour.AMOUNT
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
local delete = behaviour.delete
local createMoney = behaviour.createMoney
local deleteMoney = behaviour.deleteMoney
local queue = behaviour.queue
local idle = behaviour.idle
local nop = behaviour.nop
local previous = behaviour.previous

local rules = behaviour.rules {
	{{ADDRESS, "hello$", FORCE}, {"Welcome, %N."}},
	{{ADDRESS, "hi$", FORCE}, {previous}},
	{{ADDRESS, FORCE}, {idle}},
	{{BUSY, "hello$", FORCE}, {"One moment, %N. I'll be with you in no time.", queue}},
	{{BUSY, "hi$", FORCE}, {previous}},
	{{BUSY, FORCE}, {nop}},
	{{VANISH, FORCE}, {"Take care."}},

	{{"bye"}, {"Take care.", idle}},
	{{"farewell"}, {previous}},
	{{"how", "are", "you"}, {"Very well. Thank you."}},
	{{"offer"}, {"I only sell my antidote runes and I'll be happy to buy some blueberries from you."}},
	{{"job"}, {"I am a druid, bound to the spirit of nature. I'm selling antidote runes that help against poison. Oh, and I buy blueberries, of course."}},
	{{"name"}, {"My name is Lily."}},

	{{"hyacinth"}, {"Hyacinth lives in the forest. He's never in town so I don't know him very well."}},
	{{"time"}, {"It is about %T."}},
	{{"help"}, {"I can sell you an antidote rune. It's against the poison of so many dangerous creatures."}},

	{{"monster"}, {"Many monsters are poisonous. Don't let them bite you or you will need one of my antidote runes."}},
	{{"creature"}, {previous}},
	{{"poison"}, {previous}},
	{{"life", "fluid"}, {"I'm sorry, but Hyacinth is the only one on Rookgaard who knows how to brew life fluids."}},

	{{"antidote"}, {setType(3153), setData(1), setAmount(1), setPrice(40), "Do you want to buy an antidote rune for %P gold?", setTopic(1)}},
	{{"rune"}, {previous}},
	{{P1, lt(1, P1), "antidote"}, {setType(3153), setData(1), setAmount(P1), setPrice(mul(40, P1)), "Do you want to buy %A antidote runes for %P gold?", setTopic(1)}},
	{{P1, lt(1, P1), "rune"}, {previous}},

	{{eq(TOPIC, 1), "yes", ge(COUNTMONEY, PRICE)}, {"Here you are.", deleteMoney, create(TYPE)}},
	{{eq(TOPIC, 1), "yes"}, {"Sorry, you don't have enough gold."}},
	{{eq(TOPIC, 1)}, {"As you wish."}},

	{{"sell", "blueberry"}, {setType(3588), setAmount(5), setPrice(1), "Do you want to sell 5 blueberries for %P gold?", setTopic(2)}},
	{{"sell", "berry"}, {previous}},
	{{"sell", "blueberries"}, {previous}},
	{{"sell", "berries"}, {previous}},

	{{eq(TOPIC, 2), "yes", ge(count(TYPE), AMOUNT)}, {"Fine! Here's your gold.", delete(TYPE), createMoney}},
	{{eq(TOPIC, 2), "yes"}, {"Oh, I'm sorry. I'm not buying less than 5 blueberries."}},
	{{eq(TOPIC, 2)}, {"As you wish."}},
}

function onCreatureSay(npc, creature, text, situation)
	behaviour.react(npc, rules, text, situation)
end

function onCreatureDisappear(npc, creature)
	behaviour.react(npc, rules, "", NPC_SITUATION_VANISH)
end
