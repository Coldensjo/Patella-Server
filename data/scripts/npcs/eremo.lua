-- npcs/eremo.lua: the behaviour of the NPC Eremo, converted from
-- eremo.npc by patella-convert. The NPC is its [[npc]] entry in npcs.toml;
-- lib/npc.lua matches and runs the rules below, in the order of the file
-- (scripting.hh, "NPCs"). The comments are those of eremo.npc.
--
-- GIMUD - Graphical Interface Multi User Dungeon
-- eremo.npc: Datenbank für den Weisen Eremo

local behaviour = require("lib.npc")
local ADDRESS = behaviour.ADDRESS
local BUSY = behaviour.BUSY
local VANISH = behaviour.VANISH
local KNIGHT = behaviour.KNIGHT
local PALADIN = behaviour.PALADIN
local SORCERER = behaviour.SORCERER
local DRUID = behaviour.DRUID
local PREMIUM = behaviour.PREMIUM
local PROMOTED = behaviour.PROMOTED
local PZBLOCK = behaviour.PZBLOCK
local PVPENFORCED = behaviour.PVPENFORCED
local FORCE = behaviour.FORCE
local eq = behaviour.eq
local lt = behaviour.lt
local gt = behaviour.gt
local ge = behaviour.ge
local TOPIC = behaviour.TOPIC
local PRICE = behaviour.PRICE
local AMOUNT = behaviour.AMOUNT
local TYPE = behaviour.TYPE
local LEVEL = behaviour.LEVEL
local COUNTMONEY = behaviour.COUNTMONEY
local count = behaviour.count
local spellKnown = behaviour.spellKnown
local spellLevel = behaviour.spellLevel
local questValue = behaviour.questValue
local setTopic = behaviour.setTopic
local setPrice = behaviour.setPrice
local setAmount = behaviour.setAmount
local setType = behaviour.setType
local setQuestValue = behaviour.setQuestValue
local effectOpp = behaviour.effectOpp
local teachSpell = behaviour.teachSpell
local create = behaviour.create
local delete = behaviour.delete
local createMoney = behaviour.createMoney
local deleteMoney = behaviour.deleteMoney
local queue = behaviour.queue
local idle = behaviour.idle
local nop = behaviour.nop
local teleport = behaviour.teleport
local previous = behaviour.previous

