-- npcs/lugri.lua: the behaviour of the NPC Lugri, converted from
-- lugri.npc by patella-convert. The NPC is its [[npc]] entry in npcs.toml;
-- lib/npc.lua matches and runs the rules below, in the order of the file
-- (scripting.hh, "NPCs"). The comments are those of lugri.npc.
--
-- GIMUD - Graphical Interface Multi User Dungeon
-- lugri.npc: Datenbank fuer den Zathrothpriester Lugri

local behaviour = require("lib.npc")
local ADDRESS = behaviour.ADDRESS
local BUSY = behaviour.BUSY
local VANISH = behaviour.VANISH
local FORCE = behaviour.FORCE
local eq = behaviour.eq
local ge = behaviour.ge
local TOPIC = behaviour.TOPIC
local PRICE = behaviour.PRICE
local AMOUNT = behaviour.AMOUNT
local TYPE = behaviour.TYPE
local COUNTMONEY = behaviour.COUNTMONEY
local count = behaviour.count
local setTopic = behaviour.setTopic
local setPrice = behaviour.setPrice
local setAmount = behaviour.setAmount
local setType = behaviour.setType
local burning = behaviour.burning
local effectMe = behaviour.effectMe
local effectOpp = behaviour.effectOpp
local delete = behaviour.delete
local deleteMoney = behaviour.deleteMoney
local idle = behaviour.idle
local nop = behaviour.nop
local previous = behaviour.previous

