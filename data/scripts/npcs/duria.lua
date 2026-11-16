-- npcs/duria.lua: the behaviour of the NPC Duria, converted from
-- duria.npc by patella-convert. The NPC is its [[npc]] entry in npcs.toml;
-- lib/npc.lua matches and runs the rules below, in the order of the file
-- (scripting.hh, "NPCs"). The comments are those of duria.npc.
--
-- GIMUD - Graphical Interface Multi User Dungeon
-- duria.npc: Datenbank fuer die Ritterin Duria

local behaviour = require("lib.npc")
local ADDRESS = behaviour.ADDRESS
local BUSY = behaviour.BUSY
local VANISH = behaviour.VANISH
local KNIGHT = behaviour.KNIGHT
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
	{{ADDRESS, KNIGHT, "hello$", FORCE}, {"Hiho, fellow knight %N!"}},
	{{ADDRESS, KNIGHT, "hi$", FORCE}, {previous}},
	{{ADDRESS, KNIGHT, "hiho$", FORCE}, {previous}},
	{{ADDRESS, "hello$", FORCE}, {"Hiho, visitor %N. Whatdoyouwant?"}},
	{{ADDRESS, "hi$", FORCE}, {previous}},
	{{ADDRESS, "hiho$", FORCE}, {previous}},
	{{ADDRESS, FORCE}, {idle}},
	{{BUSY, "hello$", FORCE}, {"Waitaminute %N.", queue}},
	{{BUSY, "hi$", FORCE}, {previous}},
	{{BUSY, FORCE}, {nop}},
	{{VANISH, FORCE}, {"Becarefulloutthere, jawoll."}},

	{{"bye"}, {"Goodbye.", idle}},
	{{"farewell"}, {previous}},
	{{"job"}, {"Iam the Highknight of the dwarfs."}},
	{{"name"}, {"I am Duria Steelbender, daughter of Fire, of the Dragoneaters."}},
	{{"time"}, {"Dunno."}},
	{{"hero"}, {"Heroes are rare in this days, jawoll."}},
	{{"tibia"}, {"Bah, to much plantsandstuff, to few tunnels ifyoudaskme."}},
	{{"thais"}, {"Was there once. Can't handle the crime overthere."}},
	{{"knight"}, {"Knights are proud of being dwarfs, jawoll."}},
	{{"vocation"}, {"Vocation, vocation, wouldratherlike a vacation."}},
	{{"spellbook"}, {"Sellingno spellbooks here. Do I look like a sorc?"}},
	{{KNIGHT, "spell"}, {"Can teach ye healing spells and support spells. What kind of spell you like? Or for which level you want a spell?", setTopic(2)}},
	{{"spell"}, {"Sorry, selling spells only to knights, jawoll."}},

	{{KNIGHT, "instant", "spell"}, {"Can teach ye healing spells and support spells. What kind of spell you like?"}},
	{{KNIGHT, "healing", "spell"}, {"In this category I have 'Light Healing' and 'Antidote'."}},
	{{KNIGHT, "support", "spell"}, {"In this category I have 'Light', 'Find Person' and 'Great Light'."}},
	{{KNIGHT, "level"}, {"For which level you want a spell?", setTopic(2)}},

	{{KNIGHT, "find", "person"}, {setType(20), setPrice(80), "Do you want to buy the spell 'Find Person' for %P gold?", setTopic(3)}},
	{{KNIGHT, "light"}, {setType(10), setPrice(100), "Do you want to buy the spell 'Light' for %P gold?", setTopic(3)}},
	{{KNIGHT, "light", "healing"}, {setType(1), setPrice(170), "Do you want to buy the spell 'Light Healing' for %P gold?", setTopic(3)}},
	{{KNIGHT, "antidote"}, {setType(29), setPrice(150), "Do you want to buy the spell 'Antidote' for %P gold?", setTopic(3)}},
	{{KNIGHT, "great", "light"}, {setType(11), setPrice(500), "Do you want to buy the spell 'Great Light' for %P gold?", setTopic(3)}},

	{{eq(TOPIC, 2), "healing", "spell"}, {"In this category I have 'Light Healing' and 'Antidote'."}},
	{{eq(TOPIC, 2), "support", "spell"}, {"In this category I have 'Light', 'Find Person' and 'Great Light'."}},
	{{eq(TOPIC, 2), "bye"}, {"Goodbye.", idle}},

	{{eq(TOPIC, 2), "8$"}, {"For level 8 I have 'Find Person' and 'Light'.", setTopic(2)}},
	{{eq(TOPIC, 2), "9$"}, {"For level 9 I have 'Light Healing'.", setTopic(2)}},
	{{eq(TOPIC, 2), "10$"}, {"For level 10 I have 'Antidote'.", setTopic(2)}},
	{{eq(TOPIC, 2), "13$"}, {"For level 13 I have 'Great Light'.", setTopic(2)}},
	{{eq(TOPIC, 2)}, {"Sorry, I have only spells for level 8, 9, 10 and 13.", setTopic(2)}},

	{{eq(TOPIC, 3), "yes", eq(spellKnown(TYPE), 1)}, {"Youknowthatspell."}},
	{{eq(TOPIC, 3), "yes", lt(LEVEL, spellLevel(TYPE))}, {setAmount(spellLevel(TYPE)), "Nah, you havetobe level %A to learn this one."}},
	{{eq(TOPIC, 3), "yes", lt(COUNTMONEY, PRICE)}, {"Hey! Whereisyourgold?"}},
	{{eq(TOPIC, 3), "yes"}, {"Hereyouare. It's now in your spellbook, jawoll.", deleteMoney, effectOpp(13), teachSpell(TYPE)}},
	{{eq(TOPIC, 3)}, {"Maybe nexttime."}},
}

function onCreatureSay(npc, creature, text, situation)
	behaviour.react(npc, rules, text, situation)
end

function onCreatureDisappear(npc, creature)
	behaviour.react(npc, rules, "", NPC_SITUATION_VANISH)
end
