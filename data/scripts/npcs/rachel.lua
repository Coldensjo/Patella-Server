-- npcs/rachel.lua: the behaviour of the NPC Rachel, converted from
-- rachel.npc by patella-convert. The NPC is its [[npc]] entry in npcs.toml;
-- lib/npc.lua matches and runs the rules below, in the order of the file
-- (scripting.hh, "NPCs"). The comments are those of rachel.npc.
--
-- GIMUD - Graphical Interface Multi User Dungeon
-- rachel.npc: Datenbank für die Magierin Rachel

local behaviour = require("lib.npc")
local genTRunesFreeS = require("lib.npcs.gen-t-runes-free-s")
local genTWandsFreeS = require("lib.npcs.gen-t-wands-free-s")
local ADDRESS = behaviour.ADDRESS
local BUSY = behaviour.BUSY
local VANISH = behaviour.VANISH
local MALE = behaviour.MALE
local FEMALE = behaviour.FEMALE
local SORCERER = behaviour.SORCERER
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
local burning = behaviour.burning
local effectMe = behaviour.effectMe
local effectOpp = behaviour.effectOpp
local create = behaviour.create
local delete = behaviour.delete
local createMoney = behaviour.createMoney
local deleteMoney = behaviour.deleteMoney
local queue = behaviour.queue
local idle = behaviour.idle
local nop = behaviour.nop
local previous = behaviour.previous

