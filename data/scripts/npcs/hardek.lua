-- npcs/hardek.lua: the behaviour of the NPC Hardek, converted from
-- hardek.npc by patella-convert. The NPC is its [[npc]] entry in npcs.toml;
-- lib/npc.lua matches and runs the rules below, in the order of the file
-- (scripting.hh, "NPCs"). The comments are those of hardek.npc.
--
-- GIMUD - Graphical Interface Multi User Dungeon
-- Hardek.npc: Datenbank für den Aufkäufer Hardek

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
local AMOUNT = behaviour.AMOUNT
local TYPE = behaviour.TYPE
local count = behaviour.count
local mul = behaviour.mul
local setTopic = behaviour.setTopic
local setPrice = behaviour.setPrice
local setAmount = behaviour.setAmount
local setType = behaviour.setType
local delete = behaviour.delete
local createMoney = behaviour.createMoney
local queue = behaviour.queue
local idle = behaviour.idle
local nop = behaviour.nop
local previous = behaviour.previous

local rules = behaviour.rules {
	{{ADDRESS, "hello$", FORCE}, {"Hello %N! Do you need my services?"}},
	{{ADDRESS, "hi$", FORCE}, {previous}},
	{{ADDRESS, FORCE}, {idle}},
	{{BUSY, "hello$", FORCE}, {"One moment please %N, I am busy.", queue}},
	{{BUSY, "hi$", FORCE}, {previous}},
	{{BUSY, FORCE}, {nop}},
	{{VANISH, FORCE}, {"Good bye. Visit me whenever you want to sell something."}},

	{{"bye"}, {"Good bye. Visit me whenever you want to sell something.", idle}},
	{{"farewell"}, {previous}},
	{{"job"}, {"I am buying some weapons and armors."}},
	{{"forestaller"}, {previous}},
	{{"name"}, {"I am Hardek, the forestaller."}},
	{{"time"}, {"It is %T."}},
	{{"help"}, {"I buy stuff. If you want to sell something, offer it to me, and we'll see if it catches my interest."}},
	{{"thanks"}, {"You are welcome."}},
	{{"thank", "you"}, {previous}},

	{{"sell", "brass", "shield"}, {setType(3411), setAmount(1), setPrice(16), "Do you want to sell a brass shield for %P gold?", setTopic(2)}},
	{{"sell", "plate", "shield"}, {setType(3410), setAmount(1), setPrice(31), "Do you want to sell a plate shield for %P gold?", setTopic(2)}},
	{{"sell", "battle", "shield"}, {setType(3413), setAmount(1), setPrice(95), "Do you want to sell a battle shield for %P gold?", setTopic(2)}},
	{{"sell", "guardian", "shield"}, {setType(3415), setAmount(1), setPrice(180), "Do you want to sell a guardian shield for %P gold?", setTopic(2)}},
	{{"sell", "dragon", "shield"}, {setType(3416), setAmount(1), setPrice(360), "Do you want to sell a dragon shield for %P gold?", setTopic(2)}},

	{{"sell", "coat"}, {setType(3562), setAmount(1), setPrice(1), "Do you want to sell a coat for %P gold?", setTopic(2)}},
	{{"sell", "jacket"}, {setType(3561), setAmount(1), setPrice(1), "Do you want to sell a jacket for %P gold?", setTopic(2)}},
	{{"sell", "brass", "armor"}, {setType(3359), setAmount(1), setPrice(112), "Do you want to sell a brass armor for %P gold?", setTopic(2)}},
	{{"sell", "knight", "armor"}, {setType(3370), setAmount(1), setPrice(875), "Do you want to sell a knight armor for %P gold?", setTopic(2)}},
	{{"sell", "golden", "armor"}, {setType(3360), setAmount(1), setPrice(1500), "Do you want to sell a golden armor for %P gold?", setTopic(2)}},

	{{"sell", "brass", "helmet"}, {setType(3354), setAmount(1), setPrice(30), "Do you want to sell a brass helmet for %P gold?", setTopic(2)}},
	{{"sell", "viking", "helmet"}, {setType(3367), setAmount(1), setPrice(66), "Do you want to sell a viking helmet for %P gold?", setTopic(2)}},
	{{"sell", "iron", "helmet"}, {setType(3353), setAmount(1), setPrice(145), "Do you want to sell an iron helmet for %P gold?", setTopic(2)}},
	{{"sell", "devil", "helmet"}, {setType(3356), setAmount(1), setPrice(450), "Do you want to sell a devil helmet for %P gold?", setTopic(2)}},
	{{"sell", "warrior", "helmet"}, {setType(3369), setAmount(1), setPrice(696), "Do you want to sell a warrior helmet for %P gold?", setTopic(2)}},

	{{"sell", "leather", "legs"}, {setType(3559), setAmount(1), setPrice(1), "Do you want to sell leather legs for %P gold?", setTopic(2)}},
	{{"sell", "chain", "legs"}, {setType(3558), setAmount(1), setPrice(20), "Do you want to sell chain legs for %P gold?", setTopic(2)}},
	{{"sell", "brass", "legs"}, {setType(3372), setAmount(1), setPrice(49), "Do you want to sell brass legs for %P gold?", setTopic(2)}},
	{{"sell", "plate", "legs"}, {setType(3557), setAmount(1), setPrice(115), "Do you want to sell plate legs for %P gold?", setTopic(2)}},
	{{"sell", "knight", "legs"}, {setType(3371), setAmount(1), setPrice(375), "Do you want to sell knight legs for %P gold?", setTopic(2)}},

	{{"sell", "dagger"}, {setType(3267), setAmount(1), setPrice(1), "Do you want to sell a dagger for %P gold?", setTopic(2)}},
	{{"sell", "longsword"}, {setType(3285), setAmount(1), setPrice(51), "Do you want to sell a longsword for %P gold?", setTopic(2)}},
	{{"sell", "spike", "sword"}, {setType(3271), setAmount(1), setPrice(225), "Do you want to sell a spike sword for %P gold?", setTopic(2)}},
	{{"sell", "fire", "sword"}, {setType(3280), setAmount(1), setPrice(1000), "Do you want to sell a fire sword for %P gold?", setTopic(2)}},
	{{"sell", "mace"}, {setType(3286), setAmount(1), setPrice(23), "Do you want to sell a mace for %P gold?", setTopic(2)}},

	{{"sell", P1, lt(1, P1), "brass", "shield"}, {setType(3411), setAmount(P1), setPrice(mul(16, P1)), "Do you want to sell %A brass shields for %P gold?", setTopic(2)}},
	{{"sell", P1, lt(1, P1), "plate", "shield"}, {setType(3410), setAmount(P1), setPrice(mul(31, P1)), "Do you want to sell %A plate shields for %P gold?", setTopic(2)}},
	{{"sell", P1, lt(1, P1), "battle", "shield"}, {setType(3413), setAmount(P1), setPrice(mul(95, P1)), "Do you want to sell %A battle shields for %P gold?", setTopic(2)}},
	{{"sell", P1, lt(1, P1), "guardians", "shield"}, {setType(3415), setAmount(P1), setPrice(mul(180, P1)), "Do you want to sell %A guardians shields for %P gold?", setTopic(2)}},
	{{"sell", P1, lt(1, P1), "dragon", "shield"}, {setType(3416), setAmount(P1), setPrice(mul(360, P1)), "Do you want to sell %A dragon shields for %P gold?", setTopic(2)}},

	{{"sell", P1, lt(1, P1), "coat"}, {setType(3562), setAmount(P1), setPrice(mul(1, P1)), "Do you want to sell %A coats for %P gold?", setTopic(2)}},
	{{"sell", P1, lt(1, P1), "jacket"}, {setType(3561), setAmount(P1), setPrice(mul(1, P1)), "Do you want to sell %A jackets for %P gold?", setTopic(2)}},
	{{"sell", P1, lt(1, P1), "brass", "armor"}, {setType(3359), setAmount(P1), setPrice(mul(112, P1)), "Do you want to sell %A brass armors for %P gold?", setTopic(2)}},
	{{"sell", P1, lt(1, P1), "knight", "armor"}, {setType(3370), setAmount(P1), setPrice(mul(875, P1)), "Do you want to sell %A knight armors for %P gold?", setTopic(2)}},
	{{"sell", P1, lt(1, P1), "golden", "armor"}, {setType(3360), setAmount(P1), setPrice(mul(1500, P1)), "Do you want to sell %A golden armors for %P gold?", setTopic(2)}},

	{{"sell", P1, lt(1, P1), "brass", "helmet"}, {setType(3354), setAmount(P1), setPrice(mul(30, P1)), "Do you want to sell %A brass helmets for %P gold?", setTopic(2)}},
	{{"sell", P1, lt(1, P1), "viking", "helmet"}, {setType(3367), setAmount(P1), setPrice(mul(66, P1)), "Do you want to sell %A viking helmets for %P gold?", setTopic(2)}},
	{{"sell", P1, lt(1, P1), "iron", "helmet"}, {setType(3353), setAmount(P1), setPrice(mul(145, P1)), "Do you want to sell %A iron helmets for %P gold?", setTopic(2)}},
	{{"sell", P1, lt(1, P1), "devil", "helmet"}, {setType(3356), setAmount(P1), setPrice(mul(450, P1)), "Do you want to sell %A devil helmets for %P gold?", setTopic(2)}},
	{{"sell", P1, lt(1, P1), "warrior", "helmet"}, {setType(3369), setAmount(P1), setPrice(mul(696, P1)), "Do you want to sell %A warrior helmets for %P gold?", setTopic(2)}},

	{{"sell", P1, lt(1, P1), "leather", "legs"}, {setType(3559), setAmount(P1), setPrice(mul(1, P1)), "Do you want to sell %A leather legs for %P gold?", setTopic(2)}},
	{{"sell", P1, lt(1, P1), "chain", "legs"}, {setType(3558), setAmount(P1), setPrice(mul(20, P1)), "Do you want to sell %A chain legs for %P gold?", setTopic(2)}},
	{{"sell", P1, lt(1, P1), "brass", "legs"}, {setType(3372), setAmount(P1), setPrice(mul(49, P1)), "Do you want to sell %A brass legs for %P gold?", setTopic(2)}},
	{{"sell", P1, lt(1, P1), "plate", "legs"}, {setType(3557), setAmount(P1), setPrice(mul(115, P1)), "Do you want to sell %A plate legs for %P gold?", setTopic(2)}},
	{{"sell", P1, lt(1, P1), "knight", "legs"}, {setType(3371), setAmount(P1), setPrice(mul(375, P1)), "Do you want to sell %A knight legs for %P gold?", setTopic(2)}},

	{{"sell", P1, lt(1, P1), "dagger"}, {setType(3267), setAmount(P1), setPrice(mul(1, P1)), "Do you want to sell %A daggers for %P gold?", setTopic(2)}},
	{{"sell", P1, lt(1, P1), "longsword"}, {setType(3285), setAmount(P1), setPrice(mul(51, P1)), "Do you want to sell %A longswords for %P gold?", setTopic(2)}},
	{{"sell", P1, lt(1, P1), "spike", "sword"}, {setType(3271), setAmount(P1), setPrice(mul(225, P1)), "Do you want to sell %A spike swords for %P gold?", setTopic(2)}},
	{{"sell", P1, lt(1, P1), "fire", "sword"}, {setType(3280), setAmount(P1), setPrice(mul(1000, P1)), "Do you want to sell %A fireswords for %P gold?", setTopic(2)}},
	{{"sell", P1, lt(1, P1), "mace"}, {setType(3286), setAmount(P1), setPrice(mul(23, P1)), "Do you want to sell %A maces for %P gold?", setTopic(2)}},

	{{eq(TOPIC, 2), "yes", ge(count(TYPE), AMOUNT)}, {"Ok. Here is your money.", delete(TYPE), createMoney}},
	{{eq(TOPIC, 2), "yes"}, {"Sorry, you do not have one."}},
	{{eq(TOPIC, 2), "yes", gt(AMOUNT, 1)}, {"Sorry, you do not have that many."}},
	{{eq(TOPIC, 2)}, {"Maybe next time."}},
}

function onCreatureSay(npc, creature, text, situation)
	behaviour.react(npc, rules, text, situation)
end

function onCreatureDisappear(npc, creature)
	behaviour.react(npc, rules, "", NPC_SITUATION_VANISH)
end
