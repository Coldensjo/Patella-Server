-- npcs/nelliem.lua: the behaviour of the NPC Nelliem, converted from
-- nelliem.npc by patella-convert. The NPC is its [[npc]] entry in npcs.toml;
-- lib/npc.lua matches and runs the rules below, in the order of the file
-- (scripting.hh, "NPCs"). The comments are those of nelliem.npc.
--
-- GIMUD - Graphical Interface Multi User Dungeon
-- Nelliem.npc: Datenbank für den Gartenbedarfhändler Nelliem

local behaviour = require("lib.npc")
local ADDRESS = behaviour.ADDRESS
local BUSY = behaviour.BUSY
local VANISH = behaviour.VANISH
local FORCE = behaviour.FORCE
local P1 = behaviour.P1
local eq = behaviour.eq
local lt = behaviour.lt
local ge = behaviour.ge
local TOPIC = behaviour.TOPIC
local PRICE = behaviour.PRICE
local TYPE = behaviour.TYPE
local COUNTMONEY = behaviour.COUNTMONEY
local mul = behaviour.mul
local setTopic = behaviour.setTopic
local setPrice = behaviour.setPrice
local setAmount = behaviour.setAmount
local setType = behaviour.setType
local create = behaviour.create
local deleteMoney = behaviour.deleteMoney
local queue = behaviour.queue
local idle = behaviour.idle
local nop = behaviour.nop
local previous = behaviour.previous

