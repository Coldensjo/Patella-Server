-- npcs/thomas.lua: the behaviour of the NPC Thomas, converted from
-- thomas.npc by patella-convert. The NPC is its [[npc]] entry in npcs.toml;
-- lib/npc.lua matches and runs the rules below, in the order of the file
-- (scripting.hh, "NPCs"). The comments are those of thomas.npc.
--
-- GIMUD - Graphical Interface Multi User Dungeon
-- thomas.npc: Datenbank für den Schreibwarenhändler Thomas

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
	{{ADDRESS, "hello$", FORCE}, {"Feel welcome, %N."}},
	{{ADDRESS, "hi$", FORCE}, {previous}},
	{{ADDRESS, FORCE}, {idle}},
	{{BUSY, "hello$", FORCE}, {"Don't be that impatient, %N. Wait a moment!", queue}},
	{{BUSY, "hi$", FORCE}, {previous}},
	{{BUSY, FORCE}, {nop}},
	{{VANISH, FORCE}, {"Bye."}},

	{{"bye"}, {"Bye.", idle}},
	{{"name"}, {"Thomas."}},
	{{"job"}, {"Selling spellbooks, notebooks, scrolls, documents, parchments, inkwells, and the like."}},
	{{"time"}, {"No idea."}},
	{{"king"}, {"Know nothing interesting about him."}},
	{{"tibianus"}, {previous}},
	{{"army"}, {"Ask in the castle."}},
	{{"ferumbras"}, {"Only heard of him."}},
	{{"excalibug"}, {"No idea."}},
	{{"thais"}, {"Never been there."}},
	{{"tibia"}, {"Like it here best."}},
	{{"carlin"}, {"Never been there."}},
	{{"edron"}, {"This town."}},
	{{"news"}, {"Nothing interesting."}},
	{{"rumors"}, {previous}},

	{{"offer"}, {"Selling scrolls, documents, parchments, spellbooks, books, and inkwells."}},
	{{"goods"}, {previous}},
	{{"do", "you", "sell"}, {previous}},
	{{"do", "you", "have"}, {previous}},
	{{"book"}, {"I offer different kind of books: brown, black and small books. Which book do you want?"}},

	{{"scroll"}, {setType(2815), setAmount(1), setPrice(5), "Do you want to buy a scroll for %P gold?", setTopic(1)}},
	{{"document"}, {setType(2834), setAmount(1), setPrice(12), "Do you want to buy a document for %P gold?", setTopic(1)}},
	{{"parchment"}, {setType(2835), setAmount(1), setPrice(8), "Do you want to buy a parchment for %P gold?", setTopic(1)}},
	{{"spellbook"}, {setType(3059), setAmount(1), setPrice(150), "Do you want to buy a spellbook for %P gold?", setTopic(1)}},
	{{"brown", "book"}, {setType(2837), setAmount(1), setPrice(15), "Do you want to buy a brown book for %P gold?", setTopic(1)}},
	{{"black", "book"}, {setType(2838), setAmount(1), setPrice(15), "Do you want to buy a black book for %P gold?", setTopic(1)}},
	{{"small", "book"}, {setType(2839), setAmount(1), setPrice(15), "Do you want to buy a small book for %P gold?", setTopic(1)}},
	{{"inkwell"}, {setType(3509), setAmount(1), setPrice(8), "Do you want to buy an inkwell for %P gold?", setTopic(1)}},

	{{P1, lt(1, P1), "scroll"}, {setType(2815), setAmount(P1), setPrice(mul(5, P1)), "Do you want to buy %A scrolls for %P gold?", setTopic(1)}},
	{{P1, lt(1, P1), "document"}, {setType(2834), setAmount(P1), setPrice(mul(12, P1)), "Do you want to buy %A documents for %P gold?", setTopic(1)}},
	{{P1, lt(1, P1), "parchment"}, {setType(2835), setAmount(P1), setPrice(mul(8, P1)), "Do you want to buy %A parchments for %P gold?", setTopic(1)}},
	{{P1, lt(1, P1), "spellbook"}, {setType(3059), setAmount(P1), setPrice(mul(150, P1)), "Do you want to buy %A spellbooks for %P gold?", setTopic(1)}},
	{{P1, lt(1, P1), "brown", "book"}, {setType(2837), setAmount(P1), setPrice(mul(15, P1)), "Do you want to buy %A brown books for %P gold?", setTopic(1)}},
	{{P1, lt(1, P1), "black", "book"}, {setType(2838), setAmount(P1), setPrice(mul(15, P1)), "Do you want to buy %A black books for %P gold?", setTopic(1)}},
	{{P1, lt(1, P1), "small", "book"}, {setType(2839), setAmount(P1), setPrice(mul(15, P1)), "Do you want to buy %A small books for %P gold?", setTopic(1)}},
	{{P1, lt(1, P1), "inkwell"}, {setType(3509), setAmount(P1), setPrice(mul(8, P1)), "Do you want to buy %A inkwells for %P gold?", setTopic(1)}},

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
