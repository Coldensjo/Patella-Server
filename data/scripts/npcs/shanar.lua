-- npcs/shanar.lua: the behaviour of the NPC Shanar, converted from
-- shanar.npc by patella-convert. The NPC is its [[npc]] entry in npcs.toml;
-- lib/npc.lua matches and runs the rules below, in the order of the file
-- (scripting.hh, "NPCs"). The comments are those of shanar.npc.
--
-- GIMUD - Graphical Interface Multi User Dungeon
-- shanar.npc: Datenbank für den Schmied Shanar (Elfenstadt)

local behaviour = require("lib.npc")
local ADDRESS = behaviour.ADDRESS
local BUSY = behaviour.BUSY
local VANISH = behaviour.VANISH
local DRUID = behaviour.DRUID
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
local LEVEL = behaviour.LEVEL
local HP = behaviour.HP
local BURNING = behaviour.BURNING
local POISON = behaviour.POISON
local COUNTMONEY = behaviour.COUNTMONEY
local count = behaviour.count
local spellKnown = behaviour.spellKnown
local spellLevel = behaviour.spellLevel
local mul = behaviour.mul
local setTopic = behaviour.setTopic
local setPrice = behaviour.setPrice
local setAmount = behaviour.setAmount
local setType = behaviour.setType
local setHp = behaviour.setHp
local burning = behaviour.burning
local poison = behaviour.poison
local effectOpp = behaviour.effectOpp
local teachSpell = behaviour.teachSpell
local create = behaviour.create
local delete = behaviour.delete
local createMoney = behaviour.createMoney
local deleteMoney = behaviour.deleteMoney
local queue = behaviour.queue
local idle = behaviour.idle
local nop = behaviour.nop
local previous = behaviour.previous

