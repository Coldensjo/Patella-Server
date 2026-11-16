-- npcs/brengus.lua: the behaviour of the NPC Brengus, converted from
-- brengus.npc by patella-convert. The NPC is its [[npc]] entry in npcs.toml;
-- lib/npc.lua matches and runs the rules below, in the order of the file
-- (scripting.hh, "NPCs"). The comments are those of brengus.npc.
--
-- GIMUD - Graphical Interface Multi User Dungeon
-- Brengus.npc: Datenbank für den waffen und rüstungshändler Brengus

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
	{{ADDRESS, "hello$", FORCE}, {"Be greeted."}},
	{{ADDRESS, "hi$", FORCE}, {previous}},
	{{ADDRESS, "hiho$", FORCE}, {previous}},
	{{ADDRESS, FORCE}, {idle}},

	{{BUSY, "hello$", FORCE}, {"Please wait, I am busy right now", queue}},
	{{BUSY, "hi$", FORCE}, {previous}},
	{{BUSY, "hiho$", FORCE}, {previous}},
	{{BUSY, FORCE}, {nop}},
	{{VANISH, FORCE}, {"Good bye."}},

	{{"bye"}, {"Good bye.", idle}},
	{{"farewell"}, {previous}},
	{{"job"}, {"I am a tradesman. I sell and buy weapons and armor."}},
	{{"name"}, {"My name is Brengus."}},
	{{"time"}, {"Sorry, my watch didn't take the moist air here too well."}},
	{{"king"}, {"This is the king's land. It was a wise decision to have us people from Venore rule this settlement."}},
	{{"venore"}, {"I miss my home like most of us here, but I have duties and responsibilities. After all, there is some meagre profit to earn here."}},
	{{"thais"}, {"A nice big city of course, but it lacks style and grandeur. Such qualities you will only find when you visit my hometown Venore."}},
	{{"carlin"}, {"I hope the king will take these rebelling women soon under Thaian guidance once again. I hate to see the profits wasted that could be earned there."}},
	{{"edron"}, {"A rich and lovely island. Sadly those knights kept our tradesmen out of business for some unknown reason. I am convinced after seeing our success with this colony here, the king will allow Venore to become more present over there too."}},
	{{"jungle"}, {"Of course there are problems. But problems are there to keep those out of business who are not prepared and diligent enough."}},

	{{"tibia"}, {"It's a world full of possibilities."}},

	{{"kazordoon"}, {"The dwarves of Kazordoon are stubborn people and it's hard to have dealings with them. But as often, the hardship is very rewarding for those who are able to handle them."}},
	{{"dwarves"}, {previous}},
	{{"dwarfs"}, {previous}},
	{{"ab'dendriel"}, {"It's complicated to negotiate with those elves but it is possible."}},
	{{"elves"}, {previous}},
	{{"elfs"}, {previous}},
	{{"darama"}, {"We have hardly scratched the surface of all the possibilities to gain profit that are hidden on this continent."}},
	{{"darashia"}, {"The sandwasp's honey is quite useful. But that's the only noteworthy thing about this unimportant desert hicktown."}},
	{{"ankrahmun"}, {"It's somewhat hard to evaluate if this city poses another threat or a new market. Only time can tell."}},
	{{"ferumbras"}, {"He is bad for business. The big trading houses of Venore have yet to decide what price they will put on his head."}},
	{{"excalibug"}, {"If you ever stumble upon that interesting piece of jewellery, contact me. I know somebody who would pay a decent amount of crystal to add it to his collection of curiosities."}},
	{{"apes"}, {"They are neither skilled in a craft nor do they know about the concept of trade. They constantly raid our colony to steal items."}},
	{{"lizzard"}, {"The lizzard folk is hostile to us but luckily they live far enough from here to be an immediate danger."}},
	{{"dworcs"}, {"They should be driven into the sea."}},

	{{"offer"}, {"My offers are weapons, armors, helmets, legs, and shields."}},
	{{"do", "you", "sell"}, {previous}},
	{{"do", "you", "have"}, {previous}},
	{{"weapon"}, {"I have hand axes, axes, spears, maces, battle hammers, swords, rapiers, daggers, and sabres. What's your choice?"}},
	{{"helmet"}, {"I am selling leather helmets and chain helmets. What do you want?"}},
	{{"armor"}, {"I am selling leather, chain and brass armors. What do you need?"}},
	{{"shield"}, {"I am selling wooden shields and steel shields. What do you want?"}},
	{{"trousers"}, {"I am selling chain legs. Do you want to buy some?"}},
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
	{{"throwing", "star"}, {setType(3287), setAmount(1), setPrice(50), "Do you want to buy a throwing star for %P gold?", setTopic(1)}},

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
	{{P1, lt(1, P1), "throwing", "star"}, {setType(3287), setAmount(P1), setPrice(mul(50, P1)), "Do you want to buy %A throwing stars for %P gold?", setTopic(1)}},

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
	{{"sell", "templar", "scytheblade"}, {setType(3345), setAmount(1), setPrice(200), "Do you want to sell a templar scytheblade for %P gold?", setTopic(2)}},
	{{"sell", "ripper", "lance"}, {setType(3346), setAmount(1), setPrice(500), "Do you want to sell a ripper lance for %P gold?", setTopic(2)}},
	{{"sell", "hunting", "spear"}, {setType(3347), setAmount(1), setPrice(250), "Do you want to sell a hunting spear for %P gold?", setTopic(2)}},
	{{"sell", "banana", "staff"}, {setType(3348), setAmount(1), setPrice(1000), "Do you want to sell a banana staff for %P gold?", setTopic(2)}},

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
	{{"sell", "tusk", "shield"}, {setType(3443), setAmount(1), setPrice(850), "Do you want to sell a tusk shield for %P gold?", setTopic(2)}},
	{{"sell", "sentinel", "shield"}, {setType(3444), setAmount(1), setPrice(120), "Do you want to sell a sentinel shield for %P gold?", setTopic(2)}},
	{{"sell", "salamander", "shield"}, {setType(3445), setAmount(1), setPrice(280), "Do you want to sell a salamander shield for %P gold?", setTopic(2)}},
	{{"sell", "tribal", "mask"}, {setType(3403), setAmount(1), setPrice(250), "Do you want to sell a tribal mask for %P gold?", setTopic(2)}},
	{{"sell", "leopard", "armor"}, {setType(3404), setAmount(1), setPrice(300), "Do you want to sell a leopard armor for %P gold?", setTopic(2)}},
	{{"sell", "horseman", "helmet"}, {setType(3405), setAmount(1), setPrice(280), "Do you want to sell a horseman helmet for %P gold?", setTopic(2)}},
	{{"sell", "feather", "headdress"}, {setType(3406), setAmount(1), setPrice(850), "Do you want to sell a feather headdress for %P gold?", setTopic(2)}},
	{{"sell", "crocodile", "boots"}, {setType(3556), setAmount(1), setPrice(100), "Do you want to sell crocodile boots for %P gold?", setTopic(2)}},
	{{"sell", "bast", "skirt"}, {setType(3560), setAmount(1), setPrice(750), "Do you want to sell a bast skirt for %P gold?", setTopic(2)}},
	{{"sell", "charmer", "tiara"}, {setType(3407), setAmount(1), setPrice(900), "Do you want to sell a charmer's tiara for %P gold?", setTopic(2)}},
	{{"sell", "beholder", "helmet"}, {setType(3408), setAmount(1), setPrice(2200), "Do you want to sell a beholder helmet for %P gold?", setTopic(2)}},

	{{"sell", "tusk"}, {"Sorry, I'm not interested in tusks, but you might want to offer them to Zaidal - as far as I know he uses them for making tables and chairs."}},
	{{"sell", P1, lt(1, P1), "tusk"}, {"Sorry, I'm not interested in tusks, but you might want to offer them to Zaidal - as far as I know he uses them for making tables and chairs."}},

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
	{{"sell", P1, lt(1, P1), "templar", "scytheblade"}, {setType(3345), setAmount(P1), setPrice(mul(200, P1)), "Do you want to sell %A templar scytheblades for %P gold?", setTopic(2)}},
	{{"sell", P1, lt(1, P1), "ripper", "lance"}, {setType(3346), setAmount(P1), setPrice(mul(500, P1)), "Do you want to sell %A ripper lances for %P gold?", setTopic(2)}},
	{{"sell", P1, lt(1, P1), "hunting", "spear"}, {setType(3347), setAmount(P1), setPrice(mul(250, P1)), "Do you want to sell %A hunting spears for %P gold?", setTopic(2)}},
	{{"sell", P1, lt(1, P1), "banana", "staff"}, {setType(3348), setAmount(P1), setPrice(mul(1000, P1)), "Do you want to sell %A banana staves for %P gold?", setTopic(2)}},

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
	{{"sell", P1, lt(1, P1), "tusk", "shield"}, {setType(3443), setAmount(P1), setPrice(mul(850, P1)), "Do you want to sell %A tusk shields for %P gold?", setTopic(2)}},
	{{"sell", P1, lt(1, P1), "sentinel", "shield"}, {setType(3444), setAmount(P1), setPrice(mul(120, P1)), "Do you want to sell %A sentinel shields for %P gold?", setTopic(2)}},
	{{"sell", P1, lt(1, P1), "salamander", "shield"}, {setType(3445), setAmount(P1), setPrice(mul(280, P1)), "Do you want to sell %A salamander shields for %P gold?", setTopic(2)}},
	{{"sell", P1, lt(1, P1), "tribal", "mask"}, {setType(3403), setAmount(P1), setPrice(mul(250, P1)), "Do you want to sell %A tribal masks for %P gold?", setTopic(2)}},
	{{"sell", P1, lt(1, P1), "leopard", "armor"}, {setType(3404), setAmount(P1), setPrice(mul(300, P1)), "Do you want to sell %A leopard armors for %P gold?", setTopic(2)}},
	{{"sell", P1, lt(1, P1), "horseman", "helmet"}, {setType(3405), setAmount(P1), setPrice(mul(280, P1)), "Do you want to sell %A horseman helmets for %P gold?", setTopic(2)}},
	{{"sell", P1, lt(1, P1), "feather", "headdress"}, {setType(3406), setAmount(P1), setPrice(mul(850, P1)), "Do you want to sell %A feather headdresses for %P gold?", setTopic(2)}},
	{{"sell", P1, lt(1, P1), "crocodile", "boots"}, {setType(3556), setAmount(P1), setPrice(mul(100, P1)), "Do you want to sell %A pairs of crocodile boots for %P gold?", setTopic(2)}},
	{{"sell", P1, lt(1, P1), "bast", "skirt"}, {setType(3560), setAmount(P1), setPrice(mul(750, P1)), "Do you want to sell %A bast skirts for %P gold?", setTopic(2)}},
	{{"sell", P1, lt(1, P1), "charmer", "tiara"}, {setType(3407), setAmount(P1), setPrice(mul(900, P1)), "Do you want to sell %A charmer's tiaras for %P gold?", setTopic(2)}},
	{{"sell", P1, lt(1, P1), "beholder", "helmet"}, {setType(3408), setAmount(P1), setPrice(mul(2200, P1)), "Do you want to sell %A beholder helmets for %P gold?", setTopic(2)}},

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
