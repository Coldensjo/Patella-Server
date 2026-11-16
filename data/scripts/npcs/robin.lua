-- npcs/robin.lua: the behaviour of the NPC Robin, converted from
-- robin.npc by patella-convert. The NPC is its [[npc]] entry in npcs.toml;
-- lib/npc.lua matches and runs the rules below, in the order of the file
-- (scripting.hh, "NPCs"). The comments are those of robin.npc.
--
-- GIMUD - Graphical Interface Multi User Dungeon
-- robin.npc: Datenbank für den Jäger Robin

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
	{{ADDRESS, "hello$", FORCE}, {"Ah, a visitor. Greetings %N!"}},
	{{ADDRESS, "hi$", FORCE}, {previous}},
	{{ADDRESS, FORCE}, {idle}},
	{{BUSY, "hello$", FORCE}, {"Learn some patience %N.", queue}},
	{{BUSY, "hi$", FORCE}, {previous}},
	{{BUSY, FORCE}, {nop}},
	{{VANISH, FORCE}, {"Farewell."}},

	{{"bye"}, {"Farewell.", idle}},
	{{"job"}, {"I am the Chief Huntsman of Thais."}},
	{{"name"}, {"I am Robin. Some call me Rob, others Woody."}},
	{{"time"}, {"Time... it's running that fast when you are as old as me."}},
	{{"king"}, {"I am his Master of the Hunt as I was his father's Master of the Hunt."}},
	{{"tibianus"}, {previous}},
	{{"quentin"}, {"My buddy Quentin is getting old, too. Things were different in our youth."}},
	{{"lynda"}, {"So young and so beautiful! She makes even an old man as me... uhm... feel a bit younger again."}},
	{{"harkath"}, {"Another one of a few friends of my youth who's still left."}},
	{{"army"}, {"These kids call themselves an army... In the old times we had a REAL army, I tell ya..."}},
	{{"general"}, {previous}},
	{{"ferumbras"}, {"A misguided follower of evil."}},
	{{"sam"}, {"I have not much use for heavy armor."}},
	{{"gorn"}, {"Sells a lot of useful stuff that guy. I rember the days when we were so poor that we could not afford anything the former owner offered."}},
	{{"frodo"}, {"Ah, I love that hut. I liked it as it was Iwan's hut, I loved it as it was Pridence's hut, and I think I will never stop to love this place."}},
	{{"elane"}, {"A master, or better mistress, of the bow. But with her big feet she just chases all game away."}},
	{{"muriel"}, {"These mages still give me shivers. I remember the first time this Ferumbras guy showed his ugly face here."}},
	{{"sorcerer"}, {previous}},
	{{"gregor"}, {"Can you imagine this youngster handles a guild? Ah, come on."}},
	{{"marvik"}, {"Druids have their ways with nature, but they would rather cuddle a bear than hunting it."}},
	{{"druid"}, {previous}},
	{{"bozo"}, {"Such guys don't live long. The grandfather of our king had a new jester every season."}},
	{{"baxter"}, {"I hardly know him."}},
	{{"oswald"}, {"Oh, what a charming young man. He's often here asking me about my youth and the people I met in my life."}},
	{{"sherry"}, {"The farmers are fine fellows."}},
	{{"donald"}, {previous}},
	{{"mcronald"}, {previous}},
	{{"crunor"}, {"Crunor gives and takes. That is his way. As long as we don't hunt more then we need we are at balance with Crunor."}},
	{{"lugri"}, {"Can you imagine his father was such a fine guy? A shame what his son has become."}},
	{{"excalibug"}, {"I don't like swords in general."}},
	{{"news"}, {"News? In the woods I learn nothing of importance to the world."}},
}

function onCreatureSay(npc, creature, text, situation)
	behaviour.react(npc, rules, text, situation)
end

function onCreatureDisappear(npc, creature)
	behaviour.react(npc, rules, "", NPC_SITUATION_VANISH)
end
