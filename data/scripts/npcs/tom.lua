-- npcs/tom.lua: the behaviour of the NPC Tom, converted from
-- tom.npc by patella-convert. The NPC is its [[npc]] entry in npcs.toml;
-- lib/npc.lua matches and runs the rules below, in the order of the file
-- (scripting.hh, "NPCs"). The comments are those of tom.npc.
--
-- GIMUD - Graphical Interface Multi User Dungeon
-- tom.npc: Datenbank fuer den Gerber Tom

local behaviour = require("lib.npc")
local ADDRESS = behaviour.ADDRESS
local BUSY = behaviour.BUSY
local VANISH = behaviour.VANISH
local MALE = behaviour.MALE
local FEMALE = behaviour.FEMALE
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
local questValue = behaviour.questValue
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
	{{ADDRESS, "hello$", FORCE}, {"I'm Tom the Tanner. How can I help you %N?"}},
	{{ADDRESS, "hi$", FORCE}, {previous}},
	{{ADDRESS, FORCE}, {idle}},
	{{BUSY, "hello$", FORCE}, {"Please wait, %N. I'll call you in a minute.", queue}},
	{{BUSY, "hi$", FORCE}, {previous}},
	{{BUSY, FORCE}, {nop}},
	{{VANISH, FORCE}, {"Doh?"}},

	{{"bye", MALE}, {"Good hunting, son.", idle}},
	{{"farewell", MALE}, {previous}},
	{{"bye", FEMALE}, {"Good hunting, child.", idle}},
	{{"farewell", FEMALE}, {previous}},
	{{"how", "are", "you"}, {"Much to do, these days."}},
	{{"job"}, {"I'm the local tanner. I buy fresh animal corpses, tan them, and convert them into fine leather clothes ...", "I'm only selling to major customers. But I'm buying fresh corpses of rats, rabbits and wolves from you."}},
	{{"name"}, {"My name is Tom the tanner."}},

	{{"tanner"}, {"That's my job. I buy fresh animal corpses, tan them, and convert them into fine leather clothes."}},
	{{"corpse"}, {"I'm buying fresh corpses of rats, rabbits and wolves. What do you want to sell?"}},
	{{"animal"}, {previous}},
	{{"monster"}, {previous}},
	{{"wares"}, {previous}},
	{{"major"}, {"Yes. Obi, Norma and good old Al. Go ask them for leather clothes."}},
	{{"customer"}, {previous}},

	{{"time"}, {"Sorry, I haven't been outside for a while, so I don't know."}},
	{{"help"}, {"Help? I will give you a few gold coins if you have some dead animals for me."}},

	{{"troll"}, {"Troll leather stinks. Can't use it."}},
	{{"orc"}, {"I don't buy Orcs. Their skin is too scratchy."}},
	{{"human"}, {"Are you crazy?!", idle}},

	{{"rat", eq(questValue(224), 0)}, {setType(3994), setAmount(1), setPrice(2), "I'll give you %P gold for a dead rat. Do you accept?", setTopic(2)}},
	{{"rabbit", eq(questValue(224), 0)}, {setType(4173), setAmount(1), setPrice(2), "I'll give you %P gold for a dead rabbit. Do you accept?", setTopic(2)}},
	{{"rat"}, {setType(3994), setAmount(1), setPrice(2), "I'll give you %P gold for a dead rat. Do you accept?", setTopic(1)}},
	{{"rabbit"}, {setType(4173), setAmount(1), setPrice(2), "I'll give you %P gold for a dead rabbit. Do you accept?", setTopic(1)}},
	{{"wolf"}, {setType(4007), setAmount(1), setPrice(5), "Do you want to sell a dead wolf for %P gold?", setTopic(1)}},

	{{P1, lt(1, P1), "rat", eq(questValue(224), 0)}, {setType(3994), setAmount(P1), setPrice(mul(2, P1)), "I'll give you %P gold for %A dead rats. Do you accept?", setTopic(2)}},
	{{P1, lt(1, P1), "rabbit", eq(questValue(224), 0)}, {setType(4173), setAmount(P1), setPrice(mul(2, P1)), "I'll give you %P gold for %A dead rabbits. Do you accept?", setTopic(2)}},
	{{P1, lt(1, P1), "rat"}, {setType(3994), setAmount(P1), setPrice(mul(2, P1)), "I'll give you %P gold for %A dead rats. Do you accept?", setTopic(1)}},
	{{P1, lt(1, P1), "rabbit"}, {setType(4173), setAmount(P1), setPrice(mul(2, P1)), "I'll give you %P gold for %A dead rabbits. Do you accept?", setTopic(1)}},
	{{P1, lt(1, P1), "wolf"}, {setType(4007), setAmount(P1), setPrice(mul(5, P1)), "Do you want to sell %A dead wolves for %P gold?", setTopic(1)}},
	{{P1, lt(1, P1), "wolves"}, {setType(4007), setAmount(P1), setPrice(mul(5, P1)), "Do you want to sell %A dead wolves for %P gold?", setTopic(1)}},

	{{eq(TOPIC, 1), "yes", ge(count(TYPE), AMOUNT)}, {"Ok. Corpse for me, gold for you.", delete(TYPE), createMoney}},
	{{eq(TOPIC, 1), "yes"}, {"Sorry, you do not have a fresh one."}},
	{{eq(TOPIC, 1), "yes", gt(AMOUNT, 1)}, {"Sorry, you do not have so many."}},
	{{eq(TOPIC, 1)}, {"Maybe another time."}},

	{{eq(TOPIC, 2), "yes", ge(count(TYPE), AMOUNT)}, {"Deal. By the way: If you'd like to hunt something bigger, check the cellar of the stables to the north. Some adventurer used to store his loot under a loose board beneath a barrel. He might have forgotten something when he left the isle.", delete(TYPE), createMoney}},
	{{eq(TOPIC, 2), "yes"}, {"Sorry, you do not have a fresh one."}},
	{{eq(TOPIC, 2), "yes", gt(AMOUNT, 1)}, {"Sorry, you do not have so many."}},
	{{eq(TOPIC, 2)}, {"Maybe another time."}},

	{{"sell"}, {"Sorry. I'm only selling to major customers. But I'm buying fresh corpses of rats, rabbits and wolves from you."}},
}

function onCreatureSay(npc, creature, text, situation)
	behaviour.react(npc, rules, text, situation)
end

function onCreatureDisappear(npc, creature)
	behaviour.react(npc, rules, "", NPC_SITUATION_VANISH)
end
