-- npcs/hyacinth.lua: the behaviour of the NPC Hyacinth, converted from
-- hyacinth.npc by patella-convert. The NPC is its [[npc]] entry in npcs.toml;
-- lib/npc.lua matches and runs the rules below, in the order of the file
-- (scripting.hh, "NPCs"). The comments are those of hyacinth.npc.
--
-- GIMUD - Graphical Interface Multi User Dungeon
-- hyacinth.npc: Datenbank fuer den Druiden Hyacinth

local behaviour = require("lib.npc")
local ADDRESS = behaviour.ADDRESS
local BUSY = behaviour.BUSY
local VANISH = behaviour.VANISH
local FORCE = behaviour.FORCE
local P1 = behaviour.P1
local eq = behaviour.eq
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
	{{ADDRESS, "hello$", FORCE}, {"Greetings, traveller %N."}},
	{{ADDRESS, "hi$", FORCE}, {previous}},
	{{ADDRESS, FORCE}, {idle}},
	{{BUSY, "hello$", FORCE}, {"Wait %N.", queue}},
	{{BUSY, "hi$", FORCE}, {previous}},
	{{BUSY, FORCE}, {nop}},
	{{VANISH, FORCE}, {"May Crunor bless you."}},

	{{"bye"}, {"May Crunor bless you.", idle}},
	{{"farewell"}, {previous}},
	{{"how", "are", "you"}, {"Thanks to the gods, I am fine."}},
	{{"sell"}, {"I just sell some revitalizing life fluids."}},
	{{"job"}, {"I am a druid and healer, a follower of Crunor."}},
	{{"name"}, {"I am Hyacinth."}},
	{{"time"}, {"Time does not matter to me."}},
	{{"help"}, {"I can only sell life fluids, ask Cipfried for further help."}},
	{{"monster"}, {"Most of the so called monsters of this isle are just creatures of the gods. On the mainland there are some beasts that truly are monstrous."}},
	{{"dungeon"}, {"The dungeons are dangerous for unexperienced adventurers."}},
	{{"sewer"}, {"I rarely visit the town."}},
	{{"god"}, {"As far as I know there is a library in the village. Teach yourself about the gods."}},
	{{"king"}, {"I don't care about kings, queens, and the like."}},
	{{"obi"}, {"A greedy and annoying person as most people are."}},
	{{"seymour"}, {"He has some inner devils that torture him."}},
	{{"dallheim"}, {"A man of the sword."}},
	{{"cipfried"}, {"His healing powers equal even mine."}},
	{{"amber"}, {"I never talked to her longer."}},
	{{"weapon"}, {"I don't care much about weapons."}},
	{{"magic"}, {"I am one of the few magic users on this isle. But I sense a follower of the dark path of magic hiding somewhere in the depths of the dungeons."}},
	{{"spell"}, {"I can't teach you magic. On the mainland you will learn your spells soon enough."}},
	{{"tibia"}, {"It is shaped by the will of the gods, so we don't have to question it."}},

	{{"life", "fluid"}, {setType(2874), setData(11), setAmount(1), setPrice(60), "Do you want to buy life fluid for %P gold?", setTopic(1)}},
	{{P1, "life", "fluid"}, {setType(2874), setData(11), setAmount(P1), setPrice(mul(60, P1)), "Do you want to buy %A potions of life fluid for %P gold?", setTopic(1)}},

	{{eq(TOPIC, 1), "yes", ge(COUNTMONEY, PRICE)}, {"Here you are. There is a deposit of 5 gold on the vial.", deleteMoney, create(TYPE)}},
	{{eq(TOPIC, 1), "yes"}, {"Come back, when you have enough money."}},
	{{eq(TOPIC, 1)}, {"Hmm, but next time."}},

	{{"deposit"}, {"I will pay you 5 gold for every empty vial. Ok?", setData(0), setTopic(2)}},
	{{"vial"}, {previous}},
	{{"flask"}, {previous}},
	{{eq(TOPIC, 2), "yes", gt(count(2874), 0)}, {setAmount(count(2874)), setPrice(mul(AMOUNT, 5)), "Here you are ... %P gold.", delete(2874), createMoney}},
	{{eq(TOPIC, 2), "yes"}, {"You don't have any empty vials."}},
	{{eq(TOPIC, 2)}, {"Hmm, but please keep Tibia litter free."}},
}

function onCreatureSay(npc, creature, text, situation)
	behaviour.react(npc, rules, text, situation)
end

function onCreatureDisappear(npc, creature)
	behaviour.react(npc, rules, "", NPC_SITUATION_VANISH)
end
