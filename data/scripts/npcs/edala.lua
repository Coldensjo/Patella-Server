-- npcs/edala.lua: the behaviour of the NPC Edala, converted from
-- edala.npc by patella-convert. The NPC is its [[npc]] entry in npcs.toml;
-- lib/npc.lua matches and runs the rules below, in the order of the file
-- (scripting.hh, "NPCs"). The comments are those of edala.npc.
--
-- GIMUD - Graphical Interface Multi User Dungeon
-- edala.npc: Datenbank für die heilerin edala (nahe Elfenstadt)

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
	{{ADDRESS, "hello$", FORCE}, {"Ashari, %N."}},
	{{ADDRESS, "hi$", FORCE}, {previous}},
	{{ADDRESS, "ashari$", FORCE}, {previous}},
	{{ADDRESS, FORCE}, {idle}},
	{{BUSY, "hello$", FORCE}, {"Sorry, please wait a moment %N.", queue}},
	{{BUSY, "hi$", FORCE}, {previous}},
	{{BUSY, "ashari$", FORCE}, {previous}},
	{{BUSY, FORCE}, {nop}},
	{{VANISH, FORCE}, {"Asha Thrazi, traveller."}},

	{{"bye"}, {"Asha Thrazi, traveller.", idle}},
	{{"asha", "thrazi"}, {previous}},
	{{"farewell"}, {previous}},
	{{"job"}, {"I am a mystic of the suns. I provide protective blessings for those in need."}},
	{{"name"}, {"My name is Edala, pilgrim."}},

	{{"mystic"}, {"We mystics are philosophers and healers."}},

	{{"cenath"}, {"I don't consider me a member of any caste, and I don't want to talk about this matter."}},
	{{"kuridai"}, {previous}},
	{{"deraisim"}, {previous}},
	{{"abdaisim"}, {previous}},
	{{"teshial"}, {previous}},

	{{"crunor"}, {"Crunor is great in his beauty."}},
	{{"priyla"}, {"The daughter of the stars is my patron."}},

	{{"excalibug"}, {"It is true that this weapon brings great power. But you should not look for power. It is wisdom you really need."}},
	{{"news"}, {"News? I don't care about news."}},

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
	{{"sacred", "places"}, {"Just ask my about the blessing you are interested in."}},
	{{"spiritual"}, {" You may receive the spiritual shielding in the whiteflowertemple south of Thais."}},
	{{"shielding"}, {previous}},
	{{"spark"}, {"The spark of the phoenix will be given to you by the dwarven priests of earth and fire in Kazordoon."}},
	{{"phoenix"}, {previous}},
	{{"embrace"}, {"The druids north of Carlin will provide you with the embrace of tibia."}},

	{{"fire"}, {"Do you wish to receive the blessing of the two suns? It will cost you 10.000 gold, pilgrim.", setPrice(10000), setTopic(5)}},
	{{"suns"}, {previous}},
	{{"wisdom"}, {"Talk to the hermit Eremo on the isle of Cormaya about this blessing."}},
	{{"solitude"}, {previous}},

	{{eq(TOPIC, 5), "yes", gt(questValue(103), 0), FORCE}, {"You already possess this blessing."}},
	{{eq(TOPIC, 5), "yes", lt(COUNTMONEY, PRICE), FORCE}, {"Oh. You do not have enough money."}},
	{{eq(TOPIC, 5), "yes", FORCE}, {"Kneel down and receive the warmth of sunfire, pilgrim.", deleteMoney, effectOpp(13), setQuestValue(103, 3)}},
	{{eq(TOPIC, 5), FORCE}, {"All right. As you wish."}},
}

function onCreatureSay(npc, creature, text, situation)
	behaviour.react(npc, rules, text, situation)
end

function onCreatureDisappear(npc, creature)
	behaviour.react(npc, rules, "", NPC_SITUATION_VANISH)
end
