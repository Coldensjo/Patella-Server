-- npcs/vladruc.lua: the behaviour of the NPC Vladruc, converted from
-- vladruc.npc by patella-convert. The NPC is its [[npc]] entry in npcs.toml;
-- lib/npc.lua matches and runs the rules below, in the order of the file
-- (scripting.hh, "NPCs"). The comments are those of vladruc.npc.
--
-- GIMUD - Graphical Interface Multi User Dungeon
-- Vladruc.npc: Datenbank für Vladruc Urghain, den Besitzer des magischen Markts

local behaviour = require("lib.npc")
local ADDRESS = behaviour.ADDRESS
local BUSY = behaviour.BUSY
local VANISH = behaviour.VANISH
local FORCE = behaviour.FORCE
local gt = behaviour.gt
local count = behaviour.count
local idle = behaviour.idle
local nop = behaviour.nop
local previous = behaviour.previous

local rules = behaviour.rules {
	{{ADDRESS, "hello$", gt(count(3083), 0), FORCE}, {"Chhhh ... Sorry, I'm busy. <cough>", idle}},
	{{ADDRESS, "hi$", gt(count(3083), 0), FORCE}, {previous}},
	{{ADDRESS, "hello$", FORCE}, {"I am Vladruc Urghain and welcome you, %N, to my house."}},
	{{ADDRESS, "hi$", FORCE}, {previous}},
	{{ADDRESS, FORCE}, {idle}},
	{{BUSY, "hello$", gt(count(3083), 0), FORCE}, {"Sorry, I'm busy."}},
	{{BUSY, "hi$", gt(count(3083), 0), FORCE}, {previous}},
	{{BUSY, "hello$", FORCE}, {"Please be seated and wait, %N."}},
	{{BUSY, "hi$", FORCE}, {previous}},
	{{BUSY, FORCE}, {nop}},
	{{VANISH, FORCE}, {"Go safely, and leave something of the happiness you bring!"}},

	{{"bye"}, {"Go safely, and leave something of the happiness you bring!", idle}},
	{{"farewell"}, {previous}},
	{{"name"}, {"I am Vladruc Urghain. Welcome to my house!"}},
	{{"job"}, {"I am a humble merchant of little importance to the beautiful Venore."}},
	{{"thais"}, {"Sadly only through books I have come to know your great Thais, and to know her is to love her."}},
	{{"books"}, {"These companions have been good friends and teachers to me."}},
	{{"adventure"}, {"The time I sought out adventure is long gone indeed."}},
	{{"Tibia"}, {"What a wonderful world we do live in ... so full of life."}},
	{{"shop"}, {"Ah, feel free to browse and buy in my humble shop below."}},
	{{"market"}, {previous}},
	{{"time"}, {"It is %T."}},
	{{"king"}, {"I am of noble blood myself. I have been so long master that none other should be master of me."}},
	{{"tibianus"}, {previous}},
	{{"venore"}, {"Our ways are not your ways, and there shall be to you many strange things."}},
	{{"army"}, {"The Thaian garrsion serves its purpose very well."}},
	{{"ferumbras"}, {"You think he is of ancient evil? Little you know about ancientness or evilness."}},
	{{"excalibug"}, {"A terrifying weapon if it does exist at all."}},
	{{"news"}, {"I am a reclusive person and learn little of the local gossip of the peasants."}},
	{{"help"}, {"I am sorry, but I can't be of much assistance to you."}},
	{{"monster"}, {"Oh yes, the children of the night ... you dwellers in the city cannot enter into the feelings of the hunter."}},
	{{"dungeon"}, {"Such lovely places, unjustly shunned by the people."}},
	{{"vampire"}, {"Please don't talk about such creatures. You are scaring me."}},
	{{"thanks"}, {"That's nothing worth to be mentioned."}},
	{{"thank", "you"}, {previous}},

	{{"offer"}, {"Please check my humble market downstairs for the wares that are offered."}},
	{{"magic"}, {"Magic is a tool to be mastered."}},
	{{"spells"}, {"I know a spell or two. You might want to buy some spells downstairs in the market."}},
	{{"alchemy"}, {"You can buy some potions downstairs."}},
	{{"blood"}, {"I like blood ... only the color, that is, of course ... <chuckles>"}},
	{{"undead"}, {"It is not dead, which can eternal lie, and in strange aeons, even death may die."}},
	{{"necroman"}, {"Death is the final frontier. Necromancers boldly go, where no one has gone before."}},
	{{"coffin"}, {"The final restingplace for all of us, isn't it?"}},
}

function onCreatureSay(npc, creature, text, situation)
	behaviour.react(npc, rules, text, situation)
end

function onCreatureDisappear(npc, creature)
	behaviour.react(npc, rules, "", NPC_SITUATION_VANISH)
end
