-- npcs/ashtamor.lua: the behaviour of the NPC Ashtamor, converted from
-- ashtamor.npc by patella-convert. The NPC is its [[npc]] entry in npcs.toml;
-- lib/npc.lua matches and runs the rules below, in the order of the file
-- (scripting.hh, "NPCs"). The comments are those of ashtamor.npc.
--
-- GIMUD - Graphical Interface Multi User Dungeon
-- ashtamor.npc: Krematoriumsbesitzer Ashtamor

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
local effectMe = behaviour.effectMe
local effectOpp = behaviour.effectOpp
local create = behaviour.create
local deleteMoney = behaviour.deleteMoney
local queue = behaviour.queue
local idle = behaviour.idle
local nop = behaviour.nop
local previous = behaviour.previous

local rules = behaviour.rules {
	{{ADDRESS, "hello$", FORCE}, {"Welcome, %N, wanderer between the worlds."}},
	{{ADDRESS, "hi$", FORCE}, {previous}},
	{{ADDRESS, "greetings$", FORCE}, {previous}},
	{{ADDRESS, FORCE}, {idle}},
	{{BUSY, "hello$", FORCE}, {"The time comes for everyone, %N.", queue}},
	{{BUSY, "hi$", FORCE}, {previous}},
	{{BUSY, "greetings$", FORCE}, {previous}},
	{{BUSY, FORCE}, {nop}},
	{{VANISH, FORCE}, {"See you again ... sooner or later, more or less alive."}},

	{{"bye"}, {"See you again ... sooner or later, more or less alive.", idle}},
	{{"job"}, {"I consider myself as a guide, a guardian over the souls who transcend the border to another world."}},
	{{"crematory"}, {"Such an ugly word for this wonderful place. It is a door, a portal to a better world than this one is."}},
	{{"name"}, {"What is a name worth in your eyes? And more important: Does the choice of your name decide your further fate? Perhaps we will never know."}},
	{{"time"}, {"It's %T now, but the true question is: How much time is left?"}},
	{{"fire"}, {"The purging force of the fire ... after having been purified, the freed souls will depart with the smoke."}},
	{{"soul"}, {"The essence of life. Source of your very self. While the body is in space and time, the soul exists in time only."}},
	{{"body"}, {"Is the mind an emination of body, or the body an invention by the mind?"}},
	{{"death"}, {"What else does it mean than the loss of your weak physical shell? And isn't the true power in the universe rather mental than physical?"}},
	{{"venore"}, {"You come to this world naked, and leave it this way, so there's no need to hold back your money, especially not in a place like Venore."}},
	{{"king"}, {"Kings, queens ... I've seen them come and go. Everything fades, even the glory and wealth of the richest."}},
	{{"monster"}, {"Oh yes, monsters can grant you a passage to the afterlife also, but it's not a comfortable trip. <chuckles>"}},
	{{"thanks"}, {"'Thank you' ... Words I rarely here these days. Tell me when I might be of service again, %N."}},
	{{"thank", "you"}, {previous}},
	{{"help"}, {"What help might I offer you except guidance? Would you like me to help you transcend the border to the afterlife?", setTopic(1)}},

	{{eq(TOPIC, 1), "yes"}, {"Are you sure? You might not be able to come back, consider that.", setTopic(2)}},
	{{eq(TOPIC, 1), "no"}, {"Come to me when you have changed your mind, wanderer."}},
	{{eq(TOPIC, 2), "yes"}, {effectOpp(16), effectMe(14), "Hmm... seems you are not ready yet to let go."}},
	{{eq(TOPIC, 2), "no"}, {"Come to me when you have changed your mind, wanderer."}},

	{{"buy"}, {"I am offering vases and amphoras, the perfect vessel for dusty remains of whatever sort."}},
	{{"do", "you", "sell"}, {previous}},
	{{"do", "you", "have"}, {previous}},
	{{"offer"}, {previous}},

	{{"vase"}, {setType(2876), setAmount(1), setPrice(3), "Do you want to buy a vase for %P gold?", setTopic(3)}},
	{{"amphora"}, {setType(2893), setAmount(1), setPrice(4), "Do you want to buy an amphora for %P gold?", setTopic(3)}},

	{{P1, lt(1, P1), "vase"}, {setType(2876), setAmount(P1), setPrice(mul(3, P1)), "Do you want to buy %A vases for %P gold?", setTopic(3)}},
	{{P1, lt(1, P1), "amphora"}, {setType(2893), setAmount(P1), setPrice(mul(4, P1)), "Do you want to buy %A amphoras for %P gold?", setTopic(3)}},

	{{eq(TOPIC, 3), "yes", ge(COUNTMONEY, PRICE)}, {"May it be of good use to you. ", deleteMoney, create(TYPE)}},
	{{eq(TOPIC, 3), "yes"}, {"Sorry, that's not even enough for a shard of my quality wares."}},
	{{eq(TOPIC, 3)}, {"Do you really want to leave the choice of your future vessel to chance?"}},
}

function onCreatureSay(npc, creature, text, situation)
	behaviour.react(npc, rules, text, situation)
end

function onCreatureDisappear(npc, creature)
	behaviour.react(npc, rules, "", NPC_SITUATION_VANISH)
end
