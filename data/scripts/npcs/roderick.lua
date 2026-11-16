-- npcs/roderick.lua: the behaviour of the NPC Roderick, converted from
-- roderick.npc by patella-convert. The NPC is its [[npc]] entry in npcs.toml;
-- lib/npc.lua matches and runs the rules below, in the order of the file
-- (scripting.hh, "NPCs"). The comments are those of roderick.npc.
--
-- GIMUD - Graphical Interface Multi User Dungeon
-- roderick.npc: Datenbank für den thaischen Botschafter Roderick of Thais (Elfenstadt)

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
	{{ADDRESS, "hello$", FORCE}, {"Salutations %N."}},
	{{ADDRESS, "hi$", FORCE}, {previous}},
	{{ADDRESS, "ashari$", FORCE}, {previous}},
	{{ADDRESS, FORCE}, {idle}},
	{{BUSY, "hello$", FORCE}, {"Wait %N!", queue}},
	{{BUSY, "hi$", FORCE}, {previous}},
	{{BUSY, "ashari$", FORCE}, {previous}},
	{{BUSY, FORCE}, {nop}},
	{{VANISH, FORCE}, {"Farewell."}},

	{{"bye"}, {"Farewell.", idle}},
	{{"farewell"}, {previous}},
	{{"job"}, {"I am ambassador of our beloved king, Tibianus III."}},
	{{"name"}, {"I am Roderick of Thais, commoner."}},
	{{"time"}, {"Ask someone else."}},
	{{"elves"}, {"Though there are differences, I am sure we can live in peace and harmony with that noble race."}},
	{{"dwarfs"}, {"The dwarfs are verry dilligent and crafty people. Our contracts with kazordoon asure the best for both of our races."}},
	{{"kazordoon"}, {previous}},
	{{"humans"}, {"Though there are differences to other races, I am sure we can live in peace and harmony with them."}},
	{{"troll"}, {"I heared about them working in the local mines. I am not sure if i like the concept of having such creatures within the walls of a city."}},
	{{"carlin"}, {"We are watching their relations with Ab'Denriel closely."}},
	{{"venore"}, {"The tradesmen of venore show great interest in trade cotracts with the elves."}},
	{{"cenath"}, {"I look forward to improve our relations with them."}},
	{{"kuridai"}, {previous}},
	{{"deraisim"}, {previous}},
	{{"abdaisim"}, {"Unfortunately I've had no contact with them yet."}},
	{{"teshial"}, {"They hardly seem more then an elven myth."}},
	{{"ferumbras"}, {"A threat to all free races."}},
	{{"crunor"}, {"I am not familiar enough with the different faithes to discuss them properly."}},
	{{"excalibug"}, {"A nice myth but nothing more."}},
	{{"news"}, {"We don't hear much at this place."}},
	{{"magic"}, {"I am impressed by the magic the elves are able to wield. Many of them can cast and even teach spells."}},
	{{"druid"}, {"The elven magic is somewhat similar to that of the druids."}},
	{{"sorcerer"}, {"Perhaps Thaian sorcerers can teach the elves their magic in exchange for knowledge of that noble race."}},
	{{"tibianus"}, {"Our beloved ruler seeks friendship and peace with the elves of Ab'Dendriel."}},
	{{"olrik"}, {"He is my servant and responsible for the mail. I wish he would not spent so much time with elven ladies and work harder."}},
}

function onCreatureSay(npc, creature, text, situation)
	behaviour.react(npc, rules, text, situation)
end

function onCreatureDisappear(npc, creature)
	behaviour.react(npc, rules, "", NPC_SITUATION_VANISH)
end
