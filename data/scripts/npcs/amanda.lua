-- npcs/amanda.lua: the behaviour of the NPC Amanda, converted from
-- amanda.npc by patella-convert. The NPC is its [[npc]] entry in npcs.toml;
-- lib/npc.lua matches and runs the rules below, in the order of the file
-- (scripting.hh, "NPCs"). The comments are those of amanda.npc.
--
-- GIMUD - Graphical Interface Multi User Dungeon
-- amanda.npc: Datenbank für die Nonne Amanda

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
	{{ADDRESS, "hello$", FORCE}, {"Welcome to the temple of Banor's blood %N."}},
	{{ADDRESS, "hi$", FORCE}, {previous}},
	{{ADDRESS, FORCE}, {idle}},
	{{BUSY, "hello$", FORCE}, {"Please wait a moment, %N.", queue}},
	{{BUSY, "hi$", FORCE}, {previous}},
	{{BUSY, "heal$", gt(BURNING, 0)}, {"You are burning. I will help you.", burning(0, 0), effectOpp(15)}},
	{{BUSY, "heal$", gt(POISON, 0)}, {"You are poisoned. I will help you.", poison(0, 0), effectOpp(14)}},
	{{BUSY, "heal$", lt(HP, 40)}, {"You are looking really bad. Let me heal your wounds.", setHp(40), effectOpp(13)}},
	{{BUSY, "heal$"}, {"You aren't looking really bad. Sorry, I can't help you."}},
	{{BUSY, FORCE}, {nop}},
	{{VANISH, FORCE}, {"If you are heavily wounded or poisoned, feel free to return for a healing."}},

	{{"bye"}, {"Farewell, %N!", idle}},
	{{"job"}, {"I am a humble nun."}},
	{{"name"}, {"I am sister Amanda."}},
	{{"tibia"}, {"That's our world."}},
	{{"god"}, {"They created the world and all life on it."}},
	{{"king"}, {"Our king is a religious man. A shining example."}},
	{{"tibianus"}, {previous}},
	{{"army"}, {"Our army lives to the ideals of Banor."}},
	{{"banor"}, {previous}},
	{{"ferumbras"}, {"He is a pawn of evil."}},
	{{"excalibug"}, {"Only a being loyal to Banor will wield this blade."}},
	{{"news"}, {"Sorry, I rarely have time to chat."}},
	{{"time"}, {"Now, it is %T."}},

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
}

function onCreatureSay(npc, creature, text, situation)
	behaviour.react(npc, rules, text, situation)
end

function onCreatureDisappear(npc, creature)
	behaviour.react(npc, rules, "", NPC_SITUATION_VANISH)
end
