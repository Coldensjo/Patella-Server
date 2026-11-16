-- npcs/imbul.lua: the behaviour of the NPC Imbul, converted from
-- imbul.npc by patella-convert. The NPC is its [[npc]] entry in npcs.toml;
-- lib/npc.lua matches and runs the rules below, in the order of the file
-- (scripting.hh, "NPCs"). The comments are those of imbul.npc.
--
-- GIMUD - Graphical Interface Multi User Dungeon
-- Imbul.npc: Datenbank für den Fährman Imbul

local behaviour = require("lib.npc")
local ADDRESS = behaviour.ADDRESS
local BUSY = behaviour.BUSY
local VANISH = behaviour.VANISH
local PREMIUM = behaviour.PREMIUM
local PZBLOCK = behaviour.PZBLOCK
local FORCE = behaviour.FORCE
local eq = behaviour.eq
local lt = behaviour.lt
local gt = behaviour.gt
local ge = behaviour.ge
local TOPIC = behaviour.TOPIC
local PRICE = behaviour.PRICE
local COUNTMONEY = behaviour.COUNTMONEY
local setTopic = behaviour.setTopic
local setPrice = behaviour.setPrice
local effectOpp = behaviour.effectOpp
local deleteMoney = behaviour.deleteMoney
local queue = behaviour.queue
local idle = behaviour.idle
local nop = behaviour.nop
local teleport = behaviour.teleport
local previous = behaviour.previous

local rules = behaviour.rules {
	{{ADDRESS, "hello$", FORCE}, {"Hi, %N."}},
	{{ADDRESS, "hi$", FORCE}, {previous}},
	{{ADDRESS, "hiho$", FORCE}, {previous}},
	{{ADDRESS, FORCE}, {idle}},

	{{BUSY, "hello$", FORCE}, {"Please wait.", queue}},
	{{BUSY, "hi$", FORCE}, {previous}},
	{{BUSY, "hiho$", FORCE}, {previous}},
	{{BUSY, FORCE}, {nop}},
	{{VANISH, FORCE}, {"Bye."}},

	{{"bye"}, {"Bye.", idle}},
	{{"farewell"}, {previous}},
	{{"job"}, {"I'm a ferryman. If you want me to transport you to the other end of the city, feel free to ask me for a passage."}},
	{{"name"}, {"My name is Imbul."}},
	{{"time"}, {"Sorry, I don't own a watch."}},
	{{"king"}, {"It must be fun to be a king."}},
	{{"venore"}, {"Here are many people from Venore. I used to live there but I lost my job and took the chance to come here."}},
	{{"thais"}, {"It's where the king lives. The roads there must be made of gold or marble at least."}},
	{{"carlin"}, {"It's a city, I know that."}},
	{{"edron"}, {"Edron is some isle where the richest knights and sorcerers live. There they are not annoyed of the constant begging of poor people."}},
	{{"jungle"}, {"We already lost many settlers to the jungle. No one knows who is next."}},

	{{"tibia"}, {"That is our world, yes."}},

	{{"kazordoon"}, {"I think those dwarves came from Kazordoon."}},
	{{"dwarves"}, {"The dwarves that live here are searching for gold."}},
	{{"dwarfs"}, {previous}},
	{{"ab'dendriel"}, {"What is that?"}},
	{{"elves"}, {"The elven queen is the most beautiful woman in Tibia."}},
	{{"elfs"}, {previous}},
	{{"darama"}, {"Many came here to make their fortune. But it might take a while to become rich."}},
	{{"darashia"}, {"That's somewhere behind that mountain."}},
	{{"ankrahmun"}, {"I heard it's a ghost town or something like that."}},
	{{"ferumbras"}, {"Why have some magicians to become evil?"}},
	{{"excalibug"}, {"I never heard about that."}},
	{{"apes"}, {"They stole my paddle once."}},
	{{"lizard"}, {"If you follow the river far enough upcountry, you might see a lizardman. But be careful, they'll attack you as soon as they catch sight of you."}},
	{{"dworcs"}, {"They are all murderers and cannibals."}},

	{{"trip"}, {"I can bring you either to the east end of Port Hope or to the centre of the town, where would you like to go?"}},
	{{"route"}, {previous}},
	{{"passage"}, {previous}},
	{{"destination"}, {previous}},
	{{"sail"}, {previous}},
	{{"go"}, {previous}},

	{{"east"}, {setPrice(7), "Do you seek a passage to the east end of Port Hope for %P gold?", setTopic(1)}},
	{{"cent"}, {setPrice(7), "Do you seek a passage to the centre of Port Hope for %P gold?", setTopic(2)}},

	{{eq(TOPIC, 1), "yes", PZBLOCK, FORCE}, {"First get rid of those blood stains! You are not going to ruin my vehicle!"}},
	{{eq(TOPIC, 2), "yes", PZBLOCK, FORCE}, {"First get rid of those blood stains! You are not going to ruin my vehicle!"}},

	{{eq(TOPIC, 1), "yes", PREMIUM, ge(COUNTMONEY, PRICE)}, {"Here we go!", deleteMoney, idle, effectOpp(11), teleport(32679, 32777, 7), effectOpp(11)}},
	{{eq(TOPIC, 1), "yes", lt(COUNTMONEY, PRICE)}, {"Sorry, you do not have enough gold."}},
	{{eq(TOPIC, 2), "yes", PREMIUM, ge(COUNTMONEY, PRICE)}, {"Here we go!", deleteMoney, idle, effectOpp(11), teleport(32628, 32771, 7), effectOpp(11)}},
	{{eq(TOPIC, 2), "yes", lt(COUNTMONEY, PRICE)}, {"Sorry, you do not have enough gold."}},
	{{gt(TOPIC, 0)}, {"Maybe another time."}},

	--"trip"         -> Price=7, "Would you like to travel to the other end of Port Hope or to the centre of the town for 7 gold?", Topic=1
	--"route"        -> *
	--"passage"      -> *
	--"town"         -> *
	--"destination"  -> *
	--"sail"         -> *
	--"go"           -> *

	--Topic=1,"end",PZBlock,! -> "First get rid of those blood stains! You are not going to ruin my vehicle!"
	--Topic=1,"centre",PZBlock,! -> "First get rid of those blood stains! You are not going to ruin my vehicle!"

	--Topic=1,"end",Premium,CountMoney>=Price -> "Here we go!", DeleteMoney, Idle, EffectOpp(11), Teleport(32679,32777,7), EffectOpp(11)
	--Topic=1,"end",CountMoney<Price            -> "Sorry, you do not have enough gold."
	--Topic=1,"centre",Premium,CountMoney>=Price -> "Here we go!", DeleteMoney, Idle, EffectOpp(11), Teleport(32628,32771,7), EffectOpp(11)
	--Topic=1,"centre",CountMoney<Price            -> "Sorry, you do not have enough gold."
	--Topic=1,"yes"                              -> "Did the dworcs have your brain for supper? I was asking you WHERE you want to travel!"
}

function onCreatureSay(npc, creature, text, situation)
	behaviour.react(npc, rules, text, situation)
end

function onCreatureDisappear(npc, creature)
	behaviour.react(npc, rules, "", NPC_SITUATION_VANISH)
end
