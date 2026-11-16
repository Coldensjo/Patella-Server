-- npcs/cipfried.lua: the behaviour of the NPC Cipfried, converted from
-- cipfried.npc by patella-convert. The NPC is its [[npc]] entry in npcs.toml;
-- lib/npc.lua matches and runs the rules below, in the order of the file
-- (scripting.hh, "NPCs"). The comments are those of cipfried.npc.
--
-- GIMUD - Graphical Interface Multi User Dungeon
-- cipfried.npc: Datenbank fuer den Moench Cipfried

local behaviour = require("lib.npc")
local ADDRESS = behaviour.ADDRESS
local BUSY = behaviour.BUSY
local VANISH = behaviour.VANISH
local FORCE = behaviour.FORCE
local lt = behaviour.lt
local gt = behaviour.gt
local HP = behaviour.HP
local BURNING = behaviour.BURNING
local POISON = behaviour.POISON
local setHp = behaviour.setHp
local burning = behaviour.burning
local poison = behaviour.poison
local effectOpp = behaviour.effectOpp
local queue = behaviour.queue
local idle = behaviour.idle
local nop = behaviour.nop
local previous = behaviour.previous

local rules = behaviour.rules {
	{{ADDRESS, "hello$", FORCE}, {"Hello, %N! Feel free to ask me for help."}},
	{{ADDRESS, "hi$", FORCE}, {previous}},
	{{ADDRESS, FORCE}, {idle}},
	{{BUSY, "hello$", FORCE}, {"Please wait, %N. I already talk to someone!", queue}},
	{{BUSY, "hi$", FORCE}, {previous}},
	{{BUSY, "heal$", gt(BURNING, 0)}, {"You are burning, %N. I will help you.", burning(0, 0), effectOpp(15)}},
	{{BUSY, "heal$", gt(POISON, 0)}, {"You are poisoned, %N. I will help you.", poison(0, 0), effectOpp(14)}},
	{{BUSY, "heal$", lt(HP, 65)}, {"You are looking really bad, %N. Let me heal your wounds.", setHp(65), effectOpp(13)}},
	{{BUSY, "heal$"}, {"You aren't looking really bad, %N. Sorry, I can't help you."}},
	{{BUSY, FORCE}, {nop}},
	{{VANISH, FORCE}, {"Well, bye then."}},

	{{"bye"}, {"Farewell, %N!", idle}},
	{{"farewell"}, {previous}},
	{{"job"}, {"I am just a humble monk. Ask me if you need help or healing."}},
	{{"name"}, {"My name is Cipfried."}},
	{{"monk"}, {"I sacrifice my life to serve the good gods of Tibia."}},
	{{"tibia"}, {"That's where we are. The world of Tibia."}},
	{{"rookgaard"}, {"The gods have chosen this isle as the point of arrival for the newborn souls."}},
	{{"god"}, {"They created Tibia and all life on it. Visit our library and learn about them."}},
	{{"life"}, {"The gods decorated Tibia with various forms of life. Plants, the citizens, and even the monsters."}},
	{{"plant"}, {"Just walk around. You will see grass, trees, and bushes."}},
	{{"citizen"}, {"Only few people live here. Walk around and talk to them."}},
	{{"obi"}, {"He is a local shop owner."}},
	{{"al", "dee"}, {previous}},
	{{"seymour"}, {"Seymour is a loyal follower of the king and responsibe for the academy."}},
	{{"academy"}, {"You should visit Seymour in the academy and ask him about a mission."}},
	{{"willie"}, {"He is a fine farmer. The farm is located to the left of the temple."}},
	{{"monster"}, {"They are a constant threat. Learn to fight by hunting rabbits, deers and sheeps. Then try to fight rats, bugs and perhaps spiders."}},
	{{"help"}, {"First you should try to get some gold and buy better equipment."}},
	{{"hint"}, {previous}},
	{{"quest"}, {previous}},
	{{"task"}, {previous}},
	{{"what", "do"}, {previous}},
	{{"gold"}, {"You have to slay monsters and take their gold. Or sell food at Willie's farm."}},
	{{"money"}, {"If you need money, you have to slay monsters and take their gold. Look for spiders and rats."}},
	{{"rat"}, {"In the north of this temple you find a sewer grate. Use it to enter the sewers if you feel prepared. Don't forget a torch; you'll need it."}},
	{{"sewer"}, {previous}},
	{{"equipment"}, {"First you need some armor and perhaps a better weapon or a shield. A real adventurer needs a rope, a shovel, and a fishing pole, too."}},
	{{"fight"}, {"Take a weapon in your hand, activate your combat mode, and select a target. After a fight you should eat something to heal your wounds."}},
	{{"slay"}, {previous}},
	{{"eat"}, {"If you want to heal your wounds you should eat something. Willie sells excellent meals. But if you are very weak, come to me and ask me to heal you."}},
	{{"food"}, {previous}},
	{{"heal$", gt(BURNING, 0)}, {"You are burning. I will help you.", burning(0, 0), effectOpp(15)}},
	{{"heal$", gt(POISON, 0)}, {"You are poisoned. I will help you.", poison(0, 0), effectOpp(14)}},
	{{"heal$", lt(HP, 65)}, {"You are looking really bad. Let me heal your wounds.", setHp(65), effectOpp(13)}},
	{{"heal$"}, {"You aren't looking really bad. Sorry, I can't help you."}},
	{{"time"}, {"Now, it is %T, my child."}},
}

function onCreatureSay(npc, creature, text, situation)
	behaviour.react(npc, rules, text, situation)
end

function onCreatureDisappear(npc, creature)
	behaviour.react(npc, rules, "", NPC_SITUATION_VANISH)
end
