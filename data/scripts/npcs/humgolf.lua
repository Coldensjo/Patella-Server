-- npcs/humgolf.lua: the behaviour of the NPC Humgolf, converted from
-- humgolf.npc by patella-convert. The NPC is its [[npc]] entry in npcs.toml;
-- lib/npc.lua matches and runs the rules below, in the order of the file
-- (scripting.hh, "NPCs"). The comments are those of humgolf.npc.
--
-- GIMUD - Graphical Interface Multi User Dungeon
-- humgolf.npc: Datenbank für den Rotwormzähmer Humgolf (Zwergenstadt)

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
local AMOUNT = behaviour.AMOUNT
local TYPE = behaviour.TYPE
local count = behaviour.count
local mul = behaviour.mul
local setTopic = behaviour.setTopic
local setPrice = behaviour.setPrice
local setAmount = behaviour.setAmount
local setType = behaviour.setType
local delete = behaviour.delete
local createMoney = behaviour.createMoney
local queue = behaviour.queue
local idle = behaviour.idle
local nop = behaviour.nop
local previous = behaviour.previous

local rules = behaviour.rules {
	{{ADDRESS, "hello$", FORCE}, {"%N, good day .. or night, whatever."}},
	{{ADDRESS, "hi$", FORCE}, {previous}},
	{{ADDRESS, FORCE}, {idle}},
	{{BUSY, "hello$", FORCE}, {"Wait %N. We talk later if you insist.", queue}},
	{{BUSY, "hi$", FORCE}, {previous}},
	{{BUSY, FORCE}, {nop}},
	{{VANISH, FORCE}, {"Bye."}},

	{{"bye"}, {"Bye", idle}},
	{{"farewell"}, {previous}},
	{{"job"}, {"I am chief rotwormtamer of Kazordoon. I sell rotworms and buy meat and tasty, fresh rats for my worms."}},
	{{"name"}, {"I am Humgolf Molesight, Son of Earth, from the Molten Rock."}},
	{{"tibia"}, {"More nice beneath this noisy green surface."}},
	{{"kazordoon"}, {"I like the mines best."}},
	{{"big", "old"}, {"The mountain seems tasty as far as my worms are concerned."}},
	{{"worms"}, {"They are so cute and so intelligent."}},
	{{"humans"}, {"They are not cute and not intelligent."}},
	{{"orcs"}, {"They are even more stupid and ugly than minotaurs."}},
	{{"minotaurs"}, {"They are stupid and ugly."}},
	{{"elves"}, {"They are not worth to be mentioned."}},
	{{"geomancer"}, {"They have an understanding of worms."}},
	{{"god"}, {"The worm does not care about gods, why should I?"}},
	{{"earth"}, {"Home of the worms, home of my people, too."}},
	{{"fire"}, {"Where earth is giving, fire is taking. That's the way of the elements."}},
	{{"buy", "rotworm"}, {"Do you want to buy a rotworm?", setTopic(1)}},

	{{eq(TOPIC, 1), "yes"}, {"Hey, you don't own a drilling licence. No deal!"}},
	{{eq(TOPIC, 1)}, {"You will regret that."}},

	{{"drilling", "licence"}, {"I am not allowed to sell worms to people without a formular 007 licence to drill or a 0815 artist licence."}},
	{{"007"}, {previous}},
	{{"0815"}, {"It's a special licence for artists. It was only used once as I sold a white worm to Frietsiek and Yor."}},
	{{"artist", "licence"}, {previous}},

	{{"life"}, {"Can't say I like it much."}},
	{{"plant"}, {"Only a rotting plant is a good plant."}},
	{{"citizen"}, {"Many noisy pepole down here are scaring my worms."}},
	{{"kroox"}, {"Poor guy, has lost his drilling licence for drinking."}},
	{{"jimbin"}, {"His tavern is too crowded and too bright for a dwarf with taste like me."}},
	{{"maryza"}, {"Don't like the way she looks at my worms."}},
	{{"bezil"}, {"Always chatting. How can someone talk that much?"}},
	{{"nezil"}, {previous}},
	{{"uzgod"}, {"We are trading now and then. Fine dwarf he is."}},
	{{"etzel"}, {"Does a worm need spells to work his kind of magic? I do neither."}},
	{{"motos"}, {"That guy is a monster! I despise rotwormkillers."}},
	{{"general"}, {previous}},
	{{"durin"}, {"If he'd live today he'd be a rotwormtamer like me."}},
	{{"duria"}, {"Thinks she's to good to talk to rotwormtamers."}},
	{{"emperor"}, {"The emperor should spend more money on rotworm husbandry."}},
	{{"kruzak"}, {previous}},
	{{"pyromancer"}, {"Hotheads."}},
	{{"technomancer"}, {"GO AWAY! I heard they think of replacing worms with machines. That is an OUTRAGE!"}},
	{{"army"}, {"They should remember old dwarfish rotworm tactics. Think like a worm and the battle is almost won."}},
	{{"colossus"}, {"Never was up there to look at it."}},
	{{"ferumbras"}, {"A true enemy of the worms."}},
	{{"excalibug"}, {"Silly fairy tale."}},
	{{"news"}, {"Who needs news if the old things are still good enough?"}},
	{{"monster"}, {"Unwormish creatures they are."}},
	{{"stone", "golem"}, {"Too hard to be gnawed away by even the finest worm."}},
	{{"help"}, {"I am here to help the worms, not the fools."}},
	{{"quest"}, {"What by the worm are you talking about?"}},
	{{"task"}, {previous}},
	{{"what", "do"}, {previous}},
	{{"gold"}, {"Gold is one of the things my worms can unearth."}},
	{{"money"}, {previous}},
	{{"equipment"}, {"If you own a good worm you need nothing else."}},
	{{"time"}, {"Time does not matter to a dwarf who understands the ways of the worm."}},

	{{"sell", "meat"}, {setType(3577), setAmount(1), setPrice(2), "So, you want to sell meat? Hmm, I give you %P gold, ok?", setTopic(2)}},
	{{"sell", "ham"}, {setType(3582), setAmount(1), setPrice(4), "So, you want to sell a ham? Hmm, I give you %P gold, ok?", setTopic(2)}},
	{{"sell", "fish"}, {"Go away with that waterthing!"}},
	{{"sell", "rat"}, {setType(3994), setAmount(1), setPrice(2), "Do you have a fresh rat for sale?", setTopic(2)}},

	{{"sell", P1, lt(1, P1), "meat"}, {setType(3577), setAmount(P1), setPrice(mul(2, P1)), "So, you want to sell %A pieces of meat? Hmm, I give you %P gold, ok?", setTopic(2)}},
	{{"sell", P1, lt(1, P1), "ham"}, {setType(3582), setAmount(P1), setPrice(mul(4, P1)), "So, you want to sell %A pieces of ham? Hmm, I give you %P gold, ok?", setTopic(2)}},
	{{"sell", P1, lt(1, P1), "rat"}, {setType(3994), setAmount(P1), setPrice(mul(2, P1)), "Do you have %A fresh rats for sale?", setTopic(2)}},

	{{eq(TOPIC, 2), "yes", ge(count(TYPE), AMOUNT)}, {"Here you are.", delete(TYPE), createMoney}},
	{{eq(TOPIC, 2), "yes"}, {"You don't have one."}},
	{{eq(TOPIC, 2), "no"}, {"Then not."}},
}

function onCreatureSay(npc, creature, text, situation)
	behaviour.react(npc, rules, text, situation)
end

function onCreatureDisappear(npc, creature)
	behaviour.react(npc, rules, "", NPC_SITUATION_VANISH)
end
