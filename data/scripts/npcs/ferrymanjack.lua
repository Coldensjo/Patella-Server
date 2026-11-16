-- npcs/ferrymanjack.lua: the behaviour of the NPC Captain Jack, converted from
-- ferrymanjack.npc by patella-convert. The NPC is its [[npc]] entry in npcs.toml;
-- lib/npc.lua matches and runs the rules below, in the order of the file
-- (scripting.hh, "NPCs"). The comments are those of ferrymanjack.npc.
--
-- GIMUD - Graphical Interface Multi User Dungeon
-- ferrymanJack.npc: Fährmann Jack auf der Mönchsinsel

local behaviour = require("lib.npc")
local ADDRESS = behaviour.ADDRESS
local BUSY = behaviour.BUSY
local VANISH = behaviour.VANISH
local PZBLOCK = behaviour.PZBLOCK
local FORCE = behaviour.FORCE
local eq = behaviour.eq
local lt = behaviour.lt
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
	{{ADDRESS, "hello$", gt(questValue(220), 0), FORCE}, {"By the gods! You must be that intruder the good brothers were talking about! Begone!", idle}},
	{{BUSY, "hello$", gt(questValue(220), 0), FORCE}, {previous}},
	{{ADDRESS, "hi$", gt(questValue(220), 0), FORCE}, {previous}},
	{{BUSY, "hi$", gt(questValue(220), 0), FORCE}, {previous}},

	{{ADDRESS, "hello$", FORCE}, {"Ahoy, matey. You are lucky to catch me here. I was preparing to set sail."}},
	{{ADDRESS, "hi$", FORCE}, {previous}},
	{{ADDRESS, FORCE}, {idle}},
	{{BUSY, "hello$", FORCE}, {"One moment please, %N.", queue}},
	{{BUSY, "hi$", FORCE}, {previous}},
	{{BUSY, FORCE}, {nop}},
	{{VANISH, FORCE}, {"Good bye. You are welcome."}},

	{{"bye"}, {"Good bye. You are welcome.", idle}},
	{{"farewell"}, {previous}},
	{{"name"}, {"I'm m ol' Captain Jack."}},
	{{"job"}, {"I work as a kind of ferryman. I transport wares and travellers for the monks."}},
	{{"sail"}, {previous}},
	{{"captain"}, {previous}},
	{{"ship"}, {"All right she's small. But she's a real beauty, don't you think?"}},
	{{"ferry"}, {previous}},
	{{"wares"}, {"They always need provisions from the cities, and they sell their wine there."}},
	{{"traveller"}, {"Sometimes pilgrims come to this place. And now and then a monk leaves the monastery for some time."}},
	{{"trip"}, {"Where do you want to go today?"}},
	{{"passage"}, {"If you have the abbot's permission, I can take you to the home of a local fisherman on the continent Tibia. We can't sail to Carlin though."}},
	{{"carlin"}, {"For some obscure political reason the monks never sail to Carlin or Thais directly."}},
	{{"island"}, {"This is the isle of the kings. All the great Tibian leaders have found their final rest here under the monastery."}},
	{{"monastery"}, {"The white raven monastery is a place of wisdom and contemplation, or so the monks say. Sounds like a pretty boring place to me! HAR HAR!"}},
	{{"monk"}, {"The order of the white raven."}},
	{{"white", "raven"}, {"I prefer parrots. And monkeys! And snakes! HAR! HAR!"}},
	{{"continent", eq(questValue(62), 2)}, {setPrice(20), "Friends of Dalbrect are my friends too! So you are looking for a passage to the continent for %P gold?", setTopic(1)}},
	{{"tibia", eq(questValue(62), 2)}, {previous}},
	{{"continent", eq(questValue(62), 1)}, {setPrice(20), "Do you want a passage to the continent for %P gold?", setTopic(1)}},
	{{"continent", lt(questValue(62), 1)}, {"Without the abbots permission I won't take sail you anywhere! Go and ask him for a passage first."}},
	{{"tibia", eq(questValue(62), 1)}, {setPrice(20), "Tibia is the main continent. Do you want a passage to the continent for %P gold?", setTopic(1)}},
	{{"tibia", lt(questValue(62), 1)}, {"Without the abbots permission I won't take you anywhere! Go and ask him for a passage first."}},

	{{eq(TOPIC, 1), "yes", PZBLOCK, FORCE}, {"First get rid of those blood stains! You are not going to ruin my vehicle!"}},

	{{eq(TOPIC, 1), "yes", ge(COUNTMONEY, PRICE)}, {"Have a nice trip!", deleteMoney, idle, effectOpp(11), teleport(32205, 31756, 6), effectOpp(11)}},
	{{eq(TOPIC, 1), "yes"}, {"You don't have enough money."}},
	{{eq(TOPIC, 1)}, {"Well, I'll be here if you change your mind."}},
}

function onCreatureSay(npc, creature, text, situation)
	behaviour.react(npc, rules, text, situation)
end

function onCreatureDisappear(npc, creature)
	behaviour.react(npc, rules, "", NPC_SITUATION_VANISH)
end
