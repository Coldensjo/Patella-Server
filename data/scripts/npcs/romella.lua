-- npcs/romella.lua: the behaviour of the NPC Romella, converted from
-- romella.npc by patella-convert. The NPC is its [[npc]] entry in npcs.toml;
-- lib/npc.lua matches and runs the rules below, in the order of the file
-- (scripting.hh, "NPCs"). The comments are those of romella.npc.
--
-- GIMUD - Graphical Interface Multi User Dungeon
-- romella.npc: Datenbank fuer die Waffenhändlerin Romella

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
local create = behaviour.create
local delete = behaviour.delete
local createMoney = behaviour.createMoney
local deleteMoney = behaviour.deleteMoney
local queue = behaviour.queue
local idle = behaviour.idle
local nop = behaviour.nop
local previous = behaviour.previous

local rules = behaviour.rules {
	{{ADDRESS, "hello$", FORCE}, {"I welcome thee, %N!"}},
	{{ADDRESS, "hi$", FORCE}, {previous}},
	{{ADDRESS, FORCE}, {idle}},
	{{BUSY, "hello$", FORCE}, {"Just one more moment please, %N.", queue}},
	{{BUSY, "hi$", FORCE}, {previous}},
	{{BUSY, FORCE}, {nop}},
	{{VANISH, FORCE}, {"Goodbye and please bring more gold next time <chuckles>. I mean, it would be nice to see you again."}},

	{{"bye"}, {"Goodbye and please bring more gold next time <chuckles>. I mean, it would be nice to see you again.", idle}},
	{{"farewell"}, {previous}},
	{{"job"}, {"I am Romella, and I will be serving you today."}},
	{{"shop"}, {previous}},
	{{"name"}, {previous}},
	{{"time"}, {"It is %T."}},
	{{"king"}, {"The only royal thing we feel here is the royal tax."}},
	{{"tibianus"}, {previous}},
	{{"army"}, {"Our warehouse is the main supplier of the local garrison."}},
	{{"ferumbras"}, {"Make sure to buy some extra weapons before facing that one."}},
	{{"excalibug"}, {"I heard the amazons are after it."}},
	{{"news"}, {"It says the amazons are looking for a certain magical weapon in this area."}},
	{{"amazon"}, {"I wonder how they finance themselves. I bet they are secretly trading in some strange stuff."}},
	{{"help"}, {"The weapons we sell are all help you need."}},
	{{"monster"}, {"Just buy enough weapons and you don't have to fear them."}},
	{{"swamp"}, {"Don't go exploring without weapons. Especially you'll need a machete."}},
	{{"thanks"}, {"You are welcome."}},
	{{"thank", "you"}, {previous}},

	{{"offer"}, {"I sell several weapons."}},
	{{"do", "you", "sell"}, {previous}},
	{{"do", "you", "have"}, {previous}},
	{{"weapon"}, {"I have hand axes, axes, spears, maces, battle hammers, swords, rapiers, daggers, sabres, and machetes. What's your choice?"}},

	{{"dagger"}, {setType(3267), setAmount(1), setPrice(5), "Do you want to buy a dagger for %P gold?", setTopic(1)}},
	{{"hand", "axe"}, {setType(3268), setAmount(1), setPrice(8), "Do you want to buy a hand axe for %P gold?", setTopic(1)}},
	{{"spear"}, {setType(3277), setAmount(1), setPrice(10), "Do you want to buy a spear for %P gold?", setTopic(1)}},
	{{"rapier"}, {setType(3272), setAmount(1), setPrice(15), "Do you want to buy a rapier for %P gold?", setTopic(1)}},
	{{"axe"}, {setType(3274), setAmount(1), setPrice(20), "Do you want to buy an axe for %P gold?", setTopic(1)}},
	{{"sabre"}, {setType(3273), setAmount(1), setPrice(35), "Do you want to buy a sabre for %P gold?", setTopic(1)}},
	{{"sword"}, {setType(3264), setAmount(1), setPrice(85), "Do you want to buy a sword for %P gold?", setTopic(1)}},
	{{"mace"}, {setType(3286), setAmount(1), setPrice(90), "Do you want to buy a mace for %P gold?", setTopic(1)}},
	{{"battle", "hammer"}, {setType(3305), setAmount(1), setPrice(350), "Do you want to buy a battle hammer for %P gold?", setTopic(1)}},
	{{"machete"}, {setType(3308), setAmount(1), setPrice(35), "Do you want to buy a machete for %P gold?", setTopic(1)}},
	{{"throwing", "knife"}, {setType(3298), setAmount(1), setPrice(25), "Do you want to buy a throwing knife for %P gold?", setTopic(1)}},

	{{P1, lt(1, P1), "dagger"}, {setType(3267), setAmount(P1), setPrice(mul(5, P1)), "Do you want to buy %A daggers for %P gold?", setTopic(1)}},
	{{P1, lt(1, P1), "hand", "axe"}, {setType(3268), setAmount(P1), setPrice(mul(8, P1)), "Do you want to buy %A hand axes for %P gold?", setTopic(1)}},
	{{P1, lt(1, P1), "spear"}, {setType(3277), setAmount(P1), setPrice(mul(10, P1)), "Do you want to buy %A spears for %P gold?", setTopic(1)}},
	{{P1, lt(1, P1), "rapier"}, {setType(3272), setAmount(P1), setPrice(mul(15, P1)), "Do you want to buy %A rapiers for %P gold?", setTopic(1)}},
	{{P1, lt(1, P1), "axe"}, {setType(3274), setAmount(P1), setPrice(mul(20, P1)), "Do you want to buy %A axes for %P gold?", setTopic(1)}},
	{{P1, lt(1, P1), "sabre"}, {setType(3273), setAmount(P1), setPrice(mul(35, P1)), "Do you want to buy %A sabres for %P gold?", setTopic(1)}},
	{{P1, lt(1, P1), "sword"}, {setType(3264), setAmount(P1), setPrice(mul(85, P1)), "Do you want to buy %A swords for %P gold?", setTopic(1)}},
	{{P1, lt(1, P1), "mace"}, {setType(3286), setAmount(P1), setPrice(mul(90, P1)), "Do you want to buy %A maces for %P gold?", setTopic(1)}},
	{{P1, lt(1, P1), "battle", "hammer"}, {setType(3305), setAmount(P1), setPrice(mul(350, P1)), "Do you want to buy %A battle hammers for %P gold?", setTopic(1)}},
	{{P1, lt(1, P1), "machete"}, {setType(3308), setAmount(P1), setPrice(mul(35, P1)), "Do you want to buy %A machetes for %P gold?", setTopic(1)}},
	{{P1, lt(1, P1), "throwing", "kni"}, {setType(3298), setAmount(P1), setPrice(mul(25, P1)), "Do you want to buy %A throwing knives for %P gold?", setTopic(1)}},

	{{"sell", "two", "handed", "sword"}, {setType(3265), setAmount(1), setPrice(450), "Do you want to sell a two handed sword for %P gold?", setTopic(2)}},
	{{"sell", "battle", "axe"}, {setType(3266), setAmount(1), setPrice(80), "Do you want to sell a battle axe for %P gold?", setTopic(2)}},
	{{"sell", "dagger"}, {setType(3267), setAmount(1), setPrice(2), "Do you want to sell this garbage? I give you %P gold, ok?", setTopic(2)}},
	{{"sell", "hand", "axe"}, {setType(3268), setAmount(1), setPrice(4), "Do you want to sell a hand axe for %P gold?", setTopic(2)}},
	{{"sell", "halberd"}, {setType(3269), setAmount(1), setPrice(400), "Do you want to sell a halberd for %P gold?", setTopic(2)}},
	{{"sell", "rapier"}, {setType(3272), setAmount(1), setPrice(5), "Do you want to sell a rapier for %P gold?", setTopic(2)}},
	{{"sell", "sabre"}, {setType(3273), setAmount(1), setPrice(12), "Do you want to sell a sabre for %P gold?", setTopic(2)}},
	{{"sell", "axe"}, {setType(3274), setAmount(1), setPrice(7), "Do you want to sell an axe for %P gold?", setTopic(2)}},
	{{"sell", "spear"}, {setType(3277), setAmount(1), setPrice(3), "Do you want to sell this garbage? I give you %P gold, ok?", setTopic(2)}},
	{{"sell", "morning", "star"}, {setType(3282), setAmount(1), setPrice(90), "Do you want to sell a morning star for %P gold?", setTopic(2)}},
	{{"sell", "mace"}, {setType(3286), setAmount(1), setPrice(30), "Do you want to sell a mace for %P gold?", setTopic(2)}},
	{{"sell", "short", "sword"}, {setType(3294), setAmount(1), setPrice(10), "Do you want to sell a short sword for %P gold?", setTopic(2)}},
	{{"sell", "battle", "hammer"}, {setType(3305), setAmount(1), setPrice(120), "Do you want to sell a battle hammer for %P gold?", setTopic(2)}},
	{{"sell", "sword"}, {setType(3264), setAmount(1), setPrice(25), "Do you want to sell a sword for %P gold?", setTopic(2)}},

	{{"sell", P1, lt(1, P1), "two", "handed", "sword"}, {setType(3265), setAmount(P1), setPrice(mul(450, P1)), "Do you want to sell %A two handed swords for %P gold?", setTopic(2)}},
	{{"sell", P1, lt(1, P1), "battle", "axe"}, {setType(3266), setAmount(P1), setPrice(mul(80, P1)), "Do you want to sell %A battle axes for %P gold?", setTopic(2)}},
	{{"sell", P1, lt(1, P1), "dagger"}, {setType(3267), setAmount(P1), setPrice(mul(2, P1)), "Do you want to sell this garbage? I give you %P gold, ok?", setTopic(2)}},
	{{"sell", P1, lt(1, P1), "hand", "axe"}, {setType(3268), setAmount(P1), setPrice(mul(4, P1)), "Do you want to sell %A hand axes for %P gold?", setTopic(2)}},
	{{"sell", P1, lt(1, P1), "halberd"}, {setType(3269), setAmount(P1), setPrice(mul(400, P1)), "Do you want to sell %A halberds for %P gold?", setTopic(2)}},
	{{"sell", P1, lt(1, P1), "rapier"}, {setType(3272), setAmount(P1), setPrice(mul(5, P1)), "Do you want to sell %A rapiers for %P gold?", setTopic(2)}},
	{{"sell", P1, lt(1, P1), "sabre"}, {setType(3273), setAmount(P1), setPrice(mul(12, P1)), "Do you want to sell %A sabres for %P gold?", setTopic(2)}},
	{{"sell", P1, lt(1, P1), "axe"}, {setType(3274), setAmount(P1), setPrice(mul(7, P1)), "Do you want to sell %A axes for %P gold?", setTopic(2)}},
	{{"sell", P1, lt(1, P1), "spear"}, {setType(3277), setAmount(P1), setPrice(mul(3, P1)), "Do you want to sell this garbage? I give you %P gold, ok?", setTopic(2)}},
	{{"sell", P1, lt(1, P1), "morning", "star"}, {setType(3282), setAmount(P1), setPrice(mul(90, P1)), "Do you want to sell %A morning stars for %P gold?", setTopic(2)}},
	{{"sell", P1, lt(1, P1), "mace"}, {setType(3286), setAmount(P1), setPrice(mul(30, P1)), "Do you want to sell %A maces for %P gold?", setTopic(2)}},
	{{"sell", P1, lt(1, P1), "short", "sword"}, {setType(3294), setAmount(P1), setPrice(mul(10, P1)), "Do you want to sell %A short swords for %P gold?", setTopic(2)}},
	{{"sell", P1, lt(1, P1), "battle", "hammer"}, {setType(3305), setAmount(P1), setPrice(mul(120, P1)), "Do you want to sell %A battle hammers for %P gold?", setTopic(2)}},
	{{"sell", P1, lt(1, P1), "sword"}, {setType(3264), setAmount(P1), setPrice(mul(25, P1)), "Do you want to sell %A swords for %P gold?", setTopic(2)}},

	{{eq(TOPIC, 1), "yes", ge(COUNTMONEY, PRICE)}, {"Thank you. Here it is.", deleteMoney, create(TYPE)}},
	{{eq(TOPIC, 1), "yes"}, {"Sorry, you do not have enough gold."}},
	{{eq(TOPIC, 1)}, {"Maybe you will buy it another time."}},

	{{eq(TOPIC, 2), "yes", ge(count(TYPE), AMOUNT)}, {"Ok. Here is your money.", delete(TYPE), createMoney}},
	{{eq(TOPIC, 2), "yes"}, {"Sorry, you do not have one."}},
	{{eq(TOPIC, 2), "yes", gt(AMOUNT, 1)}, {"Sorry, you do not have so many."}},
	{{eq(TOPIC, 2)}, {"Maybe next time."}},
}

function onCreatureSay(npc, creature, text, situation)
	behaviour.react(npc, rules, text, situation)
end

function onCreatureDisappear(npc, creature)
	behaviour.react(npc, rules, "", NPC_SITUATION_VANISH)
end
