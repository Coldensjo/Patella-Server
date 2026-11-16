-- npcs/irea.lua: the behaviour of the NPC Irea, converted from
-- irea.npc by patella-convert. The NPC is its [[npc]] entry in npcs.toml;
-- lib/npc.lua matches and runs the rules below, in the order of the file
-- (scripting.hh, "NPCs"). The comments are those of irea.npc.
--
-- GIMUD - Graphical Interface Multi User Dungeon
-- irea.npc: Datenbank für die Bognerin Irea (Elfenstadt)

local behaviour = require("lib.npc")
local ADDRESS = behaviour.ADDRESS
local BUSY = behaviour.BUSY
local VANISH = behaviour.VANISH
local PALADIN = behaviour.PALADIN
local FORCE = behaviour.FORCE
local P1 = behaviour.P1
local eq = behaviour.eq
local lt = behaviour.lt
local ge = behaviour.ge
local TOPIC = behaviour.TOPIC
local PRICE = behaviour.PRICE
local TYPE = behaviour.TYPE
local LEVEL = behaviour.LEVEL
local COUNTMONEY = behaviour.COUNTMONEY
local spellKnown = behaviour.spellKnown
local spellLevel = behaviour.spellLevel
local mul = behaviour.mul
local setTopic = behaviour.setTopic
local setPrice = behaviour.setPrice
local setAmount = behaviour.setAmount
local setType = behaviour.setType
local effectOpp = behaviour.effectOpp
local teachSpell = behaviour.teachSpell
local create = behaviour.create
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
	{{BUSY, "hello$", FORCE}, {"Wait %N.", queue}},
	{{BUSY, "hi$", FORCE}, {previous}},
	{{BUSY, "ashari$", FORCE}, {previous}},
	{{BUSY, FORCE}, {nop}},
	{{VANISH, FORCE}, {"Asha Thrazi."}},

	{{"bye"}, {"Asha Thrazi.", idle}},
	{{"farewell"}, {previous}},
	{{"asha", "thrazi"}, {previous}},
	{{"job"}, {"I sell bows, arrows, crossbows and bolts. I also teach some spells."}},
	{{"name"}, {"I am known as Irea."}},
	{{"time"}, {"That's unimportant."}},

	{{"carlin"}, {"The druids of Carlin seek our guidance now and then."}},
	{{"thais"}, {"I don't understand what interest this humans form a far away land have in our town."}},
	{{"venore"}, {"Their traders are verry intrusive."}},
	{{"roderick"}, {"What is this humans use at all? I don't understabd it."}},
	{{"olrik"}, {"His trade seems to be the delivery of messages and items."}},

	{{"elves"}, {"Humans or dwarfs will never understand us."}},
	{{"dwarfs"}, {"Bearded, heavy, and small."}},
	{{"humans"}, {"Humans have so little time to learn."}},
	{{"troll"}, {"I despise them."}},

	{{"cenath"}, {"I often listen to their tales."}},
	{{"kuridai"}, {"They provide us with tools and metal."}},
	{{"deraisim"}, {"My people love the woods."}},
	{{"abdaisim"}, {"One day we will be reunited."}},
	{{"teshial"}, {"They have left so long ago."}},
	{{"ferumbras"}, {"Who is that?"}},
	{{"crunor"}, {"The master of nature. He nurtures us and is our benevolent protector."}},
	{{"excalibug"}, {"Our people have a bugfarm in the southeast of Ab'Dendriel."}},
	{{"news"}, {"My news are not for your ears."}},

	{{"magic"}, {"I teach spells to create enchanted arrows."}},
	{{"spell"}, {"I teach 'Conjure Arrow', 'Poison Arrow', and 'Explosive Arrow'."}},

	{{PALADIN, "conjure", "arrow"}, {setType(51), setPrice(450), "Do you want to buy the spell 'Conjure Arrow' for %P gold?", setTopic(3)}},
	{{PALADIN, "poison", "arrow"}, {setType(48), setPrice(700), "Do you want to buy the spell 'Poisoned Arrow' for %P gold?", setTopic(3)}},
	{{PALADIN, "explosive", "arrow"}, {setType(49), setPrice(1000), "Do you want to buy the spell 'Explosive Arrow' for %P gold?", setTopic(3)}},

	{{"conjure", "arrow"}, {"I'm sorry, but this spell is only for paladins."}},
	{{"poison", "arrow"}, {previous}},
	{{"explosive", "arrow"}, {previous}},

	{{"bow"}, {setType(3350), setAmount(1), setPrice(350), "Do you want to buy a bow for %P gold?", setTopic(1)}},
	{{"crossbow"}, {setType(3349), setAmount(1), setPrice(450), "Do you want to buy a crossbow for %P gold?", setTopic(1)}},
	{{"arrow"}, {setType(3447), setAmount(1), setPrice(2), "Do you want to buy an arrow for %P gold?", setTopic(1)}},
	{{"bolt"}, {setType(3446), setAmount(1), setPrice(3), "Do you want to buy a bolt for %P gold?", setTopic(1)}},

	{{P1, lt(1, P1), "bow"}, {setType(3350), setAmount(P1), setPrice(mul(350, P1)), "Do you want to buy %A bows for %P gold?", setTopic(1)}},
	{{P1, lt(1, P1), "crossbow"}, {setType(3349), setAmount(P1), setPrice(mul(450, P1)), "Do you want to buy %A crossbows for %P gold?", setTopic(1)}},
	{{P1, lt(1, P1), "arrow"}, {setType(3447), setAmount(P1), setPrice(mul(2, P1)), "Do you want to buy %A arrows for %P gold?", setTopic(1)}},
	{{P1, lt(1, P1), "bolt"}, {setType(3446), setAmount(P1), setPrice(mul(3, P1)), "Do you want to buy %A bolts for %P gold?", setTopic(1)}},

	{{eq(TOPIC, 1), "yes", ge(COUNTMONEY, PRICE)}, {"And here it is.", deleteMoney, create(TYPE)}},
	{{eq(TOPIC, 1), "yes"}, {"Hey, you do not have enough gold."}},
	{{eq(TOPIC, 1)}, {"Maybe we will make the deal another time."}},

	{{eq(TOPIC, 3), "yes", eq(spellKnown(TYPE), 1)}, {"You already know that spell."}},
	{{eq(TOPIC, 3), "yes", lt(LEVEL, spellLevel(TYPE))}, {setAmount(spellLevel(TYPE)), "You need to advance to level %A to learn this spell."}},
	{{eq(TOPIC, 3), "yes", lt(COUNTMONEY, PRICE)}, {"You do not have the gold to pay me."}},
	{{eq(TOPIC, 3), "yes"}, {"You have learned it now.", deleteMoney, effectOpp(13), teachSpell(TYPE)}},
	{{eq(TOPIC, 3)}, {"As you wish."}},
}

function onCreatureSay(npc, creature, text, situation)
	behaviour.react(npc, rules, text, situation)
end

function onCreatureDisappear(npc, creature)
	behaviour.react(npc, rules, "", NPC_SITUATION_VANISH)
end
