-- npcs/faluae.lua: the behaviour of the NPC Faluae, converted from
-- faluae.npc by patella-convert. The NPC is its [[npc]] entry in npcs.toml;
-- lib/npc.lua matches and runs the rules below, in the order of the file
-- (scripting.hh, "NPCs"). The comments are those of faluae.npc.
--
-- GIMUD - Graphical Interface Multi User Dungeon
-- faluae.npc: Datenbank für die Deraisim-Anführerin Faluae (Elfenstadt)

local behaviour = require("lib.npc")
local ADDRESS = behaviour.ADDRESS
local BUSY = behaviour.BUSY
local VANISH = behaviour.VANISH
local PALADIN = behaviour.PALADIN
local DRUID = behaviour.DRUID
local FORCE = behaviour.FORCE
local eq = behaviour.eq
local lt = behaviour.lt
local TOPIC = behaviour.TOPIC
local PRICE = behaviour.PRICE
local TYPE = behaviour.TYPE
local LEVEL = behaviour.LEVEL
local COUNTMONEY = behaviour.COUNTMONEY
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
local deleteMoney = behaviour.deleteMoney
local idle = behaviour.idle
local nop = behaviour.nop
local previous = behaviour.previous

local rules = behaviour.rules {
	{{ADDRESS, "hello$", FORCE}, {"Greetings, traveller."}},
	{{ADDRESS, "hi$", FORCE}, {previous}},
	{{ADDRESS, "ashari$", FORCE}, {previous}},
	{{ADDRESS, FORCE}, {idle}},
	{{BUSY, "hello$", FORCE}, {"Sorry, I am busy now."}},
	{{BUSY, "hi$", FORCE}, {previous}},
	{{BUSY, "ashari$", FORCE}, {previous}},
	{{BUSY, FORCE}, {nop}},
	{{VANISH, FORCE}, {"Asha Thrazi, traveller."}},

	{{"bye"}, {"Asha Thrazi, traveller.", idle}},
	{{"farewell"}, {previous}},
	{{"asha", "thrazi"}, {previous}},
	{{"job"}, {"I am the spokesperson of the Deraisim caste."}},
	{{"name"}, {"I am known as Faluae Ethrathil."}},
	{{"time"}, {"Sorry, I don't know."}},

	{{"carlin"}, {"For a human city Carlin is not that bad. Its still a scar in the natural enviroment of course."}},
	{{"thais"}, {"Thais is fa away and that is a good thing."}},
	{{"venore"}, {"I don't appreciate that my people buy that much human wares but there is little I can do about it."}},
	{{"roderick"}, {"He is the thaian ambassador."}},
	{{"olrik"}, {"He is the assistant of the thaian ambassador."}},

	{{"king"}, {"I know nothing about kings."}},
	{{"elves"}, {"The elves are split in three castes: the Deraisim, the Kuridai, and the Cenath."}},
	{{"dwarfs"}, {"Funny little people sometimes. But their tunnels are harmful to the enviroment."}},
	{{"humans"}, {"I don't like their stone cities and their stench."}},
	{{"troll"}, {"I still can't stand the thought of the Kuridai keeping these disgusting creatures in our settlement."}},
	{{"army"}, {"I don't understand what you mean."}},
	{{"cenath"}, {"They think they are so wise but they lost the ability to adore the simple things."}},
	{{"kuridai"}, {"They are paranoid, what makes them so aggressive."}},
	{{"deraisim"}, {"We only stay here to keep our people together. We hunt for them, provide them with food, and scout the area."}},
	{{"abdaisim"}, {"Our lost brothers and sisters. Oh, we miss them so much."}},
	{{"teshial"}, {"I can only hope they will return one day."}},
	{{"ferumbras"}, {"I hope this fallen servant of evil will never find us."}},
	{{"crunor"}, {"I praise the Great Tree and Mother Earth, who gave birth to all life, and Nera, the celestial paladin."}},
	{{"nera"}, {"The lady of spring, helper of Crunor and Earth itself."}},
	{{"excalibug"}, {"What is that?"}},
	{{"news"}, {"Sorry, the only news I have concern the growth of plants and the coming and going of animal herds."}},

	{{DRUID, "rod", lt(questValue(333), 1)}, {"Oh, you did not purchase your first magical rod yet? Please take this little present from me as your magic teacher!", setQuestValue(333, 1), setType(3066), setAmount(1), create(TYPE)}},

	{{"magic"}, {"I learned a little about some minor spells."}},
	{{"druid"}, {"Druids are very close to Crunor."}},
	{{"sorcerer"}, {"They are so ... destructive."}},
	{{"spellbook"}, {"Sorry, I have none on me."}},
	{{"spell"}, {"I could teach you the spells of 'Light', 'Great Light', 'Food', and 'Find Person'."}},

	{{DRUID, "find", "person"}, {setType(20), setPrice(80), "Do you want to buy the spell 'Find Person' for %P gold?", setTopic(3)}},
	{{DRUID, "light"}, {setType(10), setPrice(100), "Do you want to buy the spell 'Light' for %P gold?", setTopic(3)}},
	{{DRUID, "food"}, {setType(42), setPrice(300), "Do you want to buy the spell 'Food' for %P gold?", setTopic(3)}},
	{{DRUID, "great", "light"}, {setType(11), setPrice(500), "Do you want to buy the spell 'Great Light' for %P gold?", setTopic(3)}},

	{{PALADIN, "find", "person"}, {setType(20), setPrice(80), "Do you want to buy the spell 'Find Person' for %P gold?", setTopic(3)}},
	{{PALADIN, "light"}, {setType(10), setPrice(100), "Do you want to buy the spell 'Light' for %P gold?", setTopic(3)}},
	{{PALADIN, "food"}, {setType(42), setPrice(300), "Do you want to buy the spell 'Food' for %P gold?", setTopic(3)}},
	{{PALADIN, "great", "light"}, {setType(11), setPrice(500), "Do you want to buy the spell 'Great Light' for %P gold?", setTopic(3)}},

	{{"find", "person"}, {"I'm sorry, but this spell is only for druids and paladins."}},
	{{"light"}, {previous}},
	{{"food"}, {previous}},
	{{"great", "light"}, {previous}},

	{{eq(TOPIC, 3), "yes", eq(spellKnown(TYPE), 1)}, {"You already know that spell."}},
	{{eq(TOPIC, 3), "yes", lt(LEVEL, spellLevel(TYPE))}, {setAmount(spellLevel(TYPE)), "You need to advance to level %A to learn this spell."}},
	{{eq(TOPIC, 3), "yes", lt(COUNTMONEY, PRICE)}, {"You do not have enough gold to pay my services."}},
	{{eq(TOPIC, 3), "yes"}, {"From now on you can cast this spell.", deleteMoney, effectOpp(13), teachSpell(TYPE)}},
	{{eq(TOPIC, 3)}, {"I thought so."}},
}

function onCreatureSay(npc, creature, text, situation)
	behaviour.react(npc, rules, text, situation)
end

function onCreatureDisappear(npc, creature)
	behaviour.react(npc, rules, "", NPC_SITUATION_VANISH)
end
