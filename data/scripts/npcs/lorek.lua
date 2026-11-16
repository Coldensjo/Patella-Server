-- npcs/lorek.lua: the behaviour of the NPC Lorek, converted from
-- lorek.npc by patella-convert. The NPC is its [[npc]] entry in npcs.toml;
-- lib/npc.lua matches and runs the rules below, in the order of the file
-- (scripting.hh, "NPCs"). The comments are those of lorek.npc.
--
-- GIMUD - Graphical Interface Multi User Dungeon
-- Lorek.npc: Datenbank für den Fährman Lorek

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
	{{ADDRESS, "hello$", FORCE}, {"Greetings, %N."}},
	{{ADDRESS, "hi$", FORCE}, {previous}},
	{{ADDRESS, "hiho$", FORCE}, {previous}},
	{{ADDRESS, FORCE}, {idle}},

	{{BUSY, "hello$", FORCE}, {"Just wait.", queue}},
	{{BUSY, "hi$", FORCE}, {previous}},
	{{BUSY, "hiho$", FORCE}, {previous}},
	{{BUSY, FORCE}, {nop}},
	{{VANISH, FORCE}, {"Bye."}},

	{{"bye"}, {"Bye.", idle}},
	{{"farewell"}, {previous}},
	{{"job"}, {"I am a ferryman. If you want me to transport you to the other end of the city, feel free to ask me for a passage."}},
	{{"name"}, {"I am Lorek."}},
	{{"time"}, {"I have no idea."}},
	{{"king"}, {"I wonder if he will inspect our colony some day."}},
	{{"venore"}, {"It seems the traders are incredibly rich."}},
	{{"thais"}, {"I left Thais for the opportunities that might be found here."}},
	{{"carlin"}, {"I am not sure if we are at war with them. I think they defy the rule of our king."}},
	{{"edron"}, {"Edron has to be very pretty and all people there are rich and such."}},
	{{"jungle"}, {"I can only hope that the guards protect us all from those dangerous beasts out there."}},

	{{"tibia"}, {"The world is so big that it often scares me."}},

	{{"kazordoon"}, {"I overheard the dwarves talking about it. I have no idea what it is though."}},
	{{"dwarves"}, {"There are some dwarves living here."}},
	{{"dwarfs"}, {previous}},
	{{"ab'dendriel"}, {"What?"}},
	{{"elves"}, {"I only heard of them, but I never saw one. It's said that they have funny ears."}},
	{{"elfs"}, {previous}},
	{{"darama"}, {"If more people move to Darama, I might get a better job and earn a fortune."}},
	{{"darashia"}, {"Another human settlement on this continent. It's somewhere in the desert though."}},
	{{"ankrahmun"}, {"They say it's a city full of undead and half-dead people. What a horrible thought!"}},
	{{"ferumbras"}, {"I heard he is some scary magician or so."}},
	{{"excalibug"}, {"What's that?"}},
	{{"apes"}, {"If only the guards could stop their constant attacks."}},
	{{"lizard"}, {"I have only heard about them. I hope they won't come here."}},
	{{"dworcs"}, {"Those little greenskins are more dangerous than a cobra."}},

	{{"trip"}, {"I can bring you either to the centre of Port Hope or to the west end of the town, where would you like to go?"}},
	{{"route"}, {previous}},
	{{"passage"}, {previous}},
	{{"destination"}, {previous}},
	{{"sail"}, {previous}},
	{{"go"}, {previous}},

	{{"cent"}, {setPrice(7), "Do you seek a passage to the centre of Port Hope for %P gold?", setTopic(1)}},
	{{"west"}, {setPrice(7), "Do you seek a passage to the west end of Port Hope for %P gold?", setTopic(2)}},

	{{eq(TOPIC, 1), "yes", PZBLOCK, FORCE}, {"First get rid of those blood stains! You are not going to ruin my vehicle!"}},
	{{eq(TOPIC, 2), "yes", PZBLOCK, FORCE}, {"First get rid of those blood stains! You are not going to ruin my vehicle!"}},

	{{eq(TOPIC, 1), "yes", PREMIUM, ge(COUNTMONEY, PRICE)}, {"Here we go!", deleteMoney, idle, effectOpp(11), teleport(32628, 32771, 7), effectOpp(11)}},
	{{eq(TOPIC, 1), "yes", lt(COUNTMONEY, PRICE)}, {"Sorry, you do not have enough gold."}},
	{{eq(TOPIC, 2), "yes", PREMIUM, ge(COUNTMONEY, PRICE)}, {"Here we go!", deleteMoney, idle, effectOpp(11), teleport(32558, 32780, 7), effectOpp(11)}},
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

	--Topic=1,"end",Premium,CountMoney>=Price -> "Here we go!", DeleteMoney, Idle, EffectOpp(11), Teleport(32558,32780,7), EffectOpp(11)
	--Topic=1,"end",CountMoney<Price            -> "Sorry, you do not have enough gold."
	--Topic=1,"centre",Premium,CountMoney>=Price -> "Here we go!", DeleteMoney, Idle, EffectOpp(11), Teleport(32628,32771,7), EffectOpp(11)
	--Topic=1,"centre",CountMoney<Price            -> "Sorry, you do not have enough gold."
	--Topic=1,"yes"                              -> "HELLO? <knocks on your head> Anyone in there? I was asking you WHERE you want to travel!"
}

function onCreatureSay(npc, creature, text, situation)
	behaviour.react(npc, rules, text, situation)
end

function onCreatureDisappear(npc, creature)
	behaviour.react(npc, rules, "", NPC_SITUATION_VANISH)
end