local rules = behaviour.rules {
	{{ADDRESS, "hello$", FORCE}, {"Ashari %N."}},
	{{ADDRESS, "hi$", FORCE}, {previous}},
	{{ADDRESS, "ashari$", FORCE}, {previous}},
	{{ADDRESS, FORCE}, {idle}},
	{{BUSY, "hello$", FORCE}, {"Wait, %N.", queue}},
	{{BUSY, "hi$", FORCE}, {previous}},
	{{BUSY, "ashari$", FORCE}, {previous}},
	{{BUSY, FORCE}, {nop}},
	{{VANISH, FORCE}, {"Asha Thrazi."}},

	{{"bye"}, {"Asha Thrazi.", idle}},
	{{"asha", "thrazi"}, {previous}},
	{{"farewell"}, {previous}},
	{{"job"}, {"I sell weapons, shields, and armor, and teach protective spells."}},
	{{"name"}, {"I am Shanar Ethkal."}},
	{{"time"}, {"I don't care."}},

	{{"carlin"}, {"Carlin is quite close and we have some trade now and then."}},
	{{"thais"}, {"Thais is a town of humans far away."}},
	{{"venore"}, {"Those human merchants try to sell their low quality weapons and armor here to ruin my trade."}},
	{{"roderick"}, {"He is some human who lives in a stone house in the south of the town."}},
	{{"olrik"}, {"This human is sneaking around in the tow far too often."}},

	{{"elves"}, {"That's our race, so what?"}},
	{{"dwarfs"}, {"Ugly and dirty."}},
	{{"humans"}, {"Loud and noisy."}},
	{{"troll"}, {"I own only a few."}},
	{{"cenath"}, {"Idiots."}},
	{{"kuridai"}, {"That's my caste."}},
	{{"deraisim"}, {"Squirrels."}},
	{{"abdaisim"}, {"They don't live here."}},
	{{"teshial"}, {"Don't know much about them"}},
	{{"ferumbras"}, {"A danger to all."}},
	{{"crunor"}, {"I don't care about gods."}},
	{{"excalibug"}, {"Perhaps more than a myth."}},
	{{"news"}, {"Nothing I want to talk about."}},
	{{"magic"}, {"I teach some spells of protection."}},
	{{"druid"}, {"Druids are great healers."}},
	{{"sorcerer"}, {"They understand so few..."}},

	{{"sell", "coat"}, {setType(3562), setAmount(1), setPrice(1), "Do you want to sell a coat for %P gold?", setTopic(2)}},
	{{"sell", "jacket"}, {setType(3561), setAmount(1), setPrice(1), "Do you want to sell a jacket for %P gold?", setTopic(2)}},
	{{"sell", "knight", "armor"}, {setType(3370), setAmount(1), setPrice(875), "Do you want to sell a knight's armor for %P gold?", setTopic(2)}},
	{{"sell", "golden", "armor"}, {setType(3360), setAmount(1), setPrice(1500), "Do you want to sell a golden armor for %P gold?", setTopic(2)}},
	{{"sell", "leather", "armor"}, {setType(3361), setAmount(1), setPrice(12), "Do you want to sell a leather armor for %P gold?", setTopic(2)}},
	{{"sell", "chain", "armor"}, {setType(3358), setAmount(1), setPrice(70), "Do you want to sell a chain armor for %P gold?", setTopic(2)}},
	{{"sell", "brass", "armor"}, {setType(3359), setAmount(1), setPrice(150), "Do you want to sell a brass armor for %P gold?", setTopic(2)}},
	{{"sell", "plate", "armor"}, {setType(3357), setAmount(1), setPrice(400), "Do you want to sell a plate armor for %P gold?", setTopic(2)}},
	{{"sell", "leather", "helmet"}, {setType(3355), setAmount(1), setPrice(4), "Do you want to sell a leather helmet for %P gold?", setTopic(2)}},
	{{"sell", "chain", "helmet"}, {setType(3352), setAmount(1), setPrice(17), "Do you want to sell a chain helmet for %P gold?", setTopic(2)}},
	{{"sell", "steel", "helmet"}, {setType(3351), setAmount(1), setPrice(190), "Do you want to sell a steel helmet for %P gold?", setTopic(2)}},
	{{"sell", "brass", "helmet"}, {setType(3354), setAmount(1), setPrice(30), "Do you want to sell a brass helmet for %P gold?", setTopic(2)}},
	{{"sell", "viking", "helmet"}, {setType(3367), setAmount(1), setPrice(66), "Do you want to sell a viking helmet for %P gold?", setTopic(2)}},
	{{"sell", "iron", "helmet"}, {setType(3353), setAmount(1), setPrice(145), "Do you want to sell an iron helmet for %P gold?", setTopic(2)}},
	{{"sell", "devil", "helmet"}, {setType(3356), setAmount(1), setPrice(450), "Do you want to sell a devil's helmet for %P gold?", setTopic(2)}},
	{{"sell", "warrior", "helmet"}, {setType(3369), setAmount(1), setPrice(696), "Do you want to sell a warrior's helmet for %P gold?", setTopic(2)}},
	{{"sell", "leather", "legs"}, {setType(3559), setAmount(1), setPrice(1), "Do you want to sell leather legs for %P gold?", setTopic(2)}},
	{{"sell", "chain", "legs"}, {setType(3558), setAmount(1), setPrice(20), "Do you want to sell chain legs for %P gold?", setTopic(2)}},
	{{"sell", "brass", "legs"}, {setType(3372), setAmount(1), setPrice(49), "Do you want to sell brass legs for %P gold?", setTopic(2)}},
	{{"sell", "plate", "legs"}, {setType(3557), setAmount(1), setPrice(115), "Do you want to sell plate legs for %P gold?", setTopic(2)}},
	{{"sell", "knight", "legs"}, {setType(3371), setAmount(1), setPrice(375), "Do you want to sell knight's legs for %P gold?", setTopic(2)}},

	{{"sell", P1, lt(1, P1), "coat"}, {setType(3562), setAmount(P1), setPrice(mul(1, P1)), "Do you want to sell %A coats for %P gold?", setTopic(2)}},
	{{"sell", P1, lt(1, P1), "jacket"}, {setType(3561), setAmount(P1), setPrice(mul(1, P1)), "Do you want to sell %A jackets for %P gold?", setTopic(2)}},
	{{"sell", P1, lt(1, P1), "knight", "armor"}, {setType(3370), setAmount(P1), setPrice(mul(875, P1)), "Do you want to sell %A knight's armors for %P gold?", setTopic(2)}},
	{{"sell", P1, lt(1, P1), "golden", "armor"}, {setType(3360), setAmount(P1), setPrice(mul(1500, P1)), "Do you want to sell %A golden armors for %P gold?", setTopic(2)}},
	{{"sell", P1, lt(1, P1), "leather", "armor"}, {setType(3361), setAmount(P1), setPrice(mul(12, P1)), "Do you want to sell %A leather armors for %P gold?", setTopic(2)}},
	{{"sell", P1, lt(1, P1), "chain", "armor"}, {setType(3358), setAmount(P1), setPrice(mul(70, P1)), "Do you want to sell %A chain armors for %P gold?", setTopic(2)}},
	{{"sell", P1, lt(1, P1), "brass", "armor"}, {setType(3359), setAmount(P1), setPrice(mul(150, P1)), "Do you want to sell %A brass armors for %P gold?", setTopic(2)}},
	{{"sell", P1, lt(1, P1), "plate", "armor"}, {setType(3357), setAmount(P1), setPrice(mul(400, P1)), "Do you want to sell %A plate armors for %P gold?", setTopic(2)}},
	{{"sell", P1, lt(1, P1), "leather", "helmet"}, {setType(3355), setAmount(P1), setPrice(mul(4, P1)), "Do you want to sell %A leather helmets for %P gold?", setTopic(2)}},
	{{"sell", P1, lt(1, P1), "chain", "helmet"}, {setType(3352), setAmount(P1), setPrice(mul(17, P1)), "Do you want to sell %A chain helmets for %P gold?", setTopic(2)}},
	{{"sell", P1, lt(1, P1), "steel", "helmet"}, {setType(3351), setAmount(P1), setPrice(mul(190, P1)), "Do you want to sell %A steel helmets for %P gold?", setTopic(2)}},
	{{"sell", P1, lt(1, P1), "brass", "helmet"}, {setType(3354), setAmount(P1), setPrice(mul(30, P1)), "Do you want to sell %A brass helmets for %P gold?", setTopic(2)}},
	{{"sell", P1, lt(1, P1), "viking", "helmet"}, {setType(3367), setAmount(P1), setPrice(mul(66, P1)), "Do you want to sell %A viking helmets for %P gold?", setTopic(2)}},
	{{"sell", P1, lt(1, P1), "iron", "helmet"}, {setType(3353), setAmount(P1), setPrice(mul(145, P1)), "Do you want to sell %A iron helmets for %P gold?", setTopic(2)}},
	{{"sell", P1, lt(1, P1), "devil", "helmet"}, {setType(3356), setAmount(P1), setPrice(mul(450, P1)), "Do you want to sell %A devil's helmets for %P gold?", setTopic(2)}},
	{{"sell", P1, lt(1, P1), "warrior", "helmet"}, {setType(3369), setAmount(P1), setPrice(mul(696, P1)), "Do you want to sell %A warrior's helmets for %P gold?", setTopic(2)}},
	{{"sell", P1, lt(1, P1), "leather", "legs"}, {setType(3559), setAmount(P1), setPrice(mul(1, P1)), "Do you want to sell %A leather legs for %P gold?", setTopic(2)}},
	{{"sell", P1, lt(1, P1), "chain", "legs"}, {setType(3558), setAmount(P1), setPrice(mul(20, P1)), "Do you want to sell %A chain legs for %P gold?", setTopic(2)}},
	{{"sell", P1, lt(1, P1), "brass", "legs"}, {setType(3372), setAmount(P1), setPrice(mul(49, P1)), "Do you want to sell %A brass legs for %P gold?", setTopic(2)}},
	{{"sell", P1, lt(1, P1), "plate", "legs"}, {setType(3557), setAmount(P1), setPrice(mul(115, P1)), "Do you want to sell %A plate legs for %P gold?", setTopic(2)}},
	{{"sell", P1, lt(1, P1), "knight", "legs"}, {setType(3371), setAmount(P1), setPrice(mul(375, P1)), "Do you want to sell %A knight's legs for %P gold?", setTopic(2)}},

	{{"sell", "wooden", "shield"}, {setType(3412), setAmount(1), setPrice(5), "Do you want to sell a wooden shield for %P gold?", setTopic(2)}},
	{{"sell", "battle", "shield"}, {setType(3413), setAmount(1), setPrice(95), "Do you want to sell a battle shield for %P gold?", setTopic(2)}},
	{{"sell", "steel", "shield"}, {setType(3409), setAmount(1), setPrice(80), "Do you want to sell a steel shield for %P gold?", setTopic(2)}},
	{{"sell", "brass", "shield"}, {setType(3411), setAmount(1), setPrice(16), "Do you want to sell a brass shield for %P gold?", setTopic(2)}},
	{{"sell", "plate", "shield"}, {setType(3410), setAmount(1), setPrice(31), "Do you want to sell a plate shield for %P gold?", setTopic(2)}},
	{{"sell", "guardians", "shield"}, {setType(3415), setAmount(1), setPrice(180), "Do you want to sell a guardian's shield for %P gold?", setTopic(2)}},
	{{"sell", "dragon", "shield"}, {setType(3416), setAmount(1), setPrice(360), "Do you want to sell a dragon shield for %P gold?", setTopic(2)}},

	{{"sell", P1, lt(1, P1), "wooden", "shield"}, {setType(3412), setAmount(P1), setPrice(mul(5, P1)), "Do you want to sell %A wooden shields for %P gold?", setTopic(2)}},
	{{"sell", P1, lt(1, P1), "battle", "shield"}, {setType(3413), setAmount(P1), setPrice(mul(95, P1)), "Do you want to sell %A battle shields for %P gold?", setTopic(2)}},
	{{"sell", P1, lt(1, P1), "steel", "shield"}, {setType(3409), setAmount(P1), setPrice(mul(80, P1)), "Do you want to sell %A steel shields for %P gold?", setTopic(2)}},
	{{"sell", P1, lt(1, P1), "brass", "shield"}, {setType(3411), setAmount(P1), setPrice(mul(16, P1)), "Do you want to sell %A brass shields for %P gold?", setTopic(2)}},
	{{"sell", P1, lt(1, P1), "plate", "shield"}, {setType(3410), setAmount(P1), setPrice(mul(31, P1)), "Do you want to sell %A plate shields for %P gold?", setTopic(2)}},
	{{"sell", P1, lt(1, P1), "guardians", "shield"}, {setType(3415), setAmount(P1), setPrice(mul(180, P1)), "Do you want to sell %A guardian's shields for %P gold?", setTopic(2)}},
	{{"sell", P1, lt(1, P1), "dragon", "shield"}, {setType(3416), setAmount(P1), setPrice(mul(360, P1)), "Do you want to sell %A dragon shields for %P gold?", setTopic(2)}},

	{{"sell", "two", "handed", "sword"}, {setType(3265), setAmount(1), setPrice(450), "Do you want to sell a two handed sword for %P gold?", setTopic(2)}},
	{{"sell", "longsword"}, {setType(3285), setAmount(1), setPrice(51), "Do you want to sell a longsword for %P gold?", setTopic(2)}},
	{{"sell", "dagger"}, {setType(3267), setAmount(1), setPrice(2), "Do you want to sell this garbage? I give you %P gold, ok?", setTopic(2)}},
	{{"sell", "club"}, {setType(3270), setAmount(1), setPrice(1), "Are you sure, you want to sell this garbage? Hmm, I give you %P gold, ok?", setTopic(2)}},
	{{"sell", "rapier"}, {setType(3272), setAmount(1), setPrice(5), "Do you want to sell a rapier for %P gold?", setTopic(2)}},
	{{"sell", "sabre"}, {setType(3273), setAmount(1), setPrice(12), "Do you want to sell a sabre for %P gold?", setTopic(2)}},
	{{"sell", "spear"}, {setType(3277), setAmount(1), setPrice(3), "Do you want to sell this garbage? I give you %P gold, ok?", setTopic(2)}},
	{{"sell", "short", "sword"}, {setType(3294), setAmount(1), setPrice(10), "Do you want to sell a short sword for %P gold?", setTopic(2)}},
	{{"sell", "sword"}, {setType(3264), setAmount(1), setPrice(25), "Do you want to sell a sword for %P gold?", setTopic(2)}},
	{{"sell", "spike", "sword"}, {setType(3271), setAmount(1), setPrice(225), "Do you want to sell a spike sword for %P gold?", setTopic(2)}},
	{{"sell", "fire", "sword"}, {setType(3280), setAmount(1), setPrice(1000), "Do you want to sell a fire sword for %P gold?", setTopic(2)}},
	{{"sell", "mace"}, {setType(3286), setAmount(1), setPrice(23), "Do you want to sell a mace for %P gold?", setTopic(2)}},

	{{"sell", P1, lt(1, P1), "two", "handed", "sword"}, {setType(3265), setAmount(P1), setPrice(mul(450, P1)), "Do you want to sell %A two handed swords for %P gold?", setTopic(2)}},
	{{"sell", P1, lt(1, P1), "longsword"}, {setType(3285), setAmount(P1), setPrice(mul(51, P1)), "Do you want to sell %A longswords for %P gold?", setTopic(2)}},
	{{"sell", P1, lt(1, P1), "dagger"}, {setType(3267), setAmount(P1), setPrice(mul(2, P1)), "Do you want to sell this garbage? I give you %P gold, ok?", setTopic(2)}},
	{{"sell", P1, lt(1, P1), "club"}, {setType(3270), setAmount(P1), setPrice(mul(1, P1)), "Are you sure, you want to sell this garbage? Hmm, I give you %P gold, ok?", setTopic(2)}},
	{{"sell", P1, lt(1, P1), "rapier"}, {setType(3272), setAmount(P1), setPrice(mul(5, P1)), "Do you want to sell %A rapiers for %P gold?", setTopic(2)}},
	{{"sell", P1, lt(1, P1), "sabre"}, {setType(3273), setAmount(P1), setPrice(mul(12, P1)), "Do you want to sell %A sabres for %P gold?", setTopic(2)}},
	{{"sell", P1, lt(1, P1), "spear"}, {setType(3277), setAmount(P1), setPrice(mul(3, P1)), "Do you want to sell this garbage? I give you %P gold, ok?", setTopic(2)}},
	{{"sell", P1, lt(1, P1), "short", "sword"}, {setType(3294), setAmount(P1), setPrice(mul(10, P1)), "Do you want to sell %A short swords for %P gold?", setTopic(2)}},
	{{"sell", P1, lt(1, P1), "sword"}, {setType(3264), setAmount(P1), setPrice(mul(25, P1)), "Do you want to sell %A swords for %P gold?", setTopic(2)}},
	{{"sell", P1, lt(1, P1), "spike", "sword"}, {setType(3271), setAmount(P1), setPrice(mul(225, P1)), "Do you want to sell %A spike swords for %P gold?", setTopic(2)}},
	{{"sell", P1, lt(1, P1), "fire", "sword"}, {setType(3280), setAmount(P1), setPrice(mul(1000, P1)), "Do you want to sell %A fire swords for %P gold?", setTopic(2)}},
	{{"sell", P1, lt(1, P1), "mace"}, {setType(3286), setAmount(P1), setPrice(mul(23, P1)), "Do you want to sell %A maces for %P gold?", setTopic(2)}},

	{{"dagger"}, {setType(3267), setAmount(1), setPrice(5), "Do you want to buy a dagger for %P gold?", setTopic(1)}},
	{{"spear"}, {setType(3277), setAmount(1), setPrice(10), "Do you want to buy a spear for %P gold?", setTopic(1)}},
	{{"rapier"}, {setType(3272), setAmount(1), setPrice(15), "Do you want to buy a rapier for %P gold?", setTopic(1)}},
	{{"sabre"}, {setType(3273), setAmount(1), setPrice(35), "Do you want to buy a sabre for %P gold?", setTopic(1)}},
	{{"sword"}, {setType(3264), setAmount(1), setPrice(85), "Do you want to buy a sword for %P gold?", setTopic(1)}},
	{{"staff"}, {setType(3289), setAmount(1), setPrice(40), "Do you want to buy it for %P gold?", setTopic(1)}},
	{{"longsword"}, {setType(3285), setAmount(1), setPrice(160), "Do you want to buy it for %P gold?", setTopic(1)}},
	{{"machete"}, {setType(3308), setAmount(1), setPrice(35), "Do you want to buy a machete for %P gold?", setTopic(1)}},
	{{"throwing", "knife"}, {setType(3298), setAmount(1), setPrice(25), "Do you want to buy a throwing knife for %P gold?", setTopic(1)}},

	{{P1, lt(1, P1), "spear"}, {setType(3277), setAmount(P1), setPrice(mul(10, P1)), "Do you want to buy %A spears for %P gold?", setTopic(1)}},
	{{P1, lt(1, P1), "rapier"}, {setType(3272), setAmount(P1), setPrice(mul(15, P1)), "Do you want to buy %A rapiers for %P gold?", setTopic(1)}},
	{{P1, lt(1, P1), "sabre"}, {setType(3273), setAmount(P1), setPrice(mul(35, P1)), "Do you want to buy %A sabres for %P gold?", setTopic(1)}},
	{{P1, lt(1, P1), "sword"}, {setType(3264), setAmount(P1), setPrice(mul(85, P1)), "Do you want to buy %A swords for %P gold?", setTopic(1)}},
	{{P1, lt(1, P1), "staff"}, {setType(3289), setAmount(P1), setPrice(mul(40, P1)), "Do you want to buy them for %P gold?", setTopic(1)}},
	{{P1, lt(1, P1), "longsword"}, {setType(3285), setAmount(P1), setPrice(mul(160, P1)), "Do you want to buy them for %P gold?", setTopic(1)}},
	{{P1, lt(1, P1), "machete"}, {setType(3308), setAmount(P1), setPrice(mul(35, P1)), "Do you want to buy %A machetes for %P gold?", setTopic(1)}},
	{{P1, lt(1, P1), "throwing", "kni"}, {setType(3298), setAmount(P1), setPrice(mul(25, P1)), "Do you want to buy %A throwing knives for %P gold?", setTopic(1)}},

	{{"chain", "armor"}, {setType(3358), setAmount(1), setPrice(200), "Do you want to buy a chain armor for %P gold?", setTopic(1)}},
	{{"brass", "armor"}, {setType(3359), setAmount(1), setPrice(450), "Do you want to buy a brass armor for %P gold?", setTopic(1)}},
	{{"leather", "armor"}, {setType(3361), setAmount(1), setPrice(35), "Do you want to buy a leather armor for %P gold?", setTopic(1)}},
	{{"chain", "helmet"}, {setType(3352), setAmount(1), setPrice(52), "Do you want to buy a chain helmet for %P gold?", setTopic(1)}},
	{{"leather", "helmet"}, {setType(3355), setAmount(1), setPrice(12), "Do you want to buy a leather helmet for %P gold?", setTopic(1)}},
	{{"steel", "shield"}, {setType(3409), setAmount(1), setPrice(240), "Do you want to buy a steel shield for %P gold?", setTopic(1)}},
	{{"wooden", "shield"}, {setType(3412), setAmount(1), setPrice(15), "Do you want to buy a wooden shield for %P gold?", setTopic(1)}},
	{{"chain", "legs"}, {setType(3558), setAmount(1), setPrice(80), "Do you want to buy chain legs for %P gold?", setTopic(1)}},

	{{P1, lt(1, P1), "chain", "armor"}, {setType(3358), setAmount(P1), setPrice(mul(200, P1)), "Do you want to buy %A chain armors for %P gold?", setTopic(1)}},
	{{P1, lt(1, P1), "brass", "armor"}, {setType(3359), setAmount(P1), setPrice(mul(450, P1)), "Do you want to buy %A brass armors for %P gold?", setTopic(1)}},
	{{P1, lt(1, P1), "leather", "armor"}, {setType(3361), setAmount(P1), setPrice(mul(35, P1)), "Do you want to buy %A leather armors for %P gold?", setTopic(1)}},
	{{P1, lt(1, P1), "chain", "helmet"}, {setType(3352), setAmount(P1), setPrice(mul(52, P1)), "Do you want to buy %A chain helmets for %P gold?", setTopic(1)}},
	{{P1, lt(1, P1), "leather", "helmet"}, {setType(3355), setAmount(P1), setPrice(mul(12, P1)), "Do you want to buy %A leather helmets for %P gold?", setTopic(1)}},
	{{P1, lt(1, P1), "steel", "shield"}, {setType(3409), setAmount(P1), setPrice(mul(240, P1)), "Do you want to buy %A steel shields for %P gold?", setTopic(1)}},
	{{P1, lt(1, P1), "wooden", "shield"}, {setType(3412), setAmount(P1), setPrice(mul(15, P1)), "Do you want to buy %A wooden shields for %P gold?", setTopic(1)}},
	{{P1, lt(1, P1), "chain", "legs"}, {setType(3558), setAmount(P1), setPrice(mul(80, P1)), "Do you want to buy %A chain legs for %P gold?", setTopic(1)}},

	{{eq(TOPIC, 1), "yes", ge(COUNTMONEY, PRICE)}, {"Thank you. Here it is.", deleteMoney, create(TYPE)}},
	{{eq(TOPIC, 1), "yes"}, {"Sorry, you do not have enough gold."}},
	{{eq(TOPIC, 1)}, {"Maybe you will buy it another time."}},

	{{eq(TOPIC, 2), "yes", ge(count(TYPE), AMOUNT)}, {"Ok. Here is your money.", delete(TYPE), createMoney}},
	{{eq(TOPIC, 2), "yes"}, {"Sorry, you do not have one."}},
	{{eq(TOPIC, 2)}, {"Ok, then not."}},

	{{"spell"}, {"I teach 'Poison Field', 'Fire Field', 'Energy Field', 'Poison Wall', 'Fire wall', and 'Energy Wall'."}},

	{{DRUID, "poison", "field"}, {setType(26), setPrice(300), "Do you want to buy the spell 'Poison Field' for %P gold?", setTopic(3)}},
	{{DRUID, "fire", "field"}, {setType(25), setPrice(500), "Do you want to buy the spell 'Fire Field' for %P gold?", setTopic(3)}},
	{{DRUID, "energy", "field"}, {setType(27), setPrice(700), "Do you want to buy the spell 'Energy Field' for %P gold?", setTopic(3)}},
	{{DRUID, "poison", "wall"}, {setType(32), setPrice(1600), "Do you want to buy the spell 'Poison Wall' for %P gold?", setTopic(3)}},
	{{DRUID, "fire", "wall"}, {setType(28), setPrice(2000), "Do you want to buy the spell 'Fire Wall' for %P gold?", setTopic(3)}},
	{{DRUID, "energy", "wall"}, {setType(33), setPrice(2500), "Do you want to buy the spell 'Energy Wall' for %P gold?", setTopic(3)}},

	{{"poison", "field"}, {"I'm sorry, but this spell is only for druids."}},
	{{"fire", "field"}, {previous}},
	{{"energy", "field"}, {previous}},
	{{"poison", "wall"}, {previous}},
	{{"fire", "wall"}, {previous}},
	{{"energy", "wall"}, {previous}},

	{{eq(TOPIC, 3), "yes", eq(spellKnown(TYPE), 1)}, {"You already know that spell."}},
	{{eq(TOPIC, 3), "yes", lt(LEVEL, spellLevel(TYPE))}, {setAmount(spellLevel(TYPE)), "You need to improve to level %A to learn this spell."}},
	{{eq(TOPIC, 3), "yes", lt(COUNTMONEY, PRICE)}, {"You do not have enough gold to pay my services."}},
	{{eq(TOPIC, 3), "yes"}, {"From now on you may cast this spell.", deleteMoney, effectOpp(13), teachSpell(TYPE)}},
	{{eq(TOPIC, 3)}, {"As you wish."}},

	{{"heal$", gt(BURNING, 0)}, {"You are burning. I will help you.", burning(0, 0), effectOpp(15)}},
	{{"heal$", gt(POISON, 0)}, {"You are poisoned. I will help you.", poison(0, 0), effectOpp(14)}},
	{{"heal$", lt(HP, 40)}, {"You are looking really bad. Let me heal your wounds.", setHp(40), effectOpp(13)}},
	{{"heal$"}, {"You aren't looking really bad. Sorry, I can't help you."}},

	{{"weapon"}, {"I have spears, swords, rapiers, daggers, longswords, machetes, staffs, and sabres. Interested?"}},
	{{"helmet"}, {"I am selling leather helmets and chain helmets. Anything you'd like to buy?"}},
	{{"armor"}, {"I am selling leather, chain, and brass armor. What do you need?"}},
	{{"shield"}, {"I am selling wooden shields and steel shields. What do you want?"}},
	{{"trousers"}, {"I am selling chain legs. Do you want to buy any?"}},
	{{"legs"}, {previous}},
	{{"spellbook"}, {"I have none here."}},
}

function onCreatureSay(npc, creature, text, situation)
	behaviour.react(npc, rules, text, situation)
end

function onCreatureDisappear(npc, creature)
	behaviour.react(npc, rules, "", NPC_SITUATION_VANISH)
end
