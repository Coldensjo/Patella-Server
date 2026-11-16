-- npcs/eroth.lua: the behaviour of the NPC Eroth, converted from
-- eroth.npc by patella-convert. The NPC is its [[npc]] entry in npcs.toml;
-- lib/npc.lua matches and runs the rules below, in the order of the file
-- (scripting.hh, "NPCs"). The comments are those of eroth.npc.
--
-- GIMUD - Graphical Interface Multi User Dungeon
-- eroth.npc: Datenbank für den Cenath-Anführer Eroth (Elfenstadt)

local behaviour = require("lib.npc")
local ADDRESS = behaviour.ADDRESS
local BUSY = behaviour.BUSY
local VANISH = behaviour.VANISH
local PALADIN = behaviour.PALADIN
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
local idle = behaviour.idle
local nop = behaviour.nop
local previous = behaviour.previous

local rules = behaviour.rules {
	{{ADDRESS, "hello$", FORCE}, {"I greet thee, outsider."}},
	{{ADDRESS, "hi$", FORCE}, {previous}},
	{{ADDRESS, "ashari$", FORCE}, {previous}},
	{{ADDRESS, FORCE}, {idle}},
	{{BUSY, "hello$", FORCE}, {"Silence!"}},
	{{BUSY, "hi$", FORCE}, {previous}},
	{{BUSY, "ashari$", FORCE}, {previous}},
	{{BUSY, FORCE}, {nop}},
	{{VANISH, FORCE}, {"Asha Thrazi."}},

	{{"bye"}, {"Asha Thrazi. Go, where you have to go.", idle}},
	{{"farewell"}, {previous}},
	{{"asha", "thrazi"}, {previous}},
	{{"job"}, {"I am the leader of the Cenath caste."}},
	{{"name"}, {"My name is Eroth Ramathi."}},
	{{"time"}, {"That is a inferior concept invented by the minor races."}},

	{{"carlin"}, {"Their druids seek my counsel quite often. I provide them with as many insights their little minds can keep up with and I feel appropriate."}},
	{{"thais"}, {"A city of filth and dirt. Any elf should visit this city at least once to see what a society without good guidance can become."}},
	{{"venore"}, {"The merchants of venore have prooven usefull and are therefore tolerated."}},
	{{"roderick"}, {"A stupid human who won't comprehend our complex society."}},
	{{"olrik"}, {"A human who dreams to become an elf. It would be funny if it were not that pathetic."}},

	{{"king"}, {"Our people have no use for kings or queens."}},
	{{"tibianus"}, {previous}},
	{{"elves"}, {"Our people are the children of light and darkness, the heirs of dusk and dawn."}},
	{{"dwarfs"}, {"The diggers are not welcome in our realm."}},
	{{"humans"}, {"We tolerate them and allow them to be used by us."}},
	{{"troll"}, {"The Kuridai have the distasteful habit to keep some trolls for inferior work."}},
	{{"army"}, {"Stop this Kuridai nonsense."}},
	{{"cenath"}, {"We are the shepherds of our people. The other castes need our guidance."}},
	{{"kuridai"}, {"The Kuridai are aggressive and victims of their instincts. Without our help they would surely die in a foolish war."}},
	{{"deraisim"}, {"They lack the understanding of unity. We are keeping them together and prevent them from being slaughtered one by one."}},
	{{"abdaisim"}, {"They are fools and almost deserve the extinction that awaits them. Though we will take it upon us to rescue even them by leading them home."}},
	{{"teshial"}, {"They are gone. They alone were almost equal to us Cenath among elvenkind."}},
	{{"dreamer"}, {"The Teshial were masters of the so called dream magic."}},
	{{"dream", "master"}, {"The dream masters, though overestimated, wielded some impressive power without much practical use."}},
	{{"dreammaster"}, {previous}},
	{{"ferumbras"}, {"A human born evil. Another evidence of the destructive potential of that race."}},
	{{"crunor"}, {"Gods are for the weak. We will master the world on our own. We need no gods."}},
	{{"excalibug"}, {"Just another human myth."}},
	{{"news"}, {"I heared the new human settlement in the west became independent from the human empire."}},

	{{"magic"}, {"Magic comes almost naturally to the Cenath. We keep the secrets of ages."}},
	{{"druid"}, {"Druids master spells of defence, healing, and nature."}},
	{{"sorcerer"}, {"Sorcerers are not attuned to nature and therefore can't master it."}},
	{{"vocation"}, {"You are narrow minded to think in such boundaries."}},
	{{"spellbook"}, {"Cenath rarely use spellbooks. The minor castes rely on them though."}},
	{{"spell"}, {"I can teach the spells 'Magic Shield', 'Invisible', 'Destroy Field', 'Creature Illusion', 'Chameleon', 'Convince Creature', and 'Summon Creature'."}},

	{{PALADIN, "magic", "shield"}, {setType(44), setPrice(450), "Do you want to buy the spell 'Magic Shield' for %P gold?", setTopic(3)}},
	{{PALADIN, "destroy", "field"}, {setType(30), setPrice(700), "Do you want to buy the spell 'Destroy Field' for %P gold?", setTopic(3)}},
	{{PALADIN, "invisible"}, {setType(45), setPrice(2000), "Do you want to buy the spell 'Invisible' for %P gold?", setTopic(3)}},

	{{DRUID, "magic", "shield"}, {setType(44), setPrice(450), "Do you want to buy the spell 'Magic Shield' for %P gold?", setTopic(3)}},
	{{DRUID, "destroy", "field"}, {setType(30), setPrice(700), "Do you want to buy the spell 'Destroy Field' for %P gold?", setTopic(3)}},
	{{DRUID, "chameleon"}, {setType(14), setPrice(1300), "Do you want to buy the spell 'Chameleon' for %P gold?", setTopic(3)}},
	{{DRUID, "creature", "illusion"}, {setType(38), setPrice(1000), "Do you want to buy the spell 'Creature Illusion' for %P gold?", setTopic(3)}},
	{{DRUID, "convince", "creature"}, {setType(12), setPrice(800), "Do you want to buy the spell 'Convince Creature' for %P gold?", setTopic(3)}},
	{{DRUID, "summon", "creature"}, {setType(9), setPrice(2000), "Do you want to buy the spell 'Summon Creature' for %P gold?", setTopic(3)}},
	{{DRUID, "invisible"}, {setType(45), setPrice(2000), "Do you want to buy the spell 'Invisible' for %P gold?", setTopic(3)}},

	{{"magic", "shield"}, {"I'm sorry, but this spell is only for druids and paladins."}},
	{{"destroy", "field"}, {previous}},
	{{"chameleon"}, {"I'm sorry, but this spell is only for druids."}},
	{{"creature", "illusion"}, {previous}},
	{{"convince", "creature"}, {previous}},
	{{"summon", "creature"}, {previous}},

	{{eq(TOPIC, 3), "yes", eq(spellKnown(TYPE), 1)}, {"You already know that spell."}},
	{{eq(TOPIC, 3), "yes", lt(LEVEL, spellLevel(TYPE))}, {setAmount(spellLevel(TYPE)), "You need to advance to level %A to learn this spell."}},
	{{eq(TOPIC, 3), "yes", lt(COUNTMONEY, PRICE)}, {"You do not have enough gold to pay my services."}},
	{{eq(TOPIC, 3), "yes"}, {"From now on you can cast this spell.", deleteMoney, effectOpp(13), teachSpell(TYPE)}},
	{{eq(TOPIC, 3)}, {"I thought so."}},
}

function onCreatureSay(npc, creature, text, situation)
	behaviour.react(npc, rules, text, situation)
end

function onCreatureDisappear(npc, creature)
	behaviour.react(npc, rules, "", NPC_SITUATION_VANISH)
end
