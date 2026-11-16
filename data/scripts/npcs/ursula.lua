-- npcs/ursula.lua: the behaviour of the NPC Ursula, converted from
-- ursula.npc by patella-convert. The NPC is its [[npc]] entry in npcs.toml;
-- lib/npc.lua matches and runs the rules below, in the order of the file
-- (scripting.hh, "NPCs"). The comments are those of ursula.npc.
--
-- GIMUD - Graphical Interface Multi User Dungeon
-- ursula.npc: Datenbank für die Zauberlehrerin Ursula

local behaviour = require("lib.npc")
local ADDRESS = behaviour.ADDRESS
local BUSY = behaviour.BUSY
local VANISH = behaviour.VANISH
local PALADIN = behaviour.PALADIN
local SORCERER = behaviour.SORCERER
local DRUID = behaviour.DRUID
local FORCE = behaviour.FORCE
local eq = behaviour.eq
local lt = behaviour.lt
local TOPIC = behaviour.TOPIC
local PRICE = behaviour.PRICE
local TYPE = behaviour.TYPE
local LEVEL = behaviour.LEVEL
local COUNTMONEY = behaviour.COUNTMONEY
local spellKnown = behaviour.spellKnown
local spellLevel = behaviour.spellLevel
local setTopic = behaviour.setTopic
local setPrice = behaviour.setPrice
local setAmount = behaviour.setAmount
local setType = behaviour.setType
local effectOpp = behaviour.effectOpp
local teachSpell = behaviour.teachSpell
local deleteMoney = behaviour.deleteMoney
local queue = behaviour.queue
local idle = behaviour.idle
local nop = behaviour.nop
local previous = behaviour.previous

local rules = behaviour.rules {
	{{ADDRESS, "hello$", FORCE}, {"Howdy %N."}},
	{{ADDRESS, "hi$", FORCE}, {previous}},
	{{ADDRESS, FORCE}, {idle}},
	{{BUSY, "hello$", FORCE}, {"Gimme one more minute %N.", queue}},
	{{BUSY, "hi$", FORCE}, {previous}},
	{{BUSY, FORCE}, {nop}},
	{{VANISH, FORCE}, {"Bye."}},

	{{"bye"}, {"Bye", idle}},
	{{"job"}, {"I teach some basic spells."}},
	{{"name"}, {"I am Ursula."}},
	{{"time"}, {"Sorry, I don't own a watch."}},
	{{"king"}, {"The king spent a lot of money for our library."}},
	{{"tibianus"}, {previous}},
	{{"army"}, {"I would prefer an army of spellcasters, but they are ok."}},
	{{"ferumbras"}, {"Ah, come on, he can't be that poweful and evil as all say."}},
	{{"excalibug"}, {"A myth born out of some knights' inferiority complex."}},
	{{"thais"}, {"Ah, yes, I remember my time there, old Muriel teaching me the basics."}},
	{{"tibia"}, {"Isn't it a fine world we live in."}},
	{{"carlin"}, {"They should have been more careful with this town, before they lost it."}},
	{{"edron"}, {"Sciences are thriving on this isle."}},
	{{"news"}, {"I heard about things you never would believe. Please come back when I have more time to chat."}},
	{{"rumors"}, {previous}},

	{{"spellbook"}, {"I'm sorry, but I don't have one. Ask Thomas in the west tower about that."}},
	{{"spell"}, {"I have 'Conjure Bolt', 'Animate Dead', 'Envenom', 'Heal Friend', 'Desintegrate', 'Poison Bomb', and 'Strong Haste'. Which one do you want to learn?"}},

	{{"conjure", "bolt", PALADIN}, {setType(79), setPrice(750), "Do you want to learn the spell 'Conjure Bolt' for %P gold?", setTopic(1)}},
	{{"conjure", "bolt"}, {"I'm sorry, but this spell is only for paladins."}},
	{{"animate", "dead", SORCERER}, {setType(83), setPrice(1200), "Do you want to learn the spell 'Animate Dead' for %P gold?", setTopic(1)}},
	{{"animate", "dead", DRUID}, {previous}},
	{{"animate", "dead"}, {"I'm sorry, but this spell is only for druids and sorcerers."}},
	{{"envenom", DRUID}, {setType(77), setPrice(1000), "Do you want to learn the spell 'Envenom' for %P gold?", setTopic(1)}},
	{{"envenom"}, {"I'm sorry, but this spell is only for druids."}},
	{{"heal", "friend", DRUID}, {setType(84), setPrice(800), "Do you want to learn the spell 'Heal Friend' for %P gold?", setTopic(1)}},
	{{"heal", "friend"}, {"I'm sorry, but this spell is only for druids."}},
	{{"desintegrate", PALADIN}, {setType(78), setPrice(900), "Do you want to learn the spell 'Desintegrate' for %P gold?", setTopic(1)}},
	{{"desintegrate", SORCERER}, {previous}},
	{{"desintegrate", DRUID}, {previous}},
	{{"desintegrate"}, {"I'm sorry, but this spell is only for paladins, sorcerers, and druids."}},
	{{"strong", "haste", SORCERER}, {setType(39), setPrice(1300), "Do you want to learn the spell 'Strong Haste' for %P gold?", setTopic(1)}},
	{{"strong", "haste", DRUID}, {previous}},
	{{"strong", "haste"}, {"I'm sorry, but this spell is only for druids and sorcerers."}},
	{{"poison", "bomb", DRUID}, {setType(91), setPrice(1000), "Do you want to learn the spell 'Poison Bomb' for %P gold?", setTopic(1)}},
	{{"poison", "bomb"}, {"I'm sorry, but this spell is only for druids."}},

	{{eq(TOPIC, 1), "yes", eq(spellKnown(TYPE), 1)}, {"I'm sorry, but you already know this spell."}},
	{{eq(TOPIC, 1), "yes", lt(LEVEL, spellLevel(TYPE))}, {setAmount(spellLevel(TYPE)), "I'm sorry, but you need level %A to learn this spell."}},
	{{eq(TOPIC, 1), "yes", lt(COUNTMONEY, PRICE)}, {"I'm sorry, but you don't have enough gold to pay for it."}},
	{{eq(TOPIC, 1), "yes"}, {"Congratulations. From now on you can cast this spell.", deleteMoney, effectOpp(13), teachSpell(TYPE)}},
	{{eq(TOPIC, 1)}, {"As you wish."}},
}

function onCreatureSay(npc, creature, text, situation)
	behaviour.react(npc, rules, text, situation)
end

function onCreatureDisappear(npc, creature)
	behaviour.react(npc, rules, "", NPC_SITUATION_VANISH)
end
