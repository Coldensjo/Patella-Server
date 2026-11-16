-- npcs/clyde.lua: the behaviour of the NPC Clyde, converted from
-- clyde.npc by patella-convert. The NPC is its [[npc]] entry in npcs.toml;
-- lib/npc.lua matches and runs the rules below, in the order of the file
-- (scripting.hh, "NPCs"). The comments are those of clyde.npc.
--
-- GIMUD - Graphical Interface Multi User Dungeon
-- clyde.npc: Datenbank für den Tavernenbesitzer Clyde

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
	{{ADDRESS, "hello$", FORCE}, {"Greetings, dear customer."}},
	{{ADDRESS, "hi$", FORCE}, {previous}},
	{{ADDRESS, "hiho$", FORCE}, {previous}},
	{{ADDRESS, FORCE}, {idle}},

	{{BUSY, "hello$", FORCE}, {"Please wait a moment.", queue}},
	{{BUSY, "hi$", FORCE}, {previous}},
	{{BUSY, "hiho$", FORCE}, {previous}},
	{{BUSY, FORCE}, {nop}},
	{{VANISH, FORCE}, {"Good bye."}},

	{{"bye"}, {"Good bye, visit us again.", idle}},
	{{"farewell"}, {previous}},
	{{"job"}, {"I am your host. I run this tavern."}},
	{{"name"}, {"I am Clyde, your host."}},
	{{"time"}, {"Don't worry about the time. Take a seat, have a drink. Time runs differently in Port Hope."}},
	{{"king"}, {"I wish the king would be a bit more concerned about this colony. I am convinced with a few more resources, we could improve Port Hope a lot."}},
	{{"venore"}, {"It's not always easy to deal with those Venoran tradesmen. I must admit they don't show any interest in my area of business. Just between you and me, my friend, sometimes they give me shivers."}},
	{{"thais"}, {"Sadly, Thais is far away and you will notice that in many places. You'll find out about this yourself, so lets talk about something else please."}},
	{{"carlin"}, {"I know only little about Carlin and here nobody cares about it for sure."}},
	{{"edron"}, {"As I started business here, I was hoping for a second Edron. I have not abandoned my hope though. Afer all, this place is called Port Hope, isn't it?"}},
	{{"jungle"}, {"This forest is not an ordinary forest. It's more like a force of nature, like a river or even a storm."}},
	{{"tibia"}, {"I have seen only little of this world. Probably it should be you telling me about the world, and not the other way around."}},
	{{"kazordoon"}, {"If you want to learn something about Kazordoon you should talk to our local dwarves."}},
	{{"dwarves"}, {"There was a handful of dwarves that came here when the colony was founded. They were looking for treasures and gold as far as I know. After some argument a bunch of them left, they headed into the jungle and were never seen again."}},
	{{"dwarfs"}, {previous}},
	{{"ab'dendriel"}, {"Sadly it is next to nothing that I know about the elves and their city."}},
	{{"elves"}, {previous}},
	{{"elfs"}, {previous}},
	{{"darama"}, {"It's a continent full of extremes. The jungle in the humid east, the desert in the dry west."}},
	{{"darashia"}, {"I was there quite often, using the flying carpet. It's quite different from the other towns I have seen, but surely worth a trip."}},
	{{"ankrahmun"}, {"If I were you I'd stay as far away from this town as I could. It is ruled by an undead abomination and its inhabitants worship death."}},
	{{"ferumbras"}, {"This incarnation of evil seems to concentrate his efforts on Thais and its surroundings, but who knows what comes next into the mind of this madman?"}},
	{{"excalibug"}, {"The rumours I overheard did not mention this continent as one of its hiding places."}},
	{{"apes"}, {"They seem to live in the depth of the jungle in ruins that show the markings of the lizard folk. I wonder if they now try to conquer our city too."}},
	{{"lizard"}, {"The lizards are hostile to us. They probably see no big difference between us and the ape people."}},
	{{"dworcs"}, {"The dworcs live in the south in an underground network consisting of caves. They use poisoned weapons and love to build all kind of traps. You don't want to know the fate of those that have been trapped, believe me."}},

	{{"buy"}, {"I can offer you bread, cheese, ham, or meat as well as several drinks."}},
	{{"offer"}, {previous}},
	{{"do", "you", "sell"}, {previous}},
	{{"do", "you", "have"}, {previous}},
	{{"food"}, {"Are you looking for food? I have bread, cheese, ham, and meat."}},
	{{"drink"}, {"I can offer you beer, wine, lemonade, and water."}},

	{{"bread"}, {setType(3600), setAmount(1), setPrice(4), "Do you want to buy a bread for %P gold?", setTopic(1)}},
	{{"cheese"}, {setType(3607), setAmount(1), setPrice(6), "Do you want to buy cheese for %P gold?", setTopic(1)}},
	{{"meat"}, {setType(3577), setAmount(1), setPrice(5), "Do you want to buy meat for %P gold?", setTopic(1)}},
	{{"ham"}, {setType(3582), setAmount(1), setPrice(8), "Do you want to buy a ham for %P gold?", setTopic(1)}},

	{{P1, lt(1, P1), "bread"}, {setType(3600), setAmount(P1), setPrice(mul(4, P1)), "Do you want to buy %A breads for %P gold?", setTopic(1)}},
	{{P1, lt(1, P1), "cheese"}, {setType(3607), setAmount(P1), setPrice(mul(6, P1)), "Do you want to buy %A cheese for %P gold?", setTopic(1)}},
	{{P1, lt(1, P1), "meat"}, {setType(3577), setAmount(P1), setPrice(mul(5, P1)), "Do you want to buy %A meat for %P gold?", setTopic(1)}},
	{{P1, lt(1, P1), "ham"}, {setType(3582), setAmount(P1), setPrice(mul(8, P1)), "Do you want to buy %A ham for %P gold?", setTopic(1)}},

	{{"lemonade"}, {setType(2880), setData(12), setAmount(1), setPrice(2), "Do you want to buy a mug of lemonade for %P gold?", setTopic(1)}},
	{{"beer"}, {setType(2880), setData(3), setAmount(1), setPrice(2), "Do you want to buy a mug of beer for %P gold?", setTopic(1)}},
	{{"wine"}, {setType(2880), setData(2), setAmount(1), setPrice(3), "Do you want to buy a mug of wine for %P gold?", setTopic(1)}},
	{{"water"}, {setType(2880), setData(1), setAmount(1), setPrice(1), "Do you want to buy a mug of water for %P gold?", setTopic(1)}},

	{{P1, lt(1, P1), "lemonade"}, {setType(2880), setData(12), setAmount(P1), setPrice(mul(2, P1)), "Do you want to buy %A mugs of lemonade for %P gold?", setTopic(1)}},
	{{P1, lt(1, P1), "beer"}, {setType(2880), setData(3), setAmount(P1), setPrice(mul(2, P1)), "Do you want to buy %A mugs of beer for %P gold?", setTopic(1)}},
	{{P1, lt(1, P1), "wine"}, {setType(2880), setData(2), setAmount(P1), setPrice(mul(3, P1)), "Do you want to buy %A mugs of wine for %P gold?", setTopic(1)}},
	{{P1, lt(1, P1), "water"}, {setType(2880), setData(1), setAmount(P1), setPrice(mul(1, P1)), "Do you want to buy %A mugs of water for %P gold?", setTopic(1)}},

	{{eq(TOPIC, 1), "yes", ge(COUNTMONEY, PRICE)}, {"Here it is.", deleteMoney, create(TYPE)}},
	{{eq(TOPIC, 1), "yes"}, {"I am sorry, but you do not have enough gold."}},
	{{eq(TOPIC, 1)}, {"Maybe later."}},
}

function onCreatureSay(npc, creature, text, situation)
	behaviour.react(npc, rules, text, situation)
end

function onCreatureDisappear(npc, creature)
	behaviour.react(npc, rules, "", NPC_SITUATION_VANISH)
end
