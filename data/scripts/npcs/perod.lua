-- npcs/perod.lua: the behaviour of the NPC Perod, converted from
-- perod.npc by patella-convert. The NPC is its [[npc]] entry in npcs.toml;
-- lib/npc.lua matches and runs the rules below, in the order of the file
-- (scripting.hh, "NPCs"). The comments are those of perod.npc.
--
-- GIMUD - Graphical Interface Multi User Dungeon
-- perod.npc: Datenbank für den Händler Perod

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
	{{ADDRESS, "hello$", FORCE}, {"Greetings, dear %N. What do you need?"}},
	{{ADDRESS, "hi$", FORCE}, {previous}},
	{{ADDRESS, FORCE}, {idle}},
	{{BUSY, "hello$", FORCE}, {"Please wait a moment, %N. I am already talking to someone.", queue}},
	{{BUSY, "hi$", FORCE}, {previous}},
	{{BUSY, FORCE}, {nop}},
	{{VANISH, FORCE}, {"Good bye."}},

	{{"bye"}, {"Good bye.", idle}},
	{{"farewell"}, {previous}},
	{{"job"}, {"I sell general goods which are, if I am allowed to say that, crucial when you explore the jungle."}},
	{{"name"}, {"I am Perod, how could you forget that, %N? We fought back-to-back in those troll caves on Rookgard a long time ago."}},
	{{"time"}, {"I won't tell you, but you can buy one of my quality watches to find out."}},
	{{"king"}, {"The king is far away and yet we are still his subjects. Strange, isn't it?"}},
	{{"venore"}, {"After I had left Thais I found a new home in Venore and I never regreted it."}},
	{{"thais"}, {"Thais lacked any prospect for a change. I quickly figured out that Venore is the place to be."}},
	{{"carlin"}, {"Carlin is a dull city with strange habits."}},
	{{"edron"}, {"I lived in Edron as a treasure hunter for a while, but then the place became too crowded."}},
	{{"jungle"}, {"The jungle is full of adventures and secrets that wait to be explored. Some years ago I would have surely enjoyed that. ...", "But now that I setteled down here, I don't feel excited anymore by the thought of exploring an inhospitable forest full of animals that want to kill me."}},

	{{"tibia"}, {"I have travelled a lot and still I have not seen everything. So I abandoned my life as an explorer and became an employee of a trading company domiciled in Venore."}},

	{{"kazordoon"}, {"The hidden city of the dwarves can be quite confusing for a newcomer. I got lost there a dozen times before I became familiar with that city."}},
	{{"dwarves"}, {"Some dwarves live in the city, you'll find them in the tavern."}},
	{{"dwarfs"}, {previous}},
	{{"ab'dendriel"}, {"The elves of Ab'Dendriel built a city seemingly out of trees. I wonder how they can stand the winter in those odd houses."}},
	{{"elves"}, {previous}},
	{{"elfs"}, {previous}},
	{{"darama"}, {"A new continent means new chances. But my days as an adventurer are over. My new chances lie in trade and commerce."}},
	{{"darashia"}, {"My trips there were very short, I don't like the desert and I did not like that town."}},
	{{"ankrahmun"}, {"It's one of the few cities I have never visited and no one will ever get me even close to that city of undeads and mummies."}},
	{{"ferumbras"}, {"During my days as an adventurer, I was thrilled by the thought to fight him. Looking back I must say it is better that I have never met him."}},
	{{"excalibug"}, {"Oh boy, how long have we searched for that weapon. I still wonder sometimes where it might be hidden, but I have no clue."}},
	{{"apes"}, {"Their occasional raids give me a chance to train my fighting skills."}},
	{{"lizard"}, {"I bet those lizards hide some ancient treasures in their settlements."}},
	{{"dworcs"}, {"It's the jungle variant of the orcs. I guess no matter where you go, there is always some orc waiting behind some bush, ready to thrust his blade in your body."}},

	{{"offer"}, {"I offer fishing rods, sixpacks of worms, shovels, picks, scythes, bags, ropes, backpacks, plates, jugs, mugs, cups, bottles, buckets, scrolls, documents, parchments, footballs, watches, books, torches, machetes, presents and ammunition."}},
	{{"goods"}, {previous}},
	{{"do", "you", "sell"}, {previous}},
	{{"do", "you", "have"}, {previous}},
	{{"equipment"}, {previous}},

	{{"magic"}, {"Ask somwhere else in the market."}},
	{{"fluid"}, {previous}},

	{{"book"}, {"I offer different kind of books: brown, black and small books. Which book do you want?"}},

	{{"torch"}, {setType(2920), setAmount(1), setPrice(2), "Do you want to buy a torch for %P gold?", setTopic(1)}},
	{{"bag"}, {setType(2864), setAmount(1), setPrice(5), "Do you want to buy a bag for %P gold?", setTopic(1)}},
	{{"backpack"}, {setType(2872), setAmount(1), setPrice(20), "Do you want to buy a backpack for %P gold?", setTopic(1)}},
	{{"present"}, {setType(2856), setAmount(1), setPrice(10), "Do you want to buy a present for %P gold?", setTopic(1)}},
	{{"scroll"}, {setType(2815), setAmount(1), setPrice(5), "Do you want to buy a scroll for %P gold?", setTopic(1)}},
	{{"document"}, {setType(2834), setAmount(1), setPrice(12), "Do you want to buy a document for %P gold?", setTopic(1)}},
	{{"parchment"}, {setType(2835), setAmount(1), setPrice(8), "Do you want to buy a parchment for %P gold?", setTopic(1)}},
	{{"brown", "book"}, {setType(2837), setAmount(1), setPrice(15), "Do you want to buy a brown book for %P gold?", setTopic(1)}},
	{{"black", "book"}, {setType(2838), setAmount(1), setPrice(15), "Do you want to buy a black book for %P gold?", setTopic(1)}},
	{{"small", "book"}, {setType(2839), setAmount(1), setPrice(15), "Do you want to buy a small book for %P gold?", setTopic(1)}},
	{{"bucket"}, {setType(2873), setAmount(1), setPrice(4), "Do you want to buy a bucket for %P gold?", setTopic(1)}},
	{{"bottle"}, {setType(2875), setAmount(1), setPrice(3), "Do you want to buy a bottle for %P gold?", setTopic(1)}},
	{{"mug"}, {setType(2880), setAmount(1), setPrice(4), "Do you want to buy a mug for %P gold?", setTopic(1)}},
	{{"cup"}, {setType(2881), setAmount(1), setPrice(2), "Do you want to buy a cup for %P gold?", setTopic(1)}},
	{{"jug"}, {setType(2882), setAmount(1), setPrice(10), "Do you want to buy a jug for %P gold?", setTopic(1)}},
	{{"plate"}, {setType(2905), setAmount(1), setPrice(6), "Do you want to buy a plate for %P gold?", setTopic(1)}},
	{{"watch"}, {setType(2906), setAmount(1), setPrice(20), "Do you want to buy one of my high quality watches for %P gold?", setTopic(1)}},
	{{"football"}, {setType(2990), setAmount(1), setPrice(111), "Do you want to buy a football for %P gold?", setTopic(1)}},
	{{"rope"}, {setType(3003), setAmount(1), setPrice(50), "Do you want to buy a rope for %P gold?", setTopic(1)}},
	{{"machete"}, {setType(3308), setAmount(1), setPrice(40), "Do you want to buy a machete for %P gold?", setTopic(1)}},
	{{"scythe"}, {setType(3453), setAmount(1), setPrice(50), "Do you want to buy a scythe for %P gold?", setTopic(1)}},
	{{"pick"}, {setType(3456), setAmount(1), setPrice(50), "Do you want to buy a pick for %P gold?", setTopic(1)}},
	{{"shovel"}, {setType(3457), setAmount(1), setPrice(50), "Do you want to buy a shovel for %P gold?", setTopic(1)}},
	{{"rod"}, {setType(3483), setAmount(1), setPrice(150), "Do you want to buy a fishing rod for %P gold?", setTopic(1)}},

	{{P1, lt(1, P1), "torch"}, {setType(2920), setAmount(P1), setPrice(mul(2, P1)), "Do you want to buy %A torches for %P gold?", setTopic(1)}},
	{{P1, lt(1, P1), "bag"}, {setType(2864), setAmount(P1), setPrice(mul(5, P1)), "Do you want to buy %A bags for %P gold?", setTopic(1)}},
	{{P1, lt(1, P1), "backpack"}, {setType(2872), setAmount(P1), setPrice(mul(20, P1)), "Do you want to buy %A backpacks for %P gold?", setTopic(1)}},
	{{P1, lt(1, P1), "present"}, {setType(2856), setAmount(P1), setPrice(mul(10, P1)), "Do you want to buy %A presents for %P gold?", setTopic(1)}},
	{{P1, lt(1, P1), "scroll"}, {setType(2815), setAmount(P1), setPrice(mul(5, P1)), "Do you want to buy %A scrolls for %P gold?", setTopic(1)}},
	{{P1, lt(1, P1), "document"}, {setType(2834), setAmount(P1), setPrice(mul(12, P1)), "Do you want to buy %A documents for %P gold?", setTopic(1)}},
	{{P1, lt(1, P1), "parchment"}, {setType(2835), setAmount(P1), setPrice(mul(8, P1)), "Do you want to buy %A parchments for %P gold?", setTopic(1)}},
	{{P1, lt(1, P1), "brown", "book"}, {setType(2837), setAmount(P1), setPrice(mul(15, P1)), "Do you want to buy %A brown books for %P gold?", setTopic(1)}},
	{{P1, lt(1, P1), "black", "book"}, {setType(2838), setAmount(P1), setPrice(mul(15, P1)), "Do you want to buy %A black books for %P gold?", setTopic(1)}},
	{{P1, lt(1, P1), "small", "book"}, {setType(2839), setAmount(P1), setPrice(mul(15, P1)), "Do you want to buy %A small books for %P gold?", setTopic(1)}},
	{{P1, lt(1, P1), "bucket"}, {setType(2873), setAmount(P1), setPrice(mul(4, P1)), "Do you want to buy %A buckets for %P gold?", setTopic(1)}},
	{{P1, lt(1, P1), "bottle"}, {setType(2875), setAmount(P1), setPrice(mul(3, P1)), "Do you want to buy %A bottles for %P gold?", setTopic(1)}},
	{{P1, lt(1, P1), "mug"}, {setType(2880), setAmount(P1), setPrice(mul(4, P1)), "Do you want to buy %A mugs for %P gold?", setTopic(1)}},
	{{P1, lt(1, P1), "cup"}, {setType(2881), setAmount(P1), setPrice(mul(2, P1)), "Do you want to buy %A cups for %P gold?", setTopic(1)}},
	{{P1, lt(1, P1), "jug"}, {setType(2882), setAmount(P1), setPrice(mul(10, P1)), "Do you want to buy %A jugs for %P gold?", setTopic(1)}},
	{{P1, lt(1, P1), "plate"}, {setType(2905), setAmount(P1), setPrice(mul(6, P1)), "Do you want to buy %A plates for %P gold?", setTopic(1)}},
	{{P1, lt(1, P1), "watch"}, {setType(2906), setAmount(P1), setPrice(mul(20, P1)), "Do you want to buy %A of my high quality watches for %P gold?", setTopic(1)}},
	{{P1, lt(1, P1), "football"}, {setType(2990), setAmount(P1), setPrice(mul(111, P1)), "Do you want to buy %A footballs for %P gold?", setTopic(1)}},
	{{P1, lt(1, P1), "rope"}, {setType(3003), setAmount(P1), setPrice(mul(50, P1)), "Do you want to buy %A ropes for %P gold?", setTopic(1)}},
	{{P1, lt(1, P1), "machete"}, {setType(3308), setAmount(P1), setPrice(mul(40, P1)), "Do you want to buy %A machetes for %P gold?", setTopic(1)}},
	{{P1, lt(1, P1), "scythe"}, {setType(3453), setAmount(P1), setPrice(mul(50, P1)), "Do you want to buy %A scythes for %P gold?", setTopic(1)}},
	{{P1, lt(1, P1), "pick"}, {setType(3456), setAmount(P1), setPrice(mul(50, P1)), "Do you want to buy %A picks for %P gold?", setTopic(1)}},
	{{P1, lt(1, P1), "shovel"}, {setType(3457), setAmount(P1), setPrice(mul(50, P1)), "Do you want to buy %A shovels for %P gold?", setTopic(1)}},
	{{P1, lt(1, P1), "rod"}, {setType(3483), setAmount(P1), setPrice(mul(150, P1)), "Do you want to buy %A fishing rods for %P gold?", setTopic(1)}},

	{{"worm"}, {"I sell worms only in sixpacks for 5 gold each, how many sixpacks of worms would you like to buy?"}},
	{{"worms"}, {previous}},
	{{P1, "worms"}, {setType(3492), setAmount(mul(P1, 6)), setPrice(mul(5, P1)), "So you want to buy some of my sixpacks with altogether %A worms for %P gold?", setTopic(1)}},
	{{P1, "six", "pack"}, {previous}},

	{{eq(TOPIC, 1), "yes", ge(COUNTMONEY, PRICE)}, {"Here you are.", deleteMoney, create(TYPE)}},
	{{eq(TOPIC, 1), "yes"}, {"Come back, when you have enough money."}},
	{{eq(TOPIC, 1)}, {"Hmm, but next time."}},

	{{"ammo"}, {"Do you need arrows for a bow, or bolts for a crossbow?"}},
	{{"ammunition"}, {previous}},

	{{"sell", "bow"}, {"I don't buy used bows."}},
	{{"sell", "crossbow"}, {"I don't buy used crossbows."}},

	{{"bow"}, {setType(3350), setAmount(1), setPrice(400), "Do you want to buy a bow for %P gold?", setTopic(2)}},
	{{"crossbow"}, {setType(3349), setAmount(1), setPrice(500), "Do you want to buy a crossbow for %P gold?", setTopic(2)}},
	{{"arrow"}, {setType(3447), setAmount(1), setPrice(2), "Do you want to buy an arrow for %P gold?", setTopic(2)}},
	{{"bolt"}, {setType(3446), setAmount(1), setPrice(3), "Do you want to buy a bolt for %P gold?", setTopic(2)}},

	{{P1, lt(1, P1), "bow"}, {setType(3350), setAmount(P1), setPrice(mul(400, P1)), "Do you want to buy %A bows for %P gold?", setTopic(2)}},
	{{P1, lt(1, P1), "crossbow"}, {setType(3349), setAmount(P1), setPrice(mul(500, P1)), "Do you want to buy %A crossbows for %P gold?", setTopic(2)}},
	{{P1, lt(1, P1), "arrow"}, {setType(3447), setAmount(P1), setPrice(mul(2, P1)), "Do you want to buy %A arrows for %P gold?", setTopic(2)}},
	{{P1, lt(1, P1), "bolt"}, {setType(3446), setAmount(P1), setPrice(mul(3, P1)), "Do you want to buy %A bolts for %P gold?", setTopic(2)}},

	{{eq(TOPIC, 2), "yes", ge(COUNTMONEY, PRICE)}, {"Here you are.", deleteMoney, create(TYPE)}},
	{{eq(TOPIC, 2), "yes"}, {"Come back when you have enough money."}},
	{{eq(TOPIC, 2)}, {"Hmm, but next time."}},
}

function onCreatureSay(npc, creature, text, situation)
	behaviour.react(npc, rules, text, situation)
end

function onCreatureDisappear(npc, creature)
	behaviour.react(npc, rules, "", NPC_SITUATION_VANISH)
end
