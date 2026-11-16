-- npcs/leeland.lua: the behaviour of the NPC Leeland, converted from
-- leeland.npc by patella-convert. The NPC is its [[npc]] entry in npcs.toml;
-- lib/npc.lua matches and runs the rules below, in the order of the file
-- (scripting.hh, "NPCs"). The comments are those of leeland.npc.
--
-- GIMUD - Graphical Interface Multi User Dungeon
-- leeland.npc: Datenbank für den Besitzer des allgemeinen Markts Leeland Slim

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
	{{ADDRESS, "hello$", FORCE}, {"Greetings, %N!"}},
	{{ADDRESS, "hi$", FORCE}, {previous}},
	{{ADDRESS, "hail$", FORCE}, {previous}},
	{{ADDRESS, FORCE}, {idle}},
	{{BUSY, "hello$", FORCE}, {"Please give me another minute, %N. I am talking already, but will be avaliable for you very soon.", queue}},
	{{BUSY, "hi$", FORCE}, {previous}},
	{{BUSY, "hail$", FORCE}, {previous}},
	{{BUSY, FORCE}, {nop}},
	{{VANISH, FORCE}, {"Good bye, %N. Make sure to visit my shop."}},

	{{"bye"}, {"Good bye, %N. Make sure to visit my shop.", idle}},
	{{"name"}, {"My name is Slim, Leeland Slim."}},
	{{"job"}, {"I am the owner of the Useful Things Warehouse."}},
	{{"warehouse"}, {"I offer many things. Actually I am sure you will find something you have desired for a long time."}},
	{{"time"}, {"So it is a watch you need? Make sure to buy one downstairs."}},
	{{"king"}, {"Do I hear envy in your voice? Is it that you realy want? To be ... king? Well, one never knows ... perhaps you find something royal in my warehouse."}},
	{{"tibianus"}, {previous}},
	{{"army"}, {"Your question suggests you are interested in military? I am sure you will find something interesting in my warehouse."}},
	{{"ferumbras"}, {"Ah, Ferumbras. I remember selling him torches for his first adventures as if it was yesterday."}},
	{{"excalibug"}, {"That's one of the few things even I can't aquire for you."}},
	{{"thais"}, {"Perhaps one day I will settle in thais again. I love the city's potential."}},
	{{"tibia"}, {"I have seen most of it. And I like it. <chuckles>"}},
	{{"carlin"}, {"I was there some time ago. I exchanged ideas with some important people there and even could sell them something that furthered their cause."}},
	{{"news"}, {"I would love to have the time to chat and exchange gossip, but sadly business always comes first, you know?"}},
	{{"tax"}, {"Taxes are a necessary evil, people say. I like that."}},
	{{"privilege"}, {"Privileges have to be paid for. The one way ... or the other."}},
	{{"gambling"}, {"I just love bets and gambling. It inspires people doing such interesting things."}},
}

function onCreatureSay(npc, creature, text, situation)
	behaviour.react(npc, rules, text, situation)
end

function onCreatureDisappear(npc, creature)
	behaviour.react(npc, rules, "", NPC_SITUATION_VANISH)
end
