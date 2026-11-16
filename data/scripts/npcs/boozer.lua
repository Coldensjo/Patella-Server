-- npcs/boozer.lua: the behaviour of the NPC Boozer, converted from
-- boozer.npc by patella-convert. The NPC is its [[npc]] entry in npcs.toml;
-- lib/npc.lua matches and runs the rules below, in the order of the file
-- (scripting.hh, "NPCs"). The comments are those of boozer.npc.
--
-- GIMUD - Graphical Interface Multi User Dungeon
-- boozer.npc: Datenbank für den Wirt Boozer

local behaviour = require("lib.npc")
local ADDRESS = behaviour.ADDRESS
local BUSY = behaviour.BUSY
local VANISH = behaviour.VANISH
local MALE = behaviour.MALE
local FEMALE = behaviour.FEMALE
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
	{{ADDRESS, "hello$", FORCE}, {"Welcome to the Hard Rock Racing Track, %N."}},
	{{ADDRESS, "hi$", FORCE}, {previous}},
	{{ADDRESS, FORCE}, {idle}},
	{{BUSY, "hello$", FORCE}, {"Please give me a minute, %N.", queue}},
	{{BUSY, "hi$", FORCE}, {previous}},
	{{BUSY, FORCE}, {nop}},
	{{VANISH, FORCE}, {"You'll be back."}},

	{{"bye"}, {"You'll be back.", idle}},
	{{"job"}, {"I am the bartender here at the racing track."}},
	{{"tavern"}, {previous}},
	{{"frodo"}, {"I heard about his tiny tavern in Thais."}},
	{{"name"}, {"Just call me Boozer. Everyone does that."}},
	{{"time", MALE}, {"No clue, boy."}},
	{{"time", FEMALE}, {"No clue, girl."}},
	{{"king"}, {"The king is far away, so who cares?"}},
	{{"tibianus"}, {previous}},
	{{"army"}, {"Good customers."}},
	{{"ferumbras"}, {"Guess he'd be bad news for business."}},
	{{"excalibug"}, {"Heard about it now and then. Then again I also hear there a bogeyman somewhere in the swamps."}},
	{{"bogeyman"}, {"Just a tale to scare the kids."}},
	{{"thais"}, {"If you like that Thais that much just go there."}},
	{{"tibia"}, {"People from all over Tibia come here to buy, sell, gamble, and get drunk until they puke."}},
	{{"carlin"}, {"Heard about that women there. Must visit that wenches someday."}},
	{{"amazon"}, {"I guess they just have not met the right man yet."}},
	{{"news"}, {"The swampelves, down at Shadowthorn, are up to some trouble again."}},
	{{"rumors"}, {previous}},
	{{"swampelves"}, {"Some elves gone evil so to say. They now live in a small village to the south called Shadowthorn. No big deal. Who cares about some carrot-eating musicians at all?"}},

	{{"bread"}, {setType(3600), setAmount(1), setPrice(4), "Do you want to buy a bread for %P gold?", setTopic(1)}},
	{{"cheese"}, {setType(3607), setAmount(1), setPrice(6), "Do you want to buy cheese for %P gold?", setTopic(1)}},
	{{"meat"}, {setType(3577), setAmount(1), setPrice(5), "Do you want to buy meat for %P gold?", setTopic(1)}},
	{{"ham"}, {setType(3582), setAmount(1), setPrice(8), "Do you want to buy a ham for %P gold?", setTopic(1)}},
	{{"cookie"}, {setType(3598), setAmount(1), setPrice(5), "Do you want to buy a cookie for %P gold?", setTopic(1)}},

	{{P1, lt(1, P1), "bread"}, {setType(3600), setAmount(P1), setPrice(mul(4, P1)), "Do you wanna buy %A breads for %P gold?", setTopic(1)}},
	{{P1, lt(1, P1), "cheese"}, {setType(3607), setAmount(P1), setPrice(mul(6, P1)), "Do you wanna buy %A cheese for %P gold?", setTopic(1)}},
	{{P1, lt(1, P1), "meat"}, {setType(3577), setAmount(P1), setPrice(mul(5, P1)), "Do you wanna buy %A meat for %P gold?", setTopic(1)}},
	{{P1, lt(1, P1), "ham"}, {setType(3582), setAmount(P1), setPrice(mul(8, P1)), "Do you wanna buy %A ham for %P gold?", setTopic(1)}},
	{{P1, lt(1, P1), "cookie"}, {setType(3598), setAmount(P1), setPrice(mul(5, P1)), "Do you wanna buy %A cookies for %P gold?", setTopic(1)}},

	{{"lemonade"}, {setType(2880), setData(12), setAmount(1), setPrice(2), "Do you want to buy a mug of lemonade for %P gold?", setTopic(1)}},
	{{"beer"}, {setType(2880), setData(3), setAmount(1), setPrice(2), "Do you want to buy a mug of beer for %P gold?", setTopic(1)}},
	{{"wine"}, {setType(2880), setData(2), setAmount(1), setPrice(3), "Do you want to buy a mug of wine for %P gold?", setTopic(1)}},
	{{"water"}, {setType(2880), setData(1), setAmount(1), setPrice(1), "Do you want to buy a mug of water for %P gold?", setTopic(1)}},

	{{P1, lt(1, P1), "lemonade"}, {setType(2880), setData(12), setAmount(P1), setPrice(mul(2, P1)), "Do you want to buy %A mugs of lemonade for %P gold?", setTopic(1)}},
	{{P1, lt(1, P1), "beer"}, {setType(2880), setData(3), setAmount(P1), setPrice(mul(2, P1)), "Do you want to buy %A mugs of beer for %P gold?", setTopic(1)}},
	{{P1, lt(1, P1), "wine"}, {setType(2880), setData(2), setAmount(P1), setPrice(mul(3, P1)), "Do you want to buy %A mugs of wine for %P gold?", setTopic(1)}},
	{{P1, lt(1, P1), "water"}, {setType(2880), setData(1), setAmount(P1), setPrice(mul(1, P1)), "Do you want to buy %A mugs of water for %P gold?", setTopic(1)}},

	{{eq(TOPIC, 1), "yes", ge(COUNTMONEY, PRICE)}, {"And here is what you ordered.", deleteMoney, create(TYPE)}},
	{{eq(TOPIC, 1), "yes"}, {"You don't have the gold. If we were gambling I'd call you a cheater ... and you know what happens to cheaters, don't you?"}},
	{{eq(TOPIC, 1)}, {"Then not, fine with me."}},

	{{"buy"}, {"I can offer you food and drinks. Get anything else somewhere else and don't bother me."}},
	{{"do", "you", "sell"}, {previous}},
	{{"do", "you", "have"}, {previous}},
	{{"food"}, {"So you are looking for food? We have cookies, bread, cheese, ham, and meat."}},
	{{"drink"}, {"I can offer you beer, wine, lemonade, and water."}},
}

function onCreatureSay(npc, creature, text, situation)
	behaviour.react(npc, rules, text, situation)
end

function onCreatureDisappear(npc, creature)
	behaviour.react(npc, rules, "", NPC_SITUATION_VANISH)
end
