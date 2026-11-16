-- npcs/blindorc.lua: the behaviour of the NPC Blind Orc, converted from
-- blindorc.npc by patella-convert. The NPC is its [[npc]] entry in npcs.toml;
-- lib/npc.lua matches and runs the rules below, in the order of the file
-- (scripting.hh, "NPCs"). The comments are those of blindorc.npc.
--
-- GIMUD - Graphical Interface Multi User Dungeon
-- blindorc.npc: Datenbank fuer den blinden Orkschmied

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
local create = behaviour.create
local deleteMoney = behaviour.deleteMoney
local idle = behaviour.idle
local nop = behaviour.nop
local previous = behaviour.previous

local rules = behaviour.rules {
	{{ADDRESS, "charach", FORCE}, {"Ikem Charach maruk."}},
	{{ADDRESS, FORCE}, {"Buta humak!", idle}},
	{{BUSY, "charach", FORCE}, {"Ikem napak aluk."}},
	{{BUSY, FORCE}, {nop}},
	{{VANISH, FORCE}, {"Futchi."}},

	{{"futchi"}, {"Futchi!", idle}},
	{{"ikem", "goshak"}, {"Ikem pashak porak, bata, dora. Ba goshak maruk?"}},

	-- verkauft SABRE, SHORT SWORD, SWORD, HATCHET
	{{"goshak", "porak"}, {"Ikem pashak charcha, burka, burka bata, hakhak. Ba goshak maruk?"}},
	{{"goshak", "charcha"}, {setType(3273), setAmount(1), setPrice(25), "Maruk goshak ta?", setTopic(1)}},
	{{"goshak", "burka"}, {setType(3294), setAmount(1), setPrice(30), "Maruk goshak ta?", setTopic(1)}},
	{{"goshak", "burka", "bata"}, {setType(3264), setAmount(1), setPrice(85), "Maruk goshak ta?", setTopic(1)}},
	{{"goshak", "hakhak"}, {setType(3276), setAmount(1), setPrice(85), "Maruk goshak ta?", setTopic(1)}},

	-- verkauft LEATHER ARMOR, STUDDED ARMOR, STUDDED HELMET
	{{"goshak", "bata"}, {"Ikem pashak aka bora, tulak bora, grofa. Ba goshak maruk?"}},
	{{"goshak", "bora"}, {setType(3361), setAmount(1), setPrice(25), "Maruk goshak ta?", setTopic(1)}},
	{{"goshak", "tulak", "bora"}, {setType(3378), setAmount(1), setPrice(90), "Maruk goshak ta?", setTopic(1)}},
	{{"goshak", "grofa"}, {setType(3376), setAmount(1), setPrice(60), "Maruk goshak ta?", setTopic(1)}},

	-- verkauft BRASS SHIELD
	{{"goshak", "dora"}, {"Ikem pashak donga. Ba goshak maruk?"}},
	{{"goshak", "donga"}, {setType(3411), setAmount(1), setPrice(65), "Maruk goshak ta?", setTopic(1)}},

	-- verkauft BOGEN, PFEILE
	{{"goshak", "batuk"}, {setType(3350), setAmount(1), setPrice(400), "Ahhhh, maruk goshak batuk?", setTopic(1)}},
	{{"goshak", "pixo"}, {setType(3447), setAmount(10), setPrice(30), "Maruk goshak tefar pixo ul batuk?", setTopic(1)}},

	{{eq(TOPIC, 1), "mok", ge(COUNTMONEY, PRICE)}, {"Maruk rambo zambo!", deleteMoney, create(TYPE)}},
	{{eq(TOPIC, 1), "mok"}, {"Maruk nixda!"}},
	{{eq(TOPIC, 1), "burp"}, {"Buta maruk klamuk!"}},
	{{eq(TOPIC, 1)}, {previous}},
}

function onCreatureSay(npc, creature, text, situation)
	behaviour.react(npc, rules, text, situation)
end

function onCreatureDisappear(npc, creature)
	behaviour.react(npc, rules, "", NPC_SITUATION_VANISH)
end
