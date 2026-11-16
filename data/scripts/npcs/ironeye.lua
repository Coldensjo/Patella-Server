-- npcs/ironeye.lua: the behaviour of the NPC Abran Ironeye, converted from
-- ironeye.npc by patella-convert. The NPC is its [[npc]] entry in npcs.toml;
-- lib/npc.lua matches and runs the rules below, in the order of the file
-- (scripting.hh, "NPCs"). The comments are those of ironeye.npc.
--
-- GIMUD - Graphical Interface Multi User Dungeon
-- ironeye.npc: Datenbank für den Waffen und Rüstungshandelsfürsten Abran Ironeye

local behaviour = require("lib.npc")
local ADDRESS = behaviour.ADDRESS
local BUSY = behaviour.BUSY
local VANISH = behaviour.VANISH
local FORCE = behaviour.FORCE
local idle = behaviour.idle
local nop = behaviour.nop
local previous = behaviour.previous

local rules = behaviour.rules {
	{{ADDRESS, "hello$", FORCE}, {"Hail, %N!"}},
	{{ADDRESS, "hi$", FORCE}, {previous}},
	{{ADDRESS, "hail$", FORCE}, {previous}},
	{{ADDRESS, FORCE}, {idle}},
	{{BUSY, "hello$", FORCE}, {"Stand still and wait!"}},
	{{BUSY, "hi$", FORCE}, {previous}},
	{{BUSY, "hail$", FORCE}, {previous}},
	{{BUSY, FORCE}, {nop}},
	{{VANISH, FORCE}, {"HEY! I did not dismiss you!"}},

	{{"bye"}, {"You are dismissed.", idle}},
	{{"name"}, {"I am Abran Ironeye."}},
	{{"job"}, {"I am a busy man. I run the Ironhouse."}},
	{{"ironhouse"}, {"What do you think? Here weapons and armor are forged, repaired, and sold."}},
	{{"time"}, {"You should know that on your own!"}},
	{{"king"}, {"I would like to see a true warrior-king in Thais... like in the old days. But who knows, perhaps one day the tides of fate will bring such a man to power. Who knows..."}},
	{{"tibianus"}, {previous}},
	{{"army"}, {"Such a great tool wasted for garrison duties, a shame."}},
	{{"ferumbras"}, {"Quite a challenge, but his bets for power were made without the finesse of a true warrior."}},
	{{"finesse"}, {"I won't give away my tricks, learn your own."}},
	{{"excalibug"}, {"If someone would bring me that weapon I could reshape the realm... and reward this hero beyond his dreams."}},
	{{"ironeye"}, {"I don't care if you like it or not. Stop staring at it!"}},
	{{"teddy"}, {"I don't know anything about a teddy... and if you are smart you shouldn't either..."}},
	{{"thais"}, {"Thais has outlived its usefulness since years. Its star is sinking."}},
	{{"tibia"}, {"The world is ready for a significant change."}},
	{{"warehouse"}, {"My Ironhouse is more than a warehouse."}},
	{{"carlin"}, {"Their independence is a proof for the weakness of Thais."}},
	{{"news"}, {"In Venore nothing comes for free and you could not afford my 'news'."}},
	{{"tax"}, {"The taxing keeps the Thaian kingdom alive, but it also might break its neck one day."}},
	{{"banor"}, {"As a man that grows up needs no mommy, a warrior has to outgrow his need for gods."}},
	{{"privilege"}, {"Venore's privileges are hard earned."}},
	{{"gambling"}, {"I trust only in skill, not luck."}},
}

function onCreatureSay(npc, creature, text, situation)
	behaviour.react(npc, rules, text, situation)
end

function onCreatureDisappear(npc, creature)
	behaviour.react(npc, rules, "", NPC_SITUATION_VANISH)
end
