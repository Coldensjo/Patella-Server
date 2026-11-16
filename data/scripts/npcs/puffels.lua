-- npcs/puffels.lua: the behaviour of the NPC Puffels, converted from
-- puffels.npc by patella-convert. The NPC is its [[npc]] entry in npcs.toml;
-- lib/npc.lua matches and runs the rules below, in the order of the file
-- (scripting.hh, "NPCs"). The comments are those of puffels.npc.
--
-- GIMUD - Graphical Interface Multi User Dungeon
-- puffels.npc Datenbank fuer den Zauberlehrer Puffels

local behaviour = require("lib.npc")
local ADDRESS = behaviour.ADDRESS
local BUSY = behaviour.BUSY
local VANISH = behaviour.VANISH
local KNIGHT = behaviour.KNIGHT
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
local idle = behaviour.idle
local nop = behaviour.nop
local previous = behaviour.previous

local rules = behaviour.rules {
	{{ADDRESS, "hello$", FORCE}, {"Yeah, another fool disturbing me, what a joy."}},
	{{ADDRESS, "hi$", FORCE}, {previous}},
	{{ADDRESS, FORCE}, {idle}},
	{{BUSY, "hello$", FORCE}, {"Shut up!"}},
	{{BUSY, "hi$", FORCE}, {previous}},
	{{BUSY, FORCE}, {nop}},
	{{VANISH, FORCE}, {"Whatever."}},

	{{"bye"}, {"Whatever.", idle}},
	{{"job"}, {"I have to teach the spells of least importance to some fools."}},
	{{"name"}, {"I am Magister Puffels, any problem with that?"}},
	{{"time"}, {"Where might I hide a watch, you fool?"}},
	{{"king"}, {"I give nothing for kings, queens... or other people at all."}},
	{{"tibianus"}, {previous}},
	{{"army"}, {"Fine army that is. Half of them have already deserted."}},
	{{"ferumbras"}, {"The day will come even he makes a fatal casting mistake... I know what I am talking about."}},
	{{"excalibug"}, {"I have no use for such stuff."}},
	{{"thais"}, {"I was there once, almost died. The fools there mistook me for an ordinary rat, can you believe that!?"}},
	{{"tibia"}, {"Bah, the whole Tibia can &#&$*# my #$&*!"}},
	{{"carlin"}, {"I don't care about some remote cities."}},
	{{"edron"}, {"Sciences are thriving on this isle."}},
	{{"news"}, {"I heard in Thais the new brand of cheese was... uhm..."}},
	{{"rumors"}, {previous}},
	{{"hugo"}, {"Hugo? I heared it was an accident that created this beast."}},
	{{"beast"}, {"I don't know more about it."}},

	{{"spellbook"}, {"Ask a shop owner for that."}},
	{{"spell"}, {"I have 'Magic Rope', 'Levitate', 'Haste', 'Berserk', 'Force Strike', 'Energy Strike', and 'Flame Strike'."}},

	{{"magic", "rope"}, {setType(76), setPrice(200), "Do you want to learn the spell 'Magic Rope' for %P gold?", setTopic(1)}},
	{{"levitate"}, {setType(81), setPrice(500), "Do you want to learn the spell 'Levitate' for %P gold?", setTopic(1)}},
	{{"haste"}, {setType(6), setPrice(600), "Do you want to learn the spell 'Haste' for %P gold?", setTopic(1)}},
	{{"berserk", KNIGHT}, {setType(80), setPrice(2500), "Do you want to learn the spell 'Berserk' for %P gold?", setTopic(1)}},
	{{"berserk"}, {"This spell is only for knights."}},
	{{"force", "strike", DRUID}, {setType(87), setPrice(600), "Do you want to learn the spell 'Force Strike' for %P gold?", setTopic(1)}},
	{{"force", "strike", SORCERER}, {previous}},
	{{"force", "strike"}, {"This spell is only for sorcerers and druids."}},
	{{"energy", "strike", DRUID}, {setType(88), setPrice(800), "Do you want to learn the spell 'Energy Strike' for %P gold?", setTopic(1)}},
	{{"energy", "strike", SORCERER}, {previous}},
	{{"energy", "strike"}, {"This spell is only for sorcerers and druids."}},
	{{"flame", "strike", DRUID}, {setType(89), setPrice(800), "Do you want to learn the spell 'Flame Strike' for %P gold?", setTopic(1)}},
	{{"flame", "strike", SORCERER}, {previous}},
	{{"flame", "strike"}, {"This spell is only for sorcerers and druids."}},

	{{eq(TOPIC, 1), "yes", eq(spellKnown(TYPE), 1)}, {"You already know this spell."}},
	{{eq(TOPIC, 1), "yes", lt(LEVEL, spellLevel(TYPE))}, {setAmount(spellLevel(TYPE)), "You must have level %A to learn this spell."}},
	{{eq(TOPIC, 1), "yes", lt(COUNTMONEY, PRICE)}, {"You do not have enough gold to pay my services."}},
	{{eq(TOPIC, 1), "yes"}, {"From now on you can cast this spell.", deleteMoney, effectOpp(13), teachSpell(TYPE)}},
	{{eq(TOPIC, 1)}, {"I thought so."}},
}

function onCreatureSay(npc, creature, text, situation)
	behaviour.react(npc, rules, text, situation)
end

function onCreatureDisappear(npc, creature)
	behaviour.react(npc, rules, "", NPC_SITUATION_VANISH)
end
