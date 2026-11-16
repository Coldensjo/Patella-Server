-- npcs/ray.lua: the behaviour of the NPC Ray, converted from
-- ray.npc by patella-convert. The NPC is its [[npc]] entry in npcs.toml;
-- lib/npc.lua matches and runs the rules below, in the order of the file
-- (scripting.hh, "NPCs"). The comments are those of ray.npc.
--
-- GIMUD - Graphical Interface Multi User Dungeon
-- ray.npc: Datenbank für den Postbeamten Ray

local behaviour = require("lib.npc")
local genPost = require("lib.npcs.gen-post")
local ADDRESS = behaviour.ADDRESS
local BUSY = behaviour.BUSY
local VANISH = behaviour.VANISH
local FORCE = behaviour.FORCE
local queue = behaviour.queue
local idle = behaviour.idle
local nop = behaviour.nop
local previous = behaviour.previous

local rules = behaviour.rules {
	{{ADDRESS, "hello$", FORCE}, {"Greetings, %N."}},
	{{ADDRESS, "hi$", FORCE}, {previous}},
	{{ADDRESS, FORCE}, {idle}},
	{{BUSY, "hello$", FORCE}, {"Please wait a moment, %N.", queue}},
	{{BUSY, "hi$", FORCE}, {previous}},
	{{BUSY, FORCE}, {nop}},
	{{VANISH, FORCE}, {"I hope to see you back soon."}},

	{{"bye"}, {"I hope to see you back soon.", idle}},
	{{"farewell"}, {previous}},

	{{"kevin"}, {"Ah, yes we stay in touch via mail of course."}},
	{{"postner"}, {previous}},
	{{"postmasters", "guild"}, {"The guild is far away, but the mail keeps us close to each other somehow."}},
	{{"join"}, {"The guild is always looking for competent recruits. You can submit your application to our headquarters."}},
	{{"headquarters"}, {"It can be found easily. It's on the road from Thais to Kazordoon and Ab'Dendriel."}},

	{{"job"}, {"I am responsible for this post office. If you have questions about the mail system or the depots, just ask me."}},
	{{"name"}, {"My name is Ray."}},
	{{"ray"}, {"Yes, that's me."}},
	{{"time"}, {"Now it's %T."}},
	{{"king"}, {"The king lives far away in the lovely city of Thais, but even he can be reached by our mailing system."}},
	{{"tibianus"}, {previous}},
	{{"army"}, {"We got not the best men of the Thaian army to guard this colony. Given the number of difficulties this colony faces, this is quite a problem."}},
	{{"ferumbras"}, {"I hope this colony is too remote and meaningless to him to care for a visit."}},
	{{"excalibug"}, {"There are rumours about some hidden stone tablets mentioning that weapon. Adventurers claim to have seen those tablets in the ancient lizard city."}},
	{{"news"}, {"There are so many news that I cannot retell them all. Talk to the colonists and keep your ears open."}},
	{{"thais"}, {"All cities are covered by our mailing system."}},
	{{"carlin"}, {previous}},
	{{"apes"}, {"They are a pest. A quite dangerous pest as far as I can tell."}},
	{{"lizard"}, {"The lizards give me shivers. They are so alien, even more than the minotaurs or orcs we know from the surroundings of Thais."}},
	{{"dworcs"}, {"Those bloodthirsty headhunters live in the south. I heard only horrible stories about them and I believe they are not exaggerated."}},
	{{"jungle"}, {"The jungle is a dangerous place. Many got lost there and never returned."}},

	genPost,
}

function onCreatureSay(npc, creature, text, situation)
	behaviour.react(npc, rules, text, situation)
end

function onCreatureDisappear(npc, creature)
	behaviour.react(npc, rules, "", NPC_SITUATION_VANISH)
end
