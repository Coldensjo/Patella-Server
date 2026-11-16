-- lib/npcs/gentest.lua: the rule block gentest.ndb, converted by patella-convert.
-- An NPC that included it (@"gentest.ndb") has it in its rules where the include
-- was; lib/npc.lua matches and runs them (scripting.hh, "NPCs"). The comments
-- are those of gentest.ndb.
--
-- GIMUD - Graphical Interface Multi User Dungeon
-- gentest.ndb: Datenbank für den test generischer händler

local behaviour = require("lib.npc")

return behaviour.rules {
	{{"news"}, {"I am busy. Please ask the citizens for news."}},
	{{"sell"}, {"Visit shopkeepers to buy their fine wares."}},
	{{"job"}, {"I am a trader."}},
}
