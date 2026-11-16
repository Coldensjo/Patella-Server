-- npcs/karl.lua: the behaviour of the NPC Karl, converted from
-- karl.npc by patella-convert. The NPC is its [[npc]] entry in npcs.toml;
-- lib/npc.lua matches and runs the rules below, in the order of the file
-- (scripting.hh, "NPCs"). The comments are those of karl.npc.
--
-- GIMUD - Graphical Interface Multi User Dungeon
-- karl.npc: Datenbank fuer den Wirt der vebotenen Kneipe in Carlin

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
local setAmount = behaviour.setAmount
local setType = behaviour.setType
local setData = behaviour.setData
local create = behaviour.create
local deleteMoney = behaviour.deleteMoney
local queue = behaviour.queue
local idle = behaviour.idle
local nop = behaviour.nop
local previous = behaviour.previous

local rules = behaviour.rules {
	{{ADDRESS, "hello$", FORCE}, {"Pshhhht! Not that loud ... but welcome."}},
	{{ADDRESS, "hi$", FORCE}, {previous}},
	{{ADDRESS, FORCE}, {idle}},
	{{BUSY, "hello$", FORCE}, {"Just a moment.", queue}},
	{{BUSY, "hi$", FORCE}, {previous}},
	{{BUSY, FORCE}, {nop}},
	{{VANISH, FORCE}, {"Please come back, but don't tell others."}},

	{{"bye"}, {"Please come back, but don't tell others.", idle}},
	{{"farewell"}, {previous}},
	{{"job"}, {"I am the responsible for our ... uhm ... resistance."}},
	{{"saloon"}, {previous}},
	{{"resistance"}, {"We fight the opression of the males and male needs by the women. This is our secret headquarters."}},
	{{"headquarters"}, {"Well its more a hidden tavern, so to say."}},
	{{"tavern"}, {"Our offers are limited but here a man can buy what a man needs."}},
	{{"name"}, {"I won't tell you my name."}},
	{{"karl"}, {"Who told you that???"}},
	{{"queen"}, {"Well, shes not that bad ... but some of her laws are."}},
	{{"eloise"}, {previous}},
	{{"laws"}, {"Those crazy women forbid us alcohol in the city! Imagine that!"}},
	{{"needs"}, {previous}},
	{{"opression"}, {previous}},
	{{"alcohol"}, {previous}},
	{{"army"}, {"They are the tools of opression. Hunting down every alcohol smuggler they can get."}},
	{{"smuggler"}, {"We collected money and hired one of the best smuggler in the whole land. His name is Todd."}},
	{{"Todd"}, {"A true fighter for malehood. He will bring us all the hard stuff from Thais and even contact the king there to support us."}},
	{{"king"}, {"I'm sure if the king learns about our tragedy, he will support us with alcohol."}},
	{{"hard", "stuff"}, {"Todd took all the money we could gather to buy us the best stuff on the whole continent."}},
	{{"hugo"}, {"I think Todd mentioned a Hugo once."}},
	{{"news"}, {"Some travelers from Edron told about a great treasure guarded by cruel demons in the dungeons there."}},
	{{"rumors"}, {previous}},
	{{"beer"}, {setType(2880), setData(3), setAmount(1), setPrice(20), "Do you want to buy a mug of beer for %P gold?", setTopic(1)}},
	{{eq(TOPIC, 1), "yes", ge(COUNTMONEY, PRICE)}, {"Here. Don't take it into the city though.", deleteMoney, create(TYPE)}},
	{{eq(TOPIC, 1), "yes"}, {"Sorry, come back with more gold."}},
	{{eq(TOPIC, 1)}, {"Maybe later."}},

	{{"buy"}, {"I can offer you beer. For wine and realy hard stuff we have to wait for Todd."}},
	{{"offers"}, {previous}},
	{{"do", "you", "sell"}, {previous}},
	{{"do", "you", "have"}, {previous}},
}

function onCreatureSay(npc, creature, text, situation)
	behaviour.react(npc, rules, text, situation)
end

function onCreatureDisappear(npc, creature)
	behaviour.react(npc, rules, "", NPC_SITUATION_VANISH)
end
