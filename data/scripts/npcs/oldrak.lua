-- npcs/oldrak.lua: the behaviour of the NPC Oldrak, converted from
-- oldrak.npc by patella-convert. The NPC is its [[npc]] entry in npcs.toml;
-- lib/npc.lua matches and runs the rules below, in the order of the file
-- (scripting.hh, "NPCs"). The comments are those of oldrak.npc.
--
-- GIMUD - Graphical Interface Multi User Dungeon
-- oldrak.npc: Datenbank fuer den Moench Oldrak

local behaviour = require("lib.npc")
local ADDRESS = behaviour.ADDRESS
local BUSY = behaviour.BUSY
local VANISH = behaviour.VANISH
local FORCE = behaviour.FORCE
local lt = behaviour.lt
local gt = behaviour.gt
local questValue = behaviour.questValue
local setQuestValue = behaviour.setQuestValue
local queue = behaviour.queue
local idle = behaviour.idle
local nop = behaviour.nop
local previous = behaviour.previous

local rules = behaviour.rules {
	{{ADDRESS, "hello$", FORCE}, {"Welcome %N! Rarely I can welcome visitors in these days."}},
	{{ADDRESS, "hi$", FORCE}, {previous}},
	{{ADDRESS, FORCE}, {idle}},
	{{BUSY, "hello$", FORCE}, {"Please wait a minute, %N.", queue}},
	{{BUSY, "hi$", FORCE}, {previous}},
	{{BUSY, FORCE}, {nop}},
	{{VANISH, FORCE}, {"Take care, it's dangerous out there."}},

	{{"bye"}, {"Good bye, %N!", idle}},
	{{"farewell"}, {previous}},
	{{"job"}, {"I guard this humble temple as a monument for the order of the nightmare knights."}},
	{{"name"}, {"My name is Oldrak."}},
	{{"monster"}, {"These plains are not safe for ordinary travellers. It will take heroes to survive here."}},
	{{"help"}, {"I can't help you, sorry!"}},
	{{"goshnar"}, {"The greatest necromant who ever cursed our land with the steps of his feet. He was defeated by the nightmare knights."}},
	{{"nightmare", "knights"}, {"This ancient order was created by a circle of wise humans who were called 'the dreamers'. The order became extinct a long time ago."}},
	{{"extinct"}, {"Many perished in their battles against evil, some went mad, not able to stand their nightmares any longer. Others were seduced by the darkness."}},
	{{"dreamers"}, {"They learned the ancient art of dreamwalking from some elves they befriended."}},
	{{"dreamwalking"}, {"While the dreamwalkers of the elves experienenced the brightest dreams of pleasure, the humans strangely had dreams of dark omen."}},
	{{"dark", "omen"}, {"They dreamed of doom, destruction, talked to dead, tormented souls, and gained unwanted insight into the schemes of darkness."}},
	{{"schemes", "darkness"}, {"They figured out how to interpret their dark dreams and so could foresee the plans of the dark gods and their minions."}},
	{{"plan", "dark"}, {"Using this knowledge they formed an order to thwart these plans, and because they battled their nightmares as brave as knights, they named their order accordingly."}},
	{{"necromant", "nectar"}, {"It is rumoured to open the entrance to the pits of inferno, also called the nightmare pits. Even if I knew about this secret I wouldn't tell you."}},
	{{"plains", "havok"}, {"Before the battles raged across them, they were called the fair plains."}},
	{{"time"}, {"Now, it is %T."}},
	{{"tibia"}, {"That's where we are. The world of Tibia."}},
	{{"god"}, {"They created Tibia and all life on it ... and unlife, too."}},
	{{"unlife"}, {"Beware the foul undead!"}},
	{{"undead"}, {previous}},
	{{"excalibug"}, {"A weapon of myth and legend. It was lost in ancient times ... perhaps lost forever."}},
	{{"hugo"}, {"Ah, the bane of the Plains of Havoc, the hidden beast, the unbeatable foe. I live here for years and I am sure it's only a myth."}},
	{{"myth", lt(questValue(211), 1)}, {"There are many tales about the fearsome Hugo. It's said it is an abomination, accidently created by Yenny the Gentle. It's halve demon, halve something else and people say it's still alive after dozens of years.", setQuestValue(211, 1)}},
	{{"myth", gt(questValue(211), 0)}, {"There are many tales about the fearsome Hugo. It's said it is an abomination, accidently created by Yenny the Gentle. It's halve demon, halve something else and people say it's still alive after dozens of years."}},

	{{"yenny"}, {"Yenny, known as the Gentle, was one of most powerfull magicwielders in ancient times and known throughout the world for her mercy and kindness."}},
}

function onCreatureSay(npc, creature, text, situation)
	behaviour.react(npc, rules, text, situation)
end

function onCreatureDisappear(npc, creature)
	behaviour.react(npc, rules, "", NPC_SITUATION_VANISH)
end
