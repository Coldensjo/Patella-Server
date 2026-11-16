-- npcs/oracle.lua: the behaviour of the NPC The Oracle, converted from
-- oracle.npc by patella-convert. The NPC is its [[npc]] entry in npcs.toml;
-- lib/npc.lua matches and runs the rules below, in the order of the file
-- (scripting.hh, "NPCs"). The comments are those of oracle.npc.
--
-- GIMUD - Graphical Interface Multi User Dungeon
-- oracle.npc: Datenbank fuer das Orakel auf Rookgaard

local behaviour = require("lib.npc")
local ADDRESS = behaviour.ADDRESS
local BUSY = behaviour.BUSY
local VANISH = behaviour.VANISH
local PREMIUM = behaviour.PREMIUM
local FORCE = behaviour.FORCE
local eq = behaviour.eq
local ge = behaviour.ge
local TOPIC = behaviour.TOPIC
local TYPE = behaviour.TYPE
local DATA = behaviour.DATA
local LEVEL = behaviour.LEVEL
local setTopic = behaviour.setTopic
local setType = behaviour.setType
local setData = behaviour.setData
local effectOpp = behaviour.effectOpp
local profession = behaviour.profession
local queue = behaviour.queue
local idle = behaviour.idle
local nop = behaviour.nop
local teleport = behaviour.teleport
local startPosition = behaviour.startPosition
local previous = behaviour.previous

local rules = behaviour.rules {
	{{ADDRESS, "hello$", ge(LEVEL, 8), FORCE}, {"%N, ARE YOU PREPARED TO FACE YOUR DESTINY?"}},
	{{ADDRESS, "hi$", ge(LEVEL, 8), FORCE}, {previous}},
	{{ADDRESS, "greet", ge(LEVEL, 8), FORCE}, {previous}},
	{{ADDRESS, "hello$", FORCE}, {"CHILD! COME BACK WHEN YOU HAVE GROWN UP!", idle}},
	{{ADDRESS, "hi$", FORCE}, {previous}},
	{{ADDRESS, "greet", FORCE}, {previous}},
	{{ADDRESS, FORCE}, {idle}},
	{{BUSY, "hello$", ge(LEVEL, 8), FORCE}, {"WAIT UNTIL IT IS YOUR TURN!", queue}},
	{{BUSY, "hi$", ge(LEVEL, 8), FORCE}, {previous}},
	{{BUSY, "greet", ge(LEVEL, 8), FORCE}, {previous}},
	{{BUSY, "hello$", FORCE}, {"CHILD! COME BACK WHEN YOU HAVE GROWN UP!"}},
	{{BUSY, "hi$", FORCE}, {previous}},
	{{BUSY, "greet", FORCE}, {previous}},
	{{BUSY, FORCE}, {nop}},
	{{VANISH, FORCE}, {"COME BACK WHEN YOU ARE PREPARED TO FACE YOUR DESTINY!"}},

	{{"yes", PREMIUM}, {"IN WHICH TOWN DO YOU WANT TO LIVE: CARLIN, EDRON, THAIS, OR VENORE?", setTopic(1)}},
	{{"yes"}, {"IN WHICH TOWN DO YOU WANT TO LIVE: CARLIN, THAIS, OR VENORE?", setTopic(1)}},
	{{"bye", FORCE}, {"COME BACK WHEN YOU ARE PREPARED TO FACE YOUR DESTINY!", idle}},
	{{}, {previous}},

	{{eq(TOPIC, 1), "thais"}, {setData(1), "IN THAIS! AND WHAT PROFESSION HAVE YOU CHOSEN: KNIGHT, PALADIN, SORCERER, OR DRUID?", setTopic(2)}},
	{{eq(TOPIC, 1), "carlin"}, {setData(2), "IN CARLIN! AND WHAT PROFESSION HAVE YOU CHOSEN: KNIGHT, PALADIN, SORCERER, OR DRUID?", setTopic(2)}},
	{{eq(TOPIC, 1), "venore"}, {setData(3), "IN VENORE! AND WHAT PROFESSION HAVE YOU CHOSEN: KNIGHT, PALADIN, SORCERER, OR DRUID?", setTopic(2)}},
	{{eq(TOPIC, 1), "edron", PREMIUM}, {setData(4), "IN EDRON! AND WHAT PROFESSION HAVE YOU CHOSEN: KNIGHT, PALADIN, SORCERER, OR DRUID?", setTopic(2)}},
	{{eq(TOPIC, 1), "edron"}, {"YOU NEED A PREMIUM ACCOUNT IN ORDER TO GO THERE!", setTopic(1)}},
	{{eq(TOPIC, 1), PREMIUM}, {"CARLIN, EDRON, THAIS, OR VENORE?", setTopic(1)}},
	{{eq(TOPIC, 1)}, {"CARLIN, THAIS, OR VENORE?", setTopic(1)}},

	{{eq(TOPIC, 2), "knight"}, {setType(1), "A KNIGHT! ARE YOU SURE? THIS DECISION IS IRREVERSIBLE!", setTopic(3)}},
	{{eq(TOPIC, 2), "paladin"}, {setType(2), "A PALADIN! ARE YOU SURE? THIS DECISION IS IRREVERSIBLE!", setTopic(3)}},
	{{eq(TOPIC, 2), "sorcerer"}, {setType(3), "A SORCERER! ARE YOU SURE? THIS DECISION IS IRREVERSIBLE!", setTopic(3)}},
	{{eq(TOPIC, 2), "druid"}, {setType(4), "A DRUID! ARE YOU SURE? THIS DECISION IS IRREVERSIBLE!", setTopic(3)}},
	{{eq(TOPIC, 2)}, {"KNIGHT, PALADIN, SORCERER, OR DRUID?", setTopic(2)}},

	{{eq(TOPIC, 3), eq(DATA, 1), "yes"}, {"SO BE IT!", profession(TYPE), startPosition(32369, 32241, 7), idle, effectOpp(11), teleport(32369, 32241, 7), effectOpp(11)}},
	{{eq(TOPIC, 3), eq(DATA, 2), "yes"}, {"SO BE IT!", profession(TYPE), startPosition(32360, 31782, 7), idle, effectOpp(11), teleport(32360, 31782, 7), effectOpp(11)}},
	{{eq(TOPIC, 3), eq(DATA, 3), "yes"}, {"SO BE IT!", profession(TYPE), startPosition(32957, 32076, 7), idle, effectOpp(11), teleport(32957, 32076, 7), effectOpp(11)}},
	{{eq(TOPIC, 3), eq(DATA, 4), "yes"}, {"SO BE IT!", profession(TYPE), startPosition(33217, 31814, 8), idle, effectOpp(11), teleport(33217, 31814, 8), effectOpp(11)}},
}

function onCreatureSay(npc, creature, text, situation)
	behaviour.react(npc, rules, text, situation)
end

function onCreatureDisappear(npc, creature)
	behaviour.react(npc, rules, "", NPC_SITUATION_VANISH)
end