local rules = behaviour.rules {
	{{ADDRESS, "hello$", FORCE}, {"Greetings, %N, traveller from afar..."}},
	{{ADDRESS, "hi$", FORCE}, {previous}},
	{{ADDRESS, FORCE}, {idle}},
	{{BUSY, "hello$", FORCE}, {"Hold on, %N, I am busy. Just stand in the line.", queue}},
	{{BUSY, "hi$", FORCE}, {previous}},
	{{BUSY, FORCE}, {nop}},
	{{VANISH, FORCE}, {"Go...! Learn the secret to green thumbs and may Crunor be good to you..."}},

	{{"bye"}, {"Go...! Learn the secret to green thumbs and may Crunor be good to you...", idle}},
	{{"farewell"}, {previous}},
	{{"job"}, {"To keep my thumbs green and to sell our garden equipment, as you can see on that shelves."}},
	{{"crunor"}, {"May he bless all plants."}},
	{{"name"}, {"I am Nelliem."}},
	{{"time"}, {"It's a good time to sow some seeds."}},

	{{"equipment"}, {"I sell shovels, picks, scythes, machetes, ropes, pitchforks, rakes, hoes, brooms, fishing rods, sixpacks of worms and brandnew crowbars from Kazordoon."}},
	{{"goods"}, {previous}},
	{{"do", "you", "sell"}, {previous}},
	{{"do", "you", "have"}, {previous}},
	{{"offer"}, {previous}},

	{{"shovel"}, {setType(3457), setAmount(1), setPrice(20), "Do you want to buy a shovel for %P gold?", setTopic(1)}},
	{{"scythe"}, {setType(3453), setAmount(1), setPrice(25), "Do you want to buy a scythe for %P gold?", setTopic(1)}},
	{{"pick"}, {setType(3456), setAmount(1), setPrice(50), "Do you want to buy a pick for %P gold?", setTopic(1)}},
	{{"rope"}, {setType(3003), setAmount(1), setPrice(50), "Do you want to buy a rope for %P gold?", setTopic(1)}},
	{{"rod"}, {setType(3483), setAmount(1), setPrice(150), "Do you want to buy a fishing rod for %P gold?", setTopic(1)}},
	{{"crowbar"}, {setType(3304), setAmount(1), setPrice(260), "Do you want to buy a dwarfensteel crowbar for %P gold?", setTopic(1)}},
	{{"machete"}, {setType(3308), setAmount(1), setPrice(35), "Do you want to buy a machete for %P gold?", setTopic(1)}},
	{{"pitchfork"}, {setType(3451), setAmount(1), setPrice(25), "Do you want to buy a pitchfork for %P gold?", setTopic(1)}},
	{{"rake"}, {setType(3452), setAmount(1), setPrice(20), "Do you want to buy a rake for %P gold?", setTopic(1)}},
	{{"hoe"}, {setType(3455), setAmount(1), setPrice(15), "Do you want to buy a hoe for %P gold?", setTopic(1)}},
	{{"broom"}, {setType(3454), setAmount(1), setPrice(12), "Do you want to buy a broom for %P gold?", setTopic(1)}},

	{{P1, lt(1, P1), "shovel"}, {setType(3457), setAmount(P1), setPrice(mul(20, P1)), "Do you want to buy %A shovels for %P gold?", setTopic(1)}},
	{{P1, lt(1, P1), "scythe"}, {setType(3453), setAmount(P1), setPrice(mul(25, P1)), "Do you want to buy %A scythes for %P gold?", setTopic(1)}},
	{{P1, lt(1, P1), "pick"}, {setType(3456), setAmount(P1), setPrice(mul(50, P1)), "Do you want to buy %A picks for %P gold?", setTopic(1)}},
	{{P1, lt(1, P1), "rope"}, {setType(3003), setAmount(P1), setPrice(mul(50, P1)), "Do you want to buy %A ropes for %P gold?", setTopic(1)}},
	{{P1, lt(1, P1), "rod"}, {setType(3483), setAmount(P1), setPrice(mul(150, P1)), "Do you want to buy %A fishing rods for %P gold?", setTopic(1)}},
	{{P1, lt(1, P1), "crowbar"}, {setType(3304), setAmount(P1), setPrice(mul(260, P1)), "Do you want to buy %A dwarfensteel crowbars for %P gold?", setTopic(1)}},
	{{P1, lt(1, P1), "machete"}, {setType(3308), setAmount(P1), setPrice(mul(35, P1)), "Do you want to buy %A machetes for %P gold?", setTopic(1)}},
	{{P1, lt(1, P1), "pitchfork"}, {setType(3451), setAmount(P1), setPrice(mul(25, P1)), "Do you want to buy %A pitchforks for %P gold?", setTopic(1)}},
	{{P1, lt(1, P1), "rake"}, {setType(3452), setAmount(P1), setPrice(mul(20, P1)), "Do you want to buy %A rakes for %P gold?", setTopic(1)}},
	{{P1, lt(1, P1), "hoe"}, {setType(3455), setAmount(P1), setPrice(mul(15, P1)), "Do you want to buy %A hoes for %P gold?", setTopic(1)}},
	{{P1, lt(1, P1), "broom"}, {setType(3454), setAmount(P1), setPrice(mul(12, P1)), "Do you want to buy %A brooms for %P gold?", setTopic(1)}},

	{{"worm"}, {"I sell worms only in sixpacks for 5 gold each, how many sixpacks of worms do you want to buy?"}},
	{{"worms"}, {previous}},
	{{P1, "worms"}, {setType(3492), setAmount(mul(P1, 6)), setPrice(mul(5, P1)), "So you want to buy some of my sixpacks with altogether %A worms for %P gold?", setTopic(1)}},
	{{P1, "six", "pack"}, {previous}},

	{{eq(TOPIC, 1), "yes", ge(COUNTMONEY, PRICE)}, {"Here!", deleteMoney, create(TYPE)}},
	{{eq(TOPIC, 1), "yes"}, {"No money, no deal!"}},
	{{eq(TOPIC, 1)}, {"Then not."}},
}

function onCreatureSay(npc, creature, text, situation)
	behaviour.react(npc, rules, text, situation)
end

function onCreatureDisappear(npc, creature)
	behaviour.react(npc, rules, "", NPC_SITUATION_VANISH)
end
