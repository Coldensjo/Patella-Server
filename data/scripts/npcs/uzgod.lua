-- npcs/uzgod.lua: the behaviour of the NPC Uzgod, converted from
-- uzgod.npc by patella-convert. The NPC is its [[npc]] entry in npcs.toml;
-- lib/npc.lua matches and runs the rules below, in the order of the file
-- (scripting.hh, "NPCs"). The comments are those of uzgod.npc.
--
-- GIMUD - Graphical Interface Multi User Dungeon
-- uzgod.npc: Datenbank fuer den Waffenhaendler Uzgod

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
local questValue = behaviour.questValue
local mul = behaviour.mul
local setTopic = behaviour.setTopic
local setPrice = behaviour.setPrice
local setAmount = behaviour.setAmount
local setType = behaviour.setType
local setQuestValue = behaviour.setQuestValue
local create = behaviour.create
local delete = behaviour.delete
local createMoney = behaviour.createMoney
local deleteMoney = behaviour.deleteMoney
local queue = behaviour.queue
local idle = behaviour.idle
local nop = behaviour.nop
local previous = behaviour.previous

local rules = behaviour.rules {
	{{ADDRESS, "hello$", FORCE}, {"Hiho %N! Wanna weapon, eh?"}},
	{{ADDRESS, "hi$", FORCE}, {previous}},
	{{ADDRESS, "hiho$", FORCE}, {previous}},
	{{ADDRESS, FORCE}, {idle}},
	{{BUSY, "hello$", FORCE}, {"Silence %N, Me busy! Wait!", queue}},
	{{BUSY, "hi$", FORCE}, {previous}},
	{{BUSY, "hiho$", FORCE}, {previous}},
	{{BUSY, FORCE}, {nop}},
	{{VANISH, FORCE}, {"Guut bye. Coming back soon."}},

	{{"bye"}, {"Guut bye. Coming back soon.", idle}},
	{{"farewell"}, {previous}},
	{{"job"}, {"Me a blacksmith is, an' weapons me sell. You want buy weapons?"}},
	{{"shop"}, {previous}},
	{{"name"}, {"Me is Uzgod Hammerslammer, son of Fire, from the Savage Axes. You can say you to me."}},
	{{"time"}, {"Time is %T now."}},
	{{"help"}, {"You can buy the weapons me maked or sell weapons you have, jawoll."}},
	{{"monster"}, {"Me make often hunt on big nasties. Me small, but very big muscles me have, jawoll."}},
	{{"dungeon"}, {"We no dungeon need. We prison isle have."}},
	{{"prison"}, {"Bad ones locked up there. Never come out again there, jawoll."}},
	{{"mines"}, {"Me hacking and smashing rocks as me was little dwarf, jawoll."}},
	{{"excalibug"}, {"You want sell me excalibug for 1000 platinum coins and an enchanted armor?", setTopic(3)}},
	{{"thanks"}, {"Me enjoy doing that."}},
	{{"thank", "you"}, {previous}},

	{{"spear"}, {setType(3277), setAmount(1), setPrice(10), "You will buy a spear for %P gold?", setTopic(1)}},
	{{"rapier"}, {setType(3272), setAmount(1), setPrice(15), "You will buy a rapier for %P gold?", setTopic(1)}},
	{{"sabre"}, {setType(3273), setAmount(1), setPrice(35), "You will buy a sabre for %P gold?", setTopic(1)}},
	{{"sword"}, {setType(3264), setAmount(1), setPrice(85), "You will buy a sword for %P gold?", setTopic(1)}},
	{{"battle", "axe"}, {setType(3266), setAmount(1), setPrice(235), "You will buy a battle axe for %P gold?", setTopic(1)}},
	{{"axe"}, {setType(3274), setAmount(1), setPrice(20), "You will buy an axe for %P gold?", setTopic(1)}},
	{{"battle", "hammer"}, {setType(3305), setAmount(1), setPrice(350), "You will buy a battle hammer for %P gold?", setTopic(1)}},
	{{"morning", "star"}, {setType(3282), setAmount(1), setPrice(430), "You will buy a morning star for %P gold?", setTopic(1)}},
	{{"two", "handed", "sword"}, {setType(3265), setAmount(1), setPrice(950), "You will buy a two handed sword for %P gold?", setTopic(1)}},
	{{"club"}, {setType(3270), setAmount(1), setPrice(5), "You will buy a club for %P gold?", setTopic(1)}},
	{{"dagger"}, {setType(3267), setAmount(1), setPrice(5), "You will buy a dagger for %P gold?", setTopic(1)}},
	{{"mace"}, {setType(3286), setAmount(1), setPrice(90), "You will buy a mace for %P gold?", setTopic(1)}},
	{{"throwing", "knife"}, {setType(3298), setAmount(1), setPrice(25), "Do you want to buy a throwing knife for %P gold?", setTopic(1)}},

	{{P1, lt(1, P1), "spear"}, {setType(3277), setAmount(P1), setPrice(mul(10, P1)), "You will buy %A spears for %P gold?", setTopic(1)}},
	{{P1, lt(1, P1), "rapier"}, {setType(3272), setAmount(P1), setPrice(mul(15, P1)), "You will buy %A rapiers for %P gold?", setTopic(1)}},
	{{P1, lt(1, P1), "sabre"}, {setType(3273), setAmount(P1), setPrice(mul(35, P1)), "You will buy %A sabres for %P gold?", setTopic(1)}},
	{{P1, lt(1, P1), "sword"}, {setType(3264), setAmount(P1), setPrice(mul(85, P1)), "You will buy %A swords for %P gold?", setTopic(1)}},
	{{P1, lt(1, P1), "battle", "axe"}, {setType(3266), setAmount(P1), setPrice(mul(235, P1)), "You will buy %A battle axes for %P gold?", setTopic(1)}},
	{{P1, lt(1, P1), "axe"}, {setType(3274), setAmount(P1), setPrice(mul(20, P1)), "You will buy %A axes for %P gold?", setTopic(1)}},
	{{P1, lt(1, P1), "battle", "hammer"}, {setType(3305), setAmount(P1), setPrice(mul(350, P1)), "You will buy %A battle hammers for %P gold?", setTopic(1)}},
	{{P1, lt(1, P1), "morning", "star"}, {setType(3282), setAmount(P1), setPrice(mul(430, P1)), "You will buy %A morning stars for %P gold?", setTopic(1)}},
	{{P1, lt(1, P1), "two", "handed", "sword"}, {setType(3265), setAmount(P1), setPrice(mul(950, P1)), "You will buy %A two handed swords for %P gold?", setTopic(1)}},
	{{P1, lt(1, P1), "club"}, {setType(3270), setAmount(P1), setPrice(mul(5, P1)), "You will buy %A clubs for %P gold?", setTopic(1)}},
	{{P1, lt(1, P1), "dagger"}, {setType(3267), setAmount(P1), setPrice(mul(5, P1)), "You will buy %A daggers for %P gold?", setTopic(1)}},
	{{P1, lt(1, P1), "mace"}, {setType(3286), setAmount(P1), setPrice(mul(90, P1)), "You will buy %A maces for %P gold?", setTopic(1)}},
	{{P1, lt(1, P1), "throwing", "kni"}, {setType(3298), setAmount(P1), setPrice(mul(25, P1)), "Do you want to buy %A throwing knives for %P gold?", setTopic(1)}},

	{{"sell", "mace"}, {setType(3286), setAmount(1), setPrice(23), "You want sell me a mace for %P gold?", setTopic(2)}},
	{{"sell", "dagger"}, {setType(3267), setAmount(1), setPrice(1), "You want sell me a dagger for %P gold?", setTopic(2)}},
	{{"sell", "carlin", "sword"}, {setType(3283), setAmount(1), setPrice(118), "You want sell me a carlin sword for %P gold?", setTopic(2)}},
	{{"sell", "club"}, {setType(3270), setAmount(1), setPrice(1), "Hoho, you me given wood for fireplace? Giving you %P gold, ok?", setTopic(2)}},
	{{"sell", "spear"}, {setType(3277), setAmount(1), setPrice(1), "Hoho, you me given wood for fireplace? Giving you %P gold, ok?", setTopic(2)}},
	{{"sell", "rapier"}, {setType(3272), setAmount(1), setPrice(3), "You want sell me a rapier for %P gold?", setTopic(2)}},
	{{"sell", "sabre"}, {setType(3273), setAmount(1), setPrice(5), "You want sell me a sabre for %P gold?", setTopic(2)}},
	{{"sell", "sword"}, {setType(3264), setAmount(1), setPrice(15), "You want sell me a sword for %P gold?", setTopic(2)}},
	{{"sell", "battle", "axe"}, {setType(3266), setAmount(1), setPrice(75), "You want sell me a battle axe for %P gold?", setTopic(2)}},
	{{"sell", "battle", "hammer"}, {setType(3305), setAmount(1), setPrice(50), "You want sell me a battle hammer for %P gold?", setTopic(2)}},
	{{"sell", "morning", "star"}, {setType(3282), setAmount(1), setPrice(100), "You want sell me a morning star for %P gold?", setTopic(2)}},
	{{"sell", "two", "handed", "sword"}, {setType(3265), setAmount(1), setPrice(190), "You want sell me a two handed sword for %P gold?", setTopic(2)}},
	{{"sell", "halberd"}, {setType(3269), setAmount(1), setPrice(310), "You want sell me a halberd for %P gold?", setTopic(2)}},
	{{"sell", "double", "axe"}, {setType(3275), setAmount(1), setPrice(260), "You want sell me a double axe for %P gold?", setTopic(2)}},
	{{"sell", "war", "hammer"}, {setType(3279), setAmount(1), setPrice(470), "You want sell me a war hammer for %P gold?", setTopic(2)}},
	{{"sell", "longsword"}, {setType(3285), setAmount(1), setPrice(51), "You want sell me a longsword for %P gold?", setTopic(2)}},
	{{"sell", "spike", "sword"}, {setType(3271), setAmount(1), setPrice(225), "You want sell me a spike sword for %P gold?", setTopic(2)}},
	{{"sell", "fire", "sword"}, {setType(3280), setAmount(1), setPrice(1000), "You want sell me a fire sword for %P gold?", setTopic(2)}},

	{{"sell", P1, lt(1, P1), "mace"}, {setType(3286), setAmount(P1), setPrice(mul(23, P1)), "You want sell me %A maces for %P gold?", setTopic(2)}},
	{{"sell", P1, lt(1, P1), "dagger"}, {setType(3267), setAmount(P1), setPrice(mul(1, P1)), "You want sell me %A daggers for %P gold?", setTopic(2)}},
	{{"sell", P1, lt(1, P1), "carlin", "sword"}, {setType(3283), setAmount(P1), setPrice(mul(118, P1)), "You want sell me %A carlin swords for %P gold?", setTopic(2)}},
	{{"sell", P1, lt(1, P1), "club"}, {setType(3270), setAmount(P1), setPrice(mul(1, P1)), "Hoho, you me given wood for fireplace? Giving you %P gold, ok?", setTopic(2)}},
	{{"sell", P1, lt(1, P1), "spear"}, {setType(3277), setAmount(P1), setPrice(mul(1, P1)), "Hoho, you me given wood for fireplace? Giving you %P gold, ok?", setTopic(2)}},
	{{"sell", P1, lt(1, P1), "rapier"}, {setType(3272), setAmount(P1), setPrice(mul(3, P1)), "You want sell me %A rapiers for %P gold?", setTopic(2)}},
	{{"sell", P1, lt(1, P1), "sabre"}, {setType(3273), setAmount(P1), setPrice(mul(5, P1)), "You want sell me %A sabres for %P gold?", setTopic(2)}},
	{{"sell", P1, lt(1, P1), "sword"}, {setType(3264), setAmount(P1), setPrice(mul(15, P1)), "You want sell me %A swords for %P gold?", setTopic(2)}},
	{{"sell", P1, lt(1, P1), "battle", "axe"}, {setType(3266), setAmount(P1), setPrice(mul(75, P1)), "You want sell me %A battle axes for %P gold?", setTopic(2)}},
	{{"sell", P1, lt(1, P1), "battle", "hammer"}, {setType(3305), setAmount(P1), setPrice(mul(50, P1)), "You want sell me %A battle hammers for %P gold?", setTopic(2)}},
	{{"sell", P1, lt(1, P1), "morning", "star"}, {setType(3282), setAmount(P1), setPrice(mul(100, P1)), "You want sell me %A morning stars for %P gold?", setTopic(2)}},
	{{"sell", P1, lt(1, P1), "two", "handed", "sword"}, {setType(3265), setAmount(P1), setPrice(mul(190, P1)), "You want sell me %A two handed swords for %P gold?", setTopic(2)}},
	{{"sell", P1, lt(1, P1), "halberd"}, {setType(3269), setAmount(P1), setPrice(mul(310, P1)), "You want sell me %A halberds for %P gold?", setTopic(2)}},
	{{"sell", P1, lt(1, P1), "double", "axe"}, {setType(3275), setAmount(P1), setPrice(mul(260, P1)), "You want sell me %A double axes for %P gold?", setTopic(2)}},
	{{"sell", P1, lt(1, P1), "war", "hammer"}, {setType(3279), setAmount(P1), setPrice(mul(470, P1)), "You want sell me %A war hammers for %P gold?", setTopic(2)}},
	{{"sell", P1, lt(1, P1), "longsword"}, {setType(3285), setAmount(P1), setPrice(mul(51, P1)), "You want sell me %A longswords for %P gold?", setTopic(2)}},
	{{"sell", P1, lt(1, P1), "spike", "sword"}, {setType(3271), setAmount(P1), setPrice(mul(225, P1)), "You want sell me %A spike swords for %P gold?", setTopic(2)}},
	{{"sell", P1, lt(1, P1), "fire", "sword"}, {setType(3280), setAmount(P1), setPrice(mul(1000, P1)), "You want sell me %A fire swords for %P gold?", setTopic(2)}},

	{{eq(TOPIC, 1), "yes", ge(COUNTMONEY, PRICE)}, {"Me thank you. Here is your stuff.", deleteMoney, create(TYPE)}},
	{{eq(TOPIC, 1), "yes"}, {"No gold, no deal, jawoll."}},
	{{eq(TOPIC, 1)}, {"I sorry you not buy."}},

	{{eq(TOPIC, 2), "yes", ge(count(TYPE), AMOUNT)}, {"Here is your money.", delete(TYPE), createMoney}},
	{{eq(TOPIC, 2), "yes"}, {"You not have it."}},
	{{eq(TOPIC, 2), "yes", gt(AMOUNT, 1)}, {"You not have so many."}},
	{{eq(TOPIC, 2)}, {"You my sorry not to sell."}},

	{{eq(TOPIC, 3), "yes"}, {"Stop make fun of old dwarf, you not having it!"}},
	{{eq(TOPIC, 3), "no"}, {"Me wouldn't sell it, too."}},
	{{eq(TOPIC, 3)}, {"You joke with me!"}},

	{{"do", "you", "sell"}, {"What you need? Me just the weapons sell."}},
	{{"do", "you", "have"}, {previous}},
	{{"light"}, {"Me having clubs, daggers, spears, axes, swords, maces, rapiers, and sabres. What is your choice?"}},
	{{"heavy"}, {"Me having the best two handed swords in tibia. I also sell battle hammers. What is your choice?"}},
	{{"offer"}, {"Me offer you light an' heavy weapons."}},
	{{"weapon"}, {previous}},
	{{"helmet"}, {"Me just sell weapons."}},
	{{"armor"}, {previous}},
	{{"shield"}, {previous}},

	{{"pickaxe", eq(questValue(325), 2), eq(questValue(326), 0)}, {"Get me brooch and me get you pickaxe. Look for keys that bringing you to dwarven prison and get brooch."}},
	{{"pickaxe", eq(questValue(325), 1)}, {"True dwarven pickaxes having to be maded by true weaponsmith! You wanting to get pickaxe for explorer society?", setTopic(4)}},
	{{"pickaxe"}, {"True dwarven pickaxes having to be maded by true weaponsmith! Me order book full though."}},

	{{eq(TOPIC, 4), "yes"}, {"Me order book quite full is. But telling you what: You getting me something me lost and Uzgod seeing that your pickaxe comes first. Jawoll! You interested?", setTopic(5)}},
	{{eq(TOPIC, 4), "no"}, {"Stop make fun of old dwarf."}},
	{{eq(TOPIC, 4)}, {"You joke with me!"}},

	{{eq(TOPIC, 5), "yes"}, {"Good good. You listening: Me was stolen valuable heirloom. Brooch from my family. Good thing is criminal was caught. Bad thing is, criminal now in dwarven prison of dwacatra is and must have taken brooch with him ...", "To get into dwacatra you having to get several keys. Each key opening way to other key until you get key to dwarven prison ...", "Last key should be in the generals quarter near armory. Only General might have key to enter there too. But me not knowing how to enter Generals private room at barracks. You looking on your own ...", "When got key, then you going down to dwarven prison and getting me that brooch. Tell me that you got brooch when having it.", setQuestValue(325, 2)}},
	{{eq(TOPIC, 5)}, {"Then you trying again in five years or so. Maybe pickaxe ready then."}},
	{{"pickaxe", eq(questValue(325), 2), eq(questValue(326), 1)}, {setType(4834), setAmount(1), "You got me brooch?", setTopic(6)}},
	{{"brooch", eq(questValue(325), 2), eq(questValue(326), 1)}, {previous}},

	{{eq(TOPIC, 6), "yes", ge(count(TYPE), AMOUNT)}, {"Thanking you for brooch. Me guessing you now want your pickaxe?", delete(TYPE), setType(4845), setAmount(1), setQuestValue(325, 3), setTopic(7)}},
	{{eq(TOPIC, 6), "yes", lt(count(TYPE), AMOUNT)}, {"Stop make fun of old dwarf."}},
	{{eq(TOPIC, 6), "no"}, {"Stop make fun of old dwarf."}},
	{{eq(TOPIC, 6)}, {"You joke with me!"}},

	{{"pickaxe", eq(questValue(325), 3), eq(questValue(326), 1)}, {setType(4845), setAmount(1), "You want you pickaxe?", setTopic(7)}},

	{{eq(TOPIC, 7), "yes"}, {"Here you have it.", create(TYPE), setQuestValue(325, 4)}},
	{{eq(TOPIC, 7), "no"}, {"Stop make fun of old dwarf."}},
	{{eq(TOPIC, 7)}, {"You joke with me!"}},
}

function onCreatureSay(npc, creature, text, situation)
	behaviour.react(npc, rules, text, situation)
end

function onCreatureDisappear(npc, creature)
	behaviour.react(npc, rules, "", NPC_SITUATION_VANISH)
end
