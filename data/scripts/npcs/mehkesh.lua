-- npcs/mehkesh.lua: the behaviour of the NPC Mehkesh, converted from
-- mehkesh.npc by patella-convert. The NPC is its [[npc]] entry in npcs.toml;
-- lib/npc.lua matches and runs the rules below, in the order of the file
-- (scripting.hh, "NPCs"). The comments are those of mehkesh.npc.
--
-- GIMUD - Graphical Interface Multi User Dungeon
-- mehkesh.npc: Datenbank für den pyramidenhändler mehkesh

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
	{{"job"}, {"I am a trader. I sell potions brewed by the foremost alchemists of the land."}},
	{{"name"}, {"I am the mourned Mehkesh."}},
	{{"time"}, {"Time is but one of the hardships our mortal flesh has to endure."}},
	{{"temple"}, {"The temple spreads the word of our wise pharaoh."}},
	{{"pharaoh"}, {"The pharaoh alone has achieved godhood. But in his infinite mercy he chose to stay with his people to offer them guidance."}},
	{{"arkhothep"}, {previous}},
	{{"ashmunrah"}, {"Even though his merciful son offered him undeath I doubt the old pharaoh will ever find his way to ascension."}},
	{{"scarab"}, {"The scarabs are more then just enormous insects. They are keepers of ancient secrets."}},
	{{"chosen"}, {"If we serve the pharaoh during our life time he might allow us to serve him in undeath. Only then can truly start our search for ascension."}},
	{{"tibia"}, {"One day our world will be freed from the false gods and accept the guidance of our pharaoh."}},
	{{"carlin"}, {"Those citys are only pawns of the false gods and their misguided priests."}},
	{{"thais"}, {previous}},
	{{"edron"}, {previous}},
	{{"venore"}, {previous}},
	{{"kazordoon"}, {"The dwarves should know better then to praise the mortal essence of the elements."}},
	{{"dwarves"}, {previous}},
	{{"dwarfes"}, {previous}},
	{{"ab'dendriel"}, {"Elves are foolish and their obsession with life damns them to eternal death."}},
	{{"elves"}, {previous}},
	{{"elfes"}, {previous}},
	{{"darama"}, {"These are the lands of old secrets where the mortals will be shown the true revelations."}},
	{{"darashia"}, {"A city of misguided fools."}},
	{{"daraman"}, {"The prophet caught a glimpse of ascension, but he did not understand it."}},
	{{"ankrahmun"}, {"This city will remain as an eternal testament of our immortal pharaoh's power."}},

	{{"mortality"}, {"Only if we leave our mortality behind can we achieve salvation and ascension."}},
	{{"false", "gods"}, {"The false gods use our mortal flesh to enslave us."}},

	{{"ascension"}, {"Ascension is a difficult process. As long as we are mortal we are too distracted to even think about it."}},
	{{"Akh'rah", "Uthun"}, {"The Akh'rah Uthun is the union of that which once was separate."}},
	{{"Akh"}, {"Our Akh is our mortal flesh until it is replaced with something better - with an undead body."}},

	{{"undead"}, {"Undeath is the victory over the weaknesses of mortal flesh."}},
	{{"undeath"}, {previous}},
	{{"Rah"}, {"The Rah is our essence. The divine part in all of us."}},
	{{"uthun"}, {"The Uthun is the knowledge we gather in the course of time."}},
	{{"mourn"}, {"We are so pathetic in our mortality."}},

	{{"arena"}, {"The arena is a challenge for every skilled fighter."}},
	{{"palace"}, {"You can find the palace to the east of this market hall."}},

	{{"offer"}, {"I'm selling life and mana fluids."}},
	{{"goods"}, {previous}},
	{{"do", "you", "sell"}, {previous}},
	{{"do", "you", "have"}, {previous}},

	{{"life", "fluid"}, {setType(2874), setData(11), setAmount(1), setPrice(60), "Do you want to buy life fluid for %P gold?", setTopic(2)}},
	{{"mana", "fluid"}, {setType(2874), setData(10), setAmount(1), setPrice(55), "Do you want to buy mana fluid for %P gold?", setTopic(2)}},
	{{P1, lt(1, P1), "life", "fluid"}, {setType(2874), setData(11), setAmount(P1), setPrice(mul(60, P1)), "Do you want to buy %A potions of life fluid for %P gold?", setTopic(2)}},
	{{P1, lt(1, P1), "mana", "fluid"}, {setType(2874), setData(10), setAmount(P1), setPrice(mul(55, P1)), "Do you want to buy %A potions of mana fluid for %P gold?", setTopic(2)}},

	{{"deposit"}, {"I will pay you 5 gold for every empty vial. Ok?", setData(0), setTopic(3)}},
	{{"vial"}, {previous}},
	{{"flask"}, {previous}},

	{{eq(TOPIC, 2), "yes", ge(COUNTMONEY, PRICE)}, {"Here you are. There is a deposit of 5 gold on the vial.", deleteMoney, create(TYPE)}},
	{{eq(TOPIC, 2), "yes"}, {"Come back, when you have enough money."}},
	{{eq(TOPIC, 2)}, {"Hmm, but next time."}},

	{{eq(TOPIC, 3), "yes", gt(count(2874), 0)}, {setAmount(count(2874)), setPrice(mul(AMOUNT, 5)), "Here you are ... %P gold.", delete(2874), createMoney}},
	{{eq(TOPIC, 3), "yes"}, {"You don't have any empty vials."}},
	{{eq(TOPIC, 3)}, {"Hmm, but please keep Tibia litter free."}},
}

function onCreatureSay(npc, creature, text, situation)
	behaviour.react(npc, rules, text, situation)
end

function onCreatureDisappear(npc, creature)
	behaviour.react(npc, rules, "", NPC_SITUATION_VANISH)
end
