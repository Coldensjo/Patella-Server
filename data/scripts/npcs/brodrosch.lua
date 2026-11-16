-- npcs/brodrosch.lua: the behaviour of the NPC Brodrosch, converted from
-- brodrosch.npc by patella-convert. The NPC is its [[npc]] entry in npcs.toml;
-- lib/npc.lua matches and runs the rules below, in the order of the file
-- (scripting.hh, "NPCs"). The comments are those of brodrosch.npc.
--
-- GIMUD - Graphical Interface Multi User Dungeon
-- brodrosch.npc: Datenbank für den Kapitän Brodrosch

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
local setQuestValue = behaviour.setQuestValue
local effectOpp = behaviour.effectOpp
local deleteMoney = behaviour.deleteMoney
local queue = behaviour.queue
local idle = behaviour.idle
local nop = behaviour.nop
local teleport = behaviour.teleport
local previous = behaviour.previous

local rules = behaviour.rules {
	{{ADDRESS, "hello$", FORCE}, {"Welcome, %N! May Earth protect you, even whilst sailing!"}},
	{{ADDRESS, "hi$", FORCE}, {previous}},
	{{ADDRESS, "hiho$", FORCE}, {previous}},
	{{ADDRESS, FORCE}, {idle}},
	{{BUSY, "hello$", FORCE}, {"Shut up and wait like the rest, jawoll!", queue}},
	{{BUSY, "hi$", FORCE}, {previous}},
	{{BUSY, "hiho$", FORCE}, {previous}},
	{{BUSY, FORCE}, {nop}},
	{{VANISH, FORCE}, {"Yeah, yeah, walk, it's cheaper."}},

	{{"bye"}, {"Earth under your feet ... it's still better than lava.", idle}},
	{{"farewell"}, {previous}},
	{{"job"}, {"Look at my blackened beard? I'm the steamship captain!"}},
	{{"work"}, {previous}},
	{{"name"}, {"I am Brodrosch Steamtrousers, son of the machine, of the Molten Rock."}},
	{{"tibia"}, {"Tibia? Just don't ask."}},
	{{"ship"}, {"This is a great ship. Ha! It works without wind but with fire, and it travels not on the ocean but beneath the earth!"}},
	{{"steamship"}, {previous}},
	{{"captain"}, {"Of course, I am the captain. But I am also a technomancer."}},
	{{"technomancer"}, {"Being a technomancer is a privilege few dwarfs have. We form earth and fire through powerful technology into tools. Also, we are great inventors."}},
	{{"inventors"}, {"Yes. There could have been thousands of our inventions, if they wouldn't explode all the time..."}},
	{{"inventions"}, {previous}},
	{{"sell"}, {"This is not a shop, damn it!"}},
	{{"buy"}, {previous}},
	{{"thais"}, {"This is a steamship that travels only subterreneanly. No way to get on that risky ocean. Kazordoon - Cormaya only."}},
	{{"ab'dendriel"}, {previous}},
	{{"carlin"}, {previous}},
	{{"venore"}, {previous}},
	{{"senja"}, {previous}},
	{{"folda"}, {previous}},
	{{"vega"}, {previous}},
	{{"ice", "islands"}, {previous}},
	{{"darashia"}, {previous}},
	{{"darama"}, {previous}},
	{{"kazordoon"}, {"Hey, we ARE at Kazordoon! Must be the cavemadness..."}},
	{{"beer"}, {"Sometimes being drunk means seeing two rivers. I survive by steering right between them."}},
	{{"dwarf"}, {"Deep inside, we're all dwarfs."}},
	{{"gurbasch"}, {"Ah, my brother in Cormaya. He can take you back."}},

	{{"cormaya"}, {setPrice(160), "So you want to go to Cormaya? %P gold?", setTopic(1)}},
	{{"passage"}, {previous}},

	{{"cormaya", gt(questValue(250), 2)}, {setPrice(150), "So you want to go to Cormaya? %P gold?", setTopic(1)}},
	{{"passage", gt(questValue(250), 2)}, {previous}},

	{{eq(TOPIC, 1), "yes", PZBLOCK, FORCE}, {"First get rid of those blood stains! You are not going to ruin my vehicle!"}},

	-- für post-quest

	{{eq(TOPIC, 1), "yes", PREMIUM, eq(questValue(227), 4), ge(COUNTMONEY, PRICE)}, {"Full steam ahead!", deleteMoney, idle, effectOpp(3), teleport(33309, 31989, 15), effectOpp(3), setQuestValue(227, 5)}},

	{{eq(TOPIC, 1), "yes", PREMIUM, ge(COUNTMONEY, PRICE)}, {"Full steam ahead!", deleteMoney, idle, effectOpp(3), teleport(33309, 31989, 15), effectOpp(3)}},
	{{eq(TOPIC, 1), "yes", ge(COUNTMONEY, PRICE)}, {"I'm sorry, but you need a premium account in order to travel onboard our ships."}},
	{{eq(TOPIC, 1), "yes"}, {"You don't have enough money."}},
}

function onCreatureSay(npc, creature, text, situation)
	behaviour.react(npc, rules, text, situation)
end

function onCreatureDisappear(npc, creature)
	behaviour.react(npc, rules, "", NPC_SITUATION_VANISH)
end
