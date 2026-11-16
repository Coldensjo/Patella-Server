-- npcs/ahmet.lua: the behaviour of the NPC Ahmet, converted from
-- ahmet.npc by patella-convert. The NPC is its [[npc]] entry in npcs.toml;
-- lib/npc.lua matches and runs the rules below, in the order of the file
-- (scripting.hh, "NPCs"). The comments are those of ahmet.npc.
--
-- GIMUD - Graphical Interface Multi User Dungeon
-- ahmet.npc: Datenbank für den pyramidenhändler Ahmet

local behaviour = require("lib.npc")
local ADDRESS = behaviour.ADDRESS
local BUSY = behaviour.BUSY
local VANISH = behaviour.VANISH
local FORCE = behaviour.FORCE
local P1 = behaviour.P1
local eq = behaviour.eq
local lt = behaviour.lt
local gt = behaviour.gt
local ge = behaviour.ge
local TOPIC = behaviour.TOPIC
local PRICE = behaviour.PRICE
local AMOUNT = behaviour.AMOUNT
local TYPE = behaviour.TYPE
local COUNTMONEY = behaviour.COUNTMONEY
local count = behaviour.count
local mul = behaviour.mul
local setTopic = behaviour.setTopic
local setPrice = behaviour.setPrice
local setAmount = behaviour.setAmount
local setType = behaviour.setType
local setData = behaviour.setData
local create = behaviour.create
local delete = behaviour.delete
local createMoney = behaviour.createMoney
local deleteMoney = behaviour.deleteMoney
local queue = behaviour.queue
local idle = behaviour.idle
local nop = behaviour.nop
local previous = behaviour.previous

