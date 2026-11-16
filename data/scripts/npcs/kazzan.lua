-- npcs/kazzan.lua: the behaviour of the NPC Kazzan, converted from
-- kazzan.npc by patella-convert. The NPC is its [[npc]] entry in npcs.toml;
-- lib/npc.lua matches and runs the rules below, in the order of the file
-- (scripting.hh, "NPCs"). The comments are those of kazzan.npc.
--
-- GIMUD - Graphical Interface Multi User Dungeon
-- kazzan.npc: Datenbank für Kalif Kazzan

local behaviour = require("lib.npc")
local ADDRESS = behaviour.ADDRESS
local BUSY = behaviour.BUSY
local VANISH = behaviour.VANISH
local FORCE = behaviour.FORCE
local burning = behaviour.burning
local effectMe = behaviour.effectMe
local effectOpp = behaviour.effectOpp
local queue = behaviour.queue
local idle = behaviour.idle
local nop = behaviour.nop
local previous = behaviour.previous

local rules = behaviour.rules {
	{{ADDRESS, "hello$", FORCE}, {"Feel welcome in the lands of the children of the enlightened Daraman, %N."}},
	{{ADDRESS, "hi$", FORCE}, {previous}},
	{{ADDRESS, "hail$", FORCE}, {previous}},
	{{ADDRESS, "salutations$", FORCE}, {previous}},
	{{ADDRESS, FORCE}, {idle}},
	{{BUSY, "hello$", FORCE}, {"I will test your patience %N, since I am already talking!", queue}},
	{{BUSY, "hi$", FORCE}, {previous}},
	{{BUSY, "hail$", FORCE}, {previous}},
	{{BUSY, "salutations$", FORCE}, {previous}},
	{{BUSY, FORCE}, {nop}},
	{{VANISH, FORCE}, {"May your soul flourish"}},

	{{"bye"}, {"May your soul flourish.", idle}},
	{{"news"}, {"I don't give much attention to rumours."}},
	{{"king"}, {"Tibianus is the shepherd of the lost souls of the so called Thaian empire."}},
	{{"tibianus"}, {previous}},
	{{"thais"}, {previous}},
	{{"name"}, {"I am Kazzan Ibn Gadral, caliph of Darama."}},
	{{"job"}, {"I am the caliph of the children of Daraman."}},
	{{"caliph"}, {"A caliph is a leader of his people, just like a king."}},
	{{"how", "are", "you"}, {"I am fine, thank you."}},
	{{"army"}, {"Our people are well prepared to fight for their land and their souls."}},
	{{"guard"}, {previous}},
	{{"enemies"}, {"The necromancers of Drefia fell under the wrath of the djinns once. If they challenge us again they might lose more than a city."}},
	{{"enemy"}, {previous}},
	{{"drefia"}, {"When the djinns destroyed the better part of the unholy town in their wrath, the brotherhood hid like worms in the sand."}},
	{{"brotherhood"}, {"The Brotherhood of Bones came here fleeing some war on the continent. They corrupted the settlers of the Thaian colony with ease."}},
	{{"minotaur"}, {"The minotaurs are another test we have to endure. They inhabit the pyramid which is taboo for our people, as Daraman taught us."}},
	{{"daraman"}, {"Daraman led our ancestors to the continent Darashia to live a life of simplicity and meditation."}},
	{{"taboo"}, {"Daraman knew our souls might get corupted by the things hidden there."}},
	{{"quest"}, {"I will not entrust foreigners with any quest. Live amongst us for some years and listen to Daraman's teachings and we will see."}},
	{{"mission"}, {previous}},
	{{"god"}, {"The gods are powerful but it's ultimately up to us to work on our souls' ascension."}},
	{{"excalibug"}, {"Once a djinn claimed to have seen it in a dream. I guess it's just that, some dream of a supernatural creature."}},
	{{"kazordoon"}, {"We have lost contact with the dwarf people of Kazordoon."}},
	{{"dwarf"}, {previous}},
	{{"carlin"}, {"We keep in touch with the queen but did not take sides in the conflict of Carlin and Thais ... yet."}},

	{{"fuck"}, {"Take this!", burning(10, 1), effectOpp(5), effectMe(8)}},
	{{"idiot"}, {"Take this!", burning(10, 1), effectOpp(5), effectMe(8)}},
	{{"asshole"}, {"Take this!", burning(10, 1), effectOpp(5), effectMe(8)}},
	{{"ass$"}, {"Take this!", burning(10, 1), effectOpp(5), effectMe(8)}},
	{{"fag$"}, {"Take this!", burning(10, 1), effectOpp(5), effectMe(8)}},
	{{"stupid"}, {"Take this!", burning(10, 1), effectOpp(5), effectMe(8)}},
	{{"tyrant"}, {"Take this!", burning(10, 1), effectOpp(5), effectMe(8)}},
	{{"shit"}, {"Take this!", burning(10, 1), effectOpp(5), effectMe(8)}},
	{{"lunatic"}, {"Take this!", burning(10, 1), effectOpp(5), effectMe(8)}},
}

function onCreatureSay(npc, creature, text, situation)
	behaviour.react(npc, rules, text, situation)
end

function onCreatureDisappear(npc, creature)
	behaviour.react(npc, rules, "", NPC_SITUATION_VANISH)
end
