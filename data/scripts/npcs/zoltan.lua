-- npcs/zoltan.lua: the behaviour of the NPC Zoltan, converted from
-- zoltan.npc by patella-convert. The NPC is its [[npc]] entry in npcs.toml;
-- lib/npc.lua matches and runs the rules below, in the order of the file
-- (scripting.hh, "NPCs"). The comments are those of zoltan.npc.
--
-- GIMUD - Graphical Interface Multi User Dungeon
-- zoltan.npc Datenbank fuer den Zauberlehrer Zoltan

local behaviour = require("lib.npc")
local ADDRESS = behaviour.ADDRESS
local BUSY = behaviour.BUSY
local VANISH = behaviour.VANISH
local SORCERER = behaviour.SORCERER
local DRUID = behaviour.DRUID
local FORCE = behaviour.FORCE
local eq = behaviour.eq
local lt = behaviour.lt
local gt = behaviour.gt
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
local deleteMoney = behaviour.deleteMoney
local queue = behaviour.queue
local idle = behaviour.idle
local nop = behaviour.nop
local previous = behaviour.previous

local rules = behaviour.rules {
	{{ADDRESS, "hello$", FORCE}, {"Welcome %N, student of the arcane arts."}},
	{{ADDRESS, "hi$", FORCE}, {previous}},
	{{ADDRESS, FORCE}, {idle}},
	{{BUSY, "hello$", FORCE}, {"Wait %N. Your time will come.", queue}},
	{{BUSY, "hi$", FORCE}, {previous}},
	{{BUSY, FORCE}, {nop}},
	{{VANISH, FORCE}, {"Use your knowledge wisely."}},

	{{"bye"}, {"Use your knowledge wisely", idle}},
	{{"job"}, {"I am a teacher of the most powerful spells in Tibia."}},
	{{"name"}, {"I am known in this world as Zoltan."}},
	{{"time"}, {"It's %T."}},
	{{"king"}, {"King Tibianus III was the founder of our academy."}},
	{{"tibianus"}, {previous}},
	{{"army"}, {"They rely too much on their brawn instead of their brain."}},
	{{"ferumbras"}, {"A fallen sorcerer, indeed. What a shame."}},
	{{"excalibug"}, {"You will need no weapon if you manipulate the essence of magic."}},
	{{"thais"}, {"Thais is a place of barbary."}},
	{{"tibia"}, {"There is still much left to be explored in this world."}},
	{{"carlin"}, {"Carlin's druids waste the influence they have in enviromentalism."}},
	{{"edron"}, {"Sciences are thriving on this isle."}},
	{{"news"}, {"I have no time for chit chat."}},
	{{"rumors"}, {previous}},
	{{"eremo"}, {"He is an old and wise man that has seen a lot of Tibia. He is also one of the best magicians. Visit him on his little island."}},
	{{"visit"}, {"You should visit Eremo on his little island. Just ask Pemaret on Cormaya for passage."}},

	{{"yenny", "gentle"}, {"Ah, Yenny the Gentle was one of the founders of the druid order called Crunor's Caress, that has been originated in her hometown Carlin."}},
	{{"yenny"}, {"Yenny? Which Yenny? That is a common name."}},
	{{"crunor", "caress", eq(questValue(211), 1)}, {"A quite undruidic order of druids they were, as far as we know. I have no more enlightening knowledge about them though.", setQuestValue(211, 2)}},
	{{"crunor", "caress", gt(questValue(211), 1)}, {previous}},
	{{"crunor", "caress", eq(questValue(211), 0)}, {"I am quite busy, ask another time!"}},

	{{"spellbook"}, {"Don't bother me with that. Ask in the shops for it."}},
	{{"spell"}, {"I have some very powerful spells: 'Energy Bomb', 'Mass Healing', 'Poison Storm', 'Paralyze', and 'Ultimate Explosion'."}},

	{{"energy", "bomb", SORCERER}, {setType(55), setPrice(2300), "Are you prepared to learn the spell 'Energy Bomb' for %P gold?", setTopic(1)}},
	{{"energy", "bomb"}, {"No, no, no. This dangerous spell is only for sorcerers."}},
	{{"mass", "healing", DRUID}, {setType(82), setPrice(2200), "Are you prepared to learn the spell 'Mass Healing' for %P gold?", setTopic(1)}},
	{{"mass", "healing"}, {"No, no, no. This elemental spell is only for druids."}},
	{{"poison", "storm", DRUID}, {setType(56), setPrice(3400), "Are you prepared to learn the spell 'Poison Storm' for %P gold?", setTopic(1)}},
	{{"poison", "storm"}, {"No, no, no. This elemental spell is only for druids."}},
	{{"paralyze", DRUID}, {setType(54), setPrice(1900), "Are you prepared to learn the spell 'Paralyze' for %P gold?", setTopic(1)}},
	{{"paralyze"}, {"No, no, no. This elemental spell is only for druids."}},
	{{"ultimate", "explosion", SORCERER}, {setType(24), setPrice(8000), "Are you prepared to learn the spell 'Ultimate Explosion' for %P gold?", setTopic(1)}},
	{{"ultimate", "explosion"}, {"No, no, no. This dangerous spell is only for sorcerers."}},

	{{eq(TOPIC, 1), "yes", eq(spellKnown(TYPE), 1)}, {"Want to fool me? You already know this spell."}},
	{{eq(TOPIC, 1), "yes", lt(LEVEL, spellLevel(TYPE))}, {setAmount(spellLevel(TYPE)), "Don't be in high spirits. Advance to level %A, and then come again."}},
	{{eq(TOPIC, 1), "yes", lt(COUNTMONEY, PRICE)}, {"Want to learn one of the most powerful spells, and don't even know how much money you have?"}},
	{{eq(TOPIC, 1), "yes"}, {"Congratulations. You now know one of the most powerful spells. Use it wisely.", deleteMoney, effectOpp(13), teachSpell(TYPE)}},
	{{eq(TOPIC, 1)}, {"Lost your heart?"}},
}

function onCreatureSay(npc, creature, text, situation)
	behaviour.react(npc, rules, text, situation)
end

function onCreatureDisappear(npc, creature)
	behaviour.react(npc, rules, "", NPC_SITUATION_VANISH)
end
