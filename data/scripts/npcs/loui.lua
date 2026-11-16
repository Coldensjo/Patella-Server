-- npcs/loui.lua: the behaviour of the NPC Loui, converted from
-- loui.npc by patella-convert. The NPC is its [[npc]] entry in npcs.toml;
-- lib/npc.lua matches and runs the rules below, in the order of the file
-- (scripting.hh, "NPCs"). The comments are those of loui.npc.
--
-- GIMUD - Graphical Interface Multi User Dungeon
-- Loui.npc: Datenbank für Loui den ängstlichen Mönch

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
	{{ADDRESS, "hello$", FORCE}, {"BEWARE! Beware of that hole!"}},
	{{ADDRESS, "hi$", FORCE}, {previous}},
	{{ADDRESS, FORCE}, {idle}},
	{{BUSY, "hello$", FORCE}, {"Please wait, you may listen to the frightening story I am telling!", queue}},
	{{BUSY, "hi$", FORCE}, {previous}},

	{{BUSY, FORCE}, {nop}},
	{{VANISH, FORCE}, {"STAY AWAY FROM THAT HOLE!"}},

	{{"bye"}, {"May the gods protect you! And stay away from that hole!", idle}},
	{{"farewell"}, {previous}},
	{{"job"}, {"I am a monk, collecting healing herbs."}},
	{{"name"}, {"My name is Loui."}},
	{{"monk"}, {"I am a humble servant of the gods."}},
	{{"tibia"}, {"Everything around us, that is Tibia."}},
	{{"rookgaard"}, {"This is the place where everything starts."}},
	{{"god"}, {"They created Tibia and all lifeforms. Talk to other monks and priests to learn more about them."}},
	{{"life"}, {"The gods blessed Tibia with abundant forms of life."}},
	{{"herb"}, {"I was looking for some herbs as I foolishly entered this unholy hole."}},
	{{"obi"}, {"He owns a shop in the town."}},
	{{"al", "dee"}, {"He owns a shop in the town."}},
	{{"seymour"}, {"Seymour is the headmaster of the local academy."}},
	{{"academy"}, {"Most adventurers take their first steps there."}},
	{{"willie"}, {"The gods may protect me from his foul language."}},
	{{"monster"}, {"There must be an army of them, just down this hole."}},
	{{"rabbit"}, {"So it must have been some magic wielding beasts using creature illusion. Good thing you escaped."}},

	{{"quest"}, {"I have no quests but to stay away from that hole and I'd recomend you to do the same."}},
	{{"task"}, {previous}},

	{{"gold"}, {"I am pennyless and poor as it is fit for a humble monk like me."}},
	{{"money"}, {previous}},
	{{"rat"}, {"The good thing is, those horrible rats stay in the town mostly. The bad thing is, they do so because outside the bigger Monsters devour them."}},
	{{"hole"}, {"While looking for herbs I found that hole. I went down though I had no torch. And then I heard THEM! There must be dozens!"}},
	{{"story"}, {previous}},
	{{"them"}, {"They were so many, EVERYWHERE! I could barely escape alive. I have no clue what THEY were but one more second down there and I'd be dead!"}},

	{{"heal"}, {"Sorry I am out of mana and ingredients, please visit Cipfried in the town."}},
	{{"time"}, {"Now, it is %T, my child."}},
}

function onCreatureSay(npc, creature, text, situation)
	behaviour.react(npc, rules, text, situation)
end

function onCreatureDisappear(npc, creature)
	behaviour.react(npc, rules, "", NPC_SITUATION_VANISH)
end
