-- npcs/humphrey.lua: the behaviour of the NPC Humphrey, converted from
-- humphrey.npc by patella-convert. The NPC is its [[npc]] entry in npcs.toml;
-- lib/npc.lua matches and runs the rules below, in the order of the file
-- (scripting.hh, "NPCs"). The comments are those of humphrey.npc.
--
-- GIMUD - Graphical Interface Multi User Dungeon
-- humphrey.npc: Datenbank für den druiden humphrey

local behaviour = require("lib.npc")
local ADDRESS = behaviour.ADDRESS
local BUSY = behaviour.BUSY
local VANISH = behaviour.VANISH
local PVPENFORCED = behaviour.PVPENFORCED
local FORCE = behaviour.FORCE
local eq = behaviour.eq
local lt = behaviour.lt
local gt = behaviour.gt
local TOPIC = behaviour.TOPIC
local PRICE = behaviour.PRICE
local HP = behaviour.HP
local BURNING = behaviour.BURNING
local POISON = behaviour.POISON
local COUNTMONEY = behaviour.COUNTMONEY
local questValue = behaviour.questValue
local setTopic = behaviour.setTopic
local setPrice = behaviour.setPrice
local setHp = behaviour.setHp
local burning = behaviour.burning
local poison = behaviour.poison
local setQuestValue = behaviour.setQuestValue
local effectOpp = behaviour.effectOpp
local deleteMoney = behaviour.deleteMoney
local queue = behaviour.queue
local idle = behaviour.idle
local nop = behaviour.nop
local previous = behaviour.previous

local rules = behaviour.rules {
	{{ADDRESS, "hello$", FORCE}, {"Welcome, child of nature."}},
	{{ADDRESS, "hi$", FORCE}, {previous}},
	{{ADDRESS, FORCE}, {idle}},
	{{BUSY, "hello$", FORCE}, {"Please wait a minute, %N.", queue}},
	{{BUSY, "hi$", FORCE}, {previous}},
	{{BUSY, "heal$", gt(BURNING, 0)}, {"You are burning, %N. I will help you.", burning(0, 0), effectOpp(15)}},
	{{BUSY, "heal$", gt(POISON, 0)}, {"You are poisoned, %N. I will help you.", poison(0, 0), effectOpp(14)}},
	{{BUSY, "heal$", lt(HP, 40)}, {"You are looking really bad, %N. Let me heal your wounds.", setHp(40), effectOpp(13)}},
	{{BUSY, "heal$"}, {"You aren't looking really bad, %N. Sorry, I can't help you."}},
	{{BUSY, FORCE}, {nop}},
	{{VANISH, FORCE}, {"Remember: If you are heavily wounded or poisoned, I will heal you."}},

	{{"bye"}, {"Good bye, %N!", idle}},
	{{"farewell"}, {previous}},
	{{"job"}, {"I am a guardian of nature. Also I am the keeper of the embrace of tibia, one of the five blessings."}},
	{{"name"}, {"My name is Humphrey."}},
	{{"heal$", gt(BURNING, 0)}, {"You are burning. I will help you.", burning(0, 0), effectOpp(15)}},
	{{"heal$", gt(POISON, 0)}, {"You are poisoned. I will help you.", poison(0, 0), effectOpp(14)}},
	{{"heal$", lt(HP, 40)}, {"You are looking that bad. Let me heal your wounds.", setHp(40), effectOpp(13)}},

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

	{{"pilgrimage"}, {"Whenever you recrive a lethal wound your lifeforce is damaged. With every single of the five blessings you have this damage will be reduced."}},
	{{"ashes"}, {previous}},
	{{"blessing"}, {"There are five different blessings available in five sacred places. They are: the spiritual shielding, the spark of the phoenix, the embrace of tibia, the fire of the suns and the wisdom of solitude."}},
	{{"sacred", "places"}, {"Just ask in which of the five blessings you are interested in."}},
	{{"spiritual"}, {"You can ask for the blessing of spiritual shielding the whiteflower temple south of Thais."}},
	{{"shielding"}, {previous}},
	{{"spark"}, {"The spark of the phoenix will be given to you by the dwarfen priests of earth and fire in Kazordoon."}},
	{{"phoenix"}, {previous}},
	{{"embrace"}, {"I will give to you the embrace of tibia, but you will have to make a sacrifice. Are you prepared to pay 10.000 gold for the blessing?.", setPrice(10000), setTopic(5)}},

	{{"fire"}, {"You can ask for the blessing of the two suns in the suntower near Ab'Dendriel ."}},
	{{"suns"}, {previous}},
	{{"wisdom"}, {"Talk to the hermit Eremo on the isle of Cormaya about this blessing."}},
	{{"solitude"}, {previous}},

	{{eq(TOPIC, 5), "yes", gt(questValue(105), 0)}, {"You already possess this blessing."}},
	{{eq(TOPIC, 5), "yes", lt(COUNTMONEY, PRICE)}, {"Oh. You do not have enough money."}},
	{{eq(TOPIC, 5), "yes"}, {"So receive the embrace of tibia, pilgrim", deleteMoney, effectOpp(13), setQuestValue(105, 1)}},
	{{eq(TOPIC, 5)}, {"Fine. You are free to decline my offer."}},

	{{"time"}, {"Now, it is %T. Ask Gorn for a watch, if you need one."}},
}

function onCreatureSay(npc, creature, text, situation)
	behaviour.react(npc, rules, text, situation)
end

function onCreatureDisappear(npc, creature)
	behaviour.react(npc, rules, "", NPC_SITUATION_VANISH)
end