local rules = behaviour.rules {
	{{ADDRESS, "hello$", FORCE}, {"What do you want, %N?"}},
	{{ADDRESS, "hi$", FORCE}, {previous}},
	{{ADDRESS, FORCE}, {idle}},
	{{BUSY, "hello$", FORCE}, {"SILENCE!"}},
	{{BUSY, "hi$", FORCE}, {previous}},
	{{BUSY, FORCE}, {nop}},
	{{VANISH, FORCE}, {"May darkness be with you!"}},

	{{"bye"}, {"Leave now, %N. The gods of darkness will watch your soul!", idle}},
	{{"farewell"}, {previous}},
	{{"job"}, {"I am a priest of Zathroth, the bringer of dark secrets."}},
	{{"name"}, {"My name is Lugri."}},
	{{"news"}, {"You will soon see the 'news' with your own eyes. <chuckle>"}},
	{{"tibia"}, {"The world of Tibia is to be taken by the strongest."}},
	{{"how", "are", "you"}, {"I feel the power of evil rising and enjoy that."}},
	{{"sell"}, {"I am in the death business. You wouldn't like what I have to offer."}},
	{{"god$"}, {"The gods of darkness give us the chance to reach our whole potentials, the gods of good want to capture us in eternal stasis!", setTopic(2)}},
	{{"gods$"}, {previous}},
	{{"life"}, {"Life is war. It's about survival of the fittest."}},
	{{"citizen"}, {"The people of Tibia are sheep, so be smart and strong enough to become their wolf."}},
	{{"people"}, {previous}},
	{{"king"}, {"This puny king is no threat for our master's plans."}},
	{{"monster"}, {"They are a challenge to sift the chaff from the wheat."}},
	{{"quest"}, {"Aren't we all on a quest for survival and supremacy?"}},
	{{"mission"}, {previous}},
	{{"survival"}, {previous}},
	{{"supermacy"}, {previous}},
	{{"gold"}, {setPrice(30), "Do you want to make a donation?", setTopic(1)}},
	{{"money"}, {previous}},
	{{"donation"}, {previous}},
	{{"fight"}, {"Life is an eternal fight!"}},
	{{"slay"}, {"The weak have to be slain by the strong!"}},
	{{"heal"}, {"Your wounds are your problem, not mine."}},
	{{"help"}, {"If you cant help yourself you are not worth of my assistance."}},
	{{"ferumbras"}, {"He is one of Zathroth's strongest followers and wields special powers, given to him by the dark one."}},
	{{"time"}, {"Who cares?"}},
	{{"excalibug"}, {"It's existence is just a lie to inspire hope and bravery in the hearts of the followers of good."}},

	{{eq(TOPIC, 2), "good"}, {"The so called gods of good are Fardos, Uman, the elements, Suon, Crunor, Nornur, Bastesh, Kirok, Toth, and Banor."}},
	{{eq(TOPIC, 2), "light"}, {"The so called gods of good are Fardos, Uman, the elements, Suon, Crunor, Nornur, Bastesh, Kirok, Toth, and Banor."}},
	{{"fardos"}, {"Fardos is the creator. He is a helpless watcher whose 'creation' is far more then he bargained for."}},
	{{"uman"}, {"Uman is a jealous keeper of magic. He gives only little knowledge to the mortals."}},
	{{"suon"}, {"Suon is one of the suns of our world. He gives his light mindlessly to the weak and the strong alike."}},
	{{"crunor"}, {"Crunor is a plantgod ... and plants exist to be stomped over."}},
	{{"nornur"}, {"Nornur fancies himself as god of fate without even understanding the ways of fate at all."}},
	{{"bastesh"}, {"Bastesh is so afraid that she hides in the depth of the seas."}},
	{{"kirok"}, {"Kirok, the mad one, is the patron of scientists and jesters, more a nuisance than a god."}},
	{{"toth"}, {"Toth is just the undertaker for the other 'gods of good'."}},
	{{"banor"}, {"Banor isn't a god at all, but one of their tools. It is stupidity to worship a tool, isn't it?"}},
	{{"tibiasula"}, {"Zathroth took her life, recoginzig it was necessary for the process of creation."}},
	{{eq(TOPIC, 2), "tibia"}, {"Tibia is just the mindless elemental power of earth."}},
	{{"sula"}, {"Sula is just the mindless elemental power of water."}},
	{{"air"}, {"Air is is just a mindless elemental force."}},
	{{"fire"}, {"Fire is is just a mindless elemental force."}},

	{{eq(TOPIC, 2), "evil"}, {"The glorious gods of darkness are Zathroth, Fafnar, Brog, Urgith, and the Archdemons."}},
	{{eq(TOPIC, 2), "darkness"}, {"The glorious gods of darkness are Zathroth, Fafnar, Brog, Urgith, and the Archdemons."}},
	{{"zathroth"}, {"Zathroth represents the true and unbound power of magic. He is the keeper of great secrets."}},
	{{"fafnar"}, {"Fafnar is the power of the sun. She burns the weak to ashes."}},
	{{"brog"}, {"Brog, the raging one, the great destroyer, the berserk of darkness ... call him how you like, but fear his awesome power."}},
	{{"urgith"}, {"Urgith is the master of the undead. The bonemaster also takes care of the damned souls."}},
	{{"archdemons"}, {"The demons are powerful followers of Zathroth. Their leaders are known as the ruthless seven."}},
	{{"ruthless", "seven"}, {"Infernatil, Pumin, Verminor, Tafariel, Apocalypse, Bazir and Ashfalor."}},
	{{"tafariel"}, {"She is the mistress of the damned! Rewarding or torturing, it is the same for her victims!"}},
	{{"apocalypse"}, {"It is said even speaking its TRUE name will bring total destruction to you!"}},
	{{"pumin"}, {"He is the lord of despair."}},
	{{"infernatil"}, {"The incendiary of hell."}},
	{{"bazir"}, {"He is the great deciver, the lord of lies."}},
	{{"Verminor"}, {"Ah, the plaguelord."}},
	{{"ashfalor"}, {"The right hand of Urgith. The general of the undead hordes."}},
	{{"pits", "inferno"}, {"After the ruthless seven conquered it, it's again a holy place for the followers of the dark path."}},
	{{"nightmare", "pits"}, {"That name is a disgrace. The pityful nightmare knights couldn't defend them and even lost the treasure of their order there."}},
	{{"goshnar"}, {"The necromant king was only defeated by the nightmare knights due to a bad twist of fate."}},
	{{"necromant", "nectar"}, {"That's none of your business!"}},

	{{eq(TOPIC, 1), "yes", ge(COUNTMONEY, PRICE)}, {"May the gods bless you!", deleteMoney, effectOpp(15)}},
	{{eq(TOPIC, 1), "yes"}, {"Don't be ashamed but you lack the gold."}},
	{{eq(TOPIC, 1)}, {"As you wish."}},

	{{"death", "to", "noodles"}, {setType(3061), setAmount(1), "So, I guess you bring me a magic crystal?", setTopic(3)}},
	{{eq(TOPIC, 3), "yes", ge(count(TYPE), AMOUNT)}, {"Fine. Now you get what you deserve, you fool! DIE IN AGONY!", burning(25, 25), effectOpp(6), effectMe(14), delete(TYPE), idle}},
}

function onCreatureSay(npc, creature, text, situation)
	behaviour.react(npc, rules, text, situation)
end

function onCreatureDisappear(npc, creature)
	behaviour.react(npc, rules, "", NPC_SITUATION_VANISH)
end
