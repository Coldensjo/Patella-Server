-- npcs/chemar.lua: the behaviour of the NPC Chemar, converted from
-- chemar.npc by patella-convert. The NPC is its [[npc]] entry in npcs.toml;
-- lib/npc.lua matches and runs the rules below, in the order of the file
-- (scripting.hh, "NPCs"). The comments are those of chemar.npc.
--
-- GIMUD - Graphical Interface Multi User Dungeon
-- chemar.npc: Datenbank für den Teppichpiloten Chemar in Darashia

local behaviour = require("lib.npc")
local genPost = require("lib.npcs.gen-post")
local ADDRESS = behaviour.ADDRESS
local BUSY = behaviour.BUSY
local VANISH = behaviour.VANISH
local PZBLOCK = behaviour.PZBLOCK
local FORCE = behaviour.FORCE
local eq = behaviour.eq
local gt = behaviour.gt
local ge = behaviour.ge
local TOPIC = behaviour.TOPIC
local PRICE = behaviour.PRICE
local COUNTMONEY = behaviour.COUNTMONEY
local questValue = behaviour.questValue
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
	{{ADDRESS, "hello$", FORCE}, {"Ah, the wind brings in another visitor. Feel welcome %N."}},
	{{ADDRESS, "hi$", FORCE}, {previous}},
	{{ADDRESS, FORCE}, {idle}},
	{{BUSY, "hello$", FORCE}, {"%N! Be calm as the eye of the storm, and your patience will be rewarded.", queue}},
	{{BUSY, "hi$", FORCE}, {previous}},
	{{BUSY, FORCE}, {nop}},
	{{VANISH, FORCE}, {"Daraman's blessings!"}},

	{{"bye"}, {"Daraman's blessings!", idle}},
	{{"name"}, {"My name is Chemar Ibn Kalith."}},
	{{"job"}, {"I am a licensed carpetpilot and responsible for the Darashian airmail. I can bring you to the Femor Hills, Edron, or you can buy letters and parcels."}},
	{{"time"}, {"It's %T, precisely."}},
	{{"caliph"}, {"The caliph depends heavily on his carpetfleet for commerce and for war alike."}},
	{{"kazzan"}, {previous}},
	{{"daraman"}, {"The prophet of our people; praised be his name."}},
	{{"ferumbras"}, {"This scourge of the west may have connections to the evil soils in Drefia."}},
	{{"drefia"}, {"In the west a big city existed. Its people were corrupted and drew the wrath of the djinn upon them and Drefia was destroyed."}},
	{{"excalibug"}, {"I have been almost everywhere in the world and think it's only a myth."}},
	{{"thais"}, {"I think it's a rolemodel for what befalls people if they forget the teachings of Daraman."}},
	{{"carlin"}, {"That city is getting noisier and more crowded each month."}},
	{{"news"}, {"Our carpetpilots bring in too many news to recall them all."}},
	{{"rumour"}, {previous}},
	{{"rumor"}, {previous}},
	{{"flying", "carpet"}, {"Do you want to buy a flying carpet for 5000 platinum coins?", setPrice(500000), setTopic(7)}},
	--"mail"            -> "Our mail system is unique! And everyone can use it. Do you want to know more about it?", Topic=1

	{{"passage"}, {"I can fly you to Femor Hills or Edron if you like. Where do you want to go?"}},
	{{"fly"}, {previous}},
	{{"go"}, {previous}},
	{{"transport"}, {previous}},
	{{"ride"}, {previous}},
	{{"trip"}, {previous}},
	{{"tibia"}, {previous}},

	{{"femur"}, {"Are you sure that you are not talking about the FEMOR Hills?"}},
	{{"hill"}, {setPrice(60), "Do you want to get a ride to the Femor Hills for %P gold?", setTopic(4)}},
	{{"femor"}, {previous}},
	{{"edron"}, {setPrice(40), "Do you want to get a ride to Edron for %P gold?", setTopic(5)}},

	{{"hill", gt(questValue(250), 2)}, {setPrice(50), "Do you want to get a ride to the Femor Hills for %P gold?", setTopic(4)}},
	{{"femor", gt(questValue(250), 2)}, {previous}},
	{{"edron", gt(questValue(250), 2)}, {setPrice(30), "Do you want to get a ride to Edron for %P gold?", setTopic(5)}},

	genPost,

	--"letter" -> Amount=1, Price=5,  "Do you want to buy a letter for %P gold?", Topic=2
	--"parcel" -> Amount=1, Price=10, "Do you want to buy a parcel for %P gold?", Topic=3

	--Topic=1,"yes" -> "The Darashian Airmail System enables you to send and receive letters and parcels. You can buy them here if you want."
	--Topic=1       -> "Is there anything else I can do for you?"

	--Topic=2,"yes",CountMoney>=Price -> "Here it is. Don't forget to write the name of the receiver in the first line and the address in the second one before you put the letter in a mailbox.", DeleteMoney, Create(3505)
	--Topic=2,"yes"                   -> "Oh, you have not enough gold to buy a letter."
	--Topic=2                         -> "Ok."

	--Topic=3,"yes",CountMoney>=Price -> "Here you are. Don't forget to write the name and the address of the receiver on the label. The label has to be in the parcel before you put the parcel in a mailbox.", DeleteMoney, Create(3503), Create(3507)
	--Topic=3,"yes"                   -> "Oh, you have not enough gold to buy a parcel."
	--Topic=3                         -> "Ok."

	{{eq(TOPIC, 4), "yes", PZBLOCK, FORCE}, {"First get rid of those blood stains! You are not going to ruin my vehicle!"}},
	{{eq(TOPIC, 5), "yes", PZBLOCK, FORCE}, {previous}},

	{{eq(TOPIC, 4), "yes", ge(COUNTMONEY, PRICE)}, {"Hold on!", deleteMoney, idle, effectOpp(11), teleport(32535, 31837, 4), effectOpp(11)}},
	{{eq(TOPIC, 4), "yes"}, {"You don't have enough money."}},
	{{eq(TOPIC, 4)}, {"You shouldn't miss the experience."}},

	{{eq(TOPIC, 5), "yes", ge(COUNTMONEY, PRICE)}, {"Hold on!", deleteMoney, idle, effectOpp(11), teleport(33193, 31784, 3), effectOpp(11)}},
	{{eq(TOPIC, 5), "yes"}, {"You don't have enough money."}},
	{{eq(TOPIC, 5)}, {"You shouldn't miss the experience."}},

	{{eq(TOPIC, 7), "yes", ge(COUNTMONEY, PRICE)}, {"Oh, I am sorry, but you have no pilot licence."}},
	{{eq(TOPIC, 7), "yes"}, {"You don't own enough worldly wealth to afford this item."}},
	{{eq(TOPIC, 7)}, {"Maybe another day then, my friend."}},
}

function onCreatureSay(npc, creature, text, situation)
	behaviour.react(npc, rules, text, situation)
end

function onCreatureDisappear(npc, creature)
	behaviour.react(npc, rules, "", NPC_SITUATION_VANISH)
end
