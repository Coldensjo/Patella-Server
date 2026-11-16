-- npcs/chatterbone.lua: the behaviour of the NPC Chatterbone, converted from
-- chatterbone.npc by patella-convert. The NPC is its [[npc]] entry in npcs.toml;
-- lib/npc.lua matches and runs the rules below, in the order of the file
-- (scripting.hh, "NPCs"). The comments are those of chatterbone.npc.
--
-- GIMUD - Graphical Interface Multi User Dungeon
-- chatterbone.npc: Datenbank für den Magiehändler Chatterbone

local behaviour = require("lib.npc")
local ADDRESS = behaviour.ADDRESS
local BUSY = behaviour.BUSY
local VANISH = behaviour.VANISH
local SORCERER = behaviour.SORCERER
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
	{{ADDRESS, "hello$", FORCE}, {"... Greeeeeetiiiingssss"}},
	{{ADDRESS, "hi$", FORCE}, {previous}},
	{{ADDRESS, FORCE}, {idle}},
	{{BUSY, "hello$", FORCE}, {"... Wait... %N.", queue}},
	{{BUSY, "hi$", FORCE}, {previous}},
	{{BUSY, FORCE}, {nop}},
	{{VANISH, FORCE}, {"... Good... Bye"}},

	{{"bye"}, {"... Good... Bye", idle}},
	{{"farewell"}, {previous}},
	{{"job"}, {"... Selling Spells"}},
	{{"name"}, {"... Chatterbone"}},
	{{"time"}, {"... Time?... Not important... anymore."}},
	{{"king"}, {"..."}},
	{{"tibianus"}, {previous}},
	{{"vladruc"}, {"... Maaaaassssterrrrr"}},
	{{"urghain"}, {previous}},
	{{"ferumbras"}, {"... un...important"}},
	{{"market"}, {"... You buy?"}},
	{{"excalibug"}, {"... we hid it... so long ago... so long..."}},
	{{"news"}, {"... they build a new city... Carlin shall be its name..."}},
	{{"flaming", "pit"}, {"... we conquered them... held them so long... long ago..."}},
	{{"pits", "inferno"}, {previous}},
	{{"nightmare", "pit"}, {previous}},

	{{"sorcerer"}, {"... You... buy spells?"}},
	{{"power"}, {previous}},
	{{"druid"}, {"... Ask Smiley..."}},
	{{"spellbook"}, {"... You buy book... store spells... other counter..."}},
	{{"rune"}, {"... Runes... mighty stones... other counter..."}},
	{{SORCERER, "spell"}, {"... Spells... rune spells... instant spells... what you want? ... Or for which level?", setTopic(2)}},
	{{"spell"}, {"... Only sorcerers..."}},

	{{eq(TOPIC, 2), "rune", "spell"}, {"... Attack rune spells ...  support rune spells ... Which...?"}},
	{{eq(TOPIC, 2), "instant", "spell"}, {"... Attack spells ... healing spells ... supply spells ... support spells ... summon spells. Which...?"}},
	{{eq(TOPIC, 2), "level"}, {"Which level...?", setTopic(2)}},
	{{eq(TOPIC, 2), "bye"}, {"... Good... Bye", idle}},

	{{SORCERER, "wand", lt(questValue(333), 1)}, {"Oooh... present from meee... take it... goooood start for youuuung sorcerers...", setQuestValue(333, 1), setType(3074), setAmount(1), create(TYPE)}},

	{{SORCERER, "level"}, {"Which level...?", setTopic(2)}},
	{{SORCERER, "rune", "spell"}, {"... Attack rune spells ...  support rune spells ... Which...?"}},
	{{SORCERER, "instant", "spell"}, {"... Attack spells ... healing spells ... supply spells ... support spells ... summon spells. Which...?"}},

	{{SORCERER, "attack", "rune", "spell"}, {"... Missile rune spells ... explosive rune spells ... field rune spells ... wall rune spells ... bomb rune spells."}},
	{{SORCERER, "support", "rune", "spell"}, {"In this category ... 'Destroy Field'."}},

	{{SORCERER, "missile", "rune", "spell"}, {"In this category ... 'Light Magic Missile', 'Heavy Magic Missile' and 'Sudden Death'."}},
	{{SORCERER, "explosive", "rune", "spell"}, {"In this category ... 'Fireball', 'Great Fireball' and 'Explosion'."}},
	{{SORCERER, "field", "rune", "spell"}, {"In this category ... 'Poison Field', 'Fire Field' and 'Energy Field'."}},
	{{SORCERER, "wall", "rune", "spell"}, {"In this category ... 'Fire Wall', 'Poison Wall' and 'Energy Wall'."}},
	{{SORCERER, "bomb", "rune", "spell"}, {"In this category ... 'Firebomb'."}},

	{{SORCERER, "attack", "spell"}, {"In this category ... 'Fire Wave', 'Energy Wave', 'Energy Beam' and 'Great Energy Beam'."}},
	{{SORCERER, "healing", "spell"}, {"In this category ... 'Light Healing', 'Intense Healing', 'Ultimate Healing' and 'Antidote'."}},
	{{SORCERER, "support", "spell"}, {"In this category ... 'Find Person', 'Light', 'Great Light', 'Magic Shield', 'Creature Illusion' and 'Invisible'."}},
	{{SORCERER, "summon", "spell"}, {"In this category ... 'Summon Creature'."}},

	{{SORCERER, "find", "person"}, {setType(20), setPrice(80), "... You want 'Find Person' for %P gold?", setTopic(3)}},
	{{SORCERER, "light"}, {setType(10), setPrice(100), "... You want 'Light' for %P gold?", setTopic(3)}},
	{{SORCERER, "light", "healing"}, {setType(1), setPrice(170), "... You want 'Light Healing' for %P gold?", setTopic(3)}},
	{{SORCERER, "light", "missile"}, {setType(7), setPrice(500), "... You want 'Light Magic Missile' for %P gold?", setTopic(3)}},
	{{SORCERER, "antidote"}, {setType(29), setPrice(150), "... You want 'Antidote' for %P gold?", setTopic(3)}},
	{{SORCERER, "intense", "healing"}, {setType(2), setPrice(350), "... You want 'Intense Healing' for %P gold?", setTopic(3)}},
	{{SORCERER, "poison", "field"}, {setType(26), setPrice(300), "... You want 'Poison Field' for %P gold?", setTopic(3)}},
	{{SORCERER, "great", "light"}, {setType(11), setPrice(500), "... You want 'Great Light' for %P gold?", setTopic(3)}},
	{{SORCERER, "fire", "field"}, {setType(25), setPrice(500), "... You want 'Fire Field' for %P gold?", setTopic(3)}},
	{{SORCERER, "heavy", "missile"}, {setType(8), setPrice(1500), "... You want 'Heavy Magic Missile' for %P gold?", setTopic(3)}},
	{{SORCERER, "magic", "shield"}, {setType(44), setPrice(450), "... You want 'Magic Shield' for %P gold?", setTopic(3)}},
	{{SORCERER, "fireball"}, {setType(15), setPrice(800), "... You want 'Fireball' for %P gold?", setTopic(3)}},
	{{SORCERER, "energy", "field"}, {setType(27), setPrice(700), "... You want 'Energy Field' for %P gold?", setTopic(3)}},
	{{SORCERER, "destroy", "field"}, {setType(30), setPrice(700), "... You want 'Destroy Field' for %P gold?", setTopic(3)}},
	{{SORCERER, "fire", "wave"}, {setType(19), setPrice(850), "... You want 'Fire Wave' for %P gold?", setTopic(3)}},
	{{SORCERER, "ultimate", "healing"}, {setType(3), setPrice(1000), "... You want 'Ultimate Healing' for %P gold?", setTopic(3)}},
	{{SORCERER, "great", "fireball"}, {setType(16), setPrice(1200), "... You want 'Great Fireball' for %P gold?", setTopic(3)}},
	{{SORCERER, "fire", "bomb"}, {setType(17), setPrice(1500), "... You want 'Fire Bomb' for %P gold?", setTopic(3)}},
	{{SORCERER, "energy", "beam"}, {setType(22), setPrice(1000), "... You want 'Energy Beam' for %P gold?", setTopic(3)}},
	{{SORCERER, "creature", "illusion"}, {setType(38), setPrice(1000), "... You want 'Creature Illusion' for %P gold?", setTopic(3)}},
	{{SORCERER, "poison", "wall"}, {setType(32), setPrice(1600), "... You want 'Poison Wall' for %P gold?", setTopic(3)}},
	{{SORCERER, "explosion"}, {setType(18), setPrice(1800), "... You want 'Explosion' for %P gold?", setTopic(3)}},
	{{SORCERER, "fire", "wall"}, {setType(28), setPrice(2000), "... You want 'Fire Wall' for %P gold?", setTopic(3)}},
	{{SORCERER, "great", "energy", "beam"}, {setType(23), setPrice(1800), "... You want 'Great Energy Beam' for %P gold?", setTopic(3)}},
	{{SORCERER, "invisible"}, {setType(45), setPrice(2000), "... You want 'Invisible' for %P gold?", setTopic(3)}},
	{{SORCERER, "summon", "creature"}, {setType(9), setPrice(2000), "... You want 'Summon Creature' for %P gold?", setTopic(3)}},
	{{SORCERER, "energy", "wall"}, {setType(33), setPrice(2500), "... You want 'Energy Wall' for %P gold?", setTopic(3)}},
	{{SORCERER, "energy", "wave"}, {setType(13), setPrice(2500), "... You want 'Energy Wave' for %P gold?", setTopic(3)}},
	{{SORCERER, "sudden", "death"}, {setType(21), setPrice(3000), "... You want 'Sudden Death' for %P gold?", setTopic(3)}},

	{{eq(TOPIC, 2), "8$"}, {"... For level 8 ... 'Find Person' and 'Light'.", setTopic(2)}},
	{{eq(TOPIC, 2), "9$"}, {"... For level 9 ... 'Light Healing'.", setTopic(2)}},
	{{eq(TOPIC, 2), "10$"}, {"... For level 10 ... 'Antidote'.", setTopic(2)}},
	{{eq(TOPIC, 2), "11$"}, {"... For level 11 ... 'Intense Healing'.", setTopic(2)}},
	{{eq(TOPIC, 2), "13$"}, {"... For level 13 ... 'Great Light'.", setTopic(2)}},
	{{eq(TOPIC, 2), "14$"}, {"... For level 14 ... 'Poison Field' and 'Magic Shield'.", setTopic(2)}},
	{{eq(TOPIC, 2), "15$"}, {"... For level 15 ... 'Fire Field' and 'Light Magic Missile'.", setTopic(2)}},
	{{eq(TOPIC, 2), "17$"}, {"... For level 17 ... 'Fireball' and 'Destroy Field'.", setTopic(2)}},
	{{eq(TOPIC, 2), "18$"}, {"... For level 18 ... 'Energy Field' and 'Fire Wave'.", setTopic(2)}},
	{{eq(TOPIC, 2), "20$"}, {"... For level 20 ... 'Ultimate Healing'.", setTopic(2)}},
	{{eq(TOPIC, 2), "23$"}, {"... For level 23 ... 'Great Fireball', 'Energy Beam' and 'Creature Illusion'.", setTopic(2)}},
	{{eq(TOPIC, 2), "25$"}, {"... For level 25 ... 'Summon Creature' and 'Heavy Magic Missile'.", setTopic(2)}},
	{{eq(TOPIC, 2), "27$"}, {"... For level 27 ... 'Firebomb'.", setTopic(2)}},
	{{eq(TOPIC, 2), "29$"}, {"... For level 29 ... 'Poison Wall' and 'Great Energy Beam'.", setTopic(2)}},
	{{eq(TOPIC, 2), "31$"}, {"... For level 31 ... 'Explosion'.", setTopic(2)}},
	{{eq(TOPIC, 2), "33$"}, {"... For level 33 ... 'Fire Wall'.", setTopic(2)}},
	{{eq(TOPIC, 2), "35$"}, {"... For level 35 ... 'Invisible'.", setTopic(2)}},
	{{eq(TOPIC, 2), "38$"}, {"... For level 38 ... 'Energy Wave'.", setTopic(2)}},
	{{eq(TOPIC, 2), "41$"}, {"... For level 41 ... 'Energy Wall'.", setTopic(2)}},
	{{eq(TOPIC, 2), "45$"}, {"... For level 45 ... 'Sudden Death'.", setTopic(2)}},

	{{eq(TOPIC, 2)}, {"... No spells for this level ... but for many ... from 8 to 45.", setTopic(2)}},

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
