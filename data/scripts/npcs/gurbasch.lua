-- npcs/gurbasch.lua: the behaviour of the NPC Gurbasch, converted from
-- gurbasch.npc by patella-convert. The NPC is its [[npc]] entry in npcs.toml;
-- lib/npc.lua matches and runs the rules below, in the order of the file
-- (scripting.hh, "NPCs"). The comments are those of gurbasch.npc.
--
-- GIMUD - Graphical Interface Multi User Dungeon
-- gurbasch.npc: Datenbank für den Kapitän Gurbasch

local behaviour = require("lib.npc")
local ADDRESS = behaviour.ADDRESS
local BUSY = behaviour.BUSY
local VANISH = behaviour.VANISH
local PREMIUM = behaviour.PREMIUM
local PZBLOCK = behaviour.PZBLOCK
local FORCE = behaviour.FORCE
local eq = behaviour.eq
local gt = behaviour.gt
local ge = behaviour.ge
local TOPIC = behaviour.TOPIC
local PRICE = behaviour.PRICE
local COUNTMONEY = behaviour.COUNTMONEY
local questValue = behaviour.questValue
local setTopic = behaviour.setTopic
local setPrice = behaviour.setPrice
local effectOpp = behaviour.effectOpp
local deleteMoney = behaviour.deleteMoney
local queue = behaviour.queue
local idle = behaviour.idle
local nop = behaviour.nop
local teleport = behaviour.teleport
local previous = behaviour.previous

local rules = behaviour.rules {
	{{ADDRESS, "hello$", FORCE}, {"Welcome, %N!"}},
	{{ADDRESS, "hi$", FORCE}, {previous}},
	{{ADDRESS, "hiho$", FORCE}, {previous}},
	{{ADDRESS, FORCE}, {idle}},
	{{BUSY, "hello$", FORCE}, {"Don't hurry.", queue}},
	{{BUSY, "hi$", FORCE}, {previous}},
	{{BUSY, "hiho$", FORCE}, {previous}},
	{{BUSY, FORCE}, {nop}},
	{{VANISH, FORCE}, {"No patience, these brats!"}},

	{{"bye"}, {"Until next time.", idle}},
	{{"farewell"}, {previous}},
	{{"fare", "thee", "well"}, {"Thou art truly in possession of fine manners. Blessings upon you!", effectOpp(13)}},
	{{"job"}, {"As should be quite obvious, I am operating a steamship."}},
	{{"work"}, {previous}},
	{{"name"}, {"I am Gurbasch Firejuggler, son of the machine, of the Molten Rock."}},
	{{"tibia"}, {"Tibia? Just don't ask."}},
	{{"ship"}, {"It is indeed something we dwarfs may be proud of: a ship operating by steam power."}},
	{{"steamship"}, {previous}},
	{{"captain"}, {"Captain"}},
	{{"technomancer"}, {"A technomancer wields power over incredible machines, as his knowledge is his magic."}},
	{{"inventors"}, {"You know, elves may be intelligent, but they are too lazy to invent. Really."}},
	{{"inventions"}, {previous}},
	{{"sell"}, {"I am not a vendor."}},
	{{"buy"}, {previous}},
	{{"thais"}, {"How do you expect me to go there? Fly? Hm, wait... no, sorry."}},
	{{"ab'dendriel"}, {previous}},
	{{"carlin"}, {previous}},
	{{"venore"}, {previous}},
	{{"senja"}, {previous}},
	{{"folda"}, {previous}},
	{{"vega"}, {previous}},
	{{"ice", "islands"}, {previous}},
	{{"darashia"}, {previous}},
	{{"darama"}, {previous}},
	{{"cormaya"}, {"Hey, we ARE at Cormaya! Must be the cavemadness..."}},
	{{"beer"}, {"Ah, you got some? Nah, beer only tastes fine in Kazordoon. If you have brought it from there, it tastes foul now, I guess."}},
	{{"dwarf"}, {"We are an old and proud race, although we posess the best inventions."}},
	{{"brodrosch"}, {"He is my brother working the Kazordoon steamship."}},
	{{"elves"}, {"Have one elf onboard a ship, and you are doomed."}},
	{{"elf"}, {previous}},

	{{"kazordoon"}, {setPrice(160), "Do you want to go to Kazordoon? And try the beer there? %P gold?", setTopic(1)}},
	{{"passage"}, {previous}},

	{{"kazordoon", gt(questValue(250), 2)}, {setPrice(150), "Do you want to go to Kazordoon? And try the beer there? %P gold?", setTopic(1)}},
	{{"passage", gt(questValue(250), 2)}, {previous}},

	{{eq(TOPIC, 1), "yes", PZBLOCK, FORCE}, {"First get rid of those blood stains! You are not going to ruin my vehicle!"}},

	{{eq(TOPIC, 1), "yes", PREMIUM, ge(COUNTMONEY, PRICE)}, {"Full steam ahead!", deleteMoney, idle, effectOpp(3), teleport(32658, 31957, 15), effectOpp(3)}},
	{{eq(TOPIC, 1), "yes", ge(COUNTMONEY, PRICE)}, {"I'm sorry, but you need a premium account in order to travel onboard our ships."}},
	{{eq(TOPIC, 1), "yes"}, {"You don't have enough money."}},
}

function onCreatureSay(npc, creature, text, situation)
	behaviour.react(npc, rules, text, situation)
end

function onCreatureDisappear(npc, creature)
	behaviour.react(npc, rules, "", NPC_SITUATION_VANISH)
end