local rules = behaviour.rules {
	{{ADDRESS, "hello$", FORCE}, {"Welcome to my little garden, adventurer %N!"}},
	{{ADDRESS, "hi$", FORCE}, {previous}},
	{{ADDRESS, "greetings$", FORCE}, {previous}},
	{{ADDRESS, PREMIUM, PROMOTED, "hello$", FORCE}, {"Welcome to my little garden, humble %N!"}},
	{{ADDRESS, PREMIUM, PROMOTED, "hi$", FORCE}, {previous}},
	{{ADDRESS, PREMIUM, PROMOTED, "greetings$", FORCE}, {previous}},
	{{ADDRESS, FORCE}, {idle}},
	{{BUSY, "hello$", FORCE}, {"Please wait a minute, %N.", queue}},
	{{BUSY, "hi$", FORCE}, {previous}},
	{{BUSY, FORCE}, {nop}},
	{{VANISH, FORCE}, {"Shouldn't I teleport you back to Pemaret?"}},

	{{"bye"}, {"Shouldn't I teleport you back to Pemaret?", idle}},
	{{"farewell"}, {previous}},
	{{"name"}, {"I am Eremo, an old man who has seen many things."}},
	{{"job"}, {"I teach some spells, provide one of the five blessings, and sell some amulets."}},
	{{"offer"}, {previous}},
	{{"magic"}, {previous}},
	{{"island"}, {"I have retired from my adventures to this place."}},
	{{"isle"}, {previous}},
	{{"garden"}, {previous}},
	{{"adventure"}, {"I explored dungeons, I walked through deserts, I sailed on the seas and climbed up on many a mountain."}},
	{{"thing"}, {previous}},
	{{"Tibia"}, {"A great world full of magic and wonder."}},

	{{"amulet", PVPENFORCED}, {"I've collected quite a few protection amulets. Also, I'm interested in buying broken amulets."}},
	{{"amulet"}, {"I've collected quite a few protection amulets, and some amulets of loss as well. Also, I'm interested in buying broken amulets."}},

	{{"amulet", "of", "loss"}, {setType(3057), setAmount(1), setPrice(50000), "Do you want to buy an amulet of loss for %P gold?", setTopic(3)}},
	{{"amulet", "of", "loss", PVPENFORCED, FORCE}, {"What a strange name for an amulet. Never heard about that one."}},

	{{"protection", "amulet"}, {setType(3084), setAmount(1), setPrice(700), "Do you want to buy a protection amulet for %P gold?", setTopic(3)}},
	{{"broken", "amulet"}, {setType(3080), setAmount(1), setPrice(50000), "Do you want to sell a broken amulet for %P gold?", setTopic(4)}},
	{{"amulet", "of", "life"}, {previous}},

	{{PREMIUM, PROMOTED, "spell"}, {"I can teach 'Enchant Staff' to sorcerers, 'Challenge' to knights, 'Wild Growth' to druids, and 'Power Bolt' to paladins."}},
	{{"spell"}, {"I am sorry, but you are not promoted yet."}},

	{{SORCERER, PREMIUM, PROMOTED, "enchant", "staff"}, {setType(92), setPrice(2000), "Do you want to learn the spell 'Enchant Staff' for %P gold?", setTopic(1)}},
	{{"enchant", "staff"}, {"I am sorry but this spell is only for master sorcerers."}},

	{{KNIGHT, PREMIUM, PROMOTED, "challenge"}, {setType(93), setPrice(2000), "Do you want to learn the spell 'Challenge' for %P gold?", setTopic(1)}},
	{{"challenge"}, {"I am sorry but this spell is only for elite knights."}},

	{{DRUID, PREMIUM, PROMOTED, "wild", "growth"}, {setType(94), setPrice(2000), "Do you want to learn the spell 'Wild Growth' for %P gold?", setTopic(1)}},
	{{"wild", "growth"}, {"I am sorry but this spell is only for elder druids."}},

	{{PALADIN, PREMIUM, PROMOTED, "power", "bolt"}, {setType(95), setPrice(2000), "Do you want to learn the spell 'Power Bolt' for %P gold?", setTopic(1)}},
	{{"power", "bolt"}, {"I am sorry but this spell is only for royal paladins."}},

	{{"teleport"}, {"Should I teleport you back to Pemaret?", setTopic(2)}},
	{{"pemaret"}, {previous}},
	{{"back"}, {previous}},
	{{"cormaya"}, {previous}},
	{{"edron"}, {previous}},

	{{eq(TOPIC, 1), "yes", eq(spellKnown(TYPE), 1)}, {"You already know how to cast this spell."}},
	{{eq(TOPIC, 1), "yes", lt(LEVEL, spellLevel(TYPE))}, {setAmount(spellLevel(TYPE)), "You must be have level %A  or better to learn this spell."}},
	{{eq(TOPIC, 1), "yes", lt(COUNTMONEY, PRICE)}, {"Oh. You do not have enough money."}},
	{{eq(TOPIC, 1), "yes"}, {"Here you are. Look in your spellbook for the pronounciation of this spell.", deleteMoney, effectOpp(13), teachSpell(TYPE)}},
	{{eq(TOPIC, 1)}, {"Fine. Do as you please."}},

	{{eq(TOPIC, 2), "yes", PZBLOCK, FORCE}, {"Your soul is imbalanced by death and murder. Try again after you regained your balance!"}},

	{{eq(TOPIC, 2), "yes"}, {"Here you go!", idle, effectOpp(11), teleport(33288, 31956, 6), effectOpp(11)}},
	{{eq(TOPIC, 2)}, {"Maybe later."}},

	{{eq(TOPIC, 3), "yes", ge(COUNTMONEY, PRICE)}, {"Thank you. Use it wisely.", deleteMoney, create(TYPE)}},
	{{eq(TOPIC, 3), "yes"}, {"Sorry, you do not have enough gold."}},
	{{eq(TOPIC, 3)}, {"Maybe another time."}},

	{{eq(TOPIC, 4), "yes", ge(count(TYPE), AMOUNT)}, {"Ok. Here is your money.", delete(TYPE), createMoney}},
	{{eq(TOPIC, 4), "yes"}, {"Sorry, you do not own one."}},
	{{eq(TOPIC, 4)}, {"Maybe another time."}},

	{{"blessing", PVPENFORCED, FORCE}, {"The lifeforce of this world is waning. There are no more blessings avaliable on this world."}},
	{{"pilgrimage", PVPENFORCED, FORCE}, {previous}},
	{{"ashes", PVPENFORCED, FORCE}, {previous}},

	{{"spiritual", PVPENFORCED, FORCE}, {previous}},
	{{"shielding", PVPENFORCED, FORCE}, {previous}},
	{{"sacred", "places", PVPENFORCED, FORCE}, {previous}},
	{{"spark", PVPENFORCED, FORCE}, {previous}},
	{{"phoenix", PVPENFORCED, FORCE}, {previous}},
	{{"embrace", PVPENFORCED, FORCE}, {previous}},
	{{"fire", PVPENFORCED, FORCE}, {previous}},
	{{"suns", PVPENFORCED, FORCE}, {previous}},
	{{"wisdom", PVPENFORCED, FORCE}, {previous}},
	{{"solitude", PVPENFORCED, FORCE}, {previous}},

	{{"pilgrimage"}, {"Whenever you receive a lethal wound your lifeforce is damaged. With every single of the five blessings you have this damage will be reduced."}},
	{{"ashes"}, {previous}},
	{{"blessing"}, {"There are five different blessings available in five sacred places. These blessings are: the spiritual shielding, the spark of the phoenix, the embrace of Tibia, the fire of the suns and the wisdom of solitude."}},
	{{"sacred", "places"}, {"Just tell me in which of the five blessings you are interested."}},

	{{"spiritual"}, {" You can receive the spiritual shielding in the whiteflower temple south of Thais."}},
	{{"shielding"}, {previous}},
	{{"spark"}, {"The spark of the phoenix will be given to you by the dwarven priests of earth and fire in Kazordoon."}},
	{{"phoenix"}, {previous}},
	{{"embrace"}, {"The druids north of Carlin will provide you with the embrace of Tibia."}},

	{{"fire"}, {"You should ask for the blessing of the two suns in the suntower near Ab'Dendriel."}},
	{{"suns"}, {previous}},
	{{"wisdom"}, {"I can provide you with the wisdom of solitude. But you will have to sacrifice 10.000 gold to receive it. Are you still interested?", setPrice(10000), setTopic(5)}},
	{{"solitude"}, {previous}},

	{{eq(TOPIC, 5), "yes", gt(questValue(101), 0), FORCE}, {"You already possess this blessing."}},
	{{eq(TOPIC, 5), "yes", lt(COUNTMONEY, PRICE), FORCE}, {"Oh. You do not have enough money."}},
	{{eq(TOPIC, 5), "yes", FORCE}, {"So receive the wisdom of solitude, pilgrim", deleteMoney, effectOpp(13), setQuestValue(101, 1)}},
	{{eq(TOPIC, 5), FORCE}, {"Ok. As you wish."}},
}

function onCreatureSay(npc, creature, text, situation)
	behaviour.react(npc, rules, text, situation)
end

function onCreatureDisappear(npc, creature)
	behaviour.react(npc, rules, "", NPC_SITUATION_VANISH)
end