local rules = behaviour.rules {
	{{ADDRESS, "hello$", FORCE}, {"Be mourned pilgrim in flesh."}},
	{{ADDRESS, "hi$", FORCE}, {"Be mourned pilgrim in flesh."}},
	{{ADDRESS, "be", "mourned", "pilgrim", "flesh", FORCE}, {"Be mourned pilgrim in flesh."}},
	{{ADDRESS, FORCE}, {idle}},

	{{BUSY, "hello$", FORCE}, {"Mourned %N, know that I am busy.", queue}},
	{{BUSY, "hi$", FORCE}, {previous}},
	{{BUSY, "be", "mourned", "pilgrim", "flesh", FORCE}, {previous}},
	{{BUSY, FORCE}, {nop}},
	{{VANISH, FORCE}, {"May enlightenment be your path."}},

	{{"bye"}, {"May enlightenment be your path.", idle}},
	{{"farewell"}, {previous}},
	{{"job"}, {"I sell equipment of all sorts."}},
	{{"name"}, {"I am the mourned Ahmet."}},
	{{"time"}, {"It were foolish of me to tell you the time, because then you won't buy a watch."}},
	{{"temple"}, {"The temple is a school for us mourned mortals. The teachings of the temple help us to find our way through our mortal days."}},
	{{"pharaoh"}, {"Blessed be the pharaoh. He is our saviour. I hope that one day I will be chosen."}},
	{{"arkhothep"}, {previous}},
	{{"oldpharaoh"}, {"The foolish old pharaoh withheld knowledge and power from his son, knowing that he would surpass him in every aspects. But his son granted him the chance to ascend."}},
	{{"ashmunrah"}, {previous}},
	{{"scarab"}, {"The eternal burrowers are the keepers of all the secrets their kind has unearthed in countless aeons."}},
	{{"chosen"}, {"Only the most worthy and pious are chosen to join the armies of the pharaoh. In undeath they follow the path of ascension."}},

	{{"tibia"}, {"The world is nothing but a sigil of death, a monument of decay. We have to attune to death to become one with the world."}},

	{{"carlin"}, {"The vain cities of the Tibian continent think they are at the centre of the universe. Little do they know about the wisdom of the pharaoh."}},
	{{"thais"}, {previous}},
	{{"edron"}, {previous}},
	{{"venore"}, {previous}},
	{{"kazordoon"}, {"We rarely see a traveler of the small folk here."}},
	{{"dwarves"}, {previous}},
	{{"dwarfes"}, {previous}},
	{{"ab'dendriel"}, {"Elves are a rare sight in our lands."}},
	{{"elves"}, {previous}},
	{{"elfes"}, {previous}},
	{{"darama"}, {"Life here is harsh, but only this way can we deny the temptations that might damage our Rah and our Uthun to our traitorous flesh."}},
	{{"darashia"}, {"The foolishness of their ways will eventually spell their doom."}},
	{{"daraman"}, {"He was close to the truth, but he lacked the wisdom and vision of our pharaoh."}},
	{{"ankrahmun"}, {"Our city will endure the sands of the desert and the grinding teeth of time."}},

	{{"pharaoh"}, {"Our pharaoh holds the key to our ascension. Praised be our pharaoh."}},
	{{"mortality"}, {"Mortality is our curse. Mourned shall we be."}},
	{{"false", "gods"}, {"The great traitors are trying to doom us."}},

	{{"ascension"}, {"Godhood is at our disposal if only we throw of the shackles of mortal flesh."}},
	{{"Akh'rah", "Uthun"}, {"The Akh'rah Uthun is what we are."}},
	{{"Akh"}, {"Your cursed bodys are called the Akh. As long as we are alive the Akh makes us weak and vulnerable."}},

	{{"undead"}, {"Undeath is the path to ascension which the chosen may follow."}},
	{{"undeath"}, {previous}},
	{{"Rah"}, {"The Rah could be called our soul."}},
	{{"uthun"}, {"The Uthun is that what we learn and remember."}},
	{{"mourn"}, {"We are mortals and thus miserable creatures that are to be mourned."}},

	{{"arena"}, {"The arena is east of here."}},
	{{"palace"}, {"The palace is the home of our beloved pharaoh."}},

	{{"torch"}, {setType(2920), setAmount(1), setPrice(2), "Do you want to buy a torch for %P gold?", setTopic(1)}},
	{{"candelabr"}, {setType(2911), setAmount(1), setPrice(8), "Do you want to buy a candelabrum for %P gold?", setTopic(1)}},
	{{"candlestick"}, {setType(2917), setAmount(1), setPrice(2), "Do you want to buy a candlestick for %P gold?", setTopic(1)}},
	{{"bag"}, {setType(2863), setAmount(1), setPrice(4), "Do you want to buy a bag for %P gold?", setTopic(1)}},
	{{"scroll"}, {setType(2815), setAmount(1), setPrice(5), "Do you want to buy a scroll for %P gold?", setTopic(1)}},
	{{"document"}, {setType(2834), setAmount(1), setPrice(12), "Do you want to buy a document for %P gold?", setTopic(1)}},
	{{"parchment"}, {setType(2835), setAmount(1), setPrice(8), "Do you want to buy a parchment for %P gold?", setTopic(1)}},
	{{"shovel"}, {setType(3457), setAmount(1), setPrice(10), "Do you want to buy a shovel for %P gold?", setTopic(1)}},
	{{"backpack"}, {setType(2871), setAmount(1), setPrice(10), "Do you want to buy a backpack for %P gold?", setTopic(1)}},
	{{"scythe"}, {setType(3453), setAmount(1), setPrice(50), "Do you want to buy a scythe for %P gold?", setTopic(1)}},
	{{"pick"}, {setType(3456), setAmount(1), setPrice(50), "Do you want to buy a pick for %P gold?", setTopic(1)}},
	{{"watch"}, {setType(2906), setAmount(1), setPrice(20), "Do you want to buy one of my high quality watches for %P gold?", setTopic(1)}},
	{{"rope"}, {setType(3003), setAmount(1), setPrice(50), "Do you want to buy a rope for %P gold?", setTopic(1)}},
	{{"rod"}, {setType(3483), setAmount(1), setPrice(150), "Do you want to buy a fishing rod for %P gold?", setTopic(1)}},
	{{"oil"}, {setType(2874), setData(7), setAmount(1), setPrice(20), "Do you want to buy oil for %P gold?", setTopic(1)}},
	{{"crowbar"}, {setType(3304), setAmount(1), setPrice(260), "Do you want to buy a crowbar for %P gold?", setTopic(1)}},
	{{"water", "hose"}, {setType(2901), setAmount(1), setPrice(40), setData(1), "Do you want to buy a water hose for %P gold?", setTopic(1)}},
	{{"present"}, {setType(2856), setAmount(1), setPrice(10), "Do you want to buy a present for %P gold?", setTopic(1)}},
	{{"bucket"}, {setType(2873), setAmount(1), setPrice(4), "Do you want to buy a bucket for %P gold?", setTopic(1)}},
	{{"bottle"}, {setType(2875), setAmount(1), setPrice(3), "Do you want to buy a bottle for %P gold?", setTopic(1)}},
	{{"cup"}, {setType(2881), setAmount(1), setPrice(2), "Do you want to buy a cup for %P gold?", setTopic(1)}},
	{{"plate"}, {setType(2905), setAmount(1), setPrice(6), "Do you want to buy a plate for %P gold?", setTopic(1)}},

	{{"worm"}, {"I sell worms only in sixpacks for 5 gold each, how many sixpacks of worms do you want to buy?"}},
	{{"worms"}, {previous}},
	{{P1, "worms"}, {setType(3492), setAmount(mul(P1, 6)), setPrice(mul(5, P1)), "So you want to buy some of my sixpacks with altogether %A worms for %P gold?", setTopic(1)}},
	{{P1, "six", "pack"}, {previous}},

	{{P1, lt(1, P1), "torch"}, {setType(2920), setAmount(P1), setPrice(mul(2, P1)), "Do you want to buy %A torches for %P gold?", setTopic(1)}},
	{{P1, lt(1, P1), "candelabr"}, {setType(2911), setAmount(P1), setPrice(mul(8, P1)), "Do you want to buy %A candelabra for %P gold?", setTopic(1)}},
	{{P1, lt(1, P1), "candlestick"}, {setType(2917), setAmount(P1), setPrice(mul(2, P1)), "Do you want to buy %A candlesticks for %P gold?", setTopic(1)}},
	{{P1, lt(1, P1), "bag"}, {setType(2863), setAmount(P1), setPrice(mul(4, P1)), "Do you want to buy %A bags for %P gold?", setTopic(1)}},
	{{P1, lt(1, P1), "scroll"}, {setType(2815), setAmount(P1), setPrice(mul(5, P1)), "Do you want to buy %A scrolls for %P gold?", setTopic(1)}},
	{{P1, lt(1, P1), "document"}, {setType(2834), setAmount(P1), setPrice(mul(12, P1)), "Do you want to buy %A documents for %P gold?", setTopic(1)}},
	{{P1, lt(1, P1), "parchment"}, {setType(2835), setAmount(P1), setPrice(mul(8, P1)), "Do you want to buy %A parchments for %P gold?", setTopic(1)}},
	{{P1, lt(1, P1), "shovel"}, {setType(3457), setAmount(P1), setPrice(mul(10, P1)), "Do you want to buy %A shovels for %P gold?", setTopic(1)}},
	{{P1, lt(1, P1), "backpack"}, {setType(2871), setAmount(P1), setPrice(mul(10, P1)), "Do you want to buy %A backpacks for %P gold?", setTopic(1)}},
	{{P1, lt(1, P1), "scythe"}, {setType(3453), setAmount(P1), setPrice(mul(50, P1)), "Do you want to buy %A scythes for %P gold?", setTopic(1)}},
	{{P1, lt(1, P1), "pick"}, {setType(3456), setAmount(P1), setPrice(mul(50, P1)), "Do you want to buy %A picks for %P gold?", setTopic(1)}},
	{{P1, lt(1, P1), "watch"}, {setType(2906), setAmount(P1), setPrice(mul(20, P1)), "Do you want to buy %A of my high quality watches for %P gold?", setTopic(1)}},
	{{P1, lt(1, P1), "rope"}, {setType(3003), setAmount(P1), setPrice(mul(50, P1)), "Do you want to buy %A ropes for %P gold?", setTopic(1)}},
	{{P1, lt(1, P1), "rod"}, {setType(3483), setAmount(P1), setPrice(mul(150, P1)), "Do you want to buy %A fishing rods for %P gold?", setTopic(1)}},
	{{P1, lt(1, P1), "oil"}, {setType(2874), setData(7), setAmount(P1), setPrice(mul(20, P1)), "Do you want to buy %A vials of oil for %P gold?", setTopic(1)}},
	{{P1, lt(1, P1), "crowbar"}, {setType(3304), setAmount(P1), setPrice(mul(260, P1)), "Do you want to buy %A crowbars for %P gold?", setTopic(1)}},
	{{P1, lt(1, P1), "water", "hose"}, {setType(2901), setAmount(P1), setPrice(mul(40, P1)), setData(1), "Do you want to buy %A water hoses for %P gold?", setTopic(1)}},
	{{P1, lt(1, P1), "present"}, {setType(2856), setAmount(P1), setPrice(mul(10, P1)), "Do you want to buy %A presents for %P gold?", setTopic(1)}},
	{{P1, lt(1, P1), "bucket"}, {setType(2873), setAmount(P1), setPrice(mul(4, P1)), "Do you want to buy %A buckets for %P gold?", setTopic(1)}},
	{{P1, lt(1, P1), "bottle"}, {setType(2875), setAmount(P1), setPrice(mul(3, P1)), "Do you want to buy %A bottles for %P gold?", setTopic(1)}},
	{{P1, lt(1, P1), "cup"}, {setType(2881), setAmount(P1), setPrice(mul(2, P1)), "Do you want to buy %A cups for %P gold?", setTopic(1)}},
	{{P1, lt(1, P1), "plate"}, {setType(2905), setAmount(P1), setPrice(mul(6, P1)), "Do you want to buy %A plates for %P gold?", setTopic(1)}},

	{{"sell", "rope"}, {setType(3003), setAmount(1), setPrice(8), "Do you want to sell a rope for %P gold?", setTopic(3)}},
	{{"sell", P1, lt(1, P1), "rope"}, {setType(3003), setAmount(P1), setPrice(mul(8, P1)), "Do you want to sell %A ropes for %P gold?", setTopic(3)}},

	{{eq(TOPIC, 1), "yes", ge(COUNTMONEY, PRICE)}, {"I hope it will serve you well, my prized customer.", deleteMoney, create(TYPE)}},
	{{eq(TOPIC, 1), "yes"}, {"My twelve starving children don't allow me to sell it for less, o grandmaster of haggling."}},
	{{eq(TOPIC, 1)}, {"What a pity."}},

	{{eq(TOPIC, 3), "yes", ge(count(TYPE), AMOUNT)}, {"I hardly can explain to my wife why I gave you that much money.", delete(TYPE), createMoney}},
	{{eq(TOPIC, 3), "yes"}, {"Sorry, you own none."}},
	{{eq(TOPIC, 3), "yes", gt(AMOUNT, 1)}, {"Sorry, you do not have so many."}},
	{{eq(TOPIC, 3)}, {"Maybe next time."}},

	{{"equipment"}, {"I sell shovels, picks, scythes, bags, ropes, backpacks, plates, cups, scrolls, documents, parchments, lightsources and watches. Of course, I sell fishing rods and six-packs of worms, too."}},
	{{"goods"}, {previous}},
	{{"buy"}, {previous}},
	{{"do", "you", "sell"}, {previous}},
	{{"do", "you", "have"}, {previous}},
	{{"offer"}, {previous}},
	{{"light"}, {"I sell torches, candlesticks, candelabra, and oil."}},
}

function onCreatureSay(npc, creature, text, situation)
	behaviour.react(npc, rules, text, situation)
end

function onCreatureDisappear(npc, creature)
	behaviour.react(npc, rules, "", NPC_SITUATION_VANISH)
end
