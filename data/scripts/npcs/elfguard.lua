-- npcs/elfguard.lua: the behaviour of the NPC Elf Guard, converted from
-- elfguard.npc by patella-convert. The NPC is its [[npc]] entry in npcs.toml;
-- lib/npc.lua matches and runs the rules below, in the order of the file
-- (scripting.hh, "NPCs"). The comments are those of elfguard.npc.
--
-- GIMUD - Graphical Interface Multi User Dungeon
-- elfguard.npc: Datenbank für die Elfenwache in Ab'Dendriel

local behaviour = require("lib.npc")
local ADDRESS = behaviour.ADDRESS
local BUSY = behaviour.BUSY
local VANISH = behaviour.VANISH
local FORCE = behaviour.FORCE
local queue = behaviour.queue
local idle = behaviour.idle
local nop = behaviour.nop
local previous = behaviour.previous

local rules = behaviour.rules {
	{{ADDRESS, "hello$", FORCE}, {"Ashari %N."}},
	{{ADDRESS, "hi$", FORCE}, {previous}},
	{{ADDRESS, "Ashari$", FORCE}, {previous}},
	{{ADDRESS, FORCE}, {idle}},
	{{BUSY, "hello$", FORCE}, {"Wait %N.", queue}},
	{{BUSY, "hi$", FORCE}, {previous}},
	{{BUSY, FORCE}, {nop}},
	{{VANISH, FORCE}, {"Asha Thrazi."}},

	{{"bye"}, {"Asha Thrazi.", idle}},
	{{"farewell"}, {previous}},
	{{"name"}, {"My name is unimportant, only my duty does matter."}},
	{{"job"}, {"I am a guardian of this town. I have no time to chat!"}},
	{{"time"}, {"It's %T."}},
	{{"town"}, {"This is the elven town of Ab'Dendriel."}},
	{{"city"}, {previous}},
	{{"ab'dendriel"}, {previous}},
	{{"thais"}, {"The city of the humans lies somewhere far to the south beyond the mountains of the dwarfs."}},
	{{"carlin"}, {"This city of humankind is located to the west of our area."}},
	{{"kazordoon"}, {"The dwarfish settlement is hidden somewhere in the mountains in the south."}},
	{{"elf"}, {"The elves of this city are the casts of the Cenath, the Kuridai, and the Deraisim."}},
	{{"elves"}, {previous}},
	{{"cenath"}, {"The Cenath are magic users. Look for them on the upper levels of the town."}},
	{{"kuridai"}, {"The Kuridai are the smiths and craftsmen. Look for them in the underground parts of the city."}},
	{{"deraisim"}, {"The Deraisim are scouts and hunters. You may find them on the groundlevel of the city."}},
	{{"abdaisim"}, {"The Abdaisim are wanderers. Since they live as nomads and travel the world you won't find them here."}},
	{{"teshial"}, {"There are no Teshial."}},
	{{"ferumbras"}, {"He is not allowed to enter this city."}},
	{{"army"}, {"Such a thing is a human concept. We have no need for that, though some Kuridai might think otherwise."}},
	{{"spell"}, {"Ask around in Ab'Dendriel. Many elves can teach you something about magic. The Cenath love magic most of all."}},
	{{"magic"}, {previous}},
	{{"armor"}, {"If you are looking for that kind of equipment you should ask a Kuridai."}},
	{{"weapon"}, {previous}},
	{{"food"}, {"Ask some Deraisim where you can get food."}},

	{{"tha'shi", "ab'dendriel"}, {"In the crude human language you would translate it with 'My life for Ab'Dendriel' or even 'I am one with Ab'Dendriel'."}},
	{{"bahaha", "aka"}, {"This means 'Take your punishment, defiler'."}},
}

function onCreatureSay(npc, creature, text, situation)
	behaviour.react(npc, rules, text, situation)
end

function onCreatureDisappear(npc, creature)
	behaviour.react(npc, rules, "", NPC_SITUATION_VANISH)
end
