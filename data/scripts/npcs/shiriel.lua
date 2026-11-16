-- npcs/shiriel.lua: the behaviour of the NPC Shiriel, converted from
-- shiriel.npc by patella-convert. The NPC is its [[npc]] entry in npcs.toml;
-- lib/npc.lua matches and runs the rules below, in the order of the file
-- (scripting.hh, "NPCs"). The comments are those of shiriel.npc.
--
-- GIMUD - Graphical Interface Multi User Dungeon
-- shiriel.npc: Datenbank für die Magiehändlerin Shiriel (Elfenstadt)

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
	{{ADDRESS, "hello$", FORCE}, {"Ashari %N."}},
	{{ADDRESS, "hi$", FORCE}, {previous}},
	{{ADDRESS, "ashari$", FORCE}, {previous}},
	{{ADDRESS, FORCE}, {idle}},
	{{BUSY, "hello$", FORCE}, {"Wait, %N.", queue}},
	{{BUSY, "hi$", FORCE}, {previous}},
	{{BUSY, "ashari$", FORCE}, {previous}},
	{{BUSY, FORCE}, {nop}},
	{{VANISH, FORCE}, {"Asha Thrazi."}},

	{{"bye"}, {"Asha Thrazi.", idle}},
	{{"asha", "thrazi"}, {previous}},
	{{"farewell"}, {previous}},
	{{"job"}, {"I sell mystic runes, spellbooks, wands, rods and fluids of life or mana."}},
	{{"name"}, {"I am Shiriel Sharaziel."}},
	{{"time"}, {"Time was mastered by my people long ago."}},

	{{"elves"}, {"Our noble race has knowledge of secrets beyond your comprehension."}},
	{{"dwarfs"}, {"Not worth to bother about."}},
	{{"humans"}, {"Cursed with a short livespan and not worth to be remembered."}},
	{{"troll"}, {"We should eradicate them all."}},
	{{"cenath"}, {"We are the teachers to the other castes."}},
	{{"kuridai"}, {"Their knowledge is limited."}},
	{{"deraisim"}, {"They lack the patience that suits a race with our lifespan."}},
	{{"abdaisim"}, {"I think they are lost forever."}},
	{{"teshial"}, {"They were not prepared for what they encountered in their quest for knowledge. WE will be prepared."}},
	{{"ferumbras"}, {"A humanbreed abnomination."}},
	{{"crunor"}, {"I have no time for superstition."}},
	{{"excalibug"}, {"I would love to analyse it one day."}},
	{{"news"}, {"News are secrets and you are not worthy of my secrets."}},
	{{"magic"}, {"I could teach you some spells ... but I won't."}},

	{{"rune"}, {"I sell blank runes and spell runes."}},
	{{"life", "fluid"}, {setType(2874), setData(11), setAmount(1), setPrice(60), "Do you want to buy life fluid for %P gold?", setTopic(2)}},
	{{"mana", "fluid"}, {setType(2874), setData(10), setAmount(1), setPrice(55), "Do you want to buy mana fluid for %P gold?", setTopic(2)}},
	{{"blank", "rune"}, {setType(3147), setAmount(1), setPrice(10), "Do you want to buy a blank rune for %P gold?", setTopic(1)}},
	{{"spellbook"}, {setType(3059), setAmount(1), setPrice(150), "Do you want to buy a spellbook for %P gold?", setTopic(1)}},

	{{P1, lt(1, P1), "life", "fluid"}, {setType(2874), setData(11), setAmount(P1), setPrice(mul(60, P1)), "Do you want to buy %A potions of life fluid for %P gold?", setTopic(2)}},
	{{P1, lt(1, P1), "mana", "fluid"}, {setType(2874), setData(10), setAmount(P1), setPrice(mul(55, P1)), "Do you want to buy %A potions of mana fluid for %P gold?", setTopic(2)}},
	{{P1, lt(1, P1), "blank", "rune"}, {setType(3147), setAmount(P1), setPrice(mul(10, P1)), "Do you want to buy %A blank runes for %P gold?", setTopic(1)}},
	{{P1, lt(1, P1), "spellbook"}, {setType(3059), setAmount(P1), setPrice(mul(150, P1)), "Do you want to buy %A spellbooks for %P gold?", setTopic(1)}},

	{{"deposit"}, {"I will pay you 5 gold for every empty vial, ok?", setData(0), setTopic(3)}},
	{{"vial"}, {previous}},
	{{"flask"}, {previous}},

	{{eq(TOPIC, 1), "yes", ge(COUNTMONEY, PRICE)}, {"Here you are.", deleteMoney, create(TYPE)}},
	{{eq(TOPIC, 1), "yes"}, {"Come back, when you have enough money."}},
	{{eq(TOPIC, 1)}, {"Hmm, but next time."}},

	{{eq(TOPIC, 2), "yes", ge(COUNTMONEY, PRICE)}, {"Here you are. There is a deposit of 5 gold on the vial.", deleteMoney, create(TYPE)}},
	{{eq(TOPIC, 2), "yes"}, {"Come back, when you have enough money."}},
	{{eq(TOPIC, 2)}, {"Hmm, but next time."}},

	{{eq(TOPIC, 3), "yes", gt(count(2874), 0)}, {setAmount(count(2874)), setPrice(mul(AMOUNT, 5)), "Here you are ... %P gold.", delete(2874), createMoney}},
	{{eq(TOPIC, 3), "yes"}, {"You don't have any empty vials."}},
	{{eq(TOPIC, 3)}, {"Hmm, but please keep Tibia litter free."}},

	genTRunesFreeS,
	genTWandsFreeS,
}

function onCreatureSay(npc, creature, text, situation)
	behaviour.react(npc, rules, text, situation)
end

function onCreatureDisappear(npc, creature)
	behaviour.react(npc, rules, "", NPC_SITUATION_VANISH)
end
