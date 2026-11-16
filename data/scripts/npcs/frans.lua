-- npcs/frans.lua: the behaviour of the NPC Frans, converted from
-- frans.npc by patella-convert. The NPC is its [[npc]] entry in npcs.toml;
-- lib/npc.lua matches and runs the rules below, in the order of the file
-- (scripting.hh, "NPCs"). The comments are those of frans.npc.
--
-- GIMUD - Graphical Interface Multi User Dungeon
-- frans.npc: Datenbank für den Magieladen-Verkäufer Frans

local behaviour = require("lib.npc")
local genTRunesFreeS = require("lib.npcs.gen-t-runes-free-s")
local genTWandsFreeS = require("lib.npcs.gen-t-wands-free-s")
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
	{{ADDRESS, "hello$", FORCE}, {"Beeee Greeeeted %N. What is your neeeed?"}},
	{{ADDRESS, "hi$", FORCE}, {previous}},
	{{ADDRESS, FORCE}, {idle}},
	{{BUSY, "hello$", FORCE}, {"%N, I am veeeerry busy. Unfoooortunately you will have to waiiiit.", queue}},
	{{BUSY, "hi$", FORCE}, {previous}},
	{{BUSY, FORCE}, {nop}},
	{{VANISH, FORCE}, {"Bye."}},

	{{"bye"}, {"Bye.", idle}},
	{{"farewell"}, {previous}},
	{{"name"}, {"I am a FRANS."}},
	{{"frans"}, {"Floating ReeeeAnimated Necromantic Seeeervant ... FRANS."}},
	{{"job"}, {"I am selliiiing ruuuunes, wands, roooods and spellbooooooks."}},
	{{"sorcerer"}, {"Sorcerorssss, druidssss, they all come to ussss."}},
	{{"druid"}, {previous}},
	{{"magic"}, {"Is aaaall about magic more or lesssss, isn't it?"}},
	{{"vladruc"}, {"Heeee is the bossss. Better don't messss with him!"}},
	{{"urghain"}, {previous}},
	{{"ferumbras"}, {"Wouldn't he beeee the perfect FRANS?"}},
	{{"market"}, {"Yes, that's a market heeeere, smarty ... Nice to seeeee I am not the only one without a braiiiin here."}},
	{{"excalibug"}, {"We FRANSes don't liiiike any bugssss."}},

	{{"offer"}, {"What do youuuu think I am? A lousy barberrrr? I'm selliiiing ruuuunes and spellboooooks."}},
	{{"goods"}, {previous}},
	{{"do", "you", "sell"}, {previous}},
	{{"do", "you", "have"}, {previous}},

	{{"rune"}, {"I sell blank ruuuuunes and spell ruuuuunes."}},
	{{"blank", "rune"}, {setType(3147), setAmount(1), setPrice(10), "Do youuuu want to buy a blank ruuuune for %P gold?", setTopic(1)}},
	{{"spellbook"}, {setType(3059), setAmount(1), setPrice(150), "Do youuuu want to buy a spellboooook for %P gold?", setTopic(1)}},

	{{P1, lt(1, P1), "blank", "rune"}, {setType(3147), setAmount(P1), setPrice(mul(10, P1)), "Do youuuu want to buy %A blank ruuuunes for %P gold?", setTopic(1)}},
	{{P1, lt(1, P1), "spellbook"}, {setType(3059), setAmount(P1), setPrice(mul(150, P1)), "Do youuuu want to buy %A spellboooooks for %P gold?", setTopic(1)}},

	{{eq(TOPIC, 1), "yes", ge(COUNTMONEY, PRICE)}, {"And here it isssss! Almost like magiiiic, isn't it?", deleteMoney, create(TYPE)}},
	{{eq(TOPIC, 1), "yes"}, {"Sorry, too many buttonssss and not enough gold in your pursssse."}},
	{{eq(TOPIC, 1)}, {"Better not annoy a FRANS if you don't want to deeeeal with him!"}},

	genTRunesFreeS,
	genTWandsFreeS,
}

function onCreatureSay(npc, creature, text, situation)
	behaviour.react(npc, rules, text, situation)
end

function onCreatureDisappear(npc, creature)
	behaviour.react(npc, rules, "", NPC_SITUATION_VANISH)
end
