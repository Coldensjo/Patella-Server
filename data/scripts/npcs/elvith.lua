-- npcs/elvith.lua: the behaviour of the NPC Elvith, converted from
-- elvith.npc by patella-convert. The NPC is its [[npc]] entry in npcs.toml;
-- lib/npc.lua matches and runs the rules below, in the order of the file
-- (scripting.hh, "NPCs"). The comments are those of elvith.npc.
--
-- GIMUD - Graphical Interface Multi User Dungeon
-- elvith.npc: Datenbank für den Musiker Elvith

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
local questValue = behaviour.questValue
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
	{{ADDRESS, "hello$", FORCE}, {"Ashari %N."}},
	{{ADDRESS, "hi$", FORCE}, {previous}},
	{{ADDRESS, "Ashari$", FORCE}, {previous}},
	{{ADDRESS, FORCE}, {idle}},
	{{BUSY, "hello$", FORCE}, {"Wait %N.", queue}},
	{{BUSY, "hi$", FORCE}, {previous}},
	{{BUSY, FORCE}, {nop}},
	{{VANISH, FORCE}, {"Asha Thrazi."}},

	{{"bye"}, {"Asha Thrazi.", idle}},
	{{"farewell"}, {previous}},
	{{"name"}, {"I am Elvith Rollingstone."}},
	{{"job"}, {"I sell musical instruments of many kinds."}},
	{{"time"}, {"Time has its own song. Close your eyes and listen to the symphony of the seasons."}},

	{{"carlin"}, {"Carlin is a city that thrives for a harmony it can never achive."}},
	{{"thais"}, {"I heared about Thais and id did not sound like a place I'd want to visit."}},
	{{"venore"}, {"By all what I heared this city is not only built into a swamp but its a swamp of intrigue and corruption itself."}},
	{{"roderick"}, {"This man trys too hard not to offend someone."}},
	{{"olrik"}, {"He appreciates my music and allthough he is loud and clumsy as all humans it seems not everything is lost."}},

	{{"music"}, {"Music is an attempt to condensate emotions in harmonies and save them for the times to come."}},
	{{"harmonies"}, {"Everything is a song. Life, death, history ... everything. To listen to the song of something is the first step to understand it."}},
	{{"melodies"}, {previous}},
	{{"harmony"}, {previous}},
	{{"melody"}, {previous}},
	{{"song"}, {previous}},
	{{"sing"}, {"Sorry, but there is a melody in my heart that wants to be born. I would loose it before by singing right now."}},
	{{"elf"}, {"We are the most graceful of all races. We feel the music of the universe in our hearts and souls."}},
	{{"elves"}, {previous}},
	{{"dwarf"}, {"They could at least use their picks and hammers with more rythm."}},
	{{"human"}, {"They are too loud and don't even understand the concept of a melody."}},
	{{"troll"}, {"I went down to the mines and tried to lighten up their spirit, the foolish creatures did not listen to my songs, though."}},
	{{"cenath"}, {"The Cenath think they know the 'art' but the only true art is the music."}},
	{{"kuridai"}, {"They could dig some halls for a big musical event, but they won't listen to me about that matter."}},
	{{"deraisim"}, {"The other deraisim are too much concerned with mastering the nature so they don't listen to its music anymore."}},
	{{"abdaisim"}, {"The wanderers have no patience. You need patience and passion to create and to enjoy music."}},
	{{"teshial"}, {"I bet they were great musicians."}},
	{{"ferumbras"}, {"Only humans made songs about him and his evil deeds."}},
	{{"crunor"}, {"That is some god the humans worship. Our pople are not interested in this gods anymore."}},
	{{"excalibug"}, {"There are too many songs about that weapon to retell them all. Most of them are human and therefore quite crude anyways."}},
	{{"spell"}, {"Sorry, I don't feel like teaching magic today."}},
	{{"magic"}, {previous}},

	{{"elven", "poetry", eq(questValue(311), 1), eq(questValue(312), 0)}, {"The last issue I had was bought by Randor Swiftfinger. He was banished through the hellgate and probably took the book with him ...", "I would not recommend to seek him or the book there but of course its possible."}},
	{{"song", "forest", eq(questValue(311), 1)}, {previous}},

	{{"elven", "poetry", eq(questValue(311), 1), eq(questValue(312), 1)}, {setType(4844), setAmount(1), setPrice(500), "By luck I aquired another copy of the book you are looking for. Do you want to buy a copy of 'songs of the forest' for 500 gold?.", setTopic(2)}},
	{{"song", "forest", eq(questValue(311), 1)}, {previous}},
	{{eq(TOPIC, 2), "yes", ge(COUNTMONEY, PRICE)}, {"Thank you. Here it is.", deleteMoney, create(TYPE)}},
	{{eq(TOPIC, 2), "yes"}, {"Sorry, you do not have enough gold."}},
	{{eq(TOPIC, 2)}, {"Maybe you will buy it another time."}},

	{{"elven", "poetry"}, {"Sorry, I have no issue of this book left."}},
	{{"song", "forest"}, {previous}},

	{{"hellsgate"}, {"For the worst of crimes the criminals are cast into hellgate. Its said noone can return from there. Since it is not forbidden to enter hellgate you might convince Elathriel to grant you entrance."}},
	{{"elathriel"}, {"He is a kuridai and the local Az'irel. Something like the head of the human townsguards."}},

	{{"offer"}, {"I sell lyres, lutes, drums, and simple fanfares."}},
	{{"goods"}, {previous}},
	{{"buy"}, {previous}},
	{{"do", "you", "sell"}, {previous}},
	{{"do", "you", "have"}, {previous}},
	{{"instrument"}, {previous}},

	{{"lyre"}, {setType(2949), setAmount(1), setPrice(120), "Do you want to buy a lyre for %P gold?", setTopic(1)}},
	{{"lute"}, {setType(2950), setAmount(1), setPrice(195), "Do you want to buy a lute for %P gold?", setTopic(1)}},
	{{"drum"}, {setType(2952), setAmount(1), setPrice(140), "Do you want to buy a drum for %P gold?", setTopic(1)}},
	{{"simple", "fanfare"}, {setType(2954), setAmount(1), setPrice(150), "Do you want to buy a simple fanfare for %P gold?", setTopic(1)}},

	{{P1, lt(1, P1), "lyre"}, {setType(2949), setAmount(P1), setPrice(mul(120, P1)), "Do you want to buy %A lyre for %P gold?", setTopic(1)}},
	{{P1, lt(1, P1), "lute"}, {setType(2950), setAmount(P1), setPrice(mul(195, P1)), "Do you want to buy %A lute for %P gold?", setTopic(1)}},
	{{P1, lt(1, P1), "drum"}, {setType(2952), setAmount(P1), setPrice(mul(140, P1)), "Do you want to buy %A drum for %P gold?", setTopic(1)}},
	{{P1, lt(1, P1), "simple", "fanfare"}, {setType(2954), setAmount(P1), setPrice(mul(150, P1)), "Do you want to buy %A simple fanfare for %P gold?", setTopic(1)}},

	{{eq(TOPIC, 1), "yes", ge(COUNTMONEY, PRICE)}, {"Thank you. Here it is.", deleteMoney, create(TYPE)}},
	{{eq(TOPIC, 1), "yes"}, {"Sorry, you do not have enough gold."}},
	{{eq(TOPIC, 1)}, {"Maybe you will buy it another time."}},
}

function onCreatureSay(npc, creature, text, situation)
	behaviour.react(npc, rules, text, situation)
end

function onCreatureDisappear(npc, creature)
	behaviour.react(npc, rules, "", NPC_SITUATION_VANISH)
end
