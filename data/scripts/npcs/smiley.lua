-- npcs/smiley.lua: the behaviour of the NPC Smiley, converted from
-- smiley.npc by patella-convert. The NPC is its [[npc]] entry in npcs.toml;
-- lib/npc.lua matches and runs the rules below, in the order of the file
-- (scripting.hh, "NPCs"). The comments are those of smiley.npc.
--
-- GIMUD - Graphical Interface Multi User Dungeon
-- smiley.npc: Datenbank für den Magiehändler Smiley

local behaviour = require("lib.npc")
local ADDRESS = behaviour.ADDRESS
local BUSY = behaviour.BUSY
local VANISH = behaviour.VANISH
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
local questValue = behaviour.questValue
local setTopic = behaviour.setTopic
local setPrice = behaviour.setPrice
local setAmount = behaviour.setAmount
local setType = behaviour.setType
local setQuestValue = behaviour.setQuestValue
local effectOpp = behaviour.effectOpp
local teachSpell = behaviour.teachSpell
local create = behaviour.create
local deleteMoney = behaviour.deleteMoney
local queue = behaviour.queue
local idle = behaviour.idle
local nop = behaviour.nop
local previous = behaviour.previous

local rules = behaviour.rules {
	{{ADDRESS, "hello$", FORCE}, {"... Greeeeeetiiiingssss..."}},
	{{ADDRESS, "hi$", FORCE}, {previous}},
	{{ADDRESS, FORCE}, {idle}},
	{{BUSY, "hello$", FORCE}, {"... Wait... %N.", queue}},
	{{BUSY, "hi$", FORCE}, {previous}},
	{{BUSY, FORCE}, {nop}},
	{{VANISH, FORCE}, {"... Good... Bye"}},

	{{"bye"}, {"... Good... Bye", idle}},
	{{"farewell"}, {previous}},
	{{"job"}, {"... Selling Spells"}},
	{{"name"}, {"... Smiley"}},
	{{"time"}, {"... Time?... Not important... anymore."}},
	{{"king"}, {"..."}},
	{{"tibianus"}, {previous}},
	{{"vladruc"}, {"... Maaaaassssterrrrr"}},
	{{"urghain"}, {previous}},
	{{"ferumbras"}, {"... un...important"}},
	{{"market"}, {"... You buy?"}},
	{{"excalibug"}, {"... only sell spells..."}},
	{{"news"}, {"... more spells..."}},

	{{"sorcerer"}, {"... Ask Chatterbone?"}},
	{{"druid"}, {"... You... buy spells?"}},
	{{"power"}, {previous}},
	{{"spellbook"}, {"... You buy book... store spells... other counter..."}},
	{{"rune"}, {"... Runes... mighty stones... other counter..."}},
	{{DRUID, "spell"}, {"... Spells... rune spells... instant spells... what you want? ... Or for which level?", setTopic(2)}},
	{{"spell"}, {"... Only druids..."}},

	{{DRUID, "rod", lt(questValue(333), 1)}, {"Oooh... present from meee... take it... goooood start for youuuung druuuids...", setQuestValue(333, 1), setType(3066), setAmount(1), create(TYPE)}},

	{{eq(TOPIC, 2), "rune", "spell"}, {"... Attack rune spells ... healing rune spells ... support rune spells ... summon rune spells. Which...?"}},
	{{eq(TOPIC, 2), "instant", "spell"}, {"... Healing spells ... supply spells ... support spells ... summon spells. Which...?"}},
	{{eq(TOPIC, 2), "level"}, {"Which level...?", setTopic(2)}},
	{{eq(TOPIC, 2), "bye"}, {"... Good... Bye", idle}},

	{{DRUID, "level"}, {"... Spell... which level...?", setTopic(2)}},
	{{DRUID, "rune", "spell"}, {"... Attack rune spells ... healing rune spells ... support rune spells ... summon rune spells. Which...?"}},
	{{DRUID, "instant", "spell"}, {"... Healing spells ... supply spells ... support spells ... summon spells. Which...?"}},

	{{DRUID, "attack", "rune", "spell"}, {"... Missile rune spells ... explosive rune spells ... field rune spells ... wall rune spells ... bomb rune spells."}},
	{{DRUID, "healing", "rune", "spell"}, {"In this category ... 'Intense Healing Rune', 'Antidote Rune' and 'Ultimate Healing Rune'."}},
	{{DRUID, "support", "rune", "spell"}, {"In this category ... 'Destroy Field' and 'Chameleon'."}},
	{{DRUID, "summon", "rune", "spell"}, {"In this category ... 'Convince Creature'."}},

	{{DRUID, "missile", "rune", "spell"}, {"In this category ... 'Light Magic Missile' and 'Heavy Magic Missile'."}},
	{{DRUID, "explosive", "rune", "spell"}, {"In this category ... 'Fireball', 'Great Fireball' and 'Explosion'."}},
	{{DRUID, "field", "rune", "spell"}, {"In this category ... 'Poison Field', 'Fire Field' and 'Energy Field'."}},
	{{DRUID, "wall", "rune", "spell"}, {"In this category ... 'Fire Wall', 'Poison Wall' and 'Energy Wall'."}},
	{{DRUID, "bomb", "rune", "spell"}, {"In this category ... 'Firebomb'."}},

	{{DRUID, "healing", "spell"}, {"In this category ... 'Light Healing', 'Intense Healing', 'Ultimate Healing' and 'Antidote'."}},
	{{DRUID, "supply", "spell"}, {"In this category ... 'Food'."}},
	{{DRUID, "support", "spell"}, {"In this category ... 'Find Person', 'Light', 'Great Light', 'Magic Shield', 'Creature Illusion' and 'Invisible'."}},
	{{DRUID, "summon", "spell"}, {"In this category ... 'Summon Creature'."}},

	{{DRUID, "find", "person"}, {setType(20), setPrice(80), "... You want 'Find Person' for %P gold?", setTopic(3)}},
	{{DRUID, "light"}, {setType(10), setPrice(100), "... You want 'Light' for %P gold?", setTopic(3)}},
	{{DRUID, "food"}, {setType(42), setPrice(300), "... You want 'Food' for %P gold?", setTopic(3)}},
	{{DRUID, "light", "healing"}, {setType(1), setPrice(170), "... You want 'Light Healing' for %P gold?", setTopic(3)}},
	{{DRUID, "light", "missile"}, {setType(7), setPrice(500), "... You want 'Light Magic Missile' for %P gold?", setTopic(3)}},
	{{DRUID, "antidote"}, {setType(29), setPrice(150), "... You want 'Antidote' for %P gold?", setTopic(3)}},
	{{DRUID, "intense", "healing"}, {setType(2), setPrice(350), "... You want 'Intense Healing' for %P gold?", setTopic(3)}},
	{{DRUID, "poison", "field"}, {setType(26), setPrice(300), "... You want 'Poison Field' for %P gold?", setTopic(3)}},
	{{DRUID, "great", "light"}, {setType(11), setPrice(500), "... You want 'Great Light' for %P gold?", setTopic(3)}},
	{{DRUID, "fire", "field"}, {setType(25), setPrice(500), "... You want 'Fire Field' for %P gold?", setTopic(3)}},
	{{DRUID, "heavy", "missile"}, {setType(8), setPrice(1500), "... You want 'Heavy Magic Missile' for %P gold?", setTopic(3)}},
	{{DRUID, "magic", "shield"}, {setType(44), setPrice(450), "... You want 'Magic Shield' for %P gold?", setTopic(3)}},
	{{DRUID, "intense", "healing", "rune"}, {setType(4), setPrice(600), "... You want 'Intense Healing Rune' for %P gold?", setTopic(3)}},
	{{DRUID, "antidote", "rune"}, {setType(31), setPrice(600), "... You want 'Antidote Rune' for %P gold?", setTopic(3)}},
	{{DRUID, "fireball"}, {setType(15), setPrice(800), "... You want 'Fireball' for %P gold?", setTopic(3)}},
	{{DRUID, "energy", "field"}, {setType(27), setPrice(700), "... You want 'Energy Field' for %P gold?", setTopic(3)}},
	{{DRUID, "destroy", "field"}, {setType(30), setPrice(700), "... You want 'Destroy Field' for %P gold?", setTopic(3)}},
	{{DRUID, "ultimate", "healing"}, {setType(3), setPrice(1000), "... You want 'Ultimate Healing' for %P gold?", setTopic(3)}},
	{{DRUID, "great", "fireball"}, {setType(16), setPrice(1200), "... You want 'Great Fireball' for %P gold?", setTopic(3)}},
	{{DRUID, "fire", "bomb"}, {setType(17), setPrice(1500), "... You want 'Fire Bomb' for %P gold?", setTopic(3)}},
	{{DRUID, "creature", "illusion"}, {setType(38), setPrice(1000), "... You want 'Creature Illusion' for %P gold?", setTopic(3)}},
	{{DRUID, "convince", "creature"}, {setType(12), setPrice(800), "... You want 'Convince Creature' for %P gold?", setTopic(3)}},
	{{DRUID, "ultimate", "healing", "rune"}, {setType(5), setPrice(1500), "... You want 'Ultimate Healing Rune' for %P gold?", setTopic(3)}},
	{{DRUID, "chameleon"}, {setType(14), setPrice(1300), "... You want 'Chameleon' for %P gold?", setTopic(3)}},
	{{DRUID, "poison", "wall"}, {setType(32), setPrice(1600), "... You want 'Poison Wall' for %P gold?", setTopic(3)}},
	{{DRUID, "explosion"}, {setType(18), setPrice(1800), "... You want 'Explosion' for %P gold?", setTopic(3)}},
	{{DRUID, "fire", "wall"}, {setType(28), setPrice(2000), "... You want 'Fire Wall' for %P gold?", setTopic(3)}},
	{{DRUID, "Invisible"}, {setType(45), setPrice(2000), "... You want 'Invisible' for %P gold?", setTopic(3)}},
	{{DRUID, "summon", "creature"}, {setType(9), setPrice(2000), "... You want 'Summon Creature' for %P gold?", setTopic(3)}},
	{{DRUID, "energy", "wall"}, {setType(33), setPrice(2500), "... You want 'Energy Wall' for %P gold?", setTopic(3)}},

	{{eq(TOPIC, 2), "8$"}, {"... For level 8 ... 'Find Person' and 'Light'.", setTopic(2)}},
	{{eq(TOPIC, 2), "9$"}, {"... For level 9 ... 'Light Healing'.", setTopic(2)}},
	{{eq(TOPIC, 2), "10$"}, {"... For level 10 ... 'Antidote'.", setTopic(2)}},
	{{eq(TOPIC, 2), "11$"}, {"... For level 11 ... 'Intense Healing'.", setTopic(2)}},
	{{eq(TOPIC, 2), "13$"}, {"... For level 13 ... 'Great Light'.", setTopic(2)}},
	{{eq(TOPIC, 2), "14$"}, {"... For level 14 ... 'Food', 'Poison Field' and 'Magic Shield'.", setTopic(2)}},
	{{eq(TOPIC, 2), "15$"}, {"... For level 15 ... 'Fire Field', 'Intense Healing Rune', 'Antidote Rune' and 'Light Magic Missile'.", setTopic(2)}},
	{{eq(TOPIC, 2), "16$"}, {"... For level 16 ... 'Convince Creature'.", setTopic(2)}},
	{{eq(TOPIC, 2), "17$"}, {"... For level 17 ... 'Fireball' and 'Destroy Field'.", setTopic(2)}},
	{{eq(TOPIC, 2), "18$"}, {"... For level 18 ... 'Energy Field'.", setTopic(2)}},
	{{eq(TOPIC, 2), "20$"}, {"... For level 20 ... 'Ultimate Healing'.", setTopic(2)}},
	{{eq(TOPIC, 2), "23$"}, {"... For level 23 ... 'Great Fireball' and 'Creature Illusion'.", setTopic(2)}},
	{{eq(TOPIC, 2), "24$"}, {"... For level 24 ... 'Ultimate Healing Rune'.", setTopic(2)}},
	{{eq(TOPIC, 2), "25$"}, {"... For level 25 ... 'Summon Creature' and 'Heavy Magic Missile'.", setTopic(2)}},
	{{eq(TOPIC, 2), "27$"}, {"... For level 27 ... 'Firebomb' and 'Chameleon'.", setTopic(2)}},
	{{eq(TOPIC, 2), "29$"}, {"... For level 29 ... 'Poison Wall'.", setTopic(2)}},
	{{eq(TOPIC, 2), "31$"}, {"... For level 31 ... 'Explosion'.", setTopic(2)}},
	{{eq(TOPIC, 2), "33$"}, {"... For level 33 ... 'Fire Wall'.", setTopic(2)}},
	{{eq(TOPIC, 2), "35$"}, {"... For level 35 ... 'Invisible'.", setTopic(2)}},
	{{eq(TOPIC, 2), "41$"}, {"... For level 41 ... 'Energy Wall'.", setTopic(2)}},

	{{eq(TOPIC, 2)}, {"... Only spells for level 8 to 11 ... 13 to 18 ... 20 ... 23 to 25 ... 27 ... 29 ... 31 ... 33 ... 35 and 41...", setTopic(2)}},

	{{eq(TOPIC, 3), "yes", eq(spellKnown(TYPE), 1)}, {"... You already know..."}},
	{{eq(TOPIC, 3), "yes", lt(LEVEL, spellLevel(TYPE))}, {setAmount(spellLevel(TYPE)), "... not level %A..."}},
	{{eq(TOPIC, 3), "yes", lt(COUNTMONEY, PRICE)}, {"... More money."}},
	{{eq(TOPIC, 3), "yes"}, {"... Here...", deleteMoney, effectOpp(13), teachSpell(TYPE)}},
	{{eq(TOPIC, 3)}, {"... Then not."}},
}

function onCreatureSay(npc, creature, text, situation)
	behaviour.react(npc, rules, text, situation)
end

function onCreatureDisappear(npc, creature)
	behaviour.react(npc, rules, "", NPC_SITUATION_VANISH)
end
