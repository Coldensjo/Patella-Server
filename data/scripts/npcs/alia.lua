-- npcs/alia.lua: the behaviour of the NPC Alia, converted from
-- alia.npc by patella-convert. The NPC is its [[npc]] entry in npcs.toml;
-- lib/npc.lua matches and runs the rules below, in the order of the file
-- (scripting.hh, "NPCs"). The comments are those of alia.npc.
--
-- GIMUD - Graphical Interface Multi User Dungeon
-- alia.npc: Datenbank für die Priesterin Alia

local behaviour = require("lib.npc")
local ADDRESS = behaviour.ADDRESS
local BUSY = behaviour.BUSY
local VANISH = behaviour.VANISH
local PVPENFORCED = behaviour.PVPENFORCED
local FORCE = behaviour.FORCE
local lt = behaviour.lt
local gt = behaviour.gt
local HP = behaviour.HP
local BURNING = behaviour.BURNING
local POISON = behaviour.POISON
local questValue = behaviour.questValue
local setHp = behaviour.setHp
local burning = behaviour.burning
local poison = behaviour.poison
local effectOpp = behaviour.effectOpp
local queue = behaviour.queue
local idle = behaviour.idle
local nop = behaviour.nop
local previous = behaviour.previous

local rules = behaviour.rules {
	{{ADDRESS, "hello$", FORCE}, {"Welcome %N! Welcome to the temple of Carlin."}},
	{{ADDRESS, "hi$", FORCE}, {previous}},
	{{ADDRESS, FORCE}, {idle}},
	{{BUSY, "hello$", FORCE}, {"Please wait a minute, %N.", queue}},
	{{BUSY, "hi$", FORCE}, {previous}},
	{{BUSY, "heal$", gt(BURNING, 0)}, {"You are burning, %N. I will help you.", burning(0, 0), effectOpp(15)}},
	{{BUSY, "heal$", gt(POISON, 0)}, {"You are poisoned, %N. I will help you.", poison(0, 0), effectOpp(14)}},
	{{BUSY, "heal$", lt(HP, 40)}, {"You are looking really bad, %N. Let me heal your wounds.", setHp(40), effectOpp(13)}},
	{{BUSY, "heal$"}, {"You aren't looking really bad, %N. Sorry, I can't help you."}},
	{{BUSY, FORCE}, {nop}},
	{{VANISH, FORCE}, {"Please return if you are heavily wounded or poisoned. I can heal you anytime."}},

	{{"bye"}, {"May the gods be with you, %N!", idle}},
	{{"farewell"}, {previous}},
	{{"job"}, {"I'm a nun, serving the gods of Tibia in this temple. I also heal wounded adventurers."}},
	{{"name"}, {"My name is Alia."}},
	{{"tibia"}, {"That's where we are. The world Tibia."}},
	{{"god"}, {"They created Tibia and all life on it."}},
	{{"ferumbras"}, {"Don't mention this name here."}},
	{{"excalibug"}, {"Sorry, I can't help you with that."}},
	{{"ghostlands"}, {"Uh, don't ask. Thats a place even the brave women of carlin don't dare to explore them!!!"}},

	{{"heal$", gt(BURNING, 0)}, {"You are burning. I will help you.", burning(0, 0), effectOpp(15)}},
	{{"heal$", gt(POISON, 0)}, {"You are poisoned. I will help you.", poison(0, 0), effectOpp(14)}},
	{{"heal$", lt(HP, 40)}, {"You are looking really bad. Let me heal your wounds.", setHp(40), effectOpp(13)}},

	{{"heal$", PVPENFORCED}, {"You aren't looking that bad."}},
	{{"heal$"}, {"You aren't looking that bad. Sorry, I can't help you. But if you are looking for additional protection you should go on the pilgrimage of ashes."}},

	{{"blessing", PVPENFORCED, FORCE}, {"The lifeforce of this world is waning. There are no more blessings avaliable on this world."}},
	{{"pilgrimage", PVPENFORCED, FORCE}, {previous}},
	{{"ashes", PVPENFORCED, FORCE}, {previous}},

	{{"spiritual", PVPENFORCED, FORCE}, {previous}},
	{{"shielding", PVPENFORCED, FORCE}, {previous}},
	{{"sacred", "places", PVPENFORCED, FORCE}, {previous}},
	{{"spark", PVPENFORCED, FORCE}, {previous}},
	{{"phoenix", PVPENFORCED, FORCE}, {previous}},
	{{"embrace", PVPENFORCED, FORCE}, {previous}},
	{{"fire", PVPENFORCED, FORCE}, {previous}},
	{{"suns", PVPENFORCED, FORCE}, {previous}},
	{{"wisdom", PVPENFORCED, FORCE}, {previous}},
	{{"solitude", PVPENFORCED, FORCE}, {previous}},

	{{"pilgrimage"}, {"Whenever you receive a lethal wound your lifeforce is damaged. With every single of the five blessings you have this damage will be reduced."}},
	{{"ashes"}, {previous}},
	{{"blessing"}, {"There are five different blessings available in five sacred places. These blessings are: the spiritual shielding, the spark of the phoenix, the embrace of tibia, the fire of the suns and the wisdom of solitude."}},
	{{"sacred", "places"}, {"Just ask in which of the five blessings you are interested in."}},

	{{"spiritual", gt(questValue(104), 0)}, {"I see you received the spiritual shielding in the whiteflower temple south of Thais."}},
	{{"shielding", gt(questValue(104), 0)}, {previous}},
	{{"spiritual"}, {"You can receive the spiritual shielding in the whiteflower temple south of Thais."}},
	{{"shielding"}, {previous}},

	{{"spark", gt(questValue(102), 0)}, {"I an sense that the spark of the phoenix already was given to you by the dwarven priests of earth and fire in Kazordoon."}},
	{{"phoenix", gt(questValue(102), 0)}, {previous}},
	{{"spark"}, {"The spark of the phoenix is given by the dwarven priests of earth and fire in Kazordoon."}},
	{{"phoenix"}, {previous}},

	{{"embrace", gt(questValue(105), 0)}, {"I can sense the druids north of Carlin provided you with the embrace of tibia."}},
	{{"embrace"}, {"The druids north of Carlin can provide you with the embrace of tibia."}},

	{{"fire", "suns", gt(questValue(103), 0)}, {"I can see you recieved the blessing of the two suns in the suntower near Ab'Dendriel ."}},
	{{"suns", gt(questValue(103), 0)}, {previous}},
	{{"fire", "suns"}, {"Ask for the blessing of the two suns in the suntower near Ab'Dendriel ."}},
	{{"suns"}, {previous}},

	{{"wisdom", gt(questValue(101), 0)}, {"I can sense you already talked to the hermit Eremo on the isle of Cormaya and recieved this blessing."}},
	{{"solitude", gt(questValue(101), 0)}, {previous}},
	{{"wisdom"}, {"Talk to the hermit Eremo on the isle of Cormaya about this blessing."}},
	{{"solitude"}, {previous}},

	{{"time"}, {"Now, it is %T. Ask Gorn for a watch, if you need one."}},
}

function onCreatureSay(npc, creature, text, situation)
	behaviour.react(npc, rules, text, situation)
end

function onCreatureDisappear(npc, creature)
	behaviour.react(npc, rules, "", NPC_SITUATION_VANISH)
end
