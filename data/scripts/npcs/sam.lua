-- npcs/sam.lua: the behaviour of the NPC Sam, converted from
-- sam.npc by patella-convert. The NPC is its [[npc]] entry in npcs.toml;
-- lib/npc.lua matches and runs the rules below, in the order of the file
-- (scripting.hh, "NPCs"). The comments are those of sam.npc.
--
-- GIMUD - Graphical Interface Multi User Dungeon
-- sam.npc: Datenbank fuer den Schmied Sam

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
	{{ADDRESS, "hello$", "sam", FORCE}, {"Hi %N. Can I do something for you?"}},
	{{ADDRESS, "hi$", "sam", FORCE}, {previous}},
	{{ADDRESS, "hello$", FORCE}, {"Welcome to my shop, adventurer %N!"}},
	{{ADDRESS, "hi$", FORCE}, {previous}},
	{{ADDRESS, FORCE}, {idle}},
	{{BUSY, "hello$", FORCE}, {"One moment please, %N.", queue}},
	{{BUSY, "hi$", FORCE}, {previous}},
	{{BUSY, FORCE}, {nop}},
	{{VANISH, FORCE}, {"Good bye and come again."}},

	{{"bye"}, {"Good bye and come again.", idle}},
	{{"farewell"}, {previous}},
	{{"job"}, {"I am the blacksmith. If you need weapons or armor - just ask me."}},
	{{"shop"}, {previous}},
	{{"name"}, {"My name is Samuel, but you can call me Sam."}},
	{{"time"}, {"It is %T."}},
	{{"king"}, {"The king supports Tibia's economy a lot."}},
	{{"tibianus"}, {previous}},
	{{"quentin"}, {"He is a monk of some kind!"}},
	{{"lynda"}, {"Uhm! ---blush---"}},
	{{"harkath"}, {"A warrior who is a joy for Banor."}},
	{{"general"}, {previous}},
	{{"army"}, {"I supply the army with weapons and armor."}},
	{{"ferumbras"}, {"A threat for mankind! Buy weapons to be ready to face him."}},
	{{"sam"}, {"I was named after my grandfather."}},
	{{"gorn"}, {"He can tell a tale or two about his adventures with baxter in their younger days."}},
	{{"frodo"}, {"I don't like crowded places like his bar."}},
	{{"elane"}, {"Oh, I hardly know her."}},
	{{"muriel"}, {"Sorcerers seldom need my skills."}},
	{{"gregor"}, {"His guild relies heavily on my wares."}},
	{{"marvik"}, {"I never visited his ... cave or whatever it's called."}},
	{{"bozo"}, {"He is funny now and then."}},
	{{"baxter"}, {"A fine warrior."}},
	{{"oswald"}, {"Oswald isn't one of the most liked people in this city."}},
	{{"sherry"}, {"The McRonalds are the local farmers, aren't they?"}},
	{{"donald"}, {previous}},
	{{"mcronald"}, {previous}},
	{{"lugri"}, {"I just know some rumours that he is a follower of evil."}},
	{{"excalibug"}, {"It is rumoured to be a weapon beyond mortal craftsmanship."}},
	{{"news"}, {"I know nothing of interest."}},
	{{"help"}, {"I sell and buy weapons, armor, helmets, and shields. So you are able to slash the monsters."}},
	{{"monster"}, {"Yeah, these awful beasts. They live in the forests near the city and in the sewers and dungeons."}},
	{{"dungeon"}, {"Below our city are the sewers and I heard about a passage to the deeper dungeons."}},
	{{"sewer"}, {previous}},
	{{"passage"}, {"Don't ask me. I have never been there."}},
	{{"thanks"}, {"You are welcome."}},
	{{"thank", "you"}, {previous}},

	{{"offer"}, {"My offers are weapons, armors, helmets, legs, and shields."}},
	{{"do", "you", "sell"}, {previous}},
	{{"do", "you", "have"}, {previous}},
	{{"weapon"}, {"I have hand axes, axes, spears, maces, battle hammers, swords, rapiers, daggers, and sabres. What's your choice?"}},
	{{"helmet"}, {"I am selling leather helmets and chain helmets. What do you want?"}},
	{{"armor"}, {"I am selling leather, chain and brass armors. What do you need?"}},
	{{"shield"}, {"I am selling wooden shields and steel shields. What do you want?"}},
	{{"trousers"}, {"I am selling chain legs. Do you want to buy any?"}},
	{{"legs"}, {previous}},

	{{"dagger"}, {setType(3267), setAmount(1), setPrice(5), "Do you want to buy a dagger for %P gold?", setTopic(1)}},
	{{"hand", "axe"}, {setType(3268), setAmount(1), setPrice(8), "Do you want to buy a hand axe for %P gold?", setTopic(1)}},
	{{"spear"}, {setType(3277), setAmount(1), setPrice(10), "Do you want to buy a spear for %P gold?", setTopic(1)}},
	{{"rapier"}, {setType(3272), setAmount(1), setPrice(15), "Do you want to buy a rapier for %P gold?", setTopic(1)}},
	{{"axe"}, {setType(3274), setAmount(1), setPrice(20), "Do you want to buy an axe for %P gold?", setTopic(1)}},
	{{"sabre"}, {setType(3273), setAmount(1), setPrice(35), "Do you want to buy a sabre for %P gold?", setTopic(1)}},
	{{"sword"}, {setType(3264), setAmount(1), setPrice(85), "Do you want to buy a sword for %P gold?", setTopic(1)}},
	{{"mace"}, {setType(3286), setAmount(1), setPrice(90), "Do you want to buy a mace for %P gold?", setTopic(1)}},
	{{"battle", "hammer"}, {setType(3305), setAmount(1), setPrice(350), "Do you want to buy a battle hammer for %P gold?", setTopic(1)}},
	{{"throwing", "knife"}, {setType(3298), setAmount(1), setPrice(25), "Do you want to buy a throwing knife for %P gold?", setTopic(1)}},

	{{"chain", "armor"}, {setType(3358), setAmount(1), setPrice(200), "Do you want to buy a chain armor for %P gold?", setTopic(1)}},
	{{"brass", "armor"}, {setType(3359), setAmount(1), setPrice(450), "Do you want to buy a brass armor for %P gold?", setTopic(1)}},
	{{"leather", "armor"}, {setType(3361), setAmount(1), setPrice(35), "Do you want to buy a leather armor for %P gold?", setTopic(1)}},
	{{"chain", "helmet"}, {setType(3352), setAmount(1), setPrice(52), "Do you want to buy a chain helmet for %P gold?", setTopic(1)}},
	{{"leather", "helmet"}, {setType(3355), setAmount(1), setPrice(12), "Do you want to buy a leather helmet for %P gold?", setTopic(1)}},
	{{"steel", "shield"}, {setType(3409), setAmount(1), setPrice(240), "Do you want to buy a steel shield for %P gold?", setTopic(1)}},
	{{"wooden", "shield"}, {setType(3412), setAmount(1), setPrice(15), "Do you want to buy a wooden shield for %P gold?", setTopic(1)}},
	{{"chain", "legs"}, {setType(3558), setAmount(1), setPrice(80), "Do you want to buy chain legs for %P gold?", setTopic(1)}},

	{{P1, lt(1, P1), "dagger"}, {setType(3267), setAmount(P1), setPrice(mul(5, P1)), "Do you want to buy %A daggers for %P gold?", setTopic(1)}},
	{{P1, lt(1, P1), "hand", "axe"}, {setType(3268), setAmount(P1), setPrice(mul(8, P1)), "Do you want to buy %A hand axes for %P gold?", setTopic(1)}},
	{{P1, lt(1, P1), "spear"}, {setType(3277), setAmount(P1), setPrice(mul(10, P1)), "Do you want to buy %A spears for %P gold?", setTopic(1)}},
	{{P1, lt(1, P1), "rapier"}, {setType(3272), setAmount(P1), setPrice(mul(15, P1)), "Do you want to buy %A rapiers for %P gold?", setTopic(1)}},
	{{P1, lt(1, P1), "axe"}, {setType(3274), setAmount(P1), setPrice(mul(20, P1)), "Do you want to buy %A axes for %P gold?", setTopic(1)}},
	{{P1, lt(1, P1), "sabre"}, {setType(3273), setAmount(P1), setPrice(mul(35, P1)), "Do you want to buy %A sabres for %P gold?", setTopic(1)}},
	{{P1, lt(1, P1), "sword"}, {setType(3264), setAmount(P1), setPrice(mul(85, P1)), "Do you want to buy %A swords for %P gold?", setTopic(1)}},
	{{P1, lt(1, P1), "mace"}, {setType(3286), setAmount(P1), setPrice(mul(90, P1)), "Do you want to buy %A maces for %P gold?", setTopic(1)}},
	{{P1, lt(1, P1), "battle", "hammer"}, {setType(3305), setAmount(P1), setPrice(mul(350, P1)), "Do you want to buy %A battle hammers for %P gold?", setTopic(1)}},
	{{P1, lt(1, P1), "throwing", "kni"}, {setType(3298), setAmount(P1), setPrice(mul(25, P1)), "Do you want to buy %A throwing knives for %P gold?", setTopic(1)}},

	{{P1, lt(1, P1), "chain", "armor"}, {setType(3358), setAmount(P1), setPrice(mul(200, P1)), "Do you want to buy %A chain armors for %P gold?", setTopic(1)}},
	{{P1, lt(1, P1), "brass", "armor"}, {setType(3359), setAmount(P1), setPrice(mul(450, P1)), "Do you want to buy %A brass armors for %P gold?", setTopic(1)}},
	{{P1, lt(1, P1), "leather", "armor"}, {setType(3361), setAmount(P1), setPrice(mul(35, P1)), "Do you want to buy %A leather armors for %P gold?", setTopic(1)}},
	{{P1, lt(1, P1), "chain", "helmet"}, {setType(3352), setAmount(P1), setPrice(mul(52, P1)), "Do you want to buy %A chain helmets for %P gold?", setTopic(1)}},
	{{P1, lt(1, P1), "leather", "helmet"}, {setType(3355), setAmount(P1), setPrice(mul(12, P1)), "Do you want to buy %A leather helmets for %P gold?", setTopic(1)}},
	{{P1, lt(1, P1), "steel", "shield"}, {setType(3409), setAmount(P1), setPrice(mul(240, P1)), "Do you want to buy %A steel shields for %P gold?", setTopic(1)}},
	{{P1, lt(1, P1), "wooden", "shield"}, {setType(3412), setAmount(P1), setPrice(mul(15, P1)), "Do you want to buy %A wooden shields for %P gold?", setTopic(1)}},
	{{P1, lt(1, P1), "chain", "legs"}, {setType(3558), setAmount(P1), setPrice(mul(80, P1)), "Do you want to buy %A chain legs for %P gold?", setTopic(1)}},

	{{"sell", "two", "handed", "sword"}, {setType(3265), setAmount(1), setPrice(450), "Do you want to sell a two handed sword for %P gold?", setTopic(2)}},
	{{"sell", "battle", "axe"}, {setType(3266), setAmount(1), setPrice(80), "Do you want to sell a battle axe for %P gold?", setTopic(2)}},
	{{"sell", "dagger"}, {setType(3267), setAmount(1), setPrice(2), "Do you want to sell a dagger for %P gold?", setTopic(2)}},
	{{"sell", "hand", "axe"}, {setType(3268), setAmount(1), setPrice(4), "Do you want to sell a hand axe for %P gold?", setTopic(2)}},
	{{"sell", "halberd"}, {setType(3269), setAmount(1), setPrice(400), "Do you want to sell a halberd for %P gold?", setTopic(2)}},
	{{"sell", "rapier"}, {setType(3272), setAmount(1), setPrice(5), "Do you want to sell a rapier for %P gold?", setTopic(2)}},
	{{"sell", "sabre"}, {setType(3273), setAmount(1), setPrice(12), "Do you want to sell a sabre for %P gold?", setTopic(2)}},
	{{"sell", "axe"}, {setType(3274), setAmount(1), setPrice(7), "Do you want to sell an axe for %P gold?", setTopic(2)}},
	{{"sell", "spear"}, {setType(3277), setAmount(1), setPrice(3), "Do you want to sell a spear for %P gold?", setTopic(2)}},
	{{"sell", "morning", "star"}, {setType(3282), setAmount(1), setPrice(90), "Do you want to sell a morning star for %P gold?", setTopic(2)}},
	{{"sell", "mace"}, {setType(3286), setAmount(1), setPrice(30), "Do you want to sell a mace for %P gold?", setTopic(2)}},
	{{"sell", "short", "sword"}, {setType(3294), setAmount(1), setPrice(10), "Do you want to sell a short sword for %P gold?", setTopic(2)}},
	{{"sell", "battle", "hammer"}, {setType(3305), setAmount(1), setPrice(120), "Do you want to sell a battle hammer for %P gold?", setTopic(2)}},
	{{"sell", "sword"}, {setType(3264), setAmount(1), setPrice(25), "Do you want to sell a sword for %P gold?", setTopic(2)}},

	{{"sell", "leather", "armor"}, {setType(3361), setAmount(1), setPrice(12), "Do you want to sell a leather armor for %P gold?", setTopic(2)}},
	{{"sell", "chain", "armor"}, {setType(3358), setAmount(1), setPrice(70), "Do you want to sell a chain armor for %P gold?", setTopic(2)}},
	{{"sell", "brass", "armor"}, {setType(3359), setAmount(1), setPrice(150), "Do you want to sell a brass armor for %P gold?", setTopic(2)}},
	{{"sell", "plate", "armor"}, {setType(3357), setAmount(1), setPrice(400), "Do you want to sell a plate armor for %P gold?", setTopic(2)}},
	{{"sell", "chain", "legs"}, {setType(3558), setAmount(1), setPrice(25), "Do you want to sell chain legs for %P gold?", setTopic(2)}},
	{{"sell", "leather", "helmet"}, {setType(3355), setAmount(1), setPrice(4), "Do you want to sell a leather helmet for %P gold?", setTopic(2)}},
	{{"sell", "chain", "helmet"}, {setType(3352), setAmount(1), setPrice(17), "Do you want to sell a chain helmet for %P gold?", setTopic(2)}},
	{{"sell", "steel", "helmet"}, {setType(3351), setAmount(1), setPrice(190), "Do you want to sell a steel helmet for %P gold?", setTopic(2)}},
	{{"sell", "wooden", "shield"}, {setType(3412), setAmount(1), setPrice(5), "Do you want to sell a wooden shield for %P gold?", setTopic(2)}},
	{{"sell", "battle", "shield"}, {setType(3413), setAmount(1), setPrice(95), "Do you want to sell a battle shield for %P gold?", setTopic(2)}},
	{{"sell", "steel", "shield"}, {setType(3409), setAmount(1), setPrice(80), "Do you want to sell a steel shield for %P gold?", setTopic(2)}},

	{{"sell", P1, lt(1, P1), "two", "handed", "sword"}, {setType(3265), setAmount(P1), setPrice(mul(450, P1)), "Do you want to sell %A two handed swords for %P gold?", setTopic(2)}},
	{{"sell", P1, lt(1, P1), "battle", "axe"}, {setType(3266), setAmount(P1), setPrice(mul(80, P1)), "Do you want to sell %A battle axes for %P gold?", setTopic(2)}},
	{{"sell", P1, lt(1, P1), "dagger"}, {setType(3267), setAmount(P1), setPrice(mul(2, P1)), "Do you want to sell %A daggers for %P gold?", setTopic(2)}},
	{{"sell", P1, lt(1, P1), "hand", "axe"}, {setType(3268), setAmount(P1), setPrice(mul(4, P1)), "Do you want to sell %A hand axes for %P gold?", setTopic(2)}},
	{{"sell", P1, lt(1, P1), "halberd"}, {setType(3269), setAmount(P1), setPrice(mul(400, P1)), "Do you want to sell %A halberds for %P gold?", setTopic(2)}},
	{{"sell", P1, lt(1, P1), "rapier"}, {setType(3272), setAmount(P1), setPrice(mul(5, P1)), "Do you want to sell %A rapiers for %P gold?", setTopic(2)}},
	{{"sell", P1, lt(1, P1), "sabre"}, {setType(3273), setAmount(P1), setPrice(mul(12, P1)), "Do you want to sell %A sabres for %P gold?", setTopic(2)}},
	{{"sell", P1, lt(1, P1), "axe"}, {setType(3274), setAmount(P1), setPrice(mul(7, P1)), "Do you want to sell %A axes for %P gold?", setTopic(2)}},
	{{"sell", P1, lt(1, P1), "spear"}, {setType(3277), setAmount(P1), setPrice(mul(3, P1)), "Do you want to sell %A spears for %P gold?", setTopic(2)}},
	{{"sell", P1, lt(1, P1), "morning", "star"}, {setType(3282), setAmount(P1), setPrice(mul(90, P1)), "Do you want to sell %A morning stars for %P gold?", setTopic(2)}},
	{{"sell", P1, lt(1, P1), "mace"}, {setType(3286), setAmount(P1), setPrice(mul(30, P1)), "Do you want to sell %A maces for %P gold?", setTopic(2)}},
	{{"sell", P1, lt(1, P1), "short", "sword"}, {setType(3294), setAmount(P1), setPrice(mul(10, P1)), "Do you want to sell %A short swords for %P gold?", setTopic(2)}},
	{{"sell", P1, lt(1, P1), "battle", "hammer"}, {setType(3305), setAmount(P1), setPrice(mul(120, P1)), "Do you want to sell %A battle hammers for %P gold?", setTopic(2)}},
	{{"sell", P1, lt(1, P1), "sword"}, {setType(3264), setAmount(P1), setPrice(mul(25, P1)), "Do you want to sell %A swords for %P gold?", setTopic(2)}},

	{{"sell", P1, lt(1, P1), "leather", "armor"}, {setType(3361), setAmount(P1), setPrice(mul(12, P1)), "Do you want to sell %A leather armors for %P gold?", setTopic(2)}},
	{{"sell", P1, lt(1, P1), "chain", "armor"}, {setType(3358), setAmount(P1), setPrice(mul(70, P1)), "Do you want to sell %A chain armors for %P gold?", setTopic(2)}},
	{{"sell", P1, lt(1, P1), "brass", "armor"}, {setType(3359), setAmount(P1), setPrice(mul(150, P1)), "Do you want to sell %A brass armors for %P gold?", setTopic(2)}},
	{{"sell", P1, lt(1, P1), "plate", "armor"}, {setType(3357), setAmount(P1), setPrice(mul(400, P1)), "Do you want to sell %A plate armors for %P gold?", setTopic(2)}},
	{{"sell", P1, lt(1, P1), "chain", "legs"}, {setType(3558), setAmount(P1), setPrice(mul(25, P1)), "Do you want to sell %A chain legs for %P gold?", setTopic(2)}},
	{{"sell", P1, lt(1, P1), "leather", "helmet"}, {setType(3355), setAmount(P1), setPrice(mul(4, P1)), "Do you want to sell %A leather helmets for %P gold?", setTopic(2)}},
	{{"sell", P1, lt(1, P1), "chain", "helmet"}, {setType(3352), setAmount(P1), setPrice(mul(17, P1)), "Do you want to sell %A chain helmets for %P gold?", setTopic(2)}},
	{{"sell", P1, lt(1, P1), "steel", "helmet"}, {setType(3351), setAmount(P1), setPrice(mul(190, P1)), "Do you want to sell %A steel helmets for %P gold?", setTopic(2)}},
	{{"sell", P1, lt(1, P1), "wooden", "shield"}, {setType(3412), setAmount(P1), setPrice(mul(5, P1)), "Do you want to sell %A wooden shields for %P gold?", setTopic(2)}},
	{{"sell", P1, lt(1, P1), "battle", "shield"}, {setType(3413), setAmount(P1), setPrice(mul(95, P1)), "Do you want to sell %A battle shields for %P gold?", setTopic(2)}},
	{{"sell", P1, lt(1, P1), "steel", "shield"}, {setType(3409), setAmount(P1), setPrice(mul(80, P1)), "Do you want to sell %A steel shields for %P gold?", setTopic(2)}},

	{{eq(TOPIC, 1), "yes", ge(COUNTMONEY, PRICE)}, {"Thank you. Here it is.", deleteMoney, create(TYPE)}},
	{{eq(TOPIC, 1), "yes"}, {"Sorry, you do not have enough gold."}},
	{{eq(TOPIC, 1)}, {"Maybe you will buy it another time."}},

	{{eq(TOPIC, 2), "yes", ge(count(TYPE), AMOUNT)}, {"Ok. Here is your money.", delete(TYPE), createMoney}},
	{{eq(TOPIC, 2), "yes"}, {"Sorry, you do not have one."}},
	{{eq(TOPIC, 2), "yes", gt(AMOUNT, 1)}, {"Sorry, you do not have so many."}},
	{{eq(TOPIC, 2)}, {"Maybe next time."}},

	{{"sell", "magic", "plate", "armor"}, {"WOW! Do you really want to sell me a MAGIC plate armor?", setTopic(3)}},
	{{eq(TOPIC, 3), "yes"}, {setType(3366), setAmount(1), setPrice(6400), "Oh, unbelievable! I would pay %P gold for this wonderful piece of armor. Are you still interested?", setTopic(4)}},
	{{eq(TOPIC, 3)}, {"Hmmm, what a pity! I am looking for such an armor since I live in Thais."}},
	{{eq(TOPIC, 4), "yes", ge(count(TYPE), AMOUNT)}, {"Finally it is mine! Here is your money. Can I be of any further help?", delete(TYPE), createMoney}},
	{{eq(TOPIC, 4), "yes"}, {"Argl! You do not have one! Trying to tease me? Get lost or I call the guards!", idle}},
	{{eq(TOPIC, 4)}, {"Maybe my offer is too low? Unfortunately I can not bring up more money, I am just a smith."}},

	{{"backpack", gt(questValue(289), 0)}, {"Yes, you brought back my old backpack. Thank you again."}},
	{{"backpack", lt(questValue(289), 1)}, {setType(3244), setAmount(1), "What? Are you telling me you found my old adventurer's backpack that I lost years ago??", setTopic(5)}},

	{{eq(TOPIC, 5), "yes", ge(count(TYPE), AMOUNT)}, {"Thank you verry much! This brings back good old memories! Please, as a reward, travel to kazordoon and ask my old friend Kroox to provide you a special dwarven armor. ...", "I will mail him about you immediately. Just tell him, his old buddy sam is sending you.", delete(TYPE), setQuestValue(289, 1)}},

	{{eq(TOPIC, 5), "yes", lt(count(TYPE), AMOUNT)}, {"No, you don't have my old backpack. What a pity."}},
	{{eq(TOPIC, 5), "no"}, {"What a pity."}},
}

function onCreatureSay(npc, creature, text, situation)
	behaviour.react(npc, rules, text, situation)
end

function onCreatureDisappear(npc, creature)
	behaviour.react(npc, rules, "", NPC_SITUATION_VANISH)
end
