-- npcs/tandros.lua: the behaviour of the NPC Tandros, converted from
-- tandros.npc by patella-convert. The NPC is its [[npc]] entry in npcs.toml;
-- lib/npc.lua matches and runs the rules below, in the order of the file
-- (scripting.hh, "NPCs"). The comments are those of tandros.npc.
--
-- GIMUD - Graphical Interface Multi User Dungeon
-- tandros.npc: Datenbank für den magierhändler Tandros

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
	{{ADDRESS, "hello$", FORCE}, {"Be greeted, student of the arcane."}},
	{{ADDRESS, "hi$", FORCE}, {previous}},
	{{ADDRESS, "hiho$", FORCE}, {previous}},
	{{ADDRESS, FORCE}, {idle}},

	{{BUSY, "hello$", FORCE}, {"Show some patience please.", queue}},
	{{BUSY, "hi$", FORCE}, {previous}},
	{{BUSY, "hiho$", FORCE}, {previous}},
	{{BUSY, FORCE}, {nop}},
	{{VANISH, FORCE}, {"Farewell."}},

	{{"bye"}, {"Farewell.", idle}},
	{{"farewell"}, {previous}},
	{{"job"}, {"I am selling items of magic power such as runes, wands, rods, life fluids and mana fluids."}},
	{{"name"}, {"I am Tandros the magnificent."}},
	{{"time"}, {"It is a crime against the order of things to try measuring time. The very thought of squeezing the majesty of centuries and centuries into a puny mechanical device is blasphemy."}},
	{{"king"}, {"Our king is a worldly one. But if you buy enough of my fluids and runes you might become the king of magic one day."}},
	{{"venore"}, {"Technically I am an employee of those trade barons of Venore but of course no one can control my magnificent mind."}},
	{{"thais"}, {"It is so crowded and people there are always busy. I dare to say that this is a city that has lost its magic at some point."}},
	{{"carlin"}, {"I heard there are many druids that are quite influential. They should know how to keep the magic of a place alive. I am looking forward to travel there one day."}},
	{{"edron"}, {"Edron is rumoured to be a place of ancient mysteries and powerful magic."}},
	{{"jungle"}, {"The magic is out there somewhere."}},

	{{"tibia"}, {"The world is full of magic that is waiting to be used ... perhaps by you! Take the first step by buying my wares to gather even more magic power for yourself."}},

	{{"kazordoon"}, {"Dwarves have little love for magic. That makes them quite suspicious, doesn't it?"}},
	{{"dwarves"}, {previous}},
	{{"dwarfs"}, {previous}},
	{{"ab'dendriel"}, {"Elves are such marvelous, mythic creatures. They are full of magic."}},
	{{"elves"}, {previous}},
	{{"elfs"}, {previous}},
	{{"darama"}, {"Although our people, spoken in cosmological terms, have setteled here just recently, there is already much history hidden here. ...", "Not only mysteries and magical secrets but also many treasures are here to be explored by that person that is equipped with enough runes and fluids to master all dangers."}},
	{{"darashia"}, {"An unremarkable little town, but riding there by carpet is pure magic."}},
	{{"ankrahmun"}, {"A city that breathes evil and dark magic. Stay away or be at least well prepared if you intend to visit the city of the dead."}},
	{{"ferumbras"}, {"He might be evil, but his powers are unimaginable! To stand a chance against evil overlords like him, you have to buy loads of my runes and fluids."}},
	{{"excalibug"}, {"A weapon of unparalleled magic. Don't listen to people that tell you that this is only a myth. It might be a dream but remember, dreams can come true."}},
	{{"apes"}, {"They are attacking travellers and even our settlement now and then. What can be a better way for you to survive than by preparing yourself well and to buy enough fluids and runes?"}},
	{{"lizard"}, {"The lizards live far away on the other side of the dangerous jungle. If you want to go there to learn more about their secrets, I strongly advise you to supply yourself with runes and fluids."}},
	{{"dworcs"}, {"The dworcs are fierce enemies and the poison they use is lethal. If you don't have some fluids and runes with you, you are easy prey to them."}},

	{{"offer"}, {"I'm selling life and mana fluids, runes, wands, rods and spellbooks."}},
	{{"goods"}, {previous}},
	{{"do", "you", "sell"}, {previous}},
	{{"do", "you", "have"}, {previous}},

	{{"rune"}, {"I sell blank runes and spell runes."}},
	{{"life", "fluid"}, {setType(2874), setData(11), setAmount(1), setPrice(60), "Do you want to buy life fluid for %P gold?", setTopic(2)}},
	{{"mana", "fluid"}, {setType(2874), setData(10), setAmount(1), setPrice(55), "Do you want to buy mana fluid for %P gold?", setTopic(2)}},
	{{"blank", "rune"}, {setType(3147), setAmount(1), setPrice(10), "Do you want to buy a blank rune for %P gold?", setTopic(1)}},
	{{"spellbook"}, {setType(3059), setAmount(1), setPrice(150), "Do you want to buy a spellbook for %P gold?", setTopic(1)}},

	{{P1, lt(1, P1), "life", "fluid"}, {setType(2874), setData(11), setAmount(P1), setPrice(mul(60, P1)), "Do you want to buy %A potions of life fluid for %P gold?", setTopic(2)}},
	{{P1, lt(1, P1), "mana", "fluid"}, {setType(2874), setData(10), setAmount(P1), setPrice(mul(55, P1)), "Do you want to buy %A potions of mana fluid for %P gold?", setTopic(2)}},
	{{P1, lt(1, P1), "blank", "rune"}, {setType(3147), setAmount(P1), setPrice(mul(10, P1)), "Do you want to buy %A blank runes for %P gold?", setTopic(1)}},
	{{P1, lt(1, P1), "spellbook"}, {setType(3059), setAmount(P1), setPrice(mul(150, P1)), "Do you want to buy %A spellbooks for %P gold?", setTopic(1)}},

	{{"deposit"}, {"I will pay you 5 gold for every empty vial. Ok?", setData(0), setTopic(3)}},
	{{"vial"}, {previous}},
	{{"flask"}, {previous}},

	{{eq(TOPIC, 1), "yes", ge(COUNTMONEY, PRICE)}, {"Here you are.", deleteMoney, create(TYPE)}},
	{{eq(TOPIC, 1), "yes"}, {"Come back when you have enough money."}},
	{{eq(TOPIC, 1)}, {"Hmm, but next time."}},

	{{eq(TOPIC, 2), "yes", ge(COUNTMONEY, PRICE)}, {"Here you are. There is a deposit of 5 gold on every vial.", deleteMoney, create(TYPE)}},
	{{eq(TOPIC, 2), "yes"}, {"Come back when you have enough money."}},
	{{eq(TOPIC, 2)}, {"Hmm, but next time."}},

	{{eq(TOPIC, 3), "yes", gt(count(2874), 0)}, {setAmount(count(2874)), setPrice(mul(AMOUNT, 5)), "Here you are ... %P gold.", delete(2874), createMoney}},
	{{eq(TOPIC, 3), "yes"}, {"You don't have any empty vials."}},
	{{eq(TOPIC, 3)}, {"Hmm, but please keep Tibia litter free."}},

	genTRunesFreeS,
	genTWandsPremS,
}

function onCreatureSay(npc, creature, text, situation)
	behaviour.react(npc, rules, text, situation)
end

function onCreatureDisappear(npc, creature)
	behaviour.react(npc, rules, "", NPC_SITUATION_VANISH)
end
