-- npcs/willard.lua: the behaviour of the NPC Willard, converted from
-- willard.npc by patella-convert. The NPC is its [[npc]] entry in npcs.toml;
-- lib/npc.lua matches and runs the rules below, in the order of the file
-- (scripting.hh, "NPCs"). The comments are those of willard.npc.
--
-- GIMUD - Graphical Interface Multi User Dungeon
-- willard.npc: Datenbank für den Schmied Willard

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
	{{ADDRESS, "hello$", FORCE}, {"Greetings and Banor be with you, %N!"}},
	{{ADDRESS, "hi$", FORCE}, {previous}},
	{{ADDRESS, FORCE}, {idle}},
	{{BUSY, "hello$", FORCE}, {"I'm ready soon, %N. Please wait.", queue}},
	{{BUSY, "hi$", FORCE}, {previous}},
	{{BUSY, FORCE}, {nop}},
	{{VANISH, FORCE}, {"Farewell."}},

	{{"bye"}, {"Farewell.", idle}},
	{{"name"}, {"I am Willard, the smith."}},
	{{"job"}, {"I am the blacksmith of castle Bloodrock."}},
	{{"time"}, {"Now it is %T."}},
	{{"king"}, {"Hail to the King! He's our benevolent protector."}},
	{{"tibianus"}, {previous}},
	{{"army"}, {"I supply them with all they need."}},
	{{"ferumbras"}, {"I would be honored if it's one of my blades that one day delivers him his punishment."}},
	{{"excalibug"}, {"Adventurers search for this blade all over the world. Even here."}},
	{{"thais"}, {"A fine city, but I love the peace of Edron more."}},
	{{"tibia"}, {"It's my dream that one day the whole world will profit from the Thaian governance."}},
	{{"carlin"}, {"If the king sees the time is right, he will certainly start a campaign to reclaim what belongs to Thais."}},
	{{"edron"}, {"Edron is a fine city to live in."}},
	{{"news"}, {"Rumors are too sinister things that a true warrior would care for them."}},
	{{"rumors"}, {previous}},

	{{"buy"}, {"What do you need? I sell weapons, armors, helmets, and shields."}},
	{{"do", "you", "sell"}, {previous}},
	{{"do", "you", "have"}, {previous}},
	{{"offer"}, {"My offers are weapons, ammunition, armors, helmets, legs, and shields."}},
	{{"weapon"}, {"I have hand axes, axes, barbarian axes, spears, maces, clerical maces, battle hammers, swords, rapiers, daggers, sabres, bows, and crossbows. What's your choice?"}},
	{{"ammunition"}, {"I have arrows for bows and bolts for crossbows. What do you want?"}},
	{{"helmet"}, {"I am selling leather helmets and chain helmets. What do you want?"}},
	{{"armor"}, {"I am selling leather, chain, and brass armor. What do you need?"}},
	{{"shield"}, {"I am selling wooden shields, steel shields, and viking shields. What do you want?"}},
	{{"trousers"}, {"I am selling chain legs. Do you want to buy any?"}},
	{{"legs"}, {previous}},

	{{"dagger"}, {setType(3267), setAmount(1), setPrice(5), "Do you want to buy a dagger for %P gold?", setTopic(1)}},
	{{"hand", "axe"}, {setType(3268), setAmount(1), setPrice(8), "Do you want to buy a hand axe for %P gold?", setTopic(1)}},
	{{"spear"}, {setType(3277), setAmount(1), setPrice(10), "Do you want to buy a spear for %P gold?", setTopic(1)}},
	{{"rapier"}, {setType(3272), setAmount(1), setPrice(15), "Do you want to buy a rapier for %P gold?", setTopic(1)}},
	{{"axe"}, {setType(3274), setAmount(1), setPrice(20), "Do you want to buy an axe for %P gold?", setTopic(1)}},
	{{"barbar", "axe"}, {setType(3317), setAmount(1), setPrice(590), "Do you want to buy a barbarian axe for %P gold?", setTopic(1)}},
	{{"sabre"}, {setType(3273), setAmount(1), setPrice(35), "Do you want to buy a sabre for %P gold?", setTopic(1)}},
	{{"sword"}, {setType(3264), setAmount(1), setPrice(85), "Do you want to buy a sword for %P gold?", setTopic(1)}},
	{{"mace"}, {setType(3286), setAmount(1), setPrice(90), "Do you want to buy a mace for %P gold?", setTopic(1)}},
	{{"cleric", "mace"}, {setType(3311), setAmount(1), setPrice(540), "Do you want to buy a clerical mace for %P gold?", setTopic(1)}},
	{{"battle", "hammer"}, {setType(3305), setAmount(1), setPrice(350), "Do you want to buy a battle hammer for %P gold?", setTopic(1)}},
	{{"bow"}, {setType(3350), setAmount(1), setPrice(400), "Do you want to buy a bow for %P gold?", setTopic(1)}},
	{{"crossbow"}, {setType(3349), setAmount(1), setPrice(500), "Do you want to buy a crossbow for %P gold?", setTopic(1)}},
	{{"arrow"}, {setType(3447), setAmount(1), setPrice(2), "Do you want to buy an arrow for %P gold?", setTopic(1)}},
	{{"bolt"}, {setType(3446), setAmount(1), setPrice(3), "Do you want to buy a bolt for %P gold?", setTopic(1)}},
	{{"throwing", "star"}, {setType(3287), setAmount(1), setPrice(50), "Do you want to buy a throwing star for %P gold?", setTopic(1)}},

	{{P1, lt(1, P1), "dagger"}, {setType(3267), setAmount(P1), setPrice(mul(5, P1)), "Do you want to buy %A daggers for %P gold?", setTopic(1)}},
	{{P1, lt(1, P1), "hand", "axe"}, {setType(3268), setAmount(P1), setPrice(mul(8, P1)), "Do you want to buy %A hand axes for %P gold?", setTopic(1)}},
	{{P1, lt(1, P1), "spear"}, {setType(3277), setAmount(P1), setPrice(mul(10, P1)), "Do you want to buy %A spears for %P gold?", setTopic(1)}},
	{{P1, lt(1, P1), "rapier"}, {setType(3272), setAmount(P1), setPrice(mul(15, P1)), "Do you want to buy %A rapiers for %P gold?", setTopic(1)}},
	{{P1, lt(1, P1), "axe"}, {setType(3274), setAmount(P1), setPrice(mul(20, P1)), "Do you want to buy %A axes for %P gold?", setTopic(1)}},
	{{P1, lt(1, P1), "barbar", "axe"}, {setType(3317), setAmount(P1), setPrice(mul(590, P1)), "Do you want to buy %A barbarian axes for %P gold?", setTopic(1)}},
	{{P1, lt(1, P1), "sabre"}, {setType(3273), setAmount(P1), setPrice(mul(35, P1)), "Do you want to buy %A sabres for %P gold?", setTopic(1)}},
	{{P1, lt(1, P1), "sword"}, {setType(3264), setAmount(P1), setPrice(mul(85, P1)), "Do you want to buy %A swords for %P gold?", setTopic(1)}},
	{{P1, lt(1, P1), "mace"}, {setType(3286), setAmount(P1), setPrice(mul(90, P1)), "Do you want to buy %A maces for %P gold?", setTopic(1)}},
	{{P1, lt(1, P1), "cleric", "mace"}, {setType(3311), setAmount(P1), setPrice(mul(540, P1)), "Do you want to buy %A clerical maces for %P gold?", setTopic(1)}},
	{{P1, lt(1, P1), "battle", "hammer"}, {setType(3305), setAmount(P1), setPrice(mul(350, P1)), "Do you want to buy %A battle hammers for %P gold?", setTopic(1)}},
	{{P1, lt(1, P1), "bow"}, {setType(3350), setAmount(P1), setPrice(mul(400, P1)), "Do you want to buy %A bows for %P gold?", setTopic(1)}},
	{{P1, lt(1, P1), "crossbow"}, {setType(3349), setAmount(P1), setPrice(mul(500, P1)), "Do you want to buy %A crossbows for %P gold?", setTopic(1)}},
	{{P1, lt(1, P1), "arrow"}, {setType(3447), setAmount(P1), setPrice(mul(2, P1)), "Do you want to buy %A arrows for %P gold?", setTopic(1)}},
	{{P1, lt(1, P1), "bolt"}, {setType(3446), setAmount(P1), setPrice(mul(3, P1)), "Do you want to buy %A bolts for %P gold?", setTopic(1)}},
	{{P1, lt(1, P1), "throwing", "star"}, {setType(3287), setAmount(P1), setPrice(mul(50, P1)), "Do you want to buy %A throwing stars for %P gold?", setTopic(1)}},

	{{"chain", "armor"}, {setType(3358), setAmount(1), setPrice(200), "Do you want to buy a chain armor for %P gold?", setTopic(1)}},
	{{"brass", "armor"}, {setType(3359), setAmount(1), setPrice(450), "Do you want to buy a brass armor for %P gold?", setTopic(1)}},
	{{"leather", "armor"}, {setType(3361), setAmount(1), setPrice(35), "Do you want to buy a leather armor for %P gold?", setTopic(1)}},
	{{"chain", "helmet"}, {setType(3352), setAmount(1), setPrice(52), "Do you want to buy a chain helmet for %P gold?", setTopic(1)}},
	{{"leather", "helmet"}, {setType(3355), setAmount(1), setPrice(12), "Do you want to buy a leather helmet for %P gold?", setTopic(1)}},
	{{"steel", "shield"}, {setType(3409), setAmount(1), setPrice(240), "Do you want to buy a steel shield for %P gold?", setTopic(1)}},
	{{"wooden", "shield"}, {setType(3412), setAmount(1), setPrice(15), "Do you want to buy a wooden shield for %P gold?", setTopic(1)}},
	{{"viking", "shield"}, {setType(3431), setAmount(1), setPrice(260), "Do you want to buy a viking shield for %P gold?", setTopic(1)}},
	{{"chain", "legs"}, {setType(3558), setAmount(1), setPrice(80), "Do you want to buy chain legs for %P gold?", setTopic(1)}},

	{{P1, lt(1, P1), "chain", "armor"}, {setType(3358), setAmount(P1), setPrice(mul(200, P1)), "Do you want to buy %A chain armors for %P gold?", setTopic(1)}},
	{{P1, lt(1, P1), "brass", "armor"}, {setType(3359), setAmount(P1), setPrice(mul(450, P1)), "Do you want to buy %A brass armors for %P gold?", setTopic(1)}},
	{{P1, lt(1, P1), "leather", "armor"}, {setType(3361), setAmount(P1), setPrice(mul(35, P1)), "Do you want to buy %A leather armors for %P gold?", setTopic(1)}},
	{{P1, lt(1, P1), "chain", "helmet"}, {setType(3352), setAmount(P1), setPrice(mul(52, P1)), "Do you want to buy %A chain helmets for %P gold?", setTopic(1)}},
	{{P1, lt(1, P1), "leather", "helmet"}, {setType(3355), setAmount(P1), setPrice(mul(12, P1)), "Do you want to buy %A leather helmets for %P gold?", setTopic(1)}},
	{{P1, lt(1, P1), "steel", "shield"}, {setType(3409), setAmount(P1), setPrice(mul(240, P1)), "Do you want to buy %A steel shields for %P gold?", setTopic(1)}},
	{{P1, lt(1, P1), "wooden", "shield"}, {setType(3412), setAmount(P1), setPrice(mul(15, P1)), "Do you want to buy %A wooden shields for %P gold?", setTopic(1)}},
	{{P1, lt(1, P1), "viking", "shield"}, {setType(3431), setAmount(P1), setPrice(mul(260, P1)), "Do you want to buy %A viking shields for %P gold?", setTopic(1)}},
	{{P1, lt(1, P1), "chain", "legs"}, {setType(3558), setAmount(P1), setPrice(mul(80, P1)), "Do you want to buy %A chain legs for %P gold?", setTopic(1)}},

	{{"sell", "two", "handed", "sword"}, {setType(3265), setAmount(1), setPrice(450), "Do you want to sell a two handed sword for %P gold?", setTopic(2)}},
	{{"sell", "battle", "axe"}, {setType(3266), setAmount(1), setPrice(80), "Do you want to sell a battle axe for %P gold?", setTopic(2)}},
	{{"sell", "dagger"}, {setType(3267), setAmount(1), setPrice(2), "Do you want to sell this garbage? I give you %P gold, ok?", setTopic(2)}},
	{{"sell", "hand", "axe"}, {setType(3268), setAmount(1), setPrice(4), "Do you want to sell a hand axe for %P gold?", setTopic(2)}},
	{{"sell", "halberd"}, {setType(3269), setAmount(1), setPrice(400), "Do you want to sell a halberd for %P gold?", setTopic(2)}},
	{{"sell", "club"}, {setType(3270), setAmount(1), setPrice(1), "Are you sure, you want to sell this garbage? Hmm, I give you %P gold, ok?", setTopic(2)}},
	{{"sell", "spike", "sword"}, {setType(3271), setAmount(1), setPrice(240), "Do you want to sell a spike sword for %P gold?", setTopic(2)}},
	{{"sell", "rapier"}, {setType(3272), setAmount(1), setPrice(5), "Do you want to sell a rapier for %P gold?", setTopic(2)}},
	{{"sell", "sabre"}, {setType(3273), setAmount(1), setPrice(12), "Do you want to sell a sabre for %P gold?", setTopic(2)}},
	{{"sell", "axe"}, {setType(3274), setAmount(1), setPrice(7), "Do you want to sell an axe for %P gold?", setTopic(2)}},
	{{"sell", "spear"}, {setType(3277), setAmount(1), setPrice(3), "Do you want to sell this garbage? I give you %P gold, ok?", setTopic(2)}},
	{{"sell", "morning", "star"}, {setType(3282), setAmount(1), setPrice(90), "Do you want to sell a morning star for %P gold?", setTopic(2)}},
	{{"sell", "mace"}, {setType(3286), setAmount(1), setPrice(30), "Do you want to sell a mace for %P gold?", setTopic(2)}},
	{{"sell", "short", "sword"}, {setType(3294), setAmount(1), setPrice(10), "Do you want to sell a short sword for %P gold?", setTopic(2)}},
	{{"sell", "battle", "hammer"}, {setType(3305), setAmount(1), setPrice(120), "Do you want to sell a battle hammer for %P gold?", setTopic(2)}},
	{{"sell", "cleric", "mace"}, {setType(3311), setAmount(1), setPrice(170), "Do you want to sell a clerical mace for %P gold?", setTopic(2)}},
	{{"sell", "barbar", "axe"}, {setType(3317), setAmount(1), setPrice(185), "Do you want to sell a barbarian axe for %P gold?", setTopic(2)}},
	{{"sell", "sword"}, {setType(3264), setAmount(1), setPrice(25), "Do you want to sell a sword for %P gold?", setTopic(2)}},

	{{"sell", P1, lt(1, P1), "two", "handed", "sword"}, {setType(3265), setAmount(P1), setPrice(mul(450, P1)), "Do you want to sell %A two handed swords for %P gold?", setTopic(2)}},
	{{"sell", P1, lt(1, P1), "battle", "axe"}, {setType(3266), setAmount(P1), setPrice(mul(80, P1)), "Do you want to sell %A battle axes for %P gold?", setTopic(2)}},
	{{"sell", P1, lt(1, P1), "dagger"}, {setType(3267), setAmount(P1), setPrice(mul(2, P1)), "Do you want to sell this garbage? I give you %P gold, ok?", setTopic(2)}},
	{{"sell", P1, lt(1, P1), "hand", "axe"}, {setType(3268), setAmount(P1), setPrice(mul(4, P1)), "Do you want to sell %A hand axes for %P gold?", setTopic(2)}},
	{{"sell", P1, lt(1, P1), "halberd"}, {setType(3269), setAmount(P1), setPrice(mul(400, P1)), "Do you want to sell %A halberds for %P gold?", setTopic(2)}},
	{{"sell", P1, lt(1, P1), "club"}, {setType(3270), setAmount(P1), setPrice(mul(1, P1)), "Are you sure, you want to sell this garbage? Hmm, I give you %P gold, ok?", setTopic(2)}},
	{{"sell", P1, lt(1, P1), "spike", "sword"}, {setType(3271), setAmount(P1), setPrice(mul(240, P1)), "Do you want to sell %A spike swords for %P gold?", setTopic(2)}},
	{{"sell", P1, lt(1, P1), "rapier"}, {setType(3272), setAmount(P1), setPrice(mul(5, P1)), "Do you want to sell %A rapiers for %P gold?", setTopic(2)}},
	{{"sell", P1, lt(1, P1), "sabre"}, {setType(3273), setAmount(P1), setPrice(mul(12, P1)), "Do you want to sell %A sabres for %P gold?", setTopic(2)}},
	{{"sell", P1, lt(1, P1), "axe"}, {setType(3274), setAmount(P1), setPrice(mul(7, P1)), "Do you want to sell %A axes for %P gold?", setTopic(2)}},
	{{"sell", P1, lt(1, P1), "spear"}, {setType(3277), setAmount(P1), setPrice(mul(3, P1)), "Do you want to sell this garbage? I give you %P gold, ok?", setTopic(2)}},
	{{"sell", P1, lt(1, P1), "morning", "star"}, {setType(3282), setAmount(P1), setPrice(mul(90, P1)), "Do you want to sell %A morning stars for %P gold?", setTopic(2)}},
	{{"sell", P1, lt(1, P1), "mace"}, {setType(3286), setAmount(P1), setPrice(mul(30, P1)), "Do you want to sell %A maces for %P gold?", setTopic(2)}},
	{{"sell", P1, lt(1, P1), "short", "sword"}, {setType(3294), setAmount(P1), setPrice(mul(10, P1)), "Do you want to sell %A short swords for %P gold?", setTopic(2)}},
	{{"sell", P1, lt(1, P1), "battle", "hammer"}, {setType(3305), setAmount(P1), setPrice(mul(120, P1)), "Do you want to sell %A battle hammers for %P gold?", setTopic(2)}},
	{{"sell", P1, lt(1, P1), "cleric", "mace"}, {setType(3311), setAmount(P1), setPrice(mul(170, P1)), "Do you want to sell %A clerical maces for %P gold?", setTopic(2)}},
	{{"sell", P1, lt(1, P1), "barbar", "axe"}, {setType(3317), setAmount(P1), setPrice(mul(185, P1)), "Do you want to sell %A barbarian axes for %P gold?", setTopic(2)}},
	{{"sell", P1, lt(1, P1), "sword"}, {setType(3264), setAmount(P1), setPrice(mul(25, P1)), "Do you want to sell %A swords for %P gold?", setTopic(2)}},

	{{"sell", "leather", "armor"}, {setType(3361), setAmount(1), setPrice(12), "Do you want to sell a leather armor for %P gold?", setTopic(2)}},
	{{"sell", "chain", "armor"}, {setType(3358), setAmount(1), setPrice(70), "Do you want to sell a chain armor for %P gold?", setTopic(2)}},
	{{"sell", "brass", "armor"}, {setType(3359), setAmount(1), setPrice(150), "Do you want to sell a brass armor for %P gold?", setTopic(2)}},
	{{"sell", "plate", "armor"}, {setType(3357), setAmount(1), setPrice(400), "Do you want to sell a plate armor for %P gold?", setTopic(2)}},
	{{"sell", "chain", "legs"}, {setType(3558), setAmount(1), setPrice(25), "Do you want to sell chain legs for %P gold?", setTopic(2)}},
	{{"sell", "leather", "helmet"}, {setType(3355), setAmount(1), setPrice(4), "Do you want to sell a leather helmet for %P gold?", setTopic(2)}},
	{{"sell", "iron", "helmet"}, {setType(3353), setAmount(1), setPrice(150), "Do you want to sell an iron helmet for %P gold?", setTopic(2)}},
	{{"sell", "chain", "helmet"}, {setType(3352), setAmount(1), setPrice(17), "Do you want to sell a chain helmet for %P gold?", setTopic(2)}},
	{{"sell", "steel", "helmet"}, {setType(3351), setAmount(1), setPrice(190), "Do you want to sell a steel helmet for %P gold?", setTopic(2)}},
	{{"sell", "wooden", "shield"}, {setType(3412), setAmount(1), setPrice(5), "Do you want to sell a wooden shield for %P gold?", setTopic(2)}},
	{{"sell", "battle", "shield"}, {setType(3413), setAmount(1), setPrice(95), "Do you want to sell a battle shield for %P gold?", setTopic(2)}},
	{{"sell", "steel", "shield"}, {setType(3409), setAmount(1), setPrice(80), "Do you want to sell a steel shield for %P gold?", setTopic(2)}},
	{{"sell", "viking", "shield"}, {setType(3431), setAmount(1), setPrice(85), "Do you want to sell a viking shield for %P gold?", setTopic(2)}},

	{{"sell", P1, lt(1, P1), "leather", "armor"}, {setType(3361), setAmount(P1), setPrice(mul(12, P1)), "Do you want to sell %A leather armors for %P gold?", setTopic(2)}},
	{{"sell", P1, lt(1, P1), "chain", "armor"}, {setType(3358), setAmount(P1), setPrice(mul(70, P1)), "Do you want to sell %A chain armors for %P gold?", setTopic(2)}},
	{{"sell", P1, lt(1, P1), "brass", "armor"}, {setType(3359), setAmount(P1), setPrice(mul(150, P1)), "Do you want to sell %A brass armors for %P gold?", setTopic(2)}},
	{{"sell", P1, lt(1, P1), "plate", "armor"}, {setType(3357), setAmount(P1), setPrice(mul(400, P1)), "Do you want to sell %A plate armors for %P gold?", setTopic(2)}},
	{{"sell", P1, lt(1, P1), "chain", "legs"}, {setType(3558), setAmount(P1), setPrice(mul(25, P1)), "Do you want to sell %A chain legs for %P gold?", setTopic(2)}},
	{{"sell", P1, lt(1, P1), "leather", "helmet"}, {setType(3355), setAmount(P1), setPrice(mul(4, P1)), "Do you want to sell %A leather helmets for %P gold?", setTopic(2)}},
	{{"sell", P1, lt(1, P1), "iron", "helmet"}, {setType(3353), setAmount(P1), setPrice(mul(150, P1)), "Do you want to sell %A iron helmets for %P gold?", setTopic(2)}},
	{{"sell", P1, lt(1, P1), "chain", "helmet"}, {setType(3352), setAmount(P1), setPrice(mul(17, P1)), "Do you want to sell %A chain helmets for %P gold?", setTopic(2)}},
	{{"sell", P1, lt(1, P1), "steel", "helmet"}, {setType(3351), setAmount(P1), setPrice(mul(190, P1)), "Do you want to sell %A steel helmets for %P gold?", setTopic(2)}},
	{{"sell", P1, lt(1, P1), "wooden", "shield"}, {setType(3412), setAmount(P1), setPrice(mul(5, P1)), "Do you want to sell %A wooden shields for %P gold?", setTopic(2)}},
	{{"sell", P1, lt(1, P1), "battle", "shield"}, {setType(3413), setAmount(P1), setPrice(mul(95, P1)), "Do you want to sell %A battle shields for %P gold?", setTopic(2)}},
	{{"sell", P1, lt(1, P1), "steel", "shield"}, {setType(3409), setAmount(P1), setPrice(mul(80, P1)), "Do you want to sell %A steel shields for %P gold?", setTopic(2)}},
	{{"sell", P1, lt(1, P1), "viking", "shield"}, {setType(3431), setAmount(P1), setPrice(mul(85, P1)), "Do you want to sell %A viking shields for %P gold?", setTopic(2)}},

	{{eq(TOPIC, 1), "yes", ge(COUNTMONEY, PRICE)}, {"Thank you. Here it is.", deleteMoney, create(TYPE)}},
	{{eq(TOPIC, 1), "yes"}, {"Sorry, you do not have enough gold."}},
	{{eq(TOPIC, 1)}, {"Maybe you will buy it another time."}},

	{{eq(TOPIC, 2), "yes", ge(count(TYPE), AMOUNT)}, {"Ok. Here is your money.", delete(TYPE), createMoney}},
	{{eq(TOPIC, 2), "yes"}, {"Sorry, you do not have one."}},
	{{eq(TOPIC, 2), "yes", gt(AMOUNT, 1)}, {"Sorry, you do not have so much."}},
	{{eq(TOPIC, 2)}, {"Maybe next time."}},
}

function onCreatureSay(npc, creature, text, situation)
	behaviour.react(npc, rules, text, situation)
end

function onCreatureDisappear(npc, creature)
	behaviour.react(npc, rules, "", NPC_SITUATION_VANISH)
end
