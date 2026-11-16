-- npcs/shirith.lua: the behaviour of the NPC Shirith, converted from
-- shirith.npc by patella-convert. The NPC is its [[npc]] entry in npcs.toml;
-- lib/npc.lua matches and runs the rules below, in the order of the file
-- (scripting.hh, "NPCs"). The comments are those of shirith.npc.
--
-- GIMUD - Graphical Interface Multi User Dungeon
-- shirith.npc: Datenbank für den Minenleiter Shirith (Elfenstadt)

local behaviour = require("lib.npc")
local ADDRESS = behaviour.ADDRESS
local BUSY = behaviour.BUSY
local VANISH = behaviour.VANISH
local FORCE = behaviour.FORCE
local eq = behaviour.eq
local ge = behaviour.ge
local TOPIC = behaviour.TOPIC
local PRICE = behaviour.PRICE
local TYPE = behaviour.TYPE
local COUNTMONEY = behaviour.COUNTMONEY
local setTopic = behaviour.setTopic
local setPrice = behaviour.setPrice
local setType = behaviour.setType
local setData = behaviour.setData
local create = behaviour.create
local deleteMoney = behaviour.deleteMoney
local queue = behaviour.queue
local idle = behaviour.idle
local nop = behaviour.nop
local previous = behaviour.previous

local rules = behaviour.rules {
	{{ADDRESS, "hello$", FORCE}, {"Ashari %N."}},
	{{ADDRESS, "hi$", FORCE}, {previous}},
	{{ADDRESS, "ashari$", FORCE}, {previous}},
	{{ADDRESS, FORCE}, {idle}},
	{{BUSY, "hello$", FORCE}, {"Wait, %N.", queue}},
	{{BUSY, "hi$", FORCE}, {previous}},
	{{BUSY, "ashari$", FORCE}, {previous}},
	{{BUSY, FORCE}, {nop}},
	{{VANISH, FORCE}, {"Asha Thrazi."}},

	{{"bye"}, {"Asha Thrazi.", idle}},
	{{"farewell"}, {previous}},
	{{"asha", "thrazi"}, {previous}},
	{{"job"}, {"I am the overseer of the mines."}},
	{{"name"}, {"I am called Shirith Blooddancer."}},
	{{"time"}, {"It is %T."}},

	{{"carlin"}, {"I think those humans are trespassing elven teritory far too often."}},
	{{"thais"}, {"Thais is far away as all humans should be."}},
	{{"venore"}, {"If it comes to trade, I can respect those merchants. As long as they leave as soon as they finished buisness, that is."}},
	{{"roderick"}, {"We don't need him or any other ambassador here."}},
	{{"olrik"}, {"As a post officer he has some use ... as a troll has some use for mining."}},

	{{"elves"}, {"We are a superior race, indeed."}},
	{{"dwarfs"}, {"They could be of ... some use."}},
	{{"human"}, {"Humans are more annoying than our trolls."}},
	{{"troll"}, {"We give these useless creatures a reason to live by serving us."}},
	{{"cenath"}, {"They think they are better then us."}},
	{{"kuridai"}, {"We keep this society running. Without our tools and work our case would be a lost one."}},
	{{"deraisim"}, {"They could do more for us if they would try more hard."}},
	{{"abdaisim"}, {"Let them go, we don't need them."}},
	{{"teshial"}, {"Who needs dreamers in these days?"}},
	{{"ferumbras"}, {"He should be destroyed."}},
	{{"mines"}, {"We hardly get the ore we need. The worthless trolls are lazy workers. I keep them locked up the whole time."}},
	{{"locked"}, {"I keep the keys to the mines."}},
	{{"excalibug"}, {"Nonsense."}},
	{{"news"}, {"Trolls are boring, I have no news to tell."}},

	{{"key"}, {setType(2969), setData(3033), setPrice(50), "I would sell you a key for 50 gold, ok?", setTopic(1)}},
	{{eq(TOPIC, 1), "yes", ge(COUNTMONEY, PRICE)}, {"Here it is.", deleteMoney, create(TYPE)}},
	{{eq(TOPIC, 1), "yes"}, {"You do not have enough gold."}},
	{{eq(TOPIC, 1)}, {"Ok, then not."}},
}

function onCreatureSay(npc, creature, text, situation)
	behaviour.react(npc, rules, text, situation)
end

function onCreatureDisappear(npc, creature)
	behaviour.react(npc, rules, "", NPC_SITUATION_VANISH)
end
