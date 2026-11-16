-- npcs/xed.lua: the behaviour of the NPC Xed, converted from
-- xed.npc by patella-convert. The NPC is its [[npc]] entry in npcs.toml;
-- lib/npc.lua matches and runs the rules below, in the order of the file
-- (scripting.hh, "NPCs"). The comments are those of xed.npc.
--
-- GIMUD - Graphical Interface Multi User Dungeon
-- Xed.npc: Datenbank für den Bogner Xed

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
local create = behaviour.create
local deleteMoney = behaviour.deleteMoney
local queue = behaviour.queue
local idle = behaviour.idle
local nop = behaviour.nop
local previous = behaviour.previous

local rules = behaviour.rules {
	{{ADDRESS, "hello$", FORCE}, {"Greetings, %N. Welcome to the distance fighting booth of the Ironhouse."}},
	{{ADDRESS, "hi$", FORCE}, {previous}},
	{{ADDRESS, FORCE}, {idle}},
	{{BUSY, "hello$", FORCE}, {"One moment please, %N.", queue}},
	{{BUSY, "hi$", FORCE}, {previous}},
	{{BUSY, FORCE}, {nop}},
	{{VANISH, FORCE}, {"Goodbye, and may the gods be with you."}},

	{{"bye"}, {"Goodbye, and may the gods be with you.", idle}},
	{{"farewell"}, {previous}},
	{{"job"}, {"I am the humble supplier for distance fighting weapons of the Ironhouse, owned by Abran Ironeye."}},
	{{"fletcher"}, {previous}},
	{{"name"}, {"People call me Xed, but my full name is Xedem."}},
	{{"time"}, {"I don't know, maybe what you really need is a watch."}},
	{{"hurt"}, {"Go to a priest. I am sure they will fix you up."}},
	{{"Abran", "Ironeye"}, {"He is the owner of this market, although - just between you and me - I'm not so sure he's honest."}},
	{{"honest"}, {"Well, I overheard the boss discussing some shady deals with a man in a black cloak."}},
	{{"shady", "deals"}, {"Something about a sword only great warlords can use and a rare distance fighting item."}},
	{{"rare", "distance"}, {"Yes, but I believe this is nothing but lies seeing that there are only a few distance fighting weapons."}},
	{{"amazons"}, {"They are a band or tribe of strange women that have nothing in common with civilized men like me."}},
	{{"general"}, {"You must be talking of the great general Benjamin. He saved the kingdom from ferumbras you know."}},
	{{"army"}, {"We supply the archers of the army with distance fighting weapons."}},
	{{"ferumbras"}, {"I heard rumours somewhere that his father was called Hugo."}},
	{{"Xed"}, {"Yeah, nice name, eh?"}},
	{{"excalibug"}, {"I think that was the sword they were talking about. Said something about a man in Edron that could get it for him."}},
	{{"news"}, {"Some people say Ferumbras isn't really dead. Crazy kids!"}},
	{{"help"}, {"I sell items of the distance type."}},
	{{"monster"}, {"Yeah, these awful beasts. They live in the swamps near the city and in dark dungeons."}},
	{{"dungeon"}, {"Oh, they are all over. You never see more of them than in Kaz, though."}},
	{{"Kaz"}, {"Oh, that's short for Kazordoon."}},

	{{"buy"}, {"I am selling bows, crossbows, and ammunition. Do you need anything?"}},
	{{"do", "you", "sell"}, {previous}},
	{{"do", "you", "have"}, {previous}},
	{{"offer"}, {previous}},
	{{"goods"}, {previous}},
	{{"ammo"}, {"Do you need arrows for a bow, or bolts for a crossbow?"}},
	{{"ammunition"}, {previous}},

	{{"sell", "bow"}, {"I don't buy used bows."}},
	{{"sell", "crossbow"}, {"I don't buy used crossbows."}},

	{{"bow"}, {setType(3350), setAmount(1), setPrice(400), "Do you want to buy a bow for %P gold?", setTopic(1)}},
	{{"crossbow"}, {setType(3349), setAmount(1), setPrice(500), "Do you want to buy a crossbow for %P gold?", setTopic(1)}},
	{{"arrow"}, {setType(3447), setAmount(1), setPrice(2), "Do you want to buy an arrows for %P gold?", setTopic(1)}},
	{{"bolt"}, {setType(3446), setAmount(1), setPrice(3), "Do you want to buy a bolts for %P gold?", setTopic(1)}},

	{{P1, lt(1, P1), "bow"}, {setType(3350), setAmount(P1), setPrice(mul(400, P1)), "Do you want to buy %A bows for %P gold?", setTopic(1)}},
	{{P1, lt(1, P1), "crossbow"}, {setType(3349), setAmount(P1), setPrice(mul(500, P1)), "Do you want to buy %A crossbows for %P gold?", setTopic(1)}},
	{{P1, lt(1, P1), "arrow"}, {setType(3447), setAmount(P1), setPrice(mul(2, P1)), "Do you want to buy %A arrows for %P gold?", setTopic(1)}},
	{{P1, lt(1, P1), "bolt"}, {setType(3446), setAmount(P1), setPrice(mul(3, P1)), "Do you want to buy %A bolts for %P gold?", setTopic(1)}},

	{{eq(TOPIC, 1), "yes", ge(COUNTMONEY, PRICE)}, {"Here you are.", deleteMoney, create(TYPE)}},
	{{eq(TOPIC, 1), "yes"}, {"Come back, when you have enough money."}},
	{{eq(TOPIC, 1)}, {"Hmm, but next time."}},
}

function onCreatureSay(npc, creature, text, situation)
	behaviour.react(npc, rules, text, situation)
end

function onCreatureDisappear(npc, creature)
	behaviour.react(npc, rules, "", NPC_SITUATION_VANISH)
end
