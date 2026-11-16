-- npcs/urkalio.lua: the behaviour of the NPC Urkalio, converted from
-- urkalio.npc by patella-convert. The NPC is its [[npc]] entry in npcs.toml;
-- lib/npc.lua matches and runs the rules below, in the order of the file
-- (scripting.hh, "NPCs"). The comments are those of urkalio.npc.
--
-- GIMUD - Graphical Interface Multi User Dungeon
-- urkalio.npc: Datenbank für den Wirt Urkalio

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
local setData = behaviour.setData
local create = behaviour.create
local deleteMoney = behaviour.deleteMoney
local queue = behaviour.queue
local idle = behaviour.idle
local nop = behaviour.nop
local previous = behaviour.previous

local rules = behaviour.rules {
	{{ADDRESS, "hello$", FORCE}, {"Welcome to the pits of the Hard Rock Tavern, %N."}},
	{{ADDRESS, "hi$", FORCE}, {previous}},
	{{ADDRESS, FORCE}, {idle}},
	{{BUSY, "hello$", FORCE}, {"Hey, some patience, %N. You'll be served soon enough.", queue}},
	{{BUSY, "hi$", FORCE}, {previous}},
	{{BUSY, FORCE}, {nop}},
	{{VANISH, FORCE}, {"We're some hasty one, aren't we?"}},

	{{"bye"}, {"Have a good fight, %N.", idle}},
	{{"job"}, {"I am responsible for the Hard Rock Pits Tavern."}},
	{{"tavern"}, {previous}},
	{{"upper", "part"}, {"If you can't stand some blood and battlecrys, just go upstairs."}},
	{{"pits"}, {"Choose your enemies with care."}},
	{{"asrak"}, {"He's the best. To be the man, you'll have to beat the minotaur, so to say. Not that you could provoke him to a fight at all."}},
	{{"name"}, {"I am Urkalio."}},
	{{"maria"}, {"She's kind of my boss."}},
	{{"time"}, {"No clue, it's equally dark down here at any time."}},
	{{"king"}, {"Down here everyone is king as far as where his weapons reach."}},
	{{"tibianus"}, {previous}},
	{{"army"}, {"A shame they don't visit our pits for some training."}},
	{{"ferumbras"}, {"THAT would be some attraction down here."}},
	{{"excalibug"}, {"I would love to see that weapon in a fight."}},
	{{"thais"}, {"Such a boring city. I wonder why anyone would live there."}},
	{{"tibia"}, {"Sooner or later everyone comes here, so why bother to travel."}},
	{{"carlin"}, {"I don't care about their 'independence war'."}},
	{{"amazon"}, {"Some came here to challenge the local champions. I can't say I was impressed by their skills. However, they took a few heads as trophies."}},
	{{"news"}, {"I bet you want to hear about those swampelves from Shadowthorn."}},
	{{"rumors"}, {previous}},
	{{"swampelves"}, {"If they want a fight that bad, why don't they just come here and fight in the pits?"}},

	{{"buy"}, {"I sell food and drinks for the hungry and thirsty."}},
	{{"do", "you", "sell"}, {previous}},
	{{"do", "you", "have"}, {previous}},
	{{"food"}, {"I have cookies, bread, cheese, ham, and meat."}},
	{{"drink"}, {"So do you want beer, wine, lemonade, or ... uhm ... water?"}},

	{{"bread"}, {setType(3600), setAmount(1), setPrice(4), "Do you want to buy a bread for %P gold?", setTopic(1)}},
	{{"cheese"}, {setType(3607), setAmount(1), setPrice(6), "Do you want to buy cheese for %P gold?", setTopic(1)}},
	{{"meat"}, {setType(3577), setAmount(1), setPrice(5), "Do you want to buy meat for %P gold?", setTopic(1)}},
	{{"ham"}, {setType(3582), setAmount(1), setPrice(8), "Do you want to buy a ham for %P gold?", setTopic(1)}},
	{{"cookie"}, {setType(3598), setAmount(1), setPrice(5), "Do you want to buy a cookie for %P gold?", setTopic(1)}},

	{{"lemonade"}, {setType(2880), setData(12), setAmount(1), setPrice(2), "Do you want to buy a mug of lemonade for %P gold?", setTopic(1)}},
	{{"beer"}, {setType(2880), setData(3), setAmount(1), setPrice(2), "Do you want to buy a mug of beer for %P gold?", setTopic(1)}},
	{{"wine"}, {setType(2880), setData(2), setAmount(1), setPrice(3), "Do you want to buy a mug of wine for %P gold?", setTopic(1)}},
	{{"water"}, {setType(2880), setData(1), setAmount(1), setPrice(1), "<cough chuckle cough> So you want to buy a mug of ... water for %P gold?", setTopic(1)}},

	{{P1, lt(1, P1), "bread"}, {setType(3600), setAmount(P1), setPrice(mul(4, P1)), "Do you want to buy %A breads for %P gold?", setTopic(1)}},
	{{P1, lt(1, P1), "cheese"}, {setType(3607), setAmount(P1), setPrice(mul(6, P1)), "Do you want to buy %A cheese for %P gold?", setTopic(1)}},
	{{P1, lt(1, P1), "meat"}, {setType(3577), setAmount(P1), setPrice(mul(5, P1)), "Do you want to buy %A meat for %P gold?", setTopic(1)}},
	{{P1, lt(1, P1), "ham"}, {setType(3582), setAmount(P1), setPrice(mul(8, P1)), "Do you want to buy %A ham for %P gold?", setTopic(1)}},
	{{P1, lt(1, P1), "cookie"}, {setType(3598), setAmount(P1), setPrice(mul(5, P1)), "Do you want to buy %A cookies for %P gold?", setTopic(1)}},

	{{P1, lt(1, P1), "lemonade"}, {setType(2880), setData(12), setAmount(P1), setPrice(mul(2, P1)), "Do you want to buy %A mugs of lemonade for %P gold?", setTopic(1)}},
	{{P1, lt(1, P1), "beer"}, {setType(2880), setData(3), setAmount(P1), setPrice(mul(2, P1)), "Do you want to buy %A mugs of beer for %P gold?", setTopic(1)}},
	{{P1, lt(1, P1), "wine"}, {setType(2880), setData(2), setAmount(P1), setPrice(mul(3, P1)), "Do you want to buy %A mugs of wine for %P gold?", setTopic(1)}},
	{{P1, lt(1, P1), "water"}, {setType(2880), setData(1), setAmount(P1), setPrice(mul(1, P1)), "<cough chuckle cough> So you want to buy %A mugs of ... water for %P gold?", setTopic(1)}},

	{{eq(TOPIC, 1), "yes", ge(COUNTMONEY, PRICE)}, {"Here you are.", deleteMoney, create(TYPE)}},
	{{eq(TOPIC, 1), "yes"}, {"I guess you run out of money. Leave before I run out of patience.", idle}},
	{{eq(TOPIC, 1)}, {"Don't waste my time, kid."}},
}

function onCreatureSay(npc, creature, text, situation)
	behaviour.react(npc, rules, text, situation)
end

function onCreatureDisappear(npc, creature)
	behaviour.react(npc, rules, "", NPC_SITUATION_VANISH)
end
