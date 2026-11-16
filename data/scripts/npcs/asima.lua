-- npcs/asima.lua: the behaviour of the NPC Asima, converted from
-- asima.npc by patella-convert. The NPC is its [[npc]] entry in npcs.toml;
-- lib/npc.lua matches and runs the rules below, in the order of the file
-- (scripting.hh, "NPCs"). The comments are those of asima.npc.
--
-- GIMUD - Graphical Interface Multi User Dungeon
-- asima.npc: Datenbank fuer die Magiebedarfshändlerin Asima

local behaviour = require("lib.npc")
local genTRunesFreeS = require("lib.npcs.gen-t-runes-free-s")
local genTWandsPremS = require("lib.npcs.gen-t-wands-prem-s")
local ADDRESS = behaviour.ADDRESS
local BUSY = behaviour.BUSY
local VANISH = behaviour.VANISH
local FORCE = behaviour.FORCE
local P1 = behaviour.P1
local eq = behaviour.eq
local lt = behaviour.lt
local gt = behaviour.gt
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
	{{ADDRESS, "hello$", FORCE}, {"Daraman's blessings, %N."}},
	{{ADDRESS, "hi$", FORCE}, {previous}},
	{{ADDRESS, FORCE}, {idle}},
	{{BUSY, "hello$", FORCE}, {"Wait, %N!", queue}},
	{{BUSY, "hi$", FORCE}, {previous}},
	{{BUSY, FORCE}, {nop}},
	{{VANISH, FORCE}, {"Daraman's blessings."}},

	{{"bye"}, {"Daraman's blessings.", idle}},
	{{"name"}, {"I am Asima, student of arcane magic and right hand of Shalmar."}},
	{{"job"}, {"Shalmar's ears and eyesight have gotten really bad lately. I'm helping him with his magic store so he can focus on teaching spells."}},
	{{"time"}, {"It's %T right now."}},

	{{"offer"}, {"I'm selling life and mana fluids, runes, wands, rods and spellbooks."}},
	{{"goods"}, {previous}},
	{{"do", "you", "sell"}, {previous}},
	{{"do", "you", "have"}, {previous}},

	{{"rune"}, {"I sell blank runes and spell runes."}},
	{{"life", "fluid"}, {setType(2874), setData(11), setAmount(1), setPrice(60), "Do you want to buy life fluid for %P gold?", setTopic(5)}},
	{{"mana", "fluid"}, {setType(2874), setData(10), setAmount(1), setPrice(55), "Do you want to buy mana fluid for %P gold?", setTopic(5)}},
	{{"blank", "rune"}, {setType(3147), setAmount(1), setPrice(10), "Do you want to buy a blank rune for %P gold?", setTopic(4)}},
	{{"spellbook"}, {setType(3059), setAmount(1), setPrice(150), "In a spellbook, your spells are listed. There you will find the pronunciation of each spell. Do you want to buy one for %P gold?", setTopic(4)}},

	{{P1, lt(1, P1), "life", "fluid"}, {setType(2874), setData(11), setAmount(P1), setPrice(mul(60, P1)), "Do you want to buy %A potions of life fluid for %P gold?", setTopic(5)}},
	{{P1, lt(1, P1), "mana", "fluid"}, {setType(2874), setData(10), setAmount(P1), setPrice(mul(55, P1)), "Do you want to buy %A potions of mana fluid for %P gold?", setTopic(5)}},
	{{P1, lt(1, P1), "blank", "rune"}, {setType(3147), setAmount(P1), setPrice(mul(10, P1)), "Do you want to buy %A blank runes for %P gold?", setTopic(4)}},
	{{P1, lt(1, P1), "spellbook"}, {setType(3059), setAmount(P1), setPrice(mul(150, P1)), "In a spellbook, your spells are listed. There you will find the pronunciation of each spell. Do you want to buy %A spellbooks for %P gold?", setTopic(4)}},

	{{"deposit"}, {"I will pay you 5 gold for every empty vial. Ok?", setData(0), setTopic(6)}},
	{{"vial"}, {previous}},
	{{"flask"}, {previous}},

	genTRunesFreeS,
	genTWandsPremS,

	{{eq(TOPIC, 4), "yes", ge(COUNTMONEY, PRICE)}, {"Here you are.", deleteMoney, create(TYPE)}},
	{{eq(TOPIC, 4), "yes"}, {"Come back, when you have enough money."}},
	{{eq(TOPIC, 4)}, {"Hmm, but next time."}},

	{{eq(TOPIC, 5), "yes", ge(COUNTMONEY, PRICE)}, {"Here you are. There is a deposit of 5 gold on the vial.", deleteMoney, create(TYPE)}},
	{{eq(TOPIC, 5), "yes"}, {"Come back, when you have enough money."}},
	{{eq(TOPIC, 5)}, {"Hmm, but next time."}},

	{{eq(TOPIC, 6), "yes", gt(count(2874), 0)}, {setAmount(count(2874)), setPrice(mul(AMOUNT, 5)), "Here you are ... %P gold.", delete(2874), createMoney}},
	{{eq(TOPIC, 6), "yes"}, {"You don't have any empty vials."}},
	{{eq(TOPIC, 6)}, {"Hmm, but please keep Tibia litter free."}},
}

function onCreatureSay(npc, creature, text, situation)
	behaviour.react(npc, rules, text, situation)
end

function onCreatureDisappear(npc, creature)
	behaviour.react(npc, rules, "", NPC_SITUATION_VANISH)
end