local rules = behaviour.rules {
	{{ADDRESS, "hello$", SORCERER, MALE, FORCE}, {"Welcome back, brother %N!", setTopic(1)}},
	{{ADDRESS, "hello$", SORCERER, FEMALE, FORCE}, {"Welcome back, sister %N! Isn't your name %N?", setTopic(1)}},
	{{ADDRESS, "hi$", SORCERER, MALE, FORCE}, {"Welcome back, brother %N! Wasn't your name %N?", setTopic(1)}},
	{{ADDRESS, "hi$", SORCERER, FEMALE, FORCE}, {"Welcome back, sister %N! Wasn't your name %N?", setTopic(1)}},
	{{ADDRESS, "hi$", FORCE}, {"Welcome %N! Whats your need?"}},
	{{ADDRESS, "hello$", FORCE}, {"Welcome %N! Whats your need?"}},
	{{ADDRESS, FORCE}, {idle}},
	{{BUSY, "hello$", FORCE}, {"Wait, %N! One after the other.", queue}},
	{{BUSY, "hi$", FORCE}, {previous}},
	{{BUSY, FORCE}, {nop}},
	{{VANISH, FORCE}, {"These impatient young brats!"}},

	{{"bye"}, {"Good bye, %N", idle}},
	{{"farewell"}, {previous}},
	{{"job"}, {"I am the head alchemist of Carlin. I keep the secret recipies of our ancestors. Besides, I am selling mana and life fluids, spellbooks, wands, rods and runes."}},
	{{"name"}, {"I am the illusterous Rachel, of course."}},
	{{"time"}, {"Time is of no meaning to us sorcerers."}},
	{{"wisdom"}, {"Wisdom arises from patience."}},
	{{"patience"}, {"You have to free yourself from unpatience to learn the deeper secrets of magic."}},
	{{"ancestor"}, {"We are a guild of old traditions and even older secrets."}},
	{{"sorcerer"}, {"Spells are the minor parts that make a sorcerer. To be one is a state of mind, not of a full spellbook."}},
	{{"power"}, {"Power is important, but it is just the way, not the ultimate goal."}},
	{{"goal"}, {"This secrect will be taught you by life, not by me."}},
	{{"vocation"}, {"Your vocation is your profession. There are four vocations in Tibia: Sorcerers, paladins, knights, and druids."}},
	{{"spell$"}, {"I am too busy to teach you, ask in your guild about that."}},
	{{"spells"}, {previous}},

	{{"rune"}, {"I sell blank runes and spell runes."}},
	{{"spellbook"}, {setType(3059), setAmount(1), setPrice(150), "A spellbook is a nice tool for beginners. Do you want to buy one for %P gold?", setTopic(2)}},
	{{"blank", "rune"}, {setType(3147), setAmount(1), setPrice(10), "Do you want to buy one for %P gold?", setTopic(2)}},
	{{"mana", "fluid"}, {setType(2874), setData(10), setAmount(1), setPrice(55), "Do you want to buy mana fluid for %P gold?", setTopic(3)}},
	{{"life", "fluid"}, {setType(2874), setData(11), setAmount(1), setPrice(60), "Do you want to buy life fluid for %P gold?", setTopic(3)}},

	{{P1, lt(1, P1), "spellbook"}, {setType(3059), setAmount(P1), setPrice(mul(150, P1)), "Do you want to buy %A spellbooks for %P gold?", setTopic(2)}},
	{{P1, lt(1, P1), "blank", "rune"}, {setType(3147), setAmount(P1), setPrice(mul(10, P1)), "Do you want to buy %A runes for %P gold?", setTopic(2)}},
	{{P1, lt(1, P1), "mana", "fluid"}, {setType(2874), setData(10), setAmount(P1), setPrice(mul(55, P1)), "Do you want to buy %A potions of mana fluid for %P gold?", setTopic(3)}},
	{{P1, lt(1, P1), "life", "fluid"}, {setType(2874), setData(11), setAmount(P1), setPrice(mul(60, P1)), "Do you want to buy %A potions of life fluid for %P gold?", setTopic(3)}},

	{{"sell", "talon"}, {setType(3034), setAmount(1), setPrice(320), "Do you want to sell one of the magic gems called talon for %P gold?", setTopic(6)}},
	{{"sell", P1, lt(1, P1), "talon"}, {setType(3034), setAmount(P1), setPrice(mul(320, P1)), "Do you want to sell %A magic gems called talon for %P gold?", setTopic(6)}},

	{{eq(TOPIC, 1), "yes"}, {"I thought so, what do you want?"}},
	{{eq(TOPIC, 1), "no"}, {"First lesson: DON'T LIE TO RACHEL!", burning(10, 4), effectMe(15), effectOpp(16)}},
	{{eq(TOPIC, 1), SORCERER}, {"I thought only intelligent persons are allowed to become sorcerers."}},
	{{eq(TOPIC, 1)}, {"I am glad that only intelligent persons are allowed to become sorcerers."}},

	{{eq(TOPIC, 2), "yes", ge(COUNTMONEY, PRICE)}, {"Here you are.", deleteMoney, create(TYPE)}},
	{{eq(TOPIC, 2), "yes"}, {"Come back when you have enough money."}},
	{{eq(TOPIC, 2)}, {"Hmm, maybe next time."}},

	{{eq(TOPIC, 3), "yes", ge(COUNTMONEY, PRICE)}, {"Here you are. There is a deposit of 5 gold for an empty vial.", deleteMoney, create(TYPE)}},
	{{eq(TOPIC, 3), "yes"}, {"Come back, when you have enough money."}},
	{{eq(TOPIC, 3)}, {"Hmm, but next time."}},

	{{"deposit"}, {"I will pay you 5 gold for every empty vial. Ok?", setData(0), setTopic(5)}},
	{{"vial"}, {previous}},
	{{"flask"}, {previous}},
	{{eq(TOPIC, 5), "yes", gt(count(2874), 0)}, {setAmount(count(2874)), setPrice(mul(AMOUNT, 5)), "Here you are ... %P gold.", delete(2874), createMoney}},
	{{eq(TOPIC, 5), "yes"}, {"You don't have any empty vials."}},
	{{eq(TOPIC, 5)}, {"Hmm, but please keep Tibia litter free."}},

	{{eq(TOPIC, 6), "yes", ge(count(TYPE), AMOUNT)}, {"Ok. Here is your money.", delete(TYPE), createMoney}},
	{{eq(TOPIC, 6), "yes"}, {"Sorry, you do not have one."}},
	{{eq(TOPIC, 6), "yes", gt(AMOUNT, 1)}, {"Sorry, you do not have so many."}},
	{{eq(TOPIC, 6)}, {"Maybe next time."}},

	genTRunesFreeS,
	genTWandsFreeS,
}

function onCreatureSay(npc, creature, text, situation)
	behaviour.react(npc, rules, text, situation)
end

function onCreatureDisappear(npc, creature)
	behaviour.react(npc, rules, "", NPC_SITUATION_VANISH)
end
