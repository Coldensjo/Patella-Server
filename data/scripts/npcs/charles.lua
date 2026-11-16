-- npcs/charles.lua: the behaviour of the NPC Charles, converted from
-- charles.npc by patella-convert. The NPC is its [[npc]] entry in npcs.toml;
-- lib/npc.lua matches and runs the rules below, in the order of the file
-- (scripting.hh, "NPCs"). The comments are those of charles.npc.
--
-- GIMUD - Graphical Interface Multi User Dungeon
-- charles.npc: Datenbank für den Kapitän Charles

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
	{{ADDRESS, "hello$", FORCE}, {"Ahoi."}},
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
	{{"job"}, {"I am the captain of the Poodle, the proudest ship on all oceans."}},
	{{"name"}, {"It's Charles."}},
	{{"time"}, {"It is precisely %T."}},
	{{"king"}, {"His majesty himself was present at the day the Poodle was launched."}},

	{{"jungle"}, {"It's a fascinating forest, full of exotic life. If it weren't for my duties, I would spend some time just exploring this jungle."}},

	{{"tibia"}, {"We live in a fascinating world with even more fascinating oceans. And all its major harbours are known to me."}},
	{{"major", "harbour"}, {"Well the harbours of thais, venore, carlin, edron, darashia and ankrahmun. Do you have any questions about one of those harbours?", setTopic(20)}},
	{{eq(TOPIC, 20), "venore"}, {"The Venorans build fine ships. Enough said about them."}},
	{{eq(TOPIC, 20), "thais"}, {"Thais is the proud capital of the largest kingdom in the known world."}},
	{{eq(TOPIC, 20), "carlin"}, {"Rebellious women might be amusing for a while, but it is time for them to stop this nonsense and return to the kingdom."}},
	{{eq(TOPIC, 20), "edron"}, {"The coastline of Edron is treacherous and it takes some skills to sail a ship safely into the harbour."}},
	{{eq(TOPIC, 20), "darashia"}, {"An unremarkable little town with a small harbour and quiet people."}},
	{{eq(TOPIC, 20), "ankrahmun"}, {"The city is surely worth a look although its inhabitants are somewhat strange and their customs oddish."}},

	{{"kazordoon"}, {"An inland town of dwarves, somewhere in the middle of nowhere."}},
	{{"dwarves"}, {"It's fun to see a seasoned dwarven fighter turnining into a shivering green something as soon as we get a mild breeze on sea."}},
	{{"dwarfs"}, {previous}},
	{{"ab'dendriel"}, {"My visits there were interesting and I learnt a lot about the elves and their city. I can only recommend a visit there and if it is only to admire the amazing architectural style in which the city was built."}},
	{{"elves"}, {"Elves are very special creatures. They keep in touch with nature almost like druids. Although I don't really understand their way of life, I think we could learn one or two things of them."}},
	{{"elfs"}, {previous}},
	{{"darama"}, {"I sailed around the whole continent once and I have seen many of its wonders. For sure there are more waiting to be discovered."}},

	{{"ferumbras"}, {"He is that for the land what giant sea serpents are for the sea."}},
	{{"excalibug"}, {"You better ask some knight about it."}},
	{{"apes"}, {"I would love to catch a living exemplar and bring it to Thais so the king could see it."}},
	{{"lizard"}, {"They have a small settlement in the southeast of the jungle next to the coast. It looks somewhat primitive but there is evidence it was erected only recently."}},
	{{"dworcs"}, {"They attacked us when we set our feet on the south shore of the continent. They are poison using savages, nothing more."}},

	{{"thais"}, {setPrice(160), "Do you seek a passage to Thais for %P gold?", setTopic(1)}},
	{{"darashia"}, {setPrice(180), "Do you seek a passage to Darashia for %P gold?", setTopic(2)}},
	{{"edron"}, {setPrice(150), "Do you seek a passage to Edron for %P gold?", setTopic(4)}},
	{{"venore"}, {setPrice(160), "Do you seek a passage to Venore for %P gold?", setTopic(5)}},
	{{"ankrahmun"}, {setPrice(110), "Do you seek a passage to Ankrahmun for %P gold?", setTopic(6)}},

	{{"thais", gt(questValue(250), 2)}, {setPrice(150), "Do you seek a passage to Thais for %P gold?", setTopic(1)}},
	{{"darashia", gt(questValue(250), 2)}, {setPrice(170), "Do you seek a passage to Darashia for %P gold?", setTopic(2)}},
	{{"edron", gt(questValue(250), 2)}, {setPrice(140), "Do you seek a passage to Edron for %P gold?", setTopic(4)}},
	{{"venore", gt(questValue(250), 2)}, {setPrice(150), "Do you seek a passage to Venore for %P gold?", setTopic(5)}},
	{{"ankrahmun", gt(questValue(250), 2)}, {setPrice(100), "Do you seek a passage to Ankrahmun for %P gold?", setTopic(6)}},

	{{gt(TOPIC, 0), lt(TOPIC, 8), "yes", PZBLOCK, FORCE}, {"First get rid of those blood stains! You are not going to ruin my vehicle!"}},

	--Topic=2,"yes",PZBlock,! -> "First get rid of those blood stains! You are not going to ruin my vehicle!"
	--Topic=4,"yes",PZBlock,! -> *
	--Topic=5,"yes",PZBlock,! -> *
	--Topic=6,"yes",PZBlock,! -> *
	--Topic=1,"yes",PZBlock,! -> *

	{{eq(TOPIC, 1), "yes", PREMIUM, ge(COUNTMONEY, PRICE)}, {"Set the sails!", deleteMoney, idle, effectOpp(11), teleport(32312, 32211, 6), effectOpp(11)}},
	{{eq(TOPIC, 2), "yes", PREMIUM, ge(COUNTMONEY, PRICE)}, {"Set the sails!", deleteMoney, idle, effectOpp(11), teleport(33290, 32481, 6), effectOpp(11)}},
	{{eq(TOPIC, 4), "yes", PREMIUM, ge(COUNTMONEY, PRICE)}, {"Set the sails!", deleteMoney, idle, effectOpp(11), teleport(33175, 31764, 6), effectOpp(11)}},
	{{eq(TOPIC, 5), "yes", PREMIUM, ge(COUNTMONEY, PRICE)}, {"Set the sails!", deleteMoney, idle, effectOpp(11), teleport(32954, 32023, 6), effectOpp(11)}},
	{{eq(TOPIC, 6), "yes", PREMIUM, ge(COUNTMONEY, PRICE)}, {"Set the sails!", deleteMoney, idle, effectOpp(11), teleport(33091, 32883, 6), effectOpp(11)}},
	{{gt(TOPIC, 0), "yes", ge(COUNTMONEY, PRICE)}, {"I'm sorry, but you need a premium account in order to travel on board of our ships."}},
	{{gt(TOPIC, 0), "yes"}, {"You don't have enough money."}},
	{{gt(TOPIC, 0)}, {"We would like to serve you some time."}},

	{{"passenger"}, {"We would like to welcome you on board."}},
	{{"trip"}, {"Where do you want to go? To Thais, Darashia, Venore, Ankrahmun or Edron?"}},
	{{"route"}, {previous}},
	{{"passage"}, {previous}},
	{{"town"}, {previous}},
	{{"destination"}, {previous}},
	{{"sail"}, {previous}},
	{{"go"}, {previous}},
}

function onCreatureSay(npc, creature, text, situation)
	behaviour.react(npc, rules, text, situation)
end

function onCreatureDisappear(npc, creature)
	behaviour.react(npc, rules, "", NPC_SITUATION_VANISH)
end
