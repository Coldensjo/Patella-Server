-- npcs/bigben.lua: the behaviour of the NPC A Sweaty Cyclops, converted from
-- bigben.npc by patella-convert. The NPC is its [[npc]] entry in npcs.toml;
-- lib/npc.lua matches and runs the rules below, in the order of the file
-- (scripting.hh, "NPCs"). The comments are those of bigben.npc.
--
-- GIMUD - Graphical Interface Multi User Dungeon
-- bigben.npc: Datenbank für den Zyklopenschmied Ben (Elfenstadt)

local behaviour = require("lib.npc")
local ADDRESS = behaviour.ADDRESS
local BUSY = behaviour.BUSY
local VANISH = behaviour.VANISH
local FORCE = behaviour.FORCE
local eq = behaviour.eq
local TOPIC = behaviour.TOPIC
local setTopic = behaviour.setTopic
local queue = behaviour.queue
local idle = behaviour.idle
local nop = behaviour.nop
local previous = behaviour.previous

local rules = behaviour.rules {
	{{ADDRESS, "hello$", FORCE}, {"Hum Humm! Welcume lil' %N."}},
	{{ADDRESS, "hi$", FORCE}, {previous}},
	{{ADDRESS, FORCE}, {idle}},
	{{BUSY, "hello$", FORCE}, {"%N waits. Me talks.", queue}},
	{{BUSY, "hi$", FORCE}, {previous}},
	{{BUSY, FORCE}, {nop}},
	{{VANISH, FORCE}, {"Hum Humm."}},

	{{"bye"}, {"Good bye lil' one.", idle}},
	{{"farewell"}, {previous}},
	{{"job"}, {"I am smith."}},
	{{"name"}, {"I called Bencthyclthrtrprr by me people. Lil' ones me call Big Ben."}},

	{{"tibia"}, {"One day I'll go and look."}},
	{{"ab'dendriel"}, {"Me parents live here before town was. Me not care about lil' ones."}},
	{{"big", "old"}, {"Mountain in south. Lil' lil' ones living there."}},
	{{"elves"}, {"Me not fight them, they not fight me."}},
	{{"humans"}, {"Always asking me for stuff they can't afford."}},
	{{"orcs"}, {"Silly ones. Not talk much. Always screaming and hitting."}},
	{{"minotaurs"}, {"They were friend with me parents. Long before elves here, they often made visit. No longer come here."}},
	{{"dwarfs"}, {"Lil' lil' ones are so fun. We often chat."}},
	{{"lil", "lil"}, {previous}},
	{{"god"}, {"You shut up. Me not want to hear."}},
	{{"smith"}, {"Working steel is my profession."}},
	{{"steel"}, {"Manny kinds of. Like Mesh Kaha Rogh, Za'Kalortith, Uth'Byth, Uth'Morc, Uth'Amon, Uth'Maer, Uth'Doon, and Zatragil"}},

	{{"Mesh", "Kaha", "Rogh"}, {"Steel that is singing when forged. No one knows where find today."}},
	{{"Za'Kalortith"}, {"It's evil. Demon iron is. No good cyclops goes where you can find and need evil flame to melt."}},
	{{"Uth'Byth"}, {"Not good to make stuff off. Bad steel it is. But eating magic, so useful is."}},
	{{"Uth'Morc"}, {"Lil' ones it thieves' steel call sometimes. It's dark and making not much noise."}},
	{{"Uth'Amon"}, {"Brigthsteel is. Much art made with it. Sorcerers to lazy and afraid to enchant much."}},
	{{"Uth'Maer"}, {"Heartiron from heart of big old mountain, found very deep. Lil' lil ones fiercely defend. Not wanting to have it used for stuff but holy stuff."}},
	{{"Uth'Doon"}, {"It's high steel called. Only lil' lil' ones know how make."}},
	{{"Zatragil"}, {"Most ancients use dream silver for different stuff. Now ancients most gone. Most not know about."}},

	{{"Teshial"}, {"Is one of elven family or such thing. Me not understand lil' ones and their busisness."}},
	{{"Deraisim"}, {previous}},
	{{"Cenath"}, {previous}},
	{{"Kuridai"}, {previous}},

	{{"cyclops"}, {"Me people not live here much. Most are far away."}},
	{{"excalibug"}, {"Me wish I could make weapon like it."}},

	{{"fire", "sword"}, {"Do lil' one want to trade a fire sword?", setTopic(1)}},
	{{"bright", "word"}, {"Do lil' one want to trade a bright sword?", setTopic(1)}},
	{{"warlord", "sword"}, {"Do lil' one want to trade a warlord sword?", setTopic(1)}},
	{{"sword", "of", "valor"}, {"Do lil' one want to trade a sword of valor?", setTopic(1)}},
	{{"serpent", "sword"}, {"Do lil' one want to trade a serpent sword?", setTopic(1)}},
	{{"enchanted", "plate"}, {"Do lil' one want to trade an enchanted plate armor?", setTopic(1)}},
	{{"dragon", "shield"}, {"Do lil' one want to trade a dragon shield?", setTopic(1)}},

	{{eq(TOPIC, 1), "yes"}, {"You not have stuff me want for."}},
	{{eq(TOPIC, 1)}, {"Silly lil' one you are."}},
}

function onCreatureSay(npc, creature, text, situation)
	behaviour.react(npc, rules, text, situation)
end

function onCreatureDisappear(npc, creature)
	behaviour.react(npc, rules, "", NPC_SITUATION_VANISH)
end
